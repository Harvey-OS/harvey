#include "lib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

static char qsep[] = " \t\r\n";

static char*
qtoken(char *s)
{
	int quoting;
	char *t;

	quoting = 0;
	t = s;	/* s is output string, t is input string */
	while(*t!='\0' && (quoting || strchr(qsep, *t)==nil)){
		if(*t != '\''){
			*s++ = *t++;
			continue;
		}
		/* *t is a quote */
		if(!quoting){
			quoting = 1;
			t++;
			continue;
		}
		/* quoting and we're on a quote */
		if(t[1] != '\''){
			/* end of quoted section; absorb closing quote */
			t++;
			quoting = 0;
			continue;
		}
		/* doubled quote; fold one quote into two */
		t++;
		*s++ = *t++;
	}
	if(*s != '\0'){
		*s = '\0';
		if(t == s)
			t++;
	}
	return t;
}

static int
tokenize(char *s, char **args, int maxargs)
{
	int nargs;

	for(nargs=0; nargs<maxargs; nargs++){
		while(*s!='\0' && strchr(qsep, *s)!=nil)
			s++;
		if(*s == '\0')
			break;
		args[nargs] = s;
		s = qtoken(s);
	}

	return nargs;
}

Waitmsg*
_WAIT(void)
{
	int n, l;
	char buf[512], *fld[5];
	Waitmsg *w;

	n = _AWAIT(buf, sizeof buf-1);
	if(n < 0)
		return nil;
	buf[n] = '\0';
	if(tokenize(buf, fld, 5) != 5){
		strcpy(buf, "couldn't parse wait message");
		_ERRSTR(buf, sizeof buf);
		return nil;
	}
	l = strlen(fld[4])+1;
	w = malloc(sizeof(Waitmsg)+l);
	if(w == nil)
		return nil;
	w->pid = atoi(fld[0]);
	w->time[0] = atoi(fld[1]);
	w->time[1] = atoi(fld[2]);
	w->time[2] = atoi(fld[3]);
	w->msg = (char*)&w[1];
	memmove(w->msg, fld[4], l);
	return w;
}

