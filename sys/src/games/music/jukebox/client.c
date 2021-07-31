#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include "client.h"
#include "playlist.h"
#include "../debug.h"

char *srvmount = "/mnt/juke";

char*
getroot(void)
{
	return "root";
}

void
fillbrowsebot(char *onum)
{
	char name[256], *p, *q;
	Biobuf *b, *d;
	int c;

	snprint(name, sizeof name, "%s/%s/children", srvmount, onum);
	b = Bopen(name, OREAD);
	if(b == nil)
abort();//		sysfatal("getchildren: %s: %r", name);
	while(p = Brdline(b, '\n')){
		c = strtol(p, &q, 0);
		assert(*q == '\n');
		*q = 0;
		snprint(name, sizeof name, "%s/%d/type", srvmount, c);
		d = Bopen(name, OREAD);
		if(d == nil)
			sysfatal("getchildren: %s: %r", name);
		q = Brdstr(d, '\n', 1);
		if(q == nil){
			abort();
		}
		Bterm(d);
		if(strcmp(q, "performance") == 0)
			continue;
		snprint(name, sizeof name, "%s/%d/digest", srvmount, c);
		d = Bopen(name, OREAD);
		if(d == nil)
			sysfatal("getchildren: %s: %r", name);
		q = Brdstr(d, '\n', 1);
		if(q == nil){
			Bterm(d);
			continue;
		}
		addchild(strdup(p), q);
		Bterm(d);
	}
	Bterm(b);
}

void
doplay(char *onum){
	char name[256], *p, *q;
	Biobuf *b;
	int m;

	snprint(name, sizeof name, "%s/%s/files", srvmount, onum);
	b = Bopen(name, OREAD);
	if(b == nil)
abort();//		sysfatal("doplay: %s: %r", name);
	while(p = Brdline(b, '\n')){
		m = Blinelen(b);
		assert(p[m-1] == '\n');
		p[m-1] = '\0';
		q = strchr(p, '	');
		if(q == nil)
			sysfatal("doplay: %s: format", name);
		*q++ = '\0';
		sendplaylist(strdup(q), strdup(p));
	}
	Bterm(b);
}

void
fillbrowsetop(char *onum)
{
	char name[256], *p;
	Biobuf *b;
	int m;

	snprint(name, sizeof name, "%s/%s/parentage", srvmount, onum);
	b = Bopen(name, OREAD);
	if(b == nil)
abort();//		sysfatal("gettopwin: %s: %r", name);
	while(p = Brdline(b, '\n')){
		m = Blinelen(b);
		assert(p[m-1] == '\n');
		p[m-1] = '\0';
		addparent(p);
	}
	Bterm(b);
}

void
fillplaytext(char *onum)
{
	char name[256], *p;
	Biobuf *b;
	int m;

	snprint(name, sizeof name, "%s/%s/parentage", srvmount, onum);
	b = Bopen(name, OREAD);
	if(b == nil)
		sysfatal("fillplaytext: %s: %r", name);
	while(p = Brdline(b, '\n')){
		m = Blinelen(b);
		assert(p[m-1] == '\n');
		p[m-1] = '\0';
		addplaytext(p);
	}
	Bterm(b);
}

char *
getoneliner(char *onum)
{
	char name[256], *p;
	Biobuf *b;

	snprint(name, sizeof name, "%s/%s/miniparentage", srvmount, onum);
	b = Bopen(name, OREAD);
	if(b == nil)
		sysfatal("gettopwin: %s: %r", name);
	p = Brdstr(b, '\n', 1);
	Bterm(b);
	return p;
}

char *
getparent(char *onum)
{
	char name[256], *p;
	Biobuf *b;

	snprint(name, sizeof name, "%s/%s/parent", srvmount, onum);
	b = Bopen(name, OREAD);
	if(b == nil)
abort();//		sysfatal("gettopwin: %s: %r", name);
	p = Brdstr(b, '\n', 1);
	Bterm(b);
	return p;
}
