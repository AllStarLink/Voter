/***************************************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        DtmfGen.s
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
***************************************************************************************/
    .include "Detconst.inc"
 
; -----------------------------------------------------------------------------
;  Description:   This function initializes parameters for DTMF detection.
; -----------------------------------------------------------------------------

    .global _DTMFDetInit
    .section  .text
   
_DTMFDetInit:

    bclr.b     CORCON, #5

    mov  w0, w5
 
    clr  w4
 
    clr  w3
 
    repeat #99            ;DTMF working Buffer
    mov    w4, [w0++] 
    mov   [w1++], [w0++]  ; Init frametype
    mov   [w1++], [w0++]  ; Disable the Shape Test
    mov   [w1++], [w0++]  ; Init the Current Digit
    mov   [w1++], [w0++]  ; Init the prev detected digit
    mov   w4,[w0++]	      ;counter for subframes
    mov   w4,[w0++]	      ;counter for silence
    mov   [w1++], [w0++]  ; Input Type
    mov   w4,[w0++]	      ;frame
    mov   w4,[w0++]	      ;Energy Threshold
    mov   w4,[w0++]	      ;Energy Threshold
    mov   w4,[w0++]	      ;Silence Threshold
    mov   w4,[w0++]	      ;Silence Threshold

    return
 
;******************************************************************************
;  DTMFDetection:                                                             *
;  ======================                                                     *
;  Description: This Function detects the Valid DTMF digit.                   *
;                                                                             *
;  Input:  w0 = State Stucture                                                     *
;          w1 = Buffer address                                                *
;          w2 = Address of digitdetect variable                               *
;                                                                             *
;  Output: w0 = process type                                                  *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2,w4,w6,w9,w10      used not restored                              *
;                                                                             *
;  Functions Called:  MemCopy, FrameProcess                                   *
;                                                                             *
;******************************************************************************

    .global _DTMFDetection
    .section  .text

_DTMFDetection:

    push       w2                      ;save the address of digitdetect

    
    mov       w0,w7
     push      w7

    CLR W4

    mov  #DTMFworkBuff, w4

    repeat     #99
    mov   [w0++], [w4++]
    mov  #DTMFframeType,w4
    mov [w0++], [w4]
    mov  #DTMFshapeTest,w4
    mov [w0++], [w4]
    mov  #DTMFcurrentDigit,w4
    mov [w0++], [w4]
    mov  #DTMFdeclaredDigit,w4
    mov [w0++], [w4]
    mov  #DTMFsubframeCount,w4
    mov [w0++],[w4]
    mov  #DTMFsilenceCount,w4
    mov [w0++], [w4]
    mov  #DTMFinputType,w4
    mov [w0++], [w4]
    mov  #DTMFframe,w4
    mov [w0++], [w4]
    mov  #DTMFEnergyThreshold,w4
    mov [w0++], [w4]
    mov  #DTMFEnergyThreshold+2,w4
    mov [w0++], [w4]
    mov  #DTMFSilenceThreshold,w4
    mov [w0++], [w4]
    mov  #DTMFSilenceThreshold+2,w4
    mov [w0], [w4]
  
   clr w0

   mov  DTMFinputType, w3

   mov w3,w0

    clr  w4
    clr  w3
   
    cp0        DTMFsubframeCount       ;chk subframecount
    bra        nz, APPEND_20SAMP

    mov        w1, w0
    mov        #DTMFworkBuff, w1       ;Address of the working buffer
    mov        #80, w2                 ;Loop count

    call       _MemCopy                ;copy the samples into working buffer

    mov        #BUFFER_DELAY, w0       ;Return Buffer Delay
    push       w0

    mov        #1, w5
    mov        w5, DTMFsubframeCount   ;Increment the subframe count

    bra        EXIT_DTMFDETECT

APPEND_20SAMP:

    mov       DTMFsubframeCount, w9
    cp        w9, #1                   ;Chk if subframecount is 1
    bra        nz, APPEND_40SAMP

    mov        w0, w4 
    mov        w1, w0
    push       w1

    mov        #160, w6                ;offset to the working buffer
    mov        #DTMFworkBuff, w1
    add        w1, w6, w1
    mov        #20, w2                 ;Copy 20 samples into working buffer

    call       _MemCopy                ;Buffer is now contains 100 samples
                                       ;and can be sent for frame processing

    mov        #LEFT_JUSTIFIED, w0
    cp         w4, w0                  ;Chk input type
    bra        nz, PROCESS_FRAME
    mov        #DTMFworkBuff, w10
    
    do         #99, END_SHIFT          ;If left justified, do right justiified
    mov        [w10], w6               ;for all samples of the buffer
    asr        w6, #2, w6
    mov        w6, [w10++]
END_SHIFT:
    nop
PROCESS_FRAME:
    call       _FrameProcess           ;Process the frame of 100 samples
    pop        w1
    mov        #40, w6
    push       w0                      ;save the process type
    add        w1, w6, w0              ;Update the input buffer
    mov        #DTMFworkBuff, w1
    mov        #60, w2                 ;Loop count
    
    call       _MemCopy                ;copy the samples to working buffer
        
    mov        #2, w0
    mov        w0, DTMFsubframeCount   ;Increment the subframecount 

    bra        EXIT_DTMFDETECT

APPEND_40SAMP:

    mov       DTMFsubframeCount, w9
    cp        w9, #2                    ;Chk if subframecount is 2
    bra        nz, APPEND_60SAMP

    mov        w0, w4
    mov        w1, w0
    push       w1

    mov        #120, w6
    mov        #DTMFworkBuff, w1
    add        w1, w6, w1              ;Add the offset to the working buffer
    mov        #40, w2                 ;Loop count

    call       _MemCopy                ;Copy the samples into working buffer

    mov        #LEFT_JUSTIFIED, w0
    cp         w4, w0                  ;Chk input type
    bra        nz, PROCESS_FRAME1
    mov        #DTMFworkBuff, w10
    
    do         #99, END_SHIFT1         ;If left justified, do right justiified
    mov        [w10], w6               ;for all samples of the buffer
    asr        w6, #2, w6

    mov        w6, [w10++]
END_SHIFT1:
    nop
        
PROCESS_FRAME1:
    call       _FrameProcess           ;Process the frame of 100 samples
    pop        w1
    mov        #80, w6
    push       w0
    add        w1, w6, w0              ;Update the input buffer
    mov        #DTMFworkBuff, w1
    mov        #40, w2
    
    call       _MemCopy                ;Copy the samples for next processing
        
    mov        #3, w0
    mov        w0, DTMFsubframeCount   ;Increment the subframecount

    bra        EXIT_DTMFDETECT
    
APPEND_60SAMP:

    mov       DTMFsubframeCount, w9
    cp        w9, #3                   ;Chk if subframecount is 3
    bra        nz, APPEND_80SAMP

    mov        w0, w4
    mov        w1, w0
    push       w1

    mov        #80, w6
    mov        #DTMFworkBuff, w1
    add        w1, w6, w1              ;Add the offset to the working buffer
    mov        #60, w2                 ;Loop count

    call       _MemCopy                ;Copy the samples into working buffer

    mov        #LEFT_JUSTIFIED, w0
    cp         w4, w0                  ;Chk input type
    bra        nz, PROCESS_FRAME2
    mov        #DTMFworkBuff, w10
    
    do         #99, END_SHIFT2         ;If left justified, do right justiified
    mov        [w10], w6               ;for all samples of the buffer
    asr        w6, #2, w6
    mov        w6, [w10++]
END_SHIFT2:
    nop

PROCESS_FRAME2:
    call       _FrameProcess           ;Process the frame of 100 samples
    pop        w1
    mov        #120, w6
    push       w0
    add        w1, w6, w0              ;Update the input buffer
    mov        #DTMFworkBuff, w1
    mov        #20, w2
    
    call       _MemCopy                ;Copy the samples for next processing
        
    mov        #4, w0
    mov        w0, DTMFsubframeCount   ;Increment subframecount

    bra        EXIT_DTMFDETECT

APPEND_80SAMP:

    mov       DTMFsubframeCount, w9
    cp        w9, #4                   ;Chk if subframecount is 4
    bra        nz, APPEND_80SAMP

    mov        w0, w4
    mov        w1, w0
    push       w1

    mov        #40, w6
    mov        #DTMFworkBuff, w1
    add        w1, w6, w1              ;Add the offset to the working buffer
    mov        #80, w2                 ;Loop count

    call       _MemCopy                ;Copy the samples into working buffer

    mov        #LEFT_JUSTIFIED, w0
    cp         w4, w0                  ;Chk input type
    bra        nz, PROCESS_FRAME3
    mov        #DTMFworkBuff, w10
    
    do         #99, END_SHIFT3         ;If left justified, do right justiified
    mov        [w10], w6               ;for all samples of the buffer
    asr        w6, #2, w6

    mov        w6, [w10++]
END_SHIFT3:
    nop
        
PROCESS_FRAME3:
    call       _FrameProcess           ;Process the frame of 100 samples
    pop        w1
    push       w0
    clr        DTMFsubframeCount       ;Reset the subframecount

EXIT_DTMFDETECT:
    pop        w0                      ;Return the process type
    pop        w7
    pop        w2
    mov        DTMFcurrentDigit, w1
    mov        w1, [w2]                ;Save the current digit as digit detect
    
   mov #DTMFworkBuff,w4
   repeat  #99
   mov [w4++],[w7++]

   mov  #DTMFframeType,w4
   mov [w4],[w7++]
   mov #DTMFshapeTest,w4
   mov [w4],[w7++]
   mov #DTMFcurrentDigit,w4
   mov [w4],[w7++]
   mov #DTMFdeclaredDigit,w4
   mov [w4],[w7++]
   mov #DTMFsubframeCount,w4
   mov [W4],[w7++]
   mov #DTMFsilenceCount,w4
   mov [W4],[w7++]
   mov #DTMFinputType,w4
   mov [w4],[w7++]
   mov #DTMFframe,w4
   mov [W4],[w7++]
   mov #DTMFEnergyThreshold,w4
   mov [W4],[w7++]
   mov #DTMFEnergyThreshold+2,w4
   mov [W4],[w7++]
   mov #DTMFSilenceThreshold,w4
   mov [W4],[w7++]
   mov #DTMFSilenceThreshold+2,w4
   mov [W4],[w7]

    return
; -----------------------------------------------------------------------------
;    END OF FILE
; -----------------------------------------------------------------------------

