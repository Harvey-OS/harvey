#include <alef.h>
#include <bio.h>
#include "win.h"


void*
erealloc(void *p, uint n)
{
	p = realloc(p, n);
	if(p == nil)
		fprint(2, "realloc failed: %r");
	return p;
}

void
Win.wnew(Win *w)
{
	byte buf[12];

	w->ctl = open("/mnt/8½/new/ctl", ORDWR);
	if(w->ctl<0 || read(w->ctl, buf, 12)!=12)
		 fprint (2, "can't open window ctl file: %r");
	w->ctlwrite("noscroll\n");
	w->winid = atoi(buf);
	w->event = w->openfile("event");
	w->addr = -1;	/* will be opened when needed */
	w->body = nil;
	w->data = -1;
}

int
Win.openfile(Win *w, byte *f)
{
	byte buf[64];
	int fd;

	sprint(buf, "/mnt/8½/%d/%s", w->winid, f);
	fd = open(buf, ORDWR|OCEXEC);
	if(fd < 0)
		 fprint (2,"can't open window %s file: %r");
	return fd;
}

void
Win.openbody(Win *w, int mode)
{
	byte buf[64];

	sprint(buf, "/mnt/8½/%d/body", w->winid);
	w->body = Bopen(buf, mode|OCEXEC);
	if(w->body == nil)
		 fprint (2,"can't open window body file: %r");
}

void
Win.wwritebody(Win *w, byte *s, int n)
{
	if(w->body == nil)
		w->openbody(OWRITE);
	if(w->body->write(s, n) != n)
		  fprint(2,"write error to window: %r");
	w->body->flush();
}

void
Win.wreplace(Win *w, byte *addr, byte *repl, int nrepl)
{
	if(w->addr < 0)
		w->addr = w->openfile("addr");
	if(w->data < 0)
		w->data = w->openfile("data");
	if(write(w->addr, addr, strlen(addr)) < 0){
		fprint(2, "mail: warning: badd address %s:%r\n", addr);
		return;
	}
	if(write(w->data, repl, nrepl) != nrepl)
		 fprint(2, "writing data: %r");
}

intern int
nrunes(byte *s, int nb)
{
	int i, n;
	Rune r;

	n = 0;
	for(i=0; i<nb; n++)
		i += chartorune(&r, s+i);
	return n;
}

void
Win.wread(Win *w, uint q0, uint q1, byte *data)
{
	int m, n, nr;
	byte buf[256];

	if(w->addr < 0)
		w->addr = w->openfile("addr");
	if(w->data < 0)
		w->data = w->openfile("data");
	m = q0;
	while(m < q1){
		n = sprint(buf, "#%d", m);
		if(write(w->addr, buf, n) != n)
			  fprint(2,"writing addr: %r");
		n = read(w->data, buf, sizeof buf);
		if(n <= 0)
			  fprint(2,"reading data: %r");
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
Win.wselect(Win *w, byte *addr)
{
	if(w->addr < 0)
		w->addr = w->openfile("addr");
	if(write(w->addr, addr, strlen(addr)) < 0)
		  fprint(2,"writing addr");
	w->ctlwrite("dot=addr\n");
}

void
Win.wtagwrite(Win *w, byte *s, int n)
{
	int fd;

	fd = w->openfile("tag");
	if(write(fd, s, n) != n)
		  fprint(2,"tag write: %r");
	close(fd);
}

void
Win.ctlwrite(Win *w, byte *s)
{
	int n;

	n = strlen(s);
	if(write(w->ctl, s, n) != n)
		 fprint(2,"write error to ctl file: %r");
}

int
Win.wdel(Win *w)
{
	if(write(w->ctl, "del\n", 4) != 4)
		return False;
	w->wdormant();
	close(w->ctl);
	w->ctl = -1;
	close(w->event);
	w->event = -1;
	return True;
}

void
Win.wname(Win *w, byte *s)
{
	byte buf[128];

	sprint(buf, "name %s\n", s);
	w->ctlwrite(buf);
}

void
Win.wclean(Win *w)
{
	if(w->body)
		w->body->flush();
	w->ctlwrite("clean\n");
}

void
Win.wdormant(Win *w)
{
	if(w->addr >= 0){
		close(w->addr);
		w->addr = -1;
	}
	if(w->body != nil){
		w->body->term();
		w->body = nil;
	}
	if(w->data >= 0){
		close(w->data);
		w->data = -1;
	}
}

int
Win.getec(Win *w)
{
	if(w->nbuf == 0){
		w->nbuf = read(w->event, w->buf, sizeof w->buf);
		if(w->nbuf <= 0)
			  fprint(2,"event read error: %r");
		w->bufp = w->buf;
	}
	w->nbuf--;
	return *w->bufp++;
}

int
Win.geten(Win *w)
{
	int n, c;

	n = 0;
	while('0'<=(c=w->getec()) && c<='9')
		n = n*10+(c-'0');
	if(c != ' ')
		 fprint(2, "event number syntax");
	return n;
}

int
Win.geter(Win *w, byte *buf, int *nb)
{
	Rune r;
	int n;

	r = w->getec();
	buf[0] = r;
	n = 1;
	if(r < Runeself)
		goto Return;
	while(!fullrune(buf, n))
		buf[n++] = w->getec();
	chartorune(&r, buf);
    Return:
	*nb = n;
	return r;
}


void
Win.wevent(Win *w, Event *e)
{
	int i, nb;

	e->c1 = w->getec();
	e->c2 = w->getec();
	e->q0 = w->geten();
	e->q1 = w->geten();
	e->flag = w->geten();
	e->nr = w->geten();
	if(e->nr > EVENTSIZE)
		  fprint(2, "wevent: event string too long");
	e->nb = 0;
	for(i=0; i<e->nr; i++){
		e->r[i] = w->geter(e->b+e->nb, &nb);
		e->nb += nb;
	}
	e->r[e->nr] = 0;
	e->b[e->nb] = 0;
	if(w->getec() != '\n')
		 fprint(2, "wevent: event syntax 2");
}

void
Win.wslave(Win *w, chan(Event) ce)
{
	Event e;

	for(;;){
		w->wevent(&e);
		ce <-= e;
	}
}

void
Win.wwriteevent(Win *w, Event *e)
{
	fprint(w->event, "%c%c%d %d\n", e->c1, e->c2, e->q0, e->q1);
}

(int, byte*)
Win.wreadall(Win *w)
{
	byte *s;
	int m, na, n;

	if(w->body != nil)
		w->body->term();
	w->openbody(OREAD);
	s = nil;
	na = 0;
	n = 0;
	for(;;){
		if(na < n+512){
			na += 1024;
			s = erealloc(s, na+1);
		}
		m = w->body->read(s+n, na-n);
		if(m <= 0)
			break;
		n += m;
	}
	s[n] = 0;
	w->body->term();
	w->body = nil;
	return (n, s);
}
