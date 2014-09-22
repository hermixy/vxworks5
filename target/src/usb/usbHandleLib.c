/* usbHandleLib.c - handle utility functions */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01b,07mar00,rcb  Add casts as necessary to reflect change of
		 GENERIC_HANDLE from UINT32 to pVOID.
01a,07jun99,rcb  First.
*/

/*
DESCRIPTION

Implements a set of general-purpose handle creation and validation functions.

Using these services, libraries can return handles to callers which can 
subsequently be validated for authenticity.  This provides libraries with
an additional measure of "bullet-proofing."
*/

/* includes */

#include "usb/usbPlatform.h"
#include "usb/ossLib.h"
#include "usb/usbHandleLib.h"	    /* our API */


/* defines */

#define DEFAULT_HANDLES     1024    /* Default number of handles */


/* INDEX() calculates the handle[] index based on a handle number */

#define INDEX(h)    ((((UINT32) h) - 1) % numHandles)


/* typedefs */

/*
 * HDL_DESCR - Describes each handle
 */

typedef struct hdl_descr
    {
    GENERIC_HANDLE handle;	    /* Currently assigned handle */
    UINT32 handleSig;		    /* signature used to validate handle */
    pVOID handleParam;		    /* arbitrary parameter used by caller */
    } HDL_DESCR, *pHDL_DESCR;


/* locals */

LOCAL int initCount = 0;	    /* Initialization nesting count */

LOCAL HDL_DESCR *handles = NULL;    /* Pointer to array of handles */
LOCAL UINT32 numHandles = 0;	    /* Number of handles allocated */
LOCAL UINT32 usedHandles = 0;	    /* Number of handles in use */
LOCAL UINT32 nextHandleNum = 0;     /* Handle numbers always increment */
LOCAL MUTEX_HANDLE mutex = NULL;    /* mutex for handle allocation */


/* functions */

/***************************************************************************
*
* usbHandleInitialize - Initializies the handle utility library
*
* Initializes the handle utility library.  Must be called at least once
* prior to any other calls into the handle utility library.  Calls to 
* usbHandleInitialize() may be nested, allowing multiple clients to use the
* library without requiring that they be coordinated.
*
* <maxHandles> defines the maximum number of handles which should be
* allocated by the library.  Passing a zero in <maxHandles> causes the 
* library to allocate a default number of handles.  <maxHandles> is ignored 
* on "nested" calls to usbHandleInitialize().  
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbHandleLib_OUT_OF_MEMORY
*  S_usbHandleLib_OUT_OF_RESOURCES
*/

STATUS usbHandleInitialize
    (
    UINT32 maxHandles		    /* max handles allocated by library */
    )

    {
    STATUS s = OK;


    /* Check if we're already initialized */

    if (++initCount > 1)
	return OK;


    /* Create the handle control structures */

    if (maxHandles == 0)
	maxHandles = DEFAULT_HANDLES;

    if ((handles = OSS_CALLOC (sizeof (HDL_DESCR) * maxHandles)) == NULL)
	{
	s = ossStatus (S_usbHandleLib_OUT_OF_MEMORY);
	}
    else
	{
	if (OSS_MUTEX_CREATE (&mutex) != OK)
	    {
	    s = ossStatus (S_usbHandleLib_OUT_OF_RESOURCES);
	    }
	else
	    {

	    /* Initialize handle array. */

	    numHandles = maxHandles;
	    usedHandles = 0;
	    nextHandleNum = 0;

	    return OK;
	    }

	OSS_FREE (handles);
	handles = NULL;
	}


    /* Failed to initialize. */

    --initCount;

    return s;
    }


/***************************************************************************
*
* usbHandleShutdown - Shuts down the handle utility library
*
* Shuts down the handle utility library.  When calls to usbHandleInitialize()
* have been nested,  usbHandleShutdown() must be called a corresponding number
* of times.
*
* RETURNS: OK or ERROR
*/

STATUS usbHandleShutdown (void)

    {
    if (initCount > 0)
	{
	if (--initCount == 0)
	    {
	    if (mutex != NULL)
		{
		OSS_MUTEX_DESTROY (mutex);
		mutex = NULL;
		}

	    if (handles != NULL)
		{
		OSS_FREE (handles);
		handles = NULL;
		numHandles = 0;
		}
	    }
	}

    return OK;
    }


/***************************************************************************
*
* usbHandleCreate - Creates a new handle
*
* Creates a new handle.  The caller passes an arbitrary <handleSignature>
* and a <handleParam>.	The <handleSignature> will be used in subsequent
* calls to usbHandleValidate().
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbHandleLib_NOT_INITIALIZED
*  S_usbHandleLib_BAD_PARAM
*  S_usbHandleLib_GENERAL_FAULT
*  S_usbHandleLib_OUT_OF_HANDLES
*/

STATUS usbHandleCreate
    (
    UINT32 handleSignature,	    /* Arbitrary handle signature */
    pVOID handleParam,		    /* Arbitrary handle parameter */
    pGENERIC_HANDLE pHandle	    /* Newly allocated handle */
    )

    {
    STATUS s = OK;


    /* Are we initialized? */

    if (initCount == 0)
	return ossStatus (S_usbHandleLib_NOT_INITIALIZED);


    /* Check parameters */

    if (pHandle == NULL)
	return ossStatus (S_usbHandleLib_BAD_PARAM);


    /* Take the mutex */

    if (OSS_MUTEX_TAKE (mutex, OSS_BLOCK) != OK)
	return ossStatus (S_usbHandleLib_GENERAL_FAULT);


    /* Is there a free handle? */

    if (usedHandles == numHandles) 
	{
	s = ossStatus (S_usbHandleLib_OUT_OF_HANDLES);
	}
    else
	{

	/* Find a free handle and assign a handle number to it */

	*pHandle = NULL;
    
	while (*pHandle == NULL)
	    {

	    /* We don't use handle number 0. (handle numbers are always 1
	    greater than the index into the handles[] array. */

	    if (nextHandleNum == 0)
		++nextHandleNum;

	    if (handles [INDEX (nextHandleNum)].handle == 0)
		{

		/* We found a free handle */

		*pHandle = (GENERIC_HANDLE) nextHandleNum;

		handles [INDEX (nextHandleNum)].handle = (GENERIC_HANDLE) nextHandleNum;
		handles [INDEX (nextHandleNum)].handleSig = handleSignature;
		handles [INDEX (nextHandleNum)].handleParam = handleParam;
		}

	    /* Whether or not we used this handle, we increment nextHandleNum
	    so the next search will always begin after where we left off. */

	    if (++nextHandleNum > numHandles) 
		nextHandleNum = 0;
	    }
	}


    /* Release the mutex and return */

    OSS_MUTEX_RELEASE (mutex);

    return s;
    }


/***************************************************************************
*
* usbHandleDestroy - Destroys a handle
*
* This functions destroys the <handle> created by calling usbHandleCreate().
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbHandleLib_GENERAL_FAULT
*  S_usbHandleLib_BAD_HANDLE
*/

STATUS usbHandleDestroy
    (
    GENERIC_HANDLE handle	    /* handle to be destroyed */
    )

    {
    STATUS s = OK;
    

    /* Take the mutex */

    if (OSS_MUTEX_TAKE (mutex, OSS_BLOCK) != OK)
	return ossStatus (S_usbHandleLib_GENERAL_FAULT);


    /* Is this a valid handle? */

    if (handles [INDEX (handle)].handle != handle)
	{
	s = ossStatus (S_usbHandleLib_BAD_HANDLE);
	}
    else
	{

	/* Release the indicated handle descriptor */

	handles [INDEX (handle)].handle = 0;
	handles [INDEX (handle)].handleSig = 0;
	handles [INDEX (handle)].handleParam = 0;   
	}


    /* Release the mutext and return */

    OSS_MUTEX_RELEASE (mutex);

    return s;
    }


/***************************************************************************
*
* usbHandleValidate - Validates a handle
*
* This function validates <handle>.  The <handle> must match the
* <handleSignature> used when the handle was originally created.  If the
* handle is valid, the <pHandleParam> will be returned.
*
* RETURNS: OK or ERROR
*
* ERRNO:
*  S_usbHandleLib_NOT_INITIALIZED
*  S_usbHandleLib_BAD_HANDLE
*/

STATUS usbHandleValidate
    (
    GENERIC_HANDLE handle,	    /* handle to be validated */
    UINT32 handleSignature,	    /* signature used to validate handle */
    pVOID *pHandleParam 	    /* Handle parameter on return */
    )

    {
    
    /* Are we initialized? */

    if (initCount == 0)
	return ossStatus (S_usbHandleLib_NOT_INITIALIZED);


    /* Is the indicated handle valid? */

    if (handles [INDEX (handle)].handle != handle ||
	handles [INDEX (handle)].handleSig != handleSignature)
	return ossStatus (S_usbHandleLib_BAD_HANDLE);


    /* Handle is valid.  Return param associated with handle */

    if (pHandleParam != NULL)
	*pHandleParam = handles [INDEX (handle)].handleParam;

    return OK;
    }


/* End of file. */

