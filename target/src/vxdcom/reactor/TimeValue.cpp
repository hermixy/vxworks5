/* TimeValue - Encapsulates a time value */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,17dec01,nel  Add include symbol for TimeValue.
01a,08Jul99,aim  created
*/

#include "TimeValue.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_TimeValue (void)
    {
    return 0;
    }

TimeValue::~TimeValue ()
    {}

TimeValue::TimeValue ()
    {
    set (0, 0);
    }

TimeValue::TimeValue
    (
    const timeval& tv
    )
    {
    set (tv);
    }

TimeValue::TimeValue
    (
    long seconds,
    long useconds
    )
    {
    set (seconds, useconds);
    }

void
TimeValue::set
    (
    const timeval& tv
    )
    {
    m_tv.tv_sec = tv.tv_sec;
    m_tv.tv_usec = tv.tv_usec;
    normalize ();
    }

void
TimeValue::set
    (
    int sec,
    int usec
    )
    {
    m_tv.tv_sec = sec;
    m_tv.tv_usec = usec;
    normalize ();
    }

int
operator >
    (
    const TimeValue& tv1,
    const TimeValue& tv2
    )
    {
    if (tv1.sec () > tv2.sec ())
	return 1;
    else if (tv1.sec () == tv2.sec () && tv1.usec () > tv2.usec ())
	return 1;
    else
	return 0;
    }

int
operator <
    (
    const TimeValue& tv1,
    const TimeValue& tv2
    )
    {
    return tv2 > tv1;
    }

int
operator <=
    (
    const TimeValue& tv1,
    const TimeValue& tv2
    )
    {
    return tv2 >= tv1;
    }

int
operator >=
    (
    const TimeValue& tv1,
    const TimeValue& tv2	
    )
    {
    if (tv1.sec () > tv2.sec ())
	return 1;
    else if (tv1.sec () == tv2.sec () && tv1.usec () >= tv2.usec ())
	return 1;
    else
	return 0;
    }

int
operator ==
    (
    const TimeValue& tv1,
    const TimeValue& tv2
    )
    {
    return (tv1.sec () == tv2.sec () && tv1.usec () == tv2.usec ());
    }

int
operator !=
    (
    const TimeValue& tv1,
    const TimeValue& tv2
    )
    {
    return !(tv1 == tv2);
    }

TimeValue&
TimeValue::operator += (const TimeValue& rhs)
    {
    m_tv.tv_sec += rhs.sec ();
    m_tv.tv_usec += rhs.usec ();
    normalize ();
    return *this;
    }

TimeValue&
TimeValue::operator -= (const TimeValue& rhs)
    {
    m_tv.tv_sec -= rhs.sec ();
    m_tv.tv_usec -= rhs.usec ();
    normalize ();
    return *this;
    }

const TimeValue
operator +
    (
    const TimeValue &lhs,
    const TimeValue &rhs
    )
    {
    return TimeValue (lhs) += rhs;
    }

const TimeValue
operator -
    (
    const TimeValue &lhs,
    const TimeValue &rhs
    )
    {
    return TimeValue (lhs) -= rhs;
    }

void
TimeValue::normalize ()
    {
    static const long ONE_SECOND_IN_USECS = 1000000L;

    if (m_tv.tv_usec >= ONE_SECOND_IN_USECS)
	{
	do
	    {
	    m_tv.tv_sec++;
	    m_tv.tv_usec -= ONE_SECOND_IN_USECS;
	    }
	while (m_tv.tv_usec >= ONE_SECOND_IN_USECS);
	}
    else if (m_tv.tv_usec <= -ONE_SECOND_IN_USECS)
	{
	do
	    {
	    m_tv.tv_sec--;
	    m_tv.tv_usec += ONE_SECOND_IN_USECS;
	    }
	while (m_tv.tv_usec <= -ONE_SECOND_IN_USECS);
	}

    if (m_tv.tv_sec >= 1 && m_tv.tv_usec < 0)
	{
	m_tv.tv_sec--;
	m_tv.tv_usec += ONE_SECOND_IN_USECS;
	}
    else if (m_tv.tv_sec < 0 && m_tv.tv_usec > 0)
	{
	m_tv.tv_sec++;
	m_tv.tv_usec -= ONE_SECOND_IN_USECS;
	}
    }

TimeValue::operator timeval () const
    {
    return m_tv;
    }

TimeValue::operator const timeval* () const
    {
    return reinterpret_cast<const timeval*> (&m_tv);
    }

TimeValue::operator timeval* ()
    {
    return &m_tv;
    }

const TimeValue
TimeValue::now ()
    {
    TimeValue now;
    // XXX no return value checks here??
#ifdef VXDCOM_PLATFORM_VXWORKS
    struct timespec ts;
    ::clock_gettime (CLOCK_REALTIME, &ts);
    now.m_tv.tv_sec = ts.tv_sec;
    now.m_tv.tv_usec = ts.tv_nsec / 1000L; // timespec has nsec, but timeval has usec
#else
    ::gettimeofday (now, 0);
#endif
    return now;
    }

const TimeValue&
TimeValue::zero ()
    {
    static TimeValue zero (0, 0);
    return zero;
    }

long
TimeValue::sec () const
    {
    return m_tv.tv_sec;
    }

long
TimeValue::usec () const
    {
    return m_tv.tv_usec;
    }

static int 
tv_abs (int d)
    { 
    return d > 0 ? d : -d;
    }

ostream&
operator<< (ostream &os, const TimeValue &tv)
    {
    if (tv.usec () < 0 || tv.sec () < 0)
	os << "-";

    os << tv_abs (int (tv.sec ()))
       << "."
       << tv_abs (int (tv.usec ()));

    return os;
    }
