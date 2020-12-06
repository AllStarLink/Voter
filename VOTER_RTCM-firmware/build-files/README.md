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

This version also adds correction for leap seconds in TSIP devices. Some TSIP devices (ie Resolution T) report their time in GPS time, not UTC. That means that they lag UTC time by the current number of "leap seconds". If you have ALL the same devices in your system, this isn't a problem, since they will be all off by the same number of leap seconds, and chan_voter won't care.

However, if you introduce another type of GPS that reports time in UTC (ie a uBlox), that device will never get voted (silent fail), as while it will connect to the server, it will get excluded from voting by chan_voter, due to the time differential.

This fix examines the Primary Timing Packet from the TSIP receiver, and looks at the flags to see if it is using GPS time, or UTC time. If it is using GPS time, it then takes the supplied UTC offset (current leap seconds), and subtracts it from GPS time, to syncronize this device with UTC time, so it will play well with others.

Added additional bytes (gps_buf[2] and [3]) to the TSIP debug to see Receiver mode and Discipline Mode. No checks currently implemented against them (memory constraints).

Fixed the check of Supplemental Timing Packet 0xAC Minor Alarms, gps_buf bytes were swapped. Not critical, as we are checking for everything to be 0 (no alarms) anyways, but debugging makes more sense when we are looking at the right bits. gps_buf[12] is the low byte (Bits 0-7), and gps_buf[11] is the high byte (Bits 8-12).

