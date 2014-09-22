/* usrSmObj.c - shared memory object initialization */

/* Copyright 1992-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,13feb02,mas  fixed staged delay/check for slave node startup (SPR 73189)
01d,02oct01,mas  added staged delay/check for slave node startup (SPR 62128)
01c,13sep01,jws  fix smObjSpinTries initialization (SPR68418)
01b,20jan99,scb  modified to use "sm=" before SM_ANCHOR_ADRS (23035)
01a,16feb97,ms   based on old 01i version.
*/

/*
DESCRIPTION
This file is used to configure and initialize the VxWorks shared memory
object support.  This file is included by usrConfig.c.

SEE ALSO: usrExtra.c

NOMANUAL
*/

/******************************************************************************
*
* usrSmObjInit - initialize shared memory objects
*
* This routine initializes the shared memory objects facility.  It sets up
* the shared memory objects facility if called from processor 0.
* Then it initializes a shared memory descriptor and calls smObjAttach()
* to attach this CPU to the shared memory object facility.
*
* When the shared memory pool resides on the local CPU dual ported memory,
* SM_OBJ_MEM_ADRS must be set to NONE in configAll.h and the shared memory
* objects pool is allocated from the VxWorks system pool.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* INTERNAL
* The delayed start for slave processors used below is required.  The anchor
* may not yet be mapped to the bus.  So, probing of shared memory locations is
* used to overcome Bus Errors which occur on many boards if the slave accesses
* shared memory (SM) before the master has finished initializing it.  The code
* here simply delays access to the SM region until the SM master has finished
* initializing it.
*
* The method used is to repetitively probe key locations in the SM region
* after delay periods until valid values are detected or a time-out occurs.
* The initial delay period is set based on processor number.  (The master
* processor does not delay.)  If the first probe of a location fails, an
* exponential increase in delay period is used to reduce bus contention on
* subsequent probes.
*
* This method is no better than receiving raw BERRs and does reduce bus
* contention and the number of BERRs.
*
* NOMANUAL
*/

STATUS usrSmObjInit 
    (
    char * bootString		/* boot parameter string */
    )
    {
    char *           smAnchor;		/* anchor address */
    char *           smObjFreeAdrs;	/* free pool address */
    int              smObjMemSize;	/* pool size */
    BOOT_PARAMS      params;		/* boot paramters */
    BOOL             allocatedPool;	/* TRUE if pool is maloced */
    SM_OBJ_PARAMS    smObjParams;	/* smObj setup parameters */
    char 	     bb;		/* bit bucket for vxMemProbe */
    int              tics;		/* SM probe delay period */
    UINT             temp;		/* temp for smUtilMemProbe() */
    UINT             maxWait   = SM_MAX_WAIT;
    char *           cp;
    SM_OBJ_MEM_HDR * pSmObjHdr = NULL;	/* ptr to SMO header */

    allocatedPool = FALSE;

    /* Check for hardware test and set availability */

    if (SM_TAS_TYPE != SM_TAS_HARD)
        {
        printf ("\nError initializing shared memory objects, ");
	printf ("hardware test-and-set required.\n");
        return (ERROR);
        }

    if (smObjLibInit () == ERROR)	/* initialize shared memory objects */
	{
        printf("\nERROR smObjLibInit : shared memory objects already initialized.\n");
	return (ERROR);
	}

    if (bootString == NULL)
        bootString = BOOT_LINE_ADRS;

    /* interpret boot command */

    if (usrBootLineCrack (bootString, &params) != OK)
        return (ERROR);

    /* set processor number: may establish vme bus access, etc. */

    if (_procNumWasSet != TRUE)
	{
    	sysProcNumSet (params.procNum);
	_procNumWasSet = TRUE;
	}

    /* if we booted via the sm device use the same anchor address for smObj */

    if (strncmp (params.bootDev, "sm=", 3) == 0)
        {
        if (bootBpAnchorExtract (params.bootDev, &smAnchor) < 0)
            {
	    printf ("\nError initializing shared memory objects, invalid ");
            printf ("anchor address specified: \"%s\"\n", params.bootDev);
            return (ERROR);
            }
        }
    else
        {
	smAnchor = (char *) SM_ANCHOR_ADRS;         /* default anchor */
        }

    /* If we are shared memory master CPU, set up shared memory object (SMO) */

    if (params.procNum == SM_MASTER)
        {
        smObjFreeAdrs = (char *) SM_OBJ_MEM_ADRS;
	smObjMemSize  = SM_OBJ_MEM_SIZE;

        /* allocate the shared memory object pool if needed */

        if (smObjFreeAdrs == (char *) NONE)
            {
            /* check cache configuration - must be read and write coherent */

	    if (!CACHE_DMA_IS_WRITE_COHERENT() || !CACHE_DMA_IS_READ_COHERENT())
                {
		printf ("usrSmObjInit - cache coherent buffer not available. Giving up.  \n");
		return (ERROR);
                }

            allocatedPool = TRUE;

            smObjFreeAdrs = (char *) cacheDmaMalloc (SM_OBJ_MEM_SIZE);

            if (smObjFreeAdrs == NULL)
		{
		printf ("usrSmObjInit - cannot allocate shared memory pool. Giving up.\n");
                return (ERROR);
		}
            }

        if (!allocatedPool)
            {
            /* free memory pool must be behind the anchor */
            smObjFreeAdrs += sizeof (SM_ANCHOR);

	    /* adjust pool size */
	    smObjMemSize = SM_OBJ_MEM_SIZE - sizeof (SM_ANCHOR);
            }

	/* probe anchor address */

	if (vxMemProbe (smAnchor, VX_READ, sizeof (char), &bb) != OK)
	    {
	    printf ("usrSmObjInit - anchor address %#x unreachable. Giving up.\n", (unsigned int) smAnchor);
	    return (ERROR);
	    }

	/* probe beginning of shared memory */

	if (vxMemProbe (smObjFreeAdrs, VX_WRITE, sizeof (char), &bb) != OK)
	    {
	    printf ("usrSmObjInit - shared memory address %#x unreachable. Giving up.\n", (unsigned int) smObjFreeAdrs);
	    return (ERROR);
	    }

        /* set up shared memory objects */
        
        smObjSpinTries = SM_OBJ_MAX_TRIES; /* must do before smObjSetup() */

        smObjParams.allocatedPool = allocatedPool;
        smObjParams.pAnchor       = (SM_ANCHOR *) smAnchor;
        smObjParams.smObjFreeAdrs = (char *) smObjFreeAdrs;
        smObjParams.smObjMemSize  = smObjMemSize;
        smObjParams.maxCpus       = DEFAULT_CPUS_MAX;
        smObjParams.maxTasks      = SM_OBJ_MAX_TASK;
        smObjParams.maxSems       = SM_OBJ_MAX_SEM;
        smObjParams.maxMsgQueues  = SM_OBJ_MAX_MSG_Q;
        smObjParams.maxMemParts   = SM_OBJ_MAX_MEM_PART;
        smObjParams.maxNames      = SM_OBJ_MAX_NAME;

        if (smObjSetup (&smObjParams) != OK)
            {
            if (errno == S_smObjLib_SHARED_MEM_TOO_SMALL)
               printf("\nERROR smObjSetup : shared memory pool too small.\n");

            if (allocatedPool)
                free (smObjFreeAdrs);			/* cleanup */

            return (ERROR);
            }
        }

    /*
     * Else, we are not the master CPU, so wait until the anchor and SMO
     * header regions are accessible (the master CPU has initialized them)
     * before continuing.
     */

    else
        {
        /* first wait for valid anchor region */

        tics = params.procNum;
        for (tics <<= 1; tics < maxWait; tics <<= 1)
            {
            smUtilDelay (NULL, tics);
            if ((smUtilMemProbe (smAnchor, VX_READ, sizeof (UINT),
                                 (char *)&temp) == OK) &&
                (ntohl (temp) == SM_READY))
                {
                break;
                }
            }

        if (tics >= maxWait)
            {
            printf ("usrSmObjInit: anchor @ %p unreachable. Giving up.\n",
                    smAnchor);
            return (ERROR);
            }

        /* Finally, wait for master to init SM Object facility */

        tics = params.procNum;
        cp = (char *)(&((SM_ANCHOR *)smAnchor)->smObjHeader);
        for (tics <<= 1; tics < maxWait; tics <<= 1)
            {
            if ((smUtilMemProbe (cp, VX_READ, sizeof (UINT), (char *)&temp)
                              == OK) &&
                (temp != (UINT)~0)   &&
                (temp != 0))
                {
                break;
                }
            smUtilDelay (NULL, tics);
            }

        pSmObjHdr = SM_OFFSET_TO_LOCAL (ntohl (temp), (int)smAnchor,
                                        SM_OBJ_MEM_HDR *);

        cp = (char *)(&pSmObjHdr->initDone);
        for ( ; tics < maxWait; tics <<= 1)
            {
            if ((smUtilMemProbe (cp, VX_READ, sizeof (UINT), (char *)&temp)
                              == OK) &&
                (ntohl (temp) == TRUE))
                {
                break;
                }
            smUtilDelay (NULL, tics);
            }

        if (tics >= maxWait)
            {
            printf ("usrSmObjInit: Error: time out awaiting SM Object init\n");
            return (ERROR);
            }
        }

    /*
     * initialize shared memory descriptor
     *
     *  Note that SM_OBJ_MAX_TRIES is passed to smObjInit(),
     *  and is used to set the global int smObjSpinTries.
     *  This has already been done above to fix SPR68418.
     */

    smObjInit (&smObjDesc, (SM_ANCHOR *) smAnchor, sysClkRateGet (),
               SM_OBJ_MAX_TRIES, SM_INT_TYPE, SM_INT_ARG1,
	       SM_INT_ARG2, SM_INT_ARG3);

    /* attach to shared memory object facility */

    printf ("Attaching shared memory objects at %#x... ", (int) smAnchor);

    if (smObjAttach (&smObjDesc) != OK)
	{
	printf ("failed: errno = %#x.\n", errno);
        return (ERROR);
	}

    printf ("done\n");
    return (OK);
    }

