/* dosVDirLib.c - MS-DOS VFAT-style Long File names dir handler */ 

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01p,10dec01,jkf  SPR#72039, various fixes from Mr. T. Johnson.
01o,09nov01,jkf  SPR#70968, chkdsk destroys boot sector
01n,21aug01,jkf  SPR#69031, common code for both AE & 5.x.
01m,06mar01,jkf  SPR#34704, rootNSec calculation must round up.
01n,01aug00,dat  SPR 31480, directory cluster corruption fix (neeraj)
01l,07mar00,jkf  Corrected pentium specific compiler warning "using %eax 
                 instead of %ax due to l" (source of P6 reg stall?)
01k,29feb00,jkf  T3 changes, cleanup. 
01j,31aug99,jkf  changes for new CBIO API.
01i,31jul99,jkf  moved the fix for SPR#28276 from dosVDirFileCreateInDir() 
                 to dosVDirNameEncodeShort because it was only mangling the 
                 name in 01g and not creating an alias for it, no SPR.
01h,31jul99,jkf  default debug global to zero, no SPR.
01g,31jul99,jkf  Fixed dosVDirFileCreateInDir to mangle 8.3 names containing 
                 char-space-char, per MS compatability.  MS Scandisk and 
		 Norton Disk Doctor stops reporting a false orphaned LFN 
		 which they repair by removing it.  SPR#28276
01f,31jul99,jkf  T2 merge, tidiness & spelling.
01e,07dec98,lrn  minor Show output cosmetics
01d,22nov98,vld  all offsets of directory entry changed to be
		 counted in absolute offset from parent directory
		 start
01c,28sep98,vld  gnu extensions dropped from DBG_MSG/ERR_MSG macros
01b,02jul98,lrn  review doc
01a,18jan98,vld  written,
*/

/*
DESCRIPTION
This library is part of dosFsLib and is designed to handle the
VFAT "long filename" standard disk directory structure.
VFAT is the file name format initially introduced with OS/2 operating
system, and subsequently adopted by Microsoft in Windows 95 and NT
operating systems for FAT-based file systems.

With this format, every file has a Long Name which occupies a number of
directory entries, and a short file name which adheres to the vintage
MS-DOS file naming conventions. The later are called aliases.

This handler performs its lookup only by file long names, not by aliases,
in other words, the aliases, are provided only for data interchange with
Microsoft implementations of FAT, and are ignored otherwise.

Aliases, that are created for long file names are Windows95/98 compatible,
that means, they are readable from Windows applications, but they are
not produced from the corresponding long file name.

In this implementation the alias is made of numbers to ensure that
every alias is unique in its directory without having to scan the entire
directory.  This ensure determinism.

Therefore if a volume written with this handler is every transported to
an old MS-DOS system or comparable implementation of dosFs,
which only accepts vintage 8.3 file names, the names of files will not
be readily associated with their original names, and thus practically
unusable. The goal was to ensure determinism and file safety.

Uppercase 8.3 filename that follow strict 8.3 rules such as 
"UPPERFIL.TXT" will not be stored in a long filename.  

Lowercase 8.3 Filenames such as "Upperfil.txt" will have a readable
alias created of "UPPERFIL.TXT"   

Lowercase 8.3 Filenames such as "File   " will have an alias 
created of "FILE    ".

Filenames with a sequence of char, space, char strings. 
Such as "3D Pipes.scr" will also have a munged alias created 
akin to "3~999997".

The routine dosVDirInit() routine has to be called once to install 
this handler into dosFsLib. Once that has been done this directory
handler will be automatically mounted for each new DOS volume being 
mounted and containing VFAT or early DOS directory structure.

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
	{ if( (lvl) <= dosVDirDebug )		{		\
	    char cBuf = ((char *)(pStr))[len];			\
	    (pStr)[len] = EOS; 					\
	    printErr( (fmt),(pStr),(arg) );			\
	    ((char *)(pStr))[len] = cBuf; 	} }

#   define DBG_MSG( lvl, fmt, arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8 )	\
	{ if( (lvl) <= dosVDirDebug )					\
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
	{ if( (lvl) <= dosVDirDebug ) 				\
            logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }

#endif /* DEBUG */
 
#include "assert.h"

#ifdef ERR_SET_SELF
#   define errnoSet( err ) errnoSetOut( __FILE__, __LINE__, #err, (err) )
#endif /* ERR_SET_SELF */

#define VFAT_MAX_ENT	( (DOS_VFAT_NAME_LEN + CHAR_PER_VENTRY - 1) \
			  / CHAR_PER_VENTRY )
					/* max directory entries required */
					/* to store VFAT long name */
#define MAX_VFAT_FULL_DIRENT	( DOS_DIRENT_STD_LEN * (VFAT_MAX_ENT + 1) )
					/* max space of long name + alias */
					/* may occupy on disk */
#define CHAR_PER_VENTRY		13	/* num of characters of long name */
					/* encoded within directory entry */
#define VFAT_ENTNUM_MASK	0x1f	/* mask of "entry number within */
					/* long name" field */ 

#define DOS_VFAT_CHKSUM_OFF	13
#define DOS_ATTR_VFAT		0x0f	/* value of attribute field for */
					/* directory entries, that encodes */
					/* VFAT long name */

/* special characters */

#define DOS_VLAST_ENTRY	0x40	/* last entry in long name representation */

/* special function argument value */

#define DH_ALLOC	(1<<7)	/* add cluster to directory chain */
#define PUT_CURRENT	(1<<0)	/* store currently pointed directory entry */
#define	PUT_NEXT	(1<<1)	/* store next directory entry */

/* macros */

/* typedefs */

typedef enum {RD_FIRST, RD_CURRENT, RD_NEXT, FD_ENTRY} RDE_OPTION;
				/* function argument */

typedef enum {STRICT_SHORT, NOT_STRICT_SHORT, NO_SHORT} SHORT_ENCODE;
				/* function argument */

typedef struct DOS_VDIR_DESCR	/* directory handler's part of */
				/* volume descriptor */
    {
    DOS_DIR_PDESCR	genDirDesc;	/* generic descriptor */
    } DOS_VDIR_DESCR;

typedef DOS_VDIR_DESCR *	DOS_VDIR_DESCR_ID;

/* globals */

unsigned int	dosVDirDebug = 0;

/* locals */

/* positions of filename characters encoding within VFAT directory entry */

LOCAL const u_char	chOffsets[] = { 1, 3, 5, 7, 9,
					14, 16, 18, 20, 22, 24, 	
					28, 30 };


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

#ifdef ERR_SET_SELF
/*******************************************************************************
* errnoSetOut - put error message
*
* This routine is called instead errnoSet during debugging.
*/
static VOID errnoSetOut(char * file, int line, const char * str, int errCode )
    {
    if( errCode != OK  && strcmp( str, "errnoBuf" ) != 0 )
        printf( " %s : line %d : %s = %p, task %p\n",
                file, line, str, (void *)errCode,
                (void *)taskIdSelf() );
    errno = errCode;
    }
#endif  /* ERR_SET_SELF */

/***************************************************************************
*
* dosVDirFillFd - fill file descriptor for directory entry.
*
* RETURNS: N/A.
*/
LOCAL void dosVDirFillFd
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    u_char *	pDirEnt,	/* directory entry from disk */
    DIRENT_PTR_ID	pLnPtr	/* start of long name */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    DOS_DIR_HDL_ID	pDirHdl = (void *)&(pFd->pFileHdl->dirHdl);
    DIRENT_DESCR_ID	pDeDesc;
    
    pDeDesc = &pDirDesc->deDesc;
    
    if( pDirEnt == NULL )	/* root directory */
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
    	pDirHdl->lnSector = 0;
    	pDirHdl->lnOffset = 0;
    	
    	pFd->cbioCookie = (cookie_t) NULL;
    	pDirHdl->cookie = (cookie_t) NULL;
    	
    	goto rewind;
    	}
    
    /* at the beginning fill directory handle using file descriptor */
    
    pDirHdl->parDirStartCluster = pFd->pFileHdl->startClust;
    pDirHdl->sector = pFd->curSec;
    pDirHdl->offset = pFd->pos;

    pDirHdl->cookie = pFd->cbioCookie;
    
    /* long name ptr */
    
    if( pLnPtr != NULL )
    	{
    	pDirHdl->lnSector = pLnPtr->sector;
    	pDirHdl->lnOffset = pLnPtr->offset;
    	}
    else	/* just a short name */
    	{
    	pDirHdl->lnSector = 0;
    	pDirHdl->lnOffset = 0;
    	}

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
    DBG_MSG( 100, "StartCluster = %u = %p, size = %lu\n",
    		pFd->pFileHdl->startClust, (void *)pFd->pFileHdl->startClust,
    		(u_long)pFd->pFileHdl->size & 0xffffffff ,0,0,0,0,0);
        
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
    } /* dosVDirFillFd() */

/***************************************************************************
*
* dosVDirRewindDir - rewind directory pointed by <pFd>.
*
* This routine sets current sector in pFd to directory start sector
* and current position  (offset in sector) to 0.
* 
* RETURNS: N/A.
*/
LOCAL void dosVDirRewindDir
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
    } /* dosVDirRewindDir() */
   
/***************************************************************************
*
* dosVDirPathParse - parse a full pathname into an array of names.
*
* This routine is similar to pathParse(), but on the contrary it does not 
* allocate additional buffers nor changes the path string.
*
* Parses a path in directory tree which has directory names
* separated by '/' or '\'s.  It fills the supplied array of
* structures with pointers to directory and file names and 
* correspondence name length.
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
* .CE
*
* RETURNS: number of levels in path.
*
* ERRNO:
* S_dosFsLib_ILLEGAL_PATH
* S_dosFsLib_ILLEGAL_NAME
*/
LOCAL int dosVDirPathParse
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    u_char *		path,
    PATH_ARRAY_ID	pnamePtrArray
    )
    {
    FAST u_int		numPathLevels;
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
    	while( *pName != SLASH && *pName != BACK_SLASH &&
    	       *pName != EOS )
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
    } /* dosVDirPathParse() */
    
/***************************************************************************
*
* dosVDirChkSum - count checksum for long name alias.
*
* RETURNS: N/A.
*/
LOCAL u_char dosVDirChkSum
    (
    u_char *	alias
    )
    {
    u_int	i;
    u_char	chkSum;
    
    for( chkSum = 0, i = 0; i < DOS_STDNAME_LEN + DOS_STDEXT_LEN; i++ )
    	{
    	chkSum = ( ( ( chkSum &    1 ) << 7 ) |
    	    	   ( ( chkSum & 0xfe ) >> 1 )
    	    	 ) +  alias[ i ];
    	}
    return chkSum;
    } /* dosVDirChkSum() */
    
/***************************************************************************
*
* dosVDirTDDecode - decode time-date field from disk format.
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
LOCAL time_t dosVDirTDDecode
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
    } /* dosVDirTDDecode() */

/***************************************************************************
*
* dosVDirTDEncode - encode time-date to disk format.
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
LOCAL void dosVDirTDEncode
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
     * in <pDate> year is related to 1980, but in tm, - to 1900
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
    } /* dosVDirTDEncode() */
    
/***************************************************************************
*
* dosVDirCharEncode - validate and encode character.
*
* RETURNS: OK or ERROR.
*/
LOCAL STATUS dosVDirCharEncode
    (
    u_char *	pSrc,
    u_char *	pDst,
    const u_char *	codeTbl	/* characters table */
    				/* ( shortNamesChar or longNamesChar ) */
    )
    {
    /* allow all high characters */
    	
    if( *pSrc & 0x80 )
    	{
    	*pDst = *pSrc;
    	}
    else if( codeTbl[ *pSrc ] != INVALID_CHAR )
    	{
    	*pDst = codeTbl[ *pSrc ];
    	}
    else
    	{
    	return ERROR;
    	}
    
    return OK;
    } /* dosVDirCharEncode() */
    
/***************************************************************************
*
* dosVDirNameEncodeShort - encode name in short style.
*
* This routine encodes incoming file name into 8+3 uppercase format.
* During encoding the
* name is verified to be composed of valid characters.
*
* RETURNS: STRICT_SHORT, if name is valid short and uppercase,
*  NOT_STRICT_SHORT, if name is short, but not uppercase,
*  NO_SHORT, if name can not be encoded as short.
* 
*/
LOCAL SHORT_ENCODE dosVDirNameEncodeShort
    ( 
    DOS_DIR_PDESCR_ID	pDirDesc,
    PATH_ARRAY_ID	pNamePtr,	/* name buffer */
    u_char *	pDstName	/* buffer for name in disk format */
    )
    {
    u_char *	pSrc;		/* source name dynamic ptr */
    u_char *	pDstBuf;	/* for debug output only */
    int		i,j;		/* work */
    SHORT_ENCODE	retVal = STRICT_SHORT;
    /* extension length (0 - no extension) */
    u_char	extLen = pDirDesc->deDesc.extLen;
    
    pDstBuf = pDstName;
    
    bfill( (char *)pDstName,
    	   pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen, SPACE );
    
    /* encode name and check it by the way */
    
    DBG_MSG( 600, "",0,0,0,0,0,0,0,0 );
    DBG_PRN_STR( 600, "%s\n", pNamePtr->pName, pNamePtr->nameLen, 0 );

    for( i = 0, pSrc = pNamePtr->pName;
    	 i < min( pDirDesc->deDesc.nameLen,
    	 	  pNamePtr->nameLen );
    	 i++, pDstName++, pSrc++  )
    	{
    	/* check for extension */
    	
    	if( extLen != 0 && *pSrc == DOT ) 
    	    break;
    	
    	if( dosVDirCharEncode( pSrc, pDstName, shortNamesChar ) == ERROR )
    	    goto error;
    	
    	if( *pDstName != *pSrc )
    	    retVal = NOT_STRICT_SHORT;
    	}

    /* check state */
    
    if( i == pNamePtr->nameLen )
    	goto retOK;	/* name finished */
    
    if( *pSrc != DOT )	/* name too long */
        goto error;
    
    pSrc++;	/* pass DOT */
    pDstName += pDirDesc->deDesc.nameLen - i;
    i++;
    
    /* encode extension */
    
    for( j = 0; j < extLen && i < pNamePtr->nameLen;
    	 i++, j++, pDstName++, pSrc++  )
    	{
    	if( dosVDirCharEncode( pSrc, pDstName, shortNamesChar ) == ERROR )
    	    goto error;
    	if( *pDstName != *pSrc )
    	    retVal = NOT_STRICT_SHORT;
    	}
    
    /* check status */
    
    if( i < pNamePtr->nameLen )	/* extension too long */
    	goto error;

retOK:

    /* 
     * A space in a short file name is indeed technically allowed, but
     * in order to be compatible with M$ Scandisk and Norton Disk Doctor 
     * we mangle 8.3 names if they contain a space.  This will prevent 
     * ScanDisk and Norton Disk Doctor from reporting a (false) orphaned 
     * LFN entry on the alias, and offering to correct it.  Both will
     * correct it by marking the LFN invalid (fragmenting the disk...)
     * and simply using the short file name, which destroys the case
     * sensitive part of the filename.
     * Basically, this code is added to appease the windoze tools 
     * that complain about our file system, since that supposedly 
     * looks bad....but windo$e should NOT need to mangle a short
     * filename with a space....but Windows 95 OSR2 does just that..
     * Note only with "3D MAZES.SCR" style.  
     * Not with "NAME    .   " filenames.
     */

    /* check backwards through name, wont be more than 11 chars (0-10) */

    i = pNamePtr->nameLen - 1;

    while (i > 1) /* ignore 0 and 1 chars, name won't begin with space.*/
        {
	/* ignore "name   " type filenames */
	if (*(pNamePtr->pName + i) != SPACE)	
	    break;        /* found non-space char, break */ 
	i--; 
	}

    if (i != 1) /* don't care if "2   " style name */
	{
	while (i > 0) /* now look for a space */
	    {
	    if (*(pNamePtr->pName + i) == SPACE)	
	        {
		goto error; /* windoze likes it a longname */
		}
	    i--;
    	    }  
	}

    
    DBG_PRN_STR( 600, "result: %s\n", pDstBuf, 
    		 pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,0 );
    return retVal;
    
error:
    bzero( (char *)pDstBuf, DOS_DIRENT_STD_LEN );
    return NO_SHORT;
    } /* dosVDirNameEncodeShort() */
    
/***************************************************************************
*
* dosVDirNameEncode - encode name to disk format.
*
* This routine encodes incoming file name into Windows-95 long names
* format. During encoding the
* name is verified to be composed of valid characters.
* 
* The source name is
* split into several directory entries, that has valid format,
* unless alias checksum.
* Also src name is attempted to be encoded in short manner.
*
* RETURNS: directory entries per long name include alias or (-1) if name is
*  illegal.
* 
* ERRNO:
* S_dosFsLib_ILLEGAL_NAME
*/
LOCAL int dosVDirNameEncode
    ( 
    DOS_DIR_PDESCR_ID	pDirDesc,
    PATH_ARRAY_ID	pNamePtr,	/* name buffer */
    u_char *	pDstName,	/* buffer for name in disk format */
    SHORT_ENCODE *	pShortEncodeStrat /* short name encodding */
    					 /* strategy/result */	
    )
    {
    u_char *	pSrc = pNamePtr->pName;	/* source name dynamic ptr */
    u_char *	pDst;		/* ptr to encoded destination entry */
    u_char	numEnt;		/* entries per long name */
    u_char	entNum, chNum;	/* loop counters */
    
    if( pNamePtr->nameLen > DOS_VFAT_NAME_LEN )
    	goto error;
    
    /* number of entries per long name */
    
    numEnt = ( pNamePtr->nameLen + CHAR_PER_VENTRY  - 1 ) /
    	     CHAR_PER_VENTRY;
    
    /* try to encode src name as short */
    
    bzero( (char *)pDstName, DOS_DIRENT_STD_LEN );

    if( ( dosVDirNameEncodeShort( pDirDesc, pNamePtr,
    			          pDstName ) == STRICT_SHORT ) )
    	
    	{
    	if( *pShortEncodeStrat == STRICT_SHORT )
	    {
	    return 1; 
    	    }
        *pShortEncodeStrat = STRICT_SHORT;
	}
    else
    	{
    	*pShortEncodeStrat = NOT_STRICT_SHORT;
    	}
    	
    
    bcopy( (char *)pDstName, (char *)pDstName + numEnt * DOS_DIRENT_STD_LEN,
           DOS_DIRENT_STD_LEN );

    bzero( (char *)pDstName, numEnt * DOS_DIRENT_STD_LEN );
    			    
    /* start encoding from bottom (first) entry */
    
    pDst = pDstName + (numEnt - 1) * DOS_DIRENT_STD_LEN;
    for( entNum = 1;
    	 entNum <= numEnt;
    	 entNum ++, pDst -=  DOS_DIRENT_STD_LEN )
    	{
    	*pDst = entNum;	/* encode entry number */
    	*(pDst + pDirDesc->deDesc.atrribOff) = DOS_ATTR_VFAT;
    				/* VFAT long name representation */
    	
    	/* encode characters */
    	
    	for( chNum = 0;
    	     chNum < CHAR_PER_VENTRY &&
    	     pSrc < pNamePtr->pName + pNamePtr->nameLen;
    	     pSrc ++, chNum ++ )
    	    {
    	    if( dosVDirCharEncode( pSrc, pDst + chOffsets[ chNum ],
    	    			   longNamesChar ) == ERROR )
    	    	{
    	    	goto error;
    	    	}
    	    }
    	
        /* fill extra characters in last entry with 0xff */
    	
    	for( chNum ++; chNum < CHAR_PER_VENTRY; chNum ++ )
    	    {
    	    assert( pSrc >= pNamePtr->pName + pNamePtr->nameLen );
    	    
    	    *(pDst + chOffsets[ chNum ]) = 0xff;
    	    *(pDst + chOffsets[ chNum ] + 1) = 0xff;
    	    }
    	}
    
    /* mark last entry */
    
    pDst += DOS_DIRENT_STD_LEN;
    *pDst |= DOS_VLAST_ENTRY;
        
    return numEnt + 1;

error:
    errnoSet( S_dosFsLib_ILLEGAL_NAME );
    return (-1);
    } /* dosVDirNameEncode() */
    
/***************************************************************************
*
* dosVDirClustNext - get next or add and init cluster to directory.
*
* RETURNS: OK or ERROR if end of directory is reached or
* no more cluster could be allocated or disk is full.
*/
LOCAL STATUS dosVDirClustNext
    (
    DOS_FILE_DESC_ID	pFd,
    u_int	alloc	/* 0 or DH_ALLOC */
    )
    {
    block_t	sec;	/* work count */
    
    /* get next/allocate cluster */
    
    alloc = ( alloc == 0 ) ? FAT_NOT_ALLOC : FAT_ALLOC_ONE;

    if( pFd->pVolDesc->pFatDesc->getNext( pFd, alloc ) == ERROR )
    	return ERROR;
    
    assert( pFd->pFileHdl->startClust != 0 );
    
    if( alloc == FAT_NOT_ALLOC )
    	return OK;
    
    /* init cluster */

    for( sec = pFd->curSec; sec < pFd->curSec + pFd->nSec; sec ++ )
    	{
    	if( cbioIoctl(pFd->pVolDesc->pCbio,
			CBIO_CACHE_NEWBLK, (void *)sec ) == ERROR )
    	    {
    	    return ERROR;
    	    }
    	}
        
    return (OK);
    } /* dosVDirClustNext() */

/***************************************************************************
*
* dosVDirDirentGet - get bundle directory entry from disk.
*
* This routine reads bundle directory entry from disk. 
* On this level each entry containing part of long name is accepted
* independent of others.
*
* <which> argument defines which entry to get.
* This routine can be
* used for readdir (<which> = RD_FIRST/RD_CURRENT/RD_NEXT),
* in that case <pFd> describes the
* directory is being read, or for getting directory entry
* corresponding to <pFd> (<which> = FD_ENTRY).
*
* RETURNS: OK or ERROR if directory chain end reached or
*  disk access error.
*/
LOCAL STATUS dosVDirDirentGet
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
    	dosVDirRewindDir( pFd );	/* rewind directory */
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

    /* read directory entry */
    
    if( cbioBytesRW(workFd.pVolDesc->pCbio, workFd.curSec,
    		    OFFSET_IN_SEC( workFd.pVolDesc, workFd.pos ),
		    (addr_t)pDirEnt, dirEntSize, CBIO_READ,
    		    &workFd.cbioCookie ) == ERROR )
    	{
    	return ERROR;
    	}
    DBG_PRN_STR( 600, "entry: %s\n", pDirEnt,
    		 pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,0 );
    		 
    return OK;
    } /* dosVDirDirentGet() */

/***************************************************************************
*
* dosVDirDeStore - store some contiguous fields of directory entry.
*
* This routine stores <nBytes> pointed by <pData> starting
* from offset <offset> in directory entry is currently
* pointed by <pFd> or next to this one, depending on
* the <which> argument.
* 
* <which> can be one of PUT_CURRENT or PUT_NEXT bitwise or-ed with
* DH_ALLOC, if new directory entry is being created.
*
* RETURNS: OK or ERROR if write error occurred.
*/
LOCAL STATUS dosVDirDeStore
    (
    DOS_FILE_DESC_ID	pFd,	/* directory descriptor */
    off_t	offset,	/* offset in directory entry */
    size_t	nBytes,	/* num of bytes to write */
    void *	pData,	/* ptr to data buffer */
    u_int	which	/* (PUT_CURRENT or PUT_NEXT) ored with */
    			/* DH_ALLOC */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    
    if( (which & PUT_NEXT) != 0 )
    	{
    	pFd->pos += DOS_DIRENT_STD_LEN;

    	if( OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ) == 0 )
    	    {
    	    pFd->curSec ++;
    	    pFd->nSec --;
    	    }
    	}
    
    if( pFd->nSec == 0 )	/* current cluster group exhausted */
    	{
    	if( dosVDirClustNext( pFd, (which & DH_ALLOC) ) == ERROR )
    	    return ERROR;
    	}
    
    /* store data */
    
    if( cbioBytesRW( pVolDesc->pCbio, pFd->curSec,
                     OFFSET_IN_SEC( pVolDesc, pFd->pos ) + offset,
    		pData, nBytes, CBIO_WRITE, &pFd->cbioCookie ) == ERROR )
    	{
    	return ERROR;
    	}
    
    return OK;
    } /* dosVDirDeStore() */

/***************************************************************************
*
* dosVDirEntryDel - mark long name as deleted.
* 
* This routine marks <nEnt> traditional directory entries as deleted.
* Entries are started from position is pointed by <pLnPtr>.
*
* RETURNS: OK or ERROR if disk access failed.
*/
LOCAL STATUS dosVDirEntryDel
    (
    DOS_VOLUME_DESC_ID	pVolDesc,/* dos volume descriptor */
    DIRENT_PTR_ID	pLnPtr,	/* long name start ptr */
    u_int		nEnt,	/* number of entries to mark as deleted */
    				/* 0 - to accept the number from disk */
    BOOL		always	/* delete independent of is check disk */
    				/* in progress or not */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc;
    DOS_FILE_DESC	workFd = {0};	/* temporary file descriptor */
    DOS_FILE_HDL	workFileHdl; /* temporary file handle */
    u_char	dirent[ DOS_DIRENT_STD_LEN ]; /* directory entry buffer */
    cookie_t	cookie = (cookie_t) NULL;		/* temp buffer */
    u_int	which;
    
    if( ! always && pVolDesc->chkLevel <= DOS_CHK_ONLY )
    	return OK;

#if TRUE /* SPR#70968 fix enabled */
    /* 
     * 01o,09nov01,jkf  SPR#70968, chkdsk destroys boot sector
      *
     * When chkdsk() is in repair mode(DOS_CHK_REPAIR): it can
     * destroy the boot sector's first byte (sector 0, offset 0) 
     * by writing DOS_DEL_MARK(0xe5). The cause of the problem is
     * function dosVDirEntryDel() lacks a sanity check for
     * pLnPtr->sector being zero.
     *
     * Workaround:
     * 
     * Add an "if" statement to check if pLnPtr is illegal: 
     * 
     * if( ! always && pVolDesc->chkLevel <= DOS_CHK_ONLY )
     *     return OK;    
     *
     * /@ SPR#70968: check if pLnPtr is illegal @/
     *
     * if ( 0 == pLnPtr->sector ) 
     *     return ERROR; 
     */

    if ( 0 == pLnPtr->sector ) 
        {
        /* return ERROR if long name start pointer is from 0 sector */
        return (ERROR);
        }
#endif /* SPR#70968 fix enabled */

    pDirDesc = (void *)pVolDesc->pDirDesc;

    bzero( (char *)&workFileHdl, sizeof( workFileHdl ) );

    workFd.pVolDesc = pVolDesc;

    workFd.pFileHdl = &workFileHdl;

    workFd.curSec = pLnPtr->sector;

    workFd.pos = pLnPtr->offset;

    workFileHdl.startClust = NONE;	/* don't care */
    
    if( workFd.curSec < pVolDesc->dataStartSec )
    	{	/* old root */
    	workFd.nSec = pVolDesc->dataStartSec - workFd.curSec;
    	}
    else if( pVolDesc->pFatDesc->seek(
    				&workFd, workFd.curSec, 0 ) == ERROR )
        {
        assert( FALSE );
        return ERROR;
        }
    
    /* get number of entries per long name */
    
    if( nEnt == 0 )
    	{
        if( cbioBytesRW(pVolDesc->pCbio, workFd.curSec,
		OFFSET_IN_SEC( pVolDesc, workFd.pos ),
    		(addr_t)dirent, DOS_DIRENT_STD_LEN, 
		CBIO_READ, &cookie ) == ERROR )
    	    {
    	    return ERROR;
    	    }
    	
    	assert( dirent[ pDirDesc->deDesc.atrribOff ] == DOS_ATTR_VFAT );
    	
    	nEnt = ( *dirent & VFAT_ENTNUM_MASK ) + 1 /* include alias */;
    	}
    
    /* mark all directory entries */
    
    *dirent = DOS_DEL_MARK;
    for( which = PUT_CURRENT; nEnt > 0; nEnt --, which = PUT_NEXT )
    	{
    	if( dosVDirDeStore( &workFd, 0, 1, &dirent, which ) == ERROR )
    	    {
    	    return ERROR;
    	    }
    	}
    
    return OK;
    } /* dosVDirEntryDel() */
    
/***************************************************************************
*
* dosVDirFullEntGet - get VFAT full directory entry from disk.
*
* This routine reads entry out of directory pointed by <pFd>. 
* It puts into buffer <pEntry> full long name and alias.
* Invalid entries (with inconsistent checksum and so on)
* are usually passed, but deleted if check disk is in progress.
*
* <which> argument defines which entry to get and may be
* RD_FIRST, RD_CURRENT and RD_NEXT.
*
* RETURNS: OK or ERROR if directory chain end reached.
*/
LOCAL STATUS dosVDirFullEntGet
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    u_char *		pEntry,
    RDE_OPTION	 	which,	/* which entry to get */
    DIRENT_PTR_ID	pLnPtr,	/* to fill with long name start ptr */
    u_int *		nEntries /* entries counter in directory */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    STATUS	status = ERROR;
    u_char *	pDst = pEntry;	/* current destination position */
    int	entNum = NONE;		/* expected sequence number of */
    				/* current entry per long name */
    int	numEnt = 0;		/* number of entries per long name */
    u_char	chkSum = 0;	/* alias checksum */
    u_char	atrribOff = pDirDesc->deDesc.atrribOff;
    
    assert( nEntries != NULL );
    
    pLnPtr->sector = pLnPtr->offset = 0;

    for( (status = dosVDirDirentGet( pFd, pDst, which ));
    	 status != ERROR;
    	 (status = dosVDirDirentGet( pFd, pDst, RD_NEXT )) )
    	{
    	if( *pDst != LAST_DIRENT )
    	    (*nEntries)++;
    	
    	/*
    	 * return just a short name, volume label, deleted
    	 * or last entry in directory
    	 */

    	if( (entNum == NONE && *(pDst + atrribOff ) != DOS_ATTR_VFAT) ||
    					/* just a short || */
    	    *pDst == DOS_DEL_MARK ||	/* deleted || */
    	    *pDst == LAST_DIRENT ||	/* last */
    	    ((*(pDst + atrribOff ) & DOS_ATTR_VOL_LABEL) != 0 &&
    	     (*(pDst + atrribOff ) != DOS_ATTR_VFAT)) ) /* vol label */
    	    {
    	    bcopy( (char *)pDst, (char *)pEntry, DOS_DIRENT_STD_LEN );
    	    goto retShort;
    	    }
    	
    	/* === long name representation === */
    	
    	if( *(pDst + atrribOff ) == DOS_ATTR_VFAT )
    	    {
    	    /* last (top) entry per long name */
    	    
    	    /* 
    	     * TBD:
    	     * Windows checks characters following EOS in last
	     * per long name entry to be 0xff, so
    	     * some entries, that are accepted by this library
    	     * can be ignored as erroneous by Win.
    	     */
    	    if( (*pDst & DOS_VLAST_ENTRY) != 0 )
    	    	{
		/* check for erroneous long names */
    	    	
    	    	if( entNum != NONE ) /* long name already started */
    	    	    {
    	    	    ERR_MSG( 10, "Bad long name structure (breached))\n",
			     0,0,0,0,0,0 );
    	    	    DBG_MSG(0, "Long name interrupted on ent %u by "
    	    	                "other long name at sec %u, off %u\n",
    	    	            entNum, pFd->curSec,
                            OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ),
                            0,0,0,0,0 );
    	    	    
    	    	    /* remove invalid entries ( if check disk in progress */
    	    	    
    	    	    assert( numEnt != 0 );
    	    	    dosVDirEntryDel( pFd->pVolDesc, pLnPtr, numEnt - entNum,
    	    	    		     FALSE );
    	    	    
    	    	    /* let now deal with this name */
    	    	    
    	    	    bcopy( (char *)pDst, (char *)pEntry, 
    	    	           DOS_DIRENT_STD_LEN );
    	    	    pDst = pEntry;
    	    	    }
    	    	
    	    	numEnt = entNum = *pDst & VFAT_ENTNUM_MASK;
    	    	if( entNum > VFAT_MAX_ENT )
    	    	    {
    	    	    ERR_MSG( 10, "Bad long name structure (too long)\n",
			     0,0,0,0,0,0 );
    	    	    DBG_MSG( 0, "max number of entries per long name (%u) "
    	    	                 " overloaded (%u); (sec %u, off %u)\n",
    	    	                 VFAT_MAX_ENT, entNum,
    	    	                 pFd->curSec,
                                 OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ),
                                 0,0,0,0);
    	    	    goto errCont;
    	    	    }
    	    	chkSum = *(pDst + DOS_VFAT_CHKSUM_OFF);
    	    	pLnPtr->sector = pFd->curSec;
    	    	pLnPtr->offset = pFd->pos;
    	    	}
    	    else if( entNum == NONE ) /* no long name started yet */
    	    	{
    	    	DBG_MSG( 0, "Long name internal entry without first one "
    	    		     "( sec %u, off %u )\n",
    	    			pFd->curSec,
                                OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ),
                                0,0,0,0,0,0 );
    	    	goto errCont;
    	    	}
    	    else if( entNum != (*pDst & VFAT_ENTNUM_MASK) )
    	    	{
    	    	ERR_MSG( 10, "Malformed long name structure\n",
			 0,0,0,0,0,0 );
    	    	DBG_MSG( 0, "invalid entry num in long name "
    	    		     "internal entry (start: sec %u, off %u; "
    	    		     "current: sec %u, off %u)\n",
    	    		     pLnPtr->sector, pLnPtr->offset,
    	    		     pFd->curSec,
			     OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ),
			     0,0,0,0);
    	    	goto errCont;
    	    	}
    	    else /* one more entry in long name; check entry number */
    	    	{
    	    	/* control checksum */
    	    
    	    	if( chkSum != *(pDst + DOS_VFAT_CHKSUM_OFF) )
    	    	    {
    	    	    ERR_MSG( 10, "Malformed long name structure\n",
			     0,0,0,0,0,0 );
	    	    DBG_MSG( 0, "ChkSum changed in long name internal "
    	    		     "entry (start: sec %u, off %u; "
    	    		     "current: sec %u, off %u)\n",
    	    		     pLnPtr->sector, pLnPtr->offset,
    	    		     pFd->curSec,
			     OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ),
			     0,0,0,0);
    	    	    goto errCont;
    	    	    }
    	    	} /* else */
    	    	
    	    /* expected next entry of long name */
    	    	
    	    entNum --;	/* next expected */
    	    pDst += DOS_DIRENT_STD_LEN; 
    	    continue;	/* get the next entry in this long name */
    	    } /* if( *(pDst + atrribOff ) == DOS_ATTR_VFAT ) */
    	
	/* = long name alias encountered; count and control checksum = */
    	
    	assert( entNum != NONE );
    	
    	if( entNum != 0 || dosVDirChkSum( pDst ) != chkSum )
    	    {
    	    /*
    	     * alias appeared before long name finished or
    	     * checksum error.
    	     * It is abnormal situation, but in fact the short name
	     * may be OK. So return the short name.
    	     */
    	    ERR_MSG( 10, "Bad long name structure\n", 0,0,0,0,0,0 );
    	    DBG_MSG( 0, "start: sec %u, off %u \n",
    	    		 pLnPtr->sector, pLnPtr->offset, 0,0,0,0,0,0 );
    	    
    	    goto retShort;

#if FALSE /* HELP dead code which followed above goto */
    	    /* remove invalid entries ( if check disk in progress */
    	    dosVDirEntryDel( pFd->pVolDesc, pLnPtr, numEnt - entNum, FALSE );
    	    bcopy( (char *)pDst, (char *)pEntry, DOS_DIRENT_STD_LEN );
    	    goto ret;
#endif 
    	    }
    	
    	/* full long name and alias extracted */
    	
    	goto ret;
    	
errCont:
    	/* remove invalid entries ( if check disk in progress */
    	    
    	dosVDirEntryDel( pFd->pVolDesc, pLnPtr, numEnt - entNum,
    	    	    	 FALSE );
    	    
    	entNum = NONE;
    	pDst = pEntry;
    	pLnPtr->sector = pLnPtr->offset = 0;
    	} /* for( (status ... */
    
ret:
    return status;

retShort:

    if( entNum != NONE )
    	{
    	ERR_MSG(10, "Erroneous long name\n", 0,0,0,0,0,0);
    	DBG_MSG(0, "ent %u at sec %u, off %u\n",
    	    	    entNum, pFd->curSec,
		    OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos ),
		    0,0,0,0,0);
    	    	    
    	/* remove invalid entries ( if check disk in progress ) */
    	    	    
        assert( numEnt != 0 );
    	dosVDirEntryDel( pFd->pVolDesc, pLnPtr, numEnt - entNum,
    	    	    	 FALSE );
    	bcopy( (char *)pDst, (char *)pEntry, DOS_DIRENT_STD_LEN );
    	}
    pLnPtr->sector = pLnPtr->offset = 0; /* short name */
    return OK;
    } /* dosVDirFullEntGet() */
    
/***************************************************************************
*
* dosVDirNameCmp - compare long names.
*
* This routine compares long names' directory entries.
*
* If <caseSens> is TRUE case sensitive comparison is performed.
*
* RETURNS: OK if names are identical or ERROR.
*/
LOCAL STATUS dosVDirNameCmp
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    u_char *	pName,		/* incoming name */
    u_char *	pDiskName,	/* name to compare with */
    BOOL	caseSens	/* lkup case sensitively */
    )
    {
    int	entNum 	= (*pName & VFAT_ENTNUM_MASK);
    int	i 	= 0;	/* loop counter */
    
    if( entNum != (*pDiskName & VFAT_ENTNUM_MASK) )
    	return ERROR;
    
    for( ; entNum > 0; pName += DOS_DIRENT_STD_LEN, 
    		       pDiskName += DOS_DIRENT_STD_LEN, entNum -- )
    	{
    	for( i = 0; i < CHAR_PER_VENTRY; i++ )
    	    {
    	    if( caseSens )
    	        {
    	        if( pName[ chOffsets[ i ] ] !=
    	            pDiskName[ chOffsets[ i ] ] )
    	    	    {
    	    	    return ERROR;
    	    	    }
    	    	}
    	    else if( toupper( pName[ chOffsets[ i ] ] ) !=
    	    	     toupper( pDiskName[ chOffsets[ i ] ] ) )
    	    	{
    	    	return ERROR;
    	    	}

    	    if( pName[ chOffsets[ i ] ] == 0 )
    	    	break;
    	    }
    	} /* for( ; entNum ... */
    return OK;
    } /* dosVDirNameCmp() */
    
/***************************************************************************
*
* dosVDirLkupInDir - lookup directory for specified name.
*
* This routine searches directory, that is pointed by <pFd> for name, that
* is pointed by <pNamePtr> structure and fills <pFd> in accordance with
* directory entry data, if found.
*
* If name not found, <pFreeEnt> will be filled with pointer onto
* deleted entry in directory large enough to create entry
* with name is being searched for, or onto free space in last directory
* cluster. If both of them not found, <pFreeEnt->sector> is set to 0.
*
* If <caseSens> is TRUE the name is searched case sensitively.
*
* RETURNS: OK or ERROR if name not found or invalid name.
*/
LOCAL STATUS dosVDirLkupInDir
    (
    DOS_FILE_DESC_ID	pFd,	/* dos file descriptor to fill */
    PATH_ARRAY_ID	pNamePtr,	/* name buffer */
    DIRENT_PTR_ID	pFreeEnt,	/* empty entry in directory */
    int			*pFreeEntLen,	/* Return numbers of free chanks */
    BOOL		caseSens	/* lkup case sensitively */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    u_char *	pName = pDirDesc->nameBuf; /* src name in disk format */
    u_char *	pNameAl;	/* name alias in <pLName> buffer */
    u_char *	pDiskName = pDirDesc->nameBuf +
    			    MAX_VFAT_FULL_DIRENT;
    			    		/* directory entry buffer */
    STATUS	status = ERROR;	/* search result status */
    u_int	nEntries;	/* number of entries in directory */
    size_t	freeChankLen = 0; /* num of contiguous deleted entries */
    DIRENT_PTR	lnPtr = {0};	/* ptr to long name start on disk */
    u_int	which;
    SHORT_ENCODE	shortNameEncode; /* encodding of short name */
    short	aliasOff = 0;	/* offset of alias in disk name buffer */
    short	nEntInName;	/* number of entries per name, */
    				/* if will be created  */
    
    *pFreeEntLen = 0; /* No free entries to start with */
    
    DBG_MSG( 400, "pFd = %p ", pFd,0,0,0,0,0,0,0 );
    DBG_PRN_STR( 400, " name: %s\n", pNamePtr->pName,
    			pNamePtr->nameLen, 0 );
    
    pFreeEnt->deNum = 0;
    pFreeEnt->sector = 0;
    pFreeEnt->offset = 0;
    
    /* protect buffers */
    
    if( semTake( pDirDesc->bufSem, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /* prepare name */
    
    shortNameEncode = NOT_STRICT_SHORT;
    nEntInName = dosVDirNameEncode( pDirDesc, pNamePtr, pName,
    				    &shortNameEncode );
    if( nEntInName == (short)ERROR )
    	goto ret;
    
    aliasOff = DOS_DIRENT_STD_LEN * (nEntInName - 1);
    pNameAl = pName + aliasOff;
    
    /*
     * strict short name does not require extra long entries.
     *
     * Do not compare short alias, if case sensitive 
     * lkup in progress for name, that contains lower case characters
     */
    if( shortNameEncode ==  STRICT_SHORT )
    	nEntInName = 1;
    else if( caseSens )
    	*pNameAl = EOS;
    
    nEntries = 0;

    for( which = RD_FIRST; 1; which = RD_NEXT )
    	{
    	status = dosVDirFullEntGet( pFd, pDiskName,
    	                            which, &lnPtr, &nEntries );
    	if( status == ERROR || *pDiskName == LAST_DIRENT )
    	    break;
    	
    	/* pass deleted entry, that later can be used to store new entry */
    	
    	if( *pDiskName == DOS_DEL_MARK )
    	    {
    	    if( freeChankLen == 0 )
    	    	{
    	    	pFreeEnt->deNum = nEntries;
                pFreeEnt->sector = pFd->curSec; /* new empty chain */
    	    	pFreeEnt->offset = pFd->pos;
    	    	}
    	    freeChankLen ++;
    	    continue;
    	    }
    	
        /* non empty entry interrupts empty chain */
    	
    	if( freeChankLen < (size_t)nEntInName )
    	    freeChankLen = 0;
    	 
    	/* long name entry */
    	
    	if( *(pDiskName + pDirDesc->deDesc.atrribOff) == DOS_ATTR_VFAT )
    	    {
    	    /* compare long names */
    	    
    	    if( dosVDirNameCmp( pDirDesc, pName, pDiskName, caseSens ) == OK )
    	        {
    	    	goto ret;
    	    	}
    	    
    	    continue;
    	    }
    	
        /* pass volume label */
    	
    	if( ( *(pDiskName + pDirDesc->deDesc.atrribOff) & 
    	      DOS_ATTR_VOL_LABEL ) != 0 )
    	    {
    	    continue;
    	    }
    	    	
    	/* compare short names, that does not have long version */
    	
    	if( bcmp( (char *)pNameAl, (char *)pDiskName,
    		  DOS_STDNAME_LEN + DOS_STDEXT_LEN ) == 0 )
    	    {
    	    aliasOff = 0;
    	    goto ret;
    	    }
    	}
    
    /* if no appropriate free space found, reset pointers */
    
    if( freeChankLen < (size_t)nEntInName )
    	{
    	pFreeEnt->deNum = nEntries + ((status == ERROR)? 1 : 0);
    	pFreeEnt->sector = 0;
    	pFreeEnt->offset = 0;
    	}
    	
    /* check result */
    
    if( status == ERROR )
    	{
    	goto ret;
    	}
    status = ERROR;
    
    /* *pDiskName == LAST_DIRENT */
    
    if( IS_ROOT( pFd ) &&
    	nEntries + nEntInName > pDirDesc->rootMaxEntries )
        {
    	goto ret;
    	}
    
    /* will create new entry on this place, if required */
    
    if( pFreeEnt->sector == 0 )
    	{
    	pFreeEnt->deNum = nEntries;
    	pFreeEnt->sector = pFd->curSec;
    	pFreeEnt->offset = pFd->pos;
    	}

ret:

    if( status != ERROR ) /* file found; fill file descriptor */
    	{
    	dosVDirFillFd( pFd, pDiskName + aliasOff, &lnPtr );
    	}

    *pFreeEntLen = freeChankLen; /* Return numbers of free ents */
    
    semGive( pDirDesc->bufSem );
    return status; 
    } /* dosVDirLkupInDir() */
    
/***************************************************************************
*
* dosVDirAliasCreate - create alias for long file name.
*
* This routine takes 1 first byte of file name and expands it
* with ~ and 6 decimal digits of sequence number of alias's
* entry in directory. Then it takes three characters of long name
* following last dot symbol and puts them to alias extension.
*
* RETURNS: N/A.
*/
LOCAL void dosVDirAliasCreate
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    PATH_ARRAY_ID	pNamePtr,
    u_char *	pAlias,	/* dst alias */
    u_int	entNum	/* empty entry in directory */
    )
    {
    u_char *	pSrc;	/* loop pointer to src name */
    u_char *	pDst;	/* loop pointer to dst alias */
    char	entNumBuf[ DOS_STDNAME_LEN + DOS_STDEXT_LEN + 1 ];
    
    entNum =  1000000 - entNum % 1000000;
    
    bfill( (char *)pAlias, DOS_STDNAME_LEN + DOS_STDEXT_LEN, SPACE );
    
    pSrc = pNamePtr->pName;
    pDst = pAlias;
    
    /* pass top dots in name */
    
    for( ; *pSrc == DOT; pSrc ++ );
    assert( *pSrc != EOS );

    for( ; *pSrc != DOT  &&
           pSrc != pNamePtr->pName + pNamePtr->nameLen &&
           pDst != pAlias + DOS_STDNAME_LEN;
    	 pSrc++, pDst++ )
    	{
    	if( dosVDirCharEncode( pSrc, pDst, shortNamesChar ) == ERROR )
    	    break;
    	}
    	 
    /* encode directory entry number */
    
    sprintf( entNumBuf, "%c%u", TILDA, entNum );
    entNum = strlen( entNumBuf );
    
    if( pDst > pAlias + DOS_STDNAME_LEN - entNum )
    	pDst = pAlias + DOS_STDNAME_LEN - entNum;
    
    bcopy( entNumBuf, (char *)pDst, entNum );
    
    /* === extension === */
    
    pDst = pAlias + DOS_STDNAME_LEN;
    
    /* find last DOT */
    
    for( pSrc = pNamePtr->pName + pNamePtr->nameLen - 1;
         pSrc != pNamePtr->pName && *pSrc != DOT; pSrc -- );
    
    /* encode 3 extension characters */
    
    if( pSrc == pNamePtr->pName )
    	return;	/* no extension */
    
    /* *pSrc == DOT */
    
    for( pSrc++;
    	 pDst != pAlias + DOS_STDNAME_LEN + DOS_STDEXT_LEN &&
    	 pSrc != pNamePtr->pName + pNamePtr->nameLen;
    	 pSrc++, pDst++ )
    	{
    	if( dosVDirCharEncode( pSrc, pDst, shortNamesChar ) == ERROR )
    	    {
    	    *pDst = SPACE;
    	    break;
    	    }
    	}
    DBG_PRN_STR( 700, "result: %s\n", pAlias, 
    		 pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,0 );
    } /* dosVDirAliasCreate() */



#ifdef	__unused__
/***************************************************************************
*
* dosVDirFullDeUpdate - update fields in long name directory entries.
*
* This routine deletes full long name entry or updates.
*
* RETURNS: OK or ERROR if disk write error occurred.
*/
LOCAL STATUS dosVDirFullDeUpdate
    (
    DOS_FILE_DESC_ID	pFd,	/* directory descriptor */
    u_int	operation	/* DH_DELETE */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    DOS_FILE_DESC	workFd = {0};	/* temporary file descriptor */
    DOS_FILE_HDL	workFileHdl; /* temporary file handle */
    UINT32	value = 0; /* start cluster number in disk format */
    STATUS	st = ERROR;
    u_char	nEnt;	/* entries per long name */
    u_char	fieldOff = 0, fieldLen = 0;
    
    if( operation == DH_DELETE )
    	{
    	fieldOff = 0;
    	fieldLen = 1;
    	*(u_char *)&value = DOS_DEL_MARK;
    	}
    else
    	{
    	assert( FALSE );
    	}

    /* init file descriptor */
    
    bzero( (char *)&workFileHdl, sizeof( workFileHdl ) );
    workFd.pVolDesc = pVolDesc;
    workFd.pFileHdl = &workFileHdl;
    workFd.pFileHdl->startClust = 
    			pFd->pFileHdl->dirHdl.parDirStartCluster;
    
    /* get number of entries per long name */
    
    if( pFd->pFileHdl->dirHdl.lnSector == 0 )
    	{	/* short name only */
    	nEnt = 0;
    	workFd.curSec = pFd->pFileHdl->dirHdl.sector;
    	workFd.pos = pFd->pFileHdl->dirHdl.offset;
    	}
    else	/* long name */
    	{
    	workFd.curSec = pFd->pFileHdl->dirHdl.lnSector;
    	workFd.pos = pFd->pFileHdl->dirHdl.lnOffset;
    	
        if( cbioBytesRW(
    		pVolDesc->pCbio, workFd.curSec,
		OFFSET_IN_SET( pVolDesc, workFd.pos ),
    		&nEnt, 1, CBIO_READ, NULL ) == ERROR )
    	    {
    	    return ERROR;
    	    }
        if( (nEnt & DOS_VLAST_ENTRY) == 0 )
    	    {
    	    ERR_MSG(10, "Long name terminating flag missing\n", 0,0,0,0 );
    	    return ERROR;
    	    }
    	}    
    
    if( workFd.curSec < pVolDesc->dataStartSec )
    	{	/* old root */
    	workFd.nSec = pVolDesc->dataStartSec - workFd.curSec;
    	}
    else if( pVolDesc->pFatDesc->seek(
    				&workFd, workFd.curSec, 0 ) == ERROR )
        {
        assert( FALSE );
        return ERROR;
        }
    
    /* update all directory entries */
    
    for( nEnt = (nEnt & VFAT_ENTNUM_MASK),
         (st = dosVDirDeStore( &workFd, fieldOff, fieldLen, &value,
         		       PUT_CURRENT ));
         st != ERROR && nEnt > 0; nEnt -- )
    	{
    	st = dosVDirDeStore( &workFd, fieldOff, fieldLen, &value,
         		     PUT_NEXT );
         }
    return st;
    } /* dosVDirFullDeUpdate() */

#endif /* __unused__ */

/***************************************************************************
*
* dosVDirUpdateEntry - set new directory entry contents.
*
* This routine pulls file size, attributes and so on out of
* file descriptor and stores it in the correspondence directory
* entry.
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
* RETURNS: OK or ERROR if name is invalid or disk access error occurred.
*
*/
LOCAL STATUS dosVDirUpdateEntry
    (
    DOS_FILE_DESC_ID	pFd,
    u_int		flags,
    time_t		curTime		/* time to encode */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    u_char	dirent[ DOS_DIRENT_STD_LEN ]; /* directory entry buffer */
    
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
    
    if( flags & DH_DELETE )	/* delete entry */
    	{
    	DIRENT_PTR	dePtr;
    	u_int	nEnt = 0;
    	
    	if( pFd->pFileHdl->dirHdl.lnSector == 0 )
    	    {	/* short name only */
    	    dePtr.sector = pFd->pFileHdl->dirHdl.sector;
    	    dePtr.offset = pFd->pFileHdl->dirHdl.offset;
    	    nEnt = 1;
    	    }
    	else	/* long name */
    	    {
    	    dePtr.sector = pFd->pFileHdl->dirHdl.lnSector;
    	    dePtr.offset = pFd->pFileHdl->dirHdl.lnOffset;
    	    }
    	    
    	return dosVDirEntryDel( pFd->pVolDesc, &dePtr, nEnt, TRUE );
    	}
    
    /* get directory entry */
    
    if( dosVDirDirentGet( pFd, dirent, FD_ENTRY ) == ERROR )
    	return ERROR;
    
    /*
     * encode other fields, but also start cluster, because entire
     * directory entry will be stored
     */
    START_CLUST_ENCODE( &pDirDesc->deDesc,
    		        pFd->pFileHdl->startClust,  dirent );
    dirent[ pDirDesc->deDesc.atrribOff ] = pFd->pFileHdl->attrib;
    VX_TO_DISK_32( pFd->pFileHdl->size,
    		   dirent + pDirDesc->deDesc.sizeOff );
    if( pDirDesc->deDesc.extSizeOff != (u_char) NONE )
    	{
    	EXT_SIZE_ENCODE( &pDirDesc->deDesc, dirent, pFd->pFileHdl->size );
    	}
    
    dosVDirTDEncode( pDirDesc, dirent, flags, curTime );
    
    /* store directory entry */

    return cbioBytesRW( pFd->pVolDesc->pCbio, pFd->pFileHdl->dirHdl.sector,
    		OFFSET_IN_SEC( pFd->pVolDesc, pFd->pFileHdl->dirHdl.offset ),
		(addr_t)dirent, pDirDesc->deDesc.dirEntSize, CBIO_WRITE,
    		&pFd->pFileHdl->dirHdl.cookie );
    } /* dosVDirUpdateEntry */


/***************************************************************************
*
* dosVDirFileCreateInDir - create new entry in directory.
*
* This routine creates new directory entry in place of
* deleted entry. If no deleted entry large enough found in directory,
* additional entry created at the directory tail.
*
* <pFreeEnt> must contain a pointer into an appropriate deleted entry in
* the directory or into free space in bottom of the directory.
* If <pFreeEnt->sector> eq. to 0 indicates,
* that no free entries found in directory and
* <pFd> reached directory end. In this case one more cluster is
* added to directory chain and new file is created in it.
*
* RETURNS: OK or ERROR if new entry can not be created.
*
* ERRNO:
* S_dosFsLib_DIR_READ_ONLY
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_ROOT_DIR_FULL
* 
*/
LOCAL STATUS dosVDirFileCreateInDir
    (
    DOS_FILE_DESC_ID	pFd,	/* directory descriptor */
    PATH_ARRAY_ID	pNamePtr,
    u_int		options,	/* creat flags */
    DIRENT_PTR_ID	pFreeEnt,	/* empty entry in directory */
    int			freeEntLen	/* Number of contig empty entries in directory */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    DOS_DIR_HDL_ID	pDirHdl = (void *)&(pFd->pFileHdl->dirHdl);
    cookie_t	cookie;		/* temp buffer */
    u_char *	dirent = pDirDesc->nameBuf; /* directory entry buffer */
    u_char *	alias;		/* long name alias */
    u_int	whichEntry;
    u_int	allocFlag;
    int numEnt;	/* directory entries per full VFAT entry */
    int i;
    SHORT_ENCODE	shortNameEncode = STRICT_SHORT;
    				/* encodding of short name strategy */
    STATUS	retStat = ERROR;
    u_char	chkSum = 0;	/* alias checksum */
    u_char	parentIsRoot = 0;	/* creation in root */
    
    /* check permissions */
    
    if( pFd->pFileHdl->attrib & DOS_ATTR_RDONLY )
    	{
    	errnoSet( S_dosFsLib_DIR_READ_ONLY );
    	return ERROR;
    	}
    	
    /* only file and directory can be created */
    
    if( !( S_ISREG( options ) || S_ISDIR( options ) ) )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}
    
    if( IS_ROOT( pFd ) )
    	parentIsRoot = 1;
    
    /* protect buffers */
    
    if( semTake( pDirDesc->bufSem, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /* check/encode file name */
    
    numEnt = dosVDirNameEncode( pDirDesc, pNamePtr, dirent,
    				&shortNameEncode );
    if( numEnt == ERROR )
    	goto ret;
    
    /* set alias pointer */
    
    alias = dirent + ( numEnt - 1 ) * DOS_DIRENT_STD_LEN;
    
    /*
     * if name can not be directly encoded as short,
     * create special alias. 
     */

    
    if ( *alias ==  EOS ) 
    	{
    	assert( alias != dirent );
    	dosVDirAliasCreate(
    			pDirDesc, pNamePtr, alias, pFreeEnt->deNum + numEnt );
    	}

    /* encode time fields */
    
    dosVDirTDEncode( pDirDesc, alias,
    		     DH_TIME_CREAT | DH_TIME_MODIFY | DH_TIME_ACCESS,
    		     time( NULL ) );
    
    /* alias checksum */
    
    chkSum = dosVDirChkSum( alias );
    
    /* use empty entry, that was found during path lkup */
    
    if( pFreeEnt->sector != 0 )
    	{
    	/* point file descriptor onto the deleted entry position */
    	
    	if( IS_ROOT( pFd ) && pDirDesc->rootMaxEntries < (u_int)(-1) )
    	    {
    	    pFd->nSec += pFd->curSec - pFreeEnt->sector;
    	    pFd->curSec = pFreeEnt->sector;
    	    }
    	else if( pFd->pVolDesc->pFatDesc->seek(
    			pFd, pFreeEnt->sector, 0 ) == ERROR )
    	    {
    	    goto ret;
    	    }

    	pFd->pos = pFreeEnt->offset;
    	}
    else if( IS_ROOT( pFd ) && pDirDesc->rootMaxEntries < (u_int)(-1) )
    	{	/* no free entries in directory */
    	errnoSet( S_dosFsLib_ROOT_DIR_FULL );
    	goto ret;
    	}
    else /* pointer to name start not initialized yet */
    	{
    	/* 
    	 * VVV : It may be that the parent directory chain is exhausted
    	 * and the new entry will be the first in addition cluster.
    	 *  In this case <pFreeEnt> will be set later in order to point
    	 * to actual position of first entry of name entries.
    	 * (see comment VVV hereafter)
    	 */
    	}
    	    
    /* --- create new entry --- */
    
    for( i = numEnt, whichEntry = PUT_CURRENT;
    	 i  > 0;
         i--, dirent += DOS_DIRENT_STD_LEN, whichEntry = PUT_NEXT, --freeEntLen )
    	{
    	/* encode checksum and attribute to long name entry */
    	 
    	if( i > 1 )
    	    {
    	    dirent[ DOS_VFAT_CHKSUM_OFF ] = chkSum;
    	    dirent[ pDirDesc->deDesc.atrribOff ] = DOS_ATTR_VFAT;
    	    }
    	
    	
	/* Allocate new Entry only if no Contig entries available */
	if (freeEntLen > 0)
	   {
	   allocFlag = 0;
	   }
	else
	   {
	   allocFlag = DH_ALLOC;
	   }

    	/* store consecutive directory entry */

    	if( dosVDirDeStore( pFd, 0, DOS_DIRENT_STD_LEN,
    	 		    dirent, whichEntry | allocFlag ) == ERROR )
    	    {
    	    goto ret;
    	    }

    	/* 
    	 * VVV: set <pFreeEnt> to point to actual position of first
    	 *  entry of name entries. (see comments VVV  above)
    	 */
    	if( i == numEnt ) /* (whichEntry == PUT_CURRENT) */
    	    {
    	    pFreeEnt->sector = pFd->curSec;
    	    pFreeEnt->offset = pFd->pos;
    	    }
    	}
    
    /* correct parent directory modification date/time */
    
    dosVDirUpdateEntry( pFd, DH_TIME_MODIFY, time( NULL ) );
    
    /* fill file descriptor for the new entry */
    
    cookie = pDirHdl->cookie;	/* backup cbio qsearch cookie */
    dosVDirFillFd( pFd, alias, ((numEnt > 1)?pFreeEnt:NULL) );
        
    /* write f i l e entry to disk */
    
    if( S_ISREG( options ) )
    	{
    	retStat = OK;
    	goto ret;
    	}
    
    /* --- now we deal with directory --- */
    
    dirent = alias;
    *(alias + pDirDesc->deDesc.atrribOff) = DOS_ATTR_DIRECTORY;
    pFd->pFileHdl->attrib = DOS_ATTR_DIRECTORY;
    /*
     * for directory we have to create two entries: "." and ".."
     * start - allocate cluster and init it in structure.
     */
    if( dosVDirClustNext( pFd, DH_ALLOC ) == ERROR )
    	goto ret;
    
    START_CLUST_ENCODE( &pDirDesc->deDesc,
    		        pFd->pFileHdl->startClust,  alias );
    
    /*
     * second - init directory entry for ".";
     * it is similar to directory entry itself, except name
     */
    bcopy( (char *)alias, (char *)dirent, DOS_DIRENT_STD_LEN ); 
    bfill( (char *)dirent,
    	   pDirDesc->deDesc.nameLen + pDirDesc->deDesc.extLen,
    	   SPACE );
    *dirent = DOT;
    if( dosVDirDeStore( pFd, 0, DOS_DIRENT_STD_LEN, dirent,
    			PUT_CURRENT ) == ERROR )
    	{
    	goto ret;
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

    if( dosVDirDeStore( pFd, 0, DOS_DIRENT_STD_LEN, dirent,
    			PUT_NEXT | DH_ALLOC ) == ERROR )
    	{
    	goto ret;
    	}
    
    /* store start cluster number */
    
    if( dosVDirUpdateEntry( pFd, 0, 0 ) == ERROR )
    	{
    	goto ret;
    	}

    /* now finish directory entry initialization */
    
    dosVDirRewindDir( pFd );	/* rewind directory */
    
    retStat = OK;
    
ret:
    semGive( pDirDesc->bufSem );
    return retStat;
    } /* dosVDirFileCreateInDir() */
    
/***************************************************************************
*
* dosVDirNameDecodeShort - decode short name from directory entry format.
*
* RETURNS: N/A.
*/
LOCAL void dosVDirNameDecodeShort
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    u_char *	pDirent,	/* directory entry buffer */
    u_char *	pDstName	/* destination name buffer */
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
    } /* dosVDirNameDecodeShort() */
    
/***************************************************************************
*
* dosVDirNameDecode - decode name from VFAT format.
*
* RETURNS: N/A.
*/
LOCAL void dosVDirNameDecode
    (
    DOS_DIR_PDESCR_ID	pDirDesc,
    u_char *	pDirent,	/* directory entry buffer */
    u_char *	pDstName	/* destination name buffer */
    )
    {
    int	i;	/* loop counter */
    u_char *	pDst = pDstName; /* work destination position pointer */
    u_char *	pAlias; /* work destination position pointer */
    
    if( pDirent[ pDirDesc->deDesc.atrribOff ] != DOS_ATTR_VFAT )
	{
	dosVDirNameDecodeShort( pDirDesc, pDirent, pDstName );
    	return; 
	}
    
    /* start from first (bottom) direntry of long name */
    
    pDirent += (*pDirent & VFAT_ENTNUM_MASK) * DOS_DIRENT_STD_LEN;
    pAlias = pDirent;
    do	{
    	pDirent -= DOS_DIRENT_STD_LEN;
    	for( i = 0;
    	     i < CHAR_PER_VENTRY && pDirent[ chOffsets[i] ] != EOS;
    	     i++, pDst ++ )
    	    {
    	    if( pDst - pDstName == NAME_MAX )
    	    	{
    	    	ERR_MSG( 10,"Name is too long; use alias instead\n",
    	    	         0,0,0,0,0,0 );
		dosVDirNameDecodeShort( pDirDesc, pAlias, pDstName );
    	    	return; 
    	    	}
    	    *pDst = pDirent[ chOffsets[i] ];
    	    }
    	   
    	} while( (*pDirent & DOS_VLAST_ENTRY) == 0 );
    
    *pDst = EOS;
    return;
    } /* dosVDirNameDecode() */
    
/***************************************************************************
*
* dosVDirReaddir - FIOREADDIR backend.
*
* Optionally this routine fills file descriptor for the encountered entry,
* if <pResFd> is not NULL.
*
* RETURNS: OK or ERROR if no directory entries remain.
*/
LOCAL STATUS dosVDirReaddir
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
    DIRENT_PTR	lnPtr = {0};	/* ptr to long name start on disk */
    u_char *	dirent = pDirDesc->nameBuf; /* directory entry buffer */
    u_int	nEntries = 0;	/* work buffer */
    STATUS	retStat = ERROR;
    
    assert( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY );
    
    /* check status of directory */
    
    if( (int)pDir->dd_cookie == (-1) )
    	return ERROR;
    
    readFlag = ( pDir->dd_cookie == 0 )? RD_FIRST : RD_NEXT;
    
    /* position must be directory entry size aligned */

    if( pFd->pos % DOS_STDNAME_LEN != 0 )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	goto ret;
    	}

    /* protect buffers */
    
    if( semTake( pDirDesc->bufSem, WAIT_FOREVER ) == ERROR )
    	return ERROR;

    do
    	{
    	if( dosVDirFullEntGet( pFd, dirent, readFlag, &lnPtr,
    			       &nEntries ) == ERROR )
            {
    	    goto ret;
    	    }
    	readFlag = RD_NEXT;
    	}
    while( *dirent == DOS_DEL_MARK ||
    	   ( dirent[ pDirDesc->deDesc.atrribOff ] != DOS_ATTR_VFAT &&
    	     (dirent[ pDirDesc->deDesc.atrribOff ] &
    	      DOS_ATTR_VOL_LABEL) != 0 ) );
    
    if( *dirent == LAST_DIRENT )	/* end of directory */
    	{
    	goto ret;
    	}
    
    /* name decode */
    
    dosVDirNameDecode( pDirDesc, dirent, (u_char *)pDir->dd_dirent.d_name );
    
    /* fill file descriptor for the entry */
    
    if( pResFd != NULL )
    	{
    	void *	pFileHdl = pResFd->pFileHdl;
    	
    	/* prepare file descriptor contents */
    	
    	*pResFd = *pFd;
    	pResFd->pFileHdl = pFileHdl;
    	*pResFd->pFileHdl = *pFd->pFileHdl;
    	
    	/* point to alias */
    	
    	if( dirent[ pDirDesc->deDesc.atrribOff ] == DOS_ATTR_VFAT )
    	    dirent += (*dirent & VFAT_ENTNUM_MASK) * DOS_DIRENT_STD_LEN;
    	
    	dosVDirFillFd( pResFd, dirent, &lnPtr );
    	}
    
    retStat = OK;
ret:
    /* save current offset in directory */
    
    if( retStat == OK )
    	pDir->dd_cookie = POS_TO_DD_COOKIE( pFd->pos );
    else
    	pDir->dd_cookie = (-1);

    semGive( pDirDesc->bufSem );

    return retStat;
    } /* dosVDirReaddir() */
    
/***************************************************************************
*
* dosVDirPathLkup - lookup for file/dir in tree.
*
* This routine recursively searches directory tree for the <path> and
* fills file descriptor for the target entry if successful.
* Optionally new entry may be created in accordance with creatFlags.
*
* Generally VFAT creates long names case sensitively,
* but makes lookup in case insensitive manner. Special option
* DOS_O_CASENS causes to make a search case sensitively.
*
* .CS
* <options> :	O_CREAT - creat file;
*		O_CREAT | DOS_ATTR_DIRECTORY - create directory.
*		<somth> | DOS_O_CASENS - lkup for name in case
*		                         sensitive manner.
* .CE
*
* RETURNS: OK or ERROR if file not found.
*
* ERRNO:
* S_dosFsLib_FILE_NOT_FOUND
*/
LOCAL STATUS dosVDirPathLkup
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
    int		freeEntLen=0;	/* Number of empty entries in directory */
    u_char	dirLevel;	/* dynamic directory level counter */
    
    assert( path != NULL );
    
    /* disassemble path */
    
    numPathLevels = dosVDirPathParse( pFd->pVolDesc, path, 
    					namePtrArray ); 
    if( numPathLevels == ERROR )
    	return ERROR;
    assert( numPathLevels <= DOS_MAX_DIR_LEVELS );
    
    /* start from root directory */
    
    dosVDirFillFd( pFd, ROOT_DIRENT, NULL );
    
    errnoBuf = errnoGet();
    errnoSet( OK );
    for( dirLevel = 0; dirLevel < numPathLevels; dirLevel ++  )
    	{
    	/* only directory can be searched */
    	
    	if( ! (pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) )
    	    break;
    	
    	if( dosVDirLkupInDir( pFd, namePtrArray + dirLevel, &freeEnt, &freeEntLen,
    			      ((options & DOS_O_CASENS) != 0) ) == ERROR )
    	    {
    	    if( errnoGet() != OK )	/* subsequent error */
    	    	return ERROR;
    	    	
    	    break;
    	    }
    	}
    
    if( errnoGet() == OK )
    	errnoSet( errnoBuf );
    
    /* 
     * check result if dirLevel == numPathLevels, then we found the file
     */
    
    if( dirLevel == numPathLevels )
    	{
    	return OK;
    	}
    
    /* --- file not found --- */
    
    /* only last file in path can be created */
    
    if( dirLevel == numPathLevels - 1 &&  (options & O_CREAT) )
    	{
    	if( dosVDirFileCreateInDir( pFd,
    				      namePtrArray + dirLevel,
    			              options, &freeEnt, freeEntLen ) == OK )
    	    return OK;
    	}
    else
    	{
    	errnoSet( S_dosFsLib_FILE_NOT_FOUND );
    	}
    
    /* error; release dir handle */
    
    return ERROR;
    } /* dosVDirPathLkup() */

/***************************************************************************
*
* dosVDirDateGet - fill in date-time fields in stat structure.
*
* RETURNS: OK or ERROR if disk access error.
*
*/
LOCAL STATUS dosVDirDateGet
    (
    DOS_FILE_DESC_ID	pFd,
    struct stat *	pStat
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pFd->pVolDesc->pDirDesc;
    u_char	dirent[ DOS_DIRENT_STD_LEN ]; /* directory entry buffer */
    
    /* root directory has not its own entry */
    
    if( IS_ROOT( pFd ) )
    	{
    	pStat->st_mtime = pDirDesc->rootModifTime;
    	return OK;
    	}
    
    /* get directory entry */
    
    if( dosVDirDirentGet( pFd, dirent, FD_ENTRY ) == ERROR )
    	return ERROR;
    
    pStat->st_ctime = dosVDirTDDecode( pDirDesc, dirent, DH_TIME_CREAT );
    pStat->st_mtime = dosVDirTDDecode( pDirDesc, dirent, DH_TIME_MODIFY );
    pStat->st_atime = dosVDirTDDecode( pDirDesc, dirent, DH_TIME_ACCESS );
    return OK;
    } /* dosVDirDateGet() */
    
/*******************************************************************************
*
* dosVDirVolLabel - set/get dos volume label.
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
LOCAL STATUS dosVDirVolLabel
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    u_char *	label,
    u_int	request		/* FIOLABELSET, FIOLABELGET */
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    u_char	dirent[ DOS_DIRENT_STD_LEN ]; /* directory entry buffer */
    DOS_FILE_DESC	fd;	/* work file descriptor */
    DOS_FILE_HDL	fHdl;	/* work file handle */
    STATUS	status = ERROR;	/* search status */
    u_int	numEnt;	/* number of passed entries */
    char *	noName = "NO LABLE";
    u_short	labOffInBoot;	/* volume label offset in boot sector */
    
    assert( request == FIOLABELSET || request == FIOLABELGET );
    
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
    dosVDirFillFd( &fd, ROOT_DIRENT, NULL );
    
    /* search for volume label */
    
    for( numEnt = 0, (status = dosVDirDirentGet( &fd, dirent, RD_FIRST ));
    	 (status != ERROR) && *dirent != LAST_DIRENT;
    	 numEnt ++, status = dosVDirDirentGet( &fd, dirent, RD_NEXT ) )
    	{
    	if( dirent[ pDirDesc->deDesc.atrribOff ] != DOS_ATTR_VFAT &&
    	    ( dirent[ pDirDesc->deDesc.atrribOff ] &
    	      DOS_ATTR_VOL_LABEL ) != 0 )
    	    {
    	    break;
    	    }
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
    
    status = dosVDirDeStore( &fd, 0, DOS_DIRENT_STD_LEN, dirent,
    			     PUT_CURRENT | DH_ALLOC );
   
#ifdef CHANGE_BOOT
#warning CHANGE_BOOT defined for dosVDirLib.c
    /* XXX - avoid changing boot record to avoid false disk-change event */

    /* store in boot sector */

    bcopy( (char *)dirent, pVolDesc->bootVolLab, DOS_VOL_LABEL_LEN );

    status |= cbioBytesRW(
    			pVolDesc->pCbio, DOS_BOOT_SEC_NUM,
    			labOffInBoot, dirent, 
    			DOS_VOL_LABEL_LEN, CBIO_WRITE, NULL );
    
#endif /* CHANGE_BOOT */

    return status;
    } /* dosVDirVolLabel() */
    
/*******************************************************************************
*
* dosVDirNameChk - validate file name.
*
* This routine validates incoming file name to be composed
* of valid characters.
*
* RETURNS: OK if name is OK or ERROR.
*/
LOCAL STATUS dosVDirNameChk
    (
    DOS_VOLUME_DESC_ID	pVolDesc,
    u_char *	name
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    SHORT_ENCODE	shortNameEncode = NOT_STRICT_SHORT;
    int	result;
    PATH_ARRAY	namePtr;
    
    namePtr.pName = name;
    namePtr.nameLen = strlen((char *)name);
    
    /* protect buffers */
    
    if( semTake( pDirDesc->bufSem, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    result = dosVDirNameEncode( pDirDesc, &namePtr,
    				pDirDesc->nameBuf, &shortNameEncode );
    
    semGive( pDirDesc->bufSem );
    
    return ( (result == (-1))? ERROR : OK );
    } /* dosVDirNameChk() */
    
/*******************************************************************************
*
* dosVDirShow - display handler specific volume configuration data.
*
* RETURNS: N/A.
*/
LOCAL void dosVDirShow
    (
    DOS_VOLUME_DESC_ID	pVolDesc
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    
    printf(" - directory structure:		VFAT\n" );
    
    if( ! pVolDesc->mounted )
    	return;
    
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
    	printf(" - root dir start cluster:	%u\n",
    			pDirDesc->rootStartClust );
    	}
    
    return;
    } /* dosVDirShow() */

/*******************************************************************************
*
* dosVDirVolUnmount - free all allocated resources.
*
* This routine only deallocates names buffer.
*
* RETURNS: N/A.
*/
LOCAL void dosVDirVolUnmount
    (
    DOS_VOLUME_DESC_ID	pVolDesc
    )
    {
    DOS_DIR_PDESCR_ID	pDirDesc = (void *)pVolDesc->pDirDesc;
    
    if (pDirDesc == NULL)
	return ;

    semTake( pDirDesc->bufSem, WAIT_FOREVER ); /* XXX */
    
    if (pDirDesc->nameBuf != NULL)
    	{
    	KHEAP_FREE (pDirDesc->nameBuf);
    	pDirDesc->nameBuf = NULL;
    	}

    semGive (pDirDesc->bufSem);
    
    return;
    } /* dosVDirVolUnmount() */

/*******************************************************************************
*
* dosVDirVolMount - init all data required to access the volume.
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
LOCAL STATUS dosVDirVolMount
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
    	pVolDesc->pDirDesc->volUnmount != dosVDirVolUnmount )
    	{
    	/* unmount previous directory handler */
    	    
    	if( pVolDesc->pDirDesc != NULL &&
    	    pVolDesc->pDirDesc->volUnmount != NULL )
    	    {
    	    pVolDesc->pDirDesc->volUnmount( pVolDesc );
    	    }
    	    
    	/* allocate directory handler descriptor */
    	    
    	pVolDesc->pDirDesc = KHEAP_REALLOC((char *) pVolDesc->pDirDesc,
    	    			      sizeof( DOS_DIR_PDESCR ) );
    	if( pVolDesc->pDirDesc == NULL )
    	    return ERROR;
    	
    	bzero( (char *)pVolDesc->pDirDesc, sizeof( DOS_DIR_PDESCR ) );
    	}
    
    pDirDesc = (void *)pVolDesc->pDirDesc;
    pDeDesc = &pDirDesc->deDesc;
    
    if( bcmp( bootSec + DOS_BOOT_SYS_ID, DOS_VX_LONG_NAMES_SYS_ID,
    	      strlen( DOS_VX_LONG_NAMES_SYS_ID ) ) == 0 )
    	{
    	/* use vxWorks long names */
    	
    	ERR_MSG( 1, "VxWorks proprietary long names not supported\n",
    		 0,0,0,0,0,0 );
    	errnoSet( S_dosFsLib_UNSUPPORTED );
    	return ERROR;
    	}
    else
    	{
    	DBG_MSG( 10, "VFAT long names\n", 0,0,0,0,0,0,0,0 );
    	
    	pDirDesc->nameStyle = STDDOS;
    	
    	pDeDesc->dirEntSize	= DOS_DIRENT_STD_LEN;
    	pDeDesc->nameLen	= DOS_STDNAME_LEN;
    	pDeDesc->extLen		= DOS_STDEXT_LEN;
    	pDeDesc->atrribOff	= DOS_ATTRIB_OFF;
    	
    	pDeDesc->creatTimeOff	= DOS_CREAT_TIME_OFF;
    	pDeDesc->creatDateOff	= DOS_CREAT_DATE_OFF;
    	
    	pDeDesc->accessTimeOff	= DOS_LAST_ACCESS_TIME_OFF;
    	pDeDesc->accessDateOff	= DOS_LAST_ACCESS_DATE_OFF;
    	
    	pDeDesc->modifTimeOff	= DOS_MODIF_TIME_OFF;
    	pDeDesc->modifDateOff	= DOS_MODIF_DATE_OFF;
    	
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
    	ERR_MSG( 1, "cluster size %d bytes is too small, min = %d\n",
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
    	DBG_MSG( 10, "FAT32 \n", 0,0,0,0,0,0,0,0 );
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
    
    pDirDesc->dirDesc.pathLkup		= dosVDirPathLkup;
    pDirDesc->dirDesc.readDir		= dosVDirReaddir;
    pDirDesc->dirDesc.updateEntry	= dosVDirUpdateEntry;
    pDirDesc->dirDesc.dateGet		= dosVDirDateGet;
    pDirDesc->dirDesc.volLabel		= dosVDirVolLabel;
    pDirDesc->dirDesc.nameChk		= dosVDirNameChk;
    pDirDesc->dirDesc.volUnmount	= dosVDirVolUnmount;
    pDirDesc->dirDesc.show		= dosVDirShow;

    /* init long name buffers and semaphore */
    
    if (pDirDesc->nameBuf == NULL)
    	{
    	pDirDesc->nameBuf = KHEAP_ALLOC (2 * MAX_VFAT_FULL_DIRENT);
    	if (pDirDesc->nameBuf == NULL)
	    {
    	    return ERROR;
	    }
	else
	    {
	    bzero (pDirDesc->nameBuf,  2 * MAX_VFAT_FULL_DIRENT );
	    }
    	
    	pDirDesc->bufSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
    	}
    
    return OK;
    }

/*******************************************************************************
*
* dosVDirLibInit - install VFAT directory handler into dosFsLib
* 
* RETURNS: OK or ERROR if handler installation failed.
*
* SEE ALSO: dosFsLib
*
* NOMANUAL
*/
STATUS dosVDirLibInit ( void )
    {
    DOS_HDLR_DESC	hdlr;
    
    hdlr.id = DOS_VDIR_HDLR_ID;
    hdlr.mountRtn = dosVDirVolMount;
    hdlr.arg = NULL;
    
    return dosFsHdlrInstall( dosDirHdlrsList, &hdlr );
    } /* dosVDirLibInit() */
/* End of File */

