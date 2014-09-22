/* iosLib.c - I/O system library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
04n,03oct01,dcb  Fix SPR 20033.  iosFdSet uses malloc without checking return
                 code.
04m,31aug98,ms   add _func_ioTaskStdSet initialization.
04l,24aug96,sgv  fix for spr 6802. Modified iosFdNew to check for maxFiles
		 after a fd value is chosen.
04k,23aug96,ms   writing to null device no longer an error (SPR 7076)
04k,11jan95,rhp  explain "default device" in iosDevFind() man page (SPR#2462)
04j,17oct94,rhp  removed obsolete reference to pathLib in doc (SPR#3712).
04i,26jul94,dvs  made iosWrite/iosRead consistant so they both return ERROR 
		 if no driver routine - also changed doc (SPR #2019 and 2020).
04h,14jul94,dvs  made write return 0 if no driver routine (SPR #2020).
04g,07dec93,elh  added asynchronous I/O support for Posix.
04f,21jan93,jdi  documentation cleanup for 5.1.
04e,13nov92,dnw  added include of semLibP.h
04d,23aug92,jcf  split show routines into iosShow.c. 
		 moved typedefs/defines to private/iosLibP.h.
		 changed semMInit->semBInit.  changed lst* to dll*.
04c,18jul92,smb  Changed errno.h to errnoLib.h.
04b,26may92,rrr  the tree shuffle
04a,25nov91,rrr  cleanup of some ansi warnings.
03z,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
03y,30apr91,jdi  documentation tweaks.
03x,26apr91,shl  fixed potential race in iosFdFree() (spr 997).
03w,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 moved position of iosDrvShow(); doc review by dnw.
03v,08feb92,jaa	 documentation cleanup.
03u,05oct90,dnw	 made iosNextDevGet() be NOMANUAL
03t,10aug90,kdl  added forward declarations for functions returning void.
03s,26jun90,jcf  changed iosSemaphore to mutex.
03r,29may90,dnw  fixed iosDevDelete to free memory allocated for name string.
		 tweaked documentation for iosDevAdd & iosDevDelete.
03q,25may90,dnw  fixed printf format in ios{Dev,Fd}Show to show entire name
		   instead of just 1st 20 chars.
		 changed iosFdSet to malloc space for filename, unless filename
		   is same as device name.  Note that callers should NOT
		   malloc the name before calling iosFdSet anymore.
		   See doc in iosFdSet.
		 made several obscure iosFd... routines be "no manual".
03p,11may90,yao  added missing modification history (03o) for the last checkin.
03o,09may90,yao  typecasted malloc to (char *).
03n,07may90,hjb  added iosFdDevFind routine.
03m,14mar90,jdi  documentation cleanup.
03l,14nov88,dnw  changed ioTaskStdGet to take taskId arg.
03i,08oct88,gae  fixed checking for task specific std. input/output/error fd's.
03h,15aug88,jcf  lint.
03g,15jul88,llk  changed iosFdSet.  fd names are now allocated before iosFdSet
		   is called.
03f,30jun88,llk  added iosNextDevGet().
		 changed iosDevAdd() so that it will not add devices with
		   duplicate names.
03e,04jun88,llk  replaced ioDefDev with ioDefPath.
03d,30may88,dnw  changed to v4 names.
03c,28may88,dnw  made ios{Create,Delete,Open,Close,Read,Write,Ioctl} LOCAL.
03b,23apr88,jcf  fixed semaphore calls for new semLib.
03a,30mar88,gae  made drvTable & fdTable definition local.
		 made fd's 0, 1, & 2 be standard in/out/err a la UNIX.
		 added ios{Creat,Delete,Open,Read,Write,Ioctl,Close}() for use
		 by ioLib.c.  Got rid of iosFdGetFree() and added iosFdNew()
		 to simplify setting up of new file descriptor.
		 iosFdCheck() turned into a macro for speed/jsr overhead.
		 iosDevList() shows driver number.
		 iosFdList() shows driver number and indicates std. in/out/err.
02b,20feb88,dnw  fixed missing return value in iosDrvRemove().
02a,16dec87,jlf  added iosDevDelete (), iosDrvRemove (), and iosDrvList ().
		 changed iosDrvInstall to look for an unused entry, instead
		    of just adding new drivers at the end.
01p,05nov87,rdc  fixed documentation for iosDevFind.
01o,30sep87,gae  made fdTable LOCAL; added iosFdSet() to set values in
		   fdTable; now keep file name in table; added iosFdList().
01n,28apr87,gae  made iosLock() and iosUnlock() LOCAL.
01m,24mar87,jlf  documentation.
01l,20dec86,dnw  changed iosDevMatch() to find longest (instead of just first)
		   device name that is initial substring of file name.
		 changed to not get include files from default directories.
01k,01dec86,dnw  changed iosDevAdd() to put device name in allocated memory,
		   instead of in fixed length array.
01j,14apr86,rdc  changed memAllocates to mallocs.
01i,11oct85,dnw  de-linted.
01h,20jul85,jlf  documentation.
01g,19sep84,jlf  worked on the comments a little.
01f,05sep84,jlf  fixed iosDevMatch to work properly.  Added copyright.  Added
		 comments.
01e,04sep84,dnw  added iosDevList.
01d,10aug84,dnw  changed ioDevFind to include "default device" search
		   if device name not explicitly specified.
01c,07aug84,ecs  added calls to setStatus to iosDevFind, iosDrvInstall,
		   iosFdCheck, and iosFdGetFree
01b,29jun84,ecs  changed iosDevFind to use new version of cmpbuf
01a,11jun84,dnw  culled from old drvLib and ioLib
*/

/*
This library is the driver-level interface to the I/O system.  Its
primary purpose is to route user I/O requests to the proper drivers, using
the proper parameters.  To do this, iosLib keeps tables describing the
available drivers (e.g., names, open files).

The I/O system should be initialized by calling iosInit(), before calling
any other routines in iosLib.  Each driver then installs itself by calling
iosDrvInstall().  The devices serviced by each driver are added to the I/O
system with iosDevAdd().

The I/O system is described more fully in the 
.I "I/O System"
chapter of the
.I "Programmer's Guide."

INCLUDE FILES: iosLib.h

SEE ALSO: intLib, ioLib,
.pG "I/O System"
*/

#include "vxWorks.h"
#include "dllLib.h"
#include "stdlib.h"
#include "semLib.h"
#include "ioLib.h"
#include "string.h"
#include "errnoLib.h"
#include "stdio.h"
#include "private/iosLibP.h"
#include "private/semLibP.h"
#include "private/funcBindP.h"

/* globals */

DL_LIST 	iosDvList;			/* list of I/O device headers */
DRV_ENTRY *	drvTable;			/* driver entry point table */
FD_ENTRY *	fdTable;			/* table of fd entries */
int 		maxDrivers;			/* max installed drivers */
int 		maxFiles;			/* max open files */
char		ioDefPath[MAX_FILENAME_LENGTH];	/* default I/O prefix */
int		mutexOptionsIosLib = SEM_Q_FIFO | SEM_DELETE_SAFE;
BOOL		iosLibInitialized = FALSE;

VOIDFUNCPTR	iosFdNewHookRtn = NULL;
VOIDFUNCPTR	iosFdFreeHookRtn = NULL;

/* forward static functions */

static DEV_HDR *iosDevMatch (char *name);
static void 	iosLock (void);
static void 	iosUnlock (void);

/* locals */

LOCAL SEMAPHORE iosSemaphore;	/* semaphore to interlock access to io tables */
LOCAL DEV_HDR	nullDevHdr;	/* device header for null device */

/******************************************************************************
*
* nullWrite - NULL device write routine
*/ 

static STATUS nullWrite
    (
    int    dummy,
    char * pBuf,
    int    nBytes
    )
    {
    return (nBytes);
    }

/*******************************************************************************
*
* iosInit - initialize the I/O system
*
* This routine initializes the I/O system.
* It must be called before any other I/O system routine.
*
* RETURNS: OK, or ERROR if memory is insufficient.
*/

STATUS iosInit
    (
    int max_drivers,            /* maximum number of drivers allowed */
    int max_files,              /* max number of files allowed open at once */
    char *nullDevName           /* name of the null device (bit bucket) */
    )
    {
    int i;
    int size;

    maxDrivers	= max_drivers;
    maxFiles	= max_files;

    ioDefPath [0] = EOS;

    /* allocate file table and make all entries free */

    size = maxFiles * sizeof (FD_ENTRY);
    fdTable = (FD_ENTRY *) malloc ((unsigned) size);

    if (fdTable == NULL)
	return (ERROR);

    bzero ((char *)fdTable, size);

    for (i = 0; i < maxFiles; i++)
	iosFdFree (STD_FIX(i));

    /* allocate driver table and make all entries null */

    size = maxDrivers * sizeof (DRV_ENTRY);
    drvTable = (DRV_ENTRY *) malloc ((unsigned) size);

    if (drvTable == NULL)
	return (ERROR);

    bzero ((char *) drvTable, size);
    for (i = 0; i < maxDrivers; i++)
	drvTable [i].de_inuse = FALSE;

    /* initialize the device list and data structure semaphore;
     * add the null device to the system 
     */

    semBInit (&iosSemaphore, SEM_Q_PRIORITY, SEM_FULL);

    dllInit (&iosDvList);

    drvTable [0].de_write = nullWrite;
    iosDevAdd (&nullDevHdr, nullDevName, 0);
    iosLibInitialized = TRUE;

    _func_ioTaskStdSet = (FUNCPTR)ioTaskStdSet;

    return (OK);
    }
/*******************************************************************************
*
* iosDrvInstall - install an I/O driver
*
* This routine should be called once by each I/O driver.  It hooks up the
* various I/O service calls to the driver service routines, assigns
* the driver a number, and adds the driver to the driver table.
*
* RETURNS:
* The driver number of the new driver, or ERROR if there is no room for the
* driver.
*/

int iosDrvInstall
    (
    FUNCPTR pCreate,    /* pointer to driver create function */
    FUNCPTR pDelete,    /* pointer to driver delete function */
    FUNCPTR pOpen,      /* pointer to driver open function */
    FUNCPTR pClose,     /* pointer to driver close function */
    FUNCPTR pRead,      /* pointer to driver read function */
    FUNCPTR pWrite,     /* pointer to driver write function */
    FUNCPTR pIoctl      /* pointer to driver ioctl function */
    )
    {
    FAST DRV_ENTRY *pDrvEntry = NULL;
    FAST int drvnum;

    iosLock ();

    /* Find a free driver table entry.  Never assign driver number 0. */

    for (drvnum = 1; drvnum < maxDrivers; drvnum++)
	if (! drvTable [drvnum].de_inuse)
	    {
	    /* We've got a free entry */

	    pDrvEntry = &drvTable [drvnum];
	    break;
	    }

    if (pDrvEntry == NULL)
	{
	/* we couldn't find a free driver table entry */

	errnoSet (S_iosLib_DRIVER_GLUT);
	iosUnlock ();
	return (ERROR);
	}


    pDrvEntry->de_inuse	= TRUE;
    pDrvEntry->de_create= pCreate;
    pDrvEntry->de_delete= pDelete;
    pDrvEntry->de_open	= pOpen;
    pDrvEntry->de_close	= pClose;
    pDrvEntry->de_read	= pRead;
    pDrvEntry->de_write	= pWrite;
    pDrvEntry->de_ioctl	= pIoctl;

    iosUnlock ();

    return (drvnum);
    }
/*******************************************************************************
*
* iosDrvRemove - remove an I/O driver
*
* This routine removes an I/O driver (added by iosDrvInstall()) from 
* the driver table.
* 
*
* RETURNS: OK, or ERROR if the driver has open files.
*
* SEE ALSO: iosDrvInstall()
*/

STATUS iosDrvRemove
    (
    int drvnum,       /* no. of driver to remove,             */
		      /* returned by iosDrvInstall()          */
    BOOL forceClose   /* if TRUE, force closure of open files */
    )
    {
    DEV_HDR *pDevHdr;
    FAST int fd;
    FAST FD_ENTRY *pFdEntry;
    FAST DRV_ENTRY *pDrvEntry = &drvTable [drvnum];
    FUNCPTR drvClose = pDrvEntry->de_close;

    iosLock ();

    /* See if there are any open fd's for this driver */

    for (fd = 0; fd < maxFiles; fd++)
	{
	pFdEntry = &fdTable [fd];
	if (pFdEntry->inuse && pFdEntry->pDevHdr->drvNum == drvnum)
	    {
	    if (! forceClose)
		{
		iosUnlock ();
		return (ERROR);
		}
	    else
		{
		if (drvClose != NULL)
		    (* drvClose) (pFdEntry->value);

		iosFdFree (STD_FIX(fd));
		}
	    }
	}

    /* remove any devices for this driver */

    for (pDevHdr = (DEV_HDR *) DLL_FIRST (&iosDvList); pDevHdr != NULL;
				pDevHdr = (DEV_HDR *) DLL_NEXT (&pDevHdr->node))
	{
	if (pDevHdr->drvNum == drvnum)
	    {
	    free (pDevHdr->name);
	    dllRemove (&iosDvList, &pDevHdr->node);
	    }
	}


    pDrvEntry->de_inuse	= FALSE;
    pDrvEntry->de_create= NULL;
    pDrvEntry->de_delete= NULL;
    pDrvEntry->de_open	= NULL;
    pDrvEntry->de_close	= NULL;
    pDrvEntry->de_read	= NULL;
    pDrvEntry->de_write	= NULL;
    pDrvEntry->de_ioctl	= NULL;

    iosUnlock ();

    return (OK);
    }
/*******************************************************************************
*
* iosDevAdd - add a device to the I/O system
*
* This routine adds a device to the I/O system device list, making the
* device available for subsequent open() and creat() calls.
*
* The parameter <pDevHdr> is a pointer to a device header, DEV_HDR (defined
* in iosLib.h), which is used as the node in the device list.  Usually this
* is the first item in a larger device structure for the specific device
* type.  The parameters <name> and <drvnum> are entered in <pDevHdr>.
*
* RETURNS: OK, or ERROR if there is already a device with the specified name.
*/

STATUS iosDevAdd
    (
    DEV_HDR *pDevHdr, /* pointer to device's structure */
    char *name,       /* name of device */
    int drvnum        /* no. of servicing driver, */
		      /* returned by iosDrvInstall() */
    )
    {
    DEV_HDR *pDevMatch = iosDevMatch (name);

    /* don't add a device with a name that already exists in the device list.
     * iosDevMatch will return NULL if a device name is a substring of the
     * named argument, so check that the two names really are identical.
     */

    if ((pDevMatch != NULL) && (strcmp (pDevMatch->name, name) == 0))
	{
	errnoSet (S_iosLib_DUPLICATE_DEVICE_NAME);
	return (ERROR);
	}

    pDevHdr->name   = (char *) malloc ((unsigned) (strlen (name) + 1));
    pDevHdr->drvNum = drvnum;

    if (pDevHdr->name == NULL)
	return (ERROR);

    strcpy (pDevHdr->name, name);

    iosLock ();

    dllAdd (&iosDvList, &pDevHdr->node);

    iosUnlock ();

    return (OK);
    }
/*******************************************************************************
*
* iosDevDelete - delete a device from the I/O system
*
* This routine deletes a device from the I/O system device list, making it
* unavailable to subsequent open() or creat() calls.  No interaction with
* the driver occurs, and any file descriptors open on the device or pending
* operations are unaffected.
*
* If the device was never added to the device list, unpredictable results
* may occur.
*
* RETURNS: N/A
*/

void iosDevDelete
    (
    DEV_HDR *pDevHdr            /* pointer to device's structure */
    )
    {
    iosLock ();
    dllRemove (&iosDvList, &pDevHdr->node);
    iosUnlock ();

    free (pDevHdr->name);
    }
/*******************************************************************************
*
* iosDevFind - find an I/O device in the device list
*
* This routine searches the device list for a device whose name matches the
* first portion of <name>.  If a device is found, iosDevFind() sets the
* character pointer pointed to by <pNameTail> to point to the first
* character in <name>, following the portion which matched the device name.
* It then returns a pointer to the device.  If the routine fails, it returns
* a pointer to the default device (that is, the device where the current
* working directory is mounted) and sets <pNameTail> to point to the
* beginning of <name>.  If there is no default device, iosDevFind() 
* returns NULL.
*
* RETURNS:
* A pointer to the device header, or NULL if the device is not found.
*
*/

DEV_HDR *iosDevFind
    (
    char *name,                 /* name of the device */
    char **pNameTail            /* where to put ptr to tail of name */
    )
    {
    FAST DEV_HDR *pDevHdr = iosDevMatch (name);

    if (pDevHdr != NULL)
	*pNameTail = name + strlen (pDevHdr->name);
    else
	{
	pDevHdr = iosDevMatch (ioDefPath);
	*pNameTail = name;
	}

    if (pDevHdr == NULL)
	errnoSet (S_iosLib_DEVICE_NOT_FOUND);

    return (pDevHdr);
    }
/**********************************************************************
*
* iosDevMatch - find device whose name matches specified string
*
* RETURNS: Pointer to device header, or NULL if device not found.
*/

LOCAL DEV_HDR *iosDevMatch
    (
    char *name
    )
    {
    FAST DEV_HDR *pDevHdr;
    FAST int len;
    DEV_HDR *pBestDevHdr = NULL;
    int maxLen = 0;

    iosLock ();

    for (pDevHdr = (DEV_HDR *) DLL_FIRST (&iosDvList); pDevHdr != NULL;
				pDevHdr = (DEV_HDR *) DLL_NEXT (&pDevHdr->node))
	{
	len = strlen (pDevHdr->name);

	if (strncmp (pDevHdr->name, name, len) == 0)
	    {
	    /* this device name is initial substring of name;
	     *   if it is longer than any other such device name so far,
	     *   remember it.
	     */

	    if (len > maxLen)
		{
		pBestDevHdr = pDevHdr;
		maxLen = len;
		}
	    }
	}

    iosUnlock ();

    return (pBestDevHdr);
    }
/*******************************************************************************
*
* iosNextDevGet - get the next device in the device list
*
* This routine gets the next device in the device list.
* If the passed pointer is NULL, it starts at the top of the list.
*
* RETURNS:  Pointer to a device, or NULL if <pDev> is
* the last device in the device list.
*
* NOTE: This routine was public in 4.0.2 but is obsolete.  It is made
* no-manual in 5.0 and should be removed in the next major release.
*
* NOMANUAL
*/

DEV_HDR *iosNextDevGet
    (
    DEV_HDR *pDev
    )
    {
    if (pDev == NULL)
	return ((DEV_HDR *) DLL_FIRST (&iosDvList));
    else
	return ((DEV_HDR *) DLL_NEXT (&pDev->node));
    }
/*******************************************************************************
*
* iosFdValue - validate an open file descriptor and return the driver-specific value
*
* This routine checks to see if a file descriptor is valid and
* returns the driver-specific value.
*
* RETURNS: The driver-specific value, or ERROR if the file descriptor is 
* invalid.
*/

int iosFdValue
    (
    FAST int fd         /* file descriptor to check */
    )
    {
    int xfd = STD_MAP (fd);

    if (xfd >= 0 && xfd < maxFiles && fdTable[xfd].inuse)
	return (fdTable[xfd].value);
    else
	{
	errnoSet (S_iosLib_INVALID_FILE_DESCRIPTOR);
	return (ERROR);
	}
    }
/*******************************************************************************
*
* iosFdDevFind - verify if open file descriptor is valid and return a pointer to DEV_HDR
*
* NOMANUAL
*/

DEV_HDR *iosFdDevFind
    (
    FAST int fd
    )
    {
    int xfd = STD_MAP (fd);

    if (xfd >= 0 && xfd < maxFiles && fdTable[xfd].inuse)
	return (fdTable[xfd].pDevHdr);
    else
	{
	errnoSet (S_iosLib_INVALID_FILE_DESCRIPTOR);
	return (NULL);
	}
    }
/*******************************************************************************
*
* iosFdFree - free an file descriptor entry in the file descriptor table
*
* This routine frees a file descriptor table entry.
*
* NOMANUAL
*/

void iosFdFree
    (
    int fd              /* fd to free up */
    )
    {
    FAST FD_ENTRY *pFdEntry;
    FAST int xfd = STD_MAP(fd);

    if ((pFdEntry = FD_CHECK (xfd)) != NULL)
	{

	if (pFdEntry->name != NULL)
	    {
	    /* free name unless it is just the device name */

	    if (pFdEntry->name != pFdEntry->pDevHdr->name)
		free (pFdEntry->name);

	    pFdEntry->name = NULL;
	    }

	if (iosFdFreeHookRtn != NULL)
	    (* iosFdFreeHookRtn) (fd);

	pFdEntry->inuse = FALSE;
	}
    }
/*******************************************************************************
*
* iosFdSet - record file descriptor specifics in file descriptor table
*
* This routine records the passed information about an file descriptor 
* into the file descriptor table.
*
* The file name should NOT have been malloc'd prior to calling this routine.
* The file name is now treated as follows:
*   if no file name is specified (NULL),
*	then the name pointer in the file descriptor entry is left NULL.
*   if the file name is the same as the device name,
*	then the name pointer in the file descriptor entry is set to point 
*       to the device name
*   otherwise
*	space is malloc'd for the file name and it is copied to that space
*
* This routine and iosFdFree ensure that the proper free'ing is done in each
* of the cases.
*
* This name handling eliminates the malloc/free in cases where the file name
* is the device name (i.e. sockets, pipes, serial devices, etc).
*
* NOMANUAL
*/

STATUS iosFdSet
    (
    int fd,             /* file descriptor */
    DEV_HDR *pDevHdr,   /* associated driver header */
    char *name,         /* name of file */
    int value           /* arbitrary driver info */
    )
    {
    FAST FD_ENTRY *pFdEntry = &fdTable [STD_UNFIX(fd)];
    STATUS        returnVal = OK;
    int           error = 0;

    /* free name unless it is just the device name */

    if ((pFdEntry->name != NULL) && (pFdEntry->name != pDevHdr->name))
	free (pFdEntry->name);

    /* if no name specified, set it NULL;
     * if name is same as device name, make fd name point to device name;
     * otherwise malloc and copy file name
     */
    if (name == NULL)
	pFdEntry->name = NULL;
    else if (strcmp (name, pDevHdr->name) == 0)
	pFdEntry->name = pDevHdr->name;
    else
	{
	if ((pFdEntry->name = (char *) malloc ((unsigned) (strlen (name) + 1)))
	    == NULL)
	    {
	    error = 1;
	    goto handleError;
	    }
	strcpy (pFdEntry->name, name);
	}

    pFdEntry->pDevHdr = pDevHdr;
    pFdEntry->value   = value;


    return returnVal;
	
    
handleError:
    if (error > 0)
	{
	returnVal = ERROR;
	}

    
    return returnVal;
    }
/*******************************************************************************
*
* iosFdNew - allocate and initialize a new fd
*
* This routine gets the index of a free entry in the file descriptor table.  
* The entry is marked as reserved and will not be available again until it is
* explicitly freed with iosFdFree().
*
* RETURNS: The file descriptor or ERROR.
*
* NOMANUAL
*/

int iosFdNew
    (
    DEV_HDR *pDevHdr,   /* associated driver header */
    char *name,         /* name of file */
    int value           /* arbitrary driver info */
    )
    {
    FAST int      fd;
    FAST FD_ENTRY *pFdEntry = NULL;
    FAST int      error = 0;

    iosLock ();

    for (fd = 0; fd < maxFiles; fd++)
	{
	if (!fdTable[fd].inuse)
	    {
	    pFdEntry = &fdTable[fd];
	    pFdEntry->inuse  = TRUE; /* reserve this entry */
	    pFdEntry->name   = NULL;
	    break;
	    }
	}

    iosUnlock ();

    fd = STD_FIX(fd);

    if (fd >= maxFiles)
	{
	errnoSet (S_iosLib_TOO_MANY_OPEN_FILES);
	error = 1;
	goto handleError;
	}

    if ((iosFdSet (fd, pDevHdr, name, value)) == ERROR)
	{
	error = 1;
	goto handleError;
	}


    if (iosFdNewHookRtn != NULL)
	{
	(* iosFdNewHookRtn) (fd);
	}


    return fd;
    

handleError:
    if (error > 0)
	{
	if (pFdEntry != NULL)
	    {
	    pFdEntry->inuse = FALSE;
	    pFdEntry->name  = NULL;
	    }
	fd = (int)ERROR;
	}
    
    
    return (fd);
    }
/*******************************************************************************
*
* iosCreat - invoke driver to open file
*
* RETURNS: OK if there is no create routine, or a driver-specific value.
*
* NOMANUAL
*/

int iosCreate
    (
    DEV_HDR *pDevHdr,
    char *fileName,
    int mode
    )
    {
    FUNCPTR drvCreate = drvTable [pDevHdr->drvNum].de_create;

    if (drvCreate != NULL)
	return ((*drvCreate)(pDevHdr, fileName, mode));
    else
	return (OK);
    }
/*******************************************************************************
*
* iosDelete - invoke driver to delete file
*
* RETURNS: OK if there is no delete routine, or a driver-specific value.
*
* NOMANUAL
*/

int iosDelete
    (
    DEV_HDR *pDevHdr,
    char *fileName
    )
    {
    FUNCPTR drvDelete = drvTable [pDevHdr->drvNum].de_delete;

    if (drvDelete != NULL)
	return ((*drvDelete)(pDevHdr, fileName));
    else
	return (OK);
    }
/*******************************************************************************
*
* iosOpen - invoke driver to open file
*
* RETURNS:
* OK if the driver has no open routine, or
* whatever the driver's open routine returns.
*
* NOMANUAL
*/

int iosOpen
    (
    DEV_HDR *pDevHdr,
    char *fileName,
    int flags,
    int mode
    )
    {
    FUNCPTR drvOpen = drvTable [pDevHdr->drvNum].de_open;

    if (drvOpen != NULL)
	return ((*drvOpen)(pDevHdr, fileName, flags, mode));
    else
	return (OK);
    }
/*******************************************************************************
*
* iosClose - invoke driver to close file
*
* RETURNS:
* OK if the driver has no close routine, or
* whatever the driver's close routine returns, or
* ERROR if the file descriptor is invalid.
*
* NOMANUAL
*/

STATUS iosClose
    (
    int fd
    )
    {
    STATUS status;
    FUNCPTR drvClose;
    FD_ENTRY *pFdEntry;
    FAST int xfd = STD_MAP(fd);

    if ((pFdEntry = FD_CHECK (xfd)) == NULL)
	status = ERROR;		/* status set when bad fd */
    else
	{
	drvClose = drvTable [pFdEntry->pDevHdr->drvNum].de_close;

	if (drvClose != NULL)
	    status = (*drvClose)(pFdEntry->value);
	else
	    status = OK;

	iosFdFree (STD_FIX(xfd));
	}

    return (status);
    }
/*******************************************************************************
*
* iosRead - invoke driver read routine
*
* RETURNS:
* Whatever the driver's read routine returns (number of bytes read), or
* ERROR if the file descriptor is invalid or if the driver has no read 
* routine.  If the driver has no read routine, errno is set to ENOTSUP.
*
* NOMANUAL
*/

int iosRead
    (
    int fd,
    char *buffer,
    int maxbytes
    )
    {
    FUNCPTR drvRead;
    FAST FD_ENTRY *pFdEntry;
    FAST int xfd = STD_MAP(fd);

    if ((pFdEntry = FD_CHECK (xfd)) == NULL)
	return (ERROR);

    drvRead = drvTable [pFdEntry->pDevHdr->drvNum].de_read;

    if (drvRead == NULL)
	{
	errno = ENOTSUP;
	return (ERROR);
	}
    else
	return ((* drvRead) (pFdEntry->value, buffer, maxbytes));
    }
/*******************************************************************************
*
* iosWrite - invoke driver write routine
*
* RETURNS:
* Whatever the driver's write routine returns (number of bytes written), or
* ERROR if the file descriptor is invalid or if the driver has no write
* routine. If the driver has no write routine, errno is set to ENOTSUP.
*
* NOMANUAL
*/

int iosWrite
    (
    int fd,
    char *buffer,
    int nbytes
    )
    {
    FUNCPTR drvWrite;
    FAST FD_ENTRY *pFdEntry;
    FAST int xfd = STD_MAP(fd);

    if ((pFdEntry = FD_CHECK (xfd)) == NULL)
	return (ERROR);

    drvWrite = drvTable [pFdEntry->pDevHdr->drvNum].de_write;

    if (drvWrite == NULL)
	{
	errno = ENOTSUP;
	return (ERROR);
	}
    else
	return ((* drvWrite) (pFdEntry->value, buffer, nbytes));
    }
/*******************************************************************************
*
* iosIoctl - invoke driver ioctl routine
*
* RETURNS:
* OK if the function is FIOGETNAME (and arg set to name), or
* if driver has no ioctl routine and
* function is FIONREAD then 0 otherwise ERROR, or
* whatever driver ioctl routine returns, or
* ERROR if invalid file descriptor
*
* NOMANUAL
*/

int iosIoctl
    (
    int fd,
    int function,
    int arg
    )
    {
    FUNCPTR drvIoctl;
    FAST FD_ENTRY *pFdEntry;
    FAST int xfd = STD_MAP(fd);

    if ((pFdEntry = FD_CHECK (xfd)) == NULL)
	return (ERROR);

    if (function == FIOGETNAME)
	{
	strcpy ((char *) arg, pFdEntry->name);
	return (OK);
	}

    drvIoctl = drvTable [pFdEntry->pDevHdr->drvNum].de_ioctl;

    if (drvIoctl == NULL)
	{
	if (function == FIONREAD)
	    {
	    * (char *) arg = 0;
	    return (OK);
	    }
	else
	    {
	    errnoSet (S_ioLib_UNKNOWN_REQUEST);
	    return (ERROR);
	    }
	}
    else
	{
	return ((* drvIoctl) (pFdEntry->value, function, arg));
	}
    }

/*******************************************************************************
*
* iosLock - get exclusive use of I/O data structures
*
* This routine takes exclusive use of the I/O system's data structures,
* by taking the semaphore used for that purpose.  If the semaphore
* is already taken, the calling task will suspend until the
* semaphore is available.
*/

LOCAL void iosLock (void)
    {
    semTake (&iosSemaphore, WAIT_FOREVER);
    }
/*******************************************************************************
*
* iosUnlock - release exclusive use of I/O data structures
*
* This routine releases the semaphore which had been checked out by iosLock.
* This routine should only be called by a task which currently has the
* semaphore.
*/

LOCAL void iosUnlock (void)
    {
    semGive (&iosSemaphore);
    }

