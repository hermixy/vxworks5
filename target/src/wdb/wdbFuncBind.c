/* wdbFuncBind.c - indirect module calls for the wdb agent */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,07jun95,ms	added __wdbEvtptDeleteAll
01a,12jan95,ms  written.
*/

/*
DESCPRIPTION

This library is used to decouple some wdb modules.
*/

#include "vxWorks.h"
#include "rpc/rpc.h"

BOOL	(*__wdbEventListIsEmpty)	(void);
void	(*__wdbEvtptDeleteAll)		(void);
