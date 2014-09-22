/* xdrwtx.c  - xdr routines for Wind River Tool eXchange protocol */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03b,03apr02,c_c  Changed xdr_wrapstring to xdr_string to workaround a xdr
                 limitation on the target (SPR #74773).
03a,15jun01,pch  Add new WTX_TS_INFO_GET_V2 service to handle WTX_RT_INFO
                 change (from BOOL32 hasFpp to UINT32 hasCoprocessor) without
                 breaking existing clients.
02z,10may01,dtr  Changing coprocessor flags into one word.
02y,06feb01,dtr  Adding probe for Altivec registers.
02x,18aug98,pcn  Re-use WTX_MSG_EVTPT_LIST_2.
02w,20jul98,pcn  Added evtptNum and toolId in the wtxEventpointListGet return
                 list.
02v,19jun98,pcn  Changed xdr_WTX_MSG_FILE_LOAD_DESC to accept new fields.
02u,26may98,pcn  Changed WTX_MSG_EVTPT_LIST_2 in WTX_MSG_EVTPT_LIST_GET.
02t,26mar98,pcn  Updated history.
02s,23mar98,pcn  WTX 2: added test in xdr_WTX_MSG_FILE_LOAD_DESC to take care 
		 of different behaviors of wtxObjModuleLoad_2.
02r,02mar98,pcn  WTX 2: added xdr_MEM_WIDTH_COPY_ALLOC,
                 xdr_WTX_MSG_MEM_WIDTH_READ_DESC,
                 xdr_WTX_MSG_MEM_WIDTH_COPY_DESC, xdr_WTX_EVENT_NODE,
                 xdr_MSG_EVENT_LIST, xdr_WTX_MSG_FILE_LOAD_DESC,
                 xdr_WTX_LD_M_FILE_DESC, xdr_WTX_EVENT_2, xdr_WTX_EVTPT_2,
                 xdr_WTX_MSG_EVTPT_DESC_2, xdr_WTX_MSG_EVTPT_LIST_2. Changed
                 xdr_WTX_MSG_EVTPT_LIST, xdr_WTX_EVENT_DESC.
02q,13feb98,c_c  Fixed a memory leak in xdr_WTX_MSG_CONTEXT_DESC
02p,29aug97,fle  Adding the WTX_MEM_DISASSEMBLE service
02o,30sep96,elp  put in share, adapted to be compiled on target side
		 (SPR# 6775).
02n,28jun96,c_s  subvert prototypes in AIX's <rpc/xdr.h> to temporarily
                 work around problems with XDR arguments that change
                 signedness.
02m,17jun96,p_m	 changed flags type in xdr_WTX_MSG_OPEN_DESC() (SPR# 4941).
02l,10jun96,elp	 re-installed 02j (fix SPR# 6502 without protocol change).
02k,20may96,elp	 updated xdr_WTX_MSG_SYM_TBL_INFO to new WTX_MSG_SYM_TBL_INFO
		 type (SPR# 6205).
02j,31aug95,p_m  added free() in xdr_WTX_MSG_VIO_COPY_DESC() (SPR# 4804).
02i,06jun95,p_m  took care of WTX_MSG_SYM_MATCH_DESC changes in
		 xdr_WTX_MSG_SYM_MATCH_DESC().
02h,01jun95,p_m  removed WTX_MSG_CALL_PARAM and modified WTX_MSG_CONTEXT_DESC.
02g,30may95,p_m  completed WTX_MEM_SCAN and WTX_MEM_MOVE implementation.
02f,26may95,p_m  added match field in WTX_MSG_MEM_SCAN_DESC.
02e,23may95,p_m  made missing name changes.
02d,22may95,jcf  name revision.
02c,19may95,p_m  got rid of DETECTION related stuff.
02b,16may95,p_m  added xdr_WTX_MSG_KILL_DESC.
02a,14may95,c_s  fixed memory leak in xdr_WTX_MEM_XFER; fixed free-memory 
                   fault in xdr_WTX_SYM_LIST.
01z,10may95,pad  modified xdr_WTX_MSG_SYM_LIST and xdr_WTX_MSG_SYM_MATCH_DESC.
01y,09may95,p_m  re-implemented wtxregd related XDR procedures. Added Target 
		 Server version in WTX_MSG_TS_INFO.
01x,05may95,p_m  added protVersion in xdr_WTX_CORE.
01w,02may95,pad  xdr_WTX_MSG_MODULE_INFO now handles format field as char *
01v,30mar95,p_m  changed xdr_CONTEXT_ID_T() to xdr_WTX_CONTEXT_ID_T().
01u,20mar95,c_s  updated GOPHER structure filters.
01t,16mar95,p_m  added xdr_WTX_MSG_VIO_FILE_LIST().
01s,10mar95,p_m  changed endian field to u_long.
01r,03mar95,p_m  merged with wtxxdr.c.
		 added gopher routines.
01q,27feb95,p_m  cleanup up xdr_WTX_MSG_MEM_SCAN_DESC.
		 added symTblId to xdr_WTX_MSG_SYM_TBL_INFO.
		 simplified WTX_WDB_SERVICE_DESC.
01p,25feb95,jcf  extended xdr_WTX_TS_INFO.
01o,21feb95,pad  added xdr_WTX_WTX_SERVICE_DESC() and modified
		 xdr_WTX_MSG_SERVICE_DESC().
01n,08feb95,p_m  added loadFlag in WTX_MODULE_INFO.
		 changed to xdr_WTX_MSG... for more coherency.
		 added many filters now that WTX and WDB are separated.
01m,30jan95,p_m  added additional routines objects and name to 
		 xdr_SERVICE_DESC().
		 added xdr_WTX_MEM_XFER(), xdr_WTX_REG_WRITE()
		 and  xdr_WTX_MEM_SET_DESC().
01l,23jan95,p_m  updated xdr_TOOL_DESC and xdr_WTX_TS_INFO.
		 replaced #include "wdb/xdrwdb.h" with #include "xdrwdb.h".	
		 replace rtnName by initRtnName in xdr_SERVICE_DESC().
01k,21jan95,jcf  updated include files.
01j,13dec94,p_m  updated to use files generated from wtx.x.
		 added xdr_WTX_MEM_INFO().
		 added WTX_SYM_LIST to LD_M_FILE to return undefined symbols.
		 changed xdr_WTX_WRAP_STRING() to xdr_WRAPSTRING().	
		 added addlDataLen and addData fields to WTX_EVENT.
01i,17nov94,p_m  added symFlag to xdr_LD_M_FILE(). changed all routine 
		 pointers to routine names in SERVICE_DESC.
01h,14nov94,p_m  renamed VIO_OPEN_DESC to OPEN_DESC and added channel to 
		 OPEN_DESC.
01g,11nov94,p_m  removed xdr_WTX_GOPHER_TAPE.
01f,28oct94,p_m  added returnType field in WTX_CALL_PARAMS.
		 added pEvtpt field management in EVTPT_LIST.
		 added xdr_SYM_TAB_INFO() and xdr_SYM_TAB_LIST().
		 changed xdr_SYM_TAB_PARAM() to xdr_SYM_TBL_PARAM().
		 added xdr object file names in xdr_SERVICE_DESC.
		 added display in xdr_CONSOLE_DESC.
01e,27oct94,p_m  added exactName field in WTX_SYMBOL.
01d,24oct94,p_m  added xdr_WTX_WRAP_STRING() and xdr_SYM_MATCH_DESC. 
	 	 changed xdr_WTX_CALL_PARAMS().
		 added moduleId handling in xdr_LD_M_FILE().
		 added xdr_WTX_SYM_LIST().
01c,20oct94,p_m  added xdr_MEM_COPY_ALLOC() and xdr_WTX_SYMBOL(). Added 
		 moduleId handling in xdr_WTX_MODULE_INFO. Added null
		 pointer handling in xdr_MOD_NAME_OR_ID.
01b,19oct94,p_m  added xdr_pointers in xdr_WTX_TS_INFO.
01a,04oct94,p_m  written.
*/

/*
DESCRIPTION
This module contains the eXternal Data Representation (XDR) routines
for the WTX protocol.
*/

#if defined (RS6000_AIX4)
/* AIX is extremely picky about the signedness of arguments passed to 
   stock xdr rouintes.  This hack ends that debate. */
#define _NO_PROTO
#endif /* !RS6000_AIX4 */
#include <stdlib.h>

#include "rpc/rpc.h"
#include "wtx.h"
#include "wtxmsg.h"
#include "wtxxdr.h"

#define LASTUNSIGNED      ((u_int) 0-1) /* taken from SUN xdr.c file */

/*******************************************************************************
*
* xdr_WRAPSTRING - possibly NULL string pointer
*
*/

bool_t xdr_WRAPSTRING 
    (
    XDR *	xdrs,
    char **	string
    )
    {
    BOOL	moreData;

    moreData = (*string != NULL);		/* string is a NULL pointer ?*/
    if (!xdr_bool(xdrs, &moreData))		/* code/decode test result */
	return (FALSE);
    if (!moreData)
	*string = NULL;				/* YES: string is NULL */
    else 					/* NO: code/decode the string */
	{
	if (xdrs->x_op == XDR_DECODE)
	    *string = NULL;
    	if (!xdr_string (xdrs, string, LASTUNSIGNED))
	    return (FALSE);
	}
    return (TRUE);
    }

#ifdef HOST
/*******************************************************************************
*
* xdr_TGT_ADDR_T - WTX
*
*/

bool_t xdr_TGT_ADDR_T 
    (
    XDR *           xdrs,
    TGT_ADDR_T * objp
    )
    {
    if (!xdr_u_long (xdrs, objp))
	return (FALSE);

    return (TRUE);
    }
#endif

/*******************************************************************************
*
* xdr_TGT_ARG_T - WTX
*
*/

bool_t xdr_TGT_ARG_T 
    (
    XDR *       xdrs,
    TGT_ARG_T * objp
    )
    {
    if (!xdr_u_long (xdrs, objp))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_CONTEXT_ID_T - WTX
*
*/

bool_t xdr_WTX_CONTEXT_ID_T 
    (
    XDR * 	       xdrs,
    WTX_CONTEXT_ID_T * objp
    )
    {
    if (!xdr_u_long (xdrs, (u_long *)objp))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_VALUE - WTX
*
*/

bool_t xdr_WTX_VALUE
    (
    XDR *xdrs,
    WTX_VALUE *objp
    )
    {
	if (!xdr_enum(xdrs, (enum_t *)&objp->valueType)) {
		return (FALSE);
	}
	switch (objp->valueType) {
	case 0:
		if (!xdr_char(xdrs, &objp->value_u.v_int8)) {
			return (FALSE);
		}
		break;
	case 1:
		if (!xdr_short(xdrs, &objp->value_u.v_int16)) {
			return (FALSE);
		}
		break;
	case 2:
		if (!xdr_long(xdrs, (long *)&objp->value_u.v_int32)) {
			return (FALSE);
		}
		break;
	case 3:
		if (!xdr_u_char(xdrs, &objp->value_u.v_uint8)) {
			return (FALSE);
		}
		break;
	case 4:
		if (!xdr_u_short(xdrs, &objp->value_u.v_uint16)) {
			return (FALSE);
		}
		break;
	case 5:
		if (!xdr_u_long(xdrs, (u_long *)&objp->value_u.v_uint32)) {
			return (FALSE);
		}
		break;
	case 6:
		if (!xdr_double(xdrs, &objp->value_u.v_double)) {
			return (FALSE);
		}
		break;
	case 7:
		if (!xdr_long(xdrs, (long *)&objp->value_u.v_bool32)) {
			return (FALSE);
		}
		break;
	case 8:
		if (!xdr_WRAPSTRING(xdrs, &objp->value_u.v_pchar)) {
			return (FALSE);
		}
		break;

	/* XXX p_m treat void * as a long for now */

	case 9:
		if (!xdr_long(xdrs, (long *)&objp->value_u.v_pvoid)) {
			return (FALSE);
		}
		break;
	case 10:
		if (!xdr_TGT_ADDR_T(xdrs, &objp->value_u.v_tgt_addr)) {
			return (FALSE);
		}
		break;
	case 11:
		if (!xdr_TGT_ARG_T(xdrs, &objp->value_u.v_tgt_arg)) {
			return (FALSE);
		}
		break;
	}
	return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_RESULT - WTX
*
*/

bool_t xdr_WTX_MSG_RESULT 
    (
    XDR *           xdrs,
    WTX_MSG_RESULT * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_VALUE (xdrs, &objp->val))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_PARAM - WTX
*
*/

bool_t xdr_WTX_MSG_PARAM 
    (
    XDR *           xdrs,
    WTX_MSG_PARAM * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_VALUE (xdrs, &objp->param))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MEM_REGION - WTX
*
*/

bool_t xdr_WTX_MEM_REGION 
    (
    XDR *           xdrs,
    WTX_MEM_REGION * objp
    )
    {
    if (!xdr_TGT_ADDR_T (xdrs, &objp->baseAddr))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->size))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->attribute))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_TGT_LINK_DESC - target link descriptor
*
*/

bool_t xdr_WTX_TGT_LINK_DESC 
    (
    XDR *	    xdrs,
    WTX_TGT_LINK_DESC * objp
    )
    {
    if (!xdr_WRAPSTRING (xdrs, &objp->name))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->type))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->speed))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_RT_INFO - 
*
*/

LOCAL bool_t xdr_WTX_RT_INFO
    (
    XDR *		xdrs,
    WTX_RT_INFO *	objp,
    int			msgVersion
    )
    {
	if (!xdr_u_long(xdrs, (u_long *)&objp->rtType)) {
		return (FALSE);
	}
	if (!xdr_WRAPSTRING(xdrs, &objp->rtVersion)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->cpuType)) {
		return (FALSE);
	}
	if (msgVersion == 1) {
	    /*
	     * The BOOL32 field named hasFpp has been replaced by a
	     * bit-significant UINT32 named hasCoprocessor.  For a
	     * v1 inquiry, report only the FPP bit (as a BOOL32).
	     */
	    BOOL32 bTemp = objp->hasCoprocessor & WTX_FPP_MASK;
	    if (!xdr_bool(xdrs, (bool_t *)&bTemp)) {
		return (FALSE);
	    }
	}
	else {
	    if (!xdr_u_long(xdrs, (u_long *)&objp->hasCoprocessor)) {
		return (FALSE);
	    }
	}
	if (!xdr_bool(xdrs, (bool_t *)&objp->hasWriteProtect)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->pageSize)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->endian)) {
		return (FALSE);
	}
	if (!xdr_WRAPSTRING(xdrs, &objp->bspName)) {
		return (FALSE);
	}
	if (!xdr_WRAPSTRING(xdrs, &objp->bootline)) {
		return (FALSE);
	}
	if (!xdr_TGT_ADDR_T(xdrs, &objp->memBase)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->memSize)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->numRegions)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->memRegion, 
	sizeof(WTX_MEM_REGION), xdr_WTX_MEM_REGION)) {
		return (FALSE);
	}
	if (!xdr_TGT_ADDR_T(xdrs, &objp->hostPoolBase)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->hostPoolSize)) {
		return (FALSE);
	}
    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_AGENT_INFO - 
*
*/

bool_t xdr_WTX_AGENT_INFO
    (
    XDR *	xdrs,
    WTX_AGENT_INFO * objp
    )
    {
	if (!xdr_WRAPSTRING(xdrs, &objp->agentVersion)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->mtu)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, (u_long *)&objp->mode)) {
		return (FALSE);
	}
    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_TGT_INFO - WTX
*
*/

LOCAL bool_t xdr_WTX_TGT_INFO 
    (
    XDR *		xdrs,
    WTX_TGT_INFO *	objp,
    int			msgVersion
    )
    {
    if (!xdr_WTX_AGENT_INFO (xdrs, &objp->agentInfo))
	return (FALSE);
    if (!xdr_WTX_RT_INFO (xdrs, &objp->rtInfo, msgVersion))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_TOOL_DESC - host tool descriptor
*
*/

bool_t xdr_WTX_TOOL_DESC 
    (
    XDR *       xdrs,
    WTX_TOOL_DESC * objp
    )
    {
    if (!xdr_u_long (xdrs, (u_long *)&objp->id))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->toolName))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->toolArgv))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->toolArgv))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->userName))
	return (FALSE);
    if (!xdr_pointer (xdrs, (char **) &objp->next, 
			sizeof(WTX_TOOL_DESC), xdr_WTX_TOOL_DESC))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TOOL_DESC - WTX host tool descriptor
*
*/

bool_t xdr_WTX_MSG_TOOL_DESC 
    (
    XDR *           xdrs,
    WTX_MSG_TOOL_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_TOOL_DESC (xdrs, &objp->wtxToolDesc))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TS_INFO - target server information
*
*/

LOCAL bool_t xdr_WTX_MSG_TS_INFO 
    (
    XDR *		xdrs,
    WTX_MSG_TS_INFO *	objp,
    int			msgVersion
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_TGT_LINK_DESC (xdrs, &objp->tgtLinkDesc))
	return (FALSE);
    if (!xdr_WTX_TGT_INFO (xdrs, &objp->tgtInfo, msgVersion))
	return (FALSE);
    if (!xdr_pointer (xdrs, (char **)&objp->pWtxToolDesc, 
		      sizeof (WTX_TOOL_DESC), xdr_WTX_TOOL_DESC))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->version))	
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->userName))	
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->startTime))	
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->accessTime))	
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->lockMsg))	
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TS_INFO_V1 - target server information
*
*/

bool_t xdr_WTX_MSG_TS_INFO_V1
    (
    XDR *		xdrs,
    WTX_MSG_TS_INFO *	objp
    )
    {
    return xdr_WTX_MSG_TS_INFO (xdrs, objp, 1);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TS_INFO_V2 - target server information
*
*/

bool_t xdr_WTX_MSG_TS_INFO_V2
    (
    XDR *		xdrs,
    WTX_MSG_TS_INFO *	objp
    )
    {
    return xdr_WTX_MSG_TS_INFO (xdrs, objp, 2);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TS_LOCK - lock target server
*
*/

bool_t xdr_WTX_MSG_TS_LOCK 
    (
    XDR *	  xdrs,
    WTX_MSG_TS_LOCK * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->lockType))	
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TS_UNLOCK - unlock target server
*
*/

bool_t xdr_WTX_MSG_TS_UNLOCK 
    (
    XDR *	  xdrs,
    WTX_MSG_TS_UNLOCK * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->force))	
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_MEM_COPY_ALLOC - allocate space and decode a MEM_COPY stucture
*
* This function decodes or frees the MEM_COPY structure protocol and
* allocate space to copy the data area referenced by the MEM_COPY
* structure.  The start address is handled by the source field and the size
* by numBytes. At the decoding time, the data are decoded and saved in a
* memory area allocated.  This function is needed to hanlde WTX_MEM_WRITE
* requests that use a target address as the destination pointer. In this
* case we must first copy the data into the target server before forwarding
* them to the target.
* 
* RETURNS:  TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_MEM_COPY_ALLOC
    (
    XDR *	xdrs,
    WTX_MSG_MEM_COPY_DESC *	objp
    )
    {
    char * buff;	/* buffer where to decode data */

    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->source))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->destination))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
	return (FALSE);

    switch (xdrs->x_op) 
	{
    	case XDR_DECODE:
            {						 /* export block */
	    if ((buff = (char *)malloc (objp->numBytes)) == NULL)
	        return (FALSE);

	    if (!xdr_opaque (xdrs, (char *)buff, objp->numBytes))
	        return (FALSE);

            /* 
	     * Save the allocated buffer address in objp->source for
	     * later use by the target server.
	     */
	    objp->source = (u_int) buff;

	    return (TRUE);
	    }

    	case XDR_ENCODE:			
	    return (FALSE);			/* encoding not supported */

    	case XDR_FREE:				/* free memory */
	    return (TRUE);
	}

    return (FALSE);
    }

/*******************************************************************************
*
* xdr_MEM_WIDTH_COPY_ALLOC - allocate space and decode a MEM_COPY stucture
*
* This function decodes or frees the MEM_COPY structure protocol and
* allocate space to copy the data area referenced by the MEM_COPY
* structure.  The start address is handled by the source field and the size
* by numBytes. At the decoding time, the data are decoded and saved in a
* memory area allocated.  This function is needed to hanlde WTX_MEM_WRITE
* requests that use a target address as the destination pointer. In this
* case we must first copy the data into the target server before forwarding
* them to the target.
*
* RETURNS:  TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_MEM_WIDTH_COPY_ALLOC
    (
    XDR *                             xdrs,
    WTX_MSG_MEM_WIDTH_COPY_DESC *     objp
    )
    {
    char * buff;        /* buffer where to decode data */

    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->source))
        return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->destination))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
        return (FALSE);
    if (!xdr_u_char (xdrs, &objp->width))
        return (FALSE);

    switch (xdrs->x_op)
        {
        case XDR_DECODE:
            {                                            /* export block */
            if ((buff = (char *)malloc (objp->numBytes)) == NULL)
                return (FALSE);

            if (!xdr_opaque (xdrs, (char *)buff, objp->numBytes))
                return (FALSE);

            /*
             * Save the allocated buffer address in objp->source for
             * later use by the target server.
             */
            objp->source = (u_int) buff;

            return (TRUE);
            }

        case XDR_ENCODE:
            return (FALSE);                     /* encoding not supported */

        case XDR_FREE:                          /* free memory */
            return (TRUE);
        }

    return (FALSE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_TOOL_ID - WTX tool identifier
*
*/

bool_t xdr_WTX_MSG_TOOL_ID 
    (
    XDR *        	xdrs,
    WTX_MSG_TOOL_ID *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_KILL_DESC - WTX kill operation descriptor
*
*/

bool_t xdr_WTX_MSG_KILL_DESC 
    (
    XDR *        	xdrs,
    WTX_MSG_KILL_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->request))
	return (FALSE);
    if (!xdr_WTX_VALUE (xdrs, &objp->arg))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_EVENT - WTX
*
*/

bool_t xdr_WTX_EVENT 
    (
    XDR *        xdrs,
    WTX_EVENT * objp
    )
    {
    if (!xdr_enum (xdrs, (enum_t *)&objp->eventType))
	return (FALSE);
    if (!xdr_TGT_ARG_T (xdrs, &objp->eventArg))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_ACTION - WTX
*
*/

bool_t xdr_WTX_ACTION 
    (
    XDR *        xdrs,
    WTX_ACTION * objp
    )
    {
    if (!xdr_enum (xdrs, (enum_t *)&objp->actionType))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->actionArg))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->callRtn))
	return (FALSE);
    if (!xdr_TGT_ARG_T (xdrs, &objp->callArg))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_CONTEXT - WTX context
*
*/

bool_t xdr_WTX_CONTEXT 
    (
    XDR *        xdrs,
    WTX_CONTEXT * objp
    )
    {
    if (!xdr_enum (xdrs, (enum_t *)&objp->contextType))
	return (FALSE);
    if (!xdr_WTX_CONTEXT_ID_T (xdrs, &objp->contextId))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_CONTEXT - WTX context message
*
*/

bool_t xdr_WTX_MSG_CONTEXT 
    (
    XDR *        xdrs,
    WTX_MSG_CONTEXT * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_enum (xdrs, (enum_t *)&objp->contextType))
	return (FALSE);
    if (!xdr_WTX_CONTEXT_ID_T (xdrs, &objp->contextId))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_EVTPT - WTX
*
*/

bool_t xdr_WTX_EVTPT 
    (
    XDR *        xdrs,
    WTX_EVTPT * objp
    )
    {
    if (!xdr_WTX_EVENT (xdrs, &objp->event))
	return (FALSE);
    if (!xdr_WTX_CONTEXT (xdrs, &objp->context))
	return (FALSE);
    if (!xdr_WTX_ACTION (xdrs, &objp->action))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_EVTPT_DESC - WTX
*
*/

bool_t xdr_WTX_MSG_EVTPT_DESC 
    (
    XDR *        xdrs,
    WTX_MSG_EVTPT_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_EVTPT (xdrs, &objp->wtxEvtpt))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_EVTPT_LIST - eventpoint list
*
*/

bool_t xdr_WTX_MSG_EVTPT_LIST 
    (
    XDR *        xdrs,
    WTX_MSG_EVTPT_LIST * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nEvtpt))
	return (FALSE);
    if (!xdr_array (xdrs, (char **)&objp->pEvtpt, (u_int *)&objp->nEvtpt, 
		    ~0, sizeof (WTX_EVTPT), xdr_WTX_EVTPT))

	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MEM_XFER - allocate space and decode a WTX_MEM_XFER stucture
*
* This function decodes or frees the WTX_MEM_XFER structure and
* allocate space to copy the data area referenced by the WTX_MEM_XFER
* structure.  The start address is handled by the source field and the size
* by numBytes. At the decoding time, the data are decoded and saved in a
* memory area allocated.  
* 
* RETURNS:  TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MEM_XFER
    (
    XDR *		xdrs,
    WTX_MEM_XFER *	objp
    )
    {
    char * buff;	/* buffer where to decode data */

    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->source))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->destination))
	return (FALSE);

    switch (xdrs->x_op) 
	{
    	case XDR_DECODE:
            {						 /* export block */
	    if ((buff = (char *) malloc (objp->numBytes)) == NULL)
	        return (FALSE);

	    if (!xdr_opaque (xdrs, (char *) buff, objp->numBytes))
	        return (FALSE);

            /* 
	     * Save the allocated buffer address in objp->destination for
	     * later use by the client.
	     */

	    objp->source = (char *) buff;

	    return (TRUE);
	    }

    	case XDR_ENCODE:			
	    if (!xdr_opaque (xdrs, (char *) objp->source, objp->numBytes))
	        return (FALSE);
	    else
		return (TRUE);

    	case XDR_FREE:				/* free memory */
	    {
	    free (objp->source);
	    return (TRUE);
	    }
	}

    return (FALSE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_XFER_DESC - memory set descriptor
*
*/

bool_t xdr_WTX_MSG_MEM_XFER_DESC 
    (
    XDR *          xdrs,
    WTX_MSG_MEM_XFER_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_MEM_XFER (xdrs, &objp->memXfer))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_REG_WRITE - code, decode or free the WTX_MSG_REG_WRITE structure. 
*
* This function codes, decodes or frees the WTX_MSG_REG_WRITE structure.
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_REG_WRITE
    (
    XDR *	xdrs,
    WTX_MSG_REG_WRITE *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_enum (xdrs, (enum_t *)&objp->regSetType))
	return (FALSE);

    if (!xdr_WTX_CONTEXT (xdrs, &objp->context))
	return (FALSE);

    if (!xdr_WTX_MEM_XFER (xdrs, &objp->memXfer))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_REG_READ - code, decode or free the WTX_MSG_REG_READ structure. 
*
* This function codes, decodes or frees the WTX_MSG_REG_READ structure.
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_REG_READ
    (
    XDR *	xdrs,
    WTX_MSG_REG_READ *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_enum (xdrs, (enum_t *)&objp->regSetType))
	return (FALSE);

    if (!xdr_WTX_CONTEXT (xdrs, &objp->context))
	return (FALSE);

    if (!xdr_WTX_MEM_REGION (xdrs, &objp->memRegion))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_BLOCK_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_BLOCK_DESC
    (
    XDR *       xdrs,		/* xdr handle */
    WTX_MSG_MEM_BLOCK_DESC * objp	
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->startAddr))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_READ_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_READ_DESC
    (
    XDR *	xdrs,
    WTX_MSG_MEM_READ_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->source))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->destination))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_WIDTH_READ_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_WIDTH_READ_DESC
    (
    XDR *       xdrs,
    WTX_MSG_MEM_WIDTH_READ_DESC *     objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->source))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->destination))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
        return (FALSE);
    if (!xdr_u_char (xdrs, &objp->width))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_COPY_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_COPY_DESC
    (
    XDR *	xdrs,
    WTX_MSG_MEM_COPY_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->source))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->destination))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
	return (FALSE);

    if (xdrs->x_op == XDR_ENCODE)
        {                                                /* export block */
        if (!xdr_opaque (xdrs, (char *)objp->source, objp->numBytes))
            return (FALSE);
        }
    else if (xdrs->x_op == XDR_DECODE)                  /* import block */
        {
        if (!xdr_opaque (xdrs, (char *)objp->destination, objp->numBytes))
            return (FALSE);
	}

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_WIDTH_COPY_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_WIDTH_COPY_DESC
    (
    XDR *       xdrs,
    WTX_MSG_MEM_WIDTH_COPY_DESC *     objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->source))
        return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->destination))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
        return (FALSE);
    if (!xdr_u_char (xdrs, &objp->width))
        return (FALSE);

    if (xdrs->x_op == XDR_ENCODE)
        {                                                /* export block */
        if (!xdr_opaque (xdrs, (char *)objp->source, objp->numBytes))
            return (FALSE);
        }
    else if (xdrs->x_op == XDR_DECODE)                  /* import block */
        {
        if (!xdr_opaque (xdrs, (char *)objp->destination, objp->numBytes))
            return (FALSE);
        }

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_SCAN_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_SCAN_DESC
    (
    XDR *	xdrs,
    WTX_MSG_MEM_SCAN_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->match))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->startAddr))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->endAddr))
	return (FALSE);

    if (!xdr_bytes (xdrs, (char **)&objp->pattern, (u_int *)&objp->numBytes,
		    WTX_MAX_PATTERN_LEN))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_MOVE_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_MOVE_DESC
    (
    XDR *	xdrs,
    WTX_MSG_MEM_MOVE_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_TGT_ADDR_T (xdrs, &objp->source))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->destination))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numBytes))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_SET_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_MEM_SET_DESC
    (
    XDR *	xdrs,
    WTX_MSG_MEM_SET_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->value))
	return (FALSE);
    if (!xdr_u_char (xdrs, &objp->width))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->startAddr))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numItems))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_CONTEXT_DESC - WTX
*
* RETURNS:  TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_CONTEXT_DESC
    (
    XDR *		xdrs,	/* xdr handle */
    WTX_MSG_CONTEXT_DESC * objp	
    )
    {
    u_int	 argNum = WTX_MAX_ARG_CNT;
    TGT_ADDR_T * pArgs = (TGT_ADDR_T *)objp->args;/* argument table pointer */

    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_enum (xdrs, (enum_t *)&objp->contextType))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->returnType))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->name))	
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->priority))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->options))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->stackBase))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->stackSize))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->entry))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->redirIn))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->redirOut))
	return (FALSE);

    /*
     * The arg array is not dynamically allocated. So when freeing memory, 
     * xdr_array *MUST NOT* free this array.
     */
    if (xdrs->x_op == XDR_FREE)
	{
	char *	ptr = NULL;	/* dummy array */

	if (!xdr_array (xdrs, (caddr_t *)&ptr, &argNum, ~0, 
		    sizeof (TGT_ARG_T), xdr_TGT_ARG_T))
	return (FALSE);
	}
    else
    	{
	if (!xdr_array (xdrs, (caddr_t *)&pArgs, &argNum, ~0, 
		    sizeof (TGT_ARG_T), xdr_TGT_ARG_T))
	return (FALSE);
	}

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_CONTEXT_STEP_DESC - WTX
*
* RETURNS: TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_CONTEXT_STEP_DESC
    (
    XDR *	xdrs,
    WTX_MSG_CONTEXT_STEP_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_CONTEXT (xdrs, &objp->context))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->startAddr))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->endAddr))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_OPEN_DESC - virtual I/O descriptor  
*
*/

bool_t xdr_WTX_MSG_OPEN_DESC
    (
    XDR *	xdrs,
    WTX_MSG_OPEN_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->filename))
	return (FALSE);
    if (!xdr_enum (xdrs, (enum_t *)&objp->flags))
	return (FALSE);
    if (!xdr_int (xdrs, &objp->mode))
	return (FALSE);
    if (!xdr_int (xdrs, &objp->channel))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_VIO_CTL_DESC - virtual I/O control descriptor
*
*/

bool_t xdr_WTX_MSG_VIO_CTL_DESC
    (
    XDR *          xdrs,
    WTX_MSG_VIO_CTL_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->channel))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->request))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->arg))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_VIO_COPY_DESC - 
*
* This function codes, decodes or frees the VIO_COPY structure and copies the
* data buffer of a specific virtual I/O channel. If the VIO_COPY baseAddr field
* is a NULL pointer no data are copied, otherwise the data are coded. The
* number of bytes to encode are handled by the maxBytes field. When the data
* are decoded their values are saved in a memory area allocted by the
* xdr_opaque function. 
*
* RETURNS:  TRUE if it succeeds, FALSE otherwise.
*/

bool_t xdr_WTX_MSG_VIO_COPY_DESC
    (
    XDR *	xdrs,
    WTX_MSG_VIO_COPY_DESC *	objp
    )
    {
    bool_t moreData;				/* NULL pointer flag */

    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->channel))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->maxBytes))
	return (FALSE);

    moreData = (objp->baseAddr != NULL);	/*  NULL pointer ? */
    if (! xdr_bool(xdrs, (bool_t *)&moreData))	/* code/decode test result */
	return (FALSE);
    if (! moreData)
	objp->baseAddr  = NULL;			/* YES: return a NULL pointer */
    else					/* NO: code/decode the data */
	{
	if (xdrs->x_op == XDR_DECODE)		/* if decode force mem alloc */
	    objp->baseAddr  = (void *) malloc (objp->maxBytes);

	if (xdrs->x_op == XDR_FREE)		/* if free de-alloc memory */
	    free (objp->baseAddr);

	if (! xdr_opaque (xdrs, (char *) objp->baseAddr, objp->maxBytes))
	    return (FALSE);
	}

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_VIO_FILE_DESC - WTX VIO file descriptor
*
*/

bool_t xdr_WTX_VIO_FILE_DESC 
    (
    XDR *          xdrs,
    WTX_VIO_FILE_DESC * objp
    )
    {
    if (!xdr_WRAPSTRING (xdrs, &objp->name))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->fd))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->channel))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->fp))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->type))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->mode))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->status))
	return (FALSE);
    if (!xdr_long (xdrs, (long *)&objp->addlInfo))
	return (FALSE);
    if (!xdr_pointer (xdrs, (char **)&objp->next, sizeof (WTX_VIO_FILE_DESC),
		      xdr_WTX_VIO_FILE_DESC))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_VIO_FILE_LIST - WTX VIO files list message
*
*/

bool_t xdr_WTX_MSG_VIO_FILE_LIST 
    (
    XDR *       	   xdrs,
    WTX_MSG_VIO_FILE_LIST * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_pointer (xdrs, (char **)&objp->pVioFileList, 
		      sizeof (WTX_VIO_FILE_DESC), xdr_WTX_VIO_FILE_DESC))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_SYMBOL - symbol information
*
*/

bool_t xdr_WTX_SYMBOL 
    (
    XDR *       	xdrs,
    WTX_SYMBOL * 	objp
    )
    {
    if (!xdr_u_long (xdrs, (u_long *)&objp->status))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->symTblId))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->name))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->exactName))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->value))
	return (FALSE);
    if (!xdr_u_char (xdrs, &objp->type))
	return (FALSE);
    if (!xdr_u_char (xdrs, &objp->typeMask))
	return (FALSE);
    if (!xdr_u_short (xdrs, &objp->group))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->moduleName))
	return (FALSE);
    /* 
     * There is no need to xdr the next field since it is only meaningfull
     * on one side of the protocol.
     */

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SYMBOL_DESC - WTX symbol descriptor
*
*/

bool_t xdr_WTX_MSG_SYMBOL_DESC 
    (
    XDR *       	xdrs,
    WTX_MSG_SYMBOL_DESC * 	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_SYMBOL (xdrs, &objp->symbol))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_SYM_LIST - symbol list
*
* We avoid recursives routines here since the symbol list may be huge and
* can blow the C stack.
*/

bool_t xdr_WTX_SYM_LIST 
    (
    XDR *       	xdrs,
    WTX_SYM_LIST * 	pList
    )
    {
    bool_t		moreData;
    WTX_SYMBOL *	pSym;
    WTX_SYMBOL *	pNext;
    WTX_SYMBOL **	ppSymLink = 0;

    /* The tough one, xdr the list of symbols */

    if (xdrs->x_op == XDR_FREE)
	{
	/* move through the list and free each one. */

	for (pSym = pList->pSymbol; pSym; pSym = pNext)
	    {
	    pNext = pSym->next;
	    if (!xdr_reference (xdrs, (caddr_t *)&pSym, sizeof (WTX_SYMBOL), 
				xdr_WTX_SYMBOL))
		return (FALSE);
	    }
	}
    else
	{
	/* we need to store the links ourselves. */

	ppSymLink = &(pList->pSymbol);

	for (;;)
	    {
	    moreData = (*ppSymLink != NULL);	/* test for end of list */

	    if (!xdr_bool(xdrs, &moreData))	/* xdr the moreData field */
		return (FALSE);

	    if (!moreData)		/* if end of list it's time to leave */
		break;

	    /* 
	     * Now xdr the current symbol. Note that xdr_WTX_SYMBOL is not 
	     * recursive and will only serialize/de-serialize one symbol node.
	     */

	    if (!xdr_reference (xdrs, (caddr_t *)ppSymLink, sizeof (WTX_SYMBOL), 
				xdr_WTX_SYMBOL))
		return (FALSE);

	    /* 
	     * point to next node, if we are freeing buffers, use the node
	     * whose address was saved prior calling xdr_reference().
	     */

	    ppSymLink = & ((*ppSymLink)->next);
	    }
	}

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_LD_M_SECTION - object module section desciptor
*
*/

bool_t xdr_LD_M_SECTION
    (
    XDR *          xdrs,
    LD_M_SECTION * objp
    )
    {
    if (!xdr_u_long (xdrs, (u_long *)&objp->flags))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->addr))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->length))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_LD_M_FILE_DESC - multiple section object file
*
*/

bool_t xdr_WTX_MSG_LD_M_FILE_DESC 
    (
    XDR *       xdrs,
    WTX_MSG_LD_M_FILE_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->filename))
	return (FALSE);
    if (!xdr_int (xdrs, &objp->loadFlag))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->moduleId))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nSections))
	return (FALSE);

    if (!xdr_array (xdrs, (char **) &objp->section, 
		    (u_int *) &objp->nSections, 
		    (WTX_MAX_SECTION_CNT * sizeof (LD_M_SECTION)), 
		    sizeof (LD_M_SECTION), xdr_LD_M_SECTION))
	return (FALSE);

    if (!xdr_WTX_SYM_LIST (xdrs, &objp->undefSymList))
	return (FALSE);
    
    return (TRUE);
    }

/*******************************************************************************
*
* xdr_MODULE_LIST - object module list
*
*/

bool_t xdr_WTX_MSG_MODULE_LIST
    (
    XDR *          xdrs,
    WTX_MSG_MODULE_LIST * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->numObjMod))
	return (FALSE);
    if (!xdr_array (xdrs, (char **)&objp->modIdList, (u_int *)&objp->numObjMod, 
		    (WTX_MAX_MODULE_CNT * sizeof (u_int)), sizeof (u_int), 
		    xdr_u_long))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_OBJ_SEGMENT - object module segment
*
*/

bool_t xdr_OBJ_SEGMENT
    (
    XDR *         xdrs,
    OBJ_SEGMENT * objp
    )
    {
    if (!xdr_u_long (xdrs, (u_long *)&objp->flags))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->addr))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->length))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->reserved1))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->reserved2))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MODULE_INFO - object module information
*
*/

bool_t xdr_WTX_MSG_MODULE_INFO 
    (
    XDR *         	xdrs,
    WTX_MSG_MODULE_INFO * 	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->moduleId))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->moduleName))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->format))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->group))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nSegments))
	return (FALSE);
    if (!xdr_int (xdrs, &objp->loadFlag))
	return (FALSE);
    if (!xdr_array (xdrs, (char **)&objp->segment, (u_int *)&objp->nSegments, 
		    (WTX_MAX_OBJ_SEG_CNT * sizeof (OBJ_SEGMENT)), 
		    sizeof (OBJ_SEGMENT), xdr_OBJ_SEGMENT))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MOD_NAME_OR_ID - object module name or identifier
*
*/

bool_t xdr_WTX_MSG_MOD_NAME_OR_ID
    (
    XDR *            xdrs,
    WTX_MSG_MOD_NAME_OR_ID * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->moduleId))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->filename))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SYM_LIST - symbol list message
*
*/

bool_t xdr_WTX_MSG_SYM_LIST
    (
    XDR *           xdrs,
    WTX_MSG_SYM_LIST * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_SYM_LIST (xdrs, &objp->symList))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->moreToCome))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SYM_MATCH_DESC - symbol matching information
*
*/

bool_t xdr_WTX_MSG_SYM_MATCH_DESC 
    (
    XDR *       	xdrs,
    WTX_MSG_SYM_MATCH_DESC * 	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->symTblId))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->matchString))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->adrs))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->stringToMatch))
	return (FALSE);

    if (!xdr_bool (xdrs, (bool_t *)&objp->byModuleName)) 
	return (FALSE);

    /* check if we were given a module name or a module Identifier */

    if (objp->byModuleName) 
	{
	if (!xdr_WRAPSTRING (xdrs, &objp->module.moduleName))
	    return (FALSE);
	}
    else
	{
	if (!xdr_u_long (xdrs, (u_long *)&objp->module.moduleId))
	    return (FALSE);
	}

    if (!xdr_bool (xdrs, (bool_t *)&objp->listUnknownSym))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->giveMeNext))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SYM_TBL_INFO - symbol table information
*
*/

bool_t xdr_WTX_MSG_SYM_TBL_INFO
    (
    XDR *          xdrs,
    WTX_MSG_SYM_TBL_INFO * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->symTblId))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->symNum))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->sameNameOk))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_EVENT_DESC - WTX event descriptor
*
*/

bool_t xdr_WTX_EVENT_DESC
    (
    XDR *       xdrs,
    WTX_EVENT_DESC * objp
    )
    {
    if (!xdr_WRAPSTRING (xdrs, &objp->event))
	return (FALSE);
    if (!xdr_bytes (xdrs, (char **) &objp->addlData, 
		    (u_int *) &objp->addlDataLen, ~0))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_EVENT_DESC - WTX event descriptor
*
*/

bool_t xdr_WTX_MSG_EVENT_DESC
    (
    XDR *       xdrs,
    WTX_MSG_EVENT_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_EVENT_DESC (xdrs, &objp->eventDesc))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_EVENT_REG_DESC - register for event descriptor
*
*/

bool_t xdr_WTX_MSG_EVENT_REG_DESC 
    (
    XDR *            xdrs,
    WTX_MSG_EVENT_REG_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->evtRegExp))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_WDB_SERVICE_DESC - service descriptor 
*
*/

bool_t xdr_WTX_WDB_SERVICE_DESC
    (
    XDR *          xdrs,
    WTX_WDB_SERVICE_DESC * objp
    )
    {
    if (!xdr_u_int (xdrs, &objp->rpcNum))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->name))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->svcObjFile))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->initRtnName))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_WTX_SERVICE_DESC - service descriptor 
*
*/

bool_t xdr_WTX_WTX_SERVICE_DESC
    (
    XDR *          xdrs,
    WTX_WTX_SERVICE_DESC * objp
    )
    {
    if (!xdr_WRAPSTRING (xdrs, &objp->svcObjFile))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->svcProcName))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->inProcName))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->outProcName))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SERVICE_DESC - 
*
*/

bool_t xdr_WTX_MSG_SERVICE_DESC 
    (
    XDR *            xdrs,
    WTX_MSG_SERVICE_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_pointer (xdrs, (char **)&objp->pWtxServiceDesc,
		      sizeof (WTX_WTX_SERVICE_DESC), 
		      xdr_WTX_WTX_SERVICE_DESC))
	return (FALSE);
    if (!xdr_pointer (xdrs, (char **)&objp->pWdbServiceDesc,
		      sizeof (WTX_WDB_SERVICE_DESC), 
		      xdr_WTX_WDB_SERVICE_DESC))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SERVICE_LIST - list of protocol services
*
*/

bool_t xdr_WTX_MSG_SERVICE_LIST 
    (
    XDR *          xdrs,
    WTX_MSG_SERVICE_LIST * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nServices))
	return (FALSE);
    if (!xdr_array (xdrs, (char **)&objp->serviceDesc, 
		    (u_int *)&objp->nServices, 
		    (WTX_MAX_SERVICE_CNT * sizeof (WTX_WDB_SERVICE_DESC)), 
		    sizeof (WTX_WDB_SERVICE_DESC), xdr_WTX_WDB_SERVICE_DESC))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_CONSOLE_DESC - WTX console descriptor
*
*/

bool_t xdr_WTX_CONSOLE_DESC 
    (
    XDR *          xdrs,
    WTX_CONSOLE_DESC * objp
    )
    {
    if (!xdr_int (xdrs, &objp->fdIn))
	return (FALSE);
    if (!xdr_int (xdrs, &objp->fdOut))
	return (FALSE);
    if (!xdr_int (xdrs, &objp->pid))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->name))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->display))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_CONSOLE_DESC - WTX console descriptor message
*
*/

bool_t xdr_WTX_MSG_CONSOLE_DESC 
    (
    XDR *          xdrs,
    WTX_MSG_CONSOLE_DESC * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_CONSOLE_DESC (xdrs, &objp->wtxConsDesc))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_MEM_INFO - target server memory information
*
*/

bool_t xdr_WTX_MSG_MEM_INFO 
    (
    XDR *	  xdrs,
    WTX_MSG_MEM_INFO * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->curBytesFree))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->curBytesAllocated))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->cumBytesAllocated))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->curBlocksFree))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->curBlocksAlloc))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->cumBlocksAlloc))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->avgFreeBlockSize))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->avgAllocBlockSize))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->cumAvgAllocBlockSize))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->biggestBlockSize))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_CORE - xdr routine for wtx core
*/

bool_t xdr_WTX_CORE
    (
    XDR *	xdrs,
    WTX_CORE *	objp
    )
    {
    if (!xdr_u_long (xdrs, (u_long *)&objp->objId))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->errCode))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->protVersion))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_SVR_DESC - xdr routine for wtx descriptor
*/

bool_t xdr_WTX_SVR_DESC
    (
    XDR *		xdrs,
    WTX_SVR_DESC *	objp
    )
    {
    if (!xdr_WRAPSTRING (xdrs, &objp->wpwrName))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->wpwrType))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->wpwrKey))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SVR_DESC - xdr routine for wtx descriptor
*/

bool_t xdr_WTX_MSG_SVR_DESC
    (
    XDR *		xdrs,
    WTX_MSG_SVR_DESC *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_WTX_SVR_DESC (xdrs, &objp->wtxSvrDesc))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_SVR_DESC_Q - xdr routine for wtx all descriptor
*/

bool_t xdr_WTX_SVR_DESC_Q
    (
    XDR *		xdrs,
    WTX_SVR_DESC_Q *	objp
    )
    {
    if (!xdr_WTX_SVR_DESC (xdrs, &objp->wtxSvrDesc))
	return (FALSE);
    if (!xdr_pointer (xdrs, (char **)&objp->pNext, sizeof (WTX_SVR_DESC_Q), 
		      xdr_WTX_SVR_DESC_Q))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_SVR_DESC - xdr routine for wtx descriptor
*/

bool_t xdr_WTX_MSG_SVR_DESC_Q
    (
    XDR *			xdrs,
    WTX_MSG_SVR_DESC_Q *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_WTX_SVR_DESC_Q (xdrs, &objp->wtxSvrDescQ))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_WTXREGD_PATTERN - xdr routine for WindPower deamon pattern
*/

bool_t xdr_WTX_MSG_WTXREGD_PATTERN
    (
    XDR *			xdrs,
    WTX_MSG_WTXREGD_PATTERN *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);

    if (!xdr_WRAPSTRING (xdrs, &objp->name))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->type))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->key))
	return (FALSE);

    return (TRUE);
    }

/******************************************************************************
*
* xdr_WTX_GOPHER_TAPE -
*/

static bool_t xdr_WTX_GOPHER_TAPE
    (
    XDR *		xdrs,
    WTX_GOPHER_TAPE *	objp
    )
    {
    u_int len = objp->len;

    if (!xdr_u_short (xdrs, &objp->len))
	return (FALSE);
    if (!xdr_bytes (xdrs, &objp->data, &len, ~0))
	return (FALSE);

    return (TRUE);
    }

/******************************************************************************
*
* xdr_WTX_MSG_GOPHER_TAPE -
*/

bool_t xdr_WTX_MSG_GOPHER_TAPE
    (
    XDR *			xdrs,
    WTX_MSG_GOPHER_TAPE *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WTX_GOPHER_TAPE  (xdrs, &objp->tape))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_DASM_DESC - XDR WTX_MSG_DASM_DESC transfer routine
*
*/

bool_t xdr_WTX_MSG_DASM_DESC 
    (
    XDR *       	xdrs,
    WTX_MSG_DASM_DESC * 	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->startAddr))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nInst))
	return (FALSE);
    if (!xdr_TGT_ADDR_T (xdrs, &objp->endAddr))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->printAddr))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->printOpcodes))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->hexAddr))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *)&objp->giveMeNext))
	return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_DASM_INST_LIST - XDR WTX_MSG_DASM_INST_LIST transfer routine
*
*/

bool_t xdr_WTX_MSG_DASM_INST_LIST
    (
    XDR *			xdrs,
    WTX_MSG_DASM_INST_LIST *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
	return (FALSE);
    if (!xdr_WRAPSTRING (xdrs, &objp->instList.pInst))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *) &objp->instList.nextInst))
	return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *) &objp->instList.listSize))
	return (FALSE);
    if (!xdr_bool (xdrs, (bool_t *) &objp->moreToCome))
	return (FALSE);

    return (TRUE);
    }

/******************************************************************************
*
* xdr_WTX_EVENT_NODE -
*/
bool_t xdr_WTX_EVENT_NODE
    (
    XDR *             xdrs,
    WTX_EVENT_NODE *  objp
    )
    {
    if (!xdr_WTX_EVENT_DESC (xdrs, &objp->event))
        return (FALSE);
    if (!xdr_pointer (xdrs, (char **)&objp->pNext, sizeof (WTX_EVENT_NODE),
                      xdr_WTX_EVENT_NODE))
        return (FALSE);

    return (TRUE);
    }

/******************************************************************************
*
* xdr_MSG_EVENT_LIST -
*
*/

bool_t xdr_WTX_MSG_EVENT_LIST
    (
    XDR *                  xdrs,
    WTX_MSG_EVENT_LIST *   objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_pointer(xdrs, (char **)&objp->pEventList,
                     sizeof(WTX_EVENT_NODE), xdr_WTX_EVENT_NODE))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_FILE_LOAD_DESC -
*
*/

bool_t xdr_WTX_MSG_FILE_LOAD_DESC
    (
    XDR *       xdrs,
    WTX_MSG_FILE_LOAD_DESC * objp
    )
    {
    /* Buffer where to decode data */

    char * buff;

    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->flags))
	return (FALSE);

    if (!xdr_WTX_LD_M_FILE_DESC (xdrs, &objp->fileDesc))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->buffer))
        return (FALSE);
    if (!xdr_int (xdrs, &objp->numItems))
        return (FALSE);
    if (!xdr_int (xdrs, &objp->fileSize))
        return (FALSE);
    if (!xdr_int (xdrs, &objp->numPacket))
        return (FALSE);

    switch (xdrs->x_op)
        {
        case XDR_DECODE:
            {
	    if (objp->numItems != 0)
		{
		/* Export block */

		if ((buff = (char *) malloc (objp->numItems)) == NULL)
		    return (FALSE);

		if (!xdr_opaque (xdrs, (char *) buff, objp->numItems))
		    return (FALSE);

		/*
		 * Save the allocated buffer address in objp->destination for
		 * later use by the client.
		 */

		objp->buffer = (char *) buff;
		}
	    else
		objp->buffer = NULL;

	    return (TRUE);
	    }

        case XDR_ENCODE:
	    {
	    if (objp->numItems != 0)
		{
		if (!xdr_opaque (xdrs, (char *) objp->buffer, objp->numItems))
		    return (FALSE);
		}

	    return (TRUE);
	    }

        case XDR_FREE:
            {
            /* Free memory */

            free (objp->buffer);
            return (TRUE);
            }
        }

    return (FALSE);
    }

/*******************************************************************************
*
* xdr_WTX_LD_M_FILE_DESC - multiple section object file
*
*/

bool_t xdr_WTX_LD_M_FILE_DESC
    (
    XDR *       xdrs,
    WTX_LD_M_FILE_DESC * objp
    )
    {
    if (!xdr_WRAPSTRING (xdrs, &objp->filename))
        return (FALSE);
    if (!xdr_int (xdrs, &objp->loadFlag))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->moduleId))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nSections))
        return (FALSE);
    if (!xdr_array (xdrs, (char **)&objp->section, (u_int *)&objp->nSections,
                    (WTX_MAX_SECTION_CNT * sizeof (LD_M_SECTION)),
                    sizeof (LD_M_SECTION), xdr_LD_M_SECTION))
        return (FALSE);
    if (!xdr_WTX_SYM_LIST (xdrs, &objp->undefSymList))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_EVENT_2 - WTX
*
*/

bool_t xdr_WTX_EVENT_2
    (
    XDR *        xdrs,
    WTX_EVENT_2 * objp
    )
    {
    if (!xdr_enum (xdrs, (enum_t *)&objp->eventType))
        return (FALSE);

    if (!xdr_u_long (xdrs, (u_long *)&objp->numArgs))
        return (FALSE);

    if (!xdr_array (xdrs, (char **)&objp->args, &objp->numArgs,
                    ~0, sizeof (u_long), xdr_u_long))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_EVTPT_2 - WTX
*
*/

bool_t xdr_WTX_EVTPT_2
    (
    XDR *        xdrs,
    WTX_EVTPT_2 * objp
    )
    {
    if (!xdr_WTX_EVENT_2 (xdrs, &objp->event))
        return (FALSE);
    if (!xdr_WTX_CONTEXT (xdrs, &objp->context))
        return (FALSE);
    if (!xdr_WTX_ACTION (xdrs, &objp->action))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_EVTPT_DESC_2 - WTX
*
*/

bool_t xdr_WTX_MSG_EVTPT_DESC_2
    (
    XDR *        xdrs,
    WTX_MSG_EVTPT_DESC_2 * objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_WTX_EVTPT_2 (xdrs, &objp->wtxEvtpt))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_EVTPT_INFO - eventpoint info
*
*/

bool_t xdr_WTX_EVTPT_INFO
    (
    XDR *		xdrs,
    WTX_EVTPT_INFO *	objp
    )
    {
    if (!xdr_WTX_EVTPT_2 (xdrs, &objp->wtxEvtpt))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->toolId))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->evtptNum))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->pReserved1))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->pReserved2))
        return (FALSE);

    return (TRUE);
    }

/*******************************************************************************
*
* xdr_WTX_MSG_EVTPT_LIST_2 - eventpoint list
*
*/

bool_t xdr_WTX_MSG_EVTPT_LIST_2
    (
    XDR *			xdrs,
    WTX_MSG_EVTPT_LIST_2 *	objp
    )
    {
    if (!xdr_WTX_CORE (xdrs, &objp->wtxCore))
        return (FALSE);
    if (!xdr_u_long (xdrs, (u_long *)&objp->nEvtpt))
        return (FALSE);
    if (!xdr_array (xdrs, (char **)&objp->pEvtptInfo, (u_int *)&objp->nEvtpt,
                    ~0, sizeof (WTX_EVTPT_INFO), xdr_WTX_EVTPT_INFO))
        return (FALSE);

    return (TRUE);
    }
