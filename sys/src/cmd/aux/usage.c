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
main(void)
{
	Fmt fmt;
	char buf[512];
	char *argv0, *args, *flags, *p, *p0;
	int single;
	Rune r;

	argv0 = getenv("0");
	if((p = strrchr(argv0, '/')) != nil)
		argv0 = p+1;
	flags = getenv("flagfmt");
	args = getenv("args");

	if(argv0 == nil){
		fprint(2, "aux/usage: $0 not set\n");
		exits("$0");
	}
	if(flags == nil)
		flags = "";
	if(args == nil)
		args = "";

	fmtfdinit(&fmt, 2, buf, sizeof buf);
	fmtprint(&fmt, "usage: %s", argv0);
	if(flags[0]){
		single = 0;
		for(p=flags; *p; ){
			p += chartorune(&r, p);
			if(*p == ',' || *p == 0){
				if(!single){
					fmtprint(&fmt, " [-");
					single = 1;
				}
				fmtprint(&fmt, "%C", r);
				if(*p == ',')
					p++;
				continue;
			}
			while(*p == ' ')
				p++;
			if(single){
				fmtprint(&fmt, "]");
				single = 0;
			}
			p0 = p;
			p = strchr(p0, ',');
			if(p == nil)
				p = "";
			else
				*p++ = 0;
			fmtprint(&fmt, " [-%C %s]", r, p0);
		}
		if(single)
			fmtprint(&fmt, "]");
	}
	if(args)
		fmtprint(&fmt, " %s", args);
	fmtprint(&fmt, "\n");
	fmtfdflush(&fmt);
	exits("usage");
}
