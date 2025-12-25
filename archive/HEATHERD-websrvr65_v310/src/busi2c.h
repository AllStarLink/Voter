/**
 * @brief           Part of the Modtronix Configurable Buses. This modules uses buffers from
 *                  the buses module for it's transmit and receive buffers.
 * @file            busudp.h
 * @author          <a href="www.modtronix.com">Modtronix Engineering</a>
 * @dependencies    -
 * @compiler        MPLAB C18 v3.21 or higher
 * @ingroup         mod_buses
 *
 *
 * @section description Description
 **********************************
 * This module makes a I2C master bus available for writing and reading data to
 * and from. The bus is configured at 400kbits/sec.
 *
 * All data written to the transmit buffer is transmitted. A special escape
 * character 0xFA is used for sending commands. To send an actual 0xFA byte,
 * two 0xFA bytes are send after each other. The commands are:
 * - 0xFA, 0x00 = Send start bit
 * - 0xFA, 0x01 = Send stop bit
 * - 0xFA, 0x02 = Send repeated start bit
 * - 0xFA, 0x10-7F = Send (cmd byte - 0x10) bytes. A maximum of 111 bytes can be sent.
 *   For example, if the command byte is 0x15, we must send 0x15-0x10=5 bytes.
 *   The bytes to send will follow this command.
 * - 0xFA, 0x80-EF = Read (cmd byte - 0x80) bytes. A maximum of 111 bytes can be read.
 *   For example, if the command byte is 0x85, we must read 0x85-0x80=5 bytes.
 * @subsection busi2c1_conf Configuration
 *****************************************
 * The following defines are used to configure this module, and should be placed
 * in the projdefs.h (or similar) file.
 * For details, see @ref mod_conf_projdefs "Project Configuration".
 * To configure the module, the required
 * defines should be uncommended, and the rest commented out.
 @code
 //*********************************************************************
 //-------------- BusI2C1 Configuration --------------------
 //*********************************************************************

 @endcode
 *********************************************************************/

 /*********************************************************************
 * Software License Agreement
 *
 * The software supplied herewith is owned by Modtronix Engineering, and is
 * protected under applicable copyright laws. The software supplied herewith is
 * intended and supplied to you, the Company customer, for use solely and
 * exclusively on products manufactured by Modtronix Engineering. The code may
 * be modified and can be used free of charge for commercial and non commercial
 * applications. All rights are reserved. Any use in violation of the foregoing
 * restrictions may subject the user to criminal sanctions under applicable laws,
 * as well as to civil liability for the breach of the terms and conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES, WHETHER EXPRESS,
 * IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE
 * COMPANY SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 **********************************************************************
 * File History
 *
 * 2008-07-18, David Hosken (DH):
 *    - Initial version
 *********************************************************************/

/**
 * @defgroup mod_bus_busi2c1 Buses
 * @ingroup mod_bus
 *********************************************************************/
#ifndef _BUSI2C_H_
#define _BUSI2C_H_

#include "buses.h"

/////////////////////////////////////////////////
//Global defines


/////////////////////////////////////////////////
//i2c1Stat defines
#define I2C1_TXBUF_OVERRUN    0x01    //The transmit buffer had overrun. Must be cleared by the user
#define I2C1_RXBUF_OVERRUN    0x10    //The receive buffer had overrun. Must be cleared by the user

//The given I2C channel is currently transmitting. The txrxCount byte will give number of bytes left to tx
#define I2C_TXING  0x08

//The given I2C channel is currently receiving. The txrxCount byte will give number of bytes left to tx
#define I2C_RXING  0x80


/////////////////////////////////////////////////
//Global data
#if !defined(THIS_IS_BUSI2C)
extern BYTE i2c1Stat;
#endif

/////////////////////////////////////////////////
//Function prototypes


/**
 * Initialize all I2C modules
 */
void i2cBusInit(void);


/**
 * Service all I2C modules
 */
void i2cBusService(void);


/**
 * Resets this module, and empties all buffers.
 */
void i2c1BusReset(void);


/**
 * Are there any bytes in the receive buffer.
 * @return 0 if not empty, 1 if empty
 */
#define i2c1BusRxBufEmpty() busIsRxBufEmpty(BUSID_I2C1)


/**
 * Are there any bytes in the I2C1 Bus receive buffer.
 * @return 1 if true, else 0
 */
#define i2c1BusIsGetReady() busRxBufHasData(BUSID_I2C1)

/**
 * Get the next byte in the RX buffer. Before calling this function, the
 * i2c1IsGetReady() function should be called to check if there is any
 * data available.
 *
 * @return Returns next byte in receive buffer.
 */
BYTE i2c1BusGetByte(void);


/**
 * Add a byte to the TX buffer.
 *
 * @param c     Byte to write out on the serial port
 */
void i2c1BusPutByte(BYTE c);


/**
 * Send the ASCII hex value of the given byte to the BUS. It is added to the transmit buffer, and
 * asynchronously transmitted. For example, if c=11, then "0B" will be sent to the Bus
 *
 * @param c     Byte to write out on the bus
 */
void i2c1BusPutByteHex(BYTE c);


/**
 * Transmit a NULL terminated string. It is added to the transmit buffer, and asynchronously
 * transmitted. The NULL is NOT sent!
 *
 * @param s     Null terminated string to write out on the bus.
 */
void i2c1BusPutString(BYTE* s);


/**
 * Transmit a NULL terminated string. It is added to the transmit buffer, and asynchronously
 * transmitted. The NULL is NOT sent!
 *
 * @param str   Null terminated string to write out on the bus.
 */
void i2c1BusPutRomString(ROM char* str);


#endif    //_BUSI2C_H_




