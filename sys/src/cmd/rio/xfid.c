#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"

#define	MAXSNARF	100*1024

char Einuse[] =		"file in use";
char Edeleted[] =	"window deleted";
char Ebadreq[] =	"bad graphics request";
char Etooshort[] =	"buffer too small";
char Ebadtile[] =	"unknown tile";
char Eshort[] =		"short i/o request";
char Elong[] = 		"snarf buffer too long";
char Eunkid[] = 	"unknown id in attach";
char Ebadrect[] = 	"bad rectangle in attach";
char Ewindow[] = 	"cannot make window";
char Enowindow[] = 	"window has no image";
char Ebadmouse[] = 	"bad format on /dev/mouse";
char Ebadwrect[] = 	"rectangle outside screen";
char Ebadoffset[] = 	"window read not on scan line boundary";
extern char Eperm[];

static	Xfid	*xfidfree;
static	Xfid	*xfid;
static	Channel	*cxfidalloc;	/* chan(Xfid*) */
static	Channel	*cxfidfree;	/* chan(Xfid*) */

static	char	*tsnarf;
static	int	ntsnarf;

void
xfidallocthread(void*)
{
	Xfid *x;
	enum { Alloc, Free, N };
	static Alt alts[N+1];

	alts[Alloc].c = cxfidalloc;
	alts[Alloc].v = nil;
	alts[Alloc].op = CHANRCV;
	alts[Free].c = cxfidfree;
	alts[Free].v = &x;
	alts[Free].op = CHANRCV;
	alts[N].op = CHANEND;
	for(;;){
		switch(alt(alts)){
		case Alloc:
			x = xfidfree;
			if(x)
				xfidfree = x->free;
			else{
				x = emalloc(sizeof(Xfid));
				x->c = chancreate(sizeof(void(*)(Xfid*)), 0);
				x->flushc = chancreate(sizeof(int), 0);	/* notification only; no data */
				x->flushtag = -1;
				x->next = xfid;
				xfid = x;
				threadcreate(xfidctl, x, 16384);
			}
			if(x->ref != 0){
				fprint(2, "%p incref %ld\n", x, x->ref);
				error("incref");
			}
			if(x->flushtag != -1)
				error("flushtag in allocate");
			incref(x);
			sendp(cxfidalloc, x);
			break;
		case Free:
			if(x->ref != 0){
				fprint(2, "%p decref %ld\n", x, x->ref);
				error("decref");
			}
			if(x->flushtag != -1)
				error("flushtag in free");
			x->free = xfidfree;
			xfidfree = x;
			break;
		}
	}
}

Channel*
xfidinit(void)
{
	cxfidalloc = chancreate(sizeof(Xfid*), 0);
	cxfidfree = chancreate(sizeof(Xfid*), 0);
	threadcreate(xfidallocthread, nil, STACK);
	return cxfidalloc;
}

void
xfidctl(void *arg)
{
	Xfid *x;
	void (*f)(Xfid*);
	char buf[64];

	x = arg;
	snprint(buf, sizeof buf, "xfid.%p", x);
	threadsetname(buf);
	for(;;){
		f = recvp(x->c);
		(*f)(x);
		if(decref(x) == 0)
			sendp(cxfidfree, x);
	}
}

void
xfidflush(Xfid *x)
{
	Fcall t;
	Xfid *xf;

	for(xf=xfid; xf; xf=xf->next)
		if(xf->flushtag == x->oldtag){
			xf->flushtag = -1;
			xf->flushing = TRUE;
			incref(xf);	/* to hold data structures up at tail of synchronization */
			if(xf->ref == 1)
				error("ref 1 in flush");
			if(canqlock(&xf->active)){
				qunlock(&xf->active);
				sendul(xf->flushc, 0);
			}else{
				qlock(&xf->active);	/* wait for him to finish */
				qunlock(&xf->active);
			}
			xf->flushing = FALSE;
			if(decref(xf) == 0)
				sendp(cxfidfree, xf);
			break;
		}
	filsysrespond(x->fs, x, &t, nil);
}

void
xfidattach(Xfid *x)
{
	Fcall t;
	int id, hideit, scrollit;
	Window *w;
	char *err, *n, *dir, errbuf[ERRMAX];
	int pid, newlymade;
	Rectangle r;
	Image *i;

	t.qid = x->f->qid;
	qlock(&all);
	w = nil;
	err = Eunkid;
	newlymade = FALSE;
	hideit = 0;

	if(x->aname[0] == 'N'){	/* N 100,100, 200, 200 - old syntax */
		n = x->aname+1;
		pid = strtoul(n, &n, 0);
		if(*n == ',')
			n++;
		r.min.x = strtoul(n, &n, 0);
		if(*n == ',')
			n++;
		r.min.y = strtoul(n, &n, 0);
		if(*n == ',')
			n++;
		r.max.x = strtoul(n, &n, 0);
		if(*n == ',')
			n++;
		r.max.y = strtoul(n, &n, 0);
  Allocate:
		if(!goodrect(r))
			err = Ebadrect;
		else{
			if(hideit)
				i = allocimage(display, r, screen->chan, 0, DWhite);
			else
				i = allocwindow(wscreen, r, Refbackup, DWhite);
			if(i){
				border(i, r, Selborder, display->black, ZP);
				if(pid == 0)
					pid = -1;	/* make sure we don't pop a shell! - UGH */
				w = new(i, hideit, scrolling, pid, nil, nil, nil);
				flushimage(display, 1);
				newlymade = TRUE;
			}else
				err = Ewindow;
		}
	}else if(strncmp(x->aname, "new", 3) == 0){	/* new -dx -dy - new syntax, as in wctl */
		pid = 0;
		if(parsewctl(nil, ZR, &r, &pid, nil, &hideit, &scrollit, &dir, x->aname, errbuf) < 0)
			err = errbuf;
		else
			goto Allocate;
	}else{
		id = atoi(x->aname);
		w = wlookid(id);
	}
	x->f->w = w;
	if(w == nil){
		qunlock(&all);
		x->f->busy = FALSE;
		filsysrespond(x->fs, x, &t, err);
		return;
	}
	if(!newlymade)	/* counteract dec() in winshell() */
		incref(w);
	qunlock(&all);
	filsysrespond(x->fs, x, &t, nil);
}

void
xfidopen(Xfid *x)
{
	Fcall t;
	Window *w;

	w = x->f->w;
	if(w->deleted){
		filsysrespond(x->fs, x, &t, Edeleted);
		return;
	}
	switch(FILE(x->f->qid)){
	case Qconsctl:
		if(w->ctlopen){
			filsysrespond(x->fs, x, &t, Einuse);
			return;
		}
		w->ctlopen = TRUE;
		break;
	case Qkbdin:
		if(w !=  wkeyboard){
			filsysrespond(x->fs, x, &t, Eperm);
			return;
		}
		break;
	case Qmouse:
		if(w->mouseopen){
			filsysrespond(x->fs, x, &t, Einuse);
			return;
		}
		/*
		 * Reshaped: there's a race if the appl. opens the
		 * window, is resized, and then opens the mouse,
		 * but that's rare.  The alternative is to generate
		 * a resized event every time a new program starts
		 * up in a window that has been resized since the
		 * dawn of time.  We choose the lesser evil.
		 */
		w->resized = FALSE;
		w->mouseopen = TRUE;
		break;
	case Qsnarf:
		if(x->mode==ORDWR || x->mode==OWRITE){
			if(tsnarf)
				free(tsnarf);	/* collision, but OK */
			ntsnarf = 0;
			tsnarf = malloc(1);
		}
		break;
	case Qwctl:
		if(x->mode==OREAD || x->mode==ORDWR){
			/*
			 * It would be much nicer to implement fan-out for wctl reads,
			 * so multiple people can see the resizings, but rio just isn't
			 * structured for that.  It's structured for /dev/cons, which gives
			 * alternate data to alternate readers.  So to keep things sane for
			 * wctl, we compromise and give an error if two people try to
			 * open it.  Apologies.
			 */
			if(w->wctlopen){
				filsysrespond(x->fs, x, &t, Einuse);
				return;
			}
			w->wctlopen = TRUE;
			w->wctlready = 1;
			wsendctlmesg(w, Wakeup, ZR, nil);
		}
		break;
	}
	t.qid = x->f->qid;
	t.iounit = messagesize-IOHDRSZ;
	x->f->open = TRUE;
	x->f->mode = x->mode;
	filsysrespond(x->fs, x, &t, nil);
}

void
xfidclose(Xfid *x)
{
	Fcall t;
	Window *w;
	int nb, nulls;

	w = x->f->w;
	switch(FILE(x->f->qid)){
	case Qconsctl:
		if(w->rawing){
			w->rawing = FALSE;
			wsendctlmesg(w, Rawoff, ZR, nil);
		}
		if(w->holding){
			w->holding = FALSE;
			wsendctlmesg(w, Holdoff, ZR, nil);
		}
		w->ctlopen = FALSE;
		break;
	case Qcursor:
		w->cursorp = nil;
		wsetcursor(w, FALSE);
		break;
	case Qmouse:
		w->resized = FALSE;
		w->mouseopen = FALSE;
		if(w->i != nil)
			wsendctlmesg(w, Refresh, w->i->r, nil);
		break;
	/* odd behavior but really ok: replace snarf buffer when /dev/snarf is closed */
	case Qsnarf:
		if(x->f->mode==ORDWR || x->f->mode==OWRITE){
			snarf = runerealloc(snarf, ntsnarf+1);
			cvttorunes(tsnarf, ntsnarf, snarf, &nb, &nsnarf, &nulls);
			free(tsnarf);
			tsnarf = nil;
			ntsnarf = 0;
		}
		break;
	case Qwctl:
		if(x->f->mode==OREAD || x->f->mode==ORDWR)
			w->wctlopen = FALSE;
		break;
	}
	wclose(w);
	filsysrespond(x->fs, x, &t, nil);
}

void
xfidwrite(Xfid *x)
{
	Fcall fc;
	int c, cnt, qid, nb, off, nr;
	char buf[256], *p;
	Point pt;
	Window *w;
	Rune *r;
	Conswritemesg cwm;
	Stringpair pair;
	enum { CWdata, CWflush, NCW };
	Alt alts[NCW+1];

	w = x->f->w;
	if(w->deleted){
		filsysrespond(x->fs, x, &fc, Edeleted);
		return;
	}
	qid = FILE(x->f->qid);
	cnt = x->count;
	off = x->offset;
	x->data[cnt] = 0;
	switch(qid){
	case Qcons:
		nr = x->f->nrpart;
		if(nr > 0){
			memmove(x->data+nr, x->data, cnt);	/* there's room: see malloc in filsysproc */
			memmove(x->data, x->f->rpart, nr);
			cnt += nr;
			x->f->nrpart = 0;
		}
		r = runemalloc(cnt);
		cvttorunes(x->data, cnt-UTFmax, r, &nb, &nr, nil);
		/* approach end of buffer */
		while(fullrune(x->data+nb, cnt-nb)){
			c = nb;
			nb += chartorune(&r[nr], x->data+c);
			if(r[nr])
				nr++;
		}
		if(nb < cnt){
			memmove(x->f->rpart, x->data+nb, cnt-nb);
			x->f->nrpart = cnt-nb;
		}
		x->flushtag = x->tag;

		alts[CWdata].c = w->conswrite;
		alts[CWdata].v = &cwm;
		alts[CWdata].op = CHANRCV;
		alts[CWflush].c = x->flushc;
		alts[CWflush].v = nil;
		alts[CWflush].op = CHANRCV;
		alts[NCW].op = CHANEND;
	
		switch(alt(alts)){
		case CWdata:
			break;
		case CWflush:
			filsyscancel(x);
			return;
		}

		/* received data */
		x->flushtag = -1;
		if(x->flushing){
			recv(x->flushc, nil);	/* wake up flushing xfid */
			pair.s = runemalloc(1);
			pair.ns = 0;
			send(cwm.cw, &pair);		/* wake up window with empty data */
			filsyscancel(x);
			return;
		}
		qlock(&x->active);
		pair.s = r;
		pair.ns = nr;
		send(cwm.cw, &pair);
		fc.count = x->count;
		filsysrespond(x->fs, x, &fc, nil);
		qunlock(&x->active);
		return;

	case Qconsctl:
		if(strncmp(x->data, "holdon", 6)==0){
			if(w->holding++ == 0)
				wsendctlmesg(w, Holdon, ZR, nil);
			break;
		}
		if(strncmp(x->data, "holdoff", 7)==0 && w->holding){
			if(--w->holding == FALSE)
				wsendctlmesg(w, Holdoff, ZR, nil);
			break;
		}
		if(strncmp(x->data, "rawon", 5)==0){
			if(w->holding){
				w->holding = FALSE;
				wsendctlmesg(w, Holdoff, ZR, nil);
			}
			if(w->rawing++ == 0)
				wsendctlmesg(w, Rawon, ZR, nil);
			break;
		}
		if(strncmp(x->data, "rawoff", 6)==0 && w->rawing){
			if(--w->rawing == 0)
				wsendctlmesg(w, Rawoff, ZR, nil);
			break;
		}
		filsysrespond(x->fs, x, &fc, "unknown control message");
		return;

	case Qcursor:
		if(cnt < 2*4+2*2*16)
			w->cursorp = nil;
		else{
			w->cursor.offset.x = BGLONG(x->data+0*4);
			w->cursor.offset.y = BGLONG(x->data+1*4);
			memmove(w->cursor.clr, x->data+2*4, 2*2*16);
			w->cursorp = &w->cursor;
		}
		wsetcursor(w, !sweeping);
		break;

	case Qlabel:
		if(off != 0){
			filsysrespond(x->fs, x, &fc, "non-zero offset writing label");
			return;
		}
		free(w->label);
		w->label = emalloc(cnt+1);
		memmove(w->label, x->data, cnt);
		w->label[cnt] = 0;
		break;

	case Qmouse:
		if(w!=input || Dx(w->screenr)<=0)
			break;
		if(x->data[0] != 'm'){
			filsysrespond(x->fs, x, &fc, Ebadmouse);
			return;
		}
		p = nil;
		pt.x = strtoul(x->data+1, &p, 0);
		if(p == nil){
			filsysrespond(x->fs, x, &fc, Eshort);
			return;
		}
		pt.y = strtoul(p, nil, 0);
		if(w==input && wpointto(mouse->xy)==w)
			wsendctlmesg(w, Movemouse, Rpt(pt, pt), nil);
		break;

	case Qsnarf:
		/* always append only */
		if(ntsnarf > MAXSNARF){	/* avoid thrashing when people cut huge text */
			filsysrespond(x->fs, x, &fc, Elong);
			return;
		}
		tsnarf = erealloc(tsnarf, ntsnarf+cnt+1);	/* room for NUL */
		memmove(tsnarf+ntsnarf, x->data, cnt);
		ntsnarf += cnt;
		snarfversion++;
		break;

	case Qwdir:
		if(cnt == 0)
			break;
		if(x->data[cnt-1] == '\n'){
			if(cnt == 1)
				break;
			x->data[cnt-1] = '\0';
		}
		/* assume data comes in a single write */
		/*
		  * Problem: programs like dossrv, ftp produce illegal UTF;
		  * we must cope by converting it first.
		  */
		snprint(buf, sizeof buf, "%.*s", cnt, x->data);
		if(buf[0] == '/'){
			free(w->dir);
			w->dir = estrdup(buf);
		}else{
			p = emalloc(strlen(w->dir) + 1 + strlen(buf) + 1);
			sprint(p, "%s/%s", w->dir, buf);
			free(w->dir);
			w->dir = cleanname(p);
		}
		break;

	case Qkbdin:
		keyboardsend(x->data, cnt);
		break;

	case Qwctl:
		if(writewctl(x, buf) < 0){
			filsysrespond(x->fs, x, &fc, buf);
			return;
		}
		flushimage(display, 1);
		break;

	default:
		fprint(2, buf, "unknown qid %d in write\n", qid);
		sprint(buf, "unknown qid in write");
		filsysrespond(x->fs, x, &fc, buf);
		return;
	}
	fc.count = cnt;
	filsysrespond(x->fs, x, &fc, nil);
}

int
readwindow(Image *i, char *t, Rectangle r, int offset, int n)
{
	int ww, y;

	offset -= 5*12;
	ww = bytesperline(r, screen->depth);
	r.min.y += offset/ww;
	if(r.min.y >= r.max.y)
		return 0;
	y = r.min.y + n/ww;
	if(y < r.max.y)
		r.max.y = y;
	if(r.max.y <= r.min.y)
		return 0;
	return unloadimage(i, r, (uchar*)t, n);
}

void
xfidread(Xfid *x)
{
	Fcall fc;
	int n, off, cnt, c;
	uint qid;
	char buf[128], *t;
	char cbuf[30];
	Window *w;
	Mouse ms;
	Rectangle r;
	Image *i;
	Channel *c1, *c2;	/* chan (tuple(char*, int)) */
	Consreadmesg crm;
	Mousereadmesg mrm;
	Consreadmesg cwrm;
	Stringpair pair;
	enum { CRdata, CRflush, NCR };
	enum { MRdata, MRflush, NMR };
	enum { WCRdata, WCRflush, NWCR };
	Alt alts[NCR+1];

	w = x->f->w;
	if(w->deleted){
		filsysrespond(x->fs, x, &fc, Edeleted);
		return;
	}
	qid = FILE(x->f->qid);
	off = x->offset;
	cnt = x->count;
	switch(qid){
	case Qcons:
		x->flushtag = x->tag;

		alts[CRdata].c = w->consread;
		alts[CRdata].v = &crm;
		alts[CRdata].op = CHANRCV;
		alts[CRflush].c = x->flushc;
		alts[CRflush].v = nil;
		alts[CRflush].op = CHANRCV;
		alts[NMR].op = CHANEND;

		switch(alt(alts)){
		case CRdata:
			break;
		case CRflush:
			filsyscancel(x);
			return;
		}

		/* received data */
		x->flushtag = -1;
		c1 = crm.c1;
		c2 = crm.c2;
		t = malloc(cnt+UTFmax+1);	/* room to unpack partial rune plus */
		pair.s = t;
		pair.ns = cnt;
		send(c1, &pair);
		if(x->flushing){
			recv(x->flushc, nil);	/* wake up flushing xfid */
			recv(c2, nil);			/* wake up window and toss data */
			free(t);
			filsyscancel(x);
			return;
		}
		qlock(&x->active);
		recv(c2, &pair);
		fc.data = pair.s;
		fc.count = pair.ns;
		filsysrespond(x->fs, x, &fc, nil);
		free(t);
		qunlock(&x->active);
		break;

	case Qlabel:
		n = strlen(w->label);
		if(off > n)
			off = n;
		if(off+cnt > n)
			cnt = n-off;
		fc.data = w->label+off;
		fc.count = cnt;
		filsysrespond(x->fs, x, &fc, nil);
		break;

	case Qmouse:
		x->flushtag = x->tag;

		alts[MRdata].c = w->mouseread;
		alts[MRdata].v = &mrm;
		alts[MRdata].op = CHANRCV;
		alts[MRflush].c = x->flushc;
		alts[MRflush].v = nil;
		alts[MRflush].op = CHANRCV;
		alts[NMR].op = CHANEND;

		switch(alt(alts)){
		case MRdata:
			break;
		case MRflush:
			filsyscancel(x);
			return;
		}

		/* received data */
		x->flushtag = -1;
		if(x->flushing){
			recv(x->flushc, nil);		/* wake up flushing xfid */
			recv(mrm.cm, nil);			/* wake up window and toss data */
			filsyscancel(x);
			return;
		}
		qlock(&x->active);
		recv(mrm.cm, &ms);
		c = 'm';
		if(w->resized)
			c = 'r';
		n = sprint(buf, "%c%11d %11d %11d %11ld ", c, ms.xy.x, ms.xy.y, ms.buttons, ms.msec);
		w->resized = 0;
		fc.data = buf;
		fc.count = min(n, cnt);
		filsysrespond(x->fs, x, &fc, nil);
		qunlock(&x->active);
		break;

	case Qcursor:
		filsysrespond(x->fs, x, &fc, "cursor read not implemented");
		break;

	/* The algorithm for snarf and text is expensive but easy and rarely used */
	case Qsnarf:
		getsnarf();
		if(nsnarf)
			t = runetobyte(snarf, nsnarf, &n);
		else {
			t = nil;
			n = 0;
		}
		goto Text;

	case Qtext:
		t = wcontents(w, &n);
		goto Text;

	Text:
		if(off > n){
			off = n;
			cnt = 0;
		}
		if(off+cnt > n)
			cnt = n-off;
		fc.data = t+off;
		fc.count = cnt;
		filsysrespond(x->fs, x, &fc, nil);
		free(t);
		break;

	case Qwdir:
		t = estrdup(w->dir);
		n = strlen(t);
		goto Text;

	case Qwinid:
		n = sprint(buf, "%11d ", w->id);
		t = estrdup(buf);
		goto Text;


	case Qwinname:
		n = strlen(w->name);
		if(n == 0){
			filsysrespond(x->fs, x, &fc, "window has no name");
			break;
		}
		t = estrdup(w->name);
		goto Text;

	case Qwindow:
		i = w->i;
		if(i == nil || Dx(w->screenr)<=0){
			filsysrespond(x->fs, x, &fc, Enowindow);
			return;
		}
		r = w->screenr;
		goto caseImage;

	case Qscreen:
		i = display->image;
		if(i == nil){
			filsysrespond(x->fs, x, &fc, "no top-level screen");
			break;
		}
		r = i->r;
		/* fall through */

	caseImage:
		if(off < 5*12){
			n = sprint(buf, "%11s %11d %11d %11d %11d ",
				chantostr(cbuf, screen->chan),
				i->r.min.x, i->r.min.y, i->r.max.x, i->r.max.y);
			t = estrdup(buf);
			goto Text;
		}
		t = malloc(cnt);
		fc.data = t;
		n = readwindow(i, t, r, off, cnt);	/* careful; fc.count is unsigned */
		if(n < 0){
			buf[0] = 0;
			errstr(buf, sizeof buf);
			filsysrespond(x->fs, x, &fc, buf);
		}else{
			fc.count = n;
			filsysrespond(x->fs, x, &fc, nil);
		}
		free(t);
		return;

	case Qwctl:	/* read returns rectangle, hangs if not resized */
		if(cnt < 4*12){
			filsysrespond(x->fs, x, &fc, Etooshort);
			break;
		}
		x->flushtag = x->tag;

		alts[WCRdata].c = w->wctlread;
		alts[WCRdata].v = &cwrm;
		alts[WCRdata].op = CHANRCV;
		alts[WCRflush].c = x->flushc;
		alts[WCRflush].v = nil;
		alts[WCRflush].op = CHANRCV;
		alts[NMR].op = CHANEND;

		switch(alt(alts)){
		case WCRdata:
			break;
		case WCRflush:
			filsyscancel(x);
			return;
		}

		/* received data */
		x->flushtag = -1;
		c1 = cwrm.c1;
		c2 = cwrm.c2;
		t = malloc(cnt+1);	/* be sure to have room for NUL */
		pair.s = t;
		pair.ns = cnt+1;
		send(c1, &pair);
		if(x->flushing){
			recv(x->flushc, nil);	/* wake up flushing xfid */
			recv(c2, nil);			/* wake up window and toss data */
			free(t);
			filsyscancel(x);
			return;
		}
		qlock(&x->active);
		recv(c2, &pair);
		fc.data = pair.s;
		if(pair.ns > cnt)
			pair.ns = cnt;
		fc.count = pair.ns;
		filsysrespond(x->fs, x, &fc, nil);
		free(t);
		qunlock(&x->active);
		break;

	default:
		fprint(2, "unknown qid %d in read\n", qid);
		sprint(buf, "unknown qid in read");
		filsysrespond(x->fs, x, &fc, buf);
		break;
	}
}
