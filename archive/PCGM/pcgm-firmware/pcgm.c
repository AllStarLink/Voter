/* "PCGM" -- A Programmable Clock Generator Module which takes 10.0 MHz and
	generates a clock for driving Digitally Synthesized radios.
    Jim Dixon, WB6NIL  Ver. 0.4 12/28/11
	This program is placed in the public domain, and may be used
	by anyone in any way for any legal purpose */

/* This environment (and therfore this implementation) is *RATHER SWINE* in
   many respects. First of all, the PIC (we picked... pun intended) was
   BLAZINGLY NEW at the time, and did not have proper development support,
   much less having working development support. There were silicon bugs. There
   were IDE bugs. There were compiler bugs. There were horrible library bugs.
   There were Tarrantulas (well, this *is* Coarsegold, CA, after all).

   Some rather UGLY and QUITE non-standard things had to be done here to make
   stuff work, so if you see anything that makes you wonder "why in the heck
   was this done this way", you will understand why. 

   This program now works with the latest current version of the C30 compiler
   (v3.30c), along with previous versions.

*/


#include <libpic30.h>
#include <stdio.h>
#include <stdlib.h>
#include <p24FV32KA301.h>
#include <stddef.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "GenericTypeDefs.h"
#include "UART.h"

extern int __C30_UART;

#define GetPeripheralClock() (16000000ul)
#define	BAUD_RATE1 57600
#define	LED_COUNT 50000


#define TESTBIT LATAbits.LATA4
#define LED LATBbits.LATB4
#define STROBE LATAbits.LATA2
#define PDTS LATBbits.LATB15
#define EN1 LATBbits.LATB8
#define EN2 LATBbits.LATB9

#define	DESIRED_FREQ_KHZ 14400

#define XTAL2X (10000.0 * 2.0)

_FBS( BWRP_OFF & BSS_OFF )
_FGS( GSS0_OFF & GWRP_OFF )
_FOSCSEL( FNOSC_FRCPLL & IESO_OFF & SOSCSRC_DIG)
_FOSC( FCKSM_CSECMD & OSCIOFNC_ON & POSCMOD_NONE  )
_FPOR( BOREN_BOR3 & LVRCFG_OFF & PWRTEN_OFF & BORV_V27 & MCLRE_ON)	
_FICD( ICS_PGx2 )

BOOL aborted;
DWORD ledcounter;

WORD  __attribute__((space(eedata))) dat;

void write_freq(WORD newData)
{
  WORD offset;

  NVMCON = 0x4058;

  TBLPAG = __builtin_tblpage(&dat);
  offset = __builtin_tbloffset(&dat);
  __builtin_tblwtl(offset,0);

  asm volatile("disi #5");
  __builtin_write_NVM();
  while(NVMCONbits.WR == 1);

  NVMCON = 0x4004;

  TBLPAG = __builtin_tblpage(&dat);
  offset = __builtin_tbloffset(&dat);
  __builtin_tblwtl(offset,newData);

  asm volatile("disi #5");
  __builtin_write_NVM();
  while(NVMCONbits.WR == 1);
}

WORD read_freq(void)
{
  WORD offset;

  TBLPAG = __builtin_tblpage(&dat);
  offset = __builtin_tbloffset(&dat);
  return(__builtin_tblrdl(offset));
}


unsigned int mygets(unsigned int length,char *buffer)
{
    while(length)                         /* read till length is 0 */
    {
        while(!DataRdyUART());
        *buffer++ = ReadUART();          /* data word from HW buffer to SW buffer */
        length--;
    }

    return(length);                       /* number of data yet to be received i.e.,0 */
}



// Calculate the divisors and stuff for the synth chip
double ics_mkdiv(DWORD khz,BYTE *p)
{

float fvdw,frdw,fvco,fmin,fr;
double rv,outfac;
WORD s123,rdw,vdw;

	s123 = 0;
	fvco = khz * 10.0;
	outfac = 10.0;
	if (khz > 15000)
	{
		s123 = 4;
		fvco = khz * 5.0;
		outfac = 5.0;
	}
	if (khz > 30000)
	{
		s123 = 6;
		fvco = khz * 3.0;
		outfac = 3.0;
	}
	if (khz > 60000)
	{
		s123 = 1;
		fvco = khz * 2.0;
		outfac = 2.0;
	}
    fmin = 10000.0;
    rv = -999999999;
    for(rdw = 2; rdw <= 50; rdw++)
    {
		LED ^= 1;
		frdw = rdw + 2;
		for(vdw = 10; vdw <= 511; vdw++)
		{
			ClrWdt();
			fvdw = vdw + 8;
			fr = XTAL2X * fvdw;
			fr /= frdw;
			if (fabs(fr - fvco) < fmin)
			{
				fmin = fabs(fr - fvco);
				p[0] = 0x10 | s123;
				p[1] = vdw >> 1;
				p[2] = rdw;
				if (vdw & 1) p[2] |= 0x80;
				rv = (double)fr / outfac;
            }
    	}
	}
	return(rv);
}

/* write a byte to SPI */
void write_spi(unsigned char val)
{
unsigned char dummy;

		SPI1BUF = val;
		while ((SPI1STATbits.SPITBF == 1) || (SPI1STATbits.SPIRBF == 0));
		dummy = SPI1BUF;
}


void main_processing_loop(void)
{

	ClrWdt();
	if (ledcounter++ > LED_COUNT) 
	{
		ledcounter = 0;
		LED ^= 1;
	}

}

int write(int handle, void *buffer, unsigned int len)
{
int i;
BYTE *cp;

		i = len;
		if ((handle >= 0) && (handle <= 2))
		{
			cp = (BYTE *)buffer;
			while(i--)
			{
				while(BusyUART()); 
				if (*cp == '\n')
				{
					WriteUART('\r');
					while(BusyUART());
				}
				WriteUART(*cp);
				cp++;
			}
		}
		else
		{
			errno = EBADF;
			return(-1);
		}
	return(len);
}

int read(int handle, void *buffer, unsigned int len)
{
	return 0;
}

int myfgets(char *dest,unsigned int len)
{
BYTE c;
int count,x;

	ClrWdt();

	aborted = 0;
	count = 0;
	while(count < len)
	{
		dest[count] = 0;
		for(;;)
		{
			if (DataRdyUART())
			{
				c = ReadUART() & 0x7f;
				break;
			}
			main_processing_loop();
		}
		if (c == 3) 
		{
			while(BusyUART()); 
			WriteUART('^');
			while(BusyUART()); 
			WriteUART('C');
			aborted = 1;
			dest[0] = '\n';
			dest[1] = 0;
			count = 1;
			break;
		}
		if ((c == 8) || (c == 127) || (c == 21))
		{
			if (!count) continue;
			if (c == 21) x = count;
			else x = 1;
			while(x--)
			{
				count--;
				dest[count] = 0;
				while(BusyUART());
				WriteUART(8);
				while(BusyUART());
				WriteUART(' ');
				while(BusyUART());
				WriteUART(8);
			}
			continue;
		}
		if ((c != '\r') && (c < ' ')) continue;
		if (c > 126) continue;
		if (c == '\r') c = '\n';
		dest[count++] = c;
		dest[count] = 0;
		if (c == '\n') break;
		while(BusyUART()) ;
		WriteUART(c);

	}
	while(BusyUART());
	WriteUART('\r');
	while(BusyUART());
	WriteUART('\n');
	return(count);
}

void myultoa(DWORD Value, char* Buffer)
{
	BYTE i;
	DWORD Digit;
	DWORD Divisor;
	BOOL Printed = FALSE;

	if(Value)
	{
		for(i = 0, Divisor = 1000000000; i < 10; i++)
		{
			Digit = Value/Divisor;
			if(Digit || Printed)
			{
				*Buffer++ = '0' + Digit;
				Value -= Digit*Divisor;
				Printed = TRUE;
			}
			Divisor /= 10;
		}
	}
	else
	{
		*Buffer++ = '0';
	}

	*Buffer = '\0';
}


void myutoa3(WORD Value, char* Buffer)
{
	BYTE i;
	DWORD Digit;
	DWORD Divisor;
	BOOL Printed = TRUE;

	for(i = 0, Divisor = 100; i < 3; i++)
	{
		Digit = Value/Divisor;
		if(Digit || Printed)
		{
			*Buffer++ = '0' + Digit;
			Value -= Digit*Divisor;
			Printed = TRUE;
		}
		Divisor /= 10;
	}

	*Buffer = '\0';
}

int main(void) {

WORD i;
DWORD khz,l;
static BYTE b[3];
char a[20],a1[20];
double f;

	CLKDIV = 0; 
	__builtin_write_OSCCONH(0x01);
 	__builtin_write_OSCCONL(0x01);

	TRISA = 0; 
	TRISB = 0x2006;  /* bits 1 (U2RX), 2 (U1RX), and 14 (SDI) are inputs */
	LATA = 0;
	LATB = 0x310;
	ANSELA = 0;
	ANSELB = 0;
	SPI1CON1 = 0x123;
	SPI1STAT = 0x0000;
	STROBE = 0;
	aborted = 0;
	ledcounter = 0;

	U1MODE = 0x8000;		// Set UARTEN.  Note: this must be done before setting UTXEN
	U1STA = 0x400;	// UTXEN set

	#define CLOSEST_U1BRG_VALUE ((GetPeripheralClock()+8ul*BAUD_RATE1)/16/BAUD_RATE1-1)
	#define BAUD_ACTUAL1 (GetPeripheralClock()/16/(CLOSEST_U1BRG_VALUE+1))

	#define BAUD_ERROR1 ((BAUD_ACTUAL1 > BAUD_RATE1) ? BAUD_ACTUAL1-BAUD_RATE1 : BAUD_RATE1-BAUD_ACTUAL1)
	#define BAUD_ERROR_PRECENT1	((BAUD_ERROR1*100+BAUD_RATE1/2)/BAUD_RATE1)
	#if (BAUD_ERROR_PRECENT1 > 3)
			#warning UART1 frequency error is worse than 3%
	#elif (BAUD_ERROR_PRECENT1 > 2)
		#warning UART1 frequency error is worse than 2%
	#endif

	U1BRG = CLOSEST_U1BRG_VALUE;
	__C30_UART = 1;

	khz = read_freq();
	if (!khz) khz = DESIRED_FREQ_KHZ;

	printf("\nPCGM (Programmable Clock Generator Module) Ver. 0.4 12/28/2011\n\n");

	while(1)
	{

		printf("Calculating.....\n\n");

		f = ics_mkdiv(khz,b);

		myultoa(khz,a);
		printf("Desired Freq: %s KHz\n",a);
	
		l = f * 1000.0;
		myultoa(l,a);
		strcpy(a1,a);
		i = strlen(a);
		a[i - 3] = 0;
		printf("Actual Freq: %s.%s KHz\n\n",a,a1 + (i - 3));

		for(i = 0; i < 1000; i++) Nop();  
	
		CLKDIV = 0x0400;
	 	__builtin_write_OSCCONH(0x7);
	 	__builtin_write_OSCCONL(0x01);
		PDTS = 1;
		SPI1STATbits.SPIEN = 1;
		write_spi(0x00);  // write extra byte of 0 to clear poo out of register
		write_spi(b[0]);
		write_spi(b[1]);
		write_spi(b[2]);
		SPI1STATbits.SPIEN = 0;
		Nop();
		Nop();
		STROBE = 1;
		Nop();
		Nop();
		STROBE = 0;
		for(i = 0; i < 100; i++) Nop();  /* delay at least 10ms for synth to lock */
		EN1 = 0;
		EN2 = 0;

		CLKDIV = 0; 
		__builtin_write_OSCCONH(0x01);
	 	__builtin_write_OSCCONL(0x01);
	
		for(i = 0; i < 1000; i++) Nop();

		printf("Enter New Freq. in KHz, or \"SAVE\" : \n");
		fflush(stdout);

		myfgets(a,sizeof(a) - 1);
		if (aborted) continue;
		if (strlen(a) < 2) continue;
		if (strstr(a,"FROG"))
		{
			printf("\n\nA frog in a blender with a base diameter of 3 inches going\n");
			printf("75139293848.398697 RPM will be travelling at warp factor 1.000000,\n");
			printf("based upon the Jacobsen Frog Corollary.\n\n");
			continue;
		}
		if (strstr(a,"SAVE"))
		{
			write_freq(khz);
			printf("New Freq. Saved\n\n");
		}
		else
		{
			l = atol(a);
			if ((l >= 6000) && (l <= 200000)) khz = l;
			else printf("Invalid Freq. (must be >= 6000 KHz)\n\n");
		
		}	
	}
}


