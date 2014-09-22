/* zbufLib.c - zbuf interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,07may02,kbw  man page edits
01g,15oct01,rae  merge from truestack ver 01g, base 01f (AE / 5_X)
01f,26jan95,jdi  format tweak.
01e,24jan95,rhp  revisions based on jdi feedback
01d,16jan95,rhp  fix SEE ALSO subroutine ref format
01c,15nov94,dzb  doc tweaks.
01b,09nov94,rhp  library man page added, subroutine man pages edited.
01a,08nov94,dzb  written.
*/

/*
DESCRIPTION
This library contains routines to create, build, manipulate, and
delete zbufs.  Zbufs, also known as "zero copy buffers," are a data
abstraction designed to allow software modules to share buffers
without unnecessarily copying data.

To support the data abstraction, the subroutines in this library hide
the implementation details of zbufs.  This also maintains the
library's independence from any particular implementation mechanism,
thus permitting the zbuf interface to be used with other buffering schemes.

Zbufs have three essential properties.  First, a zbuf holds a
sequence of bytes.  Second, these bytes are organized into one or more
segments of contiguous data, although the successive segments
themselves are not usually contiguous.  Third, the data within a
segment may be shared with other segments; that is, the data may be in use
by more than one zbuf at a time.

ZBUF TYPES
The following data types are used in managing zbufs:
.iP ZBUF_ID 16 3
An arbitrary (but unique) integer that identifies a particular zbuf.
.iP ZBUF_SEG
An arbitrary (but unique within a single zbuf) integer that identifies
a segment within a zbuf.
.LP

ADDRESSING BYTES IN ZBUFS
The bytes in a zbuf are addressed by the combination <zbufSeg>,
<offset>.  The <offset> may be positive or negative, and is simply the
number of bytes from the beginning of the segment <zbufSeg>.

A <zbufSeg> can be specified as NULL, to identify the segment at the
beginning of a zbuf.  If <zbufseg> is NULL, <offset> is the
absolute offset to any byte in the zbuf.  However, it is more
efficient to identify a zbuf byte location relative to the <zbufSeg>
that contains it; see zbufSegFind() to convert any <zbufSeg>, <offset>
pair to the most efficient equivalent.

Negative <offset> values always refer to bytes before the
corresponding <zbufSeg>, and are not usually the most efficient
address formulation (though using them may save your
program other work in some cases).

The following special <offset> values, defined as constants,
allow you to specify the very beginning or the very end of an entire zbuf,
regardless of the <zbufSeg> value:
.iP ZBUF_BEGIN 16 3
The beginning of the entire zbuf.
.iP ZBUF_END
The end of the entire zbuf (useful for appending to a zbuf; see below).
.LP

INSERTION AND LIMITS ON OFFSETS
An <offset> is not valid if it points outside the zbuf.  Thus, to
address data currently within an N-byte zbuf, the valid offsets
relative to the first segment are 0 through N-1.

Insertion routines are a special case: they obey the usual
convention, but they use <offset> to specify where the new data
begins after the insertion is complete.  Therefore, the original
zbuf data is always inserted just before the byte
location addressed by the <offset> value.  The value of this
convention is that it permits inserting (or concatenating) data either
before or after the existing data.  To insert before all the data
currently in a zbuf segment, use 0 as <offset>.  To insert after all
the data in an N-byte segment, use N as <offset>. An <offset> of N-1
inserts the data just before the last byte in an N-byte segment.

An <offset> of 0 is always a valid insertion point; for an empty zbuf,
0 is the only valid <offset> (and NULL the only valid <zbufSeg>).

SHARING DATA
The routines in this library avoid copying segment data whenever
possible.  Thus, by passing and manipulating ZBUF_IDs rather than
copying data, multiple programs can communicate with greater
efficiency.  However, each program must be aware of data sharing:
changes to the data in a zbuf segment are visible to all zbuf segments
that reference the data.

To alter your own program's view of zbuf data without affecting other
programs, first use zbufDup() to make a new zbuf; then you can use an
insertion or deletion routine, such as zbufInsertBuf(), to add a
segment that only your program sees (until you pass a zbuf containing
it to another program).  It is safest to do all direct data
manipulation in a private buffer, before enrolling it in a zbuf: in
principle, you should regard all zbuf segment data as shared.

Once a data buffer is enrolled in a zbuf segment, the zbuf library is
responsible for noticing when the buffer is no longer in use by any
program, and freeing it.  To support this, zbufInsertBuf() requires
that you specify a callback to a free routine each time you build a
zbuf segment around an existing buffer.  You can use this callback to notify
your application when a data buffer is no longer in use.

VXWORKS AE PROTECTION DOMAINS
Under VxWorks AE, this feature is restricted to the kernel protection 
domain.  This restriction does not apply under non-AE versions of VxWorks.  

To use this feature, include the following component:
INCLUDE_ZBUF_SOCK

SEE ALSO
zbufSockLib

INTERNAL

Extensions:

zbufJoin (zbufId, zbufSeg) - coalesce two adjacent segment fragments
    This routine combines two or more contiguous zbuf segments into a single
    zbuf segment.  Such an operation is only feasible for joining segments
    that have the same freeRtn and freeArg, and that follow eachother in
    the zbuf.  This could be useful for coalescing zbuf segments fragmented
    by split operations.

zbufOwn (zbufId, zbufSeg, offset, len) - own a local copy of zbuf data
    This routine makes a local copy of a portion of a zbuf.  This
    is useful if you want to change the actual data in a zbuf, but want to
    make sure that you are not corrupting another zbuf that shares the
    same data.  If you are the only zbuf segment using a particular buffer,
    this routine does nothing.  If other zbufs reference the same data, this
    routine inserts a copy of the requested data, then deletes reference
    to the original data.
*/

/* includes */

#include "vxWorks.h"
#include "zbufLib.h"
#include "stdlib.h"
#include "memPartLib.h"

/* locals */

/*
 * N.B.
 * As other back-ends to zbuf are ever developed, and need to be run
 * concurrently with existing back-ends, the zbufFunc table needs to have
 * the back-end resident function table pointer in the ZBUF_ID.  This will
 * require a modification to the current method of simply passing through
 * the ZBUF_ID to the back-end.  Instead, the ZBUF_ID will have to point
 * to a structure, which would contain the back-end function table pointer
 * and a back-end-specific XX_ID pointer.
 */

LOCAL ZBUF_FUNC		zbufFuncNull =	/* null funcs for unconnected zbufs */
    {
    NULL,				/* zbufCreate()		*/
    NULL,				/* zbufDelete()		*/
    NULL,				/* zbufInsert()		*/
    NULL,				/* zbufInsertBuf()	*/
    NULL,				/* zbufInsertCopy()	*/
    NULL,				/* zbufExtractCopy()	*/
    NULL,				/* zbufCut()		*/
    NULL,				/* zbufSplit()		*/
    NULL,				/* zbufDup()		*/
    NULL,				/* zbufLength()		*/
    NULL,				/* zbufSegFind()	*/
    NULL,				/* zbufSegNext()	*/
    NULL,				/* zbufSegPrev()	*/
    NULL,				/* zbufSegData()	*/
    NULL				/* zbufSegLength()	*/
    };

LOCAL ZBUF_FUNC *	pZbufFunc = &zbufFuncNull;

/*******************************************************************************
*
* zbufLibInit - initialize the zbuf interface library
*
* This routine initializes the zbuf interface facility.  It
* must be called before any zbuf routines are used.  This routine is
* typically called during the initialization of a VxWorks facility which
* uses or extends the zbuf interface, although this is not a requirement.
* The zbuf library may be used on its own, provided an appropriate back-end
* buffer mechanism is available, and is specified by <libInitRtn>.
*
* RETURNS:
* OK, or ERROR if the zbuf interface cannot be initialized.
*
* NOMANUAL
*/

STATUS zbufLibInit
    (
    FUNCPTR		libInitRtn	/* back-end buffer mechanism init rtn */
    )
    {
    ZBUF_FUNC *		pZbufFuncTemp;	/* temp return of back-end func table */

    /* call the back-end initialization routine - watch for NULL func ptr */

    if ((libInitRtn == NULL) || ((pZbufFuncTemp =
	(ZBUF_FUNC *) (libInitRtn) ()) == NULL))
	return (ERROR);

    pZbufFunc = pZbufFuncTemp;			/* connect back-end func tbl */
    return (OK);
    }

#if	FALSE		/* Pool routines not in service yet */

/*******************************************************************************
*
* zbufSegPoolCreate - create an empty zbuf segment pool
*
* This routine creates a zbuf segment pool.  It remains empty (that is, does
* not contain any free segments) until segments are added by calling
* zbufSegPoolAdd().  Operations performed on zbuf segments pools require
* a zbuf segment pool ID, which is returned by this routine.
*
* RETURNS:
* A zbuf segment pool ID, or NULL if an ID cannot be created.
*
* SEE ALSO: zbufSegPoolDelete(), zbufSegPoolAdd()
*
* NOMANUAL
*/

ZBUF_POOL_ID zbufSegPoolCreate (void)
    {
    ZBUF_POOL_ID	zbufPoolId;		/* new ID */
    
    if ((zbufPoolId = (ZBUF_POOL_ID) KHEAP_ALLOC(sizeof(struct zbufPoolId))) ==
	NULL)
	return (NULL);				/* malloc pool ID struct */

    /* create mutex sem for guarding access to zbuf segment pool */

    if ((zbufPoolId->poolSem = semMCreate ((SEM_Q_PRIORITY |
	SEM_INVERSION_SAFE | SEM_DELETE_SAFE))) == ERROR)
	{
	KHEAP_FREE(zbufPoolId);			/* free resources */
	return (NULL);
	}

    /* create binary sem for waiting for free segments to come into pool */

    if ((zbufPoolId->waitSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY)) == ERROR)
	{
        semDelete (zbufPoolId->poolSem);	/* free resources */
	KHEAP_FREE(zbufPoolId);
	return (NULL);
	}

    zbufPoolId->use = 0;			/* no loaned buffers yet */
    zbufPoolId->blockHead = NULL;		/* no blocks/segs yet */

    return (zbufPoolId);
    }

/*******************************************************************************
*
* zbufSegPoolDelete - delete a zbuf segment pool and free associated segments
*
* This routine will free any segments associated with <zbufPoolId>,
* then delete the zbuf segment pool ID itself.  To do this, all segments
* must be in the pool (that is, no segments may currently be part of a zbuf).
* <zbufPoolId> must not be used after this routine executes successfully.
*
* Associated segments will be returned to the system memory pool if they
* were allocated with zbufSegPoolAdd().  No action will be taken upon
* user segments added to the segment pool by specifying the segment block
* start address. These segments should be considered free after this
* routine executes successfully.
* 
* RETURNS:
* OK, or ERROR if the zbuf segment pool has outstanding segments.
*
* SEE ALSO: zbufSegPoolCreate(), zbufSegPoolAdd()
*
* NOMANUAL
*/

STATUS zbufSegPoolDelete
    (
    ZBUF_POOL_ID	zbufPoolId	/* zbuf segment pool to delete */
    )
    {
    ZBUF_BLOCK_ID	zbufBlock;		/* ptr to block headers */
    ZBUF_BLOCK_ID	zbufBlockTemp;		/* temp storage of block hdr */

    /* obtain exclusive access to zbuf segment pool structures */

    if ((semTake (zbufPoolId->poolSem, WAIT_FOREVER)) == ERROR)
	return (ERROR);

    if (zbufPoolId->use != 0)			/* any segments in use ? */
	{
	semGive (zbufPoolId->poolSem);		/* give up access */
	return (ERROR);				/* cannot feee if in use */
	}

    /* go through list of block headers, freein all alloc'ed mem */

    for (zbufBlock = zbufPoolId->blockHead; zbufBlock != NULL;)
	{
	zbufBlockTemp = zbufBlock->blockNext;	
	KHEAP_FREE(zbufBlock);			/* free each hdr & memory */
        zbufBlock = zbufBlockTemp;		/* bump to next block hdr */
        }

    semDelete (zbufPoolId->waitSem);		/* free wait binary sem */
    semDelete (zbufPoolId->poolSem);		/* free pool mutex sem */

    KHEAP_FREE(zbufPoolId);			/* free actual pool ID struct */

    return (OK);
    }

/*******************************************************************************
*
* zbufSegPoolAdd - add a block of zbuf segments to a zbuf segment pool
*
* This routine adds a block of zbuf segments to <zbufPoolId>.  This routine
* may be called multiple times to add segment blocks of different sizes and
* numbers.
*
* The segment size <segSise> will be rounded up to the memory allignment
* requirements of the target architecture.  The minimum size of a
* segment is the size of a void pointer.
*
* If system memory is to be used for the segment allocation, set <blockStart>
* to NULL, in which case <blockLength> parameter will be ignored.
* Otherwise, a block of memory to be used may be specified by <blockStart> and
* <blockLen>.  If a segment block is specified with <blockStart>, the
* <segNum> parameter is ignored.  Instead, the number of alligned segments
* that can be obtained from <memLength> are added to the pool.
* 
* If a zbuf segment pool is deleted with zbufSegPoolDelete(), any allocated
* segment blocks will be returned to the system memory pool.  Specific memory
* blocks obtained through <blockStart> should be considered free after
* zbufSegPoolDelete() executes successfully, but the user will not be
* given any further indication of the memory being released from the pool.
* 
* RETURNS:
* OK, or ERROR if the zbuf segment block cannot be added to the pool.
*
* SEE ALSO: zbufSegPoolDelete()
*
* NOMANUAL
*/

STATUS zbufSegPoolAdd
    (
    ZBUF_POOL_ID	zbufPoolId,	/* pool to add zbuf segments to */
    int			segSize,	/* size of new segments */
    int			segNum,		/* number of new segs (system memory) */
    caddr_t		blockStart,	/* start of seg block (user memory) */
    int			blockLen	/* length of seg block (user memory) */
    )
    {
    ZBUF_BLOCK_ID	zbufBlockNew;		/* new block header */
    ZBUF_BLOCK_ID 	zbufBlock = NULL;	/* location to insert new hdr */
    STATUS		status = ERROR;		/* return value */
    int			ix;			/* counter for block init */

    /* obtain exclusive access to zbuf segment pool structures */

    if ((semTake (zbufPoolId->poolSem, WAIT_FOREVER)) == ERROR)
	return (ERROR);

    /* adjust segSize for mem allign - must be at least void * (see below) */

    segSize = min (MEM_ROUND_UP(segSize), sizeof (void *));

    if (blockStart == NULL)			/* if allocating from system */
	{
	if ((zbufBlockNew = (ZBUF_BLOCK_ID) KHEAP_ALLOC((sizeof(struct zbufBlockId)
	    + (segSize * segNum)))) == NULL)	/* allocate hdr and seg block */
	    goto release;

	blockStart = (caddr_t) (zbufBlockNew + 1);/* set start of seg block */
        }
    else					/* use supplied user memory */
	{
        /* calc the number of segments that can be obtained from user mem */

	if ((segNum = blockLen / segSize) == 0)	/* must be at least one */
	    goto release;

        if ((zbufBlockNew = (ZBUF_BLOCK_ID) KHEAP_ALLOC
	    (sizeof(struct zbufBlockId))) == NULL)/* allocate block hdr only */
	    goto release;
	}

    zbufBlockNew->length = segSize;		/* block is for "segSize" */
    zbufBlockNew->zbufPoolId = zbufPoolId;	/* for _zbufSegPoolFree() */
    zbufBlockNew->segFree = (void *) blockStart;/* first seg in block is free */

    /*
     * Go through block, and divide up into segNum number of segments.
     * The first location of each segment points to the next free segment,
     * with the last segment pointing to NULL.  In this way, a sll of free
     * segments is kept, with the pointer in the segment memory itself.
     */

    for (ix = 1; ix < segNum; ix++, blockStart += segSize)
	*((void **) blockStart) = (void *) (blockStart + segSize);
    *((void **) blockStart) = NULL;

    /* find the proper location in the pool to insert the new seg block */

    if ((zbufPoolId->blockHead != NULL) &&
        (zbufPoolId->blockHead->length < segSize))
	{
        zbufBlock = zbufPoolId->blockHead;
        for (; zbufBlock->blockNext != NULL; zbufBlock = zbufBlock->blockNext)
	    if (zbufBlock->blockNext->length >= segSize)
	        break;
        }

    if (zbufBlock == NULL)			/* insert new block at start */
	{
	zbufBlockNew->blockNext = zbufPoolId->blockHead;/* new to first */
        zbufPoolId->blockHead = zbufBlockNew;	/* head to new */
	}
    else					/* insert in middle or end */
	{
	zbufBlockNew->blockNext = zbufBlock->blockNext;	/* new to next */
	zbufBlock->blockNext = zbufBlockNew;	/* prev to new */
	}

    status = OK;				/* done! */

release:					/* error encountered */

    semGive (zbufPoolId->poolSem);		/* give up access */

    return (status);
    }

/*******************************************************************************
*
* _zbufSegPoolFree - return a zbuf segment to a zbuf segment pool
*
* This routine returns a zbuf segment to a zbuf segment pool.
* To return <buf> to the zbuf segment block that it was originally allocated
* from, place the segment <buf> at the start of the segment block associated
* with <zbufBlockId>.  If <zbufBlockId> is NULL, the segment <buf> was
* allocated from system memory, and should be returned with free().
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void _zbufSegPoolFree 
    (
    caddr_t		buf,		/* zbuf segment to return */
    ZBUF_BLOCK_ID	zbufBlockId	/* seg block to return <buf> to */
    )
    {
    ZBUF_POOL_ID	zbufPoolId;		/* back pointer to pool ID */

    if (zbufBlockId == NULL)			/* if originally malloc'ed */
	KHEAP_FREE(buf);
    else					/* else return to pool */
	{
	zbufPoolId = zbufBlockId->zbufPoolId;	/* obtain back pointer to ID */

	/* obtain exclusive access to zbuf segment pool structures */

        if ((semTake (zbufPoolId->poolSem, WAIT_FOREVER)) == ERROR)
	    return;

        *((void **) buf) = zbufBlockId->segFree;/* hook my next to first */
	zbufBlockId->segFree = (void *) buf;	/* hook head to me */

        zbufPoolId->use--;			/* check it in */

        semFlush (zbufPoolId->waitSem);		/* free those waiting for seg */

        semGive (zbufPoolId->poolSem);		/* give up access */
	}
    }

/*******************************************************************************
*
* zbufInsertPool - obtain a segment and insert into a zbuf
*
* This routine obtains a zbuf segment from <zbufPoolId> and
* inserts it into zbuf <zbufId> at the specified location.  The requested
* segment is not initialized (that is, it is not zeroed out) by this routine.
* 
* This routine searches <zbufPoolId> looking for a suitably sized free
* segment.  The smallest free segment that is at least <len> bytes in length
* is obtained and inserted into <zbufId>.  If no free segments are found,
* this routine will pend for <timeout> ticks waiting for a zbuf segment to be
* released from another zbuf.  As segments are released from other zbufs,
* they are examined to see if they are at least <len> bytes.  If they are
* not suitable, the timeout is reset to <timeout> ticks, and this routine
* will pend again waiting for a segment to be released.
*
* If <zbufPoolId> is NULL, the system memory pool is used to allocate a
* segment of <len> bytes.  Upon release of this segment from the zbuf, it
* will returned to the system memory pool.
*
* The location of insertion is specified by <zbufSeg> and <offset>.  See
* the zbufLib manual page for more information on specifying
* a byte location within a zbuf.  In particular, insertion within
* a zbuf occurs before the byte location specified by <zbufSeg> and <offset>.
* Additionally, <zbufSeg> and <offset> must be NULL and 0,
* respectively, when inserting into an empty zbuf.
*
* RETURNS:
* The zbuf segment ID of the inserted segment,
* or NULL if the operation fails.
*
* NOMANUAL
*/

ZBUF_SEG zbufInsertPool
    (
    ZBUF_POOL_ID	zbufPoolId,	/* pool to obtain zbuf segment from */
    ZBUF_ID		zbufId,		/* zbuf to insert segment into */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    int			len,		/* requested segment length in bytes */
    int			timeout		/* timeout in ticks */
    )
    {
    ZBUF_BLOCK_ID 	zbufBlock = NULL;	/* segment block header */
    caddr_t		buf = NULL;		/* requested segment buf */
    int			lenLast = NULL;		/* last seg block length seen */

    if (zbufPoolId == NULL)			/* alloc from system mem */
	if ((buf = KHEAP_ALLOC(len)) == NULL)
	    return (NULL);

    /* if not allocating from system memory, try to obtain from seg pool */

    while (buf == NULL)	
        {
	/* obtain exclusive access to zbuf segment pool structures */

        if ((semTake (zbufPoolId->poolSem, timeout)) == ERROR)
	    return (NULL);

        /*
         * Search the pool's seg blocks headers for a suitable size (first fit)
         * and that has free segments attached to it.  An over-sized segment
	 * will be taken if no more suitably sized segments are available.
	 * No fragmentation is performed.  A segment remains whole.
	 * If no segments are available (right-fit or oversized), pend on
	 * waitSem for segment to come in from _zbufSegPoolFree().
         */

        zbufBlock = zbufPoolId->blockHead;
        for (; zbufBlock != NULL; zbufBlock = zbufBlock->blockNext)
            if (((lenLast = zbufBlock->length) >= len) &&
		(zbufBlock->segFree != NULL))
                break;

        if (zbufBlock == NULL)
	    {
	    /* going to pend or exit, so give up access */

            semGive (zbufPoolId->poolSem);

	    /* check that there are seg blocks large enough for len */

            if ((lenLast == NULL) || (lenLast < len))
		return (NULL);

            /* wait for segments to come back in... */

	    if ((semTake (zbufPoolId->waitSem, timeout)) == ERROR)
	        return (NULL);
	    }
        else					/* found suitable segment */
	    {
	    buf = (caddr_t) zbufBlock->segFree;	/* segment is first */
	    zbufBlock->segFree = *((void **) buf); /* first is next */
            zbufPoolId->use++;			/* check it out */

            semGive (zbufPoolId->poolSem);	/* give up access */
	    }
        }

    /* insert the new segment into zbufId at the specified location */

    if ((zbufSeg = zbufInsertBuf (zbufId, zbufSeg, offset,
	buf, len, _zbufSegPoolFree, (int) zbufBlock)) == NULL)
	_zbufSegPoolFree (buf, zbufBlock);	/* free seg on failure */

    return (zbufSeg);
    }
#endif		/* FALSE */

/*******************************************************************************
*
* zbufCreate - create an empty zbuf
*
* This routine creates a zbuf, which remains empty (that is, it contains
* no data) until segments are added by the zbuf insertion routines.
* Operations performed on zbufs require a zbuf ID, which is returned by
* this routine.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, the returned ID is valid within 
* the kernel protection domain only.  This restriction does not apply 
* under non-AE versions of VxWorks.  
*
* RETURNS:
* A zbuf ID, or NULL if a zbuf cannot be created.
*
* SEE ALSO: zbufDelete()
*/

ZBUF_ID zbufCreate (void)
    {
    return ((pZbufFunc->createRtn == NULL) ? NULL :
	(ZBUF_ID) (pZbufFunc->createRtn) ());
    }

/*******************************************************************************
*
* zbufDelete - delete a zbuf
*
* This routine deletes any zbuf segments in the specified zbuf, then
* deletes the zbuf ID itself.  <zbufId> must not be used after this routine
* executes successfully.
*
* For any data buffers that were not in use by any other zbuf, zbufDelete()
* calls the associated free routine (callback).
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* OK, or ERROR if the zbuf cannot be deleted.
*
* SEE ALSO: zbufCreate(), zbufInsertBuf()
*/

STATUS zbufDelete
    (
    ZBUF_ID		zbufId		/* zbuf to be deleted */
    )
    {
    return ((pZbufFunc->deleteRtn == NULL) ? ERROR :
	(pZbufFunc->deleteRtn) (zbufId));
    }

/*******************************************************************************
*
* zbufInsert - insert a zbuf into another zbuf
*
* This routine inserts all <zbufId2> zbuf segments into <zbufId1> at the
* specified byte location.
*
* The location of insertion is specified by <zbufSeg> and <offset>.  See
* the zbufLib manual page for more information on specifying
* a byte location within a zbuf.  In particular, insertion within
* a zbuf occurs before the byte location specified by <zbufSeg> and <offset>.
* Additionally, <zbufSeg> and <offset> must be NULL and 0,
* respectively, when inserting into an empty zbuf.
*
* After all the <zbufId2> segments are inserted into <zbufId1>, the zbuf ID
* <zbufId2> is deleted.  <zbufId2> must not be used after this routine
* executes successfully.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ZBUF_SEG is valid within the kernel protection 
* domain only.  This restriction does not apply under non-AE versions of 
* VxWorks.  
*
* RETURNS:
* The zbuf segment ID for the first inserted segment,
* or NULL if the operation fails.
*/

ZBUF_SEG zbufInsert
    (
    ZBUF_ID		zbufId1,	/* zbuf to insert <zbufId2> into */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    ZBUF_ID		zbufId2		/* zbuf to insert into <zbufId1> */
    )
    {
    return ((pZbufFunc->insertRtn == NULL) ? NULL :
        (ZBUF_SEG) (pZbufFunc->insertRtn) (zbufId1, zbufSeg, offset, zbufId2));
    }

/*******************************************************************************
*
* zbufInsertBuf - create a zbuf segment from a buffer and insert into a zbuf
*
* This routine creates a zbuf segment from the application buffer <buf>
* and inserts it at the specified byte location in <zbufId>.
*
* The location of insertion is specified by <zbufSeg> and <offset>.  See
* the zbufLib manual page for more information on specifying
* a byte location within a zbuf.  In particular, insertion within
* a zbuf occurs before the byte location specified by <zbufSeg> and <offset>.
* Additionally, <zbufSeg> and <offset> must be NULL and 0,
* respectively, when inserting into an empty zbuf.
*
* The parameter <freeRtn> specifies a free-routine callback that runs when
* the data buffer <buf> is no longer referenced by any zbuf segments.  If
* <freeRtn> is NULL, the zbuf functions normally, except that the
* application is not notified when no more zbufs segments reference <buf>.
* The free-routine callback runs from the context of the task that last
* deletes reference to the buffer.  Declare the <freeRtn> callback as
* follows (using whatever routine name suits your application):
* .CS
*	void freeCallback
*	    (
*	    caddr_t	buf,	/@ pointer to application buffer @/
*	    int		freeArg	/@ argument to free routine @/
*	    )
* .CE
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf segment ID of the inserted segment,
* or NULL if the operation fails.
*/

ZBUF_SEG zbufInsertBuf
    (
    ZBUF_ID		zbufId,		/* zbuf in which buffer is inserted */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    caddr_t		buf,		/* application buffer for segment */
    int			len,		/* number of bytes to insert */
    VOIDFUNCPTR		freeRtn,	/* free-routine callback */
    int			freeArg		/* argument to free routine */
    )
    {
    return ((pZbufFunc->insertBufRtn == NULL) ? NULL :
	(ZBUF_SEG) (pZbufFunc->insertBufRtn) (zbufId, zbufSeg, offset,
	buf, len, freeRtn, freeArg));
    }

/*******************************************************************************
*
* zbufInsertCopy - copy buffer data into a zbuf
*
* This routine copies <len> bytes of data from the application buffer <buf>
* and inserts it at the specified byte location in <zbufId>.  The
* application buffer is in no way tied to the zbuf after this operation;
* a separate copy of the data is made.
*
* The location of insertion is specified by <zbufSeg> and <offset>.  See
* the zbufLib manual page for more information on specifying
* a byte location within a zbuf.  In particular, insertion within
* a zbuf occurs before the byte location specified by <zbufSeg> and <offset>.
* Additionally, <zbufSeg> and <offset> must be NULL and 0,
* respectively, when inserting into an empty zbuf.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned value is valid in the protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf segment ID of the first inserted segment,
* or NULL if the operation fails.
*/

ZBUF_SEG zbufInsertCopy
    (
    ZBUF_ID		zbufId,		/* zbuf into which data is copied */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    caddr_t		buf,		/* buffer from which data is copied */
    int			len		/* number of bytes to copy */
    )
    {
    return ((pZbufFunc->insertCopyRtn == NULL) ? NULL :
        (ZBUF_SEG) (pZbufFunc->insertCopyRtn) (zbufId, zbufSeg,
	offset, buf, len));
    }

/*******************************************************************************
*
* zbufExtractCopy - copy data from a zbuf to a buffer
*
* This routine copies <len> bytes of data from <zbufId> to the application
* buffer <buf>.
*
* The starting location of the copy is specified by <zbufSeg> and <offset>.
* See the zbufLib manual page for more information on
* specifying a byte location within a zbuf.  In particular, the first
* byte copied is the exact byte specified by <zbufSeg> and <offset>.
*
* The number of bytes to copy is given by <len>.  If this parameter
* is negative, or is larger than the number of bytes in the zbuf after the
* specified byte location, the rest of the zbuf is copied.
* The bytes copied may span more than one segment.
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The number of bytes copied from the zbuf to the buffer,
* or ERROR if the operation fails.
*/

int zbufExtractCopy
    (
    ZBUF_ID		zbufId,		/* zbuf from which data is copied */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    caddr_t		buf,		/* buffer into which data is copied */
    int			len		/* number of bytes to copy */
    )
    {
    return ((pZbufFunc->extractCopyRtn == NULL) ? ERROR :
        (pZbufFunc->extractCopyRtn) (zbufId, zbufSeg, offset, buf, len));
    }

/*******************************************************************************
*
* zbufCut - delete bytes from a zbuf
*
* This routine deletes <len> bytes from <zbufId> starting at the specified
* byte location.
*
* The starting location of deletion is specified by <zbufSeg> and <offset>.
* See the zbufLib manual page for more information on
* specifying a byte location within a zbuf.  In particular, the first
* byte deleted is the exact byte specified by <zbufSeg> and <offset>.
*
* The number of bytes to delete is given by <len>.  If this parameter
* is negative, or is larger than the number of bytes in the zbuf after the
* specified byte location, the rest of the zbuf is deleted.
* The bytes deleted may span more than one segment.
*
* If all the bytes in any one segment are deleted, then the segment is
* deleted, and the data buffer that it referenced will be freed if no
* other zbuf segments reference it.  No segment may survive with zero bytes
* referenced.
* 
* Deleting bytes out of the middle of a segment splits the segment
* into two.  The first segment contains the portion of the data buffer
* before the deleted bytes, while the other segment contains the end
* portion that remains after deleting <len> bytes.
*
* This routine returns the zbuf segment ID of the segment just
* after the deleted bytes.  In the case where bytes are cut off the end
* of a zbuf, a value of ZBUF_NONE is returned.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned value is valid in the protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf segment ID of the segment following the deleted bytes,
* or NULL if the operation fails.
*/

ZBUF_SEG zbufCut
    (
    ZBUF_ID		zbufId,		/* zbuf from which bytes are cut */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    int			len		/* number of bytes to cut */
    )
    {
    return ((pZbufFunc->cutRtn == NULL) ? NULL :
	(ZBUF_SEG) (pZbufFunc->cutRtn) (zbufId, zbufSeg, offset, len));
    }

/*******************************************************************************
*
* zbufSplit - split a zbuf into two separate zbufs
*
* This routine splits <zbufId> into two separate zbufs at the specified
* byte location.  The first portion remains in <zbufId>, while the end
* portion is returned in a newly created zbuf.
*
* The location of the split is specified by <zbufSeg> and <offset>.  See
* the zbufLib manual page for more information on specifying
* a byte location within a zbuf.  In particular, after the split
* operation, the first byte of the returned zbuf is the exact byte
* specified by <zbufSeg> and <offset>.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned value is valid in the protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf ID of a newly created zbuf containing the end portion of <zbufId>,
* or NULL if the operation fails.
*/

ZBUF_ID zbufSplit
    (
    ZBUF_ID		zbufId,		/* zbuf to split into two */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset		/* relative byte offset */
    )
    {
    return ((pZbufFunc->splitRtn == NULL) ? NULL :
	(ZBUF_ID) (pZbufFunc->splitRtn) (zbufId, zbufSeg, offset));
    }

/*******************************************************************************
*
* zbufDup - duplicate a zbuf
*
* This routine duplicates <len> bytes of <zbufId> starting at the specified
* byte location, and returns the zbuf ID of the newly created duplicate zbuf. 
*
* The starting location of duplication is specified by <zbufSeg> and <offset>.
* See the zbufLib manual page for more information on
* specifying a byte location within a zbuf.  In particular, the first
* byte duplicated is the exact byte specified by <zbufSeg> and <offset>.
*
* The number of bytes to duplicate is given by <len>.  If this parameter
* is negative, or is larger than the number of bytes in the zbuf after the
* specified byte location, the rest of the zbuf is duplicated.
* 
* Duplication of zbuf data does not usually involve copying of the data.
* Instead, the zbuf segment pointer information is duplicated, while the data
* is not, which means that the data is shared among all zbuf segments
* that reference the data.  See the zbufLib manual
* page for more information on copying and sharing zbuf data.
*
* RETURNS:
* The zbuf ID of a newly created duplicate zbuf,
* or NULL if the operation fails.
*/

ZBUF_ID zbufDup
    (
    ZBUF_ID		zbufId,		/* zbuf to duplicate */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <offset> */
    int			offset,		/* relative byte offset */
    int			len		/* number of bytes to duplicate */
    )
    {
    return ((pZbufFunc->dupRtn == NULL) ? NULL :
	(ZBUF_ID) (pZbufFunc->dupRtn) (zbufId, zbufSeg, offset, len));
    }

/*******************************************************************************
*
* zbufLength - determine the length in bytes of a zbuf
*
* This routine returns the number of bytes in the zbuf <zbufId>.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The number of bytes in the zbuf,
* or ERROR if the operation fails.
*/

int zbufLength
    (
    ZBUF_ID		zbufId		/* zbuf to determine length */
    )
    {
    return ((pZbufFunc->lengthRtn == NULL) ? ERROR :
	(pZbufFunc->lengthRtn) (zbufId));
    }

/*******************************************************************************
*
* zbufSegFind - find the zbuf segment containing a specified byte location
*
* This routine translates an address within a zbuf to its most local
* formulation.  zbufSegFind() locates the zbuf segment in <zbufId>
* that contains the byte location specified by <zbufSeg> and *<pOffset>,
* then returns that zbuf segment, and writes in *<pOffset> the new offset
* relative to the returned segment.
*
* If the <zbufSeg>, *<pOffset> pair specify a byte location past the end
* of the zbuf, or before the first byte in the zbuf, zbufSegFind()
* returns NULL.
*
* See the zbufLib manual page for a full discussion of addressing zbufs
* by segment and offset.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned value is valid in the protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf segment ID of the segment containing the specified byte,
* or NULL if the operation fails.
*/

ZBUF_SEG zbufSegFind
    (
    ZBUF_ID		zbufId,		/* zbuf to examine */
    ZBUF_SEG		zbufSeg,	/* zbuf segment base for <pOffset> */
    int *		pOffset		/* relative byte offset */
    )
    {
    return ((pZbufFunc->segFindRtn == NULL) ? NULL :
	(ZBUF_SEG) (pZbufFunc->segFindRtn) (zbufId, zbufSeg, pOffset));
    }

/*******************************************************************************
*
* zbufSegNext - get the next segment in a zbuf
*
* This routine finds the zbuf segment in <zbufId> that is just after
* the zbuf segment <zbufSeg>.  If <zbufSeg> is NULL, the segment after
* the first segment in <zbufId> is returned.  If <zbufSeg> is the last
* segment in <zbufId>, NULL is returned.
*
* RETURNS:
* The zbuf segment ID of the segment after <zbufSeg>,
* or NULL if the operation fails.
*/

ZBUF_SEG zbufSegNext
    (
    ZBUF_ID		zbufId,		/* zbuf to examine */
    ZBUF_SEG		zbufSeg		/* segment to get next segment */
    )
    {
    return ((pZbufFunc->segNextRtn == NULL) ? NULL :
        (ZBUF_SEG) (pZbufFunc->segNextRtn) (zbufId, zbufSeg));
    }

/*******************************************************************************
*
* zbufSegPrev - get the previous segment in a zbuf
*
* This routine finds the zbuf segment in <zbufId> that is just before the zbuf 
* segment <zbufSeg>. If <zbufSeg> is NULL, or is the first segment in <zbufId>,
* NULL is returned.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned value is valid in the protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The zbuf segment ID of the segment before <zbufSeg>, or NULL if the 
* operation fails.
*/

ZBUF_SEG zbufSegPrev
    (
    ZBUF_ID		zbufId,		/* zbuf to examine */
    ZBUF_SEG		zbufSeg		/* segment to get previous segment */
    )
    {
    return ((pZbufFunc->segPrevRtn == NULL) ? NULL :
        (ZBUF_SEG) (pZbufFunc->segPrevRtn) (zbufId, zbufSeg));
    }

/*******************************************************************************
*
* zbufSegData - determine the location of data in a zbuf segment
*
* This routine returns the location of the first byte of data in the zbuf
* segment <zbufSeg>.  If <zbufSeg> is NULL, the location of data in the
* first segment in <zbufId> is returned.
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned value is valid in the protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* A pointer to the first byte of data in the specified zbuf segment,
* or NULL if the operation fails.
*
*/

caddr_t zbufSegData
    (
    ZBUF_ID		zbufId,		/* zbuf to examine */
    ZBUF_SEG		zbufSeg		/* segment to get pointer to data */
    )
    {
    return ((pZbufFunc->segDataRtn == NULL) ? NULL :
	(caddr_t) (pZbufFunc->segDataRtn) (zbufId, zbufSeg));
    }

/*******************************************************************************
*
* zbufSegLength - determine the length of a zbuf segment
*
* This routine returns the number of bytes in the zbuf segment <zbufSeg>.
* If <zbufSeg> is NULL, the length of the first segment in <zbufId> is
* returned.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS:
* The number of bytes in the specified zbuf segment,
* or ERROR if the operation fails.
*/

int zbufSegLength
    (
    ZBUF_ID		zbufId,		/* zbuf to examine */
    ZBUF_SEG		zbufSeg		/* segment to determine length of */
    )
    {
    return ((pZbufFunc->segLengthRtn == NULL) ? ERROR :
	(pZbufFunc->segLengthRtn) (zbufId, zbufSeg));
    }
