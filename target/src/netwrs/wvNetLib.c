/* wvNetLib.c - WindView for Networking Interface Library */

/* Copyright 1990 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,10may02,kbw  making man page edits
01c,25oct00,ham  doc: cleanup for vxWorks AE 1.0.
01b,07jun00,ham  fixed a compilation warning.
01a,12jan00,spm  written
*/

/*
DESCRIPTION
This library provides the user interface to the network-related events
for the WindView system visualization tool. These events are divided into
two WindView classes. The NET_CORE_EVENT class indicates events directly
related to data transfer. All other events (such as memory allocation and
API routines) use the NET_AUX_EVENT class. Within each class, events are
assigned one of eight priority levels. The four highest priority levels 
(EMERGENCY, ALERT, CRITICAL, and ERROR) indicate the occurrence of errors 
and the remaining four (WARNING, NOTICE, INFO, and VERBOSE) provide 
progressively more detailed information about the internal processing in 
the network stack. 

USER INTERFACE
If WindView support is included, the wvNetStart() and wvNetStop() routines 
will enable and disable event reporting for the network stack. The start
routine takes a single parameter specifying the minimum priority level for
all network components. That setting may be modified with the wvNetLevelAdd() 
and wvNetLevelRemove() routines. Individual events may be included or removed
with the wvNetEventEnable() and wvNetDisable() routines.

The wvNetAddressFilterSet() and wvNetPortFilterSet() routines provide further 
screening for some events.

INTERNAL
The WindView monitor executing on a VxWorks target transmits events to the
WindView parser on the host based on the event class. Increasing the number
of classes beyond two would allow more precise control over event generation 
and reduce the load on the VxWorks target, but the total number of available 
classes is limited. 

Events are generated within each source code module using macros defined
in the wvNetLib.h include file. The WV_BLOCK_START macro determines whether 
WindView is active and verifies that the given event class has been selected. 
All WindView related processing must be enclosed between a WV_BLOCK_START 
and WV_BLOCK_END pair.

The WV_NET_EVENT_TEST macro immediately follows the start of the WindView
event block. It verifies that an individual event is selected by testing
the contents of the event selection map. The event map contains bitmaps for 
all events within each of the eight priority levels. It is modified by the 
user interface routines as the settings are changed. Currently, no priority 
level contains more than 64 events. If that limit is exceeded, the EVENT_MASK 
structure must be expanded. Local variables indicate the number of events 
currently defined for each priority level.

If an event is active, the WV_NET_FILTER_TEST macro verifies than any 
remaining conditions are satisfied.

If an event fulfills all the conditions, the event identifier is constructed
with the WV_NET_EVENT_SET or WV_NET_MARKER_SET macros and the event
is logged with the evtLogOInt() routine. The event identifier encodes
the related system component (currently 0x30) and the source code module, 
priority level, and data transfer direction for all events.

To use this feature, include the following component:
INCLUDE_WVNET

INCLUDE FILES:

SEE ALSO:
.I "WindView for Tornado User's Guide"
*/

/* includes */

#include "vxWorks.h"
#include "wvLib.h"
#include "wvNetLib.h"

/* globals */

EVENT_MASK wvNetEventMap [8]; /* Bitmasks for all event priorities. */

BOOL wvNetAddressFilterTest (int, int, ULONG, ULONG);
BOOL wvNetPortFilterTest (int, USHORT, USHORT);

/* external variables */

IMPORT EVENT_MASK * pWvNetEventMap;
IMPORT u_long inet_addr (char * inetString);

/* locals */

LOCAL int maxEmergencyOffset = 45;       /*                                 */
LOCAL int maxAlertOffset = 8;            /* The maximum offset value for    */
LOCAL int maxCriticalOffset = 34;        /* each priority level is one      */
LOCAL int maxErrorOffset = 47;           /* less than the number of events. */
LOCAL int maxWarningOffset = 19;         /* If it exceeds 63, the bitmap    */
LOCAL int maxNoticeOffset = 22;          /* size must be increased.         */
LOCAL int maxInfoOffset = 56;
LOCAL int maxVerboseOffset = 57;

LOCAL BOOL wvNetInputSrcAddrFlag = FALSE;
LOCAL BOOL wvNetInputDstAddrFlag = FALSE;

LOCAL int wvNetInputSrcAddr;
LOCAL int wvNetInputSrcMask;
LOCAL int wvNetInputDstAddr;
LOCAL int wvNetInputDstMask;

LOCAL BOOL wvNetOutputSrcAddrFlag = FALSE;
LOCAL BOOL wvNetOutputDstAddrFlag = FALSE;

LOCAL int wvNetOutputSrcAddr;
LOCAL int wvNetOutputSrcMask;
LOCAL int wvNetOutputDstAddr;
LOCAL int wvNetOutputDstMask;

LOCAL BOOL wvNetInputSrcPortFlag;
LOCAL int wvNetInputSrcPort;

LOCAL BOOL wvNetInputDstPortFlag;
LOCAL int wvNetInputDstPort;

LOCAL BOOL wvNetOutputSrcPortFlag;
LOCAL int wvNetOutputSrcPort;

LOCAL BOOL wvNetOutputDstPortFlag;
LOCAL int wvNetOutputDstPort;

/* forward declarations */

/*******************************************************************************
*
* wvNetInit - stub routine for linker
*
* This routine is called during system startup so that the global variables
* storing the WindView settings for the networking instrumentation are 
* available. It also initializes the event bitmaps so that all events
* are reported when logging begins. Event selection can be customized
* with the appropriate library routines.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void wvNetInit (void)
    {
    int index;
    int loop;

    /* 
     * Enable all events for each level. Because of the array indexing,
     * the index into the event map is one less than the value assigned
     * to the priority level.
     */

    for (index = WV_NET_EMERGENCY - 1; index < WV_NET_VERBOSE; index++)
        {
        /* 
         * Currently, no priority level uses more than 64 events. This
         * inner loop must be increased if that limit is exceeded.
         */

        for (loop = 0; loop < 8; loop++)
            wvNetEventMap [index].bitmap [loop] = 255;
        }

    /* Provide access to the event map from instrumented modules. */

    pWvNetEventMap = wvNetEventMap;

    _func_wvNetAddressFilterTest = wvNetAddressFilterTest;
    _func_wvNetPortFilterTest = (FUNCPTR)wvNetPortFilterTest;

    return;
    } 

/*******************************************************************************
*
* wvNetEnable - begin reporting network events to WindView
*
* This routine activates WindView event reporting for network components,
* after disabling all events with a priority less than <level>. The
* default value (or a <level> of WV_NET_VERBOSE) will not disable
* any additional events. The available priority values are:
*
*     WV_NET_EMERGENCY (1)
*     WV_NET_ALERT (2)
*     WV_NET_CRITICAL (3)
*     WV_NET_ERROR (4)
*     WV_NET_WARNING (5)
*     WV_NET_NOTICE (6)
*     WV_NET_INFO (7)
*     WV_NET_VERBOSE (8)
* 
* If an event is not explicitly disabled by the priority level, it uses the 
* current event selection map and class settings. The initial values enable
* all events of both classes. 
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void wvNetEnable
    (
    int priority    /* minimum priority, or 0 for default of WV_NET_VERBOSE */
    )
    {
    int index;
    int loop;

    /*
     * Because of the array indexing, the <priority> parameter provides the
     * starting offset into the event map for the lower priority events. 
     * Events with priorities greater than or equal to the given level
     * remain enabled. All other events are disabled.
     */

    if (priority == 0)	/* Set index so no events are disabled by default.*/
        priority = WV_NET_VERBOSE;

    for (index = priority; index < WV_NET_VERBOSE; index++)
        {
        /* 
         * Set event map so WV_NET_EVENT_TEST macro will reject events
         * with lower priority (currently up to 64 per level). 
         */

        for (loop = 0; loop < 8; loop++)
            wvNetEventMap [index].bitmap [loop] = 0;
        }

    /*
     * Begin reporting network events from both the primary and auxiliary
     * classes. Also enable the class 1 (context switching) events,
     * in case they are not already active.
     */

    WV_EVTCLASS_SET (WV_NET_CORE_CLASS | WV_NET_AUX_CLASS);

    return;
    }

/*******************************************************************************
*
* wvNetDisable - end reporting of network events to WindView
*
* This routine stops WindView event reporting for all network components.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void wvNetDisable (void)
    {
    /* Stop reporting events from either class. */

    WV_EVTCLASS_UNSET (WV_NET_CORE_CLASS | WV_NET_AUX_CLASS);

    return;
    }

/*******************************************************************************
*
* wvNetLevelAdd - enable network events with specific priority level
*
* This routine changes the event selection map to allow reporting of any 
* events with priority equal to <level>. It will override current event 
* selections for the given priority, but has no effect on settings for 
* events with higher or lower priorities. The available priority values 
* are:
*
*     WV_NET_EMERGENCY (1)
*     WV_NET_ALERT (2)
*     WV_NET_CRITICAL (3)
*     WV_NET_ERROR (4)
*     WV_NET_WARNING (5)
*     WV_NET_NOTICE (6)
*     WV_NET_INFO (7)
*     WV_NET_VERBOSE (8)
* 
* Events are only reported based on the current WindView class setting. The 
* initial (default) setting includes networking events from both classes.
*
* RETURNS: OK, or ERROR for unknown event level.
*
* ERRNO: N/A
*/

STATUS wvNetLevelAdd
    (
    int priority 	/* priority level to enable */
    )
    {
    int index;
    int loop;

    if (priority < WV_NET_EMERGENCY || priority > WV_NET_VERBOSE)
        return (ERROR);

    /* 
     * Because of array indexing, the index into the event map is one
     * less than the value assigned to the priority level.
     */

    index = priority - 1;

    /*
     * Set event map so WV_NET_EVENT_TEST macro will accept any events 
     * with selected priority (currently up to 64 per level).
     */

    for (loop = 0; loop < 8; loop++)
         wvNetEventMap [index].bitmap [loop] = 255;

    return (OK);
    }

/*******************************************************************************
*
* wvNetLevelRemove - disable network events with specific priority level
*
* This routine changes the event selection map to prevent reporting of any 
* events with priority equal to <level>. It will override the current event 
* selection for the given priority, but has no effect on settings for events 
* with higher or lower priorities. The available priority values are:
*
*     WV_NET_EMERGENCY (1)
*     WV_NET_ALERT (2)
*     WV_NET_CRITICAL (3)
*     WV_NET_ERROR (4)
*     WV_NET_WARNING (5)
*     WV_NET_NOTICE (6)
*     WV_NET_INFO (7)
*     WV_NET_VERBOSE (8)
* 
* Events are only reported based on the current WindView class setting. The
* initial (default) setting includes networking events from both classes.
*
* RETURNS: OK, or ERROR for unknown event level.
*
* ERRNO: N/A
*/

STATUS wvNetLevelRemove
    (
    int priority 	/* priority level to disable */
    )
    {
    int index;
    int loop;

    if (priority < WV_NET_EMERGENCY || priority > WV_NET_VERBOSE)
        return (ERROR);

    /* 
     * Because of array indexing, the index into the event map is one
     * less than the value assigned to the priority level.
     */

    index = priority - 1;

    /*
     * Set event map so WV_NET_EVENT_TEST macro will reject any events 
     * with selected priority (currently up to 64 per level).
     */

    for (loop = 0; loop < 8; loop++)
         wvNetEventMap [index].bitmap [loop] = 0;

    return (OK);
    }

/*******************************************************************************
*
* wvNetEventEnable - activate specific network events
*
* This routine allows reporting of a single event within the priority equal 
* to <level>. The activation is overridden if the setting for the entire 
* priority level changes. The available priority values are:
*
*     WV_NET_EMERGENCY (1)
*     WV_NET_ALERT (2)
*     WV_NET_CRITICAL (3)
*     WV_NET_ERROR (4)
*     WV_NET_WARNING (5)
*     WV_NET_NOTICE (6)
*     WV_NET_INFO (7)
*     WV_NET_VERBOSE (8)
* 
* Offset values for individual events are listed in the documentation.
*
* RETURNS: OK, or ERROR for unknown event.
*
* ERRNO: N/A
*/

STATUS wvNetEventEnable
    (
    int priority, 	/* priority level of event */
    int offset 		/* identifier within priority level */
    )
    {
    STATUS result = OK;
    int index;
    UCHAR mask;
    int byteOffset;
    int byteIndex;

    switch (priority)
        {
        case WV_NET_EMERGENCY:
            if (offset > maxEmergencyOffset)
                result = ERROR;
            break;
        case WV_NET_ALERT:
            if (offset > maxAlertOffset)
                result = ERROR;
            break;
        case WV_NET_CRITICAL:
            if (offset > maxCriticalOffset)
                result = ERROR;
            break;
        case WV_NET_ERROR:
            if (offset > maxErrorOffset)
                result = ERROR;
            break;
        case WV_NET_WARNING:
            if (offset > maxWarningOffset)
                result = ERROR;
            break;
        case WV_NET_NOTICE:
            if (offset > maxNoticeOffset)
                result = ERROR;
            break;
        case WV_NET_INFO:
            if (offset > maxInfoOffset)
                result = ERROR;
            break;
        case WV_NET_VERBOSE:
            if (offset > maxVerboseOffset)
                result = ERROR;
            break;
        default: 	/* Unknown priority level */
            result = ERROR;
            break;
        }

    /* 
     * Because of array indexing, the index into the event map is one
     * less than the value assigned to the priority level.
     */

    index = priority - 1;

    if (result == OK)
        {        
        byteOffset = offset / 8;

        /* 
         * Convert the offset to an array index based on the number of
         * bytes in the bitmap.
         */

        byteIndex = WVNET_MASKSIZE - 1 - byteOffset;

        /*
         * Set event map so WV_NET_EVENT_TEST macro will accept the
         * specified event.
         */

        mask = 1 << (offset % 8);

        wvNetEventMap [index].bitmap [byteIndex] |= mask;
        }

    return (result);
    }

/*******************************************************************************
*
* wvNetEventDisable - deactivate specific network events
*
* This routine prevents reporting of a single event within the priority equal 
* to <level>. The activation is overridden if the setting for the entire 
* priority level changes. The available priority values are:
*
*     WV_NET_EMERGENCY (1)
*     WV_NET_ALERT (2)
*     WV_NET_CRITICAL (3)
*     WV_NET_ERROR (4)
*     WV_NET_WARNING (5)
*     WV_NET_NOTICE (6)
*     WV_NET_INFO (7)
*     WV_NET_VERBOSE (8)
* 
* Offset values for individual events are listed in the documentation.
*
* RETURNS: OK, or ERROR for unknown event.
*
* ERRNO: N/A
*/

STATUS wvNetEventDisable
    (
    int priority, 	/* priority level of event */
    int offset 		/* identifier within priority level */
    )
    {
    STATUS result = OK;
    int index;
    UCHAR mask;
    int byteOffset;
    int byteIndex;

    switch (priority)
        {
        case WV_NET_EMERGENCY:
            if (offset > maxEmergencyOffset)
                result = ERROR;
            break;
        case WV_NET_ALERT:
            if (offset > maxAlertOffset)
                result = ERROR;
            break;
        case WV_NET_CRITICAL:
            if (offset > maxCriticalOffset)
                result = ERROR;
            break;
        case WV_NET_ERROR:
            if (offset > maxErrorOffset)
                result = ERROR;
            break;
        case WV_NET_WARNING:
            if (offset > maxWarningOffset)
                result = ERROR;
            break;
        case WV_NET_NOTICE:
            if (offset > maxNoticeOffset)
                result = ERROR;
            break;
        case WV_NET_INFO:
            if (offset > maxInfoOffset)
                result = ERROR;
            break;
        case WV_NET_VERBOSE:
            if (offset > maxVerboseOffset)
                result = ERROR;
            break;
        default: 	/* Unknown priority level */
            result = ERROR;
            break;
        }

    /* 
     * Because of array indexing, the index into the event map is one
     * less than the value assigned to the priority level.
     */

    index = priority - 1;

    if (result == OK)
        {        
        byteOffset = offset / 8;

        /* 
         * Convert the offset to an array index based on the number of
         * bytes in the bitmap.
         */

        byteIndex = WVNET_MASKSIZE - 1 - byteOffset;

        /*
         * Set event map so WV_NET_EVENT_TEST macro will reject the
         * specified event.
         */

        mask = ~ (1 << (offset % 8));

        wvNetEventMap [index].bitmap [byteIndex] &= mask;
        }

    return (result);
    }

/*******************************************************************************
*
* wvNetAddressFilterSet - specify an address filter for events
*
* This routine activates an additional test that disables certain events
* that do not match the specified IP address. The <pAddress> parameter
* provides the test value in dotted-decimal format. The <type> parameter
* indicates whether that address is compared against the source or
* destination values, and the <direction> value identifies whether the 
* <type> is interpreted from the perspective of incoming or outgoing traffic.
* The <pMask> parameter provides a network mask to support testing for a
* group of events.
*
* RETURNS: OK if filter set, or ERROR otherwise.
*
* ERRNO: N/A
*
*/

STATUS wvNetAddressFilterSet
    (
    char * 	pAddress, 	/* target address for event comparisons */
    char * 	pMask, 		/* mask value applied to data fields */
    int 	type, 		/* 0 for source, 1 for destination */ 
    int 	direction 	/* 0 for input, 1 for output */
    )
    {
    int targetAddr;
    int targetMask;
    STATUS result = OK;

    targetAddr = inet_addr (pAddress);
    targetMask = inet_addr (pMask);

    if (targetAddr == ERROR)   /* targetMask of 255.255.255.255 is OK. */
        return (ERROR);

    if (direction == 0 && type == 0)
        {
        wvNetInputSrcAddr = targetAddr;
        wvNetInputSrcMask = targetMask;
        wvNetInputSrcAddrFlag = TRUE;
        }
    else if (direction == 0 && type == 1)
        {
        wvNetInputDstAddr = targetAddr;
        wvNetInputDstMask = targetMask;
        wvNetInputDstAddrFlag = TRUE;
        }
    else if (direction == 1 && type == 0)
        {
        wvNetOutputSrcAddr = targetAddr;
        wvNetOutputSrcMask = targetMask;
        wvNetOutputSrcAddrFlag = TRUE;
        }
    else if (direction == 1 && type == 1)
        {
        wvNetOutputDstAddr = targetAddr;
        wvNetOutputDstMask = targetMask;
        wvNetOutputDstAddrFlag = TRUE;
        } 
    else
        result = ERROR;

    return (result);
    }

/*******************************************************************************
*
* wvNetAddressFilterClear - remove the address filter for events
*
* This routine removes any active address filter test indicated by
* the <type> and <direction> parameters used to enable it. Affected
* events will be reported unconditionally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void wvNetAddressFilterClear
    (
    int 	type, 		/* 0 for source, 1 for destination */ 
    int 	direction 	/* 0 for input, 1 for output */
    )
    {
    if (direction == 0 && type == 0)
        wvNetInputSrcAddrFlag = FALSE;
    else if (direction == 0 && type == 1)
        wvNetInputDstAddrFlag = FALSE;
    else if (direction == 1 && type == 0)
        wvNetOutputSrcAddrFlag = FALSE;
    else if (direction == 1 && type == 1)
        wvNetOutputDstAddrFlag = FALSE;

    return;
    }

/*******************************************************************************
*
* wvNetPortFilterSet - specify an address filter for events
*
* This routine activates an additional filter, which disables certain events 
* that do not match the specified port value. The <port> parameter provides 
* the test value and the <type> parameter indicates whether that value is 
* compared against the source or destination fields. The <direction> setting 
* identifies whether the <type> is interpreted from the perspective of 
* incoming or outgoing traffic.
*
* RETURNS: OK if filter set, or ERROR otherwise.
*
* ERRNO: N/A
*/

STATUS wvNetPortFilterSet
    (
    int 	port, 		/* target port for event comparisons */
    int 	type, 		/* 0 for source, 1 for destination */ 
    int 	direction 	/* 0 for input, 1 for output */
    )
    {
    STATUS result = OK;

    if (direction == 0 && type == 0)
        {
        wvNetInputSrcPort = port;
        wvNetInputSrcPortFlag = TRUE;
        }
    else if (direction == 0 && type == 1)
        {
        wvNetInputDstPort = port;
        wvNetInputDstPortFlag = TRUE;
        }
    else if (direction == 1 && type == 0)
        {
        wvNetOutputSrcPort = port;
        wvNetOutputSrcPortFlag = TRUE;
        }
    else if (direction == 1 && type == 1)
        {
        wvNetOutputDstPort = port;
        wvNetOutputDstPortFlag = TRUE;
        } 
    else
        result = ERROR;

    return (result);
    }

/*******************************************************************************
*
* wvNetPortFilterClear - remove the port number filter for events
*
* This routine removes any active port filter test indicated by
* the <type> and <direction> parameters used to enable it. Affected
* events will be reported unconditionally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void wvNetPortFilterClear
    (
    int 	type, 		/* 0 for source, 1 for destination */ 
    int 	direction 	/* 0 for input, 1 for output */
    )
    {
    if (direction == 0 && type == 0)
        wvNetInputSrcPortFlag = FALSE;
    else if (direction == 0 && type == 1)
        wvNetInputDstPortFlag = FALSE;
    else if (direction == 1 && type == 0)
        wvNetOutputSrcPortFlag = FALSE;
    else if (direction == 1 && type == 1)
        wvNetOutputDstPortFlag = FALSE;

    return;
    }

/*******************************************************************************
*
* wvNetAddressFilterTest - compare the given addresses with filter values
*
* This routine is invoked by the WV_NET_ADDR_FILTER_TEST macro within
* an event block. It returns TRUE if no filters are in effect or if the
* target values for each filter match the given addresses.
*
* RETURNS: TRUE if filter passes, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

BOOL wvNetAddressFilterTest
    (
    int filterType, 	/* 0 for input filter or 1 for output filter */
    int targetType,	/* 1 for dst only, 2 for src only, or 3 for both */
    ULONG srcAddr, 	/* current source address for event block */
    ULONG dstAddr	/* current destination address for event block */
    )
    {
    BOOL result = TRUE;

    if (filterType == 0) 	/* Compare given addresses to input values. */
        {
        if (wvNetInputSrcAddrFlag && targetType != 1)
            result = ((srcAddr & wvNetInputSrcMask) == wvNetInputSrcAddr);

        /* Only test the destination address if any source test passed. */

        if (result && wvNetInputDstAddrFlag && targetType != 2)
            result = ((dstAddr & wvNetInputDstMask) == wvNetInputDstAddr);
        }

    if (filterType == 1)       /* Compare given addresses to output values. */
        {
        if (wvNetOutputSrcAddrFlag && targetType != 1)
            result = ((srcAddr & wvNetOutputSrcMask) == wvNetOutputSrcAddr);

        /* Only test the destination address if any source test passed. */

        if (result && wvNetOutputDstAddrFlag && targetType != 2)
            result = ((dstAddr & wvNetOutputDstMask) == wvNetOutputDstAddr);
        }

    /* if (!result)    /@ XXX - for testing, include if desired. @/
        logMsg ("Address filter ignoring event.\n", 0, 0, 0, 0, 0, 0); */

    return (result);
    }

/*******************************************************************************
*
* wvNetPortFilterTest - compare the given port numbers with filter values
*
* This routine is invoked by the WV_NET_PORT_FILTER_TEST macro within
* an event block. It returns TRUE if no filters are in effect or if the
* target values for each filter match the given port numbers.
*
* INTERNAL
* Unlike the address filter tests, both the source and destination ports
* are always available for the filtered events, so the <targetType> 
* parameter is not needed.
*
* RETURNS: TRUE if filter passes, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

BOOL wvNetPortFilterTest
    (
    int filterType, 	/* 0 for input filter or 1 for output filter */
    USHORT srcPort, 	/* current source port for event block */
    USHORT dstPort	/* current destination port for event block */
    )
    {
    BOOL result = TRUE;

    if (filterType == 0) 	/* Compare given ports to input values. */
        {
        if (wvNetInputSrcPortFlag)
            result = (srcPort == wvNetInputSrcPort);

        /* Only test the destination port if any source test passed. */

        if (result && wvNetInputDstPortFlag)
            result = (dstPort == wvNetInputDstPort);
        }

    if (filterType == 1)       /* Compare given addresses to output values. */
        {
        if (wvNetOutputSrcPortFlag)
            result = (srcPort == wvNetOutputSrcPort);

        /* Only test the destination address if any source test passed. */

        if (result && wvNetOutputDstPortFlag)
            result = (dstPort == wvNetOutputDstPort);
        }

    return (result);
    }

#if 0
/*******************************************************************************
*
* wvNetEventTest - check if event is enabled
*
* This routine duplicates the WV_NET_EVENT_TEST macro. It checks the bitmap
* for an event to determine if it should be reported to the host-side of the 
* WindView tool. It is never called, and is present only to illustrate the 
* macro behavior.
*
* RETURNS: TRUE if event is selected, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

BOOL wvNetEventTest
    (
    int priority, 	/* priority level of event */
    int offset 		/* identifier within priority level */
    )
    {
    BOOL result;
    int index;
    UCHAR mask;
    int byteOffset;
    int byteIndex;

    /* 
     * Because of array indexing, the index into the event map is one
     * less than the value assigned to the priority level.
     */

    index = priority - 1;

    byteOffset = offset / 8;

    /* 
     * Convert the offset to an array index based on the number of
     * bytes in the bitmap.
     */

    byteIndex = WVNET_MASKSIZE - 1 - byteOffset;

    /* Check the flag in the bitmap for the specified event. */

    mask = 1 << (offset % 8);

    result = (wvNetEventMap [index].bitmap [byteIndex] & mask);

    return (result);
    }
#endif
