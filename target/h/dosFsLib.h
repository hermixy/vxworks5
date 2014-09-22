/* dosFsLib.h - DOS file system header file */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01p,04mar02,jkf  Adding dosSetVolCaseSens(),SPR#29751, orig by chn.
01o,30nov01,jkf  SPR#68203, adding dosFsLastAccessDateEnable().
01n,14nov01,jkf  SPR#71720, avoiding unaligned pointer accesses.
                 added leading underscore to _WRS_PACK_ALIGN ifndef 
01m,11nov01,jkf  added WRS_PACK_ALIGN ifndef for AE 1.1 support, this 
                 allows same dosFs2 files to build in AE 1.1 environment.
01l,10nov01,jkf  SPR#32178, made dosFsVolDescGet public
                 Added WRS_PACK_ALIGN (2) to legacy struct for MIPS.
01k,21sep01,jkf  SPR#69031, common code for both AE & 5.x.
01j,21sep01,jkf  changed error codes to WRS standard.
01i,29feb00,jkf  T3 changes
01h,31aug99,jkf  changes for new CBIO API.
01g,31jul99,jkf  T2 merge, tidiness & spelling.
01f,15oct98,lrn  moved 64-bit extended ioctl codes to ioLib.h
01e,11sep98,vld  prototype of dosFsDevCreate() to accept arbitrary
		 block device ptr.
01d,09jul98,lrn  added more defines for back compatibility
01c,30jun98,lrn  moved ERRNO_PX_FLAG to errno.h, renamed dosFsInit to
                 dosFsLibInit, added b.c., cosmetics
01b,28jun98,vld  tested and checked in
01a,22jan98,vld  written,
*/

#ifndef __INCdoFsLib
#define __INCdoFsLib

#ifdef __cplusplus
extern "C" {
#endif

#include <vxWorks.h>
#include <cbioLib.h>


/* defines */
#define	DOS_FS_COMPAT	/* enable definition for backwards compatibility */

/* check disk levels */

#define DOS_CHK_ONLY	1
#define DOS_CHK_REPAIR	2
#define DOS_CHK_VERB_0	(0xff<<8)
#define DOS_CHK_VERB_SILENT	DOS_CHK_VERB_0
#define DOS_CHK_VERB_1	(1<<8)
#define DOS_CHK_VERB_2	(2<<8)

#define DOS_MAX_DIR_LEVELS      20      /* max expected directory levels */
#define DOS_VOL_LABEL_LEN 	11	/* length of volume label */

/* ioctl FIOCHKDSK - check disk levels */

#define CHK_ONLY	1
#define CHK_REPAIR	2

/* additional open() flags */

#define DOS_O_CONTIG_CHK	0x10000	/* check file for contiguity */
#define DOS_O_CASENS		0x20000	/* case-sensitive file lookup */

/* File attribute byte values */

#define DOS_ATTR_RDONLY         0x01            /* read-only file */
#define DOS_ATTR_HIDDEN         0x02            /* hidden file */
#define DOS_ATTR_SYSTEM         0x04            /* system file */
#define DOS_ATTR_VOL_LABEL      0x08            /* volume label (not a file) */
#define DOS_ATTR_DIRECTORY      0x10            /* entry is a sub-directory */
#define DOS_ATTR_ARCHIVE        0x20            /* file subject to archiving */

/* volume format definitions */

/* data types */
/* Volume configuration data */

typedef struct          /* DOS_VOL_CONFIG */
    {                           /* Legend: M- modifiable, C- Calculated */
    enum
        { _AUTO=0, _FAT12=12, _FAT16=16, _FAT32=32 }
                fatType;        /* M- Fat format Type */
    const char * sysId ;        /* M- sys ID string */
    int         secPerClust;    /* M- sectors per cluster (minimum 1) */
    short       nResrvd;        /* M- number of reserved sectors (min 1) */
    short       maxRootEnts;    /* M- max number of entries in root dir */
    char        nFats;          /* M- number of FAT copies (minimum 1) */
    ULONG       secPerFat;      /* C- number of sectors per FAT copy */
    ULONG       nClust;         /* C- # of clusters on disk */
    ULONG       nHidden;        /* C- number of hidden sectors */
    ULONG       volSerial ;     /* M- disk serial number */
    int         secPerTrack;    /* M- sectors per track */
    char        nHeads;         /* M- number of heads */
    char        mediaByte;      /* M- media descriptor byte */
    char        volLabel[ 11 ]; /* X- disk volume label */
    } DOS_VOL_CONFIG;

/* 
 * A DOS_VOLUME_DESC_ID is pointer to DOS_VOLUME_DESC. For details
 * see h/private/dosFsLibP.h.  Note: That is a WRS private header file.
 */ 

typedef struct DOS_VOLUME_DESC *        DOS_VOLUME_DESC_ID;

/* dosFsVolFormat() options: */
#define DOS_OPT_DEFAULT         0x0000  /* format with default options */
#define DOS_OPT_PRESERVE        0x0001  /* preserve boot block if possible */
#define DOS_OPT_BLANK           0x0002  /* create a clean boot block */
#define DOS_OPT_QUIET           0x0004  /* dont produce any output */
#define DOS_OPT_FAT16           0x0010  /* format with FAT16 file system */
#define DOS_OPT_FAT32           0x0020  /* format with FAT32 file system */
#define DOS_OPT_VXLONGNAMES     0x0100  /* format with VxLong file names */

/* error codes */
/*
 * The codes which are defined with ERRNO_PX_FLAG can be mapped
 * to POSIX error codes:
 * if (errno & ERRNO_PX_FLAG) errno &= 0x7fff ;
 */

#define S_dosFsLib_32BIT_OVERFLOW		(M_dosFsLib |  1)
#define S_dosFsLib_DISK_FULL			(M_dosFsLib |  2)
#define S_dosFsLib_FILE_NOT_FOUND		(M_dosFsLib |  3)
#define S_dosFsLib_NO_FREE_FILE_DESCRIPTORS 	(M_dosFsLib |  4)
#define S_dosFsLib_NOT_FILE			(M_dosFsLib |  5)
#define S_dosFsLib_NOT_SAME_VOLUME	        (M_dosFsLib |  6)
#define S_dosFsLib_NOT_DIRECTORY		(M_dosFsLib |  7)
#define S_dosFsLib_DIR_NOT_EMPTY		(M_dosFsLib |  8)
#define S_dosFsLib_FILE_EXISTS			(M_dosFsLib |  9)
#define S_dosFsLib_INVALID_PARAMETER		(M_dosFsLib | 10)
#define S_dosFsLib_NAME_TOO_LONG		(M_dosFsLib | 11)
#define S_dosFsLib_UNSUPPORTED			(M_dosFsLib | 12)
#define S_dosFsLib_VOLUME_NOT_AVAILABLE		(M_dosFsLib | 13)
#define S_dosFsLib_INVALID_NUMBER_OF_BYTES	(M_dosFsLib | 14)
#define S_dosFsLib_ILLEGAL_NAME			(M_dosFsLib | 15)
#define S_dosFsLib_CANT_DEL_ROOT		(M_dosFsLib | 16)
#define S_dosFsLib_READ_ONLY			(M_dosFsLib | 17)
#define S_dosFsLib_ROOT_DIR_FULL		(M_dosFsLib | 18)
#define S_dosFsLib_NO_LABEL			(M_dosFsLib | 19)
#define S_dosFsLib_NO_CONTIG_SPACE		(M_dosFsLib | 20)
#define S_dosFsLib_FD_OBSOLETE			(M_dosFsLib | 21)
#define S_dosFsLib_DELETED			(M_dosFsLib | 22)
#define S_dosFsLib_INTERNAL_ERROR		(M_dosFsLib | 23)
#define S_dosFsLib_WRITE_ONLY			(M_dosFsLib | 24)
#define S_dosFsLib_ILLEGAL_PATH			(M_dosFsLib | 25)
#define S_dosFsLib_ACCESS_BEYOND_EOF		(M_dosFsLib | 26)
#define S_dosFsLib_DIR_READ_ONLY		(M_dosFsLib | 27)
#define S_dosFsLib_UNKNOWN_VOLUME_FORMAT	(M_dosFsLib | 28)
#define S_dosFsLib_ILLEGAL_CLUSTER_NUMBER	(M_dosFsLib | 29)

/* macros */

/* typedefs */

/* interface functions prototypes */

IMPORT STATUS dosFsLibInit( int ignored );
IMPORT STATUS dosFsDevCreate ( char * pDevName, CBIO_DEV_ID cbio,
			       u_int maxFiles, u_int autoChkLevel );
IMPORT STATUS dosFsVolUnmount ( void * pDev );
IMPORT STATUS dosFsShow( void * dev, u_int level );
IMPORT STATUS dosFsVolFormat( void * pDev, int opt, FUNCPTR pPromptFunc );
IMPORT void dosFsFmtLibInit( void );
IMPORT void dosChkLibInit( void );
IMPORT STATUS dosDirOldLibInit( void );
IMPORT STATUS dosVDirLibInit ( void );
IMPORT STATUS dosFsFatInit ( void );
IMPORT DOS_VOLUME_DESC_ID dosFsVolDescGet (void * pDevNameOrPVolDesc, 
                                           u_char **   ppTail);

IMPORT STATUS dosFsLastAccessDateEnable (DOS_VOLUME_DESC_ID dosVolDescId, 
                                  BOOL enable);

IMPORT STATUS dosSetVolCaseSens /* set rename case sens */
    (
    DOS_VOLUME_DESC_ID pVolDesc,
    BOOL sensitivity /* TRUE == case sens */
    );

/* OS Macro's */

#ifndef _WRS_PACK_ALIGN
#    define _WRS_PACK_ALIGN(x) __attribute__((packed, aligned(x)))
#endif /* _WRS_PACK_ALIGN */


#if TRUE /* SPR#71720, avoiding unaligned pointer accesses. */

/* GNU versions of new OS macros */

#ifndef _WRS_ALIGNOF
/*
 * _WRS_ALIGNOF - return the alignment of an item, in bytes
 *
 * Returns the byte alignment for an item.  The returned value
 * is the alignment in units of bytes.
 */

#    define _WRS_ALIGNOF(x) \
            __alignof__(x)

#endif /* ifndef _WRS_ALIGNOF */

#ifndef _WRS_ALIGN_CHECK
/*
 * _WRS_ALIGN_CHECK - test a pointer for a particular alignment
 *
 * Returns TRUE if the pointer is aligned sufficiently to be a 
 * pointer to a specified data type.  Returns FALSE if the pointer
 * value is not aligned to be a valid pointer to the data type.
 */

#    define _WRS_ALIGN_CHECK(ptr, type) \
            (((int)(ptr) & ( _WRS_ALIGNOF(type) - 1)) == 0 ? TRUE : FALSE)

#endif /* ifndef _WRS_ALIGN_CHECK */

#endif /* TRUE/FALSE - SPR#71720, avoiding unaligned pointer accesses. */



#ifdef	DOS_FS_COMPAT
/* types for backwards compatibility, should be discontinued */
#include "blkIo.h"

typedef	void * DOS_VOL_DESC ;

/* DOS_PART_TBL is obsolete: use dpartCbio/usrFdiskPartLib instead */

typedef struct		/* DOS_PART_TBL */
    {
    UINT8		dospt_status;		/* partition status */
    UINT8		dospt_startHead;	/* starting head */
    short		dospt_startSec;		/* starting sector/cylinder */
    UINT8		dospt_type;		/* partition type */
    UINT8		dospt_endHead;		/* ending head */
    short		dospt_endSec;		/* ending sector/cylinder */
    ULONG		dospt_absSec;		/* starting absolute sector */
    ULONG		dospt_nSectors;		/* number of sectors in part */
    } _WRS_PACK_ALIGN (2) DOS_PART_TBL;


/* dosFs Date and Time Structure - should be replaced with ANSI time */
 
typedef struct          /* DOS_DATE_TIME */
    {
    int         dosdt_year;             /* current year */
    int         dosdt_month;            /* current month */
    int         dosdt_day;              /* current day */
    int         dosdt_hour;             /* current hour */
    int         dosdt_minute;           /* current minute */
    int         dosdt_second;           /* current second */
    } DOS_DATE_TIME;

/* old defines */
#define	DOS_BOOT_SYS_ID		0x03	/* system ID string          (8 bytes)*/
#define	DOS_SYS_ID_LEN		8       /* length of system ID string */
#define	DOS_BOOT_PART_TBL	0x1be	/* first disk partition tbl (16 bytes)*/

/* old proprietary long file names, now renamed VXLONGNAMES */
#define DOS_OPT_LONGNAMES       0x4    

/* All other old options are non applicable anymore, defined for compiles */
#define DOS_OPT_CHANGENOWARN    0x1     /* disk may be changed w/out warning */
#define DOS_OPT_AUTOSYNC        0x2     /* autoSync mode enabled */
#define DOS_OPT_EXPORT          0x8     /* file system export enabled */
#define DOS_OPT_LOWERCASE       0x40    /* filenames use lower-case chars */

/* old prototypes */
extern DOS_VOL_DESC *	dosFsDevInit (char *pDevName, BLK_DEV *pBlkDev,
		                      DOS_VOL_CONFIG *pConfig);
extern STATUS 		dosFsInit (int maxFiles);
extern DOS_VOL_DESC *	dosFsMkfs (char *pVolName, BLK_DEV *pBlkDev);

/* end of backward compatibility types */
#endif	/* DOS_FS_COMPAT */

#ifdef __cplusplus
    }
#endif

#endif /* __INCdoFsLib */
