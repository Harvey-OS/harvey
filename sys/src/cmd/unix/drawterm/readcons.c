#include <u.h>
#include <libc.h>
#include "drawterm.h"

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil)
		sysfatal("out of memory");
	return v;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("out of memory");
	return s;
}

char*
estrappend(char *s, char *fmt, ...)
{
	char *t;
	va_list arg;

	va_start(arg, fmt);
	t = vsmprint(fmt, arg);
	if(t == nil)
		sysfatal("out of memory");
	va_end(arg);
	s = erealloc(s, strlen(s)+strlen(t)+1);
	strcat(s, t);
	free(t);
	return s;
}

/*
 *  prompt for a string with a possible default response
 */
char*
readcons(char *prompt, char *def, int raw)
{
	int fdin, fdout, ctl, n;
	char line[10];
	char *s;

	fdin = open("/dev/cons", OREAD);
	if(fdin < 0)
		fdin = 0;
	fdout = open("/dev/cons", OWRITE);
	if(fdout < 0)
		fdout = 1;
	if(def != nil)
		fprint(fdout, "%s[%s]: ", prompt, def);
	else
		fprint(fdout, "%s: ", prompt);
	if(raw){
		ctl = open("/dev/consctl", OWRITE);
		if(ctl >= 0)
			write(ctl, "rawon", 5);
	} else
		ctl = -1;
	s = estrdup("");
	for(;;){
		n = read(fdin, line, 1);
		if(n == 0){
		Error:
			close(fdin);
			close(fdout);
			if(ctl >= 0)
				close(ctl);
			free(s);
			return nil;
		}
		if(n < 0)
			goto Error;
		if(line[0] == 0x7f)
			goto Error;
		if(n == 0 || line[0] == '\n' || line[0] == '\r'){
			if(raw){
				write(ctl, "rawoff", 6);
				write(fdout, "\n", 1);
			}
			close(ctl);
			close(fdin);
			close(fdout);
			if(*s == 0 && def != nil)
				s = estrappend(s, "%s", def);
			return s;
		}
		if(line[0] == '\b'){
			if(strlen(s) > 0)
				s[strlen(s)-1] = 0;
		} else if(line[0] == 0x15) {	/* ^U: line kill */
			if(def != nil)
				fprint(fdout, "\n%s[%s]: ", prompt, def);
			else
				fprint(fdout, "\n%s: ", prompt);
			
			s[0] = 0;
		} else {
			s = estrappend(s, "%c", line[0]);
		}
	}
	return nil; /* not reached */
}

