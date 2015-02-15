/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "authcmdlib.h"

int
readarg(int fd, char *arg, int len)
{
	char buf[1];
	int i;

	i = 0;
	memset(arg, 0, len);
	while(read(fd, buf, 1) == 1){
		if(i < len - 1)
			arg[i++] = *buf;
		if(*buf == '\0'){
			arg[i] = '\0';
			return 0;
		}
	}
	return -1;
}
