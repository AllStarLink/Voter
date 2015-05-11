/* 
* VersaPatch
*
* Copyright (C) 2015
* Jim Dixon, WB6NIL <jim@lambdatel.com>
*
* This file is part of the VersaPatch Project 
*
*   VOTER System is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.

*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <dsp.h>
#include "HardwareProfile.h"
#include "UART.h"
#include "XEEPROM.h"
#include "Detconst_c.h"
#include "Dtmf_Detection.h"

// Set Configuration Registers
_FGS( GSS_OFF & GCP_OFF & GWRP_OFF )
_FOSCSEL( FNOSC_PRIPLL & IESO_OFF )
_FOSC( FCKSM_CSDCMD & IOL1WAY_OFF & OSCIOFNC_OFF & POSCMD_XT )
_FWDT( FWDTEN_OFF & WINDIS_OFF & WDTPRE_PR32 & WDTPOST_PS4096)
_FPOR(ALTI2C_OFF & FPWRT_PWR1 )	
_FICD(JTAGEN_OFF & ICS_PGD3 )

#define RINGIN (!_RB1)

#define SYSLED _LATB3
#define PTT _LATA4
#define OFFHOOK _LATB2

#define	BAUD_RATE1 57600

#define	FRAME_SIZE 80
#define	ADCOTHERS 1
#define	ADCCOR 0
#define	NCOLS 75

#define	VOX_ON_DEBOUNCE_COUNT 3
#define	VOX_OFF_DEBOUNCE_COUNT 20
#define	VOX_MAX_THRESHOLD 10000.0
#define	VOX_MIN_THRESHOLD 3000.0
#define	VOX_TIMEOUT_MS 4000
#define	VOX_HOLDOVER_MS 500
#define DTMF_TONE_MS 50
#define	DTMF_PAUSE_MS 50
#define	DTMF_INITIAL_MS 2000

#define	VOX_TIME 250
#define ANTIVOX_TIME 300
#define	DELFRAMES 20
#define RING_TIME 150
#define	DTMF_MAKE_TIME 50
#define	DTMF_BREAK_TIME 50
#define	DIAL_TIME 3000
#define	DIAL_TIME1 7000
#define	COR_TIME 300

#define	COR_THRESHOLD 2048

#define	RINGFREQ 440.0
#define RINGLEVEL 1000
#define	D2LEVEL 5000

#define M_PI       3.14159265358979323846

struct vox {
	float	speech_energy;
	float	noise_energy;
	int	enacount;
	char	voxena;
	char	lastvox;
	int	offdebcnt;
	int	ondebcnt;
} ;

#define	mymax(x,y) ((x > y) ? x : y)
#define	mymin(x,y) ((x < y) ? x : y)

#define	ISDUPLEX (AppConfig.simplexmode == 0)
#define ISVOX (AppConfig.simplexmode == 1)
#define	ISDTMF (AppConfig.simplexmode == 2)

// Application-dependent structure used to contain address information
typedef struct __attribute__((__packed__)) 
{
  BYTE dodeemp;
  BYTE dopreemp;
  BYTE simplexmode;
  DWORD voxtimeout;
  DWORD voxholdover;
  BYTE corinvert;
  WORD corthreshold;
  WORD CWSpeed;
  WORD CWBeforeTime;
  WORD CWAfterTime;
  DWORD DebugLevel;
  BYTE rfu[32];
} APP_CONFIG;

APP_CONFIG AppConfig __attribute__ ((space(dma)));
struct vox myvox;

#ifdef DMWDIAG
unsigned char ulaw_digital_milliwatt[8] = { 0x1e, 0x0b, 0x0b, 0x1e, 0x9e, 0x8b, 0x8b, 0x9e };
BYTE mwp;
#endif

char dtmf_digs[] = "0123456789ABCD*#";

// Private helper functions.
// These may or may not be present in all applications.
static BYTE InitAppConfig(void);
static void InitializeBoard(void);
static void SaveAppConfig(void);

#define CWTONELEN 10

static ROM short cwtone[CWTONELEN] = {
    0,  2407, 3895, 3895,  2407, 0, -2407, -3895, -3895, -2407
};

struct morse_bits
{
    BYTE len;
    BYTE dat;
} ;

static ROM struct morse_bits mbits[] = {
    {0, 0}, /* SPACE */
    {0, 0},
    {6, 18},/* " */
    {0, 0},
    {7, 72},/* $ */
    {0, 0},
    {0, 0},
    {6, 30},/* ' */
    {5, 13},/* ( */
    {6, 29},/* ) */
    {0, 0},
    {5, 10},/* + */
    {6, 51},/* , */
    {6, 33},/* - */
    {6, 42},/* . */
    {5, 9}, /* / */
    {5, 31},/* 0 */
    {5, 30},/* 1 */
    {5, 28},/* 2 */
    {5, 24},/* 3 */
    {5, 16},/* 4 */
    {5, 0}, /* 5 */
    {5, 1}, /* 6 */
    {5, 3}, /* 7 */
    {5, 7}, /* 8 */
    {5, 15},/* 9 */
    {6, 7}, /* : */
    {6, 21},/* ; */
    {0, 0},
    {5, 33},/* = */
    {0, 0},
    {6, 12},/* ? */
    {0, 0},
    {2, 2}, /* A */
    {4, 1}, /* B */
    {4, 5}, /* C */
    {3, 1}, /* D */
    {1, 0}, /* E */
    {4, 4}, /* F */
    {3, 3}, /* G */
    {4, 0}, /* H */
    {2, 0}, /* I */
    {4, 14},/* J */
    {3, 5}, /* K */
    {4, 2}, /* L */
    {2, 3}, /* M */
    {2, 1}, /* N */
    {3, 7}, /* O */
    {4, 6}, /* P */
    {4, 11},/* Q */
    {3, 2}, /* R */
    {3, 0}, /* S */
    {1, 1}, /* T */
    {3, 4}, /* U */
    {4, 8}, /* V */
    {3, 6}, /* W */
    {4, 9}, /* X */
    {4, 13},/* Y */
    {4, 3}  /* Z */
};

ROM struct {
	char c;
	float freq1;
	float freq2;
} dtmfs[] = {
	{'1',697.0,1209.0},
	{'2',697.0,1336.0},
	{'3',697.0,1477.0},
	{'A',697.0,1633.0},
	{'4',770.0,1209.0},
	{'5',770.0,1336.0},
	{'6',770.0,1477.0},
	{'B',770.0,1633.0},
	{'7',852.0,1209.0},
	{'8',852.0,1336.0},
	{'9',852.0,1477.0},
	{'C',852.0,1633.0},
	{'*',941.0,1209.0},
	{'0',941.0,1336.0},
	{'#',941.0,1477.0},
	{'D',941.0,1633.0},
	{0,0,0}
};

/////////////////////////////////////////////////
//Global variables 
BYTE filling_buffer;
WORD fillindex;
BOOL filled;
int audio_rx_buf[2][FRAME_SIZE] __attribute__ ((space(dma)));
BYTE draining_buffer;
WORD drainindex;
BOOL drained;
int audio_tx_buf[2][FRAME_SIZE] __attribute__ ((space(dma)));
BYTE filling_buffer2;
WORD fillindex2;
BOOL filled2;
int audio_rx_buf2[2][FRAME_SIZE] __attribute__ ((space(dma)));
BYTE draining_buffer2;
WORD drainindex2;
BOOL drained2;
int audio_tx_buf2[2][FRAME_SIZE] __attribute__ ((space(dma)));
int delbuf[DELFRAMES][FRAME_SIZE];
BYTE adcstate;
WORD adcothers[ADCOTHERS];
BYTE adcindex;
BYTE mscount;
WORD ledtimer;
DWORD dtmftimer;
DWORD dtmftimer2;
WORD disptimer;
WORD voxtimer;
WORD antivoxtimer;
WORD ringtimer;
DWORD voxtimer1;
WORD cwtimer;
WORD cwtimer1;
WORD dialtimer;
WORD cortimer;
WORD d2timer;
BYTE d2state;
BYTE d2idx;
char d2str[30];
char last_digit;
char last_digit2;
BYTE aborted;
BYTE indisplay;
long discfactor;
long discounterl;
long discounteru;
short amax;
short amin;
WORD apeak;
long de_state;
long pre_state;
short last_val;
short last_val2;
BYTE dtmfres;
DWORD dtmfduration;
BYTE dtmfres2;
DWORD dtmfduration2;
BYTE mute;
BYTE mute2;
BYTE patchon;
BYTE isvox;
BYTE lastvox;
BYTE voxrecover;
long tone_v1;
long tone_v2;
long tone_v3;
long tone_fac;
long tone_v12[2];
long tone_v22[2];
long tone_v32[2];
long tone_fac2[2];
float curtonefreq;
long curtonelevel;
BYTE cwlen;
BYTE cwdat;
char *cwptr;
WORD cwtimer;
BYTE cwidx;
BYTE doind;
char dialstr[30];
WORD dialidx;
BYTE dialed;

char dummy_loc;

dtmfdet_sConfig CF1;           //Handle to containing data for 
                                   //initializing the DTMF Detection algorithm.

dtmfdet_sHandle CH1 __attribute__ ((space(dma)));           //Handle to an instance of DTMF Detection.
dtmfdet_sHandle CH2 __attribute__ ((space(dma)));           //Handle to an instance of DTMF Detection.

void critical_processing_loop(void);

void __attribute__((interrupt, auto_psv)) _ADC1Interrupt(void)
{
WORD index;
long accum;
short saccum;

	CORCONbits.PSV = 1;
	index = ADC1BUF0;
	if (adcstate == 2) // sample other ADC stuff
	{
		adcstate = 0;
		adcothers[adcindex++] = index;
		if(adcindex >= ADCOTHERS) adcindex = 0;
		AD1CHS0 = 0;
		if (mscount++ >= 8) // 1 ms timer
		{
			mscount = 0;
			ledtimer++;
			dtmftimer++;
			dtmftimer2++;
			disptimer++;
			if (voxtimer) voxtimer--;
			voxtimer1++;
			if (antivoxtimer) antivoxtimer--;
			if (ringtimer) ringtimer--;
			if (cwtimer1) cwtimer1--;
			if (dialtimer > 1) dialtimer--;
			if (d2timer) d2timer--;
			if (cortimer) cortimer--;
		}
	}
	else if (adcstate == 1)  // sample audio channel 2 (telephone)
	{
		// turn ADC sample into 16 bit signed-linear value
		saccum = index;
		saccum -= 2048;
		saccum *= 16;
		last_val2 = saccum;
		audio_rx_buf2[filling_buffer2][fillindex2++] = saccum;
		if (fillindex2 >= FRAME_SIZE)
		{
			fillindex2 = 0;
			filling_buffer2 ^= 1;
			filled2 = 1;
			apeak = (long)(amax - amin) / 2;
		}
		AD1CHS0 = adcindex + 2;
		adcstate = 2;
	}
	else // sample audio channel 1 (radio)
	{
		// turn ADC sample into 16 bit signed-linear value
		saccum = index;
		saccum -= 2048;
		saccum *= 16;
		accum = saccum;
		last_val = saccum;
		// process stuff for peak level reading
        if (accum > amax)
        {
            amax = accum;
            discounteru = discfactor;
        }
        else if (--discounteru <= 0)
        {
            discounteru = discfactor;
            amax = (long)((amax * 32700) / 32768L);
        }
        if (accum < amin)
        {
            amin = accum;
            discounterl = discfactor;
        }
        else if (--discounterl <= 0)
        {
            discounterl = discfactor;
            amin = (long)((amin * 32700) / 32768L);
        }
		audio_rx_buf[filling_buffer][fillindex++] = saccum;
		if (fillindex >= FRAME_SIZE)
		{
			fillindex = 0;
			filling_buffer ^= 1;
			filled = 1;
		}
		AD1CHS0 = 1;
		adcstate = 1;
	}
	IFS0bits.AD1IF = 0;
}

void __attribute__((interrupt, auto_psv)) _DAC1LInterrupt(void)
{
short s;
BYTE c;

	CORCONbits.PSV = 1;
	IFS4bits.DAC1LIF = 0;
	s = 0;
	if (PTT && tone_fac)
	{  
      	tone_v1 = tone_v2;
        tone_v2 = tone_v3;
        tone_v3 = (tone_fac * tone_v2 >> 15) - tone_v1;
		s += tone_v3;
	}
	if (PTT && cwptr && (!cwtimer1))
	{
		if (--cwtimer)
		{
			if ((cwlen > 1) && (!(cwlen & 1))) s = cwtone[cwidx++];
			if (cwidx >= CWTONELEN) cwidx = 0;
		}
		else
		{
			cwidx = 0;
			if (cwlen)
			{
				cwlen--;
				cwtimer = AppConfig.CWSpeed;
				if (!(cwlen & 1))
				{
					if (cwlen > 1)
					{
						if (cwdat & 1) cwtimer = AppConfig.CWSpeed * 3;
						cwdat = cwdat >> 1;
					}
				}
				else if (cwlen == 1) 
					cwtimer = AppConfig.CWSpeed * 2;
			}
			else
			{
				cwtimer = 0;
				cwlen = 0;
				cwdat = 0;
				while((c = *cwptr++))
				{
					if (c < ' ') continue;
					if (c > 'Z') continue;
					c -= ' ';
					if (mbits[c].len)
					{
						cwlen = mbits[c].len * 2;
						cwdat = mbits[c].dat;
						if (cwdat & 1) cwtimer = AppConfig.CWSpeed * 3;
						else cwtimer = AppConfig.CWSpeed;
						cwdat = cwdat >> 1;
					}
					else
					{
						cwlen = 1;
						cwdat = 0;
						cwtimer = AppConfig.CWSpeed * 6;
					}
					break;
				}
				if (!cwlen) 
				{
					cwptr = 0;
					cwtimer1 = AppConfig.CWAfterTime;
				}
			}
		}
	}
	DAC1LDAT = s + audio_tx_buf[draining_buffer][drainindex++];
	if (drainindex >= FRAME_SIZE)
	{
		drainindex = 0;
		draining_buffer ^= 1;
		drained = 1;
	}
}

void __attribute__((interrupt, auto_psv)) _DAC1RInterrupt(void)
{
short s;


	CORCONbits.PSV = 1;
	IFS4bits.DAC1RIF = 0;
	s = 0;
	if (tone_fac2[0])
	{
      	tone_v12[0] = tone_v22[0];
        tone_v22[0] = tone_v32[0];
        tone_v32[0] = (tone_fac2[0] * tone_v22[0] >> 15) - tone_v12[0];
		s += tone_v32[0];
	}
	if (tone_fac2[1])
	{
      	tone_v12[1] = tone_v22[1];
        tone_v22[1] = tone_v32[1];
        tone_v32[1] = (tone_fac2[1] * tone_v22[1] >> 15) - tone_v12[1];
		s += tone_v32[1];
	}
	DAC1RDAT = s + audio_tx_buf2[draining_buffer2][drainindex2++];
	if (drainindex2 >= FRAME_SIZE)
	{
		drainindex2 = 0;
		draining_buffer2 ^= 1;
		drained2 = 1;
	}
}


void __attribute__((interrupt, auto_psv)) _DefaultInterrupt(void)
{
   Nop();
   Nop();
}

void __attribute__((interrupt, auto_psv)) _OscillatorFail(void)
{
   Nop();
   Nop();
}
void _ISR __attribute__((__no_auto_psv__)) _AddressError(void)
{
   Nop();
   Nop();
}
void _ISR __attribute__((__no_auto_psv__)) _StackError(void)
{
   Nop();
   Nop();
}
void __attribute__((interrupt, auto_psv)) _MathError(void)
{
   Nop();
   Nop();
}

int myfgets(char *buffer, unsigned int len);

void SimpPatch_Reset(void)
{

	PTT = 0;
	while(!EmptyUART()) ClrWdt();
	Reset();
	while(1) DISABLE_INTERRUPTS();
}

BYTE HasCOR(void)
{
BYTE x;

	x = 0;
	if (adcothers[ADCCOR] >= AppConfig.corthreshold)
		x = 1;
	return(x ^ AppConfig.corinvert);
}

void SetTxTone(float freq, WORD gain)
{
	if ((freq == curtonefreq) &&
		(gain == curtonelevel)) return;

	curtonefreq = freq;
	curtonelevel = gain;

	if ((freq <= 0.0) || (gain < 1))
	{
		tone_fac = 0;
		tone_v1 = 0;
		tone_v2 = 0;
		tone_v3 = 0;
		return;
	}

	tone_v1 = 0;
	// Last previous two samples
	tone_v2 = sin(-4.0 * M_PI * (freq / 8000.0)) * gain;
	tone_v3 = sin(-2.0 * M_PI * (freq / 8000.0)) * gain;
	// Frequency factor
	tone_fac = 2.0 * cos(2.0 * M_PI * (freq / 8000.0)) * 32768.0;
}

void SetTone2(BYTE idx, float freq, WORD gain)
{

	if ((freq <= 0.0) || (gain < 1))
	{
		tone_fac2[idx] = 0;
		tone_v12[idx] = 0;
		tone_v22[idx] = 0;
		tone_v32[idx] = 0;
		return;
	}

	tone_v12[idx] = 0;
	// Last previous two samples
	tone_v22[idx] = sin(-4.0 * M_PI * (freq / 8000.0)) * gain;
	tone_v32[idx] = sin(-2.0 * M_PI * (freq / 8000.0)) * gain;
	// Frequency factor
	tone_fac2[idx] = 2.0 * cos(2.0 * M_PI * (freq / 8000.0)) * 32768.0;
}

static void dial(char *str)
{
	d2idx = 0;
	d2state = 3;
	d2timer = DTMF_INITIAL_MS;
	strncpy(d2str,str,sizeof(d2str) - 1);
}


static void domorse(char *str)
{
BYTE c;

	if (!cwptr)
	{
		while((c = *str++))
		{
			if (c < ' ') continue;
			if (c > 'Z') continue;
			c -= ' ';
			if (mbits[c].len)
			{
				cwlen = mbits[c].len * 2;
				cwdat = mbits[c].dat;
				if (cwdat & 1) cwtimer = AppConfig.CWSpeed * 3;
				else cwtimer = AppConfig.CWSpeed;
				cwdat = cwdat >> 1;
			}
			else
			{
				cwlen = 1;
				cwdat = 0;
				cwtimer = AppConfig.CWSpeed * 6;
			}
			break;
		}
		cwtimer1 = AppConfig.CWBeforeTime;
		cwptr = str;
	}
}

static int dovox(struct vox *v,int *buf,int bs)
{

	int i;
	float	esquare = 0.0;
	float	energy = 0.0;
	float	threshold = 0.0;
	
	if (v->voxena < 0) return(v->lastvox);
	for(i = 0; i < bs; i++)
	{
		esquare += (float) buf[i] * (float) buf[i];
	}
	energy = sqrt(esquare);

	if (energy >= v->speech_energy)
		v->speech_energy += (energy - v->speech_energy) / 4;
	else
		v->speech_energy += (energy - v->speech_energy) / 64;

	if (energy >= v->noise_energy)
		v->noise_energy += (energy - v->noise_energy) / 64;
	else
		v->noise_energy += (energy - v->noise_energy) / 4;
	
	if (v->voxena) threshold = v->speech_energy / 8;
	else
	{
		threshold = mymax(v->speech_energy / 16,v->noise_energy * 2);
		threshold = mymin(threshold,VOX_MAX_THRESHOLD);
	}
	threshold = mymax(threshold,VOX_MIN_THRESHOLD);
	if (energy > threshold)
	{
		if (v->voxena) v->noise_energy *= 0.75;
		v->voxena = 1;
	} else 	v->voxena = 0;
	if (v->lastvox != v->voxena)
	{
		if (v->enacount++ >= ((v->lastvox) ? v->offdebcnt : v->ondebcnt))
		{
			v->lastvox = v->voxena;
			v->enacount = 0;
		}
	} else v->enacount = 0;
	return(v->lastvox);
}


/* Perform standard 6db/octave de-emphasis */
static short deemph(short input,long *state)
{

short coeff00 = 6878;
short coeff01 = 25889;
long accum; /* 32 bit accumulator */

        accum = input;
        /* YES! The parenthesis REALLY do help on this one! */
        *state = accum + ((*state * (long)coeff01) >> 15);
        accum = (*state * (long)coeff00);
        /* adjust gain so that we have unity @ 1KHz */
        return((accum >> 14) + (accum >> 15));
}

/* Perform standard 6db/octave pre-emphasis */
static short preemph(short input,long *state)
{

short coeff00 = 17610;
short coeff01 = -17610;
short adjval = 13404;
long y,temp0,temp1;

        temp0 = *state * (long)coeff01;
        *state = (long)input;
        temp1 = input * (long)coeff00;
        y = (temp0 + temp1) / (long)adjval;
        if (y > 32767) y=32767;
        else if (y <-32767) y=-32767;
        return((short)y);
}

void critical_processing_loop(void)
{
int i,j,x,Digit;
char c;
BYTE toptt;


	if (RINGIN) 
		ringtimer = RING_TIME;
	if (filled && drained && filled2 && drained2)
	{
		memset(&audio_tx_buf[draining_buffer ^ 1][0],0,FRAME_SIZE * sizeof(int));
		memset(&audio_tx_buf2[draining_buffer ^ 1][0],0,FRAME_SIZE * sizeof(int));
		if (AppConfig.dodeemp)
		{
			for(i = 0; i < FRAME_SIZE; i++)
			{
				x = audio_rx_buf[filling_buffer ^ 1][i];
				audio_rx_buf[filling_buffer ^ 1][i] = deemph(x,&de_state);
			}
		}
		x = DTMFDetection(&CH1,&audio_rx_buf[filling_buffer ^ 1][0],&Digit);
		if (x != BUFFER_DELAY)
		{
			if ((x == DIGIT_DETECTED) && (!last_digit))
			{
				c = dtmf_digs[Digit];
				dtmfres = c;
				dtmfduration = 1;
				last_digit = c;
				dtmftimer = 0;
				mute = 1;
				if (dtmfres == '#')
				{
					if (patchon)
					{
						patchon = 0;
						doind = 2;
					}
					if (dialtimer)
						doind = 2;
					d2idx = 0;
					d2str[0] = 0;
					dialtimer = 0;
					dialed = 0;
				}
			}
			else if (last_digit && ((x == NOT_A_DIGIT_FRAME) || (x == ERROR_FRAME)))
			{
				dtmfres = last_digit;
				dtmfduration = dtmftimer;
				last_digit = 0;
				mute = 0;
				if (dtmfres == '*')
				{
					if ((!ISDUPLEX) && dialtimer)
					{
						patchon = 1;
						doind = 3;
						dtmftimer = 0;
						dialed = 1;
						dialidx = 0;
						dialstr[0] = 0;
					}
					else if (!patchon)
					{
						if (ISDUPLEX)
						{
							patchon = 1;
						}
						else if (!dialtimer)
						{
							dialtimer = DIAL_TIME1 + 1;
						}
						doind = 1;
					}
				}
				else if ((!dialed) && (dtmfres >= '0') && (dtmfres <= '9'))
				{
					if (dialtimer > 1)
					{
						if (dialidx < (sizeof(d2str) - 1))
						{
							dialstr[dialidx++] = dtmfres;
							dialstr[dialidx] = 0;
						}
						dialtimer = DIAL_TIME + 1;
					}
				}
			}
		}
		if ((dialtimer == 1) && (!dialed))
		{
			if (dialidx)
			{
				patchon = 1;
				dialed = 1;
				dial(dialstr);
			}
			else
			{
				doind = 2;
			}
			dialtimer = 0;
			dialidx = 0;
			dialstr[0] = 0;
		}
		if ((patchon == 1) && (ISDUPLEX || HasCOR()))
		{
			for(i = 0; i < FRAME_SIZE; i++)
			{
				audio_tx_buf2[draining_buffer2 ^ 1][i] = 
					audio_rx_buf[filling_buffer ^ 1][i];
			}
		}
		// Do everything with the pre-muted audio here
		if (mute)
		{
			memset(&audio_rx_buf[filling_buffer ^ 1][0],0,FRAME_SIZE * sizeof(int));
		}
		if ((patchon != 1) && (ISDUPLEX || HasCOR()))
		{
			for(i = 0; i < FRAME_SIZE; i++)
			{
				audio_tx_buf[draining_buffer ^ 1][i] = 
					audio_rx_buf[filling_buffer ^ 1][i];
			}
		}
		x = DTMFDetection(&CH2,&audio_rx_buf2[filling_buffer2 ^ 1][0],&Digit);
		if (x != BUFFER_DELAY)
		{
			if ((x == DIGIT_DETECTED) && (!last_digit2))
			{
				c = dtmf_digs[Digit];
				dtmfres2 = c;
				dtmfduration2 = 1;
				last_digit2 = c;
				dtmftimer2 = 0;
				mute2 = 1;
			}
			else if (last_digit2 && ((x == NOT_A_DIGIT_FRAME) || (x == ERROR_FRAME)))
			{
				dtmfres2 = last_digit2;
				dtmfduration2 = dtmftimer2;
				last_digit2 = 0;
				mute2 = 0;
			}
		}
		isvox = dovox(&myvox,&audio_rx_buf2[filling_buffer2 ^ 1][0],FRAME_SIZE);
		if (isvox) voxtimer = VOX_TIME;
		if (patchon == 1)
		{
			for(i = 0; i < FRAME_SIZE; i++)
			{
				if (!ISDUPLEX)
				{
					audio_tx_buf[draining_buffer ^ 1][i] = delbuf[0][i];
					for(j = 0; j < DELFRAMES - 1; j++)
					{
						delbuf[j][i] = delbuf[j + 1][i];
					}
					delbuf[j][i] = audio_rx_buf2[filling_buffer2 ^ 1][i];
				}
				else
				{
					audio_tx_buf[draining_buffer ^ 1][i] =
						audio_rx_buf2[filling_buffer2 ^ 1][i];
				}
			}
		}
		if (mute2)
		{
			memset(&audio_rx_buf2[filling_buffer2 ^ 1][0],0,FRAME_SIZE * sizeof(int));
		}
		if (AppConfig.dopreemp)
		{
			for(i = 0; i < FRAME_SIZE; i++)
			{
				x = audio_tx_buf[draining_buffer ^ 1][i];
				audio_tx_buf[draining_buffer ^ 1][i] = preemph(x,&pre_state);
			}
		}
		filled = 0;
		drained = 0;
		filled2 = 0;
		drained2 = 0;
	}
	toptt = 0;
	// PTT handling routines
	if (ISDUPLEX)
	{
		toptt = HasCOR();
	}
	if (ISVOX)
	{
		if (patchon == 1) // if patch is on
		{
			if (HasCOR()) antivoxtimer = ANTIVOX_TIME;
			if (antivoxtimer) voxtimer = 0;
			if (voxtimer) // if vox is active
			{
				if (!voxrecover)
				{
					if (voxtimer1 < AppConfig.voxtimeout)
					{
						if (voxtimer)
							toptt = 1;
					}
					else
					{
						voxrecover = 1;
						voxtimer1 = 0;
					}
				}
				else if (voxtimer1 >= AppConfig.voxholdover)
				{
					voxrecover = 0;
					voxtimer1 = 0;
					if (voxtimer)
						toptt = 1;
				}
			}
			else // Vox is not active
			{
				voxtimer1 = 0;
				voxrecover = 0;
			}
		}
		else // patch is not on
		{
			antivoxtimer = 0;
			voxtimer = 0;
			voxtimer1 = 0;
			voxrecover = 0;
		}
	}
	// Tone handling routines
	if ((!patchon) && ringtimer)
	{
		toptt = 1;
		SetTxTone(RINGFREQ,RINGLEVEL);
	}
	else
	{
		SetTxTone(0,0);
	}

	if (patchon && ISDUPLEX)
		toptt = 1;
	if (cwptr || cwtimer1)
		toptt = 1;
	if (toptt)
		PTT = 1;
	else
		PTT = 0;
	OFFHOOK = ((patchon != 1));
	if (HasCOR())
		cortimer = COR_TIME;
	if (doind && (!cortimer))
	{
		if (doind == 1)
			domorse("I");
		else if (doind == 2)
			domorse("S");
		else
			domorse("H");
		doind = 0;
	}
	// Process channel 2 DTMF encoding
	if (d2str[d2idx])
	{
		if (!d2state) // start tone seq
		{
			for(i = 0; dtmfs[i].c ; i++)
			{
				if (dtmfs[i].c == d2str[d2idx]) break;
			}
			if (dtmfs[i].c)
			{
				d2timer = DTMF_MAKE_TIME;
				SetTone2(0,dtmfs[i].freq1,D2LEVEL);
				SetTone2(1,dtmfs[i].freq2,D2LEVEL);
				d2state = 1;
			}
			else
			{
				d2idx++;
			}
		}
		else if (d2state == 1)
		{
			if (!d2timer)
			{
				d2state = 2;
				d2timer = DTMF_BREAK_TIME;
				SetTone2(0,0.0,0);
				SetTone2(1,0.0,0);
			}
		}
		else if (d2state == 2)
		{
			if (!d2timer)
			{
				d2state = 0;
				d2idx++;
			}
		}
		else if (d2state == 3)
		{
			if (!d2timer)
			{
				d2state = 0;
				d2idx = 0;
			}
		}
	}
	else
	{
		SetTone2(0,0.0,0);
		SetTone2(1,0.0,0);
		d2state = 0;
		d2idx = 0;
		d2str[0] = 0;
		d2timer = 0;
	}
}

void main_processing_loop(void)
{
long meas,thresh;
WORD i;

   // Blink LEDs as appropriate
    if(ledtimer >= 500)
    {
		ledtimer = 0;
		SYSLED ^= 1;
    }
	if (dtmfduration)
	{
		if (AppConfig.DebugLevel & 1)
		{
			if (dtmfduration == 1)
				printf("DTMF digit: %c\n",dtmfres);
			else if (dtmfduration > 1)
				printf("DTMF digit was: %c, len: %ld ms\n",dtmfres,dtmfduration);
		}
		dtmfduration = 0;
	}
	if (dtmfduration2)
	{
		if (AppConfig.DebugLevel & 1)
		{
			if (dtmfduration2 == 1)
				printf("DTMF(2) digit: %c\n",dtmfres2);
			else if (dtmfduration2 > 1)
				printf("DTMF(2) digit was: %c, len: %ld ms\n",dtmfres2,dtmfduration2);
		}
		dtmfduration2 = 0;
	}
	if (!indisplay) disptimer = 0;
	// Rx Level Display handler
    if(indisplay && (disptimer >= 100))
	{
       	disptimer++;
		if (HasCOR())
			meas = apeak;
		else
			meas = 0;
		putchar('|');
        for(i = 0; i < NCOLS; i++)
        {
                thresh = (meas * (long)NCOLS) / 16384;
                if (i < thresh) putchar('=');
                else if (i == thresh) putchar('>');
                else putchar(' ');
        }
		putchar('|');
		putchar('\r');
		fflush(stdout);
		amin = amax = 0;
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
				while(BusyUART()) critical_processing_loop();
				if (*cp == '\n')
				{
					WriteUART('\r');
					while(BusyUART())critical_processing_loop();
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

int read(int handle,void *buffer, unsigned int len)
{
	return 0;
}

int myfgets(char *dest, unsigned int len)
{

BYTE c;
int count,x;

	ClrWdt();

	fflush(stdout);
	aborted = 0;
	count = 0;
	while(count < len)
	{
		dest[count] = 0;
		for(;;)
		{
			ClrWdt();
			if (DataRdyUART())
			{
				c = ReadUART() & 0x7f;
				break;
			}
			critical_processing_loop();
			main_processing_loop();
		}
		if (c == 127) continue;
		if (c == 3) 
		{
			while(BusyUART())  critical_processing_loop();
			WriteUART('^');
			while(BusyUART())  critical_processing_loop();
			WriteUART('C');
			aborted = 1;
			dest[0] = '\n';
			dest[1] = 0;
			count = 1;
			break;
		}
		if ((c == 8) || (c == 21))
		{
			if (!count) continue;
			if (c == 21) x = count;
			else x = 1;
			while(x--)
			{
				count--;
				dest[count] = 0;
				while(BusyUART())  critical_processing_loop();
				WriteUART(8);
				while(BusyUART()) critical_processing_loop();
				WriteUART(' ');
				while(BusyUART()) critical_processing_loop();
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
		while(BusyUART())  critical_processing_loop();
		WriteUART(c);

	}
	while(BusyUART())  critical_processing_loop();
	WriteUART('\r');
	while(BusyUART())  critical_processing_loop();
	WriteUART('\n');
	return(count);
}


int main(void)
{

	WORD sel;
	BYTE i,dw;
	char cmdstr[100];

	ROM char menu[] = "\nSelect the following values to View/Modify:\n\n" 
		"1  - Perform De-Emphasis (from Radio) (%d)\n"
		"2  - Perform Pre-Emphasis (to Radio) (%d)\n"
		"3  - Simplex Mode (0=duplex,1=VOX PTT) (%d)\n"
		"4  - VOX timeout (msec) (%ld)\n"
		"5  - VOX holdover (msec) (%ld)\n"
		"5  - COR Polarity (0=Normal,1=Inverted) (%u)\n"
		"7  - COR Input Threshold (0-4095) (%u)\n"
		"8  - CW Speed (%u) (1/8000 secs)\n"
		"9  - Pre-CW Delay (%u) (msecs)\n"
		"10  - Post-CW Delay (%u) (msecs)\n"
		"11  - Debug Level (%lu)\n"
		"\n";

	ROM char entsel[] = "Enter Selection (1-27,97-99,r,q,d) : ",
		invalselection[] = "Invalid Selection\n", 
		paktc[] = "\nPress The Any Key (Enter) To Continue\n",
		entnewval[] = "Enter New Value : ", 
		newvalchanged[] = "Value Changed Successfully\n",
		saved[] = "Configuration Settings Written to EEPROM\n", 
		newvalerror[] = "Invalid Entry, Value Not Changed\n",
		newvalnotchanged[] = "No Entry Made, Value Not Changed\n",
		booting[] = "System Re-Booting...\n",
		signon[] = "\n\nVersa-Patch ver. %s\n\n",
		rxvoicestr[] = " \rRX VOICE DISPLAY:\n                                  v -- 3KHz        v -- 5KHz\n",
		VERSION[] = "1.1 05/11/2015";;


	// Initialize application specific hardware
	InitializeBoard();

	RCONbits.SWDTEN = 0;

	SYSLED = 1;
	OFFHOOK = 1;
	PTT = 0;

	// Initialize Stack and application related NV variables into AppConfig.
	dw = InitAppConfig();

	U1MODE = 0x8000;			// Set UARTEN.  Note: this must be done before setting UTXEN

	U1STA = 0x0400;		// UTXEN set

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

	InitUARTS();

	// "zero" all the system-wide variables
	filling_buffer = 0;
	fillindex = 0;
	filled = 0;
	memset(audio_rx_buf,0,sizeof(audio_rx_buf));
	draining_buffer = 0;
	drainindex = 0;
	drained = 0;
	memset(audio_tx_buf,0,sizeof(audio_tx_buf));
	filling_buffer2 = 0;
	fillindex2 = 0;
	filled2 = 0;
	memset(audio_rx_buf2,0,sizeof(audio_rx_buf2));
	draining_buffer2 = 0;
	drainindex2 = 0;
	drained2 = 0;
	memset(audio_tx_buf2,0,sizeof(audio_tx_buf2));
	memset(delbuf,0,sizeof(delbuf));
	adcstate = 0;
	memset(adcothers,0,sizeof(adcothers));
	adcindex = 0;
	mscount = 0;
	ledtimer = 0;
	dtmftimer = 0;
	dtmftimer2 = 0;
	disptimer = 0;
	voxtimer = 0;
	antivoxtimer = 0;
	ringtimer = 0;
	voxtimer1 = 0;
	cwtimer = 0;
	cwtimer1 = 0;
	dialtimer = 0;
	cortimer = 0;
	d2timer = 0;
	d2state = 0;
	d2idx = 0;
	memset(d2str,0,sizeof(d2str));
	last_digit = 0;
	last_digit2 = 0;
	aborted = 0;
	indisplay = 0;
	discfactor = 0;
	discounterl = 0;
	discounteru = 0;
	amax = 0;
	amin = 0;
	apeak = 0;
	de_state = 0;
	pre_state = 0;
	last_val = 0;
	last_val2 = 0;
	dtmfres = 0;
	dtmfduration = 0;
	dtmfres2 = 0;
	dtmfduration2 = 0;
	mute = 0;
	mute2 = 0;
	patchon = 0;
	isvox = 0;
	lastvox = 0;
	voxrecover = 0;
	tone_v1 = 0;
	tone_v2 = 0;
	tone_v3 = 0;
	tone_fac = 0;
	memset(tone_v12,0,sizeof(tone_v12));
	memset(tone_v22,0,sizeof(tone_v22));
	memset(tone_v32,0,sizeof(tone_v32));
	memset(tone_fac2,0,sizeof(tone_fac2));
	curtonefreq = 0;
	curtonelevel = 0;
	cwlen = 0;
	cwdat = 0;
	cwptr = 0;
	cwtimer = 0;
	cwidx = 0;
	doind = 0;
	memset(dialstr,0,sizeof(dialstr));
	dialidx = 0;
	dialed = 0;

	discfactor = 1000;

	ClrWdt();
	RCONbits.SWDTEN = 1;
	ClrWdt();

    CF1.DTMFframeType = NOT_A_DIGIT_FRAME;               //Init frametype
    CF1.DTMFshapeTest =  YES;                            //Disable the Shape Test 
    CF1.DTMFcurrentDigit = CURRENT_DIGIT;                //Init 20 as the Current Digit
    CF1.DTMFdeclaredDigit = DECLARED_DIGIT;              //Init 30 as the prev detected digit
    CF1.DTMFinputType = LEFT_JUSTIFIED;

	DTMFDetInit(&CH1,&CF1);
	DTMFDetInit(&CH2,&CF1);

	myvox.speech_energy = 0.0;
	myvox.noise_energy = 0.0;
	myvox.enacount = 0;
	myvox.voxena = 0;
	myvox.lastvox = 0;
	myvox.ondebcnt = VOX_ON_DEBOUNCE_COUNT;
	myvox.offdebcnt = VOX_OFF_DEBOUNCE_COUNT;

	printf(signon,VERSION);

	if (dw)
		printf("Default config values written\n");

	if (AppConfig.DebugLevel & 2)
		domorse("HI");

    while(1) 
	{
		char ok;
		unsigned int i1;
		unsigned long l;

		printf(menu,AppConfig.dodeemp,AppConfig.dopreemp,AppConfig.simplexmode,
			AppConfig.voxtimeout,AppConfig.voxholdover,AppConfig.corinvert,AppConfig.corthreshold,
			AppConfig.CWSpeed,AppConfig.CWBeforeTime,AppConfig.CWAfterTime,AppConfig.DebugLevel);

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
				printf(booting);
				SimpPatch_Reset();
		}
		sel = atoi(cmdstr);
		if ((sel >= 1) && (sel <= 11)) 
		{
			printf(entnewval);
			if (aborted) continue;
			if ((!myfgets(cmdstr,sizeof(cmdstr) - 1)) || ((strlen(cmdstr) < 2) && (sel != 15)))
			{
				printf(newvalnotchanged);
				continue;
			}
			if (aborted) continue;
		}
		ok = 0;
		switch(sel)
		{
			case 1: // De-emphasis
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 1))
				{
					AppConfig.dodeemp = i1;
					ok = 1;
				}
				break;
			case 2: // Pre-emphasis
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 1))
				{
					AppConfig.dopreemp = i1;
					ok = 1;
				}
				break;
			case 3: // Simplex mode
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 1))
				{
					AppConfig.simplexmode = i1;
					ok = 1;
				}
				break;
			case 4: // Vox Timeout (ms)
				if ((sscanf(cmdstr,"%lu",&l) == 1) && (l >= 100) && (l <= 200000))
				{
					AppConfig.voxtimeout = l;
					ok = 1;
				}
				break;
			case 5: // Vox Holdover (ms)
				if ((sscanf(cmdstr,"%lu",&l) == 1) && (l >= 100) && (l <= 30000))
				{
					AppConfig.voxholdover = l;
					ok = 1;
				}
				break;
			case 6: // COR Polarity
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 1))
				{
					AppConfig.corinvert = i1;
					ok = 1;
				}
				break;
			case 7: // COR Threshold
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 4095))
				{
					AppConfig.corthreshold = i1;
					ok = 1;
				}
				break;
			case 8: // CW Speed
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 >= 100))
				{
					AppConfig.CWSpeed = i1;
					ok = 1;
				}
				break;
			case 9: // Pre-CW Delay
				if (sscanf(cmdstr,"%u",&i1) == 1) 
				{
					AppConfig.CWBeforeTime = i1;
					ok = 1;
				}
				break;
			case 10: // Post-CW Delay
				if (sscanf(cmdstr,"%u",&i1) == 1) 
				{
					AppConfig.CWAfterTime = i1;
					ok = 1;
				}
				break;
			case 11: // Debug Level
				if (sscanf(cmdstr,"%lu",&l) == 1)
				{
					AppConfig.DebugLevel = l;
					ok = 1;
				}
				break;
			case 86:		
			    XEEBeginWrite(0x0000);
			    XEEWrite(0xFF);
			    XEEEndWrite();
				printf(booting);
				SimpPatch_Reset();
				break;
			case 97: // Display RX Level Quasi-Graphically  
			 	putchar(' ');
		      	for(i = 0; i < NCOLS; i++) putchar(' ');
		        printf(rxvoicestr);
				indisplay = 1;
				myfgets(cmdstr,sizeof(cmdstr) - 1);
				indisplay = 0;
				continue;
			case 90:
				domorse("TEST");
				continue;
			case 91:
				dial("2361514");
				continue;
			case 98:
				printf("\nS/W Version: %s\n",VERSION);
				printf("COR: %d\n",HasCOR());
				printf("PTT: %d\n",PTT);
				printf("OffHook: %d, dialing: %d, dialed: %d\n",patchon,((dialtimer > 1)),dialed);
				printf("\n%s",paktc);
				myfgets(cmdstr,sizeof(cmdstr) - 1);
				continue;
			case 99:
				SaveAppConfig();
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


/****************************************************************************
  Function:
    static void InitializeBoard(void)

  Description:
    This routine initializes the hardware.  It is a generic initialization
    routine for many of the Microchip development boards, using definitions
    in HardwareProfile.h to determine specific initialization.

  Precondition:
    None

  Parameters:
    None - None

  Returns:
    None

  Remarks:
    None
  ***************************************************************************/
static void InitializeBoard(void)
{	

	// Crank up the core frequency Fosc = 76.8MHz, Fcy = 38.4MHz
	PLLFBD = 30;				// Multiply by 32 for 153.6MHz VCO output (9.6MHz XT oscillator)
	CLKDIV = 0x0000;			// FRC: divide by 2, PLLPOST: divide by 2, PLLPRE: divide by 2
	ACLKCON = 0x780;


//_________________________________________________________________
	DISABLE_INTERRUPTS();

	T3CON = 0x10;			//TMR3 divide Fosc by 8
	PR3 = 199;				//Set to period of 200 (for 24 ks/s)
	T3CONbits.TON = 1;		//Turn it on

	AD1PCFGL = 0xFFF8;				// Enable AN0-AN2 as analog

	AD1CON1 = 0x8444;			// TMR Sample Start, 12 bit mode (on parts with a 12bit A/D)
	AD1CON2 = 0x0;			// AVdd, AVss, int every conversion, MUXA only, no scan
	AD1CON3 = 0x1003;			// 16 Tad auto-sample, Tad = 3*Tcy
	AD1CON1bits.ADON = 1;		// Turn On

	IFS0bits.AD1IF = 0;
	IEC0bits.AD1IE = 1;
	IPC3bits.AD1IP = 6;

	DAC1DFLT = 0x8000;
	DAC1STAT = 0;
	DAC1STATbits.ROEN = 1;
	DAC1STATbits.LOEN = 1;
	DAC1STATbits.LITYPE = 0;
	DAC1STATbits.RITYPE = 0;
	DAC1CON = 0x1100 + 74;		// Divide by 75 for 8K Samples/sec
	IFS4bits.DAC1LIF = 0;
	IFS4bits.DAC1RIF = 0;
	IEC4bits.DAC1LIE = 1;
	IPC19bits.DAC1LIP = 6;
	IEC4bits.DAC1RIE = 1;
	IPC19bits.DAC1RIP = 5;
	DAC1CONbits.DACEN = 1;

	LATA=0xFFEF;
	LATB=0xF483;	
	TRISA = 0xFFEF; // RA4 is PTT
	//RB2-4 are outputs, RB5-6 are Programming pins, RB7 is Serial in, RB8 is RP8/SCK,
	//RB9 is RP9/SDO, RB10 is RP10/SDI, RB11 is RP11/U1TX, RB12-15 are DAC outputs
	TRISB = 0xF483;	

	__builtin_write_OSCCONL(OSCCON & ~0x40); //clear the bit 6 of OSCCONL to unlock pin re-map
	_U1RXR = 7;	// RP7 is UART1 RX
	_RP11R = 3;		// RP11 is UART1 TX (U1TX)
	_SDI1R = 10;	// RP10 is SPI1 MISO
	_RP8R = 8;		// RP8 is SPI1 CLK (SCK1OUT) 8
	_RP9R = 7;		// RP9 is SPI1 MOSI (SDO1) 7
	__builtin_write_OSCCONL(OSCCON | 0x40); //set the bit 6 of OSCCONL to lock pin re-map

	XEEInit();

	INTCON1bits.NSTDIS = 1;

	ENABLE_INTERRUPTS();

}

static void SaveAppConfig(void)
{
    XEEBeginWrite(0x0000);
    XEEWrite(0x69);
    XEEWriteArray((BYTE*)&AppConfig, sizeof(AppConfig));
}

static BYTE InitAppConfig(void)
{
BYTE c,rv;

	rv = 0;
	memset(&AppConfig,0,sizeof(AppConfig));
	AppConfig.voxtimeout = VOX_TIMEOUT_MS;
	AppConfig.voxholdover = VOX_HOLDOVER_MS;
	AppConfig.corinvert = 1;
	AppConfig.corthreshold = COR_THRESHOLD;
	AppConfig.CWSpeed = 400;
	AppConfig.CWBeforeTime = 500;
	AppConfig.CWAfterTime = 500;
	XEEReadArray(0x0000, &c, 1);
    if(c == 0x69u)
	    XEEReadArray(0x0001, (BYTE*)&AppConfig, sizeof(AppConfig));
    else
	{
        SaveAppConfig();
		rv = 1;
	}
	return(rv);
}


