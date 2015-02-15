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

int alarmed;
int done;

void
usage(void)
{
	fprint(2, "usage: %s [-q]\n", argv0);
	exits("usage");
}

void
ding(void*, char *s)
{
	if(strstr(s, "alarm")){
		alarmed = 1;
		noted(NCONT);
	} else
		noted(NDFLT);
}


void
main(int argc, char **argv)
{
	int fd, cfd;
	int i;
	char buf[1];
	int quiet = 0;

	ARGBEGIN {
	case 'q':
		quiet = 1;
		break;
	} ARGEND;

	notify(ding);

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		sysfatal("opening /dev/cons: %r");
	cfd = open("/dev/consctl", OWRITE);
	if(cfd >= 0)
		fprint(cfd, "rawon");

	switch(rfork(RFPROC|RFFDG|RFMEM)){
	case -1:
		sysfatal("forking: %r");
	default:
		// read until we're done writing or
		// we get an end of line
		while(!done){
			alarmed = 0;
			alarm(250);
			i = read(0, buf, 1);
			alarm(0);

			if(i == 0)
				break;
			if(i < 0){
				if(alarmed)
					continue;
				else
					break;
			}
			if(!quiet && write(fd, buf, 1) < 1)
				break;
			if(buf[0] == '\n' || buf[0] == '\r')
				break;
		}	
		break;
	case 0:
		// pass one character at a time till end of line
		for(;;){
			if(read(fd, buf, 1) <= 0)
				break;
			if(write(1, buf, 1) < 0)
				break;
			if(buf[0] == '\n' || buf[0] == '\r')
				break;
		}

		// tell reader to give up after next char
		done = 1;
		break;
	}
	exits(0);
}
