/**********************************************************************
* © 2007 Microchip Technology Inc.
*
* FileName:        Detconst_c.h
* Dependencies:    Header (.h) files if applicable, see below
* Processor:       dsPIC33Fxxxx
* Compiler:        MPLAB® C30 v3.00 or higher
*
* SOFTWARE LICENSE AGREEMENT:
* Microchip Technology Incorporated ("Microchip") retains all ownership and 
* intellectual property rights in the code accompanying this message and in all 
* derivatives hereto.  You may use this code, and any derivatives created by 
* any person or entity by or on your behalf, exclusively with Microchip's
* proprietary products.  Your acceptance and/or use of this code constitutes 
* agreement to the terms and conditions of this notice.
*
* CODE ACCOMPANYING THIS MESSAGE IS SUPPLIED BY MICROCHIP "AS IS".  NO 
* WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED 
* TO, IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE APPLY TO THIS CODE, ITS INTERACTION WITH MICROCHIP'S 
* PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
*
* YOU ACKNOWLEDGE AND AGREE THAT, IN NO EVENT, SHALL MICROCHIP BE LIABLE, WHETHER 
* IN CONTRACT, WARRANTY, TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY), 
* STRICT LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT, SPECIAL, 
* PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, FOR COST OR EXPENSE OF 
* ANY KIND WHATSOEVER RELATED TO THE CODE, HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN 
* ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWABLE BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO 
* THIS CODE, SHALL NOT EXCEED THE PRICE YOU PAID DIRECTLY TO MICROCHIP SPECIFICALLY TO 
* HAVE THIS CODE DEVELOPED.
*
* You agree that you are solely responsible for testing the code and 
* determining its suitability.  Microchip has no obligation to modify, test, 
* certify, or support the code.
************************************************************************/
#ifndef _DETCONST_C_H_
#define _DETCONST_C_H_

//#include "datatype.h"

#define LEFT_JUSTIFIED         1     // 16 bit input, right shift
#define RIGHT_JUSTIFIED        0     // 14 bit input, no right shift

#define NUM                    100   // mem buffer length
#define FLEN                   80    // length of the input buffer

#define BUFFER_DELAY           5     // during subframe count = 0
#define VALID_DIGIT_FRAME      0     // valid DTMF digit just detected
#define POSSIBLE_DIGIT_FRAME   1     // a valid digit may be present 
#define DIGIT_DETECTED         2
#define PAUSE_AFTER_DIGIT_FRAME 4    //  silence period after the digit
#define NOT_A_DIGIT_FRAME      6     // frame not a part of DTMF digit
#define ERROR_FRAME           -1     // error in frame
#define TONE_SHAPE_TEST        3     // a tone has been detected
                                         // shape test in progress
#define NO                     0     // false flag
#define YES                    1     // true flag

#define CODECBUFLEN            240

typedef struct {
    INT FrameLength;          //PCM frame Length specified
    INT *IN;                  //PCM samples bffer for receiver
    INT *OUT;                 //Buffer for filling modulated samples
} PCMDATASTRUCT;

void INTx_IO_Init(void);
INT DTMF_Detection(INT,INT*,INT*); 
 
#endif

