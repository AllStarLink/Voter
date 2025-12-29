# How-to (Re-)Compile Firmware for the VOTER/RTCM Boards
If you look in the votersystem.pdf, you will find a procedure to modify and load the bootloader in to the PIC of a VOTER/RTCM board. 

Unfortunately, there is an important step missing in that procedure, which is covered below.

In addition, the old links for the MPLAB software are dead, so let's update this info and get you going.

## Download Required MPLAB Tools
Currently (December 2025), you can get the required software from:

[https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem](https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem)

Download MPLAB IDE 32-bit Windows v8.66:

* [https://ww1.microchip.com/downloads/en/DeviceDoc/MPLAB_IDE_v8_66.zip](https://ww1.microchip.com/downloads/en/DeviceDoc/MPLAB_IDE_v8_66.zip)
* [http://dvswitch.org/files/AllStarLink/Voter/MPLAB_IDE_v8_66.zip](http://dvswitch.org/files/AllStarLink/Voter/MPLAB_IDE_v8_66.zip) (alternate source)

Download MPLAB C Compiler for PIC24 and dsPIC DSCs v3.31 ***NOT v.3.25***:

* [https://ww1.microchip.com/downloads/Secure/en/DeviceDoc/mplabc30-v3_31-windows-installer.exe](https://ww1.microchip.com/downloads/Secure/en/DeviceDoc/mplabc30-v3_31-windows-installer.exe) (Requires Microchip Login/Account)
* [http://dvswitch.org/files/AllStarLink/Voter/mplabc30-v3_31-windows-installer.exe](http://dvswitch.org/files/AllStarLink/Voter/mplabc30-v3_31-windows-installer.exe) (alternate source)

## Install MPLAB Tools
The Microchip tools will install and run on Windows 10, they MAY run on Windows 11 (untested). You **can** install the tools in Windows Sandbox, if you don't want to install them permanently on your system (just remember that closing the Sandbox window will require you to re-install from scratch next time).

* Run the MPLAB IDE installer. You don't need to install the HI-TECH C Compiler at the end (click no).
* Run the MPLAB C Compiler installer.
    * Select Legacy Directory Name.
    * ***Select Lite Compiler*** (Free)
* Launch the MPLAB IDE.
    * Go to Configure --> Settings --> Projects and de-select one-to-one project mode.

## Get the Source
Go to:

* [https://github.com/AllStarLink/Voter](https://github.com/AllStarLink/Voter)
* Ensure you are on the "Master" branch (unless you specifically want a different branch)
* From the green Code dropdown button, select "Download Zip"

That will get you `voter-master.zip` which is a download of the whole VOTER tree from GitHub. Extract it somewhere that you can find.

## Programming the Bootloader
If you need to load the bootloader in to a fresh board, you will need to follow these steps.

1) Go to Project --> Open --> voter-bootloader.mcp --> Open
2) Go to File --> Import --> voter-bootloader --> ENC_C30.cof --> Open ***This step is missing from the 
original procedure.***
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
4) Go to Programmer --> Program. This will program the bootloder firmware into the PIC device on the board.


## (Re-)Compiling the Firmware
To compile the firmware (if you want to make custom changes)...

1) Go to Project --> Open --> navigate to the `VOTER_RTCM-firmware --> build-files` folder in the source archive, and open the `voter.mcp` file it finds
    * `voter.mcp` is for a plain VOTER
    * `voter-dspbew.mcp` is for a VOTER using DSP BEW firmware
    * `voter-smt.mcp` is for a plain RTCM
    * `voter-smt-dspbew.mcp` is for a RTCM using DSP BEW firmware
2) Go to Project --> Build Configuration and select `Release`. This may not be necessary (I don't believe that option is used in the firmware), but it removes the compiler option of `__DEBUG` being passed, so theoretically it would build "normal" firmware
    * If you get license errors trying to build a `Release` version, just leave it in `DEBUG`
3) Ensure you have the correct device selected, go to Configure --> Select Device
    * pick dsPIC33FJ128GP802 (VOTER) --> ok
    * pick dsPIC33FL128GP804 (RTCM) --> ok
4) Project --> Build All and it should build compile everything and show you `Build Succeeded`.

A `voter.cry` file should be in the `build-files` folder. You can load this with the "ENC Loader".

There is also a `.hex` file in there that you could load with a programmer... but that would wipe the bootloader... so don't do that.

If you want to compile the DSPBEW version, open the DSPBEW project file instead.

**NOTE:** As shown above, the `.mcp` files with the `smt` suffix are for the RTCM (built with SMT parts). The non-smt files are for the ORIGINAL through-hole VOTER boards. The difference is that the VOTER uses a dsPIC33FJ128GP802 and the RTCM uses a dsPIC33FJ128GP804.

**NOTE:** Be sure to select the correct device before you compile. If you don't select the right one, the board **will not boot**.

That's it!

Now you should be able to modify at-will. :)

## Baseband Examination Window (BEW) Firmware
Typically, the discriminator of an FM communications receiver produces results containing audio spectrum from the "sub-audible" range (typically < 100 Hz) to well above frequencies able to be produced by modulating audio. These higher frequencies can be utilized to determine signal quality, since they can only contain noise (or no noise, if a sufficiently strong signal is present).

For receivers (such as the Motorola Quantar, etc.) that do not provide sufficient spectral content at these "noise" frequencies (for various reasons), The "DSP/BEW (Digital Signal Processor / Baseband Examination Window)" feature of the VOTER/RTCM firmware may be utilized.

These receivers are perfectly capable of providing a valid "noise" signal with no modulation on the input of the receiver, but with strong modulation (high frequency audio and high deviation), it severely interferes with proper analysis of signal strength.

This feature provides a means by which a "Window" of baseband (normal audio range) signal is examined by a DSP and a determination of whether or not sufficient audio is present to cause interference of proper signal strength is made. During the VERY brief periods of time when it is determined that sufficient audio is present to cause interference, the signal strength value is "held" (the last valid value previous to the time of interference) until such time that the interfering audio is no longer present.

The DSP/BEW feature is selectable, and should not be used for a receiver that does not need it.

***If you compile/load firmware in the VOTER/RTCM with BEW features, you will LOSE the Diagnostics Menu, as there isn't enough room in the dsPIC for both!***

Firmware compiled with the `BEW` option will indicate such in the version at the top of the console screen on the device.
