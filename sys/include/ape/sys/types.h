#ifndef __TYPES_H
#define __TYPES_H

typedef	unsigned short	ino_t;
typedef	unsigned short	dev_t;
typedef	long		off_t;
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

#endif /* __TYPES_H */
