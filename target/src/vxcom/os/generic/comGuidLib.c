/* comGuidLib.c - vxcom GUID library */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01d,19nov01,nel  Add delay back into GUID creation routine.
01c,01nov01,nel  SPR#71383. Correct time adjustment algorithm.
01b,27jun01,dbs  fix compiler warnings
01a,21jun01,nel  created

*/

/*

DESCRIPTION

A library for generated GUIDs as defined in the DCE spec. There is 
a protection mechanism against generating GUIDs to quickly for the 
timer granularity to cope with.

*/

typedef struct _GUID
    {
    unsigned long	timeLow;
    unsigned short	timeMid;
    unsigned short	timeHiAndVersion;
    unsigned char	clockSeqHiAndReserved;
    unsigned char	clockSeqLow;
    unsigned char	node[6];
    } GUID;

/*
 * Various constants and masks used in uuid generation.
 */
#define UUID_C_VERSION 			(1)
#define CLOCK_SEQ_LOW_MASK		(0xff)
#define CLOCK_SEQ_HIGH_MASK		(0x3f00)
#define CLOCK_SEQ_HIGH_SHIFT_COUNT	(8)
#define CLOCK_SEQ_FIRST			(1)
#define CLOCK_SEQ_LAST			(0x3fff)	/* same as RAND_MASK */
#define RAND_MASK			(CLOCK_SEQ_LAST)

/* 'adjustment' for backwards clocks */
static int			clockSeq = 0;

/* Last time a UUID was generated or 0 if first time class is exectued. */
static long unsigned long	last = 0;

/* Adjustment to cope with reduced granularity of the clock. */
static int			adjustment = 0;

/* Used to store last uuid generated so that duplicates can be checked for. */
static GUID lastGuid = { 0, };

/*
 * Create's a new clock sequence value. This is a randomnly gerenated field
 * in the UUID which ensures that two UUIDs generated at the same time stand
 * some chance of being unique.
 */
static void newClockSeq (void)
    {
    clockSeq = rand() & RAND_MASK;

    clockSeq = (clockSeq + 1) & CLOCK_SEQ_LAST;
    if (clockSeq == 0)
        {
        clockSeq++;
        }
    }

static long unsigned long clockGet (void)
    {
    long unsigned long	usec;
    long unsigned long	clockReg;

    /* convert seconds to usecs */
    usec = (long unsigned long)comSysTimeGet () * 1000000; 

    if (last == 0)
	{
	newClockSeq();
	last = usec - 1;
	}

    if (usec < last )
	{
	clockSeq = (clockSeq + 1) & 0x1FFF;
	adjustment = 0;
	}
    else if (usec == last)
	{
	if (adjustment >= 999)
	    {
	    newClockSeq ();
	    adjustment = 0;
	    }
	else
	    adjustment++;
	}
    else
	adjustment = 0;

    last = usec;

    clockReg = usec * 10;
    clockReg += adjustment;
    clockReg += 0x01B21DD213814000LL; /* Difference between DTSS UTC and Unix */
    				      /* DTSS starts October 15, 1582. */

    return clockReg;
    }


void comSysGuidCreate 
    (
    void *		result		/* Result that is mapped onto an */
    					/* IDL GUID structure by higher layers*/
    )
    {
    do
        {
        long unsigned long timeNow = clockGet ();
        ((GUID *)result)->timeLow = timeNow & 0xffffffff;
        ((GUID *)result)->timeMid = (timeNow >> 32) & 0xffff;

        ((GUID *)result)->timeHiAndVersion = ((timeNow >> 48) & 0xffff) | 
        				      (UUID_C_VERSION << 12);

        ((GUID *)result)->clockSeqLow = (short) (clockSeq & CLOCK_SEQ_LOW_MASK);
        ((GUID *)result)->clockSeqHiAndReserved =
                ((short) ((clockSeq & CLOCK_SEQ_HIGH_MASK) >> 
                    CLOCK_SEQ_HIGH_SHIFT_COUNT)) | 0x80;

    	comSysAddressGet ( ((GUID *)result)->node );

        /**
         * check to see if the last guid has been duplicated, if it has then
         * wait a bit and try again. this should ensure that the next guid
         * generated is unique.
         */
        if (memcmp (&lastGuid, result, sizeof (GUID)) != 0)
            {
            /* last GUID is different from current GUID so is correct */
            memcpy (&lastGuid, result, sizeof (GUID));
            return;
            }
	taskDelay (1);
        } while (1); /* loop until a vaild GUID is generated */
    }
