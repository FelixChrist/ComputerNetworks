void DLLTX(uint8_t *netPacket, uint8_t length);
int DLLRX(uint8_t *frame, uint8_t length);
struct Buffer{
	uint8_t *pointToFrameStart;
	uint8_t seqNum;
	uint8_t length;
	//uint8_t key;
	struct Buffer *next;
};
struct Buffer DLLRXBuffer[0];
struct Buffer DLLTXBuffer[0];
uint8_t framesBuffer[0];
uint8_t globalSequenceNumber = 0;