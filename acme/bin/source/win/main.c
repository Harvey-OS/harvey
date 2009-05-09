#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include <ctype.h>
#include "dat.h"

void	mainctl(void*);
void	startcmd(char *[], int*);
void	stdout2body(void*);

int	debug;
int	notepg;
int	eraseinput;
int	dirty = 0;

Window *win;		/* the main window */

void
usage(void)
{
	fprint(2, "usage: win [command]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int i, j;
	char *dir, *tag, *name;
	char buf[1024], **av;

	quotefmtinstall();
	rfork(RFNAMEG);
	ARGBEGIN{
	case 'd':
		debug = 1;
		chatty9p++;
		break;
	case 'e':
		eraseinput = 1;
		break;
	case 'D':
{extern int _threaddebuglevel;
		_threaddebuglevel = 1<<20;
}
	}ARGEND

	if(argc == 0){
		av = emalloc(3*sizeof(char*));
		av[0] = "rc";
		av[1] = "-i";
		name = getenv("sysname");
	}else{
		av = argv;
		name = utfrrune(av[0], '/');
		if(name)
			name++;
		else
			name = av[0];
	}

	if(getwd(buf, sizeof buf) == 0)
		dir = "/";
	else
		dir = buf;
	dir = estrdup(dir);
	tag = estrdup(dir);
	tag = eappend(estrdup(tag), "/-", name);
	win = newwindow();
	snprint(buf, sizeof buf, "%d", win->id);
	putenv("winid", buf);
	winname(win, tag);
	wintagwrite(win, "Send Noscroll", 5+8);
	threadcreate(mainctl, win, STACK);
	mountcons();
	threadcreate(fsloop, nil, STACK);
	startpipe();
	startcmd(av, &notepg);

	strcpy(buf, "win");
	j = 3;
	for(i=0; i<argc && j+1+strlen(argv[i])+1<sizeof buf; i++){
		strcpy(buf+j, " ");
		strcpy(buf+j+1, argv[i]);
		j += 1+strlen(argv[i]);
	}

	ctlprint(win->ctl, "scroll");
	winsetdump(win, dir, buf);
}

int
EQUAL(char *s, char *t)
{
	while(tolower(*s) == tolower(*t++))
		if(*s++ == '\0')
			return 1;
	return 0;
}

int
command(Window *w, char *s)
{
	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	if(strcmp(s, "Delete")==0 || strcmp(s, "Del")==0){
		write(notepg, "hangup", 6);
		windel(w, 1);
		threadexitsall(nil);
		return 1;
	}
	if(EQUAL(s, "scroll")){
		ctlprint(w->ctl, "scroll\nshow");
		return 1;
	}
	if(EQUAL(s, "noscroll")){
		ctlprint(w->ctl, "noscroll");
		return 1;
	}
	return 0;
}

static long
utfncpy(char *to, char *from, int n)
{
	char *end, *e;

	e = to+n;
	if(to >= e)
		return 0;
	end = memccpy(to, from, '\0', e - to);
	if(end == nil){
		end = e;
		if(end[-1]&0x80){
			if(end-2>=to && (end[-2]&0xE0)==0xC0)
				return end-to;
			if(end-3>=to && (end[-3]&0xF0)==0xE0)
				return end-to;
			while(end>to && (*--end&0xC0)==0x80)
				;
		}
	}else
		end--;
	return end - to;
}

/* sendinput and fsloop run in the same proc (can't interrupt each other). */
static Req *q;
static Req **eq;
static int
__sendinput(Window *w, ulong q0, ulong q1)
{
	char *s, *t;
	int n, nb, eofchar;
	static int partial;
	static char tmp[UTFmax];
	Req *r;
	Rune rune;

	if(!q)
		return 0;

	r = q;
	n = 0;
	if(partial){
	Partial:
		nb = partial;
		if(nb > r->ifcall.count)
			nb = r->ifcall.count;
		memmove(r->ofcall.data, tmp, nb);
		if(nb!=partial)
			memmove(tmp, tmp+nb, partial-nb);
		partial -= nb;
		q = r->aux;
		if(q == nil)
			eq = &q;
		r->aux = nil;
		r->ofcall.count = nb;
		if(debug)
			fprint(2, "satisfy read with partial\n");
		respond(r, nil);
		return n;
	}
	if(q0==q1)
		return 0;
	s = emalloc((q1-q0)*UTFmax+1);
	n = winread(w, q0, q1, s);
	s[n] = '\0';
	t = strpbrk(s, "\n\004");
	if(t == nil){
		free(s);
		return 0;
	}
	r = q;
	eofchar = 0;
	if(*t == '\004'){
		eofchar = 1;
		*t = '\0';
	}else
		*++t = '\0';
	nb = utfncpy((char*)r->ofcall.data, s, r->ifcall.count);
	if(nb==0 && s<t && r->ifcall.count > 0){
		partial = utfncpy(tmp, s, UTFmax);
		assert(partial > 0);
		chartorune(&rune, tmp);
		partial = runelen(rune);
		free(s);
		n = 1;
		goto Partial;
	}
	n = utfnlen(r->ofcall.data, nb);
	if(nb==strlen(s) && eofchar)
		n++;
	r->ofcall.count = nb;
	q = r->aux;
	if(q == nil)
		eq = &q;
	r->aux = nil;
	if(debug)
		fprint(2, "read returns %lud-%lud: %.*q\n", q0, q0+n, n, r->ofcall.data);
	respond(r, nil);
	return n;
}

static int
_sendinput(Window *w, ulong q0, ulong *q1)
{
	char buf[32];
	int n;

	n = __sendinput(w, q0, *q1);
	if(!n || !eraseinput)
		return n;
	/* erase q0 to q0+n */
	sprint(buf, "#%lud,#%lud", q0, q0+n);
	winsetaddr(w, buf, 0);
	write(w->data, buf, 0);
	*q1 -= n;
	return 0;
}

int
sendinput(Window *w, ulong q0, ulong *q1)
{
	ulong n;
	Req *oq;

	n = 0;
	do {
		oq = q;
		n += _sendinput(w, q0+n, q1);
	} while(q != oq);
	return n;
}

Event esendinput;
void
fsloop(void*)
{
	Fsevent e;
	Req **l, *r;

	eq = &q;
	memset(&esendinput, 0, sizeof esendinput);
	esendinput.c1 = 'C';
	for(;;){
		while(recv(fschan, &e) == -1)
			;
		r = e.r;
		switch(e.type){
		case 'r':
			*eq = r;
			r->aux = nil;
			eq = &r->aux;
			/* call sendinput with hostpt and endpt */
			sendp(win->cevent, &esendinput);
			break;
		case 'f':
			for(l=&q; *l; l=&(*l)->aux){
				if(*l == r->oldreq){
					*l = (*l)->aux;
					if(*l == nil)
						eq = l;
					respond(r->oldreq, "interrupted");
					break;
				}
			}
			respond(r, nil);
			break;
		}
	}
}	

void
sendit(char *s)
{
//	char tmp[32];

	write(win->body, s, strlen(s));
/*
 * RSC: The problem here is that other procs can call sendit,
 * so we lose our single-threadedness if we call sendinput.
 * In fact, we don't even have the right queue memory,
 * I think that we'll get a write event from the body write above,
 * and we can do the sendinput then, from our single thread.
 *
 * I still need to figure out how to test this assertion for
 * programs that use /srv/win*
 *
	winselect(win, "$", 0);
	seek(win->addr, 0UL, 0);
	if(read(win->addr, tmp, 2*12) == 2*12)
		hostpt += sendinput(win, hostpt, atol(tmp), );
 */
}

void
execevent(Window *w, Event *e, int (*command)(Window*, char*))
{
	Event *ea, *e2;
	int n, na, len, needfree;
	char *s, *t;

	ea = nil;
	e2 = nil;
	if(e->flag & 2)
		e2 = recvp(w->cevent);
	if(e->flag & 8){
		ea = recvp(w->cevent);
		na = ea->nb;
		recvp(w->cevent);
	}else
		na = 0;

	needfree = 0;
	s = e->b;
	if(e->nb==0 && (e->flag&2)){
		s = e2->b;
		e->q0 = e2->q0;
		e->q1 = e2->q1;
		e->nb = e2->nb;
	}
	if(e->nb==0 && e->q0<e->q1){
		/* fetch data from window */
		s = emalloc((e->q1-e->q0)*UTFmax+2);
		n = winread(w, e->q0, e->q1, s);
		s[n] = '\0';
		needfree = 1;
	}else 
	if(na){
		t = emalloc(strlen(s)+1+na+2);
		sprint(t, "%s %s", s, ea->b);
		if(needfree)
			free(s);
		s = t;
		needfree = 1;
	}

	/* if it's a known command, do it */
	/* if it's a long message, it can't be for us anyway */
	if(!command(w, s) && s[0]!='\0'){	/* send it as typed text */
		/* if it's a built-in from the tag, send it back */
		if(e->flag & 1)
			fprint(w->event, "%c%c%d %d\n", e->c1, e->c2, e->q0, e->q1);
		else{	/* send text to main window */
			len = strlen(s);
			if(len>0 && s[len-1]!='\n' && s[len-1]!='\004'){
				if(!needfree){
					/* if(needfree), we left room for a newline before */
					t = emalloc(len+2);
					strcpy(t, s);
					s = t;
					needfree = 1;
				}
				s[len++] = '\n';
				s[len] = '\0';
			}
			sendit(s);
		}
	}
	if(needfree)
		free(s);
}

int
hasboundary(Rune *r, int nr)
{
	int i;

	for(i=0; i<nr; i++)
		if(r[i]=='\n' || r[i]=='\004')
			return 1;
	return 0;
}

void
mainctl(void *v)
{
	Window *w;
	Event *e;
	int delta, pendingS, pendingK;
	ulong hostpt, endpt;
	char tmp[32];

	w = v;
	proccreate(wineventproc, w, STACK);

	hostpt = 0;
	endpt = 0;
	winsetaddr(w, "0", 0);
	pendingS = 0;
	pendingK = 0;
	for(;;){
		if(debug)
			fprint(2, "input range %lud-%lud\n", hostpt, endpt);
		e = recvp(w->cevent);
		if(debug)
			fprint(2, "msg: %C %C %d %d %d %d %q\n",
				e->c1 ? e->c1 : ' ', e->c2 ? e->c2 : ' ', e->q0, e->q1, e->flag, e->nb, e->b);
		switch(e->c1){
		default:
		Unknown:
			fprint(2, "unknown message %c%c\n", e->c1, e->c2);
			break;

		case 'C':	/* input needed for /dev/cons */
			if(pendingS)
				pendingK = 1;
			else
				hostpt += sendinput(w, hostpt, &endpt);
			break;

		case 'S':	/* output to stdout */
			sprint(tmp, "#%lud", hostpt);
			winsetaddr(w, tmp, 0);
			write(w->data, e->b, e->nb);
			pendingS += e->nr;
			break;
	
		case 'E':	/* write to tag or body; body happens due to sendit */
			delta = e->q1-e->q0;
			if(e->c2=='I'){
				endpt += delta;
				if(e->q0 < hostpt)
					hostpt += delta;
				else
					hostpt += sendinput(w, hostpt, &endpt);
				break;
			}
			if(!islower(e->c2))
				fprint(2, "win msg: %C %C %d %d %d %d %q\n",
					e->c1, e->c2, e->q0, e->q1, e->flag, e->nb, e->b);
			break;
	
		case 'F':	/* generated by our actions (specifically case 'S' above) */
			delta = e->q1-e->q0;
			if(e->c2=='D'){
				/* we know about the delete by _sendinput */
				break;
			}
			if(e->c2=='I'){
				pendingS -= e->q1 - e->q0;
				if(pendingS < 0)
					fprint(2, "win: pendingS = %d\n", pendingS);
				if(e->q0 != hostpt)
					fprint(2, "win: insert at %d expected %lud\n", e->q0, hostpt);
				endpt += delta;
				hostpt += delta;
				sendp(writechan, nil);
				if(pendingS == 0 && pendingK){
					pendingK = 0;
					hostpt += sendinput(w, hostpt, &endpt);
				}
				break;
			}
			if(!islower(e->c2))
				fprint(2, "win msg: %C %C %d %d %d %d %q\n",
					e->c1, e->c2, e->q0, e->q1, e->flag, e->nb, e->b);
			break;

		case 'K':
			delta = e->q1-e->q0;
			switch(e->c2){
			case 'D':
				endpt -= delta;
				if(e->q1 < hostpt)
					hostpt -= delta;
				else if(e->q0 < hostpt)
					hostpt = e->q0;
				break;
			case 'I':
				delta = e->q1 - e->q0;
				endpt += delta;
				if(endpt < e->q1)	/* just in case */
					endpt = e->q1;
				if(e->q0 < hostpt)
					hostpt += delta;
				if(e->nr>0 && e->r[e->nr-1]==0x7F){
					write(notepg, "interrupt", 9);
					hostpt = endpt;
					break;
				}
				if(e->q0 >= hostpt
				&& hasboundary(e->r, e->nr)){
					/*
					 * If we are between the S message (which
					 * we processed by inserting text in the
					 * window) and the F message notifying us
					 * that the text has been inserted, then our
					 * impression of the hostpt and acme's
					 * may be different.  This could be seen if you
					 * hit enter a bunch of times in a con
					 * session.  To work around the unreliability,
					 * only send input if we don't have an S pending.
					 * The same race occurs between when a character
					 * is typed and when we get notice of it, but
					 * since characters tend to be typed at the end
					 * of the buffer, we don't run into it.  There's
					 * no workaround possible for this typing race,
					 * since we can't tell when the user has typed
					 * something but we just haven't been notified.
					 */
					if(pendingS)
						pendingK = 1;
					else
						hostpt += sendinput(w, hostpt, &endpt);
				}
				break;
			}
			break;
	
		case 'M':	/* mouse */
			delta = e->q1-e->q0;
			switch(e->c2){
			case 'x':
			case 'X':
				execevent(w, e, command);
				break;
	
			case 'l':	/* reflect all searches back to acme */
			case 'L':
				if(e->flag & 2)
					recvp(w->cevent);
				winwriteevent(w, e);
				break;
	
			case 'I':
				endpt += delta;
				if(e->q0 < hostpt)
					hostpt += delta;
				else
					hostpt += sendinput(w, hostpt, &endpt);
				break;

			case 'D':
				endpt -= delta;
				if(e->q1 < hostpt)
					hostpt -= delta;
				else if(e->q0 < hostpt)
					hostpt = e->q0;
				break;
			case 'd':	/* modify away; we don't care */
			case 'i':
				break;
	
			default:
				goto Unknown;
			}
		}
	}
}

enum
{
	NARGS		= 100,
	NARGCHAR	= 8*1024,
	EXECSTACK 	= STACK+(NARGS+1)*sizeof(char*)+NARGCHAR
};

struct Exec
{
	char		**argv;
	Channel	*cpid;
};

int
lookinbin(char *s)
{
	if(s[0] == '/')
		return 0;
	if(s[0]=='.' && s[1]=='/')
		return 0;
	if(s[0]=='.' && s[1]=='.' && s[2]=='/')
		return 0;
	return 1;
}

/* adapted from mail.  not entirely free of details from that environment */
void
execproc(void *v)
{
	struct Exec *e;
	char *cmd, **av;
	Channel *cpid;

	e = v;
	rfork(RFCFDG|RFNOTEG);
	av = e->argv;
	close(0);
	open("/dev/cons", OREAD);
	close(1);
	open("/dev/cons", OWRITE);
	dup(1, 2);
	cpid = e->cpid;
	free(e);
	procexec(cpid, av[0], av);
	if(lookinbin(av[0])){
		cmd = estrstrdup("/bin/", av[0]);
		procexec(cpid, cmd, av);
	}
	error("can't exec %s: %r", av[0]);
}

void
startcmd(char *argv[], int *notepg)
{
	struct Exec *e;
	Channel *cpid;
	char buf[64];
	int pid;

	e = emalloc(sizeof(struct Exec));
	e->argv = argv;
	cpid = chancreate(sizeof(ulong), 0);
	e->cpid = cpid;
	sprint(buf, "/mnt/wsys/%d", win->id);
	bind(buf, "/dev/acme", MREPL);
	proccreate(execproc, e, EXECSTACK);
	do
		pid = recvul(cpid);
	while(pid == -1);
	sprint(buf, "/proc/%d/notepg", pid);
	*notepg = open(buf, OWRITE);
}
