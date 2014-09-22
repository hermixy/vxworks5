/* ppc403.h - PowerPC 403 specific header */

/* Copyright 1984-1997 Wind River Systems, Inc. */
/*
modification history
--------------------
01o,13jun02,jtp  identify class of MMU support (SPR #78396)
01n,22nov01,pch  Add _WRS_STRICT_ALIGNMENT for target/src/ostool/loadElfLib.c,
		 and _WRS_NO_TGT_SHELL_FP for target/src/ostool/shell*
01m,21oct98,elg  added hardware breakpoints for PPC403
01l,18mar97,tam  redefined INT_MASK to mask MSR[CE] (SPR 8192). 
01k,24feb97,tam  added definitions for the PPC403GC and PPC403GCX. 
01j,03oct96,tam  added more definitions for the bank registers. 
01i,20sep96,tam  added _PPC403_ICCR_DEFAULT_VAL and _PPC403_ICCR_DEFAULT_VAL. 
01h,18jun96,tam  added defines for the PowerPC 403 debug registers. 
		 removed TBL/TBU and PVR macros. 
01g,17jun96,tpr  added defines PowerPC 403 specific.
01f,09may96,tam  added missing definitions for dma chained count registers (1-3)
01e,07mar96,tam  fix DMASR macro values. (spr# 6018) - added PVR definition.
01d,08feb96,kkk  fix iocr macro values. (spr# 5416)
01c,17feb95,yao  added macros for dma register.
01b,17jan95,yao  added macros for external interrupt enable/status register.  
		 added definitions for debug control/ status registers.
01a,17mar94,yao  written.
*/

#ifndef __INCppc403h
#define __INCppc403h

#ifdef __cplusplus
extern "C" {
#endif

/* INT_MASK definition (mask EE & CE bits) : overwrite the one in asmPpc.h */

#undef	INT_MASK
#define INT_MASK(src, des)	rlwinm  des, src, 0, 17, 15; \
				rlwinm  des, des, 0, 15, 13

/* Tell the loader this processor can't handle any misalignment */
#define	_WRS_STRICT_ALIGNMENT	1

/* No floating point support in target shell */
#define	_WRS_NO_TGT_SHELL_FP	1

/* No MMU support in architecture library */
#undef _WRS_TLB_MISS_CLASS_SW

/* MSR definitions */

#undef 	_PPC_MSR_POW_U		/* power managemnet not supported */
#undef 	_PPC_MSR_ILE_U		/* little endian mode not supported */
#undef 	_PPC_MSR_SF_U		/* 64 bit mode not implemented */

#undef 	_PPC_MSR_BE		/* branch trace not supported */
#undef 	_PPC_MSR_FE1		/* floating point not supported */
#undef 	_PPC_MSR_FE0		/* floating point not supported */
#undef 	_PPC_MSR_IT		/* instruction address translation unsupported*/
#undef 	_PPC_MSR_DT		/* data address translation unsupported */
#undef 	_PPC_MSR_RI		/* recoverable interrupt unsupported */
#undef 	_PPC_MSR_LE		/* little endian mode unsupported */
#undef 	_PPC_MSR_SE		/* single step unsupported */
#undef 	_PPC_MSR_FP		/* floating point unsupported */

#define	_PPC_MSR_WE_U	0x0004	/* wait state enable */
#define _PPC_MSR_CE_U	0x0002	/* critical interrupt enable */
#define _PPC_MSR_DE	0x0200	/* debug exception enable */
#define _PPC_MSR_IR	0x0020	/* instruction relocate (403GC/GCX) */
#define _PPC_MSR_DR	0x0010	/* date relocate (403GC/GCX) */
#define _PPC_MSR_PE	0x0008	/* protection enable */
#define _PPC_MSR_PX	0x0004	/* protection exclusive mode */
#define _PPC_MSR_CE	0x00020000	/* critical interrupt enable mask */

/* Device Control Register PowerPC403 specific */

#undef  DEC		/* decrementer not supported */

#define	BEAR	0x090	/* bus error adress register read/clear */
#define	BESR	0x091	/* bus error syndrome register read/clear */
#define	BR0	0x080	/* bank register 0 */
#define	BR1	0x081	/* bank register 1 */
#define	BR2	0x082	/* bank register 2 */
#define	BR3	0x083	/* bank register 3 */
#define	BR4	0x084	/* bank register 4 */
#define	BR5	0x085	/* bank register 5 */
#define	BR6	0x086	/* bank register 6 */
#define	BR7	0x087	/* bank register 7 */
#define	BRH0	0x070	/* bank register 0 high */
#define	BRH1	0x071	/* bank register 1 high */
#define	BRH2	0x072	/* bank register 2 high */
#define	BRH3	0x073	/* bank register 3 high */
#define	BRH4	0x074	/* bank register 4 high */
#define	BRH5	0x075	/* bank register 5 high */
#define	BRH6	0x076	/* bank register 6 high */
#define	BRH7	0x077	/* bank register 7 high */
#define DMACC0  0xc4	/* dma chained count 0 r/w */
#define DMACC1 	0xcc	/* dma chained count 1 r/w */
#define DMACC2 	0xd4	/* dma chained count 2 r/w */
#define DMACC3 	0xdc	/* dma chained count 3 r/w */
#define DMACR0 	0xc0	/* dma channel control register 0 r/w */
#define DMACR1 	0xc8	/* dma channel control register 1 r/w */
#define DMACR2 	0xd0	/* dma channel control register 2 r/w */
#define DMACR3	0xd8	/* dma channel control register 3 r/w */
#define DMACT0 	0xc1	/* dma count register 0 r/w */
#define DMACT1 	0xc9	/* dma count register 1 r/w */
#define DMACT2 	0xd1	/* dma count register 2 r/w */
#define DMACT3 	0xd9	/* dma count register 3 r/w */
#define DMADA0 	0xc2	/* dma destination address register 0 r/w */
#define DMADA1 	0xca	/* dma destination address register 1 r/w */
#define DMADA2 	0xd2	/* dma destination address register 2 r/w */
#define DMADA3 	0xda	/* dma destination address register 3 r/w */
#define DMASA0 	0xc3	/* dma source address register 0 r/w */
#define DMASA1 	0xcb	/* dma source address register 1 r/w */
#define DMASA2 	0xd3	/* dma source address register 2 r/w */
#define DMASA3 	0xdb	/* dma source address register 3 r/w */
#define DMASR  	0xe0	/* dma status register r/w */
#define	EXIER	0x42	/* external interrupt enable register r/w */
#define	EXISR	0x40	/* external interrupt status register r/c */
#define	IOCR	0x0a0	/* input/output configuration register r/w */

/* Special Purpose Register PowerPC403 specific */

#define CDBCR	0x3d7	/* cache debug control register r/w */
#define DAC1	0x3f6	/* data adress compare register 1 r/w */
#define DAC2	0x3f7	/* data adress compare register 2 r/w */
#define	DBSR	0x3f0	/* debug status register read/clear */
#define	DBCR	0x3f2	/* debug control register read/write */
#define	DCCR	0x3fa	/* data cache control register r/w */
#define DCWR    0x3ba	/* data cache write-thru register (PPC403GC) r/w */
#define	DEAR	0x3d5	/* data exception address registers r */
#define	ESR	0x3d4	/* exception syndrom register r/w */
#define	EVPR	0x3d6	/* exception prefix register r/w */
#define IAC1	0x3f4	/* instruction adress compare register 1 r/w */
#define IAC2	0x3f5	/* instruction adress compare register 2 r/w */
#define	ICCR	0x3fb	/* instruction cache cacheability register r/w */
#define	ICDBDR	0x3d3	/* instruction cache debug data register r */
#define	PBL1	0x3fc	/* protection bound lower 1 r/w */
#define	PBL2	0x3fe	/* protection bound lower 2 r/w */
#define	PBU1	0x3fd	/* protection bound upper 1 r/w */
#define	PBU2	0x3ff	/* protection bound upper 2 r/w */
#define	PID	0x3b1	/* process id (403GC/GCX) r */
#define	PIT	0x3db	/* programmable interval timer r/w */
#define SGR    	0x3b9	/* storage guarded register (PPC403GC) r/w */
#define	SRR2	0x3de	/* save/restore register 2 r/w */
#define	SRR3	0x3df	/* save/restore register 3 r/w */
#define	TBLO	0x3dd	/* time base low r/w */
#define	TBHI	0x3dc	/* time base high r/w */
#define	TCR	0x3da	/* timer control register r/w */
#define	TSR	0x3d8	/* timer status register read/clear */
#define	TSRS	0x3d9	/* timer status register set (set only) */
#define	ZPR	0x3b0	/* zone protection register (PPC403GC/GCX) r/w */

#define mtdcr(dcrn, rs) .long   (0x7c0001c3 | (rs << 21) | (dcrf << 11))

#define MTDCR		0x7c0001c3
#define MFDCR		0x7c000143
#define _EXISR_OP	0x00001000
#define _EXIER_OP	0x00021000
#define MFEXISR_P0	.long   0x7c601286
#define MTEXISR_P0	.long   0x7c601386
#define MFEXISR_P1	.long   0x7c801286
#define MTEXISR_P1	.long   0x7c801386
#define MTEXIER_P0	.long   0x7c621386
#define MFEXIER_P0	.long   0x7c621286
#define MTEXIER_P1	.long   0x7c811386
#define MTEXIER_P2	.long   0x7ca11386
#define MFEXIER_P1	.long   0x7c811286

#define MTBESR_P0	.long   0x7c712386
#define RFCI		.long   0x4c000066

/* defines for cache support */

#undef	_CACHE_ALIGN_SIZE
#define	_CACHE_ALIGN_SIZE	16	/* cache line size */

#define _ICACHE_LINE_NUM	64	/* 64 cache lines per set */
#define _DCACHE_LINE_NUM	32	/* 32 cache lines per set */

#define	_PPC403_ICCR_DEFAULT_VAL	0x80000001 /* def. inst. cachability */
#define	_PPC403_DCCR_DEFAULT_VAL	0x80000001 /* def. data cachability */

#define	CACHE_SAFE_ADRS(x)	((int) (x) | 0x80000000)
#define	CACHE_ORIG_ADRS(x)	((int) (x) & 0x7fffffff)

#define _PPC_CACHE_UNIFIED	FALSE	/* XXX */

/* 403ga specific special purpouse registers */

#define	_PPC403_DBSR	0x3f0	/* debug status register read/clear */
#define _PPC403_DCCR	0x3fa	/* data cache control register r/w */
#define _PPC403_DEAR	0x3d5	/*  data exception address registers r */
#define _PPC403_ESR	0x3d4	/* exception syndrom register r/w */
#define _PPC403_EVPR	0x3d6	/* exception prefix register r/w */
#define _PPC403_ICCR	0x3fb	/* instruction cache control register r/w */
#define _PPC403_PBL1	0x3fc	/* protection bound lower 1 r/w */
#define _PPC403_PBL2	0x3fe	/* protection bound lower 2 r/w */
#define _PPC403_PBU1	0x3fd	/* protection bound upper 1 r/w */
#define _PPC403_PBU2	0x3ff	/* protection bound upper 2 r/w */
#define _PPC403_PIT	0x3db	/* programmable interval timer r/w */
#define _PPC403_SRR2	0x3de	/* save/restore register 2 r/w */
#define _PPC403_SRR3	0x3df	/* save/restore register 3 r/w */
#define _PPC403_SGR    	0x3b9	/* storage guarded register (PPC403GC) r/w */
#define _PPC403_DCWR    0x3ba   /* data cache write-thru (PPC403GC)r/w */

/* debug control register */

#define _DBCR_EDM_U	0x8000		/* external debug mode */
#define _DBCR_IDM_U	0x4000		/* internal debug mode */
#define _DBCR_IC_U	0x0800		/* instruction completion debug event */
#define _DBCR_BT_U	0x0400		/* branch taken */
#define _DBCR_EDE_U	0x0200		/* exception debug event */
#define _DBCR_TDE_U	0x0100		/* trap debug event */
#define _DBCR_FT_U	0x0004		/* freeze timers on debug */
#define _DBCR_IA1_U	0x0002		/* instruction address compare 1 */
#define _DBCR_IA2_U	0x0001		/* instruction address compare 2 */
#define _DBCR_EDM	0x80000000	/* external debug mode */
#define _DBCR_IDM	0x40000000	/* internal debug mode */
#define _DBCR_IC	0x08000000	/* instruction completion debug event */
#define _DBCR_BT	0x04000000	/* branch taken */
#define _DBCR_EDE	0x02000000	/* exception debug event */
#define _DBCR_TDE	0x01000000	/* trap debug event */
#define _DBCR_FT	0x00040000	/* freeze timers on debug */
#define _DBCR_IA1	0x00020000	/* instruction address compare 1 */
#define _DBCR_IA2	0x00010000	/* instruction address compare 2 */
#define _DBCR_D1R	0x00008000	/* data address compare read 1 */
#define _DBCR_D1W	0x00004000	/* data address compare write 1 */
#define _DBCR_D2R	0x00000800	/* data address compare read 2 */
#define _DBCR_D2W	0x00000400	/* data address compare write 2 */
#define _DBCR_JOI	0x00000002	/* jtag serial outbound */
#define _DBCR_JII	0x00000001	/* jtag serial inbound */

/* set access from type in DBCR */

#define _DBCR_D1A(x)	(((x) & 0x03) << 14)	/* for first breakpoint */
#define _DBCR_D2A(x)	(((x) & 0x03) << 10)	/* for second breakpoint */

/* set size from type in DBCR */

#define _DBCR_D1S(x)	(((x) & 0x0C) << 10)	/* for first breakpoint */
#define _DBCR_D2S(x)	(((x) & 0x0C) << 6)	/* for first breakpoint */

/* get access from DBCR */

#define _DBCR_D1_ACCESS(x)	(((x) >> 14) & 0x03)
#define _DBCR_D2_ACCESS(x)	(((x) >> 10) & 0x03)

/* get size from DBCR */

#define _DBCR_D1_SIZE(x)	(((x) >> 10) & 0x0C)
#define _DBCR_D2_SIZE(x)	(((x) >> 6) & 0x0C)

/* debug status register */

#define _DBSR_IC_U	0x8000		/* instruction completion */
#define _DBSR_BT_U	0x4000		/* branch taken */
#define _DBSR_EXC_U	0x2000		/* exception debug */
#define _DBSR_TIE_U	0x1000		/* trap instruction */
#define _DBSR_UDE_U	0x0800		/* unconditional debug */
#define _DBSR_IA1_U	0x0400		/* instruction address compare 1 */
#define _DBSR_IA2_U	0x0200		/* instruction address compare 2 */
#define _DBSR_DR1_U	0x0100		/* data address compare read 1 */
#define _DBSR_DW1_U	0x0080		/* data address compare write 1 */
#define _DBSR_DR2_U	0x0040		/* data address compare read 2 */
#define _DBSR_DW2_U	0x0020		/* data address compare write 2 */
#define _DBSR_IDE_U	0x0010		/* imprecise debug */
#define _DBSR_IC	0x80000000	/* instruction completion */
#define _DBSR_BT	0x40000000	/* branch taken */
#define _DBSR_EXC	0x20000000	/* exception debug */
#define _DBSR_TIE	0x10000000	/* trap instruction */
#define _DBSR_UDE	0x08000000	/* unconditional debug */
#define _DBSR_IA1	0x04000000	/* instruction address compare 1 */
#define _DBSR_IA2	0x02000000	/* instruction address compare 2 */
#define _DBSR_DR1	0x01000000	/* data address compare read 1 */
#define _DBSR_DW1	0x00800000	/* data address compare write 1 */
#define _DBSR_DR2	0x00400000	/* data address compare read 2 */
#define _DBSR_DW2	0x00200000	/* data address compare write 2 */
#define _DBSR_IDE	0x00100000	/* imprecise debug */
#define _DBSR_MRRM	0x00000300	/* Most recent reset mask */
#define _DBSR_MRR(n)	((n) << 8) 	/* Most recent reset */
#define _DBSR_JIF	0x00000004	/* jtag serial inbound buffer full */
#define _DBSR_JIO	0x00000002	/* jtag serial inbound buffer overrun */
#define _DBSR_JOE	0x00000001	/* jtag serial outbound buffer empty */

/* mask for hardware breakpoints */

#define _DBSR_HWBP_MSK	(_DBSR_IA1 | _DBSR_IA2 | \
                         _DBSR_DR1 | _DBSR_DW1 | \
			 _DBSR_DR2 | _DBSR_DW2)

/* device control registers */

#define _PPC403_BEAR	0x90	/* bus error address register (read only) */
#define _PPC403_BESR	0x91	/* bus error syndrom register (r/w) */
#define _PPC403_BR0	0x80	/* bank register 0 (r/w) */
#define _PPC403_BR1	0x81	/* bank register 1 (r/w) */
#define _PPC403_BR2	0x82	/* bank register 2 (r/w) */
#define _PPC403_BR3	0x83	/* bank register 3 (r/w) */
#define _PPC403_BR4	0x84	/* bank register 4 (r/w) */
#define _PPC403_BR5	0x85	/* bank register 5 (r/w) */
#define _PPC403_BR6	0x86	/* bank register 6 (r/w) */
#define _PPC403_BR7	0x87	/* bank register 7 (r/w) */

#define _PPC403_DMACC0 0xc4	/* dma chained count 0 (r/w) */
#define _PPC403_DMACC1 0xcc     /* dma chained count 1 (r/w) */
#define _PPC403_DMACC2 0xd4     /* dma chained count 2 (r/w) */
#define _PPC403_DMACC3 0xdc     /* dma chained count 3 (r/w) */
#define _PPC403_DMACR0 0xc0	/* dma channel control register 0 (r/w) */
#define _PPC403_DMACR1 0xc8	/* dma channel control register 1 (r/w) */
#define _PPC403_DMACR2 0xd0	/* dma channel control register 2 (r/w) */
#define _PPC403_DMACR3 0xd8	/* dma channel control register 3 (r/w) */
#define _PPC403_DMACT0 0xc1	/* dma count register 0 (r/w) */
#define _PPC403_DMACT1 0xc9	/* dma count register 1 (r/w) */
#define _PPC403_DMACT2 0xd1	/* dma count register 2 (r/w) */
#define _PPC403_DMACT3 0xd9	/* dma count register 3 (r/w) */
#define _PPC403_DMADA0 0xc2	/* dma destination address register 0 (r/w) */
#define _PPC403_DMADA1 0xca	/* dma destination address register 1 (r/w) */
#define _PPC403_DMADA2 0xd2	/* dma destination address register 2 (r/w) */
#define _PPC403_DMADA3 0xda	/* dma destination address register 3 (r/w) */
#define _PPC403_DMASA0 0xc3	/* dma source address register 0 (r/w) */
#define _PPC403_DMASA1 0xcb	/* dma source address register 1 (r/w) */
#define _PPC403_DMASA2 0xd3	/* dma source address register 2 (r/w) */
#define _PPC403_DMASA3 0xdb	/* dma source address register 3 (r/w) */
#define _PPC403_DMASR  0xe0	/* dma status register (r/w) */

#define _PPC403_EXISR  0x40	/* external interrupt status register (r/w) */
#define _PPC403_EXIER  0x42	/* external interrupt enable register (r/w) */
#define _PPC403_IOCR   0xa0	/* input/output configuration register (r/w) */

/* time control register */

#define _PPC403_TCR_WPM_U	0xc000	/* watchdog period mask */
#define _PPC403_TCR_WRCM_U	0x3000	/* wotchdog reset control */
#define _PPC403_TCR_WIEM_U	0x0800	/* watchdog interrupt enable */
#define _PPC403_TCR_PIEM_U	0x0400	/* pit interupt enable */
#define _PPC403_TCR_FPM_U	0x0300	/* fit period */
#define _PPC403_TCR_FIEM_U	0x0080	/* fit interrupt enable */

/* timer control register */

#define _PPC403_TCR_WPM_U	0xc000	/* watchdog period */
#define _PPC403_TCR_WRCM_U	0x3000	/* watchdog reset control */
#define _PPC403_TCR_WIEM_U	0x0800	/* watchdog interrupt enable */
#define _PPC403_TCR_PIEM_U	0x0400	/* pit interrupt enable */
#define _PPC403_TCR_FP_U	0x0300	/* fit period */
#define _PPC403_TCR_FIE_U	0x0080	/* fit interrupt enable */
#define _PPC403_TCR_ARE_U	0x0040	/* fit auto-reload enable */

/* timer status register */

#define _PPC403_TSR_SNWM_U	0x8000	/* supress next watchdog */
#define _PPC403_TSR_WISM_U	0x4000	/* watchdog interrupt status */
#define _PPC403_TSR_WRSM_U	0x3000	/* watchdog reset status */
#define _PPC403_TSR_PISM_U	0x0800	/* pit interrupt status */
#define _PPC403_TSR_FISM_U	0x0400	/* fit interrupt status */


/* defintion of external interrupt status and enable registers' mask bits */
/* Interrupts are enabled by setting the corresponding bits in the EXIER  */
/* When interrupts occur, the corresponding bits in EXISR are set to one. */

#define _PPC403_EXI_CI	0x80000000	/* critical interrupt */
#define _PPC403_EXI_SRI	0x08000000	/* serial port receiver interrupt */
#define _PPC403_EXI_STI	0x04000000	/* serial port transmitter interrupt */
#define _PPC403_EXI_JRI	0x02000000	/* JTAG serial port receiver */
#define _PPC403_EXI_JTI	0x01000000	/* JTAG serial port receiver */
#define _PPC403_EXI_D0I	0x00800000	/* dma channel 0 */
#define _PPC403_EXI_D1I	0x00400000	/* dma channel 1 */
#define _PPC403_EXI_D2I	0x00200000	/* dma channel 2 */
#define _PPC403_EXI_D3I	0x00100000	/* dma channel 3 */
#define _PPC403_EXI_E0I	0x00000010	/* external interrupt0 */
#define _PPC403_EXI_E1I	0x00000008	/* external interrupt1 */
#define _PPC403_EXI_E2I	0x00000004	/* external interrupt2 */
#define _PPC403_EXI_E3I	0x00000002	/* external interrupt3 */
#define _PPC403_EXI_E4I	0x00000001	/* external interrupt4 */


#define	_PPC403_IOCR_E0TM	0x80000000  /* external intr 0 triggering */
					    /* 0 - level sensitive */
					    /* 1 - edge triggered */
#define	_PPC403_IOCR_E0LM	0x40000000  /* external inter 0 active level */
#define	_PPC403_IOCR_E1TM	0x20000000  /* external intr 1 triggering */
#define	_PPC403_IOCR_E1LM	0x10000000  /* external intr 1 active level */
#define	_PPC403_IOCR_E2TM	0x08000000  /* external intr 2 triggering */
#define	_PPC403_IOCR_E2LM	0x04000000  /* external intr 2 active level */
#define	_PPC403_IOCR_E3TM	0x02000000  /* external intr 3 triggering */
#define	_PPC403_IOCR_E3LM	0x01000000  /* external intr 3 active level */
#define	_PPC403_IOCR_E4TM	0x00800000  /* external intr 4 triggering */
#define	_PPC403_IOCR_E4LM	0x00400000  /* external intr 4 active level */
#define	_PPC403_IOCR_EDT	0x00080000  /* enable dram tri-state (GCX) */
#define	_PPC403_IOCR_SOR	0x00040000  /* enable sampling data (GCX) */
#define	_PPC403_IOCR_EDO	0x00008000  /* EDO dram enable (GCX) */
#define	_PPC403_IOCR_2XC	0x00004000  /* clock double core enable (GCX) */
#define	_PPC403_IOCR_ATC	0x00002000  /* adress tri-state control */
#define	_PPC403_IOCR_SPD	0x00001000  /* static power disable */
#define	_PPC403_IOCR_BEM	0x00000800  /* byte enable mode  SRAM accesses*/
#define	_PPC403_IOCR_PTD	0x00000400  /* device-paced mode disable SRAM */
#define	_PPC403_IOCR_ARE	0x00000080  /* asynchronous ready enable SRAM */

#define	_PPC403_IOCR_DRCM	0x00000020  /* DRAM read on CAS */
#define	_PPC403_IOCR_RDMM	0x00000018  /* real time debug mode */
					    /* b'00 - trace status output disabled */
					    /* b'01 - program and bus status */
					    /* b'10 - program status and trace output */
					    /* b'11 - reserved */
#define	_PPC403_IOCR_RDM(n)	((n)<<3)    /* real time debug mode */
#define	_PPC403_IOCR_TCSM	0x00000004  /* timer clock source */
					    /* 0 - SysClk pin */
					    /* 1 - TimerClk pin */
#define	_PPC403_IOCR_SCSM	0x00000002  /* serial port clock source */
					    /* 0 - SysClk pin */
					    /* 1 - SerClk pin */
#define	_PPC403_IOCR_SPCM	0x00000001  /* serial port configuration */
					    /* 0 - DSR/DTR */
					    /* 1 - CTS/RTS */
/* exception syndrome register mask bits: 
 * 0 - error not occured 1 - error occured */

#define _PPC403_ESR_IMCPM 0x80000000  /* instr machine check protection */
#define _PPC403_ESR_IMCNM 0x40000000  /* instr machine check non-configured */
#define _PPC403_ESR_IMCBM 0x20000000  /* instr machine check bus error */
#define _PPC403_ESR_IMCTM 0x10000000  /* instr machine check timeout */
#define _PPC403_ESR_PEIM  0x08000000  /* program exception - illegal */
#define _PPC403_ESR_PEPM  0x04000000  /* program exception - previledged */
#define _PPC403_ESR_PETM  0x02000000  /* program exception - trap */
#define _PPC403_ESR_DST   0x00800000  /* data storage exception - store fault */
#define _PPC403_ESR_DIZ   0x00400000  /* data/inst storage exception - zone f.*/

/* bus error syndrome register mask bits: 0 - no eroor  1 - error occured */

#define _PPC403_BESR_DSESM	0x80000000  /* data-side error status */
#define _PPC403_BESR_DMESM	0x40000000  /* dma error status */
#define _PPC403_BESR_RWSM	0x20000000  /* read/write status */
					    /* 0 - write error */
					    /* 1 - read error */
#define _PPC403_BESR_ETM	0x1c000000  /* error type */
					    /* 000 - protection violation */
					    /* 001 - parity error (GCX) */
					    /* 010 - access to non configured bank */
					    /* 100 - active level error on bus eroor input pin */
					    /* 110 - bus time-out */
					    /* 111 - reserved */
/* exception vector prefix register */

#define _PPC403_EVPR_EVPM	0xffff0000  /* exception vector prefix mask */

/* common bits for sram and dram configuration */

#define _PPC403_BR_BASM	   0xff000000	/* base address select mask*/
#define _PPC403_BR_BAS(n)  ((n)<<24)	/* base address select */
#define _PPC403_BR_BSM	   0x00e00000	/* bank size mask */
#define _PPC403_BR_BS_1MB  0x00000000	/* bank size : 000 - 1MB */
#define _PPC403_BR_BS_2MB  0x00200000	/* bank size : 001 - 2MB */
#define _PPC403_BR_BS_4MB  0x00400000	/* bank size : 010 - 4MB */
#define _PPC403_BR_BS_8MB  0x00600000	/* bank size : 011 - 8MB */
#define _PPC403_BR_BS_16MB 0x00800000	/* bank size : 100 - 16MB */
#define _PPC403_BR_BS_32MB 0x00a00000	/* bank size : 101 - 32MB */
#define _PPC403_BR_BS_64MB 0x00c00000	/* bank size : 110 - 64MB */
#define _PPC403_BR_BUM	   0x00180000	/* bank usage mask */
#define _PPC403_BR_BU_DIS  0x00000000	/* bank usage: 00 - disable */ 
#define _PPC403_BR_BU_RO   0x00080000	/* bank usage: 01 - read only */ 
#define _PPC403_BR_BU_WO   0x00100000	/* bank usage: 10 - write only */ 
#define _PPC403_BR_BU_RW   0x00180000	/* bank usage: 11 - read/write only */ 
#define _PPC403_BR_SLFM	   0x00040000	/* sram-dram sequential line fill */
#define _PPC403_BR_BWM	   0x00018000	/* bus width mask */
#define _PPC403_BR_BW_8	   0x00000000	/* bus width 00 - 8 bit  */
#define _PPC403_BR_BW_16   0x00008000	/* bus width 01 - 16 bit */
#define _PPC403_BR_BW_32   0x00010000	/* bus width 10 - 32 bit */
#define _PPC403_BR_BW_64   0x00018000	/* bus width 11 - 64 bit */
#define _PPC403_BR_SDM	   0x00000001	/* sram-dram selection 0-dram 1-sram */

/* sram configuration (br0 - br7) */

#define _PPC403_BRS_BMEM   0x00020000	/* burst mode enable - 0 disabled */
#define _PPC403_BRS_REM	   0x00004000	/* ready enable - 0 disabled */
#define _PPC403_BRS_TWM	   0x00003f00	/* transfer wiat-contains the number
					 * of wait states inserted by the 
					 * processor into all transactions */
#define _PPC403_BRS_TWT(n)  ((n)<<8)	/* transfer wait states - non-burst */
#define _PPC403_BRS_FWM     0x00003c00	/* first wait states mask */
#define _PPC403_BRS_FWT(n)  ((N)<<10)	/* first wait states - burst */
#define _PPC403_BRS_BWM     0x00000300	/* burst wait states mask */
#define _PPC403_BRS_BWT(n)  ((N)<<10)	/* burst wait states */
#define _PPC403_BRS_CSTM    0x00000080	/* chip select on timing
					 * 0 - valid when address is valid
					 * 1 - valid one SysClk cycle after
					 * address is valid */
#define _PPC403_BRS_OETM 0x00000040	/* output enable on timing
					 * 0 - valid when chip select is valid
					 * 1 - valid one SysClk cycle after
					 * chip select is valid */
#define _PPC403_BRS_WBNM 0x00000020	/* write byte enable on timing
					 * 0 - valid one chip select is valid
					 * 1 - on SysClk Cycle after */
#define _PPC403_BRS_WBFM 0x00000010	/* write byte enable off on timing
					 * 0 - inactive when chip select 
					 *     becomes inactive
					 * 1 - inactive one SysClk cycle 
					 *     before chip select becoms
					 *     inactive */
#define _PPC403_BRS_THM	0x0000000e	/* transfer hold mask */
#define _PPC403_BRS_TH(n)  ((n)<<1)	/* transfer hold */

/* dram configuration (br4 - br7) */

#define _PPC403_BRD_ERM	 0x00020000	/* Early Ras Mode */
#define _PPC403_BRD_IEM	 0x00004000	/* internal/external multiplex */
#define _PPC403_BRD_RCTM 0x00002000	/* RAS active to CAS active timing */
#define _PPC403_BRD_ARMM 0x00001000	/* alternate refresh mode 
					 * 0 - normal refresh
					 * 1 - immediate or self refresh */
#define _PPC403_BRD_PMM	 0x00000800	/* page mode 
					 * 0 - single accesses only
					 * 1 - burst access supported */
#define _PPC403_BRD_FACM 0x00000600	/* first access timing mask */
#define	_PPC403_BRD_FAC(n) ((n)<<9)	/* first access timing */
#define _PPC403_BRD_BACM 0x00000180	/* burst access timing mask */
#define _PPC403_BRD_BAC(n) ((n)<<7)	/* burst access timing */
#define _PPC403_BRD_PPCM 0x00000040	/* precharge cycles */
#define _PPC403_BRD_RARM 0x00000020	/* RAS active during refresh
					 * 0 - one and half sysclk cycle
					 * 1 - two and half */
#define _PPC403_BRD_RRM	0x0000001e	/* refresh interval mask */
#define _PPC403_BRD_RR(n)  ((n)<<1)	/* refresh interval */

/* bank register high configuration (brh0 - brh7) */


#define _PPC403_BRH_PCE	0x80000000	/* parity check enable */

/* dma channel control register */

#define _DMACR_CE	0x80000000	/* channel enable */
#define _DMACR_CIE	0x40000000	/* channel interrupt enable */
#define _DMACR_TD	0x20000000	/* transfer direction */
#define _DMACR_PL	0x10000000	/* peripheral location */
#define _DMACR_PW_MASK  0x0c000000	/* peripheral width */
#define _DMACR_PW_BYTE 	0x00000000	/* byte */
#define _DMACR_PW_SHORT 0x04000000	/* halfword */
#define _DMACR_PW_WORD 	0x08000000	/* word */
#define _DMACR_PW_RES 	0x0c000000	/* reserved */
#define _DMACR_DAI	0x02000000	/* destination address increment */
#define _DMACR_SAI	0x01000000	/* source address increment */
#define _DMACR_CP	0x00800000	/* channel priority */
#define _DMACR_TM_M	0x00600000	/* transfer mode mask */
#define _DMACR_TM_B	0x00000000	/* buffered dma */
#define _DMACR_TM_F	0x00200000	/* fly-by dma */
#define _DMACR_TM_S	0x00400000	/* software initiated mem-to-mem dma */
#define _DMACR_TM_H	0x00600000	/* hardware initiated mem-to-mem dma */
#define _DMACR_PSC_M	0x00180000	/* peripheral setup cycles mask */
#define _DMACR_PSC_0	0x00000000	/* no cycles */
#define _DMACR_PSC_1	0x00080000	/* one cycles */
#define _DMACR_PSC_2	0x00100000	/* two cycles */
#define _DMACR_PSC_3	0x00180000	/* three cycles */
#define _DMACR_PWC_MASK	0x0007e000 	/* peripheral wait cycles mask */
#define _DMACR_PHC_MASK 0x00001c00	/* peripheral hold cycles */
#define _DMACR_ETD	0x00000200	/* eoc/tc pin direction */
#define _DMACR_TCE	0x00000100	/* terminal count enable 
					 * 0 - end-of-transfer input
					 * 1 - terminal count output
					 */
#define _DMACR_CH	0x00000080	/* chaining enable */
#define _DMACR_BME	0x00000040	/* burst mode enable  */
#define _DMACR_ECE	0x00000020	/* EOT chain mode enable */
#define _DMACR_TCD	0x00000010	/* TC chain mode disable */
#define _DMACR_PCE	0x00000008	/* parity check enable */

/* dma status registers */

#define _DMASR_CS0_MASK	0x80000000	/* channel 0 terminal status */
#define _DMASR_CS1_MASK	0x40000000	/* channel 1 terminal status */
#define _DMASR_CS2_MASK	0x20000000	/* channel 2 terminal status */
#define _DMASR_CS3_MASK	0x10000000	/* channel 3 terminal status */

#define _DMASR_TS0_MASK	0x08000000	/* channel 0 end-of-transfer status */
#define _DMASR_TS1_MASK	0x04000000	/* channel 1 end-of-transfer status */
#define _DMASR_TS2_MASK	0x02000000	/* channel 2 end-of-transfer status */
#define _DMASR_TS3_MASK	0x01000000	/* channel 3 end-of-transfer status */

#define _DMASR_RI0_MASK	0x00800000	/* channel 0 error status */
#define _DMASR_RI1_MASK	0x00400000	/* channel 1 error status */
#define _DMASR_RI2_MASK	0x00200000	/* channel 2 error status */
#define _DMASR_RI3_MASK	0x00100000	/* channel 3 error status */

#define _DMASR_IR0_MASK	0x00040000	/* internal dma reguest 0 */
#define _DMASR_IR1_MASK	0x00020000	/* internal dma reguest 1 */
#define _DMASR_IR2_MASK	0x00010000	/* internal dma reguest 2 */
#define _DMASR_IR3_MASK	0x00008000	/* internal dma reguest 3 */

#define _DMASR_ER0_MASK	0x00004000	/* external dma reguest 0 */
#define _DMASR_ER1_MASK	0x00002000	/* external dma reguest 1 */
#define _DMASR_ER2_MASK	0x00001000	/* external dma reguest 2 */
#define _DMASR_ER3_MASK	0x00000800	/* external dma reguest 3 */

#define _DMASR_CB0_MASK	0x00000400	/* channel 0 busy */
#define _DMASR_CB1_MASK	0x00000200	/* channel 1 busy */
#define _DMASR_CB2_MASK	0x00000100	/* channel 2 busy */
#define _DMASR_CB3_MASK	0x00000080	/* channel 3 busy */

#define _DMASR_CT0_MASK	0x00080000	/* chained transfer on channel 0 */
#define _DMASR_CT1_MASK 0x00000040      /* chained transfer on channel 1 */
#define _DMASR_CT2_MASK 0x00000020      /* chained transfer on channel 1 */
#define _DMASR_CT3_MASK 0x00000010      /* chained transfer on channel 1 */

/* PVR definition */

#define _PVR_FAM_MSK	0xfff00000	/* processor family mask*/
#define _PVR_MEM_MSK	0x000f0000	/* processor family member mask */
#define _PVR_CCF_MSK	0x0000f000	/* Core Configuration mask */
#define _PVR_PCF_MSK	0x00000f00	/* Peripheral Configuration mask */
#define _PVR_MAJ_MSK	0x000000f0	/* Major Change Level mask */
#define _PVR_MIN_MSK	0x0000000f	/* Minor Change Level mask */

#define _PVR_CONF_403GA	    0x0000	/* processor conf. bits for 403GA */
#define _PVR_CONF_403GC	    0x0002	/* processor conf. bits for 403GC */
#define _PVR_CONF_403GCX    0x0014	/* processor conf. bits for 403GCX */

#ifdef __cplusplus
}
#endif

#endif /* __INCppc403h */
