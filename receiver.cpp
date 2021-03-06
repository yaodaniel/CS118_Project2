#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <packet.h>

/*GO BACK-N 
** Receiver **
1. Doesn't need to keep track of a window
2. Receiver drops all packets not received in sequence
3. ACKS server of the correctly in-order received SEQ #
4. Receiver checks for current SEQ # first before checking last packet flag
** Sender **
1. Sequence # starts at 1
2. Need at least CWND + 1 SEQ #s
3. 1 timer for entire CWND
4. Sends all packets in CWND.
5. When an appropriate ACK is received, increment window
*/

int main(int argc, char *argv[]) { //hostAddress, hostPort, requestedFile, Pr(loss), Pr(corruption)
	srand(time(0)); //Seed random number generator
	int receiver_sockfd; //Socket descriptor
	int portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server; //Contains info regarding server IP and others
	struct timeval timeout={5,0}; //set timeout for 5 seconds
	packet* Packet;
	char* buffer = (char *)calloc(PACKET_SIZE, sizeof(char));
	unsigned long currentExpectedSeqNumber = 4;
	double lossProbability = 0.0, corruptionProbability = 0.0; //Assume no loss/corruption by default

	if(argc != 4 && argc != 6) {
		fprintf(stderr,"Invalid number of arguments");
		exit(0);
	}

	if(argc == 4) {
	}

	if(argc == 6) {
		lossProbability = atof(argv[4]);
		corruptionProbability = atof(argv[5]);
	}

	portno = atoi(argv[2]);
	receiver_sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
	if(receiver_sockfd < 0)
		printf("ERROR opening socket");
	server = gethostbyname(argv[1]);

	if(server == NULL) {
		fprintf(stderr, "ERROR no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //initialize server's address
    	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    	serv_addr.sin_port = htons(portno);
	//We use the packet struct to modify buffer's values
	Packet = (packet*)buffer;
	setsockopt(receiver_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
	//fcntl(receiver_sockfd, F_SETFL, O_NONBLOCK);
	socklen_t serv_addr_size = sizeof(serv_addr);

	//SIMPLIFIED THREE WAY HANDSHAKE
	while(1) { //initial request
		//STEP 1
			createPacket(true, SYN, 1, 2, 0, (char *)"", Packet);
			n = sendto(receiver_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);	
			if(n < 0)
				printf("ERROR in sendto\n");
			else
				printf("Sent SYN: %d bytes\n", n);
		while(1) {
			//TODO IF NO RESPONSE IN # TIME, BREAK
			n = recvfrom(receiver_sockfd, buffer, PACKET_SIZE, 0, (sockaddr*) &serv_addr, &serv_addr_size);
			if(n != -1)
				break;
		}
		//1st packet response received (Should be server's ACK)
		printf("Received the following packet\n");
		printPacket(Packet);
		//STEP 3
		if(Packet->packetType == SYN_ACK) {
			printf("Received Packet is SYN-ACK\n");
			createPacket(true, REQUEST, Packet->ACK_num, Packet->ACK_num+1, strlen(argv[3]), argv[3], Packet);
			n = sendto(receiver_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);
			if(n < 0) {
				printf("ERROR sending file request for %s, Retrying...\n", argv[3]);
				exit(0);
			}
			else {
				printf("Sent file request for: %s, total %d bytes\n", argv[3], n);
				break;
			}
		}
	}

	//GO BACK-N FILE TRANSFER STARTS
	while(1) {
		//while(1) {
			//TODO IF NO RESPONSE IN # TIME, BREAK
			n = recvfrom(receiver_sockfd, buffer, PACKET_SIZE, 0, (sockaddr*) &serv_addr, &serv_addr_size);
			//if(n != -1)
				//break;
		//}
		if(n == -1) {
			printf("Socket timed out, assuming sender dc-ed, exiting...\n");
			free(buffer);
			exit(1);
		}
		if(random_prob() < lossProbability) {
			printf("Packet was lost.\n");
			continue;
		}
		if(random_prob() < corruptionProbability) {
			printf("Packet corrupted.\n");
			continue;
		}
		if(n >= 0) {
			printf("Received following packet with : %zd bytes\n", n);
			printPacket(Packet);
			printf("DEBUG: Current Expected Sequence Number: %lu\n", currentExpectedSeqNumber);
			if(Packet->seq_num == currentExpectedSeqNumber) { //Received the packet we're expecting
				std::string requestedFile(argv[3]);
				std::string extension = requestedFile.substr(requestedFile.find(".")); //Extract file extension
				std::string destinationFile = "receivedFile"+extension;
				FILE *f = fopen(destinationFile.c_str(), "a"); //Open file for appending
					if (f == NULL) {
					    printf("Error writing to file! Exiting...\n");
						//TODO Disconnection w/ server OR retry saving
					    exit(1);
					}
					printf("Writing packet to file...\n");
					fwrite(Packet->data, sizeof(char), Packet->total_size, f);
					fclose(f);
					printf("Packet written!\n");
				if(Packet->isFinalPacket) {
					printf("Everything Received!...\n");
					//TODO DO FIN stuff here
					createPacket(true, FIN, Packet->ACK_num, Packet->ACK_num+1, 0, (char *)"", Packet);
					n = sendto(receiver_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);
					if(n < 0) {
						printf("ERROR sending FIN\n");
					}
					else {
						printf("FIN Sent: %d bytes\n", n); 
						free(buffer);
						break; //We're not waiting for server to respond to this FIN
					}
				}
				else { //ACK for current received packet.
					createPacket(true, ACK, Packet->ACK_num, Packet->ACK_num+1, 0, (char *)"", Packet);
					n = sendto(receiver_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);
					if(n < 0)
						printf("ERROR sending ACK\n");
					else {
						printf("Sent: %d bytes\n", n);
						currentExpectedSeqNumber = Packet->seq_num+1;
						//printf("DEBUG: currentExpectedSeqNumber: %lu\n", currentExpectedSeqNumber);
					}
				}
			}
			else {
				//Received Packet should be ignored because it is out of ordered/lost/corrupted
				//Continue listening for other packets
				createPacket(true, ACK, currentExpectedSeqNumber-1, currentExpectedSeqNumber, 0, (char *)"", Packet);
				n = sendto(receiver_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);
				if(n < 0)
					printf("ERROR sending ACK\n");
				else {
					printf("Sent: %d bytes\n", n);
				}
				continue;
			}
		}
		else { //Unexpected results, Resend current packet (either initial file request or ACKs)
			n = sendto(receiver_sockfd, buffer, PACKET_SIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);	
			if(n < 0)
				printf("ERROR in sendto\n");
			else
				printf("Sent: %d bytes\n", n);
		}
	}
}