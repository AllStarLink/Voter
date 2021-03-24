Firmware Changelog

1.50 04/25/2015
This is the base version of the repository for the initial commits, as far as we can tell.

1.51 08/07/2017
Adds a patch to process_gps in voter.c for TSIP receivers, targeted/assumed to be Trimble Thunderbolts to fix a 1997 date issue:

gps_time = (DWORD) mktime(&tm) + 619315200; 

This effectively added 1024 weeks to the time, to correct the date.

Note, this is a CRUDE fix, and likely breaks other Trimble GPS' that speak TSIP. To be resolved in a future version.


1.60 12/06/2020
Reverts the above patch, and replaces it with some logic. Adds a new configuration option (81), to identify if the GPS being used is a Trimble Tunderbolt. If it is, then the GPS week reported by the GPS is evaluated, and the appropriate correction is applied to the time, either adding 1024 or 2048 weeks.

This version also adds correction for leap seconds in TSIP devices. Some TSIP devices (ie Resolution T) report their time in GPS time, not UTC. That means that they lead UTC time by the current number of "leap seconds". If you have ALL the same devices in your system, this isn't a problem, since they will be all off by the same number of leap seconds, and chan_voter won't care.

However, if you introduce another type of GPS that reports time in UTC (ie a uBlox), that device will never get voted (silent fail), as while it will connect to the server, it will get excluded from voting by chan_voter, due to the time differential.

This fix examines the Primary Timing Packet from the TSIP receiver, and looks at the flags to see if it is using GPS time, or UTC time. If it is using GPS time, it then takes the supplied UTC offset (current leap seconds), and subtracts it from GPS time, to synchronize this device with UTC time, so it will play well with others.

Added additional bytes (gps_buf[2] and [3]) to the TSIP debug to see Receiver mode and Discipline Mode. No checks currently implemented against them (memory constraints).

Fixed the check of Supplemental Timing Packet 0xAC Minor Alarms, gps_buf bytes were swapped. Not critical, as we are checking for everything to be 0 (no alarms) anyways, but debugging makes more sense when we are looking at the right bits. gps_buf[12] is the low byte (Bits 0-7), and gps_buf[11] is the high byte (Bits 8-12).


1.61 1/11/2021
The mktime() sub-routine in MPLAB C30 has a bug, see https://www.microchip.com/forums/m653169.aspx

After 12/31/2020 23:59:59, mktime() now returns -1, instead of epoch time. That BREAKS the firmware, as on boot, the 
date/time starts counting from epoch, and nothing will syncrohize anymore, since all VOTER/RTCM's will have different 
times once restarted. The main receiver will still receive, you just lose all voting.

David Maciorowski, WA1JHK, wrote a patch to replace using mktime(). It takes the known value of epoch seconds up until 
01/01/2021 00:00:00, and then uses the time/date from the GPS to add the offset to current time/date. Crude, but effective, 
since we don't care about time in the past, only need to know the time now.


2.00 3/24/2021
This version drops the original squelch code (which actually had a bug in it), and makes "Chuck Squelch" the default squelch. As such, all binaries will have Chuck Squelch, there will be no binaries compiled with the original squelch (that code has been removed).

Add some comments to the source, trying to figure out what some parts do. Looks like the un-documented "Saywer" mode forces the PL filter OUT of the receive audio path, when in OFFLINE mode, if enabled (Sawyer=1).

Remove the "autoconfiguration" of the baud rate, and resetting PPS/GPS polarity to 0 when changing to/from NMEA/TSIP. This just adds confusion when trying to set up a GPS. Leave the baud and polarity settings alone.

This version reverses the logic for ToS/DSCP marking of packets. Now, by default, we will mark all packets outbound from the VOTER/RTCM with DSCP 48 (802.1p Class 6 aka Network Control ToS). Debug Level 16 now DISABLES ToS, changing the packets to Routine. Don't forget, you still need utos=y in your voter.conf to mark packets from the server TO the VOTER/RTCM.

Add another GPS debug feature to help determine PPS polarity. GPS debug will now report if you have PPS configured (set to 0 or 1), but it doesn't see detect a PPS pulse. This is likely because you are using the wrong polarity. Also added entry to 98-status menu to show if the PPS is bad (and suggest checking polarity). If PPS is set to ignore, the status menu will show 0 anyways, since it is not used.

Add another menu config option (82) to allow you to add an arbitrary number of seconds to this device's GPS time, in order to synchronize with the master. Different brands have different firmware bugs, and may not always come up with the right rime. This makes it easier to line those times up, as long as it is a consistent offset. ie. if you need to add 19.4 years, that would be 19.4 * 365 * 24 * 60 * 60 = 611798400 seconds. This is similar to the change proposed by Chuck Henderson (WB9UUS), except that it adds it to the main menu, and allows for an arbitrary amount of time, up to 25 years.

3.00 3/24/2021
This is another major release, as it introduces the ability to remotely adjust the squelch of the VOTER/RTCM.

By default (originially), the Squelch Pot (R22) just sets a voltage on an ADC line of the PIC. Based on the ADC value read, the setting of the squelch is determined.

It is often desired to be able to remotely tune the squelch of the receiver, without having to drive to the radio site to "diddle the pot", and as it turns out, this is relatively trivial to do in software, by directly setting the value that would normally read from the ADC.

In addition, there is a "squelch tunable" in the firmware, called "hysteresis", that will be brought out so it can be adjusted without having to recompile the firmware. By default, this has always been set to "24", unless you specifically changed it, and compiled your own firmware.

Therefore, this version adds a new (S)quelch menu, that lets you adjust the squelch level and hysteresis remotely.

Note, due to space constraints in the PIC, the option to display the "diagnostic cable pinout" has been removed from the diagnostic meny. This allows us to have the option to select using the hardware squelch pot, or software squelch pot.

When using the software squelch setting, the change is immediate, but do not forget to save the EEPROM settings (99) when you are done your adjustment, to make it permanent.
