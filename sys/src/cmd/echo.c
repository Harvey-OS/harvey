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

void
main(int argc, char *argv[])
{
	int nflag;
	int i, len;
	char *buf, *p;

	nflag = 0;
	if(argc > 1 && strcmp(argv[1], "-n") == 0)
		nflag = 1;

	len = 1;
	for(i = 1+nflag; i < argc; i++)
		len += strlen(argv[i])+1;

	buf = malloc(len);
	if(buf == 0)
		exits("no memory");

	p = buf;
	for(i = 1+nflag; i < argc; i++){
		strcpy(p, argv[i]);
		p += strlen(p);
		if(i < argc-1)
			*p++ = ' ';
	}
		
	if(!nflag)
		*p++ = '\n';

	if(write(1, buf, p-buf) < 0){
		fprint(2, "echo: write error: %r\n");
		exits("write error");
	}

	exits((char *)0);
}
