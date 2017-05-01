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
xfidallocthread(void* vacio)
{
	print_func_entry();
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
	print_func_exit();
}

Channel*
xfidinit(void)
{
	print_func_entry();
	cxfidalloc = chancreate(sizeof(Xfid*), 0);
	cxfidfree = chancreate(sizeof(Xfid*), 0);
	threadcreate(xfidallocthread, nil, STACK);
	print_func_exit();
	return cxfidalloc;
}

void
xfidctl(void *arg)
{
	print_func_entry();
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
	print_func_exit();
}

void
xfidflush(Xfid *x)
{
	print_func_entry();
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
	print_func_exit();
}

void
xfidattach(Xfid *x)
{
	extern Console HardwareConsole;
	print_func_entry();
	Fcall t;
	int id;
	Window *w;
	char *err = nil;
	int pid = -1, newlymade;

	t.qid = x->f->qid;
	qlock(&all);
	w = nil;
	//err = Eunkid;
	newlymade = FALSE;

	if(x->aname[0] == 'N'){	/* N  */
		if(pid == 0)
			pid = -1;	/* make sure we don't pop a shell! - UGH */
		w = new(&HardwareConsole, pid, nil, nil, nil);
		newlymade = TRUE;
	}else{
		id = atoi(x->aname);
		w = wlookid(id);
	}
	x->f->w = w;
	if(w == nil){
		qunlock(&all);
		x->f->busy = FALSE;
		filsysrespond(x->fs, x, &t, err);
		print_func_exit();
		return;
	}
	if(!newlymade)	/* counteract dec() in winshell() */
		incref(w);
	qunlock(&all);
	filsysrespond(x->fs, x, &t, nil);
	print_func_exit();
}

void
xfidopen(Xfid *x)
{
	print_func_entry();
	Fcall t;
	Window *w;

	w = x->f->w;
	if(w->deleted){
		filsysrespond(x->fs, x, &t, Edeleted);
		print_func_exit();
		return;
	}
	switch(FILE(x->f->qid)){
	case Qconsctl:
		if(w->ctlopen){
			filsysrespond(x->fs, x, &t, Einuse);
			print_func_exit();
			return;
		}
		w->ctlopen = TRUE;
		break;
	case Qkbdin:
		if(w !=  wkeyboard){
			filsysrespond(x->fs, x, &t, Eperm);
			print_func_exit();
			return;
		}
		break;
	case Qmouse:
		if(w->mouseopen){
			filsysrespond(x->fs, x, &t, Einuse);
			print_func_exit();
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
				print_func_exit();
				return;
			}
			w->wctlopen = TRUE;
			w->wctlready = 1;
			wsendctlmesg(w, Wakeup, nil);
		}
		break;
	}
	t.qid = x->f->qid;
	t.iounit = messagesize-IOHDRSZ;
	x->f->open = TRUE;
	x->f->mode = x->mode;
	filsysrespond(x->fs, x, &t, nil);
	print_func_exit();
}

void
xfidclose(Xfid *x)
{
	print_func_entry();
	Fcall t;
	Window *w;
	int nb, nulls;

	w = x->f->w;
	switch(FILE(x->f->qid)){
	case Qconsctl:
		if(w->rawing){
			w->rawing = FALSE;
			wsendctlmesg(w, Rawoff, nil);
		}
		if(w->holding){
			w->holding = FALSE;
			wsendctlmesg(w, Holdoff, nil);
		}
		w->ctlopen = FALSE;
		break;
#if 0
	case Qmouse:
		w->resized = FALSE;
		w->mouseopen = FALSE;
		if(w->i != nil)
			wsendctlmesg(w, Refresh, nil);
		break;
#endif
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
	print_func_exit();
}

void
xfidwrite(Xfid *x)
{
	print_func_entry();
	Fcall fc;
	int c, cnt, qid, nb, off, nr;
	char buf[256], *p;
	Window *w;
	Rune *r;
	Conswritemesg cwm;
	Stringpair pair;
	enum { CWdata, CWflush, NCW };
	Alt alts[NCW+1];

	w = x->f->w;
	if(w->deleted){
		filsysrespond(x->fs, x, &fc, Edeleted);
		print_func_exit();
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
			print_func_exit();
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
			print_func_exit();
			return;
		}
		qlock(&x->active);
		pair.s = r;
		pair.ns = nr;
		send(cwm.cw, &pair);
		fc.count = x->count;
		filsysrespond(x->fs, x, &fc, nil);
		qunlock(&x->active);
		print_func_exit();
		return;

	case Qconsctl:
		if(strncmp(x->data, "holdon", 6)==0){
			if(w->holding++ == 0)
				wsendctlmesg(w, Holdon, nil);
			break;
		}
		if(strncmp(x->data, "holdoff", 7)==0 && w->holding){
			if(--w->holding == FALSE)
				wsendctlmesg(w, Holdoff, nil);
			break;
		}
		if(strncmp(x->data, "rawon", 5)==0){
			if(w->holding){
				w->holding = FALSE;
				wsendctlmesg(w, Holdoff, nil);
			}
			if(w->rawing++ == 0)
				wsendctlmesg(w, Rawon, nil);
			break;
		}
		if(strncmp(x->data, "rawoff", 6)==0 && w->rawing){
			if(--w->rawing == 0)
				wsendctlmesg(w, Rawoff, nil);
			break;
		}
		filsysrespond(x->fs, x, &fc, "unknown control message");
		print_func_exit();
		return;

	case Qlabel:
		if(off != 0){
			filsysrespond(x->fs, x, &fc, "non-zero offset writing label");
			print_func_exit();
			return;
		}
		free(w->label);
		w->label = emalloc(cnt+1);
		memmove(w->label, x->data, cnt);
		w->label[cnt] = 0;
		break;

	case Qmouse:
		if(w!=input)
			break;
		if(x->data[0] != 'm'){
			filsysrespond(x->fs, x, &fc, Ebadmouse);
			print_func_exit();
			return;
		}
		p = nil;
		if(p == nil){
			filsysrespond(x->fs, x, &fc, Eshort);
			print_func_exit();
			return;
		}
		break;

	case Qsnarf:
		/* always append only */
		if(ntsnarf > MAXSNARF){	/* avoid thrashing when people cut huge text */
			filsysrespond(x->fs, x, &fc, Elong);
			print_func_exit();
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
			print_func_exit();
			return;
		}
		break;

	default:
		fprint(2, buf, "unknown qid %d in write\n", qid);
		sprint(buf, "unknown qid in write");
		filsysrespond(x->fs, x, &fc, buf);
		print_func_exit();
		return;
	}
	fc.count = cnt;
	filsysrespond(x->fs, x, &fc, nil);
	print_func_exit();
}

void
xfidread(Xfid *x)
{
	print_func_entry();
	Fcall fc;
	int n, off, cnt, c;
	uint qid;
	char buf[128], *t;
	Window *w;
	Mouse ms;
	Channel *c1, *c2;	/* chan (tuple(char*, int)) */
	Consreadmesg crm;
	Mousereadmesg mrm;

	Stringpair pair;
	enum { CRdata, CRflush, NCR };
	enum { MRdata, MRflush, NMR };
	enum { WCRdata, WCRflush, NWCR };
	Alt alts[NCR+1];

	w = x->f->w;
	if(w->deleted){
		filsysrespond(x->fs, x, &fc, Edeleted);
		print_func_exit();
		return;
	}
	qid = FILE(x->f->qid);
	off = x->offset;
	cnt = x->count;
	/* for now, a zero length read of anything, even invalid things,
	 * just returns immediately.
	 */
	if (cnt == 0){
		qlock(&x->active);
		x->flushtag = -1;
		fc.data = nil;
		fc.count = 0;
		filsysrespond(x->fs, x, &fc, nil);
		qunlock(&x->active);
		print_func_exit();
		return;
	}
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
			print_func_exit();
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
			print_func_exit();
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
			print_func_exit();
			return;
		}

		/* received data */
		x->flushtag = -1;
		if(x->flushing){
			recv(x->flushc, nil);		/* wake up flushing xfid */
			recv(mrm.cm, nil);			/* wake up window and toss data */
			filsyscancel(x);
			print_func_exit();
			return;
		}
		qlock(&x->active);
		recv(mrm.cm, &ms);
		c = 'm';
		if(w->resized)
			c = 'r';
		n = sprint(buf, "%c%d%d", c, ms.buttons, ms.msec);
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

	default:
		fprint(2, "unknown qid %d in read\n", qid);
		sprint(buf, "unknown qid in read");
		filsysrespond(x->fs, x, &fc, buf);
		break;
	}
	print_func_exit();
}
