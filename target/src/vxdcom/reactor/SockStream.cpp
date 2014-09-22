/* SockStream */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,17dec01,nel  Add include symbol for diab build.
01c,19aug99,aim  change assert to VXDCOM_ASSERT
01b,07jun99,aim  close no longer calls shutdown
01a,11May99,aim  created
*/

#include "SockStream.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_SockStream (void)
    {
    return 0;
    }

SockStream::SockStream ()
    {
    TRACE_CALL;
    }

SockStream::~SockStream ()
    {
    TRACE_CALL;
    }

int
SockStream::readerClose ()
    {
    TRACE_CALL;

    if (handleInvalid ())
	return INVALID_REACTOR_HANDLE;
    else
	return ::shutdown (handleGet (), 0);
    }

int
SockStream::writerClose ()
    {
    TRACE_CALL;

    if (handleInvalid ())
	return INVALID_REACTOR_HANDLE;
    else
	return ::shutdown (handleGet (), 1);
    }

int
SockStream::close ()
    {
    TRACE_CALL;
    return SockEP::close ();
    }
