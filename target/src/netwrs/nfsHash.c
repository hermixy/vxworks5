/* nfsHash.c - file based hash table for file handle to file name and reverse */

/* Copyright 1998 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"
 
/*
modification history
--------------------
01b,19oct01,rae  merge from truestack ver 01g, base o1a (SPRs 62705, 35767)
01a,15nov98,rjc  written
*/

/*
INTERNAL
DESCRIPTION
The nfs server is required to maintain a mapping of file names to file
handles since the file handle is of limited size but nfs  requests
use a file handle to id the file. This map is being implemented as a 2 
way hash table so given a file name we can find out the corresponding
file handle and correspondingly get the file name given the file handle.

The name to handle operation would typically be done infrequently
compared to the handle to name op since the former corresponds to 
opening the file while the latter is needed for every nfs request. 
Therefore the hash table has been implememnted with a bias towards
keeping the latter operation more efficient.

Conceptually the hash table is simple. Given a hash table entry we
compute the hash value for each of the 2 hashing functions and then 
plece the entry into two lists. While deleting we remove it from both
lists. However since we must keep this data structure on disk we
must avoid the typical in memory implementation as a linked list of 
entries. That's because it is inefficient to implement a fragmentation/
compaction memory management system as is done in the case of the
standard malloc/free based implementation.

Our design allocates the disk space in larger chunks (than may be needed
to store a single entry) and uses a bit map to keep track of free blocks.


The hash table is maintained in a dos file. The file offset is used as
a pointer.

Data structures

File block bit map table

This structure which is maintained in memory contains a bit per block
of the dos file. In order to locate a free block we simply scan this bit 
from the start until a free block is found. If none are found we would
expand the hash file by seeking beyond the existing end of the file 
and update the bit map block correspondingly. Freeing a block is done by 
turning off the bit. Currently we do not reduce the size of the file
as it would be extremely inefficient in general (in case free block is not
at the end of the file. Free blocks at the end of the list could be 
returned to the file system by truncating the file.


Contents of a hash table block.
 
Each block contains a sequence of entries followed by a pointer to 
the next block in the list keyed by file handle. In other words all entries
in a block and the blocks after it, would have hashed to the same value,
and we can locate a required entry given the file handle by examining the
entries in a block and traversing the linked list of blocks

Contents of an entry -

Each entry consists of a triple comprising a next entry pointer, the
file handle , and the file name. The next entry pointer points to the next
entry in the hash chain, keyed by file name. Unlike the file handle based 
chain these entries are not organized into blocks as doing so would require 
that we maintain a duplicate entry since in general entries which hash into
a block when keyed by file handle will not hash into the same block when
keyed by file name. While maintaining duplicates will work around this
problem we would end up doubling the storage requirements and increasing
the size of this file would increase search times anyway and most likely
negate most of the benefits of this organization.

To use this feature, include the following component:
INCLUDE_NETWRS_NFSHASH

NOMANUAL
*/


/* includes */
#include "vxWorks.h"
#include "unistd.h"
#include "xdr_nfs.h"
#include "string.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "memLib.h"
#include "nfsdLib.h"
#include "ctype.h"
#include "sys/stat.h"
#include "semLib.h"
#include "memPartLib.h"

/* defines */
#define MAX_DEVS 10
#define MAX_MNTS 20
#define MAX_HASH_BLKS  10000
#define HASH_BKT_SZ   (1 * 1024)
#define CHAR_BIT_LEN    8
#define INT_BIT_LEN     (sizeof (int) * CHAR_BIT_LEN)
#define BAD_OFFSET      ((off_t)(~0x0))
#define FH_HASH_TBL_LEN      512
#define NAME_HASH_TBL_LEN    512
#define MIN_ENTRY_SZ    (sizeof (HNDL_NAME))
#define STR_INT_LEN(s)  ((strlen (s) + 1 + sizeof (int) - 1) / sizeof (int))
#define STR_ALIGN_LEN(s) (((strlen (s) + 1 + sizeof (int) - 1) / sizeof (int)) \
			   *  sizeof (int))

#define HASH_BKT_HDR_SZ    (sizeof (off_t) + sizeof (int))
#define ENTRY_HDR_SZ    (sizeof (HNDL_NAME) - sizeof(int))

/* forward declaration */
LOCAL int blkRead (int, off_t, char *, int);
LOCAL int blkWrite (int, off_t, char *, int );


/* typedefs */
/* descriptor for a nfs hashtable */
typedef struct tblDesc  
    {
    int     allocBits [MAX_HASH_BLKS/INT_BIT_LEN + 1];  /* bits allocated */
    int     numBlocks;                          /* number of blocks allocated */
    int     fd;              /* file descriptor of file containing hash table*/
    off_t   nameHash [NAME_HASH_TBL_LEN - 1];  /* hash by name table */
    int     nextInode;        /* next inode number to allocate */
    int     refCnt;           /* reference count */
    SEM_ID  sem;              /* protection lock */
    char    nmBuf [NFS_MAXPATHLEN];  /* file name for hash table */
    }  TBL_DESC;

/* 
 * nfs handle and name pairs are stored in this structure. This structure 
 * is contained within the hash buckets which are written to disk. These
 * structures will have overall length a multple of sizeof (int) so
 * that  the next field stays aligned on a word boundary when in
 * memory 
 */

typedef struct hndlName
    {
    off_t        next;         /* next entry ptr */
    off_t        prev;         /* prev entry ptr */
    short        totalLen;     /* total length of entry */
    short        nameLen;      /* length of name string, 0 for free netry */
    int          fh;     /* file handlei/inode  */
    char         name [sizeof (int)]; /* actual str length is upto max 
					 pathname lwngth, this is just for 
					 a minimum string. Entry length must 
					 be a multiple of sizeof (int) for
					 alignment */
    }   HNDL_NAME;


/* 
 * hash bucket contains a number of HNDL_NAME pairs along with free space
 */

typedef struct hashBkt  
    {
    off_t        next;       /* next bkt ptr */
    int          freeSpace;  /* free space remaining in bucket */
    char         entries [HASH_BKT_SZ - sizeof (off_t) - sizeof (int)];
			  /* buffer for entries of type HNDL_NAME */
    }  HASH_BKT;


/* 
 * Mapping from a file system device id to corresponding hash table descriptor.
 * There is one hash table per file system device if there are nfs exported
 *  directories in that file system
 */

typedef struct devIdPair
    {
    int         dev;
    TBL_DESC *  pTbl;
    } DEV_ID_PAIR  ;


/* 
 * Mapping from the nfs volume mount id to the hash table descriptor.
 * The nfs volume id is generated in the nfsExport () routine and is
 * maintained within the nfs file handle.
 */
typedef struct mntIdPair
    {
    int         mntId;
    TBL_DESC *  pTbl;
    } MNT_ID_PAIR;


/* locals */

/* device to hash table descriptor table */
LOCAL DEV_ID_PAIR   devTbl[MAX_DEVS];
/* mount volume  to hash table descriptor table */
LOCAL MNT_ID_PAIR   mntTbl[MAX_MNTS];

/******************************************************************************
* 
* insertMntId - insert entry into mount id based table 
*
* Inserts pointer <pTbl> to a hash file table descriptor in the mount id
* based table for nfs mount point <mntId>
*
* NOMANUAL
*
* RETURNS: N/A.
*/

LOCAL void insertMntId 
    (
    TBL_DESC *    pTbl,
    int           mntId
    )
    {
    MNT_ID_PAIR *   pMnt;

    for (pMnt = mntTbl; pMnt < mntTbl + MAX_MNTS; ++pMnt)
	{
	if (pMnt->mntId == 0)
	    {
	    pMnt->mntId = mntId;
	    pMnt->pTbl = pTbl;
	    return ;
	    }
        }
    }

/******************************************************************************
* 
* getTblEnt - get entry from mnt table 
*
* Returns ptr to table descriptor for give mount id <id>.
*
* RETURNS: ptr to entry or NULL
*/

LOCAL TBL_DESC *  tblDescGet 
    (
    int   id   /* mount id */
    )
    {
    int ix;

    for (ix = 0; ix < MAX_MNTS; ++ix)
	{
	if (mntTbl[ix].mntId ==  id)
	    return (mntTbl[ix].pTbl);
        }

    return (NULL);
    }

/******************************************************************************
* 
* nfsHashTblInit - initialize a hash table 
*
* Initialize a hash table given the corresponding descriptor <pTbl>
* creating the hash file in dir <dirName>.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS nfsHashTblInit
    (
    TBL_DESC *   pTbl,
    char *       dirName
    )
    {
    off_t *      pOff;
    HASH_BKT     initBuf;
    HNDL_NAME *  pEnt;
    int          ix;

    memset ((char*) pTbl->allocBits, 0, sizeof (pTbl->allocBits));
    for (pOff = pTbl->nameHash; pOff < (pTbl->nameHash + NAME_HASH_TBL_LEN); 
	 ++pOff)
	{
	*pOff = BAD_OFFSET;
	}
     sprintf  (pTbl->nmBuf, "%s/leofs", dirName);

     if ((pTbl->fd = open (pTbl->nmBuf, O_CREAT | O_RDWR | O_TRUNC, 0777)) 
	 == ERROR)
	 {
	 printf ("cannot create file %s\n", pTbl->nmBuf);
	 return (ERROR);
	 }
     /* create the fh hash blocks in the file */
     pTbl->numBlocks = FH_HASH_TBL_LEN;
     initBuf.next = BAD_OFFSET;
     initBuf.freeSpace = sizeof (initBuf) - sizeof (initBuf.next) - 
			 sizeof (initBuf.freeSpace);
     pEnt = (HNDL_NAME*)initBuf.entries;
     pEnt->totalLen = initBuf.freeSpace;
     pEnt->nameLen = 0;
     for (ix = 0; ix < FH_HASH_TBL_LEN ; ++ix)
	 {
	 if (write (pTbl->fd, (char *)&initBuf, sizeof (initBuf)) 
	     != sizeof (initBuf))
	     {
	     return (ERROR);
	     }
	 }
     pTbl->refCnt = 0;
     pTbl->nextInode = 0;
     pTbl->sem = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
     if (pTbl->sem == NULL)
	return (ERROR);
     return (OK);
     }


/******************************************************************************
*  
* nfsHashTblSet - set up nfs hash table
*
* Set up an nfs hash table for exported directory <pDir> with mount voleume id
* <mntId> and file system device id <devId>. If a hash fiel already exists
* for <devId> then we simply need to setup a ptr to the existing hash table
* descriptor alse a new hash file must be allocated adn initialized.
*
* RETURNS: N/A
*/

void nfsHashTblSet
    (
    char *   pDir,     /* directory name */
    int      mntId,    /*mount id */
    u_long   devId     /* device Id */
    )
    {
    DEV_ID_PAIR *   pDev;

    for (pDev = devTbl; pDev < devTbl + MAX_DEVS; ++pDev)
	{
	if (pDev->dev != 0 && pDev->dev == devId)
	    break;
	}
    if (pDev >= devTbl + MAX_DEVS)
	{
        for (pDev = devTbl; pDev < devTbl + MAX_DEVS; ++pDev)
	    {
	    if (pDev->dev == 0)
		{
		pDev->dev = devId;
		pDev->pTbl = KHEAP_ALLOC (sizeof (TBL_DESC));
		if (nfsHashTblInit (pDev->pTbl, pDir) == ERROR)
		    {
		    printf ("cannot initialize nfs hash file\n");
		    KHEAP_FREE((char *)pDev->pTbl);
		    return;
		    }
		break;
                }
            }
        }
    pDev->pTbl->refCnt++;
    insertMntId (pDev->pTbl, mntId);
    }


/******************************************************************************
*  
* nfsHashTblUnset - clean up  nfs hash table
*
* Clean up an nfs hash table for exported directory <pDir> with mount volume id
* <mntId> and file system device id <devId>. Delete the allocated table 
* descriptor if there are no other references to it. 
*
* RETURNS: N/A
*/

void nfsHashTblUnset
    (
    char *   pDir,     /* directory name */
    int      mntId    /*mount id */
    )
    {
    DEV_ID_PAIR *   pDev;
    MNT_ID_PAIR *   pMnt;
    TBL_DESC *      pTbl;

    for (pMnt = mntTbl; pMnt < mntTbl + MAX_MNTS; ++pMnt)
	{
	if (pMnt->mntId  == mntId)
	    break;
	}
    if (pMnt >= mntTbl + MAX_MNTS)
	{
	return;
	}
    else
	{
	pTbl = pMnt->pTbl;
	pMnt->mntId = 0;
	pMnt->pTbl -= 0;
	pTbl->refCnt--;
	if (pTbl->refCnt == 0)
	    {
	    close (pTbl->fd);
	    unlink (pTbl->nmBuf);
	    semDelete (pTbl->sem);
	    for (pDev = devTbl; pDev < devTbl + MAX_DEVS; ++pDev)
		{
		if (pDev->pTbl == pTbl)
		    {
		    KHEAP_FREE (pTbl);
		    pDev->pTbl = 0;
		    pDev->dev = 0;
		    }
	         }
             }
         }
    }


/******************************************************************************
*  
* blockAlloc - allocates block from the file to be used as a hash bucket 
*
* The length of the file being used is incremented by HASH_BKT_SZ
* and the bit map updated to reflect that. The bit corresponding to
* the allocated block is turned off marking the block as being in use,
* so in case caller does not actually use the block he shuld free the block
* with blockFree below
*
* RETURNS: file offset of allocated block else BAD_OFFSET
* NOMANUAL
*/

LOCAL off_t blockAlloc
    (
    TBL_DESC *     pTbl
    )
    {
    int *  pBits = pTbl->allocBits;
    int *  pMax = pTbl->allocBits + 
		  ((pTbl->numBlocks + INT_BIT_LEN -1) / sizeof (int));  
		  /* 1 beyond last word */
    char *  pIx;
    int     bitPos; /* bit position of allocated block */
    int     ix;


    if (pTbl->numBlocks >= MAX_HASH_BLKS)
	{
	return (BAD_OFFSET);
	}

    for ( ; *pBits == 0 && pBits < pMax ; ++pBits)
	{}

    if (pBits == pMax)
	{
        /* allocate a new block in the file at the end */

	if (lseek (pTbl->fd, (pTbl->numBlocks + 1) *  HASH_BKT_SZ - 1, 
	    SEEK_SET) == ERROR)
	    {
	    return (BAD_OFFSET);
	    }
        pTbl->numBlocks ++;

	return ((pTbl->numBlocks - 1) * HASH_BKT_SZ);
	}
    else
        {
	/* existing free block available */
	/* scan byte wise then bit wise */
	pIx = (char *) pBits;
	for (; *pIx == 0; ++pIx)
	    {};
        for (ix = 0; ix < CHAR_BIT_LEN; ++ix)
	    {
	    if (*pIx & (0x1 << ix))
		{
		bitPos =  (pIx - (char*)pTbl->allocBits) * CHAR_BIT_LEN + 
			  (CHAR_BIT_LEN - ix) - 1;
		*pIx &= ~(0x1 << ix);  
		return (bitPos * HASH_BKT_SZ);
		}
            }

	/* impossible to reach here */
        }
    return (BAD_OFFSET);
    }

/******************************************************************************
*  
* namehashFn - hash by name function
*
* RETURNS: index into hash table
*/

LOCAL int  nameHashFn 
    (
    char *  pName
    )
    {
    int   i = strlen (pName);

    return (pName[i - 2] + pName[i - 1]) % NAME_HASH_TBL_LEN;
    }

/******************************************************************************
*  
* namehashFn - hash by file handle function
*
* Index into hash table, must be multiplies by the hash file bucket size
* to get the actual file offset.
*
* RETURNS: index into hash table
*/

LOCAL int fhHashFn 
    (
    int   fh 
    )
    {
    return  fh % FH_HASH_TBL_LEN;
    }


/******************************************************************************
*  
* name2Fh - convert name to file handle 
*
* Given a file name <pName>, convert that to a file handle, using the hash 
* table given by <ptbl>. We only do a lookup, so if the name does not exist 
* in the lookup structure we do not create the file handle for it. 
*
* RETURNS: N/A
*/

LOCAL int  name2Fh
    (
    TBL_DESC *   pTbl,
    char *       pName
    )
    {
    off_t        off_1 = 0;
    char         buf [NFS_MAXPATHLEN + 1 + sizeof (HNDL_NAME)];
    HNDL_NAME *  pEnt;
    int          nameLen = strlen (pName);

    semTake (pTbl->sem, WAIT_FOREVER);
    for (off_1 = pTbl->nameHash [nameHashFn(pName)];
	off_1 != BAD_OFFSET;
	off_1 = pEnt->next
	)
	{
	if (blkRead (pTbl->fd, off_1, buf, sizeof buf) == ERROR)
	    {
	    semGive (pTbl->sem);
	    return (ERROR);
	    }
        pEnt = (HNDL_NAME*)buf;
        if (pEnt->nameLen == nameLen && (strcmp (pEnt->name, pName) == 
					 0)) 
            {
	    semGive (pTbl->sem);
	    return (pEnt->fh);
	    }
        }
    semGive (pTbl->sem);
    return (ERROR);
    }


/******************************************************************************
*  
* fh2Name - convert  file handle  to name
*
* Find the file name corresponding to a file handle <fh> using hahs table
* <pTbl> and return name in <pName>.
*
* RETURNS: OK else ERROR if name not found.
*/

LOCAL STATUS  fh2Name
    (
    TBL_DESC *  pTbl,            /* hash file descriptor ptr */
    int         fh,             /* file handle */
    char *      pName            /* name output buffer */
    )
    {
    off_t              off_1 ;
    HNDL_NAME *        pEnt;
    HASH_BKT  *        pBlk;
    char               buf [HASH_BKT_SZ];
    struct stat        statBuf;
    HNDL_NAME *        pTmpEnt;
    char               entBuf [NFS_MAXPATHLEN + 1 + sizeof (HNDL_NAME)];

    semTake (pTbl->sem, WAIT_FOREVER);
    for (off_1 = fhHashFn(fh) *  HASH_BKT_SZ; off_1 != BAD_OFFSET;
	 off_1 = pBlk->next
	)
	{
	if (blkRead (pTbl->fd, off_1, buf, sizeof buf) == ERROR)
	    {
	    semGive (pTbl->sem);
	    return (ERROR);
	    }
	pBlk = (HASH_BKT *) buf;
        pEnt = (HNDL_NAME*) pBlk->entries;

	for (pEnt = (HNDL_NAME *)pBlk->entries; 
	     (char*)pEnt < (buf + sizeof(buf) - MIN_ENTRY_SZ); 
             pEnt = (HNDL_NAME*) ((char *)pEnt + pEnt->totalLen))
	    {
            if (pEnt->fh == fh) 
                {
		strcpy (pName, pEnt->name);
                semGive (pTbl->sem);
		if (stat(pName, &statBuf) == ERROR)
		    {
		    /* delete from hash file since handle is not valid */

                    if (blkRead (pTbl->fd, pEnt->next, entBuf, sizeof (entBuf))
			== ERROR)
			{
			return (ERROR);
			}

		    pTmpEnt = (HNDL_NAME*)entBuf;
		    pTmpEnt->prev = pEnt->prev;
                    if (blkWrite (pTbl->fd, pEnt->next, entBuf, sizeof (entBuf))
			== ERROR)
			{
			return (ERROR);
			}

                    if (blkRead (pTbl->fd, pEnt->prev, entBuf, sizeof (entBuf))
			== ERROR)
			{
			return (ERROR);
			}

		    pTmpEnt->next = pEnt->next;
                    if (blkWrite (pTbl->fd, pEnt->prev, entBuf, sizeof (entBuf))
			== ERROR)
			{
			return (ERROR);
			}

		    pEnt->nameLen = 0;
		    pBlk->freeSpace += pEnt->totalLen;
		    /* if all entries in a block are gone then we could free
		     * the block however that is not done currently  since the 
		     * space will most likely be used up again unless
		     * the hash table gets unbalanced.
		     */
		    }
    	        return (OK);
    	        }
            }
        }
    semGive (pTbl->sem);
    return (ERROR);
    }


/******************************************************************************
*  
* fhInsert - insert a file handle and name pair into hash table
* Inser the file handle <fh> and name <pName> into the hash table
* <pTbl>.
*
* RETURNS: OK or ERROR if unable to insert entry.
*/

LOCAL STATUS   fhInsert 
    (
    TBL_DESC *  pTbl,
    int         fh,
    char *      pName
    )
    {
    /* first select block based on hash by fh , insert into blk
       then onto list of entries by name */

    off_t            offPrev  = 0;
    off_t            offNow ;
    HNDL_NAME *      pEnt = 0;
    HNDL_NAME *      pTmpEnt;
    HASH_BKT *       pBlk = (HASH_BKT *) NULL;
    char             buf [HASH_BKT_SZ];
    int              need;
    int              ix;
    int              newEntryLen;
    char             entBuf [NFS_MAXPATHLEN + 1 + sizeof (HNDL_NAME)];

    need = ENTRY_HDR_SZ  +  STR_ALIGN_LEN (pName) ;
    semTake (pTbl->sem, WAIT_FOREVER);

    for (offNow = fhHashFn (fh) * HASH_BKT_SZ ; offNow != BAD_OFFSET;
	offPrev = offNow,  offNow = pBlk->next
	)
	{
	if (blkRead (pTbl->fd, offNow, buf, sizeof buf) == ERROR)
	    {
            semGive (pTbl->sem);
	    return (ERROR);
	    }
	pBlk = (HASH_BKT *)buf;
	if (pBlk->freeSpace < need)
	    {
	    continue;
	    }
	for (pEnt = (HNDL_NAME *)pBlk->entries; 
	     (char*)pEnt <= (buf + sizeof(buf) - need); 
             pEnt = (HNDL_NAME*) ((char *)pEnt + pEnt->totalLen))
	    {
	    if (pEnt->nameLen == 0 && pEnt->totalLen >= need)
		{
		/* found an empty entry big enough */
		goto scanDone;
                }
            }
        }

    /* At this point we may need to allocate a new blk */
    /* allocate new block and link it in then set pEnt and offNow
     * as in earlier case
     */

scanDone:

    if (offNow == BAD_OFFSET)
	{
        offNow = blockAlloc (pTbl);
	/* link new block into prev block */
	if (blkRead (pTbl->fd, offPrev, buf, sizeof buf) == ERROR)
	    {
	    semGive (pTbl->sem);
	    return (ERROR);
	    }
        pBlk = (HASH_BKT*)buf;
	pBlk->next = offNow;
	if (blkWrite (pTbl->fd, offPrev, buf, sizeof buf) == ERROR)
	    {
	    semGive (pTbl->sem);
	    return (ERROR);
	    }
	/* Now convert buf into a new empty block. 
	   initially the whole block is just one full sized empty entry */
	pBlk->next = BAD_OFFSET;
	pBlk->freeSpace = HASH_BKT_SZ -  HASH_BKT_HDR_SZ;
	pEnt = (HNDL_NAME*) (pBlk->entries);
        pEnt->totalLen = HASH_BKT_SZ -  HASH_BKT_HDR_SZ ;
	pEnt->nameLen = 0;
	}
    /* At this point offNow points to the block to be updated and
       pEnt points to the correct entry in the buffered bucket
     */

    ix = nameHashFn (pName);
    pEnt->next = pTbl->nameHash [ix];
    pEnt->prev = BAD_OFFSET;
    pTbl->nameHash[ix] = offNow + ((char *)pEnt - buf);
    /* update back ptr of next entry if there is one */
    if (pEnt->next != BAD_OFFSET)
	{
        if (blkRead (pTbl->fd, pEnt->next, entBuf, sizeof (entBuf) == ERROR))
	    {
	    return (ERROR);
	    };
        pTmpEnt = (HNDL_NAME*)entBuf;
        pTmpEnt->prev = pTbl->nameHash[ix];
        if (blkWrite (pTbl->fd, pEnt->next, entBuf, sizeof (entBuf) == ERROR))
	    {
	    return (ERROR);
	    };
	}
    /* if there is sufficient space in this entry to accomodate
     * another entry at this end then divide this to get an empty entry
     * at the end of first entry
     */
    newEntryLen = ENTRY_HDR_SZ + STR_ALIGN_LEN (pName);
    if (pEnt->totalLen - newEntryLen >= MIN_ENTRY_SZ)
       {
       pTmpEnt = (HNDL_NAME*) ((char*)pEnt + newEntryLen);
       pTmpEnt->totalLen = pEnt->totalLen - newEntryLen;
       pTmpEnt->nameLen = 0;
       pEnt->totalLen = newEntryLen;
       }
    pBlk->freeSpace -= pEnt->totalLen;
    strcpy (pEnt->name, pName);
    pEnt->fh = fh;
    pEnt->nameLen = strlen (pName);
    if (blkWrite (pTbl->fd, offNow, buf, sizeof buf) == ERROR)
        {
        return (ERROR);
        }
    semGive (pTbl->sem);
    return (OK);
    }


/******************************************************************************
*  
* blkRead - read block from file
* 
* Read block from file with file descriptor <fd>, offset <offset>
* buffer <buf> and buffer len <len>
* 
* RETURNS: num chars read or ERROR.
*/

LOCAL   int blkRead 
    (
    int      fd,
    off_t    offset,
    char *   buf,
    int      len
    )
    {
    if (lseek (fd, offset, SEEK_SET) == ERROR)
        {
        return (ERROR);
        }
    return (read ( fd, buf, len));
    }



/******************************************************************************
*  
* blkWrite - write block to file
* 
* Write block to file with file descriptor <fd>, offset <offset>
* buffer <buf> and buffer len <len>
* 
* RETURNS: num chars written or ERROR.
 */

LOCAL int blkWrite 
    (
    int      fd,
    off_t    offset,
    char *   buf,
    int      len
    )
    {
    if (lseek (fd, offset, SEEK_SET) == ERROR)
        {
        return (ERROR);
        }
    return (write ( fd, buf, len));
    }


/******************************************************************************
*  
* nfsFhLkup - find name corresponding to file handle 
*
* using file handle <fh> as the key get corresponding file name
* in <pName>.
*
* RETURNS: OK or ERROR if name not found
*/

STATUS nfsFhLkup
    (
    NFS_FILE_HANDLE *   pFh,  /* key for lookup */
    char *              pName  /* output name */
    )
    {
    TBL_DESC *    pT;
    pT = tblDescGet (pFh->volumeId);
    if (pT == NULL)
       {
       return (ERROR);
	}


    if (fh2Name (pT, pFh->inode, pName) == ERROR)
	{
	return (ERROR);
	}
    else
	{
	return OK;
	}
    }


/******************************************************************************
*  
* nfsNmLkupIns - find fh corresponding to file name
*
* Find the file handle  corresponding to the given file name  <Pname> with
* nfs mount volume id <mntId> and create and insert an entry in the table
* if not found.
*
* RETURNS: file handle or ERROR;
* NOMANUAL
*/

int nfsNmLkupIns 
    (
    int    mntId,     /* mount id no */
    char * pName      /* seach key */
    )
    {
    TBL_DESC *    pT;
    int           in;

    pT = tblDescGet (mntId);
    if (pT == NULL)
       return (ERROR);
    if ((in = name2Fh (pT, pName)) == ERROR)
	{
	in = ++pT->nextInode;
	if (fhInsert (pT, pT->nextInode, pName)== ERROR)
	   return (ERROR);
	}
    return (in);
    }
