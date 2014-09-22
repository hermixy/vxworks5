/* dosChkLib.c - DOS file system sanity check utility */ 

/* Copyright 1999-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,10nov01,jkf  SPR#67890, chkdsk writes to RDONLY device
01n,18oct01,jkf   warning cleaning.
01n,18sep00,nrj   Fixed SPR#34108, check the write mode before synchronzing the
                  FAT
01m,29feb00,jkf   T3 changes
01l,31aug99,jkf   changes for new CBIO API.  Changed date check logic.
                  Added docs for date check.  Added global for date
                  check.  Default debug global to zero.
01k,31jul99,jkf   T2 merge, tidiness & spelling.
01j,28sep98,vld   gnu extensions dropped from DBG_MSG/ERR_MSG macros
01i,13oct98,vld   fixed message positioning on  screen, - '\b' avoided.
01h,23sep98,vld   added usage of reserved FAT32 copy for data buffering
		  in reduced memory environments, bit map of FAT entries
		  is filled in parallel with 4-byte MAP array in this case
01g,23sep98,vld   fixed bug of backward time setting
01f,23sep98,vld   disable FAT mirroring while check disk in progress;
		  synchronize FAT copies after check disk.
01e,17sep98,vld   FAT copy number argument added to calls of
		  clustValueSet()/clustValueGet();
01d,08jul98,vld   print64Lib.h moved to h/private directory.
01c,08jul98,vld   fixed bug in dosFsChkTree() of deletion
		  invalid name entry through other file descriptor.
01b,02jul98,lrn   review doc and messages
01a,18jan98,vld	  written, preliminary
*/

/*
DESCRIPTION
This library is part of dosFsLib, it manages the Consistency Checking
procedure, which may be invoked automatically during device mount
procedure, or manual by means of the FIOCHKDSK ioctl.

be checked which is managed by that particular handler.

Check disk requires temporary buffer large enough to store information
on every FAT entry. Memory requirements can be a problem for FAT32
volumes. If no enough available, temporary data is buffered in
second (reserved) FAT copy, that can take extremely long time on a 
slow processors.

The minimum acceptable system time & date is set by the global variable
(time_t) dosChkMinDate.  If the system time is greater than that value, 
it is assumed that a hardware clock mechanism is in place and the time 
setting is honored.  If dosChkLib detects that the system is set to an 
earlier date than dosChkMinDate, the system date will be adjusted to 
either the most recent date found on the file system volume or the minimum 
acceptable system time & date, whichever is later.  A warning indicicating 
the new system time is displayed on the console when this occurs.
Do not set dosChkMinDate prior to 1980!

INTERNAL
*/

/* includes */

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stat.h"
#include "time.h"
#include "dirent.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "errnoLib.h"
#include "memLib.h"
#include "taskLib.h"
#include "time.h"
#include "timers.h"
#include "tickLib.h"

#include "dosFsLib.h"
#include "private/print64Lib.h"
#include "private/dosFsLibP.h"

/* defines */

/* macros */

#undef DBG_MSG
#undef ERR_MSG
#undef NDEBUG

#ifdef DEBUG
#   undef LOCAL
#   define LOCAL
#   undef ERR_SET_SELF
#   define ERR_SET_SELF
#   define DBG_MSG( lvl, fmt, arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8 )	\
	{ if( (lvl) <= dosChkDebug )					\
	    printErr( "%s : %d : " fmt,				\
	               __FILE__, __LINE__, arg1,arg2,	\
		       arg3,arg4,arg5,arg6,arg7,arg8 ); }

#   define ERR_MSG( lvl, fmt, a1,a2,a3,a4,a5,a6 )		\
        { logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }
#else	/* NO DEBUG */

#   define NDEBUG
#   define DBG_MSG(lvl,fmt,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) 	{}

#   define ERR_MSG( lvl, fmt, a1,a2,a3,a4,a5,a6 )		\
	{ if( (lvl) <= dosChkDebug ) 				\
            logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }

#endif /* DEBUG */

#include "assert.h"

#define DOSCHKLIB_YEAR_VALUE (((60*60)*24)*365*21)

int dosChkDebug = 0;

time_t dosChkMinDate = (time_t) DOSCHKLIB_YEAR_VALUE;


/* error messages */

#define CHK_CROSS_L_START_MSG	"\"%s\" is cross-linked on first cluster. "
#define CHK_START_MSG		"%s  - disk check in progress ..."
#define CHK_NOT_ROOM_MSG	"%s - Not enough room to run check disk.\n"
#define CHK_LOST_CHAINS_MSG	" - lost chains "
#define CHK_DEL_MSG		" Entry will be deleted. "
#define CHK_REMAIN_MSG		"Entry remains intact. "
#define CHK_CLUST_VAL		"(cluster %lu, value %lu)."
#define CHK_SHORT_CHAIN_MSG	"\"%s\" file larger then its chain of clusters"
#define CHK_BAD_DIR_MSG		"\"%s\" directory has invalid structure."
#define CHK_DIR_LVL_OVERFL "directory nesting limit reached, \"%s\" skipped"
#define CHK_ROOT_CHAIN	 "%s - Root directory cluster chain is corrupted.\n"
#define CHK_ROOT_TRUNC_MSG	"      Root directory truncated. \n"
#define CHK_BAD_NAME		"\"%s\" is an illegal name. "
#define CHK_FAIL_MSG		"     Check disk failed. \n"\
                                "     Disk should be formatted.\n"
#define CHK_NOT_REPAIRED	"%s  - Errors detected. "\
				"To correct disk structure, please start "\
				"check disk with a permission to repair."
#define CHK_RESTORE_FREE	"Errors detected. "\
				"All corrections stored to disk "\
				"and lost chains recovered."
#define CHK_BAD_START_MSG	"\"%s\" has illegal start cluster"
#define CHK_CROSS_START_MSG	"\"%s\" cross-linked at start"
#define CHK_BAD_CHAIN_MSG	"\"%s\" has illegal cluster in chain"
#define CHK_CROSS_LINK_MSG	"\"%s\" cross-linked with another chain"
#define CHK_LONG_CHAIN_MSG	"\"%s\" too many clusters in file, adjusted."
#define CHK_BAD_CLUST_MSG	"illegal cluster"
#define CHK_NO_ERRORS		"%s  - Volume is OK "

#define START_CLUST_MASK	0x80000000
#define LOST_CHAIN_MARK		0xc0000000

#define MARK_SET	1
#define MARK_GET	2
#define IS_BUSY		3
#define MARK_BUF_INIT	255

/* typedefs */

/* check disk status */
 
typedef enum {
             CHK_OK=OK, CHK_SIZE_ERROR,
             CHK_RESTART, CHK_BAD_DIR, CHK_BAD_FILE,
             CHK_CROSS_LINK, CHK_BAD_CHAIN, CHK_BAD_START,
             CHK_ERROR=ERROR
             }  CHK_STATUS;
        

/* dosChkMsg() argument */

typedef enum { CURRENT_PATH, GET_PATH }	MSG_PATH;

/* dosChkEntryFind() argument */

typedef enum { FIND_BY_CLUST, FIND_BY_NAME }	CHK_SEARCH_KEY_TYPE;


/* globals */

/* locals */

/* forward declarations */

LOCAL void dosChkChainUnmark
    (
    DOS_FILE_DESC_ID    pFd             /* pointer to file descriptor */
    );

LOCAL STATUS dosChkChainStartGet
    (
    DOS_FILE_DESC_ID    pFd,            /* pointer to file descriptor */
    uint32_t            entNum,
    uint32_t *          pParDirStartClust,
    uint32_t *          pStartClust
    );

LOCAL CHK_STATUS dosChkChainVerify
    (
    DOS_FILE_DESC_ID    pFd            /* pointer to file descriptor */
    );

LOCAL STATUS dosChkLostFind
    (
    DOS_FILE_DESC_ID    pFd             /* pointer to file descriptor */
    );

LOCAL STATUS dosChkLostFree
    (
    DOS_FILE_DESC_ID    pFd             /* pointer to file descriptor */
    );


/*******************************************************************************
*
* dosChkStatPrint - print statistics.
*
* RETURNS: N/A.
*/
LOCAL void dosChkStatPrint
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    CHK_DSK_DESC_ID	pChkDesc = pFd->pVolDesc->pChkDesc;
    fsize_t	valBuf;
    
    if( pVolDesc->chkVerbLevel < (DOS_CHK_VERB_1 >> 8) )
    	return;
    
    /* print statistics */
    
    print64Fine(  "\n" "	  total # of clusters:	", 
	pVolDesc->nFatEnts, "\n", 10 );

    print64Fine( "	   # of free clusters:	", 
	pChkDesc->chkNFreeClusts, "\n", 10 );

    print64Fine( "	    # of bad clusters:	", 
	pChkDesc->chkNBadClusts, "\n", 10 );
    
    valBuf = pChkDesc->chkNFreeClusts;
    valBuf = ( valBuf * pVolDesc->secPerClust ) <<
    	     pVolDesc->secSizeShift;
    print64Mult( "	     total free space:	",
	valBuf, "\n", 10 );
    
    valBuf = (fsize_t)pVolDesc->pFatDesc->maxContig( pFd ) << 
    	     pVolDesc->secSizeShift;
    print64Fine( "     max contiguous free space:  ",
	valBuf, " bytes\n", 10 );

    print64Fine( "		   # of files:	",
	pChkDesc->chkNFiles, "\n", 10 );

    print64Fine( "		 # of folders:	",
	pChkDesc->chkNDirs, "\n", 10 );

    print64Mult( "	 total bytes in files:	",
	pChkDesc->chkTotalFSize, "\n", 10 );

    print64Fine( "	     # of lost chains:	",
	pChkDesc->chkNLostChains, "\n", 10 );

    print64Mult( "   total bytes in lost chains:  ",
	      pChkDesc->chkTotalBytesInLostChains, "\n", 10 );
    }
    
/*******************************************************************************
*
* dosChkFinish - print statistics and deallocate resources.
*
* RETURNS: N/A.
*/
LOCAL void dosChkFinish
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    if( pFd->pVolDesc->pChkDesc->chkFatMap != NULL )
    	KHEAP_FREE(((char *)pFd->pVolDesc->pChkDesc->chkFatMap));

    KHEAP_FREE(((char *)pFd->pVolDesc->pChkDesc));
    pFd->pVolDesc->pChkDesc = NULL;
    
    return;
    } /* dosChkFinish() */
    
/*******************************************************************************
*
* dosChkEntryMark - set/get FAT entry marker.
*
* RETURNS: marker value on MARK_GET operation, STATUS on MARK_SET
*  and MARK_BUF_INIT operation.
*/
uint32_t dosChkEntryMark
    (
    DOS_FILE_DESC_ID	pFd,
    u_int	operation,         /* MARK_SET/MARK_GET/MARK_BUF_INIT */
    uint32_t	entryNum,
    uint32_t	mark
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    CHK_DSK_DESC_ID	pChkDesc = pVolDesc->pChkDesc;
    uint8_t	fatCopyNum = 0;

    if (! pChkDesc->bufToDisk)
    	{
    	switch (operation)
    	    {
    	    case MARK_GET :
    	    case IS_BUSY :
    	    	return pChkDesc->chkFatMap [entryNum];
    	    case MARK_SET :
    	    	pChkDesc->chkFatMap [entryNum] = mark;
    	    	break;
    	    case MARK_BUF_INIT :
    	    	bzero ((char *)pChkDesc->chkFatMap,
   	   	       pVolDesc->nFatEnts *
                       sizeof( *pVolDesc->pChkDesc->chkFatMap ));
    	    	break;
    	    default:
    	    	assert (FALSE);
    	    }
      	return OK;
    	}
    else	/* use reserved FAT copy as buffer */
    	{
    	fatCopyNum = pVolDesc->pFatDesc->activeCopyNum + 1;
    	switch (operation)
    	    {
    	    case MARK_GET :
            	pVolDesc->pFatDesc->clustValueGet (pFd, fatCopyNum,
						   entryNum, &mark );
    	    	return mark;

    	    case IS_BUSY :
    	    	return pChkDesc->chkFatMap [
                       entryNum / 8 / sizeof(*pChkDesc->chkFatMap)] &
                       (1 <<
                        (entryNum & (sizeof(*pChkDesc->chkFatMap) * 8 - 1)));

    	    case MARK_SET :
    	    	pChkDesc->chkFatMap [
			entryNum / 8 / sizeof(*pChkDesc->chkFatMap)] |=
	        1 << (entryNum & (sizeof(*pChkDesc->chkFatMap) * 8 - 1));

            	return pVolDesc->pFatDesc->clustValueSet (
			    pFd, fatCopyNum, entryNum, DOS_FAT_RAW, mark );

    	    case MARK_BUF_INIT :
    	    	bzero ((char *)pChkDesc->chkFatMap,
                       pVolDesc->nFatEnts / 8  + 4);
    	    	return pVolDesc->pFatDesc->clustValueSet(
                            pFd, fatCopyNum, 0, DOS_FAT_SET_ALL,  0 );

    	    default:
    	    	assert (FALSE);
    	    }
    	}

    return ERROR;
    } /* dosChkEntryMark() */

/*******************************************************************************
*
* dosChkEntryFind - find entry in directory.
*
* This routine searches directory, that starts from <dirStartClust>,
* for entry, by entry name or start cluster. File descriptor
* <pFd> is filled for entry found and entry name is placed into
* <pChkDesc->chkDir>.
*
* RETURNS: OK or ERROR if disk access error.
*/
LOCAL STATUS dosChkEntryFind
    (
    DOS_FILE_DESC_ID	pFd,
    UINT32	dirStartClust,
    void *	key,	/* start clust value or name buffer */
    CHK_SEARCH_KEY_TYPE	keyType
    )
    {
    UINT32	startClust = (UINT32)key;
    DIR *	pDir = &pFd->pVolDesc->pChkDesc->chkDir;
    DOS_FILE_DESC	fd;
    DOS_FILE_HDL	flHdl;
    
    fd = *pFd;
    flHdl = *pFd->pFileHdl;
    fd.pFileHdl = &flHdl;
    
    if( dirStartClust == 0 )	/* parent is root */
    	{
    	if( pFd->pVolDesc->pDirDesc->pathLkup( &fd, "", 0 ) == ERROR )
    	    return ERROR;
    	}
    else if( dirStartClust == (UINT32)NONE ) /* search for root */
    	{
    	if( pFd->pVolDesc->pDirDesc->pathLkup( &fd, "", 0 ) == ERROR )
    	    return ERROR;
    	strcpy( pDir->dd_dirent.d_name, pFd->pVolDesc->devHdr.name );
    	return OK;
    	}
    else
    	{
    	fd.pFileHdl->startClust = dirStartClust;
    	fd.pFileHdl->attrib = DOS_ATTR_DIRECTORY;
    	}
    
    /* rewind directory */
    
    pDir->dd_cookie = 0;
    
    while( pFd->pVolDesc->pDirDesc->readDir( &fd, pDir, pFd ) == OK )
    	{
    	switch( keyType )
    	    {
    	    case FIND_BY_CLUST:
    	    	if( pFd->pFileHdl->startClust == startClust )
    	    	   return OK;
    	    	break;
    	    case FIND_BY_NAME:
    	    	if( strcmp( (char *)key, pDir->dd_dirent.d_name ) == 0 )
    	    	   return OK;
    	    	break;
    	    default:
    	    	assert( FALSE );
    	    }
    	}
    assert( FALSE );
    return ERROR;
    } /* dosChkEntryFind() */
    
/*******************************************************************************
*
* dosChkBuildPath - reconstruct file path.
*
* This routine reconstructs full path of the file is described
* by <pFd> and puts it to <pChkDesc->chkPath> buffer.
*
* RETURNS: OK or ERROR if disk access error.
*/
LOCAL STATUS dosChkBuildPath
    (
    DOS_FILE_DESC_ID	pSrcFd
    )
    {
    DIR *		pDir = &pSrcFd->pVolDesc->pChkDesc->chkDir;
    DOS_FILE_DESC	fd;
    DOS_FILE_HDL	flHdl;
    u_char *		pPath;
    u_char *		chkPath = pSrcFd->pVolDesc->pChkDesc->chkPath;
    u_short		nameLen;
    
    pPath = chkPath + 
            sizeof( pSrcFd->pVolDesc->pChkDesc->chkPath ) - 1;

    bzero( (char *)chkPath,
    	   sizeof( pSrcFd->pVolDesc->pChkDesc->chkPath ) );
    if( (pSrcFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) != 0 )
    	{
    	pPath --;
    	*pPath = SLASH;
    	}
    
    fd = *pSrcFd;
    flHdl = *pSrcFd->pFileHdl;
    fd.pFileHdl = &flHdl;
    
    FOREVER
    	{
    	if( dosChkEntryFind( &fd, flHdl.dirHdl.parDirStartCluster,
    			     (void *)flHdl.startClust,
    			     FIND_BY_CLUST ) == ERROR )
    	    {
    	    return ERROR;
    	    }
    	
    	/* insert name to path */
    	
    	nameLen = strlen( pDir->dd_dirent.d_name );
    	if( nameLen > pPath - chkPath )
    	    {
    	    do  {
    	    	pPath = memchr( pPath + 1, SLASH, strlen( (char *)pPath ) );
    	    	}
    	    while( pPath - chkPath < 4 );
    	    
    	    pPath -= 4;
    	    *pPath = DOT, *(pPath + 1) = DOT, *(pPath + 2) = DOT;
    	    *(pPath + 3) = SLASH;
    	    break;
    	    }
    	
    	pPath -= strlen( pDir->dd_dirent.d_name );
    	bcopy( pDir->dd_dirent.d_name, (char *)pPath,
    	       strlen( pDir->dd_dirent.d_name ) );
    	
    	if( IS_ROOT( &fd ) )
    	    break;
    	
    	pPath --;
    	*pPath = SLASH;
    	
    	/* find cluster containing parent directory */
    	
    	if( flHdl.dirHdl.parDirStartCluster == 0 )
    	    {	/* current parent directory is root */
    	    flHdl.dirHdl.parDirStartCluster = (UINT32)NONE;
    	    }
    	else if( dosChkChainStartGet(
    			&fd, flHdl.dirHdl.parDirStartCluster, 
    			(uint32_t *)&flHdl.dirHdl.parDirStartCluster,
                	(uint32_t *)&flHdl.startClust ) == ERROR )
            {
            assert( FALSE );
            }
    	}
    
    memmove( chkPath, pPath, strlen( (char *)pPath ) + 1 );
    return OK;
    } /* dosChkBuildPath() */
    
/*******************************************************************************
*
* dosChkMsg - print message.
*
*/
LOCAL void dosChkMsg
    (
    DOS_FILE_DESC_ID	pFd,
    char *	msgFormat,
    MSG_PATH	msgPath, /* whether to build file path or use current path */
    int		arg1,
    int		arg2,
    int		arg3,
    int		arg4,
    char *	extraMsg
    )
    {
    char *	path = NULL;
    
    if( msgPath == CURRENT_PATH )
    	path = pFd->pVolDesc->pChkDesc->chkCurPath;
    else if( dosChkBuildPath( pFd ) == OK )
    	path = (char *)pFd->pVolDesc->pChkDesc->chkPath;
    else
    	return;
    
    printf( "\r" );

    printf( msgFormat, path, arg1, arg2, arg3, arg4 );

    if( extraMsg != NULL && *extraMsg != EOS )
    	printf("%s", extraMsg );

    printf( "\n" );

    return;
    } /* dosChkMsg() */
    
/*******************************************************************************
*
* dosChkEntryDel - delete entry and unmark chain , if required.
*
* This routine deletes entry is described by pFd, unmarks
* associated FAT chain, if <unmark> is TRUE and outputs
* message in accordance with <mess> format string.
*
* RETURNS: CHK_OK, or CHK_ERROR if disk I/O error.
*/
LOCAL CHK_STATUS dosChkEntryDel
    (
    DOS_FILE_DESC_ID	pFd,
    BOOL	unmark,
    u_char *	mess,
    MSG_PATH	msgPath,
    int		msgArg1,
    int		msgArg2,
    int		msgArg3,
    int		msgArg4
    )
    {
    /* put message */
    
    dosChkMsg( pFd, (char *)mess, msgPath, msgArg1, msgArg2, msgArg3, msgArg4,
               CHK_DEL_MSG );
    
    pFd->pVolDesc->pChkDesc->nErrors ++;
    
    if( pFd->pVolDesc->chkLevel > CHK_ONLY )
    	{
    	if( pFd->pVolDesc->pDirDesc->updateEntry(
    				pFd, DH_DELETE, 0 ) == ERROR )
    	    {
    	    return CHK_ERROR;
    	    }
    	}
    
    /* unmark chain */
    
    if( unmark )
    	dosChkChainUnmark( pFd );
    
    /* mark file descriptor as deleted */
    
    pFd->pFileHdl->deleted = 1;
    
    return CHK_OK;
    } /* dosChkEntryDel() */
    
/*******************************************************************************
*
* dosChkStartCrossProc - process entries are cross linked on start cluster.
*
* This routine deletes entry, that was created earlier.
*
* RETURNS: CHK_OK or CHK_RESTART, if directory has been deleted.
*/
LOCAL CHK_STATUS dosChkStartCrossProc
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    DOS_FILE_DESC	localFd;
    DOS_FILE_HDL	localFlHdl;
    UINT32	entryClust;	/* entry location cluster */
    UINT32	startClust;	/* file/dir start cluster */
    struct stat	fStat;		/* used to get creation time */
    time_t	creatTime;	/* creation time of one of the files */
    BOOL	unmark = TRUE;	/* whether to unmark chain */
    MSG_PATH	msgPath = GET_PATH;	/* whether to build file path */
    					/* or to use current path */
    
    assert( pFd != NULL && pFd->pVolDesc->magic == DOS_FS_MAGIC );
    
    bzero( (char *)&localFd, sizeof( localFd ) );
    bzero( (char *)&localFlHdl, sizeof( localFlHdl ) );
    
    localFd.busy = 1;
    localFd.pFileHdl = &localFlHdl;
    localFd.pVolDesc = pFd->pVolDesc;
    
    /* find peer entry */
    
    dosChkChainStartGet( pFd, pFd->pFileHdl->startClust,
    			(uint32_t *)&entryClust, 
                        (uint32_t *)&startClust );
    
    if( dosChkEntryFind( &localFd, entryClust,
    			(void *)startClust, FIND_BY_CLUST ) == ERROR )
    	{
    	return CHK_ERROR;
    	}
    
    assert( pFd->pFileHdl->startClust == localFlHdl.startClust );
    
    /* delete older entry, but never delete ROOT */
    
    pFd->pVolDesc->pDirDesc->dateGet( pFd, &fStat );
    creatTime = fStat.st_ctime;
    pFd->pVolDesc->pDirDesc->dateGet( &localFd, &fStat );
    
    /* let <localFd> describes file to be deleted */
    
    if( IS_ROOT( &localFd ) || creatTime >= fStat.st_ctime )
    	{
    	dosChkMsg( &localFd, CHK_CROSS_L_START_MSG, GET_PATH,
    		   0,0,0,0, CHK_REMAIN_MSG );
    	
    	localFd = *pFd;
    	localFlHdl =  *pFd->pFileHdl;
    	localFd.pFileHdl = &localFlHdl;
    	
    	/* the chain already checked. Do not unmark and recheck */
    	
    	unmark = FALSE;
    	msgPath = CURRENT_PATH;
    	pFd->pFileHdl->deleted = 1; 
    	}
    else
    	{
    	dosChkMsg( pFd, CHK_CROSS_L_START_MSG, CURRENT_PATH,
    		   0,0,0,0, CHK_REMAIN_MSG );
    	}
    
    /* delete the file */
    
    return dosChkEntryDel( &localFd, unmark, 
    			   (u_char *)CHK_CROSS_L_START_MSG, msgPath, 0,0,0,0 );
    } /* dosChkStartCrossProc() */
    
/*******************************************************************************
*
* dosChkDirStruct - check internal directory structure.
*
* This routine checks, that directory has valid . and ..
* entries at top of it.
*
* RETURNS: CHK_OK, CHK_RESTART, if the directory has been deleted or
* CHK_ERROR if another error occurred.
*/
LOCAL CHK_STATUS dosChkDirStruct
    (
    DOS_FILE_DESC_ID	pDirFd,
    DOS_FILE_DESC_ID	pLocalFd
    )
    {
    u_char	dots;
    DIR *	pDir = &pDirFd->pVolDesc->pChkDesc->chkDir;
    
    /* root directory does not contain . and .. entries */
    
    if( IS_ROOT( pDirFd ) )
    	return CHK_OK;
    
    for( dots = 1; dots <= 2; dots ++ )
    	{
    	if( pDirFd->pVolDesc->pDirDesc->readDir(
    				pDirFd, pDir, pLocalFd ) != OK )
    	    {
    	    break;
    	    }
    	if( *(pDir->dd_dirent.d_name) != '.' ||
    	    *(pDir->dd_dirent.d_name + dots - 1) != '.' ||
    	    *(pDir->dd_dirent.d_name + dots) != EOS )
    	    {
    	    break;
    	    }
    	if( dots == 1 &&	/* . */
    	    pDirFd->pFileHdl->startClust !=
    	    pLocalFd->pFileHdl->startClust )
    	    {
    	    break;
    	    }
    	else if( dots == 2 &&	/* .. */
    	        pDirFd->pFileHdl->startClust !=
    	        pLocalFd->pFileHdl->dirHdl.parDirStartCluster )
    	    {
    	    break;
    	    }
    	}
    
    if( dots <= 2 )
    	{
    	return dosChkEntryDel( pDirFd, TRUE, (u_char *)CHK_BAD_DIR_MSG,
    			       CURRENT_PATH, 0,0,0,0 );
    	}
    
    return CHK_OK;  
    } /* dosChkDirStruct() */
    
/*******************************************************************************
*
* dosFsChkTree - DOS FS sanity check utility.
*
* RETURNS: OK or ERROR if device is not a valid DOS device,
*  corrections can not be written to disk
*  or disk read error.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
*/
CHK_STATUS dosFsChkTree
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    CHK_DSK_DESC_ID	pChkDesc = pFd->pVolDesc->pChkDesc;
    DOS_FILE_DESC	localFd;
    DOS_FILE_HDL	localFlHdl;
    UINT32	work1;
    u_long	ddCookie;
    char *	pCurPathPos = NULL;
    	
    assert( pFd != NULL && pFd->pVolDesc->magic == DOS_FS_MAGIC );
        
    bzero( (char *)&localFd, sizeof( localFd ) );
    bzero( (char *)&localFlHdl, sizeof( localFlHdl ) );
    
    localFd.busy = 1;
    localFd.pFileHdl = &localFlHdl;
    localFd.pVolDesc = pFd->pVolDesc;
    
    /* separately check ROOT directory chain */
    
    if( IS_ROOT( pFd ) )
    	{
	if( pFd->pFileHdl->startClust != 0 &&
            dosChkChainVerify( pFd ) != CHK_OK )
    	    {
    	    return CHK_ERROR;
    	    }
    	}
    
    pCurPathPos = pChkDesc->chkCurPath + 
    		  strlen( pChkDesc->chkCurPath );
    *pCurPathPos = SLASH;
    pCurPathPos ++;
    
    /* rewind directory */
    
    pChkDesc->chkDir.dd_cookie = 0;
    
    /* check . and .. */
    
    work1 = dosChkDirStruct( pFd, &localFd );
    if( work1 != CHK_OK )
    	{
    	pChkDesc->nErrors ++;
    	return work1;
    	}
    
    /* pass directory */
    
    for(  ;
         pFd->pVolDesc->pDirDesc->readDir(
    			pFd, &pChkDesc->chkDir, &localFd ) == OK;
    	 pChkDesc->chkDir.dd_cookie = ddCookie )
    	{
    	/*
    	 * backup current position in directory, because
    	 * other routines use the same DIR structure
    	 */
    	ddCookie = pChkDesc->chkDir.dd_cookie;
    	
    	work1 = strlen( pCurPathPos );
    	strcpy( pCurPathPos, pChkDesc->chkDir.dd_dirent.d_name );
    	
    	if( pFd->pVolDesc->chkVerbLevel >= (DOS_CHK_VERB_2 >> 8) )
    	    {
    	    printf( "\r%s", pChkDesc->chkCurPath );
    	    for(  ; work1 >= strlen( pCurPathPos ); work1 -- )
    	    	printf( " " );	/* clear extra characters */
    	
    	    }
    	
    	DBG_MSG( 100, "%s : stCl %u, size %u attrib %p\n",
    		pChkDesc->chkDir.dd_dirent.d_name,
    		localFlHdl.startClust, localFlHdl.size,
    		(u_int)localFlHdl.attrib,0,0,0,0 );
    	
    	/* some statistics */
    	
    	if( (localFlHdl.attrib & DOS_ATTR_DIRECTORY) == 0 )
    	    pChkDesc->chkNFiles ++;
    	else
    	    pChkDesc->chkNDirs ++;
    	
    	/* check name */
    	
    	if( pFd->pVolDesc->pDirDesc->nameChk != NULL &&
    	    pFd->pVolDesc->pDirDesc->nameChk( pFd->pVolDesc, 
    	    		(u_char *)pChkDesc->chkDir.dd_dirent.d_name) == ERROR)
    	    {
    	    if( dosChkEntryDel( &localFd, FALSE, (u_char *)CHK_BAD_NAME, 
				CURRENT_PATH, 0,0,0,0 ) == CHK_ERROR )
    	    	{
    	    	return CHK_ERROR;
    	    	}
    	    
    	    continue;
    	    }
    	
    	/* check chain */
    	
    	if( dosChkChainVerify( &localFd ) == CHK_ERROR )
    	    return CHK_ERROR;
    	
    	if( localFlHdl.deleted )
    	    {
    	    if( (localFlHdl.attrib & DOS_ATTR_DIRECTORY) == 0 )
    	    	pChkDesc->chkNFiles --;
    	    else
    	    	pChkDesc->chkNDirs --;

    	    continue;
    	    }
    	    	
    	
    	/* chain is OK. */
    	 
    	/* total files size */
    	
    	pChkDesc->chkTotalFSize += localFlHdl.size;
    	
    	/* absolute max time */
    	
    	if( pFd->pVolDesc->pDirDesc->dateGet(
    		&localFd, &pChkDesc->stat ) == ERROR )
    	    {
    	    return CHK_ERROR;
    	    }
    	
    	if( pChkDesc->chkMaxCreatTime < pChkDesc->stat.st_ctime )
    	    pChkDesc->chkMaxCreatTime = pChkDesc->stat.st_ctime;
    	
    	if( pChkDesc->chkMaxModifTime < pChkDesc->stat.st_mtime )
    	    pChkDesc->chkMaxModifTime = pChkDesc->stat.st_mtime;
    	
    	if( pChkDesc->chkMaxAccTime < pChkDesc->stat.st_atime )
    	    pChkDesc->chkMaxAccTime = pChkDesc->stat.st_atime;
    	
    	/* recursively check directory */
    	
    	if( (localFlHdl.attrib & DOS_ATTR_DIRECTORY) != 0 )
    	    {
    	    if( pChkDesc->curPathLev ==  DOS_MAX_DIR_LEVELS )
    	    	{
    	    	pChkDesc->nErrors ++;
    	    	dosChkEntryDel( &localFd, TRUE, (u_char *)CHK_DIR_LVL_OVERFL,
    	    			CURRENT_PATH, 0,0,0,0 );
    	    	continue;
    	    	}
    	    
    	    pChkDesc->curPathLev ++;
    	    work1 = dosFsChkTree( &localFd );
    	    pChkDesc->curPathLev --;
    	    
    	    if( work1 != CHK_OK )
    	    	{
    	    	return work1;
    	    	}
    	    }
    	} /* for ... */
    
    
    /* check, that device is OK yet */
    
    if( TRUE == cbioRdyChgdGet (pFd->pVolDesc->pCbio) )
    	return CHK_ERROR;
    
    return CHK_OK;
    } /* dosFsChkTree() */

/*******************************************************************************
*
* dosChkInit - allocate resources.
*
* RETURNS: OK or ERROR if KHEAP_ALLOC failed.
*/
LOCAL STATUS dosChkInit
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    void *	chkFatMap;
    BOOL	bufToDisk;
    
    if( pVolDesc->pChkDesc == NULL )
    	{
    	/* allocate descriptor */

    	pVolDesc->pChkDesc = KHEAP_ALLOC((sizeof(*pVolDesc->pChkDesc)));
    	if( pVolDesc->pChkDesc == NULL )
    	    return ERROR;
    	
    	pVolDesc->pChkDesc->bufToDisk = FALSE;

    	/* check memory size and allocate FAT buffer */

    	pVolDesc->pChkDesc->chkFatMap = NULL; /* do not remove this string */

    	if( ((u_long) (memFindMax())  <=  
	     (u_long) (pVolDesc->nFatEnts * sizeof 
			(*pVolDesc->pChkDesc->chkFatMap) +32)) ||
	    (NULL == (pVolDesc->pChkDesc->chkFatMap = 
			KHEAP_ALLOC( ((pVolDesc->nFatEnts) * 
			(sizeof(*pVolDesc->pChkDesc->chkFatMap)))))) )
    	    {
    	    /*
             * only FAT32 disks can use reserved FAT copy instead
             * of memory buffer
             */
    	    if( (pVolDesc->fatType != FAT32) 				    ||
    	    	(pVolDesc->pFatDesc->activeCopyNum == pVolDesc->nFats - 1)  ||
    	    	((pVolDesc->pChkDesc->chkFatMap = 
			KHEAP_ALIGNED_ALLOC(\
				(sizeof(*pVolDesc->pChkDesc->chkFatMap)),\
                                (pVolDesc->nFatEnts / 8 + 4))) == NULL))
            	{
    	    	printf( CHK_NOT_ROOM_MSG, pVolDesc->devHdr.name );
                dosChkFinish( pFd );
    	    	return ERROR;
    	    	}

    	    pVolDesc->pChkDesc->bufToDisk = TRUE;
    	    }

        if( pVolDesc->chkVerbLevel >= (DOS_CHK_VERB_1 >> 8) )
    	    dosChkMsg( pFd, CHK_START_MSG, GET_PATH, 0,0,0,0,  NULL );
    	}
    
    chkFatMap = pVolDesc->pChkDesc->chkFatMap;
    
    bufToDisk = pVolDesc->pChkDesc->bufToDisk;
    bzero( (char *) pVolDesc->pChkDesc, sizeof( *pVolDesc->pChkDesc ) );
    pVolDesc->pChkDesc->chkFatMap = chkFatMap;
    pVolDesc->pChkDesc->bufToDisk = bufToDisk;

    /* disable FAT mirroring */
    
    pVolDesc->pFatDesc->syncToggle (pVolDesc, FALSE);

    /* zero FAT buffer */
    
    dosChkEntryMark (pFd, MARK_BUF_INIT, 0, 0 );
    
    return OK;
    } /* dosChkInit() */
    
/*******************************************************************************
*
* dosChkDsk - start disk check procedure.
*
* RETURNS: OK or ERROR if device is not a valid DOS device,
*  corrections can not be written to disk
*  or disk read error.
*
* NOMANUAL
*/
STATUS dosChkDsk
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    CHK_STATUS	result;
    size_t	i;
    struct timespec	tv;
    
    do
    	{
    	if( dosChkInit( pFd ) == ERROR )
    	    return ERROR;
    	
    	pFd->pVolDesc->pChkDesc->curPathLev = 1;
    	
    	/* current path is just a device name */
    	
    	strcpy( pFd->pVolDesc->pChkDesc->chkCurPath,
    		pFd->pVolDesc->devHdr.name );
    	
    	result = dosFsChkTree( pFd );
    	}
    while( result == CHK_RESTART );
    
    /* clear current path from screen */

    printf( "\r" );
    for( i = 0; i < strlen( pFd->pVolDesc->pChkDesc->chkCurPath ); i ++ )
    	{
    	printf( "%4c", SPACE );
    	}
    printf( "\r" );
    
    if( result == CHK_ERROR )
    	goto ret;
    
    /*
     * set system clock to be later, than the last time on volume,
     * but only in case it seems a Real-Time Clock chip
     * not present
     */

    if( time( NULL ) < dosChkMinDate )
    	{
    	tv.tv_sec = max (dosChkMinDate, 
			 (max(pFd->pVolDesc->pChkDesc->chkMaxCreatTime,
		 	   max(pFd->pVolDesc->pChkDesc->chkMaxModifTime,
		    	       pFd->pVolDesc->pChkDesc->chkMaxAccTime))));

    	if( tv.tv_sec > time( NULL ) )
    	    {
    	    tv.tv_nsec = time( NULL );

	    /* Let the user know we are adjusting the system time */
	    printf ("dosChkLib : ");
    	    printf("CLOCK_REALTIME is being reset to %s", 
		   ctime( &tv.tv_sec ) );

    	    printf("Value obtained from file system volume "
	  	"descriptor pointer: %p\n", pFd->pVolDesc);

    	    tv.tv_nsec = time( NULL );
    	    printf("The old setting was %s", ctime((time_t *)&tv.tv_nsec ) );

    	    tv.tv_nsec = dosChkMinDate;
    	    printf("Accepted system dates are greater than %s", 
		   ctime((time_t *)&tv.tv_nsec));

    	    tv.tv_nsec = 1;
    	    clock_settime( CLOCK_REALTIME, &tv );
    	    }
    	}

    /* before deal with lost chains, store all corrections to disk */
    
    if(O_RDONLY != cbioModeGet(pFd->pVolDesc->pCbio))
        {
        if( cbioIoctl (pFd->pVolDesc->pCbio, CBIO_CACHE_FLUSH,
    			(void *)(-1) ) == ERROR )
    	    {
    	    result = CHK_ERROR;
    	    goto ret;
    	    }
    	}
    
    /* compose "current path" for attractive messages */
    
    strcpy( pFd->pVolDesc->pChkDesc->chkCurPath,
    	    pFd->pVolDesc->devHdr.name );
    strcat( pFd->pVolDesc->pChkDesc->chkCurPath,
    	    CHK_LOST_CHAINS_MSG );
    
    if( dosChkLostFind( pFd ) == ERROR /* collect lost chains */ ||
    	pFd->pVolDesc->pChkDesc->nErrors > 0 )
    	{
        dosChkLostFree( pFd );	/* free/ count lost chains */
        
    	if( pFd->pVolDesc->chkLevel == DOS_CHK_ONLY )
    	    {
    	    dosChkMsg( pFd, CHK_NOT_REPAIRED, GET_PATH, 0,0,0,0, NULL );
    	    result = CHK_ERROR;
    	    }
    	else if( pFd->pVolDesc->chkLevel >= DOS_CHK_REPAIR )
    	    {
    	    dosChkMsg( pFd, CHK_RESTORE_FREE, GET_PATH, 0,0,0,0, NULL );
    	    }
    	}
    else
    	{
    	dosChkMsg( pFd, CHK_NO_ERRORS, GET_PATH, 0,0,0,0,  NULL );
    	}
    
    /* set a random volume Id, if this initialized to a bad value */
    
    if ((pFd->pVolDesc->chkLevel > DOS_CHK_ONLY) &&
        ((pFd->pVolDesc->volId == 0) || 
         (pFd->pVolDesc->volId == (u_int)(-1))) &&
        (O_RDONLY != cbioModeGet(pFd->pVolDesc->pCbio)))
    	{
    	printf("Change volume Id from %p ", (void *)pFd->pVolDesc->volId );

    	VX_TO_DISK_32( tickGet(), &pFd->pVolDesc->volId );

    	cbioBytesRW (pFd->pVolDesc->pCbio, pFd->pVolDesc->bootSecNum,
    		     pFd->pVolDesc->volIdOff, (void *)&pFd->pVolDesc->volId,
    		     sizeof( pFd->pVolDesc->volId ), CBIO_WRITE, NULL );

    	pFd->pVolDesc->volId = DISK_TO_VX_32( &pFd->pVolDesc->volId );
    	
    	printf("to %p\n", (void *)pFd->pVolDesc->volId );
    	}
    
    dosChkStatPrint( pFd );
    
ret:
    dosChkFinish( pFd );
    
     /* If not write protected enable FAT mirroring and synchronize copies */

    if ((pFd->pVolDesc->chkLevel > DOS_CHK_ONLY) && 
        (O_RDONLY != cbioModeGet(pFd->pVolDesc->pCbio)))
       {
       pFd->pVolDesc->pFatDesc->syncToggle (pFd->pVolDesc, TRUE);
       }

    /*
     * !!! do not set cbioReadyChanged to TRUE here,
     *	   because dosFsLib has to synchronize cache before it
     */

    
    return ( result == CHK_OK )? OK : ERROR;
    } /* dosChkDsk() */
    
/*******************************************************************************
*
* dosChkLibInit - install dos sanity check utility.
*
* RETURNS: N/A.
*
* NOMANUAL
*/
void dosChkLibInit( void )
    {
    dosFsChkRtn = dosChkDsk;
    }

/*******************************************************************************
*
* dosChkEntryMarkSet - mark entry if it is not market yet
*
* RETURNS: if failed to mark the entry
*/

LOCAL STATUS dosChkEntryMarkSet
    (
    DOS_FILE_DESC_ID	pFd,		/* pointer to file descriptor */
    uint32_t		entryNum,	/*  */
    uint32_t		markValue	/*  */
    )
    {
    uint32_t	mark;

   /* Check entry status */

    mark = dosChkEntryMark (pFd, MARK_GET, entryNum, 0);

    if (mark != 0)				/* already marked entry */
        return ERROR;

    /* Mark entry */

    dosChkEntryMark (pFd, MARK_SET, entryNum, markValue);

    return OK;
    } /* dosChkEntryMarkSet */

/*******************************************************************************
*
* dosChkChainMark - mark an entire chain
*
* RETURNS: size of file represented by this chain
*/

LOCAL uint32_t dosChkChainMark
    (
    DOS_FILE_DESC_ID	pFd,		/* pointer to file descriptor */
    uint32_t		startClust,	/* start cluster of chain */
    uint32_t		entNum,		/* valid entry number */
    uint32_t		entCont		/* entry contents */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    DOS_FAT_DESC_ID	pFatDesc = pVolDesc->pFatDesc;
    uint32_t		fileSize;
    uint32_t		clustSize;
    uint32_t		entValue;	/* sense of entry contents */
    uint32_t		nextEntCont;	/* next entry content, if current entry 
					   contents valid cluster number */
    uint32_t		prevEnt;	/*  */
    uint32_t		numClusts;	/* size of chain in clusters */
    uint32_t		eofClust;
    char *		errMsg;

    errMsg   = NULL;
    eofClust = 0;

    numClusts = 1;		/* start cluster */

    if ( (pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) == 0 )
        {
        clustSize = pVolDesc->secPerClust * pVolDesc->bytesPerSec;
        fileSize = (pFd->pFileHdl->size + clustSize - 1) / clustSize;
        }
    else			/* this is a directory */
        fileSize = 0xffffffff;

    nextEntCont = entCont;
    entCont     = entNum;
    entNum      = startClust;
    entValue    = DOS_FAT_ALLOC;

    do  {
        /* Proceed with next entry */

        prevEnt = entNum;	/* previous entry */
        entNum  = entCont;      /* entry to check */
        entCont = nextEntCont;  /* contents of the entry to check */

        if (fileSize == numClusts)
            {	      	/* file size is reached */
            errMsg = CHK_LONG_CHAIN_MSG;
            eofClust = prevEnt;
            break;
            }

        /* Check for cross link and mark entry */

        if (dosChkEntryMarkSet (pFd, entNum, startClust) != OK)
            {      		/* cross-link */
            if (dosChkEntryMark (pFd, MARK_GET, entNum, 0) == LOST_CHAIN_MARK)
                dosChkEntryMark (pFd, MARK_SET, entNum, startClust);
            else
                {
                errMsg = CHK_CROSS_LINK_MSG;
                eofClust = prevEnt;
                }

            break;
            }

        numClusts += 1; 	/* increase number of clusters in chain */
       }  /* while entry content is next entry number */
       while (( entValue = pFatDesc->clustValueGet (
				pFd, pFatDesc->activeCopyNum,
                                entCont, &nextEntCont) ) == DOS_FAT_ALLOC);

    switch (entValue)
        {
        case DOS_FAT_BAD:
            errMsg = CHK_BAD_CHAIN_MSG;	/* ??? */
            eofClust = prevEnt;
            numClusts -= 1;
            break;

        case DOS_FAT_AVAIL:
        case DOS_FAT_INVAL:
        case DOS_FAT_RESERV:
            errMsg = CHK_BAD_CHAIN_MSG;	/* ??? */
            eofClust = entNum;
            break;

        case DOS_FAT_ALLOC:
        case DOS_FAT_EOF:
	default:
	    break;
        }

    if (errMsg != NULL)
        {
        dosChkMsg (pFd, errMsg, CURRENT_PATH, 0, 0, 0, 0, NULL);
        pVolDesc->pChkDesc->nErrors++;
        }

    if ( (eofClust != 0) && (pVolDesc->chkLevel >= DOS_CHK_REPAIR) )
    	    {
            pFatDesc->clustValueSet (pFd, pFatDesc->activeCopyNum,
				     eofClust, DOS_FAT_EOF, 0);
    	    }

    if ( ((pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) == 0) && 
         (fileSize > numClusts) )
        dosChkEntryDel (pFd, TRUE, (u_char *)CHK_SHORT_CHAIN_MSG, 
			CURRENT_PATH, 0, 0, 0, 0);

    return numClusts;
    } /* dosChkChainMark */

/*******************************************************************************
*
* dosChkChainVerify - verify a cluster chain
*
* RETURNS: special status representing the state of this chain
*/

LOCAL CHK_STATUS dosChkChainVerify
    (
    DOS_FILE_DESC_ID	pFd		/* pointer to file descriptor */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    DOS_FAT_DESC_ID	pFatDesc = pVolDesc->pFatDesc;
    DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
    uint32_t		parDirStartClust;
    uint32_t		startClust;
    uint32_t		nextEnt;
    uint32_t		nextEntCont = 0;
    uint32_t		entValue;	/* sense of entry contents */

    parDirStartClust = pFd->pFileHdl->dirHdl.parDirStartCluster;

    startClust = pFileHdl->startClust;

    /* Check start cluster pointer */

    entValue = pFatDesc->clustValueGet (pFd, pFatDesc->activeCopyNum,
					startClust, &nextEnt);
    if (entValue != DOS_FAT_ALLOC)
        {
        if ( (entValue == DOS_FAT_AVAIL) &&	/* start cluster is 0 */
             ((pFileHdl->attrib & DOS_ATTR_DIRECTORY) == 0) &&	/* a file */
             (pFileHdl->size == 0) )			/* size is 0 */
            return CHK_OK;

        return dosChkEntryDel (pFd, FALSE, (u_char *)CHK_BAD_START_MSG, 
			       CURRENT_PATH, 0, 0, 0, 0);
        }

    /* Check start cluster contents */

    entValue = pFatDesc->clustValueGet (pFd, pFatDesc->activeCopyNum,
                                        nextEnt, &nextEntCont);
    if ( (entValue & (DOS_FAT_ALLOC | DOS_FAT_EOF)) == 0 )
        {
        if (entValue == DOS_FAT_BAD)
            return dosChkEntryDel (pFd, FALSE, (u_char *)CHK_BAD_START_MSG, 
				   CURRENT_PATH, 0, 0, 0, 0);	/* ??? */
        else
            {
            /* ??? */
            dosChkMsg (pFd, CHK_BAD_START_MSG, CURRENT_PATH, 0, 0, 0, 0, NULL);
            pVolDesc->pChkDesc->nErrors++;

            if (pVolDesc->chkLevel >= DOS_CHK_REPAIR)
    	    	{
                pFatDesc->clustValueSet (pFd, pFatDesc->activeCopyNum,
                                         startClust, DOS_FAT_EOF, 0);
    	    	}
            }
        }

    if (entValue != DOS_FAT_ALLOC)
        {
        /* File consists of 1 start cluster only */

        if ( ((pFileHdl->attrib & DOS_ATTR_DIRECTORY) == 0) &&
             (pFileHdl->size > pVolDesc->secPerClust * pVolDesc->bytesPerSec) )
            return dosChkEntryDel (pFd, FALSE, (u_char *)CHK_SHORT_CHAIN_MSG, 
                                   CURRENT_PATH, 0, 0, 0, 0);
        }

    /* Mark start cluster as file's chain start */

    if (dosChkEntryMarkSet (pFd, startClust,
			    parDirStartClust | START_CLUST_MASK) 
	!= OK)
        {
        if ( (dosChkEntryMark (pFd, MARK_GET, startClust, 0) &
	      START_CLUST_MASK) == 0)
            return dosChkEntryDel (pFd, FALSE, (u_char *)CHK_CROSS_LINK_MSG, 
				   CURRENT_PATH, 0, 0, 0, 0);	/* ??? */

        dosChkStartCrossProc (pFd);

        if (pFileHdl->deleted)
            return CHK_OK;

        dosChkEntryMark (pFd, MARK_SET, startClust,
    			 parDirStartClust | START_CLUST_MASK);
        }

    if (entValue == DOS_FAT_ALLOC)
        if (dosChkChainMark (pFd, startClust, nextEnt, nextEntCont) == 
		(uint32_t)CHK_ERROR)
        return CHK_ERROR;

    return CHK_OK;
    } /* dosChkChainVerify */

/*******************************************************************************
*
* dosChkChainStartGet - find where is beginning of a chain
*
* RETURNS: OK
*/

LOCAL STATUS dosChkChainStartGet
    (
    DOS_FILE_DESC_ID	pFd,			/* pointer to file descriptor */
    uint32_t		entNum,			/* FAT entry number */
    uint32_t *		pParDirStartClust,	/* where to return start cluster
						number of parent directory */
    uint32_t *		pStartClust		/* where to return start cluster
						number of chain */
    )
    {
    uint32_t mark;

    mark = dosChkEntryMark (pFd, MARK_GET, entNum, 0);

    if ((mark & START_CLUST_MASK) == 0)		/* not start cluster */
        {					/* go to start cluster */
        entNum = mark;
        mark   = dosChkEntryMark (pFd, MARK_GET, entNum, 0);
        }

    *pStartClust       = entNum;

    *pParDirStartClust = mark & (~START_CLUST_MASK);

    return OK;
    } /* dosChkChainStartGet */

/*******************************************************************************
*
* dosChkChainUnmark - Un-mark an entire chain
*
* RETURNS:
*/

LOCAL void dosChkChainUnmark
    (
    DOS_FILE_DESC_ID	pFd		/* pointer to file descriptor */
    )
    {
    DOS_FAT_DESC_ID	pFatDesc = pFd->pVolDesc->pFatDesc;
    uint32_t		entNum;
    uint32_t		nextEnt;

    entNum = pFd->pFileHdl->startClust; 

    while ( (pFatDesc->clustValueGet (pFd, pFatDesc->activeCopyNum,
                                      entNum, &nextEnt) == DOS_FAT_ALLOC) &&
            (dosChkEntryMark (pFd, MARK_GET, entNum, 0) != 0) )
        {
        dosChkEntryMark (pFd, MARK_SET, entNum, 0);
        entNum = nextEnt;
        }
    } /* dosChkChainUnmark */

/*******************************************************************************
*
* dosChkLostFind - find lost chains in the FAT
*
* RETURNS:OK or ERROR if any of the chains contained an illegal value
*/

LOCAL STATUS dosChkLostFind
    (
    DOS_FILE_DESC_ID	pFd		/* pointer to file descriptor */
    )
    {
    DOS_VOLUME_DESC_ID 	pVolDesc = pFd->pVolDesc;
    DOS_FAT_DESC_ID	pFatDesc = pVolDesc->pFatDesc;
    CHK_DSK_DESC_ID	pChkDesc = pVolDesc->pChkDesc;
    uint32_t		entNum;
    uint32_t		entCont;
    uint32_t		nextEntCont;
    uint32_t		entValue;
    uint32_t		retVal = OK;
    uint32_t		lostClusts = 0;
    cookie_t		bufCookie = (cookie_t) NULL, fatCookie = (cookie_t) NULL;

    for (entNum = DOS_MIN_CLUST; entNum < pFd->pVolDesc->nFatEnts; entNum++)
        {
        if (dosChkEntryMark (pFd, IS_BUSY, entNum, 0) == 0)
            {
	    /* Get content of the entry */

    	    bufCookie = pFd->fatHdl.cbioCookie;
    	    pFd->fatHdl.cbioCookie = fatCookie;
            pFatDesc->clustValueGet (pFd, pFatDesc->activeCopyNum,
                                     entNum, &entCont);

            /* Value the entry content */

            entValue = pFatDesc->clustValueGet (pFd, pFatDesc->activeCopyNum,
                                                entCont, &nextEntCont);
    	    fatCookie = pFd->fatHdl.cbioCookie;
    	    pFd->fatHdl.cbioCookie = bufCookie;

            switch (entValue)
                {
                case DOS_FAT_ALLOC:
                case DOS_FAT_EOF:
                case DOS_FAT_INVAL:
                case DOS_FAT_RESERV:
                    pChkDesc->chkNLostChains++;/* Count number of lost chains */
                    pChkDesc->nErrors++;	/* Count number of errors */
                    retVal = ERROR;
                    dosChkEntryMark (pFd, MARK_SET, entNum, LOST_CHAIN_MARK);

                    if (entValue == DOS_FAT_ALLOC)
                        lostClusts += dosChkChainMark (pFd, entNum, entCont, 
                                                       nextEntCont);
                    else
                        lostClusts += 1;

                    break;

                case DOS_FAT_BAD:
                    pChkDesc->chkNBadClusts++;
                    break;

                case DOS_FAT_AVAIL:
                    pChkDesc->chkNFreeClusts++;
                    break;

                } /* switch */
            }
        } /* for */

    pChkDesc->chkTotalBytesInLostChains = lostClusts * pVolDesc->secPerClust *
                                          pVolDesc->bytesPerSec;;

    return retVal;
    } /* dosChkLostFind */

/*******************************************************************************
*
* dosChkLostFree - free all lost chains in the FAT
*
* RETURNS: OK.
*/

LOCAL STATUS dosChkLostFree
    (
    DOS_FILE_DESC_ID	pFd		/* pointer to file descriptor */
    )
    {
    DOS_VOLUME_DESC_ID 	pVolDesc = pFd->pVolDesc;
    DOS_FAT_DESC_ID	pFatDesc = pVolDesc->pFatDesc;
    uint32_t		entNum;
    uint32_t		nextEnt;
    uint32_t		nextEntCont;
    uint32_t		entVal;
    uint32_t		i;

    if (pVolDesc->chkLevel < DOS_CHK_REPAIR)
        return OK;

    for (i = DOS_MIN_CLUST; i < pFd->pVolDesc->nFatEnts; i++)
        {
        entNum = i;

        if (dosChkEntryMark (pFd, MARK_GET, entNum, 0) == LOST_CHAIN_MARK)
            {
            /* Follow lost chain and free clusters */

            pFatDesc->clustValueGet (pFd, pFatDesc->activeCopyNum,
                                     entNum, &nextEnt);

            /* While entry content is next cluster number */

            while ( (entVal = pFatDesc->clustValueGet (
				pFd, pFatDesc->activeCopyNum,
                                nextEnt, &nextEntCont)) == DOS_FAT_ALLOC )
                {
                pFatDesc->clustValueSet (pFd, pFatDesc->activeCopyNum,
                                         entNum, DOS_FAT_AVAIL, 0);

                entNum  = nextEnt;
                nextEnt = nextEntCont;
                }

            /* For end of file */

            pFatDesc->clustValueSet (pFd, pFatDesc->activeCopyNum,
                                     entNum, DOS_FAT_AVAIL, 0);
            }
        } /* for */

    return OK;
    } /* dosChkLostFree */

/* End of File */

