/***************************************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        Goertzel.s
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
    .include "Dettables.inc"

   .section nbss,bss

    DTMFSilenceThreshold: .space 4     ;Silence Threshold
    DTMFEnergyThreshold:  .space 4     ;Energy Threshold

    .align 2

    DTMFworkBuff:         .space 200   ;DTMF working Buffer
    DTMFframeType:        .space 2     ;frame type
    DTMFshapeTest:        .space 2     ;Enable/Disable Shape Test
    DTMFcurrentDigit:     .space 2     ;Current Digit detected
    DTMFdeclaredDigit:    .space 2     ;Digit stored in previous frame
    DTMFsubframeCount:    .space 2     ;counter for subframes
    DTMFsilenceCount:     .space 2     ;counter for silence
    DTMFinputType:        .space 2     ;input pattern type
    DTMFframe:            .space 2     ;frame

;******************************************************************************
;  energy:                                                                    *
;  ======================                                                     *
;  Description: This function computes the energy of the signal               *
;                                                                             *
;  Input:  w0 = Address of DTMF working Buffer                                *
;                                                                             *
;  Output: w0 w1 = signalEnergy                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w4,w8            used not restored                                  *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************
    .global _energy
    .section  .text

_energy:

    push       CORCON
    mov        #0x0084, w1
    mov        w1, CORCON
    mov        w0, w8 
    nop       
    clr        A, [w8]+=2, w4          ;Clear ACC A and prefetch the samples  
    repeat     #(NUM-1)
    
    mac        w4*w4, A, [w8]+=2, w4   ;Compute the Energy     
       
    mov        ACCAH, w1
    mov        ACCAL, w0               ;Return the signal energy
    pop        CORCON
    return

;******************************************************************************
;  MemCopy:                                                                   *
;  ======================                                                     *
;  Description: This function copies the samples from input buffer to the     *
;               DTMF working Buffer                                           *
;                                                                             *
;  Input:  w0 = Addr of input Buffer                                          *
;          w1 = Addr of DTMF working Buffer                                   *
;          w2 = No of samples to be copied                                    *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2               used not restored                                  *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;****************************************************************************** 
    .global _MemCopy
       
_MemCopy:

    dec        w2, w2                  ;Decrement the Loop count
    repeat     w2
    mov        [w0++], [w1++]          ;Copy samples from input buf to Work Buf
    
    return
    
;******************************************************************************
;  gaincontrol:                                                               *
;  ======================                                                     *
;  Description: This function perform adaptive scaling of the signal to       *
;               prevent overflow.                                             *
;                                                                             *
;  Input:  w0 w1 = signalEnergy                                               *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2,w4,w5, w8     used not restored                                  *
;   ACC A                  used not restored                                  *
;                                                                             *
;  Functions Called:  squareroot, divide                                      *
;                                                                             *
;******************************************************************************
    .global _gaincontrol
    
_gaincontrol:

    mov        w0, ACCAL
    mov        w1, ACCAH               ;Load the signalEnergy into Acc A
    
    sftac      A, #6
    
    mov        ACCAL, w0
    mov        ACCAH, w1
    call       _squareroot
    
    mov        w0, w4                  ;Compute the Gain power
    mov        #0x4868, w5
    mpy        w4*w5, A
    sac        A, w4                   ;Gain Amplitude
    
    mov        #164, w0

    cp         w4,w0
    bra        gt,DIVDE_1
    
    mov        #164,w4

DIVDE_1: 
    call       _divide                 ;Get the Gain scaling factor
    
    mov        #DTMFworkBuff, w8
    mov        w0, w5
    do         #(NUM-1), END_GC

    mov        [w8], w4
    mpy        w4*w5, A
    sac        A, w2
    
    mov        w2, [w8++]              ;Multiply all the samples with the
END_GC:                                ;scaling factor
    nop                                
    return
;******************************************************************************
;  FrameProcess:                                                              *
;  ======================                                                     *
;  Description: This function processes the frame of 100 samples.             *
;                                                                             *
;  Input:  Void                                                               *
;                                                                             *
;  Output: process type                                                       *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2,w4,w5,w8,w13,w14         used not restored                       *
;                                                                             *
;  Functions Called:  energy,gaincontrol,validation0,validation1,validation2  *
;                     goertzel,prevalidation                                  *
;                                                                             *
;******************************************************************************
    .global _FrameProcess
        
_FrameProcess:

    lnk        #34
  
    mov        #DTMFworkBuff, w0
    rcall      _energy                 ;compute energy of incoming signal
    
    rcall      _gaincontrol            ;Perform adaptive scaling of the signal
                                       ;to prevent overflow
    mov        #DTMFworkBuff, w0
    rcall      _energy
    mov        w0, w12                 ;Energy after gain control(gain energy)
    mov        w1, w13
    
    mov        #NOT_A_DIGIT_FRAME, w0
    cp         DTMFframeType           ;chk if previous frametype was not vaild
    bra        nz, CHK_POSSIBLEDIGIT
    
    mov        #DTMFworkBuff, w0       ;Base Addr of working Buffer
       
    bset       CORCON, #2
    mov        #psvpage(coeff), w8
    mov        w8, PSVPAG
    mov        #psvoffset(coeff), w8
    mov        w14, w1
    
    rcall      _goertzel               ;Perform DFT computation 16 times
    
    mov        w14, w2
    mov        w13, w1                 ;gain energy
    mov        w12, w0
    mov        #32, w3
    add        w14, w3, w3
    push       w12
    push       w13
    push       w3

    push       w14
    rcall      _prevalidation          ;perform Vaildation checks
    pop        w14
    pop        w3
    
    mov        w0, w1
    mov        w3, w0
    
    rcall      _validation0
    pop        w13
    pop        w12
    
    mov        #POSSIBLE_DIGIT_FRAME, w0
    cp         DTMFframeType           ;Chk if frametype is of Possible digit
    bra        nz, EXIT_FRMPROCESS
    mov        w12, DTMFEnergyThreshold;set the Energy Threshold
    mov        w13, DTMFEnergyThreshold+2
    bra        EXIT_FRMPROCESS
    
CHK_POSSIBLEDIGIT:

    mov        #POSSIBLE_DIGIT_FRAME, w0
    cp         DTMFframeType           ;chk if previous frametype had 
    bra        nz, CHK_SILENCE         ;identified a possible digit
    
    mov        #DTMFworkBuff, w0       ;Base Addr of working Buffer
 
    bset       CORCON, #2
    mov        #psvpage(coeff), w8
    mov        w8, PSVPAG
    mov        #psvoffset(coeff), w8
    mov        w14, w1
     
    rcall      _goertzel               ;Perform DFT computation 16 times
    
    mov        w14, w2
    mov        w13, w1
    mov        w12, w0
    mov        #32, w3
    add        w14, w3, w3
    push       w12
    push       w13
    
    push       w3   
    push       w14
    rcall      _prevalidation          ;Perform Validation checks
    pop        w14
    pop        w3
    
    mov        w0, w1
    mov        w3, w0
    rcall      _validation1
;after two consecutive detections, update the energythreshold
;by the average energy of the previous two frames    

    pop        w13
    pop        w12
    
    mov        #DIGIT_DETECTED, w0
    cp         DTMFframeType
    bra        nz, PREV_POSSIBLEDIGIT
; consider 50% of previous energy threshold and 
; 50% of current threshold

    clr        A
    mov        DTMFEnergyThreshold, w4
    mov        DTMFEnergyThreshold+2, w5
    mov        w4, ACCAL
    mov        w5, ACCAH               ;Load Energy Threshold to ACC A
    sftac      A, #1
    
    clr        B    
    mov        w12, ACCBL
    mov        w13, ACCBH              ;Load gain energy to ACC B
    sftac      B, #1
    mov        ACCBL, w12
    mov        ACCBH, w13              ;Update the gain energy
        
    add        A
    mov        ACCAL, w4
    mov        ACCAH, w5               ;Update the Energy Threshold
    mov        w4, DTMFEnergyThreshold
    mov        w5, DTMFEnergyThreshold+2
    
    bra        EXIT_FRMPROCESS
    
PREV_POSSIBLEDIGIT:

    mov        #POSSIBLE_DIGIT_FRAME, w0
    cp         DTMFframeType           ;chk if prev frametype is possible digit
    bra        nz, EXIT_FRMPROCESS
    
    mov        w12, DTMFEnergyThreshold
    mov        w13, DTMFEnergyThreshold+2;update energy threshold
    bra        EXIT_FRMPROCESS
    
CHK_SILENCE:    
   
; if previous frame type had detected a valid digit but needs 
; to be confirmed based on the shape test. This frame needs a 
; liberal test and should not to be conidered invalid tone whatsoever
; here we expect silence, valid digit or none.    
  
    mov        #DIGIT_DETECTED, w0
    cp         DTMFframeType           ;chk if prev frame has detected Digit
    bra        z, CHK_GAINENERGY
    
    mov        #TONE_SHAPE_TEST, w0
    cp         DTMFframeType           ;Chk Tone Shape Test is enabled
    bra        nz, CHK_PREVPAUSE
CHK_GAINENERGY:    
    
    mov        DTMFEnergyThreshold, w4
    mov        DTMFEnergyThreshold+2, w5
    mov        w4, ACCAL
    mov        w5, ACCAH               ;Load energy threshold into ACC A
    sftac      A, #2
    
    mov        ACCAL, w4
    mov        ACCAH, w5
    
    sub        w12, w4, w4
    subb       w13, w5, w5             ;Chk gainenergy > Energythreshold
    bra        le, CHK_SHAPETEST
    
    mov        #DTMFworkBuff, w0
 
    bset       CORCON, #2              ;Set psv bit for program memory Access 
    mov        #psvpage(coeff), w8
    mov        w8, PSVPAG
    mov        #psvoffset(coeff), w8
    mov        w14, w1
    rcall      _goertzel               ;Perform DFT 16 times
    
    mov        w14, w2
    mov        w13, w1
    mov        w12, w0                 ;Pass gainenergy in w0w1
    mov        #32, w3
    add        w14, w3, w3
    push       w12                     ;Restore gain energy
    push       w13

    push       w3
    push       w14
    rcall      _prevalidation          ;Perform validation checks
    pop        w14
    pop        w3
    mov        w0, w1                  ;Success flag
    mov        w3, w0
    rcall      _validation2            ;Pass the mapped digit
    
    pop        w13
    pop        w12                     ;Retrieve gain energy
    
    mov        #TONE_SHAPE_TEST, w0
    cp         DTMFframeType           ;Chk if Tone shape test is enabled
    bra        nz, CHK_NOTADIGIT
        
    mov        DTMFEnergyThreshold, w4
    mov        DTMFEnergyThreshold+2, w5

    sub        w12, w4, w4
    subb       w13, w5, w5             ;Chk gainenergy > EnergyThreshold
    bra        le, EXIT_FRMPROCESS
    
    mov        DTMFEnergyThreshold, w4
    mov        DTMFEnergyThreshold+2, w5
    mov        w4, ACCAL
    mov        w5, ACCAH               ;Load EnergyThreshold into ACC A
    sftac      A, #1
        
    mov        w12, ACCBL
    mov        w13, ACCBH              ;Load gain energy into ACC B
    sftac      B, #1
    mov        ACCBL, w12
    mov        ACCBH, w13
        
    add        A
    mov        ACCAL, w4
    mov        ACCAH, w5               ;Update the EnergyThreshold
    mov        w4, DTMFEnergyThreshold
    mov        w5, DTMFEnergyThreshold+2
    bra        EXIT_FRMPROCESS
    
CHK_NOTADIGIT:

    mov        #NOT_A_DIGIT_FRAME, w0
    cp         DTMFframeType           ;Chk if Not a Digit Detected
    bra        nz, EXIT_FRMPROCESS
    mov        #TONE_SHAPE_TEST, w0
    mov        w0, DTMFframeType       ;Enable Tone Shape Test
    bra        EXIT_FRMPROCESS
    
CHK_SHAPETEST:

    mov        #YES, w0
    cp         DTMFshapeTest           ;Chk if ShapeTest is Enabled
    bra        nz, EXIT_FRMPROCESS
    
    inc        DTMFsilenceCount        ;Increment the silence count
    
    mov        #PAUSE_AFTER_DIGIT_FRAME, w0
    mov        w0, DTMFframeType

;Update silenceThreshold with the gain energy
   
    mov        w12, DTMFSilenceThreshold
    mov        w13, DTMFSilenceThreshold+2
    bra        EXIT_FRMPROCESS

CHK_PREVPAUSE:

    mov        #PAUSE_AFTER_DIGIT_FRAME, w0
    cp         DTMFframeType           ;Chk if previous frame has Paused
    bra        nz, CHK_PREVVALIDDIGIT
    
    mov        DTMFEnergyThreshold, w4
    mov        DTMFEnergyThreshold+2, w5
    mov        w4, ACCAL
    mov        w5, ACCAH               ;Load the EnergyThreshold into ACC A
    sftac      A, #2
    mov        ACCAL, w4
    mov        ACCAH, w5
    
    sub        w12, w4 , w4
    subb       w13, w5, w5             ;Chk gainenergy > EnergyThreshold
    bra        le, CHK_SHAPETEST1
    
    mov        #DTMFworkBuff, w0
 
    bset       CORCON, #2              ;Set psv bit for program Mem Access
    mov        #psvpage(coeff), w8
    mov        w8, PSVPAG
    mov        #psvoffset(coeff), w8
    mov        w14, w1
    rcall      _goertzel               ;Compute DFT 16 times
    
    mov        w14, w2
    mov        w13, w1
    mov        w12, w0                 ;Pass gain energy in w0w1
    mov        #32, w3
    add        w14, w3, w3

    push       w12
    push       w13                     ;Restore gain energy

    push       w3
    push       w14
    rcall      _prevalidation          ;Perform validation checks
    pop        w14
    pop        w3
    mov        w0, w1
    mov        w3, w0
    rcall      _validation0            ;decide the success of the DTMF digit
    pop        w13
    pop        w12                     ;Retrieve gain energy
    
    mov        #POSSIBLE_DIGIT_FRAME, w0
    cp         DTMFframeType           ;Chk the frame type
    bra        nz, EXIT_FRMPROCESS
;Update the EnergyThreshold with the gain energy
    
    mov        w12, DTMFEnergyThreshold
    mov        w13, DTMFEnergyThreshold+2 
    bra        EXIT_FRMPROCESS

CHK_SHAPETEST1:
;Update the EnergyThreshold with the gain energy

    mov        w12, DTMFEnergyThreshold
    mov        w13, DTMFEnergyThreshold+2
    
    mov        #YES, w0
    cp         DTMFshapeTest           ;Chk if shape test is enabled
    bra        nz, NOTADIGIT
    
    inc        DTMFsilenceCount        ;Increment silence count
    mov        #2, w0
    cp         DTMFsilenceCount        ;chk silenceCount >= 2
    bra        lt, PAUSEAFTERDIGIT
    
    mov        DTMFcurrentDigit, w0
    mov        #VALID_DIGIT_FRAME, w1
    mov        w0, DTMFdeclaredDigit   ;Set the current digit as declared digit
    
    mov        w1, DTMFframeType       ;Set frame as Valid
    
    clr        DTMFsilenceCount        ;Reset the silence count
    mov        #NO, w0
    mov        w0, DTMFshapeTest       ;Disable the shape test
    bra        EXIT_FRMPROCESS
PAUSEAFTERDIGIT:

    mov        #PAUSE_AFTER_DIGIT_FRAME, w0
    mov        w0, DTMFframeType       ;Set frame type as Pause
    bra        EXIT_FRMPROCESS
        
NOTADIGIT:

    mov        #NOT_A_DIGIT_FRAME, w0
    mov        w0, DTMFframeType       ;Declare frame as not a digit
    bra        EXIT_FRMPROCESS
    
CHK_PREVVALIDDIGIT:

    mov        #VALID_DIGIT_FRAME, w0
    cp         DTMFframeType           ;Chk if frame is Valid
    bra        nz, EXIT_FRMPROCESS
    mov        #DTMFworkBuff, w0
 
    bset       CORCON, #2              ;Set psv bit for program Mem Access
    mov        #psvpage(coeff), w8
    mov        w8, PSVPAG
    mov        #psvoffset(coeff), w8
    mov        w14, w1
    rcall      _goertzel               ;Compute DFT 16 times
    
    mov        w14, w2
    mov        w13, w1
    mov        w12, w0                 ;Pass gain energy in w0 w1
    mov        #32, w3
    add        w14, w3, w3

    push       w12
    push       w13                     ;Restore the gain energy

    push       w3
    push       w14
    rcall      _prevalidation          ;Perform validation checks
    pop        w14
    pop        w3
    mov        w0, w1
    mov        w3, w0
    rcall      _validation0            ;Chk success of the digit
    
    pop        w13
    pop        w12                     ;Retrieve the gain energy
    
    mov        #POSSIBLE_DIGIT_FRAME, w0
    cp         DTMFframeType
    bra        nz, EXIT_FRMPROCESS
;Update Energy Threshold with the gain energy
    
    mov        w12, DTMFEnergyThreshold
    mov        w13, DTMFEnergyThreshold+2

EXIT_FRMPROCESS:

    mov        DTMFframeType, w0       ;Return the frame Type
    ulnk
    return            

;******************************************************************************
;  validation0:                                                               *
;  ======================                                                     *
;  Description: This function decide the success of the dtmf digit detection  *
;                                                                             *
;  Input:  Void                                                               *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1                  used not restored                                  *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************

    .global _validation0
    
_validation0:
    
    cp         w1, #5                  ;Chk the Success flag
    bra        lt, DECLARE_NOTADIGIT
    
    mov        [w0], w0                ;Get the digit detected
    mov        w0, DTMFcurrentDigit
    mov        #POSSIBLE_DIGIT_FRAME, w0
    mov        w0, DTMFframeType       ;Set the frame type
    return
    
DECLARE_NOTADIGIT:

    mov        #NOT_A_DIGIT_FRAME, w0
    mov        w0, DTMFframeType       ;Declare frame as not a digit
    return    
    
;******************************************************************************
;  validation1:                                                               *
;  ======================                                                     *
;  Description: This function decide the success of the dtmf digit detection  *
;                                                                             *
;  Input:  Void                                                               *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2,w6,w8         used not restored                                  *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************    
    .global _validation1
   
_validation1:

    cp        w1, #5                   ;Chk the success flag
    bra       lt, NOT_A_DIGIT
    
    mov       [w0], w0                 ;Get the detected digit
    cp        DTMFcurrentDigit         ;Compare with the current digit
    bra       nz, POSSIBLE_DIGIT
    
    mov       #DIGIT_DETECTED, w0
    mov       #YES, w6
    mov       w0, DTMFframeType        ;Set frame type as digit detected
    mov       w6, DTMFshapeTest        ;Enable Shape Test
    return
        
POSSIBLE_DIGIT:

    mov       #digit, w8
    mov       #POSSIBLE_DIGIT_FRAME, w0
    mov       [w8], w2
    mov       w0, DTMFframeType        ;Set frame as possible Digit
    mov       w2, DTMFcurrentDigit     ;Store the detected digit
    return    
    
NOT_A_DIGIT:

    mov       #NOT_A_DIGIT_FRAME, w0
    mov       w0, DTMFframeType        ;Declare frame as not a Digit
    return
      
;******************************************************************************
;  validation2:                                                               *
;  ======================                                                     *
;  Description: This function decide the success of the dtmf digit detection  *
;                                                                             *
;  Input:  Void                                                               *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1                  used not restored                                  *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************

    .global _validation2
    
_validation2:

    cp        w1, #5                   ;Chk the Success flag
    bra       lt, NOT_A_DIGIT1
    
    mov       [w0], w0                 ;Get the detected digit
    cp        DTMFcurrentDigit
    bra       nz, EXIT_VALIDATION2
    
    mov       #TONE_SHAPE_TEST, w0
    mov       w0, DTMFframeType        ;Enable Tone Shape Test
    return
            
NOT_A_DIGIT1:

    mov       #NOT_A_DIGIT_FRAME, w0
    mov       w0, DTMFframeType        ;Declare frame as not a Digit
    
EXIT_VALIDATION2:

    mov       DTMFframeType, w0        ;Return the Frame type
    return
    
;******************************************************************************
;  goertzel:                                                                  *
;  ======================                                                     *
;  Description: This function performs DFT and uses 16 goertzel filters for 8 *
;               fundamnetal frequencies and 8 second harmonics.               *
;                                                                             *
;  Input:  Void                                                               *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w3,w4,w5,w6,w7,w8,w9,w10    used not restored                       *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************
     
    .global _goertzel
    
_goertzel:

    push       CORCON
 
    mov        w0, w10                 ;Save the work Buff address
; Perform DFT (Goertzel algorithm)    
    do         #15, END_DFT 
    clr        A
    mov        [w10], w5               ;Get the input sample
    mov        [w8], w4                ;coeff[i]
    
    mpy        w4*w5, A

    push       w0
    push       w1
    call       Shiftleft
    pop        w1
    pop        w0
    
    add       w10, #2, w3

    clr        B
    lac        [w3], #0, B
    
    add        A
    sac        A, w7
    
    add        w10, #4, w0

/* w5 = dft0 */    
;Feed Back phase    
    do         #(NUM-2-1), END_UPD
    
    mov        [w0++], w4
    clr        A
    clr        B

    mov        w4, ACCBH               ;vk[n - 1]
    mov        w5, ACCAH               ;vk[n - 2]
    sub        B
    mov        [w8], w4
    mpy        w4*w7, A
    push       w0
    push       w1
    call       Shiftleft
    pop        w1
    pop        w0

    add        A 
    sac        A, w6
    
    mov        w7, w5
END_UPD:    
    mov        w6, w7                  ;Update the Delay Line

;Squared magnitude energy computation for each frequency
; Feed forward phase
   
    mpy        w5*w7, A                ;vk[N] * vk[N-1] 
    sftac      A, #-1
    sac        A, w4
    mov        [w8++], w6
    
    mpy        w4*w6, A                ;2cos(2*pi* k/N) * vk[N] * vk[N-1] 
    
    mpy        w5*w5, B                ;vk[N-1] * vk[N-1] 
    
    sub        B
    
    mpy        w7*w7, A                ;vk[N] * vk[N] 
    
    add        A
    sac        A, w9
END_DFT:
    mov        w9, [w1++]    
    
    pop        CORCON
    return
    
;******************************************************************************
;  prevalidation:                                                             *
;  ======================                                                     *
;  Description: This function checks the signal strength and maps the detected*
;               digit after all validation checks.                            *
;                                                                             *
;  Input:  w0 w1 = gain energy                                                *
;          w2 = Addr of sqrenergy buffer                                      *
;          w3 = Addr of digit variable                                        *
;                                                                             *
;  Output: Void                                                               *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2,w3,w4         used not restored                                  *
;                                                                             *
;  Functions Called:  divide                                                  *
;                                                                             *
;******************************************************************************
    .global _prevalidation
   
_prevalidation:


   lnk        #6
   
   /* maximum search  for low frequencies  */

   clr        w10
   mov        w10, [w14]
   mov        w10, [w14+2]
   mov        w10, [w14+4]
   
   mov        w2, w10
   mov        [w2], w7                 ;rowmax = w7
   clr        w11
   do         #3, LOOP
   
   mov        [w10++], w6
   cp         w7, w6                   ;Chk rowmax <= sqrenergy[i]
   bra        gt, LOOP
   
   mov        w6, w7
   mov        w11, w9                  ;rowno = w9
LOOP:
   inc        w11, w11
   
   sl         w9, #1, w5
   add        w5, w2, w4
   mov        #1, w5
   mov        w5, [w4]                 ;sqrEnergy[rowno] = 1
   
 /* maximum search  for high frequencies */ 
 
   mov        [w10], w6                ;colmax = w6
   mov        #4, w11
   do         #3, LOOP1
   
   mov        [w10++], w4
   cp         w6, w4                   ;Chk colmax <= sqrenergy[i]
   bra        gt, LOOP1
   
   mov        w4, w6
   mov        w11, w12                 ;colno = w12
LOOP1:
   inc        w11, w11

   sl         w12, #1, w5
   add        w5, w2, w4
   mov        #1, w5
   mov        w5, [w4]                 ;sqrEnergy[colno] = 1
   
/* Check if the power levels per frequency components is 
   greater than the minimum required **/
   
      
   mov        #400, w4
   cp         w7, w4
   bra        le, NO_CHECKS
   
   cp         w6, w4
   bra        le, NO_CHECKS
   
/********** Perform the different validation checks **************/
/* Signal strength check */

;Etotal = Sum of Eindividual
   add        w7, w6, w4
   
   mov        w4, ACCAL
   clr        ACCAH                    ;Eg = Goertzel energy
   
   sftac      A, #-11
;CHk Es < (Eg  << 11)
   
   mov        ACCAL, w4
   mov        ACCAH, w5
   mov        w14, w13
   sub        w0, w4, w4
   subb       w1, w5, w5
   bra        gt, INITFLAG1
   
   mov        #1, w4
   mov        w4, [w13]                ;success++
   bra        TWIST_CHK
   
INITFLAG1:
   mov        #1, w4
   mov        w4, [w13+2]              ;flag1 = 1
   
/* perform twist checks i.e ratio of row and column energies */
TWIST_CHK:

   cp         w7, w6
   bra        le, STD_TWIST
; reverse twist
   
   mov        w6, w0
   mov        w7, w4
   push       w2
   call       _divide                  ;colmax/rowmax
   pop        w2
   mov        #THR_REVTWI, w4
   cp         w0, w4                   ;Chk ratio is <= = -8 dB
   bra        gt, TO_SUCCESS
   
   mov        #NOT_A_DIGIT_FRAME, w4
   mov        w4, DTMFframeType        ;Declare not a Digit
   
   mov        #1, w4
   mov        w4, [w13+4]              ;flag2 = 1
   
   bra        STD_TWIST

TO_SUCCESS:
   
   inc        [w14], [w14]             ;success++
   
STD_TWIST:

/* standard twist*/

   cp         w7, w6                   ;Chk rowmax < colmax
   bra        ge, ROW_PEAK
   
   mov        w7, w0
   mov        w6, w4
   push       w2
   call       _divide                  ;rowmax/colmax
   pop        w2
   mov        #THR_STDTWI, w4
   cp         w0, w4                   ;chk ratio is <= +4dB
   bra        ge, TO_SUCCESS1
   
   mov        #NOT_A_DIGIT_FRAME, w4
   mov        w4, DTMFframeType        ;Declare not a Digit
   
   mov        #1, w4
   mov        w4, [w13+4]              ;flag2 = 1
   
   bra        ROW_PEAK

TO_SUCCESS1:
   
   inc        [w14], [w14]             ;success++

/* Relative row peak check */
   
ROW_PEAK:

    mov       w2, w10
    mov       #0, w5     
    do        #3, LOOP2
;chk relative row peak    
    mov       [w10++], w0
    mov       w7, w4
    push      w2
    call      _divide                  ;relpeak/rowmax
    pop       w2
    mov       #THR_ROWREL, w1
    cp        w0, w1                   ;relrow > 12 dB
    bra       le, INCCNT
    
    mov       #NOT_A_DIGIT_FRAME, w4
    mov       w4, DTMFframeType        ;Declare not a Digit
    bra       LOOP2

INCCNT:
    inc       w5,w5
LOOP2:
    nop
    
    cp        w5, #3 
    bra       lt, RESETCNT
    inc       [w14], [w14]             ; success++
RESETCNT:
    clr       w5 
    
        
/* Relative column peak check */     
     
    do        #3, LOOPING
    
    mov       [w10++], w0
    mov       w6, w4
    push      w2
    call      _divide                  ;relpeak/colmax
    pop       w2
    mov       #THR_COLREL, w1
    cp        w0, w1                   ;chk relrow > 12 dB
    bra       le, INCCNT11
    
    mov       #NOT_A_DIGIT_FRAME, w4
    mov       w4, DTMFframeType        ;Declare not a Digit
    bra       LOOPING
INCCNT11:
    inc       w5,w5
    
LOOPING:
   nop
        
    cp        w5, #3
    bra       lt, RESETCNT1
    inc       [w14], [w14]             ;success++
RESETCNT1:
    clr       w5 

         
/* Second harmonic low freq check  */
;To discriminate DTMF tones from possible speech or music
   mov        #8, w4
   add        w9, w4, w4
   sl         w4, #1, w4
   add        w2, w4, w4
   push       w2
   mov        [w4], w0
   mov        w7, w4

   cp         w0,w4
   bra        le, DIVIDE_2

   mov        #0x7fff,w0
   bra        DIVIDE_FOLLOW 

DIVIDE_2:

   call       _divide                  ;Second Harmonic low freq/funda comp

DIVIDE_FOLLOW:

   pop        w2
   mov        #THR_ROW2nd, w4
   cp         w0, w4                   ;Chk the ratio is equal to -22 dB 
   bra        lt, INCSUCC
   
   mov        #NOT_A_DIGIT_FRAME, w4
   mov        w4, DTMFframeType        ;Declare not a Digit
   bra        SEC_HARMHIGHFREQCHK
INCSUCC:

   inc        [w14], [w14]             ;success++
   
   /* second harmonic high freq check  */
SEC_HARMHIGHFREQCHK:
   
   mov        #8, w4
   add        w4, w12, w4
   sl         w4, #1, w4
   
   add        w4, w2, w4
   push       w2
   mov        [w4], w0
   mov        w6, w4

   call       _divide                  ;Second Harmonic high freq/funda comp
   pop        w2
   mov        #THR_COL2nd, w4
   cp         w0, w4                   ;Chk the ratio is equal to -22 dB 
   bra        lt, INCSUCC111
   
   mov        #NOT_A_DIGIT_FRAME, w4
   mov        w4, DTMFframeType        ;Declare not a Digit
   bra        CHK_FLAGS
INCSUCC111:

   inc        [w14], [w14]             ;success++
      
CHK_FLAGS:

   mov        [w14+2], w0
   cp         w0, #1
   bra        z, RESETSUCC
   
   mov        [w14+4], w0
   cp         w0, #2                   ;Chk the flag1 and flag2
   bra        nz, MAP_DIGIT
RESETSUCC:
   
   clr        [w14]

NO_CHECKS:
   
   clr        [w14]   
   
MAP_DIGIT:
/* map the detected digit */
;This is done by mapping the tone-pair to its corresponding
; digit and comparing it with the previously detected digit

   mul.uu       w9, #10, w4
   add          w12, w4, w4
   mov          #0, w8
   bset         CORCON, #2             ;Set psv bit for prog mem Access
   mov          #psvpage(digitmap), w10
   mov          w10, PSVPAG
;point w10 to digitmap array
   
   mov          #psvoffset(digitmap), w10

   do           #15, LOOP4
   
   mov          [w10++], w0
   cp           w4, w0
   bra          nz, LOOP4
   mov          w8, w5
   
LOOP4:
   inc          w8, w8
   
   bset         CORCON, #2             ;Set psv bit for prog mem Access
   mov          #psvpage(digit), w8
   mov          w8, PSVPAG
;point w10 to digit array   
   mov          #psvoffset(digit), w8
   sl           w5, #1, w5
   mov          [w14], w0              ;success
   mov          [w8+w5], w5
   mov          w5, [w3]               ;Store the detected digit
   
   ulnk
   return   
      
   
; -----------------------------------------------------------------------------
;    END OF FILE
; -----------------------------------------------------------------------------
    
