/*
* VOTER System Monitor Server
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
*
*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*
*   command line args: 
*     -f  - Run in foreground (do not become daemon)
*     -p <port #> - Listen for TCP connections on specified port
*     -u <port #> - Listen for UDP packets from VOTER channel driver on specified port
*     (Default port numbers for both TCP and UDP are 1667)
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>

#define	MAXCLIENTS 1000		/* max # of simultaneous connections */
#define	FRAME_SIZE 160

/* the structure that holds the connected client info */
static struct votmonclient {
	int s;			/* the file descriptor of their active socket */
} clients[MAXCLIENTS];

int debug = 0;			/* set this to 1 to see "stream of conciousness" messages */
int udp_port = 1667;
int tcp_port = 1667;
int udp_socket = -1;

unsigned long long lasttime = 0;

#pragma pack(push)
#pragma pack(1)
typedef struct {
	uint32_t vtime_sec;
	uint32_t  vtime_nsec;
} VTIME;

typedef struct {
	VTIME curtime;
	uint8_t audio[FRAME_SIZE];
	char str[152];
} VOTER_STREAM;
#pragma pack(pop)

int tvdiff_ms(struct timeval end, struct timeval start)
{
	/* the offset by 1,000,000 below is intentional...
	   it avoids differences in the way that division
	   is handled for positive and negative numbers, by ensuring
	   that the divisor is always positive
	*/
	return  ((end.tv_sec - start.tv_sec) * 1000) +
		(((1000000 + end.tv_usec - start.tv_usec) / 1000) - 1000);
}

/* the main body of the program */
int main(int argc,char *argv[])
{
int	c,i,j,n,s,ss,isdaemon = 1;
uint64_t thistime;
socklen_t recvlen,fromlen,u;
VOTER_STREAM buf;
struct sockaddr_in sin,myaddr,clientaddr;
struct linger set_linger;
struct timeval tv;
fd_set fds;
FILE	*fp;
struct timeval lastrx = {0,0},now;


	while ((c = getopt (argc, argv, "fp:u:")) != -1)
	{
		switch (c)
		{
		    case 'f' :
			isdaemon = 0;
			break;
		    case 'p' :
			tcp_port = atoi(optarg);
			break;
		    case 'u' :
			udp_port = atoi(optarg);
			break;
		}
	}

	if (isdaemon)
	{
		/* fork so that daemon can run in background */
		i = fork();
		/* if fork had error */
		if (i < 0)
		{
			perror("Cannot fork");
			exit(255);
		}
		/* if is the original process */
		if (i > 0)
		{
			/* open PID file */
			fp = fopen("/var/run/votmond.pid","w");
			/* if error */
			if (!fp)
			{
				perror("Cannot open pid file");
				exit(255);
			}
			/* write PID into file */
			fprintf(fp,"%d\n",i);
			/* close it */
			fclose(fp);
			/* exit this process */
			exit(0);
		}
	}
	if ((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("UDP socket() failed");
		exit(255);
	}
	memset((char *) &sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(udp_port);               
	if (bind(udp_socket, (struct sockaddr *)&sin, sizeof(sin)) == -1) 
	{
		perror("bind() of UDP failed");
		exit(255);
	}
	/* create a socket to listen for connections */
	if ((ss  = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("TCP server socket() failed");
		exit(255);
	}

	/* define its listening address and port */
	memset(&myaddr, 0, sizeof(myaddr));   /* Zero out structure */
	myaddr.sin_family = AF_INET;                /* Internet address family */
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(tcp_port);      /* Local port */
	/* bind the address info to the socket */
	if (bind(ss, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0)
	{
		perror("bind() failed");
		close(udp_socket);
		close(ss);
		exit(255);
	}
	/* make it listen for incomming connections */
	if (listen(ss, 5) < 0)
	{
		perror("listen() failed");
		close(udp_socket);
		close(ss);
		exit(255);
	}
	/* make it clear the port after it closes (dont linger) */
	set_linger.l_onoff = 1;
	set_linger.l_linger = 0;
	setsockopt(ss, SOL_SOCKET, SO_LINGER, (char *)&set_linger,
		sizeof(set_linger));

	/* loop forever */
	for(;;)
	{
		FD_ZERO(&fds);
		FD_SET(ss,&fds);
		j = ss;
		FD_SET(udp_socket,&fds);
		if (udp_socket > j) j = udp_socket;
		/* go thru all the slots and add active ones */
		for(i = 0; i < MAXCLIENTS; i++)
		{
			/* skip if inactive */
			if (clients[i].s < 1) continue;
			/* add it to the list */
			FD_SET(clients[i].s,&fds);
			/* if this fd greater then greatest, then
				make it greatest */
			if (clients[i].s > j) j = clients[i].s;
		}
		/* set up a 100ms timeout */
		tv.tv_sec = 0;
		tv.tv_usec = 20000;
		/* wait for something to happen */
		n = select(j + 1,&fds,NULL,NULL,&tv);
		/* if timeout or error, do it again */
		if (n < 0) continue;
		gettimeofday(&now,NULL);
		if (lastrx.tv_sec && (tvdiff_ms(now,lastrx) >= 60))
		{
			memset(&buf,0,sizeof(buf));
			for(i = 0; i < MAXCLIENTS; i++)
			{
				if (clients[i].s < 1) continue;
				send(clients[i].s,&buf,recvlen,0);
			}
			lastrx.tv_sec = lastrx.tv_usec = 0;
		}						
		if (n < 1) continue;
		/* if we a UDP packet from VOTER channel */
		if (FD_ISSET(udp_socket,&fds))
		{
			fromlen = sizeof(struct sockaddr_in);
			recvlen = recvfrom(udp_socket,&buf,sizeof(buf),0,
				(struct sockaddr *)&sin,&fromlen);
			if (recvlen < 1) break;
			if (recvlen != 320) continue;
			thistime = ((uint64_t)buf.curtime.vtime_sec << 32) + buf.curtime.vtime_nsec;
			if (thistime <= lasttime) continue; 
			lasttime = thistime;
			lastrx = now;
			for(i = 0; i < MAXCLIENTS; i++)
			{
				if (clients[i].s < 1) continue;
				send(clients[i].s,&buf,recvlen,0);
			}
			continue;
		}
		/* if we got a new incomming connection */
		if (FD_ISSET(ss,&fds))
		{
		      /* Set the size of the in-out parameter */
			u = sizeof(clientaddr);
			/* get client's socket */
		        if ((s = accept(ss, (struct sockaddr *)	&clientaddr,&u)) < 0)
			{
				perror("accept() failed");
				continue;
			}

			fcntl(s,F_SETFL,O_NONBLOCK);
			i = 1;
			/* disable TCP Tinygram Avoidance (Nagle Algorithm) */ 
			setsockopt(s,IPPROTO_TCP,TCP_NODELAY,(char *) &i, sizeof(int));
		        /* if you want to get the clients address in ASCII at this point, use  
				inet_ntoa(clientaddr.sin_addr) */

			/* find an open slot */
			for(i = 0; i < MAXCLIENTS; i++)
			{
				/* if we found an open one */
				if (clients[i].s < 1)
				{
					/* zero-out the structure */
					memset(&clients[i],0,sizeof(struct votmonclient));
					/* record the client's socket */
					clients[i].s = s;
					/* were done with this */
					break;
				}					
			}
			continue;
		}
		/* go thru list and see if we can find one that is active */
		for(i = 0; i < MAXCLIENTS; i++)
		{
			/* skip if inactive */
			if (clients[i].s < 1) continue;
			/* if this socket has something for us */
			if (FD_ISSET(clients[i].s,&fds))
			{
				/* receive packet from client */
				j = recv(clients[i].s,&buf,sizeof(buf) - 1,0);
				/* if some sort of error, terminate this session */
				if (j < 1)
				{
					/* close the socket */
					close(clients[i].s);
					/* wipe out the slot entry */
					memset(&clients[i],0,sizeof(struct votmonclient));
					continue;
				}
				continue;
			}				
		}
	}
	close(ss);
	close(udp_socket);	
	/* exit the program */
	exit(0);
}


