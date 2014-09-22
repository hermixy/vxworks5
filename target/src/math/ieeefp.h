/* ieeefp.h */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,14nov01,to  use IEEE little endian for ARM
01a,31oct01,sn  better deduction of endianness
                added this mod history
*/

#ifndef __INCieeefph
#define __INCieeefph

#include "vxWorks.h"

#if (CPU_FAMILY == SH)
#ifdef __SH3E__
#define _DOUBLE_IS_32BITS
#endif
#endif

#if defined (__H8300__) || defined (__H8300H__)
#define __SMALL_BITFIELDS
#define _DOUBLE_IS_32BITS
#endif

#ifdef __H8500__
#define __SMALL_BITFIELDS
#define _DOUBLE_IS_32BITS
#endif

#ifdef __W65__
#define __SMALL_BITFIELDS
#define _DOUBLE_IS_32BITS
#endif

#ifndef __IEEE_BIG_ENDIAN
#ifndef __IEEE_LITTLE_ENDIAN
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define __IEEE_LITTLE_ENDIAN
#else
#define __IEEE_BIG_ENDIAN
#endif /* _BYTE_ORDER == _LITTLE_ENDIAN */
#endif /* not __IEEE_LITTLE_ENDIAN */
#endif /* not __IEEE_BIG_ENDIAN */

#endif /* __INCieeefph */
