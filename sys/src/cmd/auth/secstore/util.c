#include <u.h>
#include <libc.h>

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

// name changed from getpass() to avoid conflict with Irix stdlib.h
int
getpasswd(char *prompt, char *line, int len)
{
	char *p;
	int n, nr;
	static int cons, consctl;  // closing and reopening fails in ssh environment

	if(cons == 0){ // first time
		cons = open("/dev/cons", ORDWR);
		if(cons < 0){
			fprint(2, "couldn't open cons\n");
			exits("no cons");
		}
		consctl = open("/dev/consctl", OWRITE);
		if(consctl < 0){
			fprint(2, "couldn't set raw mode\n");
			exits("no consctl");
		}
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
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			fprint(consctl, "rawoff");
			fprint(cons, "\n");
			return nr;
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
		if(nr == len){
			fprint(cons, "line too long; try again\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
	return -1;  // NOT REACHED
}
