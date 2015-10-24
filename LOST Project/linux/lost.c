/*
 * L O S T -- Logger Of Signal Transitions
 * Version 1.0, 10/24/2015
 *
 * Copyright 2015, Jim Dixon <jim@lambdatel.com>, All Rights Reserved
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/time.h>
#include <mysql/mysql.h>
#include <pthread.h>


#define	MAX_GPS_TIMEOUT 2

int debug = 0;

char gpsok = -1;

char *port = "/dev/ttyUSB0";
int	fd = -1;
MYSQL mysql;
char *gport = "/dev/ttyS0";
int gps_baud = 38400;
int istsip = 0;
int	gfd = -1;
MYSQL gmysql;

time_t lastgps = 0;

pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;

/*
 * *****************************************
 * Generic serial I/O routines             *
 * *****************************************
*/

struct timeval now,last;

static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int Base64encode_len(int len)
{
    return ((len + 2) / 3 * 4) + 1;
}

int Base64encode(char *encoded, char *string, int len)
{
    int i;
    char *p;

    p = encoded;
    for (i = 0; i < len - 2; i += 3) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    *p++ = basis_64[((string[i] & 0x3) << 4) |
                    ((int) (string[i + 1] & 0xF0) >> 4)];
    *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                    ((int) (string[i + 2] & 0xC0) >> 6)];
    *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    if (i == (len - 1)) {
        *p++ = basis_64[((string[i] & 0x3) << 4)];
        *p++ = '=';
    }
    else {
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
    }
    *p++ = '=';
    }
    *p++ = '\0';
    return p - encoded;
}


void mynow(char *str, int maxlen)
{
struct timeval now;
struct tm *tm;

	gettimeofday(&now,NULL);
	tm = gmtime((time_t *)&now.tv_sec);
	sprintf(str,"%u.%06u,",(unsigned int)now.tv_sec,(unsigned int)now.tv_usec);
	strftime(str + strlen(str),maxlen,"%F %T",tm);
	sprintf(str + strlen(str),".%06u",(unsigned int)now.tv_usec);
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

static unsigned int twoascii(char *s)
{
unsigned int rv;

	rv = s[1] - '0';
	rv += 10 * (s[0] - '0');
	return(rv);
}

/*
 * Generic serial port open command 
 */

static int serial_open(char *fname, int speed, int stop2)
{
	struct termios mode;
	int fd;

	fd = open(fname,O_RDWR);
	if (fd == -1)
	{
		fprintf(stderr,"Can not open serial port %s\n",fname);
		return -1;
	}
	
	memset(&mode, 0, sizeof(mode));
	if (tcgetattr(fd, &mode)) {
		fprintf(stderr,"Unable to get serial parameters on %s: %s\n", fname, strerror(errno));
		return -1;
	}
#ifndef	SOLARIS
	cfmakeraw(&mode);
#else
        mode.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                        |INLCR|IGNCR|ICRNL|IXON);
        mode.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
        mode.c_cflag &= ~(CSIZE|PARENB|CRTSCTS);
        mode.c_cflag |= CS8;
	if(stop2)
		mode.c_cflag |= CSTOPB;
	mode.c_cc[VTIME] = 3;
	mode.c_cc[VMIN] = 1; 
#endif

	cfsetispeed(&mode, speed);
	cfsetospeed(&mode, speed);
	if (tcsetattr(fd, TCSANOW, &mode)){
		fprintf(stderr,"Unable to set serial parameters on %s: %s\n", fname, strerror(errno));
		return -1;
	}
	usleep(100000);
	return(fd);	
}

/*
 * Return receiver ready status
 *
 * Return 1 if an Rx byte is avalable
 * Return 0 if none was avaialable after a time out period
 * Return -1 if error
 */


static int serial_rxready(int fd, int timeoutms)
{
	fd_set readyset;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 1000 * timeoutms;
        FD_ZERO(&readyset);
        FD_SET(fd, &readyset);
        return select(fd + 1, &readyset, NULL, NULL, &tv);
}

static int tvdiff_ms(struct timeval end, struct timeval start)
{
	/* the offset by 1,000,000 below is intentional...
	   it avoids differences in the way that division
	   is handled for positive and negative numbers, by ensuring
	   that the divisor is always positive
	*/
	return  ((end.tv_sec - start.tv_sec) * 1000) +
		(((1000000 + end.tv_usec - start.tv_usec) / 1000) - 1000);
}


/*
 * Receive a string from the serial device
 */

static int serial_rx(int fd, char *rxbuf, int rxmaxbytes, unsigned timeoutms, char termchr)
{
	char c;
	int i, j, res;
	long diffms;

	if ((!rxmaxbytes) || (rxbuf == NULL)){ 
		return 0;
	}
	memset(rxbuf,0,rxmaxbytes);
	i = 0;
	while(i < rxmaxbytes){
		if(timeoutms){
			res = serial_rxready(fd, timeoutms);
			if(res < 0)
				return -1;
			if(!res)
			{
				gettimeofday(&now,NULL);
				diffms = tvdiff_ms(now,last);
				continue;
			}
		}
		j = read(fd,&c,1);
		if(j == -1){
			fprintf(stderr,"read failed: %s\n", strerror(errno));
			return -1;
		}
		if (j == 0) return 0;
		if (c == termchr) break;
		if (c >= ' ')
		{
			rxbuf[i++] = c;
			rxbuf[i] = 0;
		} 
	}					
	return i;
}

int getTSIPPacket(unsigned char *gps_buf, int maxlen, int timeoutms)
{
char     c,TSIPwasdle;
int gps_bufindex,res;
unsigned int tdiff;
time_t	now;

	gps_bufindex = 0;
	for(;;)
	{
		time(&now);
		pthread_mutex_lock(&mylock);
		tdiff = (unsigned int)now - (unsigned int)lastgps;
		if (tdiff > MAX_GPS_TIMEOUT)
			gpsok = -1;
		pthread_mutex_unlock(&mylock);
		res = serial_rxready(gfd, timeoutms);
		if(res < 0)
			return -1;
		if(!res) continue;
		res = read(gfd,&c,1);
		if(res == -1)
		{
			fprintf(stderr,"GPS read failed: %s\n", strerror(errno));
			return -1;
		}
		if (res == 0)
			continue;
	        if (gps_bufindex == 0)
	        {
	                TSIPwasdle = 0;
	                if (c == 16)
	                {
	                        TSIPwasdle = 1;
	                        gps_bufindex++;
	                }
			continue;
		}
	        if (gps_bufindex > maxlen)
	        {
	                gps_bufindex = 0;
			continue;
	        }
	        if ((c == 16) && (!TSIPwasdle))
	        {
	                TSIPwasdle = 1;
			continue;
		}
	        else if (TSIPwasdle && (c == 3))
	        {
	                break;
	        }
	        TSIPwasdle = 0;
	        gps_buf[gps_bufindex - 1] = c;
			gps_bufindex++;
	}
        return gps_bufindex;
}


void *gpsthread(void *foo)
{
int i,n;
unsigned char gps_buf[100];
time_t t,gps_time;
char sql[10000],buf[2000],buf1[200],buf2[2000],*strs[30];

	for(;;)
	{
	    if (istsip)
	    {
		i = getTSIPPacket(gps_buf,sizeof(gps_buf),100);
		if (debug)
			printf("got TSIP packet len %d\n",i);
		if (gps_buf[0] != 0x8f) continue;
		if (gps_buf[1] == 0xab) 
		{
			struct tm tm;
			unsigned short w;
	
			memset(&tm,0,sizeof(tm));
			tm.tm_sec = gps_buf[11];
			tm.tm_min = gps_buf[12];
			tm.tm_hour = gps_buf[13];
			tm.tm_mday = gps_buf[14];
			tm.tm_mon = gps_buf[15] - 1; 
			w = gps_buf[17] | ((unsigned short)gps_buf[16] << 8);
			tm.tm_year = w - 1900;
			tm.tm_isdst = 1;
			gps_time = mktime(&tm);
			if (debug)
				printf("GPS time: %s",ctime(&gps_time));
			else
			{
				printf("T");
				fflush(stdout);
			}
			time(&t);
			pthread_mutex_lock(&mylock);
			lastgps = t;
			pthread_mutex_unlock(&mylock);
			Base64encode(buf,(char *)gps_buf,i);
			mynow(buf1,sizeof(buf1));
			sprintf(sql,"INSERT INTO gps (what,raw,systime) VALUES('GPS TIME: %24.24s','%s','%s') ;",
				ctime(&gps_time),buf,buf1);
			if (mysql_real_query(&gmysql,sql,strlen(sql)))
			{
			    fprintf(stderr,"Failed to insert record: Error: %s\n",mysql_error(&mysql));
			    exit(255);
			}
			continue;
		}
		if (gps_buf[1] == 0xac)
		{
			char happy;

			happy = 1;
			if (gps_buf[13]) happy = 0;
			if ((gps_buf[14] != 0) && (gps_buf[14] != 8)) happy = 0;
			if (gps_buf[9] || gps_buf[10]) happy = 0;
			if ((gps_buf[12] & 0x1f) | gps_buf[11]) happy = 0;
			pthread_mutex_lock(&mylock);
			gpsok = happy;
			pthread_mutex_unlock(&mylock);
			if (debug)
			{
				printf("GPS-DEBUG: TSIP: ok %d, 9 - 14: %02x %02x %02x %02x %02x %02x\n",
					happy,gps_buf[9],gps_buf[10],gps_buf[11],gps_buf[12],gps_buf[13],gps_buf[14]);
			}
			else
			{
				printf("S");
				fflush(stdout);
			}
			Base64encode(buf,(char *)gps_buf,i);
			mynow(buf1,sizeof(buf1));
			sprintf(sql,"INSERT INTO gps (what,raw,systime) VALUES('TSIP: ok %d, 9 - 14: %02x %02x %02x %02x %02x %02x','%s','%s') ;",
				happy,gps_buf[9],gps_buf[10],gps_buf[11],gps_buf[12],gps_buf[13],gps_buf[14],buf,buf1);
			if (mysql_real_query(&gmysql,sql,strlen(sql)))
			{
			    fprintf(stderr,"Failed to insert record: Error: %s\n",mysql_error(&mysql));
			    exit(255);
			}
		}
	    }
	    else // is NMEA
	    {
		if (serial_rx(gfd,buf,sizeof(buf) - 1,100,'\r') < 1) continue;
		printf("Got NMEA: %s\n",buf);
		strcpy(buf2,buf);
		n = explode_string((char *)buf2,strs,30,',','\"');
		if (n < 1) continue;
		if (!strcmp(strs[0],"$GPRMC"))
		{
			struct tm tm;
			char happy;
	
			if (n < 10) continue;
			if (!strcmp(strs[2],"A"))
			{
				happy = 1;
			}
			else
			{
				happy = 0;
			}
			memset(&tm,0,sizeof(tm));
			tm.tm_sec = twoascii(strs[1] + 4);
			tm.tm_min = twoascii(strs[1] + 2);
			tm.tm_hour = twoascii(strs[1]);
			tm.tm_mday = twoascii(strs[9]);
			tm.tm_mon = twoascii(strs[9] + 2) - 1;
			tm.tm_year = twoascii(strs[9] + 4) + 100;
			pthread_mutex_lock(&mylock);
			gpsok = happy;
			pthread_mutex_unlock(&mylock);
			tm.tm_isdst = 1;
			gps_time = mktime(&tm);
			if (debug)
				printf("GPS time: %s, ok: %d",ctime(&gps_time),gpsok);
			else
			{
				printf("T");
				fflush(stdout);
			}
			time(&t);
			pthread_mutex_lock(&mylock);
			lastgps = t;
			pthread_mutex_unlock(&mylock);
			mynow(buf1,sizeof(buf1));
			sprintf(sql,"INSERT INTO gps (what,raw,systime) VALUES('GPS STAT,TIME: %d, %24.24s','%s','%s') ;",
				gpsok,ctime(&gps_time),buf,buf1);
		}
		else
		{
			time(&t);
			mynow(buf1,sizeof(buf1));
			sprintf(sql,"INSERT INTO gps (what,raw,systime) VALUES('GPS','%s','%s') ;",
				buf,buf1);
		}
		if (mysql_real_query(&gmysql,sql,strlen(sql)))
		{
		    fprintf(stderr,"Failed to insert record: Error: %s\n",mysql_error(&mysql));
		    exit(255);
		}
	    }
	}
	pthread_exit(0);
}

int main(int argc, char *argv[])
{
char	buf[1024],state,sql[10000],buf1[200],isok;
int gbaud,opt;
unsigned int c;
unsigned long long x;
MYSQL mysql;
double d;
pthread_attr_t attr;
pthread_t gps_thread;
// With the intention of being able to match the start time for each session
// in both tables, we use this to create a unique and match-able identifier.
// Once matched, you can then know what seqno to start from in each table.
time_t starttime;

	time(&starttime);
	while ((opt = getopt(argc, argv, "b:g:p:t")) != -1) {
               switch (opt) {
               case 'b':  // Baud Rate
                   gps_baud = atoi(optarg);
                   break;
               case 'g':  // GPS serial port
                   gport = optarg;
                   break;
               case 'p':  // Main serial port
                   port = optarg;
                   break;
               case 't':
                   istsip = 1;
                   break;
               default: /* '?' */
                   fprintf(stderr, "Usage: %s [-p main_serial_port] [-g gps_serial_port] [-b gps_baud_rate] [-t]\n",
                           argv[0]);
                   exit(255);
               }
           }
	switch(gps_baud)
	{
		case 1200:
		    gbaud = B1200;
		    break;
		case 1800:
		    gbaud = B1800;
		    break;
		case 2400:
		    gbaud = B2400;
		    break;
		case 4800:
		    gbaud = B4800;
		    break;
		case 9600:
		    gbaud = B9600;
		    break;
		case 19200:
		    gbaud = B19200;
		    break;
		case 38400:
		    gbaud = B38400;
		    break;
		case 57600:
		    gbaud = B57600;
		    break;
		case 115200:
		    gbaud = B115200;
		    break;
		case 230400:
		    gbaud = B230400;
		    break;
		default:
		    fprintf(stderr,"Unrecognized baud rate: %d\n",gbaud);
		    exit(255);
	}
	printf("Using parameters:\nMain serial port: %s\nGPS serial port: %s\nGPS baud: %d\nGPS protocol: %s\n\n",
		port,gport,gps_baud,(istsip) ? "TSIP" : "NMEA");
	fd = serial_open(port,B115200,0);
	if (fd == -1) exit(255);
	gfd = serial_open(gport,gbaud,0);
	if (gfd == -1) exit(255);

	if (!mysql_init(&mysql))
	{
		fprintf(stderr,"Cant initialize MYSQL instance!!\n");
		exit(255);
	}
	if (!mysql_real_connect(&mysql,NULL,"root","mypassword",NULL,0,NULL,0))
	{
	    fprintf(stderr,"Failed to connect to MySQL: Error: %s\n",mysql_error(&mysql));
	    exit(255);
	}
	if (mysql_select_db(&mysql,"lost"))
	{
	    fprintf(stderr,"Failed to select MySQL database: Error: %s\n",mysql_error(&mysql));
	    exit(255);
	}
	if (!mysql_init(&gmysql))
	{
		fprintf(stderr,"Cant initialize MYSQL instance!!\n");
		exit(255);
	}
	if (!mysql_real_connect(&gmysql,NULL,"root","mypassword",NULL,0,NULL,0))
	{
	    fprintf(stderr,"Failed to connect to MySQL: Error: %s\n",mysql_error(&gmysql));
	    exit(255);
	}
	if (mysql_select_db(&gmysql,"lost"))
	{
	    fprintf(stderr,"Failed to select MySQL database: Error: %s\n",mysql_error(&gmysql));
	    exit(255);
	}
	mynow(buf1,sizeof(buf1));
	sprintf(sql,"INSERT INTO log (channel,val,secs,gpsok,raw,systime) VALUES('-1','0','0.0','-1','%u','%s') ;",
		(unsigned int)starttime,buf1);
	if (mysql_real_query(&mysql,sql,strlen(sql)))
	{
	    fprintf(stderr,"Failed to insert record: Error: %s\n",mysql_error(&mysql));
	    exit(255);
	}
	sprintf(sql,"INSERT INTO gps (what,raw,systime) VALUES('STARTUP','%u','%s') ;",(unsigned int)starttime,buf1);
	if (mysql_real_query(&gmysql,sql,strlen(sql)))
	{
	    fprintf(stderr,"Failed to insert record: Error: %s\n",mysql_error(&mysql));
	    exit(255);
	}
	pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&gps_thread,&attr,gpsthread,NULL);


	gettimeofday(&last,NULL);
	while(serial_rx(fd,buf,sizeof(buf) - 1,100,'\r') > 0)
	{
		if ((strlen(buf) != 19) || (buf[0] != ':'))
		{
			printf("Error!! Got buf %s, len %d\n",buf,(int)strlen(buf));
			continue;
		}
		if (debug)
			puts(buf);
		sscanf(buf + 1,"%02X%LX",&c,&x);
		state = x & 1;
		x >>= 1;
		d = (double)x / (double)100000000.0;
		pthread_mutex_lock(&mylock);
		isok = gpsok;
		pthread_mutex_unlock(&mylock);
		if (debug)
			printf("ch: %d, state: %d, Time: %0.9f\n",c,state,d);
		else
		{
			printf(".");
			fflush(stdout);
		}
		mynow(buf1,sizeof(buf1));
		sprintf(sql,"INSERT INTO log (channel,val,secs,gpsok,raw,systime) VALUES('%d','%d','%0.9f','%d','%s','%s') ;",
			c,state,d,isok,buf,buf1);
		if (mysql_real_query(&mysql,sql,strlen(sql)))
		{
		    fprintf(stderr,"Failed to insert record: Error: %s\n",mysql_error(&mysql));
		    exit(255);
		}
	}
	exit(0);
}


