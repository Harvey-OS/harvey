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
#include <thread.h>
#include <keyboard.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"
#include <ctype.h>

char	Ebadwr[]		= "bad rectangle in wctl request";
char	Ewalloc[]		= "window allocation failed in wctl request";

/* >= Top are disallowed if mouse button is pressed */
enum
{
	New,
	Resize,
	Move,
	Scroll,
	Noscroll,
	Set,
	Top,
	Bottom,
	Current,
	Hide,
	Unhide,
	Delete,
};

static char *cmds[] = {
	[New]	= "new",
	[Resize]	= "resize",
	[Move]	= "move",
	[Scroll]	= "scroll",
	[Noscroll]	= "noscroll",
	[Set]		= "set",
	[Top]	= "top",
	[Bottom]	= "bottom",
	[Current]	= "current",
	[Hide]	= "hide",
	[Unhide]	= "unhide",
	[Delete]	= "delete",
	nil
};

enum
{
	Cd,
	Deltax,
	Deltay,
	Hidden,
	Id,
	Maxx,
	Maxy,
	Minx,
	Miny,
	PID,
	R,
	Scrolling,
	Noscrolling,
};

static char *params[] = {
	[Cd]	 			= "-cd",
	[Deltax]			= "-dx",
	[Deltay]			= "-dy",
	[Hidden]			= "-hide",
	[Id]				= "-id",
	[Maxx]			= "-maxx",
	[Maxy]			= "-maxy",
	[Minx]			= "-minx",
	[Miny]			= "-miny",
	[PID]				= "-pid",
	[R]				= "-r",
	[Scrolling]			= "-scroll",
	[Noscrolling]		= "-noscroll",
	nil
};

static
int
word(char **sp, char *tab[])
{
	print_func_entry();
	char *s, *t;
	int i;

	s = *sp;
	while(isspace(*s))
		s++;
	t = s;
	while(*s!='\0' && !isspace(*s))
		s++;
	for(i=0; tab[i]!=nil; i++)
		if(strncmp(tab[i], t, strlen(tab[i])) == 0){
			*sp = s;
			print_func_exit();
			return i;
	}
	print_func_exit();
	return -1;
}

int
set(int sign, int neg, int abs, int pos)
{
	print_func_entry();
	if(sign < 0) {
		print_func_exit();
		return neg;
	}
	if(sign > 0) {
		print_func_exit();
		return pos;
	}
	print_func_exit();
	return abs;
}

void
shift(int *minp, int *maxp, int min, int max)
{
	print_func_entry();
	if(*minp < min){
		*maxp += min-*minp;
		*minp = min;
	}
	if(*maxp > max){
		*minp += max-*maxp;
		*maxp = max;
	}
	print_func_exit();
}


/* permit square brackets, in the manner of %R */
int
riostrtol(char *s, char **t)
{
	print_func_entry();
	int n;

	while(*s!='\0' && (*s==' ' || *s=='\t' || *s=='['))
		s++;
	if(*s == '[')
		s++;
	n = strtol(s, t, 10);
	if(*t != s)
		while((*t)[0] == ']')
			(*t)++;
	print_func_exit();
	return n;
}


int
parsewctl(char **argp, int *pidp, int *idp,
	  char **cdp, char *s,
	  char *err)
{
	print_func_entry();
	int cmd, param, xy = 0;

	*pidp = 0;
	*cdp = nil;
	cmd = word(&s, cmds);
	if(cmd < 0){
		strcpy(err, "unrecognized wctl command");
		print_func_exit();
		return -1;
	}
	strcpy(err, "missing or bad wctl parameter");
	while((param = word(&s, params)) >= 0){
		switch(param){	/* special cases */
		case Id:
			if(idp != nil)
				*idp = xy;
			break;
		case PID:
			if(pidp != nil)
				*pidp = xy;
			break;
		}
	}

	while(isspace(*s))
		s++;
	if(cmd!=New && *s!='\0'){
		strcpy(err, "extraneous text in wctl message");
		print_func_exit();
		return -1;
	}

	if(argp)
		*argp = s;

	print_func_exit();
	return cmd;
}

int
wctlnew(char *arg, int pid, char *dir, char *err)
{
	print_func_entry();
	char **argv;
	Console *i = nil;

	argv = emalloc(4*sizeof(char*));
	argv[0] = "rc";
	argv[1] = "-c";
	while(isspace(*arg))
		arg++;
	if(*arg == '\0'){
		argv[1] = "-i";
		argv[2] = nil;
	}else{
		argv[2] = arg;
		argv[3] = nil;
	}
	if(i == nil){
		strcpy(err, Ewalloc);
		print_func_exit();
		return -1;
	}
	//new(i, hideit, scrollit, pid, dir, "/bin/rc", argv);

	free(argv);	/* when new() returns, argv and args have been copied */
	print_func_exit();
	return 1;
}

int
writewctl(Xfid *x, char *err)
{
	print_func_entry();
	int cnt, cmd, j, id, pid;
	char *arg, *dir;
	Window *w;

	w = x->f->w;
	cnt = x->count;
	x->data[cnt] = '\0';
	id = 0;

	cmd = parsewctl(&arg, &pid, &id, &dir, x->data, err);
	if(cmd < 0) {
		print_func_exit();
		return -1;
	}

	if(id != 0){
		for(j=0; j<nwindow; j++)
			if(window[j]->id == id)
				break;
		if(j == nwindow){
			strcpy(err, "no such window id");
			print_func_exit();
			return -1;
		}
		w = window[j];
		if(w->deleted){
			strcpy(err, "window deleted");
			print_func_exit();
			return -1;
		}
	}

	switch(cmd){
	case New:
		print_func_exit();
		return wctlnew(arg, pid, dir, err);
	case Set:
		if(pid > 0)
			wsetpid(w, pid, 0);
		print_func_exit();
		return 1;
	case Current:
		wcurrent(w);
		print_func_exit();
		return 1;
	case Delete:
		//wsendctlmesg(w, Deleted, ZR, nil);
		print_func_exit();
		return 1;
	}
	strcpy(err, "invalid wctl message");
	print_func_exit();
	return -1;
}

void
wctlthread(void *v)
{
	print_func_entry();
	char *buf, *arg, *dir;
	int cmd, id, pid;
	char err[ERRMAX];
	Channel *c;

	c = v;

	threadsetname("WCTLTHREAD");

	for(;;){
		buf = recvp(c);
		cmd = parsewctl(&arg, &pid, &id, &dir, buf, err);

		switch(cmd){
		case New:
			wctlnew(arg, pid, dir, err);
		}
		free(buf);
	}
	print_func_exit();
}

void
wctlproc(void *v)
{
	print_func_entry();
	char *buf;
	int n, eofs;
	Channel *c;

	threadsetname("WCTLPROC");
	c = v;

	eofs = 0;
	for(;;){
		buf = emalloc(messagesize);
		n = read(wctlfd, buf, messagesize-1);	/* room for \0 */
		if(n < 0)
			break;
		if(n == 0){
			if(++eofs > 20)
				break;
			continue;
		}
		eofs = 0;

		buf[n] = '\0';
		sendp(c, buf);
	}
	print_func_exit();
}
