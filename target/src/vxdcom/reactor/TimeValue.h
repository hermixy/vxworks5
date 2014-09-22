/* TimeValue - Encapsulates a time value */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,10dec01,dbs  diab build
01b,19jan00,nel  Modifications for Linux debug build
01a,08Jul99,aim  created
*/

#ifndef __INCTimeValue_h
#define __INCTimeValue_h

#include "time.h"
#ifdef VXDCOM_PLATFORM_VXWORKS
#include "sys/times.h"
#endif
#ifdef VXDCOM_PLATFORM_LINUX
#include "sys/time.h"
#endif
#include <iostream>

class TimeValue
    {
  public:

    virtual ~TimeValue ();

    TimeValue ();

    TimeValue (const timeval& tv);

    TimeValue (long sec, long usec = 0);

    TimeValue& operator += (const TimeValue &rhs);
    TimeValue& operator -= (const TimeValue &rhs);

    friend const TimeValue operator +
	(
	const TimeValue &lhs,
	const TimeValue &rhs
	);

    friend const TimeValue operator -
	(
	const TimeValue &lhs,
	const TimeValue &rhs
	);

    friend int operator >
	(
	const TimeValue &tv1,
	const TimeValue &tv2
	);

    friend int operator <
	(
	const TimeValue &tv1,
	const TimeValue &tv2
	);

    friend int operator <=
	(
	const TimeValue &tv1,
	const TimeValue &tv2
	);

    friend int operator >=
	(
	const TimeValue &tv1,
	const TimeValue &tv2
	);

    friend int operator ==
	(
	const TimeValue &tv1,
	const TimeValue &tv2
	);

    friend int operator !=
	(
	const TimeValue &tv1,
	const TimeValue &tv2
	);

    friend ostream& operator <<
	(
	ostream& os,
	const TimeValue&
	);

    long sec () const;
    long usec () const;

    void set (const timeval&);
    void set (int, int);

    operator timeval () const;
    operator const timeval* () const;
    operator timeval* ();

    static const TimeValue  now ();
    static const TimeValue& zero ();

  private:

    void normalize ();
    // Put the timevalue into a canonical form.

    timeval m_tv;
    };

#endif // __INCTimeValue_h
