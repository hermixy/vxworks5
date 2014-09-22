/* comStl.h */

/*
modification history
--------------------
01q,22feb02,nel  SPR#73592. Remove 68K arch specific fix for old compiler bug.
01p,07jan02,nel  Remove dependency for stl_config.h for ARM/diab compilers.
01o,10dec01,dbs  diab build
01n,13nov01,nel  Add STL_LIST.
01m,30jul01,dbs  add STL_MAP_LL etc for non-VxWorks builds
01l,24jul01,dbs  retain allocators only for T2/VxWorks5.4 build
01k,18jul01,dbs  re-instate allocators
01j,16jul01,dbs  remove vxdcom allocators completely
01i,27jun01,dbs  fix include paths and names
01h,21jun01,dbs  fix up new name, tidy up allocator stuff
01g,31may00,sn   commented fact that allocators need to be updated
                 to Standard C++ ones (currently use SGI syntax)
01f,27aug99,dbs  fix ARM problem with stl member templates
01e,29jul99,aim  added STL_DEQUE
01d,16jul99,dbs  add STL_MAP_CMP macro to define map class with
                 comparison for 68K compiler bug
01c,16jul99,aim  Change Free to Dealloc
01b,06jul99,aim  checks for delete 0
01a,17jun99,aim  created modification history entry
*/

#ifndef __INCcomStl_h
#define __INCcomStl_h

// The ARM compilers have a problem in the generated code with some
// methods of the vector class which involves member-templates
// (specifically the insert() method) and so by undefining this flag
// the STL reverts to 'no member templates' configuration and the
// compiler seems to emit correct code...

#ifdef __GNUC__
#if defined(CPU_FAMILY) && defined (ARM) && (CPU_FAMILY==ARM)
#include <stl_config.h>
#undef __STL_MEMBER_TEMPLATES
#endif
#endif

#include <set>
#include <map>
#include <vector>
#include <deque>
#include <list>

//#ifdef VXDCOM_PLATFORM_VXWORKS
//struct less_longlong
//    {
//    bool operator() (long long a, long long b) const
//	{ return (a < b); }
//    };
//#endif

#if defined(VXDCOM_PLATFORM_VXWORKS) && (VXDCOM_PLATFORM_VXWORKS == 5)

#ifdef __GNUC__

//////////////////////////////////////////////////////////////////////////
//
// For VxWorks 5.4 (and Tornado 2.0 with gcc2.7.2 compilers) we have
// our _own_ default allocator because the vxWorks stl has exception
// handling turned on.  We currently compile with -fno-exception so we
// need an allocator that won't throw any exceptions.  Here it is.
//
//////////////////////////////////////////////////////////////////////////

class __vxdcom_safe_alloc
    {

  public:

    static void* allocate(size_t n)
	{
	void *result = malloc(n);
	if (0 == result) exit (1); // what should we do here?? (aim)
	return result;
	}

    static void deallocate(void *p, size_t /* n */)
	{
	if (p) free (p);
	}

    static void* reallocate(void *p, size_t /* old_sz */, size_t new_sz)
	{
	void * result = realloc(p, new_sz);
	if (0 == result) exit (1); // what should we do here?? (aim)
	return result;
	}

    };

#define SAFE_MAP(k,v)	map<k,v, less<k>, __vxdcom_safe_alloc>
#define STL_MAP(k,v)	map<k,v, less<k>, __vxdcom_safe_alloc>
#define STL_SET(t)	set<t, less<t>, __vxdcom_safe_alloc>
#define STL_VECTOR(t)	vector<t, __vxdcom_safe_alloc>
#define STL_DEQUE(t)	deque<t, __vxdcom_safe_alloc>
#define STL_LIST(t)	list<t, __vxdcom_safe_alloc>
#define STL_SET_LL	set<LONGLONG, less<LONGLONG>, __vxdcom_safe_alloc>
#define STL_MAP_LL(v)	map<LONGLONG, v, less<LONGLONG>, __vxdcom_safe_alloc>
#else /* (VXDCOM_PLATFORM_VXWORKS == 5) */
#define SAFE_MAP(k,v)	map<k,v>
#define STL_MAP(k,v)	map<k,v>
#define STL_SET(t)	set<t>
#define STL_VECTOR(t)	vector<t>
#define STL_DEQUE(t)	deque<t>
#define STL_LIST(t)	list<t>
#define STL_SET_LL	set<LONGLONG>
#define STL_MAP_LL(v)	map<LONGLONG, v>
#endif /* (VXDCOM_PLATFORM_VXWORKS == 5) */
#elif defined (__DCC__)
#define SAFE_MAP(k,v)	map<k,v>
#define STL_MAP(k,v)	map<k,v>
#define STL_SET(t)	set<t>
#define STL_VECTOR(t)	vector<t>
#define STL_DEQUE(t)	deque<t>
#define STL_LIST(t)	list<t>
#define STL_SET_LL	set<LONGLONG>
#define STL_MAP_LL(v)	map<LONGLONG, v>
#endif /* __GNUC__ */

#endif /* __INCcomStl_h */
