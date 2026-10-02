/*********************************************************************
 *
 *	Telnet Server
 *  Module for Microchip TCP/IP Stack
 *	 -Provides Telnet services on TCP port 23
 *	 -Reference: RFC 854
 *
 *********************************************************************
 * FileName:        Telnet.c
 * Dependencies:    TCP
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder     9/12/06	Original
 * VE7FET				9/20/26 Add comments, clean up formatting. This
 *								is a custom application based on the
 *								Microchip demo.
 ********************************************************************/
#define __TELNET_C

#include "TCPIPConfig.h"

#if defined(STACK_USE_TELNET_SERVER)

#include "TCPIP_Stack/TCPIP.h"
#include "UART.h"

/* Set up configuration parameter defaults if not overridden in TCPIPConfig.h */
#if !defined(TELNETS_PORT)	
    /* SSL Secured Telnet port (ignored if STACK_USE_SSL_SERVER is undefined) */
	#define TELNETS_PORT		992	
#endif
#if !defined(MAX_TELNET_CONNECTIONS)
    // Maximum number of Telnet connections
	#define MAX_TELNET_CONNECTIONS	(3u)
#endif

/* Override the defaults with those from EEPROM set by the user. */
#define TELNET_PORT     AppConfig.TelnetPort
#define TELNET_USERNAME AppConfig.TelnetUsername
#define TELNET_PASSWORD AppConfig.TelnetPassword

/* Make sure this is <= Telnet TX FIFO size!! */
#define MAXTERMBUF 100

/* Title string */
static ROM BYTE strTitle[] = "\r\n\nVOTER System Serial # ", strTitle1[] = " Remote Console Access\r\n\nLogin: ";
/* Password
 * DO Suppress Local Echo (stop Telnet client from printing typed characters)
 */
static ROM BYTE strPassword[] = "Password: \xff\xfd\x2d";
/* Access denied message */
static ROM BYTE strAccessDenied[]	= "\r\nAccess denied\r\n\r\n";
/* Successful authentication message */
static ROM BYTE strAuthenticated[] = "\r\n\xff\xfe\x22Logged in successfully, now joining console session...\r\n\r\n";

/* Define our Telnet state machine states. */
enum {
	SM_HOME = 0,
	SM_PRINT_LOGIN,
	SM_GET_LOGIN,
	SM_GET_PASSWORD,
	SM_GET_PASSWORD_BAD_LOGIN,
	SM_AUTHENTICATED
} TelnetState;

static TCP_SOCKET hTelnetSockets[MAX_TELNET_CONNECTIONS];
static BYTE vTelnetStates[MAX_TELNET_CONNECTIONS];
static BOOL bInitialized = FALSE;
static BYTE termbuf[MAXTERMBUF];
extern WORD termbufidx;
extern WORD termbuftimer;
extern BYTE AN0String[8];

/*********************************************************************
 * Function:        void TelnetTask(void)
 *
 * PreCondition:    Stack is initialized()
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs Telnet Server related tasks.  Contains
 *                  the Telnet state machine and state tracking
 *                  variables.
 *
 * Note:            None
 ********************************************************************/
void TelnetTask(void)
{
	BYTE		vTelnetSession;
	WORD		w, w2;
	TCP_SOCKET	MySocket;
	char        outstr[64];

	/* Perform one time initialization on power up. */
	if (!bInitialized) {
		for (vTelnetSession = 0; vTelnetSession < MAX_TELNET_CONNECTIONS; vTelnetSession++) {
			hTelnetSockets[vTelnetSession] = INVALID_SOCKET;
			vTelnetStates[vTelnetSession] = SM_HOME;
		}
		bInitialized = TRUE;
	}

	/* Loop through each Telnet session and process state changes and TX/RX data. */
	for (vTelnetSession = 0; vTelnetSession < MAX_TELNET_CONNECTIONS; vTelnetSession++) {
		/* Load up static state information for this session. */
		MySocket = hTelnetSockets[vTelnetSession];
		TelnetState = vTelnetStates[vTelnetSession];

		/* Reset our state if the remote client disconnected from us. */
		if (MySocket != INVALID_SOCKET) {
			if (TCPWasReset(MySocket)) {
				TelnetState = SM_PRINT_LOGIN;
			}
		}
	
		/* Handle session state. */
		switch(TelnetState) {
			case SM_HOME:
				/* Connect a socket to the remote TCP server. */
				MySocket = TCPOpen(0, TCP_OPEN_SERVER, TELNET_PORT, TCP_PURPOSE_TELNET);
				
				/* Abort operation if no TCP socket of type TCP_PURPOSE_TELNET is available
				 * If this ever happens, you need to go add one to TCPIPConfig.h
				 */
				if (MySocket == INVALID_SOCKET) {
					break;
				}
	
				/* Open an SSL listener if SSL server support is enabled */
				#if defined(STACK_USE_SSL_SERVER)
					TCPAddSSLListener(MySocket, TELNETS_PORT);
				#endif
	
				TelnetState++;
				break;
	
			case SM_PRINT_LOGIN:
				#if defined(STACK_USE_SSL_SERVER)
					/* Reject unsecured connections if TELNET_REJECT_UNSECURED is defined. */
					#if defined(TELNET_REJECT_UNSECURED)
						if (!TCPIsSSL(MySocket)) {
							if (TCPIsConnected(MySocket)) {
								TCPDisconnect(MySocket);
								TCPDisconnect(MySocket);
								break;
							}	
						}
					#endif
						
					/* Don't attempt to transmit anything if we are still handshaking. */
					if (TCPSSLIsHandshaking(MySocket)) {
						break;
					}
				#endif
				
				/* Print the serial number from the EEPROM. */
				sprintf(outstr, "%s%d%s", (char *)strTitle, (unsigned int)AppConfig.SerialNumber, (char *)strTitle1);
				/* Make certain the socket can be written to. */
				if (TCPIsPutReady(MySocket) < strlen(outstr)) {
					break;
				}
				/* Place the application protocol data into the transmit buffer. */
				TCPPutString(MySocket, (BYTE *)outstr);
				/* Send the packet. */
				TCPFlush(MySocket);
				/* Advance the state machine. */
				TelnetState++;
	
			case SM_GET_LOGIN:
				/* Make sure we can put the password prompt. */
				if (TCPIsPutReady(MySocket) < strlenpgm((ROM char*)strPassword)) {
					break;
				}
				/* See if the user pressed return. */
				w = TCPFind(MySocket, '\n', 0, FALSE);
				if (w == 0xFFFFu) {
					if(TCPGetRxFIFOFree(MySocket) == 0u) {
						TCPPutROMString(MySocket, (ROM BYTE*)"\r\nToo much data.\r\n");
						TCPDisconnect(MySocket);
					}
					break;
				}
				/* Search for the username -- case insensitive. */
				w2 = TCPFindArray(MySocket, TELNET_USERNAME, strlen((char*)TELNET_USERNAME), 0, TRUE);
				/* w2 is a WORD, so (w2 < 0) is always false and never rejects the 0xFFFF value that
				 * TCPFindArray returns when the username is absent.
				 *
				 * w, w2, and strlen(TELNET_USERNAME) are all unsigned, so the remaining comparison wraps:
				 *		- If the login line length before '\n' equals strlen(TELNET_USERNAME), then 
				 *		(w - strlen) - 1 evaluates to 0xFFFF and matches the not-found sentinel.
				 *		- If that length equals strlen(TELNET_USERNAME) - 1, then w - strlen evaluates
				 *		to 0xFFFF and also matches.
				 *
				 * In both cases the state becomes SM_GET_PASSWORD instead of SM_GET_PASSWORD_BAD_LOGIN,
				 * so a client that sends a wrong username of that exact length is authenticated on the password alone.
				 *
				 * Ensure we do the proper tests to prevent that from happening.
				 */
				if ((w2 == 0xFFFFu) || (w < strlen((char*)TELNET_USERNAME)) || !((w2 == ((w - strlen((char*)TELNET_USERNAME)) - 1)) ||
					(w2 == (w - strlen((char*)TELNET_USERNAME))))) {
					/* Did not find the username, but let's pretend we did so we don't leak the username
					 * validity. Set the state machine accordingly.
					 */
					TelnetState = SM_GET_PASSWORD_BAD_LOGIN;	
				} else {
					TelnetState = SM_GET_PASSWORD;
				}
				/* Username verified, throw this line of data away. */
				TCPGetArray(MySocket, NULL, w + 1);
				/* Print the password prompt. */
				TCPPutROMString(MySocket, strPassword);
				TCPFlush(MySocket);
				break;
	
			case SM_GET_PASSWORD:
			case SM_GET_PASSWORD_BAD_LOGIN:
				/* Make sure we can put the authenticated (password) prompt. */
				if (TCPIsPutReady(MySocket) < strlenpgm((ROM char*)strAuthenticated)) {
					break;
				}
				/* See if the user pressed return. */
				w = TCPFind(MySocket, '\n', 0, FALSE);
				if (w == 0xFFFFu) {
					if (TCPGetRxFIFOFree(MySocket) == 0u) {
						TCPPutROMString(MySocket, (ROM BYTE*)"Too much data.\r\n");
						TCPDisconnect(MySocket);
					}
					break;
				}
				/* Search for the password -- case sensitive. */
				w2 = TCPFindArray(MySocket, TELNET_PASSWORD, strlen((char*)TELNET_PASSWORD), 0, FALSE);
				if ((w2 != 3u) || !((strlen((char*)TELNET_PASSWORD) == w - 4) || (strlen((char*)TELNET_PASSWORD) == w - 3))
					|| (TelnetState == SM_GET_PASSWORD_BAD_LOGIN)) {
					/* Did not find the password.
					 * Set the state machine accordingly.
					 */
					TelnetState = SM_PRINT_LOGIN;	
					TCPPutROMString(MySocket, strAccessDenied);
					TCPDisconnect(MySocket);
					break;
				}
				/* Password verified, throw this line of data away. */
				TCPGetArray(MySocket, NULL, w + 1);
				/* Print the authenticated prompt. */
				TCPPutROMString(MySocket, strAuthenticated);
				TCPFlush(MySocket);
				/* Advance the state machine. */
				TelnetState = SM_AUTHENTICATED;
				/* No break. */
		
			case SM_AUTHENTICATED:
				/*
				 * VOTER console traffic is handled through
				 * GetTelnetConsole(), PutTelnetConsole(), and
				 * ProcessTelnetTimer().
				 */
				break;
		}

		/* Save session state back into the static array. */
		hTelnetSockets[vTelnetSession] = MySocket;
		vTelnetStates[vTelnetSession] = TelnetState;
	}
}

/* Helper function used in Voter.c to get the current Telnet console TCP socket. */
BYTE GetTelnetConsole(void)
{
    BYTE vTelnetSession, c;
    TCP_SOCKET MySocket;

    for (vTelnetSession = 0; vTelnetSession < MAX_TELNET_CONNECTIONS; vTelnetSession++) {
        MySocket = hTelnetSockets[vTelnetSession];

        if (vTelnetStates[vTelnetSession] != SM_AUTHENTICATED) {
            continue;
		}

        if (TCPIsGetReady(MySocket)) {
            TCPGet(MySocket, &c);
            return c;
        }
    }

    return 0;
}

/* Helper function used in Voter.c to put UART data to the current Telnet session. */
BOOL PutTelnetConsole(char c)
{
    TCP_SOCKET MySocket;
    WORD i;

    MySocket = hTelnetSockets[0];

    if (vTelnetStates[0] != SM_AUTHENTICATED) {
        return 1;
	}

    if (termbufidx < MAXTERMBUF) {
        termbuf[termbufidx++] = c;
        termbuftimer = 0;
        return 1;
    }

    if (TCPIsPutReady(MySocket) < termbufidx) {
        StackTask();
        StackApplications();
        return 0;
    }

    for (i = 0; i < termbufidx; i++)
        TCPPut(MySocket, termbuf[i]);

    TCPFlush(MySocket);
    termbufidx = 0;
    termbuftimer = 0;
    return 0;
}

/* Helper function for Voter.c to process the output buffer timer. */
void ProcessTelnetTimer(void)
{
    TCP_SOCKET MySocket;
    WORD i;

    MySocket = hTelnetSockets[0];

    if (vTelnetStates[0] != SM_AUTHENTICATED) {
        return;
	}

    if (termbufidx < 1) {
        return;
	}

    if(TCPIsPutReady(MySocket) < termbufidx) {
        StackTask();
        StackApplications();
        return;
    }

    for (i = 0; i < termbufidx; i++)
        TCPPut(MySocket, termbuf[i]);

    termbufidx = 0;
    termbuftimer = 0;
    TCPFlush(MySocket);
}

/* Helper function for Voter.c to terminate the current Telnet session. */
void CloseTelnetConsole(void)
{
    BYTE vTelnetSession;
    TCP_SOCKET MySocket;

    termbufidx = 0;
    termbuftimer = 0;

    for (vTelnetSession = 0; vTelnetSession < MAX_TELNET_CONNECTIONS; vTelnetSession++) {
        MySocket = hTelnetSockets[vTelnetSession];

        if (vTelnetStates[vTelnetSession] != SM_AUTHENTICATED) {
            continue;
		}

        TCPDisconnect(MySocket);
        TelnetState = SM_PRINT_LOGIN;
        vTelnetStates[vTelnetSession] = TelnetState;
    }
}

#endif	//#if defined(STACK_USE_TELNET_SERVER)
