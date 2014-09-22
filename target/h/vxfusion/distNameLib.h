/* distNameLib.h - distributed name database header (VxFusion) */

/* Copyright 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,24may99,drm  adding vxfusion prefix to VxFusion related includes
01b,12aug98,drm  removed #include of private file
01a,10jun97,ur   written.
*/

#ifndef __INCdistNameLibh
#define __INCdistNameLibh

#include "vxWorks.h"
#include "vwModNum.h"
#include "vxfusion/distLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

#define T_DIST_MSG_Q	DIST_OBJ_TYPE_MSG_Q  /* message queue */
#define T_DIST_NODE		16                   /* node identifier */
#define T_DIST_UINT8	64                   /* 8 bit integer */
#define T_DIST_UINT16	65                   /* 16 bit integer */
#define T_DIST_UINT32	66                   /* 32 bit integer */
#define T_DIST_UINT64	67                   /* 64 bit integer */
#define T_DIST_FLOAT	68                   /* float */
#define T_DIST_DOUBLE	69                   /* double */

#define S_distNameLib_NAME_TOO_LONG		(M_distNameLib | 1)  /* error code */
#define S_distNameLib_ILLEGAL_LENGTH	(M_distNameLib | 2)  /* error code */
#define S_distNameLib_INVALID_WAIT_TYPE	(M_distNameLib | 3)  /* error code */
#define S_distNameLib_DATABASE_FULL		(M_distNameLib | 4)  /* error code */
#define S_distNameLib_INCORRECT_LENGTH	(M_distNameLib | 5)  /* error code */

/* typedefs */

typedef uint16_t DIST_NAME_TYPE;		/* type of object assoc. with name */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

void	distNameLibInit (void);
STATUS	distNameInit (int sizeLog2);
STATUS	distNameAdd (char *name, void *value, int len, DIST_NAME_TYPE type);
STATUS	distNameFind (char *name, void **pValue, DIST_NAME_TYPE *pType,
				int waitType);
STATUS	distNameFindByValueAndType (void *value, DIST_NAME_TYPE pType,
				char *name, int waitType);
STATUS	distNameRemove (char *name);
void	distNameShow (void);
void	distNameFilterShow (DIST_NAME_TYPE type);

#else	/* __STDC__ */

void	distNameLibInit ();
STATUS	distNameInit ();
STATUS	distNameAdd ();
STATUS	distNameFind ();
STATUS	distNameFindByValueAndType ();
STATUS	distNameRemove ();
void	distNameShow ();
void	distNameFilterShow ();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCdistNameLibh */

