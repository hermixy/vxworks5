/* dsmLib.c - i80x86 disassembler */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01k,27mar02,hdn  fixed the CALL/JMP ptr16:16/32 offset size (spr 73624)
01j,30aug01,hdn  added SIMD, sysenter/exit support.
		 always print the disassembled address on 8 digits with 0x.
01i,06may98,fle  added P5 and P6 related instructions and facilities
01h,14nov94,hdn  changed D->DISR, W->WFUL, S->SEXT, P->POP, A->AX, I->IMM.
01g,29may94,hdn  removed I80486.
01f,31aug93,hdn  changed a type of 1st parameter, from char to UCHAR.
01e,02aug93,hdn  fixed a case that has mod=0,rm=5,disp32 operand.
01d,01jun93,hdn  updated to 5.1
		  - changed functions to ansi style
		  - fixed #else and #endif
		  - changed VOID to void
		  - changed copyright notice
01c,18mar93,hdn  supported 486 instructions.
01b,05nov92,hdn  supported "16 bit operand","rep","repne","shift by 1".
		 fixed a bug that is about "empty index".
01a,23jun92,hdn  written. 
*/

/*
This library contains everything necessary to print i80x86 object code in
assembly language format. 

The programming interface is via dsmInst(), which prints a single disassembled
instruction, and dsmNbytes(), which reports the size of an instruction.

To disassemble from the shell, use l(), which calls this
library to do the actual work.  See dbgLib for details.

INCLUDE FILE: dsmLib.h

SEE ALSO: dbgLib
*/

#include "vxWorks.h"
#include "dsmLib.h"
#include "symLib.h"
#include "string.h"
#include "stdio.h"
#include "errnoLib.h"

/*
 * This table is ordered by the number of bits in an instruction's 
 * two word mask, beginning with the greatest number of bits in masks.  
 * This scheme is used for avoiding conflicts between instructions 
 * when matching bit patterns.  The instruction ops are arranged 
 * sequentially within each group of instructions for a particular 
 * mask so that uniqueness can be easily spotted.  
 */

/* globals */

LOCAL INST inst [] =
    {

    /* OP3 instructions */

    {"ADDSS", itAddss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x58,	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"CMPSS", itCmpss,		OP3|MODRM|I8,XMMREG|XMMRM,
    				0xf3, 0x0f, 0xc2, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"CVTSI2SS", itCvtsi2ss,    OP3|MODRM|REGRM,XMMREG,
    				0xf3, 0x0f, 0x2a, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"CVTSS2SI", itCvtss2si,	OP3|MODRM|REG,XMMRM,
    				0xf3, 0x0f, 0x2d, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"CVTTSS2SI", itCvttss2si,	OP3|MODRM|REG,XMMRM,
    				0xf3, 0x0f, 0x2c, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"DIVSS", itDivss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x5e, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"MAXSS", itMaxss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x5f, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"MINSS", itMinss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x5d, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"MOVSS", itMovss,          OP3|MODRM|DISR,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x10, 	/* opcode */
				0xff, 0xff, 0xfe},	/* mask */

    {"MULSS", itMulss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x59, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"RCPSS", itRcpss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x53, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"RSQRTSS", itRsqrtss,	OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x52, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"SQRTSS", itSqrtss,	OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x51, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"SUBSS", itSubss,		OP3|MODRM,XMMREG|XMMRM,
    				0xf3, 0x0f, 0x5c, 	/* opcode */
				0xff, 0xff, 0xff},	/* mask */

    {"SFENCE", itSfence,	OP3,0,
    				0x0f, 0xae, 0xc0,	/* opcode */
				0xff, 0xff, 0xc0},	/* mask */


    /* OP2 instructions extended by bits 3,4,5 of MODRM */

    {"BT", itBtI,       	OP2|MODRM|I8,0,
    				0x0f, 0xba, 0x20, 	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"BTC", itBtcI,     	OP2|MODRM|I8,0,
    				0x0f, 0xba, 0x38,	/* opcode */
				0xff, 0xff, 0x38},	/* mask	*/

    {"BTR", itBtrI,    	  	OP2|MODRM|I8,0,
    				0x0f, 0xba, 0x30,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"BTS", itBtsI,      	OP2|MODRM|I8,0,
    				0x0f, 0xba, 0x28,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"LGDT", itLgdt,    	OP2|MODRM,0,
    				0x0f, 0x01, 0x10,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"LIDT", itLidt,     	OP2|MODRM,0,
    				0x0f, 0x01, 0x18,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"LLDT", itLldt,     	OP2|MODRM,0,
    				0x0f, 0x00, 0x10,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"LMSW", itLmsw,     	OP2|MODRM,0,
    				0x0f, 0x01, 0x30,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"LTR", itLtr,       	OP2|MODRM,0,
    				0x0f, 0x00, 0x08,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"SGDT", itSgdt,     	OP2|MODRM,0,
    				0x0f, 0x01, 0x00,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"SIDT", itSidt,     	OP2|MODRM,0,
    				0x0f, 0x01, 0x08,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"SLDT", itSldt,     	OP2|MODRM,0,
    				0x0f, 0x00, 0x00,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */
				
    {"SMSW", itSmsw,     	OP2|MODRM,0,
    				0x0f, 0x01, 0x20,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"STR", itStr,       	OP2|MODRM,0,
    				0x0f, 0x00, 0x08,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"VERR", itVerr,      	OP2|MODRM,0,
    				0x0f, 0x00, 0x20,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"VERW", itVerw,      	OP2|MODRM,0,
    				0x0f, 0x00, 0x28,	/* opcode */
				0xff, 0xff, 0x38},	/* mask */

    {"FXRSTOR", itFxrstor,      OP2|MODRM,0,
                                0x0f, 0xae, 0x08,       /* opcode */
				0xff, 0xff, 0x38},      /* mask */

    {"FXSAVE", itFxsave,        OP2|MODRM,0,
				0x0f, 0xae, 0x00,       /* opcode */
				0xff, 0xff, 0x38},      /* mask */

    {"LDMXCSR", itLdmxcsr,      OP2|MODRM,0,
				0x0f, 0xae, 0x10,       /* opcode */
				0xff, 0xff, 0x38},      /* mask */
    
    {"STMXCSR", itStmxcsr,      OP2|MODRM,0,
				0x0f, 0xae, 0x18,       /* opcode */
				0xff, 0xff, 0x38},      /* mask */

    {"PREFETCHT0", itPrefetcht0,        OP2|MODRM,0,
					0x0f, 0x18, 0x08,       /* opcode */
					0xff, 0xff, 0x38},      /* mask */

    {"PREFETCHT1", itPrefetcht1,        OP2|MODRM,0,
					0x0f, 0x18, 0x10,       /* opcode */
					0xff, 0xff, 0x38},      /* mask */

    {"PREFETCHT2", itPrefetcht2,        OP2|MODRM,0,
					0x0f, 0x18, 0x18,       /* opcode */
					0xff, 0xff, 0x38},      /* mask */

    {"PREFETCHNTA", itPrefetchnta,      OP2|MODRM,0,
					0x0f, 0x18, 0x00,       /* opcode */
					0xff, 0xff, 0x38},      /* mask */

    {"MOVHLPS", itMovhlps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x12, 0xc0,	/* opcode */
				0xff, 0xff, 0xc0},	/* mask */

    {"MOVLHPS", itMovlhps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x16, 0xc0,	/* opcode */
				0xff, 0xff, 0xc0},	/* mask */

    /* OP2 instructions */

    {"AAD", itAad,       	OP2,0,
   				0xd5, 0x0a, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"AAM", itAam,       	OP2,0,
    				0xd4, 0x0a, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"BSF", itBsf,       	OP2|MODRM|REG,0,
    				0x0f, 0xbc, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"BSR", itBsr,       	OP2|MODRM|REG,0,
    				0x0f, 0xbd, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"BT", itBtR,        	OP2|MODRM|REG,0,
    				0x0f, 0xa3, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"BTC", itBtcR,      	OP2|MODRM|REG,0,
    				0x0f, 0xbb, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"BTR", itBtrR,      	OP2|MODRM|REG,0,
    				0x0f, 0xb3, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"BTS", itBtsR,      	OP2|MODRM|REG,0,
    				0x0f, 0xab, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CLTS", itClts,     	OP2,0,
    				0x0f, 0x06, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CMPXCHG8B", itCmpxchg8b,	OP2|MODRM,0,
    				0x0f, 0xc7, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CPUID", itCpuid,		OP2,0,
    				0x0f, 0xa2, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"EMMS", itEmms,		OP2,0,
    				0x0f, 0x77, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"F2XM1", itF2xm1,		OP2,0,
    				ESC|0x01, 0xf0, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FABS", itFabs,		OP2,0,
    				ESC|0x01, 0xe1, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FCHS", itFchs,		OP2,0,
    				ESC|0x01, 0xe0, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FCLEX", itFclex,		OP2,0,
    				ESC|0x03, 0xe2, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FCOMPP", itFcompp,	OP2,0,
    				ESC|0x06, 0xd9, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FCOS", itFcos,		OP2,0,
    				ESC|0x01, 0xff, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FDECSTP", itFdecstp,	OP2,0,
    				ESC|0x01, 0xf6, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FINCSTP", itFincstp,	OP2,0,
    				ESC|0x01, 0xf7, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FINIT", itFinit,		OP2,0,
    				ESC|0x03, 0xe3, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLDZ", itFldZ,		OP2,0,
    				ESC|0x01, 0xee, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLD1", itFld1,		OP2,0,
    				ESC|0x01, 0xe8, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLDPI", itFldPI,		OP2,0,
    				ESC|0x01, 0xeb, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLDL2T", itFldL2T,	OP2,0,
    				ESC|0x01, 0xe9, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLDL2E", itFldL2E,	OP2,0,
    				ESC|0x01, 0xea, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLDLG2", itFldLG2,	OP2,0,
    				ESC|0x01, 0xec, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FLDLN2", itFldLN2,	OP2,0,
    				ESC|0x01, 0xed, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FNOP", itFnop,		OP2,0,
    				ESC|0x01, 0xd0, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FPATAN", itFpatan,	OP2,0,
    				ESC|0x01, 0xf3, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FPREM", itFprem,		OP2,0,
    				ESC|0x01, 0xf8, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FPREM1", itFprem1,	OP2,0,
    				ESC|0x01, 0xf5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FPTAN", itFptan,		OP2,0,
    				ESC|0x01, 0xf2, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FRNDINT", itFrndint,	OP2,0,
    				ESC|0x01, 0xfc, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FSCALE", itFscale,	OP2,0,
    				ESC|0x01, 0xfd, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FSIN", itFsin,		OP2,0,
    				ESC|0x01, 0xfe, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FSINCOS", itFsincos,	OP2,0,
    				ESC|0x01, 0xfb, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FSQRT", itFsqrt,		OP2,0,
    				ESC|0x01, 0xfa, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FSTSW", itFstswA,		OP2|AX,0,
    				ESC|0x07, 0xe0, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FTST", itFtst,		OP2,0,
    				ESC|0x01, 0xe4, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FUCOMPP", itFucompp,	OP2,0,
    				ESC|0x02, 0xe9, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FXAM", itFxam,		OP2,0,
    				ESC|0x01, 0xe5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FXTRACT", itFxtract,	OP2,0,
    				ESC|0x01, 0xf4, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FYL2X", itFyl2x,		OP2,0,
    				ESC|0x01, 0xf1, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"FYL2XP1", itFyl2xp1,	OP2,0,
    				ESC|0x01, 0xf9, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"IMUL", itImulRwiRM, 	OP2|MODRM|REG,0,
    				0x0f, 0xaf, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"INVD", itInvd,       	OP2,0,
    				0x0f, 0x08, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"INVLPG", itInvlpg,       	OP2|MODRM,0,
    				0x0f, 0x01, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"LAR", itLar,       	OP2|MODRM|REG,0,
    				0x0f, 0x02, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"LFS", itLfs,       	OP2|MODRM|REG,0,
    				0x0f, 0xb4, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"LGS", itLgs,       	OP2|MODRM|REG,0,
    				0x0f, 0xb5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"LSL", itLsl,       	OP2|MODRM|REG,0,
    				0x0f, 0x03, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"LSS", itLss,       	OP2|MODRM|REG,0,
    				0x0f, 0xb2, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    /* 
     * MOV Changes
     * see mask byte 2 changes on next entry.
     * This change is only done on entries that are extact,
     * with exception to opcode byte 2 bit 2
     *
     * {"MOV", itMovC,          OP2|DISR|EEE|MODRM,0,
     *                          0x0f, 0x22, 0x00,
     *                          0xff, 0xff, 0x00},
     */

    {"MOV", itMovC,      	OP2|DISR|EEE|MODRM,0,
    				0x0f, 0x20, 0x00,	/* opcode */
				0xff, 0xfd, 0x00},	/* mask */

    {"MOV", itMovD,      	OP2|DISR|EEE|MODRM,0,
    				0x0f, 0x21, 0x00,	/* opcode */
				0xff, 0xfd, 0x00},	/* mask */

    {"MOV", itMovT,      	OP2|DISR|EEE|MODRM,0,
    				0x0f, 0x24, 0x00,	/* opcode */
				0xff, 0xfd, 0x00},	/* mask */

    /* 
     * MOV Changes
     * see mask byte 2 changes on next entry.
     * This change is only done on entries that are extact,
     * with exception to opcode byte 2 bit 4
     *
     * {"MOVD", itMovd,         OP2|MMXREG|MODRM,0,
     *                          0x0f, 0x7e, 0x00,
     *                          0xff, 0xff, 0x00},
     */

    {"MOVD", itMovd,            OP2|MMXREG|MODRM|DISR,0,
                                0x0f, 0x6e, 0x00,       /* opcode */
				0xff, 0xef, 0x00},      /* mask */

    {"MOVQ", itMovq,            OP2|MMXREG|MMXRM|MODRM|DISR,0,
				0x0f, 0x6f, 0x00,       /* opcode */
				0xff, 0xef, 0x00},      /* mask */

    {"PACKSSDW", itPackssdw,	OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x6b, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PACKSSWB", itPacksswb,	OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x63, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PACKUSWB", itPackuswb,	OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x67, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PAND", itPand,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xdb, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PANDN", itPandn,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xdf, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMADD", itPmadd,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xf5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMULH", itPmulh,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xe5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMULL", itPmull,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xd5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"POR", itPor,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xeb, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PXOR", itPxor,		OP2|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xef, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"RDTSC", itRdtsc,		OP2,0,
    				0x0f, 0x31, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"RDMSR", itRdmsr,		OP2,0,
    				0x0f, 0x32, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"RDPMC", itRdpmc,		OP2,0,
    				0x0f, 0x33, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"RSM", itRsm,		OP2,0,
    				0x0f, 0xaa, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SHLD", itShldRMbyI, 	OP2|MODRM|REG|I8,0,
    				0x0f, 0xa4, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SHLD", itShldRMbyCL, 	OP2|MODRM|REG|CL,0,
    				0x0f, 0xa5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SHRD", itShrdRMbyI, 	OP2|MODRM|REG|I8,0,
    				0x0f, 0xac, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SHRD", itShrdRMbyCL, 	OP2|MODRM|REG|CL,0,
    				0x0f, 0xad, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SYSENTER", itSysenter,	OP2,0,
    				0x0f, 0x34, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SYSEXIT", itSysexit,	OP2,0,
    				0x0f, 0x35, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"UD2", itUd2,		OP2,0,
    				0x0f, 0x0b, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"WBINVD", itWbinvd,       	OP2,0,
    				0x0f, 0x09, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"WRMSR", itWrmsr,		OP2,0,
    				0x0f, 0x30, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */


    /* SIMD Instructions - 16 bit */

    {"ADDPS", itAddps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x58, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"ANDNPS", itAndnps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x55, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"ANDPS", itAndps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x54, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CMPPS", itCmpps,		OP2|MODRM|I8,XMMREG|XMMRM,
    				0x0f, 0xc2, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"COMISS", itComiss,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x2f, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CVTPI2PS", itCvtpi2ps,    OP2|MODRM|MMXRM,XMMREG,
    				0x0f, 0x2a, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CVTPS2PI", itCvtps2pi,	OP2|MODRM|MMXREG,XMMRM,
    				0x0f, 0x2d, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"CVTTPS2PI", itCvttps2pi,	OP2|MODRM|MMXREG,XMMRM,
    				0x0f, 0x2c, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"DIVPS", itDivps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x5e, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MAXPS", itMaxps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x5f, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MINPS", itMinps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x5d, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MOVAPS", itMovaps,	OP2|MODRM|DISR,XMMREG|XMMRM,
    				0x0f, 0x28, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"MOVHPS", itMovhps,	OP2|MODRM|DISR,XMMREG,
    				0x0f, 0x16, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"MOVLPS", itMovlps,	OP2|MODRM|DISR,XMMREG,
    				0x0f, 0x12, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"MOVMSKPS", itMovmskps,	OP2|MODRM|REG,XMMRM,
    				0x0f, 0x50, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MOVUPS", itMovups,	OP2|MODRM|DISR,XMMREG|XMMRM,
    				0x0f, 0x10, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"MULPS", itMulps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x59, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"ORPS", itOrps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x56, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"RCPPS", itRcpps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x53, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"RSQRTPS", itRsqrtps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x52, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SHUFPS", itShufps,	OP2|MODRM|I8,XMMREG|XMMRM,
    				0x0f, 0xc6, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SQRTPS", itSqrtps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x51, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"SUBPS", itSubps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x5c, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"UCOMISS", itUcomiss,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x2e, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"UNPCKHPS", itUnpckhps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x15, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"UNPCKLPS", itUnpcklps,	OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x14, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"XORPS", itXorps,		OP2|MODRM,XMMREG|XMMRM,
    				0x0f, 0x57, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PAVGB", itPavgb,		OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xe0, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */
				
    {"PAVGW", itPavgw,		OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xe3, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PEXTRW", itPextrw,	OP2|DISR|MODRM|MMXREG|REGRM|I8,0,
    				0x0f, 0xc5, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PINSRW", itPinsrw,	OP2|MODRM|MMXREG|REGRM|I8,0,
    				0x0f, 0xc4, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMAXSW", itPmaxsw,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xee, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMAXUB", itPmaxub,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xde, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMINSW", itPminsw,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xea, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMINUB", itPminub,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xda, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMOVMSKB", itPmovmskb,	OP2|MODRM|MMXREG|REGRM|DISR,0,
    				0x0f, 0xd7, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PMULHUW", itPmulhuw,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xe4, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PSADBW", itPsadbw,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xf6, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"PSHUFW", itPshufw,	OP2|MODRM|MMXREG|MMXRM|I8,0,
    				0x0f, 0x70, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MASKMOVQ", itMaskmovq,	OP2|MODRM|MMXREG|MMXRM,0,
    				0x0f, 0xf7, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MOVNTPS", itMovntps,	OP2|MODRM|DISR,XMMREG|XMMRM,
    				0x0f, 0x2b, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    {"MOVNTQ", itMovntq,	OP2|MODRM|DISR,XMMREG|XMMRM,
    				0x0f, 0xe7, 0x00,	/* opcode */
				0xff, 0xff, 0x00},	/* mask */

    /* 15 bits mask */

    {"CMPXCHG", itCmpxchg,     	OP2|WFUL|MODRM|REG,0,
    				0x0f, 0xb0, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"MOVSX", itMovsx,   	OP2|WFUL|MODRM|REG,0,
    				0x0f, 0xbe, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"MOVZX", itMovzx,   	OP2|WFUL|MODRM|REG,0,
    				0x0f, 0xb6, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REP INS", itRins,      	OP2|WFUL,0,
    				0xf3, 0x6c, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REP LODS", itRlods,    	OP2|WFUL,0,
    				0xf3, 0xac, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REP MOVS", itRmovs,    	OP2|WFUL,0,
    				0xf3, 0xa4, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REP OUTS", itRouts,    	OP2|WFUL,0,
    				0xf3, 0x6e, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REP STOS", itRstos,    	OP2|WFUL,0,
    				0xf3, 0xaa, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REPE CMPS", itRcmps,   	OP2|WFUL,0,
    				0xf3, 0xa6, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REPE SCAS", itRscas,   	OP2|WFUL,0,
    				0xf3, 0xae, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REPNE CMPS", itRNcmps, 	OP2|WFUL,0,
    				0xf2, 0xa6, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"REPNE SCAS", itRNscas, 	OP2|WFUL,0,
    				0xf2, 0xae, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    {"XADD", itXadd,       	OP2|WFUL|MODRM|REG,0,
    				0x0f, 0xc0, 0x00,	/* opcode */
				0xff, 0xfe, 0x00},	/* mask */

    /* 14 + 5 bits mask */

    {"PSLL", itPsll,		OP2|GG|MODRM|MMXRM|I8,0,
    				0x0f, 0x70, 0xf0,	/* opcode */
				0xff, 0xfc, 0xf8},	/* mask */

    {"PSRA", itPsra,		OP2|GG|MODRM|MMXRM|I8,0,
    				0x0f, 0x70, 0xe0,	/* opcode */
				0xff, 0xfc, 0xf8},	/* mask */

    {"PSRL", itPsrl,		OP2|GG|MODRM|MMXRM|I8,0,
    				0x0f, 0x70, 0xd0,	/* opcode */
				0xff, 0xfc, 0xf8},	/* mask */

    /* 14 bits mask */

    {"PADD", itPadd,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xfc, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PADDS", itPadds,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xec, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PADDUS", itPaddus,	OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xdc, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PCMPEQ", itPcmpeq,	OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x74, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PCMPGT", itPcmpgt,	OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x64, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PSLL", itPsll,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xf0, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PSRA", itPsra,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xe0, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PSRL", itPsrl,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xd0, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PSUB", itPsub,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xf8, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PSUBS", itPsubs,		OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xe8, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PSUBUS", itPsubus,	OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0xd8, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PUNPCKH", itPunpckh,	OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x68, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},	/* mask */

    {"PUNPCKL", itPunpckl,	OP2|GG|MMXREG|MMXRM|MODRM,0,
    				0x0f, 0x60, 0x00,	/* opcode */
				0xff, 0xfc, 0x00},

    /* 13 bits mask */

    {"BSWAP", itBswap,       	OP1|MODRM,0,
    				0x0f, 0xc8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVB", itFcmovb,	OP2|ST,0,
    				ESC|0x02, 0xc0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVBE", itFcmovbe,	OP2|ST,0,
    				ESC|0x02, 0xd0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVE", itFcmove,	OP2|ST,0,
    				ESC|0x02, 0xc8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVU", itFcmovu,	OP2|ST,0,
    				ESC|0x02, 0xd8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVNB", itFcmovnb,	OP2|ST,0,
    				ESC|0x03, 0xc0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVNBE", itFcmovnbe,	OP2|ST,0,
    				ESC|0x03, 0xd0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVNE", itFcmovne,	OP2|ST,0,
    				ESC|0x03, 0xc8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCMOVNU", itFcmovnu,	OP2|ST,0,
    				ESC|0x03, 0xd8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCOM", itFcomST,		OP2|ST,0,
    				ESC|0x00, 0xd0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCOMI", itFcomi,		OP2|ST,0,
    				ESC|0x03, 0xf0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FCOMP", itFcompST,	OP2|ST,0,
    				ESC|0x00, 0xd8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FFREE", itFfree,		OP2|ST,0,
    				ESC|0x05, 0xc0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FLD", itFldST,		OP2|ST,0,
    				ESC|0x01, 0xc0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FST", itFstST,		OP2|ST,0,
    				ESC|0x05, 0xd0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FSTP", itFstpST,		OP2|ST,0,
    				ESC|0x05, 0xd8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FUCOM", itFucom,		OP2|ST,0,
    				ESC|0x05, 0xe0, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FUCOMP", itFucomp,	OP2|ST,0,
    				ESC|0x05, 0xe8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"FXCH", itFxch,		OP2|ST,0,
    				ESC|0x01, 0xc8, 0x00,	/* opcode */
				0xff, 0xf8, 0x00},	/* mask */

    {"POP", itPopS,      	OP2|SREG3,0,
    				0x0f, 0x81, 0x00,	/* opcode */
				0xff, 0xc7, 0x00},	/* mask */

    {"PUSH", itPushS,    	OP1|SREG3,0,
    				0x0f, 0x80, 0x00,	/* opcode */
				0xff, 0xc7, 0x00},	/* mask */

    /* 12 + 3 bits mask */

    {"CMOV", itCmovcc,		OP2|TTTN|MODRM|REG,0,
    				0x0f, 0x40, 0x00,	/* opcode */
				0xff, 0xf0, 0x00},	/* mask */

    {"CSET", itCset,     	OP2|TTTN|MODRM,0,
    				0x0f, 0x90, 0x00,	/* opcode */
				0xff, 0xf0, 0x38},	/* mask */

    /* 12 bits mask */

    {"CJMPF", itCjmp,    	OP2|TTTN|DIS,0,
    				0x0f, 0x80, 0x00,	/* opcode */
				0xff, 0xf0, 0x00},	/* mask */

    /* 11 bits mask */

    {"CALL", itCallRM,   	OP1|MODRM,0,
    				0xff, 0x10, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"CALL", itCallSegRM, 	OP1|MODRM,0,
    				0xff, 0x18, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FADD", itFaddST,		OP2|POP|FD|ST,0,
    				ESC|0x00, 0xc0, 0x00,	/* opcode */
				0xf9, 0xf8, 0x00},	/* mask */

    {"FLDbcd", itFldBCDM,	OP1|MODRM,0,
    				ESC|0x07, 0x20, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FLDCW", itFldcw,		OP1|MODRM,0,
    				ESC|0x01, 0x28, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FLDENV", itFldenv,	OP1|MODRM,0,
    				ESC|0x01, 0x20, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FLDext", itFldERM,	OP1|MODRM,0,
    				ESC|0x03, 0x28, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FLDint", itFldLIM,	OP1|MODRM,0,
    				ESC|0x07, 0x28, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FMUL", itFmulST,		OP2|POP|FD|ST,0,
    				ESC|0x00, 0xc8, 0x00,	/* opcode */
				0xf9, 0xf8, 0x00},	/* mask */

    {"FRSTOR", itFrstor,	OP1|MODRM,0,
    				ESC|0x05, 0x20, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSAVE", itFsave,		OP1|MODRM,0,
    				ESC|0x05, 0x30, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSTCW", itFstcw,		OP1|MODRM,0,
    				ESC|0x01, 0x38, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSTENV", itFstenv,	OP1|MODRM,0,
    				ESC|0x01, 0x30, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSTPbcd", itFstpBCDM,	OP1|MODRM,0,
    				ESC|0x07, 0x30, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSTPext", itFstpERM,	OP1|MODRM,0,
    				ESC|0x03, 0x38, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSTPint", itFstpLIM,	OP1|MODRM,0,
    				ESC|0x07, 0x38, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"FSTSW", itFstsw,		OP1|MODRM,0,
    				ESC|0x05, 0x38, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"IMUL", itImulAwiRM, 	OP1|WFUL|MODRM|AX,0,
    				0xf6, 0x28, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"JMP", itJmpRM,     	OP1|MODRM,0,
    				0xff, 0x20, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"JMP", itJmpSegRM,  	OP1|MODRM,0,
    				0xff, 0x28, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"POP", itPopRM,     	OP1|MODRM,0,
    				0x8f, 0x00, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    {"PUSH", itPushRM,   	OP1|MODRM,0,
    				0xff, 0x30, 0x00,	/* opcode */
				0xff, 0x38, 0x00},	/* mask */

    /* 10 bits mask */

    {"DEC", itDecRM,     	OP1|WFUL|MODRM,0,
    				0xfe, 0x08, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"DIV", itDiv,       	OP1|WFUL|MODRM|AX,0,
    				0xf6, 0x30, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"FDIV", itFdivST,		OP2|POP|FD|ST,0,
    				ESC|0x00, 0xf0, 0x00,	/* opcode */
				0xf9, 0xf0, 0x00},	/* mask */

    {"FSUB", itFsubST,		OP2|POP|FD|ST,0,
    				ESC|0x00, 0xe0, 0x00,	/* opcode */
				0xf9, 0xf0, 0x00},	/* mask */

    {"IDIV", itIdiv,     	OP1|WFUL|MODRM|AX,0,
    				0xf6, 0x38, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"INC", itIncRM,     	OP1|WFUL|MODRM,0,
    				0xfe, 0x00, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"MOV", itMovItoRM,  	OP1|WFUL|MODRM|IMM,0,
    				0xc6, 0x00, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"MUL", itMulAwiRM,  	OP1|WFUL|MODRM|AX,0,
    				0xf6, 0x20, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"NEG", itNeg,       	OP1|WFUL|MODRM,0,
    				0xf6, 0x18, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"NOT", itNot,       	OP1|WFUL|MODRM,0,
    				0xf6, 0x10, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    {"TEST", itTestIanRM, 	OP1|WFUL|MODRM|IMM,0,
    				0xf6, 0x00, 0x00,	/* opcode */
				0xfe, 0x38, 0x00},	/* mask */

    /* 9 bits mask */

    {"ADD", itAddItoRM,  	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x00, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"ADC", itAdcItoRM,  	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x10, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"AND", itAndItoRM,  	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x20, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"CMP", itCmpIwiRM,  	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x38, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"FADD", itFaddIRM,		OP1|MF|MODRM,0,	
    				ESC|0x00, 0x00, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"FCOM", itFcomIRM,		OP1|MF|MODRM,0,	
    				ESC|0x00, 0x10, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"FCOMP", itFcompIRM,	OP1|MF|MODRM,0,	
    				ESC|0x00, 0x18, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"FLD", itFldIRM,		OP1|MF|MODRM,0,	
    				ESC|0x01, 0x00, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"FMUL", itFmulIRM,		OP1|MF|MODRM,0,	
    				ESC|0x00, 0x08, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"FST", itFstIRM,		OP1|MF|MODRM,0,	
    				ESC|0x01, 0x10, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"FSTP", itFstpIRM,		OP1|MF|MODRM,0,	
    				ESC|0x01, 0x18, 0x00,	/* opcode */
				0xf9, 0x38, 0x00},	/* mask */

    {"OR", itOrItoRM,    	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x08, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"SBB", itSbbIfrRM,  	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x18, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"SUB", itSubIfrRM,  	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x28, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */

    {"XOR", itXorItoRM,   	OP1|SEXT|WFUL|MODRM|IMM,0,
    				0x80, 0x30, 0x00,	/* opcode */
				0xfc, 0x38, 0x00},	/* mask */


    /* 8 bits mask */

    {"AAA", itAaa,       	OP1,0,
    				0x37, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"AAS", itAas,       	OP1,0,
    				0x3f, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"ARPL", itArpl,     	OP1|MODRM|REG,0,
    				0x63, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"ASIZE", itAsize,   	OP1,0,
    				0x67, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"BOUND", itBound,   	OP1|MODRM|REG,0,
    				0x62, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CALL", itCall,     	OP1|DIS,0,
    				0xe8, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CALL", itCallSeg,  	OP1|OFFSEL,0,
    				0x9a, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CBW", itCbw,       	OP1,0,
    				0x98, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CLC", itClc,       	OP1,0,
    				0xf8, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CLD", itCld,       	OP1,0,
    				0xfc, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CLI", itCli,       	OP1,0,
    				0xfa, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CMC", itCmc,       	OP1,0,
    				0xf5, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CS", itCs,         	OP1,0,
    				0x2e, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"CWD", itCwd,       	OP1,0,
    				0x99, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"DAA", itDaa,       	OP1,0,
    				0x27, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"DAS", itDas,       	OP1,0,
    				0x2f, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"DS", itDs,         	OP1,0,
    				0x3e, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"ENTER", itEnter,   	OP1|D16L8,0,
    				0xc8, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"ES", itEs,         	OP1,0,
    				0x26, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"FDIV", itFdivIRM,		OP1|MF|MODRM,0,
    				ESC|0x00, 0x30, 0x00,	/* opcode */
				0xf9, 0x30, 0x00},	/* mask */

    {"FS", itFs,         	OP1,0,
    				0x64, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"FSUB", itFsubIRM,		OP1|MF|MODRM,0,
    				ESC|0x00, 0x20, 0x00,	/* opcode */
				0xf9, 0x30, 0x00},	/* mask */

    {"GS", itGs,         	OP1,0,
    				0x65, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"HLT", itHlt,       	OP1,0,
    				0xf4, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"INT", itInt,       	OP1|I8,0,
    				0xcd, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"INT", itInt3,      	OP1,0,
    				0xcc, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"INTO", itInto,     	OP1,0,
    				0xce, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"IRET", itIret,     	OP1,0,
    				0xcf, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"JECXZ", itJcxz,     	OP1|D8,0,
    				0xe3, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"JMP", itJmpS,      	OP1|D8,0,
    				0xeb, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"JMP", itJmpD,      	OP1|DIS,0,
    				0xe9, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"JMP", itJmpSeg,    	OP1|OFFSEL,0,
    				0xea, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LAHF", itLahf,     	OP1,0,
    				0x9f, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LDS", itLds,       	OP1|MODRM|REG,0,
    				0xc5, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LEA", itLea,       	OP1|MODRM|REG,0,
    				0x8d, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LEAVE", itLeave,   	OP1,0,
    				0xc9, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LES", itLes,       	OP1|MODRM|REG,0,
    				0xc4, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LOCK", itLock,     	OP1,0,
    				0xf0, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LOOP", itLoop,     	OP1|D8,0,
    				0xe2, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LOOPZ", itLoopz,   	OP1|D8,0,
    				0xe1, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"LOOPNZ", itLoopnz, 	OP1|D8,0,
    				0xe0, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"NOP", itNop,       	OP1,0,
    				0x90, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"OSIZE", itOsize,   	OP1,0,
    				0x66, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"POPA", itPopa,     	OP1,0,
    				0x61, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"POPF", itPopf,     	OP1,0,
    				0x9d, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"PUSHA", itPusha,   	OP1,0,
    				0x60, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"PUSHF", itPushf,   	OP1,0,
    				0x9c, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"REP", itRep,		OP1,0,
    				0xf3, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"REPNE", itRepNe,		OP1,0,
    				0xf2, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"RET", itRet,       	OP1,0,
    				0xc3, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"RET", itRetI,      	OP1|D16,0,
    				0xc2, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"RET", itRetSeg,    	OP1,0,
    				0xcb, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"RET", itRetSegI,   	OP1|D16,0,
    				0xca, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"SAHF", itSahf,     	OP1,0,
    				0x9e, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"SS", itSs,         	OP1,0,
    				0x36, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"STC", itStc,       	OP1,0,
    				0xf9, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"STD", itStd,       	OP1,0,
    				0xfd, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"STI", itSti,       	OP1,0,
    				0xfb, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"WAIT", itWait,     	OP1,0,
    				0x9b, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    {"XLAT", itXlat,     	OP1|WFUL,0,
    				0xd7, 0x00, 0x00,	/* opcode */
				0xff, 0x00, 0x00},	/* mask */

    /* 7 bits mask */

    {"ADC", itAdcItoA,   	SF|WFUL|IMM|AX,0,
    				0x14, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"ADD", itAddItoA,   	SF|WFUL|IMM|AX,0,
    				0x04, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"AND", itAndItoA,   	SF|WFUL|IMM|AX,0,
    				0x24, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"CMP", itCmpIwiA,   	SF|WFUL|IMM|AX,0,
    				0x3c, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"CMPS", itCmps,     	OP1|WFUL,0,
    				0xa6, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"IMUL", itImulRMwiI, 	OP1|SEXT|MODRM|REG|IMM,0,
    				0x69, 0x00, 0x00,	/* opcode */
				0xfd, 0x00, 0x00},	/* mask */

    {"IN", itInF,        	OP1|WFUL|PORT|AX,0,
    				0xe4, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"IN", itInV,        	OP1|WFUL|AX,0,
    				0xec, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"INS", itIns,       	OP1|WFUL,0,
    				0x6c, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"LODS", itLods,     	OP1|WFUL,0,
    				0xac, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"MOV", itMovRMtoS,  	OP1|DISR|MODRM|SREG3,0,
    				0x8c, 0x00, 0x00,	/* opcode */
				0xfd, 0x00, 0x00},	/* mask */

    {"MOVS", itMovs,     	OP1|WFUL,0,
    				0xa4, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"OR", itOrItoA,     	SF|WFUL|IMM|AX,0,
    				0x0c, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"OUT", itOutF,      	OP1|WFUL|PORT|AX,0,
    				0xe6, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"OUT", itOutV,      	OP1|WFUL|AX,0,
    				0xee, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"OUTS", itOuts,     	OP1|WFUL,0,
    				0x6e, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"PUSH", itPushI,    	OP1|SEXT|IMM,0,
    				0x68, 0x00, 0x00,	/* opcode */
				0xfd, 0x00, 0x00},	/* mask */

    {"SBB", itSbbIfrA,   	SF|WFUL|IMM|AX,0,
    				0x1c, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"SCAS", itScas,     	OP1|WFUL,0,
    				0xae, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"SHRO", itRolRMby1, 	OP1|WFUL|MODRM|TTT,0,
    				0xd0, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"SHRO", itRolRMbyCL, 	OP1|WFUL|MODRM|TTT|CL,0,
    				0xd2, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"SHRO", itRolRMbyI, 	OP1|WFUL|MODRM|TTT|I8,0,
    				0xc0, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"STOS", itStos,     	OP1|WFUL,0,
    				0xaa, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"SUB", itSubIfrA,   	SF|WFUL|IMM|AX,0,
    				0x2c, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"TEST", itTestRManR, 	OP1|WFUL|MODRM|REG,0,
    				0x84, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"TEST", itTestIanA, 	SF|WFUL|IMM|AX,0,
    				0xa8, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"XCHG", itXchgRM,   	OP1|WFUL|MODRM|REG,0,
    				0x86, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    {"XOR", itXorItoA,    	SF|WFUL|IMM|AX,0,
    				0x34, 0x00, 0x00,	/* opcode */
				0xfe, 0x00, 0x00},	/* mask */

    /* 6 bits mask */

    {"ADC", itAdcRMtoRM, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x10, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"ADD", itAddRMtoRM, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x00, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"AND", itAndRMtoRM, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x20, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"CMP", itCmpRMwiRM, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x38, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"MOV", itMovRMtoMR, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x88, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"MOV", itMovAMtoMA, 	SF|DISR|WFUL|DIS|AX,0,	
    				0xa0, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"OR", itOrRMtoRM,   	OP1|DISR|WFUL|MODRM|REG,0,
    				0x08, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"POP", itPopS,      	OP1|REG,0,
    				0x07, 0x00, 0x00,	/* opcode */
				0xe7, 0x00, 0x00},	/* mask */

    {"PUSH", itPushS,    	OP1|SREG2,0,
    				0x06, 0x00, 0x00,	/* opcode */
				0xe7, 0x00, 0x00},	/* mask */

    {"SBB", itSbbRMfrRM, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x18, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"SUB", itSubRMfrRM, 	OP1|DISR|WFUL|MODRM|REG,0,
    				0x28, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    {"XOR", itXorRMtoRM,  	OP1|DISR|WFUL|MODRM|REG,0,
    				0x30, 0x00, 0x00,	/* opcode */
				0xfc, 0x00, 0x00},	/* mask */

    /* 5 bits mask */

    {"DEC", itDecR,      	SF|REG,0,
    				0x48, 0x00, 0x00,	/* opcode */
				0xf8, 0x00, 0x00},	/* mask */

    {"INC", itIncR,      	SF|REG,0,
    				0x40, 0x00, 0x00,	/* opcode */
				0xf8, 0x00, 0x00},	/* mask */

    {"POP", itPopR,      	SF|REG,0,
    				0x58, 0x00, 0x00,	/* opcode */
				0xf8, 0x00, 0x00},	/* mask */

    {"PUSH", itPushR,    	SF|REG,0,
    				0x50, 0x00, 0x00,	/* opcode */
				0xf8, 0x00, 0x00},	/* mask */

    {"XCHG", itXchgA,    	SF|REG|AX,0,
    				0x90, 0x00, 0x00,	/* opcode */
				0xf8, 0x00, 0x00},	/* mask */

    /* 4 bits mask */

    {"CJMPS", itCjmp,    	OP1|TTTN|D8,0,
    				0x70, 0x00, 0x00,	/* opcode */
				0xf0, 0x00, 0x00},	/* mask */

    {"MOV", itMovItoR,   	SF|WFUL|REG|IMM,0,
    				0xb0, 0x00, 0x00,	/* opcode */
				0xf0, 0x00, 0x00},	/* mask */


    {NULL, 0,			0,0,
    				0x00, 0x00, 0x00,	/* opcode */
				0x00, 0x00, 0x00},	/* mask */

    };

/* reg[d32=0,1][reg field=0 - 7] */

LOCAL char *reg[2][8] = 
    {
    {"AX",  "CX",  "DX",  "BX",  "SP",  "BP",  "SI",  "DI"},
    {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"},
    };

/* regw[d32=0,1][w bit=0,1][reg field=0 - 7] */

LOCAL char *regw[2][2][8] = 
    {
    {{"AL",  "CL",  "DL",  "BL",  "AH",  "CH",  "DH",  "BH"},
     {"AX",  "CX",  "DX",  "BX",  "SP",  "BP",  "SI",  "DI"}},
    {{"AL",  "CL",  "DL",  "BL",  "AH",  "CH",  "DH",  "BH"},
     {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"}},
    };

LOCAL char * regmmx[8] =	/* MMX registers */
    {
    "MM0", "MM1", "MM2", "MM3", "MM4", "MM5", "MM6", "MM7"
    };
    
LOCAL char * regxmm[8] =	/* XMM registers */
    {
    "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7"
    };    

LOCAL char * gg[4] =		/* MMX instructions packed granularities */
    {
    "B",			/* packed bytes                          */
    "W",			/* packed words                          */
    "D",			/* packed double words                   */
    "Q"				/* packed quad words                     */
    };

/* immL[d32=0,1][w bit=0,1] */

LOCAL char immL[2][2] = { {1,2},{1,4} };

/* segment registers, sreg3[sreg3=0-7] */

LOCAL char *sreg3[8] = { "ES","CS","SS","DS","FS","GS","--","--" };

/* segment registers, sreg2[sreg2=0-3] */

LOCAL char *sreg2[4] = { "ES","CS","SS","DS" };

/* shift rotate opecodes, ttt[ttt=0-7] */

LOCAL char *ttt[8] = { "ROL","ROR","RCL","RCR","SHL","SHR","---","SAR" };

/* conditional jump opecodes, tttn[tttn=0-15] */

LOCAL char *tttn[16] = 
    { 
    "O","NO","B","NB","E","NE","BE","NBE",
    "S","NS","P","NP","L","NL","LE","NLE"
    };

/* control registers, */

LOCAL char *eeec[8] = { "CR0","---","CR2","CR3","CR4","---","---","---" };

/* debug registers, */

LOCAL char *eeed[8] = { "DR0","DR1","DR2","DR3","DR4","DR5","DR6","DR7" };

/* test registers, */

LOCAL char *eeet[8] = { "---","---","---","---","---","---","TR6","TR7" };

/* modrm format */

LOCAL char *modrm[2][3][8] = 
    {
    {{"[BX+SI]",		"[BX+DI]",
      "[BP+SI]",		"[BP+DI]",
      "[SI]",			"[DI]",
      "0x%04x",			"[BX]"},
     {"[BX+SI+%d]",		"[BX+DI+%d]", 
      "[BX+SI+%d]",		"[BX+DI+%d]", 
      "[SI+%d]",		"[DI+%d]", 
      "[BP+%d]",		"[BX+%d]"},
     {"[BX+SI+%d]",		"[BX+DI+%d]", 
      "[BX+SI+%d]",		"[BX+DI+%d]",
      "[SI+%d]",		"[DI+%d]", 
      "[BP+%d]",		"[BX+%d]"}},
    {{"[EAX]",			"[ECX]", 
      "[EDX]",			"[EBX]",
      "sib",			"0x%08x", 
      "[ESI]",			"[EDI]"},
     {"[EAX+%d]",		"[ECX+%d]", 
      "[EDX+%d]",		"[EBX+%d]",
      "sib",			"[EBP+%d]", 
      "[ESI+%d]",		"[EDI+%d]"},
     {"[EAX+%d]",		"[ECX+%d]", 
      "[EDX+%d]",		"[EBX+%d]",
      "sib",			"[EBP+%d]", 
      "[ESI+%d]",		"[EDI+%d]"}}
    };

/* sib format */

LOCAL char *sib[3][8] = 
    {
    {"[EAX+(%3s%2s)]",		"[ECX+(%3s%2s)]", 
     "[EDX+(%3s%2s)]",		"[EBX+(%3s%2s)]",
     "[ESP+(%3s%2s)]",		"[0x%08x+(%3s%2s)]",
     "[ESI+(%3s%2s)]",		"[EDI+(%3s%2s)]"},
    {"[EAX+(%3s%2s)+%d]",	"[ECX+(%3s%2s)+%d]",
     "[EDX+(%3s%2s)+%d]",	"[EBX+(%3s%2s)+%d]",
     "[ESP+(%3s%2s)+%d]",	"[EBP+(%3s%2s)+%d]",
     "[ESI+(%3s%2s)+%d]",	"[EDI+(%3s%2s)+%d]"},
    {"[EAX+(%3s%2s)+%d]",	"[ECX+(%3s%2s)+%d]",
     "[EDX+(%3s%2s)+%d]",	"[EBX+(%3s%2s)+%d]",
     "[ESP+(%3s%2s)+%d]",	"[EBP+(%3s%2s)+%d]",
     "[ESI+(%3s%2s)+%d]",	"[EDI+(%3s%2s)+%d]"}
    };

/* scale */

LOCAL char *scale[4] = { "x1","x2","x4","x8" };

/* indexR */

LOCAL char *indexR[8] = { "EAX","ECX","EDX","EBX","no-","EBP","ESI","EDI" };

/* st */

LOCAL char *st = "ST(%d)";

/* mf */

LOCAL char *mf[4] = { "r32", "---", "r64", "---" };

/* CMPSS variations */
LOCAL char *cmpss[8] = { "CMPEQSS",            /* imm = 0 */
                         "CMPLTSS",            /* imm = 1 */
                         "CMPLESS",            /* imm = 2 */
                         "CMPUNORDSS",         /* imm = 3 */
                         "CMPNEQSS",           /* imm = 4 */
                         "CMPNLTSS",           /* imm = 5 */
                         "CMPNLESS",           /* imm = 6 */
                         "CMPORDSS" };         /* imm = 7 */

LOCAL char *cmpps[8] = { "CMPEQPS",            /* imm = 0 */
                         "CMPLTPS",            /* imm = 1 */
                         "CMPLEPS",            /* imm = 2 */
                         "CMPUNORDPS",         /* imm = 3 */
                         "CMPNEQPS",           /* imm = 4 */
                         "CMPNLTPS",           /* imm = 5 */
                         "CMPNLEPS",           /* imm = 6 */
                         "CMPORDPS" };         /* imm = 7 */

LOCAL int prefixAsize = 0;		/* prefix Address Size, 0x67 */
LOCAL int prefixOsize = 0;		/* prefix Operand Size, 0x66 */

int dsmAsize = 1;			/* 32 bits address size */
int dsmDsize = 1;			/* 32 bits data size */
int dsmDebug = 0;			/* debug flag */

char * instKeeper = NULL;		/* keeps instruction string */

/* forward declarations */

LOCAL void dsmPass1 (FORMAT_X *pX, FORMAT_Y *pY);
LOCAL void dsmPass2 (FORMAT_X *pX, FORMAT_Y *pY);

/*******************************************************************************
*
* dsmFind - disassemble one instruction
*
* This routine figures out which instruction is pointed to by pD.
*
* RETURNS: N/A
*/

LOCAL void dsmFind
    (
    FAST UCHAR *pD,		/* Pointer to the instruction	*/
    FORMAT_X *pX,		/* Pointer to the FORMAT_X	*/
    FORMAT_Y *pY 		/* Pointer to the FORMAT_Y	*/
    )
    {
    INST *pI;
    
    pX->pD = (char *)pD;

    if (prefixAsize)
        pX->a32 = ~dsmAsize & 0x1;
    else
        pX->a32 = dsmAsize;

    if (prefixOsize)
        pX->d32 = ~dsmDsize & 0x1;
    else
        pX->d32 = dsmDsize;

    pX->w = 1;

    for (pI = &inst[0]; pI->mask0 != 0; pI++)
        {
        if ((pI->op0 == (*pD & pI->mask0)) &&
    	    (pI->op1 == (*(pD + 1) & pI->mask1)) &&
    	    (pI->op2 == (*(pD + 2) & pI->mask2)))
    	    {
	    break;
    	    }
	}
    
    if (pI->mask0 == 0)
	{
        errnoSet (S_dsmLib_UNKNOWN_INSTRUCTION);
	if (dsmDebug)
            printf ("unknown instruction.  pD = 0x%x %x %x %x\n",(UCHAR)*pD,
                (UCHAR)*(pD + 1), (UCHAR)*(pD + 2), (UCHAR)*(pD + 3));
        return;
	}
    
    pX->pI = pI;
    
    dsmPass1 (pX, pY);

    if (pX->pI->type == itAsize)	/* set prefixAsize for next inst. */
	prefixAsize = 1;
    else
	prefixAsize = 0;

    if (pX->pI->type == itOsize)	/* set prefixOsize for next inst. */
	prefixOsize = 1;
    else
	prefixOsize = 0;

    if (dsmDebug)
	{
        printf ("FORMAT_X INST   pOpc = %s\n", pX->pI->pOpc);
        printf ("                type = 0x%x\n", pX->pI->type);
        printf ("                flag = 0x%x\n", pX->pI->flag);
        printf ("        pD = 0x%x %x %x %x\n", (UCHAR)*pX->pD,
            (UCHAR)*(pX->pD + 1), (UCHAR)*(pX->pD + 2), (UCHAR)*(pX->pD + 3));
        printf ("        lenO, lenD, lenI = %d, %d, %d\n", pX->lenO,
            pX->lenD, pX->lenI);
        printf ("        d32, a32 = %d, %d\n", pX->d32, pX->a32);
        printf ("        modrm, sib = %d, %d\n", pX->modrm, pX->sib);
        printf ("        w s d = %d %d %d\n", pX->w, pX->s, pX->d);
        printf ("        mod reg rm = %d %d %d\n", pX->mod, pX->reg, pX->rm);
        printf ("        ss index base = %d %d %d\n", pX->ss, pX->index,
	    pX->base);
	}
    
    dsmPass2 (pX, pY);
    
    if (dsmDebug)
	{
        printf ("FORMAT_Y len = %d\n", pY->len);
        printf ("        obuf = %s\n", pY->obuf);
        printf ("        rbuf = %s\n", pY->rbuf);
        printf ("        mbuf = %s\n", pY->mbuf);
        printf ("        ibuf = %s\n", pY->ibuf);
        printf ("        dbuf = %s\n", pY->dbuf);
	}

    return;
    }

/*******************************************************************************
*
* dsmPass1 - fill FORMAT_X structure.
*
* RETURNS: N/A
*/

LOCAL void dsmPass1
    (
    FORMAT_X *pX,
    FORMAT_Y *pY 
    )
    {
    char *pS;

    /* pX->lenO = size of Opcode in bytes = 1, 2, or 3 */

    if (pX->pI->flag & OP3)
        pX->lenO = 3;
    else if (pX->pI->flag & OP2) 
	pX->lenO = 2;
    else
	pX->lenO = 1;

    /* get a Opecode */

    if (pX->pI->flag & TTTN)
	{
	instKeeper = pX->pI->pOpc;
	pS = tttn[*(pX->pD + pX->lenO - 1) & 0x0f];
	if (pX->pI->type == itCjmp)
	    {
	    bcopy ("J  ", pY->obuf, 3);
	    bcopy (pS, &pY->obuf[1], strlen (pS));
	    }
	if (pX->pI->type == itCset)
	    {
	    bcopy ("SET", pY->obuf, 3);
	    bcopy (pS, &pY->obuf[3], strlen (pS));
	    }
	if (pX->pI->type == itCmovcc)
	    {
	    bcopy ("CMOV", pY->obuf, 4);
	    bcopy (pS, &pY->obuf[4], strlen (pS));
	    }
	pX->pI->pOpc = pY->obuf;
	}

    if (pX->pI->flag & TTT)
	pX->pI->pOpc = ttt[(*(pX->pD + pX->lenO) & 0x38) >> 3];

    /* get an MMX granularity */

    if (pX->pI->flag & GG)
	{
	instKeeper = pX->pI->pOpc;
	pS = gg[*(pX->pD + pX->lenO - 1) & 0x03];
	bcopy (pX->pI->pOpc, pY->obuf, strlen (pX->pI->pOpc));
	bcopy (pS, &pY->obuf[strlen (pX->pI->pOpc)], strlen (pS));
	pX->pI->pOpc = pY->obuf;
	}

    /* get a W */

    if (pX->pI->flag & WFUL)
	{
	pX->w = *(pX->pD + pX->lenO - 1) & 0x01;
	if (pX->pI->type == itMovItoR)
	    pX->w = (*pX->pD & 0x08) >> 3;
	}

    /* get a S */

    if (pX->pI->flag & SEXT)
	pX->s = (*(pX->pD + pX->lenO - 1) & 0x02) >> 1;

    /* 
     * get a D 
     *
     * jhw - for our representation of data direction:
     *           pX->d = 0 is an r/m -> reg transfer 
     *           pX->d = 1 is a  reg -> r/m transfer
     */

    if (pX->pI->flag & DISR)
        {

        /* 
	 * MMX instructions reference least significant opcode byte bit 4 for 
         * data direction information.
         *
         * for Intel's representation:
         *    D bit = 0 is an r/m -> reg transfer 
         *    D bit = 1 is a  reg -> r/m transfer 
         */
						               
        if (pX->pI->flag & (MMXREG|MMXRM))
            pX->d = (*(pX->pD + pX->lenO - 1) & 0x10) >> 4;

        /* 
	 * XMM instructions reference least significant opcode byte bit 0 for 
         * data direction information.
         *
         * for Intel's representation:
         *    D bit = 0 is an r/m -> reg transfer 
         *    D bit = 1 is a  reg -> r/m transfer 
         */
      
        else if (pX->pI->flag2 & (XMMREG|XMMRM))
             pX->d = (*(pX->pD + pX->lenO - 1) & 0x01);

        /* 
	 * all others use least significant byte bit 1 for 
         * data direction information.
         * 
         * BUT... all other instructions use the opposite representation!
         *
	 * for Intel's representation:
         *    D bit = 1 is an r/m -> reg transfer 
         *    D bit = 0 is a  reg -> r/m transfer 
         */

        else
            /* invert the state */
            pX->d = (*(pX->pD + pX->lenO - 1) & 0x02) ? 0 : 1;

	/* evaluate the special case instructions! */

	/*
	 * PEXTRW is a SIMD instruction, but it operates on MMX
	 * register. Therefore it deviates from the above rules.
	 * The DISR flag is included in the definition because
	 * the data dir is reg -> r/m regardless.
	 */

	if (pX->pI->type == itPextrw)
            pX->d = 1;
	}

    /* get a REG */

    if (pX->pI->flag & SREG2)
        pX->reg = (*pX->pD & 0x18) >> 3;
    if (pX->pI->flag & SREG3)
        pX->reg = (*(pX->pD + pX->lenO) & 0x38) >> 3;
    if ( (pX->pI->flag & REG) || 
    	 (pX->pI->flag & MMXREG) ||
	 (pX->pI->flag2 & XMMREG))    
	{
	if (pX->pI->flag & SF)
	    pX->reg = *pX->pD & 0x07;
	else if (pX->pI->flag & MODRM)
	    pX->reg = (*(pX->pD + pX->lenO) & 0x38) >> 3;
        else
            {
            printf ("dsmLib.c error 0: Invalid opcode flag definition.\n");
            printf ("\top = 0x%02x 0x%02x 0x%02x 0x%02x\n",
                    (UCHAR)*pX->pD, (UCHAR)*(pX->pD + 1),
                    (UCHAR)*(pX->pD + 2), (UCHAR)*(pX->pD + 3));
            }
	}
    if (pX->pI->flag & EEE)
        pX->reg = (*(pX->pD + pX->lenO) & 0x38) >> 3;

    /* get a ST for 387*/

    if (pX->pI->flag & ST)
	pX->st = *(pX->pD + pX->lenO - 1) & 0x07;

    /* get a MF for 387*/

    if (pX->pI->flag & MF)
	pX->mf = (*pX->pD & 0x06) >> 1;

    /* get a FD for 387 */

    if (pX->pI->flag & FD)
	pX->fd = *pX->pD & 0x04;

    /* get a size of Immediate, 0, 1, 2, 4 */

    if (pX->pI->flag & I8)
	pX->lenI = 1;

    if (pX->pI->flag & IMM)
	{
	if (pX->s)
	    pX->lenI = 1;
	else
	    pX->lenI = immL[(int)pX->d32][(int)pX->w];
	}

    if (pX->pI->flag & OFFSEL)
	{

	/* 
	 * CALL/JMP ptr16:16/32
	 * The operand size attribute determines the size of offset (16/32).
	 * The operand size attribute is the D flag in the segment desc.
	 * The instruction prefix 0x66 can be used to select an operand
	 * size other than the default.
	 */

	if (pX->d32)
	    pX->lenI = 4;
	else
	    pX->lenI = 2;
	}

    if ((pX->pI->flag & D16L8) || (pX->pI->flag & PORT))
	pX->lenI = 2;

    /* get a size of Displacement, 0, 1, 2, 4 */

    if (pX->pI->flag & D8)
	pX->lenD = 1;

    if (pX->pI->flag & (DIS|D16))
	{
	if (pX->pI->flag & WFUL)
	    {
	    if (pX->pI->type == itMovAMtoMA)
		pX->lenD = immL[(int)pX->a32][(int)pX->w];
	    else
		pX->lenD = immL[(int)pX->d32][(int)pX->w];
	    }
	else
	    pX->lenD = immL[(int)pX->d32][(int)pX->w];
	}

    if (pX->pI->flag & OFFSEL)
	pX->lenD = 2;

    if (pX->pI->flag & D16L8)
	pX->lenD = 1;

    if (pX->pI->flag & MODRM)
	{
	pX->modrm = 1;
	pY->pD = pX->pD + pX->lenO;
	pX->mod = (*pY->pD & 0xc0) >> 6;
	pX->rm = *pY->pD & 0x07;
	if ((pX->a32 == 0) && (pX->mod != 3))
	    {
	    if (pX->mod == 1)
		pX->lenD = 1;
	    else if ((pX->mod == 2) || ((pX->mod == 0) && (pX->rm == 6)))
		pX->lenD = 2;
	    }
	if ((pX->a32 == 1) && (pX->mod != 3))
	    {
	    if (pX->rm == 4)
		{
	        pX->sib = 1;
	        pY->pD = pX->pD + pX->lenO + pX->modrm;
	        pX->ss = (*pY->pD & 0xc0) >> 6;
	        pX->index = (*pY->pD & 0x38) >> 3;
	        pX->base = *pY->pD & 0x07;
	        if (pX->mod == 1)
		    pX->lenD = 1;
	        else if ((pX->mod == 2) || ((pX->mod == 0) && (pX->base == 5)))
		    pX->lenD = 4;
		}
	    else
		{
	        if (pX->mod == 1)
		    pX->lenD = 1;
	        else if ((pX->mod == 2) || ((pX->mod == 0) && (pX->rm == 5)))
		    pX->lenD = 4;
		}
	    }
	}
    }
    
/*******************************************************************************
*
* dsmPass2 - fill FORMAT_Y structure.
*
* RETURNS: N/A
*/

LOCAL void dsmPass2
    (
    FORMAT_X *pX,
    FORMAT_Y *pY 
    )
    {
    FAST char *pS = 0;

    /* get an instruction length, pY->len */

    pY->len = pX->lenO + pX->modrm + pX->sib + pX->lenD + pX->lenI;

    /* get an opecode pointer, pY->pOpc */

    pY->pOpc = pX->pI->pOpc;

    if (pX->pI->flag & MF)
        {
        pS = pY->obuf;
        if (pX->mf & 1)
	    {
	    bcopy (pX->pI->pOpc, pS + 1, strlen (pX->pI->pOpc));
	    bcopy ("FI", pS, 2);
	    }
        else
	    {
	    bcopy (pX->pI->pOpc, pS, strlen (pX->pI->pOpc));
	    strcat (pS, mf[(int)pX->mf]);
	    }
        pY->pOpc = pY->obuf;
        }

    if (pX->pI->flag & POP)
        {
        bcopy (pX->pI->pOpc, pY->obuf, strlen (pX->pI->pOpc));
        strcat (pY->obuf, "P");
        pY->pOpc = pY->obuf;
        }

    /* get a register operand buffer, pY->rbuf */

    if (pX->pI->flag & SREG2)
        bcopy (sreg2[(int)pX->reg], pY->rbuf, strlen (sreg2[(int)pX->reg]));

    if (pX->pI->flag & SREG3)
        bcopy (sreg3[(int)pX->reg], pY->rbuf, strlen (sreg3[(int)pX->reg]));

    /* get register number */

    if (pX->pI->flag & REG)
        {
        if (pX->pI->flag & WFUL)
	    pS = regw[(int)pX->d32][(int)pX->w][(int)pX->reg];
        else
	    pS = reg[(int)pX->d32][(int)pX->reg];
        bcopy (pS, pY->rbuf, strlen (pS));
        }

    /* get MMX register number */

    if (pX->pI->flag & MMXREG)
	{
	pS = regmmx[ (int) pX->reg];
	bcopy (pS, pY->rbuf, strlen (pS));
	}

    /* XMM register */

    if (pX->pI->flag2 & XMMREG)
        {
	pS = regxmm[ (int) pX->reg];
	memcpy ((void *) pY->rbuf, (void *) pS, strlen (pS));
        }


    if (pX->pI->flag & EEE)
        {
        if (pX->pI->type == itMovC)
            pS = eeec[(int)pX->reg];
        else if (pX->pI->type == itMovD)
            pS = eeed[(int)pX->reg];
        else if (pX->pI->type == itMovT)
            pS = eeet[(int)pX->reg];
        bcopy (pS, pY->rbuf, strlen (pS));
        pS = reg[(int)pX->d32][(int)pX->rm];
        bcopy (pS, pY->mbuf, strlen (pS));
        }

    if (pX->pI->flag & AX)
        {
        if (pX->pI->flag & WFUL)
	    {
	    if (pX->pI->type == itMovAMtoMA)
	        pS = regw[(int)pX->a32][(int)pX->w][0];
	    else
	        pS = regw[(int)pX->d32][(int)pX->w][0];
	    }
        else
	    pS = reg[(int)pX->d32][0];

        if (pX->pI->flag & REG)
            bcopy (pS, pY->ibuf, strlen (pS));
	else
            bcopy (pS, pY->rbuf, strlen (pS));
        }

    if (pX->pI->flag & ST)
        sprintf (pY->rbuf, st, pX->st);
    
    /* get a displacement operand buffer, pY->dbuf */

    if (pX->pI->flag & (D8|D16|DIS))
        {
        pY->pD = pX->pD + pX->lenO + pX->modrm + pX->sib;
        if (pX->lenD == 1)
	    pY->addr = *(pY->pD);
        else if (pX->lenD == 2)
	    pY->addr = *(short *)pY->pD;
        else if (pX->lenD == 4)
	    {
	    if (pX->pI->flag & D16)
	        pY->addr = *(int *)pY->pD & 0x0000ffff;
	    else
	        pY->addr = *(int *)pY->pD;
	    }
	sprintf (pY->dbuf, "0x%x", pY->addr);
        }

    if (pX->pI->flag & OFFSEL)
        sprintf (pY->dbuf, "0x%x", *(USHORT *)(pX->pD + pX->lenO + pX->lenI));
    
    if (pX->pI->flag & D16L8)
        sprintf (pY->dbuf, "0x%x", *(UCHAR *)(pX->pD + pX->lenO + pX->lenI));

    /* get an immediate operand buffer, pY->ibuf */

    if (pX->pI->flag & (IMM|I8))
        {
        pY->pD = pX->pD + pX->lenO + pX->modrm + pX->sib + pX->lenD;
        if (pX->lenI == 1)
	    {
	    if (pX->s)
	        sprintf (pY->ibuf, "%d", *pY->pD);
	    else
	        sprintf (pY->ibuf, "0x%x", *(UCHAR *)pY->pD);
	    }
        if (pX->lenI == 2)
	    sprintf (pY->ibuf, "0x%x", *(USHORT *)pY->pD);
        if (pX->lenI == 4)
	    sprintf (pY->ibuf, "0x%x", *(UINT *)pY->pD);
       
        /* 
         * CMPSS and CMPPS opcode string is modified based on 
	 * the imm value.
	 */

	if ((pX->pI->type == itCmpps) || (pX->pI->type == itCmpss))
            {

            /* verify that (0 < imm < 8) */

            if (*(UCHAR *)pY->pD < 8)
                {
                if (pX->pI->type == itCmpps)
                    pY->pOpc = cmpps[*(UCHAR *)pY->pD];
                else 
                    pY->pOpc = cmpss[*(UCHAR *)pY->pD];
                }
            }
	}

    if (pX->pI->flag & PORT)
        sprintf (pY->ibuf, "0x%04x", *(USHORT *)(pX->pD + pX->lenO));

    if (pX->pI->flag & OFFSEL)
        {
        if (pX->lenI == 2)
            sprintf (pY->ibuf, "0x%x", *(USHORT *)(pX->pD + pX->lenO));
        else
            sprintf (pY->ibuf, "0x%x", *(UINT *)(pX->pD + pX->lenO));
        }

    if (pX->pI->flag & D16L8)
        sprintf (pY->ibuf, "0x%x", *(USHORT *)(pX->pD + pX->lenO));

    if (pX->pI->type == itRolRMby1)		
        sprintf (pY->ibuf, "0x1");

    /* get a memory operand buffer, pY->mbuf */

    if (pX->modrm) 
	{
	if (pX->mod == 3)
            {
	    if (pX->pI->flag & WFUL)
		pS = regw[(int)pX->d32][(int)pX->w][(int)pX->rm];
	    else if (pX->pI->flag & MMXRM)
		pS = regmmx[(int)pX->rm];
	    else if (pX->pI->flag2 & XMMRM)
		pS = regxmm[(int)pX->rm];
	    else				/* REGRM defaults to here */
	        pS = reg[(int)pX->d32][(int)pX->rm];
            bcopy (pS, pY->mbuf, strlen (pS));
            }
	else
	    {
	    if (pX->a32 == 0)
                {
                pY->pD = pX->pD + pX->lenO + pX->modrm;
	        pS = modrm[(int)pX->a32][(int)pX->mod][(int)pX->rm];
                if (pX->mod == 0)
	            {
	            if (pX->rm == 6)
	                sprintf (pY->mbuf, pS, *(USHORT *)pY->pD);
		        /* see 01e, pY->addr = *(USHORT *)pY->pD; */
	            else
	                sprintf (pY->mbuf, pS);
	            }
                else if (pX->mod == 1)
	            sprintf (pY->mbuf, pS, *pY->pD);
                else if (pX->mod == 2)
	            sprintf (pY->mbuf, pS, *(USHORT *)pY->pD);
                }
	    else
                {
                pY->pD = pX->pD + pX->lenO + pX->modrm;
	        pS = modrm[(int)pX->a32][(int)pX->mod][(int)pX->rm];
                if ((pX->sib) && (pX->rm == 4))
	            {
                    pY->pD += pX->sib;
	            pS = sib[(int)pX->mod][(int)pX->base];
	            if (pX->mod == 0)
		        {
		        if (pX->base == 5)
	                    sprintf (pY->mbuf, pS, *(int *)pY->pD, 
				     indexR[(int)pX->index],
				     scale[(int)pX->ss]);
	                else
	                    sprintf (pY->mbuf, pS, indexR[(int)pX->index], 
				     scale[(int)pX->ss]);
		        }
	            else if (pX->mod == 1)
	                sprintf (pY->mbuf, pS, indexR[(int)pX->index],
				 scale[(int)pX->ss], *pY->pD);
	            else if (pX->mod == 2)
	                sprintf (pY->mbuf, pS, indexR[(int)pX->index],
				 scale[(int)pX->ss], *(int *)pY->pD);
	            }
                else 
	            {
                    if (pX->mod == 0)
	                {
                        if (pX->rm == 5)
	                    sprintf (pY->mbuf, pS, *(int *)pY->pD);
		            /* see 01e, pY->addr = *(int *)pY->pD; */
	                else
	                    sprintf (pY->mbuf, pS);
	                }
                    else if (pX->mod == 1)
	                sprintf (pY->mbuf, pS, *pY->pD);
	            else if (pX->mod == 2)
	                sprintf (pY->mbuf, pS, *(int *)pY->pD);
	            }
                }
            }
        }
    }

/*******************************************************************************
*
* dsmPrint - print FORMAT_Y structure.
*
* RETUERNS: N/A
*/

LOCAL void dsmPrint
    (
    FORMAT_X *pX,		/* Pointer to the FORMAT_X	*/
    FORMAT_Y *pY,		/* Pointer to the FORMAT_Y	*/
    VOIDFUNCPTR prtAddress 	/* Address printing function	*/
    )
    {
    int flag;
    char *pS;
    char *pD = pX->pD;
    int bytesToPrint;
    int ix;

    if (pY->len == 0)
        pY->len = 1;

    bytesToPrint = (((pY->len - 1) >> 3) + 1) << 3;

    /* print out an address */

    printf ("0x%08x  ", (UINT)pX->pD);

    /* print out a data */

    for (ix=0; ix < bytesToPrint; ix++)
	{
	if ((ix & ~0x07) && ((ix & 0x07) == 0))
	    printf ("\n          ");
	printf ((ix < pY->len) ? "%02x " : "   ", (UCHAR)*pD++);
	}

    /* print out the unknown instruction */

    if (pX->pI == NULL)
	{
	printf (".BYTE          0x%02x\n", (UCHAR)*pX->pD);
	return;
	}

    /* set the operand pointers */

    flag = pX->pI->flag & 0xfffff;

    switch (flag)
        {
        case REG:
        case SREG3:
        case SREG2:
        case ST:
            pY->pOpr0 = pY->rbuf;
	    break;

        case MODRM:
            pY->pOpr0 = pY->mbuf;
	    if (pX->pI->type == itRolRMby1)
		pY->pOpr1 = pY->ibuf;
	    break;

        case I8:
        case IMM:
            pY->pOpr0 = pY->ibuf;
	    break;

        case D8:
        case DIS:
            pY->pOpr0 = (char *)&pY->addr;
	    break;

        case D16:
            pY->pOpr0 = pY->dbuf;
	    break;

        case (REG|IMM):
            pY->pOpr0 = pY->rbuf;
            pY->pOpr1 = pY->ibuf;
	    break;
        
        case (MODRM|IMM):
        case (MODRM|I8):
	case (MODRM|MMXRM|I8):
            if ((pX->pI->type == itCmpps) || (pX->pI->type == itCmpss))
                {
                pY->pOpr0 = pY->rbuf;
                pY->pOpr1 = pY->mbuf;
                }
            else if (pX->pI->flag2 & XMMREG)
                {
                pY->pOpr0 = pY->mbuf;
                pY->pOpr1 = pY->rbuf;
                pY->pOpr2 = pY->ibuf;
                }
            else
		{
		pY->pOpr0 = pY->mbuf;
		pY->pOpr1 = pY->ibuf;
		}
	    break;
        
        case (MODRM|REG):
        case (MODRM|REGRM):
        case (MODRM|SREG3):
        case (MODRM|EEE):
        case (MODRM|MMXREG):
        case (MODRM|MMXREG|REGRM):
        case (MODRM|MMXREG|MMXRM):
        case (MODRM|MMXRM):
            if (pX->d)
            {
                pY->pOpr0 = pY->mbuf;	/* data dir = reg to r/m */
                pY->pOpr1 = pY->rbuf;
            }
            else
            {
                pY->pOpr0 = pY->rbuf;	/* data dir = r/m to reg */
                pY->pOpr1 = pY->mbuf;
            }
	    break;

        case (MODRM|REG|I8):
	case (MODRM|MMXRM|REG|I8):
        case (MODRM|MMXREG|REGRM|I8):
        case (MODRM|MMXREG|MMXRM|I8):
        case (MODRM|REG|IMM):
            if ((pX->pI->type == itShldRMbyI) || (pX->pI->type == itShrdRMbyI))
	        {
	        pY->pOpr0 = pY->mbuf;
	        pY->pOpr1 = pY->rbuf;
	        pY->pOpr2 = pY->ibuf;
	        }
            else if (pX->d)
	        {
	        pY->pOpr0 = pY->mbuf;
	        pY->pOpr1 = pY->rbuf;
	        pY->pOpr2 = pY->ibuf;
		}
            else
	        {
	        pY->pOpr0 = pY->rbuf;
	        pY->pOpr1 = pY->mbuf;
	        pY->pOpr2 = pY->ibuf;
	        }
	    break;

        case (MODRM|REG|CL):
            pY->pOpr0 = pY->mbuf;
            pY->pOpr1 = pY->rbuf;
            pY->pOpr2 = "CL";
	    break;

        case (IMM|AX):
            pY->pOpr0 = pY->rbuf;
            pY->pOpr1 = pY->ibuf;
	    break;

        case (MODRM|AX):
            pY->pOpr0 = pY->rbuf;
            pY->pOpr1 = pY->mbuf;
	    break;

        case (MODRM|CL):
            pY->pOpr0 = pY->mbuf;
            pY->pOpr1 = "CL";
	    break;

        case (DIS|AX):
            if (pX->d)
	        {
                pY->pOpr0 = pY->dbuf;
                pY->pOpr1 = pY->rbuf;
	        }
            else
	        {
                pY->pOpr0 = pY->rbuf;
                pY->pOpr1 = pY->dbuf;
	        }
	    break;

        case OFFSEL:
        case D16L8:
            pY->pOpr0 = pY->ibuf;
            pY->pOpr1 = pY->dbuf;
	    break;

        case (FD|ST):
            if (pX->fd)
	        {
                pY->pOpr0 = pY->rbuf;
                pY->pOpr1 = "ST";
	        }
            else
	        {
                pY->pOpr0 = "ST";
                pY->pOpr1 = pY->rbuf;
	        }
	    break;

        case (PORT|AX):
	    if (pX->pI->type == itInF)
		{
		pY->pOpr0 = pY->rbuf;
		pY->pOpr1 = pY->ibuf;
		}
	    else
		{
		pY->pOpr0 = pY->ibuf;
		pY->pOpr1 = pY->rbuf;
		}
	    break;

        case (REG|AX):
	    pY->pOpr0 = pY->ibuf;
	    pY->pOpr1 = pY->rbuf;
	    break;

        case AX:
	    if (pX->pI->type == itInV)
		{
		pY->pOpr0 = pY->rbuf;
		pY->pOpr1 = "DX";
		}
	    else if (pX->pI->type == itOutV)
		{
		pY->pOpr0 = "DX";
		pY->pOpr1 = pY->rbuf;
		}
	    else
                pY->pOpr0 = pY->rbuf;
	    break;

        case 0:
	    break;

        default:
	    printf ("dsmI86.c error 1: Invalid opcode flag definition.\n");
            printf ("\top = 0x%02x 0x%02x 0x%02x 0x%02x\n",
		     (UCHAR)*pX->pD, (UCHAR)*(pX->pD + 1),
		     (UCHAR)*(pX->pD + 2), (UCHAR)*(pX->pD + 3));
        }

    if ((pX->pI->flag2 & XMMREG) || (pX->pI->flag2 & XMMRM))
        {
        if (pX->d)
            {
            pY->pOpr0 = pY->mbuf;		/* data dir = reg to r/m */
            pY->pOpr1 = pY->rbuf;
            }
        else
            {
            pY->pOpr0 = pY->rbuf;		/* data dir = r/m to reg */
            pY->pOpr1 = pY->mbuf;
            }
        }
    else if (pX->pI->flag2 != 0)
        {
	printf ("dsmI86.c error 2: Invalid opcode flag definition.\n");
        printf ("\top = 0x%02x 0x%02x 0x%02x 0x%02x\n",
 	        (UCHAR)*pX->pD, (UCHAR)*(pX->pD + 1),
	        (UCHAR)*(pX->pD + 2), (UCHAR)*(pX->pD + 3));
        }
	
    /* tune up for the special case */

    if ((pY->pOpr0 == pY->mbuf) && (pY->mbuf[0] == 0))
	pY->pOpr0 = (char *)&pY->addr;

    if ((pY->pOpr1 == pY->mbuf) && (pY->mbuf[0] == 0))
	pY->pOpr1 = (char *)&pY->addr;

    if ((pY->pOpr2 == pY->mbuf) && (pY->mbuf[0] == 0))
	pY->pOpr2 = (char *)&pY->addr;

    /* tune up for "+(no-x1)" */

    if (pY->mbuf[0] != 0)
	{
        pS = pY->mbuf;
        pD = pY->temp;
        for (ix = 0 ; ix < (int) (strlen (pY->mbuf)) ; ix++)
	    {
	    if ((*pS == '(') && (*(pS+1) == 'n') && (*(pS+2) == 'o'))
		{
	        pS += 7;
		pD -= 1;
		}
	    *pD++ = *pS++;

	    if (*pS == 0)
		break;
	    }

        bcopy (pY->temp, pY->mbuf, DSM_BUFSIZE32);
	}

    /* tune up for "+-" */

    bzero ((char *)pY->temp, DSM_BUFSIZE32);

    if (pY->mbuf[0] != 0)
	{
        pS = pY->mbuf;
        pD = pY->temp;
        for (ix = 0 ; ix < (int) (strlen (pY->mbuf)) ; ix++)
	    {
	    if ((*pS == '+') && (*(pS+1) == '-'))
	        pS += 1;
	    *pD++ = *pS++;

	    if (*pS == 0)
		break;
	    }

        bcopy (pY->temp, pY->mbuf, DSM_BUFSIZE32);
	}

    /* print out the instruction */

    printf ("%-12s   ", pY->pOpc);

    if (pY->pOpr0 != 0)
	{
        if (pY->pOpr0 == (char *)&pY->addr)
	    (*prtAddress) ((int)pX->pD + pY->len + pY->addr);
	else
	    printf ("%s", pY->pOpr0);
	}

    if (pY->pOpr1 != 0)
	{
	printf (", ");
        if (pY->pOpr1 == (char *)&pY->addr)
	    (*prtAddress) ((int)pX->pD + pY->len + pY->addr);
	else
	    printf ("%s", pY->pOpr1);
	}

    if (pY->pOpr2 != 0)
	{
	printf (", ");
        if (pY->pOpr2 == (char *)&pY->addr)
	    (*prtAddress) ((int)pX->pD + pY->len + pY->addr);
	else
	    printf ("%s", pY->pOpr2);
	}

    printf ("\n");

    return;
    }
/*******************************************************************************
*
* nPrtAddress - print addresses as numbers
*
* RETURNS: N/A
*/

LOCAL void nPrtAddress
    (
    int address		/* address to print */
    )
    {
    printf ("%#x", address); 
    }
/**************************************************************************
*
* dsmData - disassemble and print a byte as data
*
* This routine disassembles and prints a single byte as data (that is,
* as a .BYTE assembler directive) on standard out.  The disassembled data will
* be prepended with the address passed as a parameter.
* 
* RETURNS : The number of words occupied by the data (always 1).
*/

int dsmData
    (
    FAST UCHAR *pD,		/* pointer to the data       */
    int address 		/* address prepended to data */
    )
    {
    FORMAT_X formatx;
    FORMAT_Y formaty;

    bzero ((char *)&formatx, sizeof (FORMAT_X));
    bzero ((char *)&formaty, sizeof (FORMAT_Y));
    
    formatx.pD = (char *)pD;
    formaty.len = 1;

    dsmPrint (&formatx, &formaty, nPrtAddress);

    return (1);
    }
/*******************************************************************************
*
* dsmInst - disassemble and print a single instruction
*
* This routine disassembles and prints a single instruction on standard
* output.  The function passed as parameter <prtAddress> is used to print any
* operands that might be construed as addresses.  The function could be a
* subroutine that prints a number or looks up the address in a
* symbol table.  The disassembled instruction will be prepended with the
* address passed as a parameter.
* 
* If <prtAddress> is zero, dsmInst() will use a default routine that prints 
* addresses as hex numbers.
*
* ADDRESS PRINTING ROUTINE
* Many assembly language operands are addresses.  In order to print these
* addresses symbolically, dsmInst() calls a user-supplied routine, passed as a
* parameter, to do the actual printing.  The routine should be declared as:
* .CS
*     void prtAddress 
*         (
*         int    address   /@ address to print @/
*         )
* .CE
* When called, the routine prints the address on standard out in either
* numeric or symbolic form.  For example, the address-printing routine used
* by l() looks up the address in the system symbol table and prints the
* symbol associated with it, if there is one.  If not, it prints the address
* as a hex number.
* 
* If the <prtAddress> argument to dsmInst is NULL, a default print routine is
* used, which prints the address as a hexadecimal number.
* 
* The directive .DATA.H (DATA SHORT) is printed for unrecognized instructions.
* 
* The effective address mode is not checked since when the instruction with
* invalid address mode executed, an exception would happen.
* 
* INCLUDE FILE: dsmLib.h
* 
* INTERNAL  
* The instruction type is defined by the format of instruction's argument 
* list.  So the order of argument list table should not be changed.  The 
* default value of size field is defined with a special bit set in the size 
* offset field.  To distinguish FPU instructions, a special bit is set in 
* the type field in the instruction table.
*
* RETURNS: The number of bytes occupied by the instruction.
*/

int dsmInst
    (
    FAST UCHAR *pD,		/* Pointer to the instruction       */
    int address,		/* Address prepended to instruction */
    VOIDFUNCPTR prtAddress	/* Address printing function        */
    )
    {
    FORMAT_X formatx;
    FORMAT_Y formaty;

    if (prtAddress == NULL)
        prtAddress = nPrtAddress;

    bzero ((char *)&formatx, sizeof (FORMAT_X));
    bzero ((char *)&formaty, sizeof (FORMAT_Y));
    
    dsmFind (pD, &formatx, &formaty);

    dsmPrint (&formatx, &formaty, prtAddress);

    if (instKeeper != NULL)
	{
	formatx.pI->pOpc = instKeeper;
	instKeeper = NULL;
	}
    
    return ((formaty.len == 0) ? 1 : formaty.len);
    }

/*******************************************************************************
*
* dsmNbytes - determine the size of an instruction
*
* This routine reports the size, in bytes, of an instruction.
*
* RETURNS: The size of the instruction, or 0 if the instruction is unrecognized.
*/

int dsmNbytes
    (
    FAST UCHAR *pD 		/* Pointer to the instruction */
    )
    {
    FORMAT_X formatx;
    FORMAT_Y formaty;

    bzero ((char *)&formatx, sizeof (FORMAT_X));
    bzero ((char *)&formaty, sizeof (FORMAT_Y));
    
    dsmFind (pD, &formatx, &formaty);

    if (instKeeper != NULL)
	{
	formatx.pI->pOpc = instKeeper;
	instKeeper = NULL;
	}

    return (formaty.len);
    }
