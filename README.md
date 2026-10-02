This repository contains the firmware for the AllStarLink VOTER, the Micro-Node RTCM, and a bunch of 
various (obsolete) project fragments that are related to AllStar and the VOTER.

See the /docs folder for information on what the VOTER protocol is, and how the VOTER system works, 
in conjunction with `chan_voter` in AllStarLink.

The EBLEX C30 Programmer is the software utility used to load the VOTER/RTCM over Ethernet. You 
**MUST** have the bootloader installed in the main PIC, and **know the bootloader IP address*, in order 
to be able to capture the target, and load the firmware.

The `/VOTER-bootloader` folder contains the MicroChip project files, source code, and `.cof` binary 
to load in to the main PIC on the VOTER/RTCM. They allow the EBLEX Programmer (above) to be able 
to remotely load firmware in the VOTER/RTCM over Ethernet. Otherwise, you need to upload firmware 
directly on the module in question through the ICSP header, or an EEPROM programmer (for the VOTER).

The bootloader `.cof` file needs to be loaded in the PIC using the MPLAB IDE and a PICKit, or 
similar, via ICSP. Alternatively, a `.hex` file is supplied to use a standalone EEPROM programmer 
that supports the dsPIC.

The `/VOTER-pcb` folder contains the Gerbers and supporting files (schematic, BOM, etc.) for the 
original, open-source, VOTER through-hole board. This design IS fully functional, however, you 
WILL need to research and update the BOM with currently available components. It is functionally 
equivalent to the RTCM that was manufactured by Micro-Node, with the primary difference being that 
the RTCM used SMT components, instead of through-hole. 

The `/VOTER_RTCM-firmware` folder contains the firmware used in the PIC of the VOTER and RTCM. As 
noted above, the VOTER and RTCM are functionally equivalent, EXCEPT they need to use different 
firmware files, as the pin mapping is different between the DIP and SMT PIC packages. As such, 
the `RTCM_` files should ONLY be used with the RTCM, and not the VOTER. Likewise, the `VOTER_` files 
should ONLY be used with the VOTER, and not the RTCM.

If you load the wrong file in the wrong device, it will fail to boot. However, in most cases you 
should be able to recover the device by power cycling it and re-capturing the bootloader with 
the EBLEX programmer to load the correct file.

The `/VOTER-dissector` folder contains a Wireshark Lua script for disassembling VOTER protocol packets.

The `/archive` folder contains various other fragments of Jim's projects. Some of them made it in to 
production in various forms, some were prototypes, all are now obsolete. Unfortunately, while there 
is firmware and source code for a number of the projects, there are no accompanying schematics. If 
you have any of the missing information to contribute, please do!


