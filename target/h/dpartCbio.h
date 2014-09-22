/* dpartCbio.h - disk partition manager header file */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,21sep01,jkf  SPR#69031, common code for both AE & 5.x.
01d,31jul99,jkf  changed maximum partitions to 24, SPR#28277
01c,31jul99,jkf  T2 merge, tidiness & spelling.
01b,14oct98,lrn  T2.0 integration
01a,15jun98,lrn  written.
*/

#ifndef __INCdpartCbioh
#define __INCdpartCbioh

#ifdef __cplusplus
extern "C" {
#endif

/* typedefs */
typedef struct
    {
    u_long	offset ;	/* abs. # of first block in partition */
    u_long	nBlocks ;	/* total # of blocks in partition */
    int		flags ;		/* misc. flags */
    int		spare ;		/* padding, must be zero */
    } PART_TABLE_ENTRY ;

/* defines */
#define	PART_MAX_ENTRIES	24	/* Max # of partitions */

/* prototypes */
IMPORT CBIO_DEV_ID dpartDevCreate(CBIO_DEV_ID subDev,
	int nPart, FUNCPTR pPartDecodeFunc);
IMPORT CBIO_DEV_ID dpartPartGet (CBIO_DEV_ID masterHandle, int partNum);

#ifdef __cplusplus
}
#endif

#endif /*__INCdpartCbioh*/

