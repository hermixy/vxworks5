/* vxdcomExtent.h - additional data to be passed in an ORPC_EXTENT (VxDCOM) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01i,28feb00,dbs  fix extent size
01h,18feb00,dbs  fix compilation issues
01g,03aug99,drm  Changing long to int.
01f,21jul99,drm  Removing init() function from class declaration.
01e,15jul99,drm  Removing separate default constructor.
01d,14jul99,aim  fix ctor initialisation
01c,17may99,drm  changing VXDCOM_EXTENT to be a derived class of ORPC_EXTENT
01b,14may99,drm  changed 'int priority' to 'long priority'
01a,12may99,drm  created

*/

#ifndef __INCvxdcomExtent_h
#define __INCvxdcomExtent_h

// includes

#include "orpc.h"
#include "dcomExtent.h"

class VXDCOM_EXTENT : public ORPC_EXTENT
    {
  public:
    VXDCOM_EXTENT (int priority = 0);
  
    void setPriority (int priority);
    int  getPriority ();

  private:
    VXDCOMEXTENT	extent;
    };

#endif // __INCvxdcomExtent_h


