/* bootLib.c - boot ROM subroutine library */

/* Copyright 1995-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02v,13mar02,jkf  Fixing SPR#74251, using bootLib.h macros for array sizes 
                 instead of numeric constants.  Changed copyright to 2002.
02u,30mar99,jdi  doc: corrected misspelling of NOMANUAL which caused
		    bootScanNum() to publish.
02t,28mar99,jdi  doc: fixed tiny formatting errors in .iP section.
02s,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
02r,04feb99,dbt  when bootParamsShow() routine is called without any
                 parameter, print an error message (Fixed SPR #24885).
02q,26aug97,spm  modified bootLeaseExtract to allow improved treatment of DHCP 
                 address information by startup routines
02p,27nov96,spm  added DHCP client and support for network device unit numbers.
02p,12sep97,dgp  doc: fix SPR 5330 boot line has required parameters
02o,19jun96,dbt  fixed spr 6691. Modified function promptParamString in order
		 to take into account the size of the field to read
		 Modified promptRead to read the entire string even if it is 
		 too long. This is to empty the reception buffer of the
		 standard input.
		 Update copyright.
02n,14oct95,jdi  doc: revised pathnames for Tornado.
02m,16feb95,jdi  doc format corrections.
02l,12jan93,jdi  fixed poor style in bootParamsPrompt().
02k,20jan93,jdi  documentation cleanup for 5.1.
02j,28jul92,rdc  made bootStructToString write enable memory before writing.
02i,26may92,rrr  the tree shuffle
02h,01mar92,elh  changed printParamNum to take an int param (instead of char).
02g,05dec91,rrr  shut up some ansi warnings.
02f,19nov91,rrr  shut up some ansi warnings.
02e,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
02d,30apr91,jdi	 documentation tweak.
02c,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02b,24mar91,jaa	 documentation.
02a,15jul90,dnw  added "targetName", "startupScript", and "other" fields
		 replaced bootParamsToString() with bootStructToString()
		 replaced bootStringToParams() with bootStringToStruct()
		 changed to initialize procNum to 0 instead of NONE
		 routines accept and generate boot strings w/o host & filename
		   (required for bootp)
		 removed clearFlag from promptParamString - now all fields
		   can be cleared
		 promptParamNum now accepts "." to be clear (0)
		 added bootParamsErrorPrint
01r,10jul90,dnw  lint clean-up, esp to allow void to be void one day
		 fixed incorrect declaration of promptParamString() &
		   promptParamNum(); was void, now int
		   declared to return int instead of void
		 changed name of scanNum() to bootScanNum() to not conflict
		   with routine in fioLib.c
01q,02jan90,dab  fixed bootStringsToParams() bug in parsing vbnum value.
		 disabled clearing of boot device, host name, or boot file
		   fields.
01p,23may89,dnw  made promptRead be LOCAL.
01o,20aug88,gae  documentation
01n,30may88,dnw  changed to v4 names.
01m,29may88,dnw  changed calls to atoi() to sscanf().
01l,28may88,dnw  moved skipSpace here from fioLib.
		 changed calls to fioStdIn to STD_IN.
		 made promptParam...() LOCAL again (copies in nfsLib now).
01k,19apr88,llk  made promptParamNum and promptParamString global routines.
01j,19feb88,dnw  added bootExtractNetmask().
01i,18nov87,ecs  lint.
		 added include of strLib.h.
		 documentation.
01h,15nov87,dnw  changed to print '?' for unprintable chars in parameters.
01g,06nov87,jlf  documentation
01f,13oct87,dnw  added flags field to boot line encoding and decoding.
01e,09oct87,jlf  changed to deal with new ISI boot format with ethernet adrs.
01d,14may87,rdc  procnum can now be defined by a line VBNUM=n or VB=n for
	   +dnw    compatability with isi bootproms.
		 changed prompt to indicate password is for ftp only.
		 fixed bootEncodeParams() to add EOS.
01c,23mar87,jlf  documentation.
		 changed routine names to meet naming conventions.
01b,20dec86,dnw  added promptBootParams().
		 changed to not get include files from default directories.
01a,18dec86,llk  created.
*/

/*
DESCRIPTION
This library contains routines for manipulating a boot line.
Routines are provided to interpret, construct, print, and prompt for
a boot line.

When VxWorks is first booted, certain parameters can be specified,
such as network addresses, boot device, host, and start-up file.
This information is encoded into a single ASCII string known as the boot line.
The boot line is placed at a known address (specified in config.h)
by the boot ROMs so that the system being booted can discover the parameters
that were used to boot the system.
The boot line is the only means of communication from the boot ROMs to
the booted system.

The boot line is of the form:
.CS
bootdev(unitnum,procnum)hostname:filename e=# b=# h=# g=# u=userid pw=passwd f=#
tn=targetname s=startupscript o=other
.CE
where:
.iP <bootdev> 12
the boot device (required); for example, "ex" for Excelan Ethernet, "bp" for 
backplane.  For the backplane, this field can have an optional anchor
address specification of the form "bp=<adrs>" (see bootBpAnchorExtract()).
.iP <unitnum>
the unit number of the boot device (0..n).
.iP <procnum>
the processor number on the backplane, 0..n (required for VME boards).
.iP <hostname>
the name of the boot host (required).
.iP <filename>
the file to be booted (required).
.iP "e" "" 3
the Internet address of the Ethernet interface.
This field can have an optional subnet mask
of the form <inet_adrs>:<subnet_mask>. If DHCP
is used to obtain the configuration parameters,
lease timing information may also be present.
This information takes the form <lease_duration>:<lease_origin>
and is appended to the end of the field.
(see bootNetmaskExtract() and bootLeaseExtract()).
.iP "b"
the Internet address of the backplane interface.
This field can have an optional subnet mask
and/or lease timing information as "e".
.iP "h"
the Internet address of the boot host.
.iP "g"
the Internet address of the gateway to the boot host.
Leave this parameter blank if the host is on same network.
.iP "u"
a valid user name on the boot host.
.iP "pw"
the password for the user on the host.
This parameter is usually left blank.
If specified, FTP is used for file transfers.
.iP "f"
the system-dependent configuration flags.
This parameter contains an `or' of option bits defined in
sysLib.h.
.iP "tn"
the name of the system being booted
.iP "s"
the name of a file to be executed as a start-up script.
.iP "o"
"other" string for use by the application.
.LP
The Internet addresses are specified in "dot" notation (e.g., 90.0.0.2).
The order of assigned values is arbitrary.

EXAMPLE
.CS
enp(0,0)host:/usr/wpwr/target/config/mz7122/vxWorks e=90.0.0.2 b=91.0.0.2
 h=100.0.0.4    g=90.0.0.3 u=bob pw=realtime f=2 tn=target
 s=host:/usr/bob/startup o=any_string
.CE

INCLUDE FILES: bootLib.h

SEE ALSO: bootConfig
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "ctype.h"
#include "string.h"
#include "sysLib.h"
#include "bootLib.h"
#include "stdio.h"
#include "fioLib.h"
#include "private/vmLibP.h"
#include "taskLib.h"

/* macros that fill in size parameter */

#define GET_WORD(pp, field, delims)	\
	getWord (pp, field, sizeof (field), delims)

#define GET_ASSIGN(pp, name, field)	\
	getAssign (pp, name, field, sizeof (field))


#define PARAM_PRINT_WIDTH	21

LOCAL char strBootDevice []		= "boot device";
LOCAL char strHostName []		= "host name";
LOCAL char strTargetName []		= "target name (tn)";
LOCAL char strFileName []		= "file name";
LOCAL char strInetOnEthernet []		= "inet on ethernet (e)";
LOCAL char strInetOnBackplane []	= "inet on backplane (b)";
LOCAL char strHostInet []		= "host inet (h)";
LOCAL char strGatewayInet []		= "gateway inet (g)";
LOCAL char strUser []			= "user (u)";
LOCAL char strFtpPw []			= "ftp password (pw)";
LOCAL char strFtpPwLong []		= "ftp password (pw) (blank = use rsh)";
LOCAL char strUnitNum []                = "unit number";
LOCAL char strProcNum []		= "processor number";
LOCAL char strFlags []			= "flags (f)";
LOCAL char strStartup []		= "startup script (s)";
LOCAL char strOther []			= "other (o)";


/* forward static functions */

static STATUS bootSubfieldExtract (char *string, int *pValue, char
		delimeter);
static void addAssignNum (char *string, char *code, int value);
static void addAssignString (char *string, char *code, char *value);
static BOOL getWord (char ** ppString, char *pWord, int length, char *delim);
static BOOL getConst (char ** ppString, char *pConst);
static BOOL getNum (char ** ppString, int *pNum);
static BOOL getAssign (char ** ppString, char *valName, char *pVal, int
		maxLen);
static BOOL getAssignNum (char ** ppString, char *valName, int *pVal);
static void printClear (char *param);
static void printParamNum (char *paramName, int value, BOOL hex);
static void printParamString (char *paramName, char *param);
static int promptParamBootDevice (char *paramName, char *param, int *pValue, 
                             int sizeParamName);
static int promptParamString (char *paramName, char *param, int sizeParamName);
static int promptParamNum (char *paramName, int *pValue, BOOL hex);
static int promptRead (char *buf, int bufLen);
static void skipSpace (char ** strptr);


/*******************************************************************************
*
* bootStringToStruct - interpret the boot parameters from the boot line
*
* This routine parses the ASCII string and returns the values into
* the provided parameters.
*
* For a description of the format of the boot line, see the manual entry 
* for bootLib 
*
* RETURNS:
* A pointer to the last character successfully parsed plus one
* (points to EOS, if OK).  The entire boot line is parsed.
*/

char *bootStringToStruct
    (
    char *bootString,   		/* boot line to be parsed */
    FAST BOOT_PARAMS *pBootParams	/* where to return parsed boot line */
    )
    {
    char *p1 = bootString;
    char *p1Save;
    char *p2;
    char hostAndFile [BOOT_HOST_LEN + BOOT_FILE_LEN];

    /* initialize boot parameters */

    bzero ((char *) pBootParams, sizeof (*pBootParams));


    /* get boot device and proc num */

    if (!GET_WORD  (&p1, pBootParams->bootDev, " \t(")	||
	!getConst (&p1, "(")				||
	!getNum   (&p1, &pBootParams->unitNum)		||
	!getConst (&p1, ",")				||
	!getNum   (&p1, &pBootParams->procNum)		||
	!getConst (&p1, ")"))
	{
	return (p1);
	}

    /* get host name and boot file, if present */

    p1Save = p1;
    GET_WORD  (&p1, hostAndFile, " \t");

    if (index (hostAndFile, ':') != NULL)
	{
	p2 = hostAndFile;
	GET_WORD  (&p2, pBootParams->hostName, ":");
	getConst (&p2, ":");
	GET_WORD  (&p2, pBootParams->bootFile, " \t");
	}
    else
	p1 = p1Save;

    /* get optional assignments */

    while (skipSpace (&p1), (*p1 != EOS))
	{
	if (!GET_ASSIGN (&p1, "ead", pBootParams->ead) &&
	    !GET_ASSIGN (&p1, "e",   pBootParams->ead) &&
	    !GET_ASSIGN (&p1, "bad", pBootParams->bad) &&
	    !GET_ASSIGN (&p1, "b",   pBootParams->bad) &&
	    !GET_ASSIGN (&p1, "had", pBootParams->had) &&
	    !GET_ASSIGN (&p1, "h",   pBootParams->had) &&
	    !GET_ASSIGN (&p1, "gad", pBootParams->gad) &&
	    !GET_ASSIGN (&p1, "g",   pBootParams->gad) &&
	    !GET_ASSIGN (&p1, "usr", pBootParams->usr) &&
	    !GET_ASSIGN (&p1, "u",   pBootParams->usr) &&
	    !GET_ASSIGN (&p1, "pw",  pBootParams->passwd) &&
	    !GET_ASSIGN (&p1, "o",   pBootParams->other) &&
	    !GET_ASSIGN (&p1, "tn",  pBootParams->targetName) &&
	    !GET_ASSIGN (&p1, "hn",  pBootParams->hostName) &&
	    !GET_ASSIGN (&p1, "fn",  pBootParams->bootFile) &&
	    !GET_ASSIGN (&p1, "s",  pBootParams->startupScript) &&
	    !getAssignNum (&p1, "n", &pBootParams->procNum) &&
	    !getAssignNum (&p1, "f", &pBootParams->flags))
	    {
	    break;
	    }
	}

    return (p1);
    }

/*******************************************************************************
*
* bootStructToString - construct a boot line
*
* This routine encodes a boot line using the specified boot parameters.
*
* For a description of the format of the boot line, see the manual 
* entry for bootLib.
*
* RETURNS: OK.
*/

STATUS bootStructToString
    (
    char *paramString,    	    /* where to return the encoded boot line */
    FAST BOOT_PARAMS *pBootParams   /* boot line structure to be encoded */
    )
    {
    UINT state;
    BOOL writeProtected = FALSE;
    int pageSize = 0;
    char *pageAddr = NULL;

    /* see if we need to write enable the memory */

    if (vmLibInfo.vmLibInstalled)
	{
	pageSize = VM_PAGE_SIZE_GET();

	pageAddr = (char *) ((UINT) paramString / pageSize * pageSize);

	if (VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR)
	    if ((state & VM_STATE_MASK_WRITABLE) == VM_STATE_WRITABLE_NOT)
		{
		writeProtected = TRUE;
		TASK_SAFE();
		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE);
		}

	}

    if ((pBootParams->hostName[0] == EOS) && (pBootParams->bootFile[0] == EOS))
	sprintf (paramString, "%s(%d,%d)", pBootParams->bootDev,
		 pBootParams->unitNum, pBootParams->procNum);
    else
	sprintf (paramString, "%s(%d,%d)%s:%s", pBootParams->bootDev,
		 pBootParams->unitNum, pBootParams->procNum, 
                 pBootParams->hostName, pBootParams->bootFile);

    addAssignString (paramString, "e", pBootParams->ead);
    addAssignString (paramString, "b", pBootParams->bad);
    addAssignString (paramString, "h", pBootParams->had);
    addAssignString (paramString, "g", pBootParams->gad);
    addAssignString (paramString, "u", pBootParams->usr);
    addAssignString (paramString, "pw", pBootParams->passwd);
    addAssignNum (paramString, "f", pBootParams->flags);
    addAssignString (paramString, "tn", pBootParams->targetName);
    addAssignString (paramString, "s", pBootParams->startupScript);
    addAssignString (paramString, "o", pBootParams->other);

    if (writeProtected)
	{
	VM_STATE_SET (NULL, pageAddr, pageSize, 
		      VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT);
	TASK_UNSAFE();
	}

    return (OK);
    }

/*******************************************************************************
*
* bootParamsShow - display boot line parameters
*
* This routine displays the boot parameters in the specified boot string
* one parameter per line.
*
* RETURNS: N/A
*/

void bootParamsShow
    (
    char *paramString           /* boot parameter string */
    )
    {
    BOOT_PARAMS bootParams;
    char *pS;

    if (paramString == NULL)
    	{
	printf ("No boot parameter string specified.\n");
	return;
	}

    pS = bootStringToStruct (paramString, &bootParams);
    if (*pS != EOS)
	{
	bootParamsErrorPrint (paramString, pS);
	return;
	}

    printf ("\n");

    printParamString (strBootDevice, bootParams.bootDev);
    printParamNum (strUnitNum, bootParams.unitNum, FALSE);
    printParamNum (strProcNum, bootParams.procNum, FALSE);
    printParamString (strHostName, bootParams.hostName);
    printParamString (strFileName, bootParams.bootFile);
    printParamString (strInetOnEthernet, bootParams.ead);
    printParamString (strInetOnBackplane, bootParams.bad);
    printParamString (strHostInet, bootParams.had);
    printParamString (strGatewayInet, bootParams.gad);
    printParamString (strUser, bootParams.usr);
    printParamString (strFtpPw, bootParams.passwd);
    printParamNum (strFlags, bootParams.flags, TRUE);
    printParamString (strTargetName, bootParams.targetName);
    printParamString (strStartup, bootParams.startupScript);
    printParamString (strOther, bootParams.other);

    printf ("\n");
    }

/*******************************************************************************
*
* bootParamsPrompt - prompt for boot line parameters
*
* This routine displays the current value of each boot parameter and prompts
* the user for a new value.  Typing a RETURN leaves the parameter unchanged.
* Typing a period (.) clears the parameter.
*
* The parameter <string> holds the initial values.  The new boot line is
* copied over <string>.  If there are no initial values, <string> is
* empty on entry.
*
* RETURNS: N/A
*/

void bootParamsPrompt
    (
    char *string        /* default boot line */
    )
    {
    BOOT_PARAMS bp;
    FAST int n = 0;
    FAST int i;

    /* interpret the boot parameters */
	
    (void) bootStringToStruct (string, &bp);

    printf ("\n'.' = clear field;  '-' = go to previous field;  ^D = quit\n\n");

    /* prompt the user for each item;
     *   i:  0 = same field, 1 = next field, -1 = previous field,
     *     -98 = error, -99 = quit
     */

    FOREVER
	{
	switch (n)
	    {
	    case 0:  i = promptParamBootDevice (strBootDevice, bp.bootDev, 
                                &bp.unitNum, sizeof (bp.bootDev));  break; 
	    case 1:  i = promptParamNum (strProcNum, &bp.procNum, FALSE);break;
	    case 2:  i = promptParamString (strHostName, bp.hostName,
				sizeof (bp.hostName));	break;
	    case 3:  i = promptParamString (strFileName, bp.bootFile,
				sizeof (bp.bootFile));	break;
	    case 4:  i = promptParamString (strInetOnEthernet, bp.ead,
				sizeof (bp.ead));	break;
	    case 5:  i = promptParamString (strInetOnBackplane, bp.bad,
				sizeof (bp.bad));break;
	    case 6:  i = promptParamString (strHostInet, bp.had,
				sizeof (bp.had));	break;
	    case 7:  i = promptParamString (strGatewayInet, bp.gad,
				sizeof (bp.gad));	break;
	    case 8:  i = promptParamString (strUser, bp.usr,
				sizeof (bp.usr));		break;
	    case 9:  i = promptParamString (strFtpPwLong, bp.passwd,
				sizeof (bp.passwd));	break;
	    case 10: i = promptParamNum (strFlags, &bp.flags, TRUE);	break;
	    case 11: i = promptParamString (strTargetName, bp.targetName,
				sizeof (bp.targetName));break;
	    case 12: i = promptParamString (strStartup, bp.startupScript,
				sizeof (bp.startupScript));break;
	    case 13: i = promptParamString (strOther, bp.other,
				sizeof (bp.other));	break;
	    default: i = -99; break;
	    }

	/* check for QUIT */

	if (i == -99)
	    {
	    printf ("\n");
	    break;
	    }

	/* move to new field */

	if (i != -98)
	    n += i;
	}

    (void) bootStructToString (string, &bp);
    }
/******************************************************************************
*
* bootParamsErrorPrint - print boot string error indicator
*
* print error msg with '^' where parse failed
*
* NOMANUAL
*/

void bootParamsErrorPrint
    (
    char *bootString,
    char *pError
    )
    {
    printf ("Error in boot line:\n%s\n%*c\n", bootString,
	    (int) (pError - bootString + 1), '^');
    }

/*******************************************************************************
*
* bootLeaseExtract - extract the lease information from an Internet address
*
* This routine extracts the optional lease duration and lease origin fields 
* from an Internet address field for use with DHCP.  The lease duration can be 
* specified by appending a colon and the lease duration to the netmask field.
* For example, the "inet on ethernet" field of the boot parameters could be 
* specified as:
* .CS
*     inet on ethernet: 90.1.0.1:ffff0000:1000
* .CE
*
* If no netmask is specified, the contents of the field could be:
* .CS
*     inet on ethernet: 90.1.0.1::ffffffff
* .CE
*
* In the first case, the lease duration for the address is 1000 seconds. The 
* second case indicates an infinite lease, and does not specify a netmask for
* the address. At the beginning of the boot process, the value of the lease
* duration field is used to specify the requested lease duration. If the field 
* not included, the value of DHCP_DEFAULT_LEASE is used
* instead.
* 
* The lease origin is specified with the same format as the lease duration,
* but is added during the boot process. The presence of the lease origin
* field distinguishes addresses assigned by a DHCP server from addresses
* entered manually. Addresses assigned by a DHCP server may be replaced
* if the bootstrap loader uses DHCP to obtain configuration parameters.
* The value of the lease origin field at the beginning of the boot process
* is ignored.
*
* This routine extracts the optional lease duration by replacing the preceding 
* colon in the specified string with an EOS and then scanning the remainder as 
* a number.  The lease duration and lease origin values are returned via
* the <pLeaseLen> and <pLeaseStart> pointers, if those parameters are not NULL.
*
* RETURNS:
*   2 if both lease values are specified correctly in <string>, or 
*  -2 if one of the two values is specified incorrectly.
* If only the lease duration is found, it returns:
*   1 if the lease duration in <string> is specified correctly,
*   0 if the lease duration is not specified in <string>, or
*  -1 if an invalid lease duration is specified in <string>.
*/

int bootLeaseExtract
    (
    char *string,       /* string containing addr field */
    u_long *pLeaseLen,  /* pointer to storage for lease duration */
    u_long *pLeaseStart /* pointer to storage for lease origin */
    )
    {
    FAST char *pDelim;
    FAST char *offset;
    int result;
    int status = 0;
    int start;
    int length;

    /* find delimeter for netmask */

    offset = index (string, ':');
    if (offset == NULL)
	return (0);		/* no subfield specified */

    /* Start search after netmask field. */

    pDelim = offset + 1;

    /* Search for lease duration field. */

    offset = index (pDelim, ':');
    if (offset == NULL)         /* No lease duration tag. */
        return (0);

    /* Start search after duration. */

    pDelim = offset + 1;

    status = bootSubfieldExtract (pDelim, &start, ':');
    if (status == 1 && pLeaseStart != NULL)
        *pLeaseStart = start;

    /* Reset search pointer to obtain lease duration. */

    offset = index (string, ':');
    if (offset == NULL)      /* Sanity check - should not occur. */
        return (0);

   /* Lease duration follows netmask. */

    pDelim = offset + 1;

    /* Store lease duration if found. */

    result = bootSubfieldExtract (pDelim, &length, ':');
    if (result == 1 && pLeaseLen != NULL)
        *pLeaseLen = length;

    if (status != 0)    /* Both lease values were present. */
        {
        if (status < 0 || result < 0)    /* Error reading one of the values. */
            return (-2);
        else
            return (2);     /* Both lease values read successfully. */
        }

    return (result);
    }

/*******************************************************************************
*
* bootNetmaskExtract - extract the net mask field from an Internet address
*
* This routine extracts the optional subnet mask field from an Internet address
* field.  Subnet masks can be specified for an Internet interface by appending
* to the Internet address a colon and the net mask in hexadecimal.  
* For example, the "inet on ethernet" field of the boot parameters could 
* be specified as:
* .CS
*     inet on ethernet: 90.1.0.1:ffff0000
* .CE
* In this case, the network portion of the address (normally just 90)
* is extended by the subnet mask (to 90.1).  This routine extracts the
* optional trailing subnet mask by replacing the colon in the specified
* string with an EOS and then scanning the remainder as a hex number.
* This number, the net mask, is returned via the <pNetmask> pointer.
* 
* This routine also handles an empty netmask field used as a placeholder
* for the lease duration field (see bootLeaseExtract() ). In that case,
* the colon separator is replaced with an EOS and the value of netmask is
* set to 0. 
*
* RETURNS:
*   1 if the subnet mask in <string> is specified correctly,
*   0 if the subnet mask in <string> is not specified, or
*  -1 if an invalid subnet mask is specified in <string>.
*/

STATUS bootNetmaskExtract
    (
    char *string,       /* string containing addr field */
    int *pNetmask       /* pointer where to return net mask */
    )
    {
    FAST char *pDelim;
    char *offset;

    /* find delimeter */

    pDelim = index (string, ':');
    if (pDelim == NULL)
	return (0);		/* no subfield specified */

    /* Check if netmask field precedes timeout field. */
    offset = pDelim + 1;
    skipSpace(&offset);
    if (*offset == ':' || *offset == EOS)  /* Netmask field is placeholder. */
        {
         *pDelim = EOS;
         *pNetmask = 0;
         return (1);
        }
         
    return (bootSubfieldExtract (string, pNetmask, ':'));
    }

/******************************************************************************
*
* bootBpAnchorExtract - extract a backplane address from a device field
*
* This routine extracts the optional backplane anchor address field from a
* boot device field.  The anchor can be specified for the backplane
* driver by appending to the device name (i.e., "bp") an equal sign (=) and the
* address in hexadecimal.  For example, the "boot device" field of the boot
* parameters could be specified as:
* .CS
*     boot device: bp=800000
* .CE
* In this case, the backplane anchor address would be at address 0x800000,
* instead of the default specified in config.h.
*
* This routine picks off the optional trailing anchor address by replacing
* the equal sign (=) in the specified string with an EOS and then scanning the
* remainder as a hex number.
* This number, the anchor address, is returned via the <pAnchorAdrs> pointer.
*
* RETURNS:
*   1 if the anchor address in <string> is specified correctly,
*   0 if the anchor address in <string> is not specified, or
*  -1 if an invalid anchor address is specified in <string>.
*/

STATUS bootBpAnchorExtract
    (
    char *string,       /* string containing adrs field */
    char **pAnchorAdrs  /* pointer where to return anchor address */
    )
    {
    return (bootSubfieldExtract (string, (int *) pAnchorAdrs, '='));
    }


/******************************************************************************
*
* bootSubfieldExtract - extract a numeric subfield from a boot field
*
* Extracts subfields in fields of the form "<field><delimeter><subfield>".
* i.e. <inet>:<netmask> and bp=<anchor>
*/

LOCAL STATUS bootSubfieldExtract
    (
    char *string,       /* string containing field to be extracted */
    int *pValue,        /* pointer where to return value */
    char delimeter      /* character delimeter */
    )
    {
    FAST char *pDelim;
    int value;

    /* find delimeter */

    pDelim = index (string, delimeter);
    if (pDelim == NULL)
	return (0);		/* no subfield specified */

    /* scan remainder for numeric subfield */

    string = pDelim + 1;

    if (bootScanNum (&string, &value, TRUE) != OK)
	return (-1);		/* invalid subfield specified */

    *pDelim = EOS;		/* terminate string at the delimeter */
    *pValue = value;		/* return value */
    return (1);			/* valid subfield specified */
    }
/*******************************************************************************
*
* addAssignNum - add a numeric value assignment to a string
*/

LOCAL void addAssignNum
    (
    FAST char *string,
    char *code,
    int value
    )
    {
    if (value != 0)
	{
	string += strlen (string);
	sprintf (string, (value <= 7) ? " %s=%d" : " %s=0x%x", code, value);
	}
    }
/*******************************************************************************
*
* addAssignString - add a string assignment to a string
*/

LOCAL void addAssignString
    (
    FAST char *string,
    char *code,
    char *value
    )
    {
    if (value[0] != EOS)
	{
	string += strlen (string);
	sprintf (string, " %s=%s", code, value);
	}
    }
/*******************************************************************************
*
* getWord - get a word out of a string
*
* Words longer than the specified max length are truncated.
*
* RETURNS: TRUE if word is successfully extracted from string, FALSE otherwise;
* Also updates ppString to point to next character following extracted word.
*/

LOCAL BOOL getWord
    (
    char **ppString,    /* ptr to ptr to string from which to get word */
    FAST char *pWord,   /* where to return word */
    int length,         /* max length of word to get including EOS */
    char *delim         /* string of delimiters that can terminate word */
    )
    {
    FAST char *pStr;

    skipSpace (ppString);

    /* copy up to any specified delimeter, EOS, or max length */

    pStr = *ppString;
    while ((--length > 0) && (*pStr != EOS) && (index (delim, *pStr) == 0))
	*(pWord++) = *(pStr++);

    *pWord = EOS;


    /* if we copied anything at all, update pointer and return TRUE */

    if (pStr != *ppString)
	{
	*ppString = pStr;
	return (TRUE);
	}


    /* no word to get */

    return (FALSE);
    }
/*******************************************************************************
*
* getConst - get a constant string out of a string
*
* case insensitive compare for identical strings
*/

LOCAL BOOL getConst
    (
    char **ppString,
    FAST char *pConst
    )
    {
    FAST int ch1;
    FAST int ch2;
    FAST char *pString;

    skipSpace (ppString);

    for (pString = *ppString; *pConst != EOS; ++pString, ++pConst)
	{
	ch1 = *pString;
	ch1 = (isascii (ch1) && isupper (ch1)) ? tolower (ch1) : ch1;
	ch2 = *pConst;
	ch2 = (isascii (ch2) && isupper (ch2)) ? tolower (ch2) : ch2;

	if (ch1 != ch2)
	    return (FALSE);
	}

    /* strings match */

    *ppString = pString;
    return (TRUE);
    }
/*******************************************************************************
*
* getNum - get a numeric value from a string
*/

LOCAL BOOL getNum
    (
    FAST char **ppString,
    int *pNum
    )
    {
    skipSpace (ppString);

    if (sscanf (*ppString, "%d", pNum) != 1)
	return (FALSE);

    /* skip over number */

    while (isdigit (**ppString))
	(*ppString)++;

    return (TRUE);
    }
/*******************************************************************************
*
* getAssign - get an assignment out of a string
*/

LOCAL BOOL getAssign
    (
    FAST char **ppString,
    char *valName,
    char *pVal,
    int maxLen
    )
    {
    skipSpace (ppString);
    if (!getConst (ppString, valName))
	return (FALSE);

    skipSpace (ppString);
    if (!getConst (ppString, "="))
	return (FALSE);

    return (getWord (ppString, pVal, maxLen, " \t"));
    }
/*******************************************************************************
*
* getAssignNum - get a numeric assignment out of a string
*/

LOCAL BOOL getAssignNum
    (
    FAST char **ppString,
    char *valName,
    int *pVal
    )
    {
    char buf [BOOT_FIELD_LEN];
    char *pBuf = buf;
    char *pTempString;
    int tempVal;

    /* we don't modify *ppString or *pVal until we know we have a good scan */

    /* first pick up next field into buf */

    pTempString = *ppString;
    if (!getAssign (&pTempString, valName, buf, sizeof (buf)))
	return (FALSE);

    /* now scan buf for a valid number */

    if ((bootScanNum (&pBuf, &tempVal, FALSE) != OK) || (*pBuf != EOS))
	return (FALSE);

    /* update string pointer and return value */

    *ppString = pTempString;
    *pVal = tempVal;

    return (TRUE);
    }

/*******************************************************************************
*
* printClear - print string with '?' for unprintable characters
*/

LOCAL void printClear
    (
    FAST char *param
    )
    {
    FAST char ch;

    while ((ch = *(param++)) != EOS)
	printf ("%c", (isascii (ch) && isprint (ch)) ? ch : '?');
    }
/*******************************************************************************
*
* printParamNum - print number parameter
*/

LOCAL void printParamNum
    (
    char *paramName,
    int  value,
    BOOL hex
    )
    {
    printf (hex ? "%-*s: 0x%x \n" : "%-*s: %d \n", PARAM_PRINT_WIDTH,
	    paramName, value);
    }
/*******************************************************************************
*
* printParamString - print string parameter
*/

LOCAL void printParamString
    (
    char *paramName,
    char *param
    )
    {
    if (param [0] != EOS)
	{
	printf ("%-*s: ", PARAM_PRINT_WIDTH, paramName);
	printClear (param);
	printf ("\n");
	}
    }

/*******************************************************************************
*
* promptParamBootDevice - prompt the user for the boot device
*
* This routine reads the boot device name, and an optional unit number.
* If the unit number is not supplied, it defaults to 0.
*
* - carriage return leaves the parameter unmodified;
* - "." clears the parameter (null boot device and 0 unit number).
*/
LOCAL int promptParamBootDevice
    (
    char *paramName,
    FAST char *param,
    int *pValue,
    int sizeParamName
    )
    {
    FAST int i;
    int value;
    char buf[BOOT_FIELD_LEN];
    char *pBuf;

    sprintf(buf, "%s%d", param, *pValue);
    printf ("%-*s: ", PARAM_PRINT_WIDTH, strBootDevice);
    printClear (buf);
    if (*buf != EOS)
	printf (" ");

    if ((i = promptRead (buf, sizeParamName)) != 0)
	return (i);

    if (buf[0] == '.')
	{
	param[0] = EOS;               /* just '.'; make empty fields */
        *pValue = 0;
	return (1);
	}

    i = 0;

    /* Extract first part of name of boot device. */
    while (!isdigit(buf[i]) && buf[i] != '=' && buf[i] != EOS)
       {
        param[i] = buf[i];
        i++;
       }
    param[i] = EOS;

    /* Extract unit number, if specified. */
    if (isdigit(buf[i]))     /* Digit before equal sign is unit number. */
      {
       pBuf = &buf[i];
       if (bootScanNum (&pBuf, &value, FALSE) != OK)
          {
	  printf ("invalid unit number.\n");
          return (-98);
          }
       strcat(param, pBuf);
      }
   else                          /* No unit number specified. */
      {
       value = 0;
       if (buf[i] == '=')
          strcat(param, &buf[i]);    /* Append remaining boot device name. */
      }

    *pValue = value; 
    return (1);
    }

/*******************************************************************************
*
* promptParamString - prompt the user for a string parameter
*
* - carriage return leaves the parameter unmodified;
* - "." clears the parameter (null string).
*/

LOCAL int promptParamString
    (
    char *paramName,
    FAST char *param,
    int sizeParamName
    )
    {
    FAST int i;
    char buf [BOOT_FIELD_LEN];

    printf ("%-*s: ", PARAM_PRINT_WIDTH, paramName);
    printClear (param);
    if (*param != EOS)
	printf (" ");

    if ((i = promptRead (buf, sizeParamName)) != 0)
	return (i);

    if (buf[0] == '.')
	{
	param [0] = EOS;	/* just '.'; make empty field */
	return (1);
	}

    strcpy (param, buf);	/* update parameter */
    return (1);
    }
/*******************************************************************************
*
* promptParamNum - prompt the user for a parameter
*
* - carriage return leaves the parameter unmodified;
* - "." clears the parameter (0)
*/

LOCAL int promptParamNum
    (
    char *paramName,
    int *pValue,
    BOOL hex
    )
    {
    char buf [BOOT_FIELD_LEN];
    char *pBuf;
    int value;
    int i;

    printf (hex ? "%-*s: 0x%x " : "%-*s: %d ", PARAM_PRINT_WIDTH, paramName,
	    *pValue);

    if ((i = promptRead (buf, sizeof (buf))) != 0)
	return (i);

    if (buf[0] == '.')
	{
	*pValue = 0;		/* just '.'; make empty field (0) */
	return (1);
	}

    /* scan for number */

    pBuf = buf;
    if ((bootScanNum (&pBuf, &value, FALSE) != OK) || (*pBuf != EOS))
	{
	printf ("invalid number.\n");
	return (-98);
	}

    *pValue = value;
    return (1);
    }
/*******************************************************************************
*
* promptRead - read the result of a prompt and check for special cases
*
* Special cases:
*    '-'  = previous field
*    '^D' = quit
*    CR   = leave field unchanged
*    too many characters = error
*
* RETURNS:
*    0 = OK
*    1 = skip this field
*   -1 = go to previous field
*  -98 = ERROR
*  -99 = quit
*/

LOCAL int promptRead
    (
    char *buf,
    int bufLen
    )
    {
    FAST int i;

    i = fioRdString (STD_IN, buf, bufLen);

    if (i == EOF)
	return (-99);			/* EOF; quit */

    if (i == 1)
	return (1);			/* just CR; leave field unchanged */

    if ((i == 2) && (buf[0] == '-'))
	return (-1);			/* '-'; go back up */

    if (i >= bufLen)
	{
	printf ("too big - maximum field width = %d.\n", bufLen);
	/* We mustn't take into account the end of the string */ 
	while((i = fioRdString(STD_IN, buf, bufLen)) >= bufLen);
	return (-98);
	}

    return (0);
    }
/*******************************************************************************
*
* skipSpace - advance pointer past white space
*
* Increments the string pointer passed as a parameter to the next
* non-white-space character in the string.
*/

LOCAL void skipSpace
    (
    FAST char **strptr  /* pointer to pointer to string */
    )
    {
    while (isspace (**strptr))
	++*strptr;
    }
/******************************************************************************
*
* bootScanNum - scan string for numeric value
*
* This routine scans the specified string for a numeric value.  The string
* pointer is updated to reflect where the scan stopped, and the value is
* returned via pValue.  The value will be scanned by default in decimal unless
* hex==TRUE.  In either case, preceding the value with "0x" or "$" forces
* hex.  Spaces before and after number are skipped.
*
* RETURNS: OK if value scanned successfully, otherwise ERROR.
*
* EXAMPLES:
*	bootScanNum (&string, &value, FALSE);
*    with:
*	string = "   0xf34  ";
*	    returns OK, string points to EOS, value=0xf34
*	string = "   0xf34r  ";
*	    returns OK, string points to 'r', value=0xf34
*	string = "   r0xf34  ";
*	    returns ERROR, string points to 'r', value unchanged
*
* NOMANUAL
*/

STATUS bootScanNum
    (
    FAST char **ppString,
    int *pValue,
    FAST BOOL hex
    )
    {
    FAST char *pStr;
    FAST char ch;
    FAST int n;
    FAST int value = 0;

    skipSpace (ppString);

    /* pick base */

    if (**ppString == '$')
	{
	++*ppString;
	hex = TRUE;
	}
    else if (strncmp (*ppString, "0x", 2) == 0)
	{
	*ppString += 2;
	hex = TRUE;
	}


    /* scan string */

    pStr = *ppString;

    FOREVER
	{
	ch = *pStr;

	if (!isascii (ch))
	    break;

	if (isdigit (ch))
	    n = ch - '0';
	else
	    {
	    if (!hex)
		break;

	    if (isupper (ch))
		ch = tolower (ch);

	    if ((ch < 'a') || (ch > 'f'))
		break;

	    n = ch - 'a' + 10;
	    }

	value = (value * (hex ? 16 : 10)) + n;
	++pStr;
	}


    /* error if we didn't scan any characters */

    if (pStr == *ppString)
	return (ERROR);

    /* update users string ptr to point to character that stopped the scan */

    *ppString = pStr;
    skipSpace (ppString);

    *pValue = value;

    return (OK);
    }
