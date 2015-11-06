/*
 * Trampoline.cpp
 *
 * Created: 11/2/2015 12:08:58 PM
 *  Author: ekt
 */ 

void startApplication() __attribute__((section (".trampoline"))) __attribute__((naked));
void startApplication() {
	asm("rjmp __ctors_start");
}
