/*********************************************************************
 *
 *	Hardware specific definitions
 *
 *********************************************************************
 * FileName:        HardwareProfile.h
 * Dependencies:    None
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder		10/03/06	Original, copied from Compiler.h
 * Ken Hesky            07/01/08    Added ZG2100-specific features
 ********************************************************************/
#ifndef __HARDWARE_PROFILE_H
#define __HARDWARE_PROFILE_H

#include "GenericTypeDefs.h"
#include "Compiler.h"

// Clock frequency value.
// This value is used to calculate Tick Counter value

#define GetSystemClock()	(48000000ul)
#define GetInstructionClock()	(GetSystemClock()/4)	// Should be GetSystemClock()/4 for PIC18
#define GetPeripheralClock()	(GetSystemClock()/4)	// Should be GetSystemClock()/4 for PIC18


// #define DUMPENCREGS

	
#define UARTTX_TRIS			dummy_loc
#define UARTTX_IO			dummy_loc
#define UARTRX_TRIS			dummy_loc
#define UARTRX_IO			dummy_loc

// ENC28J60 I/O pins
#define ENC_RST_TRIS		(TRISBbits.TRISB2)
#define ENC_RST_IO			(LATBbits.LATB2)
#define ENC_CS_TRIS			(TRISBbits.TRISB3)
#define ENC_CS_IO			(LATBbits.LATB3)
#define ENC_SCK_TRIS		(TRISBbits.TRISB1)
#define ENC_SDI_TRIS		(TRISBbits.TRISB0)
#define ENC_SDO_TRIS		(TRISCbits.TRISC7)
#define ENC_SPI_IF			(PIR3bits.SSP2IF)
#define ENC_SSPBUF			(SSP2BUF)
#define ENC_SPISTAT			(SSP2STAT)
#define ENC_SPISTATbits		(SSP2STATbits)
#define ENC_SPICON1			(SSP2CON1)
#define ENC_SPICON1bits		(SSP2CON1bits)
#define ENC_SPICON2			(SSP2CON2)

// 25LC256 I/O pins
#define EEPROM_CS_TRIS		(TRISBbits.TRISB4)
#define EEPROM_CS_IO		(LATBbits.LATB4)
#define EEPROM_SCK_TRIS		(TRISBbits.TRISB1)
#define EEPROM_SDI_TRIS		(TRISBbits.TRISB0)
#define EEPROM_SDO_TRIS		(TRISCbits.TRISC7)
#define EEPROM_SPI_IF		(PIR3bits.SSP2IF)
#define EEPROM_SSPBUF		(SSP2BUF)
#define EEPROM_SPICON1		(SSP2CON1)
#define EEPROM_SPICON1bits	(SSP2CON1bits)
#define EEPROM_SPICON2		(SSP2CON2)
#define EEPROM_SPISTAT		(SSP2STAT)
#define EEPROM_SPISTATbits	(SSP2STATbits)

#define	DISABLE_INTERRUPTS() { 	INTCONbits.GIE = 0; INTCONbits.GIEL = 0; }
#define	ENABLE_INTERRUPTS() { 	INTCONbits.GIE = 1; INTCONbits.GIEL = 1; }



#endif
