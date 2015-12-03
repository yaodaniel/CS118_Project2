#include <cstring>
#include <string>
//Reason for this Packet Struct
//We use this to encapsulate our actual char[] Buffers.
//So we can more easily access/modify the buffer's values
//without using the buggy memcpy()...

/************Packet Struture************/
/*
	---------32 bits---------
	--------Packet Type------- 0-3
	-----Final Packet Flag----- 4-7
	------Sequence Numbe------- 8-11
	---------ACK Number-------- 12-15
	--------Data_Size--------- 16-19
	----------DATA---------20-1023 + nullByte
	Size per packet = 1024 bytes
	8192 bits
*/

#define SYN 0
#define SYN_ACK 1
#define ACK 2
#define FIN 3
#define FIN_ACK 4
#define REQUEST 5
#define DATA 6
#define HEADER_SIZE (2*sizeof(int) + 3*sizeof(unsigned long))
#define nullByte 1
#define PACKET_SIZE 1024 //(DATA_SIZE + nullByte + HEADER_SIZE)
#define DATA_SIZE (PACKET_SIZE - HEADER_SIZE - nullByte)
#define	bzero(ptr,n)		memset(ptr, '\0', n)
typedef struct{
	int packetType; //0: SYN, 1:SYN-ACK, 2:ACK, 3:FIN, 4:FIN-ACK, 5:REQUEST, 6:DATA
	int isFinalPacket; //0 is false, 1 is true
	unsigned long seq_num;
	unsigned long ACK_num;
	unsigned long total_size; //Actual packet data size not including nullByte, used by createPacket()
	char data[DATA_SIZE + nullByte];
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
	printf("Total Data Size: %lu\n", p->total_size);
	printf("---------END INFORMATION:----------\n");
}

//Creates a valid packet with requested fields
//This function will wipe input packet's data.
//INPUT: Desired isFinalPacket Flag, packetType, seq_num, ACK_num, total_size, dataBuffer, packet.
//OUTPUT: NONE, packet passed in is by reference, its values will be updated to desired ones.
void createPacket(bool isFinalPacket, int type, unsigned long seqNum, unsigned long ACK_num,
			unsigned long total_size, char* dataBuffer, packet* inputPacket) {
	bzero((char *)inputPacket, PACKET_SIZE);
	inputPacket->packetType = type;
	inputPacket->isFinalPacket = isFinalPacket;
	inputPacket->seq_num = seqNum;
	inputPacket->ACK_num = ACK_num;
	inputPacket->total_size = total_size;
	memcpy(inputPacket->data, dataBuffer, total_size);
}
//Used by sender to initialize all packets needed to be sent
//Each packet's data is already set within sender
//INPUT: Desired isFinalPacket Flag, packetType, seq_num, ACK_num, total_size, packet.
//OUTPUT: NONE, packet passed in is by reference, its values will be updated to desired ones.
void initializePackets(int isFinalPacket, int type, unsigned long seqNum, unsigned long ACK_num,
			unsigned long total_size, packet* inputPacket) {
	inputPacket->packetType = type;
	inputPacket->isFinalPacket = isFinalPacket;
	inputPacket->seq_num = seqNum;
	inputPacket->ACK_num = ACK_num;
	inputPacket->total_size = total_size;
}
double random_prob() {
	return (rand() % 100)/100.0;
}