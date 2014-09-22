/* ReactorTypes.h - Common types used in the reactor module */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,13jul01,dbs  fix up includes, remove win32 refs
*/

#ifndef __INCReactorTypes_h
#define __INCReactorTypes_h

#include "Syslog.h"
#include "TraceCall.h"

///////////////////////////////////////////////////////////////////////////////
// Common OS Headers

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#include "private/comMisc.h"
#include "private/comStl.h"

///////////////////////////////////////////////////////////////////////////////
// VXWORKS

#ifdef VXDCOM_PLATFORM_VXWORKS
#include "vxWorks.h"
#include "sockLib.h"
#include "hostLib.h"
#include "inetLib.h"
#include "selectLib.h"
#include "netdb.h"
#include "unistd.h"
#endif // VXDCOM_PLATFORM_VXWORKS

///////////////////////////////////////////////////////////////////////////////
// SOLARIS

#ifdef VXDCOM_PLATFORM_SOLARIS
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "netdb.h"
#include "unistd.h"
#include "fcntl.h"
#include "memory.h"
#endif // VXDCOM_PLATFORM_SOLARIS

///////////////////////////////////////////////////////////////////////////////
// LINUX

#ifdef VXDCOM_PLATFORM_LINUX
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "netdb.h"
#include "unistd.h"
#include "fcntl.h"
#include "memory.h"
#endif // VXDCOM_PLATFORM_LINUX

///////////////////////////////////////////////////////////////////////////////

typedef int REACTOR_HANDLE;
typedef fd_set REACTOR_HANDLE_SET_TYPE;
typedef unsigned long REACTOR_EVENT_MASK;
const int INVALID_REACTOR_HANDLE = -1;

// Acceptor, Connector and SvcHandler template parameters.

// Handle ACE_Connector

#if (defined VXDCOM_PLATFORM_SOLARIS || defined VXDCOM_PLATFORM_LINUX)
#   define PEER_STREAM_1 class _PEER_STREAM
#   define PEER_STREAM_2 _PEER_STREAM
#   define PEER_STREAM _PEER_STREAM
#   define PEER_STREAM_ADDR typename _PEER_STREAM::PEER_ADDR
#   define PEER_ACCEPTOR_1 class _PEER_ACCEPTOR
#   define PEER_ACCEPTOR_2 _PEER_ACCEPTOR
#   define PEER_ACCEPTOR _PEER_ACCEPTOR
#   define PEER_ACCEPTOR_ADDR typename _PEER_ACCEPTOR::PEER_ADDR
#   define PEER_CONNECTOR_1 class _PEER_CONNECTOR
#   define PEER_CONNECTOR_2 _PEER_CONNECTOR
#   define PEER_CONNECTOR _PEER_CONNECTOR
#   define PEER_CONNECTOR_ADDR typename _PEER_CONNECTOR::PEER_ADDR
#else
#   define PEER_STREAM_1 class _PEER_STREAM, class _PEER_ADDR
#   define PEER_STREAM_2 _PEER_STREAM, _PEER_ADDR
#   define PEER_STREAM _PEER_STREAM
#   define PEER_STREAM_ADDR _PEER_ADDR
#   define PEER_ACCEPTOR_1 class _PEER_ACCEPTOR, class _PEER_ADDR
#   define PEER_ACCEPTOR_2 _PEER_ACCEPTOR, _PEER_ADDR
#   define PEER_ACCEPTOR _PEER_ACCEPTOR
#   define PEER_ACCEPTOR_ADDR _PEER_ADDR
#   define PEER_CONNECTOR_1 class _PEER_CONNECTOR, class _PEER_ADDR
#   define PEER_CONNECTOR_2 _PEER_CONNECTOR, _PEER_ADDR
#   define PEER_CONNECTOR _PEER_CONNECTOR
#   define PEER_CONNECTOR_ADDR _PEER_ADDR
#   define PEER_CONNECTOR_ADDR_ANY PEER_CONNECTOR_ADDR::sap_any
#endif // (defined VXDCOM_PLATFORM_SOLARIS)

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#endif // __INCReactorTypes_h
