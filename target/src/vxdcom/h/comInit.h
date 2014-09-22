/* comInit.h - VxWorks COM Library Init API */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01a,01Mar01,nel  created 

*/

/*

 DESCRIPTION:

 This header defines the library init functions for both the COM library
 and DCOM library.

 */

#ifndef __INCcomInit_h
#define __INCcomInit_h


int comLibInit ();

int dcomLibInit
    (
    int			bstrPolicy,
    int			authnLevel,
    unsigned int	priority,
    unsigned int	numStatic,
    unsigned int	numDynamic,
    unsigned int	stackSize,
    int			clientPrioPropagation,
    int			objectExporterPortNumber
    );

#endif
