#ifndef __GRP
#define __GRP
#ifndef _POSIX_SOURCE
   This header file is not defined in pure ANSI
#endif
#pragma lib "/$M/lib/ape/libap.a"
#include <sys/types.h>

struct	group {
	char	*gr_name;
	gid_t	gr_gid;
	char	**gr_mem;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct group *getgrgid(gid_t);
extern struct group *getgrnam(const char *);

#ifdef __cplusplus
}
#endif

#endif
