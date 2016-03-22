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
#include <mouse.h>
#include <keyboard.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"

#define MOVEIT if(0)

enum
{
	HiWater	= 640000,	/* max size of history */
	LoWater	= 400000,	/* min size of history after max'ed */
	MinWater	= 20000,	/* room to leave available when reallocating */
};

static	int		topped;
static	int		id;

Window*
wmk(Mousectl *mc, Channel *ck, Channel *cctl)
{
	Window *w;

	w = emalloc(sizeof(Window));
	w->mc = *mc;
	w->ck = ck;
	w->cctl = cctl;
	w->conswrite = chancreate(sizeof(Conswritemesg), 0);
	w->consread =  chancreate(sizeof(Consreadmesg), 0);
	w->mouseread =  chancreate(sizeof(Mousereadmesg), 0);
	w->wctlread =  chancreate(sizeof(Consreadmesg), 0);
	w->maxtab = maxtab*stringwidth(font, "0");
	w->id = ++id;
	w->notefd = -1;
	w->dir = estrdup(startdir);
	w->label = estrdup("<unnamed>");
	incref(w);	/* ref will be removed after mounting; avoids delete before ready to be deleted */
	return w;
}

void
wsetname(Window *w)
{
	int i, n;
	char err[ERRMAX];
	
	n = sprint(w->name, "window.%d.%d", w->id, w->namecount++);
	for(i='A'; i<='Z'; i++){
		if(nameimage(w->i, w->name, 1) > 0)
			return;
		errstr(err, sizeof err);
		if(strcmp(err, "image name in use") != 0)
			break;
		w->name[n] = i;
		w->name[n+1] = 0;
	}
	w->name[0] = 0;
	fprint(2, "rio: setname failed: %s\n", err);
}

int
wclose(Window *w)
{
	int i;

	i = decref(w);
	if(i > 0)
		return 0;
	if(i < 0)
		error("negative ref count");
	if(!w->deleted)
		wclosewin(w);
	wsendctlmesg(w, Exited, ZR, nil);
	return 1;
}


void
winctl(void *arg)
{
	Rune *rp, *bp, *tp, *up, *kbdr;
	uint qh;
	int nr, nb, c, wid, i, npart, initial, lastb;
	char *s, *t, part[3];
	Window *w;
	Mousestate *mp, m;
	enum { WKey, WMouse, WMouseread, WCtl, WCwrite, WCread, WWread, NWALT };
	Alt alts[NWALT+1];
	Mousereadmesg mrm;
	Conswritemesg cwm;
	Consreadmesg crm;
	Consreadmesg cwrm;
	Stringpair pair;
	Wctlmesg wcm;
	char buf[4*12+1];

	w = arg;
	snprint(buf, sizeof buf, "winctl-id%d", w->id);
	threadsetname(buf);

	mrm.cm = chancreate(sizeof(Mouse), 0);
	cwm.cw = chancreate(sizeof(Stringpair), 0);
	crm.c1 = chancreate(sizeof(Stringpair), 0);
	crm.c2 = chancreate(sizeof(Stringpair), 0);
	cwrm.c1 = chancreate(sizeof(Stringpair), 0);
	cwrm.c2 = chancreate(sizeof(Stringpair), 0);
	

	alts[WKey].c = w->ck;
	alts[WKey].v = &kbdr;
	alts[WKey].op = CHANRCV;
	alts[WMouse].c = w->mc.c;
	alts[WMouse].v = &w->mc.Mouse;
	alts[WMouse].op = CHANRCV;
	alts[WMouseread].c = w->mouseread;
	alts[WMouseread].v = &mrm;
	alts[WMouseread].op = CHANSND;
	alts[WCtl].c = w->cctl;
	alts[WCtl].v = &wcm;
	alts[WCtl].op = CHANRCV;
	alts[WCwrite].c = w->conswrite;
	alts[WCwrite].v = &cwm;
	alts[WCwrite].op = CHANSND;
	alts[WCread].c = w->consread;
	alts[WCread].v = &crm;
	alts[WCread].op = CHANSND;
	alts[WWread].c = w->wctlread;
	alts[WWread].v = &cwrm;
	alts[WWread].op = CHANSND;
	alts[NWALT].op = CHANEND;

	npart = 0;
	lastb = -1;
	for(;;){
		if(w->mouseopen && w->mouse.counter != w->mouse.lastcounter)
			alts[WMouseread].op = CHANSND;
		else
			alts[WMouseread].op = CHANNOP;
		if(w->deleted || !w->wctlready)
			alts[WWread].op = CHANNOP;
		else
			alts[WWread].op = CHANSND;
		/* this code depends on NL and EOT fitting in a single byte */
		/* kind of expensive for each loop; worth precomputing? */
		else{
			alts[WCread].op = CHANNOP;
			for(i=w->qh; i<w->nr; i++){
				c = w->run[i];
				if(c=='\n' || c=='\004'){
					alts[WCread].op = CHANSND;
					break;
				}
			}
		}
		switch(alt(alts)){
		case WKey:
			for(i=0; kbdr[i]!=L'\0'; i++)
				wkeyctl(w, kbdr[i]);
//			wkeyctl(w, r);
///			while(nbrecv(w->ck, &r))
//				wkeyctl(w, r);
			break;
		case WMouse:
			if(w->mouseopen) {
				w->mouse.counter++;

				/* queue click events */
				if(!w->mouse.qfull && lastb != w->mc.buttons) {	/* add to ring */
					mp = &w->mouse.queue[w->mouse.wi];
					if(++w->mouse.wi == nelem(w->mouse.queue))
						w->mouse.wi = 0;
					if(w->mouse.wi == w->mouse.ri)
						w->mouse.qfull = TRUE;
					mp->Mouse = w->mc.Mouse;
					mp->counter = w->mouse.counter;
					lastb = w->mc.buttons;
				}
			} else
				wmousectl(w);
			break;
		case WMouseread:
			/* send a queued event or, if the queue is empty, the current state */
			/* if the queue has filled, we discard all the events it contained. */
			/* the intent is to discard frantic clicking by the user during long latencies. */
			w->mouse.qfull = FALSE;
			if(w->mouse.wi != w->mouse.ri) {
				m = w->mouse.queue[w->mouse.ri];
				if(++w->mouse.ri == nelem(w->mouse.queue))
					w->mouse.ri = 0;
			} else
				m = (Mousestate){w->mc.Mouse, w->mouse.counter};

			w->mouse.lastcounter = m.counter;
			send(mrm.cm, &m.Mouse);
			continue;
		case WCtl:
			if(wctlmesg(w, wcm.type, wcm.r, wcm.image) == Exited){
				chanfree(crm.c1);
				chanfree(crm.c2);
				chanfree(mrm.cm);
				chanfree(cwm.cw);
				chanfree(cwrm.c1);
				chanfree(cwrm.c2);
				threadexits(nil);
			}
			continue;
		case WCwrite:
			recv(cwm.cw, &pair);
			rp = pair.s;
			nr = pair.ns;
			bp = rp;
			for(i=0; i<nr; i++)
				if(*bp++ == '\b'){
					--bp;
					initial = 0;
					tp = runemalloc(nr);
					runemove(tp, rp, i);
					up = tp+i;
					for(; i<nr; i++){
						*up = *bp++;
						if(*up == '\b')
							if(up == tp)
								initial++;
							else
								--up;
						else
							up++;
					}
					if(initial){
						if(initial > w->qh)
							initial = w->qh;
						qh = w->qh-initial;
						wdelete(w, qh, qh+initial);
						w->qh = qh;
					}
					free(rp);
					rp = tp;
					nr = up-tp;
					rp[nr] = 0;
					break;
				}
			w->qh = winsert(w, rp, nr, w->qh)+nr;
			wsetselect(w, w->q0, w->q1);
			free(rp);
			break;
		case WCread:
			recv(crm.c1, &pair);
			t = pair.s;
			nb = pair.ns;
			i = npart;
			npart = 0;
			if(i)
				memmove(t, part, i);
			while(i<nb && (w->qh<w->nr || w->nraw>0)){
				if(w->qh == w->nr){
					wid = runetochar(t+i, &w->raw[0]);
					w->nraw--;
					runemove(w->raw, w->raw+1, w->nraw);
				}else
					wid = runetochar(t+i, &w->run[w->qh++]);
				c = t[i];	/* knows break characters fit in a byte */
				i += wid;
				if(!w->rawing && (c == '\n' || c=='\004')){
					if(c == '\004')
						i--;
					break;
				}
			}
			if(i==nb && w->qh<w->nr && w->run[w->qh]=='\004')
				w->qh++;
			if(i > nb){
				npart = i-nb;
				memmove(part, t+nb, npart);
				i = nb;
			}
			pair.s = t;
			pair.ns = i;
			send(crm.c2, &pair);
			continue;
		case WWread:
			w->wctlready = 0;
			recv(cwrm.c1, &pair);
			if(w->deleted || w->i==nil)
				pair.ns = sprint(pair.s, "");
			else{
				s = "visible";
				for(i=0; i<nhidden; i++)
					if(hidden[i] == w){
						s = "hidden";
						break;
					}
				t = "notcurrent";
				if(w == input)
					t = "current";
				pair.ns = snprint(pair.s, pair.ns, "%11d %11d %11d %11d %s %s ",
					w->i->r.min.x, w->i->r.min.y, w->i->r.max.x, w->i->r.max.y, t, s);
			}
			send(cwrm.c2, &pair);
			continue;
		}
		if(!w->deleted)
			flushimage(display, 1);
	}
}

void
waddraw(Window *w, Rune *r, int nr)
{
	w->raw = runerealloc(w->raw, w->nraw+nr);
	runemove(w->raw+w->nraw, r, nr);
	w->nraw += nr;
}

/*
 * Need to do this in a separate proc because if process we're interrupting
 * is dying and trying to print tombstone, kernel is blocked holding p->debug lock.
 */
void
interruptproc(void *v)
{
	int *notefd;

	notefd = v;
	write(*notefd, "interrupt", 9);
	free(notefd);
}

void
wdelete(Window *w, uint q0, uint q1)
{
	uint n, p0, p1;

	n = q1-q0;
	if(n == 0)
		return;
	runemove(w->run+q0, w->run+q1, w->nr-q1);
	w->nr -= n;
	if(q0 < w->q0)
		w->q0 -= min(n, w->q0-q0);
	if(q0 < w->q1)
		w->q1 -= min(n, w->q1-q0);
	if(q1 < w->qh)
		w->qh -= n;
	else if(q0 < w->qh)
		w->qh = q0;
	if(q1 <= w->org)
		w->org -= n;
	else if(q0 < w->org+w->nchars){
		p1 = q1 - w->org;
		if(p1 > w->nchars)
			p1 = w->nchars;
		if(q0 < w->org){
			w->org = q0;
			p0 = 0;
		}else
			p0 = q0 - w->org;
		frdelete(w, p0, p1);
		wfill(w);
	}
}


static Window	*clickwin;
static uint	clickmsec;
static Window	*selectwin;
static uint	selectq;

void
wselect(Window *w)
{
	uint q0, q1;
	int b, x, y, first;

	first = 1;
	selectwin = w;
	/*
	 * Double-click immediately if it might make sense.
	 */
	b = w->mc.buttons;
	q0 = w->q0;
	q1 = w->q1;
	selectq = w->org+frcharofpt(w, w->mc.xy);
	if(clickwin==w && w->mc.msec-clickmsec<500)
	if(q0==q1 && selectq==w->q0){
		wdoubleclick(w, &q0, &q1);
		wsetselect(w, q0, q1);
		q0 = w->q0;	/* may have changed */
		q1 = w->q1;
		selectq = q0;
	}
	if(w->mc.buttons == b){
		w->scroll = framescroll;
		/* horrible botch: while asleep, may have lost selection altogether */
		if(selectq > w->nr)
			selectq = w->org + w->p0;
		if(selectq < w->org)
			q0 = selectq;
		else
			q0 = w->org + w->p0;
		if(selectq > w->org+w->nchars)
			q1 = selectq;
		else
			q1 = w->org+w->p1;
	}
	if(q0 == q1){
		if(q0==w->q0 && clickwin==w && w->mc.msec-clickmsec<500){
			wdoubleclick(w, &q0, &q1);
			clickwin = nil;
		}else{
			clickwin = w;
			clickmsec = w->mc.msec;
		}
	}else
		clickwin = nil;
	wsetselect(w, q0, q1);
	while(w->mc.buttons){
		w->mc.msec = 0;
		b = w->mc.buttons;
		if(b & 6){
			if(b & 2){
				wsnarf(w);
				wcut(w);
			}else{
				if(first){
					first = 0;
					getsnarf();
				}
				wpaste(w);
			}
		}
		while(w->mc.buttons == b)
			readmouse(&w->mc);
		clickwin = nil;
	}
}

void
wsendctlmesg(Window *w, int type, Rectangle r, Image *image)
{
	Wctlmesg wcm;

	wcm.type = type;
	wcm.r = r;
	wcm.image = image;
	send(w->cctl, &wcm);
}

int
wctlmesg(Window *w, int m, Rectangle r, Image *i)
{
	char buf[64];

	switch(m){
	default:
		error("unknown control message");
		break;
	case Wakeup:
		break;
	case Rawon:
		break;
	case Rawoff:
		if(w->deleted)
			break;
		while(w->nraw > 0){
			wkeyctl(w, w->raw[0]);
			--w->nraw;
			runemove(w->raw, w->raw+1, w->nraw);
		}
		break;
	case Holdon:
	case Holdoff:
		if(w->deleted)
			break;
		break;
	case Deleted:
		if(w->deleted)
			break;
		write(w->notefd, "hangup", 6);
		proccreate(deletetimeoutproc, estrdup(w->name), 4096);
		wclosewin(w);
		break;
	case Exited:
		frclear(w, TRUE);
		close(w->notefd);
		chanfree(w->mc.c);
		chanfree(w->ck);
		chanfree(w->cctl);
		chanfree(w->conswrite);
		chanfree(w->consread);
		chanfree(w->mouseread);
		chanfree(w->wctlread);
		free(w->raw);
		free(w->run);
		free(w->dir);
		free(w->label);
		free(w);
		break;
	}
	return m;
}

void
wcurrent(Window *w)
{
	Window *oi;

	if(wkeyboard!=nil && w==wkeyboard)
		return;
	oi = input;
	input = w;
	if(w != oi){
		if(oi){
			oi->wctlready = 1;
			wsendctlmesg(oi, Wakeup, ZR, nil);
		}
		if(w){
			w->wctlready = 1;
			wsendctlmesg(w, Wakeup, ZR, nil);
		}
	}
}

Window*
wtop(Point pt)
{
	Window *w;

	w = wpointto(pt);
	if(w){
		if(w->topped == topped)
			return nil;
		topwindow(w->i);
		wcurrent(w);
		w->topped = ++topped;
	}
	return w;
}

void
wtopme(Window *w)
{
	if(w!=nil && w->i!=nil && !w->deleted && w->topped!=topped){
		topwindow(w->i);
		w->topped = ++ topped;
	}
}

void
wbottomme(Window *w)
{
	if(w!=nil && w->i!=nil && !w->deleted){
		bottomwindow(w->i);
		w->topped = - ++topped;
	}
}

Window*
wlookid(int id)
{
	int i;

	for(i=0; i<nwindow; i++)
		if(window[i]->id == id)
			return window[i];
	return nil;
}

void
wclosewin(Window *w)
{
	Rectangle r;
	int i;

	w->deleted = TRUE;
	if(w == input){
		input = nil;
	}
	if(w == wkeyboard)
		wkeyboard = nil;
	for(i=0; i<nhidden; i++)
		if(hidden[i] == w){
			--nhidden;
			memmove(hidden+i, hidden+i+1, (nhidden-i)*sizeof(hidden[0]));
			hidden[nhidden] = nil;
			break;
		}
	for(i=0; i<nwindow; i++)
		if(window[i] == w){
			--nwindow;
			memmove(window+i, window+i+1, (nwindow-i)*sizeof(Window*));
			w->deleted = TRUE;
			r = w->i->r;
			/* move it off-screen to hide it, in case client is slow in letting it go */
			MOVEIT originwindow(w->i, r.min, view->r.max);
			freeimage(w->i);
			w->i = nil;
			return;
		}
	error("unknown window in closewin");
}

void
wsetpid(Window *w, int pid, int dolabel)
{
	char buf[128];
	int fd;

	w->pid = pid;
	if(dolabel){
		sprint(buf, "rc %d", pid);
		free(w->label);
		w->label = estrdup(buf);
	}
	sprint(buf, "/proc/%d/notepg", pid);
	fd = open(buf, OWRITE|OCEXEC);
	if(w->notefd > 0)
		close(w->notefd);
	w->notefd = fd;
}

void
winshell(void *args)
{
	Window *w;
	Channel *pidc;
	void **arg;
	char *cmd, *dir;
	char **argv;

	arg = args;
	w = arg[0];
	pidc = arg[1];
	cmd = arg[2];
	argv = arg[3];
	dir = arg[4];
	rfork(RFNAMEG|RFFDG|RFENVG);
	if(filsysmount(filsys, w->id) < 0){
		fprint(2, "mount failed: %r\n");
		sendul(pidc, 0);
		threadexits("mount failed");
	}
	close(0);
	if(open("/dev/cons", OREAD) < 0){
		fprint(2, "can't open /dev/cons: %r\n");
		sendul(pidc, 0);
		threadexits("/dev/cons");
	}
	close(1);
	if(open("/dev/cons", OWRITE) < 0){
		fprint(2, "can't open /dev/cons: %r\n");
		sendul(pidc, 0);
		threadexits("open");	/* BUG? was terminate() */
	}
	if(wclose(w) == 0){	/* remove extra ref hanging from creation */
		notify(nil);
		dup(1, 2);
		if(dir)
			chdir(dir);
		procexec(pidc, cmd, argv);
		_exits("exec failed");
	}
}

