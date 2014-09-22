/* wvLib.c - event logging control library (WindView) */

/* Copyright 1994-2001 Wind River Systems, Inc. */


/*
modification history
--------------------
02x,26oct01,tcr  fix problem with half-word alignment in wvLogHeaderCreate()
02w,24oct01,tcr  Fix comment string for Diab compiler
02v,18oct01,tcr  move to VxWorks 5.5, add VxWorks Events
02u,03mar99,dgp  update to reference project facility instead of config.h
02t,31aug98,cth  FCS man page edit
02s,28aug98,dgp  FCS man page edit, change order of sections
02r,18aug98,cjtc event buffer full handled more gracefully (SPR 22133)
02q,06aug98,cth  added retry to upload path write on EWOULDBLOCK, updated docs
02p,16jul98,cjtc wvUploadStop returns status of upload task (SPR 21461)
02o,09jul98,cjtc fix problems with task name preservation (SPR 21350)
02n,26may98,dgp  final doc editing for WV 2.0
02m,14may98,cth  added lib man page, other doc changes
02l,13may98,cth  added continued upload if thresh bytes still in buffer, 
		 added ints locked on buffer access, added wvUploadTaskConfig
02k,07may98,dgp  clean up man pages for WV 2.0 beta release
02j,20apr98,cth  added wvLogHeaderCreate/Upload, changed wvEvtLogInit, cleanup
		 removed separate cont/defer upload priorities
02i,17apr98,cth  reworked event-log header uploading and storage, cleaned up,
		 added status to wvUpTaskId
02h,15apr98,cth  added tnHashTbl functions for post-mortem taskname preserving,
		 added wvTaskNameBufAdd, wvTaskNamesPreserve, wvTaskNamesUpload
02g,17feb98,pr   added memLib support for object instrumentation
02f,27jan98,cth  removed evtLogHeaderCreate, changed wvLibInit/2, added
		 wvEvtLogInit, changed upTask priority to 150, doc changes
02e,23jan98,pr   modified WV macros, deleted trgCheck binding.
02d,18dec97,cth  updated includes, removed tmp references, wvOn/Off, and
		 rBuffErrorHandler
02c,14dec97,pr   deleted some windview 1.0 variables.
02b,16nov97,cth  reworked for wv2.0
02a,26aug97,pr   modified evtLog functions initialization and added trgCheck.
            cth  added wvEvtLogDisable in the event of a buffer overflow.
01z,18aug97,pr   added temporary function pointer initializer for 
		 CTX level.
01y,11aug97,nps  fixed typo causing compilation problem.
01x,29jul97,nps  added ring buffer support.
01w,24jun97,pr   replaced evtLogTIsOn with evtInstMode
		 Added return (ERROR) for default case in wvEvtLogEnable
01v,09aug96,pr   deleted obj instrumentation functions from wvEvtLogEnable
01u,07aug96,pr   Added object instrumentation functions to wvEvtLog[Enable
                 ,Disable] for SPR #6998.
01t,11jul96,pr   added ifdef for PPC function evtLogT1_noTS
01s,08jul96,pr   added evtLogT1_noTS
01r,03feb95,rhp  doc tweaks from last printed version of man pages
01q,01feb95,rhp  lib man page: add SEE ALSO ref to User's Guide
01p,15sep94rdc   wvEvent insures logging is enabled to minimize overhead
		 when logging is turned off.
01o,20oct94,rdc  increased evtTask stack size.
01n,14apr94,smb  Changes made to wvObjInst to treat task objects and other
		 system object instrumentation differently.
01m,05apr94,smb  documentation modifications.
01l,30mar94,smb  fixed event buffer overflow race.
01k,07mar94,smb  changed prototypes for OSE functionality
		 removed PASSIVE_MODE
		 documentation modifications
01j,22feb94,smb  changed typedef EVENT_TYPE to event_t (SPR #3064)
01i,15feb94,smb  renamed collection modes for wvInstInit SPR #3049
		 parameter checking SPR #3050
		 added scratch-pad reset.
01h,01feb94,smb  fixed return values for wvEvtLog[Enable,Disable] SPR #2994 
		 object are not instrumented until specified SPR #2985
01g,24jan94,smb  added INT_RESTRICT to wvEvtLogEnable and wvEvtLogDisable
		 initialised function pointer for portable kernel
01f,19jan94,smb  documentation changes
		 wvEvtLog changes to wvEvtLogEnable and wvEvtLogDisable
		 tEvtTask() is no longer spawned in POST_MODE
		 added wvEvtLogStop(), wvOn() and wvOff routines
01e,18jan94,maf  tEvtTask() no longer calls connectionClose() upon event
		   buffer overflow (part of fix for SPR #2800).
		 tEvtTask() now uploads last buffer upon event buffer
		   overflow (SPR #2888).
01d,13jan94,c_s  adjusted documentation for wvEvent () and removed NOMANUAL.
                   (SPR #2870).	 
01c,04jan94,c_s  wvInstInit doesn't attempt to free user-supplied memory 
		   (SPR #2750).
01b,15dec93,smb  documentation additions
		 allows wvEvtLog() to be called multiple times.
01a,10dec93,smb  created
*/

/*
DESCRIPTION
This library contains routines that control event collection and upload of
event data from the target to various destinations.  The routines define
the interface for the target component of WindView.  When event data has
been collected, the routines in this library are used to produce event
logs that can be understood by the WindView host tools.

An event log is made up of a header, followed by the task names of each
task present in the system when the log is started, followed by a string of
events produced by the various event points throughout the kernel and
associated libraries.  In general, this information is gathered and stored
temporarily on the target, and later uploaded to the host in the proper
order to form an event log.  The routines in this file can be used to
create logs in various ways, depending on which routines are called, and in
which order the routines are called.  

There are three methods for uploading event logs.  The first is to defer
upload of event data until after logging has been stopped in order to
eliminate events associated with upload activity from the event log.  The
second is to continuously upload event data as it is gathered.  This
allows the collection of very large event logs, that may contain more
events than the target event buffer can store at one time.  The third is
to defer upload of the data until after a target reboot.  This
method allows event data to continuously overwrite earlier data in the
event buffer, creating a log of the events leading to a target failure (a
post-mortem event log).

Each of these three methods is explained in more detail 
in CREATING AN EVENT LOG.

EVENT BUFFERS AND UPLOAD PATHS
Many of the routines in wvLib require access to the buffer used to store event 
data (the event buffer) and to the communication paths from the target to the 
host (the upload paths).  Both the buffer and the path are referenced with IDs 
that provide wvLib with the appropriate information for access.  

The event buffering mechanism used by wvLib is provided by rBuffLib.
The upload paths available for use with wvLib are provided by
wvFileUploadPathLib, wvTsfsUploadPathLib and wvSockUploadPathLib.

The upload mechanism backs off and retries writing to the upload path
if an error occurs during the write attempt with the errno 'EAGAIN' 
or 'EWOULDBLOCK'.  Two global variables are used to set the amount of time to
back off and the number of retries.  The variables are:
.CS
    int wvUploadMaxAttempts   /@ number of attempts to try writing @/
    int wvUploadRetryBackoff  /@ delay between tries (in ticks - 60/sec) @/
.CE

INITIALIZATION
This library is initialized in two steps.  The first step, done by calling
wvLibInit(), associates event logging routines to system objects.  This is
done when the kernel is initialized.  The second step, done by calling
wvLibInit2(), associates all other event logging routines with the
appropriate event points.  Initialization is done automatically when
INCLUDE_WINDVIEW is defined.

Before event logging can be started, and each time a new event buffer is
used to store logged events, wvEvtLogInit() must be called to bind the
event logging routines to a specific buffer.

DETERMINING WHICH EVENTS ARE COLLECTED
There are three classes of events that can be collected.  They are:

.CS
    WV_CLASS_1              /@ Events causing context switches @/
    WV_CLASS_2	            /@ Events causing task-state transitions @/
    WV_CLASS_3	            /@ Events from object and system libraries @/
.CE

The second class includes all of the events contained within the first
class, plus additional events causing task-state transitions but not
causing context switches.  The third class contains all of the second, and
allows logging of events within system libraries.  It can also be
limited to specific objects or groups of objects:
.iP 
Using wvObjInst() allows individual objects (for example, 'sem1') to be 
instrumented.
.iP
Using wvSigInst() allows signals to be instrumented.
.iP
Using wvObjInstModeSet() allows finer control over what type of objects are
instrumented.  wvObjInstModeSet() allows types of system objects (for example,
semaphores, watchdogs) to be instrumented as they are created.  
.LP

Logging events in Class 3 generates the most data, which may be helpful
during analysis of the log.  It is also the most intrusive on the system,
and may affect timing and performance.  Class 2 is more intrusive than
Class 1.  In general, it is best to use the lowest class that still 
provides the required level of detail.

To manipulate the class of events being logged, the following routines can
be used:  wvEvtClassSet(), wvEvtClassGet(), wvEvtClassClear(), and
wvEvtClassClearAll().  To log a user-defined event, wvEvent() can be used.  
It is also possible to log an event from any point during execution using e(), 
located in dbgLib.

CONTROLLING EVENT LOGGING
Once the class of events has been specified, event logging can be started
with wvEvtLogStart() and stopped with wvEvtLogStop().

CREATING AN EVENT LOG
An event log consists of a header, a section of task names, and a list of
events logged after calling wvEvtLogStart().  As discussed above, there are
three common ways to upload an event log.

.SS "Deferred Upload"
When creating an event log by uploading the event data after event logging
has been stopped (deferred upload), the following series of calls can be
used to start and stop the collection.  In this example the memory
allocated to store the log header is in the system partition.  The event
buffer should be allocated from the system memory partition as well. 
Error checking has been eliminated to simplify the example.

.CS
    /@ wvLib and rBuffLib initialized at system start up @/  

    #include "vxWorks.h"
    #include "wvLib.h"
    #include "private/wvBufferP.h"
    #include "private/wvUploadPathP.h"
    #include "private/wvFileUploadPathLibP.h"

    BUFFER_ID  		bufId;
    UPLOAD_ID		pathId;
    WV_UPLOAD_TASK_ID	upTaskId;
    WV_LOG_HEADER_ID	hdrId;

    /@ 
     @ To prepare the event log and start logging: 
     @/

    /@ Create event buffer in memSysPart, yielding bufId. @/

    wvEvtLogInit (bufId);
    hdrId = wvLogHeaderCreate (memSysPartId);
    wvEvtClassSet (WV_CLASS_1);		/@ set to log class 1 events @/
    wvEvtLogStart ();

    /@ 
     @ To stop logging and complete the event log. 
     @/

    wvEvtLogStop ();

    /@ Create an uplaod path using wvFileUploadPathLib, yielding pathId. @/

    wvLogHeaderUpload (hdrId, pathId);
    upTaskId = wvUploadStart (bufId, pathId, TRUE);
    wvUploadStop (upTaskId);

    /@ Close the upload path and destroy the event buffer @/
.CE

Routines which can be used as they are, or modified to meet the users needs,
are located in usrWindview.c.  These routines, wvOn() and wvOff(), provide a
way to produce useful event logs without using the host user interface of
WindView.

.SS "Continuous Upload"
When uploading event data as it is still being logged to the event buffer
(continuous upload), simply rearrange the above calls:

.CS
    /@ Includes and declarations. @/

    /@ 
     @ To prepare the event log and start logging: 
     @/

    /@ Create event buffer in memSysPart, yielding bufId. @/
    /@ Create an uplaod path, yielding pathId. @/

    wvEvtLogInit (bufId);

    upTaskId = wvUploadStart (bufId, pathId, TRUE);

    hdrId = wvLogHeaderCreate (memSysPartId);
    wvLogHeaderUpload (hdrId, pathId);

    wvEvtClassSet (WV_CLASS_1);		/@ set to log class 1 events @/
    wvEvtLogStart ();

    /@ 
     @ To stop logging and complete the event log: 
     @/

    wvEvtLogStop ();
    wvUploadStop (upTaskId);

    /@ Close the upload path and destroy the event buffer @/
.CE


.SS "Post-Mortem Event Collection"
This library also contains routines that preserve task name information
throughout event logging in order to produce post-mortem event logs:
wvTaskNamesPreserve() and wvTaskNamesUpload().

Post-mortem event logs typically contain events leading up to a target
failure.  The memory containing the information to be stored in the log
must not be zeroed when the system reboots.  The event buffer is set up to
allow event data to be logged to it continuously, overwriting the data
collected earlier.  When event logging is stopped, either by a system
failure or at the request of the user, the event buffer may not contain
the first events logged due to the overwriting.  As tasks are created the
EVENT_TASKNAME that is used by the WindView host tools to associate a task
ID with a task name can be overwritten, while other events pertaining to
that task ID may still be present in the event buffer.  In order to assure
that the WindView host tools can assign a task name to a context, a copy
of all task name events can be preserved outside the event buffer and
uploaded separately from the event buffer.

Note that several of the routines in wvLib, including
wvTaskNamesPreserve(), take a memory partition ID as an argument.  This
allows memory to be allocated from a user-specified partition.  For
post-mortem data collection, the memory partition should be within memory
that is not zeroed upon system reboot.  The event buffer, preserved task
names, and log header should be stored in this partition.

Generating a post-mortem event log is similar to generating a deferred
upload log.  Typically event logging is stopped due to a system failure,
but it may be stopped in any way.  To retrieve the log header, task name
buffer, and event buffer after a target reboot, these IDs must be remembered
or stored along with the collected information in the non-zeroed memory.
Also, the event buffer should be set to allow continuous logging by
overwriting earlier event data.  The following produces a post-mortem log.
The non-zeroed memory partition has the ID <postMortemPartId>.

.CS
    /@ Includes, as in the examples above. @/

    BUFFER_ID  		bufId;
    UPLOAD_ID		pathId;
    WV_UPLOAD_TASK_ID	upTaskId;
    WV_LOG_HEADER_ID	hdrId;
    WV_TASKBUF_ID	taskBufId;

    /@ 
     @ To prepare the event log and start logging: 
     @/

    /@ 
     @ Create event buffer in non-zeroed memory, allowing overwrite, 
     @ yielding bufId. 
     @/

    wvEvtLogInit (bufId);
    taskBufId = wvTaskNamesPreserve (postMortemPartId, 32);
    hdrId = wvLogHeaderCreate (postMortemPartId);
    wvEvtClassSet (WV_CLASS_1);		/@ set to log class 1 events @/
    wvEvtLogStart ();

    /@ 
     @ System fails and reboots.  Note that taskBufId, bufId and
     @ hdrId must be preserved through the reboot so they can be
     @ used to upload the data.
     @/

    /@ Create an uplaod path, yielding pathId. @/

    wvLogHeaderUpload (hdrId, pathId);
    upTaskId = wvUploadStart (bufId, pathId, TRUE);
    wvUploadStop (upTaskId);
    wvTaskNamesUpload (taskBufId, pathId);

    /@ Close the upload path and destroy the event buffer @/
.CE

INCLUDE FILES: wvLib.h eventP.h

SEE ALSO: rBuffLib, wvFileUploadPathLib, wvSockUploadPathLib,
wvTsfsUploadPathLib,
.I WindView User's Guide

INTERNAL
All buffer accesses are made with interrupts locked.

Event logging is started by setting the evtAction variable, which is done
by the WV_ACTION_SET macro.  This global variable acts as a guard around
all event points.  When the first byte of this 16-bit word is TRUE, the
windview event point will then check the global wvEvtClass to dertermine
if this event point belongs to any of the classes being logged.  The
second half of evtAction is used in the same way for triggering.  The
event points that should respond when evtAction's triggering half is TRUE
are determined by the trgEvtClass variable.

In the most common case, when neither windview nor triggering is being
used within the kernel, having two levels of variables (evtAction is the
first level, wvEvtClass and trgEvtClass are the second) reduces the work
at each event point to the testing of a single variable, evtAction.
Unfortunately, when either windview or triggering is on, three tests,
rather than just two, have to be performed.

All three global variables, evtAction, wvEvtClass trgEvtClass are managed
through the macro interface defined in eventP.h.  The variables themselves
never need to be referenced explicitly.

The event buffer and upload paths provide a fairly simple interface 
that allows wvLib to use them.  Other buffers and upload paths can be
easily added by follwing the same interface.  

The buffer ID passed to many of wvLib's routines is actually a pointer to
a structure defined in wvBufferP.h.  This structure contains function 
pointers for reading and writing, and communicates threshold information from
the buffering library.  evtLogLib also uses this ID to write to the buffers.
The buffer library usually considers the ID as a pointer to a structure 
whose first members are like the structure defined in wvBufferP.h, but
whose other members are used to store its own private information.

Upload paths are accessed in a similar way to the event buffers,  but are 
much simpler.  The structure that defines their interface is defined in
wvUploadPathP.h.

A hash table is implemented within wvLib to maintain task name events for
post mortem log generation, as described above.  This was done because it
is possible for a tid to be reused, leaving more than one taskname for a 
single tid.  In this case one of the names has to be chosen, and in our
case it is the last.  The hash table provides near constant time lookup of
events as they are logged, in order to ensure that there is only ever one
name per tid in the table at any time.  The hash tables were rewritten here,
rather than using the vxWorks' hashLib, because the storage has to 
relocatable to other memory partitions than the system partition.  

This hash table is complete overkill however, because the WindView parser
can accept any number of taskname events with a single tid.  It always 
updates the name to the last name it received for a tid.  Given this, it
is not necessary to ensure there are no duplicate names per tid in the
taskname buffer.  With no lookup facility necessary, a simple linked list
would suffice for this data structure.  Well, the hash table works.
*/


#include "vxWorks.h"
#include "intLib.h"
#include "logLib.h"
#include "msgQLib.h"
#include "semLib.h"
#include "private/memPartLibP.h"
#include "string.h"
#include "taskLib.h"
#include "wdLib.h"
#include "sysLib.h"
#include "errnoLib.h"
#include "wvLib.h"

#include "private/eventP.h"
#include "private/objLibP.h"
#include "private/sigLibP.h"
#include "private/eventLibP.h"
#include "private/taskLibP.h"
#include "private/kernelLibP.h"
#include "private/workQLibP.h"
#include "private/semLibP.h"
#include "private/evtLogLibP.h"
#include "private/wvBufferP.h"
#include "private/wvUploadPathP.h"
#include "private/wvLibP.h"


/* global */

int wvUploadTaskPriority      = 150;	        /* upload task priority */
int wvUploadTaskStackSize     = 5000;	        /* upload task stack size */
int wvUploadTaskOptions       = VX_UNBREAKABLE; /* upload task options */
int wvUploadMaxAttempts       = 200;            /* upload write retries */
int wvUploadRetryBackoff      = 6;              /* upload retry delay - ticks */

PART_ID	pmPartId ;				/* post mortem partition id */
BOOL    wvEvtBufferFullNotify = FALSE;	/* notify the host that the evt buff */
					/* is full. Will be reset by the */
					/* when seen (handshake) or when */
					/* wvEvtLogStart is called */

/* local */

static BUFFER_ID    wvEvtBufferId;	/* a local copy for access by the host
					   through wvEvtBufferGet */

static TN_HASH_TBL *tnHashTbl;		/* Local copy of post-mortem taskname
					   event hash table.  This is required
					   for adding events, because events 
					   are added directly from event points
					   and this info isn't known at the
					   event point. */

/* extern */

extern CLASS_ID msgQClassId;
extern CLASS_ID wdClassId;


/* forward declarations */

static STATUS       uploadPathWrite        (UPLOAD_ID pathId, char *pData,
					    int nBytes);
static STATUS 	    wvUpload		   (WV_UPLOADTASK_ID upTaskId);
static TASKBUF_ID   tnHashTblCreate        (PART_ID memPart, int tblSize);
static STATUS       tnHashTblDestroy       (TN_HASH_TBL *pTbl);
static int 	    tnHash 	           (TN_HASH_TBL *tbl, int key);
static TN_ITER_KEY *tnHashTblIterInit      (TN_HASH_TBL *pTbl);
static void         tnHashTblIterDone      (TN_ITER_KEY *pIterKey, 
					    TN_HASH_TBL *pTbl);
static TN_NODE     *tnHashTblIterNext      (TN_HASH_TBL *pTbl, 
					    TN_ITER_KEY *pIterKey);
static TN_EVENT    *tnHashTblIterNextEvent (TN_HASH_TBL *pTbl, 
					    TN_ITER_KEY *pIterKey);
static STATUS       tnHashTblInsert        (TN_HASH_TBL *pTbl, 
					    TN_EVENT *pEvent, int key, 
					    TN_EVENT *pReplacedEvent);

/*******************************************************************************
*
* wvLibInit - initialize wvLib - first step (WindView)
*
* This routine starts initializing wvLib.  Its actions should be performed 
* before object creation, so it is called from usrKernelInit() in usrKernel.c.
*
* RETURNS: N/A
*/

void wvLibInit (void)
    {
    evtObjLogFuncBind ();
    }

/*******************************************************************************
*
* wvLibInit2 - initialize wvLib - final step (WindView)
*
* This routine is called after wvLibInit() to complete the initialization of
* wvLib.  It should be called before starting any event logging.
*
* RETURNS: N/A
*/

void wvLibInit2 (void)
    {
    evtLogFuncBind ();
    }

/*******************************************************************************
*
* wvEvtLogInit - initialize an event log (WindView)
*
* This routine initializes event logging by associating a particular event
* buffer with the logging functions.  It must be called before event logging
* is turned on.
*
* RETURNS: N/A
*/

void wvEvtLogInit 
    (
    BUFFER_ID 	evtBufId		/* event-buffer id */
    )
    {

    /* Bind the buffer identified by evtBufId to the logging functions. */

    evtBufferBind (evtBufId);

    /* Stash a private copy of the id to return from wvEvtBufferGet. */

    wvEvtBufferId = evtBufId;
    }

/*******************************************************************************
* wvEvtLogStart - start logging events to the buffer (WindView)
*
* This routine starts event logging.  It also resets the timestamp
* mechanism so that it can be called more than once without stopping event
* logging.
*
* RETURNS: N/A
*/

void wvEvtLogStart (void) 
    {

    /* 
     * Initialize the time-stamp facility.  If event-logging is already
     * enabled then we must disable the time-stamp facility before re-
     * enabling it.
     */

    if (_func_tmrDisable != NULL && !WV_ACTION_IS_SET)
	(* _func_tmrDisable)();

    if (_func_tmrEnable != NULL)
        (* _func_tmrEnable) ();

    /* Turn on event logging. */

    wvEvtBufferFullNotify = FALSE;
    WV_ACTION_SET;
    }

/*******************************************************************************
*
* wvEvtLogStop - stop logging events to the buffer (WindView)
*
* This routine turns off all event logging, including event-logging of
* objects and signals specifically requested by the user.  In addition,
* it disables the timestamp facility.
*
* RETURNS: N/A
*/

void wvEvtLogStop (void)
    {

    /* Disable all event logging. */

    WV_ACTION_UNSET;

    /* Turn off any user-requested event logging within objects and signals. */

    if (wvObjInstModeSet (INSTRUMENT_OFF) == INSTRUMENT_ON)
        {
        wvObjInst (OBJ_TASK, 0, INSTRUMENT_OFF);
        wvObjInst (OBJ_SEM,  0, INSTRUMENT_OFF);
        wvObjInst (OBJ_MSG,  0, INSTRUMENT_OFF);
        wvObjInst (OBJ_WD,   0, INSTRUMENT_OFF);
        wvObjInst (OBJ_MEM,  0, INSTRUMENT_OFF);
        }
    wvSigInst (INSTRUMENT_OFF);
    wvEventInst (INSTRUMENT_OFF);

    /* Disable the time-stamp facility. */

    if (_func_tmrDisable != NULL)
        (* _func_tmrDisable) ();
    }

/*******************************************************************************
*
* wvEvtClassSet - set the class of events to log (WindView)
*
* This routine sets the class of events which are logged when event
* logging is started.  <classDescription> can take the following values:
*
* .CS
*      WV_CLASS_1      /@ Events causing context switches @/
*      WV_CLASS_2      /@ Events causing task-state transitions @/
*      WV_CLASS_3      /@ Events from object and system libraries @/
* .CE
*
* See wvLib for more information about these classes, particularly Class 3.
*
* RETURNS: N/A
*
* SEE ALSO:
* wvObjInst(), wvObjInstModeSet(), wvSigInst(), wvEventInst()
*
*/

void wvEvtClassSet
    (
    UINT32 classDescription		/* description of evt classes to set */
    )
    {
    WV_EVTCLASS_SET (classDescription);
    }

/*******************************************************************************
*
* wvEvtClassGet - get the current set of classes being logged (WindView)
*
* This routine returns the set of classes currently being logged.
*
* RETURNS: The class description.
*/

UINT32 wvEvtClassGet (void)
    {
    return (wvEvtClass);
    }

/*******************************************************************************
*
* wvEvtClassClear - clear the specified class of events from those being logged (WindView)
*
* This routine clears the class or classes described by <classDescription>
* from the set of classes currently being logged.
*
* RETURNS: N/A
*/

void wvEvtClassClear
    (
    UINT32 classDescription	/* description of evt classes to clear */
    )
    {
    WV_EVTCLASS_UNSET (classDescription);
    }

/*******************************************************************************
*
* wvEvtClassClearAll - clear all classes of events from those logged (WindView)
*
* This routine clears all classes of events so that no classes are logged
* if event logging is started.
*
* RETURNS: N/A
*/

void wvEvtClassClearAll (void)
    {
    WV_EVTCLASS_EMPTY;
    }

/*******************************************************************************
*
* wvObjInstModeSet - set object instrumentation on/off  (WindView)
*
* This routine causes objects to be created either instrumented or not
* depending on the value of <mode>, which can be INSTRUMENT_ON or 
* INSTRUMENT_OFF.  All objects created after wvObjInstModeSet() is called
* with INSTRUMENT_ON and before it is called with INSTRUMENT_OFF are
* created as instrumented objects.
*
* Use wvObjInst() if you want to enable instrumentation for a specific
* object or set of objects.  Use wvSigInst() if you want to enable 
* instrumentation for all signal activity, and wvEventInst() to 
* enable instrumentation for VxWorks Event activity.
*
* This routine has effect only if INCLUDE_WINDVIEW is defined in
* configAll.h.
*
* RETURNS: The previous value of <mode> or ERROR.
*
* SEE ALSO: wvObjInst(), wvSigInst(), wvEventInst()
*/

STATUS wvObjInstModeSet 
    (
    int mode			/* object instrumentation on/off */
    )
    {

    if (mode == INSTRUMENT_ON)		/* turn instrumentation on */
	{
	if (wvObjIsEnabled == TRUE)
	    return (INSTRUMENT_ON);
	wvObjIsEnabled = TRUE;
	return (INSTRUMENT_OFF);
	}
    else if (mode == INSTRUMENT_OFF)	/* turn instrumentation off */
        {
        if (wvObjIsEnabled == FALSE)
            return (INSTRUMENT_OFF);
        wvObjIsEnabled = FALSE;
        return (INSTRUMENT_ON);
        }
    else
        return (ERROR);
    }

/*******************************************************************************
*
* wvObjInst - instrument objects (WindView)
*
* This routine instruments a specified object or set of objects and has
* effect when system objects have been enabled for event logging.
*
* <objType> can be set to one of the following: OBJ_TASK (tasks), OBJ_SEM 
* (semaphores), OBJ_MSG (message queues), or OBJ_WD (watchdogs).
* <objId> specifies the identifier of the particular object to be instrumented.
* If <objId> is NULL, then all objects of <objType> have instrumentation
* turned on or off depending on the value of <mode>. 
*
* If <mode> is INSTRUMENT_ON, instrumentation is turned on; if it is any 
* other value (including INSTRUMENT_OFF) then instrumentation is turned off
* for <objId>.
*
* Call wvObjInstModeSet() with INSTRUMENT_ON if you want to enable
* instrumentation for all objects created after a certain place in your code.
* Use wvSigInst() if you want to enable instrumentation for all signal activity.
*
* This routine has effect only if INCLUDE_WINDVIEW is defined in
* configAll.h.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: wvSigInst(), wvEventInst(), wvObjInstModeSet()
*/

STATUS wvObjInst
    (
    int    objType,			/* object type */
    void * objId,			/* object ID or NULL for all objects */
    int    mode 			/* instrumentation mode */
    )
    {
    if (objId == NULL)	/* all objects of the same type */
        {
        if (mode == INSTRUMENT_ON)
            {
            /* Turn on instrumentation */
            switch (objType)
                {
                case OBJ_TASK:
                    taskClassId->instRtn = (FUNCPTR) evtLogOInt;
                    break;
                case OBJ_SEM:
                    semClassId->instRtn = (FUNCPTR) evtLogOInt;
                    break;
                case OBJ_MSG:
                    msgQClassId->instRtn = (FUNCPTR) evtLogOInt;
                    break;
                case OBJ_WD:
                    wdClassId->instRtn = (FUNCPTR) evtLogO;
                    break;
                case OBJ_MEM:
                    memPartClassId->instRtn = (FUNCPTR) evtLogOInt;
                    break;
                default:
                    return(ERROR);
                }
            }
        else
            {
	    /* Turn off instrumentation */
            switch (objType)
                {
                case OBJ_TASK:
                    taskClassId->instRtn = (FUNCPTR) NULL;
                    break;
                case OBJ_SEM:
                    semClassId->instRtn = (FUNCPTR) NULL;
                    break;
                case OBJ_MSG:
                    msgQClassId->instRtn = (FUNCPTR) NULL;
                    break;
                case OBJ_WD:
                    wdClassId->instRtn = (FUNCPTR) NULL;
                    break;
                case OBJ_MEM:
                    memPartClassId->instRtn = (FUNCPTR) NULL;
                    break;
                default:
                    return(ERROR);
                }
            }
	}
    else			/* per object basis */
        {
        /* verify the object */
	switch (objType)
            {
            case OBJ_TASK:
                {
                if (TASK_ID_VERIFY ((WIND_TCB *) objId) == ERROR)
	            return (ERROR);
	        break;
	        }
            case OBJ_SEM:
                {
                if (OBJ_VERIFY ((SEM_ID) objId, semClassId) == ERROR)
	            return (ERROR);
		break;
		}
            case OBJ_MSG:
                {
                if (OBJ_VERIFY ((MSG_Q_ID) objId, msgQClassId) == ERROR)
		    return (ERROR);
		break;
		}
            case OBJ_WD:
                {
                if (OBJ_VERIFY ((WDOG_ID) objId, wdClassId) == ERROR)
		    return (ERROR);
		break;
		}
           case OBJ_MEM:
                {
                if (OBJ_VERIFY ((PART_ID) objId, memPartClassId) == ERROR)
	            return (ERROR);
		break;
		}
            default:
                return(ERROR);
            }

        /* the event logging routine is set/reset in the object's class */
        if (mode == INSTRUMENT_ON)
            {
	    /* change to instrumented class */
            if (TASK_ID_VERIFY ((WIND_TCB *) objId) == ERROR)
	        {
                if (OBJ_EVT_RTN (objId) == (FUNCPTR) NULL)
                    {
	            (((OBJ_ID)(objId))->pObjClass) =
                        (struct obj_class *)
                        (((OBJ_ID)(objId))->pObjClass->initRtn);
                    }
		}
	    else
		{
                if (TASK_EVT_RTN (objId) == (FUNCPTR) NULL)
                    {
		    ((OBJ_ID)(&((WIND_TCB *)(objId))->objCore))->pObjClass =
		        (struct obj_class *) (((OBJ_ID)
			(&((WIND_TCB *)(objId))->objCore))->pObjClass)->initRtn;
	 	    }
		}
            }
        else
            {
	    /* change to non-instrumented class */
	    /* change to instrumented class */
            if (TASK_ID_VERIFY ((WIND_TCB *) objId) == ERROR)
	        {
                if (OBJ_EVT_RTN (objId) != (FUNCPTR) NULL)
                    {
	            (((OBJ_ID)(objId))->pObjClass) =
                        (struct obj_class *)
                        (((OBJ_ID)(objId))->pObjClass->initRtn);
                    }
		}
	    else
	        {
                if (TASK_EVT_RTN (objId) != (FUNCPTR) NULL)
                    {
		    ((OBJ_ID)(&((WIND_TCB *)(objId))->objCore))->pObjClass =
	                (struct obj_class *) (((OBJ_ID)
			(&((WIND_TCB *)(objId))->objCore))->pObjClass)->initRtn;
                    }
		}
            }
    	return (OK);
        }	
    return (ERROR);	/* instrumentation is off */
    }

/*******************************************************************************
*
* wvSigInst - instrument signals (WindView)
*
* This routine instruments all signal activity.
*
* If <mode> is INSTRUMENT_ON, instrumentation for signals is turned on;
* if it is any other value (including INSTRUMENT_OFF), instrumentation for
* signals is turned off.
*
* This routine has effect only if INCLUDE_WINDVIEW is defined in
* configAll.h and event logging has been enabled for system objects.
*
* INTERNAL
* Because signals are not implemented as objects, they need their own logging
* mechanism.
*
* RETURNS: OK or ERROR.
*/

STATUS wvSigInst
    (
    int   mode 		/* instrumentation mode */
    )
    {
    if (mode == INSTRUMENT_ON)
        {

	/* set signal event routine */

        sigEvtRtn = (VOIDFUNCPTR) evtLogOInt;
        }
    else if (mode == INSTRUMENT_OFF)
        {

        /* reset signal routine */

        sigEvtRtn = NULL;
        }
    else
        return (ERROR);
    return (OK);
    }

/*******************************************************************************
*
* wvEventInst - instrument VxWorks Events (WindView)
*
* This routine instruments VxWorks Event activity.
*
* If <mode> is INSTRUMENT_ON, instrumentation for VxWorks events is turned on;
* if it is any other value (including INSTRUMENT_OFF), instrumentation for
* VxWorks Events is turned off.
*
* This routine has effect only if INCLUDE_WINDVIEW is defined in
* configAll.h and event logging has been enabled for system objects.
*
* INTERNAL
* Because events are not implemented as objects, they need their own logging
* mechanism.
*
* RETURNS: OK or ERROR.
*/

STATUS wvEventInst
    (
    int   mode 		/* instrumentation mode */
    )
    {
    if (mode == INSTRUMENT_ON)
        {

	/* set event logging routine */

        eventEvtRtn = (VOIDFUNCPTR) evtLogOInt;
        }
    else if (mode == INSTRUMENT_OFF)
        {

        /* reset logging routine */

        eventEvtRtn = NULL;
        }
    else
        return (ERROR);
    return (OK);
    }


/*******************************************************************************
*
* wvEvent - log a user-defined event (WindView)
*
* This routine logs a user event.  Event logging must have been started with
* wvEvtLogEnable() or from the WindView GUI to use this routine.  
* The <usrEventId> should be in the
* range 0-25535.  A buffer of data can be associated with the event;
* <buffer> is a pointer to the start of the data block, and <bufSize> is its
* length in bytes.  The size of the event buffer configured with
* wvInstInit() should be adjusted when logging large user events.
* 
* RETURNS: OK, or ERROR if the event can not be logged.
*
* SEE ALSO: dbgLib, e()
*
* INTERNAL
* This event logging routine stores
*       event id        (short )
*       timestamp       (int)
*       address         (int)   - always NULL
*       buffer size     (int)   - if size is ZERO the buffer does not exist.
*       buffer          (char)
*
*/

STATUS wvEvent
    (
    event_t    usrEventId,      /* event */
    char *     buffer,          /* buffer */
    size_t     bufSize          /* buffer size */
    )
    {
    if (WV_ACTION_IS_SET)
	return (evtLogPoint (usrEventId, NULL, bufSize, buffer));
    else
	return (OK);
    }

/*******************************************************************************
*
* uploadPathWrite - write data to upload path
*
* This routine attempts to write the data pointed to by pData to the uplaod
* path indicated by pathId.  If the data can not be written due to a blocking
* error such as EWOULDBLOCK or EAGAIN, this routine will delay for some time
* using taskDelay(), and retry until the data is written.  If the data can
* not be written within a reasonable number of attempts, or an error other
* than EWOULDBLOCK or EAGAIN is generated while writing, this routine will
* give up and return an error.
*
* RETURNS: OK, or ERROR if the data could not be written in a reasonable
*          number of attempts, or if invalid pathId
*
* SEE ALSO:
* NOMANUAL
*
*/

static STATUS uploadPathWrite
    (
    UPLOAD_ID     pathId,               /* path to write to */
    char         *pData,                /* buffer of data to write */
    int           nBytes                /* number bytes to write */
    )
    {
    int   trys;                         /* to count the attempts */
    int   nToWrite;                     /* number bytes left to write  */
    int   nThisTry;                     /* number bytes written this try */
    int   nWritten;                     /* total bytes written so far */
    char *buf;                          /* local copy of pData */

    trys     = 0;
    nWritten = 0;
    nThisTry = 0;
    nToWrite = nBytes;
    buf      = pData;

    if (pathId == NULL || pathId->writeRtn == NULL)
        return (ERROR);

    while (nWritten < nBytes && trys < wvUploadMaxAttempts)
        {
        if ((nThisTry = pathId->writeRtn (pathId, buf, nToWrite)) < 0)
            {

            /* If write failed because it would have blocked, allow retry. */

            if (errno == EWOULDBLOCK || errno == EAGAIN)
                taskDelay (wvUploadRetryBackoff);
            else
                return (ERROR);
            }
        else
            {
            nWritten += nThisTry;
            nToWrite -= nThisTry;
            buf      += nThisTry;
            }

        ++trys;
        }

    /* Check if write failed to complete within the max number of tries. */

    if (nWritten < nBytes)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* wvUpload - transfer events to the host (WindView)
*
* This routine is used to upload events stored in the event buffer to the
* host.  Events are read from the buffer using the routines and information
* referenced by the BUFFER_ID.  The upload path is accessed using
* routines referenced by the UPLOAD_ID.  Information useful for making this
* routine exit when desired, and communicating when it has exited is 
* contained in the WV_UPLOADTASK_ID.
*
* When exitWhenEmpty, within the WV_UPLOADTASK_ID, is set to true, wvUpload
* will upload all available events, and then return immediately. If this
* value is set to false, wvUpload will never exit, but it will block on the
* buffer's semaphore until sufficient data becomes available to upload.
* Setting exitWhenEmpty to false is used to continuously upload events as
* they are logged to the buffer.  The semaphore uploadCompleteSem, also
* contained in the WV_UPLOADTASK_ID, is given when the this routine has
* emptied the buffer.  This allows others to wait until the buffer is
* empty.
*
* If an error occurs when reading from the buffers or writing to the upload
* path, event logging is stopped and the upload path is closed.  The status
* field in the WV_UPLOADTASK_ID indicates whether the upload was successful
* or if an error occurred.  When writing to the upload path, if an EAGAIN
* or EWOULDBLOCK error is encountered due to a failure to write to a non-
* blocking path, this routine will backoff and retry to write to the path.
* The number of times to attempt writing, and the the amount of time to
* delay between attempts can be set with the global variables
*
* 	int wvUploadMaxAttempts		-- 200 by default
* 	int wvUploadRetryBackoff 	-- 6 by default (in ticks)
*
* If the block of data, usually one threshold in size, can not be written
* within the maximum number of attempts, this routine will stop event 
* logging, close the upload path and indicate the error within the status
* field of the WV_UPLOADTASK_ID.
*
* To avoid copying the event data to a local buffer, only to be copied again
* to the upload path, reading the buffer is done using a reserve/commit
* interface.  Reserving a number of bytes to read from the buffer provides
* access, via a pointer, directly to the event data in the buffer.  This
* pointer references only contiguous memory and can be passed directly to the
* upload mechanism.  Hence no extra copying is employed.  The buffer's
* commit interface is used to notify the buffering mechanism that the
* reserved memory has been consumed.
*
* The semaphore on which wvUpload blocks works together with the threshold,
* also part of the BUFFER_ID, as a general purpose mechanism for telling
* users of the buffer when threshold bytes of data are contained in the
* buffer.  This gives the the uploader a way of controlling the frequency 
* and size of uploads.  The threshold is assigned by the buffer mechanism,
* but can be an arbitrary value.  In general, the buffer may assign threshold
* to meet a buffer contraint, or it may allow the user to assign it, and 
* allow access to its value in the structure pointed to by the BUFFER_ID.
*
* All buffer accesses are protected with interrupts locked out.
*
* RETURNS: OK when buffer is successfully emptied and uploaded and
*          the upload task ID's exitWhenEmpty is set to TRUE.
*          ERROR if an error occurs with the upload path or buffers; event
*          logging is stopped if an error occurs.
*
* SEE ALSO:
* NOMANUAL
*
*/

static STATUS wvUpload 
    (
    WV_UPLOADTASK_ID    upTaskId	/* this task's descriptor */
    )
    {
    int       nReserved;	/* num bytes reserved and possible to upload */
    int       nToWrite;		/* num bytes that should be written to host */
    int       nUploaded;	/* number of bytes actually written to host */
    int       nToCommit;	/* number of bytes to commit in the buffer */
    int	      tmp;		/* temporary used to count uploaded bytes */
    BOOL      bufferEmpty; 	/* used to exit when buffer is empty */
    UINT8     *pData;		/* pointer to the read-reserved data */
    int       lockKey;		/* for interrupt locking */
    BUFFER_ID bufId;		/* convenient access to upTaskId->bufferId */
    UPLOAD_ID pathId;		/* same for upTaskId->uploadPathId */

    /* Check that the upload-task id has appropriate information. */

    if (upTaskId == NULL)
	return (ERROR);

    if (upTaskId->bufferId == NULL)
	{
	upTaskId->status = ERROR;
	return (ERROR);
	}

    if (upTaskId->uploadPathId == NULL)
	{
	upTaskId->status = ERROR;
	return (ERROR);
	}


    /* Initialize variables. */

    bufId   = upTaskId->bufferId;
    pathId  = upTaskId->uploadPathId;

    bufferEmpty = FALSE;
    nToWrite    = 0;

    while (TRUE)
        {

        /* 
	 * Initialize variables that allow us to stop uploading at the right
	 * time.  Assuming that there is data in the buffer (bufferEmtpy =
	 * FALSE) is alright because nothing below will error if there is
	 * not.  In other words, bufferEmpty accurately describes when the
	 * buffer is empty but does not accurately describe when the buffer
	 * is not empty.
	 */

        nUploaded = 0;
	bufferEmpty = FALSE;

	/* 
	 * If the caller requested to exit when the buffer is empty, give
	 * him/er the result immediately, even if there is no data available.
	 * Also, if there is still more than a threshold of data in the buffer,
	 * continue uploading.  Otherwise block until sufficient data becomes
	 * available.
	 */

        lockKey = intLock ();
	tmp = bufId->nBytesRtn (bufId);
	intUnlock (lockKey);

        if (! upTaskId->exitWhenEmpty && (tmp < bufId->threshold))
	    semTake (& bufId->threshXSem, WAIT_FOREVER);

	/*
	 * Upload in chunks of size bufId->threshold.  The readReserve
	 * routine of the buffer only guarantees that every time the
	 * threshXSem is able to be taken there are threshold bytes of data
	 * in the buffer.  That data may not be contiguous, and so we have
	 * to loop until threshold bytes have been uploaded.
         */
        
        while ((nUploaded < bufId->threshold) && !bufferEmpty)
	    {
	     
            /* Reserve as many contiguous bytes as possible from the buffer. */

	    lockKey = intLock ();
            nReserved = bufId->readReserveRtn (bufId, &pData);
	    intUnlock (lockKey);

            if (nReserved == ERROR)
	        {

	        /* 
	         * If an error occurs close the upload path and stop event
		 * logging.  There is no harm in turning event logging off
		 * if it is not currently on.
	         */

                lockKey = intLock ();
		wvEvtLogStop();
		intUnlock (lockKey);

		pathId->errorRtn (pathId);
	        upTaskId->status = ERROR;
		logMsg ("tWvUpload: failed to read from buffer.\n", 
			0, 0, 0, 0, 0, 0);
		return (ERROR);
                }

            /* Notice when the buffer is emptied. */

            if (nReserved == 0)
		bufferEmpty = TRUE;
            else
		bufferEmpty = FALSE;

 	    /* 
	     * Write as many bytes as possible that still need to be uploaed,
	     * to the host, but not more than threshold.  And, remember how 
	     * many to commit later.
	     */

            nToWrite = (nReserved > (bufId->threshold - nUploaded)) ? 
		       (bufId->threshold - nUploaded) : nReserved;
            nToCommit = nToWrite;

            if (uploadPathWrite (pathId, pData, nToWrite) == ERROR)
                {

                /*
                 * There is still no harm in turning off event logging
                 * if it is not currently on.
                 */

                lockKey = intLock ();
                wvEvtLogStop();
                intUnlock (lockKey);

                pathId->errorRtn (pathId);
                upTaskId->status = ERROR;
                logMsg ("tWVUpload: failed writing to host.\n",
                        0, 0, 0, 0, 0, 0);
                return (ERROR);
                }

            /* 
	     * At this point all the reserved bytes that were possible to 
	     * read in this pass through the loop have been uploaded.  Those
	     * bytes are committed before determining whether to loop again.
	     */

            lockKey = intLock ();
            tmp = bufId->readCommitRtn (bufId, nToCommit);
	    intUnlock (lockKey);
 
            if (tmp == ERROR)
		{

		/* 
		 * There is no harm in turning logging off if it is already 
		 * off.
		 */

                lockKey = intLock ();
		wvEvtLogStop();
		intUnlock (lockKey);

		pathId->errorRtn (pathId);
	        upTaskId->status = ERROR;
		logMsg ("tWvUpload: failed to commit uploaded bytes.\n", 
			0, 0, 0, 0, 0, 0);
		return (ERROR);
                }

            nUploaded += nToCommit;
	    }

            /* 
	     * At this point the buffer is empty or threshold bytes have been
	     * uploaded.  Exit only if the buffer was emptied and the user 
	     * requested not to wait for more data.
	     */

            if (upTaskId->exitWhenEmpty && bufferEmpty)
		{
	        upTaskId->status = OK;
		semGive (& upTaskId->uploadCompleteSem);
		return (OK);
		}
        }
    }

/*******************************************************************************
*
* wvUploadStart - start upload of events to the host (WindView)
*
* This routine starts uploading events from the event buffer to the host.
* Events can be uploaded either continuously or in one pass until the
* buffer is emptied.  If <uploadContinuously> is set to TRUE, the task
* uploading events pends until more data arrives in the buffer.  If FALSE,
* the buffer is flushed without waiting,  but this routine 
* returns immediately with an ID that can be used to kill the upload task.
* Upload is done by spawning the task 'tWVUpload'.  The buffer to upload is 
* identified by <bufId>, and the upload path to use is identified by <pathId>.
*
* This routine blocks if no event data is in the buffer, so it should
* be called before event logging is started to ensure the buffer does
* not overflow.
*
* RETURNS: A valid WV_UPLOADTASK_ID if started for continuous
* upload, a non-NULL value if started for one-pass upload, and NULL 
* if the task can not be spawned or memory for the descriptor 
* can not be allocated.
*
*/

WV_UPLOADTASK_ID wvUploadStart 
    (
    BUFFER_ID bufId,	     /* event data buffer ID */
    UPLOAD_ID pathId,	     /* upload path to host */
    BOOL uploadContinuously  /* upload continuously if true */
    )
    {
    WV_UPLOADTASK_ID upTaskId;	/* returned to later identify the upload task */

    /* Check for valid parameters. */

    if (bufId == NULL || pathId == NULL)
	return (NULL);

    /* 
     * Create and initialize the upload task descriptor to return to the 
     * user.  This id is necessary for stopping the task later.
     */

    if ((upTaskId = (WV_UPLOADTASK_ID) 
		    malloc (sizeof (WV_UPLOADTASK_DESC))) == NULL)
        {
	logMsg ("wvUploadStart: can't alloc uploadTask id memory.\n",
		 0,0,0,0,0,0);
	return (NULL);
	}

    if (semBInit (& upTaskId->uploadCompleteSem, SEM_Q_PRIORITY, 
		  SEM_EMPTY) == ERROR)
        {
	logMsg ("wvUploadStart: can't init uploadTask sem.\n",0,0,0,0,0,0);
	return (NULL);
	}

    upTaskId->bufferId          = bufId;
    upTaskId->uploadPathId      = pathId;
    upTaskId->status		= OK;

    if (uploadContinuously)
        upTaskId->exitWhenEmpty = FALSE;
    else
        upTaskId->exitWhenEmpty = TRUE;

    /* Now spawn the task. */

    if ((upTaskId->uploadTaskId = taskSpawn ("tWVUpload", wvUploadTaskPriority, 
				             wvUploadTaskOptions,
		                             wvUploadTaskStackSize, wvUpload,
					     (int) upTaskId, 0, 0, 0, 0, 0,
					     0, 0, 0, 0)) == ERROR)
        {
	logMsg ("wvUploadStart: can't spawn uploadTask.\n",0,0,0,0,0,0);
	return (NULL);
        }

    return (upTaskId);
    }

/*******************************************************************************
*
* wvUploadStop - stop upload of events to host (WindView)
*
* This routine stops continuous upload of events to the host.  It does this
* by making a request to the upload task to terminate after it has emptied
* the buffer.  For this reason it is important to make sure data is no
* longer being logged to the buffer before calling this routine.
* 
* This task blocks until the buffer is emptied, and then frees memory
* associated with <upTaskId>.
*
* RETURNS: OK if the upload task terminates successfully, 
* or ERROR either if <upTaskId> is invalid or if the upload task terminates
* with an ERROR.
*
*/

STATUS wvUploadStop 
    (
    WV_UPLOADTASK_ID upTaskId
    )
    {
    STATUS retStatus;

    if (upTaskId == NULL)
	return (ERROR);

    /* 
     * Ask the upload task to flush the buffer and then exit.  The upload
     * task may have emptied the buffer and be waiting on the buffer's
     * threshold-crossed semaphore, so we give that semaphore so the task
     * won't pend forever.
     */

    upTaskId->exitWhenEmpty = TRUE;
    semGive (& upTaskId->bufferId->threshXSem);

    /* Wait for flushing to complete, but only if uploader worked correctly */

    if (upTaskId->status == OK)
        semTake (& upTaskId->uploadCompleteSem, WAIT_FOREVER);

    /* Free up the memory associated with the upload descriptor. */

    semTerminate (& upTaskId->uploadCompleteSem);
    retStatus = upTaskId->status;
    free (upTaskId);

    return (retStatus);
    }

/*******************************************************************************
*
* wvUploadTaskConfig - set priority and stacksize of 'tWVUpload' task (WindView)
*
* This routine sets the stack size and priority of future instances of 
* the event-data upload task, created by calling wvUploadStart().  The default
* stack size for this task is 5000 bytes, and the default priority is 150.
* 
* RETURNS: N/A
*/

void wvUploadTaskConfig
    (
    int stackSize,		/* the new stack size for tWVUpload */
    int priority		/* the new priority for tWVUpload */
    )
    {
    wvUploadTaskStackSize = stackSize;
    wvUploadTaskPriority  = priority;
    }

/*******************************************************************************
*
* wvLogHeaderCreate - create the event-log header (WindView)
*
* This routine creates the header of EVENT_CONFIG, EVENT_BUFFER, and EVENT_BEGIN
* events that is required at the beginning of every event log.  These events are
* stored in a packed array allocated from the specified memory partition.
* In addition to this separate header, this routine also logs all tasks
* active in the system to the event buffer for uploading along with the
* other events.
*
* This routine should be called after wvEvtLogInit() is called.  If uploading
* events continuously to the host, this routine should be called after the 
* upload task is started.  This ensures that the upload task is included in 
* the snapshot of active tasks.  If upload will occur after event logging has 
* stopped (deferred upload), this routine can be called any time before event 
* logging is turned on.
*
* RETURNS: A valid WV_LOG_HEADER_ID, or NULL if memory can not be allocated.
*/

WV_LOG_HEADER_ID wvLogHeaderCreate 
    (
    PART_ID   memPart		/* partition where header should be stored */
    )
    {
    WV_LOG_HEADER *pHead;	/* local copy of the log header struct */
    int		  *pIntCur;	/* integer pointer into buf */
    short	  *pShortCur;	/* short pointer into buf */
    char	  *pByteCur;	/* char pointer into buf */


    short cfgEventId;           /* EVENT_CONIG */
    int   cfgProtocolRev;
    int   cfgTimestampFreq;
    int   cfgTimestampPeriod;
    int   cfgAutoRollover;
    int   cfgClkRate;
    int   cfgCollectionMode;
    int   cfgProcessorNum;

    short bufEventId;           /* EVENT_BUFFER */
    int   bufTaskIdCurrent;

    short begEventId;           /* EVENT_BEGIN */
    int   begCpu;
    int   begBspSize;           /* char *begBspName copied directly to buf */
    int   begTaskIdCurrent;
    int   begCollectionMode;
    int   begRevision;

    char  commentString[] = "Windview Version 2.2 Logfile.  Copyright \
 1999-2001 Wind River Systems.";

    int   commentLen;
    int   bspLen;
    
    /* 
     * Get the actual log header structure first, and then figure out how
     * big to make it, considering the variable-length bspName field.
     */

    pHead = (WV_LOG_HEADER *) memPartAlloc (memPart, sizeof (WV_LOG_HEADER));

    if (pHead == NULL)
	return (NULL);

    commentLen = strlen (commentString);

    if (commentLen & 1)
        commentLen++;
    
    bspLen = strlen (sysModel ());

    if (bspLen & 1)
        bspLen++;
    
    pHead->len = (EVENT_BEGIN_SIZE + EVENT_CONFIG_SIZE + EVENT_BUFFER_SIZE +
		  commentLen + EVENT_COMMENT_SIZE +
                  bspLen);

    if (pHead->len & 1)
	++pHead->len;

    pHead->header = memPartAlloc (memPart, pHead->len);

    if (pHead->header == NULL)
	{
	memPartFree (memPart, (char *) pHead);
	return (NULL);
	}

    pHead->memPart = memPart;


    /*
     * Get memory for an EVENT_CONFIG buffer from the event buffer, using
     * the writeReserve interface.  Then fill in that memory.
     */

    cfgEventId         = EVENT_CONFIG;
    cfgProtocolRev     = WV_REV_ID_CURRENT | WV_EVT_PROTO_REV_CURRENT;
    cfgTimestampFreq   = (* _func_tmrFreq) ();
    cfgTimestampPeriod = (* _func_tmrPeriod) ();
    cfgAutoRollover    = ((* _func_tmrConnect) ((FUNCPTR) _func_evtLogT0,
                                                EVENT_TIMER_ROLLOVER)) + 1;
    cfgClkRate         = sysClkRateGet ();
    cfgCollectionMode  = (int)(wvEvtClass & WV_CLASS_3);
    cfgProcessorNum    = sysProcNumGet ();

    pShortCur = (short *) pHead->header;

    EVT_STORE_UINT16 (pShortCur, cfgEventId);
    pIntCur   = (int *) pShortCur;
    EVT_STORE_UINT32 (pIntCur, cfgProtocolRev);
    EVT_STORE_UINT32 (pIntCur, cfgTimestampFreq);
    EVT_STORE_UINT32 (pIntCur, cfgTimestampPeriod);
    EVT_STORE_UINT32 (pIntCur, cfgAutoRollover);
    EVT_STORE_UINT32 (pIntCur, cfgClkRate);
    EVT_STORE_UINT32 (pIntCur, cfgCollectionMode);
    EVT_STORE_UINT32 (pIntCur, cfgProcessorNum);

    /*
     * Log the EVENT_LOGCOMMENT event to the buffer.
     */

    begEventId        = EVENT_LOGCOMMENT;

    pShortCur = (short *) pIntCur;

    EVT_STORE_UINT16 (pShortCur, begEventId);
    pIntCur   = (int *) pShortCur;
    EVT_STORE_UINT32 (pIntCur, commentLen);

    pByteCur = (char *) pIntCur;
    strncpy (pByteCur, commentString, commentLen);
    pByteCur += commentLen;

    pIntCur   = (int *) pByteCur;

    /*
     * Do the same for an EVENT_BUFFER.  Reserve memory for the event from
     * the event buffer, and then fill in that memory.
     */

    bufEventId       = EVENT_BUFFER;
    bufTaskIdCurrent = (kernelIsIdle) ? (int) ERROR : (int) taskIdCurrent;

    pShortCur = (short *) pIntCur;

    EVT_STORE_UINT16 (pShortCur, bufEventId);
    pIntCur   = (int *) pShortCur;
    EVT_STORE_UINT32 (pIntCur, bufTaskIdCurrent);

    /*
     * Log the EVENT_BEGIN event to the buffer.  The cpu-type member is
     * tricky because it has to be of even length to transfer to the host.
     * Also the memory reserved has to vary by its length.
     */

    begEventId        = EVENT_BEGIN;
    begCpu            = sysCpu;

    begBspSize        = bspLen;
    begTaskIdCurrent  = (int) taskIdCurrent;
    begCollectionMode  = (int)(wvEvtClass & WV_CLASS_3);
    begRevision       = WV_EVT_PROTO_REV_CURRENT;

    pShortCur = (short *) pIntCur;

    EVT_STORE_UINT16 (pShortCur, begEventId);
    pIntCur   = (int *) pShortCur;
    EVT_STORE_UINT32 (pIntCur, begCpu);
    EVT_STORE_UINT32 (pIntCur, begBspSize);

    pByteCur = (char *) pIntCur;
    strncpy (pByteCur, sysModel(), begBspSize);
    pByteCur += begBspSize;

    pIntCur   = (int *) pByteCur;
    EVT_STORE_UINT32 (pIntCur, begTaskIdCurrent);
    EVT_STORE_UINT32 (pIntCur, begCollectionMode);
    EVT_STORE_UINT32 (pIntCur, begRevision);


    /* Log a snapshot of active tasks to the event buffer. */

    evtLogTasks ();

    return (pHead);
    }

/*******************************************************************************
*
* wvLogHeaderUpload - transfer the log header to the host (WindView)
*
* This functions transfers the log header events (EVENT_BEGIN, EVENT_CONFIG,
* EVENT_BUFFER) to the host.  These events were saved to a local buffer
* with the call to wvLogHeaderCreate().  This routine should be called before
* any events or tasknames are uploaded to the host.  The events in the 
* header buffer must be the first things the parser sees.
*
* If continuously uploading events, it is best to start the uploader, and 
* then call this routine.  If deferring upload until after event logging
* is stopped, this should be called before the uploader is started.
*
* RETURNS: OK, or ERROR if there is trouble with the upload path.
*/

STATUS wvLogHeaderUpload
    (
    WV_LOG_HEADER_ID pHeader,		/* pointer to the header */
    UPLOAD_ID pathId			/* path by which to upload to host */
    )
    {
    int 	   res;			/* partly notice write errors */

    if (pHeader == NULL || pathId == NULL)
	return (ERROR);

    res = pathId->writeRtn (pathId, pHeader->header, pHeader->len);

    if (res < 0)
	return (ERROR);

    if (pmPartId != NULL)
        {

    	/* Free the header. */

    	memPartFree (pHeader->memPart, (char *) pHeader->header);
    	memPartFree (pHeader->memPart, (char *) pHeader);
	}

    return (OK);
    }

/*******************************************************************************
*
* wvEvtBufferGet - return the ID of the WindView event buffer (WindView)
*
* RETURNS: The event buffer ID if one exists, otherwise NULL.
*/

BUFFER_ID wvEvtBufferGet (void)
    {
    return ((wvEvtBufferId == 0) ? (BUFFER_ID) NULL : wvEvtBufferId);
    }

/*******************************************************************************
*
* wvTaskNamesPreserve - preserve an extra copy of task name events (WindView)
*
* This routine initializes the data structures and instrumentation necessary
* to allow WindView to store an extra copy of each EVENT_TASKNAME event,
* which is necessary for post-mortem analysis.  This routine should be called
* after wvEvtLogInit() has been called, and before event logging is started.
*
* If this routine is called before event logging is started, all
* EVENT_TASKNAME events that are produced by VxWorks are logged into the
* standard event buffer, and a copy of each is logged automatically to the
* task name buffer created by this routine.  All tasks running when this 
* routine is called are also added to the buffer.  The events in this buffer
* can be uploaded after the other events have been uploaded, to provide the
* task names for any events in the log which no longer have a corresponding
* task name event due to wrapping of data in the buffers.  Because there
* may be two copies of some of the task name events after the buffer data
* wraps around, the resultant log may have two task name events for the same
* task.  This is not a problem for the parser.
*
* Occasionally the task ID of a task is reused, and in this case, only
* the last instance of the task name event with a particular task ID is
* maintained.
*
* The buffer size must be a power of two.
*
* This routine sets the event class WV_CLASS_TASKNAMES_PRESERVE, which can
* be turned off by calling wvEvtClassClear() or wvEvtClassSet().
*
* INTERNAL
* This routine returns the buffer's ID, which is required later to
* destroy and upload the buffer.  The ID is not required to add an event
* to the buffer.  That is because additions to the buffer will be done
* from an event point, and the event point will not have access to the
* buffer's ID.  This is exactly how the event buffer is accessed in 
* evtLogLib.
*
* The buffer used to store the reserved taskname events is a hash table that 
* is implemented in wvLib.  All functions associated with this hash table are
* prefixed with tnHashTbl.  The implementation of the reserved taskname buffer
* has been hidden (albeit barely) from this function, so that any implemen-
* tation of a buffer can be used.  The type of the buffer is identified as 
* a TASKBUF_ID.  The specific implementation of the hash-table buffer is 
* a TN_HASH_TBL.
* 
* A hash table has been used because, given very specific circumstances, the
* task IDs of tasks with different names may be reused.  A hash table allows
* easy and fast replacement of the names, so that there is only one name per
* tid in the table at any time.  As new names, corresponding to a given tid,
* are added, the event is replaced with the newest event.
*
* The hash table was used to avoid multiple taskname events with the same tid,
* however there may be a copy of the same event still in the buffer, providing
* it has not been overwritten.  In this case there will be two of the same
* events anyway, so the hash table is overkill.  Oh well.
*
* RETURNS: A valid TASKBUF_ID to be used for later uploading, or NULL if 
* not enough memory exists to create the task buffer.
*/

TASKBUF_ID wvTaskNamesPreserve
    (
    PART_ID memPart,		/* memory where preserved names are stored */
    int	    size 		/* must be a power of 2 */
    )
    {
    int 	   nTasks;			/* number of active tasks */
    int		   idList [MAX_WV_TASKS]; 	/* list of active task IDs */
    int		   ix;				/* counting index */
    TN_HASH_TBL   *pTbl;			/* the taskname buffer */

    /* Create the buffer where names will be stored. */

    if ((pTbl = tnHashTblCreate (memPart, size)) == NULL)
	return (NULL);

    /* 
     * Stash a copy of the the buffer id for table adds, directly from
     * event points later.   This has to be done before we add any names.
     */

    tnHashTbl   = pTbl;

    /* Store a copy of each taskname already running in the system. */

    nTasks = taskIdListGet (idList, NELEMENTS (idList));

    for (ix = 0; ix < nTasks; ++ix)
	{
	if (taskIdVerify (idList [ix]) == OK)
	    {
	    wvTaskNamesBufAdd (EVENT_TASKNAME,
			       ((WIND_TCB *)idList[ix])->status,
			       ((WIND_TCB *)idList[ix])->priority,
			       ((WIND_TCB *)idList[ix])->lockCnt,
			       idList [ix], taskName (idList [ix]));
            }
        }

    /* 
     * Let the event point know that it should reserve a copy of each 
     * future event. It will be turned off when event loggin is turned 
     * off or reset.
     */

    WV_EVTCLASS_SET (WV_CLASS_TASKNAMES_PRESERVE);

    return ((TASKBUF_ID) pTbl);
    }

/*******************************************************************************
*
* wvTaskNamesUpload - upload preserved task name events (WindView)
*
* This routine uploads task name events, saved after calling 
* wvTaskNamesPreserve(), to the host by the specified upload path.  There 
* is no particular order to the events uploaded.  All the events contained 
* in the buffer are uploaded in one pass.  After all have been uploaded, the 
* buffer used to store the events is destroyed.
*
* RETURNS: OK, or ERROR if the upload path or task name buffer is invalid.
*/

STATUS wvTaskNamesUpload 
    (
    TASKBUF_ID taskBufId, 		/* taskname event buffer to upload */
    UPLOAD_ID pathId			/* upload path id */
    )
    {
    TN_ITER_KEY   *pIterKey;		/* key to iterate over buffer */
    TN_EVENT      *pEvent;		/* event to upload */
    int            res = OK;		/* to track errors uploading */
    TN_HASH_TBL   *pTbl;		/* just in case a taskbuf is not a 
					   taskname hash table */

    if (taskBufId == NULL || pathId == NULL)
	return (ERROR); 

    /* 
     * If the buffer implemented to store taskname events is not a TN_HASH_TBL
     * then we need to do the casting here.  It usually is.
     */

    pTbl = (TN_HASH_TBL *) taskBufId;

    /* Create an iterator key to use to access all events in buffer. */

    if ((pIterKey = tnHashTblIterInit (pTbl)) == NULL)
	return (ERROR);

    while ((pEvent = tnHashTblIterNextEvent (taskBufId, pIterKey)) != NULL)
	{

        /* Write the values one at a time to eliminate padding in the log. */

        res = pathId->writeRtn (pathId, & pEvent->eventId, sizeof (short));
        res = pathId->writeRtn (pathId, & pEvent->status, sizeof (int));
        res = pathId->writeRtn (pathId, & pEvent->priority, sizeof (int));
        res = pathId->writeRtn (pathId, & pEvent->taskLockCount, sizeof (int));
        res = pathId->writeRtn (pathId, & pEvent->tid, sizeof (int));
        res = pathId->writeRtn (pathId, & pEvent->nameSize, sizeof (int));
        res = pathId->writeRtn (pathId,   pEvent->name, pEvent->nameSize);

        if (res < 0)
	    {
	    tnHashTblIterDone (pIterKey, pTbl);
            return (ERROR);
	    }
	}
    tnHashTblIterDone (pIterKey, pTbl);

    tnHashTblDestroy (pTbl);

    return (OK);
    }

/*******************************************************************************
*
* wvTaskNamesBufAdd - add a taskname event to the taskname buffer (WindView)
*
* This routine reserves a taskname event by adding it to the buffer used to
* store a copy of all such events for WindView post-mortem use.  The buffer
* must already have been created with wvTaskNamesPreserve().  If the task ID of
* the task event being added is already in the table, this routine replaces
* the existing copy of the event with the one being added.
*
* INTERNAL:
*
* The ID of the buffer is not passed as one of the parameters to this
* routine because this routine will most likely be called from the event
* point, which does not have access to the ID at logging time.  The other
* buffer-access routines, such as destroy and upload, require the buffer
* ID as a parameter, but we use a static copy of the ID here.
*
* Because this is called when creating a task, we are not in kernelState
* nor in an ISR.  Therefore, anything goes for getting memory and calling
* utility functions.  
*
* RETURNS: OK, or ERROR if memory can not be allocated or the
* task name buffer does not exist.
*
* NOMANUAL
*/

STATUS wvTaskNamesBufAdd
    (
    short eventId,		/* event values */
    int status,
    int priority,
    int taskLockCount,
    int tid,
    char *name
    )
    {
    TN_EVENT   *pEvent;		/* struct to hold event */
    TN_EVENT   *pReplaced;	/* iff replaced instead of added */

    /* Create an event structure to hold the event, and fill it. */

    pEvent = (TN_EVENT *) memPartAlloc (tnHashTbl->memPart, sizeof (TN_EVENT));

    if (pEvent == NULL)
        return (ERROR);

    pEvent->eventId       = eventId;
    pEvent->status        = status;
    pEvent->priority      = priority;
    pEvent->taskLockCount = taskLockCount;
    pEvent->tid           = tid;
    pEvent->nameSize      = strlen (name);
    pEvent->name = (char *) memPartAlloc (tnHashTbl->memPart, pEvent->nameSize);

    if (pEvent->name == NULL)
        {
        memPartFree (tnHashTbl->memPart, (char *) pEvent);
        return (ERROR);
        }

    strncpy (pEvent->name, name, pEvent->nameSize);

    /*
     * If the tid already exists replace the existing event with this
     * one.  Otherwise simply add it.  This is the policy of the insert
     * routine.
     */

    pReplaced = NULL;

    if (tnHashTblInsert (tnHashTbl, pEvent, pEvent->tid, pReplaced) == ERROR)
        {
        memPartFree (tnHashTbl->memPart, (char *) pEvent->name);
        memPartFree (tnHashTbl->memPart, (char *) pEvent);
        return (ERROR);
        }

    /* Check if the event was replaced, and if so free the returned event. */

    if (pReplaced != NULL)
        {
        memPartFree (tnHashTbl->memPart, (char *) pReplaced->name);
        memPartFree (tnHashTbl->memPart, (char *) pReplaced);
        }

    return (OK);
    }

/*******************************************************************************
*
* tnHashTblCreate - create hash table to store reserved task name events
*
* This routine creates a hash table that will hold the reserved copy of all
* task-name events generated by the system after wvTaskNamePreserve is
* called.  The hash table allows easy and fast lookup of the task name events
* based on a the task ID as a key.  It allocates the memory for the buffer 
* from the memory partition identified by memPart.
*
* The structure of the hash table is very similar to the tables implemented
* in hashLib.  Below is what this table looks like.
*
* .CS
*
*   TN_HASH_TBL
*   --------
*   | size |     ---------------------
*   | tbl  |---> | 0| 1| 2| 3| 4| 5| 6| ...
*   --------     ---------------------
*                  |
*        |----------
*        v
*  (dummy headers)
*
*     TN_NODE (0)        TN_NODE            TN_NODE
*     --------           --------           --------
*     | next |---------->| next |---------> | next |----------
*     | key=0|           | key  |           | key  |         |
*     | event|--         | event|---        | event|---      v
*     -------- |         --------  |        --------  |     ---
*              v                   v                  v      -
*             ---               TN_EVENT           TN_EVENT
*              -                -------            -------
*                               | ... |            | ... |
*                               | ... |            | ... |
*                               | ... |            | ... |
*                               -------            -------
*
*     TN_NODE (1)
*     ...
*     TN_NODE [size]
*
* .CE
* The size of the table must be a power of two.
*
* RETURNS: TN_HASH_TBL, or error if not enough memory could be allocated
*          to create the structure.
* NOMANUAL
*/

static TN_HASH_TBL * tnHashTblCreate
    (
    PART_ID memPart,             /* the memory partion where buf is placed */
    int tblSize                 /* size of the table */
    )
    {
    TN_HASH_TBL  *pTbl;		/* the head of the hash table */
    TN_NODE     **pEdge;	/* the array making the edge of pTbl */
    int ix;			/* counting index */
    int jx;			/* counting index */

    /* Create the table struct. */

    pTbl = (TN_HASH_TBL *) memPartAlloc (memPart, sizeof (TN_HASH_TBL));

    if (pTbl == NULL)
        return (NULL);

    /* Create the edge vector of the table. */

    pEdge = (TN_NODE **) memPartAlloc (memPart, tblSize * sizeof (TN_NODE *));

    if (pEdge == NULL)
        return (NULL);

    /* Create and initialize the dummy heads on the linked lists. */

    for (ix=0; ix<tblSize; ++ix)
        {
        pEdge[ix] = (TN_NODE *) memPartAlloc (memPart, sizeof (TN_NODE));

        if (pEdge[ix] == NULL)
            {
	    for (jx = 0; jx <= ix; ++jx)
		memPartFree (memPart, (char *) pEdge [ix]);
            memPartFree (memPart, (char *) pEdge);
            memPartFree (memPart, (char *) pTbl);
            return (NULL);
            }

        pEdge[ix]->next  = NULL;
        pEdge[ix]->key   = 0;
        pEdge[ix]->event = NULL;
        }

    /* Set the size, memPart, and edge vector of the table. */

    pTbl->size   = tblSize;
    pTbl->memPart = memPart;
    pTbl->tbl    = pEdge;

    return ((TASKBUF_ID) pTbl);
    }

/*******************************************************************************
*
* tnHash - return a unique index into the table based on the tid
*
* This is the hashing function for the hash table which reserves a copy
* of taskname events for windview post-mortem use.  The hash function
* simply returns the value of the key modulo 31 (an arbitrary divisor),
* masking the bits in the result to the size of the table.
*
* RETURNS: an index into the hash table unique to the key
*
* NOMANUAL
*/

static int tnHash
    (
    TN_HASH_TBL *tbl,           /* provides the size of the tbl */
    int key                     /* usually the tid */
    )
    {
    FAST int hash;
    hash = key % 31;
    return (hash & (tbl->size - 1));
    }

/*******************************************************************************
* 
* tnHashTblInsert - add a node to the hash table
*
* This routine inserts a node into the hash table that is used as the
* taskname event buffer.  It replaces the node if the key already exists
* in the table.  If the node was replaced, the replaced node is returned
* in pReplacedEvent, otherwise this value will be set to NULL.  This
* provides an easy way to let the caller free that memory.
*
* The hash table is described in the function header of tnHashTblCreate.
*
* RETURNS:  OK, or ERROR if failed to allocate memory from the partition
*
* NOMANUAL
*/

static STATUS tnHashTblInsert
    (
    TN_HASH_TBL   *pTbl,		/* the hash table to add to */
    TN_EVENT      *pEvent,		/* the event data to add */
    int            key,			/* this is part of the data */
    TN_EVENT      *pReplacedEvent	/* replaced event iff exists */
    )
    {
    int     	   hashIx;		/* hash tbl index for insert */
    TN_NODE 	  *pHead;		/* dummy head node at hashIx */
    TN_NODE 	  *pNewNode;		/* the node to be inserted */
    TN_NODE 	  *pCurNode;		/* for updating the table */
    TN_NODE 	  *pPrevNode;		/* same */
    BOOL     	   found;		/* true iff the node already exists */

    if (pTbl == NULL)
        return (ERROR);

    /* Get the hash index. */

    hashIx  = tnHash (pTbl, key);
    pHead   = pTbl->tbl [hashIx];

    /* Try to find the key in the table. */

    found     = FALSE;
    pCurNode  = pHead->next;
    pPrevNode = pHead;

    while (!found && pCurNode != NULL)
        {
        if (pCurNode->key == key)
            {
            found = TRUE;
            }
        else
            {
            pPrevNode = pCurNode;
            pCurNode = pCurNode->next;
            }
        }

    if (found)
        {

        /*
         * If the key was found, pCurNode is pointing to the node we need
         * to replace, but it is only necessary to replace the data and key.
         */

        pReplacedEvent = pCurNode->event;

        pCurNode->key   = key;
        pCurNode->event = pEvent;
        }
    else
        {

	/* 
	 * Otherwise, we didn't find the key in the table.  Make a new node
	 * and insert it.
	 */

        pReplacedEvent = NULL;

        pNewNode = (TN_NODE *) memPartAlloc (pTbl->memPart, sizeof (TN_NODE));

        if (pNewNode == NULL)
            return (ERROR);

        pNewNode->event = pEvent;
        pNewNode->next  = NULL;
        pNewNode->key   = key;

        /* 
	 * pCurNode is always null at this point, pPrevNode->next should be
	 * updated to point to the new node.
         */

        pPrevNode->next = pNewNode;
        }

    return (OK);
    }

/*******************************************************************************
*
* tnHashTblIterInit - initialize an iterator key for the taskname hash table
*
* This routine intializes and returns the iterKey, used when calling 
* tnHashTblIterNext, to iterate over all the nodes in the table.  The iterKey
* is initialized to start interating at the beginning of the table.
*
* RETURNS: pointer to a TN_ITER_KEY, or NULL if no memory can be gotten for 
*          it or for an invalid hash table
*
* NOMANUAL
*/

static TN_ITER_KEY *tnHashTblIterInit 
    (
    TN_HASH_TBL *pTbl		/* the hash table the key will be used on */
    )
    {
    TN_ITER_KEY *pIterKey;

    if (pTbl == NULL)
	return (NULL);

    /*
     * We should be using a memory region in the main system memory partition
     * to hold the iterator key, since, if we try to upload a post-mortem log
     * immediately following a reboot, we don't want to try to allocate new
     * areas of memory in the post-mortem region which should be left 
     * untouched until the upload is complete (SPR 21350)
     */

    pIterKey = (TN_ITER_KEY *) malloc (sizeof (TN_ITER_KEY));

    if (pIterKey == NULL)
	return (NULL);

    pIterKey->index = 0;
    pIterKey->pCur  = pTbl->tbl[0]->next;
    return (pIterKey);
    }

/*******************************************************************************
*
* tnHashTblIterDone - destroy an iterator key for the taskname hash table
*
* This routine frees the memory associated with an iterator key that was
* created with tnHashTblIterInit.
*
* RETURNS: N/A
*
* NOMANUAL
*/

static void tnHashTblIterDone
    (
    TN_ITER_KEY *pIterKey,		/* iter key to destroy */
    TN_HASH_TBL *pTbl			/* hash table the key belongs to */
    )
    {
    if (pTbl != NULL)
        free ((char *) pIterKey);
    }

/*******************************************************************************
*
* tnHashTblIterNext - iterator routine, returns next node in hash table
*
* This routine is used to access each node stored in the hash table.  A
* pointer to the node is returned as the value of the function. An iterator
* key (pIterKey) is passed into and out of this function in order to store
* where in the table to go next.  
*
* The iterator key can be obtained by calling tnHashTblIterInit.
* 
* The order of nodes returned is from the first to last element in the edge
* vector of the table (hashTbl->tbl), and then from beginning to end of 
* each linked list pointed to by the element.
*
* RETURN: pointer to next node if one exists, NULL if no more events exist
*         in the table
* NOMANUAL
*/

static TN_NODE *tnHashTblIterNext
    (
    TN_HASH_TBL *pTbl, 		/* the table to iterate over */
    TN_ITER_KEY *pIterKey	/* iterator key */
    )
    {
    TN_NODE *pRetNode;		/* node to return */

    if (pTbl == NULL)
	return (NULL);

    /* Find the next node that isn't a dummy header. */

    pRetNode = NULL;

    while (pRetNode == NULL && pIterKey->index < pTbl->size)
	{
	if (pIterKey->pCur == NULL)
	    {
	    ++pIterKey->index;
	    pIterKey->pCur = pTbl->tbl [pIterKey->index]->next;
	    }
        else
	    {
	    pRetNode      = pIterKey->pCur;
	    pIterKey->pCur = pIterKey->pCur->next;
	    }
        }
    return (pRetNode);
    }

/*******************************************************************************
*
* tnHashTblIterNextEvent - returns the next event in the taskname hash table
*
* This is exactly like tnHashTblIterNext, but this simply returns the event
* hanging off the node that tnHashTblIterNext would return.  This keep the
* data independent of the storage mechanism.
*
* RETURN: pointer to next event if one exists, NULL if no more events exist
*         in the table
* NOMANUAL
*/

static TN_EVENT *tnHashTblIterNextEvent
    (
    TN_HASH_TBL *pTbl, 		/* the table to iterate over */
    TN_ITER_KEY *pIterKey	/* iterator key */
    )
    {
    TN_NODE *pNode;

    if ((pNode = tnHashTblIterNext (pTbl, pIterKey)) == NULL)
	return (NULL);
    return (pNode->event); 
    }

/*******************************************************************************
*
* tnHashTblDestroy - frees all memory associated with a taskname hash table
*
* This routine frees all the memory associated with a taskname hash table.
*
* RETURNS: OK, or ERROR if invalid hash table
*
* INTERNAL:
* In post mortem mode, it is possible after a reboot, to download a post-mortem
* log from the user reserved memory area. In this case, the hash table will
* exist, but vxWorks knowledge of the memory partition will have been lost in
* the reboot. The old (defunct) part id which is held in the hash table
* (pTbl->memPart) is invalid as far as vxWorks is concerned and any attempt to
* memPartFree it will actually result in the area of memory being returned to
* the system's free pool. This leaves a region of memory in the user reserved
* area linked into the main system free pool. The system would then attempt to
* use this for its own purposes.
* Then, when the user hits GO for a new WindView collection, UITcl will
* create it's post mortem memory partition to take up the whole of user
* reserved memory, including the bit that the system has just stolen for itself
* which will then get overwritten, resulting in a target crash.
*
* The solution to this is to define an identifer pmPartId which is set
* by UITcl when the PM Partition is created. This is in normal memory and will 
* be zeroed on reboot. This can be used here to detect if the hash table has
* been memPartAlloc'd during this reboot and can therefore be safely
* memPartFree'd
*
* NOMANUAL
*/

static STATUS tnHashTblDestroy
    (
    TN_HASH_TBL *pTbl  
    )
    {
    int ix;			/* counting index */
    TN_ITER_KEY *pIterKey;	/* key to iterate over hash table */
    TN_NODE	*pNode;		/* current node to be freed */


    if (pmPartId != NULL)
	{
    	if (pTbl == NULL)
	    return (ERROR);

    	/*
    	 * It is only possible to free the nodes as they are returned from
    	 * tnHashTblIterNext, because pIterKey does not refer back to a node
    	 * that may be getting freed.  This is only coincidental, but it makes
    	 * this routine much simpler.
    	 */

    	if ((pIterKey = tnHashTblIterInit (pTbl)) == NULL)
	    return (ERROR);

    	while ((pNode = tnHashTblIterNext (pTbl, pIterKey)) != NULL)
            {
	    memPartFree (pTbl->memPart, (char *) pNode->event->name);
	    memPartFree (pTbl->memPart, (char *) pNode->event);
	    memPartFree (pTbl->memPart, (char *) pNode);
	    }

    	tnHashTblIterDone (pIterKey, pTbl);

    	/* Free the dummy linked-list headers at each index of the table. */

    	for (ix = 0; ix < pTbl->size; ++ix)
	    memPartFree (pTbl->memPart, (char *) pTbl->tbl [ix]);

    	/* Now free up the edge vector and then the table itself. */

        memPartFree (pTbl->memPart, (char *) pTbl->tbl);
        memPartFree (pTbl->memPart, (char *) pTbl);
	}

    return (OK);
    }
