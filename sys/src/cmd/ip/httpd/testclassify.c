#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include <libsec.h>
#include "whois.h"

char *class[] =
{
[Cok]		"ok",
[Cunknown]	"unknown",
[Cbadc]		"disallowed country",
[Cbadgov]	"disallowed government",
};

int classdebug;

void
logreg(char *rv, char *ip, Ndbtuple *t)
{
	char buf[2048];
	char *p, *e;

	p = buf;
	e = p + sizeof(buf);

	p = seprint(p, e, "ip=%s status=\"%s\" ", ip, rv);
	for(;t != nil; t = t->entry){
		p = seprint(p, e, "%s=", t->attr);
		if(strchr(t->val, ' ') != nil)
			p = seprint(p, e, "\"%s\"", t->val);
		else
			p = seprint(p, e, "%s", t->val);
		if(t->entry != t->line){
			if(t->entry == nil)
				break;
			p = seprint(p, e, " | ");
		} else {
			p = seprint(p, e, " ");
		}
	}
	print("%s\n", buf);
}

void
main(int argc, char **argv)
{
	Ndbtuple *t;

	classdebug = 1;

	ARGBEGIN {
	} ARGEND;

	if(argc < 1)
		exits("usage");

	for(; argc > 0; argc--, argv++){
		/* first page, check source */
		t = whois("/net.alt", argv[0]);
		switch(classify(argv[0], t)){
		case Cok:
			break;
		case Cunknown:
			logreg("unknown", argv[0], t);
			goto reject;
		case Cbadc:
			logreg("badcountry", argv[0], t);
			goto reject;
		case Cbadgov:
			logreg("badgov", argv[0], t);
			goto reject;
		reject:
			ndbfree(t);
			continue;
		}
		logreg("ok", argv[0], t);
		ndbfree(t);
	}
}
