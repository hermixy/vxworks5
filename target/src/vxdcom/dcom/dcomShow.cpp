/* dcomShow.cpp - DCOM extended show routines */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,22apr02,nel  SPR#76087. Make read routine return 0 when buffer empty
                 instead of ee to make sure that long loops don't occur.
01i,18feb02,nel  SPR#73368. Add mutex to packet processing routines to prevent
                 mixed output to telnet/serial port.
01h,17dec01,nel  Add include symbol for diab build.
01g,10dec01,dbs  diab build
01f,06dec01,nel  Add listener socket to enable retrieval of information via
                 telnet.
01e,19oct01,nel  Correct minor typo.
01d,03oct01,nel  Add in direction indication.
01c,28sep01,nel  Add remaining code comments.
01b,25sep01,nel  Correct logic in ReadBytes routine.
01a,20aug01,nel  written
*/

/*
DESCRIPTION
This module implements a packet decoding mechanism that allows VxDCOM network
traffic to be displayed. Network data is captured directly before any
marshalling/unmarshalling. This data is then decoded by a separate algorithm
from the VxDCOM marshalling/unmarshalling code.

All system GUIDs are decoded into their symbolic representations. To have
GUIDs decoded for user CoClasses the COM Track mechanism must be enabled. The
following should be added to the build settings for each CoClass:

-DVXDCOM_COMTRACK_LEVEL=1

There are no user callable functions in this module. Including the symbol
INCLUDE_DCOM_SHOW in the kernel will enable the facility and all received
DCOM network traffic will be decoded and output. Output goes by default
to the console device.

If the configuration parameter VXDCOM_DCOM_SHOW_PORT is set to a valid,
unused port number a telnet style client can be connected to this port
and all output will be directed to this client. Only one client can be
connected at one time. If a port number of zero is given then this facility
will be disabled and all output will be directed to the console device.

This facility carries a high performance penalty and is only intended
to provide debug information to Wind River Customer Support. It should
not be included otherwise.

*/

/* includes */

#include <stdio.h>
#include <string>
#include <iostream>

#include "ReactorTypes.h"

#include "vxidl.h"
#include "private/comMisc.h"
#include "private/DebugHooks.h"
#include "rpcDceProto.h"
#include "comShow.h"
#include "private/comStl.h"
#include "dcomLib.h"
#include "taskLib.h"

#include "opccomn.h"
#include "opcda.h"
#include "opc_ae.h"
#include "orpc.h"
#include "OxidResolver.h"
#include "RemoteActivation.h"
#include "RemUnknown.h"
#include "rpcDceProto.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_dcomShow (void)
    {
    return 0;
    }

/* defines */

/* Entry in the lookup table */
#define TABLE_ENTRY(name) {#name, name}

/* Entry in common GUID search table */
#define GUID_STR(c) {&c, #c}

/* Output and indented packet header */
#define NAME(n) RpcIndentBlock _name (n)

/* Output a field name, doesn't increase indent */
#define FIELD(n,v) {RpcName _name(n); output(v, false); _name.dummy (); }

/* Common virtual functions defined in all the base classes of DataPacket */
#define COMMON_VIRTUALS()						  \
    private:								  \
    virtual BYTE ReadBYTE						  \
    	(								  \
	const char *	pName,						  \
	TABLE *		pDecodeTable = NULL				  \
	) = NULL;							  \
    virtual void ReadARRAY						  \
    	(								  \
	const char *	 pName,						  \
	ULONG 		 number) = NULL;				  \
    virtual USHORT ReadUSHORT						  \
	(								  \
	const char *		pName,					  \
	bool			swap = true,				  \
	TABLE *			pDecodeTable = NULL,			  \
	bool			doEndOfLine = true			  \
	) = NULL;							  \
    virtual ULONG ReadULONG						  \
	(								  \
	const char *		name,					  \
	bool			swap = true,				  \
	TABLE *			pDecodeTable = NULL			  \
	) = NULL;							  \
    virtual void setFlags (const BYTE flags) = NULL;			  \
    virtual BYTE getFlags () = NULL;					  \
    virtual BYTE * ReadBytes (DWORD number, bool swap = true) = NULL;	  \
    virtual const char * getChannelBinding () = NULL;			  \
    virtual DWORD BytesRemaining () = NULL;

#define PRINT_STR(str)							  \
    if CHECKHOOK(pDcomShowPrintStr)					  \
	HOOK(pDcomShowPrintStr)(str);					  \
    else								  \
	printf (str)

/* typedefs */

typedef STL_MAP(string, GUID) CTXID_MAP;

typedef struct tagTable
    {
    char *		m_name;
    ULONG		m_value;
    } TABLE;

typedef struct _GUID_ELEM
    {
    const GUID *	guid;
    const char *	pName;
    } GUID_ELEM;

/* locals */

/* Mutext used to lock critcal sections */
static VxMutex s_mutex;

/* presCtxID lookp table */
static CTXID_MAP ctxIdMap;

/* Message name lookup table */
static TABLE rpcCnTypesTable [] = {
			    TABLE_ENTRY(RPC_CN_PKT_REQUEST),
			    TABLE_ENTRY(RPC_CN_PKT_RESPONSE),
			    TABLE_ENTRY(RPC_CN_PKT_FAULT),
			    TABLE_ENTRY(RPC_CN_PKT_BIND),
			    TABLE_ENTRY(RPC_CN_PKT_BIND_ACK),
			    TABLE_ENTRY(RPC_CN_PKT_BIND_NAK),
			    TABLE_ENTRY(RPC_CN_PKT_ALTER_CONTEXT),
			    TABLE_ENTRY(RPC_CN_PKT_ALTER_CONTEXT_RESP),
			    TABLE_ENTRY(RPC_CN_PKT_AUTH3),
			    TABLE_ENTRY(RPC_CN_PKT_SHUTDOWN),
			    TABLE_ENTRY(RPC_CN_PKT_REMOTE_ALERT),
			    TABLE_ENTRY(RPC_CN_PKT_ORPHANED),
			    {NULL, 0}
			   };

/* Common GUID lookup table */
static GUID_ELEM guidList [] = 
                    { GUID_STR(GUID_NULL),
		      GUID_STR(CLSID_StdMarshal),
		      GUID_STR(IID_IWindTypes),
		      GUID_STR(IID_IUnknown),
		      GUID_STR(IID_IClassFactory),
		      GUID_STR(IID_IMultiQI),
		      GUID_STR(IID_IRegistry),
		      GUID_STR(IID_IEnumGUID),
		      GUID_STR(IID_IEnumString),
		      GUID_STR(IID_IEnumUnknown),
		      GUID_STR(IID_IMalloc),
		      GUID_STR(IID_IConnectionPoint),
		      GUID_STR(IID_IConnectionPointContainer),
		      GUID_STR(IID_IEnumConnections),
		      GUID_STR(IID_IEnumConnectionPoints),
		      GUID_STR(IID_IOPCEventServer),
		      GUID_STR(IID_IOPCEventSubscriptionMgt),
		      GUID_STR(IID_IOPCEventAreaBrowser),
		      GUID_STR(IID_IOPCEventSink),
		      GUID_STR(IID_IOPCShutdown),
		      GUID_STR(IID_IOPCCommon),
		      GUID_STR(IID_IOPCServerList),
		      GUID_STR(IID_IOPCServer),
		      GUID_STR(IID_IOPCServerPublicGroups),
		      GUID_STR(IID_IOPCBrowseServerAddressSpace),
		      GUID_STR(IID_IOPCGroupStateMgt),
		      GUID_STR(IID_IOPCPublicGroupStateMgt),
		      GUID_STR(IID_IOPCSyncIO),
		      GUID_STR(IID_IOPCAsyncIO),
		      GUID_STR(IID_IOPCItemMgt),
		      GUID_STR(IID_IEnumOPCItemAttributes),
		      GUID_STR(IID_IOPCDataCallback),
		      GUID_STR(IID_IOPCAsyncIO2),
		      GUID_STR(IID_IOPCItemProperties),
		      GUID_STR(IID_IMarshal),
		      GUID_STR(IID_ISequentialStream),
		      GUID_STR(IID_IStream),
		      GUID_STR(IID_ORPCTypes),
		      GUID_STR(IID_IOrpcProxy),
		      GUID_STR(IID_IOrpcClientChannel),
		      GUID_STR(IID_IOXIDResolver),
		      GUID_STR(IID_ISystemActivator),
		      GUID_STR(IID_IRemoteActivation),
		      GUID_STR(IID_IRemUnknown),
		      GUID_STR(IID_IRpcDceTypes),
		      {{0,}, NULL}};

/* Current level of indentation */
static int indentLevel = 0;
static int s_listenPort = 0;
static int s_outputSocket = 0;

/* Forwards definitions */
extern "C" void networkPrint (const char * str);
extern "C" void startListenerTask (void);

/* Common functions */

/**************************************************************************
*
* findGuid - finds a symbolic rep of a GUID.
*
* Looks up the supplied GUID and returns a symbolic rep of it. The lookup
* first looks in an internal table of well known GUIDs. If it isn't found 
* in this it then looks it up in the list of GUIDs collected by the COM Track
* mechanism. If it isn't found in this then the GUID is converted into a 
* string and returned.
*
* RETURNS: a symbolic string rep of the given GUID.
* NOMANUAL
*/

static const char * findGuid 
    (
    REFGUID 		value	/* GUID to find */
    )
    {
    GUID_ELEM *	pPtr = &(guidList [0]);

    /* search list of well known GUIDS */
    while (pPtr->pName)
	{
	if (value == *(pPtr->guid))
	    {
	    return pPtr->pName;
	    }
	pPtr++;
	}

    /* Not in well known list so search the COMTRACK list */

    const char * pStr = VxComTrack::theInstance ()->findGUID (value);

    if (pStr)
	return pStr;

    /* All else has failed, return the GUID as a string */

    return vxcomGUID2String (value);
    }

/**************************************************************************
*
* updateCtxTable - Adds/updates a ip/port/ctx id to the table.
*
* This function adds an ip/port/ctx id to the table. The ip/port combination
* is unique to a channel and the ctx id is unique to a channel.
*
* RETURNS: nothing.
* NOMANUAL
*/

static void updateCtxTable 
    (
    USHORT		presCtxId, 	/* context id */
    GUID		id, 		/* GUID attached to ctx id */
    const char *	pChannelBinding	/* ip/port combination */
    )
    {
    string	key;

    key += pChannelBinding;
    key += ":";
    key += presCtxId;

    ctxIdMap [key] = id;
    }

/**************************************************************************
*
* getFromCtxTable - get's the GUID from the map.
*
* This function retrieves a GUID from the map based on ip/port/ctx id. If 
* the combination isn't present the string "UNKNOWN" is returned,
* otherwise the output from findGUID is returned.
*
* RETURNS: The symbolic rep of the GUID or "UNKNOWN".
* NOMANUAL
*/

static const char * getFromCtxTable
    (
    USHORT		presCtxId,	/* Context id */
    const char *	pChannelBinding	/* ip/port binding */
    )
    {
    string		key;
    CTXID_MAP::iterator	it;

    key = pChannelBinding;
    key += ":";
    key += presCtxId;

    it = ctxIdMap.find (key);

    if (it == ctxIdMap.end ())
	{
	return "UNKNOWN";
	}
    return findGuid (it->second);
    }

/* Output functions */

/**************************************************************************
*
* indent - output the correct number of spaces to indent a line.
*
* Output the correct number of spaces to indent a line.
*
* RETURNS: None.
* NOMANUAL
*/

static void indent ()
    {
    int		i;
    for (i = 0; i < indentLevel; i++)
	{
	PRINT_STR(" ");
	}
    }

/**************************************************************************
*
* indent - increases the current indent level by four.
*
* Increases the current indent level by four.
*
* RETURNS: None.
* NOMANUAL
*/

static void padUp ()
    {
    indentLevel += 4;
    }

/**************************************************************************
*
* padDown - decreases the current indent level by four.
*
* Decreases the current indent level by four.
*
* RETURNS: None.
* NOMANUAL
*/

static void padDown ()
    {
    indentLevel -= 4;
    if (indentLevel < 0)
	indentLevel = 0;
    }

/**************************************************************************
*
* output - output a string, with optional indent.
*
* This function output's a string. If the indentLine parameter is set the
* current indent number of spaces are output.
*
* RETURNS: None.
* NOMANUAL
*/

static void output
    (
    const char * 	pValue, 		/* Data to output */
    bool 		indentLine = false	/* Indent line or not */
    )
    {
    if (indentLine)
	indent ();
    PRINT_STR(pValue);
    }

/**************************************************************************
*
* output - output a BYTE, with optional indent.
*
* This function output's a BYTE. If the indentLine parameter is set the
* current indent number of spaces are output.
*
* RETURNS: None.
* NOMANUAL
*/

static void output
    (
    const BYTE 		value, 			/* Data to output */
    bool 		indentLine = false	/* Indent line or not */
    )
    {
    char line [3];

    if (indentLine)
	indent ();
    sprintf (line, "%02X", ((int)value) & 0xFF);
    PRINT_STR(line);
    }

/**************************************************************************
*
* output - output an array of BYTE, with optional indent.
*
* This function output's an array of BYTE. If the indentLine parameter is 
* set the current indent number of spaces are output.
*
* RETURNS: None.
* NOMANUAL
*/

static void output
    (
    const BYTE 		value [], 		/* Data to output */
    int 		length, 		/* Number of elements */
    bool 		indentLine = false	/* Indent line or not */
    )
    {
    int		i;

    if (indentLine)
	indent ();
    for (i = 0; i < length; i++)
	output (value [i]);
    }

/**************************************************************************
*
* output - output a USHORT, with optional indent.
*
* This function output's a USHORT. If the indentLine parameter is set the
* current indent number of spaces are output.
*
* RETURNS: None.
* NOMANUAL
*/

static void output
    (
    const USHORT 	value, 			/* Data to output */
    bool 		indentLine = false	/* Indent line or not */
    )
    {
    char	line [5];
    if (indentLine)
	indent ();
    sprintf (line, "%04X", ((int)value) & 0xFFFF);
    PRINT_STR(line);
    }

/**************************************************************************
*
* output - output a ULONG, with optional indent.
*
* This function output's a ULONG. If the indentLine parameter is set the
* current indent number of spaces are output.
*
* RETURNS: None.
* NOMANUAL
*/

static void output
    (
    const ULONG 	value, 			/* Data to output */
    bool 		indentLine = false	/* Indent line or not */
    )
    {
    char	line [9];

    if (indentLine)
	indent ();
    sprintf (line, "%08lX", value);
    PRINT_STR(line);
    }

/**************************************************************************
*
* output - output an end of line.
*
* Output's an end of line character.
*
* RETURNS: None.
* NOMANUAL
*/

static void endOfLine ()
    {
    PRINT_STR ("\n");
    }

/**************************************************************************
*
* output - output a space.
*
* Output's a space.
*
* RETURNS: None.
* NOMANUAL
*/

static void outputSpace ()
    {
    PRINT_STR (" ");
    }

/**************************************************************************
*
* output - output a dash.
*
* Output's a dash.
*
* RETURNS: None.
* NOMANUAL
*/

static void outputDash ()
    {
    PRINT_STR ("-");
    }

/* Utility class to display and indent a field name */

class RpcName
    {
    public:

	/*****************************************************************
	*
	* RpcName::RpcName - Constructor.
	*
	* Output's the given name and indents if required.
	*
	* RETURNS: None.
	* NOMANUAL
	*/
	RpcName
	    (
	    const char *	pName, 		/* Field name */
	    bool 		indent = false	/* Indent this name? */
	    )
	    {
	    output (pName, true);
	    output (":", false);
	    if (indent)
		padUp ();
	    }

	/*****************************************************************
	*
	* RpcName::RpcName - dummy.
	*
	* Dummy function that is used to get rid of a compiler warning.
	*
	* RETURNS: None.
	* NOMANUAL
	*/
	void dummy () {}
    };

/*
Utility class to display and indent a field name. The indentation is 
removed by the destructor
*/
   
class RpcIndentBlock : RpcName
    {
    public:

	/*****************************************************************
	*
	* RpcIndentBlock::RpcIndentBlock - Constructor.
	*
	* Output's the given name and indents it.
	*
	* RETURNS: None.
	* NOMANUAL
	*/
	RpcIndentBlock (const char * pName) : 
	    RpcName (pName, true)
	    {
	    endOfLine ();
	    }

	/*****************************************************************
	*
	* RpcIndentBlock::~RpcIndentBlock - Destructor.
	*
	* Removes the indentation that the constructor applied.
	*
	* RETURNS: None.
	* NOMANUAL
	*/
	~RpcIndentBlock ()
	    {
	    padDown ();
	    }

    };

/* DCE packet classes */

/*
These classes all have one method called Read which decodes a part of the
packet. 
*/

class RpcGUID 
    {
    public:

    const GUID * Read (const char * pName)
	{
	static GUID		value;

	value.Data1 = *(DWORD *)ReadBytes (sizeof(DWORD));
	value.Data2 = *(USHORT *)ReadBytes (sizeof (USHORT));
	value.Data3 = *(USHORT *)ReadBytes (sizeof (USHORT));
	memcpy (value.Data4, ReadBytes (8), 8);

	FIELD(pName, findGuid (value));
	endOfLine ();
	return &value;
	}

    COMMON_VIRTUALS();
    };

class RpcCnPresSyntaxId : RpcGUID
    {
    public:

    const rpc_cn_pres_syntax_id_t * Read (const char * pName)
	{
	static rpc_cn_pres_syntax_id_t value;

	NAME(pName);
	value.id = *RpcGUID::Read ("id");
	value.version = ReadULONG ("version");
	return &value;
	}

    COMMON_VIRTUALS();
    };

class RpcCnPresCtxElem : RpcCnPresSyntaxId
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
	BYTE i;

	USHORT presCtxId = ReadUSHORT ("presCtxId");
    	BYTE count = ReadBYTE ("nTransferSyntaxes");
	ReadBYTE ("reserved");

	updateCtxTable (presCtxId, 
		        RpcCnPresSyntaxId::Read("abstractSyntax")->id,
		        getChannelBinding ());

	for (i = 0; i < count; i++)
	        {
		char line [25];
		sprintf (line, "transferSyntax [%d]", i);
    		RpcCnPresSyntaxId::Read (line);
		}
    	}

    COMMON_VIRTUALS();
    };

class RpcCnPresCtxList : RpcCnPresCtxElem
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

	BYTE	i;
    	BYTE	count = ReadBYTE ("numCtxElems");
	ReadBYTE ("reserved");
	ReadUSHORT ("reserved2");

	for (i = 0; i < count; i++)
	    RpcCnPresCtxElem::Read ("presCtxElem [1]");
    	}

    COMMON_VIRTUALS();
    };

class RpcCnBindHdr : RpcCnPresCtxList
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

	ReadUSHORT ("maxTxFrag");
	ReadUSHORT ("maxRxFrag");
	ReadULONG  ("assocGroupId");

	RpcCnPresCtxList::Read ("presCtxList");
    	};

    COMMON_VIRTUALS();
    };

class RpcCnPresCtxId
    {
    public:

    USHORT Read (const char * pName)
	{
    	USHORT presCtxId = ReadUSHORT (pName, true, NULL, false);
	output (" - ");
	output (getFromCtxTable (presCtxId, getChannelBinding ()));
	endOfLine ();
	return presCtxId;
	}

    COMMON_VIRTUALS ();
    };

class RpcCnResponseHdr : RpcCnPresCtxId
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
    	ReadULONG ("allocHint");
	RpcCnPresCtxId::Read ("presCtxId");
    	ReadBYTE ("alertCount");
    	ReadBYTE ("reserved");
	}

    COMMON_VIRTUALS();
    };


class RpcCnRequestHdr : RpcGUID, RpcCnPresCtxId
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

    	ReadULONG ("allocHint");
	RpcCnPresCtxId::Read ("presCtxId");
	ReadUSHORT ("methodNum");

	if (getFlags () & RPC_CN_FLAGS_OBJECT_UUID)
	    {
	    RpcGUID::Read ("objectId");
	    }
	}

    COMMON_VIRTUALS();
    };

class RpcCnPortAny
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
    	ReadUSHORT ("len");
    	ReadARRAY ("addr", 6);
	}

    COMMON_VIRTUALS();
    };

class  RpcCnPresResult : RpcCnPresSyntaxId
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
    	ReadUSHORT ("result");
    	ReadUSHORT ("reason");
	RpcCnPresSyntaxId::Read ("transferSyntax");
	}

    COMMON_VIRTUALS();
    };

class RpcCnPresResultList : RpcCnPresResult
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
	BYTE 		i;
    	BYTE 		numResults = ReadBYTE ("numResults");

        ReadBYTE ("reserved");
    	ReadUSHORT ("reserved2");

	for (i = 0; i < numResults; i++)
	    {
	    char line [20];
	    sprintf (line, "presResult [%d]", i);
	    RpcCnPresResult::Read (line);
	    }
	}

    COMMON_VIRTUALS();
    };

class  RpcCnBindAckHdr : RpcCnPortAny, RpcCnPresResultList
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
	ReadUSHORT ("maxTxFrag");
	ReadUSHORT ("maxRxFrag");
	ReadULONG ("assocGroupId");

	RpcCnPortAny::Read ("secAddr");

	RpcCnPresResultList::Read ("resultList");
	}

    COMMON_VIRTUALS();
    };

class  RpcCnAlterContextRespHdr : RpcCnPresResultList
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

    	ReadUSHORT ("maxTxFrag");
    	ReadUSHORT ("maxRxFrag");
    	ReadULONG  ("assocGroupId");

    	ReadUSHORT ("secAddr");
    	ReadUSHORT ("pad");

	RpcCnPresResultList::Read ("resultList");
	}

    COMMON_VIRTUALS();
    };

class RpcCnFaultHdr : RpcCnPresCtxId
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);
	ReadULONG ("allocHint");
	RpcCnPresCtxId::Read ("presCtxId");
	ReadBYTE ("alertCount");
	ReadBYTE ("reserved");
	ReadULONG ("status");
	ReadULONG ("reserved2");
	}

    COMMON_VIRTUALS ();
    };


class RpcCnBindNakHdr
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

	ReadUSHORT ("reason");
	ReadBYTE ("numProtocols");
	ReadBYTE ("verMajor");
	ReadBYTE ("verMinor");
	}
    COMMON_VIRTUALS ();
    };


class RpcCnAuth3Hdr
    {
    public:

    void Read (const char * pName)
	{
        ReadUSHORT ("maxTxFrag");
	ReadUSHORT ("maxRxFrag");
	}

    COMMON_VIRTUALS ();
    };

class RpcCnBody
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

	BYTE *		pBlock;
	DWORD		remain;

	while ((remain = BytesRemaining ()) > 0)
	    {
	    DWORD	blockSize = (remain < BLOCK_SIZE)?remain:BLOCK_SIZE;
	    DWORD	i;

	    pBlock = ReadBytes (blockSize, false);

	    /* Dump out numeric */
	    output (pBlock [0], true);
	    outputSpace ();

	    for (i = 1; i < blockSize; i++)
		{
		output (pBlock [i]);
		outputSpace ();
		}

	    if (blockSize < BLOCK_SIZE)
		for (i = blockSize; i < BLOCK_SIZE; i++)
		    output ("   ");

	    /* Dump out text */

	    output ("\t");

	    for (i = 0; i < blockSize; i++)
		{
		if (isprint (pBlock [i]))
		    {
		    char	line [2];

		    sprintf (line, "%c", pBlock [i]);
		    PRINT_STR(line);
		    }
		else
		    output (".");
		}

	    endOfLine ();
	    }
	endOfLine ();
	}

    COMMON_VIRTUALS ();

    private:
    	enum {BLOCK_SIZE = 16};
    };

class RpcCnCommonHdr : 
	RpcCnBindHdr, 
	RpcCnBindAckHdr, 
	RpcCnAlterContextRespHdr, 
	RpcCnRequestHdr, 
	RpcCnResponseHdr,
	RpcCnFaultHdr,
	RpcCnBindNakHdr,
	RpcCnAuth3Hdr,
	RpcCnBody
    {
    public:

    void Read (const char * pName)
	{
	NAME(pName);

	BYTE	type;

	/* Decode common header */
	ReadBYTE ("rpcVersion");
	ReadBYTE ("rpcMinorVersion");
    	type = ReadBYTE ("packetType", rpcCnTypesTable);
	setFlags(ReadBYTE ("flags"));
	ReadARRAY ("drep", 4);
	ReadUSHORT ("fragLen");
	ReadUSHORT ("authLen");
	ReadULONG ("callId");

	/* Decode type header */

	switch (type)
	    {
	    case RPC_CN_PKT_REQUEST:
		RpcCnRequestHdr::Read("request");
		break;
	    case RPC_CN_PKT_RESPONSE:
		RpcCnResponseHdr::Read("response");
		break;
	    case RPC_CN_PKT_FAULT:
		RpcCnFaultHdr::Read ("fault");
		break;
	    case RPC_CN_PKT_BIND:
		RpcCnBindHdr::Read ("bind");
		break;
	    case RPC_CN_PKT_BIND_ACK:
		RpcCnBindAckHdr::Read ("bind ack");
		break;
	    case RPC_CN_PKT_BIND_NAK:
		RpcCnBindNakHdr::Read ("bind nak");
		break;
	    case RPC_CN_PKT_ALTER_CONTEXT:
		RpcCnBindHdr::Read ("alter context");
		break;
	    case RPC_CN_PKT_ALTER_CONTEXT_RESP:
		RpcCnAlterContextRespHdr::Read ("alter context resp");
		break;
	    case RPC_CN_PKT_AUTH3:
		RpcCnAuth3Hdr::Read ("auth3");
		break;
	    case RPC_CN_PKT_SHUTDOWN:
	    case RPC_CN_PKT_REMOTE_ALERT:
	    case RPC_CN_PKT_ORPHANED:
	    default:
		/* no body to these packets, just a common hdr */
		break;
	    }

	/* Dump out remaining data */

	RpcCnBody::Read ("body");
	}

    COMMON_VIRTUALS();
    };

/* Class to encapsulate the data packet */

class DataPacket : RpcCnCommonHdr
    {
    public:

    /*************************************************************************
    *
    * DataPacket::DataPacket - Constructor
    *
    * This constructor initializes the class, works out the endianess of the 
    * target for byte swapping and stores a copy of the data buffer.
    *
    * RETURNS: None.
    * NOMANUAL
    */

    DataPacket 
	(
	const BYTE * 	pData, 		/* The packet */
	DWORD 		length, 	/* The length of the packet */
	const char * 	pHost, 		/* The ip address of the end point */
	int 		hostPort, 	/* The port on the target */
	int		peerPort,	/* The port on the end point */
	bool		outbound	/* Direction of connection, */
					/* true is outbound */
	) : m_hostPort (hostPort),
            m_peerPort (peerPort),
	    m_outbound (outbound),
	    m_pResult (0),
	    m_pBinding (0)
        {
	/* Work out byte swap order */
	USHORT	testOrder = 0x1234;
	BYTE *	pTestOrder = (BYTE *)(&testOrder);

        if (*pTestOrder == 0x12)
	    {
	    m_byteSwap = true;
	    }
	else
	    {
	    m_byteSwap = false;
	    }

	/* Store data and work out the end of the buffer */
	m_pData = pData;
	m_pEnd = const_cast<BYTE *>(m_pData) + length;

	m_pHost = new char [strlen (pHost) + 1];
	strcpy (m_pHost, pHost);
	}

    /*************************************************************************
    *
    * DataPacket::~DataPacket - Destructor
    *
    * Deletes any temp buffers
    *
    * RETURNS: None.
    * NOMANUAL
    */

    virtual ~DataPacket ()
        {
	/* Clean up any allocated temp buffer */
	if (m_pResult)
	    {
	    delete [] m_pResult;
	    m_pResult = NULL;
	    }

	if (m_pBinding)
	    {
	    delete [] m_pBinding;
	    m_pBinding = NULL;
	    }

	delete [] m_pHost;
	}

    /*************************************************************************
    *
    * DataPacket::Read - Starts decode of the packet.
    *
    * This method starts the decode of the packet. It actually just starts a 
    * new line and then passes the rest of the job to the common header class.
    *
    * RETURNS: None.
    * NOMANUAL
    */

    void Read 
	(
	const char * 	pName		/* Name to print out  for this step */
	)
	{
	endOfLine ();
	RpcCnCommonHdr::Read (pName);
	}

    /**************************************************************************
    *
    * DataPacket::getDesc - Returns the binding ip/port binding for this
    * packet.
    *
    * This method gives a channel binding as a string containing ip and port
    * addresses. For outbound bindings it is of the format:
    *     ip:host port:peer port
    * and for inbound transactions it is of the form:
    *     ip:peer port:host port
    *
    * RETURNS: The ip/port binding for the packet as a string.
    * NOMANUAL
    */

    const char * getDesc ()
	{
	if (m_pBinding)
	    {
	    delete [] m_pBinding;
	    m_pBinding = NULL;
	    }
	m_pBinding = new char [strlen (m_pHost) + 50];
	if (m_outbound)
	    {
	    sprintf (m_pBinding,
		    "From target port %d to %s port %d",
		    m_hostPort, m_pHost, m_peerPort);
	    }
	else
	    {
	    sprintf (m_pBinding,
		    "From %s port %d to target port %d",
		    m_pHost, m_peerPort, m_hostPort);
	    }
	return m_pBinding;
	}

    /*************************************************************************
    *
    * DataPacket::getChannelBinding - Returns the binding ip/port binding for 
    * this packet.
    *
    * This method gives a channel binding as a string containing ip and port
    * addresses. It is always of the form:
    *     ip:host port:peer port
    * so that easy string comps can be done. It is virtual so that the base 
    * classes can access it.
    *
    * RETURNS: The ip/port binding for the packet as a string.
    * NOMANUAL
    */

    virtual const char * getChannelBinding ()
	{
	if (m_pBinding)
	    {
	    delete [] m_pBinding;
	    m_pBinding = NULL;
	    }
	m_pBinding = new char [strlen (m_pHost) + 32];
	sprintf (m_pBinding, "%s:%d:%d", m_pHost, m_hostPort, m_peerPort);
	return m_pBinding;
	}

    /*************************************************************************
    *
    * DataPacket::getFlags - Return the stored flags BYTE.
    *
    * Return the stored flags BYTE.
    *
    * RETURNS: The DCE header flags byte.
    * NOMANUAL
    */

    BYTE getFlags ()
	{
	return m_DceFlags;
	}

    /*************************************************************************
    *
    * DataPacket::setFlags - Set the stored flags BYTE.
    *
    * Set the stored flags BYTE.
    *
    * RETURNS: Nothing.
    * NOMANUAL
    */

    void setFlags (const BYTE flags)
	{
	m_DceFlags = flags;
	}

    private:

    /*************************************************************************
    *
    * DataPacket::ReadBYTE - Read a BYTE from the packet.
    *
    * This method reads a BYTE from the packet and advances the current byte
    * pointer by one BYTE. The value is also displayed. If a decode table is 
    * given the value is matched against the table and the symbolic value is
    * displayed.
    *
    * RETURNS: The value read from the packet.
    * NOMANUAL
    */

    virtual BYTE ReadBYTE
        (
	const char *	pName, 			/* The field name to */
							/* display */
	TABLE *		pDecodeTable = NULL	/* The decode table, */
							/* or NULL for none. */
	)	
	{
	BYTE * pValue = ReadBytes (sizeof (BYTE));

	FIELD(pName, *pValue);

	if (pDecodeTable)
	    {
	    outputSpace ();
	    outputDash ();
	    outputSpace ();
	    output (DecodeFromTable ((ULONG)(*pValue), pDecodeTable));
	    }
	endOfLine ();
	return *pValue;
	}

    /*************************************************************************
    *
    * DataPacket::ReadARRAY - Read an array of BYTE from the packet.
    *
    * This method reads an array of BYTE from the packet and advances the 
    * current byte pointer by the length of the read array. The data is also 
    * displayed.
    *
    * RETURNS: Nothing.
    * NOMANUAL
    */

    virtual void ReadARRAY
        (
	const char * 	pName, 	/* The field name to display */
	ULONG 		number	/* The number of BYTEs to read. */
	)
	{
	ULONG	count;
	BYTE *	pValue = ReadBytes (number, false);

	FIELD(pName, "");

	for (count = 0; count < number; count++)
	    {
	    outputSpace ();
	    output (pValue [count]);
	    }
	endOfLine ();
	}


    /*************************************************************************
    *
    * DataPacket::ReadUSHORT - Read a USHORT from the packet.
    *
    * This method reads a USHORT from the packet and advances the current byte
    * pointer by one USHORT. The value is also displayed. If a decode table 
    * is given the value is matched against the table and the symbolic value
    * is displayed. The data is correctly byte swapped if required and an EOL
    * is emited if required.
    *
    * RETURNS: The value read from the packet.
    * NOMANUAL
    */

    virtual USHORT ReadUSHORT
	(
	const char *	pName, 			/* Name of the field */
	bool		swap = true, 		/* If true swap data */
	TABLE *		pDecodeTable = NULL,	/* Pointer to the decode */
						/* table, or NULL for none */
	bool		doEndOfLine = true	/* If false emit and EOL */
	)
	{
	USHORT * pValue = (USHORT *)ReadBytes (sizeof (USHORT), swap);

	FIELD(pName, *pValue);

	if (pDecodeTable)
	    {
	    outputSpace ();
	    outputDash ();
	    outputSpace ();
	    output (DecodeFromTable ((ULONG)(*pValue), pDecodeTable));
	    }
	if (doEndOfLine)
	    endOfLine ();
	return *pValue;
	}

    /**************************************************************************
    *
    * DataPacket::ReadULONG - Read a ULONG from the packet.
    *
    * This method reads a ULONG from the packet and advances the current byte
    * pointer by one ULONG. The value is also displayed. If a decode table is 
    * given the value is matched against the table and the symbolic value is
    * displayed. The data is correctly byte swapped if required.
    *
    * RETURNS: The value read from the packet.
    * NOMANUAL
    */

    virtual ULONG ReadULONG
	(
	const char * 	pName, 			/* Name of the field */
	bool 		swap = true, 		/* If PURE swap data */
	TABLE * 	pDecodeTable = NULL	/* Pointer to the decode */
						/* table, or NULL for none */
	)
	{
	ULONG * pValue = (ULONG *)ReadBytes (sizeof (ULONG), swap);

	FIELD(pName, *pValue);

	if (pDecodeTable)
	    {
	    outputSpace ();
	    outputDash ();
	    outputSpace ();
	    output (DecodeFromTable (*pValue, pDecodeTable));
	    }
	endOfLine ();
	return *pValue;
	}

    /**************************************************************************
    *
    * DataPacket::ReadBytes - Reads n bytes from the packet
    *
    * Reads n bytes from the packet and performs byte swamping if swap is set 
    * to true. If all data has been read 0x00 bytes are returned in the buffer.
    *
    * RETURNS: A pointer to a temp buffer containing the formated bytes.
    * NOMANUAL
    */

    virtual BYTE * ReadBytes
    	(
	DWORD		number, 		/* Number of BYTEs to read */
	bool 		swap = true		/* Swap bytes if true. */
	)
        {
	if (m_pResult)
	    {
	    delete [] m_pResult;
	    m_pResult = NULL;
	    }

	m_pResult = new BYTE [number];

	COM_ASSERT (m_pResult != NULL);

	memset (m_pResult, 0x00, number);

	BYTE *	ptr;
	DWORD	count;

	/* since we're copying data we can swap data 'in-place',    */
	/* so we may need to start filling in the buffer at the end */
	if (m_byteSwap && swap)
	    {
	    ptr = m_pResult + number - 1;
	    }
	else
	    {
	    ptr = m_pResult;
	    }

        for (count = 0; count < number; count++)
	    {
	    if (m_pData < m_pEnd)
		{
		*ptr = *m_pData;
		m_pData++;

		/* increment/decrement count depending on swap order */
		/* and whether we want to swap. */
		if (m_byteSwap && swap)
		    {
		    ptr--;
		    }
		else
		    {
		    ptr++;
		    }
		}
	    else
		{
		return m_pResult;
		}
	    }

	return m_pResult;
	}

    /**************************************************************************
    *
    * DataPacket::BytesRemaining - Number of BYTEs unread in buffer.
    *
    * Returns number of unread BYTEs in the buffer.
    *
    * RETURNS: Bytes remaining.
    * NOMANUAL
    */

    virtual DWORD BytesRemaining ()
	{
	return m_pEnd - m_pData;
	}

    /**************************************************************************
    *
    * DataPacket::DecodeFromTable - Converts a numeric value to a string via
    * a table.
    *
    * Converts a numeric value to a string via a lookup table. If the value 
    * doesn't exist in the table "Unknown Value" is returned.
    *
    * RETURNS: The string contained in the lookup table or "Unknown Value" 
    * if not found.
    * NOMANUAL
    */

    const char * DecodeFromTable
    	(
	ULONG 		value, 		/* Value to decode */
	TABLE * 	pDecodeTable	/* Decode table */
	)
	{
	TABLE *		ptr = pDecodeTable;

	while (ptr->m_name)
	    {
	    if (ptr->m_value == value)
		{
		return ptr->m_name;
		}
	    ptr++;
	    }
	return "Unknown value";
	}

    const BYTE *	m_pData;
    BYTE *		m_pEnd;
    bool		m_byteSwap;
    BYTE		m_DceFlags;
    char *		m_pHost;
    int			m_hostPort;
    int			m_peerPort;
    bool		m_outbound;
    BYTE *		m_pResult;
    char *		m_pBinding;
    };

/**************************************************************************
*
* dcomShowClientOutput - Processes packet sent from a client object to
* a server.
*
* Processes an outbound packet from a client object.
*
* RETURNS: Nothing.
* NOMANUAL
*/


extern "C" void dcomShowClientOutput
    (
    const BYTE * 	pPacket, 	/* Packet to be sent */
    DWORD 		length, 	/* Length of packet */
    const char * 	pHost, 		/* Hostname */
    int			hostPort,	/* Host endpoint */
    int			peerPort	/* Peer endpoint */
    )
    {
    VxCritSec exclude (s_mutex);

    DataPacket data(pPacket, length, pHost, hostPort, peerPort, true);

    data.Read (data.getDesc ());
    }

/**************************************************************************
*
* dcomShowClientInput - Processes packet sent to a client object from a
* server object.
*
* Processes an inbound packet to a client object.
*
* RETURNS: Nothing.
* NOMANUAL
*/

extern "C" void dcomShowClientInput
    (
    const BYTE * 	pPacket, 	/* Packet to be sent */
    DWORD 		length, 	/* Length of packet */
    const char * 	pHost, 		/* Hostname */
    int			hostPort,	/* Host endpoint */
    int			peerPort	/* Peer endpoint */
    )
    {
    VxCritSec exclude (s_mutex);

    DataPacket data(pPacket, length, pHost, hostPort, peerPort, false);

    data.Read (data.getDesc ());
    }

/**************************************************************************
*
* dcomShowServerOutput - Processes packet sent from a server object to a
* client object.
*
* Processes an outbound packet from a server object.
*
* RETURNS: Nothing.
* NOMANUAL
*/

extern "C" void dcomShowServerOutput
    (
    const BYTE * 	pPacket, 	/* Packet to be sent */
    DWORD 		length, 	/* Length of packet */
    const char * 	pHost, 		/* Hostname */
    int			hostPort,	/* Host endpoint */
    int			peerPort	/* Peer endpoint */
    )
    { 
    VxCritSec exclude (s_mutex);

    DataPacket data(pPacket, length, pHost, hostPort, peerPort, true);

    data.Read (data.getDesc ());
    }

/**************************************************************************
*
* dcomShowServerInput - Processes packet sent to a server object.
*
* Processes an inbound packet from a server object.
*
* RETURNS: Nothing.
* NOMANUAL
*/

extern "C" void dcomShowServerInput
    (
    const BYTE * 	pPacket, 	/* Packet to be sent */
    DWORD 		length, 	/* Length of packet */
    const char * 	pHost, 		/* Hostname */
    int			hostPort,	/* Host endpoint */
    int			peerPort	/* Peer endpoint */
    )
    {
    VxCritSec exclude (s_mutex);

    DataPacket data(pPacket, length, pHost, hostPort, peerPort, false);

    data.Read (data.getDesc ());
    }

/**************************************************************************
*
* listenerTask - task that listens for network connections
*
* Task that listens for network connections. If a socket is successfully
* established the appropriate debug hook is initialized.
*
* RETURNS: Nothing.
* NOMANUAL
*/

extern "C" void listenerTask
    (
    int			arg1,
    int			arg2,
    int			arg3,	
    int			arg4,
    int			arg5,
    int			arg6,
    int			arg7,
    int			arg8,
    int			arg9,
    int			arg10
    )
    {
#ifdef VXDCOM_PLATFORM_VXWORKS
    int			fromLen;
#else
    socklen_t		fromLen;
#endif
    int			s;
    int			ns;
    struct sockaddr_in	s_addr;

    PRINT_STR("DCOM_SHOW setting up socket\n");

    if ((s = ::socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
	PRINT_STR("DCOM_SHOW can't open a socket\n");
        return;
	}

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(s_listenPort);
    s_addr.sin_addr.s_addr = htonl (INADDR_ANY);

    PRINT_STR("DCOM_SHOW binding socket to port\n");
    if (bind(s, (struct sockaddr *)(&s_addr), sizeof (s_addr)) < 0)
	{
	PRINT_STR("DCOM_SHOW can't bind to specified port\n");
	close (s);
	return;
	}
    
    PRINT_STR("DCOM_SHOW listening for a connection\n");
    if (listen(s, 5) < 0)
	{
        PRINT_STR("DCOM_SHOW can't listen on specified port\n");
	close (s);
	return;
	}

    fromLen = sizeof (s_addr);
    if ((ns = accept(s, (struct sockaddr *)(&s_addr), &fromLen)) < 0)
	{
	PRINT_STR("DCOM_SHOW has dropped out of accept without connecting\n");
	close (s);
	startListenerTask ();
	return;
	}

    close(s);

    s_outputSocket = ns;

    PRINT_STR("DCOM_SHOW has set up a connection\n");
    SETDEBUGHOOK(pDcomShowPrintStr, networkPrint);
    }

/**************************************************************************
*
* networkPrint - output a string to a socket
*
* Output's a string to a network socket. If an error occurs it is assumed
* that the socket has been closed by the client and so a new connection
* listener is started.
*
* RETURNS: Nothing.
* NOMANUAL
*/

extern "C" void networkPrint (const char * str)
    {
    if (write (s_outputSocket, const_cast<char *>(str), strlen (str)) < 0)
	{
	CLEARDEBUGHOOK(pDcomShowPrintStr);
	close (s_outputSocket);
	s_outputSocket = 0;
	PRINT_STR(str);
	PRINT_STR("DCOM_SHOW has disconnected from output client\n");
	startListenerTask ();
	}
    }

/**************************************************************************
*
* startListenerTask - starts the task to listen for connections
*
* Starts a task to listen for network connections.
*
* RETURNS: Nothing.
* NOMANUAL
*/

extern "C" void startListenerTask (void)
    {
    COM_ASSERT (taskSpawn ("tDcomShow", 55, VX_FP_TASK, 10240,
		 (FUNCPTR)listenerTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) != ERROR);
    }

/**************************************************************************
*
* dcomShowInit - Init routine for the DCOM Show routines. 
*
* This initializes the module and installs the required hooks into the
* client/server structure to capture and process the network data.
* 
* This routine takes one parameter which defines a network port to listen
* for telnet style connections on. If a telnet style client is connected to
* this port all output will be sent via this network connection, rather than
* to the system console.
*
* RETURNS: Nothing.
*/

extern "C" void dcomShowInit
    (
    int			listenPort	/* Port number to listen for */
    					/* connections on, 0 to disable. */
    )
    {
    SETDEBUGHOOK(pRpcClientOutput, dcomShowClientOutput);
    SETDEBUGHOOK(pRpcClientInput, dcomShowClientInput);

    SETDEBUGHOOK(pRpcServerOutput, dcomShowServerOutput);
    SETDEBUGHOOK(pRpcServerInput, dcomShowServerInput);

    /* Initialize listen port */
    if (listenPort != 0)
	{
	s_listenPort = listenPort;
	startListenerTask ();
	}

    }

