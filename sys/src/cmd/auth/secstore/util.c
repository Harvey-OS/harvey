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
	static int cons, consctl;  // closing and reopening fails in ssh environment

	if(cons == 0){ // first time
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
		}else if(*p == 21){		/* cntrl-u */
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
	return nil;  // NOT REACHED
}

char *
validatefile(char *f)
{
	char *nl;

	if(f==nil || *f==0)
		return nil;
	if(nl = strchr(f, '\n'))
		*nl = 0;
	if(strchr(f,'/') != nil || strcmp(f,"..")==0 || strlen(f) >= 300){
		syslog(0, LOG, "no slashes allowed: %s\n", f);
		return nil;
	}
	return f;
}

