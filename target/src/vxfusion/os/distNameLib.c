/* distNameLib.c - distributed name database library (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01r,05dec01,jws  no ARM cross endian FP support (SPR 70116 final)
01q,06nov01,jws  move tmp declaration
01p,22oct01,jws  eliminate compiler warnings (SPR 71117)
                 man pages update (SPR 71239)
01o,15oct01,jws  merge ARM FP format handling back from AE (SPR 70116 prelim)
01n,24may99,drm  added vxfusion prefix to VxFusion related includes
01m,23feb99,wlf  doc edits
01l,18feb99,wlf  doc cleanup
01k,13feb99,drm  Added two routines for flipping bytes of UINT64/double
01j,12feb99,drm  Fixed a bug in DistNameMatchOne to fix SPR #25002
01i,28oct98,drm  documentation modifications
01h,11sep98,drm  added #include to pick up distPanic()
01g,13may98,ur   some cleanup, when distNameInit() fails
01f,08may98,ur   removed 8 bit node id restriction
01e,15apr98,ur   distNameRemove() returns node to free list
01d,15apr98,ur   name database update returns OK, even if broadcast failed
01c,30mar98,ur   added some more errnos;
				 changed VALUE_TOO_LONG to ILLEGAL_LENGTH.
01b,27jun97,ur   tested - ok.
01a,06jun97,ur   written.
*/

/*
DESCRIPTION
This library contains the distributed objects distributed name database and
routines for manipulating it.  Symbolic names are bound to values, such as
message queue identifiers or simple integers. Entries can be found by name
or by value and type.  The distributed name database is replicated
throughout the system, with a copy sitting on each node.

The distributed name database library is initialized by calling
distInit() in distLib.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distNameLib.h

SEE ALSO: distLib, distNameShow
*/

#include "vxWorks.h"
#if defined (DIST_NAME_REPORT) || defined (DIST_DIAGNOSTIC)
#include "stdio.h"
#endif
#include "stdlib.h"
#include "string.h"
#include "semLib.h"
#include "private/semLibP.h"
#include "sllLib.h"
#include "hashLib.h"
#include "msgQLib.h"
#include "errnoLib.h"
#include "netinet/in.h"
#include "vxfusion/msgQDistLib.h"
#include "vxfusion/distNameLib.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/distLibP.h"
#include "vxfusion/private/distObjLibP.h"
#include "vxfusion/private/msgQDistLibP.h"
#include "vxfusion/private/distTBufLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distPktLibP.h"
#include "vxfusion/private/distNameLibP.h"

/* defines */

#define UNUSED_ARG(x)  if(sizeof(x)) {} /* to suppress compiler warnings */

#define XFLOAT 0     /* no ARM cross-endian support for VxWorks 5.x */

#define KEY_ARG			65537	/* seed for hash function */
#define KEY_CMP_ARG		0	/* not used */

/* locals */

LOCAL HASH_ID			distNameDbId;
LOCAL SEMAPHORE			distNameDbLock;
LOCAL SEMAPHORE			distNameDbUpdate;
LOCAL SL_LIST			distNameFreeList;
LOCAL BOOL			distNameLibInstalled = FALSE;

/* local prototypes */

LOCAL STATUS		distNameLclAdd (char *name, int nameLen, void *value,
					int valueLen, DIST_NAME_TYPE type);
LOCAL DIST_NAME_DB_NODE	* distNameLclAddRaw (char *name, int nameLen,
			  		     void *value, int valueLen,
					     DIST_NAME_TYPE type);
LOCAL STATUS		distNameRmtAdd (DIST_NODE_ID nodeId, char *name,
					int nameLen, void *value,
					int valueLen,
					DIST_NAME_TYPE type);
LOCAL STATUS		distNameLclRemove (char *name, int nameLen);

LOCAL int		distNameHFunc (int elements,
                                       DIST_NAME_DB_NODE *pHNode,
				       int seed);
LOCAL BOOL		distNameHCmp (DIST_NAME_DB_NODE *pMatchHNode,
				      DIST_NAME_DB_NODE *pHNode,
				      int keyCmpArg);
LOCAL DIST_STATUS	distNameInput (DIST_NODE_ID nodeIdSrc,
				       DIST_TBUF_HDR *pTBufHdr);
LOCAL BOOL		distNameMatchOne (DIST_NAME_DB_NODE *pNode,
					  DIST_NAME_MATCH *pMatch);
LOCAL BOOL		distNameBurstOne (DIST_NAME_DB_NODE *pNode,
					  DIST_NAME_BURST *pBurst);
LOCAL uint32_t * distHton64 (uint32_t* hostValue, DIST_NAME_TYPE type);
LOCAL uint32_t * distNtoh64 (uint32_t* networkValue, DIST_NAME_TYPE type);

/***************************************************************************
*
* distNameLibInit - initialize the distributed name database package (VxFusion option)
*
* Initialize the distributed name database package.  This routine currently
* does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void distNameLibInit (void)
    {
    }

/***************************************************************************
*
* distNameInit - initialize the distributed name database (VxFusion option)
*
* This routine allocates space for the distributed name database and
* initializes it. The database has 2^<sizeLog2> nodes.
*
* NOTE: This routine is called by distInit(). If you use distInit() to
* initialize a node, you need not call distNameInit().
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if nodes successfully initialized.
*
* SEE ALSO: distLib
*
* NOMANUAL
*/

STATUS distNameInit
    (
    int sizeLog2        /* init 2^sizeLog2 elements */
    )
    {
    DIST_NAME_DB_NODE *   nameDb;
    STATUS                status;
    int                   hashTblSizeLog2;
    int                   nameDbNBytes;
    int                   nameDbSize;
    int                   ix;

    if (sizeLog2 < 1)
        return (ERROR);

    if (distNameLibInstalled == TRUE)
        return (OK);

    if (hashLibInit () == ERROR)    /* hashLibInit() failed */
        return (ERROR);

    semBInit (&distNameDbLock, SEM_Q_PRIORITY, SEM_EMPTY);
    semBInit (&distNameDbUpdate, SEM_Q_PRIORITY, SEM_EMPTY);

    hashTblSizeLog2 = sizeLog2 - 1;

    distNameDbId = hashTblCreate (hashTblSizeLog2, distNameHCmp,
                                  distNameHFunc, KEY_ARG);

    if (distNameDbId == NULL)   /* hashTblCreate() failed */
        return (ERROR);

    nameDbSize = 1 << sizeLog2;
    nameDbNBytes = nameDbSize * sizeof (DIST_NAME_DB_NODE);
    nameDb = (DIST_NAME_DB_NODE *) malloc (nameDbNBytes);
    if (nameDb == NULL)
        {
        distStat.memShortage++;         /* out of memory */
        hashTblDelete (distNameDbId);   /* delete the hash table */
        return (ERROR);                 /* init failed */
        }

    sllInit (&distNameFreeList);
    for (ix = 0; ix < nameDbSize; ix++)
        sllPutAtHead (&distNameFreeList, (SL_NODE *) &nameDb[ix]);

    /* we are open for requests now */

    semGive (&distNameDbLock);

    /*
     * Add GAP service to table of services.
     */

    status = distNetServAdd (DIST_PKT_TYPE_DNDB, distNameInput,
                             DIST_DNDB_SERV_NAME, DIST_DNDB_SERV_NET_PRIO,
                             DIST_DNDB_SERV_TASK_PRIO,
                             DIST_DNDB_SERV_TASK_STACK_SZ);
    if (status == ERROR)
        {
        free (nameDb);                 /* free database memory */
        hashTblDelete (distNameDbId);  /* delete the hash table */
        return (ERROR);                /* init failed */
        }

    distNameLibInstalled = TRUE;
    return (OK);
    }

/***************************************************************************
*
* distNameAdd - add an entry to the distributed name database (VxFusion option)
*
* This routine adds the name of a specified object, along with its type and 
* value, to the distributed objects distributed name database. All copies of 
* the distributed name database within the system are updated.
* 
* The <name> parameter is an arbitrary, null-terminated string with a
* maximum of 20 characters, including the null terminator.
*
* The value associated with <name> is located at <value> and is of length
* <valueLen>, currently limited to 8 bytes.
*
* By convention, <type> values of less than 0x1000 are reserved by VxWorks;
* all other values are user definable.  The following types are pre-defined
* in distNameLib.h :
*
* \ts
* Type Name | Value | Datum
* ----------|------------|------
*     T_DIST_MSG_Q |  =  0 | distributed message queue 
*     T_DIST_NODE | = 16 | node ID
*     T_DIST_UINT8 | = 64 | 8-bit unsigned integer
*     T_DIST_UINT16 | = 65 | 16-bit unsigned integer
*     T_DIST_UINT32 | = 66 | 32-bit unsigned integer
*     T_DIST_UINT64 | = 67 | 64-bit unsigned integer
*     T_DIST_FLOAT | = 68 | float (32-bit)
*     T_DIST_DOUBLE | = 69 | double (64-bit)
* \te
*
* The byte-order of pre-defined types is preserved in a
* byte-order-heterogeneous network.
*
* The value (and type!) bound to a symbolic name can be changed by calling
* distNameAdd() with a new value (and type).
*
* This routine returns OK, even if some nodes on the system do not
* respond to the add request broadcast. A node that does not acknowledge
* a transmission is assumed to have crashed. You can use the distCtl() routine
* in distLib to set a routine to be called in the event that a node crashes.
*
* NOTE:
* If you add a distributed object ID (T_DIST_MSG_Q) to the database,
* another reference to the object is built. This reference is stored
* in the database. After the return of distNameAdd(), <value> holds the
* reference (a new object ID). Use the ID returned by
* distNameAdd() each time you want to address the global object bound
* to <name>. Subsequent updates of the binding in the database are
* transparent. The original object ID specifies exactly the
* locally created object.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the operation fails.
*
* ERRNO:
* \is
* \i S_distNameLib_NAME_TOO_LONG
* The name being added to the database is too long.
* \i S_distNameLib_ILLEGAL_LENGTH
* The argument <valueLen> is not in the range 1 to 8.
* \i S_distNameLib_DATABASE_FULL
* The database is full.
* \i S_distNameLib_INCORRECT_LENGTH
* The argument <valueLen> is incorrect for the pre-defined <type>.
* \ie
*
* SEE ALSO: distLib
*/

STATUS distNameAdd
    (
    char *          name,       /* name to enter in database  */
    void *          value,      /* ptr to value to associate with name */
    int             valueLen,   /* size of value in bytes     */
    DIST_NAME_TYPE  type        /* type associated with name  */
    )
    {
    int        nameLen;
#ifdef DIST_DIAGNOSTIC
    STATUS    status;
#endif

    if ((nameLen = strlen (name)) > DIST_NAME_MAX_LENGTH)
        {
        errnoSet (S_distNameLib_NAME_TOO_LONG);
        return (ERROR);    /* name too long */
        }

    if (valueLen <= 0 || valueLen > DIST_VALUE_MAX_LENGTH)
        {
        errnoSet (S_distNameLib_ILLEGAL_LENGTH);
        return (ERROR);    /* size of value out of range */
        }

    switch (type)
        {
        case T_DIST_UINT8:
            if (valueLen != 1)
                {
                errnoSet (S_distNameLib_INCORRECT_LENGTH);
                return (ERROR);
                }
            break;
        case T_DIST_UINT16:
            if (valueLen != 2)
                {
                errnoSet (S_distNameLib_INCORRECT_LENGTH);
                return (ERROR);
                }
            break;
        case T_DIST_FLOAT:
        case T_DIST_UINT32:
        case T_DIST_NODE:
            if (valueLen != 4)
                {
                errnoSet (S_distNameLib_INCORRECT_LENGTH);
                return (ERROR);
                }
            break;
        case T_DIST_UINT64:
        case T_DIST_DOUBLE:
            if (valueLen != 8)
                {
                errnoSet (S_distNameLib_INCORRECT_LENGTH);
                return (ERROR);

                }

        /* case T_DIST_MSG_Q: no checking for complex object type */

        }

    /* Local name database update. */

    if (distNameLclAdd (name, nameLen, value, valueLen, type) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distNameAdd: error updating local name database\n");
#endif
        errnoSet (S_distNameLib_DATABASE_FULL);
        return (ERROR);
        }

    /* Remote name database update. */

#ifdef DIST_DIAGNOSTIC
    status = distNameRmtAdd (DIST_IF_BROADCAST_ADDR, name, nameLen, value,
                             valueLen, type);

    if (status == ERROR)
        {
        /* Local node has binding--one of the remote nodes may not. */

        distLog ("distNameAdd: error updating remote name databases\n");
        }
#else
    distNameRmtAdd (DIST_IF_BROADCAST_ADDR, name, nameLen, value,
                    valueLen, type);
#endif

    return (OK);    /* return OK, always */
    }

/***************************************************************************
*
* distNameLclAdd - bind name to value in local name database (VxFusion option)
*
* This routine binds a name to a given value in name database. If <type>
* is a distributed object preprocess and postprocess identifier.
*
* NOTE: Before calling distNameLclAdd(), nameLen and valueLen
* must been tested to prevent overflows.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
*
* NOMANUAL
*/

LOCAL STATUS distNameLclAdd
    (
    char *         name,     /* name to enter in database */
    int            nameLen,  /* strlen (name) */
    void *         value,    /* value associated with name */
    int            valueLen, /* size of value in bytes */
    DIST_NAME_TYPE type      /* type associated with name */
    )
    {
    DIST_NAME_DB_NODE * pDbNode;
    DIST_OBJ_NODE *     pObjNode;
    void *              pVal;
    MSG_Q_ID            msgQId;

    /* Preprocessing */

    switch (type)
        {
        case T_DIST_MSG_Q:
            {
            /*
             * A little tricky, what we do here: user gives us a
             * message queue id. Check if it is a distributed
             * message id. If not, return ERROR.
             * Create a new DIST_OBJ_NODE and copy distributed
             * id to new DIST_OBJ_NODE. Return the new MSG_Q_ID
             * (which is in fact another reference to the MSG_Q_ID,
             * the user gave us).
             */

            msgQId = *((MSG_Q_ID *) value);

            if (!ID_IS_DISTRIBUTED (msgQId))
                return (ERROR);    /* not a distributed message queue */
            if (DIST_OBJ_VERIFY (msgQId) == ERROR)
                return (ERROR);

            pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
            if (! IS_DIST_MSG_Q_OBJ (pObjNode))
                return (ERROR); /* legal object id, but not a message queue */

            pVal = &(pObjNode->objNodeUniqId);
            valueLen = sizeof (pObjNode->objNodeUniqId);

            break;
            }
        default:
            pVal = value;
        }

    /* Local database update (raw part). */

    pDbNode = distNameLclAddRaw (name, nameLen, pVal, valueLen, type);
    if (pDbNode == NULL)
        return (ERROR);

    /* Postprocessing */

    switch (type)
        {
        case T_DIST_MSG_Q:
            *((MSG_Q_ID *) value) = *((MSG_Q_ID *) &pDbNode->value);
        }

    return (OK);
    }

/***************************************************************************
*
* distNameBurst - burst out name database (VxFusion option)
*
* This routine is used by INCO to update the remote name database on
* node <nodeId>. All entries in the database are transmitted.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
*
* NOMANUAL
*/

STATUS distNameBurst
    (
    DIST_NODE_ID    nodeId    /* node receiving burst */
    )
    {
    DIST_NAME_BURST    burst;

    burst.burstNodeId = nodeId;
    burst.burstStatus = OK;

    distNameEach ((FUNCPTR) distNameBurstOne, (int) &burst);

    return (burst.burstStatus);
    }

/***************************************************************************
*
* distNameBurstOne - burst out single name database entry (VxFusion option)
*
* This routine bursts out a single database entry to a node.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if burst out successful
*
* NOMANUAL
*/

LOCAL BOOL distNameBurstOne
    (
    DIST_NAME_DB_NODE * pNode,   /* node receiving burst */
    DIST_NAME_BURST *   pBurst   /* the entry to send */
    )
    {
    char    *name = (char *) &pNode->symName;

    if ((pBurst->burstStatus = distNameRmtAdd (pBurst->burstNodeId,
            name, strlen (name), &pNode->value, pNode->valueLen,
            pNode->type)) == ERROR)
        return (FALSE);

    return (TRUE);
    }

/***************************************************************************
*
* distNameRmtAdd - bind name to value in remote name databases (VxFusion option)
*
* This routine adds or updates a name/value pair in a remote node.
*
* NOTE: Before calling distNameRmtAdd(), nameLen and valueLen
* must been tested to prevent overflows.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, if successful.
*
* NOMANUAL
*/

LOCAL STATUS distNameRmtAdd
    (
    DIST_NODE_ID    nodeId,     /* node to address */
    char *          name,       /* name to enter in database */
    int             nameLen,    /* strlen (name) */
    void *          value,      /* value associated with name */
    int             valueLen,   /* size of value in bytes */
    DIST_NAME_TYPE  type        /* type associated with name */
    )
    {
    DIST_NAME_TYPE_ALL    valNet;
    DIST_PKT_DNDB_ADD     pktDndbAdd;
    DIST_IOVEC            IOVec[3];
    STATUS                status;
    void *                pValNet;
    DIST_OBJ_UNIQ_ID *    pObjUniqId;
    DIST_OBJ_NODE *       pObjNode;
    MSG_Q_ID              msgQId;

    /*
     * We have to copy data, since some processors allow
     * memory requests to non-aligned addresses, others
     * don't.
     */

    switch (type)
        {
        case T_DIST_UINT8:
            {
            pValNet = value;
            break;
            }
        case T_DIST_UINT16:
            {
            bcopy (value, (char *) &(valNet.uint16), valueLen);
            valNet.uint16 = htons (valNet.uint16);
            pValNet = &(valNet.uint16);
            break;
            }
        case T_DIST_NODE:
        case T_DIST_FLOAT:
        case T_DIST_UINT32:
            {
            bcopy (value, (char *) &(valNet.uint32), valueLen);
            valNet.uint32 = htonl (valNet.uint32);
            pValNet = &valNet.uint32;
            break;
            }
        case T_DIST_UINT64:
        case T_DIST_DOUBLE:
            {
            bcopy (value, (char *) &(valNet.uint64), valueLen);
            pValNet = distHton64((uint32_t*)&(valNet.uint64),type);
            break;
            }
        case T_DIST_MSG_Q:        /* complex objects go here */
            {
            /*
             * A little tricky, what we do here: user gives us a
             * message queue id. Check if it is a distributed
             * message id. If not, return ERROR.
             * Get the distributed (unique!) id instead of the
             * local MSG_Q_ID (local pointer).
             */

            msgQId = *((MSG_Q_ID *) value);

            if (!ID_IS_DISTRIBUTED (msgQId))
                return (ERROR);    /* not a distributed message queue */
            if (DIST_OBJ_VERIFY (msgQId) == ERROR)
                return (ERROR);

            pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
            if (! IS_DIST_MSG_Q_OBJ (pObjNode))
                return (ERROR); /* legal object id, but not a message queue */

            bcopy ((char *) &(pObjNode->objNodeUniqId),
                   (char *) &(valNet.objUniqId),
                   sizeof (pObjNode->objNodeUniqId));

            pObjUniqId = (DIST_OBJ_UNIQ_ID *) &(valNet.objUniqId);
            pObjUniqId->objUniqReside    = htonl (pObjUniqId->objUniqReside);
            pObjUniqId->objUniqId        = htonl (pObjUniqId->objUniqId);

            pValNet = pObjUniqId;
            valueLen = sizeof (*pObjUniqId);
            break;
            }
        default:
            pValNet = value;    /* user defined, do not know what to do */
        }

    /* Remote database update. */

    pktDndbAdd.dndbAddHdr.pktType = DIST_PKT_TYPE_DNDB;
    pktDndbAdd.dndbAddHdr.pktSubType = DIST_PKT_TYPE_DNDB_ADD;
    pktDndbAdd.dndbAddType = htons (type);
    pktDndbAdd.dndbAddValueLen = (UINT8) valueLen;
    pktDndbAdd.dndbAddNameLen = (UINT8) nameLen;

    IOVec[0].pIOBuffer = &pktDndbAdd;
    IOVec[0].IOLen = DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD);
    IOVec[1].pIOBuffer = pValNet;
    IOVec[1].IOLen = valueLen;
    IOVec[2].pIOBuffer = name;
    IOVec[2].IOLen = nameLen;

    status = distNetIOVSend (nodeId, &IOVec[0], 3, WAIT_FOREVER,
            DIST_PKT_DNDB_PRIO);

    return (status);
    }

/***************************************************************************
*
* distNameFind - find an object by name in the local database (VxFusion option)
*
* This routine searches the distributed name database for an object matching
* a specified <name>. If the object is found, a pointer to the value and its
* type are copied to the address pointed to by <pValue> and <pType>.
* If the type is T_DIST_MSG_Q, the identifier returned can be used with
* generic message queue handling routines in msgQLib, such as msgQSend(), 
* msgQReceive(), and msgQNumMsgs().
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the search fails.
*
* ERRNO:
* \is
* \i S_distNameLib_NAME_TOO_LONG
* The name to be found in the database is too long.
* \i S_distNameLib_INVALID_WAIT_TYPE
* The wait type should be either NO_WAIT or WAIT_FOREVER .
* \ie
*/

STATUS distNameFind
    (
    char *           name,    /* name to search for            */
    void **          pValue,  /* where to return ptr to value */
    DIST_NAME_TYPE * pType,   /* where to return type  */
    int              waitType /* NO_WAIT or WAIT_FOREVER       */
    )
    {
    DIST_NAME_DB_NODE * pNode;
    DIST_NAME_DB_NODE   matchNode;
    int                 nameLen;

    if ((nameLen = strlen (name)) > DIST_NAME_MAX_LENGTH)
        {
        errnoSet (S_distNameLib_NAME_TOO_LONG);
        return (ERROR);    /* name too long */
        }

    if (waitType != NO_WAIT && waitType != WAIT_FOREVER)
        {
        errnoSet (S_distNameLib_INVALID_WAIT_TYPE);
        return (ERROR); /* wait type is invalid */
        }

    bcopy (name, (char *) &matchNode.symName, nameLen + 1);

again:

    distNameLclLock();

    pNode = (DIST_NAME_DB_NODE *) hashTblFind (distNameDbId,
            (HASH_NODE *)&matchNode, KEY_CMP_ARG);

    distNameLclUnlock();

    if (!pNode)
        {
        if (waitType == WAIT_FOREVER)
            {
            /* object not found; we have to wait ... */
            distNameLclBlockOnAdd();
            goto again;
            }
        else
            return (ERROR);    /* object not found */
        }
    
    *pType = pNode->type;
    *pValue = &pNode->value;

    return (OK);
    }

/***************************************************************************
*
* distNameFindByValueAndType - look up the name of an object by value and type (VxFusion option)
*
* This routine searches the distributed name database for an object matching
* a specified <value> and <type>. If the object is found, its name is 
* copied to the address pointed to by <name>.
*
* NOTE:
* Unlike the smNameFindByValue() routine, used with the shared-memory 
* objects name database, this routine must know the type of the object
* being searched for. Searching on the value only might not return a
* unique object.
* 
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the search fails.
*
* ERRNO:
* \is
* \i S_distNameLib_INVALID_WAIT_TYPE
* The wait type should be either NO_WAIT or WAIT_FOREVER .
* \ie
*/

STATUS distNameFindByValueAndType
    (
    void *           value,       /* value to search for          */
    DIST_NAME_TYPE   type,        /* type of object for which to search */
    char *           name,        /* where to return name */
    int              waitType     /* NO_WAIT or WAIT_FOREVER      */
    )
    {
    DIST_NAME_DB_NODE *    pNode;
    DIST_NAME_MATCH        match;

    if (waitType != NO_WAIT && waitType != WAIT_FOREVER)
        {
        errnoSet (S_distNameLib_INVALID_WAIT_TYPE);
        return (ERROR); /* wait type is invalid */
        }

    match.pMatchValue = value;
    match.matchType = type;

again:

    pNode = distNameEach ((FUNCPTR) distNameMatchOne, (int) &match);

    if (!pNode)
        {
        if (waitType == WAIT_FOREVER)
            {
            /* object not found; we have to wait ... */
            distNameLclBlockOnAdd();
            goto again;
            }
        else
            return (ERROR);    /* object not found */
        }
    
    strcpy (name, (char *) &(pNode->symName));
    return (OK);
    }

/***************************************************************************
*
* distNameMatchOne - helper for distNameFindByValueAndType() (VxFusion option)
*
* This routine is called by distNameFindByValueAndType() to check a database
* node for a match.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if a match is found.
*
* NOMANUAL
*/

LOCAL BOOL distNameMatchOne
    (
    DIST_NAME_DB_NODE * pNode,    /* database node to match */
    DIST_NAME_MATCH *   pMatch    /* match data */
    )
    {
    DIST_OBJ_NODE * pObjNodeMatch;
    DIST_OBJ_NODE * pObjNodeDb;
    MSG_Q_ID        msgQId;


    if (pNode->type == pMatch->matchType)
        {
        switch (pNode->type)
            {
            case T_DIST_MSG_Q:
                {
                pObjNodeDb = MSG_Q_ID_TO_DIST_OBJ_NODE (pNode->value.msgQId);
                msgQId = *((MSG_Q_ID *) pMatch->pMatchValue);
                pObjNodeMatch = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);

                /*
                 * Check for equality.  For group IDs, the objUniqId 
                 * referenced using objNodeId is enough.  However, for
                 * normal message queues, we need to check both the objUniqId
                 * (using objNodeId) and objUniqReside (using objNodeReside)
                 * fields.
                 */

                if (IS_DIST_MSG_Q_TYPE_GRP ( (DIST_MSG_Q_ID) 
                    pObjNodeMatch->objNodeId))
                    {
                    if (pObjNodeDb->objNodeId == pObjNodeMatch->objNodeId)
                        return (FALSE);        /* matched */
                    }
                else
                    {
                    if ((pObjNodeDb->objNodeId ==
                                 pObjNodeMatch->objNodeId)
                                    && 
                        (pObjNodeDb->objNodeReside ==
                                 pObjNodeMatch->objNodeReside))
                        {
                        return (FALSE);
                        }
                    }

                break;
                }
            default:
                {
                char    *valDb = (char *) &(pNode->value);
                char    *valMatch = (char *) pMatch->pMatchValue;

                if (! bcmp (valDb, valMatch, pNode->valueLen))
                    return (FALSE);        /* matched */
                }
            }
        }
    return (TRUE);    /* continue */
    }

/***************************************************************************
*
* distNameRemove - remove an entry from the distributed name database (VxFusion option)
*
* This routine removes an object, that is bound to <name>, from the
* distributed name database.  All copies of the distributed name database 
* get updated.
*
* This routine returns OK, even if some nodes on the system do not
* respond to the remove request broadcast.  A node that does not acknowledge
* a transmission is assumed to have crashed.
* You can use the distCtl() routine
* in distLib to set a routine to be called in the event that a node crashes.
*
* Removing the name of a distributed object ID (T_DIST_MSG_Q) does not
* invalidate the object ID.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the operation fails.
*
* ERRNO:
* \is
* \i S_distNameLib_NAME_TOO_LONG
* The name to be removed from the database is too long.
* \ie
*
* SEE ALSO: distLib
*/

STATUS distNameRemove
    (
    char * name            /* name of object to remove */
    )
    {
    DIST_PKT_DNDB_RM    pktDndbRm;
    DIST_IOVEC          IOVec[2];
    int                 nameLen;
#ifdef DIST_DIAGNOSTIC
    STATUS    status;
#endif

    if ((nameLen = strlen (name)) > DIST_NAME_MAX_LENGTH)
        {
        errnoSet (S_distNameLib_NAME_TOO_LONG);
        return (ERROR);    /* name too long */
        }

    if (distNameLclRemove (name, nameLen) == ERROR)
        return (ERROR); /* name not in database */

    pktDndbRm.dndbRmHdr.pktType = DIST_PKT_TYPE_DNDB;
    pktDndbRm.dndbRmHdr.pktSubType = DIST_PKT_TYPE_DNDB_RM;
    pktDndbRm.dndbRmNameLen = (UINT8) nameLen;

    IOVec[0].pIOBuffer = &pktDndbRm;
    IOVec[0].IOLen = DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_RM);
    IOVec[1].pIOBuffer = name;
    IOVec[1].IOLen = nameLen;

#ifdef DIST_DIAGNOSTIC
    status = distNetIOVSend (DIST_IF_BROADCAST_ADDR, &IOVec[0], 2,
                        WAIT_FOREVER, DIST_PKT_DNDB_PRIO);

    if (status == ERROR)
        {
        /* Local node has removed binding--one of the remote nodes may not. */

        distLog ("distNameRemove: error updating remote name databases\n");
        }
#else
    distNetIOVSend (DIST_IF_BROADCAST_ADDR, &IOVec[0], 2,
                    WAIT_FOREVER, DIST_PKT_DNDB_PRIO);
#endif

    return (OK);    /* return OK, always */
    }

/***************************************************************************
*
* distNameInput - called when a DNDB packet arrives (VxFusion option)
*
* This routine processes incoming DNDB packets.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Status of packet processing.
*
* NOMANUAL
*/

LOCAL DIST_STATUS distNameInput
    (
    DIST_NODE_ID    nodeIdSrc,    /* not currently used */
    DIST_TBUF_HDR * pTBufHdr      /* points to input packet */
    )
    {
    DIST_PKT * pPkt;
    int        pktLen;
    
    UNUSED_ARG(nodeIdSrc);
    
    distStat.dndbInReceived++;

    if ((pktLen = pTBufHdr->tBufHdrOverall) < sizeof (DIST_PKT))
        distPanic ("distNameInput: packet too short\n");

    pPkt = (DIST_PKT *) (DIST_TBUF_GET_NEXT (pTBufHdr))->pTBufData;

    switch (pPkt->pktSubType)
        {
        case DIST_PKT_TYPE_DNDB_ADD:
            {
            DIST_NAME_TYPE_ALL   valNet;
            DIST_PKT_DNDB_ADD    pktAdd;
            DIST_NAME_DB_NODE *  pNode;
            DIST_NAME_TYPE       type;
            void *               pValNet;
            char                 name[DIST_NAME_MAX_LENGTH + 1];
            int                  valueLen;
            int                  nameLen;

            if (pktLen < DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD))
                distPanic ("distNameInput/ADD: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                    (char *)&pktAdd, DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD));

            valueLen = pktAdd.dndbAddValueLen;
            nameLen = pktAdd.dndbAddNameLen;

            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD) +
                    valueLen + nameLen)
                distPanic ("distNameInput/ADD: packet has wrong length\n");
            if (valueLen > DIST_VALUE_MAX_LENGTH)
                distPanic ("distNameInput/ADD: value too long\n");
            if (nameLen > DIST_NAME_MAX_LENGTH)
                distPanic ("distNameInput/ADD: name too long\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                    (DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD) + valueLen),
                    (char *) &name, nameLen);

            switch ((type = ntohs (pktAdd.dndbAddType)))
                {
                case T_DIST_UINT16:
                    {
                    distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                            DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD),
                            (char *) &(valNet.uint16), valueLen);

                    valNet.uint16 = ntohs (valNet.uint16);
                    pValNet = &(valNet.uint16);
                    break;
                    }

                case T_DIST_FLOAT:
                case T_DIST_UINT32:
                case T_DIST_NODE:
                    {
                    distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                            DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD),
                            (char *) &(valNet.uint32), valueLen);

                    valNet.uint32 = ntohl (valNet.uint32);
                    pValNet = &(valNet.uint32);
                    break;
                    }

                case T_DIST_UINT64:
                case T_DIST_DOUBLE:
                    {
                    distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                            DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD),
                            (char *) &(valNet.uint64), valueLen);
                    pValNet = distNtoh64((uint32_t *)&(valNet.uint64),type);
                    break;
                    }

                case T_DIST_MSG_Q:    /* complex objects go here */
                    {
                    DIST_OBJ_UNIQ_ID    *pObjUniqId;

                    distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                            DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD),
                            (char *) &(valNet.objUniqId), valueLen);

                    pObjUniqId = (DIST_OBJ_UNIQ_ID *) &(valNet.objUniqId);
                    pObjUniqId->objUniqReside =
                            ntohl (pObjUniqId->objUniqReside);
                    pObjUniqId->objUniqId =
                            ntohl (pObjUniqId->objUniqId);

                    pValNet = pObjUniqId;
                    break;
                    }

                default:    /* T_DIST_UINT8 and user defined */
                    {
                    distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                            DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_ADD),
                            (char *) &(valNet.field), valueLen);
                    pValNet = &valNet.field;
                    }
                }

            pNode = distNameLclAddRaw ((char *) &name, nameLen, pValNet,
                    valueLen, type);
            if (! pNode)
                distPanic ("distNameInput/ADD: database add failed\n");

            return (DIST_NAME_STATUS_OK);
            }

        case DIST_PKT_TYPE_DNDB_RM:
            {
            DIST_PKT_DNDB_RM    pktRm;
            char                name[DIST_NAME_MAX_LENGTH + 1];
            int                 nameLen;

            if (pktLen < DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_RM))
                distPanic ("distNameInput/RM: packet too short\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr), 0,
                    (char *) &pktRm, DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_RM));

            nameLen = pktRm.dndbRmNameLen;

            if (pktLen != DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_RM) + nameLen)
                distPanic ("distNameInput/RM: packet has wrong length\n");
            if (nameLen > DIST_NAME_MAX_LENGTH)
                distPanic ("distNameInput/RM: name too long\n");

            distTBufCopy (DIST_TBUF_GET_NEXT (pTBufHdr),
                    DIST_PKT_HDR_SIZEOF (DIST_PKT_DNDB_RM),
                    (char *) &name, nameLen);
            
            if (distNameLclRemove (name, nameLen) == ERROR)
                distPanic ("distNameInput/RM: not in database\n");

            return (DIST_NAME_STATUS_OK);
            }

        default:
#ifdef DIST_DIAGNOSTIC
            distLog ("distNameInput: unknown subtype %d\n",
                    pPkt->pktSubType);
#endif
            return (DIST_NAME_STATUS_PROTOCOL_ERROR);
        }
    }

/***************************************************************************
*
* distNameLclAddRaw - bind name to raw value in local name database (VxFusion option)
*
* Internally used function to bind name in local database.
*
* NOTE: Before calling distNameLclAddRaw(), nameLen and valueLen
* must been tested to prevent overflows.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Pointer to added node, or NULL.
*
* NOMANUAL
*/

LOCAL DIST_NAME_DB_NODE * distNameLclAddRaw
    (
    char *          name,       /* name to enter in database */
    int             nameLen,    /* length of name in bytes */
    void *          value,      /* value associated with name */
    int             valueLen,   /* size of value in bytes */
    DIST_NAME_TYPE  type        /* type associated with name */
    )
    {
    DIST_NAME_DB_NODE * pDbNode;
    DIST_NAME_DB_NODE * pDbNodeOld;
    DIST_OBJ_NODE *     pObjNode;
    char *              nameDest;
    char *              valDest;

#ifdef DIST_NAME_REPORT
    printf ("distNameLclAddRaw: type %d, value: ", type);
    distDump (value, valueLen);
#endif

    /*  Get a free database node. */

    if (! (pDbNode = (DIST_NAME_DB_NODE *) sllGet (&distNameFreeList)))
        return (NULL);  /* database is full */

    nameDest = (char *) &pDbNode->symName;

    bcopy (name, nameDest, nameLen);
    *(nameDest + nameLen) = 0;

    distNameLclLock();

    /* Try to find symbolic name in database. */

    pDbNodeOld = (DIST_NAME_DB_NODE *) hashTblFind (distNameDbId,
            (HASH_NODE *) pDbNode, KEY_CMP_ARG);

    if (pDbNodeOld)
        {
        /* Symbolic name is alreay in database. Update the node. */

        sllPutAtHead (&distNameFreeList, (SL_NODE *) pDbNode);
        pDbNode = pDbNodeOld;

        if (type == T_DIST_MSG_Q)
            {
            /* New object is a distributed message queue. */

            MSG_Q_ID        msgQId;

            if (pDbNode->type == T_DIST_MSG_Q)
                {
                /* Old object is also a distributed message queue. */

                msgQId = *((MSG_Q_ID *) &pDbNode->value);
                pObjNode = MSG_Q_ID_TO_DIST_OBJ_NODE (msgQId);
                }
            else
                {
                /* Old object was *NOT* a distributed message queue. */

                pObjNode = distObjNodeGet();
                pObjNode->objNodeType = DIST_OBJ_TYPE_MSG_Q;
                pDbNode->value.msgQId = DIST_OBJ_NODE_TO_MSG_Q_ID (pObjNode);
                }

            valDest = (char *) &(pObjNode->objNodeUniqId);
            }
        else
            valDest = (char *) &pDbNode->value;
        }
    else
        {
        /* Symbolic name is *NOT* in the database. */

        if (type == T_DIST_MSG_Q)
            {
            pObjNode = distObjNodeGet();
            pObjNode->objNodeType = DIST_OBJ_TYPE_MSG_Q;
            pDbNode->value.msgQId = DIST_OBJ_NODE_TO_MSG_Q_ID (pObjNode);
            valDest = (char *) &(pObjNode->objNodeUniqId);
            }
        else
            valDest = (char *) &pDbNode->value;
        }

    pDbNode->type = type;
    pDbNode->valueLen = valueLen;
    bcopy (value, valDest, valueLen);

    if (!pDbNodeOld)
        hashTblPut (distNameDbId, (HASH_NODE *) pDbNode);

    distNameLclUnlock();

    /* signal a database update to waiting tasks */

    distNameLclSigAdd();

    return (pDbNode);
    }

/***************************************************************************
*
* distNameLclRemove - remove an object from the distributed database (VxFusion option)
*
* This routines removes a name from the local name database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if name could not be removed.
*
* NOMANUAL
*/

LOCAL STATUS distNameLclRemove
    (
    char *  name,            /* name of object to remove */
    int     nameLen          /* length of name without EOS */
    )
    {
    DIST_NAME_DB_NODE *  pNode;
    DIST_NAME_DB_NODE    matchNode;

    bcopy (name, (char *) &matchNode.symName, nameLen);
    *(((char *) &matchNode.symName) + nameLen) = '\0';

    distNameLclLock();
    pNode = (DIST_NAME_DB_NODE *) hashTblFind (distNameDbId,
            (HASH_NODE *) &matchNode, KEY_CMP_ARG);
    if (!pNode || hashTblRemove(distNameDbId, (HASH_NODE *) pNode) == ERROR)
        {
        distNameLclUnlock();
        return (ERROR);
        }

    sllPutAtHead (&distNameFreeList, (SL_NODE *) pNode); /* return node */

    distNameLclUnlock();
    return (OK);
    }

/***************************************************************************
*
* distNameHCmp - compare keys based on strings (VxFusion option)
*
* This is the hash compare function for names in the database.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: TRUE, if the names of the DB nodes are the same.
*
* NOMANUAL
*/

LOCAL BOOL distNameHCmp
    (
    DIST_NAME_DB_NODE * pMatchHNode,   /* first node */
    DIST_NAME_DB_NODE * pHNode,        /* second node */
    int                 keyCmpArg      /* not used */
    )
    {
    
    UNUSED_ARG(keyCmpArg);

    if (strcmp ((char *)&pMatchHNode->symName, (char *)&pHNode->symName) == 0)
        return (TRUE);
    else
        return (FALSE);
    }

/***************************************************************************
*
* distNameHFunc - hashing function for strings (VxFusion option)
*
* This is the hashing function for database object names.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A hash index.
*
* NOMANUAL
*/

LOCAL int distNameHFunc
    (
    int                 elements,   /* size of hash table */
    DIST_NAME_DB_NODE * pHNode,     /* node whose name to hash */
    int                 seed        /* hash seed */
    )
    {
    char *tkey;
    int  hash = 0;

    /* Compute string signature (sparse 32-bit hash value) */

    for (tkey = (char *) &pHNode->symName; *tkey != '\0'; tkey++)
        hash = hash * seed + (unsigned int) *tkey;

    return (hash & (elements - 1));    /* mask hash to (0, elements - 1) */
    }

/***************************************************************************
*
* distNameEach - each functionality on name database (VxFusion option)
*
* Successively visits every node in the database and invokes <routine>,
* passing it argument <routineArg>.
*
* NOTE: Takes <distNameDbLock>.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: Pointer to last node visited.
*
* NOMANUAL
*/

DIST_NAME_DB_NODE * distNameEach
    (
    FUNCPTR    routine,         /* routine to invoke */
    int        routineArg       /* argument for routine */
    )
    {
    DIST_NAME_DB_NODE * lastNode;

    distNameLclLock();

    lastNode = (DIST_NAME_DB_NODE *) hashTblEach (distNameDbId, routine,
                                                  routineArg);

    distNameLclUnlock();

    return (lastNode);
    }


/***************************************************************************
*
* distHton64 - convert a 64 bit value from host to network byte order (VxFusion option)
*
* This routine converts T_DIST_UINT64 and T_DIST_DOUBLE
* values from host to network
* byte order.  The value is converted in place.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: <hostValue>
*
* NOMANUAL
*/

LOCAL uint32_t * distHton64
    (
    uint32_t *     hostValue,   /* ptr to value to convert */
    DIST_NAME_TYPE type         /* T_DIST_UINT64 or T_DIST_DOUBLE */
    )
    {
#if _BYTE_ORDER==_LITTLE_ENDIAN
    uint32_t tmp;
#endif

#if !XFLOAT
    UNUSED_ARG(type);
#endif

#ifndef _BYTE_ORDER
#   error "no byte order specified !!"
#endif

#if _BYTE_ORDER==_LITTLE_ENDIAN

# if XFLOAT     /* ARM little-endian with double cross */

    if (type == T_DIST_DOUBLE)
        {
        hostValue[0] = htonl(hostValue[0]);
        hostValue[1] = htonl(hostValue[1]);
        }
    else
        {
        tmp = hostValue[0];
        hostValue[0] = htonl (hostValue[1]);
        hostValue[1] = htonl (tmp);
        }

# else           /* non-ARM little-endian */

    tmp = hostValue[0];
    hostValue[0] = htonl (hostValue[1]);
    hostValue[1] = htonl (tmp);


# endif /* XFLOAT */
#endif /* _BYTE_ORDER test */

    return hostValue;
    }


/***************************************************************************
*
* distNtoh64 - convert a 64 bit value from network to host byte order (VxFusion option)
*
* This routine converts T_DIST_UINT64 and T_DIST_DOUBLE values from
* network to host byte order.  The value is converted in place.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: <networkValue>
*
* NOMANUAL
*/

LOCAL uint32_t * distNtoh64
    (
    uint32_t *     networkValue,   /* ptr to value to convert */
    DIST_NAME_TYPE type            /* T_DIST_UINT64 or T_DIST_DOUBLE */
    )
    {
#if _BYTE_ORDER==_LITTLE_ENDIAN
    uint32_t tmp;
#endif

#if !XFLOAT
    UNUSED_ARG(type);
#endif

#ifndef _BYTE_ORDER
#   error "no byte order specified !!"
#endif

#if _BYTE_ORDER==_LITTLE_ENDIAN
    
# if XFLOAT

    if (type == T_DIST_DOUBLE)
        {
        networkValue[0] = ntohl (networkValue[0]);
        networkValue[1] = ntohl (networkValue[1]);
        }
    else
        {
        tmp=networkValue[0];
        networkValue[0] = ntohl (networkValue[1]);
        networkValue[1] = ntohl (tmp);
        }

# else

    tmp=networkValue[0];
    networkValue[0] = ntohl (networkValue[1]);
    networkValue[1] = ntohl (tmp);

# endif /* XFLOAT */
#endif /* _BYTE_ORDER test */

    return networkValue;
    }
