#include "awiki.h"

Wiki *wlist;

void
link(Wiki *w)
{
	if(w->linked)
		return;
	w->linked = 1;
	w->prev = nil;
	w->next = wlist;
	if(wlist)
		wlist->prev = w;
	wlist = w;
}

void
unlink(Wiki *w)
{
	if(!w->linked)
		return;
	w->linked = 0;

	if(w->next)
		w->next->prev = w->prev;
	if(w->prev)
		w->prev->next = w->next;
	else
		wlist = w->next;

	w->next = nil;
	w->prev = nil;
}

void
wikiname(Window *w, char *name)
{
	char *p, *q;

	p = emalloc(strlen(dir)+1+strlen(name)+1+1);
	strcpy(p, dir);
	strcat(p, "/");
	strcat(p, name);
	for(q=p; *q; q++)
		if(*q==' ')
			*q = '_';
	winname(w, p);
	free(p);
}

int
wikiput(Wiki *w)
{
	int fd, n;
	char buf[1024], *p;
	Biobuf *b;

	if((fd = open("new", ORDWR)) < 0){
		fprint(2, "Wiki: cannot open raw: %r\n");
		return -1;
	}

	winopenbody(w->win, OREAD);
	b = w->win->body;
	if((p = Brdline(b, '\n'))==nil){
	Short:
		winclosebody(w->win);
		fprint(2, "Wiki: no data\n");
		close(fd);
		return -1;
	}
	write(fd, p, Blinelen(b));

	snprint(buf, sizeof buf, "D%lud\n", w->time);
	if(email)
		snprint(buf+strlen(buf), sizeof(buf)-strlen(buf), "A%s\n", email);

	if(Bgetc(b) == '#'){
		p = Brdline(b, '\n');
		if(p == nil)
			goto Short;
		snprint(buf+strlen(buf), sizeof(buf)-strlen(buf), "C%s\n", p);
	}
	write(fd, buf, strlen(buf));
	write(fd, "\n\n", 2);

	while((n = Bread(b, buf, sizeof buf)) > 0)
		write(fd, buf, n);
	winclosebody(w->win);

	werrstr("");
	if((n=write(fd, "", 0)) != 0){
		fprint(2, "Wiki commit %lud %d %d: %r\n", w->time, fd, n);
		close(fd);
		return -1;
	}
	seek(fd, 0, 0);
	if((n = read(fd, buf, 300)) < 0){
		fprint(2, "Wiki readback: %r\n");
		close(fd);
		return -1;
	}
	close(fd);
	buf[n] = '\0';
	sprint(buf, "%s/", buf);
	free(w->arg);
	w->arg = estrdup(buf);
	w->isnew = 0;
	wikiget(w);
	wikiname(w->win, w->arg);
	return n;
}

void
wikiget(Wiki *w)
{
	char *p;
	int fd, normal;
	Biobuf *bin;

	fprint(w->win->ctl, "dirty\n");

	p = emalloc(strlen(w->arg)+8+1);
	strcpy(p, w->arg);
	normal = 1;
	if(p[strlen(p)-1] == '/'){
		normal = 0;
		strcat(p, "current");
	}else if(strlen(p)>8 && strcmp(p+strlen(p)-8, "/current")==0){
		normal = 0;
		w->arg[strlen(w->arg)-7] = '\0';
	}

	if((fd = open(p, OREAD)) < 0){
		fprint(2, "Wiki: cannot read %s: %r\n", p);
		winclean(w->win);
		return;
	}
	free(p);

	winopenbody(w->win, OWRITE);
	bin = emalloc(sizeof(*bin));
	Binit(bin, fd, OREAD);

	p = nil;
	if(!normal){
		if((p = Brdline(bin, '\n')) == nil){
			fprint(2, "Wiki: cannot read title: %r\n");
			winclean(w->win);
			close(fd);
			free(bin);
			return;
		}
		p[Blinelen(bin)-1] = '\0';
	}
	/* clear window */
	if(w->win->data < 0)
		w->win->data = winopenfile(w->win, "data");
	if(winsetaddr(w->win, ",", 0))
		write(w->win->data, "", 0);

	if(!normal)
		Bprint(w->win->body, "%s\n\n", p);

	while(p = Brdline(bin, '\n')){
		p[Blinelen(bin)-1] = '\0';
		if(normal)
			Bprint(w->win->body, "%s\n", p);
		else{
			if(p[0]=='D')
				w->time = strtoul(p+1, 0, 10);
			else if(p[0]=='#')
				Bprint(w->win->body, "%s\n", p+1);
		}
	}
	winclean(w->win);
	free(bin);
	close(fd);
}

static int
iscmd(char *s, char *cmd)
{
	int len;

	len = strlen(cmd);
	return strncmp(s, cmd, len)==0 && (s[len]=='\0' || s[len]==' ' || s[len]=='\t' || s[len]=='\n');
}

static char*
skip(char *s, char *cmd)
{
	s += strlen(cmd);
	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	return s;
}

int
wikiload(Wiki *w, char *arg)
{
	char *p, *q, *path, *addr;
	int rv;

	p = nil;
	if(arg[0] == '/')
		path = arg;
	else{
		p = emalloc(strlen(w->arg)+1+strlen(arg)+1);
		strcpy(p, w->arg);
		if(q = strrchr(p, '/')){
			++q;
			*q = '\0';
		}else
			*p = '\0';
		strcat(p, arg);
		cleanname(p);
		path = p;
	}
	if(addr=strchr(path, ':'))
		*addr++ = '\0';

	rv = wikiopen(path, addr)==0;
	free(p);
	if(rv)
		return 1;
	return wikiopen(arg, 0)==0;
}

/* return 1 if handled, 0 otherwise */
int
wikicmd(Wiki *w, char *s)
{
	char *p;
	s = skip(s, "");

	if(iscmd(s, "Del")){
		if(windel(w->win, 0))
			w->dead = 1;
		return 1;
	}
	if(iscmd(s, "New")){
		wikinew(skip(s, "New"));
		return 1;
	}
	if(iscmd(s, "History"))
		return wikiload(w, "history.txt");
	if(iscmd(s, "Diff"))
		return wikidiff(w);
	if(iscmd(s, "Get")){
		if(winisdirty(w->win) && !w->win->warned){
			w->win->warned = 1;
			fprint(2, "%s/%s modified\n", dir, w->arg);
		}else{
			w->win->warned = 0;
			wikiget(w);
		}
		return 1;
	}
	if(iscmd(s, "Put")){
		if((p=strchr(w->arg, '/')) && p[1]!='\0')
			fprint(2, "%s/%s is read-only\n", dir, w->arg);
		else
			wikiput(w);
		return 1;
	}
	return 0;
}

/* need to expand selection more than default word */
static long
eval(Window *w, char *s, ...)
{
	char buf[64];
	va_list arg;

	va_start(arg, s);
	vsnprint(buf, sizeof buf, s, arg);
	va_end(arg);

	if(winsetaddr(w, buf, 1)==0)
		return -1;

	if(pread(w->addr, buf, 24, 0) != 24)
		return -1;
	return strtol(buf, 0, 10);
}

static int
getdot(Window *w, long *q0, long *q1)
{
	char buf[24];

	ctlprint(w->ctl, "addr=dot\n");
	if(pread(w->addr, buf, 24, 0) != 24)
		return -1;
	*q0 = atoi(buf);
	*q1 = atoi(buf+12);
	return 0;
}

static Event*
expand(Window *w, Event *e, Event *eacme)
{
	long q0, q1, x;

	if(getdot(w, &q0, &q1)==0 && q0 <= e->q0 && e->q0 <= q1){
		e->q0 = q0;
		e->q1 = q1;
		return e;
	}

	q0 = eval(w, "#%lud-/\\[/", e->q0);
	if(q0 < 0)
		return eacme;
	if(eval(w, "#%lud+/\\]/", q0) < e->q0)	/* [ closes before us */
		return eacme;
	q1 = eval(w, "#%lud+/\\]/", e->q1);
	if(q1 < 0)
		return eacme;
	if((x=eval(w, "#%lud-/\\[/", q1))==-1 || x > e->q1)	/* ] opens after us */
		return eacme;
	e->q0 = q0+1;
	e->q1 = q1;
	return e;
}

void
acmeevent(Wiki *wiki, Event *e)
{
	Event *ea, *e2, *eq;
	Window *w;
	char *s, *t, *buf;
	int na;

	w = wiki->win;
	switch(e->c1){	/* origin of action */
	default:
	Unknown:
		fprint(2, "unknown message %c%c\n", e->c1, e->c2);
		break;

	case 'F':	/* generated by our actions; ignore */
		break;

	case 'E':	/* write to body or tag; can't affect us */
		break;

	case 'K':	/* type away; we don't care */
		if(e->c2 == 'I' || e->c2 == 'D')
			w->warned = 0;
		break;

	case 'M':	/* mouse event */
		switch(e->c2){		/* type of action */
		case 'x':	/* mouse: button 2 in tag */
		case 'X':	/* mouse: button 2 in body */
			ea = nil;
			//e2 = nil;
			s = e->b;
			if(e->flag & 2){	/* null string with non-null expansion */
				e2 = recvp(w->cevent);
				if(e->nb==0)
					s = e2->b;
			}
			if(e->flag & 8){	/* chorded argument */
				ea = recvp(w->cevent);	/* argument */
				na = ea->nb;
				recvp(w->cevent);		/* ignore origin */
			}else
				na = 0;
			
			/* append chorded arguments */
			if(na){
				t = emalloc(strlen(s)+1+na+1);
				sprint(t, "%s %s", s, ea->b);
				s = t;
			}
			/* if it's a known command, do it */
			/* if it's a long message, it can't be for us anyway */
		//	DPRINT(2, "exec: %s\n", s);
			if(!wikicmd(wiki, s))	/* send it back */
				winwriteevent(w, e);
			if(na)
				free(s);
			break;

		case 'l':	/* mouse: button 3 in tag */
		case 'L':	/* mouse: button 3 in body */
			//buf = nil;
			eq = e;
			if(e->flag & 2){	/* we do our own expansion for loads */
				e2 = recvp(w->cevent);
				eq = expand(w, eq, e2);
			}
			s = eq->b;
			if(eq->q1>eq->q0 && eq->nb==0){
				buf = emalloc((eq->q1-eq->q0)*UTFmax+1);
				winread(w, eq->q0, eq->q1, buf);
				s = buf;
			}
			if(!wikiload(wiki, s))
				winwriteevent(w, e);
			break;

		case 'i':	/* mouse: text inserted in tag */
		case 'd':	/* mouse: text deleted from tag */
			break;

		case 'I':	/* mouse: text inserted in body */
		case 'D':	/* mouse: text deleted from body */
			w->warned = 0;
			break;

		default:
			goto Unknown;
		}
	}
}

void
wikithread(void *v)
{
	char tmp[40];
	Event *e;
	Wiki *w;

	w = v;

	if(w->isnew){
		sprint(tmp, "+new+%d", w->isnew);
		wikiname(w->win, tmp);
		if(w->arg){
			winopenbody(w->win, OWRITE);
			Bprint(w->win->body, "%s\n\n", w->arg);
		}
		winclean(w->win);
	}else if(!w->special){
		wikiget(w);
		wikiname(w->win, w->arg);
		if(w->addr)
			winselect(w->win, w->addr, 1);
	}
	fprint(w->win->ctl, "menu\n");
	wintagwrite(w->win, "Get History Diff New", 4+8+4+4);
	winclean(w->win);
		
	while(!w->dead && (e = recvp(w->win->cevent)))
		acmeevent(w, e);

	windormant(w->win);
	unlink(w);
	free(w->win);
	free(w->arg);
	free(w);
	threadexits(nil);
}

int
wikiopen(char *arg, char *addr)
{
	Dir *d;
	char *p;
	Wiki *w;

/*
	if(arg==nil){
		if(write(mapfd, title, strlen(title)) < 0
		|| seek(mapfd, 0, 0) < 0 || (n=read(mapfd, tmp, sizeof(tmp)-2)) < 0){
			fprint(2, "Wiki: no page '%s' found: %r\n", title);
			return -1;
		}
		if(tmp[n-1] == '\n')
			tmp[--n] = '\0';
		tmp[n++] = '/';
		tmp[n] = '\0';
		arg = tmp;
	}
*/

	/* replace embedded '\n' in links by ' ' */
	for(p=arg; *p; p++)
		if(*p=='\n')
			*p = ' ';

	if(strncmp(arg, dir, strlen(dir))==0 && arg[strlen(dir)]=='/' && arg[strlen(dir)+1])
		arg += strlen(dir)+1;
	else if(arg[0] == '/')
		return -1;

	if((d = dirstat(arg)) == nil)
		return -1;

	if((d->mode&DMDIR) && arg[strlen(arg)-1] != '/'){
		p = emalloc(strlen(arg)+2);
		strcpy(p, arg);
		strcat(p, "/");
		arg = p;
	}else if(!(d->mode&DMDIR) && arg[strlen(arg)-1]=='/'){
		arg = estrdup(arg);
		arg[strlen(arg)-1] = '\0';
	}else
		arg = estrdup(arg);
	free(d);

	/* rewrite /current into / */
	if(strlen(arg) > 8 && strcmp(arg+strlen(arg)-8, "/current")==0)
		arg[strlen(arg)-8+1] = '\0';

	/* look for window already open */
	for(w=wlist; w; w=w->next){
		if(strcmp(w->arg, arg)==0){
			ctlprint(w->win->ctl, "show\n");
			return 0;
		}
	}

	w = emalloc(sizeof *w);
	w->arg = arg;
	w->addr = addr;
	w->win = newwindow();
	link(w);

	proccreate(wineventproc, w->win, STACK);
	threadcreate(wikithread, w, STACK);
	return 0;
}

void
wikinew(char *arg)
{
	static int n;
	Wiki *w;

	w = emalloc(sizeof *w);
	if(arg)
		arg = estrdup(arg);
	w->arg = arg;
	w->win = newwindow();
	w->isnew = ++n;
	proccreate(wineventproc, w->win, STACK);
	threadcreate(wikithread, w, STACK);
}

typedef struct Diffarg Diffarg;
struct Diffarg {
	Wiki *w;
	char *dir;
};

void
execdiff(void *v)
{
	char buf[64];
	Diffarg *a;

	a = v;

	rfork(RFFDG);
	close(0);
	open("/dev/null", OREAD);
	sprint(buf, "/mnt/wsys/%d/body", a->w->win->id);
	close(1);
	open(buf, OWRITE);
	close(2);
	open(buf, OWRITE);
	sprint(buf, "/mnt/wsys/%d", a->w->win->id);
	bind(buf, "/dev", MBEFORE);
	
	procexecl(nil, "/acme/wiki/wiki.diff", "wiki.diff", a->dir, nil);
}

int
wikidiff(Wiki *w)
{
	Diffarg *d;
	char *p, *q, *r;
	Wiki *nw;

	p = emalloc(strlen(w->arg)+10);
	strcpy(p, w->arg);
	if(q = strchr(p, '/'))
		*q = '\0';
	r = estrdup(p);
	strcat(p, "/+Diff");

	nw = emalloc(sizeof *w);
	nw->arg = p;
	nw->win = newwindow();
	nw->special = 1;

	d = emalloc(sizeof(*d));
	d->w = nw;
	d->dir = r;
	wikiname(nw->win, p);
	proccreate(wineventproc, nw->win, STACK);
	proccreate(execdiff, d, STACK);
	threadcreate(wikithread, nw, STACK);
	return 1;
}

