Firmware Changelog

1.50 04/25/2015
This is the base version of the repository for the initial commits, as far as we can tell.

1.51 08/07/2017
Adds a patch to process_gps in voter.c for TSIP receivers, targeted/assumed to be Trimble Thunderbolts to fix a 1997 date issue:

gps_time = (DWORD) mktime(&tm) + 619315200; 

This effectively added 1024 weeks to the time, to correct the date.

Note, this is a CRUDE fix, and likely breaks other Trimble GPS' that speak TSIP. To be resolved in a future version.


