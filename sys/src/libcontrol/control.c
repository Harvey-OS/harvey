#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

static int debug = 0;

enum	/* alts */
{
	AKey,
	AMouse,
	ACtl,
	AExit,
	NALT
};

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

char *ctltypenames[Ntypes] = {
	[Ctlunknown] =		"unknown",
	[Ctlbox] =			"box",
	[Ctlbutton] =		"button",
	[Ctlentry] =		"entry",
	[Ctlkeyboard] =		"keyboard",
	[Ctllabel] =		"label",
	[Ctlmenu] =		"menu",
	[Ctlradio] =		"radio",
	[Ctlscribble] =		"scribble",
	[Ctlslider] =		"slider",
	[Ctltabs] =			"tabs",
	[Ctltext] =			"text",
	[Ctltextbutton] =	"textbutton",
	[Ctltextbutton3] =	"textbutton3",
	[Ctlgroup] =		"group",		// divider between controls and metacontrols
	[Ctlboxbox] =		"boxbox",
	[Ctlcolumn] =		"column",
	[Ctlrow] =			"row",
	[Ctlstack] =		"stack",
	[Ctltab] =			"tab",
};

static void	_ctlcmd(Controlset*, char*);
static void	_ctlcontrol(Controlset*, char*);

static char*
_mkctlcmd(Control *c, char *fmt, va_list arg)
{
	char *name, *p, *both;

	name = quotestrdup(c->name);
	if(name == nil)
		ctlerror("quotestrdup in ctlprint failed");
	p = vsmprint(fmt, arg);
	if(p == nil){
		free(name);
		ctlerror("vsmprint1 in ctlprint failed");
	}
	both = ctlmalloc(strlen(name)+strlen(p)+2);
	strcpy(both, name);
	strcat(both, " ");
	strcat(both, p);
	free(name);
	free(p);
	return both;
}

int
ctlprint(Control *c, char *fmt, ...)
{
	int n;
	char *p;
	va_list arg;

	va_start(arg, fmt);
	p = _mkctlcmd(c, fmt, arg);
	va_end(arg);
	n = sendp(c->controlset->ctl, p);
	yield();
	return n;
}

void
_ctlprint(Control *c, char *fmt, ...)
{
	char *p;
	va_list arg;

	va_start(arg, fmt);
	p = _mkctlcmd(c, fmt, arg);
	va_end(arg);
	_ctlcmd(c->controlset, p);
	free(p);
}

int
_ctllookup(char *s, char *tab[], int ntab)
{
	int i;

	for(i=0; i<ntab; i++)
		if(tab[i] != nil && strcmp(s, tab[i]) == 0)
			return i;
	return -1;
}

static Control*
_newcontrol(Controlset *cs, uint n, char *name, char *type)
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
	c->type = _ctllookup(type, ctltypenames, Ntypes);
	if (c->type < 0)
		ctlerror("unknown type: %s", type);
	c->event = chancreate(sizeof(char*), 64);
	c->data = chancreate(sizeof(char*), 0);
	c->size = Rect(1, 1, _Ctlmaxsize, _Ctlmaxsize);
	c->hidden = 0;
	c->ctl = nil;
	c->mouse = nil;
	c->key = nil;
	c->exit = nil;
	c->setsize = nil;

	c->controlset = cs;
	c->next = cs->controls;
	cs->controls = c;
	return c;
}

static void
controlsetthread(void *v)
{
	Controlset *cs;
	Mouse mouse;
	Control *f;
	int prevbut, n, i;
	Alt alts[NALT+1];
	char tmp[64], *str;
	Rune buf[2][20], *rp;

	cs = v;
	snprint(tmp, sizeof tmp, "controlsetthread 0x%p", cs);
	threadsetname(tmp);

	alts[AKey].c = cs->kbdc;
	alts[AKey].v = &rp;
	alts[AKey].op = CHANRCV;
	alts[AMouse].c = cs->mousec;
	alts[AMouse].v = &mouse;
	alts[AMouse].op = CHANRCV;
	alts[ACtl].c = cs->ctl;
	alts[ACtl].v = &str;
	alts[ACtl].op = CHANRCV;
	alts[AExit].c = cs->csexitc;
	alts[AExit].v = nil;
	alts[AExit].op = CHANRCV;
	alts[NALT].op = CHANEND;

	cs->focus = nil;
	prevbut=0;
	n = 0;
	for(;;){
		/* toggle so we can receive in one buffer while client processes the other */
		alts[AKey].v = buf[n];
		rp = buf[n];
		n = 1-n;
		switch(alt(alts)){
		case AKey:
			if(ctldeletequits && rp[0]=='\177')
				ctlerror("delete");
			for(i=1; i<nelem(buf[0])-1; i++)
				if(nbrecv(cs->kbdc, rp+i) <= 0)
					break;
			rp[i] = L'\0';
			if(cs->focus && cs->focus->key)
				cs->focus->key(cs->focus, rp);
			break;
		case AMouse:
			/* is this a focus change? */
			if(prevbut)	/* don't change focus if button was down */
				goto Send;
			if(cs->focus!=nil && cs->focus->hidden == 0 && ptinrect(mouse.xy, cs->focus->rect))
				goto Send;
			if(cs->clicktotype == 0)
				goto Change;
			/* click to type: only change if button is down */
			if(mouse.buttons == 0)
				goto Send;
		Change:
			/* change of focus */
			if(cs->focus != nil)
				_ctlprint(cs->focus, "focus 0");
			cs->focus = nil;
			for(f=cs->actives; f!=nil; f=f->nextactive)
				if(f->hidden == 0 && f->mouse && ptinrect(mouse.xy, f->rect)){
					cs->focus = f;
					_ctlprint(f, "focus 1");
					if (f->mouse) {
						if (debug) fprint(2, "f->mouse %s\n", f->name);
						f->mouse(f, &mouse);
					}
					break;
				}
		Send:
			if(cs->focus && cs->focus->mouse) {
				if (debug) fprint(2, "cs->focus->mouse %s\n", cs->focus->name);
				cs->focus->mouse(cs->focus, &mouse);
			}
			prevbut=mouse.buttons;
			break;
		case ACtl:
			_ctlcontrol(cs, str);
			free(str);
			break;
		case AExit:
			threadexits(nil);
		}
	}
}

Control*
_createctl(Controlset *cs, char *type, uint size, char *name)
{
	Control *c;

	c = _newcontrol(cs, size, name, type);
	if(c == nil)
		ctlerror("can't create %s control %q: %r", type, name);
	return c;
}

void
closecontrol(Control *c)
{
	Control *prev, *p;

	if(c == nil)
		return;
	if (c == c->controlset->focus)
		c->controlset->focus = nil;
	if(c->exit)
		c->exit(c);

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
	if(!c->wdata)
		chanfree(c->data);
	free(c->name);
	free(c->format);
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
	vfprint(2, fmt, arg);
	va_end(arg);
	write(2, "\n", 1);
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

static void
ctokenize(char *s, CParse *cp)
{
	snprint(cp->str, sizeof cp->str, "%s", s);
	cp->args = cp->pargs;
	cp->nargs = tokenize(s, cp->args, nelem(cp->pargs));
}

static int
ctlparse(CParse *cp, char *s, int hasreceiver)
{
	int i;
	char *t;

	/* keep original string for good error messages */
	strncpy(cp->str, s, sizeof cp->str);
	cp->str[sizeof cp->str - 1] = '\0';
	ctokenize(s, cp);
	if(cp->nargs == 0)
		return -1;
	/* strip leading sender name if present */
	cp->sender = nil;
	i = strlen(cp->args[0])-1;
	if(cp->args[0][i] == ':'){
		cp->sender = cp->args[0];
		cp->sender[i] = '\0';
		cp->args++;
		cp->nargs--;
	}
	if(hasreceiver){
		if(cp->nargs-- == 0)
			return -1;
		cp->receiver = *cp->args++;
	}else
		cp->receiver = nil;
	for(i=0; i<cp->nargs; i++){
		t = cp->args[i];
		while(*t == '[')	/* %R gives [0 0] [1 1]; atoi will stop at closing ] */
			t++;
		cp->iargs[i] = atoi(t);
	}
	return cp->nargs;
}

void
_ctlargcount(Control *c, CParse *cp, int n)
{
	if(cp->nargs != n)
		ctlerror("%q: wrong argument count in '%s'", c->name, cp->str);
}

static void
_ctlcmd(Controlset *cs, char*s)
{
	CParse cp;
	char	*rcvrs[32];
	int	ircvrs[32], n, i, hit;
	Control *c;

//	fprint(2, "_ctlcmd: %s\n", s);
	cp.args = cp.pargs;
	if (ctlparse(&cp, s, 1) < 0)
		ctlerror("bad command string: %q", cp.str);
	if (cp.nargs == 0 && strcmp(cp.receiver, "sync") == 0){
		chanprint(cs->data, "sync");
		return;
	}
	if (cp.nargs == 0)
		ctlerror("no command in command string: %q", cp.str);

	n = tokenize(cp.receiver, rcvrs, nelem(rcvrs));

	// lookup type names: a receiver can be a named type or a named control
	for (i = 0; i < n; i++)
		ircvrs[i] = _ctllookup(rcvrs[i], ctltypenames, Ntypes);

	for(c = cs->controls; c != nil; c = c->next){
		/* if a control matches on more than one receiver element,
		 * make sure it gets processed once; hence loop through controls
		 * in the outer loop
		 */
		hit = 0;
		for (i = 0; i < n; i++)
			if(strcmp(c->name, rcvrs[i]) == 0 || c->type == ircvrs[i])
				hit++;
		if (hit && c->ctl)
			c->ctl(c, &cp);
	}
}

static void
_ctlcontrol(Controlset *cs, char *s)
{
	char *lines[16];
	int i, n;
	char *l;

//	fprint(2, "_ctlcontrol: %s\n", s);
	n = gettokens(s, lines, nelem(lines), "\n");
	for(i=0; i<n; i++){
		l = lines[i];
		while(*l==' ' || *l=='\t')
			l++;
		if(*l != '\0')
			_ctlcmd(cs, l);
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
		if(fprint(fd, "%.*S", n, snarf+i) < 0)
			break;
	}
	close(fd);
}

int
_ctlalignment(char *s)
{
	int i;

	i = _ctllookup(s, alignnames, Nalignments);
	if (i < 0)
		ctlerror("unknown alignment: %s", s);
	return i;
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
controlwire(Control *cfrom, char *name, Channel *chan)
{
	Channel **p;

	p = nil;
	if(strcmp(name, "event") == 0){
		p = &cfrom->event;
		cfrom->wevent = 1;
	}else if(strcmp(name, "data") == 0){
		p = &cfrom->data;
		cfrom->wdata = 1;
	}else
		ctlerror("%q: unknown controlwire channel %s", cfrom->name, name);
	chanfree(*p);
	*p = chan;
}

void
_ctlfocus(Control *me, int set)
{
	Controlset *cs;

	cs = me->controlset;
	if(set){
		if(cs->focus == me)
			return;
		if(cs->focus != nil)
			_ctlprint(cs->focus, "focus 0");
		cs->focus = me;
	}else{
		if(cs->focus != me)
			return;
		cs->focus = nil;
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

void
activate(Control *a)
{
	Control *c;

	for(c=a->controlset->actives; c; c=c->nextactive)
		if(c == a)
			ctlerror("%q already active\n", a->name);
	
	if (a->activate){
		a->activate(a, 1);
		return;
	}
	/* prepend */
	a->nextactive = a->controlset->actives;
	a->controlset->actives = a;
}

void
deactivate(Control *a)
{
	Control *c, *prev;

	/* if group, first deactivate kids, then self */
	if (a->activate){
		a->activate(a, 0);
		return;
	}
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
	cs->ctl = chancreate(sizeof(char*), 64);	/* buffer to prevent deadlock */
	cs->data = chancreate(sizeof(char*), 0);
	cs->resizeexitc = chancreate(sizeof(int), 0);
	cs->csexitc = chancreate(sizeof(int), 0);

	threadcreate(resizethread, cs, 32*1024);
	threadcreate(controlsetthread, cs, 32*1024);

	controlset = ctlrealloc(controlset, (ncontrolset+1)*sizeof(Controlset*));
	controlset[ncontrolset++] = cs;
	return cs;
}

void
closecontrolset(Controlset *cs)
{
	int i;

	sendul(cs->resizeexitc, 0);
	chanfree(cs->resizeexitc);
	sendul(cs->csexitc, 0);
	chanfree(cs->csexitc);
	chanfree(cs->ctl);
	chanfree(cs->data);

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
}
