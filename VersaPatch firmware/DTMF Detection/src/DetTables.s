/***************************************************************************************
* © 2008 Microchip Technology Inc.
*
* FileName:        DetTables.s
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

 .include "Dettables.inc"

.section .DTMFDET_TABLES, psv

;Used for mapping the detected DTMF digit
digitmap:
.hword 35,04,05,06,14,15,16,24,25,26,07,17,27,37,34,36

;DTMF digits

digit:

.hword 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15

;DTMF Detection coefficients
coeff:

.hword  27979, 26955, 25701, 24218, 19072, 16324, 13013, 9315
.hword  15013, 11582, 7549, 3032, -10565, -16502, -22318, -27471  
