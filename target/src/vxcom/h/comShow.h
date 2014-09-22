/* comShow.h - VxWorks COM show routines public API */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01d,04oct01,nel  Correct comTrackShow def with and extern "C".
01c,25sep01,nel  Add method to translate a guid to a string.
01b,06mar00,nel  Added VxComTrack class
01a,24mar99,drm  created

*/

/*

 DESCRIPTION:

 */

#ifndef __INCcomShow_h
#define __INCcomShow_h

#include "vxidl.h"

#ifdef __cplusplus
extern "C" {
#endif

int vxcomRegShow();
int comTrackShow ();

#ifdef __cplusplus
           }
#endif

class VxComTrack
    {
    public:
        VxComTrack ();

        ~VxComTrack ();

        void * addClassInstance
            (
            void          * thisPtr,
            char          * guid,
            char          * name,
            unsigned long   refCount,
	    REFCLSID	    iid 
            );

        void updateClassInstance
            (
            void          * thisPtr,
            unsigned long   refCount
            );

        void addInterface
            (
            void              * thisPtr,
            char              * guid,
            char              * name,
	    REFIID		cls
            );

        int print ();

	const char * findGUID (REFGUID guid);

        static VxComTrack * theInstance ();

        class INTERFACE
            {
            public:
                INTERFACE         * next;
                INTERFACE         * listNext;
                INTERFACE         * listPrev;
                char              * guid;
                char              * name;
                unsigned long       owner;
                unsigned long       magic1;
                void              * thisPtr;
		IID		    iid; /* added here to not break T3 inspector */
            };

        class CLASS
            {
            public:
                CLASS             * next;
                CLASS             * prev;
                void              * thisPtr;
                char              * guid;
                char              * name;
                unsigned long       refCount;
                unsigned long       owner;
                INTERFACE         * interfaces;
                unsigned long       magic1;
		CLSID		    cls; /* added here to not break T3 inspector */
            };

    private:
        enum { MAGIC1 = 0x5aa5aa55 };

        static VxComTrack * s_pTheList;

        CLASS * findClass (void * thisPtr);

        INTERFACE * findInterface
            (
            CLASS * classPtr,
            char * guid
            );

    };

#endif


