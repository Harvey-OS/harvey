#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

int	winid;

void
Window.init(Window *w, Window *clone, Rectangle r)
{
	Rectangle r1;
	File *f;
	Reffont *rf;
	Rune *rp;
	int nc;

	w->tag.w = w;
	w->body.w = w;
	w->id = ++winid;
	w->inc();
	w->ctlfid = ~0;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	reffont.inc();
	f = .File.addtext(&w->tag);
	w->tag.init(f, r1, &reffont);
	w->tag.what = Tag;
	/* tag is a copy of the contents, not a tracked image */
	if(clone){
		w->tag.delete(0, w->tag.file->nc, TRUE);
		nc = clone->tag.file->nc;
		rp = runemalloc(nc);
		clone->tag.file->read(0, rp, nc);
		w->tag.insert(0, rp, nc, TRUE);
		free(rp);
		w->tag.file->reset();
		w->tag.setselect(nc, nc);
	}
	r1 = r;
	r1.min.y += font->height + 1;
	if(r1.max.y < r1.min.y)
		r1.max.y = r1.min.y;
	f = nil;
	if(clone){
		f = clone->body.file;
		w->body.org = clone->body.org;
		w->isscratch = clone->isscratch;
		rf = .Reffont.get(FALSE, FALSE, FALSE, clone->body.reffont->f->name);
	}else
		rf = .Reffont.get(FALSE, FALSE, FALSE, nil);
	f = f->addtext(&w->body);
	w->body.what = Body;
	w->body.init(f, r1, rf);
	r1.min.y -= 1;
	r1.max.y = r1.min.y+1;
	bitblt(&screen, r1.min, &screen, r1, 0xF);
	w->body.scrdraw();
	w->r = r;
	w->r.max.y = w->body.r.max.y;
	bitblt(&screen, w->tag.scrollr.min, button, button->r, S);
	w->filemenu = TRUE;
	w->maxlines = w->body.maxlines;
	if(clone){
		w->dirty = clone->dirty;
		w->body.setselect(clone->body.q0, clone->body.q1);
		w->settag();
	}
}

int
Window.reshape(Window *w, Rectangle r, int safe)
{
	Rectangle r1;
	int y;
	Bitmap *b;

	r1 = r;
	r1.max.y = r1.min.y + font->height;
	y = r1.max.y;
	if(!safe || !eqrect(w->tag.r, r1)){
		y = w->tag.reshape(r1);
		b = button;
		if(w->body.file->mod && !w->isdir && !w->isscratch)
			b = modbutton;
		bitblt(&screen, w->tag.scrollr.min, b, b->r, S);
	}
	if(!safe || !eqrect(w->body.r, r1)){
		if(y+1+font->height > r.max.y){		/* no body */
			r1.min.y = y;
			r1.max.y = y;
			w->body.reshape(r1);
			w->r = r;
			w->r.max.y = y;
			return y;
		}
		r1 = r;
		r1.min.y = y;
		r1.max.y = y + 1;
		bitblt(&screen, r1.min, &screen, r1, 0xF);
		r1.min.y = y + 1;
		r1.max.y = r.max.y;
		y = w->body.reshape(r1);
		w->r = r;
		w->r.max.y = y;
		w->body.scrdraw();
	}
	w->maxlines = min(w->body.nlines, max(w->maxlines, w->body.maxlines));
	return w->r.max.y;
}

void
Window.lock1(Window *w, int owner)
{
	w->inc();
	w->QLock.lock();
	w->owner = owner;
}

void
Window.lock(Window *w, int owner)
{
	int i;
	File *f;

	f = w->body.file;
	for(i=0; i<f->ntext; i++)
		f->text[i]->w->lock1(owner);
}

void
Window.unlock(Window *w)
{
	int i;
	File *f;

	f = w->body.file;
	for(i=0; i<f->ntext; i++){
		w = f->text[i]->w;
		w->owner = 0;
		w->QLock.unlock();
		w->close();
		/* w->close() can change up f->text; beware */
		if(f->ntext>0 && w != f->text[i]->w)
			--i;	/* w->close() deleted window */
	}
}

void
Window.mousebut(Window *w)
{
	cursorset(div(add(w->tag.scrollr.min, w->tag.scrollr.max), 2));
}

void
Window.dirfree(Window *w)
{
	int i;
	Dirlist *dl;

	if(w->isdir){
		for(i=0; i<w->ndl; i++){
			dl = w->dlp[i];
			free(dl->r);
			free(dl);
		}
		free(w->dlp);
	}
	w->dlp = nil;
	w->ndl = 0;
}

void
Window.close(Window *w)
{
	int i;

	if(w->dec() == 0){
		w->dirfree();
		w->tag.close();
		w->body.close();
		for(i=0; i<w->nincl; i++)
			free(w->incl[i]);
		free(w->incl);
		free(w->events);
		free(w);
	}
}

void
Window.delete(Window *w)
{
	Xfid *x;

	x = w->eventx;
	if(x){
		w->nevents = 0;
		free(w->events);
		w->events = nil;
		w->eventx = nil;
		x->c <-= nil;	/* wake him up */
	}
}

void
Window.undo(Window *w, int isundo)
{
	Text *body;
	int i;
	File *f;
	Window *v;

	body = &w->body;
	(body->q0, body->q1) = body->file->undo(isundo, body->q0, body->q1);
	body->show(body->q0, body->q1);
	f = body->file;
	for(i=0; i<f->ntext; i++){
		v = f->text[i]->w;
		v->dirty = (f->seq != v->putseq);
		if(v != w){
			v->body.q0 = v->body.p0+v->body.org;
			v->body.q1 = v->body.p1+v->body.org;
		}
	}
	w->settag();
}

void
Window.setname(Window *w, Rune *name, int n)
{
	Text *t;
	Window *v;
	int i;

	t = &w->body;
	if(runeeq(t->file->name, t->file->nname, name, n) == TRUE)
		return;
	w->isscratch = FALSE;
	if(n>=6 && runeeq($"/guide", 6, name+(n-6), 6))
		w->isscratch = TRUE;
	else if(n>=7 && runeeq($"+Errors", 7, name+(n-7), 7))
		w->isscratch = TRUE;
	t->file->setname(name, n);
	for(i=0; i<t->file->ntext; i++){
		v = t->file->text[i]->w;
		v->settag();
		v->isscratch = w->isscratch;
	}
}

void
Window.type(Window *w, Text *t, Rune r)
{
	int i;

	t->type(r);
	if(t->what == Body)
		for(i=0; i<t->file->ntext; i++)
			t->file->text[i]->scrdraw();
	w->settag();
}

void
Window.cleartag(Window *w)
{
	int i, n;
	Rune *r;

	/* w must be committed */
	n = w->tag.file->nc;
	r = runemalloc(n);
	w->tag.file->read(0, r, n);
	for(i=0; i<n; i++)
		if(r[i]==' ' || r[i]=='\t')
			break;
	for(; i<n; i++)
		if(r[i] == '|')
			break;
	if(i == n)
		return;
	i++;
	w->tag.delete(i, n, TRUE);
	free(r);
	w->tag.file->mod = FALSE;
	if(w->tag.q0 > i)
		w->tag.q0 = i;
	if(w->tag.q1 > i)
		w->tag.q1 = i;
	w->tag.setselect(w->tag.q0, w->tag.q1);
}

void
Window.settag(Window *w)
{
	int i;
	File *f;

	f = w->body.file;
	for(i=0; i<f->ntext; i++)
		f->text[i]->w->settag1();
}

void
Window.settag1(Window *w)
{
	int i, j, k, n, bar, dirty;
	Rune *new, *old, *r;
	Bitmap *b;
	uint q0, q1;

	if(w->tag.file->mod)
		w->commit(&w->tag);	/* check file name */
	old = runemalloc(w->tag.file->nc);
	w->tag.file->read(0, old, w->tag.file->nc);
	for(i=0; i<w->tag.file->nc; i++)
		if(old[i]==' ' || old[i]=='\t')
			break;
	if(runeeq(old, i, w->body.file->name, w->body.file->nname) == FALSE){
		w->tag.delete(0, i, TRUE);
		w->tag.insert(0, w->body.file->name, w->body.file->nname, TRUE);
		free(old);
		old = runemalloc(w->tag.file->nc+1);
		w->tag.file->read(0, old, w->tag.file->nc);
		old[w->tag.file->nc] = 0;
	}
	new = runemalloc(w->body.file->nname+100);
	i = 0;
	runemove(new+i, w->body.file->name, w->body.file->nname);
	i += w->body.file->nname;
	runemove(new+i, $" Del Snarf", 10);
	i += 10;
	dirty = w->body.file->nname && (w->body.ncache || w->body.file->seq!=w->putseq);
	if(w->filemenu){
		if(w->body.file->delta.nc>0 || w->body.ncache){
			runemove(new+i, $" Undo", 5);
			i += 5;
		}
		if(w->body.file->epsilon.nc > 0){
			runemove(new+i, $" Redo", 5);
			i += 5;
		}
		if(!w->isdir && dirty){
			runemove(new+i, $" Put", 4);
			i += 4;
		}
	}
	if(w->isdir){
		runemove(new+i, $" Get", 4);
		i += 4;
	}
	runemove(new+i, $" |", 2);
	i += 2;
	r = strrune(old, '|');
	if(r)
		k = r-old+1;
	else{
		k = w->tag.file->nc;
		if(w->body.file->seq == 0){
			runemove(new+i, $" Look ", 6);
			i += 6;
		}
	}
	if(runeeq(new, i, old, k) == FALSE){
		n = k;
		if(n > i)
			n = i;
		for(j=0; j<n; j++)
			if(old[j] != new[j])
				break;
		q0 = w->tag.q0;
		q1 = w->tag.q1;
		w->tag.delete(j, k, TRUE);
		w->tag.insert(j, new+j, i-j, TRUE);
		/* try to preserve user selection */
		r = strrune(old, '|');
		if(r){
			bar = r-old;
			if(q0 > bar){
				bar = (strrune(new, '|')-new)-bar;
				w->tag.q0 = q0+bar;
				w->tag.q1 = q1+bar;
			}
		}
	}
	free(old);
	free(new);
	w->tag.file->mod = FALSE;
	n = w->tag.file->nc+w->tag.ncache;
	if(w->tag.q0 > n)
		w->tag.q0 = n;
	if(w->tag.q1 > n)
		w->tag.q1 = n;
	w->tag.setselect(w->tag.q0, w->tag.q1);
	b = button;
	if(!w->isdir && !w->isscratch && (w->body.file->mod || w->body.ncache))
		b = modbutton;
	bitblt(&screen, w->tag.scrollr.min, b, b->r, S);
}

void
Window.commit(Window *w, Text *t)
{
	Rune *r;
	int i;
	File *f;

	t->commit(TRUE);
	f = t->file;
	if(f->ntext > 1)
		for(i=0; i<f->ntext; i++)
			f->text[i]->commit(FALSE);	/* no-op for t */
	if(t->what == Body)
		return;
	r = runemalloc(w->tag.file->nc);
	w->tag.file->read(0, r, w->tag.file->nc);
	for(i=0; i<w->tag.file->nc; i++)
		if(r[i]==' ' || r[i]=='\t')
			break;
	if(runeeq(r, i, w->body.file->name, w->body.file->nname) == FALSE){
		seq++;
		w->body.file->mark();
		w->body.file->mod = TRUE;
		w->dirty = TRUE;
		w->setname(r, i);
		w->settag();
	}
}

void
Window.addincl(Window *w, Rune *r, int n)
{
	byte *a;
	Dir d;

	a = runetobyte(r, n);
	rescue{
		warning(nil, "%s: %r\n", a);
		free(r);
		free(a);
		return;
	}
	if(dirstat(a, &d) < 0){
		if(a[0] == '/')
			raise;
		(r, n) = dirname(&w->body, r, n);
		free(a);
		a = runetobyte(r, n);
		if(dirstat(a, &d) < 0)
			raise;
		r = runerealloc(r, n+1);
		r[n] = 0;
	}
	free(a);
	if((d.mode&CHDIR) == 0){
		warning(nil, "%s: not a directory\n", a);
		free(r);
		return;
	}
	w->nincl++;
	w->incl = realloc(w->incl, w->nincl*sizeof(Rune*));
	memmove(w->incl+1, w->incl, (w->nincl-1)*sizeof(Rune*));
	w->incl[0] = runemalloc(n+1);
	runemove(w->incl[0], r, n);
	free(r);
}

int
Window.clean(Window *w, int conservative)	/* as it stands, conservative is always TRUE */
{
	if(w->isscratch || w->isdir)	/* don't whine if it's a guide file, error window, etc. */
		return TRUE;
	if(!conservative && w->nopen[QWevent]>0)
		return TRUE;
	if(w->dirty){
		if(w->body.file->nname)
			warning(nil, "%.*S modified\n", w->body.file->nname, w->body.file->name);
		else{
			if(w->body.file->nc < 100)	/* don't whine if it's too small */
				return TRUE;
			warning(nil, "unnamed file modified\n");
		}
		w->dirty = FALSE;
		return FALSE;
	}
	return TRUE;
}

void
Window.ctlprint(Window *w, byte *buf)
{
	sprint(buf, "%11d %11d %11d %11d %11d ", w->id, w->tag.file->nc,
		w->body.file->nc, w->isdir, w->dirty);
}

void
Window.event(Window *w, byte *fmt, ...)
{
	int n;
	byte *b;
	Xfid *x;

	if(w->nopen[QWevent] == 0)
		return;
	if(w->owner == 0)
		error("no window owner");
	b = fbufalloc();
	n = doprint(b, b+BUFSIZE+1, fmt, ...) - b;
	w->events = realloc(w->events, w->nevents+1+n);
	w->events[w->nevents++] = w->owner;
	memmove(w->events+w->nevents, b, n);
	fbuffree(b);
	w->nevents += n;
	x = w->eventx;
	if(x){
		w->eventx = nil;
		x->c <-= nil;
	}
}
