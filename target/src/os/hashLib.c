/* hashLib.c - generic hashing library */

/* Copyright 1990-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01l,12feb93,kdl  changed hashLibInit() to handle multiple invocations.
01k,04jul92,jcf  scalable/ANSI/cleanup effort.
01j,26may92,rrr  the tree shuffle
01i,19nov91,rrr  shut up some ansi warnings.
01h,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01g,01oct90,jcf   fixed hashTblEach() to traverse table correctly.
01f,28sep90,jcf   documentation.
01e,17jul90,dnw   changed call to objAlloc() to objAllocExtra()
01d,05jul90,jcf   documentation.
01c,23jun90,jcf   changed ffs to ffsMsb.
01b,22apr90,jcf   removed hashTblShow to indepent show routine library.
01a,17nov89,jcf   written.
*/

/*
DESCRIPTION
This subroutine library supports the creation and maintenance of a
chained hash table.  Hash tables efficiently store hash nodes for fast access.
They are frequently used for symbol tables, or other name to identifier
functions.  A chained hash table is an array of singly linked list heads,
with one list head per element of the hash table.  During creation a hash table
is passed two user-definable functions, the hashing function, and the hash node
comparator.

HASH NODES
A hash node is a structure used for chaining nodes together in the table.
The defined structure HASH_NODE is not complete because it contains no field
for the key for referencing, and no place to store data.  The user completes
the hash node by including a HASH_NODE in a structure containing the necessary
key and data fields.  This flexibility allows hash tables to better suit
varying data representations of the key and data fields.  The hashing function
and the hash node comparator determine the full hash node representation.  Refer
to the defined structures H_NODE_INT and H_NODE_STRING for examples of the
general purpose hash nodes used by the hashing functions and hash node
comparators defined in this library.

HASHING FUNCTIONS
One function, called the hashing function, controls the distribution of nodes
in the table.  This library provides a number of standard hashing functions,
but applications can specify their own.  Desirable properties of a hashing
function are that they execute quickly, and evenly distribute the nodes
throughout the table.  The worst hashing function imaginable would be:
h(k) = 0.  This function would put all nodes in list associated with the
zero element in the hash table.  Most hashing functions find their origin
in random number generators.

Hashing functions must return an index between zero and (elements - 1).  They
take the following form:
.CS

int hashFuncXXX (elements, pHashNode, keyArg)
    int		elements;	/@ number of elements in hash table        @/
    HASH_NODE	*pHashNode;	/@ hash node to pass through hash function @/
    int		keyArg;		/@ optional argument to hash function      @/

.CE

HASH NODE COMPARATOR FUNCTIONS
The second function required is a key comparator.  Different hash tables may
choose to compare hash nodes in different ways.  For example, the hash node
could contain a key which is a pointer to a string, or simply an integer.
The comparator compares the hash node on the basis of some criteria, and
returns a boolean as to the nodes equivalence.  Additionally, the key
comparator can use the keyCmpArg for additional information to the comparator.
The keyCmpArg is passed from all the hashLib functions which use the the
comparator.  The keyCmpArg is usually not needed except for advanced
hash table queurying.

symLib is a good example of the utilization of the keyCmpArg parameter.
symLib hashes the name of the symbol.  It finds the id based on the
name using hashTblFind(), but for the purposes of putting and removing
symbols from the symbol's hash table, an additional comparison restriction
applies.  Symbols have types, and while symbols of equivalent names can exist,
no symbols of equivalent name and type can exist.  So symLib utilizes the
keyCmpArg as a flag to denote the which operation being performed on the hash
table: symbol name matching, or complete symbol name and type matching.

Key comparator functions must return a boolean.  They take the following form:

.CS

int hashKeyCmpXXX (pMatchNode, pHashNode, keyCmpArg)
    HASH_NODE	*pMatchNode;	/@ hash node to match                        @/
    HASH_NODE	*pHashNode;	/@ hash node in table being compared to      @/
    int		keyCmpArg;	/@ parameter passed to hashTblFind (2)       @/

.CE

HASHING COLLISIONS
Hashing collisions occur when the hashing function returns the same index when
given two unique keys.  This is unavoidable in cases where there are more nodes
in the hash table then there are elements in the hash table.  In a chained
hash table, collisions are resolved by treating each element of the table as
the head of a linked list.  Nodes are simply added to appropriate list
regardless of other nodes already in the list.  The list is not sorted, but
new nodes are added at the head of the list because newer entries are usually
searched for before older entries.  When nodes are removed or searched for,
the list is traversed from the head until a match is found.

STRUCTURE

.CS
   HASH_HEAD 0           HASH_NODE         HASH_NODE
   ---------             --------          --------
   | head--------------->| next----------->| next---------
   |       |             |......|          |......|      |
   | tail------          | key  |          | key  |      |
   |       |  |          | data |          | data |      v
   ---------  |          --------          --------     ---
              |                             ^            -
              |                             |
              -------------------------------

   HASH_HEAD 1           HASH_NODE
   ---------             --------
   | head--------------->| next---------
   |       |             |......|      |
   | tail------          | key  |      |
   |       |  |          | data |      v
   ---------  |          --------     ---
              |           ^            -
              |           |
              -------------

    ...
    ...

   HASH_HEAD N

   ---------
   | head-----------------
   |       |             |
   | tail---------       |
   |       |     |       v
   ---------    ---     ---
		 -	 -

.CE

CAVEATS
Hash tables must have a number of elements equal to a power of two.

INCLUDE FILE: hashLib.h
*/

#include "vxWorks.h"
#include "errno.h"
#include "hashLib.h"
#include "string.h"
#include "private/classLibP.h"
#include "private/objLibP.h"

IMPORT int ffsMsb (int bitfield);

/* locals */

LOCAL OBJ_CLASS hashClass;
LOCAL BOOL	hashLibInstalled = FALSE;  /* protect from multiple inits */

/* globals */

CLASS_ID hashClassId = &hashClass;


/*******************************************************************************
*
* hashLibInit - initialize hash table library
*
* This routine initializes the hash table package.
*/

STATUS hashLibInit (void)
    {
    if (!hashLibInstalled &&(classInit (hashClassId, sizeof (HASH_TBL), 
		       	     		OFFSET(HASH_TBL,objCore),
		       	     		(FUNCPTR) hashTblCreate, 
					(FUNCPTR) hashTblInit,
		       	     		(FUNCPTR) hashTblDestroy) == OK))
	{
	hashLibInstalled = TRUE;
	}
	
    return ((hashLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* hashTblCreate - create a hash table
*
* This rountine creates a hash table 2^sizeLog2 number of elements.  The hash
* table is carved from the system memory pool via malloc (2).  To accomidate
* the list structures associated with the table, the actual amout of memory
* alocated will be roughly eight times the number of elements requested.
* Additionallly, two routines must be specified to dictate the behavior of the
* hashing table.  The first routine is the hashing function.
*
* The hashing function's role is to disperse the hash nodes added to the table
* as evenly throughout the table as possible.  The hashing function receives as
* its parameters; the number of elements in the table, a pointer to the
* HASH_NODE structure, and finally the keyArg parameter passed to this
* routine.  The keyArg may be used to seed the hashing function.  The hash
* function returns an index between 0 and (elements - 1).  Standard hashing
* functions are available in this library.
*
* The keyCmpRtn parameter specifies the other function required by the hash
* table.  This routine tests for equivalence of two HASH_NODES.  It returns a
* boolean, TRUE if the keys match, and FALSE if they differ.  As an example,
* a hash node may contain a HASH_NODE followed by a key which is an unsigned
* integer identifiers, or a pointer to a string, depending on the application.
* Standard hash node comparators are available in this library.
*
* RETURNS: HASH_ID, or NULL if hash table could not be created.
*
* SEE ALSO: hashFuncIterScale(), hashFuncModulo(), hashFuncMultiply()
* 	    hashKeyCmp(), hashKeyStrCmp()
*/

HASH_ID hashTblCreate
    (
    int         sizeLog2,       /* number of elements in hash table log 2 */
    FUNCPTR     keyCmpRtn,      /* function to test keys for equivalence */
    FUNCPTR     keyRtn,         /* hashing function to generate hash from key */
    int         keyArg          /* argument to hashing function */
    )
    {
    unsigned extra  = (1 << sizeLog2) * sizeof (SL_LIST);
    HASH_ID hashId;
    SL_LIST *pList;

    hashId  = (HASH_ID) objAllocExtra (hashClassId, extra, (void **) &pList);

    if (hashId != NULL)
	hashTblInit (hashId, pList, sizeLog2, keyCmpRtn, keyRtn, keyArg);

    return (hashId);				/* return the hash id */
    }

/*******************************************************************************
*
* hashTblInit - initialize a hash table
*
* This routine initializes a hash table.
*
* RETURNS: OK
*/

STATUS hashTblInit
    (
    HASH_TBL    *pHashTbl,      /* pointer to hash table to initialize */
    SL_LIST     *pTblMem,       /* pointer to memory of sizeLog2 SL_LISTs */
    int         sizeLog2,       /* number of elements in hash table log 2 */
    FUNCPTR     keyCmpRtn,      /* function to test keys for equivalence */
    FUNCPTR     keyRtn,         /* hashing function to generate hash from key */
    int         keyArg          /* argument to hashing function */
    )
    {
    FAST int ix;

    pHashTbl->elements	= 1 << sizeLog2;	/* store number of elements */
    pHashTbl->keyCmpRtn	= keyCmpRtn;		/* store comparator routine */
    pHashTbl->keyRtn	= keyRtn;		/* store hashing function */
    pHashTbl->keyArg	= keyArg;		/* store hashing function arg */
    pHashTbl->pHashTbl	= pTblMem;

    /* initialize all of the linked list heads in the table */

    for (ix = 0; ix < pHashTbl->elements; ix++)
	sllInit (&pHashTbl->pHashTbl [ix]);

    objCoreInit (&pHashTbl->objCore, hashClassId);	/* initialize core */

    return (OK);
    }

/*******************************************************************************
*
* hashTblDelete - delete a hash table
*
* This routine deletes the specified hash table and frees the
* associated memory.  The hash table is marked as invalid.
*
* RETURNS: OK, or ERROR if hashId is invalid.
*/

STATUS hashTblDelete
    (
    HASH_ID hashId              /* id of hash table to delete */
    )
    {
    return (hashTblDestroy (hashId, TRUE));	/* delete the hash table */
    }

/*******************************************************************************
*
* hashTblTerminate - terminate a hash table
*
* This routine terminates the specified hash table.  The hash table is marked
* as invalid.
*
* RETURNS: OK, or ERROR if hashId is invalid.
*/

STATUS hashTblTerminate
    (
    HASH_ID hashId              /* id of hash table to terminate */
    )
    {
    return (hashTblDestroy (hashId, FALSE));	/* terminate the hash table */
    }

/*******************************************************************************
*
* hashTblDestroy - destroy a hash table
*
* This routine destroys the specified hash table and optionally frees the
* associated memory.  The hash table is marked as invalid.
*
* RETURNS: OK, or ERROR if hashId is invalid.
*/

STATUS hashTblDestroy
    (
    HASH_ID hashId,             /* id of hash table to destroy */
    BOOL    dealloc             /* deallocate associated memory */
    )
    {
    if (OBJ_VERIFY (hashId, hashClassId) != OK)
	return (ERROR);				/* invalid hash id */

    objCoreTerminate (&hashId->objCore);	/* terminate core */

    if (dealloc)
	return (objFree (hashClassId, (char *) hashId));

    return (OK);
    }

/*******************************************************************************
*
* hashTblPut - put a hash node into the specified hash table
*
* This routine puts the specified hash node in the specified hash table.
* Identical nodes will be kept in FIFO order in the hash table.
*
* RETURNS: OK, or ERROR if hashId is invalid.
*
* SEE ALSO: hashTblRemove()
*/

STATUS hashTblPut
    (
    HASH_ID     hashId,         /* id of hash table in which to put node */
    HASH_NODE   *pHashNode      /* pointer to hash node to put in hash table */
    )
    {
    int		index;

    if (OBJ_VERIFY (hashId, hashClassId) != OK)
	return (ERROR);				/* invalid hash id */

    /* invoke hash table's hashing routine to get index into table */

    index = (* hashId->keyRtn) (hashId->elements, pHashNode, hashId->keyArg);

    /* add hash node to head of linked list */

    sllPutAtHead (&hashId->pHashTbl [index], pHashNode);

    return (OK);
    }

/*******************************************************************************
*
* hashTblFind - find a hash node that matches the specified key
*
* This routine finds the hash node that matches the specified key.
*
* RETURNS: pointer to HASH_NODE, or NULL if no matching hash node is found.
*/

HASH_NODE *hashTblFind
    (
    FAST HASH_ID hashId,        /* id of hash table from which to find node */
    HASH_NODE    *pMatchNode,   /* pointer to hash node to match */
    int          keyCmpArg      /* parameter to be passed to key comparator */
    )
    {
    FAST HASH_NODE *pHNode;
    int	            ix;

    if (OBJ_VERIFY (hashId, hashClassId) != OK)
	return (NULL);				/* invalid hash node */

    /* invoke hash table's hashing routine to get index into table */

    ix = (* hashId->keyRtn) (hashId->elements, pMatchNode, hashId->keyArg);

    /* search linked list for above hash index and return matching hash node */

    pHNode = (HASH_NODE *) SLL_FIRST (&hashId->pHashTbl [ix]);

    while ((pHNode != NULL) &&
	   !((* hashId->keyCmpRtn) (pMatchNode, pHNode, keyCmpArg)))
	pHNode = (HASH_NODE *) SLL_NEXT (pHNode);

    return (pHNode);
    }

/*******************************************************************************
*
* hashTblRemove - remove a hash node from a hash table
*
* This routine removes the hash node that matches the specified key.
*
* RETURNS: OK, or ERROR if hashId is invalid or no matching hash node is found.
*/

STATUS hashTblRemove
    (
    HASH_ID     hashId,         /* id of hash table to to remove node from */
    HASH_NODE   *pHashNode      /* pointer to hash node to remove */
    )
    {
    HASH_NODE *pPrevNode;
    int	      ix;

    if (OBJ_VERIFY (hashId, hashClassId) != OK)
	return (ERROR);					/* invalid hash node */

    /* invoke hash table's hashing routine to get index into table */

    ix = (* hashId->keyRtn) (hashId->elements, pHashNode, hashId->keyArg);

    pPrevNode = sllPrevious (&hashId->pHashTbl [ix], pHashNode);

    sllRemove (&hashId->pHashTbl [ix], pHashNode, pPrevNode);

    return (OK);
    }

/*******************************************************************************
*
* hashTblEach - call a routine for each node in a hash table
*
* This routine calls a user-supplied routine once for each node in the
* hash table.  The routine should be declared as follows:
* .CS
*  BOOL routine (pNode, arg)
*      HASH_NODE *pNode;	/@ pointer to a hash table node     @/
*      int	  arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if hashTblEach() is to
* continue calling it with the remaining nodes, or FALSE if it is done and
* hashTblEach() can exit.
*
* RETURNS: NULL if traversed whole hash table, or pointer to HASH_NODE that
*          hashTblEach ended with.
*/

HASH_NODE *hashTblEach
    (
    HASH_ID     hashId,         /* hash table to call routine for */
    FUNCPTR     routine,        /* the routine to call for each hash node */
    int         routineArg      /* arbitrary user-supplied argument */
    )
    {
    FAST int  ix;
    HASH_NODE *pNode = NULL;

    if (OBJ_VERIFY (hashId, hashClassId) != OK)
	return (NULL);				/* invalid hash id */

    for (ix = 0; (ix < hashId->elements) && (pNode == NULL); ix++)
	pNode = (HASH_NODE *)sllEach (&hashId->pHashTbl[ix],routine,routineArg);

    return (pNode);		/* return node we ended with */
    }

/*******************************************************************************
*
* hashFuncIterScale - interative scaling hashing function for strings
*
* This hashing function interprets the key as a pointer to a null terminated
* string.  A seed of 13 or 27 appears to work well.  It calculates the hash as
* follows:
*
* .CS
*
*  for (tkey = pHNode->string; *tkey != '\0'; tkey++)
*	hash = hash * seed + (unsigned int) *tkey;
*
*  hash &= (elements - 1);
*
* .CE
*
* RETURNS: integer between 0 and (elements - 1)
*/

int hashFuncIterScale
    (
    int                 elements,       /* number of elements in hash table */
    H_NODE_STRING       *pHNode,        /* pointer to string keyed hash node */
    int                 seed            /* seed to be used as scalar */
    )
    {
    FAST char	*tkey;
    FAST int	hash = 0;

    /* Compute string signature (sparse 32-bit hash value) */

    for (tkey = pHNode->string; *tkey != '\0'; tkey++)
	hash = hash * seed + (unsigned int) *tkey;

    return (hash & (elements - 1));	/* mask hash to (0, elements - 1) */
    }

/*******************************************************************************
*
* hashFuncModulo - hashing function using remainder technique
*
* This hashing function interprets the key as a 32 bit quantity and applies the
* standard hashing function: h (k) = K mod D.  Where D is the passed divisor.
* The result of the hash function is masked to the appropriate number of bits
* to ensure the hash is not greater than (elements - 1).
*
* RETURNS: integer between 0 and (elements - 1)
*/

int hashFuncModulo
    (
    int         elements,       /* number of elements in hash table */
    H_NODE_INT  *pHNode,        /* pointer to integer keyed hash node */
    int         divisor         /* divisor */
    )
    {
    FAST int	hash;

    hash = pHNode->key % divisor;		/* modulo hashing function */

    return (hash & (elements - 1));		/* mask hash to (0,elements-1)*/
    }

/*******************************************************************************
*
* hashFuncMultiply - multiplicative hashing function
*
* This hashing function interprets the key as a unsigned integer quantity and
* applies the standard hashing function: h (k) = leading N bits of (B * K).
* Where N is the appropriate number of bits such that the hash is not greater
* than (elements - 1).  The overflow of B * K is discarded.  The value of B is
* passed as an argument.  The choice of B is similar to that of the seed to a
* linear congruential random number generator.  Namely, B's value should take
* on a large number (roughly 9 digits base 10) and end in ...x21 where x is an
* even number.  (Don't ask... it involves statistics mumbo jumbo)
*
* RETURNS: integer between 0 and (elements - 1)
*/

int hashFuncMultiply
    (
    int         elements,       /* number of elements in hash table */
    H_NODE_INT  *pHNode,        /* pointer to integer keyed hash node */
    int         multiplier      /* multiplier */
    )
    {
    FAST int	hash;

    hash = pHNode->key * multiplier;		/* multiplicative hash func */

    hash = hash >> (33 - ffsMsb (elements));	/* take only the leading bits */

    return (hash & (elements - 1));		/* mask hash to (0,elements-1)*/
    }

/*******************************************************************************
*
* hashKeyCmp - compare keys as 32 bit identifiers
*
* This routine compares hash node keys as 32 bit identifiers.
* The argument keyCmpArg is unneeded by this comparator.
*
* RETURNS: TRUE if keys match or, FALSE if keys do not match.
*
*ARGSUSED
*/

BOOL hashKeyCmp
    (
    H_NODE_INT  *pMatchHNode,   /* hash node to match */
    H_NODE_INT  *pHNode,        /* hash node in table to compare to */
    int         keyCmpArg       /* argument ingnored */
    )
    {
    if (pMatchHNode->key == pHNode->key)	/* simple comparison */
	return (TRUE);
    else
	return (FALSE);
    }

/*******************************************************************************
*
* hashKeyStrCmp - compare keys based on strings they point to
*
* This routine compares keys based on the strings they point to.  The strings
* must be null terminated.  The routine strcmp() is used to compare keys.
* The argument keyCmpArg is unneeded by this comparator.
*
* RETURNS: TRUE if keys match or, FALSE if keys do not match.
*
*ARGSUSED
*/

BOOL hashKeyStrCmp
    (
    H_NODE_STRING       *pMatchHNode,   /* hash node to match */
    H_NODE_STRING       *pHNode,        /* hash node in table to compare to */
    int                 keyCmpArg       /* argument ingnored */
    )
    {
    if (strcmp (pMatchHNode->string, pHNode->string) == 0)
	return (TRUE);
    else
	return (FALSE);
    }
