#ifndef __TIMES_H
#define __TIMES_H
#pragma lib "/$M/lib/ape/libap.a"

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long clock_t;
#endif

struct tms {
	clock_t	tms_utime;
	clock_t	tms_stime;
	clock_t	tms_cutime;
	clock_t	tms_cstime;
};

#ifdef __cplusplus
extern "C" {
#endif

clock_t times(struct tms *);

#ifdef __cplusplus
}
#endif

#endif
