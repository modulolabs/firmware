/*
 * IR2.h
 *
 * Created: 9/12/2015 10:37:36 AM
 *  Author: ekt
 */ 


#ifndef IR2_H_
#define IR2_H_



void IR2ReceiveEnable();
void IR2ReceiveDisable();

bool IR2IsReceiveComplete();
bool IR2Decode(int8_t *protocol, uint32_t *value);

#define IR_BUFFER_SIZE 128   // Max size of _irReadBuffer

extern volatile uint8_t irRawBuffer[IR_BUFFER_SIZE];
extern volatile uint8_t irRawLen;


#endif /* IR2_H_ */