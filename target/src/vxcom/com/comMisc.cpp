/* comMisc.cpp -- VxCOM miscellaneous support code */

/* Copyright (c) 1999-2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01e,17dec01,nel  Add include symbol for diab build.
01d,16jul01,dbs  add VxCondVar methods
01c,28jun01,dbs  move g_defaultServerPriority to comCoreLib
01b,27jun01,dbs  fix include filenames
01a,21jun01,dbs  written (adapted from the old vxdcom.cpp)
*/

#include <stdio.h>
#include "vxWorks.h"
#include "semLib.h"
#include "taskLib.h"
#include "private/comMisc.h"

/* Include symbol for diab */

extern "C" int include_vxcom_comMisc (void)
    {
    return 0;
    }

VxMutex::VxMutex ()
    {
    m_mutex = semMCreate (SEM_Q_FIFO);
    COM_ASSERT (m_mutex);
    }

VxMutex::~VxMutex ()
    {
    semDelete (m_mutex);
    }

void VxMutex::lock (unsigned long ms) const
    {
    STATUS result = semTake (m_mutex, MS2TICKS(ms));
    COM_ASSERT(result == OK);
    if (result != OK)
	taskSuspend (taskIdSelf ());
    }

void VxMutex::unlock () const
    {
    semGive (m_mutex);
    }

VxCondVar::VxCondVar ()
    {
    m_condvar = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    COM_ASSERT (m_condvar);
    }

VxCondVar::~VxCondVar ()
    {
    semDelete (m_condvar);
    }

void VxCondVar::wait (const VxMutex& mtx)
    {
    mtx.unlock ();
    semTake (m_condvar, WAIT_FOREVER);
    mtx.lock (WAIT_FOREVER);
    }

void VxCondVar::signal () const
    {
    semGive (m_condvar);
    }

void comAssertFailed
    (
    const char* expr,
    const char* fileName,
    int         lineNo
    )
    {
    printf ("VxCOM: Assertion failed: %s (%s:%d)\n",
            expr,
            fileName,
            lineNo);
    taskSuspend (taskIdSelf ());
    }
