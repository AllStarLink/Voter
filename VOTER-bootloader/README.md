# VOTER/RTCM Bootloader

The VOTER uses a dsPIC33FJ128GP802 and the RTCM uses a dsPIC33FJ128GP804.

There are **two** parts to the firmware, a *bootloader*, and then the actual *firmware* file. The bootloader starts when power is applied, and allows you to talk to the dsPIC and load new firmware files over ethernet. If the bootloader is not intercepted by the loading tool (EBLEX C30), it will continue to boot the current firmware file.

All new boards **must** have the bootloader installed first, followed by a firmware file. You ***can*** load a firmware file directly (.hex) in to the dsPIC, but then you **will not** have any of the bootloader remote loading features.

These are the current bootloader (`.cof`) files. The `-SMT` file is for the RTCM, if you needed to replace the dsPIC on it for some reason, and needed to re-load the bootloader. The `.cof` files need to be loaded with a PICKit programmer, through the ICSP header, and MPLAB IDE (and you need to load the `voter-bootloader.mcp` project file into the IDE).

Alternatively, `ENC_C30.hex` (VOTER) and `ENC_C30-SMT.hex` (RTCM) have been provided. These are the **bootloader** files that can be directly programmed into a new dsPIC with any compatible programmer. The default bootloader IP is 192.168.1.11.

Current firmware (`.cry` files) are available elsewhere in this repository. They are loaded with the EBLEX C30 Programmer via ethernet (once the bootloader is available on the dsPIC).

**Note:** We do **NOT** have the source code for the bootloader. As far as is known, this was all that was provided from the vendor when this project was created.


## Programming the Bootloader

If you need to load the bootloader in to a fresh board, you will need to follow these steps. You will require Microchip MPLAB IDE v8.66 (which is OLD!), an environment to run it in, and a suitable PICKit programmer.

1) Go to Project --> Open --> voter-bootloader.mcp --> Open
2) Go to File --> Import --> voter-bootloader --> ENC_C30.cof --> Open ***This step is missing from the original procedure.***
3) Go to Configure --> Select Device and pick dsPIC33FJ128GP802 (VOTER) --> Ok
4) Select View --> Program Memory (from the top menu bar).

### Change Default Bootloader IP Address

If you want to change the default IP address from `192.168.1.11`:

Hit Control-F (to "find") and search for the digits `00A8C0`.

These should be found at memory address `03018`.

The `A8C0` at `03018` represents the hex digits `C0` (192) and `A8` (168) which are the first two octets  of the IP address. The six digits to enter are `00` then the SECOND octet of the IP address in hex then the FIRST octet of the IP address in hex.

The `0B01` at `0301A` represents the hex digits `0B` (11) and `01` (1) which are the second two octets of the IP address. The six digits to enter are `00` then the FOURTH octet of the IP address in hex then the THIRD octet of the IP address in hex.

### Load Bootloader

Otherwise, if you just want to load the bootloader into a board...

1) Remove `JP7` on the VOTER Board. This is necessary to allow programming by the PICKit2/PICKit3 device.
2) Attach the PICKIT2/PICKit3 device to `J1` on the VOTER board. **Note:** Pin 1 is closest to the power supply modules (as indicated on the board).
3) If you have not already selected a programming device, ensure it is connected to you computer, go to Programmer --> Select Programmer and choose PICKit3 (or PICKit2, depending on what you are using)
4) Go to Programmer --> Program. This will program the bootloader firmware into the PIC device on the board.