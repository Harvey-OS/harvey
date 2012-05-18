/*
 !6c seg.c && 6l -o 6.seg seg.6
 */

#include <u.h>
#include <libc.h>
#include <seg.h>

void
main(int argc, char*argv[])
{

	void *p;
	uvlong va;
	ulong len;
	char *ep;
	argv0 = argv[0];
	if(argc == 1){
		p = newseg("testseg", 0, 65535);
		if(p == nil)
			sysfatal("newseg: %r");
		print("%s: va %#p\n", argv[1], p);
		exits(nil);
	}
	if(argc != 4){
		fprint(2, "usage: %s name va len\n", argv0);
		exits("usage");
	}
	va = strtoull(argv[2], &ep, 0);
	if(*ep)
		sysfatal("non numeric va");
	len = strtoul(argv[3], &ep, 0);
	if(*ep)
		sysfatal("non numeric len");
	p = newseg(argv[1], va, len);
	if(p == nil)
		sysfatal("newseg: %r");
	print("%s: va %#p\n", argv[1], p);
	exits(nil);
}

