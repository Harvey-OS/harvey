#ifndef __SYS_UIO_H__
#define __SYS_UIO_H__

#ifndef _BSD_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma lib "/$M/lib/ape/libbsd.a"

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uio.h	7.1 (Berkeley) 6/4/86
 */

struct iovec {
	char	*iov_base;
	int	iov_len;
};

extern int writev(int, struct iovec*, int);
extern int readv(int, struct iovec*, int);

#ifdef __cplusplus
}
#endif

#endif /* !__SYS_UIO_H__ */
