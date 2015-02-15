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
#include <authsrv.h>

static char *pbmsg = "AS protocol botch";

int
_asrdresp(int fd, char *buf, int len)
{
	int n;
	char error[64];

	if(read(fd, buf, 1) != 1){
		werrstr(pbmsg);
		return -1;
	}

	n = len;
	switch(buf[0]){
	case AuthOK:
		if(readn(fd, buf, len) != len){
			werrstr(pbmsg);
			return -1;
		}
		break;
	case AuthErr:
		if(readn(fd, error, sizeof error) != sizeof error){
			werrstr(pbmsg);
			return -1;
		}
		error[sizeof error-1] = '\0';
		werrstr("remote: %s", error);
		return -1;
	case AuthOKvar:
		if(readn(fd, error, 5) != 5){
			werrstr(pbmsg);
			return -1;
		}
		error[5] = 0;
		n = atoi(error);
		if(n <= 0 || n > len){
			werrstr(pbmsg);
			return -1;
		}
		memset(buf, 0, len);
		if(readn(fd, buf, n) != n){
			werrstr(pbmsg);
			return -1;
		}
		break;
	default:
		werrstr(pbmsg);
		return -1;
	}
	return n;
}
