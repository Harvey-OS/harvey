#ifndef __SYSTIME_H
#define __SYSTIME_H
#ifndef _BSDTIME_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/libap.a"

#ifndef __TIMEVAL__
#define __TIMEVAL__
struct timeval {
	long	tv_sec;
	long	tv_usec;
};
struct timezone {
	int	tz_minuteswest;
	int	tz_dsttime;
};
#endif /* __TIMEVAL__ */

#endif /* __SYSTIME_H */
