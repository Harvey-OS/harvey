#ifndef __PWD
#define __PWD
#ifndef _POSIX_SOURCE
   This header file is not defined in pure ANSI
#endif
#pragma lib "/$M/lib/ape/libap.a"
#include <sys/types.h>

struct passwd {
	char	*pw_name;
	uid_t	pw_uid;
	gid_t	pw_gid;
	char	*pw_dir;
	char	*pw_shell;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct passwd *getpwuid(uid_t);
extern struct passwd *getpwnam(const char *);

#ifdef __cplusplus
}
#endif

#endif
