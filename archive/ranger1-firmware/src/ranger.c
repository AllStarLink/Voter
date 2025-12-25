#include <stdio.h>
#include <errno.h>
#include <p18f26j50.h>

#pragma config WDTEN = OFF,PLLDIV = 2, STVREN = ON, XINST = OFF, CPUDIV = OSC1, CP0 = OFF
#pragma config OSC = INTOSCPLLO, T1DIG = OFF, LPT1OSC = OFF, FCMEN = OFF, IESO = OFF

// Include all headers for any enabled TCPIP Stack functions
#include "TCPIP Stack/TCPIP.h"

#define SYSLED LATAbits.LATA1
#define RXLED LATAbits.LATA2
#define HOSTLED LATAbits.LATA3
#define FOOLED LATAbits.LATA5
#define PTT LATCbits.LATC6
#define	COR PORTCbits.RC4
#define TESTBIT FOOLED

#define	BAUD_RATE 19200
#define	DEFAULT_TX_BUFFER_LENGTH 1500 // approx 300ms of Buffer
#define	VOTER_CHALLENGE_LEN 10
#define	MAX_BUFLEN 1920 // 0.5 seconds of buffer or so
#define	NCOLS 75
#define	FRAME_SIZE 160
#define GPS_FORCE_TIME (1500 * 4)  // Force a GPS (Keepalive) every 1500ms regardless
#define ATTEMPT_TIME (500 * 4) // Try connection every 500 ms
#define LASTRX_TIME (3000 * 4) // Try connection every 500 ms
#define	MULAW_SILENCE 0xff

#define OPTION_FLAG_FLATAUDIO 1	// Send Flat Audio
#define	OPTION_FLAG_SENDALWAYS 2	// Send Audio always
#define OPTION_FLAG_NOCTCSSFILTER 4 // Do not filter CTCSS
#define	OPTION_FLAG_MASTERTIMING 8  // Master Timing Source (do not delay sending audio packet)
#define	OPTION_FLAG_ADPCM 16 // Use ADPCM rather then ULAW
#define	OPTION_FLAG_MIX 32 // Request "Mix" option to host

#define ARPIsTxReady()      MACIsTxReady() 

#define	TMR1VAL (0xffff - 11305)    //5307  11833


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

#define	ARGH const far rom char *

BYTE filling_buffer;
WORD fillindex;
BOOL filled;
BOOL txreal;
#pragma udata udata12
static struct {
	VOTER_PACKET_HEADER vph;
	BYTE rssi;
	BYTE audio[FRAME_SIZE];
} audio_packet;
#pragma udata udata10
BYTE audio_buf0[FRAME_SIZE];
#pragma udata udata11
BYTE audio_buf1[FRAME_SIZE];
#pragma udata udata
WORD txdrainindex;
WORD last_drainindex;
#pragma udata udata0=0x500
BYTE txaudio0[256];
#pragma udata udata1=0x600
BYTE txaudio1[256];
#pragma udata udata2=0x700
BYTE txaudio2[256];
#pragma udata udata3=0x800
BYTE txaudio3[256];
#pragma udata udata4=0x900
BYTE txaudio4[256];
#pragma udata udata5=0xa00
BYTE txaudio5[256];
#pragma udata udata6=0xb00
BYTE txaudio6[256];
#pragma udata udata7=0xc00
BYTE txaudio7[128];
#pragma udata udata
static struct {
	VOTER_PACKET_HEADER vph;
	char lat[9];
	char lon[10];
	char elev[7];
} gps_packet;
BOOL connected;
BYTE inread;
BYTE inprocloop;
BYTE ininput;
BYTE indisplay;
BYTE aborted;
DWORD dwLastIP;
UDP_SOCKET udpSocketUser;
NODE_INFO udpServerNode;
IP_ADDR MyVoterAddr;
IP_ADDR LastVoterAddr;
IP_ADDR CurVoterAddr;
WORD dnstimer;
BOOL dnsdone;
BYTE dnsnotify;
WORD midx;
WORD gpsforcetimer;
WORD attempttimer;
WORD lastrxtimer;
long host_txseqno;
long txseqno_ptt;
long txseqno;
DWORD digest;
DWORD resp_digest;
DWORD mydigest;
BOOL badmix;
BYTE option_flags;
BYTE lastval;

char their_challenge[VOTER_CHALLENGE_LEN],challenge[VOTER_CHALLENGE_LEN];

ROM BYTE ulawtable[] = {
1,1,1,1,1,1,1,1,
1,1,1,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,3,3,3,3,
3,3,3,3,3,3,3,3,
3,3,3,3,3,4,4,4,
4,4,4,4,4,4,4,4,
4,4,4,4,4,5,5,5,
5,5,5,5,5,5,5,5,
5,5,5,5,5,5,6,6,
6,6,6,6,6,6,6,6,
6,6,6,6,6,6,6,7,
7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,
8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,
8,9,9,9,9,9,9,9,
9,9,9,9,9,9,9,9,
9,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,
10,10,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,
11,11,11,12,12,12,12,12,
12,12,12,12,12,12,12,12,
12,12,12,12,13,13,13,13,
13,13,13,13,13,13,13,13,
13,13,13,13,13,14,14,14,
14,14,14,14,14,14,14,14,
14,14,14,14,14,15,15,15,
15,15,15,15,15,15,15,15,
15,15,15,15,15,15,16,16,
16,16,16,16,16,16,17,17,
17,17,17,17,17,17,17,18,
18,18,18,18,18,18,18,19,
19,19,19,19,19,19,19,19,
20,20,20,20,20,20,20,20,
21,21,21,21,21,21,21,21,
22,22,22,22,22,22,22,22,
22,23,23,23,23,23,23,23,
23,24,24,24,24,24,24,24,
24,24,25,25,25,25,25,25,
25,25,26,26,26,26,26,26,
26,26,27,27,27,27,27,27,
27,27,27,28,28,28,28,28,
28,28,28,29,29,29,29,29,
29,29,29,29,30,30,30,30,
30,30,30,30,31,31,31,31,
31,31,31,31,32,32,32,32,
32,33,33,33,33,34,34,34,
34,35,35,35,35,36,36,36,
36,37,37,37,37,37,38,38,
38,38,39,39,39,39,40,40,
40,40,41,41,41,41,42,42,
42,42,42,43,43,43,43,44,
44,44,44,45,45,45,45,46,
46,46,46,47,47,47,47,47,
48,48,49,49,50,50,51,51,
52,52,53,53,54,54,55,55,
56,56,57,57,57,58,58,59,
59,60,60,61,61,62,62,63,
63,64,65,66,67,68,69,70,
71,72,72,73,74,75,76,77,
78,79,81,83,84,86,88,90,
92,94,96,100,104,108,112,119,
255,247,240,236,232,228,224,222,
220,218,216,214,212,211,209,207,
206,205,204,203,202,201,200,200,
199,198,197,196,195,194,193,192,
191,191,190,190,189,189,188,188,
187,187,186,186,185,185,185,184,
184,183,183,182,182,181,181,180,
180,179,179,178,178,177,177,176,
176,175,175,175,175,175,174,174,
174,174,173,173,173,173,172,172,
172,172,171,171,171,171,170,170,
170,170,170,169,169,169,169,168,
168,168,168,167,167,167,167,166,
166,166,166,165,165,165,165,165,
164,164,164,164,163,163,163,163,
162,162,162,162,161,161,161,161,
160,160,160,160,160,159,159,159,
159,159,159,159,159,158,158,158,
158,158,158,158,158,157,157,157,
157,157,157,157,157,157,156,156,
156,156,156,156,156,156,155,155,
155,155,155,155,155,155,155,154,
154,154,154,154,154,154,154,153,
153,153,153,153,153,153,153,152,
152,152,152,152,152,152,152,152,
151,151,151,151,151,151,151,151,
150,150,150,150,150,150,150,150,
150,149,149,149,149,149,149,149,
149,148,148,148,148,148,148,148,
148,147,147,147,147,147,147,147,
147,147,146,146,146,146,146,146,
146,146,145,145,145,145,145,145,
145,145,145,144,144,144,144,144,
144,144,144,143,143,143,143,143,
143,143,143,143,143,143,143,143,
143,143,143,143,142,142,142,142,
142,142,142,142,142,142,142,142,
142,142,142,142,141,141,141,141,
141,141,141,141,141,141,141,141,
141,141,141,141,141,140,140,140,
140,140,140,140,140,140,140,140,
140,140,140,140,140,140,139,139,
139,139,139,139,139,139,139,139,
139,139,139,139,139,139,139,138,
138,138,138,138,138,138,138,138,
138,138,138,138,138,138,138,138,
137,137,137,137,137,137,137,137,
137,137,137,137,137,137,137,137,
136,136,136,136,136,136,136,136,
136,136,136,136,136,136,136,136,
136,135,135,135,135,135,135,135,
135,135,135,135,135,135,135,135,
135,135,134,134,134,134,134,134,
134,134,134,134,134,134,134,134,
134,134,134,133,133,133,133,133,
133,133,133,133,133,133,133,133,
133,133,133,133,132,132,132,132,
132,132,132,132,132,132,132,132,
132,132,132,132,131,131,131,131,
131,131,131,131,131,131,131,131,
131,131,131,131,131,130,130,130,
130,130,130,130,130,130,130,130,
130,130,130,130,130,130,129,129,
129,129,129,129,129,129,129,129
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

#pragma udata udatacfg
// Declare AppConfig structure and some other supporting stack variables
APP_CONFIG AppConfig;
#pragma udata udata

/* Interrupt handler */
#pragma interrupt high_int
void high_int(void)
{
unsigned char x,y,encoded,sign;
unsigned short val;
signed short diff,s,step;
BYTE dummy;
extern volatile DWORD dwInternalTicks;

    if(INTCONbits.TMR0IF)
    {
		// Increment internal high tick counter
		dwInternalTicks++;

		// Reset interrupt flag
        INTCONbits.TMR0IF = 0;
    }
	/* TMR1 Interrupt */
	if (PIR1bits.TMR1IF && PIE1bits.TMR1IE)
	{
		BYTE x,y,y1,*cp;
		WORD val;
		short s,v;

		T1CONbits.TMR1ON = 0;
		TMR1L = TMR1VAL & 0xff;
		TMR1H = TMR1VAL >> 8;
		T1CONbits.TMR1ON = 1;
		ADCON0bits.GO = 1; 
		if (connected)
		{
			gpsforcetimer++;
			lastrxtimer++;
		}
		else attempttimer++;
		cp = txaudio0;
		cp += txdrainindex;

		v = ulawtabletx[*cp];
	
		if (v < 0) s = -v;
		else s = v;
		s = s >> 7;
		if (v < 0) s = -s;
		s += 128;
		val = s;

		*cp = MULAW_SILENCE;

		if (++txdrainindex >= AppConfig.TxBufferLength)
			txdrainindex = 0;
//		if (val != lastval)
		{
			lastval = val;
			// Set PWM accordingly
			val = val << 6;
			x = CCP1CON;
			x &= 0xcf;
			x |= (val & 0xff) >> 2;
			y = (val >> 8);
			CCP1CON = x;
			CCPR1L = y;
		}
		PIR1bits.TMR1IF = 0;
	}

	/* ADC Interrupt */
	if (PIR1bits.ADIF && PIE1bits.ADIE)
	{
		WORD index;
		short s;
		char v;
		BYTE nu;

		index = ((WORD)ADRESH << 8) + ADRESL;
		index &= 0x3ff;
		if (index >= 2) index -= 2;
		if (filling_buffer)
		{
			audio_buf1[fillindex] = ulawtable[index];
		}
		else
		{
			audio_buf0[fillindex] = ulawtable[index];
		}
		if (++fillindex >= FRAME_SIZE)
		{
			fillindex = 0;
			filling_buffer ^= 1;
			last_drainindex = txdrainindex;
			txseqno += 2;
			if (host_txseqno) host_txseqno += 2;
			filled = 1;
		}
		PIR1bits.ADIF = 0;
	}
}


/* Glue to make Interrupt work */
#pragma code HIGH_INTERRUPT_VECTOR = 0x8
void HighISR(void)
{
	_asm
	goto high_int
	_endasm
}
#pragma code

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

BYTE GetBootCS(void)
{
BYTE x = 0x69,i;

	for(i = 0; i < 4; i++) x += AppConfig.BootIPAddr.v[i];
	return(x);
}

BOOL HasCOR(void)
{
	if ((AppConfig.ExternalCTCSS == 0) && COR) return (1);
	if ((AppConfig.ExternalCTCSS == 1) && (!COR)) return (1);
	return(0);
}

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

void process_udp(UDP_SOCKET *udpSocketUser,NODE_INFO *udpServerNode)
{

	BYTE n,c,i,j,*cp;

	WORD mytxindex;
	long mytxseqno,myhost_txseqno;

	mytxindex = last_drainindex;
	mytxseqno = txseqno;
	myhost_txseqno = host_txseqno;

	if (filled)
	{
		BOOL tosend = (connected && (HasCOR() || (option_flags & OPTION_FLAG_SENDALWAYS)));

//TESTBIT ^= 1;
		filled = 0;
		if ((((!connected) && (attempttimer >= ATTEMPT_TIME)) || tosend) && UDPIsPutReady(*udpSocketUser)) {
			UDPSocketInfo[activeUDPSocket].remoteNode.MACAddr = udpServerNode->MACAddr;
			memset(&audio_packet,0,sizeof(VOTER_PACKET_HEADER));
			audio_packet.vph.curtime.vtime_sec = 0;
			audio_packet.vph.curtime.vtime_nsec = htonl(mytxseqno);
			strcpy((char *)audio_packet.vph.challenge,challenge);
			audio_packet.vph.digest = htonl(resp_digest);
			if (tosend) audio_packet.vph.payload_type = htons(4);
			i = 0;
			cp = (BYTE *) &audio_packet;
			for(i = 0; i < sizeof(VOTER_PACKET_HEADER); i++) UDPPut(*cp++);
            if (tosend)
			{
				if (HasCOR())
				{
					UDPPut(255);
					if (filling_buffer)
					{
						for(i = 0; i < FRAME_SIZE; i++) UDPPut(audio_buf0[i]);
					}
					else
					{
						for(i = 0; i < FRAME_SIZE; i++) UDPPut(audio_buf1[i]);
					}
				}
				else
				{
					UDPPut(0);
					for(i = 0; i < j; i++) UDPPut(MULAW_SILENCE);
				}
			}
			else UDPPut(OPTION_FLAG_MIX);
             //Send contents of transmit buffer, and free buffer
            UDPFlush();
			attempttimer = 0;
		}
	}
	if (UDPIsGetReady(*udpSocketUser)) {
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
						if (!(option_flags & OPTION_FLAG_MIX))
						{
							badmix = 1;
							connected = 0;
							txseqno = 0;
							txseqno_ptt = 0;
							digest = 0;
							lastrxtimer = 0;
						}
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
					if (ntohs(audio_packet.vph.payload_type) == 4) 
					{
						long index;
						short mydiff;


						mytxseqno = txseqno;
						if (mytxseqno > (txseqno_ptt + 2))
								host_txseqno = 0;
						txseqno_ptt = mytxseqno;
						if (!host_txseqno) myhost_txseqno = host_txseqno = ntohl(audio_packet.vph.curtime.vtime_nsec);
						index = (ntohl(audio_packet.vph.curtime.vtime_nsec) - myhost_txseqno);
						index *= FRAME_SIZE;
						index -= (FRAME_SIZE * 2);
						index += AppConfig.TxBufferLength - (FRAME_SIZE * 2);


                        /* if in bounds */
                        if ((index > 0) && (index <= (AppConfig.TxBufferLength - (FRAME_SIZE * 2))))
                        {
							BYTE *cp,*cp1;
							long ind1;

							ind1 = index / FRAME_SIZE;
							if ((txseqno + ind1) > txseqno_ptt) 
								txseqno_ptt = (txseqno + ind1);

			                index += mytxindex;
					   		if (index >= AppConfig.TxBufferLength) index -= AppConfig.TxBufferLength;
							mydiff = AppConfig.TxBufferLength;
							mydiff -= ((short)index + FRAME_SIZE);
							cp = txaudio0;
							cp += index;
                            if (mydiff >= 0)
                            {
                                    memcpy((void *)cp,(void *)audio_packet.audio,FRAME_SIZE);
                            }
                            else
                            {
                                    memcpy((void *)cp,(void *)audio_packet.audio,FRAME_SIZE + mydiff);
									cp = txaudio0;
									cp1 = audio_packet.audio;
									cp1 += FRAME_SIZE + mydiff;
                                    memcpy((void *)cp,(void *)cp1,-mydiff);
                            }
						}
						else 
						{
							TESTBIT ^= 1;
							host_txseqno = 0;
						}
					}
				}
			}
		}
		UDPDiscard();
	}
	if (connected && (gpsforcetimer >= GPS_FORCE_TIME))
	{
	        if (UDPIsPutReady(*udpSocketUser)) {
				UDPSocketInfo[activeUDPSocket].remoteNode.MACAddr = udpServerNode->MACAddr;
				gps_packet.vph.curtime.vtime_sec = 0;
				gps_packet.vph.curtime.vtime_nsec = htonl(0);
				strcpy((char *)gps_packet.vph.challenge,challenge);
				gps_packet.vph.digest = htonl(resp_digest);
				gps_packet.vph.payload_type = htons(2);
				cp = (BYTE *) &gps_packet.vph;
				for(i = 0; i < sizeof(gps_packet.vph); i++) UDPPut(*cp++);
	            UDPFlush();
				gpsforcetimer = 0;
	         }
	}
}


void main_processing_loop(void)
{
	static DWORD t1 = 0,tsecWait = 0;
	//UDP State machine
	#define SM_UDP_SEND_ARP     0
	#define SM_UDP_WAIT_RESOLVE 1
	#define SM_UDP_RESOLVED     2

	static BYTE smUdp = SM_UDP_SEND_ARP;

	inprocloop = 1;

	if ((!AppConfig.Flags.bIsDHCPEnabled) || (!AppConfig.Flags.bInConfigMode))
	{

		if (AppConfig.VoterFQDN[0])
		{
			if (!dnstimer)
			{
				DNSEndUsage();
				dnstimer = 120;
				dnsdone = 0;
				if(DNSBeginUsage()) 
					DNSResolve((BYTE *)AppConfig.VoterFQDN,DNS_TYPE_A);
			}
			else
			{

				if ((!dnsdone) && (DNSIsResolved(&MyVoterAddr)))
				{
					if (DNSEndUsage())
					{
						if (memcmp((const void *)&LastVoterAddr,(const void *)&MyVoterAddr,sizeof(IP_ADDR)))
						{

							if (udpSocketUser != INVALID_UDP_SOCKET) UDPClose(udpSocketUser);
	   						memset(&udpServerNode, 0,sizeof(udpServerNode));
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
	}

	if (lastrxtimer >= LASTRX_TIME)
	{
		connected = 0;
		txseqno = 0;
		txseqno_ptt = 0;
		digest = 0;
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

  	// Blink LEDs as appropriate
    if((TickGet() - t1) >= (TICK_SECOND / 2ul))
    {
		t1 = TickGet();
		SYSLED ^= 1;
	}

	HOSTLED = !connected;
	RXLED = !HasCOR();

    // This task performs normal stack task including checking
    // for incoming packet, type of packet and calling
    // appropriate stack entity to process it.
    StackTask();
    
    // This tasks invokes each of the core stack application tasks
    StackApplications();

	if ((PTT) &&(txseqno > (txseqno_ptt + 2)))
	{
		PTT = 0;
		RPOR13 = 0;
		TRISCbits.TRISC2 = 1;
	}
	else if ((!PTT) && txseqno_ptt && (txseqno <= (txseqno_ptt + 2)))
	{
		RPOR13 = 14;
		TRISCbits.TRISC2 = 0;
		PTT = 1;
	}
	inprocloop = 0;
}


void putsUART(char *buffer)
{
	while(*buffer) WriteUART(*buffer++);
}

void putrsUART(ROM char *buffer)
{
	while(*buffer) WriteUART(*buffer++);
}

unsigned int getsUART(unsigned int length,char *buffer,
                       unsigned int uart_data_wait)

{
    unsigned int wait = 0;
    while(length)                         /* read till length is 0 */
    {
        while(!DataRdyUART())
        {
            if(wait < uart_data_wait)
                wait++ ;                  /*wait for more data */
            else
                return(length);           /*Time out- Return words/bytes to be read */
        }
        wait=0;
		*buffer++ = ReadUART();
        length--;
    }
    return(length);                       /* number of data yet to be received i.e.,0 */
}


char DataRdyUART(void)
{
	if(RCSTA2bits.OERR)
	{
		RCSTA2bits.CREN = 0;
		RCSTA2bits.CREN = 1;
	}
	return(PIR3bits.RC2IF);
}

char BusyUART(void)
{  
	return(!TXSTA2bits.TRMT);
}


BYTE ReadUART(void)
{
	PIR3bits.RC2IF = 0;
	return(RCREG2);
}


void WriteUART(BYTE data)
{
	while(BusyUART());
	TXREG2 = data;
}


void _user_putc(auto char c)
{
	if (c == '\n') WriteUART('\r');
	WriteUART(c);
}

BYTE mygets(char *dest,BYTE len)
{
BYTE	c,x,count;

	inread = 1;
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
			if (!inprocloop) main_processing_loop();

		    // If the local IP address has changed (ex: due to DHCP lease change)
		    // write the new IP address to the LCD display, UART, and Announce 
		    // service
			if(dwLastIP != AppConfig.MyIPAddr.Val)
			{
				dwLastIP = AppConfig.MyIPAddr.Val;
		
				
				printf((ARGH)"\nIP Configuration Info: \n");
				if (AppConfig.Flags.bIsDHCPReallyEnabled)
					printf((ARGH)"Configured With DHCP\n");
				else
					printf((ARGH)"Static IP Configuration\n");
				printf((ARGH)"IP Address: %d.%d.%d.%d\n",AppConfig.MyIPAddr.v[0],AppConfig.MyIPAddr.v[1],AppConfig.MyIPAddr.v[2],AppConfig.MyIPAddr.v[3]);
				printf((ARGH)"Subnet Mask: %d.%d.%d.%d\n",AppConfig.MyMask.v[0],AppConfig.MyMask.v[1],AppConfig.MyMask.v[2],AppConfig.MyMask.v[3]);
				printf((ARGH)"Gateway Addr: %d.%d.%d.%d\n",AppConfig.MyGateway.v[0],AppConfig.MyGateway.v[1],AppConfig.MyGateway.v[2],AppConfig.MyGateway.v[3]);
		
				#if defined(STACK_USE_ANNOUNCE)
					AnnounceIP();
				#endif
			}
			if (dnsnotify == 1)
			{
				printf((ARGH)"Voter Host DNS Resolved to %d.%d.%d.%d\n",MyVoterAddr.v[0],MyVoterAddr.v[1],MyVoterAddr.v[2],MyVoterAddr.v[3]);
			}
			else if (dnsnotify == 2) 
			{
				printf((ARGH)"Warning: Unable to resolve DNS for Voter Host %s\n",AppConfig.VoterFQDN);
			}
			dnsnotify = 0;
			if (badmix)
			{
				printf((ARGH)"ERROR! Host not acknowledging non-GPS disciplined operation\n");
				badmix = 0;
			}
		}
		if (c == 3) 
		{
			while(BusyUART()) if (!inprocloop) main_processing_loop();
			WriteUART('^');
			while(BusyUART()) if (!inprocloop) main_processing_loop();
			WriteUART('C');
			aborted = 1;
			dest[0] = 0;
			count = 0;
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
				while(BusyUART()) main_processing_loop();
				WriteUART(' ');
				while(BusyUART()) main_processing_loop();
				WriteUART(8);
			}
			continue;
		}
		if ((c != '\r') && (c < ' ')) continue;
		if (c > 126) continue;
		if (c == '\r') c = '\n';
		if (c == '\n') break;
		dest[count++] = c;
		dest[count] = 0;
		while(BusyUART()) if (!inprocloop) main_processing_loop();
		WriteUART(c);
		}
	while(BusyUART()) if (!inprocloop) main_processing_loop();
	WriteUART('\r');
	while(BusyUART()) if (!inprocloop) main_processing_loop();
	WriteUART('\n');
	inread = 0;
	return(count);
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
	AppConfig.Flags.bIsDHCPReallyEnabled = TRUE;
	AppConfig.Flags.bInConfigMode = TRUE;
	memcpypgm2ram((void *)&AppConfig.MyMACAddr, (const far rom void *)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));
	AppConfig.SerialNumber = 6969;
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
	strcpypgm2ram(AppConfig.Password,(ARGH)"radios");
	strcpypgm2ram(AppConfig.HostPassword,(ARGH)"BLAH");
	strcpypgm2ram(AppConfig.VoterFQDN,(ARGH)"devel.lambdatel.com");


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
		if(c == 0x10u)
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
	AppConfig.ExternalCTCSS = 1;
}

void paktc(void)
{
	printf((ARGH)"\nPress The Any Key (Enter) To Continue\n");
}

void main()
{

	BOOL bootok;
	BYTE i;



	OSCCON = 0;
	OSCTUNE = 0x80;
	DSCONH = 0;
	DSCONL = 0;
	OSCTUNEbits.PLLEN = 1;
	WDTCONbits.SWDTEN = 0;
	INTCON2bits.RBPU = 0;  // P/U controlled by TRISB bits
	// pump up processor speed to 48 MHz
	CCPR1L = 0;
//	RPOR13 = 14;
	RPOR12 = 5;
	RPINR16 = 11;

	/* this was just stupid of me... */
	RPOR18 = 9;
	RPOR4 = 10;
	RPINR21 = 3;

	TRISA = 1; /* Bit 0 is analog input */
	TRISB = 0x1e; /* Bits 1-4 are outputs */
	TRISC = 0x15; /* Bits 0,2,4 are inputs */

	LATA = 0x2f;
	LATB = 0;
	LATC = 0;
	TESTBIT = 1;
	UCON = 0;
	UCFGbits.UTRDIS = 1;

	// Set up PWM initially
	CCP1CON = 0xc;
	TMR2 = 0;
	PR2 = 0x3f;
	T2CON = 4;
	CCPR1L = 0x20;
	CCPR1H = 0;
	CCP1CON = 0xc;
	T1CON = 0x43;
	TMR1L = TMR1VAL & 0xff;
	TMR1H = TMR1VAL >> 8;
	IPR1bits.TMR1IP = 1;
	PIR1bits.TMR1IF = 0;
	PIE1bits.TMR1IE = 1;

 	midx = 0;
	filling_buffer = 0;
	fillindex = 0;
	filled = 0;
	txreal = 0;
	inprocloop = 0;
	inread = 0;
	their_challenge[0] = 0;
	ininput = 0;
	aborted = 0;
	dwLastIP = 0;
	dnstimer = 0;
	dnsdone = 0;
	dnsnotify = 0;
	indisplay = 0;
	memset(&LastVoterAddr,0,sizeof(LastVoterAddr));
	memset(&CurVoterAddr,0,sizeof(CurVoterAddr));
	connected = 0;
	gpsforcetimer = 0;
	attempttimer = 0;
	lastrxtimer = 0;
	host_txseqno = 0;
	txseqno_ptt = 0;
	txseqno = 0;
	digest = 0;
	resp_digest = 0;
	mydigest = 0;
	badmix = 0;
	option_flags = 0;
	lastval = 0x80;

	memset(txaudio0,MULAW_SILENCE,sizeof(txaudio0));
	memset(txaudio1,MULAW_SILENCE,sizeof(txaudio1));
	memset(txaudio2,MULAW_SILENCE,sizeof(txaudio2));
	memset(txaudio3,MULAW_SILENCE,sizeof(txaudio3));
	memset(txaudio4,MULAW_SILENCE,sizeof(txaudio4));
	memset(txaudio5,MULAW_SILENCE,sizeof(txaudio5));
	memset(txaudio6,MULAW_SILENCE,sizeof(txaudio6));
	memset(txaudio7,MULAW_SILENCE,sizeof(txaudio7));


	// Configure USART
    TXSTA2 = 0x20;
    RCSTA2 = 0x90;

	// See if we can use the high baud rate setting
	#if ((GetPeripheralClock()+2*BAUD_RATE)/BAUD_RATE/4 - 1) <= 255
		SPBRG2 = (GetPeripheralClock()+2*BAUD_RATE)/BAUD_RATE/4 - 1;
		TXSTA2bits.BRGH = 1;
	#else	// Use the low baud rate setting
		SPBRG2 = (GetPeripheralClock()+8*BAUD_RATE)/BAUD_RATE/16 - 1;
	#endif

	stdout = _H_USER;

	PIR3bits.TX2IF = 0;
	PIR3bits.RC2IF = 0;

	// Set Up A/D for audio input
	ADCON0 = 0;
	ADCON1 = 0xa2;
	ANCON0 = 0xfe;
	ANCON1 = 0x1f;
	ADCON0bits.ADON = 1;
	// Calibrate swine A/D
	ADCON1bits.ADCAL = 1;
	ADCON0bits.GO = 1;
	while(ADCON0bits.GO);
	ADCON1bits.ADCAL = 0;

	// Set Up Interrupts for A/D
	PIR1bits.ADIF = 0;
	IPR1bits.ADIP = 1;
	PIE1bits.ADIE = 1;

	TickInit();

	SYSLED = 1;

	XEEInit();

	// Initialize Stack and application related NV variables into AppConfig.
	InitAppConfig();

	// Enable global interrupts
	INTCONbits.GIE = 1;
	INTCONbits.GIEL = 1;

	// Initialize core stack layers (MAC, ARP, TCP, UDP) and
	// application modules (HTTP, SNMP, etc.)
    StackInit();

	udpSocketUser = INVALID_UDP_SOCKET;

	sprintf(challenge,(ARGH)"%lu",GenerateRandomDWORD() % 1000000000ul);

	if (AppConfig.Flags.bIsDHCPReallyEnabled)
		dwLastIP = AppConfig.MyIPAddr.Val;

	if (!AppConfig.Flags.bIsDHCPReallyEnabled)
		AppConfig.Flags.bInConfigMode = FALSE;

  	printf((ARGH)"\nRANGER Module System verson 0.2  11/30/2011, Jim Dixon WB6NIL\n");

	while(1)
	{
		char cmdstr[50];
		int i,sel;
		WORD i1,i2,i3,i4,x;
		DWORD l;
		BOOL ok;

		bootok = ((AppConfig.BootIPCheck == GetBootCS()));

		printf((ARGH)"Select the following values to View/Modify:\n\n");

		printf((ARGH)"1  - Serial # (%d),  ",AppConfig.SerialNumber);
		printf((ARGH)"2  - (Static) IP Address (%d.%d.%d.%d)\n",AppConfig.DefaultIPAddr.v[0],AppConfig.DefaultIPAddr.v[1],
			AppConfig.DefaultIPAddr.v[2],AppConfig.DefaultIPAddr.v[3]);
		printf((ARGH)"3  - (Static) Netmask (%d.%d.%d.%d)\n",AppConfig.DefaultMask.v[0],
			AppConfig.DefaultMask.v[1],AppConfig.DefaultMask.v[2],AppConfig.DefaultMask.v[3]);
		printf((ARGH)"4  - (Static) Gateway (%d.%d.%d.%d)\n",AppConfig.DefaultGateway.v[0],AppConfig.DefaultGateway.v[1],
			AppConfig.DefaultGateway.v[2],AppConfig.DefaultGateway.v[3]);
		printf((ARGH)"5  - (Static) Primary DNS Server (%d.%d.%d.%d)\n",AppConfig.DefaultPrimaryDNSServer.v[0],
			AppConfig.DefaultPrimaryDNSServer.v[1],AppConfig.DefaultPrimaryDNSServer.v[2],
			AppConfig.DefaultPrimaryDNSServer.v[3]);
		printf((ARGH)"6  - (Static) Secondary DNS Server (%d.%d.%d.%d),   ",AppConfig.DefaultSecondaryDNSServer.v[0],
			AppConfig.DefaultSecondaryDNSServer.v[1],AppConfig.DefaultSecondaryDNSServer.v[2],
			AppConfig.DefaultSecondaryDNSServer.v[3]);
		printf((ARGH)"7  - DHCP Enable (%d)\n",AppConfig.Flags.bIsDHCPReallyEnabled);
		printf((ARGH)"8  - Voter Server Address (FQDN) (%s)\n",AppConfig.VoterFQDN);
		printf((ARGH)"9  - Voter Server Port (%d),   ",AppConfig.VoterServerPort);
		printf((ARGH)"10 - Local Port (Override) (%d)\n",AppConfig.DefaultPort);
		printf((ARGH)"11 - Client Password (%s),   ",AppConfig.Password);
		printf((ARGH)"12 - Host Password (%s)\n",AppConfig.HostPassword);
		printf((ARGH)"13 - Tx Buffer Length (%d)\n",AppConfig.TxBufferLength);
		printf((ARGH)"14 - COR/CTCSS (0=Non-Inverted, 1=Inverted) (%d)\n",AppConfig.ExternalCTCSS);
		printf((ARGH)"15 - Debug Level (%d), ",AppConfig.DebugLevel);
		if (bootok)
		{
			printf((ARGH)"16 - BootLoader IP Address (%d.%d.%d.%d) (OK)\n",AppConfig.BootIPAddr.v[0],
				AppConfig.BootIPAddr.v[1],AppConfig.BootIPAddr.v[2],AppConfig.BootIPAddr.v[3]);
		}
		else
		{
			printf((ARGH)"16 - BootLoader IP Address (%d.%d.%d.%d) (BAD)\n",AppConfig.BootIPAddr.v[0],
				AppConfig.BootIPAddr.v[1],AppConfig.BootIPAddr.v[2],AppConfig.BootIPAddr.v[3]);
		}
		printf((ARGH)"97 - RX Level, 98 - Status, 99 - Save Values to EEPROM, r - reboot system\n\n");
		
		aborted = 0;
		while(!aborted)
		{
			printf((ARGH)"Enter Selection (1-16,97-99,r) : ");
			memset(cmdstr,0,sizeof(cmdstr));
			if (!mygets(cmdstr,sizeof(cmdstr) - 1)) 
			{
				aborted = 1;
				break;
			}
			if (!strchr(cmdstr,'!')) break;
		}
		if (aborted) continue;
		if ((strchr(cmdstr,'R')) || strchr(cmdstr,'r'))
		{
				printf((ARGH)"System Re-Booting...\n");
				Reset();
		}
		sel = atoi(cmdstr);
		if ((sel >= 1) && (sel <= 16))
		{
			printf((ARGH)"Enter New Value : ");
			if (aborted) continue;
			if ((!mygets(cmdstr,sizeof(cmdstr) - 1)) || (strlen(cmdstr) < 1))
			{
				printf((ARGH)"No Entry Made, Value Not Changed\n");
				continue;
			}
		}
		ok = 0;
		switch(sel)
		{
			case 1: // Serial # (part of Mac Address)
				i1 = atoi(cmdstr);
				AppConfig.SerialNumber = atoi(cmdstr);
				ok = 1;
				break;
			case 2: // Default IP address
				if (StringToIPAddress((BYTE *)cmdstr,&AppConfig.DefaultIPAddr))
				{
					ok = 1;
				}
				break;
			case 3: // Default Netmask
				if (StringToIPAddress((BYTE *)cmdstr,&AppConfig.DefaultMask))
				{
					ok = 1;
				}
				break;
			case 4: // Default Gateway
				if (StringToIPAddress((BYTE *)cmdstr,&AppConfig.DefaultGateway))
				{
					ok = 1;
				}
				break;
			case 5: // Default Primary DNS
				if (StringToIPAddress((BYTE *)cmdstr,&AppConfig.DefaultPrimaryDNSServer))
				{
					ok = 1;
				}
				break;
			case 6: // Default Secondary DNS
				if (StringToIPAddress((BYTE *)cmdstr,&AppConfig.DefaultSecondaryDNSServer))
				{
					ok = 1;
				}
				break;
			case 7: // DHCP Enable
				i1 = atoi(cmdstr);
				if (i1 < 2)
				{
					AppConfig.Flags.bIsDHCPReallyEnabled = i1;
					ok = 1;
				}
				break;
			case 8: // VOTER Server FQDN
				x = strlen(cmdstr);
				if ((x > 1) && (x < sizeof(AppConfig.VoterFQDN)))
				{
					strcpy(AppConfig.VoterFQDN,cmdstr);
					ok = 1;
				}
				break;
			case 9: // Voter Server PORT
				i1 = atoi(cmdstr);
				AppConfig.VoterServerPort = i1;
				ok = 1;
				break;
			case 10: // My Default Port
				i1 = atoi(cmdstr);
				AppConfig.DefaultPort = i1;
				ok = 1;
				break;
			case 11: // VOTER Client Password
				x = strlen(cmdstr);
				if ((x > 1) && (x < sizeof(AppConfig.Password)))
				{
					strcpy(AppConfig.Password,cmdstr);
					ok = 1;
				}
				break;
			case 12: // VOTER Server Password
				x = strlen(cmdstr);
				if ((x > 1) && (x < sizeof(AppConfig.HostPassword)))
				{
					strcpy(AppConfig.HostPassword,cmdstr);
					ok = 1;
				}
				break;
			case 13: // Tx Buffer Length
				i1 = atoi(cmdstr);
				if ((i1 >= 800) && (i1 <= MAX_BUFLEN))
				{
					AppConfig.TxBufferLength = i1;
					ok = 1;
				}
				break;
			case 14: // EXT CTCSS
				i1 = atoi(cmdstr);
				if (i1 <= 1)
				{
					AppConfig.ExternalCTCSS = i1;
					ok = 1;
				}
				break;
			case 15: // Debug Level
				i1 = atoi(cmdstr);
				if (i1 <= 10000)
				{
					AppConfig.DebugLevel = i1;
					ok = 1;
				}
				break;
			case 16: // BootLoader IP Address
				if (StringToIPAddress((BYTE *)cmdstr,&AppConfig.BootIPAddr))
				{
					AppConfig.BootIPCheck = GetBootCS();
					ok = 1;
				}
				break;
#ifdef	DUMPENCREGS
			case 96:
				DumpETHReg();
 				paktc();
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				continue;
#endif
#if 0

			case 97: // Display RX Level Quasi-Graphically  
			 	WriteUART(' ');
		      	for(i = 0; i < NCOLS; i++) WriteUART(' ');
		        printf((ARGH)rxvoicestr);
				indisplay = 1;
				mygets(cmdstr,sizeof(cmdstr) - 1);
				indisplay = 0;
				continue;
#endif
#if 0
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
				paktc();
				fflush(stdout);
				myfgets(cmdstr,sizeof(cmdstr) - 1,stdin);
				continue;
#endif
			case 99:
				SaveAppConfig();
				printf((ARGH)"Configuration Settings Written to EEPROM\n");
				continue;
			default:
				printf((ARGH)"Invalid Selection\n");
				continue;
		}
		if (ok) printf((ARGH)"Value Changed Successfully\n");
		else printf((ARGH)"Invalid Entry, Value Not Changed\n");

#if 0
		x = mygets(cmdstr,sizeof(cmdstr) - 1);
		printf((ARGH)"Ret. %d, Got buffer: %s\n",x,cmdstr);
#endif
	}
}

