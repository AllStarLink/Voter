How-to (Re-)Compile Firmware for the VOTER Board
Lee Woldanski, VE7FET
2017/08/08

If you look in the votersystem.pdf, you will find a procedure to modify and load the bootloader in to
the PIC of a VOTER board. 

Unfortunately, there is an important step missing in that procedure, which is covered below.

In addition, the old links for the MPLAB software are dead, so let's update this info and get you going.

Currently (Feb 2017), you can get the required software from:

http://www.microchip.com/development-tools/downloads-archive

Download MPLAB IDE 32-bit Windows v8.66:
http://ww1.microchip.com/downloads/en/DeviceDoc/MPLAB_IDE_v8_66.zip
http://dvswitch.org/files/AllStarLink/Voter/MPLAB_IDE_v8_66.zip

Download MPLAB C Compiler for PIC24 and dsPIC DSCs v3.31 ***NOT v.3.25***:
http://ww1.microchip.com/downloads/en/DeviceDoc/mplabc30-v3_31-windows-installer.exe
http://dvswitch.org/files/AllStarLink/Voter/mplabc30-v3_31-windows-installer.exe

Optionally, install Windows Virtual PC and XP Mode. This is getting to be pretty old software,
so let's run it under XP Mode so we can keep it isolated (install it in a virtual machine).

Run the MPLAB IDE installer. You don't need to install the HI-TECH C Compiler at the end (click no).

Run the MPLAB C Compiler installer.

Select Legacy Directory Name.

***Select Lite Compiler***

Go to:
https://github.com/AllStarLink

voter --> Clone or Download --> Download Zip

That will get you voter-master.zip which is a download of the whole VOTER tree from GitHub.

Extract it somewhere (ie. in the XP Mode Virtual PC).

Launch the MPLAB IDE.

Go to Configure --> Settings --> Projects and de-select one-to-one project mode.


Bootloader

If you need to load the bootloader in to a fresh board, you will need to follow these steps.

Go to Project --> Open --> voter-bootloader.mcp --> Open

Go to File --> Import --> voter-bootloader --> ENC_C30.cof --> Open ***This step is missing from the 
original procedure.***

Select View --> Program Memory (from the top menu bar).

If you want to change the default IP address from 192.168.1.11:

Hit Control-F (to "find") and search for the digits "00A8C0".

These should be found at memory address "03018".

The "A8C0" at 03018 represents the hex digits C0 (192) and A8 (168) which are the first two octets 
of the IP address. The six digits to enter are 00 then the SECOND octet of the IP address in hex 
then the FIRST octet of the IP address in hex.

The "0B01" at 0301A represents the hex digits 0B (11) and 01 (1) which are the second two octets of 
the IP address. The six digits to enter are 00 then the FOURTH octet of the IP address in hex then the
THIRD octet of the IP address in hex.

Otherwise, if you just want to load the bootloader...

Remove JP7 on the VOTER Board. This is necessary to allow programming by the PICKit2/PICKit3 device.

Attach the PICKIT2/PICKit3 device to J1 on the VOTER board. Note that Pin 1 is closest to the power supply
modules (as indicated on the board).

If you have not already selected a programming device, go to Programmer --> Select Device and choose 
PICKit3 (or PICKit2, depending on what you are using).

Then go to Programmer --> Program. This will program the bootloder firmware into the PIC device on
the board.



Firmware

To compile the firmware (if you want to make custom changes)...

Go to Project --> Open --> nagivate to board-firmware and open the voter.mcp file it finds.

Go to Project --> Build Configuration and select "Release". This may not be necessary (I don't believe that 
option is used in the firmware), but it removes the compiler option of __DEBUG being passed, so 
theoretically it would build "normal" firmware.

Project --> Build All and it should build compile everything and show you "Build Succeeded".

A voter.cry file should be in the board-firmware folder. You can load this with the ENC Loader.

There is also a .hex file in there that you could load with a programmer... but that would wipe 
the bootloader... so don't do that.


If you want to enable "Chuck Squelch", open the HardwareProfile.h and un-comment #define CHUCK. 

You may also want to go down to Line 291 in Voter.c (right click on the window, go to Properties -->
"C" File Types --> Line Numbers) and tack on CHUCK after 1.60 (the current version number) so that 
when you load this firmware, the version will be shown as 1.60CHUCK, and you'll know that 
Chuck Squelch is compiled in. We should probably make that more automagic in the future...


If you want to compile the DSPBEW version, open the DSPBEW project file instead.

**NOTE***: the .mcp files with the "smt" suffix are for the RTCM (built with SMT parts). The non-smt 
files are for the ORIGINAL through-hole VOTER boards. The difference is that the VOTER uses a dsPIC33FJ128GP802 
and the RTCM uses a dsPIC33FJ128GP804.

If you are compiling for the RTCM, go to Configure --> Select Device and choose the appropriate device for 
your board from the Device list. If you don't select the right one, the board will not boot.


That's it!

Now you should be able to modify at-will. :)


Baseband Examination Window (BEW) Firmware

Typically, the discriminator of an FM communications receiver produces results containing audio spectrum from the "sub-audible" range (typically < 100 Hz) to well above frequencies able to be produced by modulating audio. These higher frequencies can be utilized to determine signal quality, since they can only contain noise (or no noise, if a sufficiently strong signal is present).

For receivers (such as the Motorola Quantar, etc) that do not provide sufficient spectral content at these "noise" frequencies (for various reasons), The "DSP/BEW (Digital Signal Processor / Baseband Examination Window)" feature of the RTCM firmware may be utilized.

These receivers are perfectly capable of providing valid "noise" signal with no modulation on the input of the receiver, but with strong modulation (high frequency audio and high deviation), it severely interferes with proper analysis of signal strength.

This feature provides a means by which a "Window" of baseband (normal audio range) signal is examined by a DSP and a determination of whether or not sufficient audio is present to cause interference of proper signal strength is made. During the VERY brief periods of time when it is determined that sufficient audio is present to cause interference, the signal strength value is "held" (the last valid value previous to the time of interference) until such time that the interfering audio is no longer present.

The DSP/BEW feature is selectable, and should not be used for a receiver that does not need it.

***If you compile/load firmware in the VOTER/RTCM with BEW features, you will LOSE the Diagnostics Menu, as there isn't enough room in the dsPIC for both!***
