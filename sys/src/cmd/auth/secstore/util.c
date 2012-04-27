#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"
#include "secstore.h"

void *
emalloc(ulong n)
{
	void *p = malloc(n);

	if(p == nil)
		sysfatal("emalloc");
	memset(p, 0, n);
	return p;
}

void *
erealloc(void *p, ulong n)
{
	if ((p = realloc(p, n)) == nil)
		sysfatal("erealloc");
	return p;
}

char *
estrdup(char *s)
{
	if ((s = strdup(s)) == nil)
		sysfatal("estrdup");
	return s;
}

char*
getpassm(char *prompt)
{
	char *p, line[4096];
	int n, nr;
	static int cons, consctl; /* closing & reopening fails in ssh environment */

	if(cons == 0){			/* first time? */
		cons = open("/dev/cons", ORDWR);
		if(cons < 0)
			sysfatal("couldn't open cons");
		consctl = open("/dev/consctl", OWRITE);
		if(consctl < 0)
			sysfatal("couldn't set raw mode via consctl");
	}
	fprint(consctl, "rawon");
	fprint(cons, "%s", prompt);
	nr = 0;
	p = line;
	for(;;){
		n = read(cons, p, 1);
		if(n < 0){
			fprint(consctl, "rawoff");
			fprint(cons, "\n");
			return nil;
		}
		if(n == 0 || *p == '\n' || *p == '\r' || *p == 0x7f){
			*p = '\0';
			fprint(consctl, "rawoff");
			fprint(cons, "\n");
			p = strdup(line);
			memset(line, 0, nr);
			return p;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else if(*p == ('u' & 037)){		/* cntrl-u */
			fprint(cons, "\n%s", prompt);
			nr = 0;
			p = line;
		}else{
			nr++;
			p++;
		}
		if(nr+1 == sizeof line){
			fprint(cons, "line too long; try again\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
}

static char *
illegal(char *f)
{
	syslog(0, LOG, "illegal name: %s", f);
	return nil;
}

char *
validatefile(char *f)
{
	char *p;

	if(f == nil || *f == '\0')
		return nil;
	if(strcmp(f, "..") == 0 || strlen(f) >= 250)
		return illegal(f);
	for(p = f; *p; p++)
		if(*p < 040 || *p == '/')
			return illegal(f);
	return f;
}
