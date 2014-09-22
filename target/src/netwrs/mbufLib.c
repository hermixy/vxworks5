/* mbufLib.c - BSD mbuf interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,15oct01,rae  merge from truestack ver 01l, base 01h
01h,25aug98,n_s  corrected error handling of buffer allocate calls. spr #22104
01g,19sep97,vin  added clBlk specific code, changed pointers to refcounts
		 of clusters to refcounts since refcount has been moved to
                 clBlk
01f,12aug97,vin	 changed mBlkGet to take _pNetDpool
01e,22nov96,vin  added cluster support replaced m_get(..) with mBlkGet(..).
01d,29aug96,vin  made the code use only m_get.
01c,06jun96,vin  made compatible with BSD4.4 mbufs.
01b,13nov95,dzb  changed to validate mbufId.type off MBUF_VALID (SPR #4066).
01a,08nov94,dzb  written.
*/

/*
DESCRIPTION
This library contains routines to create, build, manipulate, and
delete BSD mbufs.  It serves as a back-end to the zbuf API provided
by zbufLib.

Note: The user should protect mbufs before calling these routines.  For many
      operations, it may be appropriate to guard with the spl semaphore.

NOMANUAL
*/

/* includes */

#include "vxWorks.h"
#include "zbufLib.h"
#include "mbufLib.h"
#include "errnoLib.h"
#include "intLib.h"
#include "stdlib.h"
#include "memPartLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* globals */

MBUF_ID			_mbufIdHead = NULL;	/* Head ID of free chain */

/* locals */

LOCAL BOOL		mbufInit = FALSE;	/* library initialized ? */
LOCAL MBUF_DESC		mbufDescHead = NULL;	/* Head of free desc list */

/* declare mbuf interface function table */

LOCAL ZBUF_FUNC		mbufFunc =
    {
    (FUNCPTR) _mbufCreate,
    (FUNCPTR) _mbufDelete,
    (FUNCPTR) _mbufInsert,
    (FUNCPTR) _mbufInsertBuf,
    (FUNCPTR) _mbufInsertCopy,
    (FUNCPTR) _mbufExtractCopy,
    (FUNCPTR) _mbufCut,
    (FUNCPTR) _mbufSplit,
    (FUNCPTR) _mbufDup,
    (FUNCPTR) _mbufLength,
    (FUNCPTR) _mbufSegFind,
    (FUNCPTR) _mbufSegNext,
    (FUNCPTR) _mbufSegPrev,
    (FUNCPTR) _mbufSegData,
    (FUNCPTR) _mbufSegLength
    };

/* static function declarations */

LOCAL void		_mbufBufFree (MBUF_DESC mbufDesc, VOIDFUNCPTR freeRtn,
                            int freeArg);
LOCAL MBUF_SEG *	_mbufSegFindPrev (MBUF_ID mbufId, MBUF_SEG mbufSeg,
		            int *pOffset);

/*******************************************************************************
*
* _mbufLibInit - initialize the BSD mbuf interface library
*
* This routine initializes the BSD mbuf interface library and publishes
* the mbufLib API to the caller.  Lists of free ID structures and
* free mbuf cluster descriptors are allocated in this routine.
*
* The mbufLib API func table "mbufFunc" is published to the caller upon
* completion of initialization.  Typically, this func table is used by
* the zbuf facility to call mbuf routines within this library to perform
* buffering operations.  Even though the zbuf interface defines the ZBUF_FUNC
* struct, it doesn't necessarily mean that zbufs must be present.  This
* library may be used even if zbufs have been scaled out of the system.
*
* RETURNS:
* A pointer to a func table containing the mbufLib API,
* or NULL if the mbuf interface could not be initialized.
*
* NOMANUAL
*/

void * _mbufLibInit (void)
    {
    int			ix;			/* counter for list init */

    if (mbufInit == TRUE)			/* already initialized ? */
        return ((void *) &mbufFunc);

    if ((_mbufIdHead = (MBUF_ID) KHEAP_ALLOC((sizeof (struct mbufId) *
	MBUF_ID_INC))) == NULL)			/* alloc space for ID list */
        return (NULL);

    if ((mbufDescHead = (MBUF_DESC) KHEAP_ALLOC((sizeof (struct mbufDesc) *
	MBUF_DESC_INC))) == NULL)		/* alloc space for desc list */
	{
	KHEAP_FREE((char *)_mbufIdHead);
        return (NULL);
	}
	
    /* divide up into an sll of free mbuf ID's, _mbufIdHead as the head */

    for (ix = 0; ix < (MBUF_ID_INC - 1); ix++)
        _mbufIdHead[ix].mbufIdNext = &_mbufIdHead[ix + 1];
    _mbufIdHead[ix].mbufIdNext = NULL;

    /* divide up into an sll of free buf desc, mbufDescHead as the head */

    for (ix = 0; ix < (MBUF_DESC_INC - 1); ix++)
        mbufDescHead[ix].mbufDescNext = &mbufDescHead[ix + 1];
    mbufDescHead[ix].mbufDescNext = NULL;

    mbufInit = TRUE;				/* init successful */

    return ((void *) &mbufFunc);		/* return mbuf func table */
    }

/*******************************************************************************
*
* _mbufCreate - create an empty mbuf ID
*
* This routine creates an mbuf ID, which remains empty (i.e., does not
* contain any mbufs) until mbufs are added by the mbuf insertion routines.
* Operations performed on mbufs require an mbuf ID, which is returned by
* this routine.
*
* RETURNS:
* An mbuf ID, or NULL if an mbuf ID could not be created.
*
* SEE ALSO: _mbufCreate
*
* NOMANUAL
*/

MBUF_ID _mbufCreate (void)
    {
    MBUF_ID		mbufId;			/* obtained mbuf ID */
    int			ix;			/* counter for list init */
    int			lockKey = intLock ();	/* int lock cookie */

    if ((mbufId = _mbufIdHead) != NULL)		/* free list empty ? */
        {
        _mbufIdHead = mbufId->mbufIdNext;	/* pop first ID off list */
        intUnlock (lockKey);

        mbufId->type = MBUF_VALID;		/* init new ID type */
        mbufId->mbufHead = NULL;		/* init new ID head */
        }
    else					/* list is empty */
	{
        intUnlock (lockKey);

        if ((mbufId = (MBUF_ID) KHEAP_ALLOC((sizeof (struct mbufId) *
            MBUF_ID_INC))) != NULL)		/* alloc more IDs */
            {
            for (ix = 0; ix < (MBUF_ID_INC - 1); ix++)	/* into an sll */
                mbufId[ix].mbufIdNext = &mbufId[ix + 1];

            lockKey = intLock ();
            mbufId[ix].mbufIdNext = _mbufIdHead;
            _mbufIdHead = mbufId->mbufIdNext;	/* hook head onto new list */
            intUnlock (lockKey);

            mbufId->type = MBUF_VALID;		/* init new ID type */
            mbufId->mbufHead = NULL;		/* init new ID head */
            }
        }

    return (mbufId);				/* return new ID */
    }

/*******************************************************************************
*
* _mbufDelete - delete an mbuf ID and free any associated mbufs
*
* This routine will free any mbufs associated with <mbufId>, then
* delete the mbuf ID itself.  <mbufId> should not be used after this
* routine executes successfully.
*
* RETURNS:
* OK, or ERROR if the mbuf ID could not be deleted.
*
* SEE ALSO: _mbufCreate
*
* NOMANUAL
*/

STATUS _mbufDelete
    (
    MBUF_ID		mbufId		/* mbuf ID to free */
    )
    {
    /* already free ? */
    
    if (mbufId == NULL || mbufId->type != MBUF_VALID)		
	{
	errno = S_mbufLib_ID_INVALID;		/* invalid if already free */
	return (ERROR);
	}

    MBUF_ID_DELETE(mbufId);			/* delete mbuf ID */

    return (OK);				/* return OK, always */
    }

/*******************************************************************************
*
* _mbufInsert - insert an mbuf chain into another mbuf chain
*
* This routine inserts all <mbufId2> mbufs into <mbufId1> at the
* specified byte location
*
* The location of insertion is specified by <mbufSeg> and <offset>.  Note
* that insertion within a chain occurs before the byte location specified
* by <mbufSeg> and <offset>.  Additionally, note that <mbufSeg> and <offset>
* must be "NULL" and "0", respectively, when inserting into an empty mbuf ID.
*
* After all the <mbufId2> mbufs are inserted into <mbufId1>, the mbuf ID
* <mbufId2> is deleted.  <mbufId2> should not be used after this routine
* executes successfully.
*
* RETURNS:
* The mbuf pointer associated with the first inserted mbuf,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufInsert
    (
    MBUF_ID		mbufId1,	/* mbuf ID to insert <mbufId2> into */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset,		/* relative byte offset */
    MBUF_ID		mbufId2		/* mbuf ID to insert into <mbufId1> */
    )
    {
    MBUF_ID		mbufIdNew;		/* dummy ID for dup operation */
    MBUF_SEG *		pMbufPrev;		/* mbuf prev to insert */
    MBUF_SEG		mbufEnd;		/* last Id2 mbuf */
    int			maxLen = MBUF_END;	/* offset for last Id2 byte */

    /* find the mbuf ptr previous to the point of insertion */

    if ((pMbufPrev = _mbufSegFindPrev (mbufId1, mbufSeg, &offset)) == NULL)
        return (NULL);

    if ((mbufEnd = _mbufSegFind (mbufId2, NULL, &maxLen)) == NULL)
	return (NULL);				/* find end mbuf of Id2 */

    if (offset == 0)				/* if prepend... */
	{
	mbufEnd->m_next = *pMbufPrev;
	*pMbufPrev = mbufId2->mbufHead;
        }
    else					/* else insert... */
        {
	mbufSeg = *pMbufPrev;			/* find insertion mbuf */

        if ((mbufIdNew = _mbufDup (mbufId1, mbufSeg, offset,
            mbufSeg->m_len - offset)) == NULL)
            return (NULL);			/* dup later portion */

        mbufSeg->m_len = offset;		/* shorten to first portion */
        mbufIdNew->mbufHead->m_next = mbufSeg->m_next;
        mbufEnd->m_next = mbufIdNew->mbufHead;	/* insert Id2 */
        mbufSeg->m_next = mbufId2->mbufHead;
        MBUF_ID_DELETE_EMPTY(mbufIdNew);	/* delete dup ID */
	}

    mbufEnd = mbufId2->mbufHead;		/* save head for return */

    if (mbufId2->type == MBUF_VALID)
	{
        MBUF_ID_DELETE_EMPTY(mbufId2);		/* delete ref to Id2 */
	}

    return (mbufEnd);				/* return inserted mbuf */
    }

/*******************************************************************************
*
* _mbufInsertBuf - create a cluster from a buffer and insert into an mbuf chain
*
* This routine creates an mbuf cluster from the user buffer <buf> and
* inserts it at the specified byte location in <mbufId>.
*
* The location of insertion is specified by <mbufSeg> and <offset>.  Note
* that insertion within a chain occurs before the byte location specified
* by <mbufSeg> and <offset>.  Additionally, note that <mbufSeg> and <offset>
* must be "NULL" and "0", respectively, when inserting into an empty mbuf ID.
*
* The user provided free routine <freeRtn> will be called when the mbuf
* cluster created from <buf> is not being referenced by any more mbufs.
* If <freeRtn> is NULL, the mbuf will function normally, except that the
* user will not be notified when no more mbufs reference cluster containing
* <buf>.  <freeRtn> will be called from the context of the task that last
* references the cluster.  <freeRtn> should be declared as follows:
* .CS
*       void freeRtn
*           (
*           caddr_t     buf,    /@ pointer to user buffer @/
*           int         freeArg /@ user provided argument to free routine @/
*           )
* .CE
*
* RETURNS:
* The mbuf pointer associated with the inserted mbuf cluster,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufInsertBuf
    (
    MBUF_ID		mbufId,		/* mbuf	ID which buffer is inserted */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset,		/* relative byte offset */
    caddr_t		buf,		/* user buffer for mbuf cluster */
    int			len,		/* number of bytes to insert */
    VOIDFUNCPTR		freeRtn,	/* user free routine */
    int			freeArg		/* argument to free routine */
    )
    {
    MBUF_ID		mbufIdNew;		/* mbuf ID containing <buf> */
    MBUF_DESC 		mbufDesc;		/* desc for <buf> cluster */
    MBUF_SEG		mbufNew;		/* mbuf for <buf> cluster */
    CL_BLK_ID		pClBlk;			/* pointer to cluster blk */
    int			lockKey;		/* int lock cookie */
    int			ix;			/* counter for list init */

    if (len <= 0)				/* have to insert some bytes */
        {
	errno = S_mbufLib_LENGTH_INVALID;
	return (NULL);
	}

    MBUF_ID_CREATE (mbufIdNew);			/* create new mbuf ID for buf */
    if (mbufIdNew == NULL)
	return (NULL);

    lockKey = intLock ();

    if ((mbufDesc = mbufDescHead) != NULL)	/* free list empty ? */
        {
        mbufDescHead = mbufDesc->mbufDescNext;	/* pop first desc off list */
        intUnlock (lockKey);
	}
    else					/* list is empty */
	{
        intUnlock (lockKey);

	if ((mbufDesc = (MBUF_DESC) KHEAP_ALLOC((sizeof (struct mbufDesc) *
	    MBUF_DESC_INC))) != NULL)		/* alloc more desc's */
            {
            for (ix = 0; ix < (MBUF_DESC_INC - 1); ix++)
                mbufDesc[ix].mbufDescNext = &mbufDesc[ix + 1];

            lockKey = intLock ();
            mbufDesc[ix].mbufDescNext = mbufDescHead;
            mbufDescHead = mbufDesc->mbufDescNext;/* hook head onto new list */
            intUnlock (lockKey);
	    }
        }

    if (mbufDesc == NULL)			/* able to get a new desc ? */
	{
        MBUF_ID_DELETE_EMPTY(mbufIdNew);
	return (NULL);
	}

    mbufDesc->buf = buf;

    /* get mbuf for cluster */
    
    if ( (mbufNew = mBlkGet (_pNetDpool, M_WAIT, MT_DATA)) == NULL)
        {
	/* release on fail */

	lockKey = intLock ();
	mbufDescHead = mbufDesc;
	intUnlock (lockKey);

	MBUF_ID_DELETE_EMPTY (mbufIdNew);		
	return (NULL);
	}

    pClBlk = clBlkGet (_pNetDpool, M_WAIT); 
    
    if (pClBlk == NULL)			/* out of cl Blks */
        {
        m_free (mbufNew);

	lockKey = intLock ();
	mbufDescHead = mbufDesc;
	intUnlock (lockKey);

	MBUF_ID_DELETE_EMPTY (mbufIdNew);		
	return (NULL);
        }

    mbufNew->pClBlk		= pClBlk;

    /* build <buf> into an mbuf cluster */
    mbufNew->m_data		= buf;
    mbufNew->m_len		= len;
    mbufNew->m_flags		|= M_EXT;
    mbufNew->m_extBuf		= buf;
    mbufNew->m_extSize		= len;
    mbufNew->m_extFreeRtn	= (FUNCPTR) _mbufBufFree;
    mbufNew->m_extRefCnt	= 1;
    mbufNew->m_extArg1		= (int) mbufDesc;
    mbufNew->m_extArg2		= (int) freeRtn;
    mbufNew->m_extArg3		= freeArg;

    mbufIdNew->mbufHead = mbufNew;		/* put cluster into new ID */

    /* insert the new mbuf ID with <buf> into <mbufId> */

    if ((mbufSeg = _mbufInsert (mbufId, mbufSeg, offset, mbufIdNew)) == NULL)
	{
        mbufNew->m_extArg2 = (int)NULL;		/* don't call freeRtn on fail */
        MBUF_ID_DELETE(mbufIdNew);
	}

    return (mbufSeg);				/* return inserted mbuf */
    }

/*******************************************************************************
*
* _mbufBufFree - free a user cluster buffer
*
* This routine is called when a cluster buffer is deleted, and no other
* mbuf clusters share the buffer space.  This routine calls the user free
* routine connected in _mbufInsertBuf(), then pushes the buffer decsriptor
* back onto the desc free list.
*
* RETURNS:
* N/A
*
* NOMANUAL
*/

LOCAL void _mbufBufFree
    (
    MBUF_DESC		mbufDesc,	/* desc of mbuf to free */
    VOIDFUNCPTR		freeRtn,	/* user free routine */
    int			freeArg		/* argument to free routine */
    )
    {
    int			lockKey;		/* int lock cookie */

    if (freeRtn != NULL)			/* call free rtn if present */
        (*freeRtn) (mbufDesc->buf, freeArg);

    lockKey = intLock ();

    mbufDesc->mbufDescNext = mbufDescHead;	/* push desc onto free list */
    mbufDescHead = mbufDesc;

    intUnlock (lockKey);
    }

/*******************************************************************************
*
* _mbufInsertCopy - copy buffer data into an mbuf chain
*
* This routine copies <len> bytes of data from the user buffer <buf> and
* inserts it at the specified byte location in <mbufId>.  The user buffer
* is in no way tied to the mbuf chain after this operation; a separate copy
* of the data is made.
*
* The location of insertion is specified by <mbufSeg> and <offset>.  Note
* that insertion within a chain occurs before the byte location specified
* by <mbufSeg> and <offset>.  Additionally, note that <mbufSeg> and <offset>
* must be "NULL" and "0", respectively, when inserting into an empty mbuf ID.
*
* RETURNS:
* The mbuf pointer associated with the first inserted mbuf,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufInsertCopy
    (
    MBUF_ID		mbufId,		/* mbuf	ID into which data is copied */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset,		/* relative byte offset */
    caddr_t		buf,		/* buffer from which data is copied  */
    int			len		/* number of byte to copy */
    )
    {
    MBUF_ID		mbufIdNew;		/* dummy ID for dup operation */
    MBUF_SEG		mbufNew;		/* mbuf for copy */
    MBUF_SEG *		pMbufPrev;		/* mbuf previous to mbufNew */
    int			length;			/* length of each copy */

    if (len <= 0)				/* have to copy some bytes */
        {
	errno = S_mbufLib_LENGTH_INVALID;
	return (NULL);
	}

    MBUF_ID_CREATE (mbufIdNew);			/* create new ID for copy */
    if (mbufIdNew == NULL)
	return (NULL);

    pMbufPrev = &mbufIdNew->mbufHead;		/* init prev ptr to head */

    while (len)					/* while more to copy */
	{
	/* obtain a new mbuf with a data buffer pointed */

	if ( (mbufNew = mBufClGet (M_WAIT, MT_DATA, len, FALSE)) == NULL)
	    {
	    /* release on fail */

	    MBUF_ID_DELETE(mbufIdNew);		
	    return (NULL);
	    }

	length = min (len, mbufNew->m_extSize); /* num for copy */

        bcopy (buf, mtod (mbufNew, char *), length);

        buf += length;				/* bump to new buf position */
	len -= length;

	mbufNew->m_len = length;		/* set len to num copied */
        *pMbufPrev = mbufNew;                   /* hook prev into new */
	pMbufPrev = &mbufNew->m_next;           /* update prev mbuf ptr */
	}

    /* insert the new mbuf ID with copied data into <mbufId> */

    if ((mbufSeg = _mbufInsert (mbufId, mbufSeg, offset, mbufIdNew)) == NULL)
        {
	/* release on fail */

	MBUF_ID_DELETE(mbufIdNew);		
	return (NULL);
	}

    return (mbufSeg);				/* return inserted mbuf */
    }

/*******************************************************************************
*
* _mbufExtractCopy - copy data from an mbuf chain to a buffer
*
* This routine copies <len> bytes of data from <mbufId> to the user
* buffer <buf>.
*
* The starting location of the copy is specified by <mbufSeg> and <offset>.
*
* The number of bytes to be copied is given by <len>.  If this parameter
* is negative, or is larger than the number of bytes in the chain after the
* specified byte location, the rest of the chain will be copied. 
* The bytes to be copied may span more than one mbuf.
*
* RETURNS:
* The number of bytes copied from the mbuf chain to the buffer,
* or ERROR if the operation failed.
*
* NOMANUAL
*/

int _mbufExtractCopy
    (
    MBUF_ID		mbufId,		/* mbuf	ID from which data is copied */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset,		/* relative byte offset */
    caddr_t		buf,		/* buffer into which data is copied */
    int			len		/* number of bytes to copy */
    )
    {
    caddr_t		buf0 = buf;		/* save starting position */
    int			length;			/* length of each copy */

    /* find the starting location for copying */

    if ((mbufSeg = _mbufSegFind (mbufId, mbufSeg, &offset)) == NULL)
	return (ERROR);

    if (len < 0)				/* negative = rest of chain */
	len = INT_MAX;

    while (len && (mbufSeg != NULL))		/* while more to copy */
	{
        length = min (len, (mbufSeg->m_len - offset));	/* num for copy */

        bcopy (mtod (mbufSeg, char *) + offset, buf, length);

        buf += length;				/* bump to new buf position */
        len -= length;
	mbufSeg = mbufSeg->m_next;		/* bump to next mbuf in chain */
	offset = 0;				/* no more offset */
	}

    return ((int) buf - (int) buf0);		/* return num bytes copied */
    }

/*******************************************************************************
*
* _mbufCut - cut bytes from an mbuf chain
*
* This routine deletes <len> bytes from <mbufId> starting at the specified
* byte location.
*
* The starting location of deletion is specified by <mbufSeg> and <offset>.
*
* The number of bytes to be cut is given by <len>.  If this parameter
* is negative, or is larger than the number of bytes in the chain after the
* specified byte location, the rest of the chain will be deleted.
* The bytes to be deleted may span more than one mbuf.  If all the bytes
* in any one mbuf are deleted, then the mbuf will be returned to the
* system.  No mbuf may have zero bytes left in it.  
*
* Deleting bytes out of the middle of an mbuf will cause the mbuf to
* be split into two mbufs.  The first mbuf will contain the portion
* of the mbuf before the deleted bytes, while the other mbuf will
* contain the end portion that remains after <len> bytes are deleted.
*
* This routine returns the mbuf pointer associated with the mbuf just
* after the deleted bytes.  In the case where bytes are cut off the end
* of an mbuf chain, a value of MBUF_NONE is returned.
*
* RETURNS:
* The mbuf pointer associated with the mbuf following the deleted bytes,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufCut
    (
    MBUF_ID		mbufId,		/* mbuf	ID from which bytes are cut */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset,		/* relative byte offset */
    int			len		/* number of bytes to cut */
    )
    {
    MBUF_ID		mbufIdNew;	/* dummy ID for dup operation */
    MBUF_SEG *		pMbufPrev;	/* mbuf prev deleted mbuf */
    int			length;		/* length of each cut */

    /* find the mbuf ptr previous to the cut */

    if ((pMbufPrev = _mbufSegFindPrev (mbufId, mbufSeg, &offset)) == NULL)
        return (NULL);

    if ((mbufSeg = *pMbufPrev) == NULL)		/* find cut mbuf */
	{
	errno = S_mbufLib_SEGMENT_NOT_FOUND;
	return (NULL);
	}

    if (len < 0)				/* negative = rest of chain */
	len = INT_MAX;

    while (len && (mbufSeg != NULL))		/* while more to cut */
	{
        length = min (len, (mbufSeg->m_len - offset)); /* num for cut */
        len -= length;

        if (offset != 0)			/* if !cutting off front... */
	    {
	    if (mbufSeg->m_len != (offset + length))/* cut from middle*/
		{
	        /* duplicate portion remaining after bytes to be cut */

                if ((mbufIdNew = _mbufDup (mbufId, mbufSeg, offset + length,
		    mbufSeg->m_len - offset - length)) == NULL)
	            return (NULL);
		
                mbufIdNew->mbufHead->m_next = mbufSeg->m_next;
                mbufSeg->m_next = mbufIdNew->mbufHead;/* hook in saved data */
                mbufSeg->m_len = offset;	/* shorten to later portion */
                MBUF_ID_DELETE_EMPTY(mbufIdNew);/* delete dup ID */
		return (mbufSeg->m_next);	/* return next real mbuf */
		}
            else				/* cut to end */
                {
                mbufSeg->m_len -= length;	/* decrease by len deleted */
		pMbufPrev = &mbufSeg->m_next;	/* update previous */
		mbufSeg = mbufSeg->m_next;	/* bump current mbuf to next */
		}

	    offset = 0;				/* no more offset */
	    }
        else					/* cutting off front... */
            {
	    if (length == mbufSeg->m_len)	/* cutting whole mbuf ? */
		{
		mbufSeg = m_free (mbufSeg);	/* free and get next mbuf */
		*pMbufPrev = mbufSeg;		/* hook prev to next mbuf */
		}
            else				/* cut off front portion */
		{
		mbufSeg->m_data += length;	/* bump up offset */
	        mbufSeg->m_len -= length;	/* taken from front */
		}
            }
        }

    if (mbufSeg == NULL)			/* special case - cut off end */
	return (MBUF_NONE);

    return (mbufSeg);				/* return next real mbuf */
    }

/*******************************************************************************
*
* _mbufSplit - split an mbuf chain into two separate mbuf chains
*
* This routine splits <mbufId> into two separate chains at the specified
* byte location.  The first portion remains in <mbufId>, while the end
* portion is returned in a newly created mbuf ID.
*
* The location of the split is specified by <mbufSeg> and <offset>.
*
* RETURNS:
* The mbuf ID of a newly created chain containing the end portion of <mbufId>,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_ID _mbufSplit
    (
    MBUF_ID		mbufId,		/* mbuf	ID to split into two */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset		/* relative byte offset */
    )
    {
    MBUF_ID		mbufIdNew;		/* mbuf ID of later portion */
    MBUF_SEG *		pMbufPrev;		/* mbuf prev to insert */

    /* find the mbuf ptr previous to the split */

    if ((pMbufPrev = _mbufSegFindPrev (mbufId, mbufSeg, &offset)) == NULL)
        return (NULL);

    if (offset == 0)				/* in middle of mbuf */
	{
        MBUF_ID_CREATE (mbufIdNew);		/* create ID for end portion */
        if (mbufIdNew == NULL)
	    return (NULL);

        mbufIdNew->mbufHead = *pMbufPrev;	/* hook in new head */
        *pMbufPrev = NULL;			/* tie off first portion */
	}
    else					/* split on mbuf boundary */
	{
        mbufSeg = *pMbufPrev;			/* find split mbuf */

        if ((mbufIdNew = _mbufDup (mbufId, mbufSeg, offset,
	    mbufSeg->m_len - offset)) == NULL)
	    return (NULL);			/* dup later portion */

        mbufIdNew->mbufHead->m_next = mbufSeg->m_next;
        mbufSeg->m_len = offset;
        mbufSeg->m_next = NULL;			/* tie off first portion */
	}

    return (mbufIdNew);				/* return ID for end */
    }

/*******************************************************************************
*
* _mbufDup - duplicate an mbuf chain
*
* This routine duplicates <len> bytes of <mbufId> starting at the specified
* byte location, and returns the mbuf ID of the newly created duplicate mbuf.
*
* The starting location of duplication is specified by <mbufSeg> and <offset>.
* The number of bytes to be duplicated is given by <len>.  If this
* parameter is negative, or is larger than the than the number of bytes
* in the chain after the specified byte location, the rest of the chain will
* be duplicated.
*
* Duplication of mbuf data only involves copying of the data when the mbuf
* is not a cluster.  If the mbuf to be duplicated is a cluster, the mbuf
* pointer information is duplicated, while the data is not.  This implies
* that that the mbuf data is shared among all clusters associated
* with a particular cluster data buffer.
*
* RETURNS:
* The mbuf ID of a newly created dupicate mbuf chain,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_ID _mbufDup
    (
    MBUF_ID		mbufId,		/* mbuf ID to duplicate */
    MBUF_SEG		mbufSeg,	/* mbuf base for <offset> */
    int			offset,		/* relative byte offset */
    int			len		/* number of bytes to duplicate */
    )
    {
    MBUF_ID		mbufIdNew;	/* mbuf ID of duplicate */
    MBUF_SEG		mbufNew;	/* mbuf for duplicate */
    MBUF_SEG *		pMbufPrev;	/* mbuf prev to mbufNew */

    /* find the starting location for duplicate */

    if ((mbufSeg = _mbufSegFind (mbufId, mbufSeg, &offset)) == NULL)
        return (NULL);

    if (len < 0)				/* negative = rest of chain */
	len = INT_MAX;

    MBUF_ID_CREATE (mbufIdNew);			/* get ID for duplicate */
    if (mbufIdNew == NULL)
        return (NULL);

    pMbufPrev = &mbufIdNew->mbufHead;		/* init prev ptr to head */

    while (len && (mbufSeg != NULL))		/* while more to duplicate */
	{
	/* get mbuf for duplicate */
        if ( (mbufNew = mBlkGet (_pNetDpool, M_WAIT, mbufSeg->m_type)) == NULL)
	    {
	    /* release on fail */
	    
	    MBUF_ID_DELETE(mbufIdNew);		
	    return (NULL);
	    }

        mbufNew->m_len = min (len, (mbufSeg->m_len - offset));
        len -= mbufNew->m_len;			/* num to duplicate */

	/* copy the cluster header mbuf info to duplicate */
	mbufNew->m_data	= mtod (mbufSeg, char *)  + offset;
	mbufNew->m_flags	= mbufSeg->m_flags;
	mbufNew->m_ext	= mbufSeg->m_ext;

	/* bump share count */

	{
	int s = intLock ();
	++(mbufNew->m_extRefCnt);
	intUnlock (s);
	}
	    
	*pMbufPrev = mbufNew;			/* hook prev into duplicate */
        pMbufPrev = &mbufNew->m_next;		/* update prev mbuf ptr */
	mbufSeg = mbufSeg->m_next;		/* bump original chain */
	offset = 0;				/* no more offset */
	}

    return (mbufIdNew);				/* return ID of duplicate */
    }

/*******************************************************************************
*
* _mbufLength - determine the legnth in bytes of an mbuf chain
*
* This routine returns the number of bytes in the mbuf chain <mbufId>.
*
* RETURNS:
* The number of bytes in the mbuf chain,
* or ERROR if the operation failed
*
* NOMANUAL
*/

int _mbufLength
    (
    MBUF_ID		mbufId		/* mbuf	ID to find length of */
    )
    {
    MBUF_SEG		mbuf;
    int			length = 0;		/* total length */

    if (mbufId == NULL || 
	mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (ERROR);
	}

    for (mbuf = mbufId->mbufHead; mbuf != NULL; mbuf = mbuf->m_next)
        length += mbuf->m_len;

    return (length);				/* return total length */
    }

#if	FALSE
/*******************************************************************************
*
* _mbufSegJoin - coalesce two adjacent mbuf cluster fragments.
*
* This routine combines two or more contiguous mbufs into a single
* mbuf.  Such an operation is only feasible for joining mbufs
* that have the same freeRtn and freeArg, and that follow eachother in
* the chain.  This could be useful for coalescing mbufs fragmented
* by split operations.
* 
* Not in service yet -> not clear that it is a useful routine.
*
* NOMANUAL
*/

STATUS _mbufSegJoin
    (
    MBUF_ID		mbufId,		/* mbuf ID containing mbufs to join */
    MBUF_SEG		mbufSeg		/* first mbuf to join */
    )
    {
    MBUF_SEG		mbufNext;
    STATUS		status = ERROR;

    if (mbufId == NULL
	|| mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (ERROR);
	}

    if (mbufId->mbufHead == NULL)		/* is this chain empty ? */
        {	
	errno = S_mbufLib_ID_EMPTY;
	return (ERROR);
	}

    if (mbufSeg == NULL)			/* use head if mbuf NULL */
	mbufSeg = mbufId->mbufHead;

    while ((mbufNext = mbufSeg->m_next) != NULL)	/* join */
	{
        if (!(M_HASCL(mbufSeg)) || !(M_HASCL(mbufNext)))
	    break;				/* must be clusters */

	if ((mtod (mbufSeg, int) + mbufSeg->m_len) != (mtod (mbufNext, int)))
	    break;				/* data must be adjacent */

        /* check that mbuf params are the exact same */

        if ((mbufSeg->m_type		!= mbufNext->m_type) ||
           (mbufSeg->m_flags		!= mbufNext->m_flags) ||
           (mbufSeg->m_extBuf		!= mbufNext->m_extBuf) ||
           (mbufSeg->m_extRefCnt	!= mbufNext->m_extRefCnt) ||
           (mbufSeg->m_extFreeRtn	!= mbufNext->m_extFreeRtn) ||
           (mbufSeg->m_extArg1		!= mbufNext->m_extArg1) ||
           (mbufSeg->m_extArg2		!= mbufNext->m_extArg2) ||
           (mbufSeg->m_extArg3		!= mbufNext->m_extArg3) ||
           (mbufSeg->m_extSize		!= mbufNext->m_extSize))
	    break;

        mbufSeg->m_len += mbufNext->m_len;	/* join */
        mbufSeg->m_next = m_free (mbufNext);	/* bump */
        status = OK;
	}

    return (status);
    }
#endif	/* FALSE */

/*******************************************************************************
*
* _mbufSegFindPrev - find the mbuf prev to a specified byte location
*
* This routine finds the mbuf in <mbufId> that is previous to the byte
* location specified by <mbufSeg> and <pOffset>.  Once found, a pointer
* to the m_next pointer of that mbuf is returned by this routine,
* and the offset to the specified location is returned in <pOffset>. 
*
* This routine is similar to _mbufSegFind(), except that a pointer to the
* previous mbuf's m_next is returned by this routine, instead of the mbuf
* itself.  Additionally, the end boundary case differs.  This routine will
* return a valid pointer and offset when the specified byte is the byte just
* past the end of the chain.  Return values for this "imaginary" byte
* will also be returned when an offset of MBUF_END is passed in.
*
* RETURNS:
* A pointer to the m_next pointer of the mbuf previous to the mbuf associated
* with the mbuf containing the specified byte, or NULL if the operation failed.
*
* NOMANUAL
*/

LOCAL MBUF_SEG * _mbufSegFindPrev
    (
    MBUF_ID		mbufId,		/* mbuf	ID to examine */
    MBUF_SEG		mbufSeg,	/* mbuf base for <pOffset> */
    int * 		pOffset		/* relative byte offset */
    )
    {
    MBUF_SEG *		pMbufPrev;		/* prev ptr for return */
    MBUF_SEG		mbuf;			/* mbuf in chain */
    int			offset;			/* offset in bytes */
    int 		length;			/* length counter */

    if (mbufId == NULL
	|| mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (NULL);
	}

    pMbufPrev = &mbufId->mbufHead;		/* init prev ptr to head */

    if ((mbuf = mbufId->mbufHead) == NULL)	/* is this chain empty ? */
	{
	if ((mbufSeg == NULL) && (*pOffset == 0))/* empty mbuf ID ? */
	    return (pMbufPrev);			/* OK, if explicit */
        else					/* else error condition */
	    {
	    errno = S_mbufLib_ID_EMPTY;
	    return (NULL);
	    }
	}

    if ((offset = *pOffset) == MBUF_BEGIN)	/* shortcut to head of chain */
	{
	*pOffset = 0;
	return (pMbufPrev);
	}
    else if (offset == MBUF_END)		/* shortcut to end of chain */
	{
        if (mbufSeg != NULL)
	    mbuf = mbufSeg;			/* set base as <mbufSeg> */

        for (; mbuf->m_next != NULL; mbuf = mbuf->m_next)
	    ;					/* find last mbuf in chain */

        *pOffset = 0;				/* imaginary byte past mbuf */
        return (&mbuf->m_next);
	}
    else if (offset < 0)			/* counting backwards ? */
	{
        if ((mbufSeg == NULL) || (mbufSeg == mbufId->mbufHead))
	    {
	    errno = S_mbufLib_OFFSET_INVALID;
	    return (NULL);			/* offset before head */
	    }

        for (length = 0; (mbuf->m_next != NULL) && (mbuf->m_next != mbufSeg);
	    mbuf = mbuf->m_next)
	    {
	    length += mbuf->m_len;		/* find length up to base */
	    pMbufPrev = &mbuf->m_next;
	    }

        if (mbuf->m_next == NULL)
	    {
	    errno = S_mbufLib_SEGMENT_NOT_FOUND;
	    return (NULL);			/* couldn't find base mbufSeg */
	    }

        if (-(offset) < mbuf->m_len)		/* within one mbuf */
	    {
	    *pOffset += mbuf->m_len;
	    return (pMbufPrev);
	    }

	if ((offset += length + mbuf->m_len) < 0)/* adjust to positive */
	    {
	    errno = S_mbufLib_OFFSET_INVALID;
	    return (NULL);
	    }

        mbuf = mbufId->mbufHead;
        pMbufPrev = &mbufId->mbufHead;		/* init prev ptr to head */
	}
    else if ((mbufSeg != NULL) && (mbufSeg != mbuf))/* new base, init head */
        {
	if (offset < mbufSeg->m_len)		/* dest within this base ? */
	    {
	    if ((mbuf = _mbufSegPrev (mbufId, mbufSeg)) != NULL)
	        return (&mbuf->m_next);		/* located previous mbuf */
            else
	        return (NULL);
	    }

	mbuf = mbufSeg;				/* set base as <mbufSeg> */
	}

    for (; (mbuf != NULL) && (offset >= mbuf->m_len); mbuf = mbuf->m_next)
	{
        offset -= mbuf->m_len;			/* find right mbuf in chain */
	pMbufPrev = &mbuf->m_next;
	}
 
    if ((mbuf == NULL) && (offset != 0))
	{
	errno = S_mbufLib_OFFSET_INVALID;
	return (NULL);				/* too large offset */
	}

    *pOffset = offset;				/* return new offset */
    return (pMbufPrev);				/* return found mbuf */
    }

/*******************************************************************************
*
* _mbufSegFind - find the mbuf containing a specified byte location
*
* This routine finds the mbuf in <mbufId> that contains the byte location
* specified by <mbufSeg> and <pOffset>.  Once found, the mbuf is returned
* by this routine, and the offset to the specified location is returned
* in <pOffset>. 
*
* <mbufSeg> determines the base from which <pOffset> is counted.
* If <mbufSeg> is NULL, then <pOffset> starts from the beginning of <mbufId>.
* If <mbufSeg> is not NULL, then <pOffset> starts relative to that
* mbuf.  Byte locations after <mbufSeg> would be specified with a
* positive offset, whereas locations previous to <mbufSeg> would be
* accessed with a negative offset.
*
* <pOffset> is a pointer to a byte offset into the mbuf chain, with offset 0
* being the first byte of data in the mbuf.  The offset does not reset
* for each mbuf in the chain when counting through <mbufId>.  For example,
* if offset 100 is the last byte in a particular mbuf, offset 101 will be
* the first byte in the next mbuf.  Also, the offset may be a negative value.
* An offset of -1 locates the last byte in the previous mbuf.  If offset -55
* is the first byte in a mbuf, an offset -56 would be the last byte in the
* preceding mbuf.
*
* The last byte in the mbuf chain may be specified by passing in an offset of
* MBUF_END, which would cause this routine to return the last mbuf
* in the chain, and return an offset to the last byte in that mbuf.
* Likewise, an offset of MBUF_BEGIN may be passed in to specify the
* first byte in the chain, causing the routine to return the first mbuf
* in the chain, and return an offset of 0.
*
* If the <mbufSeg>, <pOffset> pair specify a byte location past the end
* of the chain, or before the first byte in the chain, NULL is returned by
* this routine.
*
* RETURNS:
* An mbuf pointer associated with the mbuf containing the specified byte,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufSegFind
    (
    MBUF_ID		mbufId,		/* mbuf	ID to examine */
    MBUF_SEG		mbufSeg,	/* mbuf base for <pOffset> */
    int * 		pOffset		/* relative byte offset */
    )
    {
    MBUF_SEG		mbuf;			/* mbuf in chain */
    int			offset;			/* offset in bytes */
    int 		length;			/* length counter */

    if (mbufId == NULL
	|| mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (NULL);
	}

    if ((mbuf = mbufId->mbufHead) == NULL)	/* is this chain empty ? */
	{
	errno = S_mbufLib_ID_EMPTY;
	return (NULL);
	}

    if ((offset = *pOffset) == MBUF_BEGIN)	/* shortcut to head of chain */
	{
	*pOffset = 0;
	return (mbuf);
	}
    else if (offset == MBUF_END)		/* shortcut to end of chain */
	{
        if (mbufSeg != NULL)
	    mbuf = mbufSeg;			/* set base as <mbufSeg> */

        for (; mbuf->m_next != NULL; mbuf = mbuf->m_next)
	    ;					/* find last mbuf in chain */

        *pOffset = mbuf->m_len - 1;		/* stuff new offset */
        return (mbuf);
	}
    else if (offset < 0)			/* counting backwards ? */
	{
        if ((mbufSeg == NULL) || (mbufSeg == mbufId->mbufHead))
	    {
	    errno = S_mbufLib_OFFSET_INVALID;
	    return (NULL);			/* offset before head */
	    }

        for (length = 0; (mbuf->m_next != NULL) && (mbuf->m_next != mbufSeg);
	    mbuf = mbuf->m_next)
	    length += mbuf->m_len;		/* find length up to base */

        if (mbuf->m_next == NULL)
	    {
	    errno = S_mbufLib_SEGMENT_NOT_FOUND;
	    return (NULL);			/* couldn't find base mbufSeg */
	    }

        if (-(offset) < mbuf->m_len)		/* within one mbuf */
	    {
	    *pOffset += mbuf->m_len;
	    return (mbuf);
	    }

	if ((offset += length + mbuf->m_len) < 0)/* adjust to positive */
	    {
	    errno = S_mbufLib_OFFSET_INVALID;
	    return (NULL);
	    }

        mbuf = mbufId->mbufHead;
	}
    else if (mbufSeg != NULL)
	mbuf = mbufSeg;				/* set base as <mbufSeg> */

    for (; (mbuf != NULL) && (offset >= mbuf->m_len); mbuf = mbuf->m_next)
        offset -= mbuf->m_len;			/* find right mbuf in chain */
 
    if (mbuf == NULL)
        errno = S_mbufLib_OFFSET_INVALID;
    else
        *pOffset = offset;			/* return new offset */

    return (mbuf);				/* return found mbuf */
    }

/*******************************************************************************
*
* _mbufSegNext - get the next mbuf in an mbuf chain
*
* This routine finds the mbuf in <mbufId> that is just after the
* mbuf <mbufSeg>.  If <mbufSeg> is NULL, the mbuf after the first
* mbuf in <mbufId> is returned.	 If <mbufSeg> is the last mbuf in
* <mbufId>, NULL is returned
*
* RETURNS:
* An mbuf pointer associated with the mbuf after <mbufSeg>, 
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufSegNext
    (
    MBUF_ID		mbufId,		/* mbuf ID to examine */
    MBUF_SEG		mbufSeg		/* mbuf to get next mbuf */
    )
    {
    if (mbufId == NULL
	|| mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (NULL);
	}

    if (mbufId->mbufHead == NULL)		/* is this chain empty ? */
	{
	errno = S_mbufLib_ID_EMPTY;
	return (NULL);
	}

    /* get mbuf after <mbufSeg> (or start of chain if <mbufSeg> is NULL) */

    if (mbufSeg == NULL)
        return (mbufId->mbufHead->m_next);
    else
        return (mbufSeg->m_next);
    }

/*******************************************************************************
*
* _mbufSegPrev - get the previous mbuf in an mbuf chain
*
* This routine finds the mbuf in <mbufId> that is just previous
* to the mbuf <mbufSeg>.  If <mbufSeg> is NULL, or is the first
* mbuf in <mbufId>, NULL will be returned.
*
* RETURNS:
* An mbuf pointer associated with the mbuf previous to <mbufSeg>,
* or NULL if the operation failed.
*
* NOMANUAL
*/

MBUF_SEG _mbufSegPrev
    (
    MBUF_ID		mbufId,		/* mbuf ID to examine */
    MBUF_SEG		mbufSeg		/* mbuf to get previous mbuf */
    )
    {
    MBUF_SEG		mbuf = mbufId->mbufHead;/* mbuf in chain */

    if (mbufId == NULL ||
	mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (NULL);
	}

    if ((mbufSeg == NULL) || (mbufSeg == mbuf))
	return (NULL);				/* no previous to first mbuf */

    /*
     * Find <mbufSeg> and return a pointer to the previous mbuf.
     * Note: OK if <mbufSeg> is not found... the return will be NULL anyway.
     */

    for (; (mbuf != NULL) && (mbuf->m_next != mbufSeg); mbuf = mbuf->m_next)
	;

    if (mbuf == NULL)				/* could not find mbufSeg */
	errno = S_mbufLib_SEGMENT_NOT_FOUND;

    return (mbuf);
    }

/*******************************************************************************
*
* _mbufSegData - determine the location of data in an mbuf
*
* This routine returns the location of the first byte of data in the mbuf
* <mbufSeg>.  If <mbufSeg> is NULL, the location of data in the first mbuf
* in <mbufId> is returned.
*
* RETURNS:
* A pointer to the first byte of data in the specified mbuf,
* or NULL if the mbuf chain is empty.
*
* NOMANUAL
*/

caddr_t _mbufSegData
    (
    MBUF_ID		mbufId,		/* mbuf ID to examine  */
    MBUF_SEG		mbufSeg		/* mbuf to get pointer to data */
    )
    {
    if (mbufId == NULL 
	|| mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (NULL);
	}

    if (mbufId->mbufHead == NULL)		/* is this chain empty ? */
	{
	errno = S_mbufLib_ID_EMPTY;
	return (NULL);
	}

    /* get data location <mbufSeg> (or start of chain if <mbufSeg> is NULL) */

    if (mbufSeg == NULL)
        return (mtod (mbufId->mbufHead, caddr_t));
    else
        return (mtod (mbufSeg, caddr_t));
    }

/*******************************************************************************
*
* _mbufSegLength - determine the length of an mbuf
*
* This routine returns the number of bytes in the mbuf <mbufSeg>.   
* If <mbufSeg> is NULL, the length of the first mbuf in <mbufId> is
* returned. 
*
* RETURNS:
* The number of bytes in the specified mbuf, or ERROR if incorrect parameters.
*
* NOMANUAL
*/

int _mbufSegLength
    (
    MBUF_ID		mbufId,		/* mbuf ID to examine  */
    MBUF_SEG		mbufSeg		/* mbuf to determine length of */
    )
    {
    if (mbufId == NULL 
	|| mbufId->type != MBUF_VALID)		/* invalid ID ? */
	{
	errno = S_mbufLib_ID_INVALID;
	return (ERROR);
	}

    if (mbufId->mbufHead == NULL)		/* is this chain empty ? */
	{
	if (mbufSeg == NULL)			/* empty mbuf chain */
	    return (0);

	errno = S_mbufLib_SEGMENT_NOT_FOUND;
	return (ERROR);
        }

    /* get length of <mbufSeg> (or start of chain if <mbufSeg> is NULL) */

    if (mbufSeg == NULL)
	return (mbufId->mbufHead->m_len);
    else
        return (mbufSeg->m_len);
    }
