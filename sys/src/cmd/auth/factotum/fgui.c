#include "dat.h"
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

int ctldeletequits = 1;

typedef struct RequestType RequestType;
typedef struct Request Request;
typedef struct Memory Memory;

struct RequestType
{
	char		*file;			/* file to read requests from */
	void		(*f)(Request*);		/* request handler */
	void		(*r)(Controlset*);	/* resize handler */
	int		fd;			/* fd = open(file, ORDWR) */
	Channel		*rc;			/* channel requests are multiplexed to */
	Controlset	*cs;
};

struct Request
{
	RequestType	*rt;
	Attr		*a;
	Attr		*tag;
};

struct Memory
{
	Memory	*next;
	Attr	*a;
	Attr	*val;
};
Memory *mem;

static void	readreq(void*);
static void	hide(void);
static void	unhide(void);
static void	openkmr(void);
static void	closekmr(void);
static Memory*	searchmem(Attr*);
static void	addmem(Attr*, Attr*);

static void	confirm(Request*);
static void	resizeconfirm(Controlset*);
static void	needkey(Request*);
static void	resizeneedkey(Controlset*);

Control *b_remember;
Control *b_accept;
Control *b_refuse;

RequestType rt[] = 
{
	{ "/mnt/factotum/confirm",	confirm,	resizeconfirm, },
	{ "/mnt/factotum/needkey",	needkey,	resizeneedkey, },
	{ 0 },
};

enum
{
	ButtonDim=	15,
};

void
threadmain(int argc, char *argv[])
{
	Request r;
	Channel *rc;
	RequestType *p;
	Font *invis;

	ARGBEGIN{
	}ARGEND;

	if(newwindow("-hide") < 0)
		sysfatal("newwindow: %r");

	fmtinstall('A', _attrfmt);

	/* create the proc's that read */
	rc = chancreate(sizeof(Request), 0);
	for(p = rt;  p->file != 0; p++){
		p->fd = -1;
		p->rc = rc;
		proccreate(readreq, p, 16*1024);
	}

	/* gui initialization */
	if(initdraw(0, 0, "auth/fgui") < 0)
		sysfatal("initdraw failed: %r");
	initcontrols();
	hide();

	/* get an invisible font for passwords */
	invis = openfont(display, "/lib/font/bit/lucm/passwd.9.font");
	if (invis == nil)
		sysfatal("fgui: %s: %r", "/lib/font/bit/lucm/passwd.9.font");
	namectlfont(invis, "invisible");

	/* serialize all requests */
	for(;;){
		if(recv(rc, &r) < 0)
			break;
		(*r.rt->f)(&r);
		_freeattr(r.a);
		_freeattr(r.tag);
	}

	threadexitsall(nil);
}

/*
 *  read requests and pass them to the main loop
 */
enum
{
	Requestlen=4096,
};
static void
readreq(void *a)
{
	RequestType *rt = a;
	char *buf, *p;
	int n;
	Request r;
	Attr **l;

	rt->fd = open(rt->file, ORDWR);
	if(rt->fd < 0)
		sysfatal("opening %s: %r", rt->file);
	rt->cs = nil;

	buf = malloc(Requestlen);
	if(buf == nil)
		sysfatal("allocating read buffer: %r");
	r.rt = rt;

	for(;;){
		n = read(rt->fd, buf, Requestlen-1);
		if(n < 0)
			break;
		buf[n] = 0;

		/* skip verb, parse attributes, and remove tag */
		p = strchr(buf, ' ');
		if(p != nil)
			p++;
		else
			p = buf;
		r.a = _parseattr(p);

		/* separate out the tag */
		r.tag = nil;
		for(l = &r.a; *l != nil; l = &(*l)->next)
			if(strcmp((*l)->name, "tag") == 0){
				r.tag = *l;
				*l = r.tag->next;
				r.tag->next = nil;
				break;
			}

		/* if no tag, forget it */
		if(r.tag == nil){
			_freeattr(r.a);
			continue;
		}

		send(rt->rc, &r);
	}
}
#ifdef asdf
static void
readreq(void *a)
{
	RequestType *rt = a;
	char *buf, *p;
	int n;
	Request r;

	rt->fd = -1;
	rt->cs = nil;

	buf = malloc(Requestlen);
	if(buf == nil)
		sysfatal("allocating read buffer: %r");
	r.rt = rt;

	for(;;){
		strcpy(buf, "adfasdf=afdasdf asdfasdf=asdfasdf");
		r.a = _parseattr(buf);
		send(rt->rc, &r);
		sleep(5000);
	}
}
#endif asdf

/*
 *  open/close the keyboard, mouse and resize channels
 */
static Channel *kbdc;
static Channel *mousec;
static Channel *resizec;
static Keyboardctl *kctl;
static Mousectl *mctl;

static void
openkmr(void)
{
	/* get channels for subsequent newcontrolset calls  */
	kctl = initkeyboard(nil);
	if(kctl == nil)
		sysfatal("can't initialize keyboard: %r");
	kbdc = kctl->c;
	mctl = initmouse(nil, screen);
	if(mctl == nil)
		sysfatal("can't initialize mouse: %r");
	mousec = mctl->c;
	resizec = mctl->resizec;
}
static void
closekmr(void)
{
	Mouse m;

	while(nbrecv(kbdc, &m) > 0)
		;
	closekeyboard(kctl);
	while(nbrecv(mousec, &m) > 0)
		;
	closemouse(mctl);
}


/*
 *  called when the window is resized
 */
void
resizecontrolset(Controlset *cs)
{
	RequestType *p;

	for(p = rt;  p->file != 0; p++){
		if(p->cs == cs){
			(*p->r)(cs);
			break;
		}
	}
}

/*
 *  hide window when not in use
 */
static void
unhide(void)
{
	int wctl;

	wctl = open("/dev/wctl", OWRITE);
	if(wctl < 0)
		return;
	fprint(wctl, "unhide");
	close(wctl);
}
static void
hide(void)
{
	int wctl;
	int tries;

	wctl = open("/dev/wctl", OWRITE);
	if(wctl < 0)
		return;
	for(tries = 0; tries < 10; tries++){
		if(fprint(wctl, "hide") >= 0)
			break;
		sleep(100);
	}
	close(wctl);
}

/*
 *  set up the controls for the confirmation window
 */
static Channel*
setupconfirm(Request *r)
{
	Controlset *cs;
	Channel *c;
	Attr *a;

	/* create a new control set for the confirmation */
	openkmr();
	cs = newcontrolset(screen, kbdc, mousec, resizec);

	createtext(cs, "msg");
	chanprint(cs->ctl, "msg image paleyellow");
	chanprint(cs->ctl, "msg border 1");
	chanprint(cs->ctl, "msg add 'The following key is being used:'");
	for(a = r->a; a != nil; a = a->next)
		chanprint(cs->ctl, "msg add '    %s = %s'", a->name,
				a->val);

	namectlimage(display->white, "i_white");
	namectlimage(display->black, "i_black");

	b_remember = createbutton(cs, "b_remember");
	chanprint(cs->ctl, "b_remember border 1");
	chanprint(cs->ctl, "b_remember mask i_white");
	chanprint(cs->ctl, "b_remember image i_white");
	chanprint(cs->ctl, "b_remember light i_black");

	createtext(cs, "t_remember");
	chanprint(cs->ctl, "t_remember image white");
	chanprint(cs->ctl, "t_remember bordercolor white");
	chanprint(cs->ctl, "t_remember add 'Remember this answer for future confirmations'");

	b_accept = createtextbutton(cs, "b_accept");
	chanprint(cs->ctl, "b_accept border 1");
	chanprint(cs->ctl, "b_accept align center");
	chanprint(cs->ctl, "b_accept text Accept");
	chanprint(cs->ctl, "b_accept image i_white");
	chanprint(cs->ctl, "b_accept light i_black");

	b_refuse = createtextbutton(cs, "b_refuse");
	chanprint(cs->ctl, "b_refuse border 1");
	chanprint(cs->ctl, "b_refuse align center");
	chanprint(cs->ctl, "b_refuse text Refuse");
	chanprint(cs->ctl, "b_refuse image i_white");
	chanprint(cs->ctl, "b_refuse light i_black");

	c = chancreate(sizeof(char*), 0);
	controlwire(b_remember, "event", c);
	controlwire(b_accept, "event", c);
	controlwire(b_refuse, "event", c);

	/* make the controls interactive */
	activate(b_remember);
	activate(b_accept);
	activate(b_refuse);
	r->rt->cs = cs;
	unhide();
	resizecontrolset(cs);

	return c;
}

/*
 *  resize the controls for the confirmation window
 */
static void
resizeconfirm(Controlset *cs)
{
	Rectangle r, mr, nr, ntr, ar, rr;
	int fontwidth;

	fontwidth = font->height;

	/* get usable window rectangle */
	if(getwindow(display, Refnone) < 0)
		ctlerror("resize failed: %r");
	r = insetrect(screen->r, 10);

	/* message box fills everything not needed for buttons */
	mr = r;
	mr.max.y = mr.min.y + font->height*((Dy(mr)-3*ButtonDim-font->height-4)/font->height);

	/* remember button */
	nr.min = Pt(mr.min.x, mr.max.y+ButtonDim);
	nr.max = Pt(mr.max.x, r.max.y);
	if(Dx(nr) > ButtonDim)
		nr.max.x = nr.min.x+ButtonDim;
	if(Dy(nr) > ButtonDim)
		nr.max.y = nr.min.y+ButtonDim;
	ntr.min = Pt(nr.max.x+ButtonDim, nr.min.y);
	ntr.max = Pt(r.max.x, nr.min.y+font->height);

	/* accept/refuse buttons */
	ar.min = Pt(r.min.x+Dx(r)/2-ButtonDim-6*fontwidth, nr.max.y+ButtonDim);
	ar.max = Pt(ar.min.x+6*fontwidth, ar.min.y+font->height+4);
	rr.min = Pt(r.min.x+Dx(r)/2+ButtonDim, nr.max.y+ButtonDim);
	rr.max = Pt(rr.min.x+6*fontwidth, rr.min.y+font->height+4);

	/* make the controls visible */
	chanprint(cs->ctl, "msg rect %R\nmsg show", mr);
	chanprint(cs->ctl, "b_remember rect %R\nb_remember show", nr);
	chanprint(cs->ctl, "t_remember rect %R\nt_remember show", ntr);
	chanprint(cs->ctl, "b_accept rect %R\nb_accept show", ar);
	chanprint(cs->ctl, "b_refuse rect %R\nb_refuse show", rr);
}

/*
 *  free the controls for the confirmation window
 */
static void
teardownconfirm(Request *r)
{
	Controlset *cs;

	cs = r->rt->cs;
	r->rt->cs = nil;
	hide();
	closecontrolset(cs);
	closekmr();
}

/*
 *  get user confirmation of a key
 */
static void
confirm(Request *r)
{
	Channel *c;
	char *s;
	int n;
	char *args[3];
	int remember;
	Attr *val;
	Memory *m;

	/* if it's something that the user wanted us not to ask again about */
	m = searchmem(r->a);
	if(m != nil){
		fprint(r->rt->fd, "%A %A", r->tag, m->val);
		return;
	}

	/* set up the controls */
	c = setupconfirm(r);

	/* wait for user to reply */
	remember = 0;
	for(;;){
		s = recvp(c);
		n = tokenize(s, args, nelem(args));
		if(n==3 && strcmp(args[1], "value")==0){
			if(strcmp(args[0], "b_remember:") == 0){
				remember = atoi(args[2]);
			}
			if(strcmp(args[0], "b_accept:") == 0){
				val = _mkattr(AttrNameval, "answer", "yes", nil);
				free(s);
				break;
			}
			if(strcmp(args[0], "b_refuse:") == 0){
				val = _mkattr(AttrNameval, "answer", "no", nil);
				free(s);
				break;
			}
		}
		free(s);
	}
	teardownconfirm(r);
	fprint(r->rt->fd, "%A %A", r->tag, val);
	if(remember)
		addmem(_copyattr(r->a), val);
	else
		_freeattr(val);
}

/*
 *  confirmations that are remembered
 */
static int
match(Attr *a, Attr *b)
{
	Attr *x;

	for(; a != nil; a = a->next){
		x = _findattr(b, a->name);
		if(x == nil || strcmp(a->val, x->val) != 0)
			return 0;
	}
	return 1;
}
static Memory*
searchmem(Attr *a)
{
	Memory *m;

	for(m = mem; m != nil; m = m->next){
		if(match(a, m->a))
			break;
	}
	return m;
}
static void
addmem(Attr *a, Attr *val)
{
	Memory *m;

	m = malloc(sizeof *m);
	if(m == nil)
		return;
	m->a = a;
	m->val = val;
	m->next = mem;
	mem = m;
}

/* controls for needkey */
Control *msg;
Control *b_done;
enum {
	Pprivate=	1<<0,
	Pneed=		1<<1,
};
typedef struct Entry Entry;
struct Entry {
	Control *name;
	Control *val;
	Control *eq;
	Attr *a;
};
static Entry *entry;
static int entries;

/*
 *  set up the controls for the confirmation window
 */
static Channel*
setupneedkey(Request *r)
{
	Controlset *cs;
	Channel *c;
	Attr *a;
	Attr **l;
	char cn[10];
	int i;

	/* create a new control set for the confirmation */
	openkmr();
	cs = newcontrolset(screen, kbdc, mousec, resizec);

	/* count attributes and allocate entry controls */
	entries = 0;
	for(l = &r->a; *l; l = &(*l)->next)
		entries++;
	if(entries == 0){
		closecontrolset(cs);
		closekmr();
		return nil;
	}
	*l = a = mallocz(sizeof *a, 1);
	a->type = AttrQuery;
	entries++;
	l = &(*l)->next;
	*l = a = mallocz(sizeof *a, 1);
	a->type = AttrQuery;
	entries++;
	entry = malloc(entries*sizeof(Entry));
	if(entry == nil){
		closecontrolset(cs);
		closekmr();
		return nil;
	}

	namectlimage(display->white, "i_white");
	namectlimage(display->black, "i_black");

	/* create controls */
	msg = createtext(cs, "msg");
	chanprint(cs->ctl, "msg image white");
	chanprint(cs->ctl, "msg bordercolor white");
	chanprint(cs->ctl, "msg add 'You need the following key.  Fill in the blanks'");
	chanprint(cs->ctl, "msg add 'and click on the DONE button.'");

	for(i = 0, a = r->a; a != nil; i++, a = a->next){
		entry[i].a = a;
		snprint(cn, sizeof cn, "name_%d", i);
		if(entry[i].a->name == nil){
			entry[i].name = createentry(cs, cn);
			chanprint(cs->ctl, "%s image yellow", cn);
			chanprint(cs->ctl, "%s border 1", cn);
		} else {
			entry[i].name = createtext(cs, cn);
			chanprint(cs->ctl, "%s image white", cn);
			chanprint(cs->ctl, "%s bordercolor white", cn);
			chanprint(cs->ctl, "%s add '%s'", cn, a->name);
		}

		snprint(cn, sizeof cn, "val_%d", i);
		if(a->type == AttrQuery){
			entry[i].val = createentry(cs, cn);
			chanprint(cs->ctl, "%s image yellow", cn);
			chanprint(cs->ctl, "%s border 1", cn);
			if(a->name != nil){
				if(strcmp(a->name, "user") == 0)
					chanprint(cs->ctl, "%s value %q", cn, getuser());
				if(*a->name == '!')
					chanprint(cs->ctl, "%s font invisible", cn);
			}
		} else {
			entry[i].val = createtext(cs, cn);
			chanprint(cs->ctl, "%s image white", cn);
			chanprint(cs->ctl, "%s add %q", cn, a->val);
		}

		snprint(cn, sizeof cn, "eq_%d", i);
		entry[i].eq = createtext(cs, cn);
		chanprint(cs->ctl, "%s image white", cn);
		chanprint(cs->ctl, "%s add ' = '", cn);
	}

	b_done = createtextbutton(cs, "b_done");
	chanprint(cs->ctl, "b_done border 1");
	chanprint(cs->ctl, "b_done align center");
	chanprint(cs->ctl, "b_done text DONE");
	chanprint(cs->ctl, "b_done image green");
	chanprint(cs->ctl, "b_done light green");

	/* wire controls for input */
	c = chancreate(sizeof(char*), 0);
	controlwire(b_done, "event", c);
	for(i = 0; i < entries; i++)
		if(entry[i].a->type == AttrQuery)
			controlwire(entry[i].val, "event", c);

	/* make the controls interactive */
	activate(msg);
	activate(b_done);
	for(i = 0; i < entries; i++){
		if(entry[i].a->type != AttrQuery)
			continue;
		if(entry[i].a->name == nil)
			activate(entry[i].name);
		activate(entry[i].val);
	}

	/* change the display */
	r->rt->cs = cs;
	unhide();
	resizecontrolset(cs);

	return c;
}

/*
 *  resize the controls for the confirmation window
 */
static void
resizeneedkey(Controlset *cs)
{
	Rectangle r, mr;
	int mid, i, n, lasty;

	/* get usable window rectangle */
	if(getwindow(display, Refnone) < 0)
		ctlerror("resize failed: %r");
	r = insetrect(screen->r, 10);

	/* find largest name */
	mid = 0;
	for(i = 0; i < entries; i++){
		if(entry[i].a->name == nil)
			continue;
		n = strlen(entry[i].a->name);
		if(n > mid)
			mid = n;
	}
	mid = (mid+2) * font->height;

	/* top line is the message */
	mr = r;
	mr.max.y = mr.min.y + 2*font->height + 2;
	chanprint(cs->ctl, "msg rect %R\nmsg show", mr);

	/* one line per attribute */
	mr.min.x += 2*font->height;
	lasty = mr.max.y;
	for(i = 0; i < entries; i++){
		r.min.x = mr.min.x;
		r.min.y = lasty+2;
		r.max.x = r.min.x + mid - 3*stringwidth(font, "=");
		r.max.y = r.min.y + font->height;
		chanprint(cs->ctl, "name_%d rect %R\nname_%d show", i, r, i);

		r.min.x = r.max.x;
		r.max.x = r.min.x + 3*stringwidth(font, "=");
		chanprint(cs->ctl, "eq_%d rect %R\neq_%d show", i, r, i);

		r.min.x = r.max.x;
		r.max.x = mr.max.x;
		if(Dx(r) > 32*font->height)
			r.max.x = r.min.x + 32*font->height;
		chanprint(cs->ctl, "val_%d rect %R\nval_%d show", i, r, i);
		lasty = r.max.y;
	}

	/* done button */
	mr.min.x -= 2*font->height;
	r.min.x = mr.min.x + mid - 3*font->height;
	r.min.y = lasty+10;
	r.max.x = r.min.x + 6*font->height;
	r.max.y = r.min.y + font->height + 2;
	chanprint(cs->ctl, "b_done rect %R\nb_done show", r);
}

/*
 *  free the controls for the confirmation window
 */
static void
teardownneedkey(Request *r)
{
	Controlset *cs;

	cs = r->rt->cs;
	r->rt->cs = nil;
	hide();
	closecontrolset(cs);
	closekmr();

	if(entry != nil)
		free(entry);
	entry = nil;
}

static void
needkey(Request *r)
{
	Channel *c;
	char *nam, *val;
	int i, n;
	int fd;
	char *args[3];

	/* set up the controls */
	c = setupneedkey(r);
	if(c == nil)
		goto out;

	/* wait for user to reply */
	for(;;){
		val = recvp(c);
		n = tokenize(val, args, nelem(args));
		if(n==3 && strcmp(args[1], "value")==0){	/* user hit 'enter' */
			free(val);
			break;
		}
		free(val);
	}

	/* get entry values */
	for(i = 0; i < entries; i++){
		if(entry[i].a->type != AttrQuery)
			continue;

		chanprint(r->rt->cs->ctl, "val_%d data", i);
		val = recvp(entry[i].val->data);
		if(entry[i].a->name == nil){
			chanprint(r->rt->cs->ctl, "name_%d data", i);
			nam = recvp(entry[i].name->data);
			if(nam == nil || *nam == 0){
				free(val);
				continue;
			}
			entry[i].a->val = estrdup(val);
			free(val);
			entry[i].a->name = estrdup(nam);
			free(nam);
		} else {
			if(val != nil){
				entry[i].a->val = estrdup(val);
				free(val);
			}
		}
		entry[i].a->type = AttrNameval;
	}

	/* enter the new key !!!!need to do something in case of error!!!! */
	fd = open("/mnt/factotum/ctl", OWRITE);
	fprint(fd, "key %A", r->a);
	close(fd);

	teardownneedkey(r);
out:
	fprint(r->rt->fd, "%A", r->tag);
}
