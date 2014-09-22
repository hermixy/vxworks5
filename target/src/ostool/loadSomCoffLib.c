/* loadSomCoffLib.c - HP-PA SOM COFF object module loader */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"
/*
modification history
--------------------
02n,30nov98,dbt  no longer clear seg.flags<xxx> after loadSegmentAllocate()
                 call. (SPR #23553).
02m,05oct98,pcn  Initialize all the fields in the SEG_INFO structure.
02l,16sep98,pcn  Set to SEG_ALIGN the flags field in seg structure
                 (SPR #21836).
02k,19aug98,cym  Fixed SPR #22030: Target Loader won't load partially linked
		 objects.
02j,17jul98,pcn  Fixed SPR #21836: alignment mismatch between sections and
                 target.
02i,01apr97,jmb  propagated an arg_reloc patch from the target server loader.
02h,14nov96,mem  rewrote handling of branch stubs in order to fix some
		 link problems.  Stubs now have the arg_reloc bits of the
		 symbol they represent stored with them, rather than
		 the bits of the call they where originally created with.
02g,02aug96,jmb  merged in ease patch (mem) for sign_ext.
02f,19dec95,mem  fixed fieldValue() function which did not always use the
		 roundConst value.
		 Modified section placement for GNU.
		 Fixed B{1,2,3,4} macros which had multiple uses of
		 pThisFixup++ in a single expression, (which is
		 undefined behavior).
02e,14nov94,kdl  removed conditional for CPU==SIMHPPA.
02d,09nov94,ms   made R_UNINIT fixup act just like R_ZEROS fixup.
02c,07nov94,kdl  merge cleanup - made conditional for CPU==SIMHPPA.
02b,02aug94,ms   cleaned up the code a bit.
02a,07jul94,ms   too many bug fixes to count. Major rewrite.
                 Added "long branch stub" generation.
                 Added "parameter relocation stub" generation
                 Added "unwind" segment generation.
01g,03may94,ms   fixed the handling of R_PREV_FIXUP requests.
01f,02may94,ms   checked in yaos work. Added minor bug fix (bssFlag).
01e,18apr94,yao  added fileType paramater to rdSymtab().  changed to add
                 DATA_SEGMENT_BASE to symbol of data type for shared 
                 object files.
01d,28mar94,yao  changed to call lst operation routines in relSegments().
01c,28feb94,yao  added loadBssSizeGet () for relocatable files.  removed 
                 macro STORE(), added routine fixBits() to deposit relocation 
                 bits.  added fieldGetDefault(). added deassemble control 
                 functions de_assemble_X(). added fixup rounding functions 
                 X_fixup().
01c,02feb94,yao  included dsmLib.h.
01b,06dec93,gae  fixed warnings.
01a,12sep93,yao  written.
*/

/*
DESCRIPTION
This library provides an object module loading facility for the
HP-PA compiler environment.  Relocatable SOM COFF format files may be 
loaded into memory, relocated, their external references resolved, 
and their external definitions added to the system symbol table for use by 
other modules and from the shell.  Modules may be loaded from any I/O stream
which supports FIOSEEK.

EXAMPLE
.CS
    fdX = open ("/devX/objFile", O_RDONLY);
    loadModule (fdX, ALL_SYMBOLS);
    close (fdX);
.CE
This code fragment would load the HP-PA SOM file "objFile" located on
device "/devX/" into memory which would be allocated from the system
memory pool.  All external and static definitions from the file would be
added to the system symbol table.

This could also have been accomplished from the shell, by typing:
.CS
    -> ld (1) </devX/objFile
.CE

INCLUDE FILE: loadSomCoffLib.h

SEE ALSO: loadLib, usrLib, symLib, memLib,
.pG "Basic OS"
*/

/*
INTERNAL

The HP-PA SOM COFF linking loader is complicated as hell.
This is due to a combination of the fact that the PA-RISC
has a very un-RISC like instruction set, and that the
calling conventions have a strange parameter passing mechanism.
The following is an attempt to describe
some of the differences between this loader and other VxWorks
loaders.

Subspace to segment mapping
---------------------------
The current VxWorks paradigm assumes there are three "segments" -
text, data, and bss. The SOM COFF format instead uses the concept
of spaces and subspaces. There are two loadable spaces; text and data.
Each of these spaces is divided into many subspaces.
For example, each function is put in its own text subspace,
and each global variable is put in its own data subspace.

To fit the VxWorks paradigm we must map the subspaces into VxWorks segments.
The routine subspaceToSegment() performs this function by looping through
each subspace and deciding what segment to put it in. The array pSubInfo
is initialized with information on each subspace: It's loadType (text,
data, or bss), and it's loadAddr (we compute a relative load address for
each subspace - it's final load address won't be know until we
allocate memory for the text, data, and bss segments).

Uninitailized global (but non-static) variables are treated by the
compiler as "request for storage" (sometimes called "common data" or
"weak externals"). All VxWorks loaders simply add these variables
as bss, so we do likewise. The routine commSizeGet() computes the number
of bytes needed for these variables, and this size is added to the bss size.
This is more efficient than performing a malloc() for each variable,
as is done in other VxWorks loaders.

A "stack unwind" segment is also generated for the stack tracer to use.
See src/arch/simhppa/trcLib.c for details.

Long branch stubs
-----------------
When code is compiled with optimization, the compiler generates
a PC-relative "BL" (branch and link) instruction for all procedure calls.
The problem is that the displacement must fit into 19 bits, so if the
called procedure is further than 256k bytes away, we have no way to
fixup the instruction.

The solution is to generate a two instruction "long branch stub",
and make that stub the target of the "BL" instruction. It is our
responsibility to generate this stub, and to make sure that the stub
itself is close enough to the caller of "BL".

For each text subspace, the procedure subspaceSizeExtra() computes
the amount of space needed by that subspace for possible long branch stubs.
Thus when we map the text subspaces to the "text segment", we actually leave
a bit of room between each subspace according to the value of
subspaceSizeExtra() for that subspace.

Parameter relocation
--------------------
The PA-RISC passes the first four words of the parameter list in
registers, and the return value also goes in a register.
floating point parameters are passed in floating point registers,
and other parameters are passed in general purpose registers.
The problem is that the caller and the callee may not agree as
to where the parameters are located. For example, printf() expects
its parameters to be in the general purpose registers, whereas a call
like
   printf ("%f", myFloat)
passes the second parameter in a floating point register.
The compiler generates information on how a procedure expects
to be called, and on how it is actually called. It is our job to
make sure they match, and if they don't, to insert a "parameter
relocation stub" between the caller and the callee.

Parameter relocation information is encoded in 10 bits - 2 bits
each for the first four arguments words and the return value.
Each 2 bit quantity specifies "unused, general register, float
register, or double persion float register".

When a new function is loaded, the 10 bit relocation info generated
by the compiler must be stored away in the VxWorks symbol table for
future reference, In order to do this I had to
create a new structure called "MY_SYMBOL", which is just like
the VxWorks "SYMBOL", but has an extra 8 bit field at the end. It turns
out that the size of MY_SYMBOL is the same as the size of SYMBOL
because of compiler padding, so no harm was done. The functions
externSymResolve() and externSymAdd() perform VxWorks symbol
table manipulation with MY_SYMBOL stuctures.

When a function is called, the 10 parameter relocation bits
are encoded in the fixup request associated with the function call.
The routines rbits1() and rbits2() get the bits from the fixup request
(actually, the 10 bits are encoded in 9 or fewer bits in the fixup
request, so rbits1() and rbits2() do some work to extract the info
and decode it to 10 bits). We then compare the callers reclocation
bits with the callees and if they differ, we may ned to generate
a stub (if we hadn't previously generated a stub for that function).

Currently, this is only done if the callee expects general registers
and the caller passses a double - which takes care of functions
like printf, sprintf, etc. The loader prints an error message if
any other type of parameter reloction is needed. It would be easy to add
other parameter relocation stubs at a later time.

Fixup requests
--------------
The linking fixup requests are passed to us as a byte stream.
The number of bytes in each fixup request depends on the
fixup request, and there are 71 different types of fixup requests!
The routine linkSubspaces() performs the linker fixup requests, and
it is complicated as hell. The comments in that routine describe it further.
just to give you an idea of how complicated our friends at HP have made
things: The linker must keep a queue of the last four multi-byte
fixup requests. There is a one byte fixup requests called "repeat
previous fixup" which directs us to repeat one of the previous fixups
from the queue, and then move the fixup to the front of the queue!
Currently there are many branches in linkSubspaces() that have
not been tested.
*/

#include "vxWorks.h"
#include "stdio.h"
#include "som_coff.h"
#include "ioLib.h"
#include "dsmLib.h"
#include "fioLib.h"
#include "bootLoadLib.h"
#include "loadLib.h"
#include "loadSomCoffLib.h"
#include "memLib.h"
#include "pathLib.h"
#include "lstLib.h"
#include "string.h"
#include "symLib.h"
#include "sysSymTbl.h"
#include "errnoLib.h"
#include "stdlib.h"
#include "symbol.h"
#include "moduleLib.h"
#include "cacheLib.h"



#define LOAD_NONE   0
#define LOAD_TEXT   1
#define LOAD_DATA   2
#define LOAD_BSS    3

#define SEG_ALIGN  8

/* Some PA-RISC opCodes */

#define LDIL_CODE       0x08
#define ADDIL_CODE      0x0a
#define BL_CODE         0x3a
#define COMBT_CODE      0x20
#define COMBF_CODE      0x22
#define ADDB_CODE       0x28
#define ADDBF_CODE      0x2a
#define BB_CODE         0x31
#define LDW_CODE        0x12
#define LDH_CODE        0x11
#define LDB_CODE        0x10
#define STW_CODE        0x1a
#define STH_CODE        0x19
#define STB_CODE        0x18
#define LDWM_CODE       0x13
#define STWM_CODE       0x1b
#define LDO_CODE        0x0d
#define ADDI_CODE       0x2d
#define ADDIT_CODE      0x2c
#define SUBI_CODE       0x25
#define COMICKR_CODE    0x24
#define ADDIBT_CODE     0x29
#define ADDIBF_CODE     0x2b
#define COMIBT_CODE     0x21
#define COMIBF_CODE     0x23
#define BE_CODE         0x38
#define BLE_CODE        0x39

/* some handy macros for linkSubspaces() */

#define B1 (*(pThisFixup++))
#define B2 ((int)(pThisFixup += 2), \
    (int) ((pThisFixup[-2] << 8) + pThisFixup[-1]))
#define B3 ((int)(pThisFixup += 3), \
    (int) ((pThisFixup[-3] << 16) + (pThisFixup[-2] << 8) + pThisFixup[-1]))
#define B4 ((int)(pThisFixup += 4), \
    (int) ((pThisFixup[-4] << 24) + (pThisFixup[-3] << 16) \
           + (pThisFixup[-2] << 8) + pThisFixup[-1]))

#define REL_STACK_SIZE  (10000)   /* size of relocation stack pointer */
#define POP_EXPR        (*--sp)
#define PUSH_EXPR(x)    (*sp ++ = (x))
#define CHECK_STACK_OVER  if (sp >= pExpStack + REL_STACK_SIZE) \
                              { \
                              printErr ("ld error: stack overflow\n");\
                              status = ERROR; \
                              }
#define CHECK_STACK_UNDER if (sp <= pExpStack) \
                              { \
                              printErr ("ld error: stack underflow\n");\
                              status = ERROR; \
                              }
#define FLAG_RESET(flag)  if (flag) flag = FALSE;

/* subspace relocation info */

typedef struct
    {
    char * loadAddr;
    int loadType;
    } SUB_INFO;

/* "previous fixup" queue */

typedef struct
    {
    NODE node;
    UCHAR *pFixup;
    } PREV_FIXUP;

/* stack unwind structures */

typedef struct
    {
    char *startAddr;
    char *endAddr;
    UINT word3;
    UINT word4;
    } UNWIND_DESC;

typedef struct
    {
    NODE node;
    UNWIND_DESC unwindDesc;
    } UNWIND_NODE;

/* list of long branch stubs */

typedef struct stubListNode
    {
    NODE node;
    int symNum;
    int arg_reloc;
    } STUB_LIST_NODE;

/* list of argument relocation stubs */

typedef struct
    {
    NODE     node;
    SYMREC * pSym;
    int      arg_reloc;
    char *   stubAddr;
    } ARG_RELOC_NODE;

/* modified symbol table entry */

typedef struct                  /* MY_SYMBOL - entry in symbol table */
    {
    SL_NODE     nameHNode;      /* hash node (must come first) */
    char        *name;          /* pointer to symbol name */
    char        *value;         /* symbol value */
    UINT16      group;          /* symbol group */
    SYM_TYPE    type;           /* symbol type */
    UCHAR       arg_reloc;      /* relocation bits */
    } MY_SYMBOL;

/* local variables */

/* SPR 22030: changed entry for fixup 62 from 0 to 1 */

static int fixupLen [256] =
    {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00 - 19 */
    1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 2, 4, 2, 4, 1, 2, 4, 2, /* 20 - 39 */
    4, 1, 2, 3, 5, 8, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, /* 40 - 59 */
    5, 5, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 5, 5, 0, 0, /* 60 - 79 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 80 - 99 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 0, 0, 0, 0, 0, 0, /* 100 - 119 */
    2, 4, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 120 - 139 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 140 - 159 */
    2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 2, 4, 1, 9, /* 160 - 179 */
    6, 1, 1, 1, 1, 2, 4, 1, 1, 2, 3, 4, 1, 1, 1, 1, 1, 1, 1, 1, /* 180 - 199 */
    1, 1, 2, 3, 4, 5, 1,12, 2, 5, 6, 1, 1, 1, 1, 1, 0, 0, 0, 0, /* 200 - 219 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 220 - 239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0              /* 240 - 255 */
    };

static UINT longBranchStub[] =
        {
        0x20200000,     /* LDIL    0, r1 */
        0xe0202002      /* BE,n    0(s4, r1) */
        };

static stubSize = 8;

/* externals */

extern low_sign_ext (int x, int len);
extern sign_ext     (int x, int len);
extern assemble_21  (int x);
extern assemble_17  (int x, int y, int z);
extern assemble_12  (int x, int y);
extern sysMemTop    (void);

/* forward static functions */

	/* The entry point to the loader */

static MODULE_ID ldSomCoffMdlAtSym (int fd, int symFlag, char **ppText, 
    char **ppData, char **ppBss, SYMTAB_ID symTbl);

	/* primary loader support functions */

static void symbolNameSet (SYMREC *pSym, int nSyms, char *pSymStr,
    SUBSPACE *pSubspace, int nSubs, char *pSpaceStr, SPACE *pSpace,
    int nSpaces);

static void subspaceToSegment (SOM_HDR *pHpHdr, UCHAR *pRelocCmds,
    SUB_INFO *pSubInfo,
    SPACE *pSpace, SUBSPACE *pSubspace, SYMREC *pSym,
    SYMTAB_ID symTbl, int *pTextSize, int *pDataSize,
    int *pBssSize);

static int subspaceSizeExtra (UCHAR *pRelCmds, SUBSPACE *pSubspace,
    SYMREC *pSym, int numSyms, SYMTAB_ID symTbl);

static void subspaceNoReloc (SOM_HDR *pHpHdr,
    SUB_INFO *  pSubInfo, SUBSPACE *  pSubspace);

static void commSizeGet (SYMREC *pSym, int nEnts, SUBSPACE *pSubspace,
    int *pCommSize);

static STATUS rdSymtab (SYMREC * pSym, FAST int nEnts,
    SUBSPACE *pSubspace, SUB_INFO *pSubInfo, int symFlag,
    SYMTAB_ID symTbl, SYMREC **pSymGlobal, char *pComm, int group);

static STATUS loadSubspaces (int fd, SOM_HDR *pHpHdr, SUB_INFO *pSubInfo, 
    SUBSPACE *pSubspace, char *pText, char *pData, char *pBss);

static STATUS linkSubspaces (UCHAR *pRelCmds, SUBSPACE *pScnHdr, 
    int numSubspace, SUB_INFO *pSubInfo, SYMREC *pSym, int numSyms,
    SYMTAB_ID pSymTab, SYMREC *pSymGlobal, MODULE_ID moduleId);

	/* Symbol table manipulation functions */

static BOOL externSymResolve (SYMTAB_ID symTblId, char * name, SYMREC *pSym);

static BOOL externSymAdd (SYMTAB_ID symTblId, char * name, char *value,
    SYM_TYPE type, UINT16 group, UCHAR arg_reloc);

	/* "argument relocation stub" functions */

static BOOL argRelocDiffer (int arg_reloc1, int arg_reloc2);

static char *argRelocStubListFind (LIST *stubList, SYMREC *pSym,
    int arg_reloc, MODULE_ID moduleId);

static void rbitsShow (int rbits);

static int  rbits1 (int x);

static int  rbits2 (int x);

	/* "long branch stub" manipulation functions */

static int stubListFind (LIST *stubList, int symNum, int arg_reloc);

static int stubListAdd (LIST *stubList, int symNum, int arg_reloc);

static void stubInit (char *stubAddr, char *procAddr);

	/* stack unwind manipulation functions */

static void unwindListAdd (LIST *unwindList, UNWIND_NODE *pUnwindNode);

static void unwindSegmentCreate (LIST *unwindList, MODULE_ID moduleId);

int loadSomCoffDebug = 0;
#define DBG_PUT if (loadSomCoffDebug == 1) printf
int loadSomCoffWarn  = 0;
#define WARN if (loadSomCoffWarn == 1) printf

/*******************************************************************************
*
* loadSomCoffInit - initialize the system for HP-PA SOM COFF load modules
*
* This routine initialized VxWorks to use an extended HP-PA SOM COFF format for
* loading modules.
*
* RETURNS: OK
*
* SEE ALSO: loadModuleAt()
*/

STATUS loadSomCoffInit (void)
    {
    loadRoutine = (FUNCPTR) ldSomCoffMdlAtSym;
    return (OK);
    }

/******************************************************************************
*
* ldSomCoffMdlAtSym - load object module into memory with symbol table
*
* This routine is the underlying routine to loadModuleAtSym().  This interface
* allows specification of the the symbol table used to resolve undefined
* external references and to which to add new symbols.
*
* RETURNS:
* MODULE_ID, or
* NULL if can't read file or not enough memory or illegal file format
*/  

static MODULE_ID ldSomCoffMdlAtSym
    (
    FAST int  fd,        /* fd from which to read module */
    int       symFlag,   /* symbols to be added to table 
                          *   ([NO | GLOBAL | ALL]_SYMBOLS) */
    char **   ppText,    /* load text segment at address pointed to by this
                          * pointer, return load address via this pointer */
    char **   ppData,    /* load data segment at address pointed to by this
                          * pointer, return load address via this pointer */
    char **   ppBss,     /* load bss segment at address pointed to by this
                          * pointer, return load address via this pointer */
    SYMTAB_ID symTbl     /* symbol table to use */
    )
    {
    char *    pText = (ppText == NULL) ? LD_NO_ADDRESS : *ppText;
    char *    pData = (ppData == NULL) ? LD_NO_ADDRESS : *ppData;
    char *    pBss  = (ppBss  == NULL) ? LD_NO_ADDRESS : *ppBss;
    char *    pComm;
    int       textSize;
    int       dataSize;
    int       bssSize;
    int       commSize;        		/* gets added to bssSize */
    int       status = ERROR;		/* function return value */
    SEG_INFO  seg;                      /* file segment info */
    char      fileName[255];		/* file being loaded */
    UINT16    group;			/* loader group */
    MODULE_ID moduleId;			/* module ID */

    SOM_HDR   somHdr;                   /* SOM header */
    SPACE *   pSpace = NULL;            /* space dictionary */
    int       spaceSize;                /* number of spaces */
    SUBSPACE *pSubspace = NULL;		/* subspace dictionary */
    int       subspaceSize;		/* number of subspaces */
    char *    pSpaceStrings = NULL;	/* space/subspace names */
    SYMREC *  pSymBuf = NULL;           /* symbol record dictionary */
    int       symBufSize;               /* number of symbols */
    char *    pSymStr = NULL;           /* symbol names */
    FIXUP *   pReloc = NULL;		/* linker fixup requests */
    int       relSize;                	/* size of fixup requests */

    SUB_INFO *pSubInfo = NULL;		/* subspace load info */
    SYMREC *  pGlobal = NULL;		/* the symbol "$global$" */

    /* initialization */

    memset ((void *)&seg, 0, sizeof (seg));

    /* Get the name of the file we are loading */

    if (ioctl (fd, FIOGETNAME, (int) fileName) == ERROR)
        {
        printErr ("ldSomCoffMdlAtSym: can't get filename\n");
        return (NULL);
        }
    
    if ((moduleId = loadModuleGet (fileName, MODULE_ECOFF, &symFlag)) == NULL)
        return (NULL);

    group = moduleId->group;

    /* read the SOM header */

    bzero ((char *) &somHdr, sizeof(SOM_HDR));
    if (read (fd, (char *)&somHdr, sizeof (SOM_HDR)) != sizeof(SOM_HDR))
        {
        errno = S_loadSomCoffLib_HDR_READ;
        goto error;
        }

    /* check object file type - only relocatable and executable are supported */

    if (somHdr.a_magic != RELOC_MAGIC &&
        somHdr.a_magic != EXEC_MAGIC &&
        somHdr.a_magic != SHARE_MAGIC)
        {
        printErr ("ldSomCoffMdlAtSym: bad magic #%#x\n", somHdr.a_magic);
        errno = S_loadSomCoffLib_OBJ_FMT;
        goto error;
        }

    /* read in space dictionary */

    if (somHdr.space_total != 0)
        {
        spaceSize=somHdr.space_total*sizeof(struct space_dictionary_record);
        if ((pSpace = calloc (1, spaceSize)) == NULL)
            {
            errno = S_loadSomCoffLib_SPHDR_ALLOC;
            goto error;
            }
        if (ioctl (fd, FIOSEEK, somHdr.space_location) == ERROR ||
            read (fd, (char*)pSpace, spaceSize) != spaceSize)
            {
            errno = S_loadSomCoffLib_SPHDR_READ;
            goto error;
            }
        }

    /* read in subspace dictionary */

    if (somHdr.subspace_total != 0)
        {
        subspaceSize = somHdr.subspace_total * SBPSZ;
        if ((pSubspace = calloc (1, subspaceSize)) == NULL)
            {
            errno = S_loadSomCoffLib_SUBSPHDR_ALLOC;
            goto error;
            }

        if (ioctl (fd, FIOSEEK, somHdr.subspace_location) == ERROR ||
            read (fd, (char*)pSubspace, subspaceSize) != subspaceSize)
            {
            errno = S_loadSomCoffLib_SUBSPHDR_READ;
            goto error;
            }
        }

    /* read in space string table */

    if (somHdr.space_strings_size != 0)
        {
        if ((pSpaceStrings = (char *) calloc (1, somHdr.space_strings_size)) ==
             NULL)
            {
            errno = S_loadSomCoffLib_SPSTRING_ALLOC;
            goto error;
            }

        if (ioctl (fd, FIOSEEK, somHdr.space_strings_location) == ERROR ||
            read (fd, pSpaceStrings, somHdr.space_strings_size) != 
                     somHdr.space_strings_size)
            {
            errno = S_loadSomCoffLib_SPSTRING_READ;
            goto error;
            }
        }
    if (somHdr.subspace_total != 0)
        {
        if ((pSubInfo = (SUB_INFO *) calloc (1, sizeof(SUB_INFO) *
                                            somHdr.subspace_total)) == NULL)
            {
            errno = S_loadSomCoffLib_INFO_ALLOC;
            goto error;
            }
        }
    else
        {
        pSubInfo = (SUB_INFO *) calloc (1, sizeof(SUB_INFO)); 
        }

    /* read in symbol table */

    symBufSize = somHdr.symbol_total * sizeof (SYMREC);

    if ((pSymBuf = (SYMREC *) calloc (1, symBufSize)) == NULL)
        {
        errno = S_loadSomCoffLib_SYM_READ;
        goto error;
        }

    if (ioctl (fd, FIOSEEK, somHdr.symbol_location) == ERROR)
        {
        errno = S_loadSomCoffLib_SYM_READ;
        goto error;
        }

    if (read (fd, (char *) pSymBuf, symBufSize) != symBufSize)
        {
        errno = S_loadSomCoffLib_SYM_READ;
        goto error;
        }

    /* read in symbol string table */

    if ((pSymStr = calloc (1, somHdr.symbol_strings_size)) == NULL)
        {
        errno = S_loadSomCoffLib_SYMSTR_READ;
        goto error;
        }

    if (ioctl (fd, FIOSEEK,somHdr.symbol_strings_location) == ERROR ||
        read (fd, pSymStr, somHdr.symbol_strings_size) != 
                 somHdr.symbol_strings_size)
        {
        errno = S_loadSomCoffLib_SYMSTR_READ;
        goto error;
        }

    /* read in linker fixup requests */

    if (somHdr.fixup_request_total != 0)
        {
        relSize = (int)somHdr.fixup_request_total;
        if ((pReloc = (FIXUP *) calloc (1, relSize)) == NULL)
            {
            errno = S_loadSomCoffLib_RELOC_ALLOC;
            goto error;
            }
        if (ioctl (fd, FIOSEEK, somHdr.fixup_request_location) == ERROR ||
            read (fd, (char*)pReloc, relSize) != relSize)
            {
            errno = S_loadSomCoffLib_RELOC_READ;
            goto error;
            }
        }

    /* make each symbol, space, and subspace point to its name string */

    symbolNameSet (pSymBuf, somHdr.symbol_total, pSymStr,
                   pSubspace, somHdr.subspace_total, pSpaceStrings,
                   pSpace, somHdr.space_total);


    /* We don't really load non-relocatable files.
     * Since VxWorks.sym is non-relocatable, we
     * allow non-relocatable files to have their
     * symbols added to the symbol table, without any loading */

    if (somHdr.a_magic != RELOC_MAGIC)
        {
        /* process subspace addresses with no relocation */

        subspaceNoReloc (&somHdr, pSubInfo, pSubspace);

        /* read in the symbol table */

        status = rdSymtab (pSymBuf, somHdr.symbol_total, pSubspace,
                           pSubInfo, symFlag, symTbl, &pGlobal,
                           NULL, group);

        goto done;
        }


    /* 
     * Loaded code won't work if VxWorks was linked without the "-N" flag.
     * However VxWorks itself will work - so we put this check after
     * allowing VxWorks to load it's symbol table
     */

    if (sysMemTop() > 0x40000000)
        {
        printErr ("\n");
        printErr ("load error: VxWorks wasn't linked with the \"-N\" flag\n");
        printErr ("Please read the \"source debugging\" section of the\n");
        printErr ("VxSim Users Guide for details\n");
        printErr ("\n");
        goto error;
        }

    /* Map SOM subspaces to VxWorks "segments", and get the segment sizes */

    subspaceToSegment (&somHdr, (UCHAR *)pReloc, pSubInfo,
        pSpace, pSubspace, pSymBuf, symTbl, &textSize, &dataSize,
        &bssSize);

    /* Allocate memory for text, data, and bss. We must be careful
     * to align everything, since loadSegmentsAllocate() will
     * put all segemnts in one block */

    seg.addrText = pText;
    seg.addrData = pData;
    seg.addrBss  = pBss;
    seg.sizeText = ROUND_UP(textSize, SEG_ALIGN);
    seg.sizeData = ROUND_UP(dataSize, SEG_ALIGN);
    seg.sizeBss  = ROUND_UP(bssSize, SEG_ALIGN);

    /* 
     * SPR #21836: pSeg->flagsText, pSeg->flagsData, pSeg->flagsBss are used
     * to save the max value of each segments. These max values are computed
     * for each sections. These fields of pSeg are only used on output, then
     * a temporary use is allowed.
     */

    seg.flagsText = SEG_ALIGN;
    seg.flagsData = SEG_ALIGN;
    seg.flagsBss  = SEG_ALIGN;

    commSizeGet (pSymBuf, somHdr.symbol_total,
                    pSubspace, (int *)&commSize);
    seg.sizeBss += ROUND_UP(commSize, SEG_ALIGN);
    DBG_PUT ("bssSize = 0x%x, commSize = 0x%x\n", bssSize, commSize);

    /* 
     * SPR #21836: loadSegmentsAllocate() allocate memory aligned on 
     * the max value of sections alignement saved in seg.flagsText,
     * seg.flagsData, seg.flagsBss.
     */

    if (loadSegmentsAllocate (&seg) != OK)
        {
        printErr ("Could not allocate segments\n");
        goto error;
        }

    /*
     * make sure all allocated segments are aligned OK.
     * This will always be true if the memory is malloc'ed,
     * since SEG_ALIGN, and _MEM_ALIGN, and the maximum
     * possible subspace alignment are all "8"
     */

    pText = (char *) ROUND_UP (seg.addrText, SEG_ALIGN);
    pData = (char *) ROUND_UP (seg.addrData, SEG_ALIGN);
    pBss  = (char *) ROUND_UP (seg.addrBss, SEG_ALIGN);
    pComm = (char *) ((int)pBss + seg.sizeBss
                      - ROUND_UP(commSize, SEG_ALIGN));
    if ( (pText != seg.addrText) ||
         (pData != seg.addrData) ||
         (pBss  != seg.addrBss) )
        {
        printErr ("loadModuleAtSym: segment alignment error\n");
        goto error;
        }
    DBG_PUT ("pText = 0x%x, pData = 0x%x, pBss = 0x%x pComm = 0x%x\n",
             (int)pText, (int)pData, (int)pBss, (int)pComm);
    DBG_PUT ("sizeText = 0x%x, sizeData = 0x%x, sizeBss = 0x%x\n",
             seg.sizeText, seg.sizeData, seg.sizeBss);

    /* load all subspaces into memory */

    if (loadSubspaces (fd, &somHdr, pSubInfo, pSubspace, pText, pData, pBss)
                      != OK)
        {
        errno = S_loadSomCoffLib_LOAD_SPACE;
        goto error;
        }

    /* add segment names to symbol table before other symbols */

    if (!(symFlag & LOAD_NO_SYMBOLS))
        addSegNames (fd, pText, pData, pBss, symTbl, group);

    /* process symbol table */

    status = rdSymtab (pSymBuf, somHdr.symbol_total, pSubspace,
                       pSubInfo, symFlag, symTbl, &pGlobal, pComm, group);

    /* Perform linker fixup requests on the subspaces */

    if (somHdr.version_id == NEW_VERSION_ID)
        {
        if (somHdr.fixup_request_total != 0)
            (void) linkSubspaces ((UCHAR *) pReloc, pSubspace, 
                                   somHdr.subspace_total, pSubInfo, pSymBuf, 
                                   somHdr.symbol_total,
                                   symTbl, pGlobal, moduleId);
        }
    else
        {
        errno = S_loadSomCoffLib_RELOC_VERSION;
        goto error;
        }

    /* return load addresses, where called for */

    if (ppText != NULL)
        *ppText = pText;

    if (ppData != NULL)
        *ppData = pData;

    if (ppBss != NULL)
        *ppBss = pBss;

    /* clear out bss */

    bzero (pBss, (int) seg.sizeBss);

    /* flush text to memory */

    CACHE_TEXT_UPDATE (pText, seg.sizeText);

    /*
     * Add the segments to the module.
     * This has to happen after the relocation gets done.
     * If the relocation happens first, the checksums won't be
     * correct.
     */

    moduleSegAdd (moduleId, SEGMENT_TEXT, pText, seg.sizeText, seg.flagsText);
    moduleSegAdd (moduleId, SEGMENT_DATA, pData, seg.sizeData, seg.flagsData);
    moduleSegAdd (moduleId, SEGMENT_BSS, pBss, seg.sizeBss, seg.flagsBss);
                
    /* error:
     * clean up dynamically allocated temporary buffers and return ERROR */

error:
done:
    if (pSubInfo != NULL)
        free ((char *) pSubInfo);
    if (pSpaceStrings != NULL)
        free ((char *) pSpaceStrings);
    if (pSpace!= NULL)
        free ((char *) pSpace);
    if (pSubspace!= NULL)
        free ((char *) pSubspace);
    if (pSymBuf != NULL)
        free ((char *) pSymBuf);
    if (pSymStr != NULL)
        free ((char *) pSymStr);
    if (pReloc!= NULL)
        free ((char *) pReloc);

    if (status == OK)
        return (moduleId);
    else
        {
        moduleDelete (moduleId);
        return (NULL);
        }
    }

/******************************************************************************
*
* symbolNameSet - make all symbols records point to their name strings
*/ 
 
static void symbolNameSet
    (
    SYMREC *   pSym,           /* pointer to symbol table */
    int        nSyms,          /* # of entries in symbol table */
    char *     pSymStr,        /* pointer to symbol string table */
    SUBSPACE * pSubspace,      /* pointer to subspace dictionary */
    int        nSubs,          /* number of subspaces */
    char *     pSpaceStr,      /* pointer to space name string table */
    SPACE *    pSpace,	       /* pointer to space dictionary */
    int        nSpaces	       /* number of spaces */
    )
    {
    int ix;

    for (ix = 0; ix < nSyms; ix++)
        pSym[ix].symbol_name = pSymStr + pSym[ix].name.n_strx;

    for (ix = 0; ix < nSubs; ix++)
        pSubspace[ix].subspace_name = pSpaceStr + pSubspace[ix].name.n_strx;

    for (ix = 0; ix < nSpaces; ix++)
        pSpace[ix].space_name = pSpaceStr + pSpace[ix].name.n_strx;
    }

/*******************************************************************************
*
* commSizeGet - this routine go through the symbol table to calculate the 
*                  size of STORAGE data for relocatable files
*
* STORAGE requests (i.e., "common data") are simply turned into BSS
* by this loader (and all other VxWorks loaders). The loader adds
* an area for these symbols by increasing the normal BSS area.
* This allows us to not have to malloc memory for each STORAGE symbol.
*
* As a side effect, all the STORAGE symbols are partially relocated.
* That is - the symbol_value field of the symbols SYMREC structure
* is turned into an offset from the start of the STORAGE segment
*/

static void commSizeGet
    (
    SYMREC * pSymBuf,              /* symbol dictionary */
    int      nEnts,                /* number of symbols */
    SUBSPACE *pSubspace,           /* subspace relocation info */
    int *    pCommSize             /* where to return comm size */
    )
    {
    SYMREC * pSym = pSymBuf;
    int      size =  0;
    int      subspaceIndex;
    int      ix;
    int      iy;
    SYMREC * pSearch;
    int      symSize;

    for (ix = 0; ix < nEnts; ix ++)
        {
        if (pSym->symbol_type == ST_STORAGE)
            {
            pSearch = pSymBuf;

            /* XXX - search for the previous record for same symbol
             * in symbol table. For some reason STORAGE symbols
             * sometime appear in pairs.
             */

            for (iy = 0; iy < ix; iy ++)
                {
                if (strcmp(pSym->symbol_name, pSearch->symbol_name) == 0)
                    break;
                pSearch ++;
                }

            /* deal with a new storage symbol */

            if (iy == ix)
                {
                subspaceIndex = pSym->symbol_info;
                size = ROUND_UP (size, pSubspace[subspaceIndex].alignment);
                symSize = pSym->symbol_value;
                pSym->symbol_value = size;
                size += symSize;
                pSym->secondary_def = 0;
                }

            /* duplicate storage symbol */

            else
                {
                pSym->secondary_def = 1;
                }
            }
        pSym ++;
        }

    *pCommSize = size;
    }

/*******************************************************************************
*
* rdSymtab - Process the VxWorks symbol table
*
* This is passed a pointer to an HP-PA SOM symbol table and processes
* each of the external symbols defined therein.  This processing performs
* two functions:
*  1) New symbols are entered in the system symbol table as
*     specified by the "symFlag" argument:
*        ALL_SYMBOLS    = all defined symbols (LOCAL and GLOBAL) are added,
*        GLOBAL_SYMBOLS = only external (GLOBAL) symbols are added,
*        NO_SYMBOLS     = no symbols are added;
*  2) Undefined externals are looked up in the system table.
*     If an undefined external cannot be found in the symbol table,
*     an error message is printed, but ERROR is not returned until the entire
*     symbol table has been read, which allows all undefined externals to be
*     reported.
*
* Absolute symbols are automatically discarded.
*
* RETURNS: OK or ERROR
*/

static STATUS rdSymtab 
    (
    SYMREC *   pSymBuf,		/* pointer to symbol table */
    FAST int   nEnts,           /* # of entries in symbol table */
    SUBSPACE * pSubspace,       /* pointer to subspace dictionary */
    SUB_INFO * pSubInfo,        /* subspace relocation info */
    int        symFlag,         /* symbols to be added to table 
                                 *   ([NO|GLOBAL|ALL]_SYMBOLS) */
    SYMTAB_ID  symTbl,          /* symbol table to use */
    SYMREC **  pSymGlobal,      /* '$global$' symbol entry pointer */
    char *     pComm,           /* start of comm area (within bss segment) */
    int        group            /* symbol group */
    )
    {
    char *   name;              /* symbol name (plus EOS) */
    SYMREC * pSym = (SYMREC *) pSymBuf;  /* symbol entry pointer */
    int      ix;		/* symbols subspace index */
    SYM_TYPE vxSymType;         /* symbol type */
    UCHAR    arg_reloc;		/* argument relocation info */
    int      status  = OK;      /* return status */

    *pSymGlobal = NULL;
    for (; nEnts > 0; nEnts -= 1, pSym ++)
        {
        /* throw away absolute symbols */

        if (pSym->symbol_type == ST_ABSOLUTE)
            continue;

        /* throw away symbol and argument extentions */
        if (pSym->symbol_type == ST_SYM_EXT)
            continue;

        if (pSym->symbol_type == ST_ARG_EXT)
            continue;

        /* throw away invalid symbols */

        if (pSym->symbol_type == ST_NULL)
            continue;

        /* get the symbol name */

        name = pSym->symbol_name;

        /* keep the symbol $global$ handy */

        if ( (pSym->symbol_type == ST_DATA) && (!strcmp(name, "$global$")) )
            *pSymGlobal = pSym;

        if (!pSubspace[pSym->symbol_info].is_loadable)
            WARN ("Warning: symbol %s is unloadable!\n", name);
        if (pSym->must_qualify)
            WARN ("Warning: symbol %s must qualify!\n", name);
        if (pSym->is_common || pSym->dup_common)
            WARN ("Warning: symbol %s is a common block!\n", name);

        /* add new symbols to the symbol table */

        if ((pSym->symbol_scope == SS_UNIVERSAL) ||
            (pSym->symbol_scope == SS_LOCAL))
            {

            /* relocate the symbol relative to it's associated subspace */

            ix = pSym->symbol_info;
            pSym->symbol_value = pSym->symbol_value
                                 - pSubspace[ix].subspace_start
                                 + (unsigned int)pSubInfo[ix].loadAddr;

            switch (pSym->symbol_type)
                {
                /* Data or BSS symbols */

                case ST_DATA:
                    if (pSubInfo[ix].loadType == LOAD_BSS)
                        {
                        vxSymType = N_BSS;
                        DBG_PUT ("new BSS symbol %s (0x%x)\n", name,
                                 pSym->symbol_value);
                        }
                    else
                        {
                        vxSymType = N_DATA;
                        DBG_PUT ("new DATA symbol %s (0x%x)\n", name, 
                                 pSym->symbol_value);
                        }
                    arg_reloc = 0;
                    break;

                /* Text symbols */

                case ST_CODE:
                case ST_PRI_PROG:
                case ST_SEC_PROG:
                case ST_MILLICODE:
                case ST_ENTRY:
                    vxSymType = N_TEXT | ((UCHAR)(pSym->arg_reloc & 3) << 6);
                    arg_reloc = (UCHAR)(pSym->arg_reloc >> 2);
                    pSym->symbol_value &= 0xfffffffc;
                    break;

                /* ignore stubs */

                case ST_STUB:
                    continue;

                /* These have never popped up yet */

                case ST_MODULE:
                case ST_PLABEL:
                case ST_OCT_DIS:
                case ST_MILLI_EXT:
                default:
                    WARN ("Warning: symbol %s is of type %d\n", 
                              name, pSym->symbol_type);
                    continue;
                }

            if ( (pSym->symbol_scope == SS_UNIVERSAL) && (!pSym->hidden) )
                vxSymType |= N_EXT;

            if ( ((symFlag & LOAD_LOCAL_SYMBOLS) && !(vxSymType & N_EXT)) ||
                ((symFlag & LOAD_GLOBAL_SYMBOLS) && (vxSymType & N_EXT)) )
                {
                DBG_PUT ("adding symbol %s to symTbl\n", name);
                if (externSymAdd (symTbl, name, (char*)pSym->symbol_value,
                            vxSymType, group, arg_reloc) != OK)
                    {
                    printErr ("can't add symbol %s (errno=%#x)\n",
                                name, errnoGet());
                    }
                }
            }

        /* Add STORAGE (common) symbols as BSS */

        else if (pSym->symbol_type == ST_STORAGE)
            {
            if (pSym->secondary_def)
                continue;
            vxSymType = N_BSS | N_EXT;
            pSym->symbol_value += (unsigned int)pComm;
            DBG_PUT ("new STORAGE symbol %s (0x%x)\n", name,
                     pSym->symbol_value);
            if (externSymAdd (symTbl, name, (char*)pSym->symbol_value,
                vxSymType, group, 0) != OK)
                {
                printErr ("can't add symbol %s (errno=%#x)\n",
                            name, errnoGet());
                }
            }

        /* look up external symbols */

        else
            {
            if (externSymResolve (symTbl, name, pSym) != OK)
                {
                printErr ("undefined symbol: %s\n", name);
                status = ERROR;
                }
            }

        }

    DBG_PUT ("done with rdSymtab\n");

    return (status);
    }

/*******************************************************************************
*
* de_assemble_21 - encode a 21 bit immediate.
*
* The PA-RISC encode immediate values in the instruction word in bizaar
* ways. See the PA-RISC architecture manual for details.
*/

static int de_assemble_21
    (
    unsigned int x
    )
    {
    return ((x & 0x100000) >> 20 | (x & 0xffe00) >> 8 |
            (x & 0x180) << 7  | ( x & 0x7c) << 14 | (x & 0x3) << 12);
    }

/*******************************************************************************
*
* de_assemble_17 - encode a 17 bit immediate.
*
* The PA-RISC encode immediate values in the instruction word in bizaar
* ways. See the PA-RISC architecture manual for details.
*/

static int de_assemble_17 
    (
    unsigned int x
    )
    {
    return ((x & 0x10000) >> 16 | (x & 0xf800) << 5 | (x & 0x400) >> 8 |
            (x & 0x3ff) << 3);
    }

/*******************************************************************************
*
* de_assemble_12 - encode a 12 bit immediate.
*
* The PA-RISC encode immediate values in the instruction word in bizaar
* ways. See the PA-RISC architecture manual for details.
*/

static int de_assemble_12
    (
    unsigned  int x
    )
    {
    return ((x & 0x800) >> 11 | (x & 0x400) >> 8 | (x & 0x3ff) << 3);
    }

/*******************************************************************************
*
* de_low_sign_ext - encode a sign extended immediate of length len
*
* The PA-RISC encode immediate values in the instruction word in bizaar
* ways. See the PA-RISC architecture manual for details.
*/

static int de_low_sign_ext
    (
    int x,
    int len
    )
    {
    int tmp;

    if (len > 32)
        return (x);

    tmp = x & ((1 << len) - 1);

    return ((tmp & ((1 << (len - 1)))) >> (len - 1) |
            ((tmp & ((1 << (len -1)) - 1)) << 1));
    }

/*******************************************************************************
*
* L_fixValue - mask off the low bits.
*/

static unsigned long L_fixValue 
    (
    unsigned long x
    )
    {
    return (x & 0xfffff800);
    }

/*******************************************************************************
*
* R_fixValue - mask off the high bits.
*/

static unsigned long R_fixValue 
    (
    unsigned long x
    )
    {
    return (x & 0x000007ff);
    }

/*******************************************************************************
*
* RND_fixValue - round to nearest 8k (0x2000) byte boundary
*/

static unsigned long RND_fixValue 
    (
    unsigned long x
    )
    {
    return ((x + 0x1000) & 0xffffe000);
    }

/******************************************************************************
*
* LR_fixValue - mask off low bits after rounding.
*/

static unsigned long LR_fixValue 
    (
    unsigned long x,
    unsigned long fixConst
    )
    {
    return (L_fixValue (x + RND_fixValue(fixConst)));
    }

/******************************************************************************
*
* RR_fixValue - mask off high bits after rounding.
*/

static unsigned long RR_fixValue 
    (
    unsigned long x,
    unsigned long fixConst
    )
    {
    return (R_fixValue (x + RND_fixValue(fixConst)) + 
            (fixConst - RND_fixValue(fixConst)));
    }

/*******************************************************************************
*
* fieldValue - perform rounding.
*
* This function performs rounding during a fixup request.
* The field selector, mode selector, and round constant are computed
* by the fuction fieldGetDefault() (see below) and passed to us.
*
* RETURNS: A rounded value, based on the original value, field selector,
*          mode selector, and round constant.
*/

static UINT fieldValue
    (
    UINT value,
    UINT fSel,
    UINT mSel,
    UINT roundConst
    )
    {
    UINT retVal = value;

    switch (fSel)
        {
        /* FSEL = no rounding needed */

      case R_FSEL:
	retVal = value + roundConst;
	break;

        /* LSEL = "left" bits (round first, then zero out the right bits) */

      case R_LSEL:
	switch (mSel)
	    {
	  case R_N_MODE:        /* round down (L') mode */
	    retVal = L_fixValue (value + roundConst);
	    break;
	  case R_S_MODE:        /* round to nearest page (LS') mode */
	    value += roundConst;
	    if (value & 0x00000400)
		retVal = L_fixValue (value + 0x800);
	    else
		retVal = value;
	    break;
	  case R_D_MODE:        /* round up (LD') mode */
	    retVal = L_fixValue (value + roundConst + 0x800);
	    break;
	  case R_R_MODE:        /* round up with adjust (LR') mode */
	    retVal = LR_fixValue (value, roundConst);
	    break;
	    }
	break;

        /* RSEL = "right" bits (round first, then zero out the left bits) */

      case R_RSEL:        
	switch (mSel)
	    {
	  case R_N_MODE:        /* round down (R') mode */
	    retVal = R_fixValue (value + roundConst);
	    break;
	  case R_S_MODE:        /* round to nearest page (RS') mode */
	    value += roundConst;
	    if (value & 0x00000400)
		retVal = value | 0xfffff800;
	    else
		retVal = value;
	    break;
	  case R_D_MODE:        /* round up (RD') mode */
	    retVal = (value + roundConst) | 0xfffff800;
	    break;
	  case R_R_MODE:        /* round with adjust (RR') mode */
	    retVal = RR_fixValue (value, roundConst);
	    break;
	    }
	break;
      default:
	WARN ("Warning: unknown field selector %d\n", fSel);
	break;
        }
    return (retVal);
    }

/*******************************************************************************
*
* fieldGetDefault - compute the field selector and rounding mode.
*
* This function looks at the current instruction to determine
* the field value, rounding mode, and rounding constant.
*/

static void fieldGetDefault 
    (
    UINT  instr,                  /* machine instruction */
    int   fieldOverride,          /* field selector override flag */
    int   dataOverride,           /* fixed constant override flag */
    int * pFieldSel,              /* where to return field selector */
    int * pFormat,                /* how to deposit bits */
    int * pFixConst               /* where to return fixup constant */
    )
    {
    unsigned int disp;
    unsigned int majorCode = (instr & 0xfc000000) >> 26;

    /* set field selector */

    if (!fieldOverride)
        {
        switch (majorCode)
            {
            case LDIL_CODE:
            case ADDIL_CODE:
                *pFieldSel = R_LSEL;
                break;
            case BL_CODE:
            case COMBT_CODE:
            case COMBF_CODE:
            case ADDB_CODE:
            case ADDBF_CODE:
            case BB_CODE:
                *pFieldSel = R_FSEL;
                break;
            default:
                *pFieldSel = R_RSEL;
                break;
            }
        }

    /* get relocation bit */

    switch (majorCode)
        {
        /* i_exp14 */
        case LDW_CODE:
        case LDH_CODE:
        case LDB_CODE:
        case STW_CODE:
        case STH_CODE:
        case STB_CODE:
        case LDWM_CODE:
        case STWM_CODE:
        case LDO_CODE:
            disp = instr & 0x3fff;
            disp = low_sign_ext (disp, 14);
            *pFormat = i_exp14;
            break;
        /* i_exp11 */
        case ADDI_CODE:
        case ADDIT_CODE:
        case SUBI_CODE:
        case COMICKR_CODE:
            disp = instr & 0x7ff;
            disp = low_sign_ext (disp, 11);
            *pFormat = i_exp11;
            break;
        /* i_exp21 */
        case LDIL_CODE:
        case ADDIL_CODE:
            disp = assemble_21 (instr & 0x1fffff);
            disp = disp & 0x3fff;
            *pFormat = i_exp21;
            break;
        /* i_rel17 */
        case BL_CODE:
        /* i_abs17 */
        case BE_CODE:
        case BLE_CODE:
            disp = instr & 0x1fffff;
            disp = sign_ext(assemble_17 ((disp & 0x1f0000)>>16, 
                                         (disp & 0x1ffc) >> 2,
                                         disp & 0x1), 17) << 2;
            *pFormat = i_rel17;        /* i_rel17 and i_abs17 same format */
            break;
        /* i_rel12 */
        case ADDIBT_CODE:
        case ADDIBF_CODE:
        case COMIBT_CODE:
        case COMIBF_CODE:
            disp = instr & 0x1ffd;
            disp = sign_ext (assemble_12 ((disp & 0x1ffc) >> 2, disp & 0x1), 12)
                   << 2;
            *pFormat = i_rel12;
            break;
        default:
            disp = 0;
            *pFormat = i_none;
            break;
        }
    if (!dataOverride)
        *pFixConst = disp;
    }

/*******************************************************************************
*
* fixBits - Perform a linker fixup to modify an instruction.
*/

static void fixBits
    (
    INSTR * adrs,        /* where to fix */
    int     value,       /* value to fix */
    int     format       /* fixup format */
    )
    {
    int mask;

    switch (format)
        {
        case i_exp14:
            value = de_low_sign_ext (value, 14);
            mask = 0x3fff;
            break;
        case i_exp21:
            value = de_assemble_21 (value >> 11);
            mask = 0x1fffff;
            break;
        case i_exp11:
            value = de_low_sign_ext (value, 11);
            mask = 0x7ff;
            break;
        case i_rel17:
        case i_abs17:
            value = de_assemble_17 (value >> 2);
            mask = 0x001f1ffd;
            break;
        case i_rel12:
            value = de_assemble_12 (value >> 2);
            mask = 0x1ffd;
            break;
        case i_data:
            mask = 0xffffffff;
            break;
        case i_milli:
        case i_break:
        default:
            mask = 0;
            break;
        }
    *adrs = ((*adrs) & ~mask) | (value & mask);
    }

/*******************************************************************************
*
* linkSubspaces - perform linker fixup commands for relocation.
*
* RETURNS: OK or ERROR
*/

static STATUS linkSubspaces
    (
    UCHAR *   pRelCmds,        /* list of relocation commands */
    SUBSPACE *pSubspace,       /* subspace dictionary */
    int       nSubs,           /* number of subspaces */
    SUB_INFO *pSubInfo,        /* load information for the subspaces */
    SYMREC *  pSym,            /* symbols dictionary */
    int       numSyms,         /* number of symbols */
    SYMTAB_ID pSymTab,         /* VxWorks symbol table */
    SYMREC *  pSymGlobal,      /* pointer to '$global$' entry */
    MODULE_ID moduleId         /* module being linked */
    )
    {
    int status = OK;		/* function return value */

    /*
     * generic fixup variables
     *
     * For each subspace, we must process a stream of fixup requests
     * in order to complete the relocation. The fixup requests are
     * variable size, and there are no "markers" to separate the
     * fixups - so ya gotta be real careful. The first byte of each
     * fixup request is called the opCode.
     */

    int ix;			/* index of subspace being fixed up */
    UCHAR * pThisFixup;		/* start of relocation fixup stream */
    UCHAR * pLastFixup;		/* end of stream */
    UCHAR opCode;               /* the type of fixup request */
    char *relAdr;               /* address that need fixing */
    int relValue;               /* value to apply (e.g., a function address) */

    /*
     * Variables for maintaining the REPEAT_PREV_FIXUP list.
     *
     * The linker maintains a list of the last 4 multibyte
     * fixup requests. When a REPEAT_PREV_FIXUP fixup occurs,
     * we repeat one of the fixups from our list.
     */

    LIST prevFixupList;         /* previous fixup list */
    PREV_FIXUP prevFixupNodes[4];    /* the nodes in the list */
    PREV_FIXUP *pPrevFixupNode; /* pointer to some node in the list */
    UCHAR *pOldFixup = 0;       /* before executing a previous fixup, we save
                                 * our current place in the fixup stream */
    BOOL usingPrevFixup;        /* are we currently are using a queued fixup? */

    /*
     * variables for fixups that manipulate the expession stack.
     *
     * An expession stack is used to evaluate complex combinations of values.
     * The R_COMPX fixups either push a value on the expression stack or
     * modifies what is on top of the stack (e.g., it might pop the top
     * two elements, and push their sum).
     * These fixups have a secondary opCode which describes what to do.
     * The value at the top of the expession stack is used in the
     * R_DATA_EXPR and R_CODE_EXPR fixups.
     * XXX - these have not been tested yet.
     */

    UCHAR opCode2;              /* 2nd opcode for stack manipulation fixups */
    int * pExpStack;                /* initial stack pointer */
    int * sp;			/* current stack pointer */
    int top1;                   /* first item on stack */
    int top2;                   /* second item on stack */
    int C;                      /* condition code for stack operation */

    /*
     * variables to hold the fixup request parameters.
     *
     * Most fixup requests pass us parameters (e.g., the symbol number
     * which we need to branch to). The parameters passed depend on
     * the fixup. The way the parameters are encoded in the fixup stream
     * also depends on the fixup.
     */

    int L;                      /* number of bytes to relocate */
    int S;                      /* symbol index used */
    int R = 0;                  /* parameter relocation bits */
    int V = 0;                  /* variable or fix up constant */
    int U;			/* stack unwind bits */
    int F;			/* procedure stack frame size */
    int D;                      /* difference of operation code */
    int X;			/* previous fixup index */
    int M;			/* number of bytes for R_REPEATED_INIT */
    int N;			/* statement number in R_STATEMENT */

    /*
     * variables for handling the rounding mode during fixups
     *
     * Many fixups come in pairs, where we move the left (high order)
     * bits of a value in the first fixup, and the right (low order)
     * bits in the next fixup. Before applying a fixup value
     * at an address, it must be rounded.
     * The rounded constant is then applied to the address.
     *
     * The field selector specifies if we should apply the
     * left bits (LSEL), the right bits (RSEL), or everything (FSEL).
     * The field selector depends on the current instruction, but
     * can be overridden with a feild selector override fixup.
     *
     * The interpretation of the field selector depends on the current
     * rounding mode. The rounding mode selector can be
     * round down (N_MODE), round to nearest page (S_MODE), round up (D_MODE),
     * or round down with adjusted constant (R_MODE).
     * The rounding mode is persistent until explicity changed by
     * a mode select fixup request. The default at the begining of each
     * subspace is N_MODE.
     *
     * The function fieldGetDefault() computes the field selector
     * (and adjusted constant if we are in R_MODE) based on the current
     * instruction and rounding mode. It also computes how to
     * apply the rounded constant to a given instruction.
     */

    int fixConstant;            /* adjusted constant used in R_MODE rounding */
    int modeSel;                /* mode select */
    int fieldSel;               /* field select */
    int fieldOverride;		/* field select override flag */
    int dataOverride;           /* data override flag */
    int relFormat;		/* how to apply the rounded constant */

    /* 
     * variables for stub manipulation.
     *
     * The linker must generate "long branch stubs" and "parameter
     * relocation stubs" whenever needed. Read the INTERNAL
     * comments at the begining of the file for more info.
     * The long branch stubs are generated per subspace, while
     * the parameter relocation stubs are generated per module.
     */

    LIST stubList;              /* list of current long branch stubs */
    char *pLongBranchStubs;     /* starting address this subspaces stubs */
    char *pStub;                /* address of current stub */
    int stubIx;                 /* current stubs index in the list */

    LIST argRelocStubList;	/* list of argument relocation stubs */

    /*
     * variables for stack unwind generation
     *
     * The PA-RISC has no frame pointer. To calculate the frame pointer,
     * the linker must generate an array of stack unwind descriptors
     * which, for each function, specifies its starting address, ending
     * address, and the amount of stack space it requires. 
     */

    LIST unwindList;		/* linked list of unwind descriptors */
    UNWIND_NODE *pUnwindNode = NULL;	/* pointer to an unwind descriptor */

    /* scratch variables */

    char *tmpBuf;               /* tempoaray buffer */
    int bufSize;                /* size of temporary buffer */

    /* create the expression stack */

    if ((pExpStack = (int *) calloc (1, REL_STACK_SIZE)) == NULL)
        {
        printErr ("ld error: unable to allocate relocation stack\n");
        return (ERROR);
        }

    /* Initialize the previous fixup queue */

    lstInit (&prevFixupList);
    for (ix = 0; ix < 4; ix++)
        {
        lstAdd (&prevFixupList, &(prevFixupNodes[ix].node));
        prevFixupNodes[ix].pFixup = NULL;
        }

    /* initialize the list of long branch and parameter relocation stubs */

    lstInit (&stubList);
    lstInit (&argRelocStubList);

    /* initialize the stack unwind list */

    lstInit (&unwindList);

    /* perform linker fixups on each subspace */

    for (ix = 0; ix < nSubs; ix ++, pSubspace++)
        {
        if (!pSubspace->is_loadable)
            {
            DBG_PUT ("linker: skipping fixups for unloadable subspace\n");
            continue;
            }

        if (pSubspace->fixup_request_quantity == 0)
            {
            continue;
            }

        DBG_PUT ("\n\n\nsubSpace: 0x%x - 0x%x\n", (int)(pSubInfo[ix].loadAddr),
                 (int)(pSubInfo[ix].loadAddr) + pSubspace->subspace_length);

        /* Initialize the long branch stub list for this subspace to be empty */

        lstFree (&stubList);
        pLongBranchStubs = pSubInfo[ix].loadAddr +
                           pSubspace->subspace_length;
        pLongBranchStubs = (char *)ROUND_UP (pLongBranchStubs,
                           pSubspace->alignment);

        /* get the starting address for this subspace */

        relAdr = (pSubInfo + ix)->loadAddr;

        /* get the list of fixup requests for this subspace */

        pThisFixup = pRelCmds + pSubspace->fixup_request_index;
        pLastFixup = pThisFixup + pSubspace->fixup_request_quantity;

        /* set some default modes */

        modeSel = R_N_MODE;	/* default rounding mode for each subspace */
        fieldSel = R_FSEL;      /* default field selector. XXX - needed? */
        dataOverride = FALSE;
        fieldOverride = FALSE;
        usingPrevFixup = FALSE;
        sp = pExpStack;		/* each subspace starts with a fresh
                                 * expression stack. XXX - added */

        /* process this subspaces fixups in a huge switch statement */

        while (pThisFixup < pLastFixup)
            {
            opCode = *pThisFixup;
            DBG_PUT ("opCode (0x%x) = %d. relAdr = 0x%x.\n", (int)pThisFixup,
                     opCode, (int)relAdr);

            /* keep track of the last 4 multibyte fixup requests */

            if (fixupLen[opCode] > 1)
                {
                pPrevFixupNode = (PREV_FIXUP *)lstNth (&prevFixupList, 4);
                lstDelete (&prevFixupList, &(pPrevFixupNode->node));
                pPrevFixupNode->pFixup = pThisFixup;
                lstInsert (&prevFixupList, NULL, &(pPrevFixupNode->node));
                }
            pThisFixup++;

            /* perform the fixup based on the opCode */

            if (/* opCode >= R_NO_RELOCATION && */ opCode < R_ZEROES )
                {
                /* 00-1f: n words, not relocatable */
                if (opCode < 24)
                    {
                    D = opCode - R_NO_RELOCATION;
                    L = (D + 1) * 4;
                    }
                else if (opCode >= 24 && opCode < 28)
                    {
                    D = opCode - 24;
                    L = ((D << 8) + B1 + 1) * 4;
                    }
                else if (opCode >= 28 && opCode < 31)
                    {
                    D = opCode - 28;
                    L = ((D << 16) + B2 + 1) * 4;
                    }
                else
                    {
                    D = opCode - 31;
                    L = B3 + 1;
                    }
                DBG_PUT ("R_NO_RELOCATION: 0x%x bytes\n", L);
                relAdr += L;
                }

            else if (opCode >= R_ZEROES && opCode < R_UNINIT)
                {
                /* 20-21: n words, all zero */
                if (opCode == R_ZEROES)
                    {
                    L = (B1 + 1) * 4;
                    }
                else
                    {
                    L = B3 + 1;
                    }
                /* insert L bytes zero */

                bufSize = (int)(pSubInfo+ix)->loadAddr +
                        pSubspace->subspace_length - (int)relAdr;
                tmpBuf = malloc (bufSize); 
                if (tmpBuf == NULL)
                        printErr ("ld error: reloc malloc failed\n");
                DBG_PUT ("inserting %d zeros at 0x%x\n",
                            L, (int)relAdr);
                bcopy (relAdr, tmpBuf, bufSize);
                bzero (relAdr, L);
                relAdr += L;
                bcopy (tmpBuf, relAdr, bufSize - L);
                free ((char *) tmpBuf);
                }

            else if (opCode >= R_UNINIT && opCode < R_RELOCATION)
                {
                /* 22-23: n words, uninitialized */
                if (opCode == R_UNINIT)
                    {
                    L = (B1 + 1) * 4;
                    }
                else
                    {
                    L = B3 + 1;
                    }

                /* skip L bytes */
                bufSize = (int)(pSubInfo+ix)->loadAddr +
                                pSubspace->subspace_length - (int)relAdr;
                tmpBuf = malloc (bufSize); 
                if (tmpBuf == NULL)
                        printErr ("ld error: reloc malloc failed\n");
                DBG_PUT ("skipping %d bytes at 0x%x.\n", L, (int)relAdr);
                bcopy ((char *) relAdr, tmpBuf, bufSize);
                bzero (relAdr, L);
                relAdr += L;
                bcopy (tmpBuf, relAdr, bufSize - L);
                free ((char *) tmpBuf);
                }

            else if (opCode == R_RELOCATION)
                {	/* XXX - untested */
                /* 24: 1 word relocatable data */
                relValue = *(int *)relAdr + (int)(pSubInfo[ix].loadAddr);
                *((unsigned int *)(relAdr)) = relValue;
                WARN ("Warning: R_RELOCATION fixup is untested\n");
                WARN ("address = 0x%x\n", (int)relAdr);
                relAdr += 4;
                }

            else if (opCode >= R_DATA_ONE_SYMBOL && opCode < R_DATA_PLABEL)
                {
                /* 25-26: 1 word, data external reference */
                if (opCode == R_DATA_ONE_SYMBOL)
                    {
                    S = B1;
                    }
                else
                    {
                    S = B3;
                    }
                relValue = *(int *)relAdr + (pSym + S)->symbol_value;
                *((int *)relAdr) = relValue;
                DBG_PUT ("R_DATA_ONE_SYMBOL: relAdr = 0x%x\n", (int)relAdr);
                relAdr +=4;
                }

            else if (opCode >= R_DATA_PLABEL && opCode < R_SPACE_REF)
                {
                /* 27-28: 1 word, data plabel reference */
                if (opCode == R_DATA_PLABEL)
                    {
                    S = B1;
                    }
                else
                    {
                    S = B3;
                    }
                relValue = ((SYMREC *)(pSym + S))->symbol_value;
                *((int *)relAdr) = relValue;
                DBG_PUT ("R_DATA_PLABEL: relAdr = 0x%x\n", (int)relAdr);
                relAdr += 4;
                }

            else if (opCode == R_SPACE_REF)
                {
                /* 29: 1 word, initialized space id */
                printErr ("ld error: R_SPACE_REF relocation unsupported\n");
                status = ERROR;
                }

            else if (opCode >= R_REPEATED_INIT && opCode < 46)
                {
                /* 30-3d: n words, repeated pattern */

                if (opCode == R_REPEATED_INIT)
                    {
                    L = 4;
                    M = (B1 + 1) * 4;
                    }
                else if (opCode == (R_REPEATED_INIT + 1))
                    {
                    L = B1 * 4;
                    M = (B1 + 1) * L;
                    }
                else if (opCode == (R_REPEATED_INIT + 2))
                    {
                    L = B1 * 4;
                    M = (B3 + 1) * 4;
                    }
                else
                    {
                    L = B3 + 1;
                    M = B4 + 1;
                    }
                DBG_PUT ("Bcopy R_REPEATED_INIT\n");
                bcopy (relAdr, relAdr + L, M);
                relAdr += M + L;
                }

            else if (opCode >= R_PCREL_CALL && opCode < 62)
                {
                /* 3e-3f: 1 word, pc-relative call */
                if (opCode >= R_PCREL_CALL && opCode < (R_PCREL_CALL + 10))
                    {
                    D = opCode - R_PCREL_CALL;
                    R = rbits1 (D);
                    S = B1;
                    }
                else if (opCode >= (R_PCREL_CALL + 10) && 
                         opCode < (R_PCREL_CALL + 12))
                    {
                    D = opCode - (R_PCREL_CALL + 10);
                    R = rbits2 ((D << 8) + B1);
                    S = B1;
                    }
                else
                    {
                    D = opCode - (R_PCREL_CALL + 12);
                    R = rbits2 ((D << 8) + B1);
                    S = B3;
                    }
                DBG_PUT ("R_PCREL_CALL %d\n", opCode - R_PCREL_CALL);
                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride, 
                                 dataOverride, &fieldSel, &relFormat, 
                                 &fixConstant);

                relValue = fieldValue ((int)((pSym+S)->symbol_value - 
                                        (int)relAdr - 8), fieldSel, 
                                        modeSel, fixConstant);

                /*
                 * if we are out of range for a PC relative branch,
                 * then we must create a "long branch stub"
                 * and patch this routine to jump to the stub 
                 */

                if ( (relValue > 0x3ffff) || (relValue < -0x40000) ||
                    argRelocDiffer (pSym[S].arg_reloc, R) )
                    {
                    stubIx = stubListFind (&stubList, S, R);
                    if (stubIx == -1)
                        {
                        char *procAddr;
			int parm_reloc;

			/* Always add unmodified arg_reloc case first */
			stubIx = stubListFind(&stubList, S, pSym[S].arg_reloc);
			if (stubIx == -1)
			    {
                            procAddr = (char *) pSym[S].symbol_value;
			    stubIx = stubListAdd (&stubList, S,
						  pSym[S].arg_reloc);
			    pStub = pLongBranchStubs + stubIx*stubSize;
			    DBG_PUT ("Creating long branch stub at 0x%x calling 0x%x\n",
				     (int) pStub, pSym[S].symbol_value);
			    stubInit (pStub, (char *) pSym[S].symbol_value);
			    }

			/* See if a parameter reloc stub is needed */
			parm_reloc = (pSym[S].arg_reloc & 0x03f3) | 0x0c;
			if (argRelocDiffer (pSym[S].arg_reloc, R)
			    && !argRelocDiffer (parm_reloc, R))
			    {
			    stubIx = stubListAdd (&stubList, S, parm_reloc);
			    pStub = pLongBranchStubs + stubIx*stubSize;
                            procAddr = argRelocStubListFind (&argRelocStubList,
                                          pSym + S, R, moduleId);
			    DBG_PUT ("Creating long branch stub at 0x%x calling 0x%x (0x%x)\n",
				     (int) pStub, (int) procAddr,
				     pSym[S].symbol_value);
			    stubInit (pStub, procAddr);
			    }
			if (stubIx == -1)
			    {
			    printErr ("stub not found\n");
			    break;
			    }
                        }
                    pStub = pLongBranchStubs + stubIx*stubSize;
                    relValue = fieldValue ((int)(pStub - (int)relAdr - 8),
                                           fieldSel, modeSel, fixConstant);
                    }
                fixBits ((INSTR *) relAdr, relValue, relFormat);
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                relAdr += 4;
                }
            else if (opCode == 62)
		{
		/* SPR 22030: ignore this reserved fixup type */
		}

            else if (opCode >= R_ABS_CALL && opCode < 78)
                {
                /* 40-4d: 1 word, absolute call */
                /* 4e-4f: reserved */
                if ( opCode >= 0x40 && opCode < 0x4a)
                    {
                    D = opCode - R_ABS_CALL;
                    R = rbits1 (D);
                    S = B1;
                    }
                else if (opCode == 74 || opCode == 75)
                    {
                    D = opCode - 74;
                    R = rbits2 ((D << 8) + B1);
                    S = B1;
                    }
                else
                    {
                    D = opCode - 76;
                    R = rbits2 ((D << 8) + B1);
                    S = B3;
                    }
                DBG_PUT ("R_ABS_CALL + %d\n", opCode - R_ABS_CALL);
                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride, 
                                 dataOverride, &fieldSel, &relFormat, 
                                 &fixConstant);

                /*
                 * if we are calling a procedure with the wrong parameters,
                 * then we must generate a "parameter relocation stub"
                 * and call the stub instead
                 */

                if (argRelocDiffer(R, pSym[S].arg_reloc))
                    {
                    tmpBuf = argRelocStubListFind (&argRelocStubList,
                                          pSym + S, R, moduleId);

                    relValue = fieldValue ((int)(tmpBuf), fieldSel,
                                            modeSel, fixConstant);
                    }
                else
                    {
                    relValue = fieldValue (
                        (int)(pSym[S].symbol_value),
                        fieldSel, modeSel, fixConstant);
                    }
                fixBits ((INSTR *) relAdr, relValue, relFormat);
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                relAdr += 4;
                }

            else if (opCode >= R_DP_RELATIVE && opCode < 114)
                {
                /* 50-72: 1 word, dp-relative load/store */
                /* 73-77: reserved */
                if (opCode == 112)
                    {
                    S = B1;
                    }
                else if (opCode == 113)
                    {
                    S = B3;
                    }
                else
                    {
                    D = opCode - R_DP_RELATIVE;
                    S = D;
                    }

                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride, 
                                 dataOverride, &fieldSel, &relFormat, 
                                 &fixConstant);
                if (pSymGlobal == NULL)
                    {
                    printErr ("ld error: no $global$\n");
                    status = ERROR;
                    }
                else
                    {
                    relValue = (int)((pSym+S)->symbol_value) - 
                               pSymGlobal->symbol_value;
                    relValue = fieldValue (relValue, fieldSel, modeSel, 
                                           fixConstant);
                    fixBits ((INSTR *) relAdr, relValue, relFormat);
                    }
                DBG_PUT ("R_DP_RELATIVE at 0x%x\n", (int)relAdr);
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                relAdr += 4;
                }

            else if (opCode >= R_DLT_REL && opCode < 122)
                {
                /* 78-79: 1 word, dlt-relative load/store */
                if (opCode == R_DLT_REL)
                    {
                    S = B1;
                    }
                else
                    {
                    S = B3;
                    }
                printErr ("ld error: R_DLT_REL relocation unsupported\n");
                status = ERROR;
                relAdr += 4;
                }

            else if (opCode >= R_CODE_ONE_SYMBOL && opCode < 162)
                {
                /* 80-a2: 1 word, relocatable code */
                /* a3-ad: reserved */
                if (opCode == 160)
                    {
                    S = B1;
                    }
                else if (opCode == 161)
                    {
                    S = B3;
                    }
                else
                    {
                    D = opCode - R_CODE_ONE_SYMBOL;
                    S = D;
                    }
                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride, 
                                 dataOverride, &fieldSel, &relFormat, 
                                 &fixConstant);
                relValue = fieldValue ((int)((pSym + S)->symbol_value),
                                        fieldSel, modeSel, fixConstant);
                DBG_PUT ("    fieldValue(0x%x, %d, %d, 0x%x) = 0x%x\n",
			 (int)((pSym + S)->symbol_value),
			 fieldSel, modeSel, fixConstant, relValue);
                fixBits ((INSTR *) relAdr, relValue, relFormat);
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                DBG_PUT ("R_CODE_ONE_SYMBOL at 0x%x\n", (int)relAdr);
                DBG_PUT ("    symVal= 0x%x fixConst= 0x%x, relValue= 0x%x\n",
			 (int)((pSym + S)->symbol_value), fixConstant,
			 relValue);
                relAdr += 4;
                }

            else if (opCode >= R_MILLI_REL && opCode < R_CODE_PLABEL)
                {
                /* ae-af: 1 word, millicode-relative branch */
                if (opCode == R_MILLI_REL)
                    {
                    S = B1;
                    }
                else
                    {
                    S = B3;
                    }
                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride, 
                                 dataOverride, &fieldSel, &relFormat, 
                                 &fixConstant);
                WARN ("Warning: R_MILLI_REL fixup is untested\n");
                WARN ("address = 0x%x\n", (int)relAdr);
                relValue = fieldValue ((pSym[S].symbol_value -
                                       pSym[1].symbol_value + fixConstant),
                                       fieldSel, modeSel, fixConstant);
                fixBits ((INSTR *) relAdr, relValue, relFormat); 
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                relAdr += 4;
                }

            else if (opCode >= R_CODE_PLABEL && opCode < R_BREAKPOINT)
                {
                /* b0-b1: 1 word, code plabel reference */
                if (opCode == R_CODE_PLABEL)
                    {
                    S = B1;
                    }
                else
                    {
                    S = B3;
                    }
                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride, 
                                 dataOverride, &fieldSel, &relFormat, 
                                 &fixConstant);
                if (fixConstant != 0 && fixConstant != 2)
                    printErr ("R_CODE_PLABEL: wrong displacement value\n");
                relValue = fieldValue ((int)(pSym + S)->symbol_value, 
                                       fieldSel, modeSel, fixConstant);
                fixBits ((INSTR *) relAdr, relValue, relFormat);
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                relAdr += 4;
                }

            else if (opCode == R_BREAKPOINT)
                {
                /* b2: 1 word, statement breakpoint */
                *(int *)relAdr = 0;
                DBG_PUT ("R_BREAKPOINT at address 0x%x\n", (int)relAdr);
                relAdr += 4;
                }

            else if (opCode >= R_ENTRY && opCode < R_ALT_ENTRY)
                {
                /* b3-b4: procedure entry */
                if (opCode == R_ENTRY)
                    {
                    U = B4; /* unwind descriptor word 3 */
                    F = B4; /* unwind descriptor word 4 */
                    }
                else
                    {
                    U = B4 << 5;
                    U |= (B1 >> 3);
                    CHECK_STACK_UNDER;
                    F = POP_EXPR;
                    }
                DBG_PUT ("proc entry at 0x%x\n", (int)relAdr);
                pUnwindNode = (UNWIND_NODE *)malloc (sizeof(UNWIND_NODE));
                pUnwindNode->unwindDesc.startAddr = relAdr;
                pUnwindNode->unwindDesc.word3 = U;
                pUnwindNode->unwindDesc.word4 = F;
                }

            else if (opCode == R_ALT_ENTRY)
                {
                /* b5: alternate entry */
                WARN ("alternate entry point at 0x%x ignored!\n", (int)relAdr);
                }

            else if (opCode == R_EXIT)
                {
                /* b6: procedure exit */
                DBG_PUT ("proc exit at 0x%x\n", (int)relAdr);
                if (pUnwindNode == NULL)
                    {
                    WARN ("proc exit has no proc entry!\n");
                    }
                else
                    {
                    pUnwindNode->unwindDesc.endAddr = relAdr - 4;
                    pUnwindNode->unwindDesc.word3 &= (~0x18000000);
                    unwindListAdd (&unwindList, pUnwindNode);
                    }
                pUnwindNode = NULL;
                }

            else if (opCode == R_BEGIN_TRY)
                {
                /* b7: start of try block */
                }

            else if (opCode >= R_END_TRY && opCode < R_BEGIN_BRTAB)
                {
                /* b8-ba: end of try block */
                if (opCode == R_END_TRY)
                    {
                    R = 0;
                    }
                else if (opCode == (R_END_TRY + 1))
                    {
                    R = B1 * 4;
                    }
                else
                    {
                    R = B3 * 4;
                    }
                }

            else if (opCode == R_BEGIN_BRTAB)
                {
                /* bb: start of branch table */
                }

            else if (opCode == R_END_BRTAB)
                {
                /* bc: end of branch table */
                }

            else if (opCode >= R_STATEMENT && opCode < R_DATA_EXPR)
                {
                /* bd-bf: statement number */
                if (opCode == R_STATEMENT)
                    {
                    N = B1;
                    }
                else if (opCode == (R_STATEMENT + 1))
                    {
                    N = B2;
                    }
                else
                    {
                    N = B3;
                    }
                }

            else if (opCode == R_DATA_EXPR)
                {		/* XXX - untested */
                /* c0: 1 word, relocatable data expr */
                CHECK_STACK_UNDER;
                relValue = (*(int *)relAdr) + POP_EXPR;
                *(int *)relAdr = relValue;
                WARN ("Warning: R_DATA_EXPR fixup is untested\n");
                WARN ("address = 0x%x\n", (int)relAdr);
                relAdr += 4;
                }

            else if (opCode == R_CODE_EXPR)
                {
                /* c1: 1 word, relocatable code expr */
                CHECK_STACK_UNDER;
                fieldGetDefault (*(unsigned int *)relAdr, fieldOverride,
                                 dataOverride, 
                                 &fieldSel, &relFormat, &fixConstant);
                relValue = fixConstant + POP_EXPR;
                fixBits ((INSTR *) relAdr, relValue, relFormat);
                FLAG_RESET (fieldOverride);
                FLAG_RESET (dataOverride);
                relAdr += 4;
                }

            else if (opCode == R_FSEL)
                {
                /* c2: F' override */
                fieldSel = R_FSEL;
                fieldOverride = TRUE;
                DBG_PUT ("R_FSEL\n");
                }

            else if (opCode == R_LSEL)
                {
                /* c3: L'/LD'/LS'/LR' override */
                fieldSel = R_LSEL;
                fieldOverride = TRUE;
                DBG_PUT ("R_LSEL\n");
                }

            else if (opCode == R_RSEL)
                {
                /* c4: R'/RD'/RS'/RR' override */
                fieldSel = R_RSEL;
                fieldOverride = TRUE;
                DBG_PUT ("R_RSEL\n");
                }

            else if (opCode == R_N_MODE)
                {
                /* c5: set L'/R' mode */
                modeSel = R_N_MODE;
                DBG_PUT ("R_N_MODE\n");
                }

            else if (opCode == R_S_MODE)
                {
                /* c6: set LS'/RS' mode */
                modeSel = R_S_MODE;
                DBG_PUT ("R_N_MODE\n");
                }

            else if (opCode == R_D_MODE)
                {
                /* c7: set LD'/RD' mode */
                modeSel = R_D_MODE;
                DBG_PUT ("R_D_MODE\n");
                }

            else if (opCode == R_R_MODE)
                {
                /* c8: set LR'/RR' mode */
                modeSel = R_R_MODE;
                DBG_PUT ("R_R_MODE\n");
                }

            else if (opCode >= R_DATA_OVERRIDE && opCode < R_TRANSLATED)
                {
                /* c9-cd: get data from fixup area */
                if (opCode == 201)
                    V = 0;
                else if (opCode == 202)
                    V = sign_ext (B1, 8);
                else if (opCode == 203)
                    V = sign_ext (B2, 16);
                else if (opCode == 204)
                    V = sign_ext (B3, 24);
                else /* opCode == 205 */
                    V = B4;
                fixConstant = V;
                dataOverride = TRUE;
                }

            else if (opCode == R_TRANSLATED)
                {
                /* ce: toggle translated mode */
                printErr ("ld error: R_TRANSLATED not supported\n");
                }

            else if (opCode == R_AUX_UNWIND)
                {
                /* cf: auxiliary unwind (proc begin) */
                int SK;
                int SN;
                int CN;

                CN = B3;
                SN = B4;
                SK = B4;
                }

            else if (opCode == R_COMP1)
                {
                /* d0: arbitrary expression */
                opCode2 = B1;
                V = opCode2 & 0x3f;
                C = opCode2 & 0x1f;
                switch (opCode2)
                    {
                    case R_PUSH_PCON1:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (abs(V));
                        break;
                    case R_PUSH_DOT:
                        CHECK_STACK_OVER;
                        PUSH_EXPR ((int)relAdr);
                        break;
                    case R_MAX:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (max (top1, top2));
                        break;
                    case R_MIN:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (min (top1, top2));
                        break;
                    case R_ADD:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top1 + top2);
                        break;
                    case R_SUB:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top2 - top1);
                        break;
                    case R_MULT:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top1 * top2);
                        break;
                    case R_DIV:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top2 / top1);
                        break;
                    case R_MOD:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top2 % top1);
                        break;
                    case R_AND:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top1 & top2);
                        break;
                    case R_OR:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top1 | top2);
                        break;
                    case R_XOR:
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_UNDER;
                        top2 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (top1 ^ top2);
                        break;
                    case R_NOT: 
                        CHECK_STACK_UNDER;
                        top1 = POP_EXPR;
                        CHECK_STACK_OVER;
                        PUSH_EXPR (~top1);
                        break;
                    case R_LSHIFT:
                        if (C == 0)
                            {
                            CHECK_STACK_UNDER;
                            top1 = POP_EXPR;
                            CHECK_STACK_UNDER;
                            top2 = POP_EXPR;
                            CHECK_STACK_OVER;
                            PUSH_EXPR (top2 << top1);
                            }
                        else
                            {
                            top1 = POP_EXPR;
                            PUSH_EXPR (top1 << C);
                            }
                        break;
                    case R_ARITH_RSHIFT:
                        if (C == 0)
                            {
                            CHECK_STACK_UNDER;
                            top1 = POP_EXPR;
                            CHECK_STACK_UNDER;
                            top2 = POP_EXPR;
                            CHECK_STACK_OVER;
                            PUSH_EXPR (top2 >> top1);
                            }
                        else
                            {
                            top1 = POP_EXPR;
                            PUSH_EXPR (top1 >> C);
                            }
                        break;
                    case R_LOGIC_RSHIFT:
                        if (C == 0)
                            {
                            CHECK_STACK_UNDER;
                            top1 = POP_EXPR;
                            CHECK_STACK_UNDER;
                            top2 = POP_EXPR;
                            PUSH_EXPR (((UINT)top2) >> top1);
                            }
                        else
                            {
                            top1 = POP_EXPR;
                            PUSH_EXPR (((UINT)top1) >> C);
                            }
                        break;
                    case R_PUSH_NCON1:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (-abs(V));
                        break;
                    default:
                        printErr ("linker: error reading fixups\n");
                        status = ERROR;
                        break;
                    }
                }

            else if (opCode == R_COMP2)
                {
                /* d1: arbitrary expression */
                opCode2 = B1;
                S = B3;
                L = opCode2 & 1;
                switch (opCode2)
                    {
                    case R_PUSH_PCON2:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (abs(V));
                        break;
                    case R_PUSH_SYM:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (pSym[S].symbol_value);
                        break;
                    case R_PUSH_PLABEL:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (pSym[S].symbol_value);
                        break;
                    case R_PUSH_NCON2:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (-abs(V));
                        break;
                    default:
                        printErr ("linker: error reading fixups\n");
                        status = ERROR;
                        break;
                    }
                }

            else if (opCode == R_COMP3)
                {
                /* d2: arbitrary expression */
                opCode2 = B1;
                V = B4;
                S = V & 0xffffff;
                R = ((opCode2 & 1) << 8) | (V >> 16);
                switch (opCode2)
                    {
                    case R_PUSH_PROC:
                        CHECK_STACK_OVER;
                        if (argRelocDiffer (pSym[S].arg_reloc, R))
                            relValue = (int)argRelocStubListFind (
                                          &argRelocStubList, pSym + S, R,
                                          moduleId);
                        else
                            relValue = pSym[S].symbol_value;
                        PUSH_EXPR (relValue);
                        break;
                    case R_PUSH_CONST:
                        CHECK_STACK_OVER;
                        PUSH_EXPR (V);
                        break;
                    default:
                        printErr ("linker: error reading fixups\n");
                        status = ERROR;
                        break;
                    }
                WARN ("Warning: R_COMP3 fixup is untested\n");
                }

            else if (opCode >= R_PREV_FIXUP && opCode < R_SEC_STMT)
                {
                /* d3-d6: apply previous fixup again */
                X = opCode - R_PREV_FIXUP + 1;
                DBG_PUT ("R_PREV_FIXUP - X = %d\n", X);

                /* remember our old place in the fixup stream */
                pOldFixup = pThisFixup;

                /* restore old fixup pointer (from list) */
                pPrevFixupNode = (PREV_FIXUP *)lstNth (&prevFixupList, X);
                if (pPrevFixupNode->pFixup == NULL)
                    printErr ("fixup request queue empty\n");
                pThisFixup = pPrevFixupNode->pFixup;

                /* remove old fixup pointer from the list */
                lstDelete (&prevFixupList, &(pPrevFixupNode->node));
                lstAdd (&prevFixupList, &(pPrevFixupNode->node));
                pPrevFixupNode->pFixup = NULL;

                usingPrevFixup = TRUE;
                continue;
                }

            else if (opCode == R_SEC_STMT)
                {
                /* d7: secondary statement number */
                /* d8-df: reserved */
                }

            else
                {        
                printErr ("loader: error reading fixup list\n");
                return (ERROR);
                }

            if (usingPrevFixup)
                {
                pThisFixup = pOldFixup;
                usingPrevFixup = FALSE;
                }
            }
        }

    /*
     * allocate an array of unwind desciptors.
     * copy the "unwindList" into the array.
     * add the array as a module segment of type SEGMENT_UNWIND
     */

    unwindSegmentCreate (&unwindList, moduleId);

    /* free up dynamically allocated memory */

    free ((char *) pExpStack);
    lstFree (&stubList);
    lstFree (&argRelocStubList);
    lstFree (&unwindList);
    return (status);
    }

/*******************************************************************************
*
* subspaceToSegment - map subspaces into VxWorks "segments"
*
* For each subspace, we decidide if it belongs in the text, data, or
* bss segment.
* This routine alse performs partial subspace relocation by setting the
* subspace load addresses relative to zero. Full relocation will have
* to wait until we have malloc'ed text, data, and bss segments.
* We are careful to leave some space ebtween text subspaces for
* possible "long branch" stub insertion later on.
* 
* RETURNS: N/A
*/

static void subspaceToSegment 
    (
    SOM_HDR *    pHpHdr,         /* file header */
    UCHAR *      pRelocCmds,     /* relocation commands */
    SUB_INFO *   pSubInfo,       /* pointer to subspace information */
    SPACE *      pSpace,         /* pointer to space dictionary */ 
    SUBSPACE *   pSubspace,      /* pointer to subspace dictionary */ 
    SYMREC *     pSym,           /* pointer to symbol table */
    SYMTAB_ID    pSymTab,        /* symbol table */
    int *        pTextSize,      /* returns textsize */
    int *        pDataSize,      /* returns data size */ 
    int *        pBssSize        /* returns bss size */ 
    )
    {
    int 	ix;
    int 	tSize = 0;
    int 	dSize = 0;
    int 	bSize = 0;
    int 	sizeExtra;

    /* locate the $LIT$ section */
    for (ix = 0; ix < pHpHdr->subspace_total; ix ++)
        {
	if (!pSubspace[ix].is_loadable)
            continue;
	if (!strcmp(pSubspace[ix].subspace_name, "$LIT$"))
	    {
	    tSize = ROUND_UP (tSize, pSubspace[ix].alignment);
	    pSubInfo[ix].loadAddr = (char *)tSize;
	    tSize += pSubspace[ix].subspace_length;
	    pSubInfo[ix].loadType = LOAD_TEXT;
	    }
	}
    /* locate the $CODE$ section */
    for (ix = 0; ix < pHpHdr->subspace_total; ix ++)
        {
	if (!pSubspace[ix].is_loadable)
            continue;
	if (!strcmp(pSubspace[ix].subspace_name, "$CODE$"))
	    {
	    tSize = ROUND_UP (tSize, pSubspace[ix].alignment);
	    pSubInfo[ix].loadAddr = (char *)tSize;
	    tSize += pSubspace[ix].subspace_length;
	    pSubInfo[ix].loadType = LOAD_TEXT;

	    /* compute how much room is needed for long branch stubs */
	    sizeExtra = subspaceSizeExtra (pRelocCmds, &pSubspace[ix], pSym,
					   pHpHdr->symbol_total, pSymTab);

	    DBG_PUT ("subspace %d sizeExtra= 0x%x\n", ix, sizeExtra);
	    tSize = ROUND_UP (tSize, pSubspace[ix].alignment);

	    tSize += sizeExtra;
	    }
	}
    /* locate the $DATA$ section */
    for (ix = 0; ix < pHpHdr->subspace_total; ix ++)
        {
	if (!pSubspace[ix].is_loadable)
            continue;
	if (!strcmp(pSubspace[ix].subspace_name, "$DATA$"))
	    {
	    dSize = ROUND_UP (dSize, pSubspace[ix].alignment);
	    pSubInfo[ix].loadAddr = (char *)dSize;
	    dSize += pSubspace[ix].subspace_length;
	    pSubInfo[ix].loadType = LOAD_DATA;
	    }
	}
    /* locate the $BSS$ section */
    for (ix = 0; ix < pHpHdr->subspace_total; ix ++)
        {
	if (!pSubspace[ix].is_loadable)
            continue;
	if (!strcmp(pSubspace[ix].subspace_name, "$BSS$"))
	    {
	    bSize = ROUND_UP (bSize, pSubspace[ix].alignment);
	    pSubInfo[ix].loadAddr = (char *)bSize;
	    bSize += pSubspace[ix].subspace_length;
	    pSubInfo[ix].loadType = LOAD_BSS;
	    }
	}

    /* set return value */
    *pTextSize = tSize;
    *pDataSize = dSize;
    *pBssSize  = bSize;
    }

/*******************************************************************************
*
* subspaceNoReloc -
*
* Set the load address to be the same as the subspace start address.
*
* RETURNS: OK or ERROR
*/

static void subspaceNoReloc
    (
    SOM_HDR *   pHpHdr,         /* file header */
    SUB_INFO *  pSubInfo,       /* pointer to subspace information */
    SUBSPACE *  pSubspace       /* pointer to subspace dictionary */
    )
    {
    int   ix;
    char *name;

    for (ix = 0; ix < pHpHdr->subspace_total; ix ++)
        {
        pSubInfo[ix].loadAddr = (char *) (pSubspace[ix].subspace_start);
        name = pSubspace[ix].subspace_name;
        if ((int)strstr (name, "BSS"))
            pSubInfo[ix].loadType = LOAD_BSS;
        }
    }

/*******************************************************************************
*
* loadSubspaces - load the subspaces with initial data from the object file.
*
* RETURNS: OK or ERROR
*/

static STATUS loadSubspaces 
    (
    int           fd,
    SOM_HDR *     pHpHdr, 
    SUB_INFO *    pSubInfo, 
    SUBSPACE *    pSubspace, 
    char *        pText, 
    char *        pData, 
    char *        pBss
    )
    {
    int      ix;

    /* loop through each subspace to get info */
    for (ix = 0; ix < pHpHdr->subspace_total; ix ++)
	{
        if (!pSubspace[ix].is_loadable || !pSubspace[ix].subspace_length)
	    continue;

        if (pSubspace[ix].continuation)
            WARN ("Warning: continuation subspaces are untested\n");

	switch (pSubInfo[ix].loadType)
	    {
	  case LOAD_TEXT:
	    pSubInfo[ix].loadAddr += (int) pText;
	    break;

	  case LOAD_DATA:
	    pSubInfo[ix].loadAddr += (int) pData;
	    break;

	  case LOAD_BSS:
	    pSubInfo[ix].loadAddr += (int) pBss;
	    break;

	  default:
            WARN ("unknown load type\n");
	    break;
	    }

        DBG_PUT ("subspace %d type= %d addr= 0x%x size= 0x%x\n",
		 ix, pSubInfo[ix].loadType, (int)pSubInfo[ix].loadAddr,
		 pSubspace[ix].subspace_length);

        /* if initialization length == 0, fill subspace with pattern
         * specified by file_loc_init_value */

	if (pSubspace[ix].initialization_length == 0)
	    {
            DBG_PUT ("initializing subspace with pattern 0x%x\n",
		     pSubspace[ix].file_loc_init_value);
            *(int *)(pSubInfo[ix].loadAddr)
		= pSubspace[ix].file_loc_init_value;
            bcopyLongs (pSubInfo[ix].loadAddr, pSubInfo[ix].loadAddr + 4,
                        (pSubspace[ix].subspace_length - 4)/4);
            continue;
            }

        /* if initialization length != 0, read from file into subspace */

        if (ioctl (fd, FIOSEEK, pSubspace[ix].file_loc_init_value)
                   == ERROR ||
            read (fd, pSubInfo[ix].loadAddr, pSubspace[ix].initialization_length)
	    != pSubspace[ix].initialization_length)
            {
            printErr ("loadSubspaces: error reading file\n");
            return (ERROR);
            }

        if (pSubspace[ix].replicate_init)
            {
            bcopy (pSubInfo[ix].loadAddr,
		   pSubInfo[ix].loadAddr + pSubspace[ix].initialization_length,
		   pSubspace[ix].subspace_length - pSubspace[ix].initialization_length);
            }

        }

    return (OK);
    }

/******************************************************************************
*
* argRelocDiffer - decide if the two argument relocation bit desciptors
*                  are compatable.
*
* RETURNS: TRUE if an argument relocation stub is needed.
*/

static BOOL argRelocDiffer (int arg_reloc1, int arg_reloc2)
    {
    int i;
    int reloc1, reloc2;
    for (i=0; i<5; i++)
        {
        reloc1 = arg_reloc1 & 3;
        reloc2 = arg_reloc2 & 3;
        if (reloc1 && reloc2 && (reloc1 != reloc2))
            return (TRUE);
        arg_reloc1 >>= 2;
        arg_reloc2 >>= 2;
        }
    return (FALSE);
    }

/******************************************************************************
*
* subspaceSizeExtra - calculate additional space needed for a subspace
*
* This routine calculates additional storage space needed for each
* subspace. It is used to reserve space for "long branch stubs"
* which we must insert later if a fixup request for a PC relative call
* can't be stuffed into a 19 bit immediate.
*
* RETURNS number of additional bytes needed
*/

static int subspaceSizeExtra
    (
    UCHAR *    pRelCmds,        /* address of relocation commands */
    SUBSPACE * pSubspace,       /* subspace index */
    SYMREC *   pSym,            /* pointer to symbol records */
    int        numSyms,         /* maximun number of symbols */
    SYMTAB_ID  symTblId
    )
    {
    UCHAR *  pNextFixup;             /* current fixup request pointer */
    UCHAR *  pLastFixup;             /* last fixup request pointer */
    UCHAR    opCode;                 /* fixup request opCode */
    int      symNum;
    int      R;

    LIST     stubList;
    int      numStubs;

    lstInit (&stubList);

    if (pSubspace->fixup_request_quantity == 0)
        return (0);

    pNextFixup = pRelCmds + pSubspace->fixup_request_index;
    pLastFixup = pNextFixup + pSubspace->fixup_request_quantity;

    while (pNextFixup < pLastFixup)
        {
        opCode = *pNextFixup;
        R = 0;

        /* do we have a pc-relative call fixup request? */

        /* SPR 22030 :Changed R_ABS_CALL below to 62 */
        if ( (opCode >= R_PCREL_CALL) && (opCode < 62) )
            {
            /* if so, get the symbol index we are calling */
            if (opCode <= 57)
                {
                R = rbits1 ((int)opCode - R_PCREL_CALL);
                symNum = *(pNextFixup + 1);
                }
            else if (opCode <= 59)
                {
                R = rbits2 ( (((int)opCode - 58)<<8) + *(pNextFixup+1) );
                symNum = *(pNextFixup + 2);
                }
            else
                {
                R = rbits2 ( (((int)opCode - 60)<<8) + *(pNextFixup+1) );
                symNum = ((int)(*(pNextFixup + 2)) << 16) +
                         ((int)(*(pNextFixup + 3)) << 8) +
                         *(pNextFixup + 4);
                }
            DBG_PUT ("sizeExtra: symNum = %d \n", symNum);
            if (stubListFind (&stubList, symNum, R) == -1)
		{
		SYMREC tmp;
		int arg_reloc;
		int parm_reloc;

		if (externSymResolve(symTblId, pSym[symNum].symbol_name, &tmp) == OK)
		    arg_reloc = tmp.arg_reloc;
                else
		    arg_reloc = pSym[symNum].arg_reloc;
                if (!arg_reloc)
		    WARN("symbol %s has arg_reloc of zero.\n",
			pSym[symNum].symbol_name);

		/* Now make sure we don't get a duplicate entry */
		if (stubListFind(&stubList, symNum, tmp.arg_reloc) == -1)
		    stubListAdd (&stubList, symNum, tmp.arg_reloc);
		/* Now see if we need a parm reloc stub */
		parm_reloc = (tmp.arg_reloc & 0x03f3) | 0x0c;
		if (argRelocDiffer(R, tmp.arg_reloc)
		    && !argRelocDiffer(R, parm_reloc))
		    {
		    stubListAdd (&stubList, symNum, parm_reloc);
		    }
                }
            }

        if (fixupLen [opCode] == 0)
            {
            printErr ("subspaceSizeExtra: error reading fixup table.\n");
            break;
            }

        pNextFixup += fixupLen [opCode];
        }

    numStubs = lstCount (&stubList);
    lstFree (&stubList);
    return (numStubs*stubSize);
    }

/******************************************************************************
*
* externSymResolve - look up a MY_SYMBOL in the VxWorks symbol table.
*/

static BOOL externSymResolve
    (
    SYMTAB_ID symTblId,
    char *    name,
    SYMREC *  pSym
    )
    {
    MY_SYMBOL *pSymbol;
    MY_SYMBOL keySym;

    keySym.name = name;
    keySym.type = N_EXT;
    pSymbol = (MY_SYMBOL *)hashTblFind (symTblId->nameHashId,
                                       &keySym.nameHNode, N_EXT);

    if (pSymbol == NULL)
        return (ERROR);

    pSym->symbol_value = (int)pSymbol->value;
    pSym->arg_reloc    = (pSymbol->arg_reloc << 2) | ((pSymbol->type >> 6) & 3);
    return (OK);
    }

/******************************************************************************
*
* externSymAdd - add a MY_SYMBOL to the VxWorks symbol table.
*/

static BOOL externSymAdd
    (
    SYMTAB_ID symTblId,
    char *    name,
    char      *value,           /* symbol address */
    SYM_TYPE  type,             /* symbol type */
    UINT16    group,            /* symbol group */
    UCHAR     arg_reloc
    )
    {
    SYMBOL *pSymbol = symAlloc (symTblId, name, value, type, group);

    if (pSymbol == NULL)                        /* out of memory? */
        return (ERROR);

    ((MY_SYMBOL *)pSymbol)->arg_reloc = arg_reloc;

    if (symTblAdd (symTblId, pSymbol) != OK)    /* try to add symbol */
        {
        symFree (symTblId, pSymbol);            /* deallocate symbol if fail */
        return (ERROR);
        }

    return (OK);
    }

/******************************************************************************
*
* rbitsShow - Display the 10 bit argument relocation info in human readable
*             form.
*/

static void rbitsShow (int x)
    {
    int i;
    DBG_PUT ("(");
    for (i=0; i<4; i++)
        {
        switch ((x >> (8 - 2*i)) & 3)
            {
            case 0:
                DBG_PUT ("? ");
                break;
            case 1:
                DBG_PUT ("int ");
                break;
            case 2:
                DBG_PUT ("float ");
                break;
            case 3:
                DBG_PUT ("double ");
                break;
            }
        DBG_PUT ("arg%d", i);
        if (i != 3)
            DBG_PUT (", ");
        }
    DBG_PUT (")");
    DBG_PUT (". Returns: ");
    switch (x & 3)
        {
        case 0:
            DBG_PUT ("?\n");
            break;
        case 1:
            DBG_PUT ("int\n");
            break;
        case 2:
            DBG_PUT ("float\n");
            break;
        case 3:
            DBG_PUT ("double\n");
            break;
        }
    }

/******************************************************************************
*
* rbits1 - return the 10 argument relocation bits encoded in a fixup.
*/

static int rbits1 (int x)
    {
    return ( ((0x55 & ~((1 << 2*(4 - (x % 5))) - 1)) << 2) | (x/5));
    }

/******************************************************************************
*
* rbits2 - return the 10 argument relocation bits encoded in a fixup.
*/

static int rbits2 (int x)
    {
    int args01;
    int args23;
    int arg0, arg1, arg2, arg3;

    args01 = (x>>2) / 10;
    if (args01 == 9)
        {
        arg0 = 0;
        arg1 = 3;
        }
    else
        {
        arg0 = args01 / 3;
        arg1 = args01 % 3;
        }

    args23 = (x>>2) % 10;
    if (args23 == 9)
        {
        arg2 = 0;
        arg3 = 3;
        }
    else
        {
        arg2 = args23 / 3;
        arg3 = args23 % 3;
        }

    return ((arg0 << 8) | (arg1 << 6) | (arg2 << 4) | (arg3 << 2) | (x & 3));
    }

/******************************************************************************
*
* stubListFind - find a long branch stub in the list.
*
* RETURNS: the index in the list of the stub, or -1 if the stub isn't found.
*/

static int stubListFind
    (
    LIST *stubList,
    int symNum,
    int arg_reloc
    )
    {
    STUB_LIST_NODE *pNode;
    int i = 0;

    for (pNode = (STUB_LIST_NODE *)lstFirst(stubList);
        pNode != NULL;
        pNode = (STUB_LIST_NODE *)lstNext(&(pNode->node)))
        {
        if ( (pNode->symNum == symNum) &&
            !argRelocDiffer (pNode->arg_reloc, arg_reloc))
            break;
        i++;
        }
    return (pNode == NULL ? -1 : i);
    }

/******************************************************************************
*
* stubListAdd - add a long branch stub to the list.
*/

static int stubListAdd
    (
    LIST *stubList,
    int symNum,
    int arg_reloc
    )
    {
    STUB_LIST_NODE *pNode;

    pNode = (STUB_LIST_NODE *)malloc (sizeof(STUB_LIST_NODE));
    pNode->symNum = symNum;
    pNode->arg_reloc = arg_reloc;
    lstAdd (stubList, &(pNode->node));
    return (lstCount(stubList) - 1);
    }

/******************************************************************************
*
* stubInit - initialize a long branch stub at address stubAddr. The stub
*            created branches to the procedure at procAddr.
*/

static void stubInit
    (
    char *stubAddr,
    char *procAddr
    )
    {
    bcopy ((char *)longBranchStub, stubAddr, stubSize);
    fixBits ((INSTR *)stubAddr, (int)procAddr, i_exp21);
    fixBits ((INSTR *)(stubAddr+4), (int)procAddr & 0x7ff, i_rel12);
    }

/******************************************************************************
*
* argRelocStubCreate - create an argument relocation stub.
*
* The stub created relocates the arguments from the arg_reloc format
* to the pSym->arg_reloc format, and then branches to procAddr.
*
* RETURNS: the address of the newly created stub. If a stub cannot be
*          generated (we currently only support one type of stub),
*          then an error message is printed and NULL is returned.
*/

static char *argRelocStubCreate
    (
    SYMREC * pSym,
    int      arg_reloc,
    char *   procAddr
    )
    {
    char *   stubAddr;
    int      parm_reloc;

   /*
    * This is the stub code for parameter relocation if the second parameter
    * is passed in a double persion floating point register, and must be
    * move to the general registers.
    * Currently, this is the only parameter relocation stub this linker
    * generates, but it would be a simple matter to add others.
    */

    static int argReloc_double_to_regs[5] =
        {
        0x2fd01227,     /* FSTDS,MA fr7, 8(r30)   */
        0x0fd91098,     /* LDWS     -4(r30),r24   */
        0x0fd130b7,     /* LDWS,MB  -8(r30),r23   */
        0x20200000,     /* LDIL    0, r1          */
        0xe0202002      /* BE,n    0(s4, r1)      */
        };

    parm_reloc = (pSym->arg_reloc & 0x03f3) | 0x0c;
    if (!argRelocDiffer (parm_reloc, arg_reloc))
        {
        int oldLoadSomCoffDebug = loadSomCoffDebug;
        loadSomCoffDebug = 1;
        printErr ("Link error: You called %s ");
        rbitsShow (arg_reloc);
        printErr ("But it is supposed to be called as ");
        rbitsShow (pSym->arg_reloc);
        printErr ("Unable to relocate these parameters between ");
        printErr ("the floating point and general purpose registers!\n");
        loadSomCoffDebug = oldLoadSomCoffDebug;
        return (NULL);
        }

    stubAddr = (char *)malloc (sizeof (argReloc_double_to_regs));

    DBG_PUT ("creating arg_reloc stub at 0x%x, which jumps to %s\n",
              (int)stubAddr, pSym->symbol_name);

    bcopy ((char *)argReloc_double_to_regs, stubAddr,
           sizeof (argReloc_double_to_regs));
    fixBits ((INSTR *)(stubAddr + 12), (int)procAddr, i_exp21);
    fixBits ((INSTR *)(stubAddr + 16), (int)procAddr & 0x7ff, i_rel12);
    return (stubAddr);
    }

/******************************************************************************
*
* argRelocStubListAdd - put an argument relocation stub into the list.
*
* After the stub is created, we add a node to the list which points to
* the stub. This way we can keep track of the stubs already generated
* so we don't generate the same stub twice.
*/

static BOOL argRelocStubListAdd
    (
    LIST *   stubList,
    SYMREC * pSym,
    int      arg_reloc,
    char *   stubAddr
    )
    {
    ARG_RELOC_NODE *pNode;

    pNode = (ARG_RELOC_NODE *)malloc (sizeof(ARG_RELOC_NODE));
    if (pNode == NULL)
        return (ERROR);
    pNode->pSym = pSym;
    pNode->arg_reloc = arg_reloc;
    pNode->stubAddr = stubAddr;
    lstAdd (stubList, &pNode->node);
    return (OK);
    }

/******************************************************************************
*
* argRelocStubListFind - Find an argument relocation stub in the list.
*                 If it doesn;t exist, create it and add it to the list.
*
* RETURNS: the address of the stub.
*/

static char *argRelocStubListFind
    (
    LIST *    stubList,
    SYMREC *  pSym,
    int       arg_reloc,
    MODULE_ID moduleId
    )
    {
    ARG_RELOC_NODE *pNode;
    char *    stubAddr;
    int       parm_reloc;

    for (pNode = (ARG_RELOC_NODE *)lstFirst(stubList);
        pNode != NULL;
        pNode = (ARG_RELOC_NODE *)lstNext(&(pNode->node)))
        {
        if ( (pNode->pSym == pSym) &&
            !argRelocDiffer (pNode->arg_reloc, arg_reloc))
            {
            DBG_PUT ("found stub at 0x%x\n", (int)pNode->stubAddr);
            return (pNode->stubAddr);
            }
        }

    stubAddr = argRelocStubCreate (pSym, arg_reloc,
                                  (char *)(pSym->symbol_value));
    if (stubAddr == NULL)
        return (NULL);

    moduleSegAdd (moduleId, SEGMENT_STUB, stubAddr, 0, SEG_FREE_MEMORY);

    parm_reloc = (arg_reloc & 0x03f3) | 0x0c;
    if (argRelocStubListAdd (stubList, pSym, parm_reloc, stubAddr) == ERROR)
        return (NULL);

    return (stubAddr);
    }

/******************************************************************************
*
* unwindSegmentCreate - Create a stack unwind segement for a module.
*
* This routine takes a sorted linked list of stack unwind descriptors.
* It uses this list to create an unwind segment for the module.
* The unwind segment is needed by the stack tracer.
*/

static void unwindSegmentCreate
    (
    LIST *    unwindList,
    MODULE_ID moduleId
    )
    {
    UNWIND_DESC * unwindArray;
    UNWIND_NODE * pThisNode;
    int           numDescs;
    int           i;

    numDescs = lstCount(unwindList);

    unwindArray = calloc (numDescs, sizeof (UNWIND_DESC));

    if (unwindArray == NULL)
        {
        printErr ("load error: can't allocate stack unwind descriptors\n");
        return;
        }

    DBG_PUT ("initializing unwind array of size %d at 0x%x\n", 
              (int)numDescs, (int)unwindArray);

    for (pThisNode = (UNWIND_NODE *)lstFirst (unwindList), i = 0;
         (pThisNode != NULL) && (i < numDescs);
         pThisNode = (UNWIND_NODE *)lstNext (&pThisNode->node), i++)
        {
        bcopy ((char *)(&pThisNode->unwindDesc), (char *)(unwindArray + i),
               sizeof (UNWIND_DESC));
        }

    moduleSegAdd (moduleId, SEGMENT_UNWIND, (void *)unwindArray,
                  sizeof (UNWIND_DESC) * numDescs, SEG_FREE_MEMORY);
    }

/******************************************************************************
*
* unwindListAdd - add a stack unwind descriptor to a list.
*
* The desciptor is sorted into the list.
*/

static void unwindListAdd
    (
    LIST *        unwindList,
    UNWIND_NODE * pUnwindNode
    )
    {
    UNWIND_NODE * pThisNode;

    for (pThisNode = (UNWIND_NODE *)lstLast (unwindList);
         pThisNode != NULL;
         pThisNode = (UNWIND_NODE *)lstPrevious (&pThisNode->node))
        {
        if (pUnwindNode->unwindDesc.startAddr > pThisNode->unwindDesc.startAddr)
            break;
        }

    lstInsert (unwindList, &pThisNode->node, &pUnwindNode->node);
    }
