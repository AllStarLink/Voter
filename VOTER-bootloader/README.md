# VOTER/RTCM Bootloader

The VOTER uses a dsPIC33FJ128GP802 and the RTCM uses a dsPIC33FJ128GP804.

There are **two** parts to the firmware, a *bootloader*, and then the actual *firmware* file. The bootloader starts when power is applied, and allows you to talk to the dsPIC and load new firmware files over ethernet. If the bootloader is not intercepted by the loading tool (EBLEX C30), it will continue to boot the current firmware file.


All new boards **must** have the bootloader installed first, followed by a firmware file. You ***can*** load a firmware file directly (.hex) in to the dsPIC, but then you **will not** have any of the bootloader remote loading features.


These are the current bootloader (`.cof`) files. The `-SMT` file is for the RTCM, if you needed to replace the dsPIC on it for some reason, and needed to re-load the bootloader. The `.cof` files need to be loaded with a PICKit programmer, through the ICSP header, and MPLAB IDE (and you need to load the `voter-bootloader.mcp` project file into the IDE).


Alternatively, `ENC_C30.hex` (VOTER) and `ENC_C30-SMT.hex` (RTCM) have been provided. These are the **bootloader** files that can be directly programmed into a new dsPIC with any compatible programmer. The default bootloader IP is 192.168.1.11.


Current firmware (.cry files) are available elsewhere in this repository. They are loaded with the EBLEX C30 Programmer via ethernet (once the bootloader is available on the dsPIC).

**Note:** We do **NOT** have the source code for the bootloader. As far as is known, this was all that was provided from the vendor when this project was created.