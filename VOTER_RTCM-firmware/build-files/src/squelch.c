/*
* squelch.c
*
* Copyright (C) 2011 Stephen A. Rodgers, All Rights Reserved
*
* This file is part of the Sentient Squelch Project (SSP)
*
*   SSP is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.

*   SSP is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*/
// 4-19-2012 Modified by Chuck Henderson, WB9UUS, CAH: to fix a minor bug and to 
// prevent giving long squelch tail for short but full quieting signals.

#include <string.h>
#include "HardwareProfile.h"
#include "TCPIP Stack/TCPIP.h"

// Tunable constants
#define SQOPEN		928		// Squelch open setting
#define CALNOISE	900		// Peak noise amplitude to look for during calibration.
#define HYSTERESIS	24		// Amount of hysteresis to open /close in noisy mode
#ifndef CHUCK_SQUELCH
#define CLOSETIME	70		// # of ticks for max close time
#define FASTSLOWLIMIT	1000		// Fast/Slow limit
#define FASTSLOWTHRESH	16		// Counts demarcating fast and slow squelch mode
#define REOPENHOLDOFF	32		// # of ticks to wait after a strong signal dissapears
#endif
#define ADCFS		1023	// ADC Full scale value
#define ADCMIN		0		// ADC minimum value

// Buffer size

#define NHBSIZE		16		// Noise History buffer size. Must be a power of 2

// States for calibrate and squelch state machines

enum {CLOSED=0, OPEN, TAIL, OPEN_FAST, REOPEN_HOLDOFF};
enum {START=0, DEBOUNCE, SQSET, TEST, DONE, TOO_LOW, TOO_HIGH};
enum {COR_NOCHANGE=0, COR_OFF, COR_ON};

// RAM variables

static BYTE sqstate;				// Squelch state
static BYTE calstate;				// Calibration state

static BYTE looptimer;				// GP loop timer for interrupt state machines
static BYTE noisehead;				// Noise history buffer head
static WORD mavnoise32;  			// Modified average noise over 32 samples
static WORD noisehistory[NHBSIZE];		// Noise history buffer
static WORD sqposm, sqposp;		// Squelch position without, with positive, and with negative hysteresis

// RAM variables intended to be read by other modules
BOOL cor;					// COR output
BOOL sqled;					// Squelch LED output
BOOL write_eeprom_cali;			// Flag to write calibration values back to EEPROM
BYTE noise_gain;			// Noise gain sent to digital pot
WORD caldiode;				// Diode voltage at calibration time (only to be written if 'wvf' is true


/*
* Set input attenuator
*/

#define PROPER_SPICON1  (0x0003 | 0x0520)   /* 16 Bit, 1:1 primary prescale, 8:1 secondary prescale, CKE=1, MASTER mode */

#define ClearSPIDoneFlag()
static inline __attribute__((__always_inline__)) void WaitForDataByte( void )
{
    while ((POT_SPISTATbits.SPITBF == 1) || (POT_SPISTATbits.SPIRBF == 0));
}

#define SPI_ON_BIT          (POT_SPISTATbits.SPIEN)

BOOL set_atten(BYTE val)
{

    volatile BYTE vDummy;
    BYTE vSPIONSave;
    WORD SPICON1Save;

    // Save SPI state
    SPICON1Save = POT_SPICON1;
    vSPIONSave = SPI_ON_BIT;

    // Configure SPI
    SPI_ON_BIT = 0;
    POT_SPICON1 = PROPER_SPICON1;
    SPI_ON_BIT = 1;

    SPISel(SPICS_POT);
    EEPROM_SSPBUF = val;
    WaitForDataByte();
    vDummy = POT_SSPBUF;
    SPISel(SPICS_POT_IDLE);

    // Restore SPI State
    SPI_ON_BIT = 0;
    POT_SPICON1 = SPICON1Save;
    SPI_ON_BIT = vSPIONSave;

	ClearSPIDoneFlag();

	return 1;
}

void init_squelch(void)
{
	write_eeprom_cali = FALSE;
	noise_gain = AppConfig.SqlNoiseGain;
	caldiode = AppConfig.SqlDiode;
	set_atten(noise_gain); // use last gain setting from NVRAM.
	sqstate = 0;
	calstate = 0;
	noisehead = 0;
	mavnoise32 = CALNOISE;
}

void service_squelch(WORD diode,WORD sqpos,WORD noise,BOOL cal,BOOL wvf,BOOL iscaled)
{

	BYTE x;
	WORD noise50msago;
	WORD sqposcomp;
	short int tcomp;

	//output_toggle(DBG1);

	if(looptimer)
		looptimer--;

	if(sqpos < HYSTERESIS + 2 )
		sqpos = HYSTERESIS + 2 ; // The tightest squelch setting has to be gte to hysteresis + 2;

	noisehistory[noisehead] = noise;
	noisehead++;
	noisehead &= (NHBSIZE - 1);

	tcomp = caldiode - diode;	
	sqposcomp = sqpos + tcomp;


	// Calculate the hysteresis band around squelch setting
	sqposm = sqposcomp - HYSTERESIS;
	if(sqposm & 0x8000)
		sqposm = ADCMIN;
	sqposp = sqposcomp + HYSTERESIS;
	if(sqposp > ADCFS)
		sqposp = ADCFS;

	// Calculate long term modified average

	// CHUCK_SQUELCH
	mavnoise32 = ((mavnoise32 * 31) + noise) >> 5; // average over the last 120ms including current noise
	// Take an average of the oldest 2 samples in the history buffer
	// on arival here noisehead points to oldest sample in 16 sample array
	// this averages samples back 13 and 14 from now in buffer to see what the
	// signal was like 50 ms before the unkey??
	x = noisehead + 1;
	x &= (NHBSIZE - 1);
	noise50msago = noisehistory[x++];  // repaired this line to increment x rather than add 1 to noise50msago
	x &= (NHBSIZE - 1);
	noise50msago += noisehistory[x];
	noise50msago >>= 1;

	// *** State machines ***

	if(cal){ // Calibration mode?
		cor = 0;
		switch(calstate){
			case START:
				noise_gain = 0;
				looptimer = 100; // For debouncing
				calstate = DEBOUNCE;
				break;

			case DEBOUNCE:
				if(!looptimer){
					if(cal){ // Double check
						sqled = 0;
						calstate = SQSET;
					}
				}
				break;
			
	
			case SQSET:
				mavnoise32 = 0; // reset modified average
				set_atten(noise_gain); // Set level
				if((noise_gain & 0x07) == 0x07)
					sqled = 1;
				else
					sqled = 0;
				looptimer = 128; // wait
				calstate = TEST;
				break;
					

			case TEST:
				if(!looptimer){
					if(mavnoise32 > CALNOISE){
						if(noise_gain < 8)
							calstate = TOO_HIGH;
						else{
							if(wvf){	
								caldiode = diode;
							}
							write_eeprom_cali = TRUE;
							calstate = DONE; // Done
						}
					}
					else{
						noise_gain++;
						if(noise_gain > 127)
							calstate = TOO_LOW; // Error, not enough noise
						else
							calstate = SQSET; // Continue
					}
				}
				break;

	
			case DONE:
				sqled = 1; // Success
				break;

	
			case TOO_LOW: // Level Too Low
				if(!looptimer){
					sqled ^= 1;
					looptimer = 200;
				}
				break;

			case TOO_HIGH: // Level Too High
				if(!looptimer){
					sqled ^= 1;
					looptimer = 25;
				}
				break;

				
			default:
				calstate = START;
				break;
		}
	}
	else{ // Normal operating mode
		calstate = START;
		// State machine
		switch(sqstate){

			case CLOSED:
				sqled = 0;
				cor = 0;
				if(mavnoise32 < sqposm){
					if (iscaled) cor = 1;
					sqled = 1;
					sqstate = OPEN;
				}
				break;

			case OPEN:
				if (iscaled) cor = 1;
				sqled = 1;
				if(noise50msago < 64) {  // relativly strong signal 50ms ago?
					if(noise >= sqposp){ // but carrier gone right now (due to unkey) (no averaging) then instantly close.
						mavnoise32 = CALNOISE;
						cor = 0;
						sqled = 0;
						sqstate = CLOSED;
					}
				}
				if(mavnoise32 >= sqposp){ // Slowly degrading carrier over 120ms average?
					cor = 0;
					sqled = 0;
					sqstate = CLOSED;
				}
				break;

			default:
				sqstate = CLOSED;
				break;
		}
	}
}

