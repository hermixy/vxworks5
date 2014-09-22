/*
**  File:     dptarget.h    
**  Version:  1.0.1
**
**  Description: Contains defines and prototypes
**               for the NetROM dualport communication protocol
**
**      Copyright (c) 1996  Applied Microsystems Corp.
**                          All Rights Reserved
**
**  Modification History:
**  1/14/97......Rewrote macros using 8-byte addr span, MPH
**    
*/
#ifndef _dptarget_h
#define	_dptarget_h

/* Target-native data storage */
typedef unsigned long   uInt32;		/* 32 bits unsigned */
typedef long            Int32;          /* 32 bits signed */
typedef unsigned short  uInt16;		/* 16 bits unsigned */
typedef short           Int16;		/* 16 bits signed */
typedef unsigned char   uChar;		/* 8 bits unsigned */

/* prevents private stuff from appearing in the link map */
#define	STATIC	         static
#define DP_DATA_SIZE     60      /* msg data size */

/*
** Dualport access macros
**
** How the Read-read macros work:
**  1. Enable RR protocol and latch start address for write
**       a. Read the RR_Enable address
**       b. Read to latch bits 0-7  of start address
**       c. Read to latch bits 8-10 of start address
**  3. Latch the data by reading (Step 3 may be repeated n times,
**     to write n consecutive bytes, NR will autoincrement the 
**     destination address.)
**  4. Disable RR protocol by reading the RR_Disable address
**
** Notes:
**  1. volatile "uChar RR_dummy" must be declared in dptarget.c for 
**     RR_ENABLE RR_WRITE_BYTE and RR_DISABLE
**  2. Start address and data are latched using an 8-byte span.
**     This lets the RR protocol work on targets that perform burst-reads.
**     Actual span used = ROMWORDWIDTH * 8
*/

/* ROMWORDWIDTH==1, so use 8-byte span */
#if (ROMWORDWIDTH == 1)
  #define ADR1_MASK(addr) (((addr) & 0x000000FF) << 3)
  #define ADR2_MASK(addr) (((addr) & 0x00000700) >> 5)
  #define ADR3_MASK(addr) (addr)
  #define DATA_MASK(data) ((data) << 3)
  #define READ_MASK(x)    (* ((volatile uChar*)(x)))

/* ROMWORDWIDTH==2, so use 16-byte span */
/* Podorder must be 0 or 0:1 with this READ_MASK */
#elif (ROMWORDWIDTH == 2)
  #define ADR1_MASK(addr) (((addr) & 0x000000FF) << 4)
  #define ADR2_MASK(addr) (((addr) & 0x00000700) >> 4)
  #define ADR3_MASK(addr) ((addr) << 1)
  #define DATA_MASK(data) ((data) << 4)
  #define READ_MASK(x)    (uChar)(* ((volatile uInt16*)(x))) 

/* ROMWORDWIDTH==4, so use 32-byte span */
/* Podorder must be 0:1 or 0:1:2:3 with this READ_MASK */
#elif (ROMWORDWIDTH == 4)
  #define ADR1_MASK(addr) (((addr) & 0x000000FF) << 5)
  #define ADR2_MASK(addr) (((addr) & 0x00000700) >> 3)
  #define ADR3_MASK(addr) ((addr) << 2)
  #define DATA_MASK(data) ((data) << 5)
  #define READ_MASK(x)    (uChar)(* ((volatile uInt32*)(x)))

/* ROMWORDWIDTH Error */
#else
#error Invalid romwordwidth(=ROMWORDWIDTH) specified in dpconfig.h
#endif

/* A. Turn on Read-read protocol and latch start address for Write */
/* B. Latch lower 8-bits of start address for Write */
/* C. Latch bits 10-8 of start address for Write */
#define RR_ENABLE(cp, addr) { \
  RR_dummy = (* ((volatile uChar *) ((cp)->rr_enable ))); \
  RR_dummy = (* ((volatile uChar *) \
		 ((cp)->dpbase_plus_index + ADR1_MASK(addr)))); \
  RR_dummy = (* ((volatile uChar *) \
		 ((cp)->dpbase_plus_index + ADR2_MASK(addr)))); \
            }

/* Use Read-read protocol to write one byte to current address */
/* After the write, destination address is incremented in hardware */
#define RR_WRITE_BYTE(cp, val) \
  RR_dummy = (*((volatile uChar*)((cp)->dpbase_plus_index + DATA_MASK(val))))

/* Turn off Read-read protocol */
#define RR_DISABLE(cp) \
  RR_dummy = (* ((volatile uChar *) ((cp)->rr_disable )))

#define NR_READ_BYTE(cp, addr) \
  READ_MASK( (cp)->dpbase_plus_index + ADR3_MASK(addr) )

#if (READONLY_TARGET == False)
#define NR_WRITE_BYTE(cp, addr, val) \
  (* ((volatile uChar*)((cp)->dpbase_plus_index + ADR3_MASK(addr) )) = (val))
#else
 /* Can't write, so use the Read-read protocol */
#define NR_WRITE_BYTE(cp, addr, val) { \
   RR_ENABLE( (cp), (addr) );  \
   RR_WRITE_BYTE( (cp), (val) );  \
   RR_DISABLE( (cp) ); }
#endif /* READONLY_TARGET == False */



/*  ------------------------------------------------------
**          Prototypes for dualport communication
**  ------------------------------------------------------
*/
Int16  nr_ConfigDP   ( uInt32, Int16, Int16 );
Int16  nr_SetBlockIO ( uInt16, uInt16 );
Int16  nr_ChanReady  ( uInt16 );    
Int16  nr_Poll       ( uInt16 );
Int16  nr_Getch      ( uInt16 );	    
Int16  nr_Putch      ( uInt16, char );
Int16  nr_FlushTX    ( uInt16 );   
Int16  nr_GetMsg     ( uInt16, char*, uInt16, uInt16* );
Int16  nr_PutMsg     ( uInt16, char*, uInt16 );
Int16  nr_Resync     ( uInt16 );
Int16  nr_Reset      ( uInt16 );
Int16  nr_IntAck     ( uInt16 );
Int16  nr_Cputs      ( uInt16, char*, uInt16 );
Int16  nr_SetMem     ( uInt16, uInt32, char*, uInt16 );
Int16  nr_TestComm   ( uInt16, uInt16, uInt16 );
Int16  nr_IntAck     ( uInt16 );
Int16  nr_Cputs      ( uInt16, char*, uInt16 );
Int16  nr_SetMem     ( uInt16, uInt32, char*, uInt16 );
void   nr_SetEmOffOnWrite ( void );

/* Return error codes */
#define Err_NoError        0
#define Err_WouldBlock    -1
#define Err_BadChan       -2
#define Err_BadLength     -3
#define Err_BadAddress    -4
#define Err_BadCommand    -5
#define Err_NotReady      -6

/* Structures for out-of-band messages */
typedef struct _dpSetMemStruct {
      uInt32   addr;
      uChar    data[DP_DATA_SIZE-6];
    } DpSetMemBuf;

typedef struct _dpCputsStruct {
      uChar    data[DP_DATA_SIZE-2];
    } DpCputsBuf;

typedef struct OOBM {
  uInt16   flags;
  uInt16   size;
  uInt16   cmd; 
  union{
    DpSetMemBuf setmem;
    DpCputsBuf  cputs;
  } ftn;
} DP_OOB_MSG;

#endif	/* _dptarget_h */













