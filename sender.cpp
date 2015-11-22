#include <stdio.h>

#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h

#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr

#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in

#include <stdlib.h>
#include <cstring>

#include <string>

#include <strings.h>

#include <sys/wait.h>	/* for the waitpid() system call */

#include <signal.h>	/* signal name macros, and the kill() prototype */





/*

	----------------32 bits-------------------

	------------Final Packet Flag------------- 0

	-------------Sequence Number-------------- 1-32

	----------------ACK Number---------------- 33-64

	------------------DATA-------------------- 65-8256

	Size per packet = 32+32+1024*8+1 bits

	or 4+4+4+1024 bytes

*/

#define BUFSIZE 1036



void error(char *msg) {

	perror(msg);

	exit(1);

}



int main(int argc, char *argv[]) { //portNumber, CWND, Pr(loss), Pr(corruption)

	int server_sockfd, newsockfd, portno, pid;

	socklen_t clilen;

	struct sockaddr_in serv_addr, cli_addr;

	char buffer[BUFSIZE];

	

	int window_size = 1; //Default window Size

	double lossProbability = 0.0, corruptionProbability = 0.0; //Assume no loss/corruption by default



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

	portno = atoi(argv[1]);
	printf("SANITY CHECK: %d\n", portno);

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

	

	while(1) {

		//bzero(buffer, BUFSIZE);
		ssize_t blah = recvfrom(server_sockfd, buffer, BUFSIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

		if(blah < 0)

			fprintf(stderr, "ERROR Receiving %d", blah);
		else if (blah == NULL)
			fprintf(stderr, "NULL ");
		else if (blah == 1036)
			fprintf(stderr, "IT's 1036!\n");
		else {
			fprintf(stderr, "SoUhhhhhh");		
			printf("blah: %lu", (unsigned long) blah);
			printf("blah: %zu", blah);
		}
		
		int packetflag = -1;
		memcpy(&packetflag, buffer, sizeof(int));
		printf("packetflag: %d\n", packetflag);
		int seqNum = -1;
		memcpy(&seqNum, buffer+sizeof(int), sizeof(int));
		printf("seqNum: %d\n", seqNum);
		int ACKNum = -1;
		memcpy(&ACKNum, buffer+8, sizeof(int));
		printf("ACKNum:%d\n", seqNum, ACKNum);

		char tests[BUFSIZE-12];
		bzero(tests,BUFSIZE-12);
		strncpy(tests, buffer+12, 5);

		printf("FileName: %s\n", tests);

		/*for(int index = 0; index < BUFSIZE; index++)

			printf("%c", buffer[index]);

		printf("\n");*/

	}

	

	//Search for file in file system
	int packetflag = -1;
	memcpy(&packetflag, buffer, sizeof(int));
	printf("%d", packetflag);
	int seqNum = -1;
	memcpy(&seqNum, buffer+sizeof(int), sizeof(int));
	int ACKNum = -1;
	memcpy(&ACKNum, buffer+8, sizeof(int));
	printf("seqNum:%d ACKNum:%d\n", seqNum, ACKNum);
	
	char data[BUFSIZE-12]; //Contains actual data
	bzero(data,BUFSIZE-12);
	strncpy(data, buffer+12, BUFSIZE-12);
	printf("\n%s", data);


	//Break it down if needed

	//append headers to packets

		//1. updated ACK number and SEQ number

		//2. append the actual data

	//send all packets back to client

	

	

	

	

	

	

	

	

	

	

	

	

	

	

}