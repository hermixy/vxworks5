/* trgLib.c - trigger events control library  */

/* Copyright 1994-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01x,21mar02,tcr  Fix SPR 74465
01w,11oct01,tcr  Fix SPR 70133 - trigger action routine cannot take
                 zero-valued argument
01v,02mar99,dgp  removed reference to e() from trgEvent()
01u,11sep98,cjtc deferred action queue for triggering modified
01t,28aug98,dgp  FCS man page edit
01s,28may98,dgp  final doc editing for WV 2.0 beta
01r,07may98,dgp  clean up man pages for WV 2.0 beta release
01q,08apr98,pr   added some comments and some cleanup
01p,25mar98,pr   deleted portWorkQAdd1 (see rBuffLib.c)
01p,25mar98,pr   added trgEvent function
01o,22mar98,pr   replaced workQAdd1 with portable version 
01n,21feb98,pr   commented out hitCnt reset in trgOn. Fix obj enable in 
		 trgCheck.
01m,22feb98,pr   added hitCnt. Cleanup
01l,13feb98,nps  made trgDelete more defensive by verifying object exists.
01k,12feb98,pr   moved trgShow to trgShow.c. Added hitCnt support.
01j,27jan98,nps  Fix merge errors.
01i,27jan98,nps  modified switch so all current deferred actions are
                 handled in the same way.
                 incorporated support for library condition fns.
01h,23jan98,pr   moved _func_trgCheck initialization, replaced macro names.
01g,16jan98,pr   modified option for msgQSend. 
01f,14dec97,pr   moved declaration of some global variables into funcBind.c
01e,14dec97,pr   deleted reference to myCnt
01d,13dec97,pr   deleted some temporary variables. Reduced the number of
		 arguments passed to trgCheck
01c,20nov97,pr   added the deferred execution of tasks, fixed some bugs
                 added trgChainSet, trgAddTcl
                 modified trgCondTest, added support for EVENT_ANY_EVENT
01b,09oct97,pr   modified the prototype: changed the structure in an obj
		 			 replaced the list with an array
					 modified the search algorithm
01a,25jun97,pr   written
*/


/*
DESCRIPTION

This library provides the interface for triggering events.  The routines
provide tools for creating, deleting, and controlling triggers.  However,
in most cases it is preferable to use the GUI to create and manage
triggers, since all order and dependency factors are automatically
accounted for there.

The event types are defined as in WindView. Triggering and WindView share
the same instrumentation points. Furthermore, one of the main uses of
triggering is to start and stop WindView instrumentation.  Triggering is
started by the routine trgOn(), which sets the shared variable 'evtAction'.
Once the variable is set, when an instrumented point is hit, trgCheck() is
called. The routine looks for triggers that apply to this event.  The
routine trgOff() stops triggering.  The routine trgEnable() enables a
specific trigger that was previously disabled with trgDisable(). (At
creation time all triggers are enabled by default.) This routine also
checks the number of triggers currently enabled, and when this is zero, it
turns triggering off.

NOTE: It is important to create a trigger before calling trgOn(). trgOn()
checks the trigger list to see if there is at least one trigger there, and if
not, it exits without setting 'evtAction'.

INCLUDE FILES: trgLibP.h

SEE ALSO: 
.I WindView User's Guide
*/

#include "vxWorks.h"
#include "intLib.h"
#include "taskLib.h"
#include "stdio.h"
#include "logLib.h"
#include "private/trgLibP.h"
#include "private/kernelLibP.h"

#include "private/windLibP.h"
#include "private/workQLibP.h"

/* extern */
extern int trgCnt;
int trgNone;
extern  VOIDFUNCPTR _func_trgCheck;
#if (CPU_FAMILY == I80X86)
extern  void portWorkQAdd1();
#endif /* CPU_FAMILY == I80X86 */

/* global */

UINT 	trgWorkQReader;			/* work queue index */
UINT 	trgWorkQWriter;			/* work queue index */
BOOL	trgWorkQFullNotify;		/* work queue full notification*/

SEM_ID	trgDefSem;

OBJ_CLASS trgClass;                   /* trg object Class */
CLASS_ID  trgClassId = &trgClass;     /* trg class ID */

MSG_Q_ID trgActionDefMsgQId = NULL;

UINT32 trgClassList[] = {TRG_CLASS_1, TRG_CLASS_2, TRG_CLASS_3, TRG_CLASS_4, TRG_CLASS_5, TRG_CLASS_6};

/* local */

TRIGGER_ID  trgWorkQ[TRG_MAX_REQUESTS];
TRIGGER_ID  * trgList;

LOCAL BOOL trgLibInstalled = FALSE;   /* protect from multiple inits */
LOCAL BOOL trgActionDefStarted = FALSE;   /* protect from multiple inits */

void trgCheck(event_t event, int index, int obj,int arg1,int arg2,int arg3,int arg4,int arg5);
TRIGGER_ID trgAdd(event_t event, int status, int contextType, UINT32 contextId, 
                  OBJ_ID  objId, int conditional, int condType, int * condEx1, 
		  int condOp, int condEx2, BOOL disable, TRIGGER_ID chain, 
		  int actionType, FUNCPTR actionFunc, BOOL actionDef, int arg);
STATUS trgInit();
STATUS trgDelete(TRIGGER_ID trgId);
void trgOff();

/*******************************************************************************
*
* trgLibInit - initialize the triggering library
*
* This routine initializes the trigger class. Triggers are VxWorks objects 
* and therefore require a class to be initialized.
*
* RETURNS: OK or ERROR.
*
*/

STATUS trgLibInit (void)
    {
    int index;

    if ((!trgLibInstalled) &&
        (classInit (trgClassId, sizeof(TRIGGER),
                    OFFSET(TRIGGER, objCore), (FUNCPTR) trgAdd,
                    (FUNCPTR) trgInit, (FUNCPTR) trgDelete) == OK))
        {
        trgList = (TRIGGER_ID *)malloc (sizeof(TRIGGER_ID)*TRG_MAX_REQUESTS );
	if ( trgInit() != OK)
	    return(ERROR);

	trgWorkQFullNotify   = FALSE;
        trgWorkQReader = 0;
        trgWorkQWriter = 0;

        for (index = 0 ; index < TRG_MAX_REQUESTS ; index ++)
            trgWorkQ[index] = NULL;

        trgLibInstalled = TRUE;
        }
    return ((trgLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* trgInit - Initializes triggering list and function
*
* initialize/reset the list and the _func_trgCheck pointer
*
* NOMANUAL
*
*/

STATUS trgInit ()
    {
    int index;

    if ( trgList == NULL )
	return(ERROR);

    for (index = 0 ; index < TRG_MAX_REQUESTS ; index ++)
        trgList[index] = NULL;

    /* set the function for the event points */

    _func_trgCheck = (VOIDFUNCPTR) trgCheck;

    return(OK);                         
    }

/*******************************************************************************
*
* trgActionDefInit - Initializes the daemon for deferred execution of tasks
*
* Tasks can be executed in deferred mode (this is the default). That is, if
* they are in a critical part of the code (i.e. Kernel state) their execution
* does not occur until they are out of the critical code.
* This task will initialize the task for the deferred execution, 
* trgActionDefPerform.
*
* NOMANUAL
*
*/

STATUS trgActionDefStart ()
    {

    if ( trgActionDefStarted == TRUE)
	return (OK);

    if ((!trgLibInstalled) && (trgLibInit () != OK))
        return (ERROR);

    taskSpawn ("trgActDef",
         TRG_ACT_PRIORITY,
         TRG_ACT_OPTIONS,
         TRG_ACT_SIZE,
         trgActionDefPerform,
         0,0,0,0,0,0,0,0,0,0);

    trgActionDefStarted = TRUE;

    return (OK);
    }


/*******************************************************************************
*
* trgWorkQReset - Resets the trigger work queue task and queue
*
* When a trigger fires, if the assocated action requires a function to be called
* in "safe" mode, a pointer to the required function will be placed on a queue
* known as the "triggering work queue". A system task "tActDef" is spawned to
* action these requests at task level. Should the user have need to reset this
* work queue (e.g. if a called task causes an exception which causes the
* trgActDef task to be SUSPENDED, or if the queue gets out of sync and becomes
* unresponsive), trgWorkQReset() may be called.
*
* Its effect is to delete the trigger work queue task and its associated 
* resources and then recreate them. Any entries pending on the triggering work
* queue will be lost. Calling this function with triggering on will result in
* triggering being turned off before the queue reset takes place. It is the
* responsibility of the user to turn triggering back on.
*
* RETURNS: OK, or ERROR if the triggering task and its associated resources
* cannot be deleted and recreated.
*
*/

STATUS trgWorkQReset (void)
    {
    int index;

    if ((!trgLibInstalled) && (trgLibInit () != OK))
        return (ERROR);                         

    if (TRG_ACTION_IS_SET)
        trgOff();

    if (trgActionDefStarted != TRUE)
	return OK;

    if (taskDelete (taskNameToId ("trgActDef")) == ERROR)
	return ERROR;

    trgActionDefStarted = FALSE;

    if (semDelete (trgDefSem) == ERROR)
	return ERROR;

    /* re-initialise the work queue */

    trgWorkQFullNotify   = FALSE;
    trgWorkQReader = 0;
    trgWorkQWriter = 0;

    for (index = 0 ; index < TRG_MAX_REQUESTS ; index ++)
        trgWorkQ[index] = NULL;

    /* restart the work queue task */

    return (trgActionDefStart ());

    }


/*******************************************************************************
*
* trgEvtToIndex - calculate the list the trigger belongs to given the event ID 
*
* The routine returns the index of the list where the trigger belongs to. The
* index are based on WindView event type classification.
*
* NOMANUAL
*
*/

int trgEvtToIndex 
    (
    event_t event
    )
    {

    if (event == EVENT_ANY_EVENT)
	return(TRG_ANY_EVENT_INDEX);

    if (IS_CLASS1_EVENT(event))
	return(TRG_CLASS1_INDEX);

    if (IS_CLASS2_EVENT(event))
	return(TRG_CLASS2_INDEX);
	
    if (IS_CLASS3_EVENT(event))
	return(TRG_CLASS3_INDEX);
	
    if (IS_USER_EVENT(event))
	return(TRG_USER_INDEX);

/* this is probably never reached, since it is a subclass of class1 */
    if (IS_INT_ENT_EVENT(event))
	return(TRG_INT_ENT_INDEX);
	
#if 0	
    if (IS_CONTROL_EVENT(event))
	return(TRG_CONTROL_INDEX);
#endif

    return(ERROR);
    }

/*******************************************************************************
*
* trgAddTcl - calls trgAdd but accepts one single parameter
*
* Since Tcl does not take more than 10 parameters, they have been saved in one
* memory location. This routine retrieves the values and passes them to trgAdd.
*
* NOMANUAL
*
*/

TRIGGER_ID trgAddTcl
    (
    char * 	buff 
    )
    {
    int * buffIndex = (int *)buff;
    int         event;
    int         status;
    int         conditional;
    int         condType, contextType, actionType;
    int         * condEx1, condOp, condEx2;
    int         actionArg;
    UINT32      contextId;
    OBJ_ID      objId;
    BOOL        disable, actionDef;
    FUNCPTR     actionFunc;
    TRIGGER     *chain;

    event = (int) *buffIndex++;
    status = (int) *buffIndex++;
    contextType = (int) *buffIndex++;
    contextId = (UINT32) *buffIndex++;
    objId = (OBJ_ID) *buffIndex++;
    conditional = (int) *buffIndex++;
    condType = (int) *buffIndex++;
    condEx1 = (int *) *buffIndex++;
    condOp = (int) *buffIndex++;
    condEx2 = (int) *buffIndex++;
    disable = (BOOL) *buffIndex++;
    chain = (TRIGGER *) *buffIndex++;
    actionType = (int) *buffIndex++;
    actionFunc = (FUNCPTR) *buffIndex++;
    actionDef = (BOOL) *buffIndex++;
    actionArg = (int) *buffIndex;

#if 0
 printf ("event = %hd \n", (event_t)event);
 printf(" %d\n", status);
 printf(" %d \n", contextType);
 printf(" %d\n", contextId);
 printf(" %p\n", objId);
 printf(" %d\n", conditional);
 printf("%d \n", condType);
 printf(" %p\n", condEx1);
 printf(" %d\n", condOp);
 printf(" %d\n", condEx2);
 printf(" %d\n", disable);
 printf(" %p\n", chain);
 printf(" %d\n", actionType);
 printf(" %p\n", actionFunc);
 printf(" %d\n", actionDef);
 printf(" %d\n", actionArg);
#endif

    return (trgAdd ( (event_t)event, status, contextType, contextId, objId, conditional, 
	        condType, condEx1, condOp, condEx2, disable, chain, actionType, 
	        actionFunc, actionDef, actionArg ));
    }
 


/*******************************************************************************
*
* trgAdd - add a new trigger to the trigger list
*
* This routine creates a new trigger and adds it to the proper trigger list.  It
* takes the following parameters:
* .iP <event> 50
* as defined in eventP.h for WindView, if given.
* .iP <status>
* the initial status of the trigger (enabled or disabled).
* .iP <contextType>
* the type of context where the event occurs.
* .iP <contextId>
* the ID (if any) of the context where the event occurs.
* .iP <objectId>
* if given and applicable.
* .iP <conditional>
* the indicator that there is a condition on the trigger.
* .iP <condType>
* the indicator that the condition is either a variable or a function.
* .iP <condEx1>
* the first element in the comparison.
* .iP <condOp>
* the type of operator (==, !=, <, <=, >, >=, |, &).
* .iP <condEx2>
* the second element in the comparison (a constant).
* .iP <disable>
* the indicator of whether the trigger must be disabled once it is hit.
* .iP <chain>
* a pointer to another trigger associated to this one (if any).
* .iP <actionType>
* the type of action associated with the trigger (none, func, lib).
* .iP <actionFunc>
* the action associated with the trigger (the function).
* .iP <actionDef>
* the indicator of whether the action can be deferred (deferred is the default).
* .iP <actionArg>
* the argument passed to the function, if any.
* .LP
*
* Attempting to call trgAdd whilst triggering is enabled is not allowed
* and will return NULL.
*
* RETURNS: TRIGGER_ID, or NULL if either the trigger ID can not be allocated,
* or if called whilst triggering is enabled.
*
* SEE ALSO: trgDelete()
*/


TRIGGER_ID trgAdd 
    (
    event_t 	event, 
    int 	status,
    int 	contextType,
    UINT32 	contextId,
    OBJ_ID  	objId, 
    int    	conditional, 
    int    	condType, 
    int		* condEx1, 
    int		condOp, 
    int		condEx2, 
    BOOL  	disable, 
    TRIGGER	*chain, 
    int 	actionType, 
    FUNCPTR 	actionFunc, 
    BOOL    	actionDef, 
    int	    	actionArg
    )
    {

    TRIGGER_ID trgId, pTrg;
    int index;

    if ( TRG_ACTION_IS_SET )
        return (NULL);

    if ((!trgLibInstalled) && (trgLibInit () != OK))
        return (NULL);                         
    
    if ((trgId = (TRIGGER_ID)objAlloc(trgClassId)) == NULL)
       return(NULL);

    objCoreInit (&trgId->objCore, trgClassId);

    /* set all the trigger parameters */

    trgId->eventId = event;
    trgId->conditional = conditional;
    if (conditional == 1)
        {
        trgId->condType = condType;

        /* FIXME: I think this should just be a pointer and not an union
           I am not changing it right now, since I do not want to make
           too many changes at the same time
         */

        switch (condType)
	    {
	    case TRIGGER_COND_VAR:
                trgId->condEx1 = condEx1;
	        break;
	    case TRIGGER_COND_FUNC:
                trgId->condEx1 = condEx1;
	        break;
            case TRIGGER_COND_LIB:
                trgId->condEx1 = condEx1;
	        break;
	    }

        trgId->condOp = condOp;
        trgId->condEx2 = condEx2;
        }
    else
        {
        trgId->condType = -1;
        trgId->condEx1 = (void *)(-1);
        trgId->condOp = -1;
        trgId->condEx2 = -1;
        }

    trgId->objId = objId;
    trgId->contextType = contextType;
    trgId->contextId = contextId; 
    trgId->disable = disable;
    trgId->chain = chain;
    trgId->actionType = actionType;
    trgId->actionDef = actionDef;
    trgId->actionFunc = actionFunc;
    trgId->actionArg = actionArg;
    trgId->status = status;
    trgId->hitCnt = 0;
    trgId->next = NULL;

    if ( actionDef  && !trgActionDefStarted ) 
	trgActionDefStart();

    if ( status == TRG_ENABLE )
        trgCnt++;

    index = trgEvtToIndex(event);

    TRG_EVTCLASS_SET(trgClassList[index]);

#if 0
	printf ("index is %d, class is %x\n", index , trgClassList[index]);
	printf ("pTrg NULL: trList[%d] is %p\n", index , trgList[index]);
#endif

    pTrg = trgList[index];

    /* add the trigger to the list */

   if (trgList[index] == NULL)
       {
       trgList[index] = trgId;
       }
   else
       {
       while (pTrg->next != NULL)
           pTrg = pTrg->next;
       pTrg->next = trgId;
       }

    return(trgId);

    }

/*******************************************************************************
*
* trgDelete - delete a trigger from the trigger list
*
* This routine deletes a trigger by removing it from the trigger list.  It
* also checks that no other triggers are still active.  If there are no
* active triggers and triggering is still on, it turns triggering off.
*
* RETURNS: OK, or ERROR if the trigger is not found.
*
* SEE ALSO: trgAdd()
*/

STATUS trgDelete
    (
    TRIGGER_ID trgId
    )
    {
    int index;
    TRIGGER_ID  pTrg;

    /* if the id is NULL, exit */

    if (trgId == NULL)
	return(ERROR);

    /* Verify the trigger exists */

    if (OBJ_VERIFY (trgId, trgClassId) != OK) {
        return(ERROR);
    }

    /* if the trigger was still enabled then decrease the counter 
     * and if the counter goes to zero, then turn the instrumentation off
     */

    if ( (trgId->status == TRG_ENABLE) && (--trgCnt == 0))
            trgOff();

    objCoreTerminate(&trgId->objCore);

    /* first, let's find the list where the trigger should be */

    index = trgEvtToIndex(trgId->eventId);
    pTrg = trgList[index];

    /* if it is empty, then there is some mistake: where did the trigger go? */

    if (pTrg == NULL)
        return (ERROR);

    if (pTrg == trgId)
	{
    	pTrg = pTrg->next;
	trgId->next = NULL;
	objFree(trgClassId, (char *)trgId);
        trgList[index] = pTrg;

	/* if this is the last in the list we should also reset 
	 * the trgEvtClass 
	 */

	if (pTrg == NULL)
	    {
	    TRG_EVTCLASS_UNSET(trgClassList[index]);
            if ( TRG_EVTCLASS_IS_SET (CLASS_NONE) )
                trgOff();
	    }

	return(OK);
	}

    while (pTrg->next != NULL)
        {
        if ( pTrg->next != trgId)
            pTrg = pTrg->next;
        else
            break;   
        }

    /* if it is not NULL, then it is the one we are looking for */

    if (pTrg->next != NULL)
        {
        pTrg->next = pTrg->next->next;
	trgId->next = NULL;
	objFree(trgClassId, (char *)trgId);
	return(OK);
        }
 
    return(ERROR); /* if we are here, then we did not found it! */
    }

/*******************************************************************************
*
* trgOn - set triggering on
*
* This routine activates triggering.  From this time on, any time an event
* point is hit, a check for the presence of possible triggers is performed.
* Start triggering only when needed since some overhead is introduced.
*
* NOTE
* If trgOn() is called when there are no triggers in the trigger list, it
* immediately sets triggering off again.  If trgOn() is called with at least
* one trigger in the list, triggering begins.  Triggers should not be 
* added to the list while triggering is on since this can create
* instability.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: trgOff()
*/

STATUS trgOn (void)
    {
    int level; 

    if ( (! TRG_ACTION_IS_SET) &&  (! TRG_EVTCLASS_IS_EMPTY) )
        {

        if ((!trgLibInstalled) && (trgLibInit () != OK))
            return (ERROR);                         

        level = intLock();
	TRG_ACTION_SET;
        intUnlock(level);

	return (OK);
	}

    return (ERROR);
    }

/*******************************************************************************
*
* trgOff - set triggering off
*
* This routine turns triggering off. From this time on, when an event point 
* is hit, no search on triggers is performed.
*
* RETURNS: N/A
*
* SEE ALSO: trgOn()
*/

void trgOff (void)
    {
    int level;

    if (TRG_ACTION_IS_SET)
	{
        level = intLock();

        /* reset the variable */

        TRG_ACTION_UNSET;

        intUnlock(level);
	}

    }

/*******************************************************************************
*
* trgEnable - enable a trigger
*
* This routine enables a trigger that has been created with trgAdd(). A counter
* is incremented to keep track of the total number of enabled triggers so that 
* trgDisable() knows when to set triggering off. If the maximum number of 
* enabled triggers is reached, an error is returned. 
*
* RETURNS: OK, or ERROR if the trigger ID is not found or if the maximum
* number of triggers has already been enabled.
*
* SEE ALSO: trgDisable()
*/

STATUS trgEnable
    (
    TRIGGER_ID trgId
    )
    {
    if (trgId == NULL)
        return(ERROR);

    if ( trgId->status != TRG_ENABLE )
	{
        trgCnt++;
        trgId->status = TRG_ENABLE;
	}

    return(OK);
    }

/*******************************************************************************
*
* trgDisable - turn a trigger off
*
* This routine disables a trigger. It also checks to see if there are triggers
* still active.  If this is the last active trigger it sets triggering off.
*
* RETURNS: OK, or ERROR if the trigger ID is not found.
*
* SEE ALSO: trgEnable()
*/

STATUS trgDisable
    (
    TRIGGER_ID trgId
    )
    {
    int level;

    if (trgId == NULL)
        return(ERROR);

    level = intLock();
    if ( trgId->status != TRG_DISABLE )
	{
        trgId->status = TRG_DISABLE;
	intUnlock(level);

        if ( (--trgCnt == 0) && TRG_ACTION_IS_SET )
            trgOff();
	}
    else
	{
	intUnlock(level);
	return(ERROR);
	}


    return(OK);
    }


/*******************************************************************************
*
* trgChainSet - chains two triggers
*
* This routine chains two triggers together.  When the first trigger fires, it
* calls trgEnable() for the second trigger.  The second trigger must be created
* disabled in order to maintain the correct sequence.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: trgEnable()
*/

STATUS trgChainSet
    (
    TRIGGER_ID fromId,
    TRIGGER_ID toId
    )
    {
    if ( fromId == NULL )
	return(ERROR);

    if ( fromId->chain != NULL && toId != NULL )
	return(ERROR);

    fromId->chain = toId;

    return (OK);
    }

/*******************************************************************************
*
* trgContextMatch - show the trigger list
*
* INTERNAL
* FIXME
*
* RETURNS:
* SEE ALSO: FIXME()
* NOMANUAL
*/

BOOL trgContextMatch
    (
    TRIGGER_ID pTrg
    )
    {
	switch ( pTrg->contextType )
	    {
	    case TRG_CTX_ANY:
		return (TRUE);
	    case TRG_CTX_SYSTEM:
		return(kernelState); 
	    case TRG_CTX_TASK:
		return ( (!kernelState) && (!INT_CONTEXT ()) &&
		     ((WIND_TCB *)(pTrg->contextId) == taskIdCurrent) );
	    case TRG_CTX_ANY_TASK:
		return ((!kernelState) && (!INT_CONTEXT ()));
	    case TRG_CTX_ISR:
	    case TRG_CTX_ANY_ISR:
		return (INT_CONTEXT ());
	    default:
		return(FALSE);
	    }
	return(FALSE);
    }

/*******************************************************************************
*
* trgCondTest - show the trigger list
*
* INTERNAL
* FIXME
*
* RETURNS:
* SEE ALSO: FIXME()
* NOMANUAL
*/

BOOL trgCondTest
    (
    TRIGGER_ID pTrg,
    int        objId
    )
    {
    BOOL retval;
    int level = intLock();

	switch (pTrg->condOp)
	    {
	    case TRIGGER_EQ: 
	        switch (pTrg->condType)
	        {
	        case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) == pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	        case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () == pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
                case TRIGGER_COND_LIB:
		       retval = (*((FUNCPTR)pTrg->condEx1)) (objId,pTrg->condEx2);
                       intUnlock(level);
                       return(retval);
	        default:
                    intUnlock(level);
		    return(FALSE);
	        }
	    case TRIGGER_NEQ: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) != pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () != pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    case TRIGGER_GRT: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) > pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () > pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    case TRIGGER_LSS: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) < pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () < pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    case TRIGGER_GEQ: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) >= pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () >= pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    case TRIGGER_LEQ: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) <= pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () <= pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    case TRIGGER_OR: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) || pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () || pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    case TRIGGER_AND: 
	       switch (pTrg->condType)
	           {
	           case TRIGGER_COND_VAR: 
                       retval = ((* ((BOOL *)pTrg->condEx1) && pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           case TRIGGER_COND_FUNC:
		       retval = (((* ((FUNCPTR)pTrg->condEx1)) () && pTrg->condEx2));
                       intUnlock(level);
                       return(retval);
	           default:
                       intUnlock(level);
		       return(FALSE);
	           }
	    default:
               intUnlock(level);
	       return (FALSE);
	    }
    intUnlock(level);
    }

/*******************************************************************************
*
* trgActionPerform - perform the action associated to a trigger 
*
* INTERNAL
* FIXME
*
* RETURNS: 
* SEE ALSO: FIXME()
* NOMANUAL
*/

void trgActionPerform
    (
    TRIGGER_ID pTrg
    )
    {
    switch (pTrg->actionType)
       {
       case TRG_ACT_WV_START:
       case TRG_ACT_WV_STOP:
       case TRG_ACT_FUNC:
           if ( pTrg->actionFunc != NULL)
               {
               (* (pTrg->actionFunc)) (pTrg->actionArg);
               }
       }
    }

/*******************************************************************************
*
* trgActionDefPerform - perform the trigger action in deferred mode
*
* INTERNAL
* FIXME
*
* RETURNS: 
* SEE ALSO: FIXME()
* NOMANUAL
*/

STATUS trgActionDefPerform ()
    {

    /* loop, executing actions and pending on the next request */

    if ((trgDefSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY)) == NULL)
        {
        return (ERROR);
        }

    while (TRUE)
        { 
        semTake (trgDefSem, WAIT_FOREVER);  /* pend on event semaphore */

	if ( trgWorkQFullNotify )
	    {
            logMsg ("Error: Triggering Work Queue overflow! \n",
		     0,0,0,0,0,0);
	    trgWorkQFullNotify = FALSE;
	    }

	while (TRUE)
	    {
	    if (trgWorkQ[trgWorkQReader] != NULL)
		{
		/* there is an entry here on the queue. Process it */

		trgActionPerform ((TRIGGER_ID)trgWorkQ[trgWorkQReader]);
		trgWorkQ[trgWorkQReader] = NULL;
		}
	    else
		{
		if (trgWorkQReader == trgWorkQWriter)
		    {
		    /*
		     * this entry is marked as unused and the read pointer
		     * has caught up the write pointer. The queue is now
		     * empty
		     */

		    break; /* exit the inner loop */
		    }
		}
	    /* increment the read pointer so it remains valid at all times */

	    if (trgWorkQReader >= (TRG_MAX_REQUESTS - 1))
		trgWorkQReader = 0;
	    else
		trgWorkQReader++;
	    }
        }
 
    /* never returns */
    return (OK);
    }
 
 
/*******************************************************************************
*
* trgCheck - check if there is a trigger associated to the event
*
* INTERNAL
*
* This routine is the core of the triggering mechanism. It is called whenever
* an event point is hit. It determines if a trigger exists for this event and
* performs the action associated to the trigger. In order to verify a trigger
* has been set for the event point, the routine performs the following steps:
*  - selects the trigger list the event belongs to, so not to look for all
*     triggers. Triggers are divided in different lists, depending on the class
*     they belong to. Classes are similar but not identical to the Windview ones.
*  - verifies the trigger is a valid object
*  - verifies the type of event matches the trigger event id
*  - verifies the object ID of the event point (if any) matches the one defined
*    in the trigger (if any)
*  - check the contect we are in is the same as the one specified in the trigger.
*    Contexts can be of the following type: ANY, ANY_TASK, TASK, SYSTEM, ANY_ISR.
*  - if a condition is also defined for the trigger, it checks it is verified.
*    Conditions, can be a function, a var or a library function. They can use
*    different operands (==, !=, >, <, ...) and are matched against a value
*  - if we ever get here, well, we have hit a trigger! At this point 3 things,
*    not necessarily mutually exclusive can happen:
*        . disable the current trigger (we want the action performed only once)
*        . perform an action
*        . enable a chained trigger (similar to a state machine)
*
* The action can be either a user/system defined routine or a library (such
* as WV_START/STOP). Action axecution represent the most complex part of the
* triggering mechanism. Event points are often set in critical regions of the
* kernel where the number of actions that can be performed is limited. In order
* to avoid conflicts and still allow the user full range of action, we have 
* provided a deferred execution mode. In this way, when an action should be 
* performed a request is sent to a task. Therefore the action itself will not be
* executed in the context where the event happens, but in a safer one. If a user
* really needs the action to be performed on the spot, this mode can be switched
* off. The way the deferred mode is implemented represent the critical part of
* the code, and it is still a work in progress.
* The first approach we followed was to use message queues. It logically satisfies
* the need we had to send information to a task pending waiting for it.
* The implementation of this approach raised a few issues:
*    - recursion: msgQSend/Receive (directly and indirectly) refer to several event
*      points. Things get worse if a trigger is set on one of them. First of all,
*      we are introducing a form of intrusion, since the triggers will be fired on
*      events generated by the trigger itself. More than that, if triggers are not
*      disabled, it will go into a loop.
*    - context switch: msgQSend can generate a context switch, which is not the most
*      desirable thing.
*    - queue does not work well with interrupts: if we are in an interrupt we can not
*      use the WAITFOREVER option. This will generate potential inconsistency in the
*      queue.
* 
* A second approach was to put a task on the workQueue, using workQAdd, but this will 
* cause the execution of the action when we are doing windExit.
* A solution to this problem would be to add some "harmless" action in the workQueue, 
* that does not cause context switch, and eventually will wake up the task. Therefore,
* the idea of semBGiveDefer. The problem now is to tell the pending task what to do.
* This is solved by putting the request into a triggering work queue.
* This is the current implementation. The work queue mechanism needs definitely to be
* revised. Atthe moment it could cause starvation. The requests are executed from the
* top of the list down, but a new request will be put at the top of the list, even if
* the list is not empty. This could be solved by using a rotation mechanism, so that 
* request are alway put at the end of the queue. This part has not been implemented yet.
*
* It requires two arguments and a few optional ones.
*
* RETURNS:
* SEE ALSO: FIXME()
* NOMANUAL
*/

void trgCheck
    (
    event_t event,
    int	    index,
    int     obj,  
    int     arg1,
    int     arg2,
    int     arg3,
    int     arg4,
    int     arg5
    )
    {
    TRIGGER_ID pTrg = NULL; 
    int level;
    int any = 0;
    pTrg = trgList[index];

restart:

    if (index == TRG_CLASS_3_ON)
	trgNone = 20;

    while (pTrg != NULL) 
        { 
	if (OBJ_VERIFY (pTrg, trgClassId) != OK)
	    pTrg = pTrg->next;
	else if ( pTrg->status != TRG_ENABLE )
	    pTrg = pTrg->next;
	else if ((pTrg->eventId != EVENT_ANY_EVENT) && 
	         (pTrg->eventId != event) )
	    pTrg = pTrg->next;
	else if ((pTrg->objId != NULL) && (((OBJ_ID)obj != NULL) &&
	         ((OBJ_ID)obj != pTrg->objId)) )
	    pTrg = pTrg->next;
        else
	    {
            switch ( pTrg->contextType )
	        {
	        case TRG_CTX_ANY:
	            break;
	        case TRG_CTX_SYSTEM:
	            if (kernelState) 
	                break;
	            else
	                goto end;
	        case TRG_CTX_TASK:
	            if ((!kernelState) && (!INT_CONTEXT ()) &&
	                 ((WIND_TCB *)(pTrg->contextId) == taskIdCurrent) )
		        break;
		    else
		        goto end;
	        case TRG_CTX_ANY_TASK:
		    if ((!kernelState) && (!INT_CONTEXT ())) 
		        break;
		    else
		        goto end;
	        case TRG_CTX_ISR:
	        case TRG_CTX_ANY_ISR:
		    if (INT_CONTEXT ())
		        break;
		    else
		        goto end;
	        default:
		        goto end;
	        }

            /* check if it is a conditional trigger and in case
             * test the condition
             */
            if ((pTrg->conditional == TRIGGER_COND_YES) && 
                (pTrg->condEx1 != NULL) && (trgCondTest(pTrg,obj) == FALSE))
                        goto end;

            level = intLock();

            /* disable the current trigger, if requested */
            if ((pTrg->disable == TRUE) && (trgDisable(pTrg) == ERROR))
                {
                intUnlock(level);
                goto end;
                }
            intUnlock(level);


            pTrg->hitCnt++;

	    /* perform the action associated with the trigger, if any */

            if ( pTrg->actionType != TRG_ACT_NONE )
	        {
		if ( pTrg->actionDef == FALSE )
                    trgActionPerform(pTrg);
		else /* defer the action */
		    {
		    /*
		     * This will put a request in the triggering work queue
		     * and release a semaphore so that the task trgActDef
		     * can execute the request once we are out of this
		     * code. We might be in Kernel state and we do not want
		     * to perform action here, if possible.
		     */

		    level = intLock();

		    if ( trgWorkQ[trgWorkQWriter] == NULL )
			{
		        trgWorkQ[trgWorkQWriter] = pTrg;

			/*
			 * Increment the writer index.
			 * Ensure that trgWorkQWriter NEVER goes to an invalid
			 * value when wrapping round. This assurance is needed
			 * in the trgActDef task
			 */

			if (trgWorkQWriter >= (TRG_MAX_REQUESTS - 1))
		            trgWorkQWriter = 0;
			else
			    trgWorkQWriter++;
			}
		    else
		 	{
			/* If the workQ is full need to stop triggering */

			trgWorkQFullNotify = TRUE;
			trgOff();
		    	intUnlock(level);
			return ;
			}
			
#if (CPU_FAMILY == I80X86)
                    portWorkQAdd1((FUNCPTR)semBGiveDefer,
                                  (int)trgDefSem);
#else
                    workQAdd1((FUNCPTR)semBGiveDefer,
                              (int)trgDefSem);
#endif /* CPU_FAMILY == I80X86 */

		    intUnlock(level);
		    }
		 }

            /* check if there is any trigger chained to the current
             * one, and if any, enable it
	    if (OBJ_VERIFY (&(pTrg->chain), trgClassId) == OK)
             */
            if ( pTrg->chain != NULL )
	        {
                trgEnable(pTrg->chain);
		if (!TRG_ACTION_IS_SET)
		    trgOn();
		}
                 
	    /* 
	     * let's loop for the next one!
             */
end:
	    pTrg = pTrg->next;
	    }
        }

    if ((any == 0) && (trgList[TRG_ANY_EVENT_INDEX] != NULL))
        {
	any = 1;
    	pTrg = trgList[TRG_ANY_EVENT_INDEX];
	goto restart;
	}
    }


/* Conditional library fn */

int isTaskNew(int taskId, int arg)
{
    REG_SET regs;

    /* Load the regs of the task being resumed */

    taskRegsGet (taskId,&regs);

    /* Compare the current PC with the initial entry point */

#if 0
logMsg("entry = %p, pc = %p\n",((WIND_TCB *) taskId)->entry,regs.pc,0,0,0,0);
#endif

    if ((FUNCPTR) regs.pc == (FUNCPTR) vxTaskEntry)
        {
        /*
         *  Task is at entry point, we can therefore deduce it has
         *  just been created.
         */

        /* now match actual user entry point */

        if((int) ((WIND_TCB *) taskId)->entry != 0 &&
           (int) ((WIND_TCB *) taskId)->entry == arg)
            {

            /* Suspend the task! */

#if 0
logMsg ("Suspending task %p\n",taskId);
#endif

#if 1
#if (CPU_FAMILY == I80X86)
            portWorkQAdd1 ((FUNCPTR)taskSuspend, taskId);
#else
            workQAdd1 ((FUNCPTR)taskSuspend, taskId);
#endif /* CPU_FAMILY == I80X86 */
#else
            excJobAdd ((VOIDFUNCPTR)taskSuspend, taskId,0,0,0,0,0);
#endif

            return (TRUE);
            }
        else
            {

            /* This is not the entry point of interest */

            return (FALSE);
            }
        }
    else
        {
        /*
         *  Task is been resumed after running on from entry point.
         */

        return (FALSE);
        }
}

/*******************************************************************************
*
* trgEvent - trigger a user-defined event 
*
* This routine triggers a user event. A trigger must exist and triggering must 
* have been started with trgOn() or from the triggering GUI to use this routine.  
* The <evtId> should be in the range 40000-65535. 
* 
* RETURNS: N/A
*
* SEE ALSO: dbgLib, e()
*
* INTERNAL
*
*/

void trgEvent
    (
    event_t    evtId      /* event */
    )
    {
    trgCheck (evtId, TRG_USER_INDEX, 0, 0, 0, 0, 0, 0);
    }
