#include <stdio.h>
#include <stdlib.h>
#include <packet.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <fcntl.h>
#include <time.h>

#define TIMEOUT 2
#define TcpMaxDataRetransmissions 5

/*GO BACK-N Sender */
int main(int argc, char *argv[]) { //portNumber, CWND, Pr(loss), Pr(corruption)
	int server_sockfd, newsockfd, portno, window_size = 1, window_start_index = 0; //Default window Size
	double lossProbability = 0.0, corruptionProbability = 0.0; //Assume no loss/corruption by default
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buffer[PACKET_SIZE];
	bzero(buffer, PACKET_SIZE);
	struct timeval timeout={2,0}; //set timeout for 2 seconds
	packet* Packet = (packet*)buffer;
	time_t timer;
	int timedOutCount = 0; //Keep track of the number of consecutive timeouts

	if(argc != 2 && argc != 5) {
		fprintf(stderr, "ERROR, Invalid number of arguments\n");
		exit(1);
	}
	if(argc == 2) { //Only provided a port # assume no loss/corruption
	}
	if(argc == 5) { //Use selected values for CWND, loss, & corruption
		window_size = atoi(argv[2]);
		lossProbability = atof(argv[3]);
		corruptionProbability = atof(argv[4]);
	}
	
	//setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
	
	portno = atoi(argv[1]);
	printf("SERVER STARTED ON PORT: %d\n", portno);
	server_sockfd = socket(AF_INET, SOCK_DGRAM, 0); //Create UDP socket
		if(server_sockfd < 0)
			fprintf(stderr, "ERROR creating UDP socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
		if (bind(server_sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
			printf("ERROR on binding");
	clilen = sizeof(cli_addr);
	fcntl(server_sockfd, F_SETFL, O_NONBLOCK);

	LISTEN:
	while(1) {
		while (1) {
			while(1){
				ssize_t n = recvfrom(server_sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
				if(n != -1)
					break;
			}
			//Print the packet we received
			printPacket(Packet);
			if(Packet->packetType == SYN) {
				printf("Received new connection\n");
				createPacket(true, SYN_ACK, Packet->ACK_num, Packet->ACK_num+1, 0, (char *)"", Packet);
				int n = sendto(server_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&cli_addr, sizeof(cli_addr));
				if(n < 0) {
					printf("ERROR sending SYN ACK\nRetrying...\n");
				}
				else {
					printf("Sent SYN ACK: %d bytes\n", n);
				}
			}
			else {
				break;
			}
		}
		//We received file request
		//Print the packet we received
		printf("Received file request\n");
		printPacket(Packet);
		
		//Initial packet has a filename in Data section
		//Search for file in file system
		printf("Searching for requested file...\n");
		FILE* fp = fopen((const char*)Packet->data, "r");
		if(fp == NULL) {
			fprintf(stderr, "Failed to open requested file\n");
			//TODO notify client or just ignore?
			goto LISTEN; //Go back to listening state
		}

		//File Found, determine how many bytes it is
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		//printf("DEBUG: FileSize: %lu\n",fileSize);

		//Read file into memory (Not the best idea, but this is CS118)
		//Calculate number of packets needed
		unsigned long num_required_packets = std::max(1,(int)((fileSize/DATA_SIZE)+1));
		printf("DEBUG: Num_Required_Packets: %lu\n", num_required_packets);
		socklen_t serv_addr_size = sizeof(serv_addr);
		packet* sendQueue[num_required_packets];
		char* packetBuffers[num_required_packets];
		char* datas[num_required_packets];

		//Initialize every packet
		unsigned long tempSeqNum = Packet->seq_num;
		unsigned long tempACKNum = Packet->ACK_num;
		for(int index = 0; index < num_required_packets; index++) {
			packetBuffers[index] = (char *)calloc(PACKET_SIZE, sizeof(char)); //Dynamic allocated memory here-freed in fin section
			sendQueue[index] = (packet *)packetBuffers[index];
			int read = pread(fileno(fp), sendQueue[index]->data, (size_t)DATA_SIZE, index*(DATA_SIZE));
			sendQueue[index]->data[read+nullByte] = '\0';
			//printf("DEBUG: Bytes read from file %d\n", read);
			if(index == num_required_packets-1) //Last packet, we want to set lastPacket flag to true
				initializePackets(1, DATA, tempACKNum, tempACKNum+1, read, sendQueue[index]);
			else
				initializePackets(0, DATA, tempACKNum, tempACKNum+1, read, sendQueue[index]);
			tempSeqNum+=2;
			tempACKNum+=2;
		}

		//At this point, we have the entire to-be-sent packets in queue.
		unsigned long receivedUpTo = 3;
		long sentUpTo = -1; //To track which packets in the window should be sent
		bool timedOut = false; //To track if we need to resend all packets in window

		while(window_start_index < num_required_packets) {
			//Send every packet in our window that has yet to be sent; jump here upon timeouts
			resend_window:
			for(int index = window_start_index; index < window_start_index + window_size; index++) {
				time(&timer); //Set/reset timer for this window
				//Send packets in window that haven't been sent, or if timeout, resend all packets in window
				if(index >= num_required_packets) //We have less packets than our window size, so we can breakout early
					break;
				if (index > sentUpTo || timedOut == true) {
					int n = sendto(server_sockfd, packetBuffers[index], PACKET_SIZE, 0, (const sockaddr*)&cli_addr, sizeof(cli_addr));
					if(n < 0)
						printf("ERROR sending packet #: %d\n", index);
					else {
						printf("Sent: %d bytes\n", n);
						sentUpTo = index;				
					}
				}
			}
			if (timedOut == true) //resent window, reset timeout flag
				timedOut = false;
			//Wait to receive an ACK, or a timeout
			while(1) {
				//TODO IF NO RESPONSE IN # TIME, BREAK
				//printf("Waiting to receive ACK");
				//printf("time now: %ld, timer+timeout: %ld", time(NULL), timer+TIMEOUT);
				if (time(NULL) > timer + TIMEOUT && timedOut == false) {
					//std::cout << "timing out\n";
					printf("Timing out, resending window from packet num %d ", window_start_index);
					timedOut = true;
					timedOutCount++;
					if(timedOutCount >= TcpMaxDataRetransmissions) {
						bzero(buffer, PACKET_SIZE);
						window_start_index = 0;
						for (int i = 0; i < num_required_packets; i++) { //Free calloc'ed memory
							free(packetBuffers[i]);
						}
						fclose(fp);
						timedOutCount = 0;
						printf("\nMax retransmission reached...\n");
						printf("AWAITING REQUESTS ON PORT: %d\n", portno);
						goto LISTEN;
					}
					goto resend_window;
				}
				if (recvfrom(server_sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen) > 0) {
					if(random_prob() < lossProbability) {
						printf("ACK lost\n");
						continue;
					}
					if(random_prob() < corruptionProbability) {
						printf("ACK corrupted\n");
						continue;
					}
					else
						break;
				}
			}
			//Print the packet we received
			printPacket(Packet);
			if(Packet->packetType == FIN) {
				printf("FIN received... Transfer complete! :)\n\n");
				//TODO send FIN-ACK back?
				//go back to initial loop and listen for new requests
				printf("AWAITING REQUESTS ON PORT: %d\n", portno);			
				bzero(buffer, PACKET_SIZE);
				window_start_index = 0;
				for (int i = 0; i < num_required_packets; i++) { //Free calloc'ed memory
					free(packetBuffers[i]);
				}
				fclose(fp);
				timedOutCount = 0;
				goto LISTEN;
			}
			//Check if we need to move window start
			if((Packet->seq_num > receivedUpTo)) {
				window_start_index += (Packet->seq_num - receivedUpTo)/2;
				receivedUpTo = Packet->seq_num;
				timedOutCount = 0;
				//TODO if we move window start, refresh timer
				//timer is refreshed at the sending of first packet in window in next loop
				//time(&timer);
			}
			else {
				//TODO, do nothing?... maybe timer related things will go here
			}
		}
	}
}