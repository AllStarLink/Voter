#define THIS_IS_STACK_APPLICATION
#include <string.h>
#include "projdefs.h"
#include "debug.h"
#include "serint.h"
#include "sercfg.h"
//#include "appcfg.h"
#include "httpexec.h"

#include "net\cpuconf.h"
#include "net\stacktsk.h"
#include "net\fsee.h"
#include "net\tick.h"
#include "net\helpers.h"
 

//**ADD
#include "net\udp.h"
#if defined(STACK_USE_DHCP)
#include "net\dhcp.h"
#endif
#if defined(STACK_USE_HTTP_SERVER)
#include "net\http.h"
#endif
#if defined(STACK_USE_FTP_SERVER)
#include "net\ftp.h"
#endif
#if defined(STACK_USE_ANNOUNCE)
#include "net\announce.h"
#endif
#if defined(STACK_USE_NBNS)
#include "net\nbns.h"
#endif

/////////////////////////////////////////////////
//Debug defines
#define debugPutMsg(msgCode) debugPut2Bytes(0xD6, msgCode)
#define debugPutMsgRomStr(msgCode, strStr) debugMsgRomStr(0xD6, msgCode, msgStr)

/////////////////////////////////////////////////
//Version number
// - n = major part, and can be 1 or 2 digets long
// - mm is minor part, and must be 2 digets long!
ROM char APP_VER_STR[] = "V3.05";

BYTE myDHCPBindCount = 0;
#if defined(STACK_USE_DHCP)
    extern BYTE DHCPBindCount;
#else
     //If DHCP is not enabled, force DHCP update.
    #define DHCPBindCount 1
#endif
//TODO Remove this later
#if defined(APP_USE_ADC8)
extern BYTE AdcValues[ADC_CHANNELS];
#elif defined(APP_USE_ADC10)
extern WORD AdcValues[ADC_CHANNELS];
#endif
/*
static union
{
    struct
    {
        unsigned int bFreezeADCBuf : 1;     //Stop updating AdcValues[] ADC buffer
    } bits;
    BYTE Val;
} mainFlags;
*/
//** ADD Create a UDP socket for receiving and sending data
static UDP_SOCKET udpSocketUser = INVALID_UDP_SOCKET;

static void InitializeBoard(void);
static void ProcessIO(void);

/////////////////////////////////////////////////
//High Interrupt ISR
#if defined(MCHP_C18)
    #pragma interrupt HighISR
    void HighISR(void)
#elif defined(HITECH_C18)
    #if defined(STACK_USE_SLIP)
        extern void MACISR(void);
    #endif
    void interrupt HighISR(void)
#endif
{
    //TMR0 is used for the ticks
    if (INTCON_TMR0IF)
    {
        TickUpdate();
        #if defined(STACK_USE_SLIP)
        MACISR();
        #endif
        INTCON_TMR0IF = 0;
    }

    #ifdef SER_USE_INTERRUPT
    //USART Receive
    if(PIR1_RCIF && PIE1_RCIE)
    {
        serRxIsr();
    }
    //USART Transmit
    if(PIR1_TXIF && PIE1_TXIE)
    {
        serTxIsr();
    }
    #endif
}
#if defined(MCHP_C18)
#pragma code highVector=HIVECTOR_ADR
void HighVector (void)
{
    _asm goto HighISR _endasm
}
#pragma code /* return to default code section */
#endif

/*
 * High Priority User process. Place all high priority code that has to be called often
 * in this function.
 *
 * !!!!! IMPORTANT !!!!!
 * This function is called often, and should be very fast!
 */
void highPriorityProcess(void)
{
    CLRWDT();
}

/*
 * Main entry point.
 */
void main(void)
{
    static TICK8 t = 0;
//**ADD
 char c;
    NODE_INFO udpServerNode;

    /*
     * Initialize any application specific hardware.
     */
    InitializeBoard();
    /*
     * Initialize all stack related components.
     * Following steps must be performed for all applications using
     * PICmicro TCP/IP Stack.
     */
    TickInit();
    /*
     * Initialize file system.
     */
    fsysInit();
    //Intialize HTTP Execution unit
    htpexecInit();
    //Initialze serial port
    serInit();
    
    /*
     * Initialize Stack and application related NV variables.
     */
    appcfgInit();
    appcfgUSART();              //Configure the USART
    #ifdef SER_USE_INTERRUPT    //Interrupt enabled serial ports have to be enabled
    serEnable();
    #endif
    appcfgCpuIO();          //Configure the CPU's I/O port pin directions - input or output
    appcfgCpuIOValues();    //Configure the CPU's I/O port pin default values
    appcfgADC();            //Configure ADC unit
    //Serial configuration menu - display it for configured time and allow user to enter configuration menu
    scfInit(appcfgGetc(APPCFG_STARTUP_SER_DLY));
    StackInit();

//**ADD
    /////////////////////////////////////////////////
    //Initialize UDP socket
    //Initialize remote IP and MAC address of udpServerNode with 0, seeing that we don't know them for the node
    //that will send us an UDP message. The first time a message is received addressed to this port, the
    //remote IP and MAC addresses are automatically updated with the addresses of the remote node.
    memclr(&udpServerNode, sizeof(udpServerNode));
    //Configure for local port 54123 and remote port INVALID_UDP_PORT. This opens the socket to
    //listed on the given port.
    udpSocketUser = UDPOpen(54123, &udpServerNode, INVALID_UDP_PORT);
    
    //An error occurred during the UDPOpen() function
    if (udpSocketUser == INVALID_UDP_SOCKET) {
        //Add user code here to take action if required!
    }
#if defined(STACK_USE_HTTP_SERVER)
    HTTPInit();
#endif
#if defined(STACK_USE_FTP_SERVER)
    FTPInit();
#endif

#if defined(STACK_USE_DHCP) || defined(STACK_USE_IP_GLEANING)
    DHCPReset();    //Initialize DHCP module
    
    //If DHCP is NOT enabled
    if ((appcfgGetc(APPCFG_NETFLAGS) & APPCFG_NETFLAGS_DHCP) == 0)
    {
        //Force IP address display update.
        myDHCPBindCount = 1;
        
        #if defined(STACK_USE_DHCP)
        DHCPDisable();
        #endif
    }
#endif
    #if (DEBUG_MAIN >= LOG_DEBUG)
        debugPutMsg(1); //@mxd:1:Starting main loop
    #endif

  

  

    /*
     * Once all items are initialized, go into infinite loop and let stack items execute
     * their tasks. If the application needs to perform its own task, it should be done at
     * the end of while loop. Note that this is a "co-operative mult-tasking" mechanism where
     * every task performs its tasks (whether all in one shot or part of it) and returns so
     * that other tasks can do their job. If a task needs very long time to do its job, it
     * must broken down into smaller pieces so that other tasks can have CPU time.
     */
    while(1)
    {
        //Blink SYSTEM LED every second.
     //   if (appcfgGetc(APPCFG_SYSFLAGS) & APPCFG_SYSFLAGS_BLINKB6) {
     //       if ( TickGetDiff8bit(t) >= ((TICK8)(TICKS_PER_SECOND/2)) )
     //       {
     //           t = TickGet8bit();
     //           TRISB_RB6 = 0;
     //           LATB6 ^= 1;
    //        }
    //    }

   //**ADD
/////////////////////////////////////////////////
        //Is there any data waiting for us on the UDP socket?
        //Because of the design of the Modtronix TCP/IP stack we have to consume all data sent to us as soon
        //as we detect it. Store all data to a buffer as soon as it is detected
        if (UDPIsGetReady(udpSocketUser)) {
            //We are only interrested in the first byte of the message.
            UDPGet(&c);
            
            if (c == '0') LATB6 = 0;    //Switch system LED off
            else if (c == '1') LATB6 = 1;    //Switch system LED on
            //Discard the socket buffer.
            UDPDiscard();
  
          } 
 

        //This task performs normal stack task including checking for incoming packet,
        //type of packet and calling appropriate stack entity to process it.
        StackTask();
#if defined(STACK_USE_HTTP_SERVER)
        //This is a TCP application.  It listens to TCP port 80
        //with one or more sockets and responds to remote requests.
        HTTPServer();
#endif
#if defined(STACK_USE_FTP_SERVER)
        FTPServer();
#endif
#if defined(STACK_USE_ANNOUNCE)
        DiscoveryTask();
#endif
#if defined(STACK_USE_NBNS)
        NBNSTask();
#endif
        //Add your application speicifc tasks here.
        ProcessIO();
        //For DHCP information, display how many times we have renewed the IP
        //configuration since last reset.
        if ( DHCPBindCount != myDHCPBindCount )
        {
            #if (DEBUG_MAIN >= LOG_INFO)
                debugPutMsg(2); //@mxd:2:DHCP Bind Count = %D
                debugPutByteHex(DHCPBindCount);
            #endif
            
            //Display new IP address
            #if (DEBUG_MAIN >= LOG_INFO)
                debugPutMsg(3); //@mxd:3:DHCP complete, IP = %D.%D.%D.%D
                debugPutByteHex(AppConfig.MyIPAddr.v[0]);
                debugPutByteHex(AppConfig.MyIPAddr.v[1]);
                debugPutByteHex(AppConfig.MyIPAddr.v[2]);
                debugPutByteHex(AppConfig.MyIPAddr.v[3]);
            #endif
            myDHCPBindCount = DHCPBindCount;
            
            #if defined(STACK_USE_ANNOUNCE)
                AnnounceIP();
            #endif             
        }
    }
}

static void ProcessIO(void)
{
//Convert next ADC channel, and store result in adcChannel array
#if (defined(APP_USE_ADC8) || defined(APP_USE_ADC10)) && (ADC_CHANNELS > 0)
    static BYTE adcChannel; //Current ADC channel. Value from 0 - n
    //We are allowed to update AdcValues[] buffer
    //if (!mainFlags.bits.bFreezeADCBuf)
    {
        //Increment to next ADC channel    
        if ((++adcChannel) >= ADC_CHANNELS)
        {
            adcChannel = 0;
        }
        //Check if current ADC channel (adcChannel) is configured to be ADC channel
        if (adcChannel < ((~ADCON1) & 0x0F))
        {
            //Convert next ADC Channel
            ADCON0 &= ~0x3C;
            ADCON0 |= (adcChannel << 2);
            ADCON0_ADON = 1;    //Switch on ADC
            ADCON0_GO = 1;      //Go
    
            while (ADCON0_GO) FAST_USER_PROCESS(); //Wait for conversion to finish
            #if defined(APP_USE_ADC8)
            AdcValues[adcChannel] = ADRESH;
            #elif defined(APP_USE_ADC10)
            AdcValues[adcChannel] = ((WORD)ADRESH << 8) || ADRESL;
            #endif
        }
        //Not ADC channel, set to 0
        else {
            AdcValues[adcChannel] = 0;
        }
    }
#endif
}
/////////////////////////////////////////////////
//Implement callback for FTPVerify() function
#if defined(STACK_USE_FTP_SERVER)
    #if (FTP_USER_NAME_LEN > APPCFG_USERNAME_LEN)
    #error "The FTP Username length can not be shorter then the APPCFG Username length!"
    #endif
BOOL FTPVerify(char *login, char *password)
{
    #if (DEBUG_MAIN >= LOG_INFO)
        debugPutMsg(4); //@mxd:4:Received FTP Login (%s) and Password (%s)
        debugPutString(login);
        debugPutString(password);
    #endif
    if (strcmpee2ram(login, APPCFG_USERNAME0) == 0) {
        if (strcmpee2ram(password, APPCFG_PASSWORD0) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}
#endif

/**
 * Initialize the boards hardware
 */
static void InitializeBoard(void)
{
    #if (defined(MCHP_C18) && (defined(__18F458) || defined(__18F448))) \
        || (defined(HITECH_C18) && (defined(_18F458) || defined(_18F448)))
        CMCON  = 0x07; /* Comparators off CM2:CM0 = 111 for PIC 18F448 & 18F458 */
    #endif

    /////////////////////////////////////////////////
    //Enable 4 x PLL if configuration bits are set to do so
    #if ( defined(MCHP_C18) && defined(__18F6621))
    OSCCON = 0x04;              //Enable PLL (PLLEN = 1)
    while (OSCCON_LOCK == 0);   //Wait until PLL is stable (LOCK = 1)
    
    //Seeing that this code does currently not work with Hi-Tech compiler, disable this feature
    OSCCON_SCS1 = 1;
    //Switch on software 4 x PLL if flag is set in configuration data
    //if (appcfgGetc(APPCFG_SYSFLAGS) & APPCFG_SYSFLAGS_PLLON) {
    //    OSCCON_SCS1 = 1;
    //}
    #endif
    
    //Disable external pull-ups
    INTCON2_RBPU = 1;
    //Enable interrupts
    T0CON = 0;
    INTCON_GIEH = 1;
    INTCON_GIEL = 1;
}
