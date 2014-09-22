/* assAltern.h - alternative assert code */

/* Copyright 1999 Wind River Systems, Inc. */

/* 
modification history:
--------------------
01a,31jul99,jkf  T2 merge, tidiness & spelling.
*/


#ifdef ASSERT_SUSP
#include <vxWorks.h>
#include <stdioLib.h>
#include <taskLib.h>

#ifdef assert
#undef assert
#endif

#define assert(a)	 if(!(a))\
    {\
    int tId=taskIdSelf();\
    fdprintf(2, "Assertion failed: %s ; task ID: 0x%x\n\
     file: "__FILE__", line: %d.  Task suspended\a\n",\
             #a, tId, __LINE__);\
    taskSuspend(tId);\
    }
#else
#include <assert.h>
#endif	/* ASSERT_SUSP */

