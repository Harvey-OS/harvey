#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

void
Text.init(Text *t, File *f, Rectangle r, Reffont *rf)
{
	t->file = f;
	t->all = r;
	t->scrollr = r;
	t->scrollr.max.x = r.min.x+Scrollwid;
	t->lastsr = nullrect;
	r.min.x += Scrollwid+Scrollgap;
	t->eq0 = ~0;
	t->ncache = 0;
	t->reffont = rf;
	t->redraw(r, rf->f, &screen, -1);
}

void
Text.redraw(Text *t, Rectangle r, Font *f, Bitmap *b, int odx)
{
	int c;

	frinit(t, r, f, b);
	c = '0';
	if(t->what==Body && t->w!=nil && t->w->isdir)
		c = ' ';
	t->maxtab = Maxtab*charwidth(f, c);
	if(t->what==Body && t->w->isdir && odx!=Dx(t->all)){
		if(t->maxlines > 0){
			t->reset();
			t->columnate(t->w->dlp,  t->w->ndl);
			t->show(0, 0);
		}
	}else{
		t->fill();
		t->setselect(t->q0, t->q1);
	}
}

int
Text.reshape(Text *t, Rectangle r)
{
	int odx;

	if(Dy(r) > 0)
		r.max.y -= Dy(r)%t->font->height;
	else
		r.max.y = r.min.y;
	odx = Dx(t->all);
	t->all = r;
	t->scrollr = r;
	t->scrollr.max.x = r.min.x+Scrollwid;
	t->lastsr = nullrect;
	r.min.x += Scrollwid+Scrollgap;
	frclear(t);
	t->redraw(r, t->font, t->b, odx);
	return r.max.y;
}

void
Text.close(Text *t)
{
	free(t->cache);
	frclear(t);
	t->file->deltext(t);
	t->reffont->close();
	if(argtext == t)
		argtext = nil;
	if(typetext == t)
		typetext = nil;
	if(seltext == t)
		seltext = nil;
	if(mousetext == t)
		mousetext = nil;
	if(barttext == t)
		barttext = nil;
}

int
dircmp(void *a, void *b)
{
	Dirlist *da, *db;
	int i, n;

	da = *(Dirlist**)a;
	db = *(Dirlist**)b;
	n = min(da->nr, db->nr);
	i = memcmp(da->r, db->r, n*sizeof(Rune));
	if(i)
		return i;
	return da->nr - db->nr;
}

void
Text.columnate(Text *t, Dirlist **dlp, int ndl)
{
	int i, j, w, colw, mint, maxt, ncol, nrow;
	Dirlist *dl;
	uint q1;

	if(t->file->ntext > 1)
		return;
	mint = charwidth(t->font, ' ');
	/* go for narrower tabs */
	t->maxtab = Maxtab*mint;
	maxt = t->maxtab;
	colw = 0;
	for(i=0; i<ndl; i++){
		dl = dlp[i];
		w = dl->wid;
		if(maxt-w%maxt < mint)
			w += mint;
		if(w % maxt)
			w += maxt-(w%maxt);
		if(w > colw)
			colw = w;
	}
	if(colw == 0)
		ncol = 1;
	else
		ncol = max(1, Dx(t->r)/colw);
	nrow = (ndl+ncol-1)/ncol;

	q1 = 0;
	for(i=0; i<nrow; i++){
		for(j=i; j<ndl; j+=nrow){
			dl = dlp[j];
			t->file->insert(q1, dl->r, dl->nr);
			q1 += dl->nr;
			if(j+nrow >= ndl)
				break;
			w = dl->wid;
			if(maxt-w%maxt < mint){
				t->file->insert(q1, $"\t", 1);
				q1++;
				w += mint;
			}
			do{
				t->file->insert(q1, $"\t", 1);
				q1++;
				w += maxt-(w%maxt);
			}while(w < colw);
		}
		t->file->insert(q1, $"\n", 1);
		q1++;
	}
}

uint
Text.load(Text *t, uint q0, byte *file)
{
	Rune *rp;
	Dirlist *dl, **dlp;
	int fd, i, n, ndl, nulls;
	uint q, q1;
	Dir d, *dbuf;
	byte tmp[NAMELEN+1];
	Text *u;

	if(t->ncache!=0 || t->file->nc || t->w==nil || t!=&t->w->body || (t->w->isdir && t->file->nname==0))
		error("text.load");
	fd = open(file, OREAD);
	rescue{
		close(fd);
		return 0;
	}
	if(fd < 0){
		warning(nil, "can't open %s: %r\n", file);
		raise;
	}
	if(dirfstat(fd, &d) < 0){
		warning(nil, "can't fstat %s: %r\n", file);
		raise;
	}
	nulls = FALSE;
	if(d.qid.path & CHDIR){
		/* this is checked in get() but it's possible the file changed underfoot */
		if(t->file->ntext > 1){
			warning(nil, "%s is a directory; can't read with multiple windows on it\n", file);
			raise;
		}
		t->w->isdir = TRUE;
		t->w->filemenu = FALSE;
		if(t->file->name[t->file->nname-1] != '/'){
			rp = runemalloc(t->file->nname+1);
			runemove(rp, t->file->name, t->file->nname);
			rp[t->file->nname] = '/';
			t->w->setname(rp, t->file->nname+1);
		}
		dlp = nil;
		ndl = 0;
		dbuf = (Dir*)fbufalloc();
		while((n=dirread(fd, dbuf, BUFSIZE-(BUFSIZE)%sizeof(Dir))) > 0){
			n /= sizeof(Dir);
			for(i=0; i<n; i++){
				dl = malloc(sizeof(Dirlist));
				memmove(tmp, dbuf[i].name, NAMELEN);
				if(dbuf[i].mode & CHDIR)
					strcat(tmp, "/");
				(dl->r, dl->nr) = bytetorune(tmp);
				dl->wid = strwidth(t->font, tmp);
				ndl++;
				dlp = realloc(dlp, ndl*sizeof(Dirlist*));
				dlp[ndl-1] = dl;
			}
		}
		fbuffree(dbuf);
		qsort(dlp, ndl, sizeof(Dirlist*), dircmp);
		t->w->dlp = dlp;
		t->w->ndl = ndl;
		t->columnate(dlp, ndl);
		q1 = t->file->nc;
	}else{
		t->w->isdir = FALSE;
		t->w->filemenu = TRUE;
		q1 = q0 + t->file->load(q0, fd, &nulls);
	}
	close(fd);
	rp = fbufalloc();
	for(q=q0; q<q1; q+=n){
		n = q1-q;
		if(n > RBUFSIZE)
			n = RBUFSIZE;
		t->file->read(q, rp, n);
		if(q < t->org)
			t->org += n;
		else if(q <= t->org+t->nchars)
			frinsert(t, rp, rp+n, q-t->org);
		if(t->lastlinefull)
			break;
	}
	fbuffree(rp);
	for(i=0; i<t->file->ntext; i++){
		u = t->file->text[i];
		if(u != t){
			if(u->org > u->file->nc)	/* will be 0 because of reset(), but safety first */
				u->org = 0;
			u->reshape(u->all);
			u->backnl(u->org, 0);	/* go to beginning of line */
		}
		u->setselect(q0, q0);
	}
	if(nulls)
		warning(nil, "%s: NUL bytes elided\n", file);
	return q1-q0;
}

(uint, int)
Text.bsinsert(Text *t, uint q0, Rune *r, uint n, int tofile)
{
	Rune *bp, *tp, *up;
	int i, initial;

	rescue{
		t->insert(q0, r, n, tofile);
		return (q0, n);
	}
	if(t->what == Tag)	/* can't happen but safety first: mustn't backspace over file name */
		raise;
	bp = r;
	for(i=0; i<n; i++)
		if(*bp++ == '\b'){
			--bp;
			initial = 0;
			tp = runemalloc(n);
			runemove(tp, r, i);
			up = tp+i;
			for(; i<n; i++){
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
				if(initial > q0)
					initial = q0;
				q0 -= initial;
				t->delete(q0, q0+initial, tofile);
			}
			n = up-tp;
			t->insert(q0, tp, n, tofile);
			free(tp);
			return (q0, n);
		}
	raise;
	return(0, 0);
}

void
Text.insert(Text *t, uint q0, Rune *r, uint n, int tofile)
{
	int c, i;
	Text *u;

	if(tofile && t->ncache != 0)
		error("text.insert");
	if(n == 0)
		return;
	if(tofile){
		t->file->insert(q0, r, n);
		if(t->what == Body)
			t->w->dirty = TRUE;
		if(t->file->ntext > 1)
			for(i=0; i<t->file->ntext; i++){
				u = t->file->text[i];
				if(u != t){
					u->w->dirty = TRUE;	/* always a body */
					u->insert(q0, r, n, FALSE);
					u->setselect(u->q0, u->q1);
					u->scrdraw();
				}
			}
					
	}
	if(q0 < t->q1)
		t->q1 += n;
	if(q0 < t->q0)
		t->q0 += n;
	if(q0 < t->org)
		t->org += n;
	else if(q0 <= t->org+t->nchars)
		frinsert(t, r, r+n, q0-t->org);
	if(t->w){
		c = 'i';
		if(t->what == Body)
			c = 'I';
		if(n <= EVENTSIZE)
			t->w->event("%c%d %d 0 %d %.*S\n", c, q0, q0+n, n, n, r);
		else
			t->w->event("%c%d %d 0 0 \n", c, q0, q0+n, n);
	}
}

void
Text.fill(Text *t)
{
	Rune *rp;
	int i, n, m, nl;

	if(t->lastlinefull || t->nofill)
		return;
	if(t->ncache > 0){
		if(t->w != nil)
			t->w->commit(t);
		else
			t->commit(TRUE);
	}
	rp = fbufalloc();
	do{
		n = t->file->nc-(t->org+t->nchars);
		if(n == 0)
			break;
		if(n > 2000)	/* educated guess at reasonable amount */
			n = 2000;
		t->file->read(t->org+t->nchars, rp, n);
		/*
		 * it's expensive to frinsert more than we need, so
		 * count newlines.
		 */
		nl = t->maxlines-t->nlines;
		m = 0;
		for(i=0; i<n; i){
			if(rp[i++] == '\n'){
				m++;
				if(m >= nl)
					break;
			}
		}
		frinsert(t, rp, rp+i, t->nchars);
	}while(t->lastlinefull == FALSE);
	fbuffree(rp);
}

void
Text.delete(Text *t, uint q0, uint q1, int tofile)
{
	uint n, p0, p1;
	int i, c;
	Text *u;

	if(tofile && t->ncache != 0)
		error("text.delete");
	n = q1-q0;
	if(n == 0)
		return;
	if(tofile){
		t->file->delete(q0, q1);
		if(t->what == Body)
			t->w->dirty = TRUE;
		if(t->file->ntext > 1)
			for(i=0; i<t->file->ntext; i++){
				u = t->file->text[i];
				if(u != t){
					u->w->dirty = TRUE;	/* always a body */
					u->delete(q0, q1, FALSE);
					u->setselect(u->q0, u->q1);
					u->scrdraw();
				}
			}
	}
	if(q0 < t->q0)
		t->q0 -= min(n, t->q0-q0);
	if(q0 < t->q1)
		t->q1 -= min(n, t->q1-q0);
	if(q1 <= t->org)
		t->org -= n;
	else if(q0 < t->org+t->nchars){
		if(q0 < t->org){
			t->org = q0;
			p0 = 0;
		}else
			p0 = q0 - t->org;
		p1 = q1 - t->org;
		if(p1 > t->nchars)
			p1 = t->nchars;
		frdelete(t, p0, p1);
		t->fill();
	}
	if(t->w){
		c = 'd';
		if(t->what == Body)
			c = 'D';
		t->w->event("%c%d %d 0 0 \n", c, q0, q1);
	}
}

Rune
Text.readc(Text *t, uint q)
{
	Rune r;

	if(t->cq0<=q && q<t->cq0+t->ncache)
		r = t->cache[q-t->cq0];
	else
		t->file->read(q, &r, 1);
	return r;
}

int
Text.bswidth(Text *t, Rune c)
{
	uint q, eq;
	Rune r;
	int skipping;

	/* there is known to be at least one character to erase */
	if(c == 0x08)	/* ^H: erase character */
		return 1;
	q = t->q0;
	skipping = TRUE;
	while(q > 0){
		r = t->readc(q-1);
		if(r == '\n'){		/* eat at most one more character */
			if(q == t->q0)	/* eat the newline */
				--q;
			break; 
		}
		if(c == 0x17){
			eq = isalnum(r);
			if(eq && skipping)	/* found one; stop skipping */
				skipping = FALSE;
			else if(!eq && !skipping)
				break;
		}
		--q;
	}
	return t->q0-q;
}

void
Text.type(Text *t, Rune r)
{
	uint q0, q1;
	int nnb, nb, n, i;
	Text *u;

	if(t->what!=Body && r=='\n')
		return;
	if(r == 0x80){		/* VIEW key */
		n = t->maxlines/2;
		q0 = t->org+frcharofpt(t, Pt(t->r.min.x, t->r.min.y+n*t->font->height));
		t->setorigin(q0, FALSE);
		return;
	}
	if(t->what == Body){
		seq++;
		t->file->mark();
	}
	if(t->q1 > t->q0){
		if(t->ncache != 0)
			error("text.type");
		cut(t, t, nil, TRUE, TRUE, nil, 0);
		t->eq0 = ~0;
	}
	t->show(t->q0, t->q0);
	switch(r){
	case 0x1B:
		if(t->eq0 != ~0)
			t->setselect(t->eq0, t->q0);
		if(t->ncache > 0){
			if(t->w != nil)
				t->w->commit(t);
			else
				t->commit(TRUE);
		}
		return;
	case 0x08:	/* ^H: erase character */
	case 0x15:	/* ^U: erase line */
	case 0x17:	/* ^W: erase word */
		if(t->q0 == 0)
			return;
if(1)	/* DEBUGGING */
	for(i=0; i<t->file->ntext; i++){
		u = t->file->text[i];
		if(u->cq0!=t->cq0 && (u->ncache!=t->ncache || t->ncache!=0))
			error("text.type inconsistent caches");
	}
		nnb = t->bswidth(r);
		q1 = t->q0;
		q0 = q1-nnb;
		for(i=0; i<t->file->ntext; i++){
			u = t->file->text[i];
			u->nofill = TRUE;
			nb = nnb;
			n = u->ncache;
			if(n > 0){
				if(q1 != u->cq0+n)
					error("text.type backspace");
				if(n > nb)
					n = nb;
				u->ncache -= n;
				u->delete(q1-n, q1, FALSE);
				nb -= n;
			}
			if(u->eq0==q1 || u->eq0==~0)
				u->eq0 = q0;
			if(nb && u==t)
				u->delete(q0, q0+nb, TRUE);
			if(u != t)
				u->setselect(u->q0, u->q1);
			else
				t->setselect(q0, q0);
			u->nofill = FALSE;
		}
		for(i=0; i<t->file->ntext; i++)
			t->file->text[i]->fill();
		return;
	}
	/* otherwise ordinary character; just insert, typically in caches of all texts */
if(1)	/* DEBUGGING */
	for(i=0; i<t->file->ntext; i++){
		u = t->file->text[i];
		if(u->cq0!=t->cq0 && (u->ncache!=t->ncache || t->ncache!=0))
			error("text.type inconsistent caches");
	}
	for(i=0; i<t->file->ntext; i++){
		u = t->file->text[i];
		if(u->eq0 == ~0)
			u->eq0 = t->q0;
		if(u->ncache == 0)
			u->cq0 = t->q0;
		else if(t->q0 != u->cq0+u->ncache)
			error("text.type cq1");
		u->insert(t->q0, &r, 1, FALSE);
		if(u != t)
			u->setselect(u->q0, u->q1);
		if(u->ncache == u->ncachealloc){
			u->ncachealloc += 10;
			u->cache = runerealloc(u->cache, u->ncachealloc);
		}
		u->cache[u->ncache++] = r;
	}
	t->setselect(t->q0+1, t->q0+1);
	if(r=='\n' && t->w!=nil)
		t->w->commit(t);
}

void
Text.commit(Text *t, int tofile)
{
	if(t->ncache == 0)
		return;
	if(tofile)
		t->file->insert(t->cq0, t->cache, t->ncache);
	if(t->what == Body)
		t->w->dirty = TRUE;
	t->ncache = 0;
}

intern Text	*clicktext;
intern uint	clickmsec;
intern Text	*selecttext;
intern uint	selectq;

/*
 * called from frame library
 */
void
framescroll(Frame *f, int dl)
{
	if(f != &selecttext->Frame)
		error("frameselect not right frame");
	selecttext->framescroll(dl);
}

void
Text.framescroll(Text *t, int dl)
{
	uint q0;

	if(dl == 0){
		scrsleep(100);
		return;
	}
	if(dl < 0){
		q0 = t->backnl(t->org, -dl);
		if(selectq > t->org+t->p0)
			t->setselect(t->org+t->p0, selectq);
		else
			t->setselect(selectq, t->org+t->p0);
	}else{
		if(t->org+t->nchars == t->file->nc)
			return;
		q0 = t->org+frcharofpt(t, Pt(t->r.min.x, t->r.min.y+dl*t->font->height));
		if(selectq > t->org+t->p0)
			t->setselect(t->org+t->p1, selectq);
		else
			t->setselect(selectq, t->org+t->p1);
	}
	t->setorigin(q0, TRUE);
}


void
Text.select(Text *t)
{
	uint q0, q1;
	int b, x, y;
	int state;

	selecttext = t;
	/*
	 * To have double-clicking and chording, we double-click
	 * immediately if it might make sense.
	 */
	b = mouse.buttons;
	q0 = t->q0;
	q1 = t->q1;
	selectq = t->org+frcharofpt(t, mouse.xy);
	if(clicktext==t && mouse.msec-clickmsec<500)
	if(q0==q1 && selectq==q0){
		t->doubleclick(&q0, &q1);
		t->setselect(q0, q1);
		bflush();
		x = mouse.xy.x;
		y = mouse.xy.y;
		/* stay here until something interesting happens */
		do
			frgetmouse();
		while(mouse.buttons==b && abs(mouse.xy.x-x)<3 && abs(mouse.xy.y-y)<3);
		mouse.xy.x = x;	/* in case we're calling frselect */
		mouse.xy.y = y;
		q0 = t->q0;	/* may have changed */
		q1 = t->q1;
		selectq = q0;
	}
	if(mouse.buttons == b){
		t->Frame.scroll = framescroll;
		frselect(t, frones, &mouse);
		/* horrible botch: while asleep, may have lost selection altogether */
		if(selectq > t->file->nc)
			selectq = t->org + t->p0;
		t->Frame.scroll = nil;
		if(selectq < t->org)
			q0 = selectq;
		else
			q0 = t->org + t->p0;
		if(selectq > t->org+t->nchars)
			q1 = selectq;
		else
			q1 = t->org+t->p1;
	}
	if(q0 == q1){
		if(q0==t->q0 && clicktext==t && mouse.msec-clickmsec<500){
			t->doubleclick(&q0, &q1);
			clicktext = nil;
		}else{
			clicktext = t;
			clickmsec = mouse.msec;
		}
	}else
		clicktext = nil;
	t->setselect(q0, q1);
	bflush();
	state = 0;	/* undo when possible; +1 for cut, -1 for paste */
	while(mouse.buttons){
		mouse.msec = 0;
		b = mouse.buttons;
		if(b & 6){
			if(state==0 && t->what==Body){
				seq++;
				t->w->body.file->mark();
			}
			if(b & 2){
				if(state==-1 && t->what==Body){
					t->w->undo(TRUE);
					t->setselect(q0, t->q0);
					state = 0;
				}else if(state != 1){
					cut(t, t, nil, TRUE, TRUE, nil, 0);
					state = 1;
				}
			}else{
				if(state==1 && t->what==Body){
					t->w->undo(TRUE);
					t->setselect(q0, t->q1);
					state = 0;
				}else if(state != -1){
					paste(t, t, nil, TRUE, XXX, nil, 0);
					state = -1;
				}
			}
			clearmouse();
		}
		while(mouse.buttons == b)
			frgetmouse();
		clicktext = nil;
	}
}

void
Text.show(Text *t, uint q0, uint q1)
{
	int qe;
	int nl;
	uint q;

	if(t->what != Body)
		return;
	if(t->w!=nil && t->maxlines==0)
		t->col->grow(t->w, 1);
	t->setselect(q0, q1);
	qe = t->org+t->nchars;
	if(t->org<=q0 && (q0<qe || (q0==qe && qe==t->file->nc+t->ncache)))
		t->scrdraw();
	else{
		if(t->w->nopen[QWevent]>0)
			nl = 3*t->maxlines/4;
		else
			nl = t->maxlines/4;
		q = t->backnl(q0, nl);
		/* avoid going backwards if trying to go forwards - long lines! */
		if(!(q0>t->org && q<t->org))
			t->setorigin(q, TRUE);
		while(q0 > t->org+t->nchars)
			t->setorigin(t->org+1, FALSE);
	}
}

void
Text.setselect(Text *t, uint q0, uint q1)
{
	int p0, p1;

	/* t->p0 and t->p1 are always right; t->q0 and t->q1 may be off */
	t->q0 = q0;
	t->q1 = q1;
	/* compute desired p0,p1 from q0,q1 */
	p0 = q0-t->org;
	p1 = q1-t->org;
	if(p0 < 0)
		p0 = 0;
	if(p1 < 0)
		p1 = 0;
	if(p0 > t->nchars)
		p0 = t->nchars;
	if(p1 > t->nchars)
		p1 = t->nchars;
	if(p0==t->p0 && p1==t->p1)
		return;
	/* screen disagrees with desired selection */
	frselectp(t, frones, ~D);
	t->p0 = p0;
	t->p1 = p1;
	frselectp(t, frones, ~D);
}

int
Text.select23(Text *t, uint *q0, uint *q1, Bitmap *txt, int mask)
{
	uint p0, p1;
	int buts;
	/*
	 * funny little dance to set things up right; frselect doesn't
	 * really condone how we're using it.
	 */
	p0 = t->p0;
	p1 = t->p1;
	t->p0 = 0;
	t->p1 = 0;
	frselectp(t, txt, S^D);
	frselect(t, txt, &mouse);
	buts = mouse.buttons;
	if((buts & mask) == 0){
		*q0 = t->p0+t->org;
		*q1 = t->p1+t->org;
	}
	frselectp(t, txt, S^D);
	t->p0 = p0;
	t->p1 = p1;
	while(mouse.buttons)
		frgetmouse();
	return buts;
}

(int, Text*)
Text.select2(Text *t, uint *q0, uint *q1)
{
	int w, h, buts;
	Bitmap *ul;

	/*
	 * align the texture to the text
	 */
	h = t->r.min.y+t->font->height - 2;
	h %= t->font->height;
	w = (8*sizeof(uint))>>screen.ldepth;	/* make texture one word wide */
	ul = balloc(Rect(0, 0, w, t->font->height), screen.ldepth);
	bitblt(ul, Pt(0, h), ul, Rect(0, h, w, h+1), 0xF);
	h++;
	if(h == t->font->height)
		h = 0;
	bitblt(ul, Pt(0, h), ul, Rect(0, h, w, h+1), 0xF);
	buts = t->select23(q0, q1, ul, 4);
	bfree(ul);
	if(buts & 4)
		return (0, nil);
	if(buts & 1)	/* pick up argument */
		return (1, argtext);
	return (1, nil);
}

int
Text.select3(Text *t, uint *q0, uint *q1)
{
	int w, h;
	Bitmap *ul;

	/*
	 * align the texture to the text
	 */
	h = t->r.min.y+t->font->height - 1;
	h %= t->font->height;
	w = (8*sizeof(uint))>>screen.ldepth;	/* make texture one word wide */
	ul = balloc(Rect(0, 0, w, t->font->height), screen.ldepth); 
	bitblt(ul, Pt(0, h), ul, Rect(0, h, w, h+1), 0xF);
	h+=2;
	if(h >= t->font->height)
		h -= t->font->height;
	bitblt(ul, Pt(0, h), ul, Rect(0, h, w, h+1), 0xF);
	h = (t->select23(q0, q1, ul, 1|2) == 0);
	bfree(ul);
	return h;
}

intern Rune left1[] =  { '{', '[', '(', '<', '«', 0 };
intern Rune right1[] = { '}', ']', ')', '>', '»', 0 };
intern Rune left2[] =  { '\n', 0 };
intern Rune left3[] =  { '\'', '"', '`', 0 };

Rune *left[] = {
	left1,
	left2,
	left3,
	nil
};
Rune *right[] = {
	right1,
	left2,
	left3,
	nil
};

void
Text.doubleclick(Text *t, uint *q0, uint *q1)
{
	int c, i;
	Rune *r, *l, *p;
	uint q;

	for(i=0; left[i]!=nil; i++){
		q = *q0;
		l = left[i];
		r = right[i];
		/* try matching character to left, looking right */
		if(q == 0)
			c = '\n';
		else
			c = t->readc(q-1);
		p = strrune(l, c);
		if(p != nil){
			if(t->clickmatch(c, r[p-l], 1, &q))
				*q1 = q-(c!='\n');
			return;
		}
		/* try matching character to right, looking left */
		if(q == t->file->nc)
			c = '\n';
		else
			c = t->readc(q);
		p = strrune(r, c);
		if(p != nil){
			if(t->clickmatch(c, l[p-r], -1, &q)){
				*q1 = *q0+(*q0<t->file->nc && c=='\n');
				*q0 = q;
				if(c!='\n' || q!=0 || t->readc(0)=='\n')
					(*q0)++;
			}
			return;
		}
	}
	/* try filling out word to right */
	while(*q1<t->file->nc && isalnum(t->readc(*q1)))
		(*q1)++;
	/* try filling out word to left */
	while(*q0>0 && isalnum(t->readc(*q0-1)))
		(*q0)--;
}

int
Text.clickmatch(Text *t, int cl, int cr, int dir, uint *q)
{
	Rune c;
	int nest;

	nest = 1;
	for(;;){
		if(dir > 0){
			if(*q == t->file->nc)
				break;
			c = t->readc(*q);
			(*q)++;
		}else{
			if(*q == 0)
				break;
			(*q)--;
			c = t->readc(*q);
		}
		if(c == cr){
			if(--nest==0)
				return 1;
		}else if(c == cl)
			nest++;
	}
	return cl=='\n' && nest==1;
}

uint
Text.backnl(Text *t, uint p, uint n)
{
	int i, j;

	/* look for start of this line if n==0 */
	if(n==0 && p>0 && t->readc(p-1)!='\n')
		n = 1;
	i = n;
	while(i-->0 && p>0){
		--p;	/* it's at a newline now; back over it */
		if(p == 0)
			break;
		/* at 128 chars, call it a line anyway */
		for(j=128; --j>0 && p>0; p--)
			if(t->readc(p-1)=='\n')
				break;
	}
	return p;
}

void
Text.setorigin(Text *t, uint org, int exact)
{
	int i, a;
	Rune *r;
	uint n;

	if(org>0 && !exact){
		/* org is an estimate of the char posn; find a newline */
		/* don't try harder than 256 chars */
		for(i=0; i<256 && org<t->file->nc; i++){
			if(t->readc(org) == '\n'){
				org++;
				break;
			}
			org++;
		}
	}
	a = org-t->org;
	if(a>=0 && a<t->nchars)
		frdelete(t, 0, a);
	else if(a<0 && -a<t->nchars){
		n = t->org - org;
		r = runemalloc(n);
		t->file->read(org, r, n);
		frinsert(t, r, r+n, 0);
		free(r);
	}else
		frdelete(t, 0, t->nchars);
	t->org = org;
	t->fill();
	t->scrdraw();
	t->setselect(t->q0, t->q1);
}

void
Text.reset(Text *t)
{
	t->file->seq = 0;
	t->eq0 = ~0;
	/* do t->delete(0, t->nc, TRUE) without building backup stuff */
	t->setselect(t->org, t->org);
	frdelete(t, 0, t->nchars);
	t->org = 0;
	t->q0 = 0;
	t->q1 = 0;
	t->file->reset();
	t->file->Buffer.reset();
}
