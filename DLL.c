#include <inttypes.h>
//#include <avr/io.h>
#include <stdio.h>
#define DEBUG 
#define VERSION 0
void DLLTX(uint8_t *netPacket, uint8_t length, uint8_t acknowledge);
void DLLRX(uint8_t *frame, uint8_t length);
struct Buffer{
	uint8_t *frames[];
	uint8_t seqNum;
};
struct Buffer DLLRXBuffer[0];
struct Buffer DLLTXBuffer[0];

uint8_t globalSequenceNumber = 0;
int main(){
	int i = 0;
	printf("Hello world!\n");
	uint8_t netPacket[60];

	for(int i=0; i<sizeof(netPacket);i++){
		netPacket[i]=2*i;
	}
	DLLTX(netPacket, sizeof(netPacket), 2);
	return 0;
}

/* 

To be called by the Network layer to initiate a transmit

*/

void DLLTX(uint8_t *netPacket, uint8_t length, uint8_t acknowledge){ //pass in a pointer the packet and the length of the array
	uint8_t i = 0, j = 0, count = 0, footer = 0xAA,sequenceNumber=globalSequenceNumber, numberOfFrames, TTL = 15;
	globalSequenceNumber++;
	if(globalSequenceNumber == 15){
		globalSequenceNumber=0;
	}

	numberOfFrames=(length/23)+1; //Calculate number of frames with each frame being maximum size
	uint8_t header[numberOfFrames], control0[numberOfFrames],control1[numberOfFrames],frameLength[numberOfFrames],checksum0[numberOfFrames],checksum1[numberOfFrames];
	uint8_t tempFrame2[32*numberOfFrames];
	/*
	Take the addresses from the network layer
	*/
	netPacket+=2; //Go past the control bytes
	uint8_t srcAddress=*netPacket; //Take SRC address byte
	netPacket++; //Go to next byte
	uint8_t destAddress=*netPacket; //Take DEST address byte
	netPacket-=3; //Go back to start of packet

	uint8_t *netPacketSplit[numberOfFrames]; //Array to be filled with pointers to each piece of net packet data
	uint8_t *frame[numberOfFrames];

	/*
	This splits the net packet into frames
	*/

	for(i=0; i<numberOfFrames; i++){
		uint8_t tempFrame[23*numberOfFrames]; //Declares a temporary frame to fill
		for(j=0; j<23; j++){
			tempFrame[j+i*23] = *(netPacket+(j+23*i)); //Sets the element of the frame to the current value of the net packet
			#ifdef DEBUG
			printf("Value at the netPacket %d is %d\n",i,tempFrame[j+i*23] );
			#endif
			//netPacket ++; //Increments the Net Packet pointer to move along the array 
			count++; //Counts each time the array is incremented
			if(count==length){
				break; //When the array has been incremented as many times as its length, break from the loop. This should happen for the final frame only
			}
		}
		uint8_t * pointer;
		pointer = tempFrame+i*23;
		netPacketSplit[i] = pointer; //Puts a pointer to the start of the net packet data for each frame in an array 
	}

	#ifdef DEBUG
	count = 0;
	for(i=0;i<numberOfFrames;i++){
		for(j=0; j<23; j++){
			printf("Frame %d part %d = %u\n",i+1,j+1,*(netPacketSplit[i]+(uint8_t)j));
			
			count++;
			if(count==length){
				break; //When the array has been incremented as many times as its length, break from the loop. This should happen for the final frame only
			}

		}

	}
	#endif

	/* 
	This sets the header byte for each frame
	*/


	for(int i=0; i<numberOfFrames; i++){
		header[i]=0;
		header[i] = (header[i] & 63) | VERSION << 6; //Puts the version number in the 2 most significant bits
		header[i] = (header[i] & 199) | i << 3; //Puts the frame number in the middle 3 bits
		header[i] = (header[i] & 248) | numberOfFrames; //Puts the max frame number in the 3 least significant bits
		#ifdef DEBUG
		printf("Header byte %d = %d\n",i,header[i] );
		#endif
	}


	/*
	This sets the control bytes for each frame
	*/

	for(int i=0; i<numberOfFrames; i++){
		/* ACKS CURRENTLY NOT IMPLEMENTED*/
		control1[i]=acknowledge; 
		control0[i]=0;
		control0[i]= (control0[i] & 15) | TTL << 4; //Sets the 4 most significant bits to the TTL
		control0[i]= (control0[i] & 240) | sequenceNumber; //Sets the 4 least significant bits to the sequence number
		#ifdef DEBUG
		printf("First control byte %d = %d\n",i,control0[i]);
		printf("Second control byte %d = %d\n",i,control1[i]);
		#endif
	}
	/*
	This sets the length for each frame
	*/

	for(int i=0; i<numberOfFrames; i++){
		if(i+1!=numberOfFrames){
			frameLength[i] = 32; //Length for all except last frame is 32
		}
		else{
			frameLength[i] = (length%23)+9; //Last frame measures the number of net packet bytes in the frame and adds it to the rest of the bits
		}
		
		#ifdef DEBUG
		printf("Length byte %d = %d\n",i,frameLength[i] );
		#endif
	}
	/* 
	This calculates the checksum of each frame
	*/

	switch(VERSION){ //Picks the checksum type based on version
		case 0:
		for(i=0; i<numberOfFrames; i++){
			checksum1[i]=0; //The second checksum byte is unused
			checksum0[i] = header[i] ^ srcAddress ^ destAddress ^frameLength[i] ^ footer; //Gets the XOR of bytes not in the net packet
			for(j=0; j<frameLength[i]-9;j++){
				checksum0[i] = checksum0[i] ^ *(netPacketSplit[i]+j+i*23); //Gets the XOR of bytes in the netpacket
				
			}
			
			#ifdef DEBUG
			printf("First Checksum byte %d = %d\n",i,checksum0[i]);
			printf("Second Checksum byte %d = %d\n",i, checksum1[i]);
			#endif
		}

	}

	/*
	This combines each section of the frame
	*/
	
	uint8_t * pointer;

	for(i=0; i<numberOfFrames; i++){
		
		tempFrame2[0+i*32] = header[i]; //Add header byte
		tempFrame2[1+i*32] = control1[i]; //Add first control byte
		tempFrame2[2+i*32] = control0[i]; //Add second control byte
		tempFrame2[3+i*32] = destAddress; //Add dest address 
		tempFrame2[4+i*32] = srcAddress; //Add scr Address
		tempFrame2[5+i*32] = frameLength[i]; //Add length
		for(j=0; j<frameLength[i]-9;j++){
			tempFrame2[j+6+i*32] = *(netPacketSplit[i]+(uint8_t)j); //Add each section of the net packet
		}
		tempFrame2[frameLength[i]-3+i*32]=checksum0[i]; //Add first checksum byte
		tempFrame2[frameLength[i]-2+i*32]=checksum1[i];  //Add second checksum byte
		tempFrame2[frameLength[i]-1+i*32]=footer; //Add footer byte
		pointer = tempFrame2+i*32;
		frame[i] = pointer; //Store pointer to frame in array
	}
	#ifdef DEBUG
	for(i=0; i<numberOfFrames; i++){
		for(j=0; j<frameLength[i]; j++){
			printf("Frame number %d byte number %d = %u\n",i,j,*(frame[i]+j));
			
		}
		
	}
	#endif


}

void DLLRX(uint8_t *frame, uint8_t length){
	uint8_t i = 0;
	uint8_t netPacket[*(frame+5)-9];
	if(*(frame + 1)==0){
		for(i=0;i<(*(frame+5))-9;i++){
			netPacket[i]=*(frame+6+i);
		}
	}
}