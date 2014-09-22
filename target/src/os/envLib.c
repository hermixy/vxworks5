/* envLib.c - environment variable library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01k,22jan93,jdi  documentation cleanup for 5.1.
01j,19jul92,smb  added some ANSI documentation.
		 made getenv parameter list ANSI.
01i,26may92,rrr  the tree shuffle
01h,19nov91,rrr  shut up some ansi warnings.
01g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01f,10aug90,kdl  added forward declaration for envDestroy.
01e,01aug90,jcf  cleanup.
01d,30jul90,rdc  ick, more lint.
01c,18jul90,rdc  lint.
01b,15jul90,dnw  fixed to coerce malloc() to (char*) where necessary
01a,12jul90,rdc  written.
*/

/*
This library provides a UNIX-compatible environment variable facility.  
Environment variables are created or modified with a call to putenv():
.CS
    putenv ("variableName=value");
.CE
The value of a variable may be retrieved with a call to getenv(), which returns
a pointer to the value string.

Tasks may share a common set of environment variables, or they may optionally
create their own private environments, either automatically when the task
create hook is installed, or by an explicit call to envPrivateCreate().
The task must be spawned with the VX_PRIVATE_ENV option set to receive a
private set of environment variables.  Private environments created by the
task creation hook inherit the values of the environment of the task that called
taskSpawn() (since task create hooks run in the context of the calling task).

INCLUDE FILES: envLib.h

SEE ALSO: UNIX BSD 4.3 manual entry for \f3environ(5V)\f1,
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: General Utilities (stdlib.h)"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "logLib.h"

#define ENV_NENTRIES_TO_ALLOC 50


/* globals */

char **ppGlobalEnviron; 	/* global envivonment variable table */

/* locals */

LOCAL int envTblSize;		/* global table size */
LOCAL int nEntries;		/* number of entries in global table */


/* forward static functions */

static void envCreateHook (WIND_TCB *pNewTcb);
static STATUS envDuplicate (char ** srcEnv, int srcEnvTblSize, int
		srcNEnvVarEntries, WIND_TCB *pTcb);
static void envDestroy (char ** ppEnv, int nEnvVarEntries);
static void envDeleteHook (WIND_TCB *pTcb);
static char ** envFind (char *name, int nameLen);


/*********************************************************************
*
* envLibInit - initialize environment variable facility
*
* If <installHooks> is TRUE, task create and delete hooks are installed that
* will optionally create and destroy private environments for the task being
* created or destroyed, depending on the state of VX_PRIVATE_ENV in the task
* options word.  If <installHooks> is FALSE and a task requires a private
* environment, it is the application's responsibility to create and destroy
* the private environment, using envPrivateCreate() and envPrivateDestroy().
*
* RETURNS: OK, or ERROR if an environment cannot be allocated or the hooks
* cannot be installed.
*/

STATUS envLibInit
    (
    BOOL installHooks
    )
    {
    /* allocate the global environ array */
    ppGlobalEnviron = (char **) calloc (ENV_NENTRIES_TO_ALLOC, sizeof (char *));

    if (ppGlobalEnviron == NULL)
	return (ERROR);

    envTblSize = ENV_NENTRIES_TO_ALLOC;
    nEntries   = 0;

    /* optionally install task create/delete hooks */

    if (installHooks)
	{
	if(taskCreateHookAdd ((FUNCPTR)envCreateHook) != OK)
	    return (ERROR);

	if (taskDeleteHookAdd ((FUNCPTR)envDeleteHook) != OK)
	    return (ERROR);
	}

    return (OK);
    }

/*********************************************************************
*
* envCreateHook - create a private environment for the new task
*
* installed by envLibInit to optionally create a private environment
* for tasks that have VX_PRIVATE_ENV set in their task options word.
*/

LOCAL void envCreateHook
    (
    WIND_TCB *pNewTcb
    )
    {
    char **	ppSrcEnviron;
    int		srcEnvTblSize;
    int		srcNEnvVarEntries;
    WIND_TCB *	pMyTcb;


    /* if VX_PRIVATE_ENV is set, copy spawning task's environment to a
       private environment and stash a pointer to it in the new task's tcb */

    if (pNewTcb->options & VX_PRIVATE_ENV)
	{
	pMyTcb = taskTcb (0);

	if ((ppSrcEnviron = pMyTcb->ppEnviron) == NULL)
	    {
	    ppSrcEnviron = ppGlobalEnviron;
	    srcEnvTblSize = envTblSize;
	    srcNEnvVarEntries = nEntries;
	    }
	else
	    {
	    srcEnvTblSize = pMyTcb->envTblSize;
	    srcNEnvVarEntries = pMyTcb->nEnvVarEntries;
	    }

	/* allocate the private environ array - make it the same size
	 * as the "current" environ array.
	 */

	if (envDuplicate (ppSrcEnviron, srcEnvTblSize, srcNEnvVarEntries,
			  pNewTcb) == ERROR)
	    {
	    logMsg ("evnCreateHook: couldn't create private environment!\n",
		    0,0,0,0,0,0);

	    /* turn VX_PRIVATE_ENV off in options word so we don't deallocate
	     * an environment that doesn't exist when the task exits.
	     */

	    taskOptionsSet ((int) pNewTcb, VX_PRIVATE_ENV, 0);
	    return;
	    }

	}
    else
	{
	/* set the environment pointer in the tcb to NULL so the global
	 * environment will be used.
	 */

	pNewTcb->ppEnviron = NULL;
	}
    }

/*********************************************************************
*
* envDuplicate - duplicate the given environment
*
* create a new environment and copy the information in the given
* source environment.
*
* RETURNS: OK, or ERROR if not enough memory
*/

LOCAL STATUS envDuplicate
    (
    FAST char **srcEnv,         /* environment table to duplicate */
    int srcEnvTblSize,          /* table size */
    int srcNEnvVarEntries,      /* N entries in use */
    WIND_TCB *pTcb              /* tcb of task to receive duplicate env */
    )
    {
    FAST char **privateEnv;
    FAST int i;

    privateEnv = (char **) calloc ((unsigned) srcEnvTblSize, sizeof (char *));

    if (privateEnv == (char **) NULL)
	return (ERROR);

    pTcb->envTblSize	 = srcEnvTblSize;
    pTcb->nEnvVarEntries = 0;
    pTcb->ppEnviron	 = privateEnv;


    /* copy the source environment to the new environment */

    for (i = 0; i < srcNEnvVarEntries; i++)
	{
	FAST char *destString;
	FAST char *srcString;

	srcString = srcEnv[i];
	destString = (char *) malloc ((unsigned) strlen (srcString) + 1);

	if (destString == NULL)
	    {
	    envDestroy (privateEnv, pTcb->nEnvVarEntries);
	    return (ERROR);
	    }

	strcpy (destString, srcString);
	privateEnv[i] = destString;
	pTcb->nEnvVarEntries++;
	}

    return (OK);
    }

/*********************************************************************
*
* envDestroy - free the given environment
*
* free all the strings and data structures associated with an
* environment.
*/

LOCAL void envDestroy
    (
    char **ppEnv,
    int nEnvVarEntries
    )
    {
    FAST int i;

    for (i = 0; i < nEnvVarEntries; i++)
	{
	free (ppEnv[i]);
	}

    /* deallocate the env array */

    free ((char *) ppEnv);
    }

/*********************************************************************
*
* envDeleteHook - delete a task's private environment
*
* installed by envLibInit to optionally delete private environments
* when a task exits.
*/

LOCAL void envDeleteHook
    (
    WIND_TCB *pTcb         /* pointer to deleted task's TCB */
    )
    {
    /* if VX_PRIVATE_ENV is set, destroy the environment */

    if (pTcb->options & VX_PRIVATE_ENV)
	if (pTcb->ppEnviron != NULL)
	    envDestroy (pTcb->ppEnviron, pTcb->nEnvVarEntries);
    }

/*********************************************************************
*
* envPrivateCreate - create a private environment
*
* This routine creates a private set of environment variables for a specified
* task, if the environment variable task create hook is not installed.
*
* RETURNS: OK, or ERROR if memory is insufficient.
*
* SEE ALSO: envLibInit(), envPrivateDestroy()
*/

STATUS envPrivateCreate
    (
    int taskId,         /* task to have private environment */
    int envSource       /* -1 = make an empty private environment */
                        /* 0 = copy global env to new private env */
                        /* taskId = copy the specified task's env */
    )
    {
    char **ppPrivateEnv;
    FAST WIND_TCB *pTcb;

    /* create specified environment and set VX_PRIVATE_ENV in tcb */

    switch (envSource)
	{
	case (-1):
	    ppPrivateEnv = (char **)
			   calloc (ENV_NENTRIES_TO_ALLOC, sizeof (char *));

	    if (ppPrivateEnv == NULL)
		return (ERROR);

	    pTcb = taskTcb (taskId);

	    if (pTcb == NULL)
		return (ERROR);

	    pTcb->envTblSize	 = ENV_NENTRIES_TO_ALLOC;
	    pTcb->nEnvVarEntries = 0;
	    pTcb->ppEnviron	 = ppPrivateEnv;
	    break;

	case (0):
	    /* duplicate the global environment */

	    if (envDuplicate (ppGlobalEnviron, envTblSize,
			      nEntries, (WIND_TCB *) taskIdSelf ()) == ERROR)
		return (ERROR);
	    break;

	default:
	    /* duplicate the given task's environment */

	    if ((pTcb = taskTcb (taskId)) == NULL)
		return (ERROR);

	    if (envDuplicate (pTcb->ppEnviron, pTcb->envTblSize,
		   	      pTcb->nEnvVarEntries, taskTcb (0)) == ERROR)
		return (ERROR);
	}

    taskOptionsSet (taskId, VX_PRIVATE_ENV, VX_PRIVATE_ENV);

    return (OK);
    }

/*********************************************************************
*
* envPrivateDestroy - destroy a private environment
*
* This routine destroys a private set of environment variables that were
* created with envPrivateCreate().  Calling this routine is unnecessary if
* the environment variable task create hook is installed and the task
* was spawned with VX_PRIVATE_ENV.
* 
* RETURNS: OK, or ERROR if the task does not exist.
*
* SEE ALSO: envPrivateCreate()
*/

STATUS envPrivateDestroy
    (
    int taskId		/* task with private env to destroy */
    )
    {
    FAST WIND_TCB *pTcb;

    if ((pTcb = taskTcb (taskId)) == NULL)
	return (ERROR);

    envDestroy (pTcb->ppEnviron, pTcb->nEnvVarEntries);

    pTcb->ppEnviron = NULL;

    taskOptionsSet (taskId, VX_PRIVATE_ENV, 0);

    return (OK);
    }

/*********************************************************************
*
* putenv - set an environment variable
*
* This routine sets an environment variable to a value by altering an
* existing variable or creating a new one.  The parameter points to a string
* of the form "variableName=value".  Unlike the UNIX implementation, the string
* passed as a parameter is copied to a private buffer.
*
* RETURNS: OK, or ERROR if space cannot be malloc'd.
*
* SEE ALSO: envLibInit(), getenv()
*/

STATUS putenv
    (
    char *pEnvString            /* string to add to env */
    )
    {
    FAST char **ppEnvLine;
    char      **ppThisEnviron;
    int		thisEnvironTblSize;
    int 	thisEnvironNEntries;
    FAST char  *pChar = pEnvString;
    WIND_TCB   *pTcb  = taskTcb (0);

    /* find end of environment variable name */

    while ((*pChar != ' ') && (*pChar != '\t') &&
	   (*pChar != '=') && (*pChar != NULL))
	pChar++;

    ppEnvLine = envFind (pEnvString, pChar - pEnvString);

    /* new environment variable? */

    if (ppEnvLine == NULL)
	{
	if (pTcb->ppEnviron == NULL)
	    { 					/* use global environment */
	    ppThisEnviron	  = ppGlobalEnviron;
	    thisEnvironTblSize  = envTblSize;
	    thisEnvironNEntries = nEntries;
	    ppEnvLine		  = &ppGlobalEnviron[nEntries++];
	    }
	else
	    { 					/* use private environment */
	    ppThisEnviron	  = pTcb->ppEnviron;
	    thisEnvironTblSize  = pTcb->envTblSize;
	    thisEnvironNEntries = pTcb->nEnvVarEntries;
	    ppEnvLine		  = &pTcb->ppEnviron[pTcb->nEnvVarEntries++];
	    }

	/* make more room in environ if necessary */

	if (thisEnvironTblSize == thisEnvironNEntries)
	    {
	    FAST char **environBuf;

	    environBuf = (char **) realloc ((char *) ppThisEnviron,
	    (unsigned ) (thisEnvironTblSize + ENV_NENTRIES_TO_ALLOC) *
			 sizeof (char *));

	    if (environBuf == NULL)
		return (ERROR);

	    /* zero out the new area */

	    bzero ((char *) &(environBuf[thisEnvironTblSize]),
		   sizeof (char *) * ENV_NENTRIES_TO_ALLOC);

	    if (pTcb->ppEnviron == NULL)
		{
		ppGlobalEnviron = environBuf;
		envTblSize += ENV_NENTRIES_TO_ALLOC;
		}
	    else
		{
		pTcb->ppEnviron = environBuf;
		pTcb->envTblSize += ENV_NENTRIES_TO_ALLOC;
		}

	    ppEnvLine = &environBuf[thisEnvironNEntries];
	    }
	}
    else
	{
	/* free the string for the given environment variable */
	free (*ppEnvLine);
	}


    /* allocate memory to hold a copy of the string */

    *ppEnvLine = (char *) malloc ((unsigned) strlen (pEnvString) + 1);
    strcpy (*ppEnvLine, pEnvString);

    return (OK);
    }

/*********************************************************************
*
* getenv - get an environment variable (ANSI)
*
* This routine searches the environment list (see the UNIX BSD 4.3 manual
* entry for \f3environ(5V)\f1) for a string of the form "name=value" and
* returns the value portion of the string, if the string is present;
* otherwise it returns a NULL pointer.
*
* RETURNS: A pointer to the string value, or a NULL pointer.
*
* SEE ALSO: envLibInit(), putenv(), UNIX BSD 4.3 manual entry 
* for \f3environ(5V)\f1,
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: General Utilities (stdlib.h)"
*/

char *getenv
    (
    FAST const char *name  /* env variable to get value for */
    )
    {
    FAST char **pEnvLine = envFind (CHAR_FROM_CONST (name), strlen (name));
    FAST char *pValue;

    if (pEnvLine == NULL)
	return (NULL);

    /* advance past the '=' and any white space */

    pValue = *pEnvLine + strlen (name);

    while ((*pValue == ' ') || (*pValue == '\t') || (*pValue == '='))
	pValue++;

    return (pValue);
    }

/*********************************************************************
*
* envFind - find an environment variable
*
*/

LOCAL char **envFind
    (
    FAST char *name,
    FAST int nameLen
    )
    {
    FAST char **envVar;
    WIND_TCB *pTcb = taskTcb (0);
    FAST int i;
    FAST int nEnvEntries;
    FAST char endChar;

    if (pTcb->ppEnviron == NULL)
	{
	envVar = ppGlobalEnviron;
	nEnvEntries = nEntries;
	}
    else
	{
	envVar = pTcb->ppEnviron;
	nEnvEntries = pTcb->nEnvVarEntries;
	}

    for (i = 0; i < nEnvEntries; envVar++, i++)
	if (strncmp (name, *envVar, nameLen) == 0)
	    {
	    /* make sure it's not just a substring */
	    endChar = (*envVar)[nameLen];
	    if ((endChar == ' ') || (endChar == '\t') ||
	       (endChar == '=') || (endChar == NULL))
		return (envVar);
	    }

    return (NULL);	/* not found */
    }

/*********************************************************************
*
* envShow - display the environment for a task
*
* This routine prints to standard output all the environment variables for a
* specified task.  If <taskId> is NULL, then the calling task's environment
* is displayed.
*
* RETURNS: N/A
*/

void envShow
    (
    int taskId	/* task for which environment is printed */
    )
    {
    FAST char **ppEnvVar;
    FAST int    i;
    FAST int    nEnvEntries;
    WIND_TCB   *pTcb = taskTcb (taskId);

    if (pTcb->ppEnviron == NULL)
	{
	printf ("(global environment)\n");
	ppEnvVar    = ppGlobalEnviron;
	nEnvEntries = nEntries;
	}
    else
	{
	printf ("(private environment)\n");
	ppEnvVar    = pTcb->ppEnviron;
	nEnvEntries = pTcb->nEnvVarEntries;
	}

    for (i = 0; i < nEnvEntries; ppEnvVar++, i++)
	printf ("%d: %s\n", i, *ppEnvVar);

    }
