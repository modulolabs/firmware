/*
 * IR.h
 *
 * Created: 10/21/2015 2:30:59 PM
 *  Author: ekt
 */ 


#ifndef IR3_H_
#define IR3_H_


#define IR_BUFFER_SIZE 96

#define IR_MAX_LENGTH 253
#define IR_TOKEN_END 254
#define IR_TOKEN_START 255

void IRInit();

// Return whether IR is idle and therefore ready to send.
// It's idle when not sending or receiving and no signal has
// been seen for a minimum amount of time.
bool IRIsIdle();

// Send IR data. This copies the data to the send buffer and returns,
// so the transmission will begin on the next timer interrupt. 
void IRSend(uint8_t *data, uint8_t len);

// Return true if there is currently a signal begin sent or detected
// Use to flash the status led during activity.
bool IRGetActivity();


// Set the minimum amount of time (in 50us ticks) between transmissions
void IRSetBreakLength(uint16_t len);

#endif /* IR_H_ */