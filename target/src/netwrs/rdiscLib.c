/* rdiscLib.c - ICMP router discovery server library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h" 


/*
modification history
--------------------
01a,29mar01,spm  file creation: copied from version 01g of tor2_0.open_stack
                 branch (wpwr VOB) for unified code base
*/

/*
DESCRIPTION

rdiscLib contains code to implement ICMP Router Discovery. This feature
allows routers to advertise an address to the hosts on each of the routers
interfaces.  This address is placed by the host into its route table as
a default router.  A host may also solicit the address by multicasting
the request to the ALL_ROUTERS address (224.0.0.2), to which a router
would respond with a unicast version of the advertisement.


There are three routines in this implementation of router discovery:
rdiscInit(), rdisc() and rdCtl().  rdiscInit() is the initialization
routine, rdisc() handles the periodic transmission of advertisements
and processing of solicitations, and rdCtl() sets/gets user parameters.

*/

 
/* includes */
#include "vxWorks.h"
#include "rdiscLib.h"
#include "stdioLib.h"
#include "netLib.h"
#include "inetLib.h"
#include "sockLib.h"
#include "taskLib.h"
#include "routeLib.h"
#include "wdLib.h"
#include "vxLib.h"   
#include "inetLib.h"
#include "string.h"
#include "logLib.h"
#include "sysLib.h"
#include "ioLib.h"
#include "math.h"
#include "netinet/ip.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip_icmp.h"
#include "netinet/in_var.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRdisc.h"
#endif /* VIRTUAL_STACK */

/* defines */

#define INITIAL_DELAY		600
#define RDISC_PACKET_SIZE	16


/* Defaults for task information */
#define RDISC_TASK_PRIORITY 127
#define RDISC_TASK_OPTIONS 0
#define RDISC_STACK_SIZE 20000

/* RFC 1256 values for each interface */
struct ifrd
    {
    char ifName[8];
    struct in_addr NetAddress;
    struct in_addr AdvertAddress;
    int subnet;
    int mask;
    int AdvertLifetime;
    int Advertise;
    int PrefLevel;
    };
    
struct rdiscPacket
    {
    char type;
    char code;
    short checkSum;
    char numAddrs;
    char entrySize;
    short lifeTime;
    unsigned int rAddr;
    unsigned int pref;
    };


/* globals */
#ifndef VIRTUAL_STACK
IMPORT struct ifnet     *ifnet;         /* list of all network interfaces */
IMPORT struct in_ifaddr    *in_ifaddr;
SEM_ID rdiscIfSem;
#endif  /* VIRTUAL_STACK */

/* locals */
#ifndef VIRTUAL_STACK
LOCAL WDOG_ID wdId;
LOCAL int rdiscSock;
LOCAL int terminateFlag = 0;
LOCAL BOOL debugFlag = 0;
LOCAL int rdiscNumInterfaces=0;
LOCAL struct ifrd *pIfDisc=0;
LOCAL int MaxAdvertInterval;
LOCAL int MinAdvertInterval;
#endif /* VIRTUAL_STACK */

/* forward declarations */
#ifdef VIRTUAL_STACK
void rdiscTimerEvent (int);
#endif /* VIRTUAL_STACK */
LOCAL void startWdTimer();

/******************************************************************************
*
* rdiscLibInit - Initialize router discovery
*
* This routine links the ICMP Router Discovery facility into the VxWorks system.
* The arguments are the task's priority, options and stackSize.
*
* Returns: N/A
*
*/

void rdiscLibInit
    (
    int priority, /* Priority of router discovery task. */
    int options, /* Options to taskSpawn(1) for router discovery task. */
    int stackSize /* Stack size for router discovery task. */
    )
    {

    taskSpawn ("tRdisc", priority, options, stackSize,
               (FUNCPTR) rdisc, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    }


/*******************************************************************************
*
* rdiscInit - initialize the ICMP router discovery function
*
* This routine allocates resources for the router discovery function. Since it
* called in the rdisc() routine, it should not be called subsequently 
*
* Returns: OK on successful initialization, ERROR otherwise
*
*/

STATUS rdiscInit()
    {

#ifdef VIRTUAL_STACK
    terminateFlag = 0;
    debugFlag = 0;
    rdiscNumInterfaces=0;
    pIfDisc=0;
#endif

    /* Create Locking semaphore so we can lock our own interface list. */
    if ((rdiscIfSem = semBCreate (SEM_Q_PRIORITY, SEM_FULL)) == NULL)
        {
        logMsg ("rdisc could not create if locking semaphore\n",
                0, 0, 0, 0, 0, 0);
        return (ERROR);
        }
          
    /* create RAW socket */
    rdiscSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (rdiscSock < 0)
        {
        logMsg("could not get raw socket\n",0,0,0,0,0,0);
        semDelete (rdiscIfSem);
        return ERROR;
        }

    pIfDisc = NULL;

    if (rdiscIfReset() != OK)
        {
        close (rdiscSock);
        semDelete (rdiscIfSem);
        return (ERROR);
        }

    terminateFlag = 0;
    return OK;
    }

/*******************************************************************************
*
* sendAdvert - send an advertisement to one location
*
* This routine sends a router advertisement using the data stored for its'
* corresponding interface.  Only the primary network address of the interface
* is used as the advertised router address.
*
* Returns: NA
*
*/


void sendAdvert
    (
    int index,
    struct in_addr dstAddr
    )

    {
    struct rdiscPacket *pRdis = NULL;
    struct sockaddr_in to;
    int status;
    char sndbuf[RDISC_PACKET_SIZE];

    if (debugFlag == TRUE)
        logMsg("advertising %s to %s\n", 
              (int)inet_ntoa((pIfDisc[index].AdvertAddress)), 
		(int)inet_ntoa(dstAddr),0,0,0,0);

    /* check Advertise enabled flag */
    if (pIfDisc[index].Advertise == 0)
	return;

    bzero (sndbuf, sizeof (sndbuf));

    /* form an advertisement packet */
    pRdis = (struct rdiscPacket*)sndbuf;
    pRdis->type = ICMP_ROUTERADVERT;
    pRdis->code = 0;
    pRdis->numAddrs = 1;
    pRdis->entrySize = 2;
    pRdis->lifeTime = htons(pIfDisc[index].AdvertLifetime);
    pRdis->rAddr = (pIfDisc[index].NetAddress.s_addr); 
    pRdis->pref = htonl(pIfDisc[index].PrefLevel); 
    pRdis->checkSum = 0;
    pRdis->checkSum = checksum((u_short *) pRdis, RDISC_PACKET_SIZE);

    bzero ((char *)&to, sizeof (to));
    to.sin_family = AF_INET;
    to.sin_len = sizeof(struct sockaddr_in);
    to.sin_addr.s_addr = (dstAddr.s_addr);  /* advertisement destination */ 

    status = sendto(rdiscSock, sndbuf, sizeof(struct rdiscPacket),
                    0, (struct sockaddr *)&to, sizeof(to));
    if (status != sizeof(struct rdiscPacket))
        {
        logMsg("rdisc: corrupt packet, errno = %d\n", 
		errno,0,0,0,0,0);
        }
    } 

/*******************************************************************************
*
* sendAdvertAll - send an advertisement to all active locations
*
* This routine sends a router advertisement using the data stored for each
* corresponding interface.  Only the primary network address of the interface
* is used as the advertised router address.
*
* Returns: NA
*
*/

void sendAdvertAll()
    {
    int i=0;
    struct in_addr ia;

    /* Make sure the interface list is not being updated. */
    semTake (rdiscIfSem, WAIT_FOREVER);

    for(i=0; i<rdiscNumInterfaces; i++)
	{

        if (pIfDisc[i].Advertise == 0)
	    continue; 

        ia.s_addr = pIfDisc[i].NetAddress.s_addr;   
	setsockopt(rdiscSock, IPPROTO_IP, IP_MULTICAST_IF, 
		   (char *)&ia, sizeof(struct in_addr));

	/* multicast to the selected network */ 
        sendAdvert(i, pIfDisc[i].AdvertAddress);

        }

    semGive (rdiscIfSem);

    }

/*******************************************************************************
*
* rdiscTimerEvent - called after watchdog timeout
*
* This routine is called when a new advertisement is to be sent. 
*
* Returns: NA
*
*/

#ifdef VIRTUAL_STACK
void rdiscTimerEventRestart
    (
    int stackNum
    )
    {
    netJobAdd ((FUNCPTR)rdiscTimerEvent, stackNum, 0, 0, 0, 0);
    }

void rdiscTimerEvent
    (
    int stackNum
    )
    {
    virtualStackNumTaskIdSet(stackNum);
#else
void rdiscTimerEvent()
    {
#endif /* VIRTUAL_STACK */

    sendAdvertAll();
   
    if (terminateFlag != 0)
	return;

    startWdTimer();
    }



/*******************************************************************************
*
* rdisc - implement the ICMP router discovery function
*
* This routine is the entry point for the router discovery function.  It
* allocates and initializes resources, listens for solicitation messages 
* on the ALL_ROUTERS (224.0.0.1) multicast address and processes the
* messages.
*
* This routine usually runs until explicitly killed by a system operator, but
* can also be terminated cleanly (see rdCtl() routine).
*
* Returns: NA
*
*/

void rdisc ()
    {

    int  i;
    int status;
    char recvBuff[128];
    struct sockaddr from;
    int FromLen;
    struct ip *pIp;
    struct rdiscPacket *rdp;
    short checkSum;

    status = rdiscInit();
    if (status == ERROR)
        {
        logMsg("rdisc: could not initialize router discovery\n", 0,0,0,0,0,0);
	return;
        }

    /* receive buffer IP packet */
    pIp = (struct ip * )recvBuff;

    /* receive buffer ICMP route discovery packet */
    rdp = (struct rdiscPacket *)&recvBuff[20];

    /* create WatchDog timer */
    wdId = wdCreate();

    sendAdvertAll();

#ifdef VIRTUAL_STACK
    status = wdStart(wdId, INITIAL_DELAY, (FUNCPTR) rdiscTimerEventRestart,
                     (int) myStackNum);
#else
    status = wdStart(wdId, INITIAL_DELAY, netJobAdd, (int)rdiscTimerEvent );
#endif /* VIRTUAL_STACK */
    if (status == ERROR) 
	{
	logMsg("rdisc: error starting watchdog timer: %d\n", errno,0,0,0,0,0);
        }

    while (terminateFlag == 0)
	{

        if (debugFlag == TRUE)
	    logMsg("rdisc: Waiting for solicitation...\n",0,0,0,0,0,0);

        status = recvfrom(rdiscSock, recvBuff, 100, 0,
			 (struct sockaddr *)&from, &FromLen);
        if (status < 0)
	    {
            logMsg("error in recvfrom returns %d, errno is %d exiting\n", 
			status, errno, 0,0,0,0);
            rdCtl (NULL, SET_MODE, (void*)MODE_STOP);
            return;
            }

	if (pIp->ip_len < 8)   /* verify IP packet length */
	    {
            logMsg("rdisc: packet too short, %d bytes\n", 
                   pIp->ip_len * 4, 0,0,0,0,0);
	    continue;
	    }

	if (rdp->code != 0) /* ICMP code */
	    {
            logMsg("rdisc: wrong code: %d\n", rdp->code, 0,0,0,0,0);
	    continue;
	    }

        if (rdp->type == 10) /* solicitation message */
	    {
	    /* NEED to check source address here per RFC */

            if (debugFlag == TRUE)
		logMsg("rdisc: received solicitation from src addr is %s\n", 
		        (int)inet_ntoa(pIp->ip_src), 0,0,0,0,0);
    
	    /* verify checksum */
            checkSum = rdp->checkSum;
            rdp->checkSum = 0;

    	    if ((u_short)checkSum != (u_short)checksum((u_short *) rdp, 8))
		{
	        logMsg ("checksum error: %d != %d\n" , (u_short)checkSum , 
				(u_short)checksum((u_short *) rdp,8), 0,0,0,0);
	        continue;
	        }

            /* Check the interface list lock. */
            semTake (rdiscIfSem, WAIT_FOREVER);
	    /* find the matching interface */
	    for(i = 0; i < rdiscNumInterfaces; i++)
                {
		if (pIfDisc[i].subnet == (htonl(pIp->ip_src.s_addr) & 
			pIfDisc[i].mask))
		   break;
		}

            /* Check for a match. */
            if (i < rdiscNumInterfaces)
                sendAdvert(i, pIp->ip_src);

            semGive (rdiscIfSem);

            }
        else
            {
            if (rdp->type == 9)
                {
                if (debugFlag == TRUE)
                    logMsg("received router advertisement from %s\n", 
                           (int)inet_ntoa(pIp->ip_src),0,0,0,0,0); 
                }
            }
        }
    
    if (pIfDisc != NULL)
        free((char *)pIfDisc);
    wdCancel(wdId);
    close(rdiscSock);
    }

/*******************************************************************************
*
* restartWdTimer - called at various place when the rdisc timer has to be
*    (re-) started
*
* This routine calculates the new interval and starts the rdisc timer.
*
* Returns: NA
*
*/


LOCAL void startWdTimer()
    {
    int interval = MinAdvertInterval+
                (rand()%(MaxAdvertInterval-MinAdvertInterval));
    interval *= sysClkRateGet();

#ifdef VIRTUAL_STACK
    wdStart(wdId, interval, (FUNCPTR)rdiscTimerEventRestart, (int) myStackNum);
#else
    wdStart(wdId, interval, netJobAdd, (int)rdiscTimerEvent);
#endif /* VIRTUAL_STACK */
    }

/*******************************************************************************
*
* searchInterface - search interface in local database
*
* Is called in rdCtl to verify the input parameter
*
* Returns: pointer to valid interface if successfull, else NULL
*
*/

LOCAL struct ifrd* searchInterface(const char* ifName){
    struct ifrd* t = pIfDisc;
    int index;
    
    /* find matching RD interface */ 
    if (NULL == ifName){
    	logMsg("rdCtl: no interface specified\n",1,2,3,4,5,6);
	return NULL;
    }
    for( index=0; index < rdiscNumInterfaces ; index++)
        {
        if( strcmp(t[index].ifName, ifName) == 0)
                break;
        }
    if (index == rdiscNumInterfaces)
        {
	logMsg("rdCtl: could not find interface %s\n", (int)ifName,2,3,4,5,6);
        return NULL;
        }
    return &t[index];    
    }

/*******************************************************************************
*
* returnFromRdCtl - central return from rdCtl function
*
* gives the central semaphore before returning from rdCtl
*
* Returns: STATUS which is passed is parameter
*
*/

LOCAL STATUS returnFromRdCtl(STATUS retVal)
    {
    semGive (rdiscIfSem);
    return retVal;
    }

/*******************************************************************************
*
* rdCtl - implement the ICMP router discovery control function 
*
* This routine allows a user to get and set router discovery parameters, and
* control the mode of operation.  
*
* OPTIONS
* The following section discuss the various flags that may be passed
* to rdCtl().
*
* .SS "SET_MODE -- Set debug mode or exit router discovery"
* This flag does not require an <interface> to be specified
* it is best to specify NULL.
*
* This flag is used in conjunction with the following values:
*
* .SS "MODE_DEBUG_ON -- Turn debugging messages on. "
* 
* .CS
*     rdCtl (NULL, SET_MODE, MODE_DEBUG_ON);
* .CE
*
* .SS "MODE_DEBUG_OFF -- Turn debugging messages off."
*
* .CS
*     rdCtl (NULL, SET_MODE, MODE_DEBUG_OFF);
* .CE
*
* .SS "MODE_STOP -- Exit from router discovery."
*
* .CS
*     rdCtl (NULL, SET_MODE, MODE_STOP);
* .CE
*
* .SS "SET_MIN_ADVERT_INT -- set minimum advertisement interval in seconds"
* Specify the minimum time between advertisements in seconds.  The minimum
* value allowed is 4 seconds, the maximum is 1800.
*
* .CS
*     rdCtl (NULL, SET_MIN_ADVERT_INT, <seconds>);
* .CE
*
* .SS "SET_MAX_ADVERT_INT -- set maximum advertisement interval in seconds"
* Specify the maximum time between advertisements in seconds.  The minimum
* value allowed is 4 seconds, the maximum is 1800.
*
* .CS
*     rdCtl (NULL, SET_MAX_ADVERT_INT, <seconds>);
* .CE
*
* .SS "SET_FLAG -- Set whether advertisements are sent on an interface."
* If this flag is 1 then advertisements are sent on this interface.  
* If it is 0 then they are not.
*
* .CS
*    rdCtl (<interface>, SET_FLAG, <0 or 1>);
* .CE
*
* .SS "SET_ADVERT_ADDRESS -- Set the IP address to which advertisements are sent. "
*  Set the multicast IP address to which advertisements are sent.
*
* .CS
*    rdCtl (<interface>, SET_ADVERT_ADDRESS, <multicast address>);
* .CE
*
* .SS "SET_ADVERT_LIFETIME -- Set the lifetime for advertisements in seconds. "
* Set the lifetime in seconds to be contained in each advertisement.
*
* .CS
*    rdCtl (<interface>, SET_ADVERT_LIFETIME, seconds);
* .CE
*
* .SS "SET_ADVERT_PREF -- Set the preference level contained in advertisements."
* Set the preference level contained in advertisements.
*
* .CS
*    rdCtl (<interface>, SET_ADVERT_PREF, value);
* .CE
*
* .SS "GET_MIN_ADVERT_INT -- Get the minimum advertisement interval. "
* .CS
*    rdCtl (NULL, GET_MIN_ADVERT_INT, &value);
* .CE
*
* .SS "GET_MAX_ADVERT_INT -- Get the maximum advertisement interval. "
* .CS
*    rdCtl (NULL, GET_MAX_ADVERT_INT, &value);
* .CE
*
* .SS "GET_FLAG -- Get the flag on an interface.. "
* .CS
*    rdCtl (<interface>, GET_FLAG, &value);
* .CE
*
* .SS "GET_ADVERT_ADDRESS -- Get the advertisement address for an interface. "
* .CS
*    rdCtl (<interface>, GET_ADVERT_ADDRESS, &value);
* .CE
*
* .SS "GET_ADVERT_LIFETIME -- Get the advertisement lifetime. "
* .CS
*    rdCtl (<interface>, GET_ADVERT_LIFETIME, &value);
* .CE
*
* .SS "GET_ADVERT_PREF -- Get the advertisement preference. "
* .CS
*    rdCtl (<interface>, GET_ADVERT_PREF, value);
* .CE
*
* Returns: OK on success, ERROR on failure
*
*/

STATUS rdCtl
    (
    char *ifName, 
    int cmd, 
    void* value		/* my be an int (set-cmds) or an int* (get-cmds) */
    )

    {
    
    /* Check the interface list lock. */
    semTake (rdiscIfSem, WAIT_FOREVER);

    /* execute command */
    
    switch(cmd)
	{
        
	case SET_MODE:
            if ((int)value == MODE_DEBUG_OFF)
                {
                logMsg("rdisc: debug mode off\n",0,0,0,0,0,0);
                debugFlag = 0;
                }
            else
                {
		if ((int)value == MODE_DEBUG_ON)
		    {
		    logMsg("rdisc: debug mode on\n",0,0,0,0,0,0);
		    debugFlag = 1;
		    }
		else 
                    {
                    if ((int)value == MODE_STOP)
                        {
			int i;
                        logMsg("rdisc: received termination message\n",0,0,0,0,0,0);
                        semGive (rdiscIfSem);
                        terminateFlag = 1;
                        for(i=0; i<rdiscNumInterfaces; i++)
                            pIfDisc[i].AdvertLifetime = 0;
                        sendAdvertAll(); /* recvfrom is blocked */
                        }
                    }
                }
            break;
            
	case SET_MIN_ADVERT_INT:
	    {
	    int numSeconds = (int)value;
            if ((numSeconds < 4) || 
                (numSeconds > 1800) || 
                (numSeconds >= MaxAdvertInterval))
                {
                return returnFromRdCtl(ERROR);
                }
            MinAdvertInterval = numSeconds;
            
            wdCancel(wdId);
            startWdTimer();
	    }
            break;
            
	case SET_MAX_ADVERT_INT:
	    {
	    int numSeconds = (int)value;
            if ((numSeconds < 4) || 
                (numSeconds > 1800) || 
                (numSeconds <= MinAdvertInterval))
                {
                return returnFromRdCtl(ERROR);
                }
            MaxAdvertInterval = numSeconds;
            
            wdCancel(wdId);
            startWdTimer();
	    }
            break;
            
	case SET_FLAG:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            interface->Advertise = (int)value ? 1:0 ;
	    }
            break;	
            
	case SET_ADVERT_ADDRESS:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            if ((int)value == INADDR_BROADCAST)
                {
                if (debugFlag == TRUE)
                    logMsg ("rdisc broadcast not yet supported%d\n",
                            1, 2, 3, 4, 5, 6);
                return returnFromRdCtl(ERROR);
                }
            interface->AdvertAddress.s_addr = (int)value;
	    }
            break;	
            
	case SET_ADVERT_LIFETIME:
	    {
	    int numSeconds = (int)value;
	    struct ifrd* interface = searchInterface(ifName);
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            if ((numSeconds < MaxAdvertInterval) ||
                (numSeconds > 9000))
                {
                return returnFromRdCtl(ERROR);
                }
            interface->AdvertLifetime = numSeconds;
	    }
            break;	
            
	case SET_ADVERT_PREF:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            interface->PrefLevel = (int)value;
	    }
            break;	
            
        case GET_MIN_ADVERT_INT:
	    {
	    int* intPtr = (int *)value;
            *intPtr = MinAdvertInterval ;
	    }
            break;
            
        case GET_MAX_ADVERT_INT:
	    {
	    int* intPtr = (int *)value;
            *intPtr = MaxAdvertInterval;
	    }
            break;
            
        case GET_FLAG:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    int* intPtr = (int *)value;
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            *intPtr = interface->Advertise;
	    }
            break;
            
        case GET_ADVERT_ADDRESS:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    int* intPtr = (int *)value;
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            *intPtr = interface->AdvertAddress.s_addr;
	    }
            break;
            
        case GET_ADVERT_LIFETIME:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    int* intPtr = (int *)value;
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            *intPtr = interface->AdvertLifetime;
	    }
            break;
            
        case GET_ADVERT_PREF:
	    {
	    struct ifrd* interface = searchInterface(ifName);
	    int* intPtr = (int *)value;
	    if (0 == interface)
	    	return returnFromRdCtl(ERROR);
            *intPtr = interface->PrefLevel;
	    }
            break;
            
	default:
	    logMsg("rdCtl: illegal cmd %d\n",cmd,2,3,4,5,6);
            return returnFromRdCtl(ERROR);
        }
    
    return returnFromRdCtl(OK) ;
    
    }

/******************************************************************************
*
* rdiscIfReset - check for new or removed interfaces for router discovery
*
* This routine MUST be called any time an interface is added to or removed
* from the system so that the router discovery code can deal with this
* case.  Failure to do so will cause the sending of packets on missing
* interfaces to fail as well as no transmission of packets on new interfaces.
*
*/

STATUS rdiscIfReset ()
    {
    
    struct in_ifaddr *ia = NULL;
    int i;
    int status;
    struct ifnet *ifp=NULL;
    struct ifaddr *ifa=NULL;
    char *t=NULL;
    struct sockaddr_in *tt=0;
    struct ip_mreq ipMreq;
    struct ifrd *pTmp=NULL;

    /* Take the lock. */
    semTake (rdiscIfSem, WAIT_FOREVER);
    
    /* If we had an old list then free it. */
    if (pIfDisc != NULL)
        {

        /* First drop the multicast stuff on all interfaces. */
        for(i = 0; i < rdiscNumInterfaces; i++)
            {
            pTmp = &pIfDisc[i];
            /* drop multicast membership for this IF */
            ipMreq.imr_multiaddr.s_addr = htonl(0xe0000002); 
            ipMreq.imr_interface.s_addr = pTmp->NetAddress.s_addr; 
            /*
             * The reason we DON'T check the return status here
             * is that if we did and the interface has been removed
             * then we what would we do?  This is a best effort
             * attempt to do remove the multicast address from
             * all UP interfaces, an interface that is missing
             * will simply return ERROR which is OK in this case.
             */
            setsockopt(rdiscSock, IPPROTO_IP,IP_DROP_MEMBERSHIP, 
                       (char *)&ipMreq, sizeof(ipMreq));
            }
        free (pIfDisc);
        }
    /* count interfaces for discovery */ 
#ifdef VIRTUAL_STACK
    for(ifp = _ifnet, i=0; ifp != 0; ifp = ifp->if_next)
#else
    for(ifp = ifnet, i=0; ifp != 0; ifp = ifp->if_next)
#endif
        {
	/* reject unwanted interfaces */
        if (!(ifp->if_flags & IFF_MULTICAST)) 
            continue;

	if (strcmp(ifp->if_name, "lo") == 0) 
	   continue;

	i++;
        }

    rdiscNumInterfaces = i;
    t = malloc(sizeof(struct ifrd) * rdiscNumInterfaces);
    if (t == NULL)
        {
        logMsg("rdiscIfReset: error allocating memory\n",0,0,0,0,0,0);
        semGive (rdiscIfSem);
        rdiscNumInterfaces = 0; /* So rdCtl won't walk an empty list. */
        rdCtl (NULL, SET_MODE, (void*)MODE_STOP);
	return ERROR;
	}

    bzero(t, sizeof(struct ifrd) * rdiscNumInterfaces);

    /* set global to head of list */
    pIfDisc = (struct ifrd *)t;

    /* set each interface for discovery */ 
#ifdef VIRTUAL_STACK
    for(ifp = _ifnet, i=0; ifp != 0; ifp = ifp->if_next)
#else
    for(ifp = ifnet, i=0; ifp != 0; ifp = ifp->if_next)
#endif
	{

	/* reject unwanted interfaces */
        if (!(ifp->if_flags & IFF_MULTICAST)) 
            continue;

	if (strcmp(ifp->if_name, "lo") == 0) 
	   continue;

        for (ifa = ifp->if_addrlist;  ifa != NULL;  ifa = ifa->ifa_next)
            {
            if (ifa->ifa_addr->sa_family == AF_INET)
                {
		tt = (struct sockaddr_in *)ifa->ifa_addr;
		break;
                }
            }

	/* find subnetmask */
#ifdef VIRTUAL_STACK
	for (ia = _in_ifaddr;  ia != NULL;  ia = ia->ia_next)
#else
	for (ia = in_ifaddr;  ia != NULL;  ia = ia->ia_next)
#endif
	   {
	      if (ia->ia_ifp == ifp)
		   break;
	   }

	/* fill in router discovery params for this IF */
        pTmp = &pIfDisc[i];
	sprintf(pTmp->ifName,"%s%d", ifp->if_name, ifp->if_unit);
        pTmp->NetAddress.s_addr = tt->sin_addr.s_addr;
        pTmp->AdvertAddress.s_addr = htonl(0xe0000001); 
        pTmp->subnet = ia->ia_subnet;
        pTmp->mask = ia->ia_subnetmask;
        pTmp->AdvertLifetime = 1800;
        pTmp->Advertise = 1;
        pTmp->PrefLevel = 0; 

	/* add multicast membership for this IF */
        ipMreq.imr_multiaddr.s_addr = htonl(0xe0000002); 
        ipMreq.imr_interface.s_addr = pTmp->NetAddress.s_addr; 
        status = setsockopt(rdiscSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
            (char *)&ipMreq, sizeof(ipMreq));
        if (status == ERROR)
	    {
                logMsg("rdisc: could not join multicast group, errno is %d\n", 
			errno,0,0,0,0,0);
                semGive (rdiscIfSem);
                rdiscNumInterfaces = 0; /* So rdCtl won't walk a bad list. */
                rdCtl (NULL, SET_MODE, (void*)MODE_STOP);
		return ERROR;
            } 
	
	MaxAdvertInterval = 600; /* max interval in seconds */
	MinAdvertInterval = 450; /* max interval in seconds */

	i++;
        }

    /* create random number stream per IF address */
    srand((uint_t)pTmp->NetAddress.s_addr);

    semGive (rdiscIfSem);
    
    return (OK);
    }
