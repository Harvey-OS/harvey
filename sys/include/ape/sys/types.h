#ifndef __TYPES_H
#define __TYPES_H

#pragma lib "/$M/lib/ape/libap.a"
typedef	unsigned short	ino_t;
typedef	unsigned short	dev_t;
typedef	long long		off_t;
typedef unsigned short	mode_t;
typedef short		uid_t;
typedef short		gid_t;
typedef short		nlink_t;
typedef int		pid_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif
#ifndef _SSIZE_T
#define _SSIZE_T
typedef long ssize_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif

#ifdef _BSD_EXTENSION
#ifndef _CADDR_T
#define _CADDR_T
typedef char * caddr_t;
#endif
#ifndef _FD_SET_T
#define _FD_SET_T
/* also cf <select.h> */
typedef struct fd_set {
	long fds_bits[3];
} fd_set;
#define FD_SET(n,p)	((p)->fds_bits[(n)>>5] |= (1 << ((n) &0x1f)))
#define FD_CLR(n,p)	((p)->fds_bits[(n)>>5] &= ~(1 << ((n) &0x1f)))
#define FD_ISSET(n,p)	((p)->fds_bits[(n)>>5] & (1 << ((n) &0x1f)))
#define FD_ZERO(p)	((p)->fds_bits[0] =0, (p)->fds_bits[1] =0, (p)->fds_bits[2] =0)
#endif
#endif

#endif /* __TYPES_H */
