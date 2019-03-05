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

void	getstr(int, char*, int);

void
main(void)
{
	char luser[128], ruser[128], term[128], err[128];

	getstr(0, err, sizeof(err));
	getstr(0, ruser, sizeof(ruser));
	getstr(0, luser, sizeof(luser));
	getstr(0, term, sizeof(term));
	write(0, "", 1);

	if(luser[0] == '\0')
		strncpy(luser, ruser, sizeof luser);
	luser[sizeof luser-1] = '\0';
	syslog(0, "telnet", "rlogind %s", luser);
	execl("/bin/ip/telnetd", "telnetd", "-n", "-u", luser, nil);
	fprint(2, "can't exec con service: %r\n");
	exits("can't exec");
}

void
getstr(int fd, char *str, int len)
{
	char c;
	int n;

	while(--len > 0){
		n = read(fd, &c, 1);
		if(n < 0)
			return;
		if(n == 0)
			continue;
		*str++ = c;
		if(c == 0)
			break;
	}
	*str = '\0';
}
