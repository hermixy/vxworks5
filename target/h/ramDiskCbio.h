/* ramDiskCbio.h - header file for ramDiskCbio.c */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,29feb00,jkf   T3 changes
01a,31aug99,jkf   written
*/

#ifndef __INCramDiskCbioh
#define __INCramDiskCbioh

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

IMPORT CBIO_DEV_ID ramDiskDevCreate
    (
    char  * pRamAddr,    /* where it is in memory (0 = malloc)     */
    int   bytesPerBlk,   /* number of bytes per block              */
    int   blksPerTrack,  /* number of blocks per track             */
    int   nBlocks,       /* number of blocks on this device        */
    int   blkOffset      /* no. of blks to skip at start of device */
    );


#else

IMPORT CBIO_DEV_ID ramDiskDevCreate();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCramDiskCbioh */
