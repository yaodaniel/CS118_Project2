#include <stdio.h>
#include <sys/time.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <netdb.h>      // define structures like hostent

#include <stdlib.h>

#include <strings.h>

#include <cstring>



/*GO BACK-N

1. Receiver drops packets not in sequence

2. ACKS server of the correctly in-order received SEQ #

3. Need at least CWND + 1 SEQ #s

4. Receiver checks for current SEQ # first before checking last packet flag

*/

/*

	----------------32 bits-------------------

	------------Final Packet Flag------------- 0

	-------------Sequence Number-------------- 1-32

	----------------ACK Number---------------- 33-64

	------------------DATA-------------------- 65-8256

	Size per packet = 32+32+1024*8+1 bits

	or 4+4+4+1024 bytes

*/

#define BUFSIZE 1036 //8257



void createPacket(int finalPacketFlag, int SeqNum, int ACKNum, char *data, char* buffer) {
	memcpy(buffer, &finalPacketFlag, sizeof(int));

	memcpy(buffer+sizeof(int), &SeqNum, sizeof(int));

	memcpy(buffer+sizeof(int)+sizeof(int), &ACKNum, sizeof(int));
	strncpy(buffer+12, data, strlen(data));

	//memcpy(buffer+12, &data, strlen(data));

}



int main(int argc, char *argv[]) {

	int receiver_sockfd; //Socket descriptor

	int portno, n;

	struct sockaddr_in serv_addr;

	struct hostent *server; //Contains info regarding server IP and others
	struct timeval timeout={2,0}; //set timeout for 2 seconds

	

	char buffer[BUFSIZE];
	bzero(buffer, BUFSIZE);

	int currentExpectedSeqNumber = 0;

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

	
	createPacket(1, 3, 1, argv[3], buffer);

	setsockopt(receiver_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

	socklen_t serv_addr_size = sizeof(serv_addr);
	while(1) { //initial requests
		int recvlen = recvfrom(receiver_sockfd, buffer, BUFSIZE, 0, (sockaddr*) &serv_addr, &serv_addr_size);
		if(recvlen >= 0) {
			//1st packet response received (contains data)
			break;
		}
		else {

			n = sendto(receiver_sockfd, buffer, BUFSIZE, 0, (const sockaddr*)&serv_addr, serv_addr_size);	
			if(n < 0)

				printf("ERROR in sendto");
			else
				printf("%d", n);
		}
	}
	//GO BACK-N STARTS
	



	





}