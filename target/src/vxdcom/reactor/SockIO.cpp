/* SockIO */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,17dec01,nel  Add include symbol for diab build.
01d,17nov99,nel  Cast const char* explicitly to char*
01c,03jun99,aim  fix preprocessor directive
01b,03jun99,aim  changes for VXDCOM_PLATFORM
01a,11May99,aim  created
*/

#include <SockIO.h>

/* Include symbol for diab */
extern "C" int include_vxdcom_SockIO (void)
    {
    return 0;
    }

SockIO::SockIO ()
    {
    TRACE_CALL;
    }

SockIO::~SockIO ()
    {
    TRACE_CALL;
    }

size_t  
SockIO::send (const void *buf, size_t n, int flags) const
    {
    TRACE_CALL;
#if (defined VXDCOM_PLATFORM_WIN32 || defined VXDCOM_PLATFORM_SOLARIS)
    return ::send (handleGet (), static_cast<const char*> (buf), n, flags);
#else
    return ::send (handleGet (), 
                   reinterpret_cast<char*> (const_cast<void*> (buf)), 
                   n, 
                   flags);
#endif
    }

size_t  
SockIO::recv (void *buf, size_t n, int flags) const
    {
    TRACE_CALL;
    return ::recv (handleGet (), static_cast<char*> (buf), n, flags);
    }

size_t  
SockIO::send (const void *buf, size_t n) const
    {
    TRACE_CALL;
#ifdef VXDCOM_PLATFORM_WIN32
    return ::send (handleGet (), static_cast<const char*> (buf), n, 0);
#else
    return ::write (handleGet (), reinterpret_cast<char *> (const_cast<void *> (buf)), n);
#endif
    }

size_t  
SockIO::recv (void *buf, size_t n) const
    {
    TRACE_CALL;
#if (defined VXDCOM_PLATFORM_WIN32 || defined VXDCOM_PLATFORM_SOLARIS)
    return ::recv (handleGet (), static_cast<char*> (buf), n, 0);
#else
    return ::read (handleGet (), static_cast<char*> (buf), n);
#endif
    }

