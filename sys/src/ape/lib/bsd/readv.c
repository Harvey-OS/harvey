/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <unistd.h>
#include <string.h>
#include <sys/uio.h>

int
readv(int fd, struct iovec *v, int ent)
{
	int x, i, n, len;
	char buf[10*1024];

	for(len = i = 0; i < ent; i++)
		len += v[i].iov_len;
	if(len > sizeof(buf))
		len = sizeof(buf);

	len = read(fd, buf, len);
	if(len <= 0)
		return len;

	for(n = i = 0; n < len && i < ent; i++){
		x = len - n;
		if(x > v[i].iov_len)
			x = v[i].iov_len;
		memmove(v[i].iov_base, buf + n, x);
		n += x;
	}

	return len;
}
