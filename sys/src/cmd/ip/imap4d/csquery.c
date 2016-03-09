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
#include <auth.h>
#include "imap4d.h"

/*
 *  query the connection server
 */
char*
csquery(char *attr, char *val, char *rattr)
{
	char token[64+4];
	char buf[256], *p, *sp;
	int fd, n;

	if(val == nil || val[0] == 0)
		return nil;
	fd = open("/net/cs", ORDWR);
	if(fd < 0)
		return nil;
	fprint(fd, "!%s=%s", attr, val);
	seek(fd, 0, 0);
	snprint(token, sizeof(token), "%s=", rattr);
	for(;;){
		n = read(fd, buf, sizeof(buf)-1);
		if(n <= 0)
			break;
		buf[n] = 0;
		p = strstr(buf, token);
		if(p != nil && (p == buf || *(p-1) == 0)){
			close(fd);
			sp = strchr(p, ' ');
			if(sp)
				*sp = 0;
			p = strchr(p, '=');
			if(p == nil)
				return nil;
			return strdup(p+1);
		}
	}
	close(fd);
	return nil;
}
