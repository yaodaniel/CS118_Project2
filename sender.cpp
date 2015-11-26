#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <packet.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>

/*GO BACK-N 
** Receiver **
1. Doesn't need to keep track of a window
2. Receiver drops all packets not received in sequence
3. ACKS server of the correctly in-order received SEQ #
4. Receiver checks for current SEQ # first before checking last packet flag

** Sender **
1. Need at least CWND + 1 SEQ #s
2. 1 timer for entire CWND
3. Sends all packets in CWND.
4. When an appropriate ACK is received, increment window
*/
int main(int argc, char *argv[]) { //portNumber, CWND, Pr(loss), Pr(corruption)
	int server_sockfd, newsockfd, portno;
	int window_size = 1, window_start_index = 0; //Default window Size
	double lossProbability = 0.0, corruptionProbability = 0.0; //Assume no loss/corruption by default
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buffer[PACKET_SIZE];
	bzero(buffer, PACKET_SIZE);
	struct timeval timeout={2,0}; //set timeout for 2 seconds
	packet* Packet = (packet*)buffer;
	
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
	setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
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
	LISTEN: while(1) {
		//bzero(buffer, PACKET_SIZE);
		ssize_t blah = recvfrom(server_sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		if(blah < 0)
			fprintf(stderr, "ERROR Receiving %d", blah);
		printPacket(Packet);
		//Packet = (packet*)buffer;
		if(Packet->packetType == SYN) { //strcmp(Packet->data, "SYN") == 0) {
			createPacket(true, SYN_ACK, Packet->ACK_num, Packet->ACK_num+1, 0, (char *)"", Packet);
			int n = sendto(server_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&cli_addr, sizeof(cli_addr));
				if(n < 0) {
					printf("ERROR sending SYN ACK\nRetrying...\n");
				}
				else
					printf("Sent SYN ACK: %d bytes\n", n);
		}
		else { //We recieved file request
			break;
		}
	}
	//Initial packet has a filename in Data section	
	//Search for file in file system
	printf("Searching for requested file...\n");
	FILE* fp = fopen((const char*)Packet->data, "r");
	if(fp == NULL) {
		fprintf(stderr, "Failed to open requested file\n");
		goto LISTEN; //Go back to listening state
	}

	//File Found, determine how many bytes it is
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	//printf("DEBUG: FileSize: %lu\n",fileSize);

	//Read file into memory (Not the best idea, but this is CS118)
	/*char fileData[fileSize];
  	fread(fileData, fileSize, 1, fp);
  	fclose(fp);
	printf("FILE read into memory\n");*/

	//Calculate number of packets needed
	unsigned long num_required_packets = std::max(1,(int)((fileSize/DATA_SIZE)+1));
	printf("Num_Required_Packets: %lu\n", num_required_packets);
	socklen_t serv_addr_size = sizeof(serv_addr);
	packet* sendQueue[num_required_packets];
	char* packetBuffers[num_required_packets];
	char* datas[num_required_packets];

	//TODO initialize sendQueue with values
	unsigned long tempSeqNum = Packet->seq_num;
	unsigned long tempACKNum = Packet->ACK_num;
	for(int index = 0; index < num_required_packets; index++) {
		packetBuffers[index] = (char *)calloc(PACKET_SIZE, sizeof(char));
		//bzero(packetBuffers[index], sizeof(char)*PACKET_SIZE);
		sendQueue[index] = (packet *)packetBuffers[index];

		//datas[index] = (char *)malloc(sizeof(char)*DATA_SIZE+1);
		//bzero(datas[index], DATA_SIZE);
		//pread(fileno(fp), datas[index], DATA_SIZE, index*DATA_SIZE);
		//char* die = (char*) calloc(DATA_SIZE+1, sizeof(char));

		int read = pread(fileno(fp), sendQueue[index]->data, (size_t)DATA_SIZE-nullByte-1, index*(DATA_SIZE-nullByte-1));
		sendQueue[index]->data[DATA_SIZE+nullByte] = '\0';
		//printf("PEICE OF SHIT: %s\n", sendQueue[0]->data);
		//printf("datasLength: %d\n", strlen(datas[index]));
		printf("Read %d\n", read);
		if(index == num_required_packets-1)
			createPacket2(false, true, DATA, tempACKNum, tempACKNum+1, 0, NULL, sendQueue[index]);
		else
			createPacket2(false, false, DATA, tempACKNum, tempACKNum+1, 0, NULL, sendQueue[index]);
		tempSeqNum+=2;
		tempACKNum+=2;
	}
	//printf("blah blah blah: %s\n", sendQueue[0]->data);
	//exit(0);
	//At this point, we have the entire to-be-sent packets in queue.
	unsigned long receivedUpTo = 3;
	while(window_start_index < num_required_packets) {
		//Send every packet in our window
		for(int index = window_start_index; index < window_start_index+window_size; index++) {
			/*printf("SendQueue[index]->data: %s\n", sendQueue[index]->data);
			printf("SendQueue[index]->data length: %d\n", strlen(sendQueue[index]->data));*/
			int n = sendto(server_sockfd, packetBuffers[index], PACKET_SIZE, 0, (const sockaddr*)&cli_addr, sizeof(cli_addr));
			/*printf("packetBuffers[0][1036]: %c\n", packetBuffers[0][1036]);			
			printf("packetBuffers[0][1037]: %c\n", packetBuffers[0][1037]);			
			printf("packetBuffers[0][1038]: %c\n", packetBuffers[0][1038]);
			printf("packetBuffers[0][1039]: %c\n", packetBuffers[0][1039]);*/

			if(n < 0)
				printf("ERROR sending packet: %d\n", index);
			else
				printf("Sent: %d bytes\n", n);
		}

		while(1) {
			//NO RESPONSE IN # TIME, BREAK
			int n = recvfrom(server_sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
			if(n != -1)
				break;
		}

		printPacket(Packet);
		if(Packet->packetType == FIN)
			exit(0);

		if((Packet->seq_num > receivedUpTo)) {
			window_start_index += (Packet->seq_num - receivedUpTo)/2;
			receivedUpTo = Packet->seq_num;
			//Check if we need to move window start
			//if we move window start, refresh timer,
		}
		else {
		}
	}
}