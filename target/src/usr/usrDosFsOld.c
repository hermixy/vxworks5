/* usrDosFsOld.c - old DosFs compatibility library */

/* Copyright 1998-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,15may02,jkf  SPR#77063, adding dosFsVolFormat option to dosFsDevInit
01i,14nov01,jkf  general clean up, removed void * use.
01h,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01g,26dec99,jkf  T3 changes
01f,31jul99,jkf  T2 merge, tidiness & spelling.
01e,23nov98,???  clean warning
01d,14jul98,lrn  usrFsLib now linked in by usrLib
01c,09jul98,lrn  moved definitions to dosFsLib.h, added dosFsCacheSizeDefault
01b,30jun98,lrn  added dosFsInit() installing all sub-modules,
		 added PCMCIA comment, dosFsDateTimeInstall
01a,24jun98,mjc  written.
*/

/*
DESCRIPTION
This library provides backward compatibility with old DosFs implementation.
Many of the routines here are just stubs and the other are simply envelopes to 
new DosFs user interface.

This backwards compatibility module is intended to allow easy upgrade of
the MS-DOS file system without modification of any of the VxWorks
configuration and initialization files.
In some cases however this backwards compatibility interface is
insufficient, and will require the user to modify the VxWorks
configuration code to use the new interface. One such configuration is
the PCMCIA support for x86 targets.

This is strongly recommended do not use the routines from this library in the
new application design.  Instead, they may be used as examples which 
demonstrate use of routines to initialize and configure devices in the new 
DosFs.

LIMITATIONS

SEE ALSO: dosFsLib, dcacheLib, dpartLib
*/

/* INCLUDES */

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "errnoLib.h"
#include "string.h"
#include "stdlib.h"
#include "dosFsLib.h"
#include "dcacheCbio.h"
#include "time.h"
#include "timers.h"
#include "usrLib.h"


/* DEFINES */

#define DEFAULT_OPTIONS         0       /* default volume options */

/* locals */

LOCAL UINT	dosFsMkfsOptions = DEFAULT_OPTIONS;	
					/* options to use during mkfs */
LOCAL FUNCPTR	pDosFsDateTimeFunc = NULL ;

LOCAL BOOL	dosFsInitCalled = FALSE ;

/* globals */
int dosFsCacheSizeDefault	= 128 * 1024 ;


/* forward */

LOCAL void dosFsDateTimeSet (void );


/*******************************************************************************
*
* dosFsInit - initialize the MS-DOS file system and related libraries
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system, and configure
* all sub-modules of the new file system implementation.
*
* In order to avoid modifying the VxWorks configuration files, this
* backward compatibility function will install all available MS-DOS file
* system sub-modules. This code is provided in source so that the user may
* eliminate the options which they do not require and save memory.
*
* RETURNS: ERROR if the main dosFsLib module has failed to initialize
*
* SEE ALSO dosFsLib
*/
STATUS dosFsInit (int ignored )
    {
    IMPORT STATUS dosFsFat16Init(void);

    if( TRUE == dosFsInitCalled )
	{
	return OK;
	}

    /* First initialize the main module */
    if( dosFsLibInit( 0 ) == ERROR )
	return ERROR;

    /* Initialize sub-modules */

    /* Sub-module: VFAT Directory Handler */
    dosVDirLibInit();

    /* Sub-module: Vintage 8.2 and VxLong Directory Handler */
    dosDirOldLibInit();

    /* Sub-module: FAT12/FAT16/FAT32 FAT Handler */
    dosFsFatInit();

    /* Sub-module: Consistency check handler */
    dosChkLibInit();

    /* Sub-module: Formatter */
    dosFsFmtLibInit();

    dosFsInitCalled = TRUE ;
    return OK;
    }


/*******************************************************************************
*
* dosFsDevInit - associate a block device with dosFs file system functions
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* This routine takes a block device structure (BLK_DEV) created by
* a device driver and defines it as a DosFs volume.  As a result, when
* high-level I/O operations (e.g., open(), write()) are performed on
* the device, the calls will be routed through dosFsLib.  The <pBlkDev>
* parameter is the address of the BLK_DEV structure which describes this
* device.
*
* This routine associates the name <pDevName> with the device and installs
* it in the VxWorks I/O system's device table.  The driver number used when
* the device is added to the table is that which was assigned to the
* DosFs library during dosFsInit().
*
* The <pConfig> parameter is now minimally supported.  If <pConfig> is NULL, 
* it is ignored.  If <pConfig> is not NULL, dosFsVolFormat is called with 
* default formatting options.
*
* If the device being initialized already has a valid DosFs file system on it,  
* the configuration data will be read from the boot sector of the disk while 
* the volume will be mounted.
*
* The specific dosFs configuration data to be used for this volume may be 
* provided during the volume formatting with the new dosFsVolFormat() call.
*
* The FIODISKINIT ioctl() command can be issued now on a preformatted device 
* only to reformat this one.
*
* This routine allocates and initializes a dummy volume descriptor 
* (DOS_VOL_DESC) for the device.  It returns a pointer to DOS_VOL_DESC.
* 
*
* RETURNS: A pointer to the dummy volume descriptor DOS_VOL_DESC, or NULL if
*          there is an error.
*
* SEE ALSO: dosFsMkfs()
*/

DOS_VOL_DESC *dosFsDevInit
    (
    char *		pDevName,       /* device name */
    BLK_DEV *		pBlkDev,       /* pointer to block device struct */
    DOS_VOL_CONFIG *	pConfig        /* pointer to volume config data */
    )
    {
    DOS_VOL_DESC *	pVolDesc;	/* pointer to volume descriptor */
    CBIO_DEV_ID	cbio;

    /* Return error if no BLK_DEV */

    if (pBlkDev == NULL)
	{
	errnoSet (S_dosFsLib_INVALID_PARAMETER);
	return (NULL);
	}

    /* make sure all modules are initialized */
    if( FALSE == dosFsInitCalled )
	{
	dosFsInit( 0 );
	}

    /* Create e.g. 128 Kbytes disk cache */

    if ( (cbio = dcacheDevCreate( (CBIO_DEV_ID) pBlkDev, NULL,
		dosFsCacheSizeDefault, pDevName)) == NULL )
        return NULL;

    /* 
     * Create file system with 20 (default) simultaneously open files and 
     * default volume integrity check on mount level (repair + verbous_1).
     */

    if (dosFsDevCreate (pDevName, cbio, 0, NONE) != OK)
        return NULL;

    /* 
     * if <pConfig> is not NULL, then dosFsVolFormat the volume 
     * This will help a subsequent diskInit() to function closer
     * to how dosFs1 user may expect...
     */
    
    if (NULL != pConfig)
        {
        dosFsVolFormat (cbio, DOS_OPT_DEFAULT, NULL);
        }

    /* Allocate a dummy dosFs volume descriptor */
    pVolDesc = (DOS_VOL_DESC *) KHEAP_ALLOC((sizeof (DOS_VOL_DESC)));

    if (NULL == pVolDesc) 
        return (NULL);				/* no memory */

    bzero ((char *)pVolDesc, sizeof (DOS_VOL_DESC));


    if ( (*pVolDesc = KHEAP_ALLOC((strlen (pDevName) + 1))) == NULL )
	return NULL;				/* no memory */

    strcpy (*pVolDesc, pDevName);

    dosFsDateTimeSet();

    return (pVolDesc);
    } /* dosFsDevInit */

/*******************************************************************************
*
* dosFsDevInitOptionsSet - specify volume options for dosFsDevInit()
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* Since the volume options which might be specified in this call are 
* inapplicable now, this routine just returns OK.
*
* RETURNS: OK always.
*
* SEE ALSO: dosFsDevInit(), dosFsVolOptionsSet()
*  
*/

STATUS dosFsDevInitOptionsSet
    (
    UINT	options		/* options for future dosFsDevInit() calls */
    )
    {
    return (OK);
    } /* dosFsDevInitOptionsSet */

/*******************************************************************************
*
* dosFsMkfs - initialize a device and create a dosFs file system
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* This routine provides a quick method of creating a DosFs file system on
* a device.
*
* It uses default values for various DosFs configuration parameters.
* These defaults differ from old DosFs ones.
*
* The only long filenames volume option is enabled by this routine now.
* This option can be set using dosFsMkfsOptionsSet().  By default, no it is 
* enabled for disks initialized by dosFsMkfs().
*
* RETURNS: A pointer to a dummy DosFs volume descriptor, or NULL if there is an 
*          error.
*
* SEE ALSO: dosFsDevInit()
*/

DOS_VOL_DESC * dosFsMkfs
    (
    char *	pVolName,	/* volume name to use */
    BLK_DEV *	pBlkDev		/* pointer to block device structure */
    )
    {
    DOS_VOL_DESC *	pVolDesc;	/* pointer to dummy volume descriptor */
    int			options;

    dosFsDateTimeSet() ;

    if ( (dosFsMkfsOptions & DOS_OPT_LONGNAMES) == 0 )
        options = DOS_OPT_DEFAULT;
    else
        options = DOS_OPT_VXLONGNAMES;

    /* Init as dosFs device */

    if ( (pVolDesc = dosFsDevInit (pVolName, pBlkDev, NULL)) == NULL )
	return (NULL);

    /* Init dosFs structures on disk */

    if (dosFsVolFormat (pVolName, options, NULL) != OK)
        return NULL;

    return (pVolDesc);
    } /* dosFsMkfs */

/*******************************************************************************
*
* dosFsMkfsOptionsSet - specify volume options for dosFsMkfs()
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* This routine allows long filenames volume option to be set that will be 
* enabled by subsequent calls to dosFsMkfs().  The value of <options> 
* will be used for all volumes that are initialized by dosFsMkfs().
*
* RETURNS: OK always.
*
* SEE ALSO: dosFsMkfs(), dosFsVolOptionsSet()
*  
*/

STATUS dosFsMkfsOptionsSet
    (
    UINT	options		/* options for future dosFsMkfs() calls */
    )
    {
    dosFsMkfsOptions = options;		/* change options */

    return (OK);
    } /* dosFsMkfsOptionsSet */

/*******************************************************************************
*
* dosFsConfigInit - initialize dosFs volume configuration structure
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* Since dosFsDevInit() routine does not take care about the <pConfig> parameter
* now, this routine just return OK.
*
* RETURNS: OK always.
*
* SEE ALSO: dosFsDevInit()
*/

STATUS dosFsConfigInit
    (
    DOS_VOL_CONFIG *	pConfig,	/* pointer to volume config structure */
    char		mediaByte,	/* media descriptor byte */
    UINT8		secPerClust,	/* sectors per cluster */
    short		nResrvd,	/* number of reserved sectors */
    char		nFats,		/* number of FAT copies */
    UINT16		secPerFat,	/* number of sectors per FAT copy */
    short		maxRootEnts,	/* max number of entries in root dir */
    UINT		nHidden,	/* number of hidden sectors */
    UINT		options		/* volume options */
    )
    {
    return (OK);
    } /* dosFsConfigInit */

/*******************************************************************************
*
* dosFsConfigGet - obtain dosFs volume configuration values
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* Since dosFsDevInit() routine does not take care about the <pConfig> parameter
* now, this routine just return OK.
*
* RETURNS: OK always.
*
*/

STATUS dosFsConfigGet
    (
    DOS_VOL_DESC *	pVolDesc,	/* pointer to volume descriptor */
    DOS_VOL_CONFIG *	pConfig		/* ptr to config structure to fill */
    )
    {
    return (OK);
    } /* dosFsConfigGet */

/*******************************************************************************
*
* dosFsConfigShow - display dosFs volume configuration data
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* This routine obtains the dosFs volume configuration for the named
* device, formats the data, and displays it on the standard output.
*
* If no device name is specified, the current default device is described.
*
* RETURNS: OK or ERROR.
*
*/

STATUS dosFsConfigShow
    (
    char                *pDevName        /* name of device */
    )
    {
    return dosFsShow (pDevName, 1);
    } /* dosFsConfigShow */

/*******************************************************************************
*
* dosFsModeChange - modify the mode of a dosFs volume
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* This routine do nothing, because O_RDONLY mod is not implemented in the 
* current release.
*
* RETURNS: N/A
* 
* SEE ALSO: dosFsReadyChange()
*/

void dosFsModeChange
    (
    DOS_VOL_DESC *	pVolDesc,	/* pointer to volume descriptor */
    int			newMod		/* O_RDONLY/O_WRONLY/O_RDWR (both) */
    )
    {
    return;				/* not implemented yet */
    } /* dosFsModeChange */

/*******************************************************************************
*
* dosFsReadyChange - notify dosFs of a change in ready status
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* Since the Disk Cache module takes care about disk removal now, this routine 
* do nothing.
*
* RETURNS: N/A
*/

void dosFsReadyChange
    (
    DOS_VOL_DESC *	pVolDesc	/* pointer to volume descriptor */
    )
    {
    return;				/* unnecessary in new DosFs design */
    } /* dosFsReadyChange */

/*******************************************************************************
*
* dosFsVolOptionsGet - get current dosFs volume options 
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* The main purpose of this routine is to allow dosFsVolOptionsSet() setting of 
* the desired options without clearing of another ones.
* Since the volume options which might be specified in the dosFsVolOptionsSet() 
* call are inapplicable now, this routine just stores zero in the field pointed 
* to by <pOptions> and return OK.
*
* RETURNS: OK, always.
*
* SEE ALSO: dosFsVolOptionsSet()
*
*/

STATUS dosFsVolOptionsGet
    (
    DOS_VOL_DESC *	pVolDesc,	/* pointer to volume descriptor */
    UINT *  	 	pOptions	/* where to put current options value */
    )
    {
    *pOptions = 0;
    return (OK);			/* N/A for new DosFs design */
    } /* dosFsVolOptionsGet */

/*******************************************************************************
*
* dosFsVolOptionsSet - set dosFs volume options 
*
* This function is part of the backward compatibility library delivered
* with the new MS-DOS file system. It is intended to provide the same
* interface as the old version of the MS-DOS file system.
*
* Since the volume options which might be specified in this call are 
* inapplicable now, this routine just returns OK.
* 
* DOS_OPT_CHANGENOWARN - detection of replaced removable disk is automatically
*                        performed by the Disk Cache module.
* DOS_OPT_AUTOSYNC     - set <syncInterval> parameter of dcacheDevTune() 
*                        procedure to 0 to achieve similar behavior.
*
* RETURNS: OK, always.
*
* SEE ALSO: dosFsDevInitOptionsSet(), dosFsMkfsOptionsSet(), 
*	    dosFsVolOptionsGet()
*
*/

STATUS dosFsVolOptionsSet
    (
    DOS_VOL_DESC *	pVolDesc,	/* pointer to volume descriptor */
    UINT    	 	options		/* new options for volume */
    )
    {
    return (OK);			/* N/A for new DosFs design */
    } /* dosFsVolOptionsSet */

/*******************************************************************************
*
* dosFsDateTimeInstall - set system date and time
*
* This is a stub function for backward compatibility only.
* Since the new MS-DOS file system implementation will only use the
* system time and date to set modification and creation times on files,
* this function will simply get the current time and date, and set the
* system time accordingly.
* 
* RETURNS: N/A.
*
* SEE ALSO: dosFsLib, ansiTime
*/
void dosFsDateTimeInstall (FUNCPTR pDateTimeFunc)
    {
    pDosFsDateTimeFunc = pDateTimeFunc ;
    }

/*******************************************************************************
*
* dosFsDateTimeSet - set system time using old dosFs type function
*
*/
LOCAL void dosFsDateTimeSet (void )
    {
    DOS_DATE_TIME dosdt ;	/* old dosFs date time format */
    struct tm tm ;		/* ANSII time format */
    struct timespec tv ;	/* POSIX time */

    if( pDosFsDateTimeFunc == NULL )
	return ;

    /* call the user-supplied function */
    (*pDosFsDateTimeFunc) ( &dosdt );

    /* convert this to ANSI time */
    tm.tm_sec       = dosdt.dosdt_second ;
    tm.tm_min       = dosdt.dosdt_minute ;
    tm.tm_hour      = dosdt.dosdt_hour ;
    tm.tm_mday      = dosdt.dosdt_day ;
    tm.tm_mon       = dosdt.dosdt_month -1 ;
    tm.tm_year      = dosdt.dosdt_year -1900 ;
    tm.tm_wday = 0 ;
    tm.tm_yday = 0 ;
    tm.tm_isdst = 0 ;

    /* convert ANSI to POSIX */
    tv.tv_sec = mktime( &tm );
    tv.tv_nsec = 0;

    /* set system time */
    clock_settime( CLOCK_REALTIME, &tv );
    }
