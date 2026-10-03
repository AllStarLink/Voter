# Firmware Changelog

## 4.01 10/02/2026
Version 4.01 is a bug fix for how we handle receive packets with the ENC28J60.

### Bug Fixes
See https://github.com/AllStarLink/Voter/issues/36 for details.

Properly handle the ENG28J60 EIR_RXERIF to track receive buffer overflows. Expose it as Debug 2, which is an EVENT counter to indicate that an overflow event occurred (it isn't a counter of how many packets overflowed).

We also add a limitation on the number of packets `StackTask()` can process each time it is called. If the traffic is VOTER related, it returns immediately after processing the packet (as it did before). The change limits the number of un-related packets we process to four. The reason for the change is that if the client is being flooded by unrelated UDP or broadcast traffic, we previously kept processing packets until the receive buffer is empty. 

The previous behaviour could lead to a situation where we are processing packets (and discarding the ones we don't want), while `lastrxtimer` keeps getting incremented by the ADC ISR. If we are still processing packets, and don't get any VOTER-related ones to return us to the main processing loop, `lastrxtimer` can expire (after 6 seconds), causing us to think we've not heard from the host, and resetting the connection.

The new behaviour puts a hard limit on the number of packets we can process, before returning to do other necessary work.

The limit is set at four, as a somewhat arbitrary number, due to the small buffer in the ENC28J60. It could be changed, if required, based on field testing. It is a tradeoff between throughput and leaving packets in the receive buffer.


## 4.00 9/30/2026
Version 4.00 is a major code change, focusing on updating the TCP/IP Stack, removing the (broken) ADPCM audio support, and changing the project to use the newer MPLAB X IDE and XC16 compiler.

### Feature Updates
This version removes support for ADPCM audio. See https://github.com/AllStarLink/app_rpt/issues/878 for further details of why this was removed.

Upgrade the TCP/IP stack to Microchip Libraries for Applications (MLA) v2013-06-15 (aka version 5.42.08). This is the most recent TCP/IP stack available for the Ethernet chip used in this project. It brings a number of upstream bug fixes to how UDP and ARP are handled, and allowed for some firmware optimization by utilizing the Microchip ARP functions, instead of custom ones.

Added .hex files for manually loading the bootloader into a fresh dsPIC. The previous method required using the legacy MPLAB IDE, which may be difficult moving forward.

### Code Cleanup
Address outstanding "todo" items in the code (cleaning up comments and removing dead code).

Refactor the code base to move away from the legacy MPLAB IDE and C30 compiler, and move to using the MPLAB X IDE (v5.5) and XC16 Compiler (v1.36b). This lets the firmware continue development on newer operating systems. Some firmware modifications were required (configuration bit setting in particular) to be compliant with new requirements. New customized linker script files (`.gld`) were also required for the new compiler, as we are using a custom firmware map due to the addition of a bootloader.

Removed extraneous TCP/IP stack files that were included in the project build, but not actually used for any of their functions.

### Bug Fixes
Back when the Software Squelch menu was added, the Hysteresis variable was exposed as a tunable. Another bug was identified where on a blank configuration EEPROM, the Hysteresis value could get set to an out of range value. This update sets the value to the default of 24, if the value read from the EEPROM is >100.

Fixed a bug identified during the TCP/IP stack upgrade that may have not properly been setting the duplex of the Ethernet chip (it may have only change the PHY and not the MAC layers).

Fixed a Telnet login security issue that could have lead to an un-authenticated login.

Fixed issues identified with UART flag handling.

Fixed upstream bugs found in the Microchip TCP/IP stack (Helpers.c and TCP.c).

Fixed a potential crash if `log2fix(0)` is called (it shouldn't ever be), which is undefined behaviour. Added a guard against that situation.

Guard against receiving malformed (short) ulaw audio packets.


## 3.30 09/15/2026
Version 3.30 is a maintenance release, focusing on resource and compiler optimization to reduce code size in the PIC.

This is likely the last of the version 3.xx code versions. Work is underway to transition to the MPLAB X IDE and newer XC16 compiler. While that won't change any interoperability, it will require some significant changes to how things get built.

### Bug Fixes
A potential issue was identified in the TSIP routines. Resetting TSIPwasdle at the same time is important: an overlength/corrupt TSIP packet should return the parser to its idle state rather than carrying the DLE state into the next packet.

This fixes parser-state recovery after an overlong TSIP packet, not an actual buffer overflow.

### Code Cleanup
Remove inclusion/compilation of the ICMP client. We can't send pings from the device, we only respond to them, which uses the ICMP server code. Remove the code inclusion to reduce code size.

Fix compiler optimizations in the project files to reduce code size.

Reduce the number of TCP/IP sockets allocated. This was set to the default (10), but there is no way we could ever open that many sockets. Each extra reserved socket used up resources on the PIC. Reduced the default number of sockets to free up resources. It can probably be reduced further, but this is a good start.

## 3.20 09/11/2026

Version 3.20 is a bug fix update, with some minor feature enhancements.

### Bug Fixes
Back around version 3.0, the Squelch menu, along with the Hysteresis parameter were added. The default Hysteresis is supposed to be 24, but that default value never got checked/updated/written into the EEPROM... so it always remained 0 (as that was how the EEPROM is initialized). This version checks to see if the EEPROM address is 0, and updates it to 24 if it is. If there is already a non-zero value there, it does nothing.

Code analysis found that the GPS Serial Polarity (Menu 9) and GPS Baud Rate (Menu 11) didn't update when changed, requiring a reboot to take effect. This release will cause those items to update immediately when changed (you still need to save the changes though to write them into the EEPROM).

### Feature Enhancement
Code analysis found that there is an un-documented "Mode 2" of DSP-BEW that makes the DSP-BEW mode twice as sensitive to energy in the passband (Mode 1 halves the amplitude of the samples before operating on them). This release updates Menu 17 to show both modes.

### Code Documentation/Cleanup
Voter.c was run through the ASL Clang formatter, and a significant number of formatting updates were made to bring it closer to ASL coding standards.

A significant amount of commenting was added to sections of Voter.c to better explain RSSI and DSP-BEW operation.

## 3.10 01/11/2026
This version is a long overdue code cleanup and optimization.

The biggest change is a refactoring of the GPS routines for both NMEA and TSIP GPS. We only use the $GPGGA and $GPRMC NMEA sentences ($GPGSV provides no added value). Do a better job of validating our GPS fix quality, and responding better when we don't have a suitable GPS signal to use.

Revise the Status Menu (98) for better readability. Change "1" and "0" status to more human-friendly results.

Update other status and debug messages for better descriptions of actual conditions.

Add functionality to the Status Menu to identify what the last reboot cause was of the device.

Update how PPS is qualified, and how the debug messages are interpreted and displayed.

Change the default UDP port to 1667, to match ASL3.

Change the default GPS baud rate to 9600, which is now more common.

Move ulaw and ADPCM encoding into functions, to save code space.

Remove extra/unused variables.

Fix a number of typos, spelling, and grammar.

Change some variable names and defines for better readability.

Better error checking for various conditions and explicitly set some variables to known conditions to prevent issues.

Fix a potential index out of bounds issue by setting index value based on received audio type. 

SIGNIFICANT comments added to the source code to aid in functional understanding of how most things work.

Through hole VOTER board firmware image files are denoted by `VOTER_`. MicroNode RTCM firmware image files are denoted by `RTCM_`. DSPBEW images contain DSP RSSI processing (from in-band audio), and do NOT contain the Diagnostic Menu function.

## 3.01 12/28/2025
Un-released version. 

Code cleanup in the source to standardize formatting. There should be no functional changes over version 3.00. Version bumped for testing/tracking only.

## 3.00 3/24/2021
This is another major release, as it introduces the ability to remotely adjust the squelch of the VOTER/RTCM.

By default (originally), the Squelch Pot (R22) just sets a voltage on an ADC line of the PIC. Based on the ADC value read, the setting of the squelch is determined.

It is often desired to be able to remotely tune the squelch of the receiver, without having to drive to the radio site to "diddle the pot", and as it turns out, this is relatively trivial to do in software, by directly setting the value that would normally read from the ADC.

In addition, there is a "squelch tunable" in the firmware, called "hysteresis", that will be brought out so it can be adjusted without having to recompile the firmware. By default, this has always been set to "24", unless you specifically changed it, and compiled your own firmware.

Therefore, this version adds a new (S)quelch menu, that lets you adjust the squelch level and hysteresis remotely.

Note, due to space constraints in the PIC, the option to display the "diagnostic cable pinout" has been removed from the diagnostic menu. This allows us to have the option to select using the hardware squelch pot, or software squelch pot.

When using the software squelch setting, the change is immediate, but do not forget to save the EEPROM settings (99) when you are done your adjustment, to make it permanent.

**NOTE:** ***You will need to manually set the Hysteresis to 24 and save, if you use this version.*** Hopefully, this will be resolved in a future release to set it to 24 by default.

## 2.00 3/24/2021
This version drops the original squelch code (which actually had a bug in it), and makes "Chuck Squelch" the default squelch. As such, all binaries will have Chuck Squelch, there will be no binaries compiled with the original squelch (that code has been removed).

Add some comments to the source, trying to figure out what some parts do. Looks like the un-documented "Saywer" mode forces the PL filter OUT of the receive audio path, when in OFFLINE mode, if enabled (Sawyer=1).

Remove the "autoconfiguration" of the baud rate, and resetting PPS/GPS polarity to 0 when changing to/from NMEA/TSIP. This just adds confusion when trying to set up a GPS. Leave the baud and polarity settings alone.

This version reverses the logic for ToS/DSCP marking of packets. Now, by default, we will mark all packets outbound from the VOTER/RTCM with DSCP 48 (802.1p Class 6 aka Network Control ToS). Debug Level 16 now DISABLES ToS, changing the packets to Routine. Don't forget, you still need `utos=y` in your `voter.conf` to mark packets from the server TO the VOTER/RTCM.

Add another GPS debug feature to help determine PPS polarity. GPS debug will now report if you have PPS configured (set to 0 or 1), but it doesn't see detect a PPS pulse. This is likely because you are using the wrong polarity. Also added entry to 98-status menu to show if the PPS is bad (and suggest checking polarity). If PPS is set to ignore, the status menu will show 0 anyways, since it is not used.

Add another menu config option (82) to allow you to add an arbitrary number of seconds to this device's GPS time, in order to synchronize with the master. Different brands have different firmware bugs, and may not always come up with the right rime. This makes it easier to line those times up, as long as it is a consistent offset. ie. if you need to add 19.4 years, that would be 19.4 * 365 * 24 * 60 * 60 = 611798400 seconds. This is similar to the change proposed by Chuck Henderson (WB9UUS), except that it adds it to the main menu, and allows for an arbitrary amount of time, up to 25 years.

## 1.61 1/11/2021
The `mktime()` sub-routine in MPLAB C30 has a bug, see [https://www.microchip.com/forums/m653169.aspx](https://www.microchip.com/forums/m653169.aspx).

After 12/31/2020 23:59:59, `mktime()` now returns `-1`, instead of epoch time. That **BREAKS** the firmware, as on boot, the date/time starts counting from epoch, and nothing will synchronize anymore, since all VOTER/RTCM's will have different times once restarted. The main receiver will still receive, you just lose all voting.

David Maciorowski, WA1JHK, wrote a patch to replace using `mktime()`. It takes the known value of epoch seconds up until 01/01/2021 00:00:00, and then uses the time/date from the GPS to add the offset to current time/date. Crude, but effective, since we don't care about time in the past, only need to know the time now.

## 1.60 12/06/2020
Reverts the below patch, and replaces it with some logic. Adds a new configuration option (81), to identify if the GPS being used is a Trimble Thunderbolt. If it is, then the GPS week reported by the GPS is evaluated, and the appropriate correction is applied to the time, either adding 1024 or 2048 weeks.

This version also adds correction for leap seconds in TSIP devices. Some TSIP devices (ie Resolution T) report their time in GPS time, not UTC. That means that they lead UTC time by the current number of "leap seconds". If you have ALL the same devices in your system, this isn't a problem, since they will be all off by the same number of leap seconds, and chan_voter won't care.

However, if you introduce another type of GPS that reports time in UTC (ie a uBlox), that device will never get voted (silent fail), as while it will connect to the server, it will get excluded from voting by `chan_voter`, due to the time differential.

This fix examines the Primary Timing Packet from the TSIP receiver, and looks at the flags to see if it is using GPS time, or UTC time. If it is using GPS time, it then takes the supplied UTC offset (current leap seconds), and subtracts it from GPS time, to synchronize this device with UTC time, so it will play well with others.

Added additional bytes (`gps_buf[2]` and `[3]`) to the TSIP debug to see Receiver mode and Discipline Mode. No checks currently implemented against them (memory constraints).

Fixed the check of Supplemental Timing Packet `0xAC` Minor Alarms, `gps_buf` bytes were swapped. Not critical, as we are checking for everything to be 0 (no alarms) anyways, but debugging makes more sense when we are looking at the right bits. `gps_buf[12]` is the low byte (Bits 0-7), and `gps_buf[11]` is the high byte (Bits 8-12).

## 1.51 08/07/2017
Adds a patch to `process_gps` in `voter.c` for TSIP receivers, targeted/assumed to be Trimble Thunderbolts to fix a 1997 date issue:

`gps_time = (DWORD) mktime(&tm) + 619315200;` 

This effectively added 1024 weeks to the time, to correct the date.

Note, this is a CRUDE fix, and likely breaks other Trimble GPS' that speak TSIP. To be resolved in a future version.

## 1.50 04/25/2015
This is the base version of the repository for the initial commits, as far as we can tell.













