/* dcacheCbio.h - disk cache manager header file */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,04mar02,jkf  SPR#32277, adding dcacheDevEnable and Disable
01f,21sep01,jkf  SPR#69031, common code for both AE & 5.x.
01e,29feb00,jkf  T3 changes
01d,31jul99,jkf  T2 merge, tidiness & spelling.
01c,09sep98,lrn  added dcacheDevMemResize() prototype
01b,06sep98,lrn  changed dcacheDevCreate() prototype for CBIO subordinate
01a,15jun98,lrn  written.
*/

#ifndef __INCdcacheCbioh
#define __INCdcacheCbioh

#ifdef __cplusplus
extern "C" {
#endif

/* defines */
#define	DCACHE_MAX_DEVS	    16		/* max # of cached devices */

/* globals */
IMPORT int dcacheUpdTaskId ;		/* updater task id, one per system */
IMPORT int dcacheUpdTaskPriority ;	/* updater task priority - tunable */
IMPORT int dcacheUpdTaskStack ;		/* updater task stack size - tunable */

/* prototypes */

IMPORT CBIO_DEV_ID dcacheDevCreate (
    CBIO_DEV_ID subDev,	  /* lower level device handle */
    char  *   pRamAddr,    /* where it is in memory (NULL = malloc)  */
    int       memSize,	  /* amount of memory to use                */
    char *    pDesc /* device pDesc string */
    ) ;

IMPORT STATUS dcacheDevTune (
    CBIO_DEV_ID dev,		/* device handle */
    int		dirtyMax,	/* max # of dirty cache blocks allowed */
    int		bypassCount,	/* request size for bypassing cache */
    int		readAhead,	/* how many blocks to read ahead */
    int		syncInterval	/* how many seconds between disk updates */
    ) ;

IMPORT void dcacheShow (
    CBIO_DEV_ID dev,	/* device handle */
    int verbose		/* 1: display state of each cache block */
    ) ;

IMPORT STATUS dcacheDevMemResize
    (
    CBIO_DEV_ID dev,		/* device handle */
    size_t	newSize		/* new cache size in bytes */
    ) ;

IMPORT STATUS dcacheDevEnable
    (
    CBIO_DEV_ID dev  /* CBIO device handle */
    );

IMPORT STATUS dcacheDevDisable
    (
    CBIO_DEV_ID dev   /* CBIO device handle */
    );

#ifdef __cplusplus
}
#endif

#endif /*__INCdcacheCbioh*/
