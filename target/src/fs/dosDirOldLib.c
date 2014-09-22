/* dosDirOldLib.c - MS-DOS old style names dir handler */ 

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,29apr02,jkf  SPR#72255, upon file creation, set the create, modification,
                 and access time fields to match creation rather than using
                 the epoch (0).
01i,10dec01,jkf  SPR#72039, various fixes from Mr. T. Johnson.
01h,21aug01,jkf  SPR#69031, common code for both AE & 5.x.
01g,06mar01,jkf  SPR#34704, rootNSec calculation must round up.
01f,29feb00,jkf  T3 merge
01e,31jul99,jkf  T2 merge, tidiness & spelling.
01d,22nov98,vld  all offsets of directory entry changed to be
		 counted in absolute offset from parent directory
		 start
01c,28sep98,vld  gnu extensions dropped from DBG_MSG/ERR_MSG macros
01b,02jul98,lrn  doc review
01a,18jan98,vld	 written
*/

/*
DESCRIPTION
This library is part of dosFsLib and  is designed to handle
the DOS4.0 standard disk directory structure as well as
vxWorks proprietary long names in case sensitive manner.

File names in DOS4.0 are composed of a 8 characters name and
3 characters extension separated by dot character.
They are stored on disk in uppercase mode, and name lookup
is case insensitive. New VFAT file name standard also
supports DOS4.0 directory structure, and if both handlers
(this one and dosVDirLib) are installed, VFAT handler take precedence,
and will be used for such a volume by default.

VxWorks proprietary long names are composed of up to 40 characters
and are fully case sensitive. Also, files with length greater, than
4GB can be stored on the volume is formatted with VxWorks proprietary
long names directory structure.

Routine dosDirOldLibInit() has to be called once to install this
into dosFsLib directory handlers list. Once it has been done this directory
handler will be automatically mounted on each new appropriate volume
being mounted.

SEE ALSO:
dosFsLib.
*/

/* includes */
#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stat.h"
#include "dirent.h"
#include "time.h"
#include "stdio.h"
#include "ctype.h"
#include "taskLib.h"
#include "stdlib.h"
#include "string.h"
#include "semLib.h"
#include "logLib.h"
#include "errnoLib.h"

#include "time.h"
#include "utime.h"
#include "memLib.h"

#include "private/dosFsLibP.h"
#include "private/dosDirLibP.h"

/* defines */

/* macros */

#undef DBG_PRN_STR
#undef DBG_MSG
#undef ERR_MSG
#undef NDEBUG
 
#ifdef DEBUG
#   undef LOCAL
#   define LOCAL
#   undef ERR_SET_SELF
#   define ERR_SET_SELF

#   define DBG_PRN_STR( lvl, fmt, pStr, len, arg )		\
	{ if( (lvl) <= dosDirOldDebug )		{		\
	    char cBuf = ((char *)(pStr))[len];			\
	    (pStr)[len] = EOS; 					\
	    printErr( (fmt),(pStr),(arg) );			\
	    ((char *)(pStr))[len] = cBuf; 	} }

#   define DBG_MSG( lvl, fmt, arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8 )	\
	{ if( (lvl) <= dosDirOldDebug )					\
	    printErr( "%s : %d : " fmt,				\
	               __FILE__, __LINE__, arg1,arg2,	\
		       arg3,arg4,arg5,arg6,arg7,arg8 ); }

#   define ERR_MSG( lvl, fmt, a1,a2,a3,a4,a5,a6 )		\
        { logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }
#else	/* NO DEBUG */

#   define NDEBUG
#   define DBG_PRN_STR( lvl, fmt, pStr, len, arg )	{}
#   define DBG_MSG(lvl,fmt,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) 	{}

#   define ERR_MSG( lvl, fmt, a1,a2,a3,a4,a5,a6 )		\
	{ if( (lvl) <= dosDirOldDebug ) 				\
            logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }

#endif /* DEBUG */
 
#include "assert.h"

#ifdef ERR_SET_SELF
#   define errnoSet( err ) errnoSetOut( __FILE__, __LINE__, #err, (err) )
#endif /* ERR_SET_SELF */

/* vxWorks long names specific  */

/* typedefs */

typedef enum {RD_FIRST, RD_CURRENT, RD_NEXT, FD_ENTRY} RDE_OPTION;
				/* function argument */

typedef struct DOS_DIROLD_DESC	/* directory handler's part of */
				/* volume descriptor */
    {
    DOS_DIR_PDESCR	genDirDesc;	/* generic descriptor */
    } DOS_DIROLD_DESC;
typedef DOS_DIROLD_DESC *	DOS_DIROLD_DESC_ID;


/* globals */

int	dosDirOldDebug = 1;

/* locals */

/* valid filename characters table ('|' is invalid character) */

static const u_char	shortNamesChar[] =
                                        /* 0123456789abcdef */
                                          "||||||||||||||||"
                                          "||||||||||||||||"
                                          " !|#$%&'()|||-||"
                                          "0123456789||||||"
                                          "@ABCDEFGHIJKLMNO"
                                          "PQRSTUVWXYZ|||^_"
                                          "`ABCDEFGHIJKLMNO"
                                          "PQRSTUVWXYZ{|}~|" ,
                        longNamesChar[] =
                                        /* 0123456789abcdef */
                                          "||||||||||||||||"
                                          "||||||||||||||||"
                                          " !|#$%&'()|+,-.|"
                                          "0123456789|;|=||"
                                          "@ABCDEFGHIJKLMNO"
                                          "PQRSTUVWXYZ[|]^_"
                                          "`abcdefghijklmno"
                                          "pqrstuvwxyz{|}~|" ;

/* forward declarations */

#ifdef ERR_SET_SELF
/*******************************************************************************
* errnoSetOut - put error message
*
* This routine is called instead errnoSet during debugging.
*/
static VOID errnoSetOut(char * pFile, int line, const char * pStr, int errCode )
    {
    if( errCode != OK  && strcmp( str, "errnoBuf" ) != 0 )
        printf( " %s : line %d : %s = %p, task %p\n",
                pFile, line, pStr, (void *)errCode,
                (void *)taskIdSelf() );
    errno = errCode;
    }
#endif  /* ERR_SET_SELF */

 /***************************************************************************
*
* dosDirOldFillFd - fill file descriptor for directory entry.
*
* RETURNS: N/A.
*/
LOCAL void dosDirOldFillFd
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    u_char *	pDirEnt		/* directory entry from disk */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    DOS_DIR_HDL_ID	pDirHdl = (void *)&(pFd->pFileHdl->dirHdl);
    DIRENT_DESCR_ID	pDeDesc;
    
    pDeDesc = &pDirDesc->deDesc;
    
    if( pDirEnt == (u_char *) NULL )	/* root directory */
    	{
    	DBG_MSG( 600, "root directory\n", 0,0,0,0,0,0,0,0 );
    	ROOT_SET( pFd );	/* via <parDirStartCluster> field */

    	pFd->curSec = pDirDesc->dirDesc.rootStartSec;
    	pFd->nSec = pDirDesc->dirDesc.rootNSec;
    	pFd->pos = 0;

    	pFd->pFileHdl->attrib = DOS_ATTR_DIRECTORY;
    	pFd->pFileHdl->startClust = pDirDesc->rootStartClust;
    	
    	pDirHdl->sector = NONE;
    	pDirHdl->offset = NONE;
    	
    	pFd->cbioCookie = (cookie_t) NULL;
    	pDirHdl->cookie = (cookie_t) NULL;
    	
    	goto rewind;
    	}
    
    /* at the beginning fill directory handle using file descriptor */
    
    pDirHdl->parDirStartCluster = pFd->pFileHdl->startClust;
    pDirHdl->sector = pFd->curSec;
    pDirHdl->offset = pFd->pos;

    pDirHdl->cookie = pFd->cbioCookie;
    pFd->cbioCookie = (cookie_t) NULL;

    /* disassemble directory entry */
    
    pFd->pos = 0;
    
    pFd->curSec = 0;
    pFd->nSec = 0;
    
    pFd->pFileHdl->attrib = *(pDirEnt + pDeDesc->atrribOff);
    pFd->pFileHdl->startClust =
    		START_CLUST_DECODE( pFd->pVolDesc, pDeDesc, pDirEnt );
    
    pFd->pFileHdl->size = DISK_TO_VX_32(pDirEnt + pDeDesc->sizeOff);
    
    if( pDeDesc->extSizeOff != (u_char) NONE )
    	{
    	pFd->pFileHdl->size += EXT_SIZE_DECODE( pDeDesc, pDirEnt );
    	}
        
rewind:
    DBG_MSG( 600, "pFd = %p "
    		  "dir hdl (sec-of-par = %u sector = %u offset = %u)\n"
                  "pFileHdl = %p "
                  "(startClust = %u size = %lu attr = %p)\n",
                  pFd, pDirHdl->parDirStartCluster,
                  pDirHdl->sector, pDirHdl->offset,
                  pFd->pFileHdl, pFd->pFileHdl->startClust,
                  pFd->pFileHdl->size,
                  (void *)((int)pFd->pFileHdl->attrib) );
    /*
     * cause FAT to start from start cluster and
     * get start contiguous block
     */
    bzero( (char *)&pFd->fatHdl, sizeof( pFd->fatHdl ) );
    return;
    } /* dosDirOldFillFd() */

/***************************************************************************
*
* dosDirOldRewindDir - rewind directory pointed by pFd.
*
* This routine sets current sector in pFd to directory start sector
* and current position  (offset in sector) to 0.
*
* RETURNS: N/A.
*/
LOCAL void dosDirOldRewindDir
    (
    DOS_FILE_DESC_ID	pFd	/* dos file descriptor to fill */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    
    pFd->pos = 0;
    pFd->nSec = 0;
    pFd->curSec = 0;
    
    /* for FAT32 curSec = rootNSec = 0 */
    
    if( IS_ROOT( pFd ) && pDirDesc->dirDesc.rootNSec != 0 )
    	{
    	pFd->curSec = pDirDesc->dirDesc.rootStartSec;
    	pFd->nSec = pDirDesc->dirDesc.rootNSec;
    	return;
    	}
    
    /* regular file or FAT32 root */

    /*
     * cause FAT to start from start cluster and
     * get start contiguous block
     */
    if( pFd->pVolDesc->pFatDesc->seek( pFd, FH_FILE_START, 0 ) == ERROR )
    	{
    	assert( FALSE );
    	}
    } /* dosDirOldRewindDir() */
   
/***************************************************************************
*
* dosDirOldPathParse - parse a full pathname into an array of names.
*
* This routine is similar to pathParse(), except it does not 
* allocate additional buffers nor changes the path string.
*
* Parses a path in directory tree which has directory names
* separated by '/' or '\\'s.  It fills the supplied array of
* structures with pointers to directory and file names and 
* corresponding name length.
*
* All occurrences of '//', '.' and '..'
* are right removed from path. All tail dots and spaces are broken from
* each name, that is name like "abc. . ." is treated as just "abc".
*
* For instance, "/usr/vw/data/../dir/file" gets parsed into
*
* .CS
*                          namePtrArray
*                         |---------|
*   ---------------------------o    |
*   |                     |    3    |
*   |                     |---------|
*   |   -----------------------o    |
*   |   |                 |    2    |
*   |   |                 |---------|
*   |   |          ------------o    |
*   |   |          |      |    3    |
*   |   |          |      |---------|
*   |   |          |   --------o    |
*   |   |          |   |  |    4    |
*   |   |          |   |  |---------|
*   v   v          v   v  |   NULL  |
*   |   |          |   |  |    0    |
*   |   |          |   |  |---------|
*   v   v          v   v 
*  |------------------------|
*  |usr/vw/data/../dir/file |
*  |-------\-----/----------|
*          ignored
*
* .CE
*
* RETURNS: number of levels in path.
*
* ERRNO:
* S_dosFsLib_ILLEGAL_PATH
* S_dosFsLib_ILLEGAL_NAME
*/
LOCAL int dosDirOldPathParse
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    u_char *	path,
    PATH_ARRAY_ID pnamePtrArray
    )
    {
    FAST u_int	numPathLevels;
    FAST u_char *	pName;
    FAST u_char *	pRegChar; /* last not DOT and not SPACE char */
    
    pnamePtrArray[0].pName = NULL;

    /* go throw path string from left to right */
    
    pName = path;
    numPathLevels = 0;
    while( *pName != EOS )	/* there is 'break' in loop also */
    	{
    	/* pass slashes */
    	
    	if( *pName == SLASH || *pName == BACK_SLASH )
    	    {
    	    pName++;
    	    continue;
    	    }
    	
    	/* process special names ( "." ".." ) */
    	
    	if( *pName == DOT )
    	   {
    	   /* "/./" - ignore "current directory" */
    	   if( *(pName + 1) == EOS || *(pName + 1) == SLASH ||
    	       *(pName + 1) == BACK_SLASH )
    	   	{
    	   	pName ++;
    	   	continue;
    	   	}
    	   
    	   /* "/../" - goto one level back */
    	   
    	   if( (*(pName + 1) == DOT) &&
    	       ( *(pName + 2) == EOS || *(pName + 2) == SLASH ||
    	         *(pName + 2) == BACK_SLASH ) )
    	   	{
    	   	if( numPathLevels > 0 )
    	   	    numPathLevels --;
    	   	pName +=2;
    	   	continue;
    	   	}
    	   } /* if( *pName == DOT ) */
    	   
    	/* regular name: insert it into array */
    	
    	if( numPathLevels >= DOS_MAX_DIR_LEVELS )
    	    break;	/* max level overloaded */
    	
    	pnamePtrArray[numPathLevels].pName = pName;

    	pnamePtrArray[numPathLevels + 1].pName = NULL;

    	pRegChar = NULL;

    	while( *pName != SLASH && *pName != BACK_SLASH && *pName != EOS )
    	    {
    	    if( *pName != DOT || *pName == SPACE )
    	    	pRegChar = pName;
    	    pName++;
    	    }
    	
    	/* name can not contain only dots */
    	
    	if( pRegChar == NULL )
    	    {
    	    errnoSet( S_dosFsLib_ILLEGAL_NAME );
    	    return ERROR;
    	    }
    	
    	pnamePtrArray[numPathLevels].nameLen =
    		pRegChar + 1 - pnamePtrArray[numPathLevels].pName;
    	
    	numPathLevels++;
    	} /* while( *pName != EOS ) */
    
    /* check result */
    
    if( *pName != EOS )	/* path termination has not been reached */
    	{
    	errnoSet( S_dosFsLib_ILLEGAL_PATH );
    	return ERROR;
    	}

#ifdef DEBUG
    DBG_MSG( 600, "path: %s, result: \n", path ,0,0,0,0,0,0,0);
    pName = (void *)pnamePtrArray;
    for( ; pnamePtrArray->pName != NULL; pnamePtrArray++ )
    	{
    	int i = pnamePtrArray - (PATH_ARRAY_ID)pName + 1;
    	DBG_PRN_STR( 600, "%s : %d", pnamePtrArray->pName,
    			pnamePtrArray->nameLen, i );
    	}
    DBG_PRN_STR(600, "\b\b \n", "", 0, 0);
#endif /* DEBUG */

    return numPathLevels;
    } /* dosDirOldPathParse() */
    
/***************************************************************************
*
* dosDirOldTDDecode - decode time-date field from disk format.
*
* This routine decodes required date-time field in the directory
* entry into time_t format.
* 
* Parameter which defines which time field to decode. It
* can be one of:
* DH_TIME_CREAT, DH_TIME_MODIFY, DH_TIME_ACCESS.
*
* RETURNS: time in time_t format.
*
*/
LOCAL time_t dosDirOldTDDecode
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    u_char *	pDirent,	/* directory entry buffer */
    u_int	which	/* what field to decode: one of */
    			/* DH_TIME_CREAT, DH_TIME_MODIFY or DH_TIME_ACCESS */
    )
    {
    struct tm	tm = {0};	/* broken down time buffer */
    UINT16	dtBuf;		/* 16-bit buffer */
    u_char	tOff = 0, dOff = 0;	/* field offset */
    
    switch( which )
        {
        case DH_TIME_CREAT:
            tOff = pDirDesc->deDesc.creatTimeOff;
            dOff = pDirDesc->deDesc.creatDateOff;
            break;
        case DH_TIME_MODIFY:
            tOff = pDirDesc->deDesc.modifTimeOff;
            dOff = pDirDesc->deDesc.modifDateOff;
            break;
        case DH_TIME_ACCESS:
            tOff = pDirDesc->deDesc.accessTimeOff;
            dOff = pDirDesc->deDesc.accessDateOff;
            break;
        default:
            assert( FALSE );
        }
    
    if( dOff != (u_char)NONE )
    	{
    	dtBuf = DISK_TO_VX_16( pDirent + dOff );
    	
    	tm.tm_mday    = dtBuf & 0x1f;
    	
	/* ANSI months are zero-based */
	
    	tm.tm_mon     = ((dtBuf >> 5) & 0x0f) - 1;
    	
	/* DOS starts at 1980, ANSI starts at 1900 */
	
    	tm.tm_year    = ((dtBuf >> 9) & 0x7f) + 1980 - 1900;
    	}
    if( tOff != (u_char)NONE )
    	{
    	dtBuf = DISK_TO_VX_16( pDirent + tOff );
    	
    	tm.tm_sec     = (dtBuf & 0x1f) << 1;
    	tm.tm_min     = (dtBuf >> 5) & 0x3f;
    	tm.tm_hour    = (dtBuf >> 11) & 0x1f;
    	}
    
    /* encode into time_t format */
    
    return mktime( &tm );
    } /* dosDirOldTDDecode() */

/***************************************************************************
*
* dosDirOldTDEncode - encode time-date to disk format.
*
* This routine takes time value is provided in <curTime> argument
* and encodes it into directory entry format.
* 
* Parameter <timesMask> defines which time fields to fill. Following
* values can be bitwise or-ed:
* DH_TIME_CREAT, DH_TIME_MODIFY, DH_TIME_ACCESS.
*
* RETURNS: N/A.
*/
LOCAL void dosDirOldTDEncode
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    u_char *	pDirEnt,
    u_int	timesMask,
    time_t	curTime	/* time to encode */
    )
    {
    struct tm   tm;	/* buffer for split time-date */
    u_char	timeB[2], dateB[2];	/* buffers for encoding */
    
    localtime_r( &curTime, &tm );

    /* encode time */
    
    VX_TO_DISK_16( ( tm.tm_sec >> 1 ) |
    		  ( tm.tm_min << 5 ) |
    		  ( tm.tm_hour << 11 ), timeB );
    
    /*
     * encode date;
     * in <pDate> year is related to 1980, but in tm,-to 1900
     */
    tm.tm_year =  ( tm.tm_year < 80 )? 80 : tm.tm_year;
    VX_TO_DISK_16( tm.tm_mday | ( ( tm.tm_mon + 1 ) << 5 ) |
    		  ( ( tm.tm_year - 80 ) <<  9 ), dateB );
    
    /* put time-date into directory entry buffer */
    
    if( timesMask & DH_TIME_CREAT &&
        pDirDesc->deDesc.creatDateOff != (u_char)NONE  )
    	{
    	pDirEnt[ pDirDesc->deDesc.creatTimeOff ] = timeB[0];
    	pDirEnt[ pDirDesc->deDesc.creatTimeOff + 1 ] = timeB[1];
    	pDirEnt[ pDirDesc->deDesc.creatDateOff ] = dateB[0];
    	pDirEnt[ pDirDesc->deDesc.creatDateOff + 1 ] = dateB[1];
    	}
    if( timesMask & DH_TIME_MODIFY &&
        pDirDesc->deDesc.modifDateOff != (u_char)NONE  )
    	{
    	pDirEnt[ pDirDesc->deDesc.modifTimeOff ] = timeB[0];
    	pDirEnt[ pDirDesc->deDesc.modifTimeOff + 1 ] = timeB[1];
    	pDirEnt[ pDirDesc->deDesc.modifDateOff ] = dateB[0];
    	pDirEnt[ pDirDesc->deDesc.modifDateOff + 1 ] = dateB[1];
    	}
    if( timesMask & DH_TIME_ACCESS &&
        pDirDesc->deDesc.accessDateOff != (u_char)NONE  )
    	{
    	if( pDirDesc->deDesc.accessTimeOff != (u_char)NONE )
    	    {
    	    pDirEnt[ pDirDesc->deDesc.accessTimeOff ] = timeB[0];
    	    pDirEnt[ pDirDesc->deDesc.accessTimeOff + 1 ] = timeB[1];
    	    }
    	pDirEnt[ pDirDesc->deDesc.accessDateOff ] = dateB[0];
    	pDirEnt[ pDirDesc->deDesc.accessDateOff + 1 ] = dateB[1];
    	}
    } /* dosDirOldTDEncode() */
    
/***************************************************************************
*
* dosDirOldNameEncode - encode name to disk format.
*
* This routine encodes incoming file name to current volume
* directory structure format
* (8+3 uppercase or VxLong names). in the same time incoming
* name is verified to be composed of valid characters.
*
* RETURNS: OK or ERROR if name is invalid.
* 
* ERRNO:
* S_dosFsLib_ILLEGAL_NAME
*/
LOCAL STATUS dosDirOldNameEncode
    ( 
    DOS_DIR_PDESCR_ID	pDirDesc,
    PATH_ARRAY_ID	pNamePtr,	/* name buffer */
    u_char *	pDstName	/* buffer for name in disk format */
    )
    {
    u_char const *	nameTransTbl;	/* name translation table */
    u_char *	pSrc;	/* source name dynamic ptr */
    u_char *	pDstBuf;	/* for debug output only */
    int		i,j;		/* work */
    u_char	extLen = pDirDesc->deDesc.extLen;
    			/* extension length (0 - no extension) */
    
    pDstBuf = pDstName;
    nameTransTbl = ( pDirDesc->nameStyle == STDDOS )?
    			shortNamesChar : longNamesChar;
    
    bfill( (char *) pDstName,
    	   pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen, 
           SPACE );
    
    /* encode name and check it by the way */
    
    DBG_MSG( 600, "",0 ,0,0,0,0,0,0,0);
    DBG_PRN_STR( 600, "%s\n", pNamePtr->pName, pNamePtr->nameLen, 0 );
    for( i = 0, pSrc = pNamePtr->pName;
    	 i < min( pDirDesc->deDesc.nameLen,
    	 	  pNamePtr->nameLen );
    	 i++, pDstName++, pSrc++  )
    	{
    	/* check for extension */
    	
    	if( extLen != 0 && *pSrc == DOT )
    	    break;
    	
    	/* allow all high characters */
    	
    	if( *pSrc & 0x80 )
    	    {
    	    *pDstName = *pSrc;
    	    continue;
    	    }
    	
    	*pDstName = nameTransTbl[ *pSrc ];
    	if( *pDstName == INVALID_CHAR )
    	    goto error;
    	}

    /* check state */
    
    if( i == pNamePtr->nameLen )
    	goto retOK;	/* name finished */
    
    if( extLen != 0 )	/* traditional DOS: 8.3 */
    	{
    	if( *pSrc != DOT )	/* name too long */
    	    goto error;
    	pSrc++;	/* pass DOT */
    	pDstName += pDirDesc->deDesc.nameLen - i;
    	i++;
    	}
    else	/* vxWorks old long names */
    	goto error;	/* name too long */
    
    /* encode extension */
    
    for( j = 0; j < extLen && i < pNamePtr->nameLen;
    	 i++, j++, pDstName++, pSrc++  )
    	{
    	/* allow all high characters */
    	
    	if( *pSrc & 0x80 )
    	    {
    	    *pDstName = *pSrc;
    	    continue;
    	    }
    	
    	*pDstName = nameTransTbl[ *pSrc ];
    	if( *pDstName == INVALID_CHAR )
    	    goto error;
    	}
    
    /* check status */
    
    if( i < pNamePtr->nameLen )	/* extension too long */
    	goto error;

retOK:
    DBG_PRN_STR( 600, "result: %s\n", pDstBuf, 
    		 pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,0 );
    return OK;
    
error:
    errnoSet( S_dosFsLib_ILLEGAL_NAME );
    return ERROR;
    } /* dosDirOldNameEncode() */
    
/***************************************************************************
*
* dosDirOldDirentGet - get directory entry from disk.
*
* This routine reads directory entry from disk. 
*
* <which> argument defines which entry to get.
* This routine can be
* used for readdir (<which> = RD_FIRST/RD_CURRENT/RD_NEXT),
* in what case <pFd> describes the
* directory is being read, or for getting directory entry
* corresponding to <pFd> (<which> = FD_ENTRY).
*
* RETURNS: OK or ERROR if directory chain end reached or
*  disk access error.
*/
LOCAL STATUS dosDirOldDirentGet
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    u_char *	pDirEnt,
    RDE_OPTION	 which		/* which entry to get */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    DOS_DIR_HDL_ID	pDirHdl = (void *)&(pFd->pFileHdl->dirHdl);
    int			dirEntSize = pDirDesc->deDesc.dirEntSize;
    DOS_FILE_DESC	workFd;
    
    /* prepare to operation */
    
    if( which == FD_ENTRY ) /* get directory entry of file */
    	{
    	DBG_MSG( 600, "pFd = %p, FD_ENTRY\n", pFd ,0,0,0,0,0,0,0);
    	/*
    	 * it is being read directory entry of the file/dir,
    	 * pointed by file descriptor.
    	 * Prepare working fd.
    	 */
    	workFd = *pFd;
    	workFd.curSec = pDirHdl->sector;
    	workFd.pos = pDirHdl->offset;
    	workFd.cbioCookie = pDirHdl->cookie;
    	
    	goto getEntry;
    	}
    
    /* readdir */
    
    if(  which == RD_CURRENT )	/* read current dir entry */
    	{
    	assert( pFd->nSec != 0 );
    	assert( pFd->curSec != 0 );
    	
    	goto getEntry;
    	}
    
    if( which == RD_FIRST )	/* read start dir entry */
    	{
    	DBG_MSG( 600, "pFd = %p, RD_FIRST\n", pFd ,0,0,0,0,0,0,0);
    	dosDirOldRewindDir( pFd );	/* rewind directory */
    	}
    else if( which == RD_NEXT )	/* read next dir entry */
    	{
    	DBG_MSG( 600, "pFd = %p, RD_NEXT\n", pFd ,0,0,0,0,0,0,0);
    	assert( pFd->curSec != 0 );
    	
    	/* correct position */
    	
    	pFd->pos += dirEntSize;
    	
    	/* check for sector bounds */
    	
    	if( OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ) == 0 )
    	    {
    	    pFd->curSec++;
    	    pFd->nSec--;
    	    }
    	}
    else	/* impossible flag */
    	{
    	assert( which != which );
    	} 
    	
    /* may be contiguous block finished - get next contiguous block */
    	    
    if( pFd->nSec == 0 )
    	{
    	if( pFd->pVolDesc->pFatDesc->getNext(
    	    			pFd, FAT_NOT_ALLOC ) == ERROR )
    	    {
    	    return ERROR;
    	    }
    	pFd->cbioCookie = (cookie_t) NULL;	/* we jumped to other sector */
    	}
    
    workFd = *pFd;
    
getEntry:

    assert( OFFSET_IN_SEC( workFd.pVolDesc, workFd.pos ) <=
		workFd.pVolDesc->bytesPerSec - dirEntSize );
    	
    /* read directory entry */
    
    if( cbioBytesRW( workFd.pVolDesc->pCbio, workFd.curSec,
    		     OFFSET_IN_SEC( workFd.pVolDesc, workFd.pos),
		     (addr_t)pDirEnt, dirEntSize, CBIO_READ,
    		     &workFd.cbioCookie ) == ERROR )
    	{
    	return ERROR;
    	}
    DBG_PRN_STR( 600, "entry: %s\n", pDirEnt,
    		 pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,0 );
    		 
    return OK;
    } /* dosDirOldDirentGet() */
    
/***************************************************************************
*
* dosDirOldLkupInDir - look in directory for specified name.
*
* This routine searches directory, that is pointed by <pFd> for name, that
* is pointed by <pNamePtr> structure and fills <pFd> in accordance with
* directory entry data, if found.
*
* If name not found, <pFreeEnt> will be filled with pointer onto
* some deleted entry in directory or onto free space in last directory
* cluster. If both not found,
* <pFreeEnt->sector> is set to 0.
*
* RETURNS: OK or ERROR if name not found or invalid name.
*/
LOCAL STATUS dosDirOldLkupInDir
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    PATH_ARRAY_ID	pNamePtr,	/* name buffer */
    DIRENT_PTR_ID	pFreeEnt	/* empty entry in directory */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    STATUS	status;	/* search result status */
    u_int	nEntries;	/* number of entries in directory */
    u_short	nameLen;	/* name length on disk */
    u_char	name[  DOS_VX_LONG_NAME_LEN ]; /* name in disk format */
    u_char	dirent[ DOS_VX_DIRENT_LEN ]; /* directory entry buffer */
    
    nameLen = pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen;

    DBG_MSG( 400, "pFd = %p", pFd ,0,0,0,0,0,0,0);
    DBG_PRN_STR( 400, "name: %s\n", pNamePtr->pName,
    			pNamePtr->nameLen, 0 );
    
    pFreeEnt->sector = 0;
    pFreeEnt->offset = 0;
    
    /* prepare name */
    
    if( dosDirOldNameEncode( pDirDesc, pNamePtr, name ) == ERROR )
    	return ERROR;
    
    for( nEntries = 0, (status = dosDirOldDirentGet( pFd, dirent, RD_FIRST ));
    	 (status != ERROR) && *dirent != LAST_DIRENT;
    	 status = dosDirOldDirentGet( pFd, dirent, RD_NEXT ), nEntries++ )
    	{
    	/* pass volume label */
    	
    	if( *(dirent + pDirDesc->deDesc.atrribOff) & DOS_ATTR_VOL_LABEL )
    	    continue;
    	
    	/* pass deleted entry, that later can be used to store new entry */
    	
    	if( *dirent == DOS_DEL_MARK )
    	    {
    	    pFreeEnt->sector = pFd->curSec;
    	    pFreeEnt->offset = pFd->pos;
    	    continue;
    	    }
    	
    	/* compare names */
    	
    	if( bcmp( (char *)name, (char *)dirent, nameLen ) == 0 )
    	    {
    	    break;
    	    }
    	}
    
    /* check result */
    
    if( status == ERROR ||
        ( IS_ROOT( pFd ) && nEntries > pDirDesc->rootMaxEntries ) )
        {
    	return ERROR;
    	}
    
    if(  *dirent == LAST_DIRENT )
    	{
        /* will create new entry on this place, if required */
    
    	if( pFreeEnt->sector == 0 )
    	    {
    	    pFreeEnt->sector = pFd->curSec;
    	    pFreeEnt->offset = pFd->pos;
    	    }
    	return ERROR;
    	}
    
    /* file found; fill file descriptor */
    
    dosDirOldFillFd( pFd, dirent );
    return OK; 
    } /* dosDirOldLkupInDir() */
    
/***************************************************************************
*
* dosDirOldClustAdd - add and init cluster to directory.
*
* RETURNS: OK or ERROR if no more cluster could be allocated.
*/
LOCAL STATUS dosDirOldClustAdd
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    block_t	sec;	/* work count */
    
    /* allocate cluster */
    
    if( pFd->pVolDesc->pFatDesc->getNext( pFd, FAT_ALLOC_ONE ) == ERROR )
    	return ERROR;
    
    assert( pFd->pFileHdl->startClust != 0 );
    
    /* fill cluster: */

    for( sec = pFd->curSec; sec < pFd->curSec + pFd->nSec;
         sec ++ )
    	{
    	if( cbioIoctl(pFd->pVolDesc->pCbio, CBIO_CACHE_NEWBLK,
			(void *)sec ) == ERROR )
    	    {
    	    return ERROR;
    	    }
    	}
        
    return OK;
    } /* dosDirOldClustAdd() */

/***************************************************************************
*
* dosDirOldUpdateEntry - set new directory entry contents.
*
* This routine pulls file size, attributes and so on out of
* file descriptor and stores in the file directory entry.
* 
* This function is also used for entry deletion. In this
* case <flags> have to be set to DH_DELETE.
*
* Time value provided in <curTime> can be encoded into appropriate
* fields within directory entry structure in accordance with
* <flags> argument. <flags> can be or-ed of
* DH_TIME_CREAT, DH_TIME_MODIFY or DH_TIME_ACCESS.
* If <curTime> is 0, current system time is used.
*
* RETURNS: OK or ERROR if name is invalid or disk access error
*  occurred.
*
*/
LOCAL STATUS dosDirOldUpdateEntry
    (
    DOS_FILE_DESC_ID	pFd,
    u_int		flags,
    time_t		curTime		/* time to encode */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    u_char	dirent[ DOS_VX_DIRENT_LEN ]; /* directory entry buffer */
    
    curTime = ( curTime == 0 )? time( NULL ) : curTime;
    
    /* root directory has not its own entry */
    
    if( IS_ROOT( pFd ) )
    	{
    	if( (flags & DH_TIME_MODIFY) != 0 )
    	    pDirDesc->rootModifTime = curTime;
    	else
    	    assert( (flags & DH_TIME_MODIFY) != 0 );
    	
    	return OK;
    	}
    
    /* get directory entry */
    
    if( dosDirOldDirentGet( pFd, dirent, FD_ENTRY ) == ERROR )
    	return ERROR;
    
    if( flags & DH_DELETE )	/* delete entry */
    	{
    	*dirent = DOS_DEL_MARK;
    	goto store;
    	}
    
    /* encode other fields */
    
    dirent[ pDirDesc->deDesc.atrribOff ] = pFd->pFileHdl->attrib;
    START_CLUST_ENCODE( &pDirDesc->deDesc,
    		        pFd->pFileHdl->startClust,  dirent );
    VX_TO_DISK_32( pFd->pFileHdl->size,
    		   dirent + pDirDesc->deDesc.sizeOff );
    		   
    if( pDirDesc->deDesc.extSizeOff != (u_char) NONE )
    	{
    	EXT_SIZE_ENCODE( &pDirDesc->deDesc, dirent, pFd->pFileHdl->size );
    	}
    
    dosDirOldTDEncode( pDirDesc, dirent, flags, curTime );
    
store:    
    /* store directory entry */

    return cbioBytesRW(
    	pFd->pVolDesc->pCbio, pFd->pFileHdl->dirHdl.sector,
    	OFFSET_IN_SEC( pFd->pVolDesc, pFd->pFileHdl->dirHdl.offset ),
	(addr_t)dirent, pDirDesc->deDesc.dirEntSize, CBIO_WRITE,
    	&pFd->pFileHdl->dirHdl.cookie );
    } /* dosDirOldUpdateEntry */

/***************************************************************************
*
* dosDirOldFileCreateInDir - create new entry in directory.
*
* This routine creates new directory entry in place of
* deleted entry. If no deleted entry found in directory,
* additional entry created at the directory tail.
*
* <pFreeEnt> contains pointer onto a deleted entry in
* directory or onto free space in last directory
* cluster, that have been already allocated. If <pFreeEnt->sector> is 0,
* suppose, that no unallocated entry exists in directory and
* <pFd> reached directory FAT chain end. In this case one more cluster is
* connected to directory chain and new entry is created in it.
*
* RETURNS: OK or ERROR if new entry can not be created.
*
* ERRNO:
* S_dosFsLib_DIR_READ_ONLY
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_ROOT_DIR_FULL
* 
*/
LOCAL STATUS dosDirOldFileCreateInDir
    (
    DOS_FILE_DESC_ID	pFd,		/* directory descriptor */
    PATH_ARRAY_ID	pNamePtr,
    u_int		creatOpt,	/* creat flags */
    DIRENT_PTR_ID	pFreeEnt	/* empty entry in directory */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    DOS_DIR_HDL_ID	pDirHdl = (void *)&(pFd->pFileHdl->dirHdl);
    cookie_t	cookie;	/* temp buffer */
    u_char	dirent[ DOS_VX_DIRENT_LEN ] = {0}; /* directory entry buffer */
    u_char	dirEntSize;
    u_char	parentIsRoot = 0;	/* creation in root */
    
    dirEntSize = pDirDesc->deDesc.dirEntSize;
    
    /* check permissions */
    
    if( pFd->pFileHdl->attrib & DOS_ATTR_RDONLY )
    	{
    	errnoSet( S_dosFsLib_DIR_READ_ONLY );
    	return ERROR;
    	}
    	
    /* only file and directory can be created */
    
    if( !( S_ISREG( creatOpt ) || S_ISDIR( creatOpt ) ) )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}
    
    if( IS_ROOT( pFd ) )
    	parentIsRoot = 1;
    
    
    /* check/encode file name */
    
    if( dosDirOldNameEncode( pDirDesc, pNamePtr, dirent ) == ERROR )
    	return ERROR;
    
    /* use empty entry, that was found during path lkup */
    
    if( pFreeEnt->sector == 0 ) /* no free entries in directory */
    	{
    	if( IS_ROOT( pFd ) && pDirDesc->rootMaxEntries < (u_int)(-1) )
    	    {
    	    errnoSet( S_dosFsLib_ROOT_DIR_FULL );
    	    return ERROR;
    	    }
    	
    	/*  allocate one more cluster */
    	
    	if( dosDirOldClustAdd( pFd ) == ERROR )
    	    return ERROR;
    	
    	pFreeEnt->sector = pFd->curSec;
    	pFreeEnt->offset = pFd->pos;
    	}

    /* correct parent directory modification date/time */
    
    dosDirOldUpdateEntry( pFd, DH_TIME_MODIFY, time( NULL ) );
    
    /* --- create new entry --- */
    
    /* at the beginning create generic (file) entry */
    
    dosDirOldTDEncode(pDirDesc, 
                      dirent, 
                      (DH_TIME_CREAT | DH_TIME_MODIFY | DH_TIME_ACCESS), 
                      time( NULL ) );
    
    cookie = pDirHdl->cookie;	/* backup cbio qsearch cookie */
    /*
     * point file descriptor on the new entree's position and
     * fill file descriptor for new file
     */
    pFd->curSec = pFreeEnt->sector;
    pFd->pos = pFreeEnt->offset;
    dosDirOldFillFd( pFd, dirent );
    
    DBG_MSG( 400, "create %s at: sector = %u offset = %u\n",
    		( S_ISREG( creatOpt )? "FILE" : "DIRECTORY" ),
    		pFreeEnt->sector, pFreeEnt->offset ,0,0,0,0,0);
    DBG_PRN_STR( 400, "\tname: %s\n", dirent,
    		 pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,0 );
    
    /* write f i l e entry to disk */
    
    if( S_ISREG( creatOpt ) )
    	{
    	return cbioBytesRW(
    		pFd->pVolDesc->pCbio, pFreeEnt->sector,
		OFFSET_IN_SEC( pFd->pVolDesc, pFreeEnt->offset ),
    		(addr_t)dirent, dirEntSize, CBIO_WRITE, &cookie );
    	}
    
    /* --- now we deal with directory --- */
    
    *(dirent + pDirDesc->deDesc.atrribOff) = DOS_ATTR_DIRECTORY;
    pFd->pFileHdl->attrib = DOS_ATTR_DIRECTORY;
    
    /*
     * for directory we have to create two entries: "." and ".."
     * start - allocate cluster and init it in structure.
     */
    if( dosDirOldClustAdd( pFd ) == ERROR )
    	return ERROR;
    
    START_CLUST_ENCODE( &pDirDesc->deDesc,
    		        pFd->pFileHdl->startClust,  dirent );
    
    /*
     * second - init directory entry for ".";
     * it is similar to directory entry itself, except name
     */
    bfill( (char *)dirent,
    	   pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,
    	   SPACE );
    *dirent = DOT;
    if( cbioBytesRW(
    		pFd->pVolDesc->pCbio, pFd->curSec, 0, (addr_t)dirent,
    		dirEntSize, CBIO_WRITE, &pDirHdl->cookie ) == ERROR )
    	{
    	return ERROR;
    	}
    
    pFd->pos += dirEntSize;
    if( OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ) == 0 )
    	{
    	pFd->nSec --;
    	pFd->curSec ++;
    	}
    
    /* if only one directory entry can be stored in cluster */
    
    if( pFd->nSec == 0 )
    	{
    	if( dosDirOldClustAdd( pFd ) == ERROR )
    	    return ERROR;
    	}
    
    /* third - creat ".." entry, that points to the parent directory */
    
    if( parentIsRoot )
    	{
    	START_CLUST_ENCODE( &pDirDesc->deDesc, 0,  dirent );
    	}
    else
    	{
    	START_CLUST_ENCODE( &pDirDesc->deDesc,
    		            pDirHdl->parDirStartCluster,  dirent );
    	}
    
    *(dirent + 1) = DOT;
    if( cbioBytesRW(
    		pFd->pVolDesc->pCbio, pFd->curSec,
		OFFSET_IN_SEC(  pFd->pVolDesc, pFd->pos ), (addr_t)dirent,
    		dirEntSize, CBIO_WRITE, &pDirHdl->cookie ) == ERROR )
    	{
    	return ERROR;
    	}
    
    /* now finish directory entry initialization */
    
    dosDirOldRewindDir( pFd );	/* rewind directory */
    
    dosDirOldNameEncode( pDirDesc, pNamePtr, dirent );
    START_CLUST_ENCODE( &pDirDesc->deDesc,
    		        pFd->pFileHdl->startClust,  dirent );
    if( cbioBytesRW(
    		pFd->pVolDesc->pCbio, pDirHdl->sector,
		OFFSET_IN_SEC( pFd->pVolDesc, pDirHdl->offset ), 
    		(addr_t)dirent, dirEntSize, CBIO_WRITE,
    		&pDirHdl->cookie ) == ERROR )
    	{
    	return ERROR;
    	}
    return OK;
    } /* dosDirOldFileCreateInDir() */
    
/***************************************************************************
*
* dosDirOldNameDecode - decode name from directory entry format.
*
* RETURNS: OK or ERROR if no disk entries remain.
*/
LOCAL void dosDirOldNameDecode
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    u_char *	pDirent,	/* directory entry buffer */
    u_char *	pDstName	/* destination for directory name */
    )
    {
    u_char *	pNameEnd = pDstName;
    u_char *	pDstBuf;	/* for debug output */
    u_char *	pDot = NULL;
    u_char *	pSrc = pDirent;
    int i, j, nameLen;
    
    pDstBuf = pDstName;
    
    for( pDot = NULL, nameLen = pDirDesc->deDesc.nameLen, i = 0; i  < 2;
    	 i++, nameLen = pDirDesc->deDesc.extLen )
    	{ 
    	for( j = 0; j < nameLen; j++, pDstName++ )
    	    {
    	    *pDstName = *pSrc++;
    	    
    	    if( *pDstName != SPACE )
    	    	pNameEnd = pDstName + 1;
    	    }
    	
    	if( pDirDesc->deDesc.extLen == 0 )	/* long name */
    	    break;
    	    
    	/* extension */
    	
    	if( pDot == NULL )	/* separate name and extension */
    	    {
    	    pDot = pNameEnd;
    	    *pNameEnd++ = DOT;
    	    pDstName = pNameEnd;
    	    }
    	else if( pDot + 1 == pNameEnd )
    	    {
    	    pNameEnd --;	/* empty extension - remove dot */
    	    break;
    	    }
    	}
    
    *pNameEnd = EOS;	/* terminate the name */
    DBG_MSG( 600, "result: %s\n", pDstBuf ,0,0,0,0,0,0,0);
    } /* dosDirOldNameDecode() */
    
/***************************************************************************
*
* dosDirOldReaddir - FIOREADDIR backend.
*
* Optionally this routine fills file descriptor for the encountered entry,
* if <pResFd> is not NULL.
*
* RETURNS: OK or ERROR if no disk entries remain.
*/
LOCAL STATUS dosDirOldReaddir
    (
    DOS_FILE_DESC_ID	pFd,	/* descriptor of the directory being read */
    DIR *	pDir,	/* destination for directory name */
    DOS_FILE_DESC_ID	pResFd	/* file descriptor to be filled for the */
    				/* being returned */
    				/* (if pResFd is not NULL) */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    int readFlag;
    u_char	dirent[ DOS_VX_DIRENT_LEN ]; /* directory entry buffer */
    
    assert( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY );
    
    /* check status of directory */
    
    if( (int)pDir->dd_cookie == (-1) )
    	return ERROR;
    
    /* position must be directory entry size aligned */

    if( pFd->pos % DOS_STDNAME_LEN != 0 )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}

    readFlag = ( pDir->dd_cookie == 0 )? RD_FIRST : RD_NEXT;
    
    do
    	{
    	if( dosDirOldDirentGet( pFd, dirent, readFlag ) == ERROR )
            {
            pDir->dd_cookie = (-1);
    	    return ERROR;
    	    }
    	readFlag = RD_NEXT;
    	}
    while( *dirent == DOS_DEL_MARK ||
    	   (dirent[ pDirDesc->deDesc.atrribOff ] & DOS_ATTR_VOL_LABEL) != 0 );
    
    if( *dirent == LAST_DIRENT )	/* end of directory */
    	{
    	pDir->dd_cookie = (-1);
    	return ERROR;
    	}
    
    /* name decode */
    
    dosDirOldNameDecode( pDirDesc, dirent,(u_char *)pDir->dd_dirent.d_name );
    
    /* fill file descriptor for the entry */
    
    if( pResFd != NULL )
    	{
    	void *	pFileHdl = pResFd->pFileHdl;
    	
    	/* prepare file descriptor contents */
    	
    	*pResFd = *pFd;
    	pResFd->pFileHdl = pFileHdl;
    	*pResFd->pFileHdl = *pFd->pFileHdl;
    	
    	dosDirOldFillFd( pResFd, dirent );
    	}
    
    pDir->dd_cookie = POS_TO_DD_COOKIE( pFd->pos );
    return OK;
    } /* dosDirOldReaddir() */
    
/***************************************************************************
*
* dosDirOldPathLkup - lookup for file/dir in tree.
*
* This routine recursively searches directory tree for the <path> and
* fills file descriptor for the target entry if successful.
* Optionally new entry may be created in accordance with flags
* in argument <options>.
*
* DOS4.0 style names are always case insensitive in contrary to
* VxLong name, that always are compared case sensitively. So
* DOS_O_CASENS is ignored in  <options> argument.
*
* <options> :	O_CREAT - creat file;
*		O_CREAT | DOS_ATTR_DIRECTORY - creat directory.
*		<somth> | DOS_O_CASENS - lkup for name in case
		                         sensitive manner (ignored).
*
* RETURNS: OK or ERROR if file not found.
*
* ERRNO:
* S_dosFsLib_FILE_NOT_FOUND
*/
LOCAL STATUS dosDirOldPathLkup
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    void *	path,	/* path in tree */
    u_int	options	/* optional creat flags */
    )
    {
    PATH_ARRAY	namePtrArray [DOS_MAX_DIR_LEVELS + 1];
    int		numPathLevels;
    int		errnoBuf;	/* swap errno */
    DIRENT_PTR	freeEnt;	/* empty (deleted) entry in directory */
    u_char	dirLevel;	/* dynamic directory level counter */
    
    assert( path != NULL );
    
    /* disassemble path */
    
    numPathLevels = dosDirOldPathParse( pFd->pVolDesc, path, 
    					namePtrArray ); 
    if( numPathLevels == ERROR )
    	return ERROR;
    assert( numPathLevels <= DOS_MAX_DIR_LEVELS );
    
    /* start from root directory */
    
    dosDirOldFillFd( pFd, ROOT_DIRENT );
    
    errnoBuf = errnoGet();
    errnoSet( OK );
    for( dirLevel = 0; dirLevel < numPathLevels; dirLevel ++  )
    	{
    	/* only directory can be searched */
    	
    	if( ! (pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) )
    	    break;
    	
    	if( dosDirOldLkupInDir( pFd, namePtrArray + dirLevel, &freeEnt )
		== ERROR )
    	    {
    	    if( errnoGet() != OK )	/* subsequent error */
    	    	return ERROR;
    	    	
    	    break;
    	    }
    	}
    
    if( errnoGet() == OK )
    	errnoSet( errnoBuf );
    
    /* check result */
    
    if( dirLevel == numPathLevels )
    	{
    	return OK;
    	}
    
    /* --- file not found --- */
    
    /* only last file in path can be created */
    
    if( dirLevel == numPathLevels - 1 &&  (options & O_CREAT) )
    	{
    	if( dosDirOldFileCreateInDir( pFd,
    				      namePtrArray + dirLevel,
    			              options, &freeEnt ) == OK )
    	    return OK;
    	}
    else
    	{
    	errnoSet( S_dosFsLib_FILE_NOT_FOUND );
    	}
    
    /* error; release dir handle */
    
    return ERROR;
    } /* dosDirOldPathLkup() */

/***************************************************************************
*
* dosDirOldDateGet - fill in date-time fields in stat structure.
*
* RETURNS: OK or ERROR if disk access error.
*
*/
LOCAL STATUS dosDirOldDateGet
    (
    DOS_FILE_DESC_ID	pFd,
    struct stat *	pStat
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    u_char	dirent[ DOS_VX_DIRENT_LEN ]; /* directory entry buffer */
    
    /* root directory has not its own entry */
    
    if( IS_ROOT( pFd ) )
    	{
    	pStat->st_mtime = pDirDesc->rootModifTime;
    	return OK;
    	}
    
    /* get directory entry */
    
    if( dosDirOldDirentGet( pFd, dirent, FD_ENTRY ) == ERROR )
    	return ERROR;
    
    pStat->st_ctime = dosDirOldTDDecode( pDirDesc, dirent, DH_TIME_CREAT );
    pStat->st_mtime = dosDirOldTDDecode( pDirDesc, dirent, DH_TIME_MODIFY );
    pStat->st_atime = dosDirOldTDDecode( pDirDesc, dirent, 
    					 DH_TIME_ACCESS );
    return OK;
    } /* dosDirOldDateGet() */
    
/*******************************************************************************
*
* dosDirOldVolLabel - set/get dos volume label.
*
* This routine gets/changes the volume label entry in the root directory of a
* dosFs volume.
*
* Only one volume label entry may exist per volume and in root directory only.
* When request to change volume label arrives and there is already a volume
* label, the name is simply
* replaced.  If there is currently no label, a new volume label entry is
* created.
*
* RETURNS: OK, or ERROR if volume is not available.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_NO_LABEL
* S_dosFsLib_ROOT_DIR_FULL
*/
LOCAL STATUS dosDirOldVolLabel
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    u_char *	label,
    u_int	request		/* FIOLABELSET, FIOLABELGET */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    DOS_FILE_DESC	fd;	/* work file descriptor */
    DOS_FILE_HDL	fHdl;	/* work file handle */
    u_int	numEnt;	/* number of passed entries */
    u_short	labOffInBoot;	/* volume label offset in boot sector */
    char *	noName = "NO LABEL";
    STATUS	status = ERROR;	/* search status */
    u_char	dirent[ DOS_VX_DIRENT_LEN ]; /* directory entry buffer */
    
    if( ! ( request == FIOLABELSET || request == FIOLABELGET ) )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}
    
    labOffInBoot = ( pVolDesc->fatType == FAT32 )? DOS32_BOOT_VOL_LABEL:
    						   DOS_BOOT_VOL_LABEL;
    if( label == NULL )
    	{
    	if( request == FIOLABELSET )
    	    label = (u_char *)noName;	/* use default name */
    	else
    	    {
    	    errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	    return ERROR;
    	    }
    	}
    
    /* init file descriptor */
    
    fd.pVolDesc = pVolDesc;
    fd.pFileHdl = &fHdl;
    dosDirOldFillFd( &fd, ROOT_DIRENT );
    
    /* search for volume label */
    
    for( numEnt = 0, (status = dosDirOldDirentGet( &fd, dirent, RD_FIRST ));
    	 (status != ERROR) && *dirent != LAST_DIRENT;
    	 numEnt ++, status = dosDirOldDirentGet( &fd, dirent, RD_NEXT ) )
    	{
    	if( *(dirent + pDirDesc->deDesc.atrribOff) & DOS_ATTR_VOL_LABEL )
    	    break;
    	}
    
    /* get label */
    
    if( request == FIOLABELGET )
    	{
    	if( status == ERROR || *dirent == LAST_DIRENT )
    	    {
    	    /*
    	     * no volume label found in root dir
    	     * extract label out of boot sector
    	     */
    	    bcopy( pVolDesc->bootVolLab, (char *)dirent, DOS_VOL_LABEL_LEN );
    	    }

    	bcopy( (char *)dirent, (char *)label, DOS_VOL_LABEL_LEN );
    	numEnt = DOS_VOL_LABEL_LEN;
    	while( numEnt > 0 && label[ numEnt-1 ] == SPACE )
    	    label[ --numEnt ] = EOS;
    	
    	return OK;
    	}
    
    /* change/create label */
    
    /* 
     * if no label found and no free entry in root,
     * add and init one more cluster.
     */
    if( status == ERROR )	/* label not found and no free entry */
    	{
    	if( numEnt >= pDirDesc->rootMaxEntries )	/* root dir is full */
    	    {
    	    errnoSet( S_dosFsLib_ROOT_DIR_FULL );
    	    return ERROR;
    	    }
    	else if( dosDirOldClustAdd( &fd ) == ERROR )
    	    return ERROR;
    	}
    
    /* encode name */
    
    bzero( (char *)dirent, sizeof( dirent ) );
    bfill( (char *)dirent, pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen, 
           SPACE );
    bcopy( (char *)label, (char *)dirent,
           min( DOS_VOL_LABEL_LEN, strlen( (char *)label ) ) );
    dirent[ pDirDesc->deDesc.atrribOff] = DOS_ATTR_VOL_LABEL;
    
    /* --- store label --- */
    
    /* store in root directory */
    
    status = cbioBytesRW(
    			pVolDesc->pCbio, fd.curSec,
			OFFSET_IN_SEC( pVolDesc, fd.pos ), (addr_t)dirent, 
    			pDirDesc->deDesc.dirEntSize, CBIO_WRITE,
    			&fHdl.dirHdl.cookie );
    
#ifdef CNANGE_BOOT
    /* XXX - avoid changing boot block to avoid false disk removal event */

    /* store in boot sector */
    bcopy( (char *)dirent, pVolDesc->bootVolLab, DOS_VOL_LABEL_LEN );
    status |= cbioBytesRW(
    			pVolDesc->pCbio, DOS_BOOT_SEC_NUM,
    			labOffInBoot, (addr_t)dirent, 
    			DOS_VOL_LABEL_LEN, CBIO_WRITE, NULL );
#endif /* CNANGE_BOOT */
    
    return status;
    } /* dosDirOldVolLabel() */
    
/*******************************************************************************
*
* dosDirOldNameChk - validate file name.
*
* This routine validates incoming file name to be composed
* of valid characters.
*
* RETURNS: OK if name is OK or ERROR.
*/
LOCAL STATUS dosDirOldNameChk
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    u_char *	name
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    PATH_ARRAY	namePtr;
    u_char	dirent[ DOS_VX_DIRENT_LEN ] = {0}; /* directory entry buffer */
    
    namePtr.pName = name;
    namePtr.nameLen = strlen((char *)name);
    
    return dosDirOldNameEncode( pDirDesc, &namePtr, dirent );
    } /* dosDirOldNameChk() */

/*******************************************************************************
*
* dosDirOldShow - display handler specific volume configuration data.
*
* RETURNS: N/A.
*/
LOCAL void dosDirOldShow
    (
    DOS_VOLUME_DESC_ID	pVolDesc
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    
    if( ! pVolDesc->mounted )
    	return;
    
    printf(" - names style:			%s\n",
    	   	( (pDirDesc->nameStyle == STDDOS)?
    	  	  "8.3 STD DOS" : "VxLong" ) );
    
    if( pDirDesc->dirDesc.rootStartSec != 0 )	/* FAT12/FAT16 */
    	{
    	printf(" - root dir start sector:		%u\n",
    			pDirDesc->dirDesc.rootStartSec );
    	printf(" - # of sectors per root:		%u\n",
    			pDirDesc->dirDesc.rootNSec );
    	printf(" - max # of entries in root:		%u\n",
    			pDirDesc->rootMaxEntries );
    	}
    else	/* FAT32 */
    	{
    	printf(" - root dir start cluster:		%u\n",
    			pDirDesc->rootStartClust );
    	}
    
    return;
    } /* dosDirOldShow() */

/*******************************************************************************
*
* dosDirOldVolUnmount - stub.
*
* RETURNS: N/A.
*/
LOCAL void dosDirOldVolUnmount
    (
    DOS_VOLUME_DESC_ID	pVolDesc
    )
    {
    return;
    } /* dosDirOldVolUnmount() */

/*******************************************************************************
*
* dosDirOldVolMount - init all data required to access the volume.
*
* This routine fills all local and shared structures fields, that
* depend on a directory structure format. All data is updated
* from boot sector of the volume is being mounted and from
* volume descriptor argument.
*
* RETURNS: OK or ERROR if the volume has inappropriate format.
*
* ERRNO:
* S_dosFsLib_UNSUPPORTED
* S_dosFsLib_UNKNOWN_VOLUME_FORMAT
*/
LOCAL STATUS dosDirOldVolMount
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    void *	arg
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = NULL;
    DIRENT_DESCR_ID	pDeDesc = NULL;
    cookie_t	cookie = 0;
    char	bootSec[DOS_BOOT_BUF_SIZE];
    
    assert( (pVolDesc != NULL) && pVolDesc->magic == DOS_FS_MAGIC );
    
    /* check for vxWorks long names */
    
    if( cbioBytesRW(
    		pVolDesc->pCbio, pVolDesc->bootSecNum, 0,
    		bootSec,
    		min( sizeof( bootSec ), pVolDesc->bytesPerSec ),
    		CBIO_READ, &cookie ) == ERROR )
    	{
    	return ERROR;
    	}
    	
    /* 
     * if previous volume had alternative directory structure,
     * unmount previous directory handler and
     * allocate directory handler descriptor
     */
    if( pVolDesc->pDirDesc == NULL ||
    	pVolDesc->pDirDesc->volUnmount != dosDirOldVolUnmount )
    	{
    	/* unmount previous directory handler */
    	    
    	if( pVolDesc->pDirDesc != NULL &&
    	    pVolDesc->pDirDesc->volUnmount != NULL )
    	    {
    	    pVolDesc->pDirDesc->volUnmount( pVolDesc );
    	    }
    	    
    	/* allocate directory handler descriptor */
    	    
    	pVolDesc->pDirDesc = 
		KHEAP_REALLOC((char *) pVolDesc->pDirDesc,
    	    			      sizeof( DOS_DIR_PDESCR ) );
    	if( pVolDesc->pDirDesc == NULL )
    	    return ERROR;
    	}
    
    pDirDesc = (void *)pVolDesc->pDirDesc;
    pDeDesc = &pDirDesc->deDesc;
    bzero( (char *)pDirDesc, sizeof( DOS_DIR_PDESCR ) );
    
    if( bcmp( bootSec + DOS_BOOT_SYS_ID, DOS_VX_LONG_NAMES_SYS_ID,
    	      strlen( DOS_VX_LONG_NAMES_SYS_ID ) ) == 0 )
    	{
    	/* use vxWorks long names */
    	
    	ERR_MSG( 10, "VxLong names\n", 0,0,0,0,0,0 );
    	
    	pDirDesc->nameStyle = VXLONG;
    	
    	pDeDesc->dirEntSize	= DOS_VX_DIRENT_LEN;
    	pDeDesc->nameLen	= DOS_VX_NAME_LEN;
    	pDeDesc->extLen		= DOS_VX_EXT_LEN;
    	pDeDesc->atrribOff	= DOS_VX_ATTRIB_OFF;
    	pDeDesc->creatTimeOff	= DOS_VX_CREAT_TIME_OFF;
    	pDeDesc->creatDateOff	= DOS_VX_CREAT_DATE_OFF;
    	pDeDesc->modifTimeOff	= DOS_VX_MODIF_TIME_OFF;
    	pDeDesc->modifDateOff	= DOS_VX_MODIF_DATE_OFF;
    	pDeDesc->accessTimeOff	= DOS_VX_LAST_ACCESS_TIME_OFF;
    	pDeDesc->accessDateOff	= DOS_VX_LAST_ACCESS_DATE_OFF;
    	pDeDesc->startClustOff	= DOS_VX_START_CLUST_OFF;
    	pDeDesc->extStartClustOff = DOS_VX_EXT_START_CLUST_OFF;
    	pDeDesc->sizeOff	= DOS_VX_FILE_SIZE_OFF;
    	pDeDesc->extSizeOff	= DOS_VX_EXT_FILE_SIZE_OFF;
    	pDeDesc->extSizeLen	= DOS_VX_EXT_FILE_SIZE_LEN;
    	}
    else
    	{
    	/* use DOS traditional 8.3 name */
    	
    	ERR_MSG( 10, "DOS 8.3 names\n", 0,0,0,0,0,0 );
    	
    	pDirDesc->nameStyle = STDDOS;
    	
    	pDeDesc->dirEntSize	= DOS_DIRENT_STD_LEN;
    	pDeDesc->nameLen	= DOS_STDNAME_LEN;
    	pDeDesc->extLen		= DOS_STDEXT_LEN;
    	pDeDesc->atrribOff	= DOS_ATTRIB_OFF;
    	pDeDesc->creatTimeOff	= DOS_MODIF_TIME_OFF;
    	pDeDesc->creatDateOff	= DOS_MODIF_DATE_OFF;
    	pDeDesc->modifTimeOff	= DOS_MODIF_TIME_OFF;
    	pDeDesc->modifDateOff	= DOS_MODIF_DATE_OFF;
    	pDeDesc->accessTimeOff	= NONE;
    	pDeDesc->accessDateOff	= NONE;
    	pDeDesc->startClustOff	= DOS_START_CLUST_OFF;
    	pDeDesc->extStartClustOff = DOS_EXT_START_CLUST_OFF;
    	pDeDesc->sizeOff	= DOS_FILE_SIZE_OFF;
    	pDeDesc->extSizeOff	= DOS_EXT_FILE_SIZE_OFF;
    	pDeDesc->extSizeLen	= DOS_EXT_FILE_SIZE_LEN;
    	}
    
    /* check correspondence of cluster and directory entry size */
    
    if( pDeDesc->dirEntSize >
        ( pVolDesc->secPerClust << pVolDesc->secSizeShift ) )
    	{
    	ERR_MSG( 0, "cluster size %d bytes is too small, min = %d\n",
    		    pVolDesc->secPerClust << pVolDesc->secSizeShift,
    		    pDeDesc->dirEntSize, 0,0,0,0 );
    	errnoSet( S_dosFsLib_UNKNOWN_VOLUME_FORMAT );
    	return ERROR;
    	}
    
    /*
     * init root directory parameters in accordance with
      * volume FAT version
     */
    if( pVolDesc->fatType == FAT32 )	/* FAT32 */
    	{
    	/*
    	 * init root directory descriptor.
    	 * Get root directory start cluster number
    	 */
    	pDirDesc->rootStartClust =
    		DISK_TO_VX_32( bootSec + DOS32_BOOT_ROOT_START_CLUST );
    	if( pDirDesc->rootStartClust < 2 )
    	    {
    	    ERR_MSG( 1, "Malformed volume format (FAT32: rootStartClust "
    			" = %u)\n", pDirDesc->rootStartClust, 0,0,0,0,0 );
    	    errnoSet( S_dosFsLib_UNKNOWN_VOLUME_FORMAT );
    	    return ERROR;
    	    }
    	pDirDesc->dirDesc.rootStartSec	 = 0;
    	pDirDesc->dirDesc.rootNSec	 = 0;
    	pDirDesc->rootMaxEntries = (u_int)(-1); /* not restricted */
    	}
    else		/* FAT12/FAT16 */
    	{
    	/*
    	 * init root directory descriptor.
    	 * Get number of entries-per-root directory
    	 */
    	pDirDesc->rootMaxEntries =
    			DISK_TO_VX_16( bootSec  + DOS_BOOT_MAX_ROOT_ENTS);
    	if( pDirDesc->rootMaxEntries == 0 )
    	    {
    	    ERR_MSG( 1, "Malformed volume format (FAT12/16: "
    			"rootMaxEntries = 0)\n", 0,0,0,0,0,0 );
    	    errnoSet( S_dosFsLib_UNKNOWN_VOLUME_FORMAT );
    	    return ERROR;
    	    }
    	
    	pDirDesc->rootStartClust = 0;	/* don't change this 0 ! */
    					/* it is important while */
    					/* creating subdir in root */
    	
        /* SPR#34704 This operation must round up. */

        pDirDesc->dirDesc.rootNSec = 
                (((pDirDesc->rootMaxEntries * pDeDesc->dirEntSize) + 
                  (pVolDesc->bytesPerSec - 1)) / pVolDesc->bytesPerSec);

    	/* root directory resides ahead regular data area */
    	
    	pDirDesc->dirDesc.rootStartSec = pVolDesc->dataStartSec;
    	pVolDesc->dataStartSec += pDirDesc->dirDesc.rootNSec;
    	}
    
    /* init root directory last modification time */
    
    pDirDesc->rootModifTime = time( NULL );
    
    /* fill functions pointers */
    
    pDirDesc->dirDesc.pathLkup		= dosDirOldPathLkup;
    pDirDesc->dirDesc.readDir		= dosDirOldReaddir;
    pDirDesc->dirDesc.updateEntry	= dosDirOldUpdateEntry;
    pDirDesc->dirDesc.dateGet		= dosDirOldDateGet;
    pDirDesc->dirDesc.volLabel		= dosDirOldVolLabel;
    pDirDesc->dirDesc.nameChk		= dosDirOldNameChk;
    pDirDesc->dirDesc.volUnmount	= dosDirOldVolUnmount;
    pDirDesc->dirDesc.show		= dosDirOldShow;

    return OK;
    } /* dosDirOldVolMount() */

/*******************************************************************************
*
* dosDirOldLibInit - install <8.3> and VxLong names handler into dosFsLib
*
* RETURNS: OK or ERROR if handler installation failed.
*
* NOMANUAL
*/
STATUS dosDirOldLibInit ( void )
    {
    DOS_HDLR_DESC	hdlr;
    
    hdlr.id = DOS_DIROLD_HDLR_ID;
    hdlr.mountRtn = dosDirOldVolMount;
    hdlr.arg = NULL;
    
    return dosFsHdlrInstall( dosDirHdlrsList, &hdlr );
    } /* dosDirOldLibInit() */
/* End of File */

