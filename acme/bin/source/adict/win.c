#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "win.h"

void*
erealloc(void *p, uint n)
{
	p = realloc(p, n);
	if(p == nil)
		threadprint(2, "realloc failed: %r");
	return p;
}

void
wnew(Win *w)
{
	char buf[12];

	w->ctl = open("/mnt/acme/new/ctl", ORDWR);
	if(w->ctl<0 || read(w->ctl, buf, 12)!=12)
		 threadprint (2, "can't open window ctl file: %r");
	ctlwrite(w, "noscroll\n");
	w->winid = atoi(buf);
	w->event = openfile(w, "event");
	w->addr = -1;	/* will be opened when needed */
	w->body = nil;
	w->data = -1;
}

int
openfile(Win *w, char *f)
{
	char buf[64];
	int fd;

	sprint(buf, "/mnt/acme/%d/%s", w->winid, f);
	fd = open(buf, ORDWR|OCEXEC);
	if(fd < 0)
		 threadprint (2,"can't open window %s file: %r", f);
	return fd;
}

void
openbody(Win *w, int mode)
{
	char buf[64];

	sprint(buf, "/mnt/acme/%d/body", w->winid);
	w->body = Bopen(buf, mode|OCEXEC);
	if(w->body == nil)
		 threadprint(2,"can't open window body file: %r");
}

void
wwritebody(Win *w, char *s, int n)
{
	if(w->body == nil)
		openbody(w, OWRITE);
	if(Bwrite(w->body, s, n) != n)
		  threadprint(2,"write error to window: %r");
	Bflush(w->body);
}

void
wreplace(Win *w, char *addr, char *repl, int nrepl)
{
	if(w->addr < 0)
		w->addr = openfile(w, "addr");
	if(w->data < 0)
		w->data = openfile(w, "data");
	if(write(w->addr, addr, strlen(addr)) < 0){
		threadprint(2, "mail: warning: badd address %s:%r\n", addr);
		return;
	}
	if(write(w->data, repl, nrepl) != nrepl)
		 threadprint(2, "writing data: %r");
}

static int
nrunes(char *s, int nb)
{
	int i, n;
	Rune r;

	n = 0;
	for(i=0; i<nb; n++)
		i += chartorune(&r, s+i);
	return n;
}

void
wread(Win *w, uint q0, uint q1, char *data)
{
	int m, n, nr;
	char buf[256];

	if(w->addr < 0)
		w->addr = openfile(w, "addr");
	if(w->data < 0)
		w->data = openfile(w, "data");
	m = q0;
	while(m < q1){
		n = sprint(buf, "#%d", m);
		if(write(w->addr, buf, n) != n)
			  threadprint(2,"writing addr: %r");
		n = read(w->data, buf, sizeof buf);
		if(n <= 0)
			  threadprint(2,"reading data: %r");
		nr = nrunes(buf, n);
		while(m+nr >q1){
			do; while(n>0 && (buf[--n]&0xC0)==0x80);
			--nr;
		}
		if(n == 0)
			break;
		memmove(data, buf, n);
		data += n;
		*data = 0;
		m += nr;
	}
}

void
wselect(Win *w, char *addr)
{
	if(w->addr < 0)
		w->addr = openfile(w, "addr");
	if(write(w->addr, addr, strlen(addr)) < 0)
		  threadprint(2,"writing addr");
	ctlwrite(w, "dot=addr\n");
}

void
wtagwrite(Win *w, char *s, int n)
{
	int fd;

	fd = openfile(w, "tag");
	if(write(fd, s, n) != n)
		  threadprint(2,"tag write: %r");
	close(fd);
}

void
ctlwrite(Win *w, char *s)
{
	int n;

	n = strlen(s);
	if(write(w->ctl, s, n) != n)
		 threadprint(2,"write error to ctl file: %r");
}

int
wdel(Win *w)
{
	if(write(w->ctl, "del\n", 4) != 4)
		return False;
	wdormant(w);
	close(w->ctl);
	w->ctl = -1;
	close(w->event);
	w->event = -1;
	return True;
}

void
wname(Win *w, char *s)
{
	char buf[128];

	sprint(buf, "name %s\n", s);
	ctlwrite(w, buf);
}

void
wclean(Win *w)
{
	if(w->body)
		Bflush(w->body);
	ctlwrite(w, "clean\n");
}

void
wdormant(Win *w)
{
	if(w->addr >= 0){
		close(w->addr);
		w->addr = -1;
	}
	if(w->body != nil){
		Bterm(w->body);
		w->body = nil;
	}
	if(w->data >= 0){
		close(w->data);
		w->data = -1;
	}
}

int
getec(Win *w)
{
	if(w->nbuf == 0){
		w->nbuf = read(w->event, w->buf, sizeof w->buf);
		if(w->nbuf <= 0)
			  threadprint(2,"event read error: %r");
		w->bufp = w->buf;
	}
	w->nbuf--;
	return *w->bufp++;
}

int
geten(Win *w)
{
	int n, c;

	n = 0;
	while('0'<=(c=getec(w)) && c<='9')
		n = n*10+(c-'0');
	if(c != ' ')
		 threadprint(2, "event number syntax");
	return n;
}

int
geter(Win *w, char *buf, int *nb)
{
	Rune r;
	int n;

	r = getec(w);
	buf[0] = r;
	n = 1;
	if(r < Runeself)
		goto Return;
	while(!fullrune(buf, n))
		buf[n++] = getec(w);
	chartorune(&r, buf);
    Return:
	*nb = n;
	return r;
}


void
wevent(Win *w, Event *e)
{
	int i, nb;

	e->c1 = getec(w);
	e->c2 = getec(w);
	e->q0 = geten(w);
	e->q1 = geten(w);
	e->flag = geten(w);
	e->nr = geten(w);
	if(e->nr > EVENTSIZE)
		  threadprint(2, "wevent: event string too long");
	e->nb = 0;
	for(i=0; i<e->nr; i++){
		e->r[i] = geter(w, e->b+e->nb, &nb);
		e->nb += nb;
	}
	e->r[e->nr] = 0;
	e->b[e->nb] = 0;
	if(getec(w) != '\n')
		 threadprint(2, "wevent: event syntax 2");
}

void
wslave(Win *w, Channel *ce)
{
	Event e;

	while(recv(ce, &e) >= 0)
		wevent(w, &e);
}

void
wwriteevent(Win *w, Event *e)
{
	threadprint(w->event, "%c%c%d %d\n", e->c1, e->c2, e->q0, e->q1);
}

int
wreadall(Win *w, char **sp)
{
	char *s;
	int m, na, n;

	if(w->body != nil)
		Bterm(w->body);
	openbody(w, OREAD);
	s = nil;
	na = 0;
	n = 0;
	for(;;){
		if(na < n+512){
			na += 1024;
			s = erealloc(s, na+1);
		}
		m = Bread(w->body, s+n, na-n);
		if(m <= 0)
			break;
		n += m;
	}
	s[n] = 0;
	Bterm(w->body);
	w->body = nil;
	*sp = s;
	return n;
}
