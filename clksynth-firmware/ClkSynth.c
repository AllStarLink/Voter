/* "ClkSynth" -- A Clock Synthesizer which takes 10.0 MHz and
	generates a 14.4 MHz clock for driving Motorola radios.
    Jim Dixon, WB6NIL  Ver. 0.1 06/30/11
	This program is placed in the public domain, and may be used
	by anyone in any way for any legal purpose */

#include <pic.h>
#include <pic16f1823.h>

__CONFIG(0x0EB4);

#define TESTBIT LATC5
#define STROBE LATC3
#define PDTS LATC4
#define EN1 LATA4
#define EN2 LATA5


/* write a byte to SPI */
void write_spi(unsigned char val)
{
unsigned char dummy;

		while(BF);
		SSP1BUF = val;
		while(!SSP1IF);
		dummy = SSP1BUF;
		SSP1IF = 0;
}

void main(void) {
unsigned short i;

	CLRWDT();
	TRISA = 0; 
	TRISC = 0;
	LATA = 0x30;
	LATC = 0;
	SSP1CON1 = 2;
	SSP1STAT = 0x40;
	SSP1IF = 0;
	BF = 0;
	STROBE = 0;
	PDTS = 1;
	SSP1CON1 |= 0x20;
	write_spi(0x00);  // write extra byte of 0 to clear poo out of register
	write_spi(0x10);
	write_spi(0x0e);
	write_spi(0x03);
	SSP1CON1 &= ~0x20;
	NOP();
	NOP();
	STROBE = 1;
	NOP();
	NOP();
	STROBE = 0;
	for(i = 0; i < 100; i++) NOP();  /* delay at least 10ms for synth to lock */
	EN1 = 0;
	EN2 = 0;
	while(1) {
		TESTBIT ^= 1;
		CLRWDT();
	}
}

