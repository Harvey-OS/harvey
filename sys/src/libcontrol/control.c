#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

static Controlset **controlset;
int	ncontrolset;
int	ctldeletequits;

char *alignnames[Nalignments] = {
	[Aupperleft] =		"upperleft",
	[Auppercenter] =	"uppercenter",
	[Aupperright] =		"upperright",
	[Acenterleft] =		"centerleft",
	[Acenter] =		"center",
	[Acenterright] =	"centerright",
	[Alowerleft] =		"lowerleft",
	[Alowercenter] =	"lowercenter",
	[Alowerright] =		"lowerright",
};

static Control*
_newcontrol(Controlset *cs, uint n, char *name)
{
	Control *c;

	for(c=cs->controls; c; c=c->next)
		if(strcmp(c->name, name) == 0){
			werrstr("control %q already defined", name);
			return nil;
		}
	c = ctlmalloc(n);
	c->screen = cs->screen;
	c->name = ctlstrdup(name);
	c->event = chancreate(sizeof(char*), 5);
	c->ctl = chancreate(sizeof(char*), 5);
	c->data = chancreate(sizeof(char*), 0);
	c->kbd = chancreate(sizeof(Rune*), 20);
	c->mouse = chancreate(sizeof(Mouse), 2);
	c->wire = chancreate(sizeof(CWire), 0);
	c->exit = chancreate(sizeof(int), 0);

	c->alts = ctlmalloc((NALT+1)*sizeof(Alt));
	c->alts[AKey].c = c->kbd;
	c->alts[AKey].v = &c->kbdr;
	c->alts[AKey].op = CHANRCV;
	c->alts[AMouse].c = c->mouse;
	c->alts[AMouse].v = &c->m;
	c->alts[AMouse].op = CHANRCV;
	c->alts[ACtl].c = c->ctl;
	c->alts[ACtl].v = &c->str;
	c->alts[ACtl].op = CHANRCV;
	c->alts[AWire].c = c->wire;
	c->alts[AWire].v = &c->cwire;
	c->alts[AWire].op = CHANRCV;
	c->alts[AExit].c = c->exit;
	c->alts[AExit].v = nil;
	c->alts[AExit].op = CHANRCV;
	c->alts[NALT].op = CHANEND;

	c->controlset = cs;
	c->next = cs->controls;
	cs->controls = c;
	return c;
}

Control*
_createctl(Controlset *cs, char *type, uint size, char *name, void (*f)(void*), int stksize)
{
	Control *c;

	c = _newcontrol(cs, size, name);
	if(c == nil)
		ctlerror("can't create %q: %r", type, name);
	if(stksize == 0)
		stksize = 4096;
	if(threadcreate(f, c, stksize) < 0)
		ctlerror("can't create %s thread for %q: %r", type, name);
	return c;
}

void
closecontrol(Control *c)
{
	Control *prev, *p;

	if(c == nil)
		return;
	sendul(c->exit, 0);
	if(recvul(c->exit) != 0)
		ctlerror("can't communicate with %q", c->name);

	prev = nil;
	for(p=c->controlset->controls; p; p=p->next){
		if(p == c)
			break;
		prev = p;
	}
	if(p == nil)
		ctlerror("closecontrol: no such control %q %p\n", c->name, c);
	if(prev == nil)
		c->controlset->controls = c->next;
	else
		prev->next = c->next;

	/* is it active? if so, delete from active list */
	prev = nil;
	for(p=c->controlset->actives; p; p=p->nextactive){
		if(p == c)
			break;
		prev = p;
	}
	if(p != nil){
		if(prev == nil)
			c->controlset->actives = c->nextactive;
		else
			prev->nextactive = c->nextactive;
	}

	if(!c->wevent)
		chanfree(c->event);
	if(!c->wctl)
		chanfree(c->ctl);
	if(!c->wdata)
		chanfree(c->data);
	chanfree(c->kbd);
	chanfree(c->mouse);
	chanfree(c->wire);
	chanfree(c->exit);
	free(c->name);
	free(c->format);
	free(c->alts);
	free(c);
}

Control*
controlcalled(char *name)
{
	Control *c;
	int i;

	for(i=0; i<ncontrolset; i++)
		for(c=controlset[i]->controls; c; c=c->next)
			if(strcmp(c->name, name) == 0)
				return c;
	return nil;
}

void
ctlerror(char *fmt, ...)
{
	va_list arg;
	char buf[256];

	va_start(arg, fmt);
	doprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);
	threadprint(2, "%s\n", buf);
	threadexitsall(buf);
}

Rune*
_ctlrunestr(char *s)
{
	Rune *r, *ret;

	ret = r = ctlmalloc((utflen(s)+1)*sizeof(Rune));
	while(*s != '\0')
		s += chartorune(r++, s);
	*r = L'\0';
	return ret;
}

char*
_ctlstrrune(Rune *r)
{
	char *s;
	s = ctlmalloc(runestrlen(r)*UTFmax+1);
	sprint(s, "%S", r);
	return s;
}

void*
ctlmalloc(uint n)
{
	void *p;

	p = mallocz(n, 1);
	if(p == nil)
		ctlerror("control allocation failed: %r");
	return p;
}

void*
ctlrealloc(void *p, uint n)
{
	p = realloc(p, n);
	if(p == nil)
		ctlerror("control reallocation failed: %r");
	return p;
}

char*
ctlstrdup(char *s)
{
	char *t;

	t = strdup(s);
	if(t == nil)
		ctlerror("control strdup(%q) failed: %r", s);
	return t;
}

int
printctl(Channel *c, char *fmt, ...)
{
	va_list arg;
	char *buf;
	enum {NBUF=1024};
	int n;

	buf = malloc(NBUF);
	if(buf == nil)
		ctlerror("malloc failed: %r");
	va_start(arg, fmt);
	doprint(buf, buf+NBUF, fmt, arg);
	va_end(arg);
	n = sendp(c, buf);
	return n;
}

static void
ctokenize(char *s, CParse *cp)
{
	int i;

	snprint(cp->str, sizeof cp->str, "%s", s);
	cp->nargs = tokenize(s, cp->args, nelem(cp->args));
	for(i=1; i<cp->nargs; i++)
		cp->iargs[i] = atoi(cp->args[i]);
	cp->sender = cp->args[0];
}


int
_ctlparse(CParse *cp, char *s, char **tab)
{
	int i, j;
	char *t;

	/* keep original string for good error messages */
	i = strlen(s);
	if(i >= sizeof(cp->str))
		i = sizeof(cp->str)-1;
	memmove(cp->str, s, i);
	cp->str[i] = '\0';
	ctokenize(s, cp);
	if(cp->nargs == 0)
		return -1;
	/* strip leading sender name if present */
	cp->sender = nil;
	i = strlen(cp->args[0])-1;
	if(cp->args[0][i] == ':'){
		cp->sender = cp->args[0];
		cp->sender[i] = '\0';
		cp->nargs--;
		memmove(cp->args, cp->args+1, cp->nargs*sizeof(char*));
	}
	for(i=1; i<cp->nargs; i++){
		t = cp->args[i];
		while(*t == '[')	/* %R gives [0 0] [1 1]; atoi will stop at closing ] */
			t++;
		cp->iargs[i] = atoi(t);
	}
	for(j=0; tab[j] != nil; j++)
		if(strcmp(tab[j], cp->args[0]) == 0)
			return j;
	return -1;
}

void
_ctlargcount(Control *c, CParse *cp, int n)
{
	if(cp->nargs != n)
		ctlerror("%q: wrong argument count in '%s'", c->name, cp->str);
}

void
_ctlcontrol(Control *c, char *s, void (*ctlf)(Control*, char*))
{
	char *lines[16];
	int i, n;
	char *l;

	n = getfields(s, lines, nelem(lines), 0, "\n");
	for(i=0; i<n; i++){
		l = lines[i];
		while(*l==' ' || *l=='\t')
			l++;
		if(*l != '\0')
			ctlf(c, l);
	}
}

Rune*
_ctlgetsnarf(void)
{
	int i, n;
	char *sn, buf[512];
	Rune *snarf;

	if(_ctlsnarffd < 0)
		return nil;
	sn = nil;
	i = 0;
	seek(_ctlsnarffd, 0, 0);
	while((n = read(_ctlsnarffd, buf, sizeof buf)) > 0){
		sn = ctlrealloc(sn, i+n+1);
		memmove(sn+i, buf, n);
		i += n;
		sn[i] = 0;
	}
	snarf = nil;
	if(i > 0){
		snarf = _ctlrunestr(sn);
		free(sn);
	}
	return snarf;
}

void
_ctlputsnarf(Rune *snarf)
{
	int fd, i, n, nsnarf;

	if(_ctlsnarffd<0 || snarf[0]==0)
		return;
	fd = open("/dev/snarf", OWRITE);
	if(fd < 0)
		return;
	nsnarf = runestrlen(snarf);
	/* snarf buffer could be huge, so fprint will truncate; do it in blocks */
	for(i=0; i<nsnarf; i+=n){
		n = nsnarf-i;
		if(n >= 256)
			n = 256;
		if(threadprint(fd, "%.*S", n, snarf+i) < 0)
			break;
	}
	close(fd);
}

int
_ctlalignment(char *s)
{
	int i;

	for(i=0; i<Nalignments; i++)
		if(strcmp(s, alignnames[i]) == 0)
			return i;
	ctlerror("unknown alignment: %s", s);
	return 0;
}

Point
_ctlalignpoint(Rectangle r, int dx, int dy, int align)
{
	Point p;

	p = r.min;	/* in case of trouble */
	switch(align%3){
	case 0:	/* left */
		p.x = r.min.x;
		break;
	case 1:	/* center */
		p.x = r.min.x+(Dx(r)-dx)/2;
		break;
	case 2:	/* right */
		p.x = r.max.x-dx;
		break;
	}
	switch((align/3)%3){
	case 0:	/* top */
		p.y = r.min.y;
		break;
	case 1:	/* center */
		p.y = r.min.y+(Dy(r)-dy)/2;
		break;
	case 2:	/* bottom */
		p.y = r.max.y - dy;
		break;
	}
	return p;
}

void
_ctlrewire(Control *c)
{
	Channel **p;
	int i;

	p = nil;
	i = -1;
	if(strcmp(c->cwire.name, "ctl") == 0){
		p = &c->ctl;
		c->wctl = 1;
		i = ACtl;
	}else if(strcmp(c->cwire.name, "event") == 0){
		p = &c->event;
		c->wevent = 1;
	}else if(strcmp(c->cwire.name, "data") == 0){
		p = &c->data;
		c->wdata = 1;
	}else
		ctlerror("%q: unknown controlwire channel %s", c->name, c->cwire.name);
	chanfree(*p);
	*p = c->cwire.chan;
	if(i >= 0)
		c->alts[i].c = *p;
	sendp(c->cwire.sig, 0);	/* signal we're ready */
}

void
controlwire(Control *cfrom, char *name, Channel *chan)
{
	CWire *cw;

	cw = ctlmalloc(sizeof(CWire));
	cw->chan = chan;
	cw->sig = chancreate(sizeof(int), 0);
	cw->name = ctlstrdup(name);
	send(cfrom->wire, cw);
	recvp(cw->sig);	/* signalling 'from' is now rewired */
	chanfree(cw->sig);
	free(cw->name);
	free(cw);
}

static void
mousethread(void *v)
{
	Mouse mouse;
	Control *f;
	int prevbut;
	Controlset *cs;
	Alt alts[3];
	char buf[64];

	cs = v;
	snprint(buf, sizeof buf, "mousethread0x%p", cs);
	threadsetname(buf);

	alts[0].c = cs->mousec;
	alts[0].v = &mouse;
	alts[0].op = CHANRCV;
	alts[1].c = cs->mouseexitc;
	alts[1].v = nil;
	alts[1].op = CHANRCV;
	alts[2].op = CHANEND;

	cs->focus = nil;
	for(prevbut=0; ; prevbut=mouse.buttons){
		switch(alt(alts)){
		case 0:
			/* is this a focus change? */
			if(prevbut)	/* don't change focus if button was down */
				goto Send;
			if(cs->focus!=nil && ptinrect(mouse.xy, cs->focus->rect))
				goto Send;
			if(cs->clicktotype == 0)
				goto Change;
			/* click to type: only change if button is down */
			if(mouse.buttons == 0)
				goto Send;
		Change:
			/* change of focus */
			if(cs->focus != nil)
				printctl(cs->focus->ctl, "focus 0");
			cs->focus = nil;
			for(f=cs->actives; f!=nil; f=f->nextactive)
				if(ptinrect(mouse.xy, f->rect)){
					cs->focus = f;
					printctl(f->ctl, "focus 1");
					send(f->mouse, &mouse);
					break;
				}
		Send:
			if(cs->focus)
				send(cs->focus->mouse, &mouse);
			break;

		case 1:
			return;
		}
	}
}

static void
resizethread(void *v)
{
	Controlset *cs;
	char buf[64];
	Alt alts[3];

	cs = v;
	snprint(buf, sizeof buf, "resizethread0x%p", cs);
	threadsetname(buf);

	alts[0].c = cs->resizec;
	alts[0].v = nil;
	alts[0].op = CHANRCV;
	alts[1].c = cs->resizeexitc;
	alts[1].v = nil;
	alts[1].op = CHANRCV;
	alts[2].op = CHANEND;

	for(;;){
		switch(alt(alts)){
		case 0:
			resizecontrolset(cs);
			break;
		case 1:
			return;
		}
	}
}

static void
keyboardthread(void *v)
{
	Controlset *cs;
	Rune buf[2][20], *rp;
	int i, n;
	char tmp[64];
	Alt alts[3];

	cs = v;
	snprint(tmp, sizeof tmp, "keyboardthread0x%p", cs);
	threadsetname(tmp);

	alts[0].c = cs->kbdc;
	alts[0].v = &rp;
	alts[0].op = CHANRCV;
	alts[1].c = cs->kbdexitc;
	alts[1].v = nil;
	alts[1].op = CHANRCV;
	alts[2].op = CHANEND;


	n = 0;
	for(;;){
		/* toggle so we can receive in one buffer while client processes the other */
		alts[0].v = buf[n];
		rp = buf[n];
		n = 1-n;
		switch(alt(alts)){
		case 0:
			if(ctldeletequits && rp[0]=='\177')
				ctlerror("delete");
			for(i=1; i<nelem(buf[0])-1; i++)
				if(nbrecv(cs->kbdc, rp+i) <= 0)
					break;
			rp[i] = L'\0';
			if(cs->focus)
				sendp(cs->focus->kbd, rp);
			break;
		case 1:
			return;
		}
	}
}

void
activate(Control *a)
{
	Control *c;

	for(c=a->controlset->actives; c; c=c->nextactive)
		if(c == a)
			ctlerror("%q already active\n", a->name);
	a->nextactive = a->controlset->actives;
	a->controlset->actives = a;
}

void
deactivate(Control *a)
{
	Control *c, *prev;

	prev = nil;
	for(c=a->controlset->actives; c; c=c->nextactive){
		if(c == a){
			if(a->controlset->focus == a)
				a->controlset->focus = nil;
			if(prev != nil)
				prev->nextactive = a->nextactive;
			else
				a->controlset->actives = a->nextactive;
			return;
		}
		prev = c;
	}
	ctlerror("%q not active\n", a->name);
}

static struct
{
	char	*name;
	ulong	color;
}coltab[] = {
	"red",			DRed,
	"green",			DGreen,
	"blue",			DBlue,
	"cyan",			DCyan,
	"magenta",		DMagenta,
	"yellow",			DYellow,
	"paleyellow",		DPaleyellow,
	"darkyellow",		DDarkyellow,
	"darkgreen",		DDarkgreen,
	"palegreen",		DPalegreen,
	"medgreen",		DMedgreen,
	"darkblue",		DDarkblue,
	"palebluegreen",	DPalebluegreen,
	"paleblue",		DPaleblue,
	"bluegreen",		DBluegreen,
	"greygreen",		DGreygreen,
	"palegreygreen",	DPalegreygreen,
	"yellowgreen",		DYellowgreen,
	"medblue",		DMedblue,
	"greyblue",		DGreyblue,
	"palegreyblue",		DPalegreyblue,
	"purpleblue",		DPurpleblue,
	nil,	0
};

void
initcontrols(void)
{
	int i;
	Image *im;

	quotefmtinstall();
	namectlimage(display->opaque, "opaque");
	namectlimage(display->transparent, "transparent");
	namectlimage(display->white, "white");
	namectlimage(display->black, "black");
	for(i=0; coltab[i].name!=nil; i++){
		im = allocimage(display, Rect(0,0,1,1), RGB24, 1, coltab[i].color);
		namectlimage(im, coltab[i].name);
	}
	namectlfont(font, "font");
	_ctlsnarffd = open("/dev/snarf", OREAD);
}

Controlset*
newcontrolset(Image *im, Channel *kbdc, Channel *mousec, Channel *resizec)
{
	Controlset *cs;

	if(im == nil)
		im = screen;
	if((mousec==nil && resizec!=nil) || (mousec!=nil && resizec==nil))
		ctlerror("must specify either or both of mouse and resize channels");

	cs = ctlmalloc(sizeof(Controlset));
	cs->screen = im;

	if(kbdc == nil){
		cs->keyboardctl = initkeyboard(nil);
		if(cs->keyboardctl == nil)
			ctlerror("can't initialize keyboard: %r");
		kbdc = cs->keyboardctl->c;
	}
	cs ->kbdc = kbdc;

	if(mousec == nil){
		cs->mousectl = initmouse(nil, im);
		if(cs->mousectl == nil)
			ctlerror("can't initialize mouse: %r");
		mousec = cs->mousectl->c;
		resizec = cs->mousectl->resizec;
	}
	cs->mousec = mousec;
	cs->resizec = resizec;
	cs->kbdexitc = chancreate(sizeof(int), 0);
	cs->mouseexitc = chancreate(sizeof(int), 0);
	cs->resizeexitc = chancreate(sizeof(int), 0);

	threadcreate(mousethread, cs, 4096);
	threadcreate(resizethread, cs, 4096);
	threadcreate(keyboardthread, cs, 4096);

	controlset = ctlrealloc(controlset, (ncontrolset+1)*sizeof(Controlset*));
	controlset[ncontrolset++] = cs;
	return cs;
}

void
closecontrolset(Controlset *cs)
{
	int i;

	for(i=0; i<ncontrolset; i++)
		if(cs == controlset[i]){
			memmove(controlset+i, controlset+i+1, (ncontrolset-(i+1))*sizeof(Controlset*));
			ncontrolset--;
			goto Found;
		}

	if(i == ncontrolset)
		ctlerror("closecontrolset: control set not found");

    Found:
	while(cs->controls != nil)
		closecontrol(cs->controls);
	sendul(cs->kbdexitc, 0);
	chanfree(cs->kbdexitc);
	sendul(cs->mouseexitc, 0);
	chanfree(cs->mouseexitc);
	sendul(cs->resizeexitc, 0);
	chanfree(cs->resizeexitc);
}
