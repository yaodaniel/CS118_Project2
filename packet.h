#include <stdbool.h>
#include <cstring>
#include <string>
//Reason for this Packet Struct
//We use this to encapsulate our actual char[] Buffers.
//So we can more easily access/modify the buffer's values
//without using the buggy memcpy()...

/************Packet Struture************/
/*
	---------32 bits---------
	----Final Packet Flag---- 0
	----Sequence Number----- 1-32
	-----ACK Number------ 33-64
	-------DATA-------65-8256
	Size per packet = 32+32+1024*8+1 bits
	or 4+4+4+1024 bytes
	8257 bits
*/

#define SYN 0
#define SYN_ACK 1
#define ACK 2
#define FIN 3
#define FIN_ACK 4
#define REQUEST 5
#define DATA 6
//#define DATA_SIZE 1023
#define HEADER_SIZE (sizeof(bool) + sizeof(int) + 2*sizeof(unsigned long))
#define nullByte 1
#define PACKET_SIZE 1024 //(DATA_SIZE + nullByte + HEADER_SIZE)
#define DATA_SIZE (PACKET_SIZE - HEADER_SIZE - nullByte)
#define	bzero(ptr,n)		memset(ptr, '\0', n)
typedef struct{
	bool isFinalPacket; //0 is false, 1 is true
	int packetType; //0: SYN, 1:SYN-ACK, 2:ACK, 3:FIN, 4:FIN-ACK, 5:REQUEST
	unsigned long seq_num;
	unsigned long ACK_num;
	//unsigned long total_size; //Actual packet data size
	char data[DATA_SIZE + nullByte]; //Data section can be up to 1024 bytes including null byte
} packet;

//Print the values of the packet
//INPUT: A pointer to a packet
//OUTPUT: NONE, packet passed by reference.
void printPacket(packet* p) {
	if(p == NULL) {
		printf("Packet is Null");	
		return;
	}
	printf("--------PACKET INFORMATION:--------\n");
	printf("Packet Type: %d\n", p->packetType);
	printf("Sequence Number: %lu\n", p->seq_num);
	printf("ACK Number: %lu\n", p->ACK_num);
	printf(p->isFinalPacket ? "isFinalPacket: true\n" : "isFinalPacket: false\n");
	//printf("Total Size: %lu\n", p->total_size);
	printf("Data Length: %d\n", strlen(p->data));
	//printf("DEBUG: Data in Text: %s\n", p->data);
	printf("---------END INFORMATION:----------\n");
}

//Creates a valid packet with requested fields
//INPUT: Desired isFinalPacket Flag, packetType, seq_num, ACK_num, total_size, dataBuffer, packet.
//OUTPUT: NONE, packet passed in is by reference, its values will be updated to desired ones.
void createPacket(bool isFinalPacket, int type, unsigned long seqNum, unsigned long ACK_num,
			unsigned long total_size, char* dataBuffer, packet* inputPacket) {
	bzero((char *)inputPacket, PACKET_SIZE);
	inputPacket->packetType = type;
	inputPacket->isFinalPacket = isFinalPacket;
	inputPacket->seq_num = seqNum;
	inputPacket->ACK_num = ACK_num;
	//inputPacket->total_size = total_size;
	//memcpy(inputPacket->data, dataBuffer, DATA_SIZE);
	strncpy(inputPacket->data, dataBuffer, strlen(dataBuffer));
	//inputPacket->data = dataBuffer;
}
//Creates a valid packet with requested fields
//INPUT: Desired isFinalPacket Flag, packetType, seq_num, ACK_num, total_size, dataBuffer, packet.
//OUTPUT: NONE, packet passed in is by reference, its values will be updated to desired ones.
void createPacket2(bool wipeData, bool isFinalPacket, int type, unsigned long seqNum, unsigned long ACK_num,
			unsigned long total_size, char* dataBuffer, packet* inputPacket) {
	if(wipeData)
	bzero((char *)inputPacket, PACKET_SIZE);
	inputPacket->packetType = type;
	inputPacket->isFinalPacket = isFinalPacket;
	inputPacket->seq_num = seqNum;
	inputPacket->ACK_num = ACK_num;
	//inputPacket->total_size = total_size;
	//memcpy(inputPacket->data, dataBuffer, DATA_SIZE);
	//strncpy(inputPacket->data, dataBuffer, strlen(dataBuffer));
	//inputPacket->data = dataBuffer;
}
