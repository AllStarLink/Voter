/*
* VOTER Client System Firmware for RTCM Simulcast board
*
* Copyright (C) 2011-2014
* Jim Dixon, WB6NIL <jim@lambdatel.com>
*
* This file is part of the VOTER System Project 
*
*   VOTER System is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.
*
*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <libpic30.h>
#include <stdio.h>
#include <stdlib.h>
#include <p24FV32KA304.h>
#include <stddef.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "GenericTypeDefs.h"
#include "UART.h"

extern int __C30_UART;

#define GetPeripheralClock() (16000000ul)
#define	BAUD_RATE1 57600
#define	LED_COUNT 5000

#define	DISABLE_INTERRUPTS() __builtin_disi(0x3FFF)
#define	ENABLE_INTERRUPTS() __builtin_disi(0)

#define	SPI_START 0xAA
#define	SPI_ESC 0xAB

#define TESTBIT LATAbits.LATA11
#define LED LATAbits.LATA10
#define UPPWR_EN LATCbits.LATC9
#define UPCLK_EN LATCbits.LATC8
#define PDTS_527 LATAbits.LATA4
#define PDTS_525 LATAbits.LATA9
#define SHFT_CLR LATAbits.LATA8
#define SHFT_LTCH LATBbits.LATB8
#define CLK1_EN LATCbits.LATC0
#define AUDDEV_EN LATCbits.LATC1
#define POCDEV_EN LATCbits.LATC2
#define POCSAG_ACT PORTAbits.RA7
#define POCSAG_EN LATBbits.LATB12
#define DEEMP_EN LATBbits.LATB4
#define DTMF_EST PORTBbits.RB7
#define DTMF_SD PORTAbits.RA1
#define DTMF_ACK LATAbits.LATA0

#define PTT_OUT LATAbits.LATA2
#define PTT_IN PORTAbits.RA3

#define	SPI_TXBUF_SIZE 128
#define	SPI_RXBUF_SIZE 128
#define SPI_TXBUF_MASK (SPI_TXBUF_SIZE-1)
#define SPI_RXBUF_MASK (SPI_RXBUF_SIZE-1)

#define	SPI_OFFLINE_TIME 500

#define	AFTERBOOT_TIME 4000

#define	DESIRED_FREQ_KHZ 14400

#define	MAX_FREQ_ERROR 0.07

#define	FREQGEN_TIME 5000 // 10000 ms settle time for clocks OK
#define	MAINRST_TIME 2000 // 5000ms for main processor reset

#define	MAXDTMFSTR 16

#define	FREQGEN_OK (freqtimer >= FREQGEN_TIME)

#define	EEPROM_MAGIC 0xbeef

_FBS( BWRP_OFF & BSS_OFF )
_FGS( GSS0_OFF & GWRP_OFF )
_FOSCSEL( FNOSC_FRCPLL & IESO_OFF & SOSCSRC_DIG)
_FOSC( FCKSM_CSECMD & OSCIOFNC_OFF & POSCMOD_NONE  )
_FPOR( BOREN_BOR3 & LVRCFG_OFF & PWRTEN_OFF & BORV_V27 & MCLRE_ON)	
_FICD( ICS_PGx3 )

BOOL aborted;
DWORD ledcounter;
unsigned short lastpoc;
unsigned short lastest;
unsigned short lastest1;
WORD lasttmr1,x1;
WORD lasttmr4,x4;
float xtal1x;
float xtal2x;
WORD khzmin;
WORD khzmax;
BYTE dtmfcnt;
BYTE cur_dtmf;
BYTE last_dtmf;
BYTE cntcnt;
WORD freqtimer;
WORD mainrsttimer;
BYTE lastfreqok;
BYTE lastfreqok1;
WORD spiidx;
WORD spilen;
BYTE wasesc;
BYTE inseq;
BYTE spicmd;
WORD spitimer;
BYTE bootedit; // As in "Booted it"  *NOT* "Boot Edit" :-)
DWORD tempstatus;
DWORD instatus;
DWORD outstatus;
DWORD lastoutstatus;
BYTE lastdummy;
BYTE freq_changed;
BYTE dtmfbuf[MAXDTMFSTR + 1];
BYTE dtmfidx;
WORD dtmftimer;
BYTE dtmfdone;
BYTE rx_inhibit;
BYTE tx_inhibit;
BYTE rpt_inhibit;
WORD afterboottimer;
BYTE lastdtmfbuf[MAXDTMFSTR];

static BYTE txBuf[SPI_TXBUF_SIZE];
static BYTE txgetidx;   
static BYTE txputidx;   
static BYTE rxBuf[SPI_RXBUF_SIZE];
static BYTE rxputidx;   
static BYTE rxgetidx; 

unsigned char div_r, div_s, div_527, ld5, ld6, ld7;
unsigned short div_v;

static const char dtmfdig[] = "D1234567890*#ABCD";

static const char VERSION[] = "0.4 03/04/2014";

struct AppConfig {
	WORD magic;
	DWORD khz;
	float xtal1x;
	BYTE txlevel;
	BYTE poclevel;
	BYTE pocmode;
	BYTE dtmfleadin;
	WORD dtmftimeout;
	BYTE dtmfmutemode;
	BYTE dtmfdeemphasis;
	BYTE dtmftxenable[MAXDTMFSTR];
	BYTE dtmfrxenable[MAXDTMFSTR];
	BYTE dtmfrepeatmode[MAXDTMFSTR];
	BYTE dtmfreboot[MAXDTMFSTR];
} AppConfig;

WORD __attribute__((space(eedata))) dat;

void __attribute__((interrupt, auto_psv)) _T2Interrupt(void)
{
WORD myt1,myt4;

	if (_T2IE && _T2IF) 
	{
		myt1 = TMR1;
		myt4 = TMR4;
		if (dtmfcnt > 1) dtmfcnt--;
		if (mainrsttimer > 0) mainrsttimer--;
		if ((freqtimer) && (freqtimer < 0xFFFF)) freqtimer++;
		if (spitimer > 0) spitimer--;
		if (dtmftimer > 0) dtmftimer--;
		if (afterboottimer > 0) afterboottimer--;

		if (myt1 < lasttmr1) // If rolled-over
		{
			x1 = 0xFFFF - lasttmr1;
			x1 += myt1 + 1;
		}
		else
		{
			x1 = myt1 - lasttmr1;
		}
		lasttmr1 = myt1;
		if (++cntcnt >= 8)
		{
			cntcnt = 0;
			if (myt4 < lasttmr4) // If rolled-over
			{
				x4 = 0xFFFF - lasttmr4;
				x4 += myt4 + 1;
			}
			else
			{
				x4 = myt4 - lasttmr4;
			}
			lasttmr4 = myt4;
		}

		TESTBIT ^= 1;
		_T2IF = 0; 
	}
}

void __attribute__((interrupt, auto_psv)) _SPI1Interrupt(void)
{
BYTE dummy;

	if (_SPI1IE && _SPI1IF) 
	{
		spitimer = SPI_OFFLINE_TIME;
		while(!SPI1STATbits.SR1MPT)
		{
			if (((rxputidx + 1) & SPI_RXBUF_MASK) != rxgetidx)
			{
				dummy = SPI1BUF; 
				if ((dummy != 0xff) || (lastdummy != 0xff))
				{
					rxBuf[rxputidx] = dummy;
				    rxputidx++;
					rxputidx &= SPI_RXBUF_MASK;
				}
				lastdummy = dummy;
			} else dummy = SPI1BUF;
		}
		while(!SPI1STATbits.SPITBF)
		{
		    if (txputidx != txgetidx) 
			{
		        SPI1BUF = txBuf[txgetidx];
		        txgetidx++;
				txgetidx &= SPI_TXBUF_MASK;
		    }
			else SPI1BUF = 0xff;
		}
	}
	_SPI1IF = 0;
}

char DataRdySPI(void)
{
BYTE ret;

	DISABLE_INTERRUPTS();
	if (rxputidx != rxgetidx) ret = 1;
	else ret = 0;
	ENABLE_INTERRUPTS();
	return(ret);
}

char BusySPI(void)
{  
BYTE ret;

	DISABLE_INTERRUPTS();
	if (((txputidx + 1) & SPI_TXBUF_MASK) == txgetidx) ret = 1;
	else ret = 0;
	ENABLE_INTERRUPTS();
	return ret;
}

void WriteSPI(BYTE data)
{
	DISABLE_INTERRUPTS();
	if (((txputidx + 1) & SPI_TXBUF_MASK) != txgetidx)
	{
        txBuf[txputidx] = data;
		txputidx++;
		txputidx &= SPI_TXBUF_MASK;
	}
	ENABLE_INTERRUPTS();
}

BYTE ReadSPI(void)
{
    BYTE c;

	DISABLE_INTERRUPTS();
    c = rxBuf[rxgetidx];
    rxgetidx++;
	rxgetidx &= SPI_RXBUF_MASK;
	ENABLE_INTERRUPTS();
	return c;
}

int PeekSPI(WORD n)
{
    BYTE c;
	WORD myidx;

	if (n > SPI_RXBUF_MASK) return(-1);
	DISABLE_INTERRUPTS();
	myidx = rxgetidx;
	myidx += n;
	myidx &= SPI_RXBUF_MASK;
	if (rxputidx == myidx) return(-1);
    c = rxBuf[myidx];
	ENABLE_INTERRUPTS();
	return (int)c;
}

void ConsumeSPI(WORD n)
{
	DISABLE_INTERRUPTS();
    rxgetidx += n;
	rxgetidx &= SPI_RXBUF_MASK;
	ENABLE_INTERRUPTS();
	return;
}

void Simul_Reset(void)
{
	DISABLE_INTERRUPTS();
	asm("reset");
}

void critical_processing_loop();

void spi_out(BYTE *buf,BYTE len)
{
BYTE	i;

	while (BusySPI()) critical_processing_loop();
	WriteSPI(SPI_START);
	while (BusySPI()) critical_processing_loop();
	WriteSPI(len);
	for(i = 0; i < len; i++)
	{
		if ((buf[i] == SPI_START) || (buf[i] == SPI_ESC))
		{
			while (BusySPI()) critical_processing_loop();
			WriteSPI(SPI_ESC);
		}
		while (BusySPI()) critical_processing_loop();
		WriteSPI(buf[i]);
	}
}

void send_status(void)
{
BYTE str[5];

	str[0] = 2; // Status command
	str[1] = outstatus & 0x7f;
	str[2] = (BYTE)((outstatus >> 8) & 0x7f);
	str[3] = (BYTE)((outstatus >> 16) & 0x7f);
	str[4] = (BYTE)((outstatus >> 24) & 0x7f);
	spi_out(str,5);
}

void write_config(void)
{
  WORD offset,*wp,i,v;

  wp = (WORD *)&AppConfig;
  AppConfig.magic = EEPROM_MAGIC;
  for(i = 0; i < sizeof(struct AppConfig); i += 2)
  { 
    NVMCON = 0x4058;
    TBLPAG = __builtin_tblpage(&dat);
    offset = __builtin_tbloffset(&dat);
  	__builtin_tblwtl(offset + i,0);

    asm volatile("disi #5");
    __builtin_write_NVM();
    while(NVMCONbits.WR == 1);

	v = *wp++;  
  	NVMCON = 0x4004;
    TBLPAG = __builtin_tblpage(&dat);
    offset = __builtin_tbloffset(&dat);
  	__builtin_tblwtl(offset + i,v);

    asm volatile("disi #5");
    __builtin_write_NVM();
    while(NVMCONbits.WR == 1);
  }
}

void read_config(void)
{
  WORD offset,*wp,i,v;

  wp = (WORD *)&AppConfig;
  for(i = 0; i < sizeof(struct AppConfig) ; i += 2)
  {
  	TBLPAG = __builtin_tblpage(&dat);
  	offset = __builtin_tbloffset(&dat);
	v = __builtin_tblrdl(offset + i);
	if ((i == 0) && (v != EEPROM_MAGIC)) break;
	*wp++ = v;
  }
  return;
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
double ics525_mkdiv(DWORD khz)
{

float fvdw,frdw,fvco,fmin,fr;
double rv,outfac;
WORD s123,rdw,vdw;

	div_527 = 0;
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
	        s123 = 3;
	        fvco = khz * 4.0;
	        outfac = 4.0;
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
			fr = (xtal2x) * fvdw;
			fr /= frdw;
			if (fabs(fr - fvco) < fmin)
			{
				fmin = fabs(fr - fvco);
				div_s = s123;
				div_r = rdw;
				div_v = vdw;
				rv = (double)fr / outfac;
			}
		}
	}
	return(rv);
}

// Calculate the divisors and stuff for the synth chip
double ics527_mkdiv(DWORD khz)
{

float fvdw,frdw,fvco,fmin,fr;
double rv,outfac;
WORD s123,rdw,vdw;

	div_527 = 0;
	fvco = khz;
	outfac = 1.0;
	if ((khz >= 18000) && (khz < 37500))
	{
		s123 = 0;
	}
	else if ((khz >= 9000) && (khz <= 18000))
	{
		s123 = 1;
	}
	else if ((khz >= 2000) && (khz <= 5000))
	{
		s123 = 2;
	}
	else if ((khz >= 37500) && (khz <= 80000))
	{
		s123 = 3;
	}
	else 
		return(-99999.0);
    fmin = 10000.0;
    rv = -999999999;
    for(rdw = 0; rdw <= 127; rdw++)
    {
		LED ^= 1;
		frdw = rdw + 2;
		for(vdw = 0; vdw <= 127; vdw++)
		{
			ClrWdt();
			fvdw = vdw + 2;
			fr = xtal1x * fvdw;
			fr /= frdw;
			if (fabs(fr - fvco) < fmin)
			{
				fmin = fabs(fr - fvco);
				div_s = s123;
				div_r = rdw;
				div_v = vdw;
				rv = (double)fr / outfac;
			}
		}
	}
	if (rv > 0.0) div_527 = 1;
	return(rv);
}

/* write a byte to SPI */
void write_spi2(unsigned short val)
{
unsigned char dummy;

	SPI2BUF = val;
	while ((SPI2STATbits.SPITBF == 1) || (SPI2STATbits.SPIRBF == 0));
	dummy = SPI2BUF;
}



void div_out(void)
{
unsigned char x;


	SHFT_LTCH = 0;
	SHFT_CLR = 0;
	Nop();
	Nop();
	SHFT_CLR = 1;

	SPI2CON1 = 0x123;
	SPI2STAT = 0x0000;
	SPI2STATbits.SPIEN = 1;
	x = (div_v >> 6) & 7;
	x |= 0xf8;
	if (ld5 > 1) x &= ~0x80;
	else if (ld5 > 0) x &= ~0x40;
	if (ld6 > 1) x &= ~0x20;
	else if (ld6 > 0) x &= ~0x10;
	if (!ld7) x &= ~0x08;
	write_spi2(x);
	x = (div_s >> 1) & 3;
	x |= (div_v & 0x3f) << 2;
	write_spi2(x);
	x = div_r & 0x7f;
	x |= (div_s & 1) << 7;
	write_spi2(x);

	SHFT_LTCH = 1;
	Nop();
	Nop();

	SHFT_LTCH = 0;
	if (div_527)
	{
		PDTS_527 = 1;
		PDTS_525 = 0;
	}
	else
	{
		PDTS_527 = 0;
		PDTS_525 = 1;
	}
	SPI2STATbits.SPIEN = 0;
}

void pocpot(unsigned char val)
{
	SPI2CON1 = 0x523; // 16 bit mode
	SPI2STAT = 0x0000;
	SPI2STATbits.SPIEN = 1;
	Nop();
	Nop();
	POCDEV_EN = 0;
	Nop();
	Nop();
	write_spi2(127 - val);
	Nop();
	Nop();
	POCDEV_EN = 1;
	Nop();
	Nop();
	SPI2STATbits.SPIEN = 0;
}

void txpot(unsigned char val)
{
	SPI2CON1 = 0x523; // 16 bit mode
	SPI2STAT = 0x0000;
	SPI2STATbits.SPIEN = 1;
	Nop();
	Nop();
	AUDDEV_EN = 0;
	Nop();
	Nop();
	write_spi2(val | 0x1100);
	Nop();
	Nop();
	AUDDEV_EN = 1;
	Nop();
	Nop();
	SPI2STATbits.SPIEN = 0;
}

void critical_processing_loop(void)
{
long l;
BYTE i,x;

	ClrWdt();

	x = POCSAG_ACT;
	if (tx_inhibit)
	{
		PTT_OUT = 0;
		POCSAG_EN = 0;
	}
	else
	{
		// If not in POCSAG paging, or in page mode 
		if (!x)
		{
			if (afterboottimer) PTT_OUT = 0; else PTT_OUT = PTT_IN;
			POCSAG_EN = 0;
		}
		else
		{
			if ((AppConfig.pocmode != 1) && (!afterboottimer))
				PTT_OUT = PTT_IN;
			else PTT_OUT = 0;
			if (AppConfig.pocmode == 0) 
			{
				POCSAG_EN = 1;
			}
			else
			{
				POCSAG_EN = 0;
			}
		}
	}

	x = FREQGEN_OK;
	if (lastfreqok != x)
	{
		mainrsttimer = MAINRST_TIME;
		bootedit = 1;
		afterboottimer = AFTERBOOT_TIME;
		lastfreqok = x;
	}
	if (mainrsttimer) 
	{
		UPPWR_EN = 0;
		CLK1_EN = 1;
		spitimer = 0;
	}
	else 
	{
		UPPWR_EN = 1;
		if (x) CLK1_EN = 0;
		else CLK1_EN = 1;
	}
	if (x) UPCLK_EN = 1;
	else UPCLK_EN = 0;

	l = ((long)x4 * 10000ul) / (long)AppConfig.khz;
	if ((l > 10700/*10500*/) || (l < 9300/*9500*/) || (x1 < khzmin) || (x1 > khzmax)) 
	{
		freqtimer = 0;
	}
	else
	{
		if (!freqtimer) freqtimer = 1;
	}
	if (freqtimer == 0) ld5 = 2;
	else if (freqtimer >= FREQGEN_TIME) ld5 = 1;

	if (!spitimer) ld7 = 1; else ld7 = 0;
	if (dtmftimer) ld6 = 1; else if (tx_inhibit || rx_inhibit) ld6 = 2;
	else ld6 = 0;

	div_out();

	if (DTMF_EST != lastest)
	{
		lastest = DTMF_EST;
		dtmfcnt = 25;
	}
	if (dtmfcnt == 1)
	{
		dtmfcnt = 0;
		if (lastest != lastest1)
		{
			lastest1 = lastest;
			if (lastest) 
			{
				x = 0;
				for(i = 0; i < 4; i++)
				{
					DTMF_ACK = 1;
					Nop();
					Nop();
					Nop();
					Nop();
					DTMF_ACK = 0;
					if (DTMF_SD) x |= 1 << i;
					Nop();
					Nop();
					Nop();
					Nop();
				}
				cur_dtmf = dtmfdig[x];
#ifdef	DTMF_LEADIN_INFUNCTIONS
				if (!dtmftimer)
				{
					if (cur_dtmf == AppConfig.dtmfleadin)
					{
						dtmftimer = AppConfig.dtmftimeout;
						dtmfidx = 0;
						dtmfdone = 0;
						memset(dtmfbuf,0,sizeof(dtmfbuf));
					}
				}
				else
				{
					dtmftimer = AppConfig.dtmftimeout;
				}
#else
				if (dtmftimer) dtmftimer = AppConfig.dtmftimeout;
				if (cur_dtmf == AppConfig.dtmfleadin)
				{
					dtmftimer = AppConfig.dtmftimeout;
					dtmfidx = 0;
					dtmfdone = 0;
					memset(dtmfbuf,0,sizeof(dtmfbuf));
				}
#endif

			}
			else
			{
				if (dtmftimer)
				{
					dtmftimer = AppConfig.dtmftimeout;
					if (dtmfidx < MAXDTMFSTR)
					{
						dtmftimer = AppConfig.dtmftimeout;
#ifdef	DTMF_LEADIN_INFUNCTIONS
						if ((dtmfidx > 0) || (cur_dtmf != AppConfig.dtmfleadin))
#else
						if (cur_dtmf != AppConfig.dtmfleadin)
#endif
						{ 
							dtmfbuf[dtmfidx++] = cur_dtmf;
							dtmfbuf[dtmfidx] = 0;
						}
					}
				}
				cur_dtmf = 0;

			}
	  	}
	}
	if (dtmfdone)
	{
		dtmftimer = 0;
		dtmfidx = 0;
		dtmfdone = 0;
	}
	if (!dtmftimer)
	{
		memset(dtmfbuf,0,sizeof(dtmfbuf));
		dtmfidx = 0;
	}

}


void main_processing_loop(void)
{
BYTE x,c,i;
long l;
static char foo = 0;

	critical_processing_loop();

	x = FREQGEN_OK;
	if (lastfreqok1 != x)
	{
		if (x) printf("Freq OK!!\n");
		else printf("Freq NOT OK!!!\n");
		lastfreqok1 = x;
	}

	if (ledcounter++ > LED_COUNT) 
	{
		ledcounter = 0;
		LED ^= 1;
		if ((freqtimer > 0) && (freqtimer < FREQGEN_TIME))
		{
			if (LED) ld5 = 1;
			else ld5 = 0;
		}
#ifdef	POCSAG_DEBUG
		if (lastpoc != POCSAG_ACT)
		{
			printf("pocsag: %d\n",POCSAG_ACT);
			lastpoc = POCSAG_ACT;
		}
#endif
		if (foo++ > 10)
		{
			l = ((long)x4 * 10000ul) / (long)AppConfig.khz;
			if (!freqtimer) printf("FREQS ERROR: %u (%u %u) %u (.%04ld)\n",x1,khzmin,khzmax,x4,l);
			foo = 0;
		}
	}
	div_out();

	if (bootedit)
	{
		if (bootedit > 1) printf("RTCM Requested Hard Re-boot\n");
		bootedit = 0;
		printf("Re-booted (Power Cycled) Main RTCM Processor\n");
	}
	if (cur_dtmf != last_dtmf)
	{
		if (cur_dtmf) 
			printf("DTMF %c\n",cur_dtmf);
//		else
//			printf("foop: Released DTMF digit %c (%s)\n",last_dtmf,dtmfbuf);
		last_dtmf = cur_dtmf;
	}
	if (!strncmp((char *)dtmfbuf,(char *)AppConfig.dtmftxenable,strlen((char *)AppConfig.dtmftxenable)))
	{
		if (strlen((char *)dtmfbuf) > strlen((char *)AppConfig.dtmftxenable))
		{
			c = dtmfbuf[strlen((char *)AppConfig.dtmftxenable)];
			if (c == '0') tx_inhibit = 1; else if (c == '1') tx_inhibit = 0;
			dtmfdone = 1;
			strcpy((char *)lastdtmfbuf,(char *)dtmfbuf);
		}
	}
	if (!strncmp((char *)dtmfbuf,(char *)AppConfig.dtmfrxenable,strlen((char *)AppConfig.dtmfrxenable)))
	{
		if (strlen((char *)dtmfbuf) > strlen((char *)AppConfig.dtmfrxenable))
		{
			c = dtmfbuf[strlen((char *)AppConfig.dtmfrxenable)];
			if (c == '0') rx_inhibit = 1; else if (c == '1') rx_inhibit = 0;
			dtmfdone = 1;
			strcpy((char *)lastdtmfbuf,(char *)dtmfbuf);
		}
	}
	if (!strncmp((char *)dtmfbuf,(char *)AppConfig.dtmfrepeatmode,strlen((char *)AppConfig.dtmfrepeatmode)))
	{
		if (strlen((char *)dtmfbuf) > strlen((char *)AppConfig.dtmfrepeatmode))
		{
			c = dtmfbuf[strlen((char *)AppConfig.dtmfrepeatmode)];
			if (c == '0') rpt_inhibit = 1; else if (c == '1') rpt_inhibit = 0;
			dtmfdone = 1;
			strcpy((char *)lastdtmfbuf,(char *)dtmfbuf);
		}
	}
	if (!strcmp((char *)dtmfbuf,(char *)AppConfig.dtmfreboot))
	{
		Simul_Reset();
	}
	i = 0;
	if (dtmftimer && (AppConfig.dtmfmutemode > 1)) i = 1;
	if (rx_inhibit) i = 1;
	if (i) outstatus |= 2UL; else outstatus &= ~2UL;
	if (rpt_inhibit) outstatus |= 4UL; else outstatus &= ~4UL;
	if (dtmftimer && (AppConfig.dtmfmutemode > 0)) outstatus |= 1UL; else outstatus &= ~1UL;
	if (lastoutstatus != outstatus)
	{
		send_status();
		lastoutstatus = outstatus;
	}
}

int write(int handle, void *buffer, unsigned int len)
{
int i;
BYTE *cp;

		if (len > 120) len = 120;
		i = len;
		if ((handle >= 0) && (handle <= 2))
		{
			cp = (BYTE *)buffer;
			while(i--)
			{
				while(BusyUART()) critical_processing_loop(); 
				if (*cp == '\n')
				{
					WriteUART('\r');
					while(BusyUART()) critical_processing_loop();
				}
				WriteUART(*cp);
				cp++;
			}
			cp = (BYTE *)buffer;
			if (spitimer)
			{
				while (BusySPI()) critical_processing_loop();
				WriteSPI(SPI_START);
				while (BusySPI()) critical_processing_loop(); 
				WriteSPI(len + 1);
				while (BusySPI()) critical_processing_loop(); 
				WriteSPI(1);
				for(i = 0; i < len; i++)
				{
					while(BusySPI()) critical_processing_loop();
					WriteSPI(cp[i]);
				}
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

BYTE CharSPI(void)
{
	WORD myidx;
	BYTE rxval;

	if (DataRdySPI())
	{
		rxval = ReadSPI();
		if ((rxval == SPI_ESC) && (!wasesc))
		{
			wasesc = 1;
		}
		else
		{
			if (!wasesc)
			{
				if (rxval == SPI_START)
				{
					spiidx = 0;
					spilen = 0;
					spicmd = 0;
				}
			}
			wasesc = 0;
			if ((spiidx == 0) && (rxval == SPI_START)) // Start Byte
			{
				spiidx = 1;
			}
			else if (spiidx == 1) // Count Byte
			{
				if (rxval > 253)
				{
					spiidx = 0;
					spilen = 0;
					spicmd = 0;
					return(0);
				}
				spilen = rxval + 2;
				spiidx++;
			}
			else if (spiidx && (spiidx < spilen)) // Data Byte
			{
				myidx = spiidx;
				spiidx++;
//				printf("SPI [%d/%d]: %02X\n",myidx - 2,spilen - 2,rxval);
				if (myidx == 2) spicmd = rxval;
				else
				{
					switch(spicmd)
					{
						case 1: // ASCII chars
							spiidx = 0;
							spilen = 0;
							return(rxval);
						case 2: // Status from other side
							if (myidx == 3)
							{
								tempstatus = (DWORD)rxval;
							}
							else if (myidx == 4)
							{
								tempstatus += ((DWORD)rxval) << 8;
							}
							else if (myidx == 5)
							{
								tempstatus += ((DWORD)rxval) << 16;
							}
							else if (myidx == 6)
							{
								tempstatus += ((DWORD)rxval) << 24;
								instatus = tempstatus;
								spiidx = 0;
								spilen = 0;
							}
							break;
						case 86: // Reboot RTCM
							spiidx = 0;
							spilen = 0;
							if (rxval != 0x86) break; // Has to be "MAGIC" Booty :-) :-)
							mainrsttimer = MAINRST_TIME;
							bootedit = 2;
							afterboottimer = AFTERBOOT_TIME;
							break;
						default:
							spiidx = 0;
							spilen = 0;
							break;
					}
				}
			}
		}
	}
	return(0);
}

void WriteCons(BYTE c)
{
	while(BusyUART()) critical_processing_loop(); 
	WriteUART(c);
	if (spitimer)
	{
		while (BusySPI()) critical_processing_loop();
		WriteSPI(SPI_START);
		while (BusySPI()) critical_processing_loop(); 
		WriteSPI(2);
		while (BusySPI()) critical_processing_loop(); 
		WriteSPI(1);
		while (BusySPI()) critical_processing_loop(); 
		WriteSPI(c);
	}
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
			c = CharSPI();
			if (c) break;
			main_processing_loop();
		}
		if (c == 3) 
		{
			WriteCons('^');
			WriteCons('C');
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
				WriteCons(8);
				WriteCons(' ');
				WriteCons(8);
			}
			continue;
		}
		if ((c != '\r') && (c < ' ')) continue;
		if (c > 126) continue;
		if (c == '\r') c = '\n';
		dest[count++] = c;
		dest[count] = 0;
		if (c == '\n') break;
		WriteCons(c);

	}
	WriteCons('\r');
	WriteCons('\n');
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



/* Processor IO Ports 

RA0 - DTMF ACK (out)
RA1 - DTMF SD (in)
RA2 - PTT OUT (out)
RA3 - PTT in (in)
RA4 - PDTS_527 (out)
RA5 - MCLR (in)
RA6 - VCap (in)
RA7 - POCSAG ACTIVE (in)
RA8 - SHFT CLR (out)
RA9 - PDTS_525 (out)
RA10 - HBEAT (LED) (out)
RA11 - ERROR (LED) (out)

RB0 - TEST (out)
RB1 - uPCLK_PD (out)
RB2 - PROG_CLK (in)
RB3 - CLK2_EN (out)
RB4 - DEEMP_EN (out)
RB5/6 - Programming (in)
RB7 - DTMF_INT (in)
RB8 - SHFT_LTCH (out)
RB9 - uP_CLK (in)
RB10 - SDI1 (in)
RB11 - SCK1 (in)
RB12 - POCSAG_EN (out)
RB13 - SDO1 (out)
RB14 - PPS (in)
RB15 - SS1 (in)

RC0 - CLK1_EN (out)
RC1 - AUD_DEV_CS (out)
RC2 - POSAG_DEV_CS (out)
RC3 - SDI2 (in)
RC4 - SDO2 (out)
RC5 - SCK2 (out)
RC6 - U1RX (in)
RC7 - U1TX (out)
RC8 - uP_CLKEN (out) (high to enable GPS-fed clock)
RC9 - uP_PWR (out) (high to enable pwr)

*/

int main(void) {

unsigned int i;
DWORD l;
BYTE c,sel,ok;
char a[20],a1[20],cmdstr[20];
double f;
float f0;

static const char menu[] = "\nRTCM Simulcast/PAGING menu\n\nSelect the following values to View/Modify:\n\n" 
	"1  - Clock Generator Freq (%lu KHz)\n"
	"2  - Clock Generator REF Freq (%f KHz)\n"
	"3  - Fine Tx Audio Level (%d)\n"
	"4  - POGSAG Paging Audio Level (%d)\n",
menu1[] = 
	"5  - POGSAG Paging Mode (%d) (0-2) (0=Analog,1=Digital,2=Digital+PTT)\n"
	"6  - DTMF lead-in character (%c)\n"
	"7  - DTMF sequence time (%d) (msecs)\n"
	"8  - DTMF mute (%d) (0-2) (0=No mute,1=Mute Audio,2=Unkey Rx)\n"
	"9  - DTMF de-emphasis (%d) (0-2) (0=Off, 1=On, 2=Follow Main RTCM)\n",
menu2[] = 
	"10 - DTMF \"Enable/Disable Tx\" Digit Seq. (%s)\n"
	"11 - DTMF \"Enable/Disable Rx\" Digit Seq. (%s)\n"
	"12 - DTMF \"Enable/Disable local repeat\" Digit Seq. (%s)\n"
	"13 - DTMF \"Reboot System\" Digit Seq. (%s)\n",
menu3[] = 
	"98 - Status,  "
	"99 - Save Values to EEPROM\n"
	"r - reboot system\n\n",
entsel[] = "Enter Selection (1-12,98,99,r) : ",entnewval[] = "Enter New Value : ", 
newvalchanged[] = "Value Changed Successfully\n",
saved[] = "Configuration Settings Written to EEPROM\n", 
newvalerror[] = "Invalid Entry, Value Not Changed\n",
newvalnotchanged[] = "No Entry Made, Value Not Changed\n",
invalselection[] = "Invalid Selection\n",
paktc[] = "\nPress The Any Key (Enter) To Continue\n";

	CLKDIV = 0; 
	__builtin_write_OSCCONH(0x01);
 	__builtin_write_OSCCONL(0x01);

	TRISA = 0x00EA; 
	TRISB = 0xCE84;  
	TRISC = 0x0048;
	LATA = 0xc60;
	LATB = 0xCE8B;
	LATC = 0x78;
	ANSELA = 0;
	ANSELB = 0;

	aborted = 0;
	ledcounter = 0;
	AUDDEV_EN = 1;
	POCDEV_EN = 1;
	CLK1_EN = 1;
	DEEMP_EN = 0;
	POCSAG_EN = 0;
	DTMF_ACK = 0;

	lastfreqok = 0xff;
	lastfreqok1 = 0xff;
	afterboottimer = AFTERBOOT_TIME;
	lastpoc = 0;
	lastest = 0;
	lastest1 = 0;
	lasttmr1 = 0;
	x1 = 0;
	lasttmr4 = 0;
	x4 = 0;
	dtmfcnt = 0;
	cur_dtmf = 0;
	last_dtmf = 0;
	cntcnt = 0;
	freqtimer = 0;
	mainrsttimer = 0;
	spiidx = 0;
	spilen = 0;
	wasesc = 0;
	inseq = 0;
	spicmd = 0;
	spitimer = 0;
	bootedit = 0;
	tempstatus = 0;
	instatus = 0;
	outstatus = 0;
	lastoutstatus = 0;
	lastdummy = 0;
	freq_changed = 0;
	memset(dtmfbuf,0,sizeof(dtmfbuf));
	memset(lastdtmfbuf,0,sizeof(dtmfbuf));
	dtmfidx = 0;
	dtmftimer = 0;
	dtmfdone = 0;
	rx_inhibit = 0;
	tx_inhibit = 0;
	rpt_inhibit = 0;
	afterboottimer = 0;

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

	memset(&AppConfig,0,sizeof(AppConfig));
	AppConfig.khz = DESIRED_FREQ_KHZ;
	AppConfig.xtal1x = 10000.0;
	AppConfig.txlevel = 128;
	AppConfig.poclevel = 64;
	AppConfig.pocmode = 0;
	AppConfig.dtmfleadin = 'D';
	AppConfig.dtmftimeout = 5000;
	AppConfig.dtmfmutemode = 1;
	AppConfig.dtmfdeemphasis = 0;
	strcpy((char *)AppConfig.dtmftxenable,"2");
	strcpy((char *)AppConfig.dtmfrxenable,"5");
	strcpy((char *)AppConfig.dtmfrepeatmode,"8");
	strcpy((char *)AppConfig.dtmfreboot,"CB");

	read_config();

	txpot(AppConfig.txlevel);
	pocpot(AppConfig.poclevel);
	if (AppConfig.dtmfdeemphasis > 1)
	{
		if (instatus & 2) DEEMP_EN = 1; else DEEMP_EN = 0;
	}
	else
	{
		if (AppConfig.dtmfdeemphasis) DEEMP_EN = 1; else DEEMP_EN = 0;
	}
	xtal1x = AppConfig.xtal1x;
	xtal2x = xtal1x * 2.0;
	f0 = xtal1x * (1.0 + MAX_FREQ_ERROR);
	khzmax = (int)f0;
	f0 = xtal1x * (1.0 - MAX_FREQ_ERROR);
	khzmin = (int)f0;

	SPI1CON1 = 0x180; // CKE + Slave Select
	SPI1CON2 = 1;
	SPI1STAT = 4;//8;
	IFS0bits.SPI1IF = 0;
	IEC0bits.SPI1IE = 1;
	_SPI1IP = 6;
	SPI1STATbits.SPIEN = 1;

	TMR2 = 0;
	PR2 = 16000;
	T2CON = 0;
	_T2IF = 0;
	_T2IE = 1;
	T2CONbits.TON = 1;

	TMR1 = 0;
	T1CON = 0x102;
	TMR4 = 0;
	T4CON = 0x12;
	lasttmr1 = 0;
	lasttmr4 = 0;
	T1CONbits.TON = 1;
	T4CONbits.TON = 1;

	INTCON1bits.NSTDIS = 1;

	ENABLE_INTERRUPTS();

	for(i = 0; i < 1000; i++) Nop();

	printf("\nRTCM Simulcast Mezzanine Board version %s\n\n",VERSION);

	UPCLK_EN = 0;
	UPPWR_EN = 0;

	PDTS_527 = 0;
	PDTS_525 = 0;

	div_r = 0;
	div_v = 0;
	div_s = 0;
	div_527 = 0;

	freq_changed = 1;

	while(1)
	{


		lastpoc = POCSAG_ACT;

		if (freq_changed)
		{
			printf("Calculating.....\n\n");
	
			f = ics527_mkdiv(AppConfig.khz);
			if (f < 0.0) f = ics525_mkdiv(AppConfig.khz);
	
			myultoa(AppConfig.khz,a);
			printf("Desired Freq: %s KHz\n",a);
		
			l = f * 1000.0;
			myultoa(l,a);
			strcpy(a1,a);
			i = strlen(a);
			a[i - 3] = 0;
			printf("Actual Freq: %s.%s KHz (%s PRECISION)\n\n",a,a1 + (i - 3),(div_527) ? "MAXIMUM" : "HIGH");
	
			for(i = 0; i < 1000; i++) Nop();  
		}

		freq_changed = 0;

		div_out();
	
		for(i = 0; i < 1000; i++) Nop();

		printf(menu,AppConfig.khz,(double)AppConfig.xtal1x,AppConfig.txlevel,AppConfig.poclevel);
		main_processing_loop();
		printf(menu1,AppConfig.pocmode,AppConfig.dtmfleadin,AppConfig.dtmftimeout,
			AppConfig.dtmfmutemode,AppConfig.dtmfdeemphasis);
		main_processing_loop();
		printf(menu2,AppConfig.dtmftxenable,AppConfig.dtmfrxenable,AppConfig.dtmfrepeatmode,
			AppConfig.dtmfreboot);
		main_processing_loop();
		printf(menu3);
		main_processing_loop();
		aborted = 0;
		while(!aborted)
		{
			printf(entsel);
			memset(cmdstr,0,sizeof(cmdstr));
			if (!myfgets(cmdstr,sizeof(cmdstr) - 1)) continue;
			if (!strchr(cmdstr,'!')) break;
		}
		if (aborted) continue;
		if ((strchr(cmdstr,'R')) || strchr(cmdstr,'r'))
		{
				printf("Re-booting...\n");
				Simul_Reset();
		}
		printf(" \n");
		sel = atoi(cmdstr);
		if ((sel >= 1) && (sel <= 12))
		{
			printf(entnewval);
			if (aborted) continue;
			if ((!myfgets(cmdstr,sizeof(cmdstr) - 1)) || (strlen(cmdstr) < 2))
			{
				if (sel < 10)
				{
					printf(newvalnotchanged);
					continue;
				}
			}
			for(i = 0; cmdstr[i]; i++)
			{
				if (cmdstr[i] < ' ') cmdstr[i] = 0;
			}
			if (aborted) continue;
		}
		ok = 0;
		switch(sel)
		{
			case 1: // ClkGen Freq
				if ((sscanf(cmdstr,"%lu",&l) == 1) && (l >= 2000) && (l <= 200000))
				{
					AppConfig.khz = l;
					freq_changed = 1;
					ok = 1;
				}
				break;
			case 2: // ClkRef Freq
#if 0
				if ((sscanf(cmdstr,"%f",&f0) == 1) && (f0 >= 2000.0) && (f0 <= 100000.0))
				{
					AppConfig.xtal1x = f0;
					xtal1x = f0;
					xtal2x = 2.0 * f0;
					freq_changed = 1;
					ok = 1;
				}
#endif
				break;
			case 3: // Tx level
				if ((sscanf(cmdstr,"%u",&i) == 1) && (i <= 255))
				{
					AppConfig.txlevel = i;
					txpot(i);
					ok = 1;
				}
				break;
			case 4: // POCSAG level
				if ((sscanf(cmdstr,"%u",&i) == 1) && (i < 128))
				{
					AppConfig.poclevel = i;
					pocpot(i);
					ok = 1;
				}
				break;
			case 5: // POCSAG paging mode
				if ((sscanf(cmdstr,"%u",&i) == 1) && (i <= 2))
				{
					AppConfig.pocmode = i;
					ok = 1;
				}
				break;
			case 6: // DTMF lead-in
				if ((sscanf(cmdstr,"%c",&c) == 1) && strchr(dtmfdig,c))
				{
					AppConfig.dtmfleadin = c;
					ok = 1;
				}
				break;
			case 7: // DTMF time-out
				if ((sscanf(cmdstr,"%u",&i) == 1) && (i <= 50000))
				{
					AppConfig.dtmftimeout = i;
					ok = 1;
				}
				break;
			case 8: // DTMF mute mode
				if ((sscanf(cmdstr,"%u",&i) == 1) && (i <= 2))
				{
					AppConfig.dtmfmutemode = i;
					ok = 1;
				}
				break;
			case 9: // DTMF de-emphasis mode
				if ((sscanf(cmdstr,"%u",&i) == 1) && (i <= 2))
				{
					AppConfig.dtmfdeemphasis = i;
					DEEMP_EN = i;
					ok = 1;
				}
				break;
			case 10: // DTMF Tx emable seq
				for(i = 0; cmdstr[i]; i++)
				{
					if (!strchr(dtmfdig,cmdstr[i])) break;
				}
				if (!cmdstr[i])
				{
					strncpy((char*)AppConfig.dtmftxenable,cmdstr,MAXDTMFSTR - 1);
					ok = 1;
				}
				break;
			case 11: // DTMF Rx enable seq
				for(i = 0; cmdstr[i]; i++)
				{
					if (!strchr(dtmfdig,cmdstr[i])) break;
				}
				if (!cmdstr[i])
				{
					strncpy((char *)AppConfig.dtmfrxenable,cmdstr,MAXDTMFSTR - 1);
					ok = 1;
				}
				break;
			case 12: // DTMF repeat mode seq
				for(i = 0; cmdstr[i]; i++)
				{
					if (!strchr(dtmfdig,cmdstr[i])) break;
				}
				if (!cmdstr[i])
				{
					strncpy((char *)AppConfig.dtmfrepeatmode,cmdstr,MAXDTMFSTR - 1);
					ok = 1;
				}
				break;
			case 13: // DTMF reboot seq
				for(i = 0; cmdstr[i]; i++)
				{
					if (!strchr(dtmfdig,cmdstr[i])) break;
				}
				if (!cmdstr[i])
				{
					strncpy((char *)AppConfig.dtmfreboot,cmdstr,MAXDTMFSTR - 1);
					ok = 1;
				}
				break;
			case 98:
				printf("RTCM Simul S/W Ver. %s\n",VERSION);
				main_processing_loop();
				if (!spitimer)
					printf("RTCM not alive (no SPI poll)\n");
				else
					printf("SPI In Status: %ld\n",instatus);
				main_processing_loop();
				printf("POCSAG active: %d\n",POCSAG_ACT);
				main_processing_loop();
				printf("Main TX Inhibit: %d\n",tx_inhibit);
				main_processing_loop();
				printf("Main RX Inhibit: %d\n",rx_inhibit);
				main_processing_loop();
				printf("Main Repeat Inhibit: %d\n",rpt_inhibit);
				main_processing_loop();
				printf("PTT: In: %d, Out: %d\n",PTT_IN,PTT_OUT);
				main_processing_loop();
				printf("DTMF De-emphasis: %d\n",DEEMP_EN);
				main_processing_loop();
				if (lastdtmfbuf[0])
					printf("Last DTMF seq: %c%s\n",AppConfig.dtmfleadin,lastdtmfbuf);
				else
					printf("Last DTMF seq: <NONE>\n");
				printf(paktc);
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1);
				continue;
			case 99:
				write_config();
				printf(saved);
				continue;
			default:
				printf(invalselection);
				continue;

		}
		if (ok) printf(newvalchanged);
		else printf(newvalerror);
	}
}


