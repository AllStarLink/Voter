/* "ClkSynth" -- A Clock Synthesizer which takes 10.0 MHz and
	generates a 14.4 MHz clock for driving Motorola radios.
    Jim Dixon, WB6NIL  Ver. 0.2 07/03/11
	This program is placed in the public domain, and may be used
	by anyone in any way for any legal purpose */

#include <pic.h>
#include <pic16f1823.h>
#include <htc.h>
#include <stddef.h>
#include <math.h>

__CONFIG(0x0EA4);

#define TESTBIT LATC5
#define STROBE LATC3
#define PDTS LATC4
#define EN1 LATA4
#define EN2 LATA5

#define	DESIRED_FREQ_KHZ 14400


#define XTAL2X (10000.0 * 2.0)

struct ee_map_s {
    unsigned long  l;
	unsigned char b[3];
};

#define EE_ADDR(member) (offsetof(struct ee_map_s, (member)))

void ee_read(unsigned char ee_addr, void *vp, unsigned char n)
{
    unsigned char *p = vp;

    while (n--) {
        *p++ = eeprom_read(ee_addr++);
    }
}

void ee_write(unsigned char ee_addr, void *vp, unsigned char n)
{
    unsigned char *p = vp;

    while (n--) {
        eeprom_write(ee_addr++, *p++);
    }
}

// Calculate the divisors and stuff for the synth chip
float ics_mkdiv(unsigned long khz,unsigned char *p)
{

float rv,fvdw,frdw,fvco,fmin,fr;
unsigned short s123,rdw,vdw;

	s123 = 0;
	fvco = khz * 10.0;
	if (khz > 15000)
	{
		s123 = 4;
		fvco = khz * 5.0;
	}
	if (khz > 30000)
	{
		s123 = 6;
		fvco = khz * 3.0;
	}
	if (khz > 60000)
	{
		s123 = 1;
		fvco = khz * 2.0;
	}
    fmin = 10000.0;
    rv = -999999999;
    for(rdw = 2; rdw <= 50; rdw++)
    {
		frdw = rdw + 2;
		for(vdw = 10; vdw <= 511; vdw++)
		{
			fvdw = vdw + 8;
			fr = XTAL2X * fvdw;
			fr /= frdw;
			TESTBIT ^= 1;
			if (fabs(fr - fvco) < fmin)
			{
				fmin = fabs(fr - fvco);
				p[0] = 0x10 | s123;
				p[1] = vdw >> 1;
				p[2] = rdw;
				if (vdw & 1) p[2] |= 0x80;
				rv = fr / 10.0;
            }
    	}
	}
	return(rv);
}

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
unsigned long khz,l;
unsigned char b[3];
float f;

	OSCCON = 0xf2;  // Crank CPU up to full speed
	ANSELA = 0;
	TRISA = 0; 
	TRISC = 0;
	LATA = 0x30;
	LATC = 0;
	SSP1CON1 = 2;
	SSP1STAT = 0x40;
	SSP1IF = 0;
	BF = 0;
	STROBE = 0;
	khz = DESIRED_FREQ_KHZ;
    ee_read(EE_ADDR(l), &l, sizeof l);
    ee_read(EE_ADDR(b), &b, sizeof b);
	if (l != khz)
	{
		f = ics_mkdiv(khz,b);
    	ee_write(EE_ADDR(l), &khz, sizeof khz);
    	ee_write(EE_ADDR(b), &b, sizeof b);
	}
	OSCCON = 0x38; // Let CPU run at 500 KHz
	PDTS = 1;
	SSP1CON1 |= 0x20;
	write_spi(0x00);  // write extra byte of 0 to clear poo out of register
	write_spi(b[0]);
	write_spi(b[1]);
	write_spi(b[2]);
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
	}
}

