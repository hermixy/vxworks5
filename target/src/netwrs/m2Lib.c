/* m2Lib.c - MIB-II API library for SNMP agents */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,25oct00,ham  doc: cleanup for vxWorks AE 1.0.
01e,25jan95,jdi  doc cleanup.
01d,11nov94,rhp  minor correction to library man page.
01c,10nov94,rhp  edited man pages.
01b,15feb94,jag  added MIB-II library documentation.
01a,05jan94,elh  written
*/

/*
DESCRIPTION
This library provides Management Information Base (MIB-II, defined in
RFC 1213) services for applications wishing to have access to MIB
parameters.

To use this feature, include the following component:
INCLUDE_MIB2_ALL

There are no specific provisions for MIB-I: all services
are provided at the MIB-II level.  Applications that use this library
for MIB-I must hide the MIB-II extensions from higher level protocols.
The library accesses all the MIB-II parameters, and presents them to
the application in data structures based on the MIB-II specifications.

The routines provided by the VxWorks MIB-II library are separated
into groups that follow the MIB-II definition.  Each supported group
has its own interface library:
.iP m2SysLib 15 3
systems group
.iP m2IfLib
interface group
.iP m2IpLib
IP group (includes AT)
.iP m2IcmpLib
ICMP group 
.iP m2TcpLib
TCP group 
.iP m2UdpLib
UDP group 
.LP

MIB-II retains the AT group for backward compatibility, but
includes its functionality in the IP group.  The EGP and SNMP groups
are not supported by this interface.  The variables in each group have
been subdivided into two types: table entries and scalar variables.
Each type has a pair of routines that get and set the variables.

USING THIS LIBRARY
There are four types of operations on each group:

  - initializing the group
  - getting variables and table entries
  - setting variables and table entries
  - deleting the group
  
Only the groups that are to be used need be initialized.  There is one
exception: to use the IP group, the interface group must also be
initialized.  Applications that require MIB-II support from all groups can
initialize all groups at once by calling the m2Init().  All MIB-II group
services can be disabled by calling m2Delete().  Applications that need
access only to a particular set of groups need only call the
initialization routines of the desired groups.

To read the scalar variables for each group, call one of the following
routines:

    m2SysGroupInfoGet()
    m2IfGroupInfoGet()
    m2IpGroupInfoGet()
    m2IcmpGroupInfoGet()
    m2TcpGroupInfoGet()
    m2UdpGroupInfoGet()

The input parameter to the routine is always a pointer to a structure
specific to the associated group.  The scalar group structures follow
the naming convention "M2_<groupname>".  The get routines fill in the
input structure with the values of all the group variables.

The scalar variables can also be set to a user supplied value. Not all
groups permit setting variables, as specified by the MIB-II definition.  
The following group routines allow setting variables:

    m2SysGroupInfoSet()
    m2IpGroupInfoSet()

The input parameters to the variable-set routines are a bit field
that specifies which variables to set, and a group structure.  The
structure is the same structure type used in the get operation.
Applications need set only the structure fields corresponding to the
bits that are set in the bit field.

The MIB-II table routines read one entry at a time.  Each MIB-II group
that has tables has a get routine for each table.  The following
table-get routines are available:

    m2IfTblEntryGet()
    m2IpAddrTblEntryGet()
    m2IpAtransTblEntryGet()
    m2IpRouteTblEntryGet()
    m2TcpConnEntryGet()
    m2UdpTblEntryGet()

The input parameters are a pointer to a table entry structure, and a
flag value specifying one of two types of table search.  Each table
entry is a structure, where the struct type name follows this naming
convention: "M2_<Groupname><Tablename>TBL".  The MIB-II RFC specifies an
index that identifies a table entry.  Each get request must specify an
index value.  To retrieve the first entry in a table, set all the
index fields of the table-entry structure to zero, and use the search
parameter M2_NEXT_VALUE.  To retrieve subsequent entries, pass the
index returned from the previous invocation, incremented to the next
possible lexicographical entry.  The search field can only be set to
the constants M2_NEXT_VALUE or M2_EXACT_VALUE:
.iP M2_NEXT_VALUE
retrieves a table entry that is either identical to
the index value specified as input, or is the closest entry following
that value, in lexicographic order.
.iP M2_EXACT_VALUE
retrieves a table entry that exactly matches the index
specified in the input structure.
.LP

Some MIB-II table entries can be added, modified and deleted.
Routines to manipulate such entries are described in the manual pages
for individual groups.

All the IP network addresses that are exchanged with the MIB-II
library must be in host-byte order; use ntohl() to convert addresses
before calling these library routines.

The following example shows how to initialize the MIB-II library for
all groups.

.CS
    extern FUNCPTR myTrapGenerator;
    extern void *  myTrapGeneratorArg;

    M2_OBJECTID mySysObjectId = { 8, {1,3,6,1,4,1,731,1} };

    if (m2Init ("VxWorks 5.1.1 MIB-II library (sysDescr)",
	        "support@wrs.com (sysContact)",
	        "1010 Atlantic Avenue Alameda, California 94501 (sysLocation)",
		 &mySysObjectId,
		 myTrapGenerator,
		 myTrapGeneratorArg,
		 0) == OK)
	/@ MIB-II groups initialized successfully @/
.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO: m2IfLib, m2IpLib, m2IcmpLib, m2UdpLib, m2TcpLib, m2SysLib 
*/

/* includes */

#include <vxWorks.h>
#include "m2Lib.h"


/******************************************************************************
*
* m2Init - initialize the SNMP MIB-2 library 
*
* This routine initializes the MIB-2 library by calling the initialization
* routines for each MIB-2 group.  The parameters <pMib2SysDescr>
* <pMib2SysContact>, <pMib2SysLocation>, and <pMib2SysObjectId> are passed
* directly to m2SysInit();  <pTrapRtn> and <pTrapArg> are passed directly to
* m2IfInit(); and <maxRouteTableSize> is passed to m2IpInit().
*
* RETURNS: OK if successful, otherwise ERROR.
*
* SEE ALSO:
* m2SysInit(), m2TcpInit(), m2UdpInit(), m2IcmpInit(), m2IfInit(), m2IpInit()
*/

STATUS m2Init 
    (
    char *		pMib2SysDescr,		/* sysDescr */
    char *		pMib2SysContact,	/* sysContact */
    char *		pMib2SysLocation,	/* sysLocation */
    M2_OBJECTID	*	pMib2SysObjectId, 	/* sysObjectID */
    FUNCPTR 		pTrapRtn, 		/* link up/down -trap routine */
    void * 		pTrapArg,		/* trap routine arg */
    int			maxRouteTableSize 	/* max size of routing table */ 
    )
    {

    /* Call the initialization routine for each group in MIB-2 */

    if ((m2SysInit (pMib2SysDescr, pMib2SysContact, pMib2SysLocation,
		    pMib2SysObjectId) == ERROR) ||
    	(m2IfInit (pTrapRtn, pTrapArg) == ERROR) ||
        (m2IpInit (maxRouteTableSize) == ERROR) || 
    	(m2TcpInit () == ERROR) ||
    	(m2IcmpInit () == ERROR) ||
	(m2UdpInit () == ERROR))
	{
	m2Delete ();			
	return (ERROR);			/* initialization failed */
	}

    return (OK);
    }

/******************************************************************************
*
* m2Delete - delete all the MIB-II library groups
*
* This routine cleans up the state associated with the MIB-II library.
*
* RETURNS: OK (always).
*
* SEE ALSO: m2SysDelete(), m2TcpDelete(), m2UdpDelete(), m2IcmpDelete(),
* m2IfDelete(), m2IpDelete()
*/
STATUS m2Delete (void)
    {
    m2SysDelete ();	
    m2IfDelete ();	
    m2IpDelete ();
    m2TcpDelete ();
    m2IcmpDelete ();
    m2UdpDelete ();
    return (OK);
    }
