/* dosFsLibP.h - DOS file system private header file */

/* Copyright 1984-2002 Wind River Systems, Inc. */
 
/* modification history
--------------------
01q,03mar02,jkf  SPR#29751, added volIsCaseSens to DOS_VOLUME_DESC, orig by chn
01p,30nov01,jkf  SPR#68203, add updateLastAccessDate boolean to DOS_VOLUME_DESC
01o,17nov01,jkf  fixing dosFsVolIsFat12 divide error.
01n,10nov01,jkf  SPR#32178, made dosFsVolDescGet public, moved 
                 DOS_VOLUME_DESC_ID typedef to public dosFsLib.h
01m,30jul01,jkf  SPR#69031, common code for both AE & 5.x. 
01l,13mar01,jkf  SPR#34704, adding dosFsVolIsFat12
01k,29feb00,jkf  removed semLibP.h usage for t3.
01j,31aug99,jkf  CBIO API overhaul changes.
01i,31jul99,jkf  added DOS_BOOT_FSTYPE_ID, and DOS_BOOT_FSTYPE_LEN,
                 SPR#28272, 28273, 28274, 28275.
01h,31jul99,jkf  T2 merge, tidiness & spelling.
01g,21dec98,mjc  changed name of <contigEnd> field in structure <DOS_FILE_HDL> 
                 to <contigEndPlus1>
01f,22nov98,vld  field <seekOut> changed to <seekOutPos> in
		 DOS_FILE_DESC structure;
		 added macros POS_TO_DD_COOKIE() and DD_COOKIE_TO_POS();
    	    	 fields <rootStartSec> and <rootNSec> added to
		 DOS_DIR_DESC structure
01e,23sep98,vld  field <bufToDisk> added to CHK_DSK_DESC;
01d,17sep98,vld  field <activeCopyNum> added to DOS_FAT_DESC;
		 changed prototypes of some func ptr fields of DOS_FAT_DESC;
		 func ptr field <syncToggle> added to DOS_FAT_DESC;
                 added constant DOS_FAT_RAW;
                 added constant DOS_FAT_SET_ALL;
    	    	 changed prototypes of some function ptrs
   	    	 in DOS_FAT_DESC;
01c,16sep98,vld  definition of DOS_VOL_LABEL_LEN moved to dosFsLib.h 
01b,02jul98,lrn  ready for prerelease
01a,22jan98,vld	 written,
*/

#ifndef __INCdoFsLibP
#define __INCdoFsLibP

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "semLib.h" 
#include "iosLib.h"
#include "cbioLib.h"
#include "dirent.h"
#include "stat.h"

#include "dosFsLib.h"

/* defines */

/* use 64-bit arithmetic */

#define SIZE64

#define DOS_FS_MAGIC	0xdfac9723	/* used for verification of vol desc */
#define DOS_VX_LONG_NAMES_SYS_ID	"VXEXT"

#define SLASH           '/'
#define BACK_SLASH      '\\'
#define DOT             '.'
#define SPACE           ' '
/* 
 * handlers ID.
 * Each handler (FAT, Directory and so on) must have its own
 * identification number in order to be able to distinguish
 * different handler types when selecting handlers
 */
    /* directory handlers */

#define DOS_DIR_HDLR_MASK	(1 << 8)	/* dir handlers mask */
#define DOS_DIROLD_ID	(DOS_DIR_HDLR_MASK +1)	/* 8.3 and vxLong names */

    /* FAT handlers */

#define DOS_FAT_HDLR_MASK	(1 << 9)	/* fat handlers mask */

/* function arguments */

/* for directory handler API */

#define DH_TIME_CREAT	1
#define DH_TIME_MODIFY	(1<<1)
#define DH_TIME_ACCESS	(1<<2)
#define DH_DELETE	(1<<8)

/* FAT handler API */

#define FAT_NOT_ALLOC	0
#define FAT_ALLOC	(1<<31)
#define FAT_ALLOC_ONE	(FAT_ALLOC|1)
#define FH_INCLUDE	0
#define FH_EXCLUDE	1
#define FH_FILE_START	((u_int)(-1))

/* dosFs file system constants */

#define DOS_BOOT_SEC_NUM        0       /* sector number of boot sector */
#define DOS_MIN_CLUST           2       /* lowest cluster number used */
#define DOS_SYS_ID_LEN          8       /* length of system ID string */
#define DOS_FAT_12BIT_MAX	4085    /* max clusters for 12-bit FAT entries*/
#define DOS_NFILES_DEFAULT	20	/* default max number of files */

/* Boot sector offsets */
 
/*   Because the MS-DOS boot sector format has word data items
 *   on odd-byte boundaries, it cannot be represented as a standard C
 *   structure.  Instead, the following symbolic offsets are used to
 *   isolate data items.  Non-string data values longer than 1 byte are
 *   in "Intel 8086" order.
 *
 *   These definitions include fields used by MS-DOS Version 4.0.
 */
#define DOS_BOOT_JMP            0x00    /* 8086 jump instruction     (3 bytes)*/
#define DOS_BOOT_SYS_ID         0x03    /* system ID string          (8 bytes)*/
#define DOS_BOOT_BYTES_PER_SEC  0x0b    /* bytes per sector          (2 bytes)*/
#define DOS_BOOT_SEC_PER_CLUST  0x0d    /* sectors per cluster       (1 byte) */
#define DOS_BOOT_NRESRVD_SECS   0x0e    /* # of reserved sectors     (2 bytes)*/
#define DOS_BOOT_NFATS          0x10    /* # of FAT copies           (1 byte) */
#define DOS_BOOT_MAX_ROOT_ENTS  0x11    /* max # of root dir entries (2 bytes)*/
#define DOS_BOOT_NSECTORS       0x13    /* total # of sectors on vol (2 bytes)*/
#define DOS_BOOT_MEDIA_BYTE     0x15    /* media format ID byte      (1 byte) */
#define DOS_BOOT_SEC_PER_FAT    0x16    /* # of sectors per FAT copy (2 bytes)*/
#define DOS_BOOT_SEC_PER_TRACK  0x18    /* # of sectors per track    (2 bytes)*/
#define DOS_BOOT_NHEADS         0x1a    /* # of heads (surfaces)     (2 bytes)*/
#define DOS_BOOT_NHIDDEN_SECS   0x1c    /* # of hidden sectors       (4 bytes)*/
#define DOS_BOOT_LONG_NSECTORS  0x20    /* total # of sectors on vol (4 bytes)*/
#define DOS_BOOT_DRIVE_NUM      0x24    /* physical drive number     (1 byte) */
#define DOS_BOOT_SIG_REC        0x26    /* boot signature record     (1 byte) */
#define DOS_BOOT_VOL_ID         0x27    /* binary volume ID number   (4 bytes)*/
#define DOS_BOOT_VOL_LABEL      0x2b    /* volume label string      (11 bytes)*/
#define DOS_BOOT_FSTYPE_ID	0x36    /* new MS ID, FAT12 or FAT16 */
#define DOS_BOOT_FSTYPE_LEN	0x08    /* length in bytes of FSTYPE */
#define DOS_BOOT_FSTYPE_FAT16   "FAT16   "  /* FSTYPE_ID FAT16 */
#define DOS_BOOT_FSTYPE_FAT12   "FAT12   "  /* FSTYPE_ID FAT12 */
#define DOS_BOOT_SYSID_FAT32    "FAT32   "  /* FAT32 SYS_ID, */
#define DOS_BOOT_PART_TBL       0x1be   /* first disk partition tbl (16 bytes)*/
#define DOS_EXT_BOOT_SIG        0x29    /* value written to boot signature */
                                        /* record to indicate extended */
                                        /* (DOS v4) boot record in use  */
/* extended FAT32 fields offsets */

#define DOS32_BOOT_SEC_PER_FAT	0x24    /* sectors per FAT           (4 bytes)*/
#define DOS32_BOOT_EXT_FLAGS	0x28    /* FAT  miscellaneous flags  (2 bytes)*/
#define DOS32_BOOT_FS_VERSION	0x2a    /* file system version       (2 bytes)*/
#define DOS32_BOOT_ROOT_START_CLUST 0x2c /* root start cluster       (4 bytes)*/
#define DOS32_BOOT_FS_INFO_SEC	0x30    /* file system info sector   (2 bytes)*/
#define DOS32_BOOT_BOOT_BKUP	0x32    /* bkup of boot sector       (2 bytes)*/
#define DOS32_BOOT_RESERVED	0x3a    /* reserved area             (6 bytes)*/
#define DOS32_BOOT_BIOS_DRV_NUM	0x40	/* 0x80 for hard disk        (1 byte)*/
#define DOS32_BOOT_SIGNATURE	0x42	/* ')' (0x29)                (1 byte)*/
#define DOS32_BOOT_VOL_ID	0x43	/* binary volume Id          (4 bytes*/
#define DOS32_BOOT_VOL_LABEL	0x47    /* volume label string      (11 bytes)*/
#define DOS32_BOOT_FS_TYPE	0x52	/* string FAT32 */

#define DOS_BOOT_BUF_SIZE	0x80	/* size of buffer large enough to */
					/* get boot sector data to */
					/* ( without partition table ) */

#define CHK_MAX_PATH	1024	/* max path in check disk's message */

/* macros */

/* endian data conversion macros, disk always L.E. */

#define DISK_TO_VX_16( pSrc )	(UINT16)(				\
				  ( *((u_char *)(pSrc)+1) << 8 ) |	\
				  ( *(u_char *)(pSrc)          ) )

#define DISK_TO_VX_32( pSrc )	(UINT32)(				\
				  ( *((u_char *)(pSrc)+3)<<24 ) |	\
                  		  ( *((u_char *)(pSrc)+2)<<16 ) |	\
                  		  ( *((u_char *)(pSrc)+1)<< 8 ) |	\
                   		  ( *(u_char *)(pSrc)         ) )
#define VX_TO_DISK_16( src, pDst )					 \
			( * (u_char *)(pDst)       = (src)       & 0xff, \
			  *((u_char *)(pDst) + 1)  = ((src)>> 8) & 0xff )

#define VX_TO_DISK_32( src, pDst )					   \
			( * (u_char *)(pDst)       = (src)        & 0xff,  \
			  *((u_char *)(pDst) + 1)  = ((src)>> 8)  & 0xff,  \
			  *((u_char *)(pDst) + 2)  = ((src)>> 16) & 0xff,  \
			  *((u_char *)(pDst) + 3)  = ((src)>> 24) & 0xff ) \

/* sector macros */

#define NSECTORS( pVolDesc, off )	( (off) >> (pVolDesc)->secSizeShift )
#define OFFSET_IN_SEC( pVolDesc, off )			\
				( (off) & ((pVolDesc)->bytesPerSec - 1) )

/* conversions of sector <-> cluster numbers */

/* check, if directory is root */

#define IS_ROOT( pFd )	\
    ((pFd)->pFileHdl->dirHdl.parDirStartCluster == (u_long) NONE)

#define ROOT_SET( pFd )	\
    ((pFd)->pFileHdl->dirHdl.parDirStartCluster = NONE)

/* FAT entry types, returned by clustValueSet() and clustValueGet() */

#define DOS_FAT_AVAIL	0x00000001	/* available cluster */
#define DOS_FAT_EOF	0x00000002	/* end of file's cluster chain */
#define DOS_FAT_BAD	0x00000004	/* bad cluster (damaged media) */
#define DOS_FAT_ALLOC	0x00000008	/* allocated cluster */
#define DOS_FAT_INVAL	0x00000010	/* invalid cluster (illegal value) */
#define DOS_FAT_RESERV	0x00000020	/* reserved cluster */
#define DOS_FAT_RAW	0x00000040	/* write raw data, when fat copy */
					/* is used as temporary buffer */
#define DOS_FAT_SET_ALL	0x00000080	/* fill all bytes of FAT copy with */
					/* a specified value. */
#define DOS_FAT_ERROR	0x11111111	/* CBIO ERROR */

/*
 * handlers Id-s. All installed handlers are sorted by
 * Id-s, and are tried on every volume in accordance with that order
 */
#define DOS_MAX_HDLRS	4	/* max handlers of one type */
#define DOS_FATALL_HDLR_ID 255	/* dosFsFat16 - supports FAT 12/16/32 */
				/* in a simple manner */
				/* ( without any buffering ) */
#define DOS_VDIR_HDLR_ID   1	/* dosVDirLib - supports VDIR */
				/* directory structure for all FAT types */
#define DOS_DIROLD_HDLR_ID 2	/* dosDirOldLib - supports DOS4.0 */
				/* "8.3" names and vxLong names for */
				/* all FAT types */

/* definition of 64-bit type and macro to check for 32-bit overflowing */

#ifdef SIZE64

typedef long long	fsize_t;
#define DOS_IS_VAL_HUGE( val )	( ( (fsize_t)(val) >> 32 ) != 0 )
					/* test for 32 bit overflowing */
#define DOS_MAX_FSIZE	( ~((fsize_t)1<<(sizeof(fsize_t)*8-1)) )
					/* max disk/file size */

#else	/* ! SIZE64 */

typedef u_long  fsize_t;
#define DOS_IS_VAL_HUGE( val )	FALSE
#define DOS_MAX_FSIZE	( (fsize_t)(-1) ) /* max disk/file size */

#endif /* SIZE64 */

/*
 * processing for dd_cookie field in DIR structure(). (see dirent.h)
 * Current position in directory supposed to be always power of 2
 */
#define POS_TO_DD_COOKIE( pos )	( ((u_long)(pos)) | 1 )
#define DD_COOKIE_TO_POS( pDir )	( ((DIR *)(pDir))->dd_cookie & (~1) )

/* typedefs */

/*
 * Directory entry descriptor.
 * This structure is part of file handle and
 * is intended to directory handlers usage.
 */
typedef struct DOS_DIR_HDL
    {
    u_int	parDirStartCluster;	/* sector containing parent */
    					/* directory entry */
    block_t	sector;	/* sector containing  alias (short name) */
			/* directory entry */
    off_t	offset;	/* alias (short name) directory entry offset  */
			/* in sector */
    block_t	lnSector;	/* sector containing long name start */
    off_t	lnOffset;	/* long name offset in sector */
    cookie_t	cookie;	/* for cache quick access */
    
    /* implementation private usage */
    
    u_long	dh_Priv1;
    u_long	dh_Priv2;
    u_long	dh_Priv3;
    u_long	dh_Priv4;
    
    u_char	crc;	/* crcon directory entry (not used) */
    } DOS_DIR_HDL;
typedef DOS_DIR_HDL *	DOS_DIR_HDL_ID;

/*
 * File FAT descriptor.
 * This structure is part of file descriptor and
 * is intended to FAT handlers usage.
 */
typedef struct DOS_FAT_HDL
    {
    uint32_t	lastClust;	/* number of last passed cluster in chain */
    uint32_t	nextClust;	/* contents of <lastClust> entry */
    cookie_t	cbioCookie;	/* CBIO dev. cookie for last accessed sector */
    uint32_t	errCode;	/*  */
    } DOS_FAT_HDL;
typedef DOS_FAT_HDL *	DOS_FAT_HDL_ID;

/*
 * File handle.
 * This structure is shared by all file descriptors (see below).
 * that are opened for the same file.
 * Field <pDirHdl> is filled and used by directory handler.
 */
typedef struct DOS_FILE_HDL
    {
    fsize_t	size;		/* recent file size */
    u_int	deleted  : 1,	/* file deleted */
    		obsolet  : 1,	/* file was changed, but not closed yet */
    		changed  : 1,	/* file was changed, but not closed yet */
    		res	 : 29;

    /* FAT fields */

    uint32_t	startClust;	/* file start cluster */
    				/* ( 0 - not allocated yet ) */
    uint32_t	contigEndPlus1;	/* end clust. of contiguous area in file + 1 */
    uint32_t	fatSector;	/* last modified FAT sector */

    DOS_DIR_HDL	dirHdl;		/* directory entry descriptor */

    u_short	nRef;		/* reference count - the number of times */
    				/* file is simultaneously opened */
    u_char	attrib;		/* file attributes */
    } DOS_FILE_HDL;

typedef DOS_FILE_HDL *	DOS_FILE_HDL_ID;

/*
 * File descriptor.
 * This structure is allocated every time new file is opened.
 * Fields <curSec>, <nSec>, <cluster> are filled by fat handler
 * via get next cluster operation and then are corrected by other
 * modules in accordance with operation being executed.
 */
typedef struct DOS_FILE_DESC
    {
    DOS_VOLUME_DESC_ID	pVolDesc;	/* volume descriptor ptr */
    DOS_FILE_HDL_ID	pFileHdl;	/* file handle, that is shared */
    					/* between file descriptors */
    					/* of the same file */
    fsize_t	pos;		/* current absolute position in file */
    	    	    	    	/* for directory: absolute offset from */
				/* from directory start */
    fsize_t	seekOutPos;	/* save position in case seek passes EOF */
    	    	    	    	/* contains 0, if no such seek occurred */
    block_t	curSec;		/* current data sector */
    u_int	nSec;		/* number of contiguous sectors */
    cookie_t	cbioCookie;	/* last accessed sector ptr */
    				/* ( is filled by cbio ) */
    u_char	accessed;	/* file was accessed */
    u_char	changed;	/* file was changed */
    
    DOS_FAT_HDL	fatHdl;
    
    u_char	openMode;	/* open mode ( DRONLY/WRONLY/RDWR ) */
    
    BOOL	busy;
    } DOS_FILE_DESC;

typedef DOS_FILE_DESC *	DOS_FILE_DESC_ID;

/*
 * FAT handler descriptor.
 * This structure defines generic FAT handler API.
 * It must be start field in every FAT handler
 * descriptor structure.
 */
typedef struct DOS_FAT_DESC
    {
    /* interface functions */
    
    STATUS (*getNext)( DOS_FILE_DESC_ID pFd, u_int allocatePolicy );
    			/* get/allocate next cluster for file */
			/* <allocatePolicy> : FAT_NOT_ALLO/FAT_ALLOC/ */
			/* FAT_ALLOC_ONE */
    STATUS (*contigChk)( DOS_FILE_DESC_ID pFd );
    			/* check file chain for contiguity */
    STATUS (*truncate)( DOS_FILE_DESC_ID pFd, uint32_t sector,
                        uint32_t flag );
    			/* truncate chain starting from <sector>, */
			/* <sector> = FH_FILE_START - from file start; */
			/* <flag> : FH_INCLUDE/FH_EXCLUDE */
    STATUS (*seek)( DOS_FILE_DESC_ID pFd, uint32_t startSec, uint32_t nSec );
    			/* seek <nSec> sectors starting from */
    			/* <startClust>, */
			/* <startClust> = FH_FILE_START - from file start */
    fsize_t (*nFree)( DOS_FILE_DESC_ID pFd );
			/* free bytes on disk */
    STATUS (*contigAlloc)( DOS_FILE_DESC_ID pFd, uint32_t nSec );
			/* allocate <nSec> contiguous first feet chain */
    size_t (*maxContig)( DOS_FILE_DESC_ID pFd );
			/* max free contiguous chain length in sectors */
    void (*volUnmount)( DOS_VOLUME_DESC_ID pVolDesc );
    			/* free all resources, that were allocated */
    			/* for the volume */
    void (*show)( DOS_VOLUME_DESC_ID pVolDesc );
			/* display handler specific data */
    STATUS (*flush)( DOS_FILE_DESC_ID pFd );
			/* flush all internal FAT handler data that */
			/* belongs to the file */
    void (*syncToggle)( DOS_VOLUME_DESC_ID pVolDesc, BOOL syncEnable );
			/* toggle FAT copies mirroring; synchronize */
			/* FAT copies, when mirror is being enabled */
    STATUS (*clustValueSet)
        (
        DOS_FILE_DESC_ID	pFd,
    	uint32_t	fatCopyNum,	/* FAT copy to use */	
        uint32_t	clustNum,	/* cluster number to set */
        uint32_t	value,		/* DOS_FAT_AVAIL, DOS_FAT_EOF, */
					/* DOS_FAT_BAD, DOS_FAT_ALLOC, */
					/* DOS_FAT_INVAL, DOS_FAT_RESERV, */
					/* DOS_FAT_RAW */
        uint32_t	nextClust	/* next cluster number */
        );
    uint32_t (*clustValueGet)
        (
        DOS_FILE_DESC_ID	pFd,
    	uint32_t	fatCopyNum,	/* FAT copy to use */	
        uint32_t	clustNum,	/* cluster number to check */
        uint32_t *	pNextClust	/* return cluster value */
        );

    uint8_t	activeCopyNum;	/* number of active FAT copy */
    } DOS_FAT_DESC;

typedef DOS_FAT_DESC * 	DOS_FAT_DESC_ID;

/*
* Directory handler descriptor.
 * This structure defines generic directory handler API.
 * It must be start field in every directory handler
 * descriptor structure.
 */
typedef struct DOS_DIR_DESC
    {
    STATUS (*pathLkup)( DOS_FILE_DESC_ID pFd, void * pPath,
    		        u_int creatFlags );
    			/* lkup path file in directory hierarchy tree */
    STATUS (*readDir)( DOS_FILE_DESC_ID pFd, DIR * pDir,
		       DOS_FILE_DESC_ID pResFd );
    			/* regular readdir; <pResFd> if not NULL, is */
			/* filled for entry just accepted */
			/* on exit <pdir->dd_coocie> should contain */
			/* POS_TO_DD_COOKIE( pFd->pos ) */
    STATUS (*updateEntry)( DOS_FILE_DESC_ID pFd, u_int flags,
			   time_t curTime );
    			/* set directory entry values; */
    			/* size, start cluster, attributes and so on */
    			/* are delivered from file descriptor */ 
    			/* <flags> can be or-ed of */
    			/* DH_TIME_CREAT, DH_TIME_MODIFY, DH_TIME_ACCESS */
    	    	    	/* to encode <curTime> value into correspondence */
			/* field or DH_DELETE for to delete the entry */
    STATUS (*dateGet)( DOS_FILE_DESC_ID pFd, struct stat * pStat );
    			/* fill in date-time fields in stat structure */
    STATUS (*volLabel)( DOS_VOLUME_DESC_ID pVolDesc, u_char * label,
			u_int operation );
			/* get/set volume label */
			/* <operation> is one of FIOLABELGET/FIOLABELSET */
    STATUS (*nameChk)( DOS_VOLUME_DESC_ID pVolDesc, u_char * name );
			/* validate name (intended to check disk) */
    void (*volUnmount)( DOS_VOLUME_DESC_ID pVolDesc );
    			/* free all resources, that were allocated */
    			/* for the volume */
    void (*show)( DOS_VOLUME_DESC_ID pVolDesc );
			/* display handler specific data */
    u_int	rootStartSec;	/* root directory start sector: */
    				/* some value for FAT12/FAT16; */
    				/* 0 for FAT32; is filled by dir handler */
    u_int	rootNSec;	/* sectors per root directory */
    				/* some value for FAT12/FAT16; */
    				/* 0 for FAT32; is filled by dir handler */
    } DOS_DIR_DESC;

typedef DOS_DIR_DESC * 	DOS_DIR_DESC_ID;

/* check disk work structure */ 
 
typedef struct CHK_DSK_DESC 
    { 
    UINT32 *	chkFatMap;      /* clusters usage map */
    DIR		chkDir;        /* readdir buffer */
    fsize_t	chkTotalFSize;  /* total bytes in all files */
    fsize_t     chkTotalBytesInLostChains;
    u_int	nErrors;	/* total number of errors detected */
    UINT32      chkNDirs;       /* total directories */
    UINT32      chkNFiles;      /* total files */
    UINT32      chkNLostChains; /* total lost chains */
    UINT32      chkNFreeClusts; /* total # of free clusters */
    UINT32      chkNBadClusts;	/* total # of bad clusters */
    time_t	chkMaxCreatTime;
    time_t	chkMaxModifTime;
    time_t	chkMaxAccTime;
    BOOL	bufToDisk;	/* FAT copy used as tmp buffer */
    struct stat	stat;		/* buffer to accept file statistics */
    u_char	chkPath[ CHK_MAX_PATH ];	/* buffer for path strings */
    u_char	curPathLev;	/* dynamic counter of path levels */
    char	chkCurPath[ CHK_MAX_PATH + 256 ];
    } CHK_DSK_DESC;

typedef CHK_DSK_DESC * CHK_DSK_DESC_ID;

/* Volume descriptor */

typedef struct DOS_VOLUME_DESC
    {
    DEV_HDR     	devHdr;		/* i/o system device header */
    u_int		magic;		/* control magic number */
    					/* DOS_FS_MAGIC */
    BOOL		mounted;	/* volume mounted */
    CBIO_DEV_ID 	pCbio;		/* cached I/O device handle */
    DOS_DIR_DESC_ID	pDirDesc;	/* directory handler descriptor ptr */
    DOS_FAT_DESC_ID	pFatDesc;	/* FAT handler descriptor ptr */
    SEM_ID		devSem;		/* device mutual semaphore, */
    SEM_ID		shortSem;	/* protect very short operations, */
    					/* such as allocation of file */
    					/* descriptor */
    DOS_FILE_DESC_ID	pFdList;	/* file descriptors array */
    DOS_FILE_HDL_ID	pFhdlList;	/* file handles array */
    SEM_ID *		pFsemList;	/* file semaphores array */
    
    /* -- boot sector data -- */
    
    u_int	bootSecNum;	/* number of sector containing boot */
				/* information */
    UINT32	volId;		/* volume Id */
    UINT32	secPerFat;	/* sectors per FAT copy */
    UINT32	nHiddenSecs;	/* hiden sectors (unused tail) */
    UINT32	totalSec;	/* total number of sectors */
    UINT16	bytesPerSec;
    UINT16	secPerClust;
    UINT16	nReservedSecs;	/* reserved sectors (ahead first fat copy) */
    UINT8	nFats;		/* number of FAT copies */
    UINT32	nFatEnts;	/* number of entries in FAT */
    char	bootVolLab[ DOS_VOL_LABEL_LEN];	/* volume label */
    /*     --    --       */
    
    enum { FAT12, FAT16, FAT32 } fatType;

    u_int	dataStartSec;		/* sector number of the start */
    					/* cluster (DOS_MIN_CLUCT) */
    u_short	maxFiles;		/* maximum open files in a time */
    u_short	nBusyFd;		/* number of fd-s in use */
    u_short	volIdOff;		/* offset of volume Id field */
					/* in boot sector */
    u_short	volLabOff;		/* offset of volume label field */
					/* in boot sector */
    u_char	secSizeShift;		/* log2( bytesPerSect ) */

    /* check disk fields */

    CHK_DSK_DESC_ID	pChkDesc;
    u_char		autoChk;	/* autocheck on mount level */
    u_char		autoChkVerb;	/* autocheck verbosity */
    u_char		chkLevel;       /* check disk progress level */
    u_char		chkVerbLevel;   /* check disk verbosity level */

    FUNCPTR	fatVolMount;
    BOOL                updateLastAccessDate; /* SPR#68203 */
    BOOL                volIsCaseSens;  /* Make SPR#29751 fix switchable */
    } DOS_VOLUME_DESC;

typedef struct DOS_HDLR_DESC
    {
    u_int	id;	/* unique handler Id */
			/* 0 - 256 reserved by implementation */
    STATUS (*mountRtn)( DOS_VOLUME_DESC_ID pVolDesc, void * arg );
    void *	arg;	/* handler dependent argument */
    }	DOS_HDLR_DESC;

typedef DOS_HDLR_DESC *	DOS_HDLR_DESC_ID;
    	    
/* function pointers to chk and fmt handlers */
IMPORT STATUS (*dosFsChkRtn)( DOS_FILE_DESC_ID pFd ); /* check disk routine */
IMPORT STATUS (*dosFsVolFormatRtn)( void * dev, int opt,
                                    FUNCPTR pPromptFunc );
/* handler lists for fat and dir handlers */
IMPORT DOS_HDLR_DESC	dosFatHdlrsList[];
IMPORT DOS_HDLR_DESC	dosDirHdlrsList[];

/* forward declarations */

IMPORT STATUS dosFsHdlrInstall( DOS_HDLR_DESC_ID hdlrsList,
			        DOS_HDLR_DESC_ID hdlr );

IMPORT int dosFsVolIsFat12 (u_char * pBootBuf); /* pick fat12 or fat16 */

#ifdef __cplusplus
    }
#endif

#endif /* __INCdoFsLibP */

