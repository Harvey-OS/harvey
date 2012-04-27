#ifndef __UTIME_H
#define __UTIME_H

#pragma lib "/$M/lib/ape/libap.a"

struct utimbuf
{
	time_t actime;
	time_t modtime;
};

#ifdef __cplusplus
extern "C" {
#endif

extern int utime(const char *, const struct utimbuf *);

#ifdef __cplusplus
}
#endif

#endif
