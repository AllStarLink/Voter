/*
* VOTER Client System Firmware for VOTER board
*
* Copyright (C) 2011
* Jim Dixon, WB6NIL <jim@lambdatel.com>
*
* This file is part of the VOTER System Project 
*
*   VOTER System is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.

*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************
For IMA ADPCM Codec:

Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*******************************************************************
Note: It would have been nicer to (1) use G.726 rather than this
ancient version of ADPCM, but G.726 was a bit too computationally
complex for this hardware platform, and (2) *not* to have to transcode
the tx audio into mulaw before outputting it, but there is no room in
RAM for signed linear audio of the necessary buffer size; sigh!

******************************************************************/

#define THIS_IS_STACK_APPLICATION

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <math.h>


#define M_PI       3.14159265358979323846

/* Un-comment this to generate digital milliwatt level on output */
/* #define DMWDIAG */

// Include all headers for any enabled TCPIP Stack functions
#include "TCPIP Stack/TCPIP.h"

#if defined(SMT_BOARD)

	#define	CTCSSIN	_RA7
	#define	JP8	_RA8
	#define	JP9	_RA9
	#define	JP10 _RA10
	#define	JP11 _RB9
	
	#define	INITIALIZE JP8 // Short JP8 on powerup to initialize EEPROM
	#define	INITIALIZE_WVF JP10  // Short on powerup while JP8 is shorted to also initialize Diode VF
	
	#define TESTBIT _LATB8

#else

	#define	IOEXP_IODIRA 0
	#define	IOEXP_IODIRB 1
	#define IOEXP_IOPOLA 2
	#define	IOEXP_IOPOLB 3
	#define	IOEXP_GPINTENA 4
	#define	IOEXP_GPINTENB 5
	#define	IOEXP_DEFVALA 6
	#define	IOEXP_DEFVALB 7
	#define IOEXP_INTCONA 8
	#define	IOEXP_INTCONB 9
	#define	IOEXP_IOCON 10
	#define	IOEXP_GPPUA 12
	#define	IOEXP_GPPUB 13
	#define	IOEXP_INTFA 14
	#define	IOEXP_INTFB 15
	#define	IOEXP_INTCAPA 16
	#define IOEXP_INTCAPB 17
	#define	IOEXP_GPIOA 18
	#define	IOEXP_GPIOB 19
	#define	IOEXP_OLATA 20
	#define	IOEXP_OLATB 21
	
	#define	CTCSSIN	(inputs1 & 0x10)
	#define	JP8	(inputs1 & 0x40)
	#define	JP9	(inputs1 & 0x80)
	#define	JP10 (inputs2 & 1)
	#define	JP11 (inputs2 & 2)
	
	#define	INITIALIZE (IOExp_Read(IOEXP_GPIOA) & 0x40) // Short JP8 on powerup to initialize EEPROM
	#define	INITIALIZE_WVF (IOExp_Read(IOEXP_GPIOB) & 1)  // Short on powerup while JP8 is shorted to also initialize Diode VF
	
	#define TESTBIT _LATA1

#endif

#define WVF JP10				// Short on pwrup to initialize default values
#define CAL JP9						// Short to calibrate squelch noise. Shorting the INITIALIZE jumper while
									// this is shorted also calibrates the temp. conpensation diode (at room temp.)
#define	LEVDISP JP11			// Short to change GPS/CONNECT LED's to be audio level display97:

#define SYSLED 0
#define	SQLED 1
#define	GPSLED 2
#define CONNLED 3

#define	BAUD_RATE1 57600
#define	BAUD_RATE2 4800

#define	FRAME_SIZE 160
#define	ADPCM_FRAME_SIZE 320
#define	MAX_BUFLEN 6400 // 0.8 seconds of buffer
#define	DEFAULT_TX_BUFFER_LENGTH 3000 // approx 300ms of Buffer
#define	VOTER_CHALLENGE_LEN 10
#define	ADCOTHERS 3
#define	ADCSQNOISE 0
#define	ADCDIODE 2
#define	ADCSQPOT 1
#define NOISE_SLOW_THRESHOLD 500   // Figures seem to be stable >= 20db quieting or so
#define DEFAULT_VOTER_PORT 667
#define	PPS_WARN_TIME (1200 * 8) // 1200ms PPS Warning Time
#define PPS_MAX_TIME (2400 * 8) // 2400 ms PPS Timeout
#define	GPS_NMEA_WARN_TIME (1200 * 8) // 1200 ms GPS Warning Time
#define GPS_NMEA_MAX_TIME (2400 * 8) // 2400 ms GPS Timeout
#define	GPS_TSIP_WARN_TIME (5000ul * 8ul) // 5000 ms GPS Warning Time
#define GPS_TSIP_MAX_TIME (10000ul * 8ul) // 10000 ms GPS Timeout
#define GPS_FORCE_TIME (1500 * 8)  // Force a GPS (Keepalive) every 1500ms regardless
#define ATTEMPT_TIME (500 * 8) // Try connection every 500 ms
#define LASTRX_TIME (3000 * 8) // Timeout if nothing heard after 3 seconds
#define TELNET_TIME (100 * 8) // Telnet output buffer timer (quasi-Nagle algorithm)
#define	MASTER_TIMING_DELAY 50 // Delay to send packet if not master (in 125us increments)
#define	NCOLS 75
#define	LEVDISP_FACTOR 25
#define	TSIP_FACTOR 57.295779513082320876798154814105
#define	ULAW_SILENCE 0xff
#define	ADPCM_SILENCE 0
#define	USE_PPS ((AppConfig.PPSPolarity != 2) && (!indiag))
#define	DIAG_WAIT_UART (TICK_SECOND / 3ul)
#define	DIAG_WAIT_MEAS (TICK_SECOND * 2)
#define	DIAG_NOISE_GAIN 0x28
#define	NAPEAKS 50

struct meas {
	WORD freq;
	WORD min;
	WORD max;
	BOOL issql;
} ;

// Unfortunately, when a signal gets sufficiently weak, its noise readings go all over the place
// On a sufficently strong signal, we use a noise average over 32 squelch sample periods (128ms)
// which is good and responsive to track a fluttery mobile station accurately. On a weaker signal
// we have to use a longer (1024ms) average, which still gives a good indication of signal strength,
// but is incapable of tracking to full extent a really fluttery signal. This should be more then
// sufficient for voting descisions.


enum {GPS_STATE_IDLE,GPS_STATE_RECEIVED,GPS_STATE_VALID,GPS_STATE_SYNCED} ;
enum {GPS_NMEA,GPS_TSIP} ;
enum {CODEC_ULAW,CODEC_ADPCM} ;

ROM char gpsmsg1[] = "GPS Receiver Active, waiting for aquisition\n", gpsmsg2[] = "GPS signal acquired, number of satellites in view = ",
	gpsmsg3[] = "  Time now syncronized to GPS\n", gpsmsg5[] = "  Lost GPS Time synchronization\n",
	gpsmsg6[] = "  GPS signal lost entirely. Starting again...\n",gpsmsg7[] = "  Warning: GPS Data time period elapsed\n",
	gpsmsg8[] = "  Warning: GPS PPS Signal time period elapsed\n",gpsmsg9[] = "GPS signal acquired\n";

typedef struct {
	DWORD vtime_sec;
	DWORD vtime_nsec;
} VTIME;

typedef struct {
	VTIME curtime;
	BYTE challenge[VOTER_CHALLENGE_LEN];
	DWORD digest;
	WORD payload_type;
} VOTER_PACKET_HEADER;

#define OPTION_FLAG_FLATAUDIO 1	// Send Flat Audio
#define	OPTION_FLAG_SENDALWAYS 2	// Send Audio always
#define OPTION_FLAG_NOCTCSSFILTER 4 // Do not filter CTCSS
#define	OPTION_FLAG_MASTERTIMING 8  // Master Timing Source (do not delay sending audio packet)
#define	OPTION_FLAG_ADPCM 16 // Use ADPCM rather then ULAW
#define	OPTION_FLAG_MIX 32 // Request "Mix" option to host


#ifdef DMWDIAG
unsigned char ulaw_digital_milliwatt[8] = { 0x1e, 0x0b, 0x0b, 0x1e, 0x9e, 0x8b, 0x8b, 0x9e };
BYTE mwp;
#endif

VTIME system_time;

// Declare AppConfig structure and some other supporting stack variables
APP_CONFIG AppConfig;
BYTE AN0String[8];
void SaveAppConfig(void);


// Private helper functions.
// These may or may not be present in all applications.
static void InitAppConfig(void);
static void InitializeBoard(void);

// squelch RAM variables intended to be read by other modules
extern BOOL cor;					// COR output
extern BOOL sqled;					// Squelch LED output
extern BOOL write_eeprom_cali;			// Flag to write calibration values back to EEPROM
extern BYTE noise_gain;			// Noise gain sent to digital pot
extern WORD caldiode;

#ifdef DUMPENCREGS
extern void DumpETHReg(void);
#endif

void service_squelch(WORD diode,WORD sqpos,WORD noise,BOOL cal,BOOL wvf,BOOL iscaled);
void init_squelch(void);
BOOL set_atten(BYTE val);

/////////////////////////////////////////////////
//Global variables 
BYTE inputs1;
BYTE inputs2;
BYTE aliveCntrMain; //Alive counter must be reset each couple of ms to prevent board reset. Set to 0xff to disable.
BOOL aliveCntrDec;
BYTE filling_buffer;
WORD fillindex;
BOOL filled;
BYTE audio_buf[2][FRAME_SIZE + 3];
BOOL connected;
BYTE rssi;
BYTE gps_buf[160];
BYTE gps_bufindex;
BYTE TSIPwasdle;
BYTE gps_state;
BYTE gps_nsat;
BOOL gpssync;
BOOL gotpps;
DWORD gps_time;
BYTE ppscount;
DWORD last_interval;
BYTE lockcnt;
BOOL adcother;
WORD adcothers[ADCOTHERS];
BYTE adcindex;
BYTE sqlcount;
BOOL sql2;
WORD vnoise32;
DWORD vnoise256;
BOOL wascor;
BOOL lastcor;
BYTE option_flags;
static struct {
	VOTER_PACKET_HEADER vph;
	BYTE rssi;
	BYTE audio[FRAME_SIZE + 3];
} audio_packet;
static struct {
	VOTER_PACKET_HEADER vph;
	char lat[9];
	char lon[10];
	char elev[7];
} gps_packet;
WORD txdrainindex;
WORD last_drainindex;
BYTE txaudio[MAX_BUFLEN];
VTIME lastrxtime;
BOOL ptt;
DWORD gpstimer;
WORD ppstimer;
WORD gpsforcetimer;
WORD attempttimer;
WORD lastrxtimer;
BOOL gpswarn;
BOOL ppswarn;
UDP_SOCKET udpSocketUser;
NODE_INFO udpServerNode;
DWORD dwLastIP;
BOOL aborted;
BOOL inread;
IP_ADDR MyVoterAddr;
IP_ADDR LastVoterAddr;
IP_ADDR CurVoterAddr;
WORD dnstimer;
BOOL dnsdone;
DWORD timing_time;
WORD timing_index;
DWORD next_time;
DWORD next_index;
WORD samplecnt;
WORD last_adcsample;
DWORD real_time;
WORD last_samplecnt;
DWORD digest;
DWORD resp_digest;
DWORD mydigest;
BOOL sendgps;
BYTE dnsnotify;
long discfactor;
long discounterl;
long discounteru;
short amax;
short amin;
WORD apeak;
BOOL indisplay;
BOOL indipsw;
short enc_valprev;	/* Previous output value */
char enc_index;		/* Index into stepsize table */
short enc_prev_valprev;	/* Previous output value */
char enc_prev_index;	/* Index into stepsize table */
BYTE enc_lastdelta;
BYTE dec_buffer[FRAME_SIZE * 2];
short dec_valprev;	/* Previous output value */
char dec_index;		/* Index into stepsize table */
BOOL time_filled;
long host_txseqno;
long txseqno_ptt;
long txseqno;
DWORD elketimer;
WORD testidx;
short *testp;
BOOL indiag;
BYTE leddiag;
BYTE diagstate;
BYTE measretstate;
BYTE errcnt;
struct meas *measp;
BYTE measidx;
char *measstr;
BYTE diag_option_flags;
WORD apeaks[NAPEAKS];
BOOL netisup;
BOOL inprocloop;
BOOL ininput;
BOOL telnet_echo;
WORD termbufidx;
WORD termbuftimer;
BYTE linkstate;

char their_challenge[VOTER_CHALLENGE_LEN],challenge[VOTER_CHALLENGE_LEN];

#ifdef SILLY
BYTE silly = 0;
DWORD sillyval;
#endif

BYTE myDHCPBindCount;

#if !defined(STACK_USE_DHCP)
     //If DHCP is not enabled, force DHCP update.
    #define DHCPBindCount 1
#endif

static ROM BYTE rssitable[] = {
255,229,214,203,195,188,182,177,
172,168,165,161,158,156,153,150,
148,146,144,142,140,138,137,135,
134,132,131,129,128,127,125,124,
123,122,121,120,119,118,117,116,
115,114,113,112,111,111,110,109,
108,107,107,106,105,104,104,103,
102,102,101,100,100,99,99,98,
97,97,96,96,95,95,94,94,
93,93,92,91,91,91,90,90,
89,89,88,88,87,87,86,86,
86,85,85,84,84,83,83,83,
82,82,81,81,81,80,80,80,
79,79,79,78,78,77,77,77,
76,76,76,75,75,75,75,74,
74,74,73,73,73,72,72,72,
71,71,71,71,70,70,70,69,
69,69,69,68,68,68,68,67,
67,67,67,66,66,66,65,65,
65,65,65,64,64,64,64,63,
63,63,63,62,62,62,62,61,
61,61,61,61,60,60,60,60,
59,59,59,59,59,58,58,58,
58,58,57,57,57,57,57,56,
56,56,56,56,55,55,55,55,
55,54,54,54,54,54,54,53,
53,53,53,53,52,52,52,52,
52,52,51,51,51,51,51,51,
50,50,50,50,50,50,49,49,
49,49,49,49,48,48,48,48,
48,48,47,47,47,47,47,47,
47,46,46,46,46,46,46,45,
45,45,45,45,45,45,44,44,
44,44,44,44,44,43,43,43,
43,43,43,43,43,42,42,42,
42,42,42,42,41,41,41,41,
41,41,41,41,40,40,40,40,
40,40,40,39,39,39,39,39,
39,39,39,38,38,38,38,38,
38,38,38,38,37,37,37,37,
37,37,37,37,36,36,36,36,
36,36,36,36,36,35,35,35,
35,35,35,35,35,35,34,34,
34,34,34,34,34,34,34,33,
33,33,33,33,33,33,33,33,
32,32,32,32,32,32,32,32,
32,32,31,31,31,31,31,31,
31,31,31,31,30,30,30,30,
30,30,30,30,30,30,29,29,
29,29,29,29,29,29,29,29,
29,28,28,28,28,28,28,28,
28,28,28,27,27,27,27,27,
27,27,27,27,27,27,26,26,
26,26,26,26,26,26,26,26,
26,26,25,25,25,25,25,25,
25,25,25,25,25,24,24,24,
24,24,24,24,24,24,24,24,
24,23,23,23,23,23,23,23,
23,23,23,23,23,22,22,22,
22,22,22,22,22,22,22,22,
22,22,21,21,21,21,21,21,
21,21,21,21,21,21,21,20,
20,20,20,20,20,20,20,20,
20,20,20,20,19,19,19,19,
19,19,19,19,19,19,19,19,
19,19,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,
17,17,17,17,17,17,17,17,
17,17,17,17,17,17,16,16,
16,16,16,16,16,16,16,16,
16,16,16,16,16,15,15,15,
15,15,15,15,15,15,15,15,
15,15,15,15,14,14,14,14,
14,14,14,14,14,14,14,14,
14,14,14,13,13,13,13,13,
13,13,13,13,13,13,13,13,
13,13,13,12,12,12,12,12,
12,12,12,12,12,12,12,12,
12,12,12,12,11,11,11,11,
11,11,11,11,11,11,11,11,
11,11,11,11,11,10,10,10,
10,10,10,10,10,10,10,10,
10,10,10,10,10,10,9,9,
9,9,9,9,9,9,9,9,
9,9,9,9,9,9,9,9,
8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,
8,8,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,
7,7,7,7,6,6,6,6,
6,6,6,6,6,6,6,6,
6,6,6,6,6,6,6,6,
5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,
5,5,5,4,4,4,4,4,
4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,
3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,
3,3,3,3,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0
};


static ROM long crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/* 1000.h: Generated from frequency 1000
   by gentone.  16 samples  */
static ROM short test_1000[] = {16,
            0,  6269, 11585, 15136, 16384, 15136, 11585,  6269,
            0, -6269, -11585, -15136, -16384, -15136, -11585, -6269,

};
/* 100.h: Generated from frequency 100
   by gentone.  160 samples  */
static ROM short test_100[] = {160,
            0,   643,  1285,  1925,  2563,  3196,  3824,  4447,
         5062,  5670,  6269,  6859,  7438,  8005,  8560,  9102,
         9630, 10143, 10640, 11121, 11585, 12031, 12458, 12866,
        13254, 13622, 13969, 14294, 14598, 14879, 15136, 15371,
        15582, 15768, 15931, 16069, 16182, 16270, 16333, 16371,
        16384, 16371, 16333, 16270, 16182, 16069, 15931, 15768,
        15582, 15371, 15136, 14879, 14598, 14294, 13969, 13622,
        13254, 12866, 12458, 12031, 11585, 11121, 10640, 10143,
         9630,  9102,  8560,  8005,  7438,  6859,  6269,  5670,
         5062,  4447,  3824,  3196,  2563,  1925,  1285,   643,
            0,  -643, -1285, -1925, -2563, -3196, -3824, -4447,
        -5062, -5670, -6269, -6859, -7438, -8005, -8560, -9102,
        -9630, -10143, -10640, -11121, -11585, -12031, -12458, -12866,
        -13254, -13622, -13969, -14294, -14598, -14879, -15136, -15371,
        -15582, -15768, -15931, -16069, -16182, -16270, -16333, -16371,
        -16384, -16371, -16333, -16270, -16182, -16069, -15931, -15768,
        -15582, -15371, -15136, -14879, -14598, -14294, -13969, -13622,
        -13254, -12866, -12458, -12031, -11585, -11121, -10640, -10143,
        -9630, -9102, -8560, -8005, -7438, -6859, -6269, -5670,
        -5062, -4447, -3824, -3196, -2563, -1925, -1285,  -643,

};
/* 2000.h: Generated from frequency 2000
   by gentone.  8 samples  */
static ROM short test_2000[] = {8,
            0, 11585, 16384, 11585,     0, -11585, -16384, -11585,

};
/* 3200.h: Generated from frequency 3200
   by gentone.  10 samples  */
static ROM short test_3200[] = {10,
            0, 15582,  9630, -9630, -15582,     0, 15582,  9630,
        -9630, -15582,
};
/* 320.h: Generated from frequency 320
   by gentone.  50 samples  */
static ROM short test_320[] = {50,
            0,  2053,  4074,  6031,  7893,  9630, 11215, 12624,
        13833, 14824, 15582, 16093, 16351, 16351, 16093, 15582,
        14824, 13833, 12624, 11215,  9630,  7893,  6031,  4074,
         2053,     0, -2053, -4074, -6031, -7893, -9630, -11215,
        -12624, -13833, -14824, -15582, -16093, -16351, -16351, -16093,
        -15582, -14824, -13833, -12624, -11215, -9630, -7893, -6031,
        -4074, -2053,
};
/* 500.h: Generated from frequency 500
   by gentone.  32 samples  */
static ROM short test_500[] = {32,
            0,  3196,  6269,  9102, 11585, 13622, 15136, 16069,
        16384, 16069, 15136, 13622, 11585,  9102,  6269,  3196,
            0, -3196, -6269, -9102, -11585, -13622, -15136, -16069,
        -16384, -16069, -15136, -13622, -11585, -9102, -6269, -3196,

};
/* 6000.h: Generated from frequency 6000
   by gentone.  8 samples  */
static ROM short test_6000[] = {8,
            0, 11585, -16384, 11585,     0, -11585, 16384, -11585,

};
/* 7200.h: Generated from frequency 7200
   by gentone.  20 samples  */
static ROM short test_7200[] = {20,
            0,  5062, -9630, 13254, -15582, 16384, -15582, 13254,
        -9630,  5062,     0, -5062,  9630, -13254, 15582, -16384,
        15582, -13254,  9630, -5062,
};

static long crc32_bufs(unsigned char *buf, unsigned char *buf1)
{
        long oldcrc32;

        oldcrc32 = 0xFFFFFFFF;
        while(buf && *buf)
        {
                oldcrc32 = crc_32_tab[(oldcrc32 ^ *buf++) & 0xff] ^ ((unsigned long)oldcrc32 >> 8);
        }
        while(buf1 && *buf1)
        {
                oldcrc32 = crc_32_tab[(oldcrc32 ^ *buf1++) & 0xff] ^ ((unsigned long)oldcrc32 >> 8);
        }
        return ~oldcrc32;

}

ROM BYTE ulawtable[] = {
1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,
9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,16,16,16,16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
17,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,18,18,18,19,19,19,19,19,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
19,19,19,19,19,19,19,19,19,19,19,19,19,19,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
21,21,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
22,22,22,22,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,24,24,24,24,24,24,24,24,24,24,
24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,
25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
25,25,25,25,25,25,25,25,25,25,26,26,26,26,26,26,
26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,
26,26,26,26,26,26,26,26,26,26,26,26,26,27,27,27,
27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,28,
28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
29,29,29,30,30,30,30,30,30,30,30,30,30,30,30,30,
30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
30,30,30,30,30,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,32,32,32,32,32,32,32,32,32,
32,32,32,32,32,32,32,32,33,33,33,33,33,33,33,33,
33,33,33,33,33,33,33,33,33,34,34,34,34,34,34,34,
34,34,34,34,34,34,34,34,34,34,35,35,35,35,35,35,
35,35,35,35,35,35,35,35,35,35,35,36,36,36,36,36,
36,36,36,36,36,36,36,36,36,36,36,36,36,37,37,37,
37,37,37,37,37,37,37,37,37,37,37,37,37,37,38,38,
38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,39,
39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,
40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,
40,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
41,41,42,42,42,42,42,42,42,42,42,42,42,42,42,42,
42,42,42,43,43,43,43,43,43,43,43,43,43,43,43,43,
43,43,43,43,44,44,44,44,44,44,44,44,44,44,44,44,
44,44,44,44,44,45,45,45,45,45,45,45,45,45,45,45,
45,45,45,45,45,45,46,46,46,46,46,46,46,46,46,46,
46,46,46,46,46,46,46,47,47,47,47,47,47,47,47,47,
47,47,47,47,47,47,47,47,48,48,48,48,48,48,48,48,
48,49,49,49,49,49,49,49,49,50,50,50,50,50,50,50,
50,50,51,51,51,51,51,51,51,51,52,52,52,52,52,52,
52,52,52,53,53,53,53,53,53,53,53,54,54,54,54,54,
54,54,54,54,55,55,55,55,55,55,55,55,55,56,56,56,
56,56,56,56,56,57,57,57,57,57,57,57,57,57,58,58,
58,58,58,58,58,58,59,59,59,59,59,59,59,59,59,60,
60,60,60,60,60,60,60,61,61,61,61,61,61,61,61,61,
62,62,62,62,62,62,62,62,63,63,63,63,63,63,63,63,
63,64,64,64,64,65,65,65,65,66,66,66,66,66,67,67,
67,67,68,68,68,68,69,69,69,69,70,70,70,70,70,71,
71,71,71,72,72,72,72,73,73,73,73,74,74,74,74,74,
75,75,75,75,76,76,76,76,77,77,77,77,77,78,78,78,
78,79,79,79,79,80,80,81,81,82,82,83,83,83,84,84,
85,85,86,86,87,87,88,88,89,89,90,90,90,91,91,92,
92,93,93,94,94,95,95,96,97,98,99,100,100,101,102,103,
104,105,106,107,108,109,110,111,112,114,116,118,119,121,123,125,
255,253,251,249,247,246,244,242,240,239,238,237,236,235,234,233,
232,231,230,229,228,228,227,226,225,224,223,223,222,222,221,221,
220,220,219,219,218,218,218,217,217,216,216,215,215,214,214,213,
213,212,212,211,211,211,210,210,209,209,208,208,207,207,207,207,
206,206,206,206,205,205,205,205,205,204,204,204,204,203,203,203,
203,202,202,202,202,202,201,201,201,201,200,200,200,200,199,199,
199,199,198,198,198,198,198,197,197,197,197,196,196,196,196,195,
195,195,195,194,194,194,194,194,193,193,193,193,192,192,192,192,
191,191,191,191,191,191,191,191,191,190,190,190,190,190,190,190,
190,189,189,189,189,189,189,189,189,189,188,188,188,188,188,188,
188,188,187,187,187,187,187,187,187,187,187,186,186,186,186,186,
186,186,186,185,185,185,185,185,185,185,185,185,184,184,184,184,
184,184,184,184,183,183,183,183,183,183,183,183,183,182,182,182,
182,182,182,182,182,182,181,181,181,181,181,181,181,181,180,180,
180,180,180,180,180,180,180,179,179,179,179,179,179,179,179,178,
178,178,178,178,178,178,178,178,177,177,177,177,177,177,177,177,
176,176,176,176,176,176,176,176,176,175,175,175,175,175,175,175,
175,175,175,175,175,175,175,175,175,175,174,174,174,174,174,174,
174,174,174,174,174,174,174,174,174,174,174,173,173,173,173,173,
173,173,173,173,173,173,173,173,173,173,173,173,172,172,172,172,
172,172,172,172,172,172,172,172,172,172,172,172,172,171,171,171,
171,171,171,171,171,171,171,171,171,171,171,171,171,171,170,170,
170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,169,
169,169,169,169,169,169,169,169,169,169,169,169,169,169,169,169,
168,168,168,168,168,168,168,168,168,168,168,168,168,168,168,168,
168,167,167,167,167,167,167,167,167,167,167,167,167,167,167,167,
167,167,166,166,166,166,166,166,166,166,166,166,166,166,166,166,
166,166,166,165,165,165,165,165,165,165,165,165,165,165,165,165,
165,165,165,165,164,164,164,164,164,164,164,164,164,164,164,164,
164,164,164,164,164,164,163,163,163,163,163,163,163,163,163,163,
163,163,163,163,163,163,163,162,162,162,162,162,162,162,162,162,
162,162,162,162,162,162,162,162,161,161,161,161,161,161,161,161,
161,161,161,161,161,161,161,161,161,160,160,160,160,160,160,160,
160,160,160,160,160,160,160,160,160,160,159,159,159,159,159,159,
159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,159,
159,159,159,159,159,159,159,159,159,159,159,159,158,158,158,158,
158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,158,
158,158,158,158,158,158,158,158,158,158,158,158,158,158,157,157,
157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,
157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,157,
156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,
156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,156,
156,156,155,155,155,155,155,155,155,155,155,155,155,155,155,155,
155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,
155,155,155,155,154,154,154,154,154,154,154,154,154,154,154,154,
154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,154,
154,154,154,154,154,154,154,153,153,153,153,153,153,153,153,153,
153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
153,153,153,153,153,153,153,153,153,152,152,152,152,152,152,152,
152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,152,
152,152,152,152,152,152,152,152,152,152,152,151,151,151,151,151,
151,151,151,151,151,151,151,151,151,151,151,151,151,151,151,151,
151,151,151,151,151,151,151,151,151,151,151,151,151,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,
149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,
149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,149,
149,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
148,148,148,147,147,147,147,147,147,147,147,147,147,147,147,147,
147,147,147,147,147,147,147,147,147,147,147,147,147,147,147,147,
147,147,147,147,147,147,146,146,146,146,146,146,146,146,146,146,
146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,
146,146,146,146,146,146,146,146,145,145,145,145,145,145,145,145,
145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,
145,145,145,145,145,145,145,145,145,145,144,144,144,144,144,144,
144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,144,
144,144,144,144,144,144,144,144,144,144,144,144,143,143,143,143,
143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,143,
142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,
142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,
142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,
142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,142,
142,142,142,142,141,141,141,141,141,141,141,141,141,141,141,141,
141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,141,
141,141,141,141,141,141,141,141,141,140,140,140,140,140,140,140,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,140,
140,140,140,140,140,140,140,140,140,140,140,140,140,139,139,139,
139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,139,
139,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,138,
138,138,138,138,138,138,137,137,137,137,137,137,137,137,137,137,
137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,137,
137,137,137,137,137,137,137,137,137,137,136,136,136,136,136,136,
136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,
136,136,136,136,136,136,136,136,136,136,136,136,136,136,135,135,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,135,
135,135,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,134,
134,134,134,134,134,134,134,133,133,133,133,133,133,133,133,133,
133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,
133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,
133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,133,
133,133,133,133,133,133,133,133,133,133,133,132,132,132,132,132,
132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,
132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,131,
131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,
131,131,131,130,130,130,130,130,130,130,130,130,130,130,130,130,
130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,
130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,
130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,130,
130,130,130,130,130,130,130,130,129,129,129,129,129,129,129,129
};

ROM short ulawtabletx[] = {
-32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
-23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
-15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
-11900,-11388,-10876,-10364,-9852,-9340,-8828,-8316,
-7932,-7676,-7420,-7164,-6908,-6652,-6396,-6140,
-5884,-5628,-5372,-5116,-4860,-4604,-4348,-4092,
-3900,-3772,-3644,-3516,-3388,-3260,-3132,-3004,
-2876,-2748,-2620,-2492,-2364,-2236,-2108,-1980,
-1884,-1820,-1756,-1692,-1628,-1564,-1500,-1436,
-1372,-1308,-1244,-1180,-1116,-1052,-988,-924,
-876,-844,-812,-780,-748,-716,-684,-652,
-620,-588,-556,-524,-492,-460,-428,-396,
-372,-356,-340,-324,-308,-292,-276,-260,
-244,-228,-212,-196,-180,-164,-148,-132,
-120,-112,-104,-96,-88,-80,-72,-64,
-56,-48,-40,-32,-24,-16,-8,0,
32124,31100,30076,29052,28028,27004,25980,24956,
23932,22908,21884,20860,19836,18812,17788,16764,
15996,15484,14972,14460,13948,13436,12924,12412,
11900,11388,10876,10364,9852,9340,8828,8316,
7932,7676,7420,7164,6908,6652,6396,6140,
5884,5628,5372,5116,4860,4604,4348,4092,
3900,3772,3644,3516,3388,3260,3132,3004,
2876,2748,2620,2492,2364,2236,2108,1980,
1884,1820,1756,1692,1628,1564,1500,1436,
1372,1308,1244,1180,1116,1052,988,924,
876,844,812,780,748,716,684,652,
620,588,556,524,492,460,428,396,
372,356,340,324,308,292,276,260,
244,228,212,196,180,164,148,132,
120,112,104,96,88,80,72,64,
56,48,40,32,24,16,8,0
};

/* Intel ADPCM step variation table */
ROM static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

ROM static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static ROM char rxvoicestr[] = " \rRX VOICE DISPLAY:\n                                  v -- 3KHz        v -- 5KHz\n",
	invalselection[] = "Invalid Selection\n", paktc[] = "\nPress The Any Key (Enter) To Continue\n",
	booting[] = "System Re-Booting...\n";

char dummy_loc;
BYTE IOExpOutA,IOExpOutB;

void __attribute__((interrupt, auto_psv)) _CNInterrupt(void)
{

//Stuff for ADPCM encode
int val;			/* Current input sample value */
int sign;			/* Current adpcm sign bit */
BYTE delta;			/* Current adpcm output value */
int diff;			/* Difference between val and valprev */
int step;			/* Stepsize */
int vpdiff;			/* Current change to valpred */
long valpred;		/* Predicted output value */
int adpcm_index;
BYTE *cp;

 	// If PPS signal is asserted
	if (((PORTAbits.RA4) && (AppConfig.PPSPolarity == 0)) ||
		((!PORTAbits.RA4) && (AppConfig.PPSPolarity == 1)))
	{
		if (USE_PPS && (!indiag))
		{
			ppstimer = 0;
			ppswarn = 0;
			if ((gps_state == GPS_STATE_VALID) && (!gotpps))
			{
				TMR3 = 0;
				gotpps = 1;
				lockcnt = 0;
				samplecnt = 0;
				fillindex = 0;
			}
			else if (gotpps) 
			{
				if (ppscount >= 3)
				{
					if ((samplecnt >= 7999) && (samplecnt <= 8001))
					{
						last_samplecnt = samplecnt;
						sendgps = 1;
						real_time++;
						if (samplecnt < 8000)  // If we are short one, insert another
						{
							if (option_flags & OPTION_FLAG_ADPCM)
							{
								val = last_adcsample;
								val -= 2048;
								val *= 16;
								adpcm_index = enc_index;
								valpred = enc_valprev;
							    step = stepsizeTable[adpcm_index];
								
								/* Step 1 - compute difference with previous value */
								diff = val - valpred;
								sign = (diff < 0) ? 8 : 0;
								if ( sign ) diff = (-diff);
							
								/* Step 2 - Divide and clamp */
								/* Note:
								** This code *approximately* computes:
								**    delta = diff*4/step;
								**    vpdiff = (delta+0.5)*step/4;
								** but in shift step bits are dropped. The net result of this is
								** that even if you have fast mul/div hardware you cannot put it to
								** good use since the fixup would be too expensive.
								*/
								delta = 0;
								vpdiff = (step >> 3);
								
								if ( diff >= step ) {
								    delta = 4;
								    diff -= step;
								    vpdiff += step;
								}
								step >>= 1;
								if ( diff >= step  ) {
								    delta |= 2;
								    diff -= step;
								    vpdiff += step;
								}
								step >>= 1;
								if ( diff >= step ) {
								    delta |= 1;
								    vpdiff += step;
								}
							
								/* Step 3 - Update previous value */
								if ( sign )
								  valpred -= vpdiff;
								else
								  valpred += vpdiff;
							
								/* Step 4 - Clamp previous value to 16 bits */
								if ( valpred > 32767 )
								  valpred = 32767;
								else if ( valpred < -32768 )
								  valpred = -32768;
							
								/* Step 5 - Assemble value, update index and step values */
								delta |= sign;
								
								adpcm_index += indexTable[delta];
								if ( adpcm_index < 0 ) adpcm_index = 0;
								if ( adpcm_index > 88 ) adpcm_index = 88;
								enc_valprev = valpred;
								enc_index = adpcm_index;
								if (fillindex & 1)
								{
									audio_buf[filling_buffer][fillindex++ >> 1]	= (enc_lastdelta << 4) | delta;
								}
								else
								{
									enc_lastdelta = delta;
								}
							}
							else  // is ULAW
							{
								audio_buf[filling_buffer][fillindex++] = ulawtable[last_adcsample];
							}
							if (fillindex >= ((option_flags & OPTION_FLAG_ADPCM) ? FRAME_SIZE * 2 : FRAME_SIZE))
							{
								if (option_flags & OPTION_FLAG_ADPCM)
								{
									cp = &audio_buf[filling_buffer][fillindex >> 1];
									*cp++ = (enc_prev_valprev & 0xff00) >> 8;
									*cp++ = enc_prev_valprev & 0xff;
									*cp = enc_index;
									enc_prev_valprev = enc_valprev;
									enc_prev_index = enc_index;
								}
								filled = 1;
								fillindex = 0;
								filling_buffer ^= 1;
								last_drainindex = txdrainindex;
								timing_index = next_index;
								timing_time = next_time;
							}
						}
						if ((!gpssync) && (gps_state == GPS_STATE_VALID))
						{
							system_time.vtime_sec = timing_time = real_time = gps_time + 1; 
							gpssync = 1;
						}
					}
					else
					{
						gpssync = 0;
						ppscount = 0;
					}
			    } else ppscount++;
			}
			samplecnt = 0;
		}
	}
	IFS1bits.CNIF = 0;
}

void __attribute__((interrupt, auto_psv)) _ADC1Interrupt(void)
{
WORD index;
long accum;
short saccum;
BYTE i;

//Stuff for ADPCM encode
int val;			/* Current input sample value */
int sign;			/* Current adpcm sign bit */
BYTE delta;			/* Current adpcm output value */
int diff;			/* Difference between val and valprev */
int step;			/* Stepsize */
int vpdiff;			/* Current change to valpred */
long valpred;		/* Predicted output value */
int adpcm_index;
BYTE *cp;

	index = ADC1BUF0;
	if (adcother)
	{
		adcothers[adcindex++] = index >> 2;
		if(adcindex >= ADCOTHERS) adcindex = 0;
		AD1CHS0 = 0;
		if (gotpps) ppstimer++;
		if (gps_state != GPS_STATE_IDLE) gpstimer++;
		if (connected) 
		{
			gpsforcetimer++;
			elketimer++;
		}
		if (!connected) attempttimer++;
		else lastrxtimer++;
		termbuftimer++;
	}
	else
	{
		if (gotpps || (!USE_PPS))
		{
			if (fillindex == 0)
			{
				next_index = samplecnt;
				next_time = real_time;
			}
			last_adcsample = index;
			// Make 16 bit number from 12 bit ADC sample
			saccum = index;
			saccum -= 2048;
			accum = saccum * 16;
            if (accum > amax)
            {
                amax = accum;
                discounteru = discfactor;
            }
            else if (--discounteru <= 0)
            {
                discounteru = discfactor;
                amax = (long)((amax * 32700) / 32768L);
            }
            if (accum < amin)
            {
                amin = accum;
                discounterl = discfactor;
            }
            else if (--discounterl <= 0)
            {
                discounterl = discfactor;
                amin = (long)((amin * 32700) / 32768L);
            }
			if ((!USE_PPS) && (samplecnt == 8000)) samplecnt = 0;
			if (samplecnt++ < 8000)
			{
				if (option_flags & OPTION_FLAG_ADPCM)
				{
					val = index;
					val -= 2048;
					val *= 16;

					adpcm_index = enc_index;
					valpred = enc_valprev;
				    step = stepsizeTable[adpcm_index];
					
					/* Step 1 - compute difference with previous value */
					diff = val - valpred;
					sign = (diff < 0) ? 8 : 0;
					if ( sign ) diff = (-diff);
				
					/* Step 2 - Divide and clamp */
					/* Note:
					** This code *approximately* computes:
					**    delta = diff*4/step;
					**    vpdiff = (delta+0.5)*step/4;
					** but in shift step bits are dropped. The net result of this is
					** that even if you have fast mul/div hardware you cannot put it to
					** good use since the fixup would be too expensive.
					*/
					delta = 0;
					vpdiff = (step >> 3);
					
					if ( diff >= step ) {
					    delta = 4;
					    diff -= step;
					    vpdiff += step;
					}
					step >>= 1;
					if ( diff >= step  ) {
					    delta |= 2;
					    diff -= step;
					    vpdiff += step;
					}
					step >>= 1;
					if ( diff >= step ) {
					    delta |= 1;
					    vpdiff += step;
					}
				
					/* Step 3 - Update previous value */
					if ( sign )
					  valpred -= vpdiff;
					else
					  valpred += vpdiff;
				
					/* Step 4 - Clamp previous value to 16 bits */
					if ( valpred > 32767 )
					  valpred = 32767;
					else if ( valpred < -32768 )
					  valpred = -32768;
				
					/* Step 5 - Assemble value, update index and step values */
					delta |= sign;
					
					adpcm_index += indexTable[delta];
					if ( adpcm_index < 0 ) adpcm_index = 0;
					if ( adpcm_index > 88 ) adpcm_index = 88;
					enc_valprev = valpred;
					enc_index = adpcm_index;
					if (fillindex & 1)
					{
						audio_buf[filling_buffer][fillindex >> 1]	= (enc_lastdelta << 4) | delta;
					}
					else
					{
						enc_lastdelta = delta;
					}
					fillindex++;
				}
				else  // is ULAW
				{
					audio_buf[filling_buffer][fillindex++] = ulawtable[index];
				}
				if (txseqno == 0) txseqno = 3;
				if (fillindex >= ((option_flags & OPTION_FLAG_ADPCM) ? FRAME_SIZE * 2 : FRAME_SIZE))
				{
					if (option_flags & OPTION_FLAG_ADPCM)
					{
						cp = &audio_buf[filling_buffer][fillindex >> 1];
						*cp++ = (enc_prev_valprev & 0xff00) >> 8;
						*cp++ = enc_prev_valprev & 0xff;
						*cp = enc_prev_index;
						enc_prev_valprev = enc_valprev;
						enc_prev_index = enc_index;
						txseqno++;
						if (host_txseqno) host_txseqno++;
					}
					txseqno++;
					if (host_txseqno) host_txseqno++;
					filled = 1;
					fillindex = 0;
					filling_buffer ^= 1;
					last_drainindex = txdrainindex;
					timing_index = next_index;
					timing_time = next_time;
					apeak = (long)(amax - amin) / 2;
					for(i = NAPEAKS -1; i; i--) apeaks[i] = apeaks[i - 1];
					apeaks[0] = apeak;
				}
			}
		}
#if defined(SMT_BOARD)
		AD1CHS0 = adcindex + 1;
#else
		AD1CHS0 = adcindex + 2;
#endif
		sqlcount++;
	}
	adcother ^= 1;
	IFS0bits.AD1IF = 0;
}

void __attribute__((interrupt, no_auto_psv)) _DAC1LInterrupt(void)
{
	IFS4bits.DAC1LIF = 0;
	// Output Tx sample
	if (testp)
	{
		DAC1LDAT = testp[testidx++ + 1];
		if (testidx >= testp[0]) testidx = 0;
	}
	else
	{
#ifdef	DMWDIAG
		DAC1LDAT = ulawtabletx[ulaw_digital_milliwatt[mwp++]];
		if (mwp > 7) mwp = 0;
#else
		DAC1LDAT = ulawtabletx[txaudio[txdrainindex]];
#endif
		txaudio[txdrainindex++] = ULAW_SILENCE;
		if (txdrainindex >= AppConfig.TxBufferLength)
		txdrainindex = 0;
	}
}


void __attribute__((interrupt, auto_psv)) _DefaultInterrupt(void)
{
   Nop();
   Nop();
}

void __attribute__((interrupt, auto_psv)) _OscillatorFail(void)
{
   Nop();
   Nop();
}
void _ISR __attribute__((__no_auto_psv__)) _AddressError(void)
{
   Nop();
   Nop();
}
void _ISR __attribute__((__no_auto_psv__)) _StackError(void)
{
   Nop();
   Nop();
}
void __attribute__((interrupt, auto_psv)) _MathError(void)
{
   Nop();
   Nop();
}

#if defined(SMT_BOARD)

ROM WORD ledmask[] = {0x1000,0x800,0x400,0x2000};

	void SetLED(BYTE led,BOOL val)
	{
		LATB &= ~ledmask[led];
		if (!val) LATB |= ledmask[led];
	}
	
	void ToggleLED(BYTE led)
	{
		LATB ^= ledmask[led];
	}
	
	void SetPTT(BOOL val)
	{
		_LATB4 = val;	
	}
	
	void SetAudioSrc(void)
	{
		BYTE myflags;

		if (indiag) myflags = diag_option_flags;
		else myflags = option_flags;
		if (myflags & 1)_LATB3 = 1;
		else _LATB3 = 0;
		if (!(myflags & 4)) _LATB2 = 1;
		else _LATB2 = 0;
	}

#else

	#define PROPER_SPICON1  (0x0003 | 0x0120)   /* 1:1 primary prescale, 8:1 secondary prescale, CKE=1, MASTER mode */
	
	#define ClearSPIDoneFlag()
	static inline __attribute__((__always_inline__)) void WaitForDataByte( void )
	{
	    while ((IOEXP_SPISTATbits.SPITBF == 1) || (IOEXP_SPISTATbits.SPIRBF == 0));
	}
	
	#define SPI_ON_BIT          (IOEXP_SPISTATbits.SPIEN)
	
	void IOExp_Write(BYTE reg,BYTE val)
	{
	
	    volatile BYTE vDummy;
	    BYTE vSPIONSave;
	    WORD SPICON1Save;
	
	    // Save SPI state
	    SPICON1Save = IOEXP_SPICON1;
	    vSPIONSave = SPI_ON_BIT;
	
	    // Configure SPI
	    SPI_ON_BIT = 0;
	    IOEXP_SPICON1 = PROPER_SPICON1;
	    SPI_ON_BIT = 1;
	
	    SPISel(SPICS_IOEXP);
	    IOEXP_SSPBUF = 0x40;
	    WaitForDataByte();
	    vDummy = IOEXP_SSPBUF;
	    IOEXP_SSPBUF = reg;
	    WaitForDataByte();
	    vDummy = IOEXP_SSPBUF;
	
	    IOEXP_SSPBUF = val;
	    WaitForDataByte();
	    vDummy = IOEXP_SSPBUF;
	    SPISel(SPICS_IDLE);
	    // Restore SPI State
	    SPI_ON_BIT = 0;
	    IOEXP_SPICON1 = SPICON1Save;
	    SPI_ON_BIT = vSPIONSave;
	
		ClearSPIDoneFlag();
	}
	
	BYTE IOExp_Read(BYTE reg)
	{
	
	    volatile BYTE vDummy,retv;
	    BYTE vSPIONSave;
	    WORD SPICON1Save;
	
	    // Save SPI state
	    SPICON1Save = IOEXP_SPICON1;
	    vSPIONSave = SPI_ON_BIT;
	
	    // Configure SPI
	    SPI_ON_BIT = 0;
	    IOEXP_SPICON1 = PROPER_SPICON1;
	    SPI_ON_BIT = 1;
	
	    SPISel(SPICS_IOEXP);
	    IOEXP_SSPBUF = 0x41;
	    WaitForDataByte();
	    vDummy = IOEXP_SSPBUF;
	    IOEXP_SSPBUF = reg;
	    WaitForDataByte();
	    vDummy = IOEXP_SSPBUF;
		IOEXP_SSPBUF = 0;
	    WaitForDataByte();
	    retv = IOEXP_SSPBUF;
	
	    SPISel(SPICS_IDLE);
	    // Restore SPI State
	    SPI_ON_BIT = 0;
	    IOEXP_SPICON1 = SPICON1Save;
	    SPI_ON_BIT = vSPIONSave;
	
		ClearSPIDoneFlag();
		return retv;
	}
	
	void IOExpInit(void)
	{
		IOExp_Write(IOEXP_IOCON,0x20);
		IOExp_Write(IOEXP_IODIRA,0xD0);
		IOExp_Write(IOEXP_IODIRB,0xf3);
		IOExpOutA = 0xDF;
		IOExp_Write(IOEXP_OLATA,IOExpOutA);
		IOExpOutB = 0xF3;
		IOExp_Write(IOEXP_OLATB,IOExpOutB);
	}
	
	void SetLED(BYTE led,BOOL val)
	{
	BYTE mask,oldout;
	
		oldout = IOExpOutA;
		mask = 1 << led;
		IOExpOutA &= ~mask;
		if (!val) IOExpOutA |= mask;
		if (IOExpOutA != oldout) IOExp_Write(IOEXP_OLATA,IOExpOutA);
	}
	
	void ToggleLED(BYTE led)
	{
	BYTE mask,oldout;
	
		oldout = IOExpOutA;
		mask = 1 << led;
		IOExpOutA ^= mask;
		if (IOExpOutA != oldout) IOExp_Write(IOEXP_OLATA,IOExpOutA);
	}
	
	void SetPTT(BOOL val)
	{
	BYTE oldout;
	
		oldout = IOExpOutA;
		IOExpOutA &= ~0x20;
		if (val) IOExpOutA |= 0x20;
		if (IOExpOutA != oldout) IOExp_Write(IOEXP_OLATA,IOExpOutA);
	}
	
	void SetAudioSrc(void)
	{
	BYTE oldout;
	
	
		oldout = IOExpOutB;
		IOExpOutB &= ~0x0c;
		if (option_flags & 1) IOExpOutB |= 8;
		if (!(option_flags & 4)) IOExpOutB |= 4;
		if (IOExpOutB != oldout) IOExp_Write(IOEXP_OLATB,IOExpOutB);
	}

#endif

BOOL HasCOR(void)
{
	if (indiag) return(0);
	if (AppConfig.CORType == 2) return(0);
	if (AppConfig.CORType == 1) return(1);
	return (cor);
}

BOOL HasCTCSS(void)
{
	if (indiag) return(0);
	if (!AppConfig.ExternalCTCSS) return (1);
	if (AppConfig.ExternalCTCSS == 3) return (0);
	if ((AppConfig.ExternalCTCSS == 1) && CTCSSIN) return (1);
	if ((AppConfig.ExternalCTCSS == 2) && (!CTCSSIN)) return(1);
	return (0);
}

void SetTxTone(int freq)
{

	DISABLE_INTERRUPTS();
	if (freq == 0)
	{
		DAC1CONbits.DACFDIV = 74;		// Divide by 75 for 8K Samples/sec
		testp = 0;
		testidx = 0;
	}
	else
	{
		DAC1CONbits.DACFDIV = 36;		// Divide by 37 for approx 16216.216 Samples/sec
		switch(freq)
		{
		    case 100:
				testp = (short *)test_100;
				break;
		    case 320:
				testp = (short *)test_320;
				break;
		    case 500:
				testp = (short *)test_500;
				break;
		    case 1000:
				testp = (short *)test_1000;
				break;
		    case 2000:
				testp = (short *)test_2000;
				break;
		    case 3200:
				testp = (short *)test_3200;
				break;
		    case 6000:
				testp = (short *)test_6000;
				break;
		    case 7200:
				testp = (short *)test_7200;
				break;
			default:
				testp = 0;
				break;
		}
	}
	ENABLE_INTERRUPTS();
}

static char *myfgets(char *s, int size, FILE *stream)
{
	char *r;

	ininput = 1;
	r = fgets(s,size,stream);
	ininput = 0;
	return(r);
}

BYTE GetBootCS(void)
{
BYTE x = 0x69,i;

	for(i = 0; i < 4; i++) x += AppConfig.BootIPAddr.v[i];
	return(x);
}


/*
* Break up a delimited string into a table of substrings
*
* str - delimited string ( will be modified )
* strp- list of pointers to substrings (this is built by this function), NULL will be placed at end of list
* limit- maximum number of substrings to process
* delim- user specified delimeter
* quote- user specified quote for escaping a substring. Set to zero to escape nothing.
*
* Note: This modifies the string str, be suer to save an intact copy if you need it later.
*
* Returns number of substrings found.
*/

static int explode_string(char *str, char *strp[], int limit, char delim, char quote)
{
int     i,l,inquo;

        inquo = 0;
        i = 0;
        strp[i++] = str;
        if (!*str)
           {
                strp[0] = 0;
                return(0);
           }
        for(l = 0; *str && (l < limit) ; str++)
        {
                if(quote)
                {
                        if (*str == quote)
                        {
                                if (inquo)
                                {
                                        *str = 0;
                                        inquo = 0;
                                }
                                else
                                {
                                        strp[i - 1] = str + 1;
                                        inquo = 1;
                                }
                        }
                }
                if ((*str == delim) && (!inquo))
                {
                        *str = 0;
                        l++;
                        strp[i++] = str + 1;
                }
        }
        strp[i] = 0;
        return(i);

}

#define	memclr(x,y) memset(x,0,y)
#define ARPIsTxReady()      MACIsTxReady() 
WORD htons(WORD x)
{
	WORD y;
	BYTE *p = (BYTE *)&x;
	BYTE *q = (BYTE *)&y;

	q[0] = p[1];
	q[1] = p[0];
	return(y);
}

WORD ntohs(WORD x)
{
	WORD y;
	BYTE *p = (BYTE *)&x;
	BYTE *q = (BYTE *)&y;

	q[0] = p[1];
	q[1] = p[0];
	return(y);
}

DWORD htonl(DWORD x)
{
	DWORD y;
	BYTE *p = (BYTE *)&x;
	BYTE *q = (BYTE *)&y;

	q[0] = p[3];
	q[1] = p[2];
	q[2] = p[1];
	q[3] = p[0];
	return(y);
}

DWORD ntohl(DWORD x)
{
	DWORD y;
	BYTE *p = (BYTE *)&x;
	BYTE *q = (BYTE *)&y;

	q[0] = p[3];
	q[1] = p[2];
	q[2] = p[1];
	q[3] = p[0];
	return(y);
}

BOOL getGPSStr(void)
{
BYTE	c;

		if (!DataRdyUART2()) return 0;
		c = ReadUART2();
		if (c == '\n')
		{
			gps_buf[gps_bufindex]= 0;
			gps_bufindex = 0;
			return 1; 
		}
		if (c < ' ') return 0;
		gps_buf[gps_bufindex++] = c;
		if (gps_bufindex >= (sizeof(gps_buf) - 1))
		{
			gps_buf[gps_bufindex]= 0;
			gps_bufindex = 0;
			return 1; 
		}
		return 0;
}

BOOL getTSIPPacket(void)
{
BYTE     c;

		if (!DataRdyUART2()) return 0;
		c = ReadUART2();
        if (gps_bufindex == 0)
        {
                TSIPwasdle = 0;
                if (c == 16)
                {
                        TSIPwasdle = 1;
                        gps_bufindex++;
                }
                return 0;
        }
        if (gps_bufindex > sizeof(gps_buf))
        {
                gps_bufindex = 0;
                return 0;
        }
        if ((c == 16) && (!TSIPwasdle))
        {
                TSIPwasdle = 1;
                return 0;
        }
        else if (TSIPwasdle && (c == 3))
        {
                gps_bufindex = 0;
                return 1;
        }
        TSIPwasdle = 0;
        gps_buf[gps_bufindex - 1] = c;
		gps_bufindex++;
        return 0;
}

static DWORD twoascii(char *s)
{
DWORD rv;

	rv = s[1] - '0';
	rv += 10 * (s[0] - '0');
	return(rv);
}

static char *logtime(void)
{
time_t	t;
static char str[50];
static ROM char notime[] = "<System Time Not Set>",
	logtemplate[] = "%m/%d/%Y %H:%M:%S";

	t = system_time.vtime_sec;
	if (t == 0) return((char *)notime);
	strftime(str,sizeof(str) - 1,(char *)logtemplate,gmtime(&t));
	sprintf(str + strlen(str),".%03lu",system_time.vtime_nsec / 1000000L);
	return(str);
}

void process_gps(void)
{
int n;
char *strs[30];
static ROM char gpgga[] = "$GPGGA",
	gpgsv[] = "$GPGSV", gprmc[] = "$GPRMC";

// Please see doubleify.c for explanation of this poo-poo
extern float doubleify(BYTE *p);
	
	if (indiag) return;
	if (gps_state == GPS_STATE_IDLE) gps_time = 0;
	if ((gpssync || (!USE_PPS)) && (gps_state == GPS_STATE_VALID))
	{
		gps_state = GPS_STATE_SYNCED;
		printf(logtime());
		printf(gpsmsg3);
	}
	if ((!gpssync) && USE_PPS && (gps_state == GPS_STATE_SYNCED))
	{
		gps_state = GPS_STATE_VALID;
		printf(logtime());
		printf(gpsmsg5);
		if (USE_PPS)
		{
			connected = 0;
			txseqno = 0;
			txseqno_ptt = 0;
			resp_digest = 0;
			digest = 0;
			their_challenge[0] = 0;
			lastrxtimer = 0;
		}
	}
	if (AppConfig.GPSProto == GPS_NMEA)
	{
		if (!getGPSStr()) return;
		n = explode_string((char *)gps_buf,strs,30,',','\"');
		if (n < 1) return;
		if (!strcmp(strs[0],gpgsv))
		{
			if (n >= 4) gps_nsat = atoi(strs[3]);
			return;
		}
		if (!strcmp(strs[0],gprmc))
		{
			struct tm tm;
	
			if (n < 10) return;
			memset(&tm,0,sizeof(tm));
			tm.tm_sec = twoascii(strs[1] + 4);
			tm.tm_min = twoascii(strs[1] + 2);
			tm.tm_hour = twoascii(strs[1]);
			tm.tm_mday = twoascii(strs[9]);
			tm.tm_mon = twoascii(strs[9] + 2);
			tm.tm_year = twoascii(strs[9] + 4) + 100;
			gps_time = (DWORD) mktime(&tm);
			if (!USE_PPS) system_time.vtime_sec = timing_time = real_time = gps_time + 1;
			return;
		}
	
		if (n < 7) return;
		if (strcmp(strs[0],gpgga)) return;
		gpswarn = 0;
		gpstimer = 0;
		if (gps_state == GPS_STATE_IDLE)
		{
			gps_state = GPS_STATE_RECEIVED;
			gpstimer = 0;
			gpswarn = 0;
			printf(gpsmsg1);
		}
		n = atoi(strs[6]);
		if ((n < 1) || (n > 2)) 
		{
			if (gps_state == GPS_STATE_RECEIVED) return;
			gps_state = GPS_STATE_IDLE;
			printf(logtime());
			printf(gpsmsg6);
			if (USE_PPS)
			{
				connected = 0;
				txseqno = 0;
				txseqno_ptt = 0;
				resp_digest = 0;
				digest = 0;
				their_challenge[0] = 0;
				gpssync = 0;
				gotpps = 0;
				lastrxtimer = 0;
			}
		}
		if ((gps_state == GPS_STATE_RECEIVED) && (gps_nsat > 0) && gps_time)
		{
			gps_state = GPS_STATE_VALID;
	
			printf(gpsmsg2);
			printf("%d\n",gps_nsat);
		}
		memclr(&gps_packet,sizeof(gps_packet));
		strncpy(gps_packet.lat,strs[2],7);
		gps_packet.lat[7] = *strs[3];
		strncpy(gps_packet.lon,strs[4],8);
		gps_packet.lon[8] = *strs[5];
		strncpy(gps_packet.elev,strs[9],6);
	}
	else /* is a Trimble Thunderbolt */
	{
		if (!getTSIPPacket()) return;
		if (gps_buf[0] != 0x8f) return;
		if (gps_buf[1] == 0xab) 
		{
			struct tm tm;
			WORD w;
	
			memset(&tm,0,sizeof(tm));
			tm.tm_sec = gps_buf[11];
			tm.tm_min = gps_buf[12];
			tm.tm_hour = gps_buf[13];
			tm.tm_mday = gps_buf[14];
			tm.tm_mon = gps_buf[15];
			w = gps_buf[17] | ((WORD)gps_buf[16] << 8);
			tm.tm_year = w - 1900;
			gps_time = (DWORD) mktime(&tm);
			if (!USE_PPS) system_time.vtime_sec = timing_time = real_time = gps_time + 1;
			return;
		}
		if (gps_buf[1] == 0xac)
		{
			BOOL happy;
			int x,y;
			float f;

			happy = 1;
			if (gps_buf[13] || gps_buf[14]) happy = 0;
			if (gps_buf[9] || gps_buf[10]) happy = 0;
			if ((gps_buf[12] & 0x1f) | gps_buf[11]) happy = 0;
			gpswarn = 0;
			gpstimer = 0;
			if (gps_state == GPS_STATE_IDLE)
			{
				gps_state = GPS_STATE_RECEIVED;
				gpstimer = 0;
				gpswarn = 0;
				printf(gpsmsg1);
			}
			if (!happy)
			{
				if (gps_state == GPS_STATE_RECEIVED) return;
				gps_state = GPS_STATE_IDLE;
				printf(logtime());
				printf(gpsmsg6);
				if (USE_PPS)
				{
					connected = 0;
					txseqno = 0;
					txseqno_ptt = 0;
					resp_digest = 0;
					digest = 0;
					their_challenge[0] = 0;
					lastrxtimer = 0;
				}
				gpssync = 0;
				gotpps = 0;
			}
			gps_nsat = 3;
			if ((gps_state == GPS_STATE_RECEIVED) && (gps_nsat > 0) && gps_time)
			{
				gps_state = GPS_STATE_VALID;
		
				printf(gpsmsg9);

			}
			memclr(&gps_packet,sizeof(gps_packet));
			f = doubleify(gps_buf + 37) * TSIP_FACTOR;
			x = (int) f;
			f -= (float) x;
			if (f < 0.0) f = -f;
			f *= 60.0;
			y = (int) f;
			f -= (float) y;
			if (f < 0.0) f = -f;
			if (x < 0)
				sprintf(gps_packet.lat,"%02d%02d.%02dS",-x,y,(int)((f * 100.0) + 0.5));
			else
				sprintf(gps_packet.lat,"%02d%02d.%02dN",x,y,(int)((f * 100.0) + 0.5));
			f = doubleify(gps_buf + 45) * TSIP_FACTOR;
			x = (int) f;
			f -= (float) x;
			if (f < 0.0) f = -f;
			f *= 60.0;
			y = (int) f;
			f -= (float) y;
			if (f < 0.0) f = -f;
			if (x < 0)
				sprintf(gps_packet.lon,"%03d%02d.%02dW",-x,y,(int)((f * 100.0) + 0.5));
			else
				sprintf(gps_packet.lon,"%03d%02d.%02dE",x,y,(int)((f * 100.0) + 0.5));
			sprintf(gps_packet.elev,"%4.1f",(double)doubleify(gps_buf + 53));
			return;
		}
	}
	return;
}

void adpcm_decoder(BYTE *indata)
{
    BYTE *inp;		/* Input buffer pointer */
    int sign;			/* Current adpcm sign bit */
    BYTE delta;			/* Current adpcm output value */
    int step;			/* Stepsize */
    long valpred;		/* Predicted value */
    int vpdiff;			/* Current change to valpred */
    int index;			/* Current step change index */
    BYTE inputbuffer;		/* place to keep next 4-bit value */
    BOOL bufferstep;		/* toggle between inputbuffer/input */
	WORD w;
	WORD i;
	long vout;

    inp = indata;

    valpred = dec_valprev;
    index = dec_index;
    step = stepsizeTable[index];

    bufferstep = 0;
    inputbuffer = 0;

   
    for ( i = 0; i < FRAME_SIZE * 2; i++) {
	
		/* Step 1 - get the delta value */
		if ( bufferstep ) {
		    delta = inputbuffer & 0xf;
		} else {
		    inputbuffer = *inp++;
		    delta = (inputbuffer >> 4) & 0xf;
		}
		bufferstep = !bufferstep;
	
		/* Step 2 - Find new index value (for later) */
		index += indexTable[delta];
		if ( index < 0 ) index = 0;
		if ( index > 88 ) index = 88;
	
		/* Step 3 - Separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;
	
		/* Step 4 - Compute difference and new predicted value */
		/*
		** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
		** in adpcm_coder.
		*/
		vpdiff = step >> 3;
		if ( delta & 4 ) vpdiff += step;
		if ( delta & 2 ) vpdiff += step >> 1;
		if ( delta & 1 ) vpdiff += step >> 2;
	
		if ( sign )
		  valpred -= vpdiff;
		else
		  valpred += vpdiff;
	
		/* Step 5 - clamp output value */
		if ( valpred > 32767 )
		  valpred = 32767;
		else if ( valpred < -32768 )
		  valpred = -32768;
	
		/* Step 6 - Update step value */
		step = stepsizeTable[index];
	
		/* Step 7 - Output value */
		vout = valpred + 32768;
		w = (vout  & 0xffff) >> 4;
		dec_buffer[i] = ulawtable[w];
    }

    dec_valprev = valpred;
    dec_index = index;
}


void process_udp(UDP_SOCKET *udpSocketUser,NODE_INFO *udpServerNode)
{

	ROM char badmix[] = "ERROR! Host not acknowledging non-GPS disciplined operation\n";

	BYTE n,c,i,j,*cp;

	WORD mytxindex;
	VTIME mysystem_time;
	long mytxseqno,myhost_txseqno;

	mytxindex = last_drainindex;
	mysystem_time = system_time;
	mytxseqno = txseqno;
	myhost_txseqno = host_txseqno;

	if (indiag) return;
	if (filled && (gpssync || (!USE_PPS)) && (!time_filled))
	{
		
		system_time.vtime_sec = timing_time;
		system_time.vtime_nsec = timing_index * 125000;
		time_filled = 1;
	}

	if (filled && ((fillindex > MASTER_TIMING_DELAY) || (option_flags & OPTION_FLAG_MASTERTIMING)))
	{
		if (gpssync || (!USE_PPS))
		{
//TESTBIT ^= 1;
			BOOL tosend = (connected && ((HasCOR() && HasCTCSS()) || (option_flags & OPTION_FLAG_SENDALWAYS)));
			if ((((!connected) && (attempttimer >= ATTEMPT_TIME)) || tosend) && UDPIsPutReady(*udpSocketUser)) {
				UDPSocketInfo[activeUDPSocket].remoteNode.MACAddr = udpServerNode->MACAddr;
				memclr(&audio_packet,sizeof(VOTER_PACKET_HEADER));
				audio_packet.vph.curtime.vtime_sec = htonl(system_time.vtime_sec);
				audio_packet.vph.curtime.vtime_nsec = 
					(!USE_PPS) ? htonl(mytxseqno) : htonl(system_time.vtime_nsec);
				strcpy((char *)audio_packet.vph.challenge,challenge);
				audio_packet.vph.digest = htonl(resp_digest);
				if (tosend) audio_packet.vph.payload_type = htons((option_flags & OPTION_FLAG_ADPCM) ? 3 : 1);
				i = 0;
				cp = (BYTE *) &audio_packet;
				for(i = 0; i < sizeof(VOTER_PACKET_HEADER); i++) UDPPut(*cp++);
				j = (option_flags & OPTION_FLAG_ADPCM) ? FRAME_SIZE + 3 : FRAME_SIZE;
				c = (option_flags & OPTION_FLAG_ADPCM) ? ADPCM_SILENCE : ULAW_SILENCE;
	            if (tosend)
				{
					if (AppConfig.CORType == 1) rssi = 255;
					if ((rssi > 0) && HasCOR() && HasCTCSS())
					{
						UDPPut(rssi);
						for(i = 0; i < j; i++) UDPPut(audio_buf[filling_buffer ^ 1][i]);
						elketimer = 0;
					}
					else
					{
						UDPPut(0);
						for(i = 0; i < j; i++) UDPPut(c);
					}
				}
				else if (!USE_PPS) UDPPut(OPTION_FLAG_MIX);
	             //Send contents of transmit buffer, and free buffer
	            UDPFlush();
				attempttimer = 0;
			}
		}
		filled = 0;
		time_filled = 0;
	}
	if (connected && (sendgps || (!USE_PPS)) && (gps_packet.lat[0] || (gpsforcetimer >= GPS_FORCE_TIME)))
	{
	        if (UDPIsPutReady(*udpSocketUser)) {
				UDPSocketInfo[activeUDPSocket].remoteNode.MACAddr = udpServerNode->MACAddr;
				gps_packet.vph.curtime.vtime_sec = htonl(real_time);
				gps_packet.vph.curtime.vtime_nsec = htonl(0);
				strcpy((char *)gps_packet.vph.challenge,challenge);
				gps_packet.vph.digest = htonl(resp_digest);
				gps_packet.vph.payload_type = htons(2);
				// Send elements one at a time -- SWINE dsPIC33 archetecture!!!
				cp = (BYTE *) &gps_packet.vph;
				for(i = 0; i < sizeof(gps_packet.vph); i++) UDPPut(*cp++);
				if (gps_packet.lat[0])
				{
					cp = (BYTE *) gps_packet.lat;
					for(i = 0; i < sizeof(gps_packet.lat); i++) UDPPut(*cp++);
					cp = (BYTE *) gps_packet.lon;
					for(i = 0; i < sizeof(gps_packet.lon); i++) UDPPut(*cp++);
					cp = (BYTE *) gps_packet.elev;
					for(i = 0; i < sizeof(gps_packet.elev); i++) UDPPut(*cp++);
				}
	            UDPFlush();
				sendgps = 0;
				gpsforcetimer = 0;
	         }
			memclr(&gps_packet,sizeof(gps_packet));
	}
	if ((gpssync || (!USE_PPS)) && UDPIsGetReady(*udpSocketUser)) {
		n = 0;
		cp = (BYTE *) &audio_packet;
		while(UDPGet(&c))
		{
			if (n++ < sizeof(audio_packet)) *cp++ = c;	
		}
		if (n >= sizeof(VOTER_PACKET_HEADER)) {
			/* if this is a new session */
			if (strcmp((char *)audio_packet.vph.challenge,their_challenge))
			{
				connected = 0;
				txseqno = 0;
				txseqno_ptt = 0;
				lastrxtimer = 0;
				resp_digest = crc32_bufs(audio_packet.vph.challenge,(BYTE *)AppConfig.Password);
				strcpy(their_challenge,(char *)audio_packet.vph.challenge);
			}
			else 
			{
				if ((!digest) || (!audio_packet.vph.digest) || (digest != ntohl(audio_packet.vph.digest)) ||
					(ntohs(audio_packet.vph.payload_type) == 0))
				{
					mydigest = crc32_bufs((BYTE *)challenge,(BYTE *)AppConfig.HostPassword);
					if (mydigest == ntohl(audio_packet.vph.digest))
					{
						digest = mydigest;
						if (!connected) gpsforcetimer = 0;
						connected = 1;
						if (n > sizeof(VOTER_PACKET_HEADER)) option_flags = audio_packet.rssi;
						else option_flags = 0;
						if ((!USE_PPS) && (!(option_flags & OPTION_FLAG_MIX)))
						{
							printf(badmix);
							connected = 0;
							txseqno = 0;
							txseqno_ptt = 0;
							digest = 0;
							lastrxtimer = 0;
						}
						else SetAudioSrc();
					}
					else
					{
						connected = 0;
						txseqno = 0;
						txseqno_ptt = 0;
						digest = 0;
						lastrxtimer = 0;
					}
				}
				else
				{
					if (!connected) gpsforcetimer = 0;
					connected = 1;
					lastrxtimer = 0;
					if ((ntohs(audio_packet.vph.payload_type) == 1) || (ntohs(audio_packet.vph.payload_type) == 3))
					{
						long index,ndiff;
						short mydiff;


						if (!USE_PPS)
						{
							mytxseqno = txseqno;
							if (mytxseqno > (txseqno_ptt + 2))
									host_txseqno = 0;
							txseqno_ptt = mytxseqno;
							if (!host_txseqno) myhost_txseqno = host_txseqno = ntohl(audio_packet.vph.curtime.vtime_nsec);
							index = (ntohl(audio_packet.vph.curtime.vtime_nsec) - myhost_txseqno);
							index *= FRAME_SIZE;
							index -= (FRAME_SIZE * 2);
//printf("%ld %ld %ld\n",index,ntohl(audio_packet.vph.curtime.vtime_nsec),myhost_txseqno);
						}
						else
						{
							index = (ntohl(audio_packet.vph.curtime.vtime_sec) - system_time.vtime_sec) * 8000;
							ndiff = ntohl(audio_packet.vph.curtime.vtime_nsec) - system_time.vtime_nsec;
							index += (ndiff / 125000);
						}
						index -= (FRAME_SIZE * 2);
						index += AppConfig.TxBufferLength - (FRAME_SIZE * 2);

                        /* if in bounds */
                        if ((index > 0) && (index <= (AppConfig.TxBufferLength - (FRAME_SIZE * 2))))
                        {
							if (!USE_PPS)
							{
								if ((txseqno + (index / FRAME_SIZE)) > txseqno_ptt) 
									txseqno_ptt = (txseqno + (index / FRAME_SIZE));
							}
							else
							{
								if ((ntohl(audio_packet.vph.curtime.vtime_sec) > lastrxtime.vtime_sec) ||
									((ntohl(audio_packet.vph.curtime.vtime_sec) == lastrxtime.vtime_sec) &&
										(ntohl(audio_packet.vph.curtime.vtime_nsec) > lastrxtime.vtime_nsec)))
								{
									lastrxtime.vtime_sec = ntohl(audio_packet.vph.curtime.vtime_sec);
									lastrxtime.vtime_nsec = ntohl(audio_packet.vph.curtime.vtime_nsec);
								}
							}
                            index += mytxindex;
					   		if (index > AppConfig.TxBufferLength) index -= AppConfig.TxBufferLength;
							mydiff = AppConfig.TxBufferLength;

							if (ntohs(audio_packet.vph.payload_type) == 3)
							{
					  			mydiff -= ((short)index + (FRAME_SIZE * 2));
								dec_valprev = (audio_packet.audio[160] << 8) + audio_packet.audio[161];
								dec_index = audio_packet.audio[162];
								adpcm_decoder(audio_packet.audio);
	                            if (mydiff >= 0)
	                            {
	                                    memcpy(txaudio + index,dec_buffer,FRAME_SIZE * 2);
	                             }
	                            else
	                            {
	                                    memcpy(txaudio + index,dec_buffer,(FRAME_SIZE * 2) + mydiff);
	                                    memcpy(txaudio,dec_buffer + ((FRAME_SIZE * 2) + mydiff),-mydiff);
	                            }

							}
							else
							{
					  			mydiff -= ((short)index + FRAME_SIZE);
	                            if (mydiff >= 0)
	                            {
	                                    memcpy(txaudio + index,audio_packet.audio,FRAME_SIZE);
	                             }
	                            else
	                            {
	                                    memcpy(txaudio + index,audio_packet.audio,FRAME_SIZE + mydiff);
	                                    memcpy(txaudio,audio_packet.audio + (FRAME_SIZE + mydiff),-mydiff);
	                            }
							}
                        }
						else if (!USE_PPS)
						{
							host_txseqno = 0;
						}
					}
				}
			}
		}
		UDPDiscard();
	}
}

void main_processing_loop(void)
{

	static ROM char  cfgwritten[] = "Squelch calibration saved, noise gain = ",diodewritten[] = "Diode calibration saved, value (hex) = ",
		dnschanged[] = " Voter Host DNS Resolved to %d.%d.%d.%d\n", dnsfailed[] = "  Warning: Unable to resolve DNS for Voter Host %s\n";
		
	static ROM char ipinfo[] = "\nIP Configuration Info: \n",ipwithdhcp[] = "Configured With DHCP\n",
			ipwithstatic[] = "Static IP Configuration\n", ipipaddr[] = "IP Address: %d.%d.%d.%d\n",
			ipsubnet[] = "Subnet Mask: %d.%d.%d.%d\n", ipgateway[] = "Gateway Addr: %d.%d.%d.%d\n";
	static ROM char diagerr1[] = "Error - Failed to read PTT/CTCSS in un-asserted state\n",
		diagerr2[] = "Error - Failed to read PTT/CTCSS in asserted state\n",
		diagerr3[] = "Error - Failed to read data from GPS UART\n",
		diagfail[] = "  \nDiagnostics Failed With %d Errors!!!!\n",
		diagpass[] = " \nDiagnostics Passed Successfully\n",
		diag1[] = "Testing PTT/External CTCSS\n",
		diag2[] = "Testing GPS UART\n",
		measerr[] = "Error -- Measured %u, should have been between %u and %u\n",
		measmsg[] = "Testing level at %d Hz for %s\n",
		flat_test_str[] = "Normal Audio",plfilt_test_str[] = "CTCSS Filtered Audio",deemp_test_str[] = "De-Emphasized Audio",
		sql_test_str[] = "Squelch Noise Detector";

	static ROM struct meas flat_test[] = {
		{100,9750,13350,0},{320,10655,14416,0},{500,10485,14186,0},{1000,9798,13257,0},{2000,8989,12162,0},{3200,6929,9374,0},{0,0,0,0} 
	}, plfilt_test[] = {
		{100,100,451,0},{320,10655,14416,0},{500,10485,14186,0},{1000,9798,13257,0},{2000,8989,12162,0},{3200,6929,9374,0},{0,0,0,0} 
	}, deemp_test[] = {
		{1000,9798,13257,0},{2000,4380,6155,0},{3200,2271,3073,0},{0,0,0,0} 
	}, sql_test[] = {
		{3200,0,50,1},{6000,200,375,1},{7200,500,1023,1},{0,0,0,0} 
	};


	static ROM BYTE diaguart[] = {0x55,0xaa,0x69,0};

	static DWORD t = 0, t1 = 0, tdisp = 0,tdiag = 0;
	long meas,thresh;
	WORD i,mypeak;
	long x,y,z;
	static BYTE dispcnt = 0;
	struct meas *m;

	//UDP State machine
	#define SM_UDP_SEND_ARP     0
	#define SM_UDP_WAIT_RESOLVE 1
	#define SM_UDP_RESOLVED     2

	static BYTE smUdp = SM_UDP_SEND_ARP;

	static DWORD  tsecWait = 0;           //General purpose wait timer
	static WORD mynoise;

	inprocloop = 1;

	if(!MACIsLinked())
	{
			connected = 0;
			txseqno = 0;
			txseqno_ptt = 0;
			resp_digest = 0;
			digest = 0;
			their_challenge[0] = 0;
			lastrxtimer = 0;
			/* if MAC was linked and now isnt, advance linkstate */
			if (linkstate == 1) linkstate++;
	}
	else
	{
			/* if first time having link, advance linkstate */
			if (linkstate == 0) linkstate++;
			/* if had link, lost it, and now have it again, reset processor (silly TCP/IP stack!) */
			if (linkstate == 2) Reset();
	}
	if ((!AppConfig.Flags.bIsDHCPEnabled) || (!AppConfig.Flags.bInConfigMode))
	{

		if (AppConfig.VoterServerFQDN[0])
		{
			if (!dnstimer)
			{
				DNSEndUsage();
				dnstimer = 120;
				dnsdone = 0;
				if(DNSBeginUsage()) 
					DNSResolve((BYTE *)AppConfig.VoterServerFQDN,DNS_TYPE_A);
			}
			else
			{

				if ((!dnsdone) && (DNSIsResolved(&MyVoterAddr)))
				{
					if (DNSEndUsage())
					{
						if (memcmp(&LastVoterAddr,&MyVoterAddr,sizeof(IP_ADDR)))
						{

							if (udpSocketUser != INVALID_UDP_SOCKET) UDPClose(udpSocketUser);
	   						memclr(&udpServerNode, sizeof(udpServerNode));
							udpServerNode.IPAddr = MyVoterAddr;
							udpSocketUser = UDPOpen(AppConfig.MyPort, &udpServerNode, AppConfig.VoterServerPort);
							smUdp = SM_UDP_SEND_ARP;
							dnsnotify = 1;
							CurVoterAddr = MyVoterAddr;
						}
						LastVoterAddr = MyVoterAddr;
					} 
					else dnsnotify = 2;

					dnsdone = 1;
				}
			}
	}

	if (connected && (lastrxtimer > LASTRX_TIME))
	{
			connected = 0;
			txseqno = 0;
			txseqno_ptt = 0;
			resp_digest = 0;
			digest = 0;
			their_challenge[0] = 0;
			lastrxtimer = 0;
	}

      if (udpSocketUser != INVALID_UDP_SOCKET) switch (smUdp) {
        case SM_UDP_SEND_ARP:
            if (ARPIsTxReady()) {
                //Remember when we sent last request
                tsecWait = TickGet();
                
                //Send ARP request for given IP address
                ARPResolve(&udpServerNode.IPAddr);
                
                smUdp = SM_UDP_WAIT_RESOLVE;
            }
            break;
        case SM_UDP_WAIT_RESOLVE:
            //The IP address has been resolved, we now have the MAC address of the
            //node at 10.1.0.101
            if (ARPIsResolved(&udpServerNode.IPAddr, &udpServerNode.MACAddr)) {
                smUdp = SM_UDP_RESOLVED;
            }
            //If not resolved after 2 seconds, send next request
            else {
				if ((TickGet() - tsecWait) >= TICK_SECOND / 2ul) {
                    smUdp = SM_UDP_SEND_ARP;
                }
            }
            break;
        case SM_UDP_RESOLVED:
			process_udp(&udpSocketUser,&udpServerNode);
            break;
       }

		if (gps_state != GPS_STATE_IDLE)
		{
			if ((!gpswarn) && (gpstimer > ((AppConfig.GPSProto == GPS_TSIP) ? GPS_TSIP_WARN_TIME : GPS_NMEA_WARN_TIME)))
			{
				gpswarn = 1;
				printf(logtime());
				printf(gpsmsg7);
			}
			if (gpstimer >((AppConfig.GPSProto == GPS_TSIP) ? GPS_TSIP_MAX_TIME : GPS_NMEA_MAX_TIME))
			{
				printf(logtime());
				printf(gpsmsg6);
				gps_state = GPS_STATE_IDLE;
				if (USE_PPS)
				{
					connected = 0;
					resp_digest = 0;
					digest = 0;
					their_challenge[0] = 0;
					lastrxtimer = 0;
				}
				gpssync = 0;
				gotpps = 0;
			}
		}
		if (gotpps && USE_PPS)
		{
			if ((!ppswarn) && (ppstimer > PPS_WARN_TIME))
			{
				ppswarn = 1;
				printf(logtime());
				printf(gpsmsg8);
			}
			if (ppstimer > PPS_MAX_TIME)
			{
				printf(logtime());
				printf(gpsmsg6);
				gps_state = GPS_STATE_IDLE;
				connected = 0;
				resp_digest = 0;
				digest = 0;
				their_challenge[0] = 0;
				gpssync = 0;
				gotpps = 0;
				lastrxtimer = 0;
			}
		}

#if !defined(SMT_BOARD)
	
		inputs1 = IOExp_Read(IOEXP_GPIOA);
		inputs2 = IOExp_Read(IOEXP_GPIOB);
#endif
	
		process_gps();
	
		if ((gotpps || (!USE_PPS)) && (sqlcount >= 32))
		{
			BOOL qualcor;

			sqlcount = 0;
			service_squelch(adcothers[ADCDIODE],0x3ff - adcothers[ADCSQPOT],adcothers[ADCSQNOISE],!CAL,!WVF,(AppConfig.SqlNoiseGain) ? 1: 0);
			sql2 ^= 1;

			qualcor = (HasCOR() && HasCTCSS());	
			if (!sql2)
			{
				if (qualcor && (!wascor))
				{
					vnoise32 = adcothers[ADCSQNOISE] << 3;
					vnoise256 = (DWORD)adcothers[ADCSQNOISE] << 3;
				}
				else vnoise32 = ((vnoise32 * 31) + (adcothers[ADCSQNOISE] << 3)) >> 5;
				vnoise256 = ((vnoise256 * 255) + ((DWORD)adcothers[ADCSQNOISE] << 3)) >> 8;
				wascor = qualcor;
			}
			mynoise = (WORD) vnoise256;
			if (mynoise < NOISE_SLOW_THRESHOLD) mynoise = vnoise32; 
			rssi = rssitable[mynoise >> 3];
			if ((rssi < 1) && (qualcor)) rssi = 1;
			if (!AppConfig.SqlNoiseGain) rssi = 0;

			lastcor = HasCOR();
			if (write_eeprom_cali)
			{
				write_eeprom_cali = 0;
				AppConfig.SqlNoiseGain = noise_gain;
				if (!WVF) AppConfig.SqlDiode = caldiode;
				SaveAppConfig();
				printf(cfgwritten);
				printf("%d\n",noise_gain);
				if (!WVF)
				{
					printf(diodewritten);
					printf("%d\n",caldiode);
				}
			}
			if (!CAL) SetLED(SQLED,sqled);
		}
		z = 100000;
		x = system_time.vtime_sec - lastrxtime.vtime_sec;

		if (!indiag)
		{
			if (connected)
			{
				if (!USE_PPS)
				{
					if (ptt && ((txseqno > (txseqno_ptt + 2)) || (AppConfig.Elkes && 
						(AppConfig.Elkes != 0xffffffff) && (elketimer >= AppConfig.Elkes))))
					{
						ptt = 0;
						SetPTT(0);
					}
					else if (!ptt && (txseqno <= (txseqno_ptt + 2)) && ((!AppConfig.Elkes) ||
						(AppConfig.Elkes == 0xffffffff) || (elketimer < AppConfig.Elkes)))
					{
						ptt = 1;
						SetPTT(1);
					}
				}
				else
				{
					if (lastrxtime.vtime_sec && (x < 100) && (z <= 100000))
					{
						y = system_time.vtime_nsec - lastrxtime.vtime_nsec;
						z = x * 1000;
						z += y / 1000000;
						z -= (AppConfig.TxBufferLength - 160) >> 3;
					}
					if (ptt && ((z > 60L) || (AppConfig.Elkes && 
						(AppConfig.Elkes != 0xffffffff) && (elketimer >= AppConfig.Elkes))))
					{
						ptt = 0;
						SetPTT(0);
					}
					else if (!ptt && (z <= 60L) && ((!AppConfig.Elkes) ||
						(AppConfig.Elkes == 0xffffffff) || (elketimer < AppConfig.Elkes)))
					{
						ptt = 1;
						SetPTT(1);
					}
				}
			}
			else SetPTT(0);	
		}
	}

	if (!indiag)
	{
		if (LEVDISP)
		{
			SetLED(CONNLED,connected);
			if ((gps_state == GPS_STATE_SYNCED) ||
				((gps_state == GPS_STATE_VALID) && (!USE_PPS)))
					SetLED(GPSLED,1);
			else if (gps_state != GPS_STATE_VALID)
				SetLED(GPSLED,0);
		}
		else
		{
			if (HasCOR() && HasCTCSS())
			{
				mypeak = apeak / (7200 / LEVDISP_FACTOR);
				if (mypeak < dispcnt) SetLED(CONNLED,1);
				else SetLED(CONNLED,0);
				if (mypeak > (dispcnt + LEVDISP_FACTOR)) SetLED(GPSLED,1);
				else SetLED(GPSLED,0);
			}
			else
			{
				SetLED(GPSLED,0);
				SetLED(CONNLED,0);
			}
			dispcnt++;
			if (dispcnt > LEVDISP_FACTOR) dispcnt = 0;
		}
	
		if (CAL)
		{	
			if (lastcor && HasCTCSS())
				SetLED(SQLED,1);
			else if ((!lastcor) || ((AppConfig.CORType != 0) && (!HasCTCSS())))
				SetLED(SQLED,0);
		}
	}

       // Blink LEDs as appropriate
       if(TickGet() - t1 >= TICK_SECOND / 6ul)
        {
		  t1 = TickGet();
		  if (indiag) ToggleLED(SYSLED);
	    }
       // Blink LEDs as appropriate
       if(TickGet() - t >= TICK_SECOND / 2ul)
       {
		if (dnstimer) dnstimer--;
        t = TickGet();
		if (!indiag)
		{
			ToggleLED(SYSLED);
			if (LEVDISP)
			{
				if ((gps_state == GPS_STATE_VALID) && USE_PPS) ToggleLED(GPSLED);
			}
			if (CAL && (AppConfig.CORType == 0) && (lastcor && (!HasCTCSS()))) ToggleLED(SQLED);
		}
#ifdef	SILLY
		printf("%lu\n",sillyval);
#endif
       }

       // This task performs normal stack task including checking
       // for incoming packet, type of packet and calling
       // appropriate stack entity to process it.
       StackTask();
       
       // This tasks invokes each of the core stack application tasks
       StackApplications();

	   if ((termbufidx > 0) && (termbuftimer > TELNET_TIME)) ProcessTelnetTimer();

	   if ((!indipsw) && (!indisplay) && (!leddiag)) tdisp = 0;

	   if (!inread)
	   {
			inprocloop = 0;
			return;
	   }

	   // Rx Level Display handler

       if(indisplay && (TickGet() - tdisp >= TICK_SECOND / 10ul))
	   {
        	tdisp = TickGet();
			if (indiag || (HasCOR() && HasCTCSS()))
				meas = apeak;
			else
				meas = 0;
			putchar('|');
            for(i = 0; i < NCOLS; i++)
            {
                    thresh = (meas * (long)NCOLS) / 16384;
                    if (i < thresh) putchar('=');
                    else if (i == thresh) putchar('>');
                    else putchar(' ');
            }
			putchar('|');
			putchar('\r');
			fflush(stdout);
			amin = amax = 0;
	   }

	   // Dip Switch diagnostic display handler
       if(indipsw && (TickGet() - tdisp >= TICK_SECOND / 10ul))
	   {
        	tdisp = TickGet();
			printf(" (%s) (%s) (%s) (%s)\r",(!JP8) ? "Down" : " Up ",(!JP9) ? "Down" : " Up ",
				(!JP10) ? "Down" : " Up ",(!JP11) ? "Down" : " Up ");
			fflush(stdout);
	   } 
       // LED Flash diagnostic handler
       if(leddiag && (TickGet() - tdisp >= TICK_SECOND * 2ul))
	   {
        	tdisp = TickGet();
			SetLED(SQLED,0);
			SetLED(GPSLED,0);
			SetLED(CONNLED,0);
			SetPTT(0);
			switch (leddiag)
			{
			    case 1:
					SetLED(SQLED,1);
				    leddiag = 2;
					break;
			    case 2:
					SetLED(CONNLED,1);
					leddiag = 3;
					break;
			    case 3:
					SetPTT(1);
					leddiag = 4;
					break;
				case 4:
					SetLED(GPSLED,1);
					leddiag = 1;
					break;
			}
	   }

       // "Diagnostic Suite" handler
	   if (indiag && (tdiag < TickGet()))
       {
		// If we have a result, perform measurement and check results
		if (measp && measidx)
		{
			BOOL isok = 1;
			DWORD accum = 0;
			WORD aval;
			short sqlval = adcothers[ADCSQNOISE] - (AppConfig.SqlDiode - adcothers[ADCDIODE]);

			m = (measp + (measidx - 1));
			for(i = 0; i < NAPEAKS; i++) accum += apeaks[i];
			aval = (WORD)(accum / NAPEAKS);
			if (sqlval < 0) sqlval = 0;
			if (m->issql)
			{
				if ((sqlval < m->min) || (sqlval > m->max)) isok = 0;
			}
			else
			{
				if ((aval < m->min) || (aval > m->max)) isok = 0;
			}
			if (!isok) 
			{
				printf(measerr,(m->issql) ? sqlval : aval,m->min,m->max);
				errcnt++;
			}
		    // Grab next "step" in current measurement sequence
			measidx++;
			m = (measp + (measidx - 1));
			memset(apeaks,0,sizeof(apeaks));
			// If no more steps
			if (!m->freq) 
			{
				measp = 0;
				measidx = 0;
				measstr = 0;
			}
			// Otherwise set new freq and start new measurement
			else
			{
				SetTxTone(m->freq);
				tdiag = TickGet() + DIAG_WAIT_MEAS;
				if (measstr) printf(measmsg,m->freq,measstr);
			}
		}
		if (!measp)
		{
			// State machine for various steps of diagnostics
			switch(diagstate)
			{
				case 1: // Make sure PTT is seen in both states and
						// set up UART and send test data
					printf(diag1);
					SetPTT(0);
					Nop();
					Nop();
					Nop();
					Nop();
					if (!CTCSSIN)
					{
						errcnt++;
						printf(diagerr1);
					}
					SetPTT(1);
					Nop();
					Nop();
					Nop();
					Nop();
					if (CTCSSIN)
					{
						errcnt++;
						printf(diagerr2);
					}
					printf(diag2);
					U2MODEbits.URXINV = 0;
					U2STAbits.UTXINV = 0;
					while (DataRdyUART2()) ReadUART2();
					putrsUART2((ROM char *)diaguart);
					diagstate = 2;
					tdiag = TickGet() + DIAG_WAIT_UART; // Wait 333 ms
					break;
				case 2: // see if UART got test data okay
					for(i = 0; diaguart[i]; i++)
					{
						if (!DataRdyUART2()) break;
						if (ReadUART2() != diaguart[i]) break;
					}
					if (diaguart[i] || DataRdyUART2())
					{
						errcnt++;
						printf(diagerr3);
					}
#if defined(SMT_BOARD)					U2MODEbits.URXINV = AppConfig.GPSPolarity;
					U2STAbits.UTXINV = AppConfig.GPSPolarity;
#else
					U2MODEbits.URXINV = AppConfig.GPSPolarity ^ 1;
					U2STAbits.UTXINV = AppConfig.GPSPolarity ^ 1;
#endif
					// Perform measurement sequence with no filters on audio
					diag_option_flags = OPTION_FLAG_FLATAUDIO | OPTION_FLAG_NOCTCSSFILTER;
					SetAudioSrc();
					diagstate = 3;
					measp = (struct meas *)flat_test;
					measstr = (char *)flat_test_str;
					measidx = 1;
					m = (measp + (measidx - 1));
					memset(apeaks,0,sizeof(apeaks));
					SetTxTone(m->freq);
					tdiag = TickGet() + DIAG_WAIT_MEAS;
					printf(measmsg,m->freq,measstr);
					break;
				case 3: // Perform measurement sequence with de-demphasis enabled
					diag_option_flags = OPTION_FLAG_FLATAUDIO;
					SetAudioSrc();
					diagstate = 4;
					measp = (struct meas *)plfilt_test;
					measstr = (char *)plfilt_test_str;
					measidx = 1;
					m = (measp + (measidx - 1));
					memset(apeaks,0,sizeof(apeaks));
					SetTxTone(m->freq);
					tdiag = TickGet() + DIAG_WAIT_MEAS;
					printf(measmsg,m->freq,measstr);
					break;
				case 4: // Perform measurement sequence with CTCSS filter enabled
					diag_option_flags = OPTION_FLAG_NOCTCSSFILTER;
					SetAudioSrc();
					diagstate = 5;
					measp = (struct meas *)deemp_test;
					measstr = (char *)deemp_test_str;
					measidx = 1;
					m = (measp + (measidx - 1));
					memset(apeaks,0,sizeof(apeaks));
					SetTxTone(m->freq);
					tdiag = TickGet() + DIAG_WAIT_MEAS;
					printf(measmsg,m->freq,measstr);
					break;
				case 5: // Perform measurement sequence of squelch noise detector
					diag_option_flags = 0;
					SetAudioSrc();
					diagstate = 255;
					measp = (struct meas *)sql_test;
					measstr = (char *)sql_test_str;
					measidx = 1;
					m = (measp + (measidx - 1));
					set_atten(DIAG_NOISE_GAIN);
					memset(apeaks,0,sizeof(apeaks));
					SetTxTone(m->freq);
					tdiag = TickGet() + DIAG_WAIT_MEAS;
					printf(measmsg,m->freq,measstr);
					break;
				case 255: // No more measurements to do
					tdiag = 0;
					if (errcnt) printf(diagfail,errcnt);
					else printf(diagpass);
					printf(paktc);
					diagstate = 0;
					break;
			}
		}
	   }
       // If the local IP address has changed (ex: due to DHCP lease change)
       // write the new IP address to the LCD display, UART, and Announce 
       // service
	if(dwLastIP != AppConfig.MyIPAddr.Val)
	{
		dwLastIP = AppConfig.MyIPAddr.Val;

		
		printf(ipinfo);
		if (AppConfig.Flags.bIsDHCPReallyEnabled)
			printf(ipwithdhcp);
		else
			printf(ipwithstatic);
		printf(ipipaddr,AppConfig.MyIPAddr.v[0],AppConfig.MyIPAddr.v[1],AppConfig.MyIPAddr.v[2],AppConfig.MyIPAddr.v[3]);
		printf(ipsubnet,AppConfig.MyMask.v[0],AppConfig.MyMask.v[1],AppConfig.MyMask.v[2],AppConfig.MyMask.v[3]);
		printf(ipgateway,AppConfig.MyGateway.v[0],AppConfig.MyGateway.v[1],AppConfig.MyGateway.v[2],AppConfig.MyGateway.v[3]);

		#if defined(STACK_USE_ANNOUNCE)
			AnnounceIP();
		#endif
	}
	if (dnsnotify == 1)
	{
		printf(logtime());
		printf(dnschanged,MyVoterAddr.v[0],MyVoterAddr.v[1],MyVoterAddr.v[2],MyVoterAddr.v[3]);
	}
	else if (dnsnotify == 2) 
	{
		printf(logtime());
		printf(dnsfailed,AppConfig.VoterServerFQDN);
	}
	dnsnotify = 0;
	inprocloop = 0;
}

int write(int handle, void *buffer, unsigned int len)
{
int i;
BYTE *cp;
BOOL instuff;

		instuff = (inread || inprocloop);
		i = len;
		if ((handle >= 0) && (handle <= 2))
		{
			cp = (BYTE *)buffer;
			while(i--)
			{
				while(BusyUART()) if (!instuff) main_processing_loop();
				if (*cp == '\n')
				{
					WriteUART('\r');
					if (netisup) while(!PutTelnetConsole('\r')) if (!instuff) main_processing_loop();
					while(BusyUART()) if (!instuff) main_processing_loop();
				}
				WriteUART(*cp);
				if (netisup) while (!PutTelnetConsole(*cp)) if (!instuff) main_processing_loop();
				cp++;
			}
		}
		else
		{
			errno = EBADF;
			return(-1);
		}
	return(len);
}

int read(int handle, void *buffer, unsigned int len)
{
BYTE *dest,c;
int count,x;

	ClrWdt();

	if ((!ininput) || inprocloop) return(0);

	inread = 1;
	aborted = 0;
	if ((handle < 0) || (handle > 2)) 
	{
		errno = EBADF;
		inread = 0;
		return(-1);
	}
	if ((handle == 1) || (handle == 2))
	{
		inread = 0;
		return(0);
	}
	dest = (BYTE *)buffer;
	count = 0;
	while(count < len)
	{
		dest[count] = 0;
		for(;;)
		{
			ClrWdt();
			if (DataRdyUART())
			{
				c = ReadUART() & 0x7f;
				break;
			}
			if (netisup)
			{
				c = GetTelnetConsole();
				if (c == 4) 
				{
					CloseTelnetConsole();
					continue;
				}
				if (c) break;
			}
			if (!inprocloop) main_processing_loop();
		}
		if (c == 3) 
		{
			while(BusyUART()) if (!inprocloop) main_processing_loop();
			WriteUART('^');
			if (telnet_echo && netisup) while(!PutTelnetConsole('^')) if (!inprocloop) main_processing_loop();
			while(BusyUART()) if (!inprocloop) main_processing_loop();
			WriteUART('C');
			if (telnet_echo && netisup) while(!PutTelnetConsole('C')) if (!inprocloop) main_processing_loop();
			aborted = 1;
			dest[0] = '\n';
			dest[1] = 0;
			count = 1;
			break;
		}
		if ((c == 8) || (c == 127) || (c == 21))
		{
			if (!count) continue;
			if (c == 21) x = count;
			else x = 1;
			while(x--)
			{
				count--;
				dest[count] = 0;
				while(BusyUART()) if (!inprocloop) main_processing_loop();
				WriteUART(8);
				if (netisup) while(!PutTelnetConsole(8)) if (!inprocloop) main_processing_loop();
				while(BusyUART()) main_processing_loop();
				WriteUART(' ');
				if (netisup) while(!PutTelnetConsole(' ')) if (!inprocloop) main_processing_loop();
				while(BusyUART()) main_processing_loop();
				WriteUART(8);
				if (netisup) while(!PutTelnetConsole(8)) if (!inprocloop) main_processing_loop();
			}
			continue;
		}
		if (c == 4) 
		{
			if (netisup) CloseTelnetConsole();
			continue;
		}
		if ((c != '\r') && (c < ' ')) continue;
		if (c > 126) continue;
		if (c == '\r') c = '\n';
		dest[count++] = c;
		dest[count] = 0;
		if (c == '\n') break;
		while(BusyUART()) if (!inprocloop) main_processing_loop();
		WriteUART(c);
		if (telnet_echo && netisup) while(!PutTelnetConsole(c)) if (!inprocloop) main_processing_loop();

	}
	while(BusyUART()) if (!inprocloop) main_processing_loop();
	WriteUART('\r');
	if (netisup) while(!PutTelnetConsole('\r')) if (!inprocloop) main_processing_loop();
	while(BusyUART()) if (!inprocloop) main_processing_loop();
	WriteUART('\n');
	if (netisup) while(!PutTelnetConsole('\n')) if (!inprocloop) main_processing_loop();
	inread = 0;
	return(count);
}

static void SetDynDNS(void)
{
	static ROM BYTE checkip[] = "checkip.dyndns.com", update[] = "members.dyndns.org";
	memset(&DDNSClient,0,sizeof(DDNSClient));
	if (AppConfig.DynDNSEnable)
	{
		DDNSClient.CheckIPServer.szROM = checkip;
		DDNSClient.ROMPointers.CheckIPServer = 1;
		DDNSClient.CheckIPPort = 80;
		DDNSClient.UpdateServer.szROM = update;
		DDNSClient.ROMPointers.UpdateServer = 1;
		DDNSClient.UpdatePort = 80;
		DDNSClient.Username.szRAM = AppConfig.DynDNSUsername;
		DDNSClient.Password.szRAM = AppConfig.DynDNSPassword;
		DDNSClient.Host.szRAM = AppConfig.DynDNSHost;
	}
}


static void DiagMenu()
{

 indiag = 1; 
 gps_state = GPS_STATE_IDLE;
 if (USE_PPS)
 {
	connected = 0;
	resp_digest = 0;
	digest = 0;
	their_challenge[0] = 0;
	lastrxtimer = 0;
 }
 gpssync = 0;
 gotpps = 0;

 while(1) 
	{
		char cmdstr[50];
		int i,sel;

	static ROM char menu[] = "Select the following Diagnostic functions:\n\n" 
		"1  - Set Initial Tone Level (and assert PTT)\n"
		"2  - Display Value of DIP Switches\n"
		"3  - Flash LED's in sequence\n"
		"4  - Run entire diag suite\n"
		"c  - Show Diagnostic Cable Pinouts\n"
		"x -  Exit Diagnostic Menu (back to main menu)\nq - Disconnect Remote Console Session, r - reboot system\n\n",
		entsel[] = "Enter Selection (1-4,c,x,q,r) : ",
		settone[] = "Adjust Tx Level for 1V P-P (1 KHz) on output, then adjust Rx Level\nto \"5 KHz\" on display\n\n",
		dipstr[] = "Dip Switch Values\n\n   SW1    SW2    SW3    SW4\n",
		diodewarn[] = "Warning!! VF Diode NOT calibrated!!!\n\n",
		ledstr[] = "LED's will flash as follows: Squelch (Top Green), GPS (Middle Yellow),\n"
				"PTT (Red), Host (Top, Right Yellow). System LED will continue flashing\nat fast speed.\n",
		diagstr[] = "Running Diagnostics...\n\n",
		diagcable[] = "Diagnostic Cable Pinouts:\n\n9 Pin D-Shell Connector (Radio Connector):\n\n"
			"1 - VIN (+12V or so)\n"
			"2 - connects to 3 and also is output to 'scope for tone measurement\n"
			"4 - connects to 7\n"
			"5 - Gnd\n\n"
			"15 Pin D-Shell Connector (GPS/Console Connector):\n\n"
			"2 - Console Pin 2\n"
			"3 - Console Pin 3\n"
			"5 - Console Pin 5\n"
			"6 - connects to 14\n"
			"\"Console\" is a 9 Pin D-shell connector that connects to serial console device\n\n";

		printf(menu);
		fflush(stdout);
		SetLED(SQLED,0);
		SetLED(GPSLED,0);
		SetLED(CONNLED,0);
		SetPTT(0);
		aborted = 0;
		while(!aborted)
		{
			printf(entsel);
			memset(cmdstr,0,sizeof(cmdstr));
			if (!myfgets(cmdstr,sizeof(cmdstr) - 1,stdin)) continue;
			if (!strchr(cmdstr,'!')) break;
		}
		if (aborted) continue;
		if ((strchr(cmdstr,'Q')) || strchr(cmdstr,'q'))
		{
				CloseTelnetConsole();
				continue;
		}
		if ((strchr(cmdstr,'R')) || strchr(cmdstr,'r'))
		{
				CloseTelnetConsole();
				printf(booting);
				Reset();
		}
		if ((strchr(cmdstr,'X')) || strchr(cmdstr,'x'))
		{
				break;
		}
		printf(" \n");
		if ((strchr(cmdstr,'C')) || strchr(cmdstr,'c'))
		{
		        printf(diagcable);
 				printf(paktc);
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				printf("\n\n");
				continue;
		}
		sel = atoi(cmdstr);
		switch(sel)
		{
			case 1: // Send 1000Hz Tone, Display RX Level Quasi-Graphically 
				SetPTT(1); 
				SetTxTone(1000);
				printf(settone);
			 	putchar(' ');
		      	for(i = 0; i < NCOLS; i++) putchar(' ');
		        printf(rxvoicestr);
				indisplay = 1;
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				indisplay = 0;
				SetPTT(0);
				SetTxTone(0);
				continue;
			case 2: // Dip Switch test  
		        printf(dipstr);
				indipsw = 1;
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				indipsw = 0;
				printf("\n\n");
				continue;
			case 3: // Flash LED's
		        printf(ledstr);
				leddiag = 1;
 				printf(paktc);
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				leddiag = 0;
				printf("\n\n");
				continue;
			case 4: // Run diags
		        printf(diagstr);
				if (!AppConfig.SqlDiode) printf(diodewarn);
				errcnt = 0;
				diagstate = 1;
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				measp = 0;
				measidx = 0;
				measstr = 0;
#if defined(SMT_BOARD)				U2MODEbits.URXINV = AppConfig.GPSPolarity;
				U2STAbits.UTXINV = AppConfig.GPSPolarity;
#else
				U2MODEbits.URXINV = AppConfig.GPSPolarity ^ 1;
				U2STAbits.UTXINV = AppConfig.GPSPolarity ^ 1;
#endif
				diagstate = 0;
				printf("\n\n");
				continue;
			default:
				printf(invalselection);
				continue;
		}
	}
	SetTxTone(0);
	SetPTT(0);
	set_atten(noise_gain);
	indiag = 0;
	SetAudioSrc();
}


int main(void)
{

	WORD sel;
	time_t t;
	BYTE i;

    static ROM char signon[] = "\nVOTER Client System verson 0.55  12/16/2011, Jim Dixon WB6NIL\n";

	static ROM char entnewval[] = "Enter New Value : ", newvalchanged[] = "Value Changed Successfully\n",
		newvalerror[] = "Invalid Entry, Value Not Changed\n", newvalnotchanged[] = "No Entry Made, Value Not Changed\n",
		saved[] = "Configuration Settings Written to EEPROM\n", defwritten[] = "\nDefault Values Written to EEPROM\n",
		defdiode[] = "Diode Calibration Value Written to EEPROM\n";
			
	static ROM char menu[] = "Select the following values to View/Modify:\n\n" 
		"1  - Serial # (%d),  "
		"2  - (Static) IP Address (%d.%d.%d.%d)\n"
		"3  - (Static) Netmask (%d.%d.%d.%d)\n"
		"4  - (Static) Gateway (%d.%d.%d.%d)\n"
		"5  - (Static) Primary DNS Server (%d.%d.%d.%d)\n"
		"6  - (Static) Secondary DNS Server (%d.%d.%d.%d),   "
		"7  - DHCP Enable (%d)\n"
		"8  - Voter Server Address (FQDN) (%s)\n"
		"9  - Voter Server Port (%d),   "
		"10 - Local Port (Override) (%d)\n"
		"11 - Client Password (%s),   "
		"12 - Host Password (%s)\n"
		"13 - Tx Buffer Length (%d),   "
		"14 - GPS Data Protocol (0=NMEA, 1=TSIP) (%d)\n"
		"15 - GPS Serial Polarity (0=Non-Inverted, 1=Inverted) (%d)\n"
		"16 - GPS PPS Polarity (0=Non-Inverted, 1=Inverted, 2=NONE) (%d)\n"
		"17 - GPS Baud Rate (%lu),  "
		"18 - RFU,  "
		"19 - Telnet Port (%d)\n"
		"20 - Telnet Username (%s),  "
		"21 - Telnet Password (%s)\n"
		"22 - DynDNS Enable (%d),   "
		"23 - DynDNS Username (%s)\n"
		"24 - DynDNS Password (%s)\n"
		"25 - DynDNS Host (%s)\n"
		"26 - External CTCSS (0=Ignore, 1=Non-Inverted, 2=Inverted) (%d)\n"
		"27 - COR Type (0=Normal, 1=IGNORE COR, 2=No Receiver) (%d)\n"
		"28 - Debug Level (%d), "
		"29 - BootLoader IP Address (%d.%d.%d.%d) (%s)\n"
		"97 - RX Level, "
		"98 - Status, "
		"99 - Save Values to EEPROM\n"
		"q - Disconnect Remote Console Session, r - reboot system, d - diagnostics\n\n",
		entsel[] = "Enter Selection (1-27,97-99,r,q,d) : ";




	static ROM char oprdata[] = "IP Address: %d.%d.%d.%d\n"
		"Netmask: %d.%d.%d.%d\n"
		"Gateway: %d.%d.%d.%d\n"
		"Primary DNS: %d.%d.%d.%d\n"
		"Secondary DNS: %d.%d.%d.%d\n"
		"DHCP: %d\n"
		"VOTER Server IP: %d.%d.%d.%d\n"
		"VOTER Server UDP Port: %d\n"
		"OUR UDP Port: %d\n"
		"GPS Lock: %d\n"
		"Connected: %d\n"
		"COR: %d\n"
		"EXT CTCSS IN: %d\n"
		"PTT: %d\n"
		"RSSI: %d\n"
		"Current Samples / Sec.: %d\n"
		"Current Peak Audio Level: %u\n"
		"Squelch Noise Gain Value: %d, Diode Cal. Value: %d, SQL pot %d\n",
		curtimeis[] = "Current Time: %s.%03lu\n";

	filling_buffer = 0;
	fillindex = 0;
	filled = 0;
	time_filled = 0;
	connected = 0;
	lastrxtimer = 0;
	memclr((char *)audio_buf,FRAME_SIZE * 2);
	gps_bufindex = 0;
	TSIPwasdle = 0;
	gps_state = GPS_STATE_IDLE;
	gps_nsat = 0;
	gotpps = 0;
	gpssync = 0;
	ppscount = 0;
	rssi = 0;
	adcother = 0;
	adcindex = 0;
	memclr(adcothers,sizeof(adcothers));
	sqlcount = 0;
	sql2 = 0;
	wascor = 0;
	lastcor = 0;
	vnoise32 = 0;
	vnoise256 = 0;
	option_flags = 0;
	txdrainindex = 0;
	last_drainindex = 0;
	memset(&lastrxtime,0,sizeof(lastrxtime));
	ptt = 0;
	myDHCPBindCount = 0xff;
	digest = 0;
	resp_digest = 0;
	their_challenge[0] = 0;
	ppstimer = 0;
	gpstimer = 0;
	gpsforcetimer = 0;
	attempttimer = 0;
	ppswarn = 0;
	gpswarn = 0;
	dwLastIP = 0;
	inread = 0;
	memset(&MyVoterAddr,0,sizeof(MyVoterAddr));
	memset(&LastVoterAddr,0,sizeof(LastVoterAddr));
	memset(&CurVoterAddr,0,sizeof(CurVoterAddr));
	dnstimer = 0;
	dnsdone = 0;
	timing_time = 0;		// Time (whole secs) at beginning of next packet to be sent out
	timing_index = 0;		// Index at beginning of next packet to be sent out
	next_time = 0;			// Time (whole secs) samppled at begnning of current ADC frame
	next_index = 0;			// Index at beginning of current ADC frame
	samplecnt = 0;			// Index of ADC relative to PPS pulse
	last_adcsample = 2048;	// Last mulaw sample from ADC (so can be repeated if falls short)
	real_time = 0;			// Actual time (whole secs)
	last_samplecnt = 0;		// Last sample count (for display purposes)
	digest = 0;	
	resp_digest = 0;
	mydigest = 0;
	discfactor = 1000;
	discounterl = 0;
	discounteru = 0;
	amax = 0;
	amin = 0;
	apeak = 0;
	indisplay = 0;
	indipsw = 0;
	leddiag = 0;
	diagstate = 0;
    dec_valprev = 0;
    dec_index = 0;
    enc_valprev = 0;
    enc_index = 0;
    enc_prev_valprev = 0;
    enc_prev_index = 0;
	enc_lastdelta = 0;
	memset(dec_buffer,0,FRAME_SIZE);
	txseqno = 0;
	txseqno_ptt = 0;
	elketimer = 0;
	testidx = 0;
	testp = 0;
	indiag = 0;
	errcnt = 0;
	measp = 0;
	measstr = 0;
	measidx = 0;
	diag_option_flags = 0;
	netisup = 0;
	inprocloop = 0;
	ininput = 0;
	telnet_echo = 0;
	termbuftimer = 0;
	termbufidx = 0;
	linkstate = 0;

	// Initialize application specific hardware
	InitializeBoard();

	RCONbits.SWDTEN = 0;

	// Initialize stack-related hardware components that may be 
	// required by the UART configuration routines
    TickInit();

	SetLED(SYSLED,1);


	// Initialize Stack and application related NV variables into AppConfig.
	InitAppConfig();


	// UART
	#if defined(STACK_USE_UART)

		U1MODE = 0x8000;			// Set UARTEN.  Note: this must be done before setting UTXEN

		U1STA = 0x0400;		// UTXEN set

		#define CLOSEST_U1BRG_VALUE ((GetPeripheralClock()+8ul*BAUD_RATE1)/16/BAUD_RATE1-1)
		#define BAUD_ACTUAL1 (GetPeripheralClock()/16/(CLOSEST_U1BRG_VALUE+1))

		#define BAUD_ERROR1 ((BAUD_ACTUAL1 > BAUD_RATE1) ? BAUD_ACTUAL1-BAUD_RATE1 : BAUD_RATE1-BAUD_ACTUAL1)
		#define BAUD_ERROR_PRECENT1	((BAUD_ERROR1*100+BAUD_RATE1/2)/BAUD_RATE1)
		#if (BAUD_ERROR_PRECENT1 > 3)
			#warning UART1 frequency error is worse than 3%
		#elif (BAUD_ERROR_PRECENT1 > 2)
			#warning UART1 frequency error is worse than 2%
		#endif
	
		U1BRG = CLOSEST_U1BRG_VALUE;

		U2MODE = 0x8000;			// Set UARTEN.  Note: this must be done before setting UTXEN
#if !defined(SMT_BOARD)		U2MODEbits.URXINV = AppConfig.GPSPolarity ^ 1;
#else
		U2MODEbits.URXINV = AppConfig.GPSPolarity;
#endif

		U2STA = 0x0400;		// UTXEN set
#if !defined(SMT_BOARD)
		U2STAbits.UTXINV = AppConfig.GPSPolarity ^ 1;
#else
		U2STAbits.UTXINV = AppConfig.GPSPolarity;
#endif
		#define CLOSEST_U2BRG_VALUE ((GetPeripheralClock()+8ul*AppConfig.GPSBaudRate)/16/AppConfig.GPSBaudRate-1)
		U2BRG = CLOSEST_U2BRG_VALUE;

		InitUARTS();
	#endif

    // Initiates board setup process if button is depressed 
	// on startup
    if(!INITIALIZE)
    {
		#if defined(EEPROM_CS_TRIS) || defined(SPIFLASH_CS_TRIS)
		// Invalidate the EEPROM contents if BUTTON0 is held down for more than 4 seconds
		DWORD StartTime = TickGet();
		SetLED(SYSLED,1);


		while(!INITIALIZE)
		{
			if(TickGet() - StartTime > 4*TICK_SECOND)
			{
				#if defined(EEPROM_CS_TRIS)
			    XEEBeginWrite(0x0000);
			    XEEWrite(0xFF);
			    XEEEndWrite();
			    #elif defined(SPIFLASH_CS_TRIS)
			    SPIFlashBeginWrite(0x0000);
			    SPIFlashWrite(0xFF);
			    #endif

				printf(defwritten);
				if (!INITIALIZE_WVF) 
				{
					InitAppConfig();
					AppConfig.SqlDiode = adcothers[ADCDIODE];
					SaveAppConfig();
					printf(defdiode);
				}
				SetLED(SYSLED,0);
				while((LONG)(TickGet() - StartTime) <= (LONG)(9*TICK_SECOND/2));
				SetLED(SYSLED,1);
				while(!INITIALIZE);
				Reset();
				break;
			}
		}
		#endif
    }

	// Initialize core stack layers (MAC, ARP, TCP, UDP) and
	// application modules (HTTP, SNMP, etc.)
    StackInit();

	SetAudioSrc();

	init_squelch();


	udpSocketUser = INVALID_UDP_SOCKET;

	sprintf(challenge,"%lu",GenerateRandomDWORD() % 1000000000ul);

	// Now that all items are initialized, begin the co-operative
	// multitasking loop.  This infinite loop will continuously 
	// execute all stack-related tasks, as well as your own
	// application's functions.  Custom functions should be added
	// at the end of this loop.
    // Note that this is a "co-operative mult-tasking" mechanism
    // where every task performs its tasks (whether all in one shot
    // or part of it) and returns so that other tasks can do their
    // job.
    // If a task needs very long time to do its job, it must be broken
    // down into smaller pieces so that other tasks can have CPU time.
__builtin_nop();

	ClrWdt();
	RCONbits.SWDTEN = 1;
	ClrWdt();

	printf(signon);

	if (AppConfig.Flags.bIsDHCPReallyEnabled)
		dwLastIP = AppConfig.MyIPAddr.Val;

	SetDynDNS();

	if (!AppConfig.Flags.bIsDHCPReallyEnabled)
		AppConfig.Flags.bInConfigMode = FALSE;


    while(1) 
	{
		char cmdstr[50],ok;
		unsigned int i1,i2,i3,i4,x;
		unsigned long l;
		BOOL bootok;

		SetTxTone(0);
		if ((!netisup) && 
			((!AppConfig.Flags.bIsDHCPEnabled) || (!AppConfig.Flags.bInConfigMode)))
				netisup = 1;
		if (indiag) 
		{	
			SetPTT(0);
			set_atten(noise_gain);
		}
		indiag = 0;
		SetAudioSrc();
		bootok = ((AppConfig.BootIPCheck == GetBootCS()));
		printf(menu,AppConfig.SerialNumber,AppConfig.DefaultIPAddr.v[0],AppConfig.DefaultIPAddr.v[1],
			AppConfig.DefaultIPAddr.v[2],AppConfig.DefaultIPAddr.v[3],AppConfig.DefaultMask.v[0],
			AppConfig.DefaultMask.v[1],AppConfig.DefaultMask.v[2],AppConfig.DefaultMask.v[3],
			AppConfig.DefaultGateway.v[0],AppConfig.DefaultGateway.v[1],AppConfig.DefaultGateway.v[2],AppConfig.DefaultGateway.v[3],
			AppConfig.DefaultPrimaryDNSServer.v[0],AppConfig.DefaultPrimaryDNSServer.v[1],AppConfig.DefaultPrimaryDNSServer.v[2],
			AppConfig.DefaultPrimaryDNSServer.v[3],AppConfig.DefaultSecondaryDNSServer.v[0],AppConfig.DefaultSecondaryDNSServer.v[1],
			AppConfig.DefaultSecondaryDNSServer.v[2],AppConfig.DefaultSecondaryDNSServer.v[3],AppConfig.Flags.bIsDHCPReallyEnabled,
			AppConfig.VoterServerFQDN,AppConfig.VoterServerPort,AppConfig.DefaultPort,AppConfig.Password,AppConfig.HostPassword,
			AppConfig.TxBufferLength,AppConfig.GPSProto,AppConfig.GPSPolarity,AppConfig.PPSPolarity,AppConfig.GPSBaudRate,
			AppConfig.TelnetPort,AppConfig.TelnetUsername,AppConfig.TelnetPassword,AppConfig.DynDNSEnable,AppConfig.DynDNSUsername,
			AppConfig.DynDNSPassword,AppConfig.DynDNSHost,AppConfig.ExternalCTCSS,AppConfig.CORType,AppConfig.DebugLevel,
			AppConfig.BootIPAddr.v[0],AppConfig.BootIPAddr.v[1],AppConfig.BootIPAddr.v[2],AppConfig.BootIPAddr.v[3],
				(bootok) ? "OK" : "BAD");

		aborted = 0;
		while(!aborted)
		{
			printf(entsel);
			memset(cmdstr,0,sizeof(cmdstr));
			if (!myfgets(cmdstr,sizeof(cmdstr) - 1,stdin)) continue;
			if (!strchr(cmdstr,'!')) break;
		}
		if (aborted) continue;
		if ((strchr(cmdstr,'Q')) || strchr(cmdstr,'q'))
		{
				CloseTelnetConsole();
				continue;
		}
		if ((strchr(cmdstr,'R')) || strchr(cmdstr,'r'))
		{
				CloseTelnetConsole();
				printf(booting);
				Reset();
		}
		if ((strchr(cmdstr,'D')) || strchr(cmdstr,'d'))
		{
				DiagMenu();
				continue;
		}
		sel = atoi(cmdstr);
		if (((sel >= 1) && (sel <= 29)) || (sel == 11780))
		{
			printf(entnewval);
			if (aborted) continue;
			if ((!myfgets(cmdstr,sizeof(cmdstr) - 1,stdin)) || (strlen(cmdstr) < 2))
			{
				printf(newvalnotchanged);
				continue;
			}
		}
		ok = 0;
		switch(sel)
		{
			case 1: // Serial # (part of Mac Address)
				if (sscanf(cmdstr,"%u",&i1) == 1)
				{
					AppConfig.SerialNumber = i1;
					ok = 1;
				}
				break;
			case 2: // Default IP address
				if ((sscanf(cmdstr,"%d.%d.%d.%d",&i1,&i2,&i3,&i4) == 4) &&
					(i1 < 256) && (i2 < 256) && (i3 < 256) && (i4 < 256))
				{
					AppConfig.DefaultIPAddr.v[0] = i1;
					AppConfig.DefaultIPAddr.v[1] = i2;
					AppConfig.DefaultIPAddr.v[2] = i3;
					AppConfig.DefaultIPAddr.v[3] = i4;
					ok = 1;
				}
				break;
			case 3: // Default Netmask
				if ((sscanf(cmdstr,"%d.%d.%d.%d",&i1,&i2,&i3,&i4) == 4) &&
					(i1 < 256) && (i2 < 256) && (i3 < 256) && (i4 < 256))
				{
					AppConfig.DefaultMask.v[0] = i1;
					AppConfig.DefaultMask.v[1] = i2;
					AppConfig.DefaultMask.v[2] = i3;
					AppConfig.DefaultMask.v[3] = i4;
					ok = 1;
				}
				break;
			case 4: // Default Gateway
				if ((sscanf(cmdstr,"%d.%d.%d.%d",&i1,&i2,&i3,&i4) == 4) &&
					(i1 < 256) && (i2 < 256) && (i3 < 256) && (i4 < 256))
				{
					AppConfig.DefaultGateway.v[0] = i1;
					AppConfig.DefaultGateway.v[1] = i2;
					AppConfig.DefaultGateway.v[2] = i3;
					AppConfig.DefaultGateway.v[3] = i4;
					ok = 1;
				}
				break;
			case 5: // Default Primary DNS
				if ((sscanf(cmdstr,"%d.%d.%d.%d",&i1,&i2,&i3,&i4) == 4) &&
					(i1 < 256) && (i2 < 256) && (i3 < 256) && (i4 < 256))
				{
					AppConfig.DefaultPrimaryDNSServer.v[0] = i1;
					AppConfig.DefaultPrimaryDNSServer.v[1] = i2;
					AppConfig.DefaultPrimaryDNSServer.v[2] = i3;
					AppConfig.DefaultPrimaryDNSServer.v[3] = i4;
					ok = 1;
				}
				break;
			case 6: // Default Secondary DNS
				if ((sscanf(cmdstr,"%d.%d.%d.%d",&i1,&i2,&i3,&i4) == 4) &&
					(i1 < 256) && (i2 < 256) && (i3 < 256) && (i4 < 256))
				{
					AppConfig.DefaultSecondaryDNSServer.v[0] = i1;
					AppConfig.DefaultSecondaryDNSServer.v[1] = i2;
					AppConfig.DefaultSecondaryDNSServer.v[2] = i3;
					AppConfig.DefaultSecondaryDNSServer.v[3] = i4;
					ok = 1;
				}
				break;
			case 7: // DHCP Enable
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 < 2))
				{
					AppConfig.Flags.bIsDHCPReallyEnabled = i1;
					ok = 1;
				}
				break;
			case 8: // VOTER Server FQDN
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.VoterServerFQDN)))
				{
					cmdstr[x - 1] = 0;
					strcpy(AppConfig.VoterServerFQDN,cmdstr);
					ok = 1;
				}
				break;
			case 9: // Voter Server PORT
				if (sscanf(cmdstr,"%u",&i1) == 1)
				{
					AppConfig.VoterServerPort = i1;
					ok = 1;
				}
				break;
			case 10: // My Default Port
				if (sscanf(cmdstr,"%u",&i1) == 1)
				{
					AppConfig.DefaultPort = i1;
					ok = 1;
				}
				break;
			case 11: // VOTER Client Password
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.Password)))
				{
					cmdstr[x - 1] = 0;
					strcpy(AppConfig.Password,cmdstr);
					ok = 1;
				}
				break;
			case 12: // VOTER Server Password
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.HostPassword)))
				{
					cmdstr[x - 1] = 0;
					strcpy(AppConfig.HostPassword,cmdstr);
					ok = 1;
				}
				break;
			case 13: // Tx Buffer Length
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 >= 800) && (i1 <= MAX_BUFLEN))
				{
					AppConfig.TxBufferLength = i1;
					ok = 1;
				}
				break;
			case 14: // GPS Type
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 >= GPS_NMEA) && (i1 <= GPS_TSIP))
				{
					if ((AppConfig.GPSProto != GPS_TSIP) && 
						(i1 == GPS_TSIP))
					{
						AppConfig.GPSBaudRate = 9600;
						AppConfig.PPSPolarity = 0;
						AppConfig.GPSPolarity = 0;
					}
					if ((AppConfig.GPSProto == GPS_TSIP) && 
						(i1 != GPS_TSIP))
					{
						AppConfig.GPSBaudRate = 4800;
						AppConfig.PPSPolarity = 0;
						AppConfig.GPSPolarity = 0;
					}
					AppConfig.GPSProto = i1;
					ok = 1;
				}
				break;
			case 15: // GPS Invert
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 < 2))
				{
					AppConfig.GPSPolarity = i1;
					ok = 1;
				}
				break;
			case 16: // PPS Invert
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 < 3))
				{
					AppConfig.PPSPolarity = i1;
					ok = 1;
				}
				break;
			case 17: // GPS Baud Rate
				if ((sscanf(cmdstr,"%lu",&l) == 1) && (l >= 300L) && (l <= 230400L))
				{
					AppConfig.GPSBaudRate = l;
					ok = 1;
				}
				break;
			case 19: // Telnet Port
				if (sscanf(cmdstr,"%u",&i1) == 1) 
				{
					AppConfig.TelnetPort = i1;
					ok = 1;
				}
				break;
			case 20: // Telnet Username
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.TelnetUsername)))
				{
					cmdstr[x - 1] = 0;
					strcpy((char *)AppConfig.TelnetUsername,cmdstr);
					ok = 1;
				}
				break;
			case 21: // Telnet Password
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.TelnetPassword)))
				{
					cmdstr[x - 1] = 0;
					strcpy((char *)AppConfig.TelnetPassword,cmdstr);
					ok = 1;
				}
				break;
			case 22: // DynDNS Enable
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 < 2))
				{
					AppConfig.DynDNSEnable = i1;
					ok = 1;
					SetDynDNS();
				}
				break;
			case 23: // DynDNS Username
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.DynDNSUsername)))
				{
					cmdstr[x - 1] = 0;
					strcpy((char *)AppConfig.DynDNSUsername,cmdstr);
					ok = 1;
					SetDynDNS();
				}
				break;
			case 24: // DynDNS Password
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.DynDNSPassword)))
				{
					cmdstr[x - 1] = 0;
					strcpy((char *)AppConfig.DynDNSPassword,cmdstr);
					ok = 1;
					SetDynDNS();
				}
				break;
			case 25: // DynDNS Host
				x = strlen(cmdstr);
				if ((x > 2) && (x < sizeof(AppConfig.DynDNSHost)))
				{
					cmdstr[x - 1] = 0;
					strcpy((char *)AppConfig.DynDNSHost,cmdstr);
					ok = 1;
					SetDynDNS();
				}
				break;
			case 26: // EXT CTCSS
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 2))
				{
					AppConfig.ExternalCTCSS = i1;
					ok = 1;
				}
				break;
			case 27: // COR Type
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 2))
				{
					AppConfig.CORType = i1;
					ok = 1;
				}
				break;
			case 28: // Debug Level
				if ((sscanf(cmdstr,"%u",&i1) == 1) && (i1 <= 10000))
				{
				//	AppConfig.DebugLevel = i1;
					SetTxTone(i1);
					ok = 1;
				}
				break;
			case 29: // BootLoader IP Address
				if ((sscanf(cmdstr,"%d.%d.%d.%d",&i1,&i2,&i3,&i4) == 4) &&
					(i1 < 256) && (i2 < 256) && (i3 < 256) && (i4 < 256))
				{
					AppConfig.BootIPAddr.v[0] = i1;
					AppConfig.BootIPAddr.v[1] = i2;
					AppConfig.BootIPAddr.v[2] = i3;
					AppConfig.BootIPAddr.v[3] = i4;
					AppConfig.BootIPCheck = GetBootCS();
					ok = 1;
				}
				break;
#ifdef	DUMPENCREGS
			case 96:
				DumpETHReg();
 				printf(paktc);
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				continue;
#endif
			case 97: // Display RX Level Quasi-Graphically  
			 	putchar(' ');
		      	for(i = 0; i < NCOLS; i++) putchar(' ');
		        printf(rxvoicestr);
				indisplay = 1;
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				indisplay = 0;
				continue;
			case 98:
				t = system_time.vtime_sec;
				printf(oprdata,AppConfig.MyIPAddr.v[0],AppConfig.MyIPAddr.v[1],AppConfig.MyIPAddr.v[2],AppConfig.MyIPAddr.v[3],
					AppConfig.MyMask.v[0],AppConfig.MyMask.v[1],AppConfig.MyMask.v[2],AppConfig.MyMask.v[3],
					AppConfig.MyGateway.v[0],AppConfig.MyGateway.v[1],AppConfig.MyGateway.v[2],AppConfig.MyGateway.v[3],
					AppConfig.PrimaryDNSServer.v[0],AppConfig.PrimaryDNSServer.v[1],AppConfig.PrimaryDNSServer.v[2],AppConfig.PrimaryDNSServer.v[3],
					AppConfig.SecondaryDNSServer.v[0],AppConfig.SecondaryDNSServer.v[1],AppConfig.SecondaryDNSServer.v[2],AppConfig.SecondaryDNSServer.v[3],
					AppConfig.Flags.bIsDHCPEnabled,CurVoterAddr.v[0],CurVoterAddr.v[1],CurVoterAddr.v[2],CurVoterAddr.v[3],
					AppConfig.VoterServerPort,AppConfig.MyPort,gpssync,connected,lastcor,CTCSSIN ? 1 : 0,ptt,rssi,last_samplecnt,apeak,
					AppConfig.SqlNoiseGain,AppConfig.SqlDiode,adcothers[ADCSQPOT]);
				strftime(cmdstr,sizeof(cmdstr) - 1,"%a  %b %d, %Y  %H:%M:%S",gmtime(&t));
				if (gpssync && connected) printf(curtimeis,cmdstr,(unsigned long)system_time.vtime_nsec/1000000L);
				printf(paktc);
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				continue;
			case 99:
				SaveAppConfig();
				printf(saved);
				continue;
			case 11780: // GPS Baud Rate
				if ((sscanf(cmdstr,"%lu",&l) == 1) && (l >=0))
				{
					AppConfig.Elkes = l * 9677ul;
					ok = 1;
				}
				break;
			default:
				printf(invalselection);
				continue;
		}
		if (ok) printf(newvalchanged);
		else printf(newvalerror);
	}
}


/****************************************************************************
  Function:
    static void InitializeBoard(void)

  Description:
    This routine initializes the hardware.  It is a generic initialization
    routine for many of the Microchip development boards, using definitions
    in HardwareProfile.h to determine specific initialization.

  Precondition:
    None

  Parameters:
    None - None

  Returns:
    None

  Remarks:
    None
  ***************************************************************************/
static void InitializeBoard(void)
{	

	// Crank up the core frequency Fosc = 76.8MHz, Fcy = 38.4MHz
	PLLFBD = 30;				// Multiply by 32 for 153.6MHz VCO output (9.6MHz XT oscillator)
	CLKDIV = 0x0000;			// FRC: divide by 2, PLLPOST: divide by 2, PLLPRE: divide by 2
	ACLKCON = 0x780;


//_________________________________________________________________
	DISABLE_INTERRUPTS();

	T3CON = 0x10;			//TMR3 divide Fosc by 8
	PR3 = 299;				//Set to period of 300
	T3CONbits.TON = 1;		//Turn it on

#if defined(SMT_BOARD)
	AD1PCFGL = 0xFFF0;				// Enable AN0-AN3 as analog 
#else
	AD1PCFGL = 0xFFE2;				// Enable AN0, AN2-AN4 as analog 
#endif

	AD1CON1 = 0x8444;			// TMR Sample Start, 12 bit mode (on parts with a 12bit A/D)
	AD1CON2 = 0x0;			// AVdd, AVss, int every conversion, MUXA only, no scan
	AD1CON3 = 0x1003;			// 16 Tad auto-sample, Tad = 3*Tcy
	AD1CON1bits.ADON = 1;		// Turn On

	IFS0bits.AD1IF = 0;
	IEC0bits.AD1IE = 1;
	IPC3bits.AD1IP = 6;

	DAC1DFLT = 0;
	DAC1STAT = 0x8000;
	DAC1STATbits.LITYPE = 0;
	DAC1CON = 0x1100 + 74;		// Divide by 75 for 8K Samples/sec
	IFS4bits.DAC1LIF = 0;
	IEC4bits.DAC1LIE = 1;
	IPC19bits.DAC1LIP = 6;
	DAC1CONbits.DACEN = 1;

#if defined(SMT_BOARD)

	PORTA=0;	
	PORTB=0x3c00;
	PORTC=7;	
	// RA4 is CN0/PPS Pulse, RA7-CTCSS, RA8-RA10 jumpers */
	TRISA = 0xFFFF;	 
	//RB0-1 are Analog, RB2-3 are audio select, RB4 is PTT, RB5-6 are Programming Pins, RB7 is INT0 (Ethenet INT)
	//RB8 is Test Pin, RB9 is JP11, RB10-13 are LEDs, RB14-15 are DAC outputs
	TRISB = 0x02E3;	
	//RC0-RC2 are CS pins, RC4 is RP20/SDO, RC5 is RP21/SCK, RC7 is RP23/U1TX, RC9 is RP25/U2TX
	TRISC = 0xFD48;

	__builtin_write_OSCCONL(OSCCON & ~0x40); //clear the bit 6 of OSCCONL to unlock pin re-map
	_U1RXR = 22;	// RP22 is UART1 RX
	_RP23R = 3;		// RP23 is UART1 TX (U1TX) 3
	_U2RXR = 24;	// RP24 is UART2 RX
	_RP25R = 5;		// RP25 is UART2 TX (U2TX) 5 
	_SDI1R = 19;	// RP19 is SPI1 MISO
	_RP21R = 8;		// RP21 is SPI1 CLK (SCK1OUT) 8
	_RP20R = 7;		// RP20 is SPI1 MOSI (SDO1) 7
	__builtin_write_OSCCONL(OSCCON | 0x40); //set the bit 6 of OSCCONL to lock pin re-map

#else

	PORTA=0;	//Initialize LED pin data to off state
	PORTB=0;	//Initialize LED pin data to off state
	// RA4 is CN0/PPS Pulse
	TRISA = 0xFFFD;	 //RA1 is Test Bit
	//RB0-2 are Analog, RB3-4 are SPI select, RB5-6 are Programming pins, RB7 is INT0 (Ethenet INT), RB8 is RP8/SCK,
	//RB9 is RP9/SDO, RB10 is RP10/SDI, RB11 is RP11/U1TX, RB12 is RP12/U1RX, RB13 is RP13/U2RX, RB14-15 are DAC outputs
	TRISB = 0x3487;	

	__builtin_write_OSCCONL(OSCCON & ~0x40); //clear the bit 6 of OSCCONL to unlock pin re-map
	_U1RXR = 12;	// RP12 is UART1 RX
	_RP11R = 3;		// RP11 is UART1 TX (U1TX)
	_U2RXR = 13;	// RP13 is UART2 RX
	_SDI1R = 10;	// RP10 is SPI1 MISO
	_RP8R = 8;		// RP8 is SPI1 CLK (SCK1OUT) 8
	_RP9R = 7;		// RP9 is SPI1 MOSI (SDO1) 7
	__builtin_write_OSCCONL(OSCCON | 0x40); //set the bit 6 of OSCCONL to lock pin re-map

	IOExpInit();

#endif

#if defined(SPIRAM_CS_TRIS)
	SPIRAMInit();
#endif
#if defined(EEPROM_CS_TRIS)
	XEEInit();
#endif
#if defined(SPIFLASH_CS_TRIS)
	SPIFlashInit();
#endif

	CNEN1bits.CN0IE = 1;
	IEC1bits.CNIE = 1;
	IPC4bits.CNIP = 7;

	INTCON1bits.NSTDIS = 1;

	ENABLE_INTERRUPTS();

}

/*********************************************************************
 * Function:        void InitAppConfig(void)
 *
 * PreCondition:    MPFSInit() is already called.
 *
 * Input:           None
 *
 * Output:          Write/Read non-volatile config variables.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
// MAC Address Serialization using a MPLAB PM3 Programmer and 
// Serialized Quick Turn Programming (SQTP). 
// The advantage of using SQTP for programming the MAC Address is it
// allows you to auto-increment the MAC address without recompiling 
// the code for each unit.  To use SQTP, the MAC address must be fixed
// at a specific location in program memory.  Uncomment these two pragmas
// that locate the MAC address at 0x1FFF0.  Syntax below is for MPLAB C 
// Compiler for PIC18 MCUs. Syntax will vary for other compilers.
//#pragma romdata MACROM=0x1FFF0
static ROM BYTE SerializedMACAddress[6] = {MY_DEFAULT_MAC_BYTE1, MY_DEFAULT_MAC_BYTE2, MY_DEFAULT_MAC_BYTE3, MY_DEFAULT_MAC_BYTE4, MY_DEFAULT_MAC_BYTE5, MY_DEFAULT_MAC_BYTE6};
//#pragma romdata

static void InitAppConfig(void)
{
	memset(&AppConfig,0,sizeof(AppConfig));
	AppConfig.Flags.bIsDHCPEnabled = TRUE;
	AppConfig.Flags.bIsDHCPReallyEnabled = FALSE;
	AppConfig.Flags.bInConfigMode = TRUE;
	memcpypgm2ram((void*)&AppConfig.MyMACAddr, (ROM void*)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));
	AppConfig.SerialNumber = 1234;
	AppConfig.DefaultIPAddr.v[0] = 192;
	AppConfig.DefaultIPAddr.v[1] = 168;
	AppConfig.DefaultIPAddr.v[2] = 1;
	AppConfig.DefaultIPAddr.v[3] = 10;
	AppConfig.BootIPAddr.v[0] = 192;
	AppConfig.BootIPAddr.v[1] = 168;
	AppConfig.BootIPAddr.v[2] = 1;
	AppConfig.BootIPAddr.v[3] = 11;
	AppConfig.BootIPCheck = GetBootCS();
	AppConfig.DefaultMask.v[0] = 255;
	AppConfig.DefaultMask.v[1] = 255;
	AppConfig.DefaultMask.v[2] = 255;
	AppConfig.DefaultMask.v[3] = 0;
	AppConfig.DefaultGateway.v[0] = 192;
	AppConfig.DefaultGateway.v[1] = 168;
	AppConfig.DefaultGateway.v[2] = 1;
	AppConfig.DefaultGateway.v[3] = 1;
	AppConfig.DefaultPrimaryDNSServer.v[0] = 192;
	AppConfig.DefaultPrimaryDNSServer.v[1] = 168;
	AppConfig.DefaultPrimaryDNSServer.v[2] = 1;
	AppConfig.DefaultPrimaryDNSServer.v[3] = 1;
	AppConfig.DefaultSecondaryDNSServer.v[0] = 0;
	AppConfig.DefaultSecondaryDNSServer.v[1] = 0;
	AppConfig.DefaultSecondaryDNSServer.v[2] = 0;
	AppConfig.DefaultSecondaryDNSServer.v[3] = 0;

	AppConfig.TxBufferLength = DEFAULT_TX_BUFFER_LENGTH;
	AppConfig.VoterServerPort = 667;
	AppConfig.GPSBaudRate = 4800;
	strcpy(AppConfig.Password,"radios");
	strcpy(AppConfig.HostPassword,"BLAH");
	strcpy(AppConfig.VoterServerFQDN,"devel.lambdatel.com");
	AppConfig.TelnetPort = 23;
	strcpy((char *)AppConfig.TelnetUsername,"admin");
	strcpy((char *)AppConfig.TelnetPassword,"radios");
	AppConfig.DynDNSEnable = 0;
	strcpy((char *)AppConfig.DynDNSUsername,"wb6nil");
	strcpy((char *)AppConfig.DynDNSPassword,"radios42");
	strcpy((char *)AppConfig.DynDNSHost,"voter-test.dyndns.org");


	#if defined(EEPROM_CS_TRIS)
	{
		BYTE c;
		
	    // When a record is saved, first byte is written as 0x60 to indicate
	    // that a valid record was saved.  Note that older stack versions
		// used 0x57.  This change has been made to so old EEPROM contents
		// will get overwritten.  The AppConfig() structure has been changed,
		// resulting in parameter misalignment if still using old EEPROM
		// contents.
		XEEReadArray(0x0000, &c, 1);/*SaveAppConfig(); */
	    if(c == 0x60u)
		    XEEReadArray(0x0001, (BYTE*)&AppConfig, sizeof(AppConfig));
	    else
	        SaveAppConfig();
	}
	#elif defined(SPIFLASH_CS_TRIS)
	{
		BYTE c;
		
		SPIFlashReadArray(0x0000, &c, 1);
		if(c == 0x60u)
			SPIFlashReadArray(0x0001, (BYTE*)&AppConfig, sizeof(AppConfig));
		else
			SaveAppConfig();
	}
	#endif
	AppConfig.MyIPAddr = AppConfig.DefaultIPAddr;
	AppConfig.MyMask = AppConfig.DefaultMask;
	AppConfig.MyGateway = AppConfig.DefaultGateway;
	AppConfig.PrimaryDNSServer = AppConfig.DefaultPrimaryDNSServer;
	AppConfig.SecondaryDNSServer = AppConfig.DefaultSecondaryDNSServer;
	AppConfig.MyPort = AppConfig.VoterServerPort;
	if (AppConfig.DefaultPort) AppConfig.MyPort = AppConfig.DefaultPort;
	AppConfig.MyMACAddr.v[5] = AppConfig.SerialNumber & 0xff;
	AppConfig.MyMACAddr.v[4] = AppConfig.SerialNumber >> 8;
	AppConfig.Flags.bIsDHCPEnabled = TRUE;
}

#if defined(EEPROM_CS_TRIS) || defined(SPIFLASH_CS_TRIS)
void SaveAppConfig(void)
{

	#if defined(EEPROM_CS_TRIS)
	    XEEBeginWrite(0x0000);
	    XEEWrite(0x60);
	    XEEWriteArray((BYTE*)&AppConfig, sizeof(AppConfig));
    #else
	    SPIFlashBeginWrite(0x0000);
	    SPIFlashWrite(0x60);
	    SPIFlashWriteArray((BYTE*)&AppConfig, sizeof(AppConfig));
    #endif
}
#endif
