/* cplusCore.cpp - core C++ run-time support */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02d,06nov01,sn   removed obsolete vec_new/vec_delete
02c,30mar01,sn   fixed to build with diab
02b,03mar98,sn   renamed new_handler to __new_handler to conform with the
                 GNU new.cc implementation.
02a,16jan98,sn   remove new/delete (henceforth provided in 
                 target/src/cplus/libgcc)
01d,19jun95,srh  rename op new/delete intf for g++.
01c,19jun95,srh  cleanup picky warnings from g++
01b,17jun95,srh  fixed declaration of VOIDFUNCPTR_ARGS
01a,19may95,srh  cobbled together from WindC++
*/

/*
DESCRIPTION
TODO: Update description
This library provides run-time support and shell utilities that support
the development of VxWorks applications in C++.  The run-time support can
be broken into three categories:
.IP - 4
Support for C++ new and delete operators.
.IP -
Support for arrays of C++ objects.
.IP -
Support for initialization and cleanup of static objects.
.LP
Shell utilities are provided for:
.IP - 4
Resolving overloaded C++ function names.
.IP -
Hiding C++ name mangling, with support for terse or complete
name demangling.
.IP -
Manual or automatic invocation of static constructors and destructors.
.LP
The usage of cplusLib is more fully described in the
.I "WindC++ Gateway User's Guide."
*/

/* Includes */

#include "vxWorks.h"
#include "sys/types.h"
#include "cplusLib.h"
#include "hashLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "new"

/* Defines */

/* Typedefs */

typedef void (* VOIDFUNCPTR_ARGS) (void *, int, ...);

/* Globals */

char __cplusCore_o;
#if defined(__GNUG__)
extern new_handler __new_handler;
#else
static void (*__new_handler)() = 0;
#endif

SEM_ID cplusNewHdlMutex;

/* Locals */

#if ! defined (__GNUG__)
LOCAL ArrayStore_T	 * pArrayStore = 0;
#endif


/* Forward declarations */

/****************************************************************
*
* cplusNewHandlerExists - true if application-specific new-handler exists
*
* NOMANUAL
*
* RETURNS: TRUE if new-handler exists, otherwise FALSE.
*/
BOOL cplusNewHandlerExists ()
    {
    semTake (cplusNewHdlMutex, WAIT_FOREVER);
    BOOL rval = (__new_handler != 0) ? TRUE : FALSE;
    semGive (cplusNewHdlMutex);
    return rval;
    }

/****************************************************************
*
* cplusCallNewHandler - call the allocation exception handler (C++)
*
* This function provides a procedural-interface to the new-handler.  It
* can be used by user-defined new operators to call the current
* new-handler. This function is specific to VxWorks and may not be
* available in other C++ environments.
*
* RETURNS: N/A
*/
void cplusCallNewHandler ()
    {
    semTake (cplusNewHdlMutex, WAIT_FOREVER);
    if (__new_handler != 0)
	{
	(*__new_handler) ();
	}
    semGive (cplusNewHdlMutex);
    }

/*******************************************************************************
*
* cplusTerminate - default final resting place of a unhandled C++ exception
*
* An unhandled exception eventually results in a call to 'terminate',
* which is set by the VxWorks startup code to call this function.
* Ths default behaviour can be changed using set_terminate. This function
* should never be called directly by user code. 
* 
* NOMANUAL
*
* RETURNS: No return
*/

void cplusTerminate ()
{
	cplusLogMsg ("Unhandled C++ exception resulted in call to terminate\n",
       0, 0, 0, 0, 0, 0);
  taskSuspend (0);
}

/*******************************************************************************
*
* cplusArraysInit - intialize run-time support for arrays
*
*
* This routine simply checks to see if the arrayStore is already initialized,
* and if it isn't, constructs a new one.
*
* RETURNS: N/A
*
* NOMANUAL
*/
#if defined (__GNUG__)
void cplusArraysInit ()
    {
    }
#else
void cplusArraysInit ()
    {
    if (pArrayStore == 0)
	{
	pArrayStore = new ArrayStore_T ();
	}
    }
/*******************************************************************************
*
* arrHashFunc - array hashing function
*
* This function takes the address of the first element of the array,
* throws away the rightmost 4 bits and returns this value, scaled to fit
* the number of elements in the hash table.
*
* RETURNS:
* Hash value h: 0 <= h < nElems
*
* NOMANUAL
*/
static int arrHashFunc
    (
    int		nElems,
    HASH_NODE	*pHashNode,
    int
    )
    {
    int hash = int ((((ArrayDesc_T *)(pHashNode)) -> pObject)) >> 4;
    hash &= nElems-1;
    return hash;
    }
/*******************************************************************************
*
* arrHashCmp - array hashing comparator
*
* This routine compares two array descriptors for identity.
*
* RETURNS:
* TRUE if descriptors indicate the same array, otherwise FALSE
*
* NOMANUAL
*/
static BOOL arrHashCmp
    (
    HASH_NODE	* pMatchNode,
    HASH_NODE	* pHashNode,
    int
    )
    {
    return (((ArrayDesc_T *)(pMatchNode))->pObject
	    ==
	    ((ArrayDesc_T *)(pHashNode))->pObject);
    }
/*******************************************************************************
* ArrayStore_T :: ArrayStore_T - initialize the ArrayStore
*
* This constructor creates a hash table in which to store the array
* descriptors, and a mutex semaphore to guard access to the hash
* table.
*
* RETURNS: N/A
*
* NOMANUAL
*/
ArrayStore_T :: ArrayStore_T ()
    {
    hashId = hashTblCreate (8,			// log2(size of hash table)
			    (FUNCPTR) arrHashCmp,
			    (FUNCPTR) arrHashFunc,
			    0);
    mutexSem = semMCreate (SEM_Q_PRIORITY
			   | SEM_DELETE_SAFE
			   | SEM_INVERSION_SAFE);
    }
/******************************************************************************
*
* ArrayStore_T :: ~ArrayStore_T - decommission the ArrayStore
*
* This destructor should not ever be called. It detects the error condition
* which arises if the arrayStore is deleted.
*
* RETURNS: N/A
*
* NOMANUAL
*/
ArrayStore_T :: ~ArrayStore_T ()
    {
    cplusLogMsg ("run-time support for C++ arrays deleted\n",
		 0, 0, 0, 0, 0, 0);
    taskSuspend (0);
    }
/****************************************************************
*
* ArrayStore_T :: insert
*
* This function creates a new array descriptor and adds it to
* the arrayStore. Problems are dealt with as for any failure of
* a new operator. Memory for the array descriptor is allocated
* by this function.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ArrayStore_T :: insert
    (
    void	* pObject,
    int		  nElems
    )
    {
    ArrayDesc_T	* pNode = new ArrayDesc_T (pObject, nElems);
    semTake (mutexSem, WAIT_FOREVER);
    // Cast the ArrayDesc_T pointer before storing in the hash table
    if (hashTblPut (hashId, (HASH_NODE *)(pNode)) == ERROR)
	{
	cplusCallNewHandler ();
	}
    semGive (mutexSem);
    }

/*****************************************************************
*
* ArrayStore_T :: fetch
*
* This function looks up a descriptor for the array specified by
* <pObject>. When found, the descriptor is removed from the hash table
* and deleted, then the number of elements is returned. Memory for the
* array descriptor is reclaimed by this function.
*
* RETURNS: the number of elements in the specified array, if found,
*          otherwise -1.
*
* NOMANUAL
*/
int ArrayStore_T :: fetch
    (
    void	* pObject
    )
    {
    int		rval;
    HASH_NODE * pNode;
    ArrayDesc_T	node (pObject, 0);
    semTake (mutexSem, WAIT_FOREVER);
    if ((pNode = hashTblFind (hashId, (HASH_NODE *) (&node), 0)) == 0)
	{
	rval = -1;
	}
    else
	{
	// Cast the fetched pointer back to ArrayDesc_T
	ArrayDesc_T * ad = (ArrayDesc_T *)(pNode);
	rval = ad->nElems;
	hashTblRemove (hashId, (HASH_NODE *) pNode);
	delete ad;
	}
    semGive (mutexSem);
    return rval;
    }
#endif // defined (__GNUG__)
