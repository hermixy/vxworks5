/* cdromFsLib.c - ISO 9660 CD-ROM read-only file system library */

/* Copyright 1989-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
05a,03jun99,pfl  fixed directory and file month entries (SPR# 26756)
04r,15nov98,jdi  added instructions on modifying BSP to include cdromFS.
04e,14jul98,cn   added prototype for cdromFsWrite().
04d,10jul98,cn   updated documentation, changed cdromFS to cdromFsLib 
		 (SPR# 21732). Also removed every reference to Rock 
		 Ridge extensions support (SPR# 21590). Moved cdromFsInit()
		 to the beginning of the file.
04c,08apr98,kbw  made minor man page edits
04b,30apr97,jdi  doc: cleanup.
04a,10apr97,dds  SPR#7538: add CDROM file system support to vxWorks.
03f,17jul96,rst  productized for release.
03e,25jun96,vld	 file was compiled with "Tornado". All warnings were
		 fixed up.
03c,25jun96,vld  new functionality: uppercase names may be reached via
		 case insensitive path string.
03b,23jun96,leo  the bug in cdromFsFixPos() was fixed up (type of second
		 argument was changed to long).
03a,23jun96,leo  the bug in cdromFsFindFileInDir() was fixed up
		 (the bug came from ASCI order, wwhere ';' > '.').
02a,23jan96,vld	 the new interpretation of ISO_DIR_REC_EAR_LEN
		 and ISO_PT_REC_EAR_LEN: it is assumed now that 
		 the length of EAR is counted in logical blocks.
		 Data always starts from LB bound.
01e,23jan96,vld  the bug in cdromFsFixPos was fixed up
01d,12oct95,rst	 FCS release, ported to 5.2
01c,03jul95,rst	 Beta release, code revew.
01b,10apr95,rst	 alex, vlad total revision of the joined text
01a,22feb95,rst	 initial version
*/

/*
DESCRIPTION
This library defines cdromFsLib, a utility that lets you use standard POSIX 
I/O calls to read data from a CD-ROM formatted according to the ISO 9660 
standard file system.

It provides access to CD-ROM file systems using any standard
BLOCK_DEV structure (that is, a disk-type driver). 
 
The basic initialization sequence is similar to installing a DOS file system
on a SCSI device.
 
1. Initialize the cdrom file system library
(preferably in sysScsiConfig() in sysScsi.c):
.CS
    cdromFsInit ();
.CE
 
2. Locate and create a SCSI physical device:
.CS
    pPhysDev=scsiPhysDevCreate(pSysScsiCtrl,0,0,0,NONE,1,0,0);
.CE
 
3. Create a SCSI block device on the physical device:
.CS
    pBlkDev = (SCSI_BLK_DEV *) scsiBlkDevCreate (pPhysDev, 0, 0);
.CE
 
4. Create a CD-ROM file system on the block device:
.CS
    cdVolDesc = cdromFsDevCreate ("cdrom:", (BLK_DEV *) pBlkDev);
.CE
 
Call cdromFsDevCreate() once for each CD-ROM drive attached to your target.
After the successful completion of cdromFsDevCreate(), the CD-ROM 
file system will be available like any DOS file system, and 
you can access data on the named CD-ROM device using open(), close(), read(), 
ioctl(), readdir(), and stat().  A write() always returns an error.  

The cdromFsLib utility supports multiple drives, concurrent access from 
multiple tasks, and multiple open files.

FILE AND DIRECTORY NAMING
The strict ISO 9660 specification allows only uppercase file names 
consisting of 8 characters plus a 3 character suffix.  To support multiple
versions of the same file, the ISO 9660 specification also supports version
numbers.  When specifying a file name in an open() call, you can select the
file version by appending the file name with a semicolon (;) followed by a 
decimal number indicating the file version.  If you omit the version number,
cdromFsLib opens the latest version of the file.

To accommodate users familiar with MS-DOS, cdromFsLib lets you use lowercase 
name arguments to access files with names consisting entirely of uppercase 
characters.  Mixed-case file and directory names are accessible only if you
specify their exact case-correct names. 

For the time being, cdromFsLib further accommodates MS-DOS users by 
allowing "\e" (backslash) instead of "/" in pathnames.  However, the use 
of the backslash is discouraged because it may not be supported in
future versions of cdromFsLib.

Finally, cdromFsLib uses an 8-bit clean implementation of ISO 9660.  Thus, 
cdromFsLib is compatible with CD-ROMs using either Latin or Asian characters in 
the file names.

IOCTL CODES SUPPORTED
.iP `FIOGETNAME' 20
Returns the file name for a specific file descriptor.
.iP `FIOLABELGET'
Retrieves the volume label.  This code can be used to verify that a 
particular volume has been inserted into the drive.
.iP `FIOWHERE'
Determines the current file position.
.iP `FIOSEEK'
Changes the current file position.
.iP `FIONREAD'
Tells you the number of bytes between the current location and the end of 
this file.
.iP `FIOREADDIR'
Reads the next directory entry.
.iP `FIODISKCHANGE'
Announces that a disk has been replaced (in case the block driver is
not able to provide this indication).
.iP `FIOUNMOUNT'
Announces that the a disk has been removed (all currently open file 
descriptors are invalidated).
.iP `FIOFSTATGET'
Gets the file status information (directory entry data).

MODIFYING A BSP TO USE CDROMFS
The following example describes mounting cdromFS on a SCSI device.

Edit your BSP's config.h to make the following changes:
.IP 1.
Insert the following macro definition: 
.CS
    #define INCLUDE_CDROMFS
.CE
.IP 2.
Change FALSE to TRUE in the section under the following comment:
.CS
    /@ change FALSE to TRUE for SCSI interface @/
.CE
.LP
Make the following changes in sysScsi.c
(or sysLib.c if your BSP has no sysScsi.c):
.IP 1.
Add the following declaration to the top of the file: 
.CS
    #ifdef INCLUDE_CDROMFS
    #include "cdromFsLib.h"
    STATUS cdromFsInit (void);
    #endif
.CE
.IP 2.
Modify the definition of sysScsiInit() to include the following:
.CS
    #ifdef INCLUDE_CDROMFS
    cdromFsInit();
    #endif
.CE

The call to cdromFsInit() initializes cdromFS.  This call must
be made only once and must complete successfully before you can
call any other cdromFsLib routines, such as cdromFsDevCreate().
Typically, you make the cdromFSInit() call at system startup.
Because cdromFS is used with SCSI CD-ROM devices, it is natural
to call cdromFSInit() from within sysScsiInit().
.IP 3.
Modify the definition of sysScsiConfig() (if included in your BSP)
to include the following: 
.CS
/@ configure a SCSI CDROM at busId 6, LUN = 0 @/

#ifdef INCLUDE_CDROMFS

if ((pSpd60 = scsiPhysDevCreate (pSysScsiCtrl, 6, 0, 0, NONE, 0, 0, 0)) == 
    (SCSI_PHYS_DEV *) NULL)
    {
    SCSI_DEBUG_MSG ("sysScsiConfig: scsiPhysDevCreate failed for CDROM.\n",
                    0, 0, 0, 0, 0, 0);
    return (ERROR);
    }
else if ((pSbdCd = scsiBlkDevCreate (pSpd60, 0, 0) ) == NULL)
    {
    SCSI_DEBUG_MSG ("sysScsiConfig: scsiBlkDevCreate failed for CDROM.\n",
                    0, 0, 0, 0, 0, 0);
    return (ERROR);
    }

/@
 * Create an instance of a CD-ROM device in the I/O system.
 * A block device must already have been created.  Internally,
 * cdromFsDevCreate() calls iosDrvInstall(), which enters the
 * appropriate driver routines in the I/O driver table.
 @/

if ((cdVolDesc = cdromFsDevCreate ("cdrom:", (BLK_DEV *) pSbdCd )) == NULL)
    {
    return (ERROR);
    }

#endif /@ end of #ifdef INCLUDE_CDROMFS @/
.CE
.IP 4.
Before the definition of sysScsiConfig(), declare the following
global variables used in the above code fragment:
.CS
    SCSI_PHYS_DEV *pSpd60;
    BLK_DEV *pSbdCd;
    CDROM_VOL_DESC_ID cdVolDesc;
.CE
.LP
The main goal of the above code fragment is to call cdromFsDevCreate().
As input, cdromFsDevCreate() expects a pointer to a block device.
In the example above, the scsiPhysDevCreate() and scsiBlkDevCreate()
calls set up a block device interface for a SCSI CD-ROM device. 

After the successful completion of cdromFsDevCreate(), the device called 
"cdrom" is accessible using the standard open(), close(), read(), ioctl(),
readdir(), and stat() calls.


INCLUDE FILES: cdromFsLib.h

CAVEATS
The cdromFsLib utility does not support CD sets containing multiple disks.

SEE ALSO: ioLib, ISO 9660 Specification
*/

/* includes */

#include <string.h>
#include <memLib.h>
#include <errnoLib.h>
#include <sysLib.h>
#include <stdlib.h>
#include <stdio.h>
#include <logLib.h>
#include <errnoLib.h>
#include <usrLib.h>
#include <iosLib.h>
#include <dirent.h>
#include <ctype.h>

#include "cdromFsLib.h"

/* defines */
    
#ifdef  DEBUG
#undef  LOCAL
#define LOCAL

    int	        cdromFsDbg	= 0;
    u_char	dbgvBuf[2048];

#define DBG_MSG(level)\
	if((level)<=cdromFsDbg)printf

#define DBG_COP_TO_BUF(str, len)\
	{\
        bzero(dbgvBuf, sizeof(dbgvBuf));\
	bcopy((str), dbgvBuf, (len));\
        }
#else
#define DBG_MSG(level)		     if (0)printf
#define DBG_COP_TO_BUF(str, len)	(str,len)

#endif	/* DEBUG */

#include <assert.h>

#define CDROM_LIB_MAX_PT_SIZE	(64 KB)	/* maximum path table size supported */

#define SLASH		'/'
#define BACK_SLASH	'\\'
#define POINT		'.'
#define SEMICOL		';'

/* SEC_BUF struct constant */

#define BUF_EMPTY		(-1)
#define	CDROM_COM_BUF_SIZE	  3	/* sectors to read via single access */

/* 
 * All character variables in the module are defined as u_char (*).
 * Following macros are defined to prevent compilation warnings
 */

#define bzero(a,b)	bzero((char *)(a), (b))
#define bcopy(a,b,c)	bcopy((char *)(a), (char *)(b), (c))
#define bcmp(a,b,c)	bcmp((char *)(a), (char *)(b), (c))
#define strlen(a)	strlen((char *)(a))
#define strncpy(a,b,c)	strncpy((char *)(a), (char *)(b), (c))
#define strcpy(a,b)	strcpy((char *)(a), (char *)(b))
#define strncmp(a,b,c)	strncmp((char *)(a), (char *)(b), (c))
#define strcmp(a,b)	strcmp((char *)(a), (char *)(b))
#define strchr(a,b)	strchr((char *)(a), (b))
#define strtok_r(a,b,c)	((u_char *)strtok_r((char *)(a), (char *)(b),\
  		        (char **)(c)))
#define strspn(a,b)	strspn((char *)(a), (char *)(b))

/* 
 * Over development all errors are not set to <errno> directly, but
 * logged on the console with some comments
 */

#ifdef ERR_SET_SELF
#define errnoSet(a)	errnoSetOut(__LINE__, (u_char *)#a)
#endif

/* data fields in buffer may not be bounded correctly */

#define	C_BUF_TO_SHORT(dest, source, start)\
	{\
        CDROM_SHORT buf;\
	bcopy((u_char *)(source)+ (start), (u_char *)&buf, 2);\
        (dest) = buf;\
        }

#define	C_BUF_TO_LONG(dest, source, start)\
	{\
        CDROM_LONG buf;\
        bcopy((u_char *)(source)+ (start), (u_char *)&buf, 4);\
        (dest) = buf;\
        }

#define LAST_BITS(val,bitNum)	((~((u_long)(-1)<<(bitNum)))&((u_long)val))

#define A_PER_B(a,b)		(((a)+(b)-1)/(a))

/* to get some fields from path table records */

#define PT_REC_SIZE(size, pPT)  ((size)=((u_char)(*(pPT)+(*(pPT)&1))+8))

#define PT_PARENT_REC(prev, pPT)\
        C_BUF_TO_SHORT (prev, pPT, ISO_PT_REC_PARENT_DIR_N)

/* 
 * they are use the same data buffer within
 * <sectBuf> fields, but different copies of
 * <sectBuf> 
 */

#define RESTORE_FD(buf, dest, retErrCode)\
    {\
    (buf).sectBuf.startSecNum = (dest).sectBuf.startSecNum;\
    (dest)			  = (buf);\
    (dest).FCDirRecPtr = cdromFsGetLB((dest).pVDList,\
    (dest).FCDirRecLB, &((dest).sectBuf));\
    if ((dest).FCDirRecPtr == NULL)\
	return retErrCode;\
    (dest).FCDirRecPtr	+= (dest).FCDirRecOffInLB;\
    }

/* to assign secBuf as empty */

#define LET_SECT_BUF_EMPTY(pSecBuf)	((pSecBuf)->startSecNum=BUF_EMPTY)
    
/* typedefs */

typedef u_short	CDROM_SHORT;	/* 2-bytes fields */
typedef u_long	CDROM_LONG;	/* 4-bytes fields */

/* globals */

STATUS cdromFsInit (void);

/* cdrom file system number in driver table */

int	cdromFsDrvNum = ERROR;

/* locals */

/* forward declarations */

#ifdef ERR_SET_SELF
LOCAL VOID errnoSetOut(int line, const u_char * str);
#endif

LOCAL T_CDROM_FILE_ID   cdromFsFDAlloc (CDROM_VOL_DESC_ID pVolDesc);
LOCAL void              cdromFsFDFree (T_CDROM_FILE_ID pFD);
LOCAL STATUS            cdromFsSectBufAlloc (CDROM_VOL_DESC_ID pVolDesc,
					     SEC_BUF_ID pSecBuf, 
					     int numSectInBuf);
LOCAL STATUS            cdromFsSectBufAllocBySize (CDROM_VOL_DESC_ID pVolDesc,
						   SEC_BUF_ID pSecBuf, 
						   int size);
LOCAL void              cdromFsSectBufFree (SEC_BUF_ID pSecBuf);
LOCAL u_char *          cdromFsGetLB (T_CDROMFS_VD_LST_ID pVDLst, u_long LBNum,
				      SEC_BUF_ID secBuf);
LOCAL u_char *          cdromFsGetLS (CDROM_VOL_DESC_ID pVolDesc, u_long LSNum,
				      SEC_BUF_ID secBuf);
LOCAL u_char            cdromFsShiftCount (u_long source, u_long dest);
LOCAL u_char *          cdromFsPTGet (T_CDROMFS_VD_LST_ID pVdLst, 
				      SEC_BUF_ID pSecBuf);
LOCAL u_long            cdromFsNextPTRec (u_char ** ppPT, u_long offset, 
					  u_long PTSize);
LOCAL STATUS            cdromFsVDAddToList (CDROM_VOL_DESC_ID pVolDesc, 
					    u_char * pVDData,
					    u_int VDPseudoLBNum, 
					    u_char VDSizeToLSSizeShift);
LOCAL void              cdromFsVolUnmount (CDROM_VOL_DESC_ID pVolDesc);
LOCAL STATUS            cdromFsVolMount (CDROM_VOL_DESC_ID pVolDesc);
LOCAL T_CDROM_FILE_ID   cdromFsOpen (CDROM_VOL_DESC_ID	pVolDesc, 
				     u_char * path, int options);
LOCAL STATUS            cdromFsClose (T_CDROM_FILE_ID pFD);
LOCAL int               cdromFsFindDirOnLevel (T_CDROMFS_VD_LST_ID pVDList, 
					       u_char * name, u_char * pPT, 
					       u_int parDirNum, u_int pathLev,
					       u_char ** ppRecord);
LOCAL STATUS            cdromFsFindPathInDirHierar(T_CDROMFS_VD_LST_ID pVDList,
						   u_char * path, 
						   u_int numPathLevs,
						   T_CDROM_FILE_ID pFD, 
						   int options);
LOCAL T_CDROM_FILE_ID   cdromFsFindPath (CDROM_VOL_DESC_ID pVolDesc,
					 u_char * path, int options);
LOCAL STATUS            cdromFsFillFDForDir (T_CDROMFS_VD_LST_ID pVDList,
					     T_CDROM_FILE_ID pFD, 
					     u_char * pPTRec,
					     u_int dirRecNumInPT, 
					     u_int dirRecOffInPT);
LOCAL STATUS            cdromFsFindFileInDir (T_CDROMFS_VD_LST_ID pVDList,
					      T_CDROM_FILE_ID pFD, 
					      u_char * name,
					      u_char * pPTRec, 
					      u_int dirRecNumInPT,
					      u_int dirRecOffInPT);
LOCAL int               cdromFsSplitPath(u_char * path);
LOCAL STATUS            cdromFsReadyChange (CDROM_VOL_DESC_ID pVDList);
LOCAL STATUS            cdromFsIoctl (T_CDROM_FILE_ID fd, int function, 
				      int arg);
LOCAL STATUS            cdromFsRead (int desc, u_char * buffer, 
				     size_t maxBytes);
LOCAL STATUS            cdromFsWrite (void);
LOCAL STATUS            cdromFsSkipDirRec (T_CDROM_FILE_ID fd, u_char flags);
LOCAL STATUS            cdromFsCountMdu (T_CDROM_FILE_ID fd, int prevabsoff);
LOCAL STATUS            cdromFsFillStat (T_CDROM_FILE_ID fd,struct stat * arg);
LOCAL STATUS            cdromFsSkipDirRec (T_CDROM_FILE_ID fd, u_char flags);
LOCAL STATUS            cdromFsFillPos (T_CDROM_FILE_ID fd,u_char *PrevDirPtr, 
					short i, int len, int NewOffs);
LOCAL STATUS            cdromFsDirBound (T_CDROM_FILE_ID fd);   
LOCAL void              cdromFsFixPos (T_CDROM_FILE_ID fd, u_long length, 
				       u_long absLb);
LOCAL void              cdromFsFixFsect (T_CDROM_FILE_ID fd);
LOCAL void              cdromFsSkipGap (T_CDROM_FILE_ID fd , u_long * absLb,
					long absPos);

/*******************************************************************************
* 
* cdromFsInit - initialize cdromFsLib
*
* This routine initializes cdromFsLib.  It must be called exactly
* once before calling any other routine in cdromFsLib. 
*
* ERRNO: S_cdromFsLib_ALREADY_INIT
*
* RETURNS: OK or ERROR, if cdromFsLib has already been initialized.
*
* SEE ALSO: cdromFsDevCreate(), iosLib.h
*/

STATUS cdromFsInit (void)
    {
    if (cdromFsDrvNum != ERROR)
	{
	return cdromFsDrvNum;
	}

    /* install cdromFs into driver table */

    cdromFsDrvNum = iosDrvInstall (
	 (FUNCPTR) NULL,  /* pointer to driver create function */
         (FUNCPTR) NULL,  /* pointer to driver delete function */
         (FUNCPTR) cdromFsOpen,    /* pointer to driver open function   */
         (FUNCPTR) cdromFsClose,   /* pointer to driver close function  */
         (FUNCPTR) cdromFsRead,    /* pointer to driver read function   */
         (FUNCPTR) cdromFsWrite,   /* pointer to driver write function  */
         (FUNCPTR) cdromFsIoctl    /* pointer to driver ioctl function  */
        );

    if (cdromFsDrvNum == ERROR)
	{
	printf("cdromFsLib: iosDrvInstall failed\n");
	printErrno(errnoGet());
	}

    else
	{
#ifdef	DEBUG
	printf("cdromFsLib: Initialized\n");
#endif
	}

    return cdromFsDrvNum;
    } /* cdromFsInit() */

#ifdef ERR_SET_SELF

/*******************************************************************************
*
* errnoSetOut - put error message
*
* This routine is called instead of errnoSet() during module creation.
*
* RETURNS: N/A
*/

LOCAL VOID errnoSetOut(int line, const u_char * str)
    {
    printf("ERROR: line %d %s\n", line, str);
    }

#endif	/* ERR_SET_SELF */

/*******************************************************************************
*
* cdromFsFDAlloc - allocate file descriptor structure
*
* This routine allocates a file descriptor structure and initializes some 
* of its base members, such as 'magic' and 'sectBuf'.  Later, you can use 
* this file descriptor structure when opening a file.  However, be aware that 
* the file descriptor allocated here is not yet connected to the volume's file 
* descriptor list.  To free the file descriptor structure allocated here, 
* use cdromFsFDFree().
* 
* RETURNS: ptr to FD or NULL.
*/

LOCAL T_CDROM_FILE_ID cdromFsFDAlloc
    (
    CDROM_VOL_DESC_ID	pVolDesc	/* processed volume */
    )
    {
    T_CDROM_FILE_ID	pFD = malloc(sizeof(T_CDROM_FILE));
    
    if (pFD == NULL)
    	return NULL;
    
    bzero (pFD, sizeof (T_CDROM_FILE));
    
    /* allocate sector reading buffer (by default size) */

    if (cdromFsSectBufAlloc (pVolDesc, & (pFD->sectBuf), 0) == ERROR)
        {
        free (pFD);
        return (NULL);
        }
    
    pFD->inList = 0;		/* FD not included to volume FD list yet */
    pFD->magic = FD_MAG;

    return (pFD);
    }

/*******************************************************************************
* 
* cdromFsFDFree - deallocate a file descriptor structure
*
* This routine deallocates all allocated memory associated with the specified
* file descriptor structure.
*
* RETURN: N/A.
*/

LOCAL void cdromFsFDFree
    (
    T_CDROM_FILE_ID	pFD
    )
    {
    pFD->magic = 0;
    cdromFsSectBufFree (&(pFD->sectBuf));
    
    if (pFD->FRecords != NULL)
        free (pFD->FRecords);
        
    if (pFD->inList)
    	lstDelete (&(pFD->pVDList->pVolDesc->FDList), (NODE *)pFD);
    
    pFD->inList = 0;	

    free (pFD);
    }

/*******************************************************************************
* 
* cdromFsSectBufAlloc - allocate a buffer for reading volume sectors
*
* This routine is designed to allocate a buffer for reading volume data 
* by Logical Sectors.  If the <numSectInBuf> parameter is a value greater 
* than zero, the buffer size is assumed to be equal to <numSectInBuf> 
* times the sector size.  If you specify a <numSectInBuf> of 0, the buffer
* size is CDROM_COM_BUF_SIZE.
* 
* The buffer may already have been connected with given control structure. 
* If one is large enough, but not two, it is just left intact, if not, - 
* free it.
*
* After use, buffer must be deallocated by means of cdromFsSectBufFree ().
*
* RETURNS: OK or ERROR;
*/

LOCAL STATUS cdromFsSectBufAlloc
    (
    CDROM_VOL_DESC_ID	pVolDesc,
    SEC_BUF_ID	pSecBuf,	/* buffer control structure */
                                /* to which buffer is connected */
    int	numSectInBuf		/* LS in buffer */
    )
    {
    assert (pVolDesc != NULL);
    assert (pSecBuf != NULL);

    numSectInBuf = (numSectInBuf == 0)?  CDROM_COM_BUF_SIZE: numSectInBuf;

    /* 
     * may be, any buffer has already been connected with given
     * control structure. Check its size.
     */

    if (pSecBuf->magic == SEC_BUF_MAG && pSecBuf->sectData != NULL)
	{
	if (pSecBuf->numSects >= numSectInBuf &&
	    pSecBuf->numSects <= numSectInBuf + 1)
	    return OK;

	free(pSecBuf->sectData);
	pSecBuf->magic = 0;
	}

    /* newly init control structure */

    LET_SECT_BUF_EMPTY (pSecBuf);
    pSecBuf->numSects	= numSectInBuf;

    /* allocation */

    pSecBuf->sectData = malloc(numSectInBuf * pVolDesc->sectSize);

    if (pSecBuf->sectData == NULL)
	return ERROR;

    pSecBuf->magic = SEC_BUF_MAG; 

    return (OK);
    } 

/*******************************************************************************
* 
* cdromFsSectBufAllocBySize - allocate buffer for reading volume.
*
* After use, buffer must be deallocated by means of cdromFsSectBufFree ().
* This routine calls cdromFsSectBufAlloc() with sufficient
* number of sectors covers <size>.
* If <size> == 0, allocated buffer is  1 sector size.
*
* RETURNS: OK or ERROR;
*/

LOCAL STATUS cdromFsSectBufAllocBySize
    (
    CDROM_VOL_DESC_ID	pVolDesc,
    SEC_BUF_ID	pSecBuf,	/* buffer control structure */
				/* to which buffer is connected */
    int	size			/* minimum buffer size in bytes */
    )
    {
    assert (pVolDesc != NULL);
    assert (pSecBuf != NULL);
    
    return (cdromFsSectBufAlloc (pVolDesc , pSecBuf,
				 A_PER_B(pVolDesc->sectSize, size) + 1));
    }

#ifdef	DEBUG

/*******************************************************************************
*
* cdromFsSectBufAllocByLB - allocate by number of LB buffer for reading volume.
*
* After using, buffer must be deallocated by means of cdromFsSectBufFree().
* This routine calls cdromFsSectBufAllocBySize() with
* size equals to total <numLB> logical blocks size.
* If <numLB> == 0, allocated buffer is of 1 sectors size.
*
* RETURNS: OK or ERROR;
*/

LOCAL STATUS cdromFsSectBufAllocByLB
    (
    T_CDROMFS_VD_LST_ID	pVDList,
    SEC_BUF_ID	pSecBuf,	/* buffer control structure */
				/* to which buffer is connected */
    int	numLB			/* minimum LS in buffer */
    )
    {
    assert (pVDList != NULL);
    assert (pVDList->pVolDesc != NULL);
    assert (pSecBuf != NULL);
    
    return (cdromFsSectBufAllocBySize (pVDList->pVolDesc , pSecBuf,
				       numLB * pVDList->LBSize));
    }

#endif /*DEBUG*/

/*******************************************************************************
*
* cdromFsSectBufFree - deallocate volume sector buffer.
*
* RETURN: N/A
*/

LOCAL void cdromFsSectBufFree
    (
    SEC_BUF_ID	pSecBuf		/* buffer control structure */
    )
    {
    assert (pSecBuf != NULL);

    if (pSecBuf->magic == SEC_BUF_MAG && pSecBuf->sectData != NULL)
	free (pSecBuf->sectData);

    /* buffer structure is unusable now */

    pSecBuf->sectData = NULL;
    pSecBuf->magic = 0;
    } 

/*******************************************************************************
*
* cdromFsGetLS - read logical sector from volume.
*
* This routine tries to find requested LS in <pSecBuf> and, if failed,
* reads sector from device. Number of read sectors equal to buffer size.
*
* ERRNO: S_cdromFsLib_DEVICE_REMOVED, if cdrom disk has not been ejected;
* or any, may be set by block device driver read function.
*
* RETURNS: ptr on LS within buffer or NULL if error accessing device.
*/

LOCAL u_char * cdromFsGetLS
    (
    CDROM_VOL_DESC_ID	pVolDesc,
    u_long	LSNum,			/* logical sector to get */
    SEC_BUF_ID	pSecBuf			/* sector data control structure, */
					/* to put data to */
    )
    {
    assert (pVolDesc != NULL);
    assert (pSecBuf->sectData != NULL);
    assert (pSecBuf->magic == SEC_BUF_MAG);
    assert (LSNum < pVolDesc->pBlkDev->bd_nBlocks / pVolDesc->LSToPhSSizeMult);

    DBG_MSG (400)("access for sector %lu\n", LSNum);
    
    /* check for disk has not been ejected */

    if (pVolDesc->pBlkDev->bd_readyChanged)
    	{
    	cdromFsVolUnmount (pVolDesc);
    	errnoSet (S_cdromFsLib_DEVICE_REMOVED);
    	return (NULL);
    	}
    
    /* may be sector already in buffer ('if' for negative condition) */

    if (pSecBuf->startSecNum == BUF_EMPTY ||
	LSNum < pSecBuf->startSecNum ||
	LSNum >= pSecBuf->startSecNum + pSecBuf->numSects)
	{
	/* sector not in the buffer, read it from disk */

	BLK_DEV * pBlkDev = pVolDesc->pBlkDev;
	u_long	  numLSRead;
	
	assert (pBlkDev != NULL);

	/* num read sects must not exceed last volume sector */
	
	numLSRead = min (pBlkDev->bd_nBlocks / pVolDesc->LSToPhSSizeMult -
			 LSNum, pSecBuf->numSects);
	
	if (pBlkDev->bd_blkRd (pBlkDev, LSNum * pVolDesc->LSToPhSSizeMult,
			       numLSRead * pVolDesc->LSToPhSSizeMult,
			       pSecBuf->sectData) == ERROR)
	    {
	    LET_SECT_BUF_EMPTY (pSecBuf);
	    DBG_MSG(0)(" CDROM ERROR: error reading volume sect %lu - %lu\a\n",
		       LSNum, pSecBuf->numSects);
	    return (NULL);
	    }

	/* successfully read */
	
	pSecBuf->startSecNum = LSNum;
	}

    return (pSecBuf->sectData +
    	     (LSNum - pSecBuf->startSecNum) * pVolDesc->sectSize);
    } 

/*******************************************************************************
*
* cdromFsGetLB - get logical block
*
* This routine tries to find requested LB in <pSecBuf> and, if it fails,
* reads sector, containing the LB from device.
* <pSecBuf> is used as sector buffer. If <pSecBuf> is NULL, global
* volume buffer is used.
*
* RETURNS: ptr on LB within buffer or NULL if error with set errno
* to appropriate value.
*/

LOCAL u_char * cdromFsGetLB
    (
    T_CDROMFS_VD_LST_ID	pVDList,
    u_long	LBNum,			/* logical block to get */
    SEC_BUF_ID	pSecBuf			/* sector data control structure, */
    					/* to put data to */
    )
    {
    u_int	secNum;		/* LS, contane LB */
    u_char *	pData;		/* ptr to LB within read data buffer */
    CDROM_VOL_DESC_ID	pVolDesc;

    assert (pVDList != NULL);
    assert (LBNum > 15);

    pVolDesc = pVDList->pVolDesc;

    assert (pVolDesc != NULL);

    if (pSecBuf == NULL)	/* to use common buffer */
	pSecBuf = &(pVDList->pVolDesc->sectBuf);
	
    assert (pSecBuf->sectData != NULL);

    DBG_MSG(300)("access for LB %lu\n", LBNum);

    secNum = LBNum >> pVDList->LBToLSShift;		/* LS, contane LB  */
    pData = cdromFsGetLS (pVolDesc, secNum, pSecBuf);

    if (pData == NULL)
    	return NULL;
    	
    DBG_MSG(400)("offset in buf: %lu(last bits = %lu)\n",
		 pVDList->LBSize * LAST_BITS(LBNum, pVDList->LBToLSShift),
		 LAST_BITS(LBNum, pVDList->LBToLSShift));
    	
    return (pData + pVDList->LBSize *
		     LAST_BITS (LBNum, pVDList->LBToLSShift));
    }

/*******************************************************************************
*
* cdromFsPTGet - retrieve Path Table for given VD from volume.
*
* By default, if pSecBuf == NULL, volume dir hierarchy PT buffer
* is used for storing path table. It is allocated only once and
* is deallocated over volume unmounting.
*
* Only if pSecBuf != NULL required space for PT automaticly allocated in it.
*
* RETURNS: ptr to PT or NULL if any error occurred.
*/

LOCAL u_char * cdromFsPTGet
    (
    T_CDROMFS_VD_LST_ID	pVdLst,
    SEC_BUF_ID	pSecBuf		/* may be NULL. Ptr to buffer control */
    				/* structure to read PT to */
    )
    {
    assert (pVdLst != NULL);
    
    if (pSecBuf == NULL)	/* use volume dir hierarchy PT buffer */
        pSecBuf = &(pVdLst->PTBuf);
     
    /* if buffer already has been allocated, following call do nothing */

    if (cdromFsSectBufAllocBySize (pVdLst->pVolDesc, pSecBuf,
				   pVdLst->PTSize) == ERROR)
	return NULL;
	
    /* if PT already in buffer, following call do nothing */

    return (cdromFsGetLB(pVdLst, pVdLst->PTStartLB, pSecBuf));
    }

/*******************************************************************************
*
* cdromFsNextPTRec - pass to a next PT record.
*
* As result, *ppPT points to the next PT record.
*
* RETURNS: offset of record from PT start or 0 if last record encounted
*/

LOCAL u_long cdromFsNextPTRec
    (
    u_char **	ppPT,		/* address of ptr to current record */
    				/* within buffer, containing PT */
    u_long	offset,		/* offset of current record from PT start */
    u_long	PTSize		/* path table size (stored in volume */
    				/* descriptor */
    )
    {
    short	size;	/* current PT record size */

    /* skip current PT record */

    PT_REC_SIZE(size, *ppPT);
    offset += size;
    *ppPT += size;

    /* 
     * set of zero bytes may follow PT record, if that is last record in LB.
     * First non zero byte starts next record
     */

    for (; offset < PTSize; offset ++, *ppPT += 1)
	if (**ppPT != 0)
	    break;

    return (offset);
    }

/*******************************************************************************
*
* cdromFsShiftCount - count shift for transfer <source> to <dest>.
*
* This routine takes two values, that are power of two and counts
* the difference of powers, which is, for instance, the number of
* bits to shift <source> in order to get <dest>.
* Because <dest> may be less, than <source>, (-1) may not be
* used as error indication return code, so an impossible value of
* 100 is taken for this purpose.
*
* ERRNO: S_cdromFsLib_ONE_OF_VALUES_NOT_POWER_OF_2.
*
* RETURNS: number of bits to shift <source>, in order to get <dest>
* or a value of (100) if it is impossible to calculate shift count.
*/

LOCAL u_char cdromFsShiftCount
    (
    u_long	source,
    u_long	dest
    )
    {
    u_char	i;

    if (source <= dest)
	{
	for (i = 0; i < sizeof (u_long) * 8; i++)
	    if ((source << i) == dest)
		return i;
	}
    else	/* source > dest */
	{
	for (i = 1; i < sizeof (u_long) * 8; i++)
	    if ((source >> i) == dest)
		return (-i);
	}
    
    errnoSet (S_cdromFsLib_ONE_OF_VALUES_NOT_POWER_OF_2);
    return (100);
    }

/*******************************************************************************
*
* cdromFsVDAddToList - add VD to VD list.
*
* Allocate VD list structure, fill in its fields (from <pVDData> buffer)
* and add to VD list.
*
* RETURNS: OK or ERROR if any failed.
*/

LOCAL STATUS cdromFsVDAddToList
    (
    CDROM_VOL_DESC_ID	pVolDesc,
    u_char *	pVDData,		/* data, has been got from disk */
    u_int	VDPseudoLBNum,		/* LB number, contains given VD, */
    					/* if let (LB size = VD size) */
    u_char	VDSizeToLSSizeShift	/* relation between VD size */
    					/* and LS size */
    )
    {
    T_CDROMFS_VD_LST_ID	pVDList;

    assert (pVolDesc != NULL);
    assert (pVDData != NULL);

    pVDList = malloc (sizeof (T_CDROMFS_VD_LST));
    if (pVDList == NULL)
	return (ERROR);
	
    bzero((u_char *) pVDList, sizeof (T_CDROMFS_VD_LST));
    
    pVDList->pVolDesc = pVolDesc;
    
    C_BUF_TO_LONG (pVDList->volSize, pVDData, ISO_VD_VOL_SPACE_SIZE);
    
    /* since PT is stored in memory, max PT size is restricted */

    C_BUF_TO_LONG (pVDList->PTSize, pVDData, ISO_VD_PT_SIZE);

    if (pVDList->PTSize > CDROM_LIB_MAX_PT_SIZE)
        {
        errnoSet (S_cdromFsLib_SUCH_PATH_TABLE_SIZE_NOT_SUPPORTED);
        free (pVDList);
    	return (ERROR);
    	}
    
    C_BUF_TO_LONG (pVDList->PTStartLB, pVDData, ISO_VD_PT_OCCUR);
    C_BUF_TO_LONG (pVDList->rootDirSize, pVDData,
		   ISO_VD_ROOT_DIR_REC + ISO_DIR_REC_DATA_LEN);
    C_BUF_TO_LONG (pVDList->rootDirStartLB, pVDData,
		   ISO_VD_ROOT_DIR_REC + ISO_DIR_REC_EXTENT_LOCATION);
    C_BUF_TO_SHORT (pVDList->volSetSize, pVDData, ISO_VD_VOL_SET_SIZE);
    C_BUF_TO_SHORT (pVDList->volSeqNum, pVDData, ISO_VD_VOL_SEQUENCE_N);

    C_BUF_TO_SHORT (pVDList->LBSize, pVDData, ISO_VD_LB_SIZE);
    pVDList->LBToLSShift = cdromFsShiftCount (pVDList->LBSize,
					      pVolDesc->sectSize);
    
    pVDList->type	= ((T_ISO_VD_HEAD_ID)pVDData)->type;
    pVDList->fileStructVersion	= *(pVDData + ISO_VD_FILE_STRUCT_VER);

    pVDList->VDInSector	= VDPseudoLBNum >> VDSizeToLSSizeShift;
    pVDList->VDOffInSect	= LAST_BITS (VDPseudoLBNum,
					     VDSizeToLSSizeShift) *
				  ISO_VD_SIZE;

    /* 
     * read PT to buffer and init dirLevBorders...[].
     * In accordance with ISO9660 all directories records in
     * PT are sorted by hierarchy levels and are numbered from 1.
     * (only root has number 1 and lays on level 1).
     * Number of levels restricted to 8.
     * dirLevBordersOff[ n ] contains offset of first PT record on level
     * (n+2) (root excluded and array encounted from 0) from PT start.
     * dirLevLastRecNum[ n ] contains number of last PT record on level
     * (n+2).
     * Base of algorithm:
     * Storing number of last PT rec on hierarchy level <n> in
     * <prevLevLastRecNum>, will skip level <n+1>, on which all
     * directories have parent only within n; first directory
     * which parent dir number exceeds <prevLevLastRecNum>
     * starts level n+2.
     */
    {
    u_char * pPT;
    u_int	 offset,		/* absolute offset from PT start */
	         level,	        	/* hierarchy level (minus 2) */
          	 prevLevLastRecNum,	/* number of last PT rec on */
        				/* previous hierarchy level */
        	 prevRecNum ;		/* previous PT record number */
    
    pPT = cdromFsPTGet (pVDList, NULL);

    if (pPT == NULL)
	{
	free (pVDList);
	return (ERROR);
	}
    
    /* 
     * pass over root dir entry, dir hierarchy level 1;
     * root dir record is first PT record and its number is 1
     */
    
    offset = cdromFsNextPTRec (&pPT, 0, pVDList->PTSize);
    
    level = 0;	/* not put to array data for root directory */
    prevRecNum = 1;	/*  root number in PT */
    prevLevLastRecNum = 1;
    bzero ((u_char *)(pVDList->dirLevBordersOff),
	   sizeof(pVDList->dirLevBordersOff));
    bzero ((u_char *)(pVDList->dirLevLastRecNum),
	   sizeof(pVDList->dirLevLastRecNum));
    
    /* 
     * over this loop all dir hierarchy levels' bounds in PT
     * will be found.
     */
    
    pVDList->dirLevBordersOff[0] = offset;
    for(; offset < pVDList->PTSize && level < CDROM_MAX_DIR_LEV - 1;)
	{
	u_int prev;	/* parent dir number for current record */
	
	PT_PARENT_REC (prev, pPT);

#ifdef	DEBUG
	/* debugging */
	
	DBG_COP_TO_BUF(pPT + ISO_PT_REC_DI, *pPT);
	DBG_MSG(200)("%4d\t%20s\tparent # %3d, D_ID_LEN %2d  level %d\n",
		     prevRecNum + 1, dbgvBuf, (int)prev, (int)*pPT,
		     level + 2);
#endif
	
	/* if directory level overed */
    	    
	if(prev > prevLevLastRecNum)
	    {
	    /* close level */
	    
	    pVDList->dirLevLastRecNum[level] = prevRecNum;
	    
	    level++;
	    if (level > CDROM_MAX_DIR_LEV - 1)
		break;
	    
	    /* current level first record offset within PT */
	    
	    pVDList->dirLevBordersOff[level] = offset;
	    prevLevLastRecNum = prevRecNum;
	    }
	
	prevRecNum ++;
	
	offset = cdromFsNextPTRec (&pPT, offset, pVDList->PTSize);
	}
    
    /* close last level */
    
    pVDList->dirLevLastRecNum[level] = prevRecNum;
    
    /* 
     * may be loop breaking only because CDROM_MAX_DIR_LEV overloading
     * before fully PT exhausting, that is an error
     */
    
    if (offset < pVDList->PTSize)
	{
	free (pVDList);
	errnoSet (S_cdromFsLib_MAX_DIR_HIERARCHY_LEVEL_OVERFLOW);
	return (ERROR);
	}
    
    pVDList->numDirLevs = level + 1;       /* <level> starts from 0 */
    pVDList->numPTRecs = prevRecNum;
    }  /* hierarchy bounds init */
    
    /* VD have been built. Add it to VD list */
    
    pVDList->magic = VD_LIST_MAG;
    
    lstAdd  (&(pVolDesc->VDList), (NODE *)pVDList);

    return (OK);
    }

/*******************************************************************************
*
* cdromFsVolUnmount - mark in all device depends data, that volume unmounted.
* 
* All volume descriptors are deallocated.
* FDList not deallocated, but only marked as "file unaccessible.
*
* RETURN: N/A
*/

LOCAL void cdromFsVolUnmount
    (
    CDROM_VOL_DESC_ID	pVolDesc  /* cdrom volume decriptor id */
    )
    {
    T_CDROM_FILE_ID	pFDList;
    T_CDROMFS_VD_LST_ID	pVDList;

    assert (pVolDesc != NULL);
    assert (pVolDesc->mDevSem != NULL);

    if (pVolDesc->unmounted)
    	return;
    	
    if (semTake(pVolDesc->mDevSem, WAIT_FOREVER) == ERROR)
	return;
	
    /* free VD list with PT buffers */

    for (pVDList = (T_CDROMFS_VD_LST_ID)lstFirst(&(pVolDesc->VDList));
	 pVDList != NULL;
	 pVDList = (T_CDROMFS_VD_LST_ID)lstNext((NODE *)pVDList))
	{
	cdromFsSectBufFree(&(pVDList->PTBuf));
	}
	
    lstFree (&(pVolDesc->VDList));

    /* mark all opened  files as unaccessible */
    
    for (pFDList = (T_CDROM_FILE_ID)lstFirst(&(pVolDesc->FDList));
	 pFDList != NULL;
	 pFDList = (T_CDROM_FILE_ID)lstNext((NODE *)pFDList))
	{
	pFDList->volUnmount = 1;
	}
    
    /* mark in VD, that volume unmounted */

    pVolDesc->unmounted = 1;

    if (semGive(pVolDesc->mDevSem) == ERROR)
	errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
    }

/*******************************************************************************
*
* cdromFsVolMount - mount cdrom volume.
*
* This routine reads volume descriptors and creates VD list.
*
* ERRNO: S_cdromFsLib_UNNOWN_FILE_SYSTEM or any errno may be set by
* supplementary functions (such as malloc).
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS cdromFsVolMount
    (
    CDROM_VOL_DESC_ID	pVolDesc  /* cdrom volume decriptor id */
    )
    {
    T_CDROMFS_VD_LST	VDList;        /* volume descriptor list  */
    u_long      	LBRead;        /* logical blk to read     */
    u_char	        VDLast = 0;
    u_char              primVDMounted; /* primary VD mounted flag */

    assert (pVolDesc != NULL);
    assert (pVolDesc->mDevSem != NULL);

    /* private semaphore */

    if (semTake (pVolDesc->mDevSem, WAIT_FOREVER) == ERROR)
	return ERROR;

    /* before mount new volume, it have to unmount previous */

    if (! pVolDesc->unmounted)
	cdromFsVolUnmount (pVolDesc);

    pVolDesc->pBlkDev->bd_readyChanged = FALSE;

    /* 
     * before VD was read let LB size equal to VD size, since
     * each VD defines its own LB size. Because VD size and LS size are
     * powers of 2, one may be get from second by means of shift.
     */

    VDList.LBSize = ISO_VD_SIZE;
    VDList.LBToLSShift = cdromFsShiftCount (ISO_VD_SIZE, pVolDesc->sectSize);

    VDList.pVolDesc = pVolDesc;
    
    /* data in buffer remain from unmounted volume, so invalid */

    LET_SECT_BUF_EMPTY (&(VDList.pVolDesc->sectBuf));
    
    /* no one primary VD was found yet */

    primVDMounted = 0;

    /* by ISO, first volume descriptor always lays in ISO_PVD_BASE_LS */

    for (LBRead = ISO_PVD_BASE_LS << VDList.LBToLSShift; ! VDLast ; LBRead ++)
	{
	u_char *	pVDData;

	/* read VD from disk */

	pVDData = cdromFsGetLB (&VDList, LBRead, NULL);
	
	if (pVDData == NULL)
	    {
	    cdromFsVolUnmount (pVolDesc);
	    semGive (pVolDesc->mDevSem);
	    return ERROR;
	    }
	
	/* check standard ISO volume ID */

	if (strncmp(((T_ISO_VD_HEAD_ID)pVDData)->stdID, ISO_STD_ID ,
		    ISO_STD_ID_SIZE))
	    {
	    /* not ISO volume ID */	   

	    /*
	     * may be any unknown VD in set, but not first, that must be
	     * ISO primary VD only (at least in current version)
	     */

	    if (primVDMounted)	/* primary have been found already */
		continue;

	    else
		{
		cdromFsVolUnmount (pVolDesc);
	        semGive (pVolDesc->mDevSem);
		logMsg ("Warning: unknown CR-ROM format detected, ignored.\n",
			0,0,0,0,0,0);
		errnoSet (S_cdromFsLib_UNNOWN_FILE_SYSTEM);
		return ERROR;
		}
	    } /* check VD ID */

	/* 
	 * only VD set termination, primary and supplementary VD are 
	 * interesting; and not process secondary copies of primary VD
	 */

	if (((T_ISO_VD_HEAD_ID)pVDData)->type == ISO_VD_SETTERM)
	    VDLast = 1;
	else if ((((T_ISO_VD_HEAD_ID)pVDData)->type == ISO_VD_PRIMARY &&
		  ! primVDMounted) ||
		 ((T_ISO_VD_HEAD_ID)pVDData)->type == ISO_VD_SUPPLEM)
	    {
	    /* first VD on volume must be primary (look ISO 9660) */
	    
	    if (((T_ISO_VD_HEAD_ID)pVDData)->type == ISO_VD_SUPPLEM &&
		! primVDMounted)
		{
		cdromFsVolUnmount (pVolDesc);
	        semGive (pVolDesc->mDevSem);
		logMsg ("Warning: unknown CR-ROM format detected, ignored.\n",
			0,0,0,0,0,0);
		errnoSet (S_cdromFsLib_UNNOWN_FILE_SYSTEM);
		return ERROR;
		}
		
	    if (cdromFsVDAddToList (pVolDesc, pVDData, LBRead,
				    VDList.LBToLSShift) == ERROR)
	        {
	        cdromFsVolUnmount (pVolDesc);
	        semGive (pVolDesc->mDevSem);
	        return ERROR;
	        }
	        
	    /* if primary VD found in VD set */
	    
	    if (((T_ISO_VD_HEAD_ID)pVDData)->type == ISO_VD_PRIMARY)
	        primVDMounted = 1;	
	    } /* else if */
	} /* for */
    
    /* each volume contains at least one, primary volume descriptor. */
    
    if (lstFirst(&(pVolDesc->VDList)) == NULL)
	{
	semGive (pVolDesc->mDevSem);
	logMsg ("Warning: unknown CD-ROM format detected, ignored.\n",
		0,0,0,0,0,0);
	errnoSet (S_cdromFsLib_UNNOWN_FILE_SYSTEM);
	return ERROR;
	}

    /* device successfully mounted */

    pVolDesc->unmounted = 0;

    /* return semaphore */

    if (semGive (pVolDesc->mDevSem) == ERROR)
	{
	cdromFsVolUnmount (pVolDesc);
	errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
	return ERROR;
	}

    return OK;
    }

/*******************************************************************************
*
* cdromFsSplitPath - split path to sequential directories/file names.
*
* Names in <path> must be separate by '\' or '/'. Content of path
* is changed by the routine.
* The routine overwrites '/' and '\' by '\0', to break the path to
* distinct directorys/file names, counting path levels by the way.
* xx//yy and  xx/./yy cases are just truncated from the path;
* in xx/../yy case level xx/.. is "caved" from path.
* As result path looks like just d1\0d2\0---dn\0f.
*
* RETURNS: number of names within path or (-1).
*/

LOCAL int cdromFsSplitPath
    (
    u_char *	path   /* file / directory path */
    )
    {
    u_char	*	prevPathLev;
    u_char      *	curName;
    u_char      *	ptr;
    int	lev, pathLen, len;
    
    if (*path == EOS)
    	return 0;
    	
    pathLen = strlen(path) + 1; 	/* include EOS */

    /* 
     * truncate from path some special cases such as
     * xx//yy, xx/./yy, xx/../yy and replace '/', '\' to '\0'
     */

    curName = path;
    
    for (ptr = path, lev = 0 ; ptr != NULL;)
        {
        /* find '/' */

        ptr = (u_char *) strchr (curName, SLASH);
	
        if (ptr == NULL)
            ptr = (u_char *) strchr (curName, BACK_SLASH);
        
        if (ptr != NULL)
            *ptr = EOS;
	
    	len = strlen (curName) + 1;		/* include EOS */
    	pathLen = max (pathLen - len, 0);	/* length of the rest */
    	
        /* xx//yy, xx/./yy */

    	if ((*curName == POINT && *(curName + 1) == EOS) ||
	    (*curName == EOS))
    	    {
    	    memmove(curName, curName + len, pathLen);
    	    }

    	/* xx/../yy, in this case 'xx' will be deleted from path */
    	
	else if (*curName == POINT && *(curName + 1) == POINT &&
		 *(curName + 2) == EOS)
    	    {
    	    /* return to the previous level name (if it may be done) */

    	    if (lev > 0)
    	    	{
		for (prevPathLev = curName - 2;
		     *(prevPathLev - 1) != EOS && prevPathLev != path;
		     prevPathLev --);
    	    	}
    	    else
		{
		*path = EOS;
    	        return (0);
    	        }
	 
    	    /* go one level down */
	    
    	    memmove (prevPathLev, curName + len, pathLen);
    	    curName = prevPathLev;
    	    lev --;
    	    }

    	/* normal case */

	else
	    {
	    lev ++;
	    curName += len;
	    }
	}	/* for */
	    
    return lev;
    }

/*******************************************************************************
* 
* cdromFsFillFDForDir - fill T_CDROM_FILE for directory (only from PT record).
*
* This routine fill in fields of T_CDROM_FILE structure (ptr on with
* comes in <pFD>) for directory, that is described by <pPTRec> PT record
*
* RETURNS: OK or ERROR if malloc() or disk access error occur.
*/

LOCAL STATUS cdromFsFillFDForDir
    (
    T_CDROMFS_VD_LST_ID pVDList,        /* ptr to VD list               */
    T_CDROM_FILE_ID	pFD,		/* FD to fill                   */
    u_char *	        pPTRec,		/* directory PT record          */
    u_int	        dirRecNumInPT,	/* dir number in PT             */
    u_int	        dirRecOffInPT	/* dir rec offset from PT start */
    )
    {
    u_short	EARSize;
    
    assert (pVDList != NULL);
    assert (pVDList->magic == VD_LIST_MAG);
    assert (pVDList->pVolDesc != NULL);
    assert (pVDList->pVolDesc->magic == VD_SET_MAG);
    assert (pFD != NULL);
    assert (pPTRec != NULL);
    assert (*pPTRec != 0);	/* DIR ID Length */
        
    /* 
     * These fields are not needed to be filled for a directory:
     * pFD->parentDirPTOffset;
     * pFD->parentDirStartLB;
     * pFD->FFirstRecLBNum;
     * pFD->FFirstDirRecOffInDir;
     * pFD->FFirstDirRecOffInLB;
     * pFD->FRecords;
     * pFD->FCSStartLB;
     * pFD->FCSFUSizeLB;
     * pFD->FCSGapSizeLB;
     * pFD->FCDOffInMDU;
     * pFD->FCMDUStartLB;
     */
    
    bzero (pFD->name, sizeof (pFD->name));
    bcopy (pPTRec + ISO_PT_REC_DI, pFD->name, *pPTRec);
    
    PT_PARENT_REC (pFD->parentDirNum, pPTRec);
    C_BUF_TO_LONG (pFD->FStartLB, pPTRec, ISO_PT_REC_EXTENT_LOCATION);
    EARSize = *(pPTRec + ISO_PT_REC_EAR_LEN);

    pFD->FStartLB += EARSize;
    pFD->FDataStartOffInLB = 0;
    
    /* 
     * Directory is stored as one extension, without records and 
     * interleaving 
     */

    pFD->FMDUType	= EAR_REC_FORM_IGNORE;
    pFD->FMDUSize	= 0;
    pFD->FCSInterleaved	= CDROM_NOT_INTERLEAVED;
    
    pFD->FType		= S_IFDIR;
    
    /* init buffer for device access */

    if (cdromFsSectBufAlloc (pVDList->pVolDesc, &(pFD->sectBuf), 0) == ERROR)
    	return ERROR;
    
    /* current condition for readDir call */

    pFD->FCDirRecNum	= 0;
    pFD->FCDirRecAbsOff	= 0;
    pFD->FCDirRecOffInLB= 0;

    pFD->FCDirRecLB	= pFD->FStartLB;
    pFD->FCSAbsOff	= 0;	          /* start position */
    pFD->FCSSequenceSetNum  = pVDList->volSeqNum;
    pFD->FDType		= (u_short)(-1);  /* not one request was served */
    pFD->FCDAbsLB	= pFD->FStartLB;
    pFD->FCDAbsPos	= 0;
    pFD->FCDOffInLB	= 0;
    pFD->FNumRecs	= 0;
    pFD->FCEOF		= 0;
    
    /* some directory specific fields */

    pFD->DNumInPT	= dirRecNumInPT;
    pFD->DRecOffInPT	= dirRecOffInPT;
    pFD->DRecLBPT	= 0;	/* 
				 * TBD: not need currently. May be counted in
    				 * principal by parent function 
				 */
    pFD->DRecOffLBPT	= 0;	

    /* read first directory record and get special dir descriptions */

    pPTRec = cdromFsGetLB (pVDList, pFD->FStartLB, &(pFD->sectBuf));

    if (pPTRec == NULL)
    	{
    	cdromFsSectBufFree (&(pFD->sectBuf));
        return ERROR;
        }

    pFD->FCDirRecPtr = pPTRec;
    
    /* first directory record must contain 0x00 as name */

    if (*(pPTRec + ISO_DIR_REC_FI) != 0)
        {
        errnoSet (S_cdromFsLib_INVALID_DIRECTORY_STRUCTURE);
    	cdromFsSectBufFree (&(pFD->sectBuf));
        return ERROR;
        }
    
    C_BUF_TO_LONG (pFD->FSize, pPTRec, ISO_DIR_REC_DATA_LEN);
    pFD->FCSSize	= pFD->FSize;
    pFD->FCSSizeLB	= A_PER_B (pVDList->LBSize, pFD->FCSSize);

    pFD->FCSFlags	= *(pPTRec + ISO_DIR_REC_FLAGS);
    
    /* fill FAbsTime field (u_long time format) */

    pFD->FDateTime	= *(T_FILE_DATE_TIME_ID)(pPTRec +
						 ISO_DIR_REC_DATA_TIME);
    pFD->pVDList	= pVDList;
    pFD->volUnmount	= 0;    
    pFD->magic		= FD_MAG;
    
    /* connect to volume's FD list */

    if (!pFD->inList)
    	lstAdd (&(pVDList->pVolDesc->FDList), (NODE *)pFD);

    pFD->inList = 1;
    
    return OK;
    }

/*******************************************************************************
*
* cdromFsFillFDForFile - fill T_CDROM_FILE for file (from file's dir record).
*
* At the beginning, pFD points to FD, which contains description of file
* parent directory, and exact file's entry as encounted over
* internal cdromFsIoctl readDir calls.
* This routine fill in the fields of T_CDROM_FILE structure, pointed by <pFD>,
* for file, exchanging its previous content.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS cdromFsFillFDForFile
    (
    T_CDROMFS_VD_LST_ID pVDList,        /* ptr to vol desc list */
    T_CDROM_FILE_ID	pFD		/* FD to fill, containing parent */
    					/* directory description currently */
    )
    {
    T_CDROM_FILE   workFD;
    u_short	   EARSize;
    u_char *	   pDirRec;
    u_long	   dirRecsTotSize;	/* all file's dir records total size */
    struct tm	   recTime;
    
    assert (pVDList != NULL);
    assert (pVDList->magic == VD_LIST_MAG);
    assert (pFD != NULL);
        
    /* count total size file's dir records */
    
    /* for return to current state, let store pFD */

    workFD = *pFD;
    
    /* skip all records */

    do
    	{
	dirRecsTotSize = *(pFD->FCDirRecPtr);
	} 
    while (cdromFsSkipDirRec (pFD, DRF_LAST_REC) == OK);
    
    /* restore initial state */

    RESTORE_FD (workFD, *pFD, ERROR);
    
    /* ptr to first file's dir record */

    pDirRec		= pFD->FCDirRecPtr;

    /* init workFD */

    bzero (&workFD, sizeof(workFD));
    workFD.sectBuf = pFD->sectBuf;

    workFD.FRecords = malloc (dirRecsTotSize);

    if (workFD.FRecords == NULL)
    	return ERROR;
    
    /* fill static file data */

    bcopy (pDirRec + ISO_DIR_REC_FI, workFD.name,
	   *(pDirRec + ISO_DIR_REC_LEN_FI));
    
    C_BUF_TO_LONG (workFD.FStartLB, pDirRec, ISO_DIR_REC_EXTENT_LOCATION);
    EARSize = *(pDirRec + ISO_DIR_REC_EAR_LEN);
    
    workFD.FStartLB		+= EARSize;
    workFD.FDataStartOffInLB	= 0;
    workFD.FFirstRecLBNum	= pFD->FCDirRecLB;
    workFD.FFirstDirRecOffInLB	= pFD->FCDirRecOffInLB;
    
    workFD.FType		= S_IFREG;
    
    /* fill FAbsTime field (u_long time format) */

    workFD.FDateTime	= *(T_FILE_DATE_TIME_ID)(pDirRec +
						 ISO_DIR_REC_DATA_TIME);
    
    recTime.tm_sec	=  pFD->FDateTime.seconds;
    recTime.tm_min	=  pFD->FDateTime.minuts;
    recTime.tm_hour	=  pFD->FDateTime.hour;
    recTime.tm_mday	=  pFD->FDateTime.day;
    recTime.tm_mon	=  pFD->FDateTime.month - 1;
    recTime.tm_year	=  pFD->FDateTime.year;
    
    workFD.FDateTimeLong	= mktime(&recTime);
    
    /* parent directory description */

    workFD.parentDirNum		= pFD->DNumInPT;
    workFD.parentDirPTOffset	= pFD->DRecOffInPT;
    workFD.parentDirStartLB	= pFD->FStartLB;
    
    /* current file section description */

    C_BUF_TO_LONG (workFD.FCSSize, pDirRec, ISO_DIR_REC_DATA_LEN);
    workFD.FCSSizeLB	= A_PER_B(pVDList->LBSize, workFD.FCSSize);
    workFD.FCSFlags	= *(pDirRec + ISO_DIR_REC_FLAGS);
    
    /*  interleaving */

    workFD.FCSFUSizeLB		= *(pDirRec + ISO_DIR_REC_FU_SIZE);
    workFD.FCSGapSizeLB		= *(pDirRec + ISO_DIR_REC_IGAP_SIZE);
    workFD.FCSInterleaved	= (workFD.FCSFUSizeLB)? CDROM_INTERLEAVED :
	                                                CDROM_NOT_INTERLEAVED;
    /* TBD: read EAR and time and permissions init */
    
    workFD.FCDirRecNum	= 0;
    workFD.FCSAbsOff	= 0;		/* start position */
    C_BUF_TO_SHORT (workFD.FCSSequenceSetNum, pDirRec, ISO_DIR_REC_VOL_SEQU_N);
    
    workFD.FCSStartLB	= workFD.FStartLB;
    
    /* current data position */

    workFD.FCDAbsLB	= workFD.FStartLB;
    workFD.FCDAbsPos	= 0;
    workFD.FCDOffInLB	= 0;	/* only EAR prefaces data itself */
    workFD.FCEOF	= 0;
    
    /* fill in file's dir records buffer and count file length */

    assert (workFD.FRecords != NULL);
    pDirRec = workFD.FRecords;
    workFD.FSize = 0;

    do
    	{
	bcopy (pFD->FCDirRecPtr, pDirRec, *(pFD->FCDirRecPtr));

	C_BUF_TO_LONG (dirRecsTotSize, pDirRec, ISO_DIR_REC_DATA_LEN);
	workFD.FSize += dirRecsTotSize;
	pDirRec += *(pFD->FCDirRecPtr);	
	}
    while (cdromFsSkipDirRec(pFD, DRF_LAST_REC) == OK);
    
    workFD.FCDirRecPtr	= workFD.FRecords;
    workFD.pVDList	= pVDList;
    workFD.volUnmount	= 0;    
    workFD.magic	= FD_MAG;
    
    /* pFD already in FD-list (so comes) */

    workFD.list		= pFD->list;
    workFD.inList	= 1;
    
    *pFD = workFD;    
    
    return OK;
    }

/*******************************************************************************
*
* cdromFsStrUp - to convert string to upper case.
*
* RETURNS: N/A.
*/

LOCAL void cdromFsStrUp
    (
    u_char *	str,  /* string */
    int	len           /* length */
    )
    {
    for (len --; len >= 0; len --)
    	str[len] = toupper(str[len]);
    }

/*******************************************************************************
*
* cdromFsFindFileInDir - to find file in given directory.
*
* This routine searches directory, described by <pFD> for name <name>.
* If <name> includes version number (xxx{;ver num}), it's considered
* as absolute file name, If no version supplied (xx[;]),
* routine searches directory for last file version.
* If file found, cdromFsFillFDForFile() is called, to refill
* <pFD> fields for file.
*
* RETURNS: OK if file found and <pFD> refilled, or ERROR.
*/

LOCAL STATUS cdromFsFindFileInDir
    (
    T_CDROMFS_VD_LST_ID	pVDList,  /* ptr to volume descriptor list */
    T_CDROM_FILE_ID	pFD,      /* cdrom file descriptor ID */
    u_char *	name,             /* file name */
    u_char *	pPTRec,           /* ptr to path table record */
    u_int	dirRecNumInPT,    /* dir record no. in path table */
    u_int	dirRecOffInPT     /* dir record offset in path table */
    )
    {
    DIR	           dir;
    BOOL absName = FALSE;
    T_CDROM_FILE   bufFD;
    char *	   pChar;
    char           found;
    
    assert (pVDList != NULL);
    assert (pFD != NULL);
    assert (name != NULL);
    
    DBG_MSG(300)("name = %s\n", name);
    
    /* what is the name, - absolute or to find last version? */

    pChar = strchr (name, SEMICOL);

    if (pChar != NULL) 
	{
	if (isdigit (*(pChar + 1)))
	    absName = TRUE;
	else
	    *(pChar) = EOS;
	}

    found = 0;

retry:
    while (cdromFsIoctl (pFD, FIOREADDIR, (int)&dir) == OK)
    	{
    	int compRes;
    	
	/* truncate current name in case last version search */

	if (!absName)
	    {
	    pChar = strchr(dir.dd_dirent.d_name, SEMICOL);
	    if (pChar != NULL)
	        *pChar = EOS;
	    }

	compRes = strcmp (name, dir.dd_dirent.d_name);
    	
	/* names in dir are sorted lexicographically */

	if (!absName && compRes < 0)
	    break;	

	/* any version of file found */
	if (compRes == 0)
	    {
	    found = 1;

	    /* if requested version */

	    if (absName)	
    	        return (cdromFsFillFDForFile (pVDList, pFD));
    	    
	    bufFD = *pFD;	/* safe found */
    	    }
    	}	/* while */
    
    if (found == 0)
    	{	
	/* case sensitive not found */

        if (cdromFsFillFDForDir (pVDList, pFD, pPTRec, dirRecNumInPT,
        			 dirRecOffInPT) == ERROR)
	    return ERROR;

	cdromFsStrUp (name, strlen(name));
    	found = (char)(-1);	/* case insensitive */
	goto retry;
	}
    
    if(found == (char)(-1))
    	return ERROR;	/* not case sensitive nor insensitive found */

    RESTORE_FD (bufFD, *pFD, ERROR);	/* return to a last found version */

    return (cdromFsFillFDForFile(pVDList, pFD));
    }
    
/*******************************************************************************
*
* cdromFsFindDirOnLevel - find directory within given PT dir hierarchy level.
*
* This routine trys to find <name> within <dirLev>
* PT dir hierarchy level, that have parent dir number <parDirNum>.
* <pPT> points to PT buffer.
* In <pPTRec> will be returned ptr to found PT record (if will).
*
* RETURNS: record number or 0 if not found.
*/

LOCAL int cdromFsFindDirOnLevel
    (
    T_CDROMFS_VD_LST_ID	pVDList, /* ptr to volume descriptor list */
    u_char *	name,	 	 /* dir name to search for (EOS terminated) */
    u_char *	pPT,		 /* PT buffer */
    u_int	parDirNum,	 /* evidently */
    u_int	pathLev,	 /* level of <name> within path */
    				 /* first path name lays on */
    				 /* zero path level, but on second dir */
    				 /* hierarchy level, so */
    u_char **	ppRecord	 /* address of current PT record ptr */
    )
    {
    int	offset;		   	   /* abs offset from PT start */
    u_char * pUpCaseName = NULL;   /* ptr to upper case name found */
    u_short	recNum;		   /* current record number */
    u_short	upCaseRecNum = 0;  /* record number of upper case name found */
    u_short	curRecParent;	   /* current record's parent record number */
    u_short	nameLen;	   /* <name> length */
    u_char	upCaseName[100] = {EOS};	
    
    /* 
     * let us path = "/d0/d1/---[/fname]"
     * first path name <d0> lays on zero path level, but on second directory
     * hierarchy level, so if Level is dir hierarchy level, on which
     * lays <name>, Level = <pathLev>+2, and therefore <pathLev> may
     * be used for encounting  dir hierarchy levels' bounds arrays.
     */

    offset = pVDList->dirLevBordersOff[ pathLev ]; 
    pPT += offset;
    recNum = (pathLev == 0)? 2: pVDList->dirLevLastRecNum[ pathLev -1 ] + 1;
    nameLen = strlen(name);
    strncpy (upCaseName, name, nameLen + 1);
    cdromFsStrUp (upCaseName, nameLen);
    
    for (; recNum <= pVDList->dirLevLastRecNum[ pathLev ]; 
	 recNum ++, offset = cdromFsNextPTRec(&pPT, offset, pVDList->PTSize))
	{
	int	compRes;	/* strncmp result */
	
	PT_PARENT_REC (curRecParent, pPT);
	if (curRecParent != parDirNum || nameLen != (u_short)*pPT)
	    continue;
	
	compRes = strncmp(name, pPT + ISO_PT_REC_DI, nameLen);
	if (compRes == 0)
	    {
	    *ppRecord = pPT;
	    return recNum;
	    }
	else if (strncmp(upCaseName, pPT + ISO_PT_REC_DI, nameLen) == 0)
	    {
	    pUpCaseName = pPT;
	    upCaseRecNum = recNum;
	    }
	else if (compRes < 0)	/* PT records are sorted by name increasing */
	    break;
	}
    
    if (pUpCaseName == NULL)	/* not found */
    	return 0;
    
    /* name found in upper case only */

    *ppRecord = pUpCaseName;
    return (upCaseRecNum);

    }
    
/*******************************************************************************
*
* cdromFsFindPathInDirHierar - find file/directory within given dir hierarchy.
*
* This routine tries to find <path> within given VD dir hierarchy.
* and fill in T_CDROM_FILE structure.
* Path levels (dir/file names) have to be splitted by EOS.
*
* RETURNS: OK or ERROR if path not found.
*/

LOCAL STATUS cdromFsFindPathInDirHierar
    (
    T_CDROMFS_VD_LST_ID	pVDList, /* ptr to volume desc list */
    u_char *	path,            /* path */
    u_int	numPathLevs, 	 /* number of names in path */
    T_CDROM_FILE_ID	pFD,	 /* FD to fill if full path found */
    int	options			 /* not used currently */
    )
    {
    int	curPathLev, parDirNum;
    u_char *	pPT;
    u_char *	pPTRec;
    STATUS	retStat = ERROR;
    
    assert (pVDList != NULL);
    assert (path != NULL);
    assert (pFD != NULL);
    
    /* 
     * numPathLevs may not exceed number of directory hierarchy levels
     * plus one for file name
     */

    if(numPathLevs > pVDList->numDirLevs + 1)
        return ERROR;
        
    pPT = cdromFsPTGet (pVDList, NULL);

    if (pPT == NULL)
	goto ret;

    for (curPathLev = 0, parDirNum = 1, pPTRec = pPT;
	 curPathLev < numPathLevs && curPathLev < pVDList->numDirLevs;
	 path += strlen(path) + 1)
	{
	parDirNum = cdromFsFindDirOnLevel (pVDList, path, pPT, parDirNum,
					   curPathLev, &pPTRec);
	curPathLev++;
	
	if (parDirNum == 0)	/* last name not found */
	    break;
	}
    
    if (curPathLev == numPathLevs ||
	(curPathLev == numPathLevs - 1 && parDirNum != 0 &&
	 curPathLev == pVDList->numDirLevs)
	)
	{
        retStat = cdromFsFillFDForDir (pVDList, pFD, pPTRec, parDirNum,
				       0 /* not use now, since PT is read
					  * wholly */
				       );
        				 
        if (retStat == ERROR || (parDirNum != 0 && curPathLev == numPathLevs))
	    /* path found */
	    goto ret;
        }
    else
    	goto ret;
    
    retStat = cdromFsFindFileInDir (pVDList, pFD, path, pPTRec, parDirNum, 0);
    
ret:
    return retStat;
    }
    
/*******************************************************************************
*
* cdromFsFindPath - find file/directory on given volume.
*
* This routine trys to find <path> within all volume
* primary/supplementary directory hierarcies and creates and fills in
* T_CDROM_FILE structure for following accsess.
* levels in path have to be splitted by '\' or '/'.
*
* ERRNO: S_cdromFsLib_MAX_DIR_LEV_OVERFLOW
* S_cdromFsLib_NO_SUCH_FILE_OR_DIRECTORY
*
* RETURNS: T_CDROM_FILE_ID or NULL if any error encounted.
*/

LOCAL T_CDROM_FILE_ID cdromFsFindPath
    (
    CDROM_VOL_DESC_ID	pVolDesc,   /* ptr to volume descriptor */
    u_char *	path,               /* path */
    int	options                     /* search options */
    )
    {
    T_CDROMFS_VD_LST_ID	pVDList;
    T_CDROM_FILE_ID	pFD;
    u_char	pPath[ CDROM_MAX_PATH_LEN + 1 ] = {EOS};
    u_int	numPathLevs;
    
    assert (pVolDesc != NULL);
    assert (path != NULL);
    
#ifndef	ERR_SET_SELF
    errnoSet(OK);
#else
    errno = OK;
#endif
    
    /* prepare path for processing (split to distinct directorys/file names) */

    strncpy (pPath, path, CDROM_MAX_PATH_LEN);
    path = pPath;
    
    numPathLevs = cdromFsSplitPath (path);
    
    if (numPathLevs == (-1))
	return (T_CDROM_FILE_ID)ERROR;
    
    if (numPathLevs > CDROM_MAX_DIR_LEV + 1)	/* one for file */
    	{
    	errnoSet (S_cdromFsLib_MAX_DIR_HIERARCHY_LEVEL_OVERFLOW);
	return (T_CDROM_FILE_ID) ERROR;
    	}
    
    /* allocate T_CDROM_FILE structure */
    pFD = cdromFsFDAlloc (pVolDesc);

    if (pFD == NULL)
	return (T_CDROM_FILE_ID)ERROR;
    
    for (pVDList = (T_CDROMFS_VD_LST_ID) lstFirst (&(pVolDesc->VDList));
    	  pVDList != NULL;
    	  pVDList = (T_CDROMFS_VD_LST_ID) lstNext ((NODE *)pVDList))
    	{
	if (cdromFsFindPathInDirHierar (pVDList, path, numPathLevs,
					pFD, options) == OK)
	    return pFD;
	}
    
    /* file/directory not found */

    cdromFsFDFree (pFD);

    if (errnoGet() == OK)
	errnoSet (S_cdromFsLib_NO_SUCH_FILE_OR_DIRECTORY);
	
    return (T_CDROM_FILE_ID)ERROR;
    }

/*******************************************************************************
*
* cdromFsFillStat - filling of stat structure 
*
* This routine transfers data from T_CDROM_FILE structure to stat structure
* defined in <sys/stat.h>
*
* RETURNS: OK or ERROR if it cannot fill stat structure
*/

LOCAL STATUS cdromFsFillStat
    (
    T_CDROM_FILE_ID fd,                 /* File descriptor */
    struct stat * arg                   /* Pointer to stat structure */
    )
    {
    u_char *ptr;                        /* pointer to LogBlock       */
    struct tm	recTime;		/* temp. structure           */
    short tmp = 0;		        /* to form permission flags  */
    short tmp2 = 0;		        /* to convert flags from EAR */
    
    arg->st_blksize = fd->pVDList->LBSize;   /* We read by LogBlocks */
    
    if (fd->FType == S_IFDIR)	/* In the case of directory */
        {
        ptr = cdromFsGetLB (fd->pVDList, fd->FStartLB, &(fd->sectBuf));
        if (ptr == NULL)
            return ERROR;

        /* pointer to a sequence of Dir. Records, not data */
        ptr += fd->FDataStartOffInLB;	
        }
    else
	{
	ptr = fd->FRecords;	/* In the case of File Section */
	}    
    arg->st_size = fd->FSize;    
    arg->st_blocks = A_PER_B (fd->pVDList->LBSize, fd->FSize);
    
    /* time fields */

    recTime.tm_sec	= fd->FDateTime.seconds;
    recTime.tm_min	= fd->FDateTime.minuts;
    recTime.tm_hour	= fd->FDateTime.hour;
    recTime.tm_mday	= fd->FDateTime.day;
    recTime.tm_mon	= fd->FDateTime.month - 1;
    recTime.tm_year	= fd->FDateTime.year;

    fd->FDateTimeLong	= mktime (&recTime);

    /*
     * The DOS-FS time fields are not supported from VxWorks 5.2
     * and above, but REQUIRED for 5.1.1 and lower to see valid
     * file dates.
     */

#ifdef	VXWORKS_511

    arg->st_fYear = fd->FDateTime.year + 1900;
    arg->st_fMonth = fd->FDateTime.month;
    arg->st_fDay = fd->FDateTime.day;
    arg->st_fHour = fd->FDateTime.hour;
    arg->st_fMinute = fd->FDateTime.minuts;
    arg->st_fSecond = fd->FDateTime.seconds;
    fd->FDateTimeLong	= 0;

#endif /* VXWORKS_511 */

    arg->st_atime = arg->st_mtime = arg->st_ctime = fd->FDateTimeLong;
    
    /* bits of permissions  processing */
    
    if (*(ptr + ISO_DIR_REC_FLAGS) & DRF_PROTECT)
        {
        if (*(ptr + ISO_DIR_REC_EAR_LEN) == 0) /* EAR must appear */
            {
            errnoSet (S_cdromFsLib_INVALID_DIR_REC_STRUCT);
            return ERROR;
            }
        /* Read ExtAttrRecord */

        ptr = cdromFsGetLB (fd->pVDList,
			    fd->FStartLB - *(ptr + ISO_DIR_REC_EAR_LEN),
			    &(fd->sectBuf));
        if (ptr == NULL)
            return ERROR;
            
        /*
	 * Now we should transfer permission bits from ExtAttrRec    
	 * Sequential reading of two bytes (processor independent)       
	 */

        tmp2 = (*(ptr + ISO_EAR_PERMIT) << 8) |
               *(ptr + ISO_EAR_PERMIT + 1);

        tmp |= (tmp2 & 0x01) ? 0 : S_IROTH;
        tmp |= (tmp2 & 0x04) ? 0 : S_IXOTH;  
        tmp |= (tmp2 & 0x10) ? 0 : S_IRUSR;
        tmp |= (tmp2 & 0x40) ? 0 : S_IXUSR;
        tmp |= (tmp2 & 0x100) ? 0 : S_IRGRP;
        tmp |= (tmp2 & 0x400) ? 0 : S_IXGRP;
        tmp |= (tmp2 & 0x1000) ? 0 : S_IROTH;
        tmp |= (tmp2 & 0x4000) ? 0 : S_IXOTH;  		
        }

    else
        {
	/* Every user has rights to read and execute */
        tmp = S_ISUID | S_ISGID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
              S_IXGRP | S_IROTH | S_IXOTH;
        }		
    arg->st_mode = fd->FType | tmp;             /* not fd->FDType */
    return OK;
    }

/*******************************************************************************
*
* cdromFsCountMdu - counts current position offset in MDU 
*
* This function counts only one value contained in a file descriptor of
* cdromFs system - FCDOffInMDU. For this purpose reading of the file itself
* is necessary (sequential, record after record). It is assumed that
* absolute position of file pointer is written in the fd, and the previous
* file pointer position is the second parametr
* NOW WE DON'T SUPPORT RECORD STRUCTURES IN FILES.
*
* RETURNS : OK or ERROR 
*/

LOCAL STATUS cdromFsCountMdu
    (
    T_CDROM_FILE_ID fd,                         /* File descriptor        */
    int prevOffs 				/* AbsPos before "seek"   */ 
    )
    {
    u_long lowBound;                            /*  low bound of a record */ 
    
    if (fd->FCDAbsPos >= prevOffs)                  /* Initial settings */
        {
        /* to the beginning of current MDU */

        lowBound = prevOffs - fd->FCDOffInMDU; 

        if (fd->FMDUType & CDROM_MDU_TYPE_FIX)
            lowBound -= 0;  /* fixed length record has only even last byte */

        else if (fd->FMDUType & CDROM_MDU_TYPE_VAR_LE || 
		 fd->FMDUType & CDROM_MDU_TYPE_VAR_BE)
            lowBound -= 0;  			 /* variable-length record */

        else
            return ERROR; 		        /* Unknown type of records */
	}
    /*  THAT'S ALL: WE DON'T SUPPORT RECORD STRUCTURES NOW ! */	
    
    return OK;       
    }

/*******************************************************************************
*
*  cdromFsFillPos - fills some fields in fd after executing "seek"
*
* That's the very simple function. The only thing it implements -
* filling of fields in T_CDROM_FILE structure after changing of 
* a file pointer position
*
* RETURNS: OK
*/

LOCAL STATUS cdromFsFillPos
    (
    T_CDROM_FILE_ID fd,  /* pointer to the file descriptor                */  
    u_char *PrevDirPtr,  /* pointer to DirRec includes point requested    */
    short i,             /* The seq.number of DirRec with requested point */ 
    int len,             /* length of all File Sections from 0 to current */
    int NewOffs          /* the offset (arg from ioctl).It's always < len */
    )
    {
    int tmp = 0;         /* temp.buffer */
    int tmp2 = 0;        /* temp.buffer */

    /*  We know the only File Section, contains the position needed       */

    fd->FCDirRecPtr = PrevDirPtr;
    fd->FCDirRecNum = i;
    		          
    /* absolute number of LB, which contains current FileDirRec */

    C_BUF_TO_LONG (fd->FCSStartLB, PrevDirPtr, ISO_DIR_REC_EXTENT_LOCATION);

    /* Section data start LB depends on ExtAttrRec inclusion in FileSect  */

    fd->FCSStartLB += *(PrevDirPtr + ISO_DIR_REC_EAR_LEN);

    C_BUF_TO_LONG (fd->FCSSize, PrevDirPtr, ISO_DIR_REC_DATA_LEN);
    fd->FCSSizeLB = A_PER_B(fd->pVDList->LBSize, fd->FCSSize);
    fd->FCSAbsOff = len - fd->FCSSize;
    tmp = (NewOffs - fd->FCSAbsOff) / fd->pVDList->LBSize; 

    if (fd->FCSInterleaved)
        {
        /* Now we should insert 'logical block gaps' if interleaved */
        
	tmp2 = A_PER_B(fd->FCSFUSizeLB, tmp);         /* Number of gaps */
        tmp += (tmp2 - 1)  * fd->FCSGapSizeLB;        /* Full gap */
        }

    fd->FCDAbsLB = fd->FCSStartLB + tmp;
    				     
    fd->FCDOffInLB = (NewOffs - fd->FCSAbsOff) %  fd->pVDList->LBSize;
    fd->FCDAbsPos = NewOffs;

    /* MDU (file records structures) are not supported */

    return OK;			     
    }

/*******************************************************************************
*
* cdromFsSkipDirRec - skipping current directory entry
*
* This routine skips all DirRecs belongs to the same directory entry
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS cdromFsSkipDirRec
    (
    T_CDROM_FILE_ID fd,                         /* File descriptor         */
    u_char flags                                /* Flags tested in DirRecs */
    )
    {
    u_char last = 0;                      /* NOT the last DirRec for entry */
    
    if (fd->FCEOF == 1)
	/* If an end of file was found out already */ 
	return ERROR;
	
    /* Last directory record of the current entry ? */
    
    if (((*(fd->FCDirRecPtr + ISO_DIR_REC_FLAGS) & flags) ^ flags) != 0)
        last = 1;      /* The last DirRec for entry */

    /* search for next DirRec */

    if (cdromFsDirBound (fd) == ERROR)         /* errno is set already */
	return ERROR;        
        
    /* File Section of directory is not finished yet, BUT... */    
    
    return (last ? ERROR : OK);     /* It depends if the DirRec was last */

    }

/*******************************************************************************
*
* cdromFsDirBound - sets new pointers to Dir Record
*
* This routine assigns a new value to pointers to the current directory
* record skipping empty places in logical sector
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS cdromFsDirBound
    (
    T_CDROM_FILE_ID fd	  /* File descriptor */
    )
    /*   
     * Hereafter the local definition will be set out. It can be used
     * only inside this function and will be excepted after the function
     * definition end.
     */

#define L_DRLEN * (fd->FCDirRecPtr + ISO_DIR_REC_REC_LEN)

    {    
    fd->FCDirRecAbsOff += L_DRLEN;                     /* To a new DirRec */
    fd->FCDirRecOffInLB += L_DRLEN;
    fd->FCDirRecNum++;
    fd->FCDirRecPtr += L_DRLEN;        /* Skip the length of prev. DirRec */

    if (fd->FCDirRecOffInLB >= fd->pVDList->LBSize)
        {
	/* go to a new logical block */

        fd->FCDirRecLB++;
        fd->FCDirRecOffInLB -= fd->pVDList->LBSize;
        fd->FCDirRecPtr = cdromFsGetLB (fd->pVDList, fd->FCDirRecLB,
					&(fd->sectBuf));
        if (fd->FCDirRecPtr == NULL)
            return ERROR;  
        fd->FCDirRecPtr += fd->FCDirRecOffInLB;
        }
         
    /* end of DirRec's in a Logical Sector !   */
    if (L_DRLEN == 0)
        {
        short offset;   /* offset in Logical Sector */
        
        offset = LAST_BITS(fd->FCDirRecLB, fd->pVDList->LBToLSShift) *
                 fd->pVDList->LBSize + fd->FCDirRecOffInLB;
                 
        /* to the end of current logical sector */
        
        fd->FCDirRecAbsOff += fd->pVDList->pVolDesc->sectSize - offset;
	fd->FCDirRecLB += 1 + fd->pVDList->LBToLSShift -
			  LAST_BITS(fd->FCDirRecLB, fd->pVDList->LBToLSShift);

        if (fd->FCDirRecAbsOff >= fd->FSize)   
            {
            fd->FCEOF = 1;
            return OK;      /* It's the first time  */
            }    

        /* the next logical sector */

        if ((fd->FCDirRecPtr = 
	     cdromFsGetLB (fd->pVDList, fd->FCDirRecLB, &(fd->sectBuf))) ==
	    NULL)
	    return ERROR;
	fd->FCDirRecOffInLB = 0;
        }

    return OK;
#undef L_DRLEN    
    } 

/*******************************************************************************
*
* cdromFsSkipGap - sets pointers to the new current data Logical Block
*
* This routine fixes data connected with new Logical Block found
*
* RETURNS: N/A
*/

LOCAL void cdromFsSkipGap
    (
    T_CDROM_FILE_ID fd, 		   /* file descriptor      */
    u_long * absLb,			   /* current block number */
    long absPos				   /* new abs position     */
    )
    {
    if (absPos - fd->FCSAbsOff >= fd->FCSSize)   /* End of FSect ? */
        {
	/* next file section */
	
	fd->FCDirRecAbsOff += *(fd->FCDirRecPtr + ISO_DIR_REC_REC_LEN);
	fd->FCDirRecNum++;
	fd->FCSAbsOff += fd->FCSSize;
    	fd->FCDirRecPtr += *(fd->FCDirRecPtr + ISO_DIR_REC_REC_LEN);
    	fd->FCDirRecNum++;
	C_BUF_TO_LONG(fd->FCSSize, fd->FCDirRecPtr,
	        	       ISO_DIR_REC_DATA_LEN);
	cdromFsFixFsect (fd);
	}
	    
    else if (fd->FCSInterleaved & CDROM_INTERLEAVED) 
	if (*absLb % (fd->FCSFUSizeLB + fd->FCSGapSizeLB) ==
	     fd->FCSFUSizeLB)			    /* End of FUnit */
	    {
	    *absLb += fd->FCSGapSizeLB;             /* Next FUnit   */
	    }    
    }
   
/*******************************************************************************
* cdromFsFixFsect - sets pointers to the new current File Section
*
* This routine fixes data connected with new File Section found
*
* RETURNS: N/A
*/

LOCAL void cdromFsFixFsect 
    (
    T_CDROM_FILE_ID fd   /* cdrom file descriptor id */
    )
    {
    UINT32 place1;                       /* Buffer to read long from DR  */
    UINT16 place2;			 /* Buffer to read short from DR */
				
    /* new pointer to the New DirRec is already set in fd->FCDirRecPtr */

    memmove (&place1, fd->FCDirRecPtr + ISO_DIR_REC_EXTENT_LOCATION, LEN32);
    fd->FCSStartLB = place1 + *(fd->FCDirRecPtr + ISO_DIR_REC_EAR_LEN);
    memmove (&place1, fd->FCDirRecPtr + ISO_DIR_REC_DATA_LEN, LEN32);

    fd->FCSSize = place1;
    fd->FCSSizeLB = A_PER_B(fd->pVDList->LBSize, fd->FCSSize);

    memmove (&place2, fd->FCDirRecPtr + ISO_DIR_REC_VOL_SEQU_N, LEN16);

    fd->FCSSequenceSetNum = place2;
    fd->FCSGapSizeLB = *(fd->FCDirRecPtr + ISO_DIR_REC_IGAP_SIZE);
    fd->FCSFUSizeLB = *(fd->FCDirRecPtr + ISO_DIR_REC_FU_SIZE);
    fd->FDType = (*(fd->FCDirRecPtr + ISO_DIR_REC_FLAGS) & DRF_DIRECTORY) ?
		 S_IFDIR : S_IFREG;    /* constants are from io.h */
    fd->FCSFlags = *(fd->FCDirRecPtr + ISO_DIR_REC_FLAGS); 	

    return; 
    }

/*******************************************************************************
*
* cdromFsFixPos - fixes current position in a file
*
* This routine sets current data pointer positions in its file descriptor
* It is called only when new and old positions are in the same FileSection
*
* RETURN: N/A
*/

LOCAL void cdromFsFixPos
    (
    T_CDROM_FILE_ID fd,                             /* file descriptor */
    u_long length,                                  /* additional offset */
    u_long absLb 				    /* new LogBlock number */
    )
    {
    fd->FCDAbsLB = absLb;
    fd->FCDOffInLB = (fd->FCDOffInLB + length) %
		     fd->pVDList->LBSize;
    fd->FCDAbsPos += length;
    return;
    }
  
/*******************************************************************************
*
* cdromFsOpen - open file/directory for following access.
*
* This routine tries to find absolute <path> within all volume
* primary/supplementary directory hierarchys and create and fill in
* T_CDROM_FILE structure for following accsess.
* levels in path have to be split by '\' or '/'.
*
* ERRNO: S_cdromFsLib_MAX_DIR_LEV_OVERFLOW
* S_cdromFsLib_NO_SUCH_FILE_OR_DIRECTORY
* S_cdromFsLib_INVALID_PATH_STRING
*
* RETURNS: T_CDROM_FILE_ID or ERROR if any error encounted.
*/

LOCAL T_CDROM_FILE_ID cdromFsOpen
    (
    CDROM_VOL_DESC_ID	pVolDesc, /* ptr to volume descriptor */
    u_char *	        path,  	  /* absolute path to directory/file to open */
    int	                options	  /* spare, not used currently */
    )
    {
    if ((pVolDesc == NULL) || pVolDesc->magic != VD_SET_MAG)
    	{
    	errnoSet (S_cdromFsLib_INVAL_VOL_DESCR);
    	return (T_CDROM_FILE_ID)ERROR;
    	}
    
    /* check for volume was mounted and has not been changed */

    if ((pVolDesc->pBlkDev->bd_statusChk != NULL) &&
	pVolDesc->pBlkDev->bd_statusChk(pVolDesc->pBlkDev) == ERROR)
    	{
    	cdromFsVolUnmount (pVolDesc);
    	errnoSet (S_cdromFsLib_VOL_UNMOUNTED);
    	return (T_CDROM_FILE_ID)ERROR;
    	}
    	
    if (pVolDesc->unmounted || pVolDesc->pBlkDev->bd_readyChanged)
	if (cdromFsVolMount (pVolDesc) == ERROR)
	    return (T_CDROM_FILE_ID)ERROR;
    
    if (path == NULL)
    	{
	errnoSet (S_cdromFsLib_INVALID_PATH_STRING);
    	return (T_CDROM_FILE_ID)ERROR;
    	}
        
    return cdromFsFindPath (pVolDesc,  path, options);
    }

/*******************************************************************************
*
* cdromFsClose - close file/directory
*
* This routine deallocates all memory, allocated over opening
* given file/directory and excludes FD from volume FD list.
*
* ERRNO: S_cdromFsLib_INVALID_FILE_DESCRIPTOR
*
* RETURNS: OK or ERROR if bad FD supplied.
*/

LOCAL STATUS cdromFsClose
    (
    T_CDROM_FILE_ID	pFD  /* ptr to cdrom file id */
    )
    {
    if ((pFD == NULL) || pFD->magic != FD_MAG)
    	{
    	errnoSet (S_cdromFsLib_INVALID_FILE_DESCRIPTOR);
    	return ERROR;
    	}
    
    cdromFsFDFree (pFD);
    
    return OK;
    }

/*******************************************************************************
*
* cdromFsWrite - cdrom pseudo write function
*
* CD-ROM is read-only device.
*
* ERRNO: S_cdromFsLib_READ_ONLY_DEVICE
*
* RETURN: ERROR.
*/

LOCAL STATUS cdromFsWrite ()
    {
    errnoSet(S_cdromFsLib_READ_ONLY_DEVICE);
    return ERROR;
    }

/*******************************************************************************
*
* cdromFsIoctl -  I/O control routine for "cdromFs" file system 
*
* This routine performs the subset of control functions defined in ioLib.h
* These functions are possible to be applied to cdromFs file system.
*  
* RETURNS: OK, ERROR or the file pointer position (if requested)
*
* ERRNO:       S_cdromFsLib_INVALID_FILE_DESCRIPTOR
*/

LOCAL STATUS cdromFsIoctl
    (
    T_CDROM_FILE_ID fd,		/* file descriptor*/
    int function,		/* an action number */
    int arg			/* special parameter for each function */
    )
    {
    if ((fd == NULL) || (fd->magic != FD_MAG))
    	{
    	errnoSet(S_cdromFsLib_INVALID_FILE_DESCRIPTOR);
    	return ERROR;		/* fd - exists at least ? */
    	}
                          
    /* check for volume has not been changed */

    if (((fd->pVDList->pVolDesc->pBlkDev->bd_statusChk != NULL) &&
	 fd->pVDList->pVolDesc->pBlkDev->bd_statusChk (
	 fd->pVDList->pVolDesc->pBlkDev) == ERROR) ||
	fd->pVDList->pVolDesc->pBlkDev->bd_readyChanged ||
	fd->pVDList->pVolDesc->unmounted)
    	{
    	cdromFsVolUnmount (fd->pVDList->pVolDesc);
    	errnoSet (S_cdromFsLib_VOL_UNMOUNTED);
    	return ERROR;
    	}
    
#ifdef NODEF

    /*
     * TBD: Vlad: You must to make this check, but, may be,
     * not for all FIO-Codes
     */

    if (fd->volUnmount != 0)                          /* No valid Volume */
	{
	errnoSet (S_cdromFsLib_VOL_UNMOUNTED);
 	return ERROR;
	}

#endif /* NODEF */

    switch (function)
        {
        case FIOGETNAME:	
            strcpy ((u_char *)arg, fd->name);
            return OK;
            
        case FIOLABELGET:
            {
            u_char * sectData;                 /* for the temp. allocation */
            
            if (semTake (fd->pVDList->pVolDesc->mDevSem, WAIT_FOREVER)
		== ERROR)
		{
		/* leave errno as set by semLib */
            	
		return ERROR;
		}
            
            /* Volume label - in the 1st (relative) sector  */

            sectData = cdromFsGetLB (fd->pVDList,
				    (fd->pVDList->pVolDesc->sectSize /
				     fd->pVDList->LBSize) * ISO_PVD_BASE_LS,
 				     NULL );
	 		     
	    if (sectData == NULL)
	    	{
		semGive (fd->pVDList->pVolDesc->mDevSem); /* No checking */
		return ERROR;
		}

	    /*  It's assumed that there's sufficient place pointed by arg */
	    
            strncpy ((u_char *)arg, sectData + ISO_VD_VOLUME_ID ,
		     ISO_VD_VOLUME_ID_SIZE);
            *((u_char *)arg + ISO_VD_VOLUME_ID_SIZE) = '\0';
            
            if (semGive (fd->pVDList->pVolDesc->mDevSem) == ERROR)
		{
		errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
		return ERROR;
                }
            return OK;
            }    
	    
        case FIOWHERE:                /* Current position */
            return fd->FCDAbsPos;     /* doesn't return OK or ERROR */
            
        case FIOSEEK:                 /* Setting of current position */
            { 
            u_char * ptr;             /* ptr to CurDirRec */
            int save;                 /* temp. buf. with position */
    	    long len = 0;             /* Sum length of all prev.FileSect */
	    UINT32 add;               /* temp. buffer */
            long i = 0;               /* Number of File Sections tested */
            
            /* 'Seek' is not defined for directories, 'pos' is not neg.  */   
            if ((arg < 0) || (fd->FType == S_IFDIR)) 
                {
                /* leave errno as set by semLib */
                return ERROR;         /* the offset is non-negative */
                }     
            if (arg >= fd->FSize)               /* Out of bounds ?? */
                {
                fd->FCEOF = 1;
                return OK;
                }
            else
                fd->FCEOF = 0;
                    
            save = fd->FCDAbsPos;
            
            /*       
	     * Firstly we should test if it isn't the same LB      
	     * and if "arg" isn't out of high bound of the File Section    
	     */
            

            if (semTake (fd->pVDList->pVolDesc->mDevSem, WAIT_FOREVER) ==
            	 ERROR)
		{
		/* leave errno as set by semLib */
		return ERROR;
		}

            if (((fd->FCDAbsPos ^ arg) <  fd->pVDList->LBSize) &&
                 (arg < (fd->FCSAbsOff + fd->FCSSize)))
                {		                 /* The same logical block */
                fd->FCDAbsPos = arg;
                fd->FCDOffInLB = arg & (fd->pVDList->LBSize - 1);
                }

            else            /* if the new offset needs dir reading */ 
                {
		ptr = fd->FRecords;
  		while (len <= arg) 	                 /* skipping entry */
		    {
		    memmove (&add, ptr + ISO_DIR_REC_DATA_LEN , LEN32); 
		    len += (u_long)add;
		    
		    if ((*(ptr + ISO_DIR_REC_FLAGS) & DRF_LAST_REC) == 0)
			break;                          /* The last DirRec */

		    i++;
		    ptr += *(ptr + ISO_DIR_REC_REC_LEN);
                    }

                if (len < arg)          /* the new offset is out of bounds */ 
                    {
		    if (semGive (fd->pVDList->pVolDesc->mDevSem) == ERROR)
			errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
                    return ERROR;
                    }
                    
                /*  
		 * len > arg, 'prev' points the necessary File Section    
		 * i - number of the File Section, the offset belongs to 
		 */      

                cdromFsFillPos (fd, ptr, i, len, arg);              	
                }
                
            cdromFsCountMdu (fd, save);  /*new offset in MDU for the arg,*/

            if (semGive (fd->pVDList->pVolDesc->mDevSem) == ERROR)
		{
                errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
                return ERROR;
                }                      /* but we don't support records  */
                
            return OK;
            }

        case FIONREAD:          /* Number of unread bytes in the file */
            {
	    *(u_long *) arg = fd->FSize - fd->FCDAbsPos;
            return OK;
            }
    
        case FIOREADDIR:	       /* Next directory entry */
            {
            short len;                 /* assumed length of the entry name */
            u_char * ptr;              /* temp.pointer to LogBlock */

            if (semTake (fd->pVDList->pVolDesc->mDevSem, WAIT_FOREVER) ==
		ERROR)
		{
		/* leave errno as set by semLib */
		return ERROR;
		}
            
	    if ((ptr = cdromFsGetLB (fd->pVDList, fd->FCDirRecLB, 	
            				&(fd->sectBuf))) == NULL)
		{
		/* errno is set already */
		semGive (fd->pVDList->pVolDesc->mDevSem);

		return ERROR;
		}
	    else
	        fd->FCDirRecPtr = ptr + fd->FCDirRecOffInLB;
	        
	    if (fd->FCEOF == 1)
	        {
	        /* Without checking */
		semGive (fd->pVDList->pVolDesc->mDevSem);
	        return ERROR;   
	        }
	        
            /* skip current directory entry */

            /* 
	     * if fd->FDType == (u_short)-1, i.e. the entry is the first 
	     * in directory, I have to return current point (truly - first 
	     * point).if not - next                                       
	    */
            
	    if (fd->FDType != (u_short)(-1))
		{
	        while (cdromFsSkipDirRec (fd, DRF_LAST_REC) == OK)
                    {
                    ; /* While not the last record is found out */
                    }
                
                if (fd->FCDirRecAbsOff >= fd->FSize)    /* End of File ? */
                    {
                    fd->FCEOF = 1;    

		    /* Without checking */
		    semGive (fd->pVDList->pVolDesc->mDevSem);
                    return ERROR;
                    }
                }    
	    
	    /* 
	     * We have skipped already the last DirRec of previous    
	     * directory entry. And we do know that we didn't come to
	     * the end of file: some DirRec's remain (a new entry)  
	     */          

	    /*
	     * Now we have to fix the new DirRec data connected,       
	     * accordingly to ISO9660, with a new directory entry       
	     */
    
	    cdromFsFixFsect (fd);
	                      
	    /* Codes of root and parent directory should be changed        */

	    if (*(fd->FCDirRecPtr + ISO_DIR_REC_FI) == '\0')
		{
		len = 1;
		((DIR *)arg)->dd_dirent.d_name[0] = '.';         /* itself */
		}

	    else if (*(fd->FCDirRecPtr + ISO_DIR_REC_FI) == '\001')
		{
		len = 2;
		((DIR *)arg)->dd_dirent.d_name[0] = '.';     /* parent dir */
		((DIR *)arg)->dd_dirent.d_name[1] = '.';
		}

	    else
	        {
	        /*  We must truncate the name to fit it in the struct DIR  */

	        len = min (*(fd->FCDirRecPtr + ISO_DIR_REC_LEN_FI),
                        sizeof(((DIR *)arg)->dd_dirent.d_name) - 1);
	        strncpy (((DIR *)arg)->dd_dirent.d_name,
                         fd->FCDirRecPtr + ISO_DIR_REC_FI, len);
	        }
	        
	    ((DIR *)arg)->dd_dirent.d_name[len] = EOS;

            if (semGive (fd->pVDList->pVolDesc->mDevSem) == ERROR)
		{
		errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
		return ERROR;
                }
            }
            return OK;
        
        case FIODISKCHANGE:                 /* setting of 'ready change' */
            
            return (cdromFsReadyChange (fd->pVDList->pVolDesc));
            
        case FIOUNMOUNT:                   /* device unmounting */
            cdromFsVolUnmount (fd->pVDList->pVolDesc);
            return OK;
            
        case FIOFSTATGET:		   /* taking of the file status */
            cdromFsFillStat (fd, (struct stat *)arg);
            return OK;
             
        default:
            if (fd->pVDList->pVolDesc->pBlkDev->bd_ioctl == NULL)
                return ERROR;     /* No 'ioctl' function in blk_dev driver */
                
            return (fd->pVDList->pVolDesc->pBlkDev->bd_ioctl(
            					fd->pVDList->pVolDesc->pBlkDev,
                                  		function, arg));     
        } /* switch */
                
    }

/*******************************************************************************
*
* cdromFsRead - routine for reading data from a CD
*
* This routine reads data from a CD into a buffer provided by user.
*
* ERRNO:    
* S_cdromFsLib_SEMGIVE_ERROR
* S_cdromFsLib_VOL_UNMOUNTED
*
* RETURNS: 0 - if end of file is found out,
* number of bytes were read - if any,
* ERROR - error reading or access behind an end of file*
*/

LOCAL STATUS cdromFsRead
    (
    int desc,		/* file descriptor */
    u_char * buffer,	/* buffer for data provided by user */ 
    size_t maxBytes	/* number of bytes to be read */
    )
    {
    T_CDROM_FILE_ID fd = (T_CDROM_FILE_ID) desc;        /* file descriptor */
    u_long rest = maxBytes;               /* number of unread bytes */
    u_long absLb = fd->FCDAbsLB;          /* current absolute LB number */
    u_long absPos = fd->FCDAbsPos;        /* absolute position in the file */
    u_char * ptr;                         /* pointer to file data */
    u_long field;                         /* current portion' length */
    u_int blocks;  	                  /* full blocks to read */
    u_char flag = 0;                      /* flag EOF */	    

    if ((fd == NULL) || (fd->magic != FD_MAG))
    	{
    	errnoSet(S_cdromFsLib_INVALID_FILE_DESCRIPTOR);
    	return ERROR;  /* fd - exists at least ? */
    	}
          
    /* check for volume has not been changed */

    if (((fd->pVDList->pVolDesc->pBlkDev->bd_statusChk != NULL) &&
           fd->pVDList->pVolDesc->pBlkDev->bd_statusChk (
           			 fd->pVDList->pVolDesc->pBlkDev) == ERROR) ||
         fd->pVDList->pVolDesc->pBlkDev->bd_readyChanged ||
         fd->pVDList->pVolDesc->unmounted)
    	{
    	cdromFsVolUnmount (fd->pVDList->pVolDesc);
    	errnoSet (S_cdromFsLib_VOL_UNMOUNTED);
    	return ERROR;
    	}
    	
    if (fd->volUnmount != 0)     /* No valid Volume */
	{
	errnoSet (S_cdromFsLib_VOL_UNMOUNTED);
 	return ERROR;
	}

    if (fd->FCEOF == 1)
        return 0;                                           /* End of File */

    if (semTake (fd->pVDList->pVolDesc->mDevSem, WAIT_FOREVER) == ERROR)
	{
	/* leave errno as set by semLib */
	return ERROR;
	}

    if ((ptr = cdromFsGetLB (fd->pVDList, fd->FCDAbsLB, 
    				               &(fd->sectBuf))) == NULL)  
        {
        semGive (fd->pVDList->pVolDesc->mDevSem);   /* Without checking */
        return ERROR;
        }

    ptr += fd->FCDOffInLB;         /* Starting position */
    
    if ((fd->FCDAbsPos + maxBytes) >= fd->FSize)
	{	/* to prevent reading after bound */  
	maxBytes = fd->FSize - fd->FCDAbsPos;
 	flag = 1;
	}

    rest = maxBytes;
    field = (rest <= fd->pVDList->LBSize - fd->FCDOffInLB) ? 
	    rest : (fd->pVDList->LBSize - fd->FCDOffInLB);
    bcopy (ptr, buffer, field);
    buffer += field;
    absPos += field;
    absLb += (fd->FCDOffInLB + field) /  fd->pVDList->LBSize ;

    /* 
     * interleave will be counted properly or in the cycle below or during 
     * next reading, because we are in the same block (maybe at the bound) 
     */

    rest -= field;
    if (rest > 0)
        {    
        /* readings of full blocks */
	if ((blocks = rest / fd->pVDList->LBSize) > 0)
	    {
	    for (; blocks > 0; blocks--)
		{
		cdromFsSkipGap (fd, &absLb, absPos);
		if ((ptr = cdromFsGetLB (fd->pVDList, absLb,
          	     			    &(fd->sectBuf)))== NULL) 
		    {
		    cdromFsFixPos (fd, maxBytes - rest , absLb); 

		    /* No checking */
                    semGive (fd->pVDList->pVolDesc->mDevSem) ;
       		    return (maxBytes - rest);
		    }

		bcopy (ptr, buffer, fd->pVDList->LBSize);
		buffer += fd->pVDList->LBSize;
    		absPos += fd->pVDList->LBSize;
    		absLb ++;    
    		rest -= fd->pVDList->LBSize;
		}
	    }
	cdromFsSkipGap (fd, &absLb, absPos); /* The last reading */
	if ((ptr = cdromFsGetLB (fd->pVDList, absLb,
				 &(fd->sectBuf)))== NULL) 
	    {
	    cdromFsFixPos (fd, maxBytes - rest , absLb); 
            semGive (fd->pVDList->pVolDesc->mDevSem);  /* No checking */
       	    return (maxBytes - rest);
	    }

	/* The last portion to read */

	bcopy (ptr, buffer, rest);
	absPos += rest;
	}

    if  (flag)
	{
        fd->FCEOF = 1;
	}  /* EOF */

    cdromFsFixPos (fd, maxBytes, absLb);
    if (semGive (fd->pVDList->pVolDesc->mDevSem) == ERROR)
	{
	errnoSet (S_cdromFsLib_SEMGIVE_ERROR);
	return ERROR;
	}

    return (maxBytes);
    }

/*******************************************************************************
*
* cdromFsReadyChange - sets special sign in the volume descriptor
*
* This function sets the value with meaning "CD was changed in the
* unit" to the block device structure assigned to the unit. The function can
* be called from the interrupt level
*
* RETURNS: N/A
*/

LOCAL STATUS cdromFsReadyChange
    (
    CDROM_VOL_DESC_ID arg
    )
    {
    if (arg == NULL)
        return ERROR;
    
    arg->pBlkDev->bd_readyChanged = TRUE;     /* It is "short", not a bit */ 
    return OK;       
    }

/*******************************************************************************
*
* cdromFsVolConfigShow - show the volume configuration information
* 
* This routine retrieves the volume configuration for the named cdromFsLib
* device and prints it to standard output.  The information displayed is 
* retrieved from the BLK_DEV structure for the specified device.
*
* RETURNS: N/A
*/

VOID cdromFsVolConfigShow
    (
    void *	arg		/* device name or CDROM_VOL_DESC * */
    )
    {
    CDROM_VOL_DESC *	pVolDesc = arg;
    char *		devName  = arg;
    T_CDROMFS_VD_LST_ID	pVDList;
    char *	nameTail;
    u_int	lsNum;
    
    if (arg == NULL)
        {
        printf("\
	device name or CDROM_VOL_DESC * must be supplyed as parametr\n");
        return;
        }

    /* check type of supplyed argument */

    if (pVolDesc->magic != VD_SET_MAG)
	{
	/* if not CDROM_VOL_DESC_ID, may be device name */

	pVolDesc = (CDROM_VOL_DESC_ID)iosDevFind(devName, &nameTail);

	if (nameTail == devName ||
	    (pVolDesc == NULL) || pVolDesc->magic != VD_SET_MAG)
	    {
	    printf("not cdrom fs device");
	    return;
	    }
	}

    devName = pVolDesc->devHdr.name;

    printf("\
\ndevice config structure ptr	0x%lx\n\
device name			%s\n\
bytes per blkDevDrv sector	%ld\n",
	(u_long)pVolDesc,
	devName,
	pVolDesc->pBlkDev->bd_bytesPerBlk);

    if (pVolDesc->unmounted)
	{
	printf("no volume mounted\n");
	return;
	}
    
    for (pVDList = (T_CDROMFS_VD_LST_ID) lstFirst(&(pVolDesc->VDList)),
	   lsNum = 16;	pVDList != NULL;	lsNum ++,
	 pVDList = (T_CDROMFS_VD_LST_ID) lstNext((NODE *)pVDList))
	{
	u_char *	pData;
	T_ISO_PVD_SVD	volDesc;
	char *	non  = "none";
	char *	space = " ";
	struct
	{
	    char	year[ ISO_V_DATE_TIME_YEAR_SIZE +1 ],
			month[ ISO_V_DATE_TIME_FIELD_STD_SIZE +1 ],
			day[ ISO_V_DATE_TIME_FIELD_STD_SIZE +1 ],
			hour[ ISO_V_DATE_TIME_FIELD_STD_SIZE +1 ],
			minute[ ISO_V_DATE_TIME_FIELD_STD_SIZE +1 ],
			sec[ ISO_V_DATE_TIME_FIELD_STD_SIZE +1 ],
			sec100[ ISO_V_DATE_TIME_FIELD_STD_SIZE +1 ];
	    } date;
	
	if (semTake (pVolDesc->mDevSem, WAIT_FOREVER) == ERROR)
	    return;
	    
	pData = cdromFsGetLS (pVolDesc, lsNum, &(pVolDesc->sectBuf));

	if (pData == NULL)
	    {
	    semGive (pVolDesc->mDevSem);
	    printf ("error reading volume\n");
	    return;
	    }
	    
	/* fill in volume configuration structure */

	bzero (&volDesc, sizeof (volDesc));
	
	volDesc.volSize		= pVDList->volSize;
	volDesc.PTSize		= pVDList->PTSize;
	volDesc.volSetSize	= pVDList->volSetSize;
	volDesc.volSeqNum	= pVDList->volSeqNum;
	volDesc.LBSize		= pVDList->LBSize;
	volDesc.PTSize		= pVDList->PTSize;
	volDesc.type		= pVDList->type;	
	volDesc.fileStructVersion	= pVDList->fileStructVersion;
	
	bcopy (((T_ISO_VD_HEAD_ID)pData)->stdID, volDesc.stdID,
	       ISO_STD_ID_SIZE );
	bcopy (pData + ISO_VD_SYSTEM_ID , volDesc.systemId,
	       ISO_VD_SYSTEM_ID_SIZE );
	bcopy (pData + ISO_VD_VOLUME_ID , volDesc.volumeId,
	       ISO_VD_ID_STD_SIZE );
	bcopy (pData + ISO_VD_VOL_SET_ID , volDesc.volSetId,
	       ISO_VD_ID_STD_SIZE);
	bcopy (pData + ISO_VD_PUBLISH_ID , volDesc.publisherId,
	       ISO_VD_ID_STD_SIZE );
	bcopy (pData + ISO_VD_DATA_PREP_ID , volDesc.preparerId,
	       ISO_VD_ID_STD_SIZE );
	bcopy (pData + ISO_VD_APPLIC_ID , volDesc.applicatId,
	       ISO_VD_ID_STD_SIZE );
	         
	bcopy (pData + ISO_VD_COPYR_F_ID , volDesc.cprightFId,
	       ISO_VD_F_ID_STD_SIZE );
	bcopy (pData + ISO_VD_ABSTR_F_ID , volDesc.abstractFId,
	       ISO_VD_F_ID_STD_SIZE );
	bcopy (pData + ISO_VD_BIBLIOGR_F_ID , volDesc.bibliogrFId,
	       ISO_VD_F_ID_STD_SIZE );
	
	bcopy (pData + ISO_VD_VOL_CR_DATE_TIME , &(volDesc.creationDate),
	       ISO_VD_VOL_DATE_TIME_STD_SIZE );
	bcopy (pData + ISO_VD_VOL_MODIF_DATE_TIME, &(volDesc.modificationDate),
	       ISO_VD_VOL_DATE_TIME_STD_SIZE );
	bcopy (pData + ISO_VD_VOL_EXPIR_DATE_TIME , &(volDesc.expirationDate),
	       ISO_VD_VOL_DATE_TIME_STD_SIZE );
	bcopy (pData + ISO_VD_VOL_EFFECT_DATE_TIME , &(volDesc.effectiveDate),
	       ISO_VD_VOL_DATE_TIME_STD_SIZE );

	printf("\
\t%s directory hierarchy:	\n\n\
standard ID			:%s\n\
volume descriptor version	:%u\n\
system ID			:%s\n\
volume ID			:%s\n\
volume size			:%lu = %lu MB\n\
number of logical blocks	:%lu = 0x%lx\n\
volume set size 		:%u\n\
volume sequence number 		:%u\n\
logical block size		:%u\n\
path table size (bytes)		:%lu\n\
path table entries		:%u\n\
volume set ID			:%s\n\
volume publisher ID		:%s\n\
volume data preparer ID		:%s\n\
volume application ID		:%s\n\
copyright file name		:%s\n\
abstract file name		:%s\n\
bibliographic file name		:%s\n",
	       (volDesc.type == ISO_VD_PRIMARY)? "\nPrimary" :
						   "\nSuplementary",
		volDesc.stdID,
		(u_int)volDesc.fileStructVersion,
		volDesc.systemId,
		volDesc.volumeId,
		volDesc.volSize * volDesc.LBSize,
			volDesc.volSize * volDesc.LBSize / 0x100000,
		volDesc.volSize,
			volDesc.volSize,
		(u_int)volDesc.volSetSize,
		(u_int)volDesc.volSeqNum,
		(u_int)volDesc.LBSize,
		volDesc.PTSize,
		pVDList->numPTRecs,
		volDesc.volSetId,
		volDesc.publisherId,
		volDesc.preparerId,
		volDesc.applicatId,
		(strspn(volDesc.cprightFId, space) == ISO_VD_F_ID_STD_SIZE)?
			non : (char *)volDesc.cprightFId,
		(strspn(volDesc.abstractFId, space) == ISO_VD_F_ID_STD_SIZE)?
			non : (char *)volDesc.abstractFId,
		(strspn(volDesc.bibliogrFId, space) == ISO_VD_F_ID_STD_SIZE)?
			non : (char *)volDesc.bibliogrFId
	     );
    
	/* date - time */
	volDesc.creationDate =
		*(T_ISO_VD_DATE_TIME_ID)(pData + ISO_VD_VOL_CR_DATE_TIME);
	volDesc.modificationDate =
		*(T_ISO_VD_DATE_TIME_ID)(pData + ISO_VD_VOL_MODIF_DATE_TIME);
	volDesc.expirationDate =
		*(T_ISO_VD_DATE_TIME_ID)(pData + ISO_VD_VOL_EXPIR_DATE_TIME);
	volDesc.effectiveDate =
		*(T_ISO_VD_DATE_TIME_ID)(pData + ISO_VD_VOL_EFFECT_DATE_TIME);

	bzero (&date, sizeof (date));
	bcopy (volDesc.creationDate.year , date.year, 
	       ISO_V_DATE_TIME_YEAR_SIZE );
	bcopy (volDesc.creationDate.month , date.month, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.creationDate.day , date.day, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.creationDate.hour , date.hour, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.creationDate.minute , date.minute, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.creationDate.sec , date.sec, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.creationDate.sec100 , date.sec100, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	       
	printf ("\
creation date			:%s.%s.%s  %s:%s:%s:%s\n",
	       date.day, date.month, date.year,
	       date.hour, date.minute, date.sec, date.sec100
	     );

	bzero (&date, sizeof(date));
	bcopy (volDesc.modificationDate.year , date.year, 
	       ISO_V_DATE_TIME_YEAR_SIZE );
	bcopy (volDesc.modificationDate.month , date.month, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.modificationDate.day , date.day, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.modificationDate.hour , date.hour, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.modificationDate.minute , date.minute, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.modificationDate.sec , date.sec, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.modificationDate.sec100 , date.sec100, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	       
	printf ("\
modification date		:%s.%s.%s  %s:%s:%s:%s\n",
	       date.day, date.month, date.year,
	       date.hour, date.minute, date.sec, date.sec100
	     );

	bzero (&date, sizeof(date));
	bcopy (volDesc.expirationDate.year , date.year, 
	       ISO_V_DATE_TIME_YEAR_SIZE );
	bcopy (volDesc.expirationDate.month , date.month, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.expirationDate.day , date.day, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.expirationDate.hour , date.hour, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.expirationDate.minute , date.minute, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.expirationDate.sec , date.sec, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.expirationDate.sec100 , date.sec100, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	       
	printf ("\
expiration date			:%s.%s.%s  %s:%s:%s:%s\n",
	       date.day, date.month, date.year,
	       date.hour, date.minute, date.sec, date.sec100
	     );

	bzero (&date, sizeof(date));
	bcopy (volDesc.effectiveDate.year , date.year, 
	       ISO_V_DATE_TIME_YEAR_SIZE );
	bcopy (volDesc.effectiveDate.month , date.month, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.effectiveDate.day , date.day, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.effectiveDate.hour , date.hour, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.effectiveDate.minute , date.minute, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.effectiveDate.sec , date.sec, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
	bcopy (volDesc.effectiveDate.sec100 , date.sec100, 
	       ISO_V_DATE_TIME_FIELD_STD_SIZE );
		
	printf ("\
effective date			:%s.%s.%s  %s:%s:%s:%s\n",
	       date.day, date.month, date.year,
	       date.hour, date.minute, date.sec, date.sec100
	     );
			
	semGive (pVolDesc->mDevSem);
	}
    return;
    }

/*******************************************************************************
*
* cdromFsDevDelete - delete a cdromFsLib device from I/O system.
*
* This routine unmounts device and deallocates all memory.
* Argument <arg> defines device to be reset. It may be device name
* or ptr to CDROM_VOL_DESC.
* Argument <retStat> must be 0.
*
* RETURNS: NULL
*
* SEE ALSO:  cdromFsInit(), cdromFsDevCreate()
*/

LOCAL CDROM_VOL_DESC_ID cdromFsDevDelete
    (
    void *	arg,		/* device name or ptr to CDROM_VOL_DESC */
    STATUS	retStat		/* 0 only */
    )
    {
    CDROM_VOL_DESC_ID	pVolDesc = arg;
    char *	devName = arg;
    char *	nameTail;
    
    if (retStat == ERROR)
	{
	printf ("cdromFsLib ERROR: device init failed: ");
	printErrno (errnoGet());
	}

    if (arg == NULL)
        {
        printf ("Invalid argument\n");
	goto ret;
	}

    /* check type of supplyed argument */

    if (pVolDesc->magic != VD_SET_MAG)
	{
	/* if not CDROM_VOL_DESC_ID, may be device name? */

	pVolDesc = (CDROM_VOL_DESC_ID)iosDevFind(devName, &nameTail);
	
	if (nameTail == devName ||
	    (pVolDesc == NULL) || pVolDesc->magic != VD_SET_MAG)
	    {
	    printf ("not cdrom fs device");
	    return NULL;
	    }
	}

    /* 
     * retStat == ERROR indicates request from cdromFsDevCreate
     * routine in case one failed, and so device not connected to list yet.
     * If not,- delete device from list
     */

    if (retStat != ERROR)
        iosDevDelete ((DEV_HDR  *)pVolDesc);

    /* make all resets and memory deallocations */
    
    /* firstly, private semaphore */

    if (pVolDesc->mDevSem != NULL)
	semTake (pVolDesc->mDevSem, WAIT_FOREVER);
    
    cdromFsVolUnmount (pVolDesc);	/* this routine resets VD and
    					 * FD lists */
    cdromFsSectBufFree (&(pVolDesc->sectBuf));

    /* volume descriptor inconsistent */

    pVolDesc->magic = 0;
    pVolDesc->pBlkDev = NULL;

    if (pVolDesc->mDevSem != NULL)
	{
	semGive (pVolDesc->mDevSem);
	semDelete (pVolDesc->mDevSem);
	}

    free (pVolDesc);

ret:
    return NULL;
    }

/*******************************************************************************
* 
* cdromFsDevCreate - create a cdromFsLib device
*
* This routine creates an instance of a cdromFsLib device in the I/O system.
* As input, this function requires a pointer to a BLK_DEV structure for 
* the CD-ROM drive on which you want to create a cdromFsLib device.  Thus,
* you should already have called scsiBlkDevCreate() prior to calling
* cdfromFsDevCreate().
*
* RETURNS: CDROM_VOL_DESC_ID, or NULL if error.
*
* SEE ALSO: cdromFsInit()
*/

CDROM_VOL_DESC_ID cdromFsDevCreate
    (
    char *	devName,    /* device name */
    BLK_DEV *	pBlkDev     /* ptr to block device */
    )
    {
    CDROM_VOL_DESC_ID	pVolDesc;

    if (cdromFsDrvNum == ERROR)
	{
	if (cdromFsInit() == ERROR)
	    return NULL ;
	}
    
    /* TBD check pBlkDev contents */

    if ((pBlkDev == NULL))
    	return NULL;

    pVolDesc = malloc(sizeof(CDROM_VOL_DESC));

    if (pVolDesc == NULL)
	return cdromFsDevDelete(pVolDesc, ERROR);
    
    bzero ((u_char *) pVolDesc, sizeof(CDROM_VOL_DESC));

    pVolDesc->mDevSem = NULL;		/* semaphore not created */
    lstInit(&(pVolDesc->VDList));	/* preparing for VD list */
    lstInit(&(pVolDesc->FDList));	/* preparing for FD list */
    pVolDesc->unmounted = 1;		/* VDList must be built */
    pVolDesc->pBlkDev  = pBlkDev;

    /* 
     * adapt to blkDevDrv block size (think of CDROM_STD_LS_SIZE is
     * less than or multiple of bd_bytesPerBlk)
     */

    if (pBlkDev->bd_bytesPerBlk < CDROM_STD_LS_SIZE)
    	{
	pVolDesc->sectSize = CDROM_STD_LS_SIZE;
	pVolDesc->LSToPhSSizeMult =
				   CDROM_STD_LS_SIZE / pBlkDev->bd_bytesPerBlk;
	}
    else
    	{
	pVolDesc->sectSize = pBlkDev->bd_bytesPerBlk;
	pVolDesc->LSToPhSSizeMult = 1;
	}
	
    pVolDesc->magic = VD_SET_MAG;
    
    /* common sectors buffer initiation */

    pVolDesc->sectBuf.sectData = NULL;

    if (cdromFsSectBufAlloc (pVolDesc, &(pVolDesc->sectBuf), 0) == ERROR)
	return cdromFsDevDelete (pVolDesc, ERROR);

    /* create device protection mutual-exclusion semaphore */

    pVolDesc->mDevSem = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);

    if (pVolDesc->mDevSem == NULL)
	return cdromFsDevDelete (pVolDesc, ERROR);

    /* add device to device list */

    if (iosDevAdd ((DEV_HDR *)pVolDesc, devName,
		cdromFsDrvNum) == ERROR)
	return cdromFsDevDelete (pVolDesc, ERROR);
	
    DBG_MSG(50)("cdromFsDevCreate done for BLK_DEV * = 0x%x, sect = %d\n",
		(int)pBlkDev, pVolDesc->sectSize);

    return pVolDesc;
    }
