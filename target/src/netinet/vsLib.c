/* vsLib.c - virtual stack management library */

/* Copyright 2000 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
02a,17jul01,ann  adding virtualStackNameGet/Set routines
01z,11jul01,jpf  Added a lot of minor refinements and virtualStackNumGet
01y,16apr01,jpf  Fixing this file to work with vs
01x,29mar01,spm  file creation: copied from version 01w of tor2_0.open_stack
                 branch (wpwr VOB) for unified code base; converted memory
                 allocation for protection domain restrictions
01w,21jul00,gnn  fixed it not to build when VIRTUAL_STACK is off
01v,23jun00,spm  used new routine for ARP initialization (from proxy ARP merge)
01u,12jun00,ta   add vsexec Shell command
01t,08jun00,ead  moved ARP initialization to arpInit in arpLib.c
01s,07jun00,ead  fixed a bug in virtualStackCreate that didn't check for the
                 stack name
01r,07jun00,ead  modifed vsMakeStack to return a STATUS result
01q,26may00,ead  added ipMaxUnits argument to virtualStackInit()
01p,25may00,pul  adding ffInit() to virtualStackInit()
01o,22may00,ead  modifed vsMakeStack
01n,22may00,ead  added llinfo_arp initialization in virtualStackInit
                 created the vsMakeStack function
01m,19may00,cn   changed all pName buffers to strings.
01l,17may00,cn   fixed bug in virtualStackNameTaskIdSet () and
                 virtualStackIdGet ().
01k,05may00,ann  adding some initializations for m2 libraries
01j,05may00,gnn  added virtualStackShow which shows ALL virtual stacks
01i,04may00,spm  fixed initialization for UDP and show routines
01h,01may00,ann  adding some initializations for sockets and UDP
01g,26apr00,spm  updated startup instructions; added routing storage handling
01f,26apr00,gnn  fixed strlen/strcpy counting bug
01e,25apr00,spm  separated initialization and creation of virtual stacks
01d,25apr00,gnn  added clearing of each vsTbl entry as it's created
                 added netLibInit() to initialize protocols
                 added domain handling code
                 fixed netPool handling code
01c,24apr00,gnn  added ifnet list handling
01b,23apr00,gnn  Added code to start up parts of the stack (splSemInit and
                 mbinit) to virtualStackCreate.
01a,09mar00,gnn	 written.
*/
 
/*
DESCRIPTION

This module implements the framework for Virtual TCP/IP Stacks.  Each stack is
represented by a BSD_GLOBAL_DATA structure defined in
target/h/netinet/vsData.h.  Please see that file for information on particular
elements of that data structure.

The system is an array of pointers to these structures stored in vsTbl and
indexed by an integer from 0 to VSID_MAX.  There is a special stack, referred
to as the management stack, which is defined as VS_MGMT_STACK.  This is
defined in target/h/netinet/vsLib.h.

For tasks calling into the stack we provide a current Virtual Stack
number concept via a task variable called myTaskNum.  This variable can be
gotten or set via a set of routines in this module.  Routines called in the
context of a task (though NEVER tNetTask) it is possible to reference elements
of the stack's virtual stack like this:

.CS

vsTbl[myTaskNum]->stackGlobalYouWantToLookAt

.CE

INCLUDE FILES: 

SEE ALSO: taskVarLib

*/

/*
 * This huge ifdef means that the file gets run through the compiler but is
 * empty in the case where we're building the stack without VIRTUAL_STACK
 * support.
 */

#ifdef VIRTUAL_STACK

/* includes */
#include "vxWorks.h"
#include "semLib.h"
#include "logLib.h"
#include "sysLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "arpLib.h"
#include "hostLib.h"
#include "usrConfig.h"
#include "taskLib.h"
#include "taskVarLib.h"
#include "netinet/vsLib.h"
#include "netinet/vsData.h"
#include "memPartLib.h"
#include "errnoLib.h"

/* defines */

/* globals */

BSD_GLOBAL_DATA* vsTbl[VSID_MAX];
int myStackNum; /* Task local stack number. */
SEM_ID vsTblLock;

/* locals */

/* forward declarations */

/*******************************************************************************
*
* virtualStackLibInit - initialize the Virtual Stack system
*
* This routine initializes all the global data required by the virtual stack
* system.
*
* RETURNS: OK or ERROR if any resources are unavailable.
*
*/

STATUS virtualStackLibInit
    (
    )
    {
    int count;

    /*
     * Create a semaphore to protect our table.
     */

    vsTblLock = semBCreate (SEM_Q_PRIORITY, SEM_FULL);

    if (vsTblLock == NULL)
        return (ERROR);
    
    for (count = 0; count < VSID_MAX; count++)
        {
        vsTbl[count] = NULL;
        }

    return (OK);
    }

/*******************************************************************************
*
* virtualStackCreate - creates an instance of a virtual stack
*
* This routine creates an instance of a Virtual Stack and returns a pointer
* to the VSID on successful completion.  Once this routine returns the
* desired initialization sequence may be called to start the appropriate
* protocols and initialize the network data structures.
*
* The <pName> argument supplies an optional name for the network stack.
* If none is provided, the stack index (i.e.  0 1 2 ...) is assigned.
*
* RETURNS: OK or ERROR. If ERROR is returned the <pVID> contents are 
* not valid.
*
*/

STATUS virtualStackCreate
    (
    char* pName, 	/* Unique stack name, or NULL for default */
    VSID* pVID 		/* Buffer for storing virtual stack identifier */
    )
    {
    int vsIndex, i;
    char tempName[VS_NAME_MAX + 1];
    
    /* Lock out access until creation is complete or error is returned. */
    semTake (vsTblLock, WAIT_FOREVER);

    /* Find the first empty slot. */

    for (vsIndex = 0; vsIndex < VSID_MAX; vsIndex++)
        {
        if (vsTbl[vsIndex] == NULL)
            break; /* Hey, we found one! */
        }

    if (vsIndex == VSID_MAX)
        {
        semGive (vsTblLock);
        return (ERROR);
        }

    /*
     * If no name is passed simply make the name the
     * VS number.
     */

    if (pName == NULL)
        sprintf (tempName, "%d", vsIndex);
    else
        strncpy (tempName, pName, min(VS_NAME_MAX, (strlen(pName) + 1)));

    /* null-terminate it, just in case... */

    tempName[VS_NAME_MAX] = EOS;

    /* Check that a stack with the same name doesn't already exist */
    for (i = 0; i < VSID_MAX; i++)
        {
        if (vsTbl[i] == NULL)  /* empty slot */
            continue;
        if (strcmp (vsTbl[i]->pName, tempName) == 0)
            {
            semGive (vsTblLock);
            return (ERROR);
            }
        }

    /* Allocate our global structure.  */

    vsTbl[vsIndex] = (BSD_GLOBAL_DATA *) KHEAP_ALLOC ((sizeof(BSD_GLOBAL_DATA)));

    *pVID = vsTbl[vsIndex]; 

    if (vsTbl[vsIndex] == NULL)
        {
        semGive (vsTblLock);
        return (ERROR);
        }

    /* Clear out the structure to NULL */
    bzero ((char *)*pVID, sizeof (BSD_GLOBAL_DATA));

    /* If no name is passed simply make the name the VS number. */

    vsTbl[vsIndex]->pName = (char *)&vsTbl[vsIndex]->name;

    bcopy (tempName, vsTbl[vsIndex]->pName, sizeof(tempName));

    semGive (vsTblLock);

    /*
     * Set the virtual stack number task variable.
     * This allows the initialization code to execute unchanged.
     */

    virtualStackNumTaskIdSet (vsIndex);

    return (OK);
    }

/*******************************************************************************
*
* virtualStackInit - initialize a virtual stack
*
* This routine initializes various data structures within the stack.
* Call this function only after calling virtualStackCreate (). Once
* virtualStackInit () has been executed, network protocols init functions
* can then be called to finish off the virtual stack initialization
* process.
*
* RETURNS: OK, or ERROR if initialization fails.
*/

STATUS virtualStackInit
    (
    VSID vsId,	  /* Stack identifier from virtualStackCreate() routine */
    int  maxUnits /* maximum number of device to be attached to IP */
    )
    {
    int count;
    int vsIndex;

    if (!vsId)
	{
	return (ERROR);
	}

     /* Verify stack identifier. Exit if not found. */

    if (virtualStackNumGet (vsId, &vsIndex) == ERROR)
	return (ERROR);

    /* Set the task variable and initialize the virtual stack. */

    virtualStackNumTaskIdSet (vsIndex);

     /* Allocate the ipDrvCtrl array based on the given maxUnits */

    ipMaxUnits = maxUnits ? maxUnits : 1;

    ipDrvCtrl = (IP_DRV_CTRL *)KHEAP_ALLOC(ipMaxUnits * sizeof(IP_DRV_CTRL));
    if ( ipDrvCtrl == (IP_DRV_CTRL *) NULL )
       return (ERROR);

    bzero ((char *)ipDrvCtrl,ipMaxUnits * sizeof(IP_DRV_CTRL));
    ipDrvCount = 0;
    ipDrvIndex = 0;

    /*
     * Network buffer initialization: Reset pointers to NULL before
     * calling mbinit() routine to force each stack to create separate
     * memory pools.
     */

    mClBlkConfig.memArea = NULL;
    for (count = 0; count < clDescTblNumEnt; count++)
        clDescTbl[count].memArea = NULL;
    sysMclBlkConfig.memArea = NULL;
    for (count = 0; count < sysClDescTblNumEnt; count++)
        sysClDescTbl[count].memArea = NULL;


    pM2IfRoot    = NULL;
    pM2IfRootPtr = &pM2IfRoot;

    /* Make sure that the list is empty on the first call. */
    _ifnet = NULL;
                                       
    /*
     * This code performs the same function as the static initialization
     * in the (no longer compiled) in_proto.c module. Since we don't get
     * auto-initialization from the compiler we have to do the work here.
     */
    
    inetdomain.dom_family = AF_INET;
    inetdomain.dom_name = KHEAP_ALLOC (strlen("internet") + 1);
    strcpy (inetdomain.dom_name, "internet");
    inetdomain.dom_init = 0;
    inetdomain.dom_externalize = 0;
    inetdomain.dom_dispose = 0;
    inetdomain.dom_protosw = inetsw;
    inetdomain.dom_protoswNPROTOSW =
        &inetsw[sizeof(inetsw)/sizeof(inetsw[0])];
    inetdomain.dom_next = NULL;
    inetdomain.dom_rtattach = rn_inithead;
    inetdomain.dom_rtoffset = 27;
    inetdomain.dom_maxrtkey = sizeof(struct sockaddr_in);

    /* Set up for the protocol initialization. */
    bzero ((caddr_t)&inetsw, sizeof(inetsw));

    _protoSwIndex = 0;
    sb_max = SB_MAX;

    /* Set up for domain calls. */
    domains = NULL;

    raw_sendspace = RAWSNDQ;
    raw_recvspace = RAWRCVQ;

    route_proto.sp_family = PF_ROUTE;

    return (OK);
    }

/*******************************************************************************
*
* virtualStackDelete - delete a virtual TCP/IP stack
*
* This routine deletes the virtual stack referenced by the VSID passed to it.
*
* RETURNS: OK or ERROR if the VSID passed to the call was in some way invalid
*
*/

STATUS virtualStackDelete
    (
    VSID vsid 				/* VID returned by virtualStackCreate */
    )
    {
    int vsIndex;
    
    semTake (vsTblLock, WAIT_FOREVER);
    
    for (vsIndex = 0; vsIndex < VSID_MAX; vsIndex++)
        if (vsTbl[vsIndex] == vsid)
            break; /* Hey, we found it! */

    /* Check that we did find it. */
    if (vsIndex == VSID_MAX)
        {
        semGive (vsTblLock);
        return (ERROR);
        }

    /* Free the structure. */
    KHEAP_FREE (vsTbl[vsIndex]);

    /* Let's be extra paranoid. */
    vsTbl[vsIndex] = NULL;
    
    semGive (vsTblLock);

    return (OK);

    }

/*******************************************************************************
*
* virtualStackIdGet - return the VSID named in pName
*
* This routine returns a VSID in the pointer passed to it in pVSID if a stack
* with pName is found in the Virtual Stack Table.
*
* RETURNS: OK or ERROR if the stack named in pName was not found
*
*/

STATUS virtualStackIdGet
    (
    char* pName, 			/* Name that was used in virtualStackCreate */
    VSID* pVSID
    )
    {
    int vsIndex;

    semTake (vsTblLock, WAIT_FOREVER);
    
    for (vsIndex = 0; vsIndex < VSID_MAX; vsIndex++)
        if (!strncmp (vsTbl[vsIndex]->pName, pName, 
		      min (strlen (pName), VS_NAME_MAX)))
            break; /* Hey, we found it! */

    /* Check that we did find it. */
    if (vsIndex == VSID_MAX)
        {
        semGive(vsTblLock);
        return (ERROR);
        }

    *pVSID = vsTbl[vsIndex];

    semGive (vsTblLock);

    return (OK);
    }

/*******************************************************************************
*
* virtualStackNameGet - return the name given the stack id
*
* This routine returns the stack name in the pointer passed to it in pVSName 
* if a stack with vsid is found in the Virtual Stack Table. This routine
* assumes that the caller has allocated the buffer area for the name.
*
* RETURNS: OK or ERROR if the stack id was not found
*
*/

STATUS virtualStackNameGet
    (
    char *    pName,        /* Buffer for the stack name */
    int       vsid
    )
    {
    int vsIndex;

    semTake (vsTblLock, WAIT_FOREVER);
    if ( (vsid < 0) || (vsid > VSID_MAX) )
        return (ERROR);

    strcpy (pName, vsTbl[vsid]->pName);
    semGive (vsTblLock);

    return (OK);
    }


/*******************************************************************************
*
* virtualStackInfo - print out some information about a Virtual Stack
*
* This routine takes the VSID in vsid and prints some information on
* the console about it.
*
* RETURNS: OK or ERROR if the VSID was somehow invalid
*
*/

void virtualStackInfo
    (
    VSID vsid
    )
    {
    int vsIndex;
    
    if (vsid == NULL)
        return;

    semTake (vsTblLock, WAIT_FOREVER);
    
    for (vsIndex = 0; vsIndex < VSID_MAX; vsIndex++)
        if (vsTbl[vsIndex] == vsid)
            break; /* Hey, we found it! */

    /* Check that we did find it. */
    if (vsIndex == VSID_MAX)
        {
        semGive(vsTblLock);
        return;
        }

    printf ("Index: %d \tName: %s\n", vsIndex,
           vsTbl[vsIndex]->pName);

    semGive(vsTblLock);

    }

/*******************************************************************************
*
* virtualStackNumGet - fetches the virtual stack number 
*
* This functions returns a valid stack number in <num> if it returns an OK.
*
* RETURNS: OK --- means that <num> points to a valid stack number;
*	   ERROR --- <num> is not valid and/or VSID is invalid;
*/

STATUS virtualStackNumGet
    (
    VSID vsid,
    int *num
    )
    {
    
    if ((vsid == NULL) && (num == NULL))
        return (ERROR);

    semTake (vsTblLock, WAIT_FOREVER);
    
    for (*num = 0; (*num < VSID_MAX) && (vsTbl[*num] != vsid); (*num)++);

    semGive(vsTblLock);

    if ( vsTbl[*num] != vsid )
	return (ERROR);

    return OK;

    }

/******************************************************************************
*
* virtualStackShow - print out the list of Virtual Stacks
*
* This routine goes through the list of Virtual Stacks and calls
* virtualStackInfo on each.
*
* RETURNS N/A
*
*/

void virtualStackShow ()
    {
    int vsIndex;
    
    for (vsIndex = 0; vsIndex < VSID_MAX; vsIndex++)
        virtualStackInfo(vsTbl[vsIndex]);

    }

/*******************************************************************************
*
* virtualStackNameTaskIdSet - set the task variable myTaskNum by name
*
* This routine takes a stack name in pName and will set the task variable
* myTaskNum to the appropriate index.
*
* RETURNS OK or ERROR if the stack named in pName is not found
*
* WARNING
* This routine is NOT to be called in the context of tNetTask.
*
*/

STATUS virtualStackNameTaskIdSet
    (
    char* pName /* Name that was used in virtualStackCreate */
    )
    {
    int vsIndex;

    semTake(vsTblLock, WAIT_FOREVER);
    
    for (vsIndex = 0; vsIndex < VSID_MAX; vsIndex++)
        if (!strncmp (vsTbl[vsIndex]->pName, pName, 
		      min (strlen (pName), VS_NAME_MAX)))
            break; /* Hey, we found it! */

    semGive (vsTblLock);

    if (vsIndex < VSID_MAX)
        return (virtualStackNumTaskIdSet (vsIndex));

    return (ERROR);
    }

/*******************************************************************************
*
* virtualStackNumTaskIdSet - set the task variable myTaskNum by index
*
* This routine takes a stack index in vsNum and will set the task variable
* myTaskNum to the appropriate index.
*
* RETURNS OK or ERROR if the stack number in vsNum is invalid
*
* WARNING
* This routine is NOT to be called in the context of tNetTask.
*
*/

STATUS virtualStackNumTaskIdSet
    (
    int vsNum
    )
    {
    /*
     * If we this task does not have the task variable then
     * create it as part of this task.
     */
    if (taskVarGet(0, (int *) &myStackNum) == ERROR)
	{
	if (errnoGet () == S_taskLib_TASK_VAR_NOT_FOUND) 
	    errnoSet (0);		/* Resets the error caused by taskVarGet */
        taskVarAdd (0, (int *) &myStackNum);
	}

    if (vsNum > VSID_MAX)
        return (ERROR);

    if (vsTbl[vsNum] == NULL)
        return (ERROR);

    myStackNum = vsNum;

    return (OK);
    }

/*******************************************************************************
 *
 * virtualStackNameSet - Set the stack name, given the stack id
 *
 * This routine sets the name of the stack identified by vsid to the
 * contents of pName. Doing so will overwrite the name provided to this
 * stack when it was created.
 *
 * RETURNS: OK or ERROR is the stack id was not found.
 *
 * This routine is not to be called in the context of tNetTask.
 */

STATUS virtualStackNameSet
    (
    char   *pName,        /* Name to be set */
    int     vsid          /* Stack id */
    )
    {
    semTake (vsTblLock, WAIT_FOREVER);
    if ( (vsid < 0) || (vsid > VSID_MAX) )
        return (ERROR);

    strcpy (vsTbl[vsid]->pName, pName);
    semGive (vsTblLock);

    return (OK);
    }

/*******************************************************************************
*
* virtualStackIdCheck - check to see if myStackNum is set in this task
*
* This routine checks to see if myStackNum is set in this task.  If it is not
* set then it is added to the task's context and set to the VS_MGMT_STACK.
*
* RETURNS OK or ERROR
*
* WARNING
* This routine is not to be called in the context of tNetTask
*
*/

STATUS virtualStackIdCheck
    (
    )
    {
    if (taskVarGet(0, (int *)&myStackNum) == ERROR)
        {
        taskVarAdd(0, (int *)&myStackNum);

        myStackNum = VS_MGMT_STACK;
        }
    return (OK);
    }



int numTestTasks;
SEM_ID virtualStackTestSem;

/*******************************************************************************
*
* virtualStackTestDummy - dummy routine called to check the myTaskNum variable
*
* This routine is called by the test harness in this module to test the
* functionality of setting task variables for stack numbers.
*
* NOMANUAL
*
*/

void virtualStackTestDummy
    (
    vsNum
    )
    {

    numTestTasks++;
    
    taskDelay (vsNum * sysClkRateGet());

    printf ("Attempting to set myself to Stack %d\n", vsNum);
    
    if (virtualStackNumTaskIdSet(vsNum) == ERROR)
        {
        printf ("Task for stack %d failed.\n", vsNum);
        return;
        }

    printf ("Task %d has myStackNum of %d\n", vsNum, myStackNum);

    printf ("Task %d has stack info of...\n", vsNum);

    virtualStackInfo (vsTbl[myStackNum]);

    numTestTasks--;
    
    if (numTestTasks == 0)
        semGive(virtualStackTestSem);

    }

/*******************************************************************************
*
* virtualStackTest - test the virtual stack library
*
* This routine tests the virtual stack library by creating VSID_MAX stacks
* and then calling the virtualStackInfo command on each.  Once this passes
* we spawn VSID_MAX seperate tasks which each attempt to set their myTaskNum
* Task Variable to the stack number they will inhabit (passed as the only
* argument to the test).  When the last task exits it releases a semaphore
* which the test is pending on.  Cleanup is then done.
*
* NOMANUAL
*
*/



void virtualStackTest
    (
    )
    {
    int count;
    VSID myStacks[VSID_MAX];
    
    printf ("Starting Virtual Stack Library Test...\n");

    for (count = 0; count < VSID_MAX; count++)
        {
        printf ("Creating stack %d\n", count);
        myStacks[count] = NULL;
        virtualStackCreate(NULL, &myStacks[count]);
        }

    printf ("Creating complete, checking for failures...\n");
    
    for (count = 0; count < VSID_MAX; count++)
        {
        printf ("Checking stack %d\n", count);
        if (myStacks[count] == NULL)
            {
            printf ("Stack creation on %d failed\n", count);
            goto cleanup;
            }
        }        

    printf ("Stacks 0 through %d created.\n", count);

    printf ("Getting stack information...\n");

    for (count = 0; count < VSID_MAX; count++)
        {
        virtualStackInfo (myStacks[count]);
        }        

    printf ("Checking task variable madness...\n");
    
    virtualStackTestSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);

    /*
     * Set the number to 0, then let them count them up
     * and down and release the semaphore.
     */
    numTestTasks = 0;
    for (count = 0; count < VSID_MAX; count++)
        {
        taskSpawn (NULL, 255, 0, 20000, (FUNCPTR)virtualStackTestDummy,
                   count, 2, 3, 4, 5, 6, 7 , 8 , 9, 10);
        }        

    /* Wait here for all the sub tasks to end. */

    semTake (virtualStackTestSem, WAIT_FOREVER);
    
    printf ("Done testing...\n");
    
cleanup:
    printf ("Cleaning up...\n");
    for (count = 0; count < VSID_MAX; count++)
        {
        if (myStacks[count] != NULL)
            virtualStackDelete (myStacks[count]);
        }
    semDelete(virtualStackTestSem);

    }


/***************************************************************************
*
* vsMakeStack - create and initialize a virtual stack
* 
* This function takes a stack name and creates a new virtual stack
* and initializes it.
*
* This function has been replaced by usrVirtualStackCreate () in
* target/config/comps/src/net/usrVirtualStack.c
*
* RETURNS: OK or ERROR
*/

STATUS vsMakeStack
    (
    char * vsName,    /* name of the stack */
    int    maxUnits   /* max number of devices attached to IP */
    )
    {
    int oldStackNum = myStackNum;
    VSID vsNum;

    if (vsName == NULL) return ERROR;

    if (virtualStackCreate(vsName, &vsNum) == ERROR)
        return ERROR;

    if (virtualStackInit(vsNum, maxUnits) == ERROR)
        return ERROR;

    myStackNum = oldStackNum;
    return OK;
    }

/***************************************************************************
*
* vsexec - call a function given by a pointer argument, using a specified virtual stack
*	
* This function is most useful when calling a function from the (target or wind) 
* shell. It is the only way to specify a stack other than 0 from the windshell.
* When spawning a task, it is a convenient way to let the spawned function for
* a different stack than 0.
* Example: sp vse, 1, rdisc - spawns a new task for rdisc in the context of
* router stack 1.
*
* RETURNS: return status from calling command
*/
int vsexec
    (
    int stackNum,	/* the stack number to use */
    int (*f)(),		/* function to call */
    int   par1,		/* parameters to function */
    int   par2,
    int   par3,
    int   par4,
    int   par5,
    int   par6,
    int   par7,
    int   par8,
    int   par9,
    int   par10
    )
    {
    if (0 == f){
    	printf("no function given\n");
	return -1;
    }
    if (virtualStackNumTaskIdSet(stackNum) == ERROR){
    	printf("error setting myStackNum\n");
	return -1;
    }
    return f(par1, par2, par3, par4, par5, par6, par7, par8, par9, par10);
    }

#endif /* VIRTUAL_STACK */
