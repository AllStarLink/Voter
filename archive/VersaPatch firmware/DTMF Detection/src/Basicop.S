/***************************************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        Basicop.s
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

;******************************************************************************
;  squareroot:                                                                *
;  ======================                                                     *
;  Description: This function computes the squareroot of a given number.      *
;                                                                             *
;  Input:  w0 w1 = Long Number                                                *
;                                                                             *
;  Output: w0 = squareroot of a long number                                   *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11         used not restored           *
;   ACC A, ACC B                                  used not restored           *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************

    .global _squareroot
    .section  .text

_squareroot:

    clr        A
    clr        B
    mov        #0, w4
    mov        #0x1, w5                ; 0x10000
    
    mov        w4, w6
    mov        w4, w7                  ; count=0
    
    mov        w4, w8
    mov        w4, w9                  ; root=0
      
    sub        w0, w4, w4
    subb       w1, w5, w5
    bra        lt, NEXT                ; if(power >= 0x10000) 
    
    clr        w2
    clr        w3
    bset       w3, #14                 ; 1<<30
    bra        CHK_BIT                 ; skip else

NEXT:
    clr        w2
    clr        w3
    bset       w2, #14                 ; 1<<14

CHK_BIT:

    sub        w2, #0, w2
    subb       w3, #0, w3
    bra        z, EXIT_SQROOT          ; chk for non-zero number
    
    add        w6, #1, w6
    addc       w7, #0, w7              ; count++
    
    add        w2, w8, w10
    addc       w3, w9, w11             ; trial=root+bit
    
    sub        w0, w10, w4
    subb       w1, w11, w5
        
    bra        lt, NEXT1               ; if (power >= trial)
    
    mov        w4, w0
    mov        w5, w1                  ; power -= trial
    
    add        w10, w2, w8
    addc       w11, w3, w9             ; root = trial+bit
    
NEXT1:
     
    mov        w8, ACCAL
    mov        w9, ACCAH
    sftac      A, #1                   ; root >>= 1
    
    mov        ACCAL, w8
    mov        ACCAH, w9
      
    mov        w2, ACCBL
    mov        w3, ACCBH
    sftac      B, #2                   ; bit >>= 2
    
    mov        ACCBH, w3
    mov        ACCBL, w2               ; update bit
    bra        CHK_BIT
    
EXIT_SQROOT:

    mov        w8, w0                  ; return root
    return

;******************************************************************************
;  divide:                                                                    *
;  ======================                                                     *
;  Description: This function performs the division operation.                *
;                                                                             *
;  Input:  w0 = dividend                                                      *
;          w4 = divisor                                                       *
;                                                                             *
;  Output: w0 = Quotient                                                      *
;                                                                             *
;  System Resource usage:                                                     *
;   w0,w4,w8,w9                       used not restored                       *
;                                                                             *
;  Functions Called:  None                                                    *
;                                                                             *
;******************************************************************************

    .global _divide

_divide:

    push        w8
    push        w9                     ;Save the required registers
 
    mov         w0,w8                  ;w8 - Dividend
    mov         w4,w9                  ;w9 - Divisor

    repeat      #17
    divf        w8,w9                  ;Execute DIVF in the loop
    
EXIT_DIV:

    pop        w9
    pop        w8

    return                             ;Return the quotient in w0

; -----------------------------------------------------------------------------
; -----------------------------------------------------------------------------

    .global Shiftleft    
    
Shiftleft:    
    mov        #0xC000, w0
    mov        w0, w1
    and        ACCAH, wreg
    bra        z, SHIFT
    cp         w0, w1
    bra        nz, NO_SHIFT
SHIFT:    
    sftac      A, #-1
    bra        LAST

NO_SHIFT:
    mov        #0x8000, w0
    and        ACCAH, wreg
    bra        z, POSITIVE
    clr        A
    lac        w0, #0, A
    bra        LAST
POSITIVE:
    mov        #0x7FFF, w0
    mov        #0xffff, w1
    lac        w0, #0, A
    mov        w1, ACCAL    
LAST:
    return
            
    

; -----------------------------------------------------------------------------
;    END OF FILE
; -----------------------------------------------------------------------------

