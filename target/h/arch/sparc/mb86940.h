/* mb86940.h - Fujitsu SPARClite companion chip */

/* Copyright 1984-1991 Wind River Systems, Inc. */
/*
modification history
--------------------
01e,22sep92,rrr  added support for c++
01d,27aug92,ccc  added tyCo function prototypes.
01c,16jun92,jwt  passed through the ansification filter
		  -changed copyright notice
01b,25jan92,ccc  added Timer and Serial definitions.
01a,20jan92,jwt  created Interrupt Request Controller (IRC) definitions.
*/

/*
This file contains constants for the Fujitsu mb86940 companion chip.

The constants SPARCLITE_CCS_OFFSET and SPARCLITE_CCS_BASE must defined when
including this header file.
*/

#ifndef __INCmb86940h
#define __INCmb86940h

#ifdef __cplusplus
extern "C" {
#endif

#define	SLCCS_ADDR(reg)	(SPARCLITE_CCS_BASE + (reg * SPARCLITE_CCS_OFFSET))

/* SPARClite Companion Chip (MB86940) Interrupt Request Controller (IRC) */
#define IRC_TRIGGER0_ADDR       SLCCS_ADDR (0x00)      /* Trigger Mode 0 */
#define IRC_TRIGGER1_ADDR       SLCCS_ADDR (0x01)      /* Trigger Mode 1 */
#define IRC_REQ_SENSE_ADDR      SLCCS_ADDR (0x02)      /* Request Sense */
#define IRC_REQ_CLEAR_ADDR      SLCCS_ADDR (0x03)      /* Request Clear */
#define IRC_MASK_ADDR           SLCCS_ADDR (0x04)      /* Interrupt Masks */
#define IRC_REQUEST_LEVEL_ADDR  SLCCS_ADDR (0x05)      /* IRL Latch / Clear */

/* SPARClite Companion Chip (MB86940) Serial Data Transmitter and Receiver */
#define SDTR0_DATA_ADDR         SLCCS_ADDR (0x08)
#define SDTR0_CSR_ADDR          SLCCS_ADDR (0x09)
#define SDTR1_DATA_ADDR         SLCCS_ADDR (0x0c)
#define SDTR1_CSR_ADDR          SLCCS_ADDR (0x0d)

/* SPARClite Companion Chip (MB86940) Timers */
#define	TIMER0_PRESCALE_ADDR	SLCCS_ADDR (0x10)
#define	TIMER0_CONTROL_ADDR	SLCCS_ADDR (0x11)
#define	TIMER0_RELOAD_ADDR	SLCCS_ADDR (0x12)
#define	TIMER0_COUNT_ADDR	SLCCS_ADDR (0x13)

#define	TIMER1_PRESCALE_ADDR	SLCCS_ADDR (0x14)
#define	TIMER1_CONTROL_ADDR	SLCCS_ADDR (0x15)
#define	TIMER1_RELOAD_ADDR	SLCCS_ADDR (0x16)
#define	TIMER1_COUNT_ADDR	SLCCS_ADDR (0x17)

#define	TIMER2_CONTROL_ADDR	SLCCS_ADDR (0x19)
#define	TIMER2_RELOAD_ADDR	SLCCS_ADDR (0x1a)
#define	TIMER2_COUNT_ADDR	SLCCS_ADDR (0x1b)

#define	TIMER3_CONTROL_ADDR	SLCCS_ADDR (0x1d)
#define	TIMER3_RELOAD_ADDR	SLCCS_ADDR (0x1e)
#define	TIMER3_COUNT_ADDR	SLCCS_ADDR (0x1f)

/* Trigger Mode Register Masks */
#define	IRC_TRIGGER_HIGH	0		/* Active HIGH Trigger */
#define	IRC_TRIGGER_LOW		1		/* Active LOW Trigger */
#define	IRC_TRIGGER_RISING	2		/* Rising Edge Trigger */
#define	IRC_TRIGGER_FALLING	3		/* Falling Edge Trigger */

#define	IRC_TRIGGER_CH15_MASK	0xC000	/* Interrupt Level 15 */
#define	IRC_TRIGGER_CH14_MASK	0x3000	/* Interrupt Level 14 */
#define	IRC_TRIGGER_CH13_MASK	0x0C00	/* Interrupt Level 13 */
#define	IRC_TRIGGER_CH12_MASK	0x0300	/* Interrupt Level 12 */
#define	IRC_TRIGGER_CH11_MASK	0x00C0	/* Interrupt Level 11 */
#define	IRC_TRIGGER_CH10_MASK	0x0030	/* Interrupt Level 10 */
#define	IRC_TRIGGER_CH9_MASK	0x000C	/* Interrupt Level 9 */
#define	IRC_TRIGGER_CH8_MASK	0x0003	/* Interrupt Level 8 */
#define	IRC_TRIGGER_CH7_MASK	0xC000	/* Interrupt Level 7 */
#define	IRC_TRIGGER_CH6_MASK	0x3000	/* Interrupt Level 6 */
#define	IRC_TRIGGER_CH5_MASK	0x0C00	/* Interrupt Level 5 */
#define	IRC_TRIGGER_CH4_MASK	0x0300	/* Interrupt Level 4 */
#define	IRC_TRIGGER_CH3_MASK	0x00C0	/* Interrupt Level 3 */
#define	IRC_TRIGGER_CH2_MASK	0x0030	/* Interrupt Level 2 */
#define	IRC_TRIGGER_CH1_MASK	0x000C	/* Interrupt Level 1 */

/* Request Sense / Clear Level Masks */
#define	IRC_REQUEST_LEVEL_15	0x8000	/* Interrupt Level 15 */
#define	IRC_REQUEST_LEVEL_14	0x4000	/* Interrupt Level 14 */
#define	IRC_REQUEST_LEVEL_13	0x2000	/* Interrupt Level 13 */
#define	IRC_REQUEST_LEVEL_12	0x1000	/* Interrupt Level 12 */
#define	IRC_REQUEST_LEVEL_11	0x0800	/* Interrupt Level 11 */
#define	IRC_REQUEST_LEVEL_10	0x0400	/* Interrupt Level 10 */
#define	IRC_REQUEST_LEVEL_9	0x0200	/* Interrupt Level 9 */
#define	IRC_REQUEST_LEVEL_8	0x0100	/* Interrupt Level 8 */
#define	IRC_REQUEST_LEVEL_7	0x0080	/* Interrupt Level 7 */
#define	IRC_REQUEST_LEVEL_6	0x0040	/* Interrupt Level 6 */
#define	IRC_REQUEST_LEVEL_5	0x0020	/* Interrupt Level 5 */
#define	IRC_REQUEST_LEVEL_4	0x0010	/* Interrupt Level 4 */
#define	IRC_REQUEST_LEVEL_3	0x0008	/* Interrupt Level 3 */
#define	IRC_REQUEST_LEVEL_2	0x0004	/* Interrupt Level 2 */
#define	IRC_REQUEST_LEVEL_1	0x0002	/* Interrupt Level 1 */
#define	IRC_REQUEST_LEVEL	0x0001	/* Interrupt Level Mask */

/* IRL Latch / IRL Clear Register */
#define	IRC_LATCH_CLEAR_BIT	0x0010	/* Latch Clear Bit */
#define	IRC_LATCH_CLEAR_LEVEL	0x000F	/* Interrupt Level Mask */

/* Miscellaneous Interrupt Definitions */
#define	TIMER1_IACK_ADDR	TIMER1_COUNT_ADDR
#define	TIMER2_IACK_ADDR	TIMER2_COUNT_ADDR

/* Timer bit definitions */
#define	TIMER_PRESCALE_INTERNAL	0x0000	/* Internal Clock is Used */
#define	TIMER_PRESCALE_EXTERNAL	0x8000	/* External Clock is Used */

#define	TIMER_PRESCALE_SELECT0	0x0000	/* First Output   */
#define	TIMER_PRESCALE_SELECT1	0x0100	/* Second Output  */
#define	TIMER_PRESCALE_SELECT2	0x0200	/* Third Output   */
#define	TIMER_PRESCALE_SELECT3	0x0300	/* Fourth Output  */
#define	TIMER_PRESCALE_SELECT4	0x0400	/* Fifth Output   */
#define	TIMER_PRESCALE_SELECT5	0x0500	/* Sixth Output   */
#define	TIMER_PRESCALE_SELECT6	0x0600	/* Seventh Output */
#define	TIMER_PRESCALE_SELECT7	0x0700	/* Eighth Output  */

#define	TIMER_CONT_OUT_LOW	0x0000	/* Out pin LOW    */
#define	TIMER_CONT_OUT_HIGH	0x8000	/* Out pin HIGH   */
#define	TIMER_CONT_IN		0x4000	/* In pin mask    */
#define	TIMER_CONT_COUNT_EN	0x0800	/* Enable Count   */
#define	TIMER_CONT_INTERNAL	0x0000	/* Internal Clock */
#define	TIMER_CONT_EXTERNAL	0x0200	/* External Clock */
#define	TIMER_CONT_PRESCALE	0x0400	/* Prescaler Output Clock */
#define	TIMER_CONT_INHIBIT	0x0600	/* Inhibit */

#define	TIMER_CONT_OUT_SAME	0x0000	/* Out pin remains same     */
#define	TIMER_CONT_OUT_SET	0x0080	/* Out pin set w/mode set   */
#define	TIMER_CONT_OUT_RESET	0x0100	/* Out pin reset w/mode set */
#define	TIMER_CONT_OUT_INHIBIT	0x0180	/* Inhibit Out pin          */
#define	TIMER_CONT_OUT_NORMAL	0x0000	/* Out noninverted	    */
#define	TIMER_CONT_OUT_INVERT	0x0040	/* Out inverted		    */

#define	TIMER_CONT_MODE0	0x0000	/* Mode0:Periodic int mode   */
#define	TIMER_CONT_MODE1	0x0008	/* Mode1:Time-out int mode   */
#define	TIMER_CONT_MODE2	0x0010	/* Mode2:Sqr wave gen mode   */
#define	TIMER_CONT_MODE3	0x0018	/* Mode3:CPU WD/One-shot     */
#define	TIMER_CONT_MODE4	0x0020	/* Mode4:Ext Dev WD/One-shot */

#define	TIMER_CONT_IN_LOW	0x0000	/* IN pin: Low Level    */
#define	TIMER_CONT_IN_HIGH	0x0001	/* IN pin: High Level	*/
#define	TIMER_CONT_IN_RISING	0x0002	/* IN pin: Rising Edge	*/
#define	TIMER_CONT_IN_FALLING	0x0003	/* IN pin: Falling Edge */
#define	TIMER_CONT_IN_BOTH	0x0004	/* IN pin: Both Edges	*/

/* SERIAL DATA TRANSMITTER and RECEIVER bit definitions */

/* Mode Register - Asynchronous mode */

#define FCC_SDTR_1_BIT          0x40            /* 1 stop bit 		 */
#define FCC_SDTR_1_5BITS        0x80            /* 1.5 stop bits 	 */
#define FCC_SDTR_2_BITS         0xc0            /* 2 stop bits 		 */
#define FCC_SDTR_ODD            0x00            /* Odd parity 		 */
#define FCC_SDTR_EVEN           0x20            /* Even parity 		 */
#define FCC_SDTR_NOPARITY       0x00            /* Disable Parity 	 */
#define FCC_SDTR_PARITY         0x10            /* Enable Parity 	 */
#define FCC_SDTR_5BITS          0x00            /* 5 data bits 		 */
#define FCC_SDTR_6BITS          0x04            /* 6 data bits 		 */
#define FCC_SDTR_7BITS          0x08            /* 7 data bits 		 */
#define FCC_SDTR_8BITS          0x0c            /* 8 data bits 		 */
#define FCC_SDTR_SYNC           0x00            /* Synchronous mode 	 */
#define FCC_SDTR_1XCLK          0x01            /* Trans/Rec Clock x1 	 */
#define FCC_SDTR_1_16XCLK       0x02            /* Trans/Rec Clock x1/16 */
#define FCC_SDTR_1_64XCLK       0x03            /* Trans/Rec Clock x1/64 */

/* Command Register */
#define FCC_SDTR_EHM            0x80            /* Executes enter hunt mode */
#define FCC_SDTR_RESET          0x40            /* Executes reset 	    */
#define FCC_SDTR_RTS_HIGH       0x00            /* RTS output High level    */
#define FCC_SDTR_RTS_LOW        0x20            /* RTS output Low level     */
#define FCC_SDTR_EFR            0x10            /* Resets all error flags   */
#define FCC_SDTR_BREAK          0x08            /* TRNDT pin outputs Low    */
#define FCC_SDTR_REC_DISABLE    0x00            /* Receive disable 	    */
#define FCC_SDTR_REC_ENABLE     0x04            /* Receive enable 	    */
#define FCC_SDTR_DTR_HIGH       0x00            /* DTR output High level    */
#define FCC_SDTR_DTR_LOW        0x02            /* DTR output Low level     */
#define FCC_SDTR_TRN_DISABLE    0x00            /* Transmit disabled 	    */
#define FCC_SDTR_TRN_ENABLE     0x01            /* Transmit enabled 	    */

#define FCC_TX_START            FCC_SDTR_RTS_LOW 	| \
				FCC_SDTR_EFR		| \
                                FCC_SDTR_REC_ENABLE 	| \
                                FCC_SDTR_DTR_LOW 	| \
                                FCC_SDTR_TRN_ENABLE

/* Status Register */

#define FCC_SDTR_STAT_DSR       0x80            /* DSR status mask	     */
#define FCC_SDTR_STAT_SYBRK     0x40            /* Syc ch(sync)/Break(async) */
#define FCC_SDTR_STAT_FERR      0x20            /* Framing error mask	     */
#define FCC_SDTR_STAT_ORUN      0x10            /* Overrun error mask	     */
#define FCC_SDTR_STAT_PERR      0x08            /* Parity error mask	     */
#define FCC_SDTR_STAT_TEMP      0x04            /* Trans buff/shft reg stat  */
#define FCC_SDTR_STAT_RRDY      0x02            /* Receive buffer status     */
#define FCC_SDTR_STAT_TRDY      0x01            /* Transmit buffer status    */

#ifndef	_ASMLANGUAGE

#include "tyLib.h"

/* serial device descriptor */

typedef struct		/* TY_CO_DEV */
    {
    TY_DEV	tyDev;
    BOOL	created;	/* TRUE if device has already been created */
    volatile int *cp;		/* control port I/O address */
    volatile int *dp;		/* data port I/O address */
    char	numChannels;	/* number of channels to support */
    } TY_CO_DEV;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

IMPORT	void	tyCoIntTx (int channel);
IMPORT	void	tyCoIntRx (int channel);
IMPORT	UINT	sysAsiGet (void * address);
IMPORT	void	sysAsiSet (void * address, UINT word);
IMPORT	UINT	sys940AsiGeth (void * address);
IMPORT	void	sys940AsiSeth (void * address, UINT word);
IMPORT	STATUS	sysBaudSet (int baudRate);

#else	/* __STDC__ */

IMPORT	void	tyCoIntTx ();
IMPORT	void	tyCoIntRx ();
IMPORT	UINT	sysAsiGet ();
IMPORT	void	sysAsiSet ();
IMPORT	UINT	sys940AsiGeth ();
IMPORT	void	sys940AsiSeth ();
IMPORT	STATUS	sysBaudSet ();

#endif	/* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCmb86940h */
