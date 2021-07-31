#include "common.h"
#include <ctype.h>
#include <plumb.h>

Plumbattr*
findattr(Plumbmsg *m, char *name)
{
	Plumbattr* a;

	for(a = m->attr; a != nil; a = a->next)
		if(strcmp(a->name, name) == 0)
			return a;
	return nil;
}

void
main(int argc, char **argv)
{
	int fd;
	Plumbmsg *m;
	char *p, *e, *v;
	char buf[256];

	ARGBEGIN {
	} ARGEND;

	fd = plumbopen("seemail", OREAD|OCEXEC);
	if(fd < 0)
		sysfatal("can't plumbopen: %r");

	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		sysfatal("can't fork");
	case 0:
		break;
	default:
		exits(0);
	}

	for(;;){
		m = plumbrecv(fd);
		if(m == nil)
			break;
		v = plumblookup(m->attr, "mailtype");
		if(v == nil || strcmp(v, "new") != 0){
			plumbfree(m);
			continue;
		}
		p = buf;
		e = buf + sizeof(buf);
		v = plumblookup(m->attr, "sender");
		p = seprint(p, e, "[ %s / ", v != nil ? v : "");
		v = plumblookup(m->attr, "subject");
		p = seprint(p, e, "%s / ", v != nil ? v : "");
		v = plumblookup(m->attr, "length");
		p = seprint(p, e, "%s ]\n", v != nil ? v : "");
		if(write(1, buf, p-buf) <= 0)
			break;
		plumbfree(m);
	}
	exits(0);
}
