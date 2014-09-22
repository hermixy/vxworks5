/* cacheLib.c - cache management library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01s,22aug01,dgp  correct drvExample1( ) per SPR 66362
01r,26jun96,dbt  added test (address == NONE) in cacheTextUpdate (SPR #4262).
		 Update copyright.
01q,02aug95,tpr  added MC68060 CPU test in cacheFuncsSet().
01p,27oct94,hdn  added support for I80X86 for external write-back cache.
01o,22sep94,rhp  doc: fix typo in CACHE_LIB structure description (spr#2491).
01n,06jan93,jdi  documentation cleanup.
01m,19oct92,jcf  retained 5.0 cache defines and reworked xxxInit param ordering.
01l,29sep92,jwt  merged cacheLibInit(), cacheReset(), and cacheModeSet().
01k,17sep92,jdi  documentation cleanup, and ansification.
01j,16sep92,jwt  documentation cleanup for DNW changes in version 01g.
01i,23aug92,jcf  moved cache* function pointer structs to funcBind.c.
01h,04aug92,ajm  passed correct parameters in cacheTextUpdate
01g,31jul92,jwt  put function pointers back in "data" section.
    30jul92,dnw  added cacheLib functions and CACHE_FUNCS structures.
01f,24jul92,jwt  added more comprehensive write buffer description.
01e,09jul92,jwt  added description for virtual<->physical, no code mods.
01d,07jul92,ajm  forward declared sysCacheLibInit for mips compiler
01c,07jul92,jwt  more clean up of descriptions and binding to BSP.
01b,06jul92,jwt  cleaned up descriptions and binding to BSP.
01a,03jul92,jwt  created - top-level cache library for VxWorks 5.1 release.
*/

/*
DESCRIPTION
This library provides architecture-independent routines for managing
the instruction and data caches.  Architecture-dependent routines are
documented in the architecture-specific libraries.  

The cache library is initialized by cacheLibInit() in usrInit().  The
cacheLibInit() routine typically calls an architecture-specific
initialization routine in one of the architecture-specific libraries.  The
initialization routine places the cache in a known and quiescent state,
ready for use, but not yet enabled.  Cache devices are enabled and disabled
by calls to cacheEnable() and cacheDisable(), respectively.  

The structure CACHE_LIB in cacheLib.h provides a function pointer that
allows for the installation of different cache implementations in an
architecture-independent manner.  If the processor family allows more than
one cache implementation, the board support package (BSP) must select the
appropriate cache library using the function pointer `sysCacheLibInit'.
The cacheLibInit() routine calls the initialization function attached to
`sysCacheLibInit' to perform the actual CACHE_LIB function pointer
initialization (see cacheLib.h).  Note that `sysCacheLibInit' must be
initialized when declared; it need not exist for architectures with a
single cache design.  Systems without caches have all NULL pointers in the
CACHE_LIB structure.  For systems with bus snooping, NULLifying the flush
and invalidate function pointers in sysHwInit() improves overall system
and driver performance.

Function pointers also provide a way to supplement the cache library or
attach user-defined cache functions for managing secondary cache systems.

Parameters specified by cacheLibInit() are used to select the cache mode,
either write-through (CACHE_WRITETHROUGH) or copyback (CACHE_COPYBACK), as
well as to implement all other cache configuration features via software
bit-flags.  Note that combinations, such as setting copyback and
write-through at the same time, do not make sense.

Typically, the first argument passed to cache routines after initialization 
is the CACHE_TYPE, which selects the data cache (DATA_CACHE) or the 
instruction cache (INSTRUCTION_CACHE).  

Several routines accept two additional arguments: an address and
the number of bytes.  Some cache operations can be applied to the
entire cache (bytes = ENTIRE_CACHE) or to a portion of the cache.  This
range specification allows the cache to be selectively locked, unlocked,
flushed, invalidated, and cleared.  The two complementary routines,
cacheDmaMalloc() and cacheDmaFree(), are tailored for efficient driver writing.
The cacheDmaMalloc() routine attempts to return a "cache-safe" buffer,
which is created by the MMU and a set of flush and invalidate function
pointers.  Examples are provided below in the section "Using the Cache
Library."

Most routines in this library return a STATUS value of OK, or 
ERROR if the cache selection is invalid or the cache operation fails.  

BACKGROUND
The emergence of RISC processors and effective CISC caches has made cache
and MMU support a key enhancement to VxWorks.  (For more information about MMU
support, see the manual entry for vmLib.)  The VxWorks cache strategy
is to maintain coherency between the data cache and RAM and between the
instruction and data caches.  VxWorks also preserves overall system
performance.  The product is designed to support several architectures and
board designs, to have a high-performance implementation for drivers, and
to make routines functional for users, as well as within the entire
operating system.  The lack of a consistent cache design, even within
architectures, has required designing for the case with the greatest number of
coherency issues (Harvard architecture, copyback mode, DMA devices,
multiple bus masters, and no hardware coherency support).

Caches run in two basic modes, write-through and copyback.  The
write-through mode forces all writes to the cache and to RAM, providing
partial coherency.  Writing to RAM every time, however, slows down the
processor and uses bus bandwidth.  The copyback mode conserves processor
performance time and bus bandwidth by writing only to the cache, not RAM.
Copyback cache entries are only written to memory on demand.  A Least
Recently Used (LRU) algorithm is typically used to determine which cache
line to displace and flush.  Copyback provides higher system performance,
but requires more coherency support.  Below is a logical diagram of a
cached system to aid in the visualization of the coherency issues.
.CS
.vs -4
.ne 20
   +---------------+     +-----------------+     +--------------+
   |               |     |                 |     |              |
   |  INSTRUCTION  |---->|    PROCESSOR    |<--->|  DATA CACHE  | (3)
   |     CACHE     |     |                 |     |  (copyback)  |
   |               |     |                 |     |              |
   +---------------+     +-----------------+     +--------------+
           ^                     (2)                     ^
           |                                             |
           |             +-----------------+             |
           |             |                 |         (1) |
           +-------------|       RAM       |<------------+
                         |                 |
                         +-----------------+
                             ^         ^
                             |         |
           +-------------+   |         |   +-------------+
           |             |   |         |   |             |
           | DMA Devices |<--+         +-->| VMEbus, etc.|
           |             |                 |             |
           +-------------+                 +-------------+
.vs
.CE
The loss of cache coherency for a VxWorks system occurs in three places:

    (1) data cache / RAM
    (2) instruction cache / data cache
    (3) shared cache lines

A problem between the data cache and RAM (1) results from asynchronous
accesses (reads and writes) to the RAM by the processor and other
masters.  Accesses by DMA devices and alternate bus masters (shared
memory) are the primary causes of incoherency, which can be remedied with
minor code additions to the drivers.  

The instruction cache and data cache (2) can get out of sync when the
loader, the debugger, and the interrupt connection routines are being
used.  The instructions resulting from these operations are loaded into
the data cache, but not necessarily the instruction cache, in which case
there is a coherency problem.  This can be fixed by "flushing" the data
cache entries to RAM, then "invalidating" the instruction cache entries.
The invalid instruction cache tags will force the retrieval of the new
instructions that the data cache has just flushed to RAM.

Cache lines that are shared (3) by more than one task create coherency
problems.  These are manifest when one thread of execution invalidates a
cache line in which entries may belong to another thread.  This can be
avoided by allocating memory on a cache line boundary, then rounding up to
a multiple of the cache line size.

The best way to preserve cache coherency with optimal performance
(Harvard architecture, copyback mode, no software intervention) is
to use hardware with bus snooping capabilities.  The caches, the RAM, the
DMA devices, and all other bus masters are tied to a physical bus
where the caches can "snoop" or watch the bus transactions.  The
address cycle and control (read/write) bits are broadcast on the bus
to allow snooping.  Data transfer cycles are deferred until absolutely
necessary.  When one of the entries on the physical side of the cache
is modified by an asynchronous action, the cache(s) marks its
entry(s) as invalid.  If an access is made by the processor (logical
side) to the now invalid cached entry, it is forced to retrieve the
valid entry from RAM.  If while in copyback mode the processor writes
to a cached entry, the RAM version becomes stale.  If another master
attempts to access that stale entry in RAM, the cache with the valid
version pre-empts the access and writes the valid data to RAM.  The
interrupted access then restarts and retrieves the now-valid data in
RAM.  Note that this configuration allows only one valid entry at any
time.  At this time, only a few boards provide the snooping capability; 
therefore, cache support software must be designed to handle
incoherency hazards without degrading performance.

The determinism, interrupt latency, and benchmarks for a cached system are
exceedingly difficult to specify (best case, worst case, average case) due
to cache hits and misses, line flushes and fills, atomic burst cycles,
global and local instruction and data cache locking, copyback versus
write-through modes, hardware coherency support (or lack of), and MMU
operations (table walks, TLB locking).

USING THE CACHE LIBRARY
The coherency problems described above can be overcome by adding cache
support to existing software.  For code segments that are not
time-critical (loader, debugger, interrupt connection), the following
sequence should be used first to flush the data cache entries and then 
to invalidate the corresponding instruction cache entries.
.CS
    cacheFlush (DATA_CACHE, address, bytes);
    cacheInvalidate (INSTRUCTION_CACHE, address, bytes);
.CE
For time-critical code, implementation is up to the driver writer.
The following are tips for using the VxWorks cache library effectively.  

Incorporate cache calls in the driver program to maintain overall system
performance.  The cache may be disabled to facilitate driver development;
however, high-performance production systems should operate with the cache
enabled.  A disabled cache will dramatically reduce system performance for
a completed application.

Buffers can be static or dynamic.  Mark buffers "non-cacheable" to avoid
cache coherency problems.  This usually requires MMU support.  Dynamic
buffers are typically smaller than their static counterparts, and they are
allocated and freed often.  When allocating either type of buffer, it
should be designated non-cacheable; however, dynamic buffers should be
marked "cacheable" before being freed.  Otherwise, memory becomes
fragmented with numerous non-cacheable dynamic buffers.

Alternatively, use the following flush/invalidate scheme to maintain 
cache coherency.
.CS
    cacheInvalidate (DATA_CACHE, address, bytes);   /@ input buffer  @/
    cacheFlush (DATA_CACHE, address, bytes);        /@ output buffer @/
.CE
The principle is to flush output buffers before each use and invalidate
input buffers before each use.  Flushing only writes modified entries back
to RAM, and instruction cache entries never get modified.

Several flush and invalidate macros are defined in cacheLib.h.  Since
optimized code uses these macros, they provide a mechanism to avoid
unnecessary cache calls and accomplish the necessary work (return OK).
Needless work includes flushing a write-through cache, flushing or
invalidating cache entries in a system with bus snooping, and flushing
or invalidating cache entries in a system without caches.  The macros
are set to reflect the state of the cache system hardware and software.
.SH "Example 1"
The following example is of a simple driver that uses cacheFlush()
and cacheInvalidate() from the cache library to maintain coherency and
performance.  There are two buffers (lines 3 and 4), one for input and
one for output.  The output buffer is obtained by the call to
memalign(), a special version of the well-known malloc() routine (line
6).  It returns a pointer that is rounded down and up to the alignment
parameter's specification.  Note that cache lines should not be shared,
therefore _CACHE_ALIGN_SIZE is used to force alignment.  If the memory
allocator fails (line 8), the driver will typically return ERROR (line
9) and quit.

The driver fills the output buffer with initialization information,
device commands, and data (line 11), and is prepared to pass the
buffer to the device.  Before doing so the driver must flush the data
cache (line 13) to ensure that the buffer is in memory, not hidden in
the cache.  The drvWrite() routine lets the device know that the data
is ready and where in memory it is located (line 14).

More driver code is executed (line 16), then the driver is ready to
receive data that the device has placed in an input buffer in memory
(line 18).  Before the driver can work with the incoming data, it must
invalidate the data cache entries (line 19) that correspond to the input
buffer's data in order to eliminate stale entries.  That done, it is safe for
the driver to retrieve the input data from memory (line 21).  Remember
to free (line 23) the buffer acquired from the memory allocator.  The
driver will return OK (line 24) to distinguish a successful from an
unsuccessful operation.
.CS
STATUS drvExample1 ()		/@ simple driver - good performance @/
    {
3:  void *	pInBuf;		/@ input buffer @/
4:  void *	pOutBuf;	/@ output buffer @/
    
6:  pOutBuf = memalign (_CACHE_ALIGN_SIZE, BUF_SIZE);

8:  if (pOutBuf == NULL)
9:      return (ERROR);		/@ memory allocator failed @/

11: /@ other driver initialization and buffer filling @/

13: cacheFlush (DATA_CACHE, pOutBuf, BUF_SIZE);
14: drvWrite (pOutBuf);		/@ output data to device @/

16: /@ more driver code @/

18: cacheClear (DATA_CACHE, pInBuf, BUF_SIZE);
19: pInBuf = drvRead ();	/@ wait for device data @/

21: /@ handle input data from device @/

23: free (pOutBuf);		/@ return buffer to memory pool @/
24: return (OK);
    }
.CE

Extending this flush/invalidate concept further, individual
buffers can be treated this way, not just the entire cache system.  The 
idea is to avoid
unnecessary flush and/or invalidate operations on a per-buffer basis by
allocating cache-safe buffers.  Calls to cacheDmaMalloc() optimize the
flush and invalidate function pointers to NULL, if possible, while
maintaining data integrity.
.SH "Example 2"
The following example is of a high-performance driver that takes advantage
of the cache library to maintain coherency.  It uses cacheDmaMalloc() and
the macros CACHE_DMA_FLUSH and CACHE_DMA_INVALIDATE.  A buffer pointer
is passed as a parameter (line 2).  If the pointer is not NULL (line 7),
it is assumed that the buffer will not experience any cache coherency
problems.  If the driver was not provided with a cache-safe buffer, it
will get one (line 11) from cacheDmaMalloc().  A CACHE_FUNCS structure
(see cacheLib.h) is used to create a buffer that will not suffer from cache
coherency problems.  If the memory allocator fails (line 13), the driver
will typically return ERROR (line 14) and quit.

The driver fills the output buffer with initialization information,
device commands, and data (line 17), and is prepared to pass the
buffer to the device.  Before doing so, the driver must flush the data
cache (line 19) to ensure that the buffer is in memory, not hidden in
the cache.  The routine drvWrite() lets the device know that the data
is ready and where in memory it is located (line 20).

More driver code is executed (line 22), and the driver is then ready to
receive data that the device has placed in the buffer in memory (line
24).  Before the driver cache can work with the incoming data, it must
invalidate the data cache entries (line 25) that correspond to the input
buffer`s data in order to eliminate stale entries.  That done, it is safe 
for the driver to handle the input data (line 27), which the driver 
retrieves from memory.  Remember to free the buffer (line 29) acquired 
from the memory allocator.  The driver will return OK (line 30) to 
distinguish a successful from an unsuccessful operation.
.CS
STATUS drvExample2 (pBuf)	/@ simple driver - great performance @/
2:  void *	pBuf;		/@ buffer pointer parameter @/

    {
5:  if (pBuf != NULL)
	{
7:	/@ no cache coherency problems with buffer passed to driver @/
	}
    else
        {
11:     pBuf = cacheDmaMalloc (BUF_SIZE);

13:     if (pBuf == NULL)
14:         return (ERROR);	/@ memory allocator failed @/
        }

17: /@ other driver initialization and buffer filling @/

19: CACHE_DMA_FLUSH (pBuf, BUF_SIZE);
20: drvWrite (pBuf);		/@ output data to device @/

22: /@ more driver code @/

24: drvWait ();			/@ wait for device data @/
25: CACHE_DMA_INVALIDATE (pBuf, BUF_SIZE);

27: /@ handle input data from device @/

29: cacheDmaFree (pBuf);	/@ return buffer to memory pool @/
30: return (OK);
    }
.CE

Do not use CACHE_DMA_FLUSH or CACHE_DMA_INVALIDATE without first calling
cacheDmaMalloc(), otherwise the function pointers may not be
initialized correctly.  Note that this driver scheme assumes all cache
coherency modes have been set before driver initialization, and that
the modes do not change after driver initialization.  The cacheFlush() and
cacheInvalidate() functions can be used at any time throughout the system
since they are affiliated with the hardware, not the malloc/free buffer.

A call to cacheLibInit() in write-through mode makes the flush function
pointers NULL.  Setting the caches in copyback mode (if supported) should
set the pointer to and call an architecture-specific flush routine.  The
invalidate and flush macros may be NULLified if the hardware provides bus
snooping and there are no cache coherency problems.
.SH "Example 3"
The next example shows a more complex driver that requires address
translations to assist in the cache coherency scheme.  The previous
example had `a priori' knowledge of the system memory map and/or the device
interaction with the memory system.  This next driver demonstrates a case 
in which the virtual address returned by cacheDmaMalloc() might differ from 
the physical address seen by the device.  It uses the CACHE_DMA_VIRT_TO_PHYS 
and CACHE_DMA_PHYS_TO_VIRT macros in addition to the CACHE_DMA_FLUSH and
CACHE_DMA_INVALIDATE macros.

The cacheDmaMalloc() routine initializes the buffer pointer (line 3).  If
the memory allocator fails (line 5), the driver will typically return
ERROR (line 6) and quit.  The driver fills the output buffer with
initialization information, device commands, and data (line 8), and is
prepared to pass the buffer to the device.  Before doing so, the driver
must flush the data cache (line 10) to ensure that the buffer is in
memory, not hidden in the cache.  The flush is based on the virtual
address since the processor filled in the buffer.  The drvWrite() routine
lets the device know that the data is ready and where in memory it is
located (line 11).  Note that the CACHE_DMA_VIRT_TO_PHYS macro converts
the buffer's virtual address to the corresponding physical address for the
device.

More driver code is executed (line 13), and the driver is then ready to
receive data that the device has placed in the buffer in memory (line
15).  Note the use of the CACHE_DMA_PHYS_TO_VIRT macro on the buffer
pointer received from the device.  Before the driver cache can work
with the incoming data, it must invalidate the data cache entries (line
16) that correspond to the input buffer's data in order to eliminate 
stale entries.  That done, it is safe for the driver to handle the input 
data (line 17), which it retrieves from memory.  Remember to free (line
19) the buffer acquired from the memory allocator.  The driver will return
OK (line 20) to distinguish a successful from an unsuccessful operation.
.CS
STATUS drvExample3 ()           /@ complex driver - great performance @/ {
3:  void * pBuf = cacheDmaMalloc (BUF_SIZE);

5:  if (pBuf == NULL)
6:      return (ERROR);		/@ memory allocator failed @/

8: /@ other driver initialization and buffer filling @/

10: CACHE_DMA_FLUSH (pBuf, BUF_SIZE);
11: drvWrite (CACHE_DMA_VIRT_TO_PHYS (pBuf));

13: /@ more driver code @/

15: pBuf = CACHE_DMA_PHYS_TO_VIRT (drvRead ());
16: CACHE_DMA_INVALIDATE (pBuf, BUF_SIZE);

17: /@ handle input data from device @/

19: cacheDmaFree (pBuf);	/@ return buffer to memory pool @/
20: return (OK);
    }
.CE
.SH "Driver Summary"
The virtual-to-physical and physical-to-virtual function pointers
associated with cacheDmaMalloc() are supplements to a cache-safe buffer.
Since the processor operates on virtual addresses and the devices access
physical addresses, discrepant addresses can occur and might prevent
DMA-type devices from being able to access the allocated buffer.
Typically, the MMU is used to return a buffer that has pages marked as 
non-cacheable.  An MMU is used to translate virtual addresses into physical
addresses, but it is not guaranteed that this will be a "transparent"
translation.

When cacheDmaMalloc() does something that makes the virtual address
different from the physical address needed by the device, it provides the
translation procedures.  This is often the case when using translation
lookaside buffers (TLB) or a segmented address space to inhibit caching
(e.g., by creating a different virtual address for the same physical
space.)  If the virtual address returned by cacheDmaMalloc() is the same
as the physical address, the function pointers are made NULL so that no calls
are made when the macros are expanded.
.SH "Board Support Packages"
Each board for an architecture with more than one cache implementation has
the potential for a different cache system.  Hence the BSP for selecting
the appropriate cache library.  The function pointer `sysCacheLibInit' is
set to cacheXxxLibInit() ("Xxx" refers to the chip-specific name of a
library or function) so that the function pointers for that cache system
will be initialized and the linker will pull in only the desired cache
library.  Below is an example of cacheXxxLib being linked in by sysLib.c.
For systems without caches and for those architectures with only one cache
design, there is no need for the `sysCacheLibInit' variable.
.CS
     FUNCPTR sysCacheLibInit = (FUNCPTR) cacheXxxLibInit;
.CE
For cache systems with bus snooping, the flush and invalidate macros
should be NULLified to enhance system and driver performance in sysHwInit().
.CS
     void sysHwInit ()
        {
        ...
	cacheLib.flushRtn = NULL;	/@ no flush necessary @/
	cacheLib.invalidateRtn = NULL;	/@ no invalidate necessary @/
        ...
        }
.CE
There may be some drivers that require numerous cache calls, so many 
that they interfere with the code clarity.  Additional checking can be
done at the initialization stage to determine if cacheDmaMalloc() returned
a buffer in non-cacheable space.  Remember that it will return a
cache-safe buffer by virtue of the function pointers.  Ideally, these are
NULL, since the MMU was used to mark the pages as non-cacheable.  The
macros CACHE_Xxx_IS_WRITE_COHERENT and CACHE_Xxx_IS_READ_COHERENT
can be used to check the flush and invalidate function pointers,
respectively.

Write buffers are used to allow the processor to continue execution while
the bus interface unit moves the data to the external device.  In theory,
the write buffer should be smart enough to flush itself when there is a
write to non-cacheable space or a read of an item that is in the buffer.
In those cases where the hardware does not support this, the software must
flush the buffer manually.  This often is accomplished by a read to
non-cacheable space or a NOP instruction that serializes the chip's
pipelines and buffers.  This is not really a caching issue; however, the
cache library provides a CACHE_PIPE_FLUSH macro.  External write buffers
may still need to be handled in a board-specific manner.

INTERNAL 
The manipulation of the cache and/or cache registers should be done in
the most efficient way possible to maintain driver performance.  Some
of the code may have to be written in assembly language depending on
the cache design.  Allowing different granularities for the cache
operations helps reduce the overhead associated with the cache
coherency software.  This may be applicable to all the cache calls,
and will vary with each architecture.  Examples are entry, line, set,
page, segment, region, context.  MMU support is needed to selectively
enable and disable parts of the address space, and is typically done
on a "page" basis.  Board-specific and some secondary cache operations
should be part of the BSP.  The inclusion of routines that assist in
debugging (cache tag dumps) is desirable.

The memory allocator must round addresses down to a cache line
boundary, and return a number of bytes that is a multiple of the cache
line size to prevent shared cache lines.  Cache operations on address
ranges must be rounded down and up to the appropriate boundaries
(line, page, etc.).  _CACHE_ALIGN_SIZE may be used to force alignment
when using memalign().

The atomicity of the cache operations is preserved by locking
interrupts and disabling caching.  Interrupts should be locked out
while the cache is disabled since interrupt handling and other tasks
would be run with caching disabled until the preempted task is
switched back in, if it is ever switched back in.  The cache must be
disabled while performing most of these operations.  For example, the
cache should not be loading new entries during an invalidate operation
and lose critical data such as stack entries.

cacheDmaMalloc() returns the memalign() return value, or NULL if there
is some procedure-specific error in cache<Xxx>DmaMalloc().  The flush
and invalidate function pointers should be set as follows:

    NO DATA CACHE		cacheDrv.flushRtn = NULL
                                cacheDrv.invalidateRtn = NULL
    DATA CACHE DISABLED		cacheDrv.flushRtn = NULL
                                cacheDrv.invalidateRtn = NULL
    DATA CACHING INHIBITED	cacheDrv.flushRtn = NULL
                                cacheDrv.invalidateRtn = NULL
    BUS SNOOPING HARDWARE	cacheDrv.flushRtn = NULL
                                cacheDrv.invalidateRtn = NULL
    WRITE-THROUGH DATA CACHE	cacheDrv.flushRtn = NULL
                                cacheDrv.invalidate = cache<Xxx>Invalidate()
    COPYBACK DATA CACHE		cacheDrv.flushRtn = cache<Xxx>Flush()
                                cacheDrv.invalidate = cache<Xxx>Invalidate()
    RETURN NULL BUFFER POINTER	cacheDrv.flushRtn = cache<Xxx>Flush()
                                cacheDrv.invalidate = cache<Xxx>Invalidate()

The setting of the flush function pointer may not be NULL if the system has 
write buffers that need to be flushed.

INCLUDE FILES: cacheLib.h

SEE ALSO: Architecture-specific cache-management libraries (cacheXxxLib), vmLib,
.pG "I/O System"
*/

#include "cacheLib.h"
#include "stdlib.h"
#include "private/vmLibP.h"

/* globals */

FUNCPTR cacheDmaMallocRtn = NULL;	/* malloc dma memory if MMU */
FUNCPTR cacheDmaFreeRtn = NULL;		/* free dma memory if MMU */

CACHE_MODE cacheDataMode = CACHE_DISABLED;	/* data cache mode */
BOOL cacheDataEnabled = FALSE;		/* data cache disabled */
BOOL cacheMmuAvailable = FALSE;		/* MMU support available */

/* externals */

IMPORT FUNCPTR sysCacheLibInit;		/* Board-specific cache library */

/**************************************************************************
*
* cacheLibInit - initialize the cache library for a processor architecture
*
* This routine initializes the function pointers for the appropriate cache 
* library.  For architectures with more than one cache implementation, the
* board support package must select the appropriate cache library with 
* `sysCacheLibInit'.  Systems without cache coherency problems (i.e., bus 
* snooping) should NULLify the flush and invalidate function pointers in 
* the cacheLib structure to enhance driver and overall system performance.
* This can be done in sysHwInit().
* 
* RETURNS: OK, or ERROR if there is no cache library installed.
*/

STATUS cacheLibInit
    (
    CACHE_MODE	instMode,		/* inst cache mode */
    CACHE_MODE	dataMode		/* data cache mode */
    )
    {
#if _ARCH_MULTIPLE_CACHELIB
    /* BSP selection of cache library */

    return ((STATUS) (sysCacheLibInit == NULL) ? ERROR :
	    (*sysCacheLibInit) (instMode, dataMode));
#else
    /* Single cache library for arch */

    return (cacheArchLibInit (instMode, dataMode));
#endif	/* _ARCH_MULTIPLE_CACHELIB */
    }

/**************************************************************************
*
* cacheEnable - enable the specified cache
*
* This routine invalidates the cache tags and enables the instruction or 
* data cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheEnable
    (
    CACHE_TYPE	cache		/* cache to enable */
    )
    {
    return ((cacheLib.enableRtn == NULL) ? ERROR : 
            (cacheLib.enableRtn) (cache));
    }

/**************************************************************************
*
* cacheDisable - disable the specified cache
*
* This routine flushes the cache and disables the instruction or data cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheDisable
    (
    CACHE_TYPE	cache		/* cache to disable */
    )
    {
    return ((cacheLib.disableRtn == NULL) ? ERROR : 
            (cacheLib.disableRtn) (cache));
    }

/**************************************************************************
*
* cacheLock - lock all or part of a specified cache
*
* This routine locks all (global) or some (local) entries in the specified 
* cache.  Cache locking is useful in real-time systems.  Not all caches can 
* perform locking.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheLock
    (
    CACHE_TYPE	cache,		/* cache to lock */
    void *	address,	/* virtual address */
    size_t	bytes		/* number of bytes to lock */
    )
    {
    return ((cacheLib.lockRtn == NULL) ? ERROR : 
            (cacheLib.lockRtn) (cache, address, bytes));
    }

/**************************************************************************
*
* cacheUnlock - unlock all or part of a specified cache
*
* This routine unlocks all (global) or some (local) entries in the 
* specified cache.  Not all caches can perform unlocking.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheUnlock
    (
    CACHE_TYPE	cache,		/* cache to unlock */
    void *	address,	/* virtual address */
    size_t	bytes		/* number of bytes to unlock */
    )
    {
    return ((cacheLib.unlockRtn == NULL) ? ERROR : 
            (cacheLib.unlockRtn) (cache, address, bytes));
    }

/**************************************************************************
*
* cacheFlush - flush all or some of a specified cache
*
* This routine flushes (writes to memory) all or some of the entries in
* the specified cache.  Depending on the cache design, this operation may
* also invalidate the cache tags.  For write-through caches, no work needs
* to be done since RAM already matches the cached entries.  Note that
* write buffers on the chip may need to be flushed to complete the flush.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheFlush
    (
    CACHE_TYPE	cache,		/* cache to flush */
    void *	address,	/* virtual address */
    size_t	bytes		/* number of bytes to flush */
    )
    {
    return ((cacheLib.flushRtn == NULL) ? OK : 
            (cacheLib.flushRtn) (cache, address, bytes));
    }

/**************************************************************************
*
* cacheInvalidate - invalidate all or some of a specified cache
*
* This routine invalidates all or some of the entries in the
* specified cache.  Depending on the cache design, the invalidation
* may be similar to the flush, or one may invalidate the tags directly.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheInvalidate
    (
    CACHE_TYPE	cache,		/* cache to invalidate */
    void *	address,	/* virtual address */
    size_t	bytes		/* number of bytes to invalidate */
    )
    {
    return ((cacheLib.invalidateRtn == NULL) ? OK : 
            (cacheLib.invalidateRtn) (cache, address, bytes));
    }

/**************************************************************************
*
* cacheClear - clear all or some entries from a cache
*
* This routine flushes and invalidates all or some entries in the
* specified cache.
*
* RETURNS: OK, or ERROR if the cache type is invalid or the cache control
* is not supported.
*/

STATUS cacheClear
    (
    CACHE_TYPE	cache,		/* cache to clear */
    void *	address,	/* virtual address */
    size_t	bytes		/* number of bytes to clear */
    )
    {
    return ((cacheLib.clearRtn == NULL) ? ERROR : 
            (cacheLib.clearRtn) (cache, address, bytes));
    }

/**************************************************************************
*
* cachePipeFlush - flush processor write buffers to memory
*
* This routine forces the processor output buffers to write their contents 
* to RAM.  A cache flush may have forced its data into the write buffers, 
* then the buffers need to be flushed to RAM to maintain coherency.
*
* RETURNS: OK, or ERROR if the cache control is not supported.
*/

STATUS cachePipeFlush (void)
    {
    return ((cacheLib.pipeFlushRtn == NULL) ? OK :
            (cacheLib.pipeFlushRtn) ());
    }

/**************************************************************************
*
* cacheTextUpdate - synchronize the instruction and data caches
*
* This routine flushes the data cache, then invalidates the instruction 
* cache.  This operation forces the instruction cache to fetch code that 
* may have been created via the data path.
*
* RETURNS: OK, or ERROR if the cache control is not supported.
*/

STATUS cacheTextUpdate 
    (
    void * address,		/* virtual address */
    size_t bytes		/* number of bytes to sync */
    )
    {
    /* if address not valid, return */
    if ((UINT32) address == NONE)
	return ERROR;

    return ((cacheLib.textUpdateRtn == NULL) ? OK :
            (cacheLib.textUpdateRtn) (address, bytes));
    }

/**************************************************************************
*
* cacheDmaMalloc - allocate a cache-safe buffer for DMA devices and drivers
*
* This routine returns a pointer to a section of memory that will not 
* experience any cache coherency problems.  Function pointers in the 
* CACHE_FUNCS structure provide access to DMA support routines.
*
* RETURNS: A pointer to the cache-safe buffer, or NULL.
*/

void * cacheDmaMalloc 
    (
    size_t bytes		/* number of bytes to allocate */
    )
    {
    return ((cacheDmaMallocRtn == NULL) ?
	    malloc (bytes) : ((void *) (cacheDmaMallocRtn) (bytes)));
    }

/**************************************************************************
*
* cacheDmaFree - free the buffer acquired with cacheDmaMalloc()
*
* This routine frees the buffer returned by cacheDmaMalloc().
*
* RETURNS: OK, or ERROR if the cache control is not supported.
*/

STATUS cacheDmaFree 
    (
    void * pBuf			/* pointer to malloc/free buffer */
    )
    {
    return ((cacheDmaFreeRtn == NULL) ?
	    free (pBuf), OK : (cacheDmaFreeRtn) (pBuf));
    }

/**************************************************************************
*
* cacheFuncsSet - set cache functions according to configuration
*
* This routine initializes the various CACHE_FUNCS structures.
*
* NOMANUAL
*/

void cacheFuncsSet (void)
    {
    if ((cacheDataEnabled == FALSE) || 
	(cacheDataMode & CACHE_SNOOP_ENABLE))
	{
	/* no cache or fully coherent cache */

	cacheUserFuncs = cacheNullFuncs;
	cacheDmaFuncs  = cacheNullFuncs;

#if	(CPU == MC68060)
	cacheDmaMallocRtn = cacheLib.dmaMallocRtn;
	cacheDmaFreeRtn	  = cacheLib.dmaFreeRtn;;
#else
	cacheDmaMallocRtn = NULL;
	cacheDmaFreeRtn	  = NULL;
#endif	/* (CPU == MC68060) */
	}
    else
	{
	/* cache is not fully coherent */

	cacheUserFuncs.invalidateRtn = cacheLib.invalidateRtn;

	if (cacheDataMode & CACHE_WRITETHROUGH)
	    cacheUserFuncs.flushRtn = NULL;
	else
	    cacheUserFuncs.flushRtn = cacheLib.flushRtn;

#if	(CPU_FAMILY==I80X86)	/* for write-back external cache */
	cacheUserFuncs.flushRtn = cacheLib.flushRtn;
#endif


	if (cacheMmuAvailable)
	    {
	    /* MMU available - dma buffers will be made non-cacheable */

	    cacheDmaFuncs.flushRtn	= NULL;
	    cacheDmaFuncs.invalidateRtn	= NULL;
	    cacheDmaFuncs.virtToPhysRtn	= cacheLib.dmaVirtToPhysRtn;
	    cacheDmaFuncs.physToVirtRtn	= cacheLib.dmaPhysToVirtRtn;

	    cacheDmaMallocRtn		= cacheLib.dmaMallocRtn;
	    cacheDmaFreeRtn		= cacheLib.dmaFreeRtn;
	    }
	else
	    {
	    /* no MMU - dma buffers are same as regular buffers */

	    cacheDmaFuncs = cacheUserFuncs;

	    cacheDmaMallocRtn = NULL;
	    cacheDmaFreeRtn   = NULL;
	    }
	}
    }

/**************************************************************************
* 
* cacheDrvFlush - flush the data cache for drivers
*
* This routine flushes the data cache entries using the function pointer 
* from the specified set.
* 
* RETURNS: OK, or ERROR if the cache control is not supported.
*/

STATUS cacheDrvFlush 
    (
    CACHE_FUNCS * pFuncs,	/* pointer to CACHE_FUNCS */
    void * address,		/* virtual address */
    size_t bytes		/* number of bytes to flush */
    )
    {
    return (CACHE_DRV_FLUSH (pFuncs, address, bytes));
    }

/**************************************************************************
* 
* cacheDrvInvalidate - invalidate data cache for drivers
* 
* This routine invalidates the data cache entries using the function pointer 
* from the specified set.
*
* RETURNS: OK, or ERROR if the cache control is not supported.
*/

STATUS cacheDrvInvalidate 
    (
    CACHE_FUNCS * pFuncs,	/* pointer to CACHE_FUNCS */
    void * address,		/* virtual address */
    size_t bytes		/* no. of bytes to invalidate */
    )
    {
    return (CACHE_DRV_INVALIDATE (pFuncs, address, bytes));
    }

/**************************************************************************
* 
* cacheDrvVirtToPhys - translate a virtual address for drivers
* 
* This routine performs a virtual-to-physical address translation using the 
* function pointer from the specified set.
* 
* RETURNS: The physical address translation of a virtual address argument.
*/

void * cacheDrvVirtToPhys 
    (
    CACHE_FUNCS * pFuncs,	/* pointer to CACHE_FUNCS */
    void * address		/* virtual address */
    )
    {
    return (CACHE_DRV_VIRT_TO_PHYS (pFuncs, address));
    }

/**************************************************************************
* 
* cacheDrvPhysToVirt - translate a physical address for drivers
* 
* This routine performs a physical-to-virtual address translation using the 
* function pointer from the specified set.
* 
* RETURNS: The virtual address that maps to the physical address argument.
*/

void * cacheDrvPhysToVirt 
    (
    CACHE_FUNCS * pFuncs,	/* pointer to CACHE_FUNCS */
    void * address		/* physical address */
    )
    {
    return (CACHE_DRV_PHYS_TO_VIRT (pFuncs, address));
    }
