#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "win.h"
#include "adict.h"

char *prog = "adict";
char *lprog = "/bin/adict";
char *xprog  = "/bin/dict";
char *dict, *pattern, *curaddr[MAXMATCH], *curone, *args[6], buffer[80];
char abuffer[80], fbuffer[80], pbuffer[80];
int curindex, count, Eopen, Mopen;
Win Mwin, Ewin, Dwin;

void openwin(char*, char*, Win*, int);
void  handle(Win*, int);
void	rexec(void*);
void	pexec(void*);
int getaddr(char*);

void
usage(void)
{
		threadprint(2, "usage: %s [-d dictname] [pattern]\n", argv0);
		threadexitsall(nil);
}

void
threadmain(int argc, char** argv)
{
	ARGBEGIN{
	case 'd':
		dict = strdup(ARGF());
		break;
	default:
		usage();
	}ARGEND

	/* if running as other name, note that fact */
	if(access(argv0, AEXIST) == 0)
		lprog = argv0;

	switch(argc){
	case 1:
		pattern = pbuffer;
		strcpy(pattern,argv[0]);
		if(dict == nil)
			dict = "oed";
		break;
	case 0:
		break;
	default:
		usage();
	}

	if ((dict == nil) && (pattern == nil))
		openwin(prog,"", &Dwin, Dictwin);
	if (pattern == nil)
		openwin(prog,"",&Ewin, Entrywin);
	if ((count = getaddr(pattern)) <= 1)
		openwin(prog,"Prev Next", &Ewin, Entrywin);
	else
		openwin(prog, "", &Mwin, Matchwin);
}

static int
procrexec(char *xprog, ...)
{
	int fpipe[2];
	void *rexarg[4];
	Channel *c;
	va_list va;
	int i;
	char *p;

	pipe(fpipe);
	va_start(va, xprog);
	p = xprog;
	for(i=0; p && i+1<nelem(args); i++){
		args[i] = p;
		p = va_arg(va, char*);
	}
	args[i] = nil;

	c = chancreate(sizeof(ulong), 0);
	rexarg[0] = xprog;
	rexarg[1] = args;
	rexarg[2] = fpipe;
	rexarg[3] = c;

	proccreate(rexec, rexarg, 8192);
	recvul(c);
	chanfree(c);
	close(fpipe[1]);
	return fpipe[0];
}

int
getaddr(char *pattern)
{
	/* Get char offset into dictionary of matches. */

	int fd, i;
	Biobuf inbuf;
	char *bufptr;
char *obuf;

	if (pattern == nil) {
		curone = nil;
		curindex = 0;
		curaddr[curindex] = nil;
		return 0;
	}

	sprint(buffer,"/%s/A", pattern);
	fd = procrexec(xprog, "-d", dict, "-c", buffer, nil);
	Binit(&inbuf, fd, OREAD);
	i = 0;
	curindex = 0;
	while ((bufptr = Brdline(&inbuf, '\n')) != nil && (i < (MAXMATCH-1))) {
		bufptr[Blinelen(&inbuf)-1] = 0;
obuf=bufptr;
		while (bufptr[0] != '#' && bufptr[0] != 0) bufptr++;
if(bufptr[0] == 0)
	print("whoops buf «%s»\n", obuf);
		curaddr[i] = malloc(strlen(bufptr));
		strcpy(curaddr[i], bufptr);
		i++;
	}
	curaddr[i] = nil;
	if (i == MAXMATCH)
		threadprint(2, "Too many matches!\n");
	Bterm(&inbuf);
	close(fd);

	curone = curaddr[curindex];
	return(i);
}

char*
getpattern(char *addr)
{
	/* Get the pattern corresponding to an absolute address.*/
	int fd;
	char *res, *t;

	res = nil;
	sprint(buffer,"%sh", addr);
	fd = procrexec(xprog, "-d", dict, "-c", buffer, nil);
	if (read(fd, pbuffer, 80) > 80)
		threadprint(2, "Error in getting addres from dict.\n");
	else {
		t = pbuffer;
		/* remove trailing whitespace, newline */
		if (t != nil){
			while(*t != 0 && *t != '\n')
				t++;
			if(t == 0 && t > pbuffer)
				t--;
			while(t >= pbuffer && (*t==' ' || *t=='\n' || *t=='\t' || *t=='\r'))
				*t-- = 0;
		}
		res = pbuffer;
	}
	close(fd);
	return(res);
}

char*
chgaddr(int dir)
{
	/* Increment or decrement the current address (curone). */

	int fd;
	char *res, *t;

	res = nil;
	if (dir < 0)
		sprint(buffer,"%s-a", curone);
	else
		sprint(buffer,"%s+a", curone);
	fd = procrexec(xprog, "-d", dict, "-c", buffer, nil);
	if (read(fd, abuffer, 80) > 80)
		threadprint(2, "Error in getting addres from dict.\n");
	else {
		res = abuffer;
		while (*res != '#') res++;
		t = res;
		while ((*t != '\n') && (t != nil)) t++;
		if (t != nil) *t = 0;
	}
	close(fd);
	return(res);
}

void
dispdicts(Win *cwin)
{
	/* Display available dictionaries in window. */

	int fd, nb, i;
	char buf[1024], *t;

	fd = procrexec(xprog, "-d", "?", nil);
	wreplace(cwin, "0,$","",0);	/* Clear window */
	while ((nb = read(fd, buf, 1024)) > 0) {
		t = buf;
		i = 0;
		if (strncmp("Usage", buf, 5) == 0) {	/* Remove first line. */
			while (t[0] != '\n') {
				t++; 
				i++;
			}
			t++; 
			i++;
		}
		wwritebody(cwin, t, nb-i);
	}
	close(fd);
	wclean(cwin);
}

void
dispentry(Win *cwin)
{
	/* Display the current selection in window. */

	int fd, nb;
	char buf[BUFSIZE];

	if (curone == nil) {
		if (pattern != nil) {
			sprint(buf,"Pattern not found.\n");
			wwritebody(cwin, buf, 19);
			wclean(cwin);
		}
		return;
	}
	sprint(buffer,"%sp", curone);
	fd = procrexec(xprog, "-d", dict, "-c", buffer, nil);
	wreplace(cwin, "0,$","",0);	/* Clear window */
	while ((nb = read(fd, buf, BUFSIZE)) > 0) {
		wwritebody(cwin, buf, nb);
	}
	close(fd);
	wclean(cwin);
}

void
dispmatches(Win *cwin)
{
	/* Display the current matches. */

	int fd, nb;
	char buf[BUFSIZE];

	sprint(buffer,"/%s/H", pattern);
	fd = procrexec(xprog, "-d", dict, "-c", buffer, nil);
	while ((nb = read(fd, buf, BUFSIZE)) > 0)
		wwritebody(cwin, buf, nb);
	close(fd);
	wclean(cwin);
}

char*
format(char *s)
{
	/* Format a string to be written in window tag.  Acme doesn't like */
	/* non alpha-num's in the tag line. */

	char *t, *h;

	t = fbuffer;
	if (s == nil) {
		*t = 0;
		return t;
	}
	strcpy(t, s);
	h = t;
	while (*t != 0) {
		if (!(((*t >= 'a') && (*t <= 'z')) || 
		    ((*t >= 'A') && (*t <= 'Z')) ||
		    ((*t >= '0') && (*t <= '9'))))
			*t = '_';
		t++;
	}
	if (strlen(h) > MAXTAG)
		h[MAXTAG] = 0;
	if (strcmp(s,h) == 0) return s;
	return h;
}

void
openwin(char *name, char *buttons, Win *twin, int wintype)
{
	char buf[80];

	wnew(twin);
	if (wintype == Dictwin)
		sprint(buf,"%s",name);
	else
		if ((wintype == Entrywin) && (count > 1))
			sprint(buf,"%s/%s/%s/%d",name, dict, format(pattern), curindex+1);
		else
			sprint(buf,"%s/%s/%s",name, dict, format(pattern));
	wname(twin, buf);
	wtagwrite(twin, buttons, strlen(buttons));
	wclean(twin);
	wdormant(twin);
	if (wintype == Dictwin)
		dispdicts(twin);
	if (wintype == Matchwin) {
		Mopen = True;
		dispmatches(twin);
	}
	if (wintype == Entrywin) {
		Eopen = True;
		dispentry(twin);
	}
	handle(twin, wintype);
}

void
vopenwin(void *v)
{
	void **arg;
	char *name, *buttons;
	Win *twin;
	int wintype;

	arg = v;
	name = arg[0];
	buttons = arg[1];
	twin = arg[2];
	wintype = (int)arg[3];
	sendul(arg[4], 0);

	openwin(name, buttons, twin, wintype);
	threadexits(nil);
}
	
void
procopenwin(char *name, char *buttons, Win *twin, int wintype)
{
	void *arg[5];
	Channel *c;

	c = chancreate(sizeof(ulong), 0);
	arg[0] = name;
	arg[1] = buttons;
	arg[2] = twin;
	arg[3] = (void*)wintype;
	arg[4] = c;
	proccreate(vopenwin, arg, 8192);
	recvul(c);
	chanfree(c);
}

void
rexec(void *v)
{
	void **arg;
	char *prog;
	char **args;
	int *fd;
	Channel *c;

	arg = v;
	prog = arg[0];
	args = arg[1];
	fd = arg[2];
	c = arg[3];

	rfork(RFENVG|RFFDG);
	dup(fd[1], 1);
	close(fd[1]);
	close(fd[0]);
	procexec(c, prog, args);
	threadprint(2, "Remote pipe execution failed: %s %r\n", prog);
abort();
	threadexits(nil);
}

void
pexec(void *v)
{
	void **arg;
	char *prog;
	char **args;
	Channel *c;

	arg = v;
	prog = arg[0];
	args = arg[1];
	c = arg[2];

	procexec(c, prog, args);
	threadprint(2, "Remote execution failed: %s %r\n", prog);
abort();
	threadexits(nil);
}

void
procpexec(char *prog, char **args)
{
	void *rexarg[4];
	Channel *c;

	c = chancreate(sizeof(ulong), 0);
	rexarg[0] = prog;
	rexarg[1] = args;
	rexarg[2] = c;

	proccreate(pexec, rexarg, 8192);
	recvul(c);
	chanfree(c);
}

void
kill(void)
{
	/* Kill all processes related to this one. */
	int fd;

	sprint(buffer, "/proc/%d/notepg", getpid());
	fd = open(buffer, OWRITE);
	rfork(RFNOTEG);
	write(fd, "kill", 4);
}

int
command(char *com, Win *w, int wintype)
{
	char *buf;

	if (strncmp(com, "Del", 3) == 0) {
		switch(wintype){
		case Entrywin:
			if (wdel(w)) {
				Eopen = False;
				threadexits(nil);
			}
			break;
		case Dictwin:
			if (wdel(w))
				threadexits(nil);
			break;
		case Matchwin:
			kill();
			if (Eopen)
				if (~wdel(&Ewin))	/* Remove the entry window */
					wdel(&Ewin);
			if (!wdel(w))
				wdel(w);
			threadexits(nil);
			break;
		}
		return True;
	}
	if (strncmp(com, "Next", 4) == 0){
		if (curone != nil) {
			curone = chgaddr(1);
			buf = getpattern(curone);
			sprint(buffer,"%s/%s/%s", prog, dict, format(buf));
			wname(w, buffer);
			dispentry(w);
		}
		return True;
	}
	if (strncmp(com, "Prev",4) == 0){
		if (curone != nil) {
			curone = chgaddr(-1);
			buf = getpattern(curone);
			sprint(buffer,"%s/%s/%s", prog, dict, format(buf));
			wname(w, buffer);
			dispentry(w);
		}
		return True;
	}
	if (strncmp(com, "Nmatch",6) == 0){
		if (curaddr[++curindex] == nil)
			curindex = 0;
		curone = curaddr[curindex];
		if (curone != nil) {
			sprint(buffer,"%s/%s/%s/%d",prog,dict,format(pattern),curindex+1);
			wname(w, buffer);
			dispentry(w);
		}
		return True;
	}
	return False;
}

void
handle(Win *w, int wintype)
{
	Event e, e2, ea, etoss;
	char *s, *t, buf[80];
	int tmp, na;

	while (True) {
		wevent(w, &e);
		switch(e.c2){
		default:
			/* threadprint(2,"unknown message %c%c\n", e.c1, e.c2); */
			break;
		case 'i':
			/* threadprint(2,"'%s' inserted in tag at %d\n", e.b, e.q0);*/
			break;
		case 'I':
			/* threadprint(2,"'%s' inserted in body at %d\n", e.b, e.q0);*/
			break;
		case 'd':
			/* threadprint(2, "'%s' deleted in tag at %d\n", e.b, e.q0);*/
			break;
		case 'D':
			/* threadprint(2, "'%s' deleted in body at %d\n", e.b, e.q0);*/
			break;
		case 'x':
		case 'X':				/* Execute command. */
			if (e.flag & 2)
				wevent(w, &e2);
			if(e.flag & 8){
				wevent(w, &ea);
				wevent(w, &etoss);
				na = ea.nb;
			} else
				na = 0;
			s = e.b;
			if ((e.flag & 2) && e.nb == 0)
				s = e2.b;
			if(na){
				t = malloc(strlen(s)+1+na+1);
				snprint(t, strlen(s)+1+na+1, "%s %s", s, ea.b);
				s = t;
			}
			/* if it's a long message, it can't be for us anyway */
			if(!command(s, w, wintype))	/* send it back */
				wwriteevent(w, &e);
			if(na)
				free(s);
			break;
		case 'l':
		case 'L':				/* Look for something. */
			if (e.flag & 2)
				wevent(w, &e);
			wclean(w);		/* Set clean bit. */
			if (wintype == Dictwin) {
				strcpy(buf, e.b);
				args[0] = lprog;
				args[1] = "-d";
				args[2] = buf;
				args[3] = nil;
				procpexec(lprog, args);	/* New adict with chosen dict. */
			}
			if (wintype == Entrywin) {
				strcpy(buf, e.b);
				args[0] = lprog;
				args[1] = "-d";
				args[2] = dict;
				args[3] = buf;
				args[4] = nil;
				procpexec(lprog, args); /* New adict with chosen pattern. */
			}
			if (wintype == Matchwin) {
				tmp = atoi(e.b) - 1;
				if ((tmp >= 0) && (tmp < MAXMATCH) && (curaddr[tmp] != nil)) {
					curindex = tmp;
					curone = curaddr[curindex];
					/* Display selected match. */
					if (Eopen) {
						sprint(buf,"%s/%s/%s/%d",prog,dict,format(pattern),curindex+1);
						wname(&Ewin, buf);
						dispentry(&Ewin);
					}
					else
						procopenwin(prog,"Nmatch Prev Next", &Ewin, Entrywin);
				}
			}
			break;
		}
	}
}
