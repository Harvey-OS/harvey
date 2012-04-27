#ifndef __UTSNAME
#define __UTSNAME
#pragma lib "/$M/lib/ape/libap.a"

struct utsname {
	char	*sysname;
	char	*nodename;
	char	*release;
	char	*version;
	char	*machine;
};

#ifdef __cplusplus
extern "C" {
#endif

int uname(struct utsname *);

#ifdef __cplusplus
}
#endif

#endif
