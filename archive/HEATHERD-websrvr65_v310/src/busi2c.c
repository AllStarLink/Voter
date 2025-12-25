 /**
 * @brief           Interrupt driven Serial receive and transmit handler.
 * @file            busi2c.c
 * @author          <a href="www.modtronix.com">Modtronix Engineering</a>
 * @dependencies    -
 * @compiler        MPLAB C18 v2.10 or higher <br>
 *                  HITECH PICC-18 V8.35PL3 or higher
 * @ingroup         mod_sys_serint
 *
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
 * 2005-09-01, David Hosken (DH):
 *    - Created documentation for existing code
 *********************************************************************/

#define THIS_IS_BUSI2C

#include "projdefs.h"
#include "busi2c.h"


static BYTE txrxCount;


//Get our bus id. This is defined in buses.h. It needed to access TX and RX buffers that are
//defined in the buses module
#define BUSID1 BUSID_I2C1


/**
 * Initialize all I2C modules
 */
void i2cBusInit(void) {
    txrxCount = 0;
}


/**
 * Service routine. Must be regulary called by main program.
 * This fuction checks if the transmit buffer contains any data that has to be transmitted. If it does,
 * it will decode and transmit it via the bus. Any data that has to be read, will be read and added to
 * the receive buffer.
 */
void i2cBusService(void) {
    /*    
    do {
        //We are currently txing or rxing. txrxCount will give number of bytes left to transmit or receive
        if ( (i2c1Stat & (I2C_TXING | I2C_RXING)) != 0) {
    
        }
    } while (txrxCount > 0);
    */
}


/**
 * Resets this module, and empties all buffers.
 */
void i2c1BusReset(void) {
    //Set transmit buffer to empty
    busEmptyTxBuf(BUSID1);

    //Set receive buffer to empty
    busEmptyRxBuf(BUSID1);
}


/**
 * Get the next byte in the RX buffer. Before calling this function, the
 * i2c1IsGetReady() function should be called to check if there is any
 * data available.
 *
 * @return Returns next byte in receive buffer.
 */
BYTE i2c1BusGetByte(void) {
    BYTE c;

    if (busRxBufHasData(BUSID1)) {
        c = busPeekByteRxBuf(BUSID1);
        busRemoveByteRxBuf(BUSID1);
    }
    
    return c;
}


/**
 * Add a byte to the TX buffer.
 *
 * @param c     Byte to write out on the serial port
 */
void i2c1BusPutByte(BYTE c) {
    //Add byte to TX buffer, and update buffer pointers
    busPutByteTxBuf(BUSID1, c);
}


/**
 * Send the ASCII hex value of the given byte to the BUS. It is added to the transmit buffer, and
 * asynchronously transmitted. For example, if c=11, then "0B" will be sent to the Bus
 *
 * @param c     Byte to write out on the bus
 */
void i2c1BusPutByteHex(BYTE c) {
    i2c1BusPutByte( (c > 0x9f) ? ((c>>4)&0x0f)+0x57 : ((c>>4)&0x0f)+'0' );
    i2c1BusPutByte( ((c&0x0f)>9) ? (c&0x0f)+0x57 : (c&0x0f)+'0' );
}


/**
 * Transmit a NULL terminated string. It is added to the transmit buffer, and asynchronously
 * transmitted. The NULL is NOT sent!
 *
 * @param s     Null terminated string to write out on the bus.
 */
void i2c1BusPutString(BYTE* s) {
    char c;

    while((c = *s++)) {
        i2c1BusPutByte(c);
        FAST_USER_PROCESS();
    }
}


/**
 * Transmit a NULL terminated string. It is added to the transmit buffer, and asynchronously
 * transmitted. The NULL is NOT sent!
 *
 * @param str   Null terminated string to write out on the bus.
 */
void i2c1BusPutRomString(ROM char* s) {
    char c;

    while((c = *s++)) {
        i2c1BusPutByte(c);
        FAST_USER_PROCESS();
    }
}
