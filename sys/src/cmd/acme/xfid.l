#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

enum
{
	Ctlsize	= 5*12
};

byte	*Edel		= "deleted window";
byte	*Ebadctl		= "ill-formed control message";
byte	*Ebadaddr	= "bad address syntax";
byte	*Eaddr		= "address out of range";
byte	*Einuse		= "already in use";
byte	*Ebadevent	= "bad event syntax";

uint	*xfidtid;
int	nxfidtid;

void
Xfid.ctl(Xfid *x)
{
	x->tid = ALEF_tid();
	nxfidtid++;
	xfidtid = realloc(xfidtid, nxfidtid*sizeof(uint));
	xfidtid[nxfidtid-1] = x->tid;
	for(;;){
		(*<-x->c)(x);
		bflush();
		cxfidfree <-= x;
	}
}

void
Xfid.flush(Xfid *x)
{
	Fcall fc;
	int i, j;
	Window *w;
	Column *c;
	Xfid *wx;

	/* search windows for matching tag */
	row.lock();
	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			w = c->w[i];
			w->lock('E');
			wx = w->eventx;
			if(wx!=nil && wx->tag==x->oldtag){
				w->eventx = nil;
				wx->flushed = TRUE;
				wx->c <-= nil;
				w->unlock();
				break 2;
			}
			w->unlock();
		}
	}
	row.unlock();
	respond(x, &fc, nil);
}

void
Xfid.walk(Xfid *x)
{
	Fcall fc;
	Window *w;

	if(strcmp(x->name, "new") != 0)
		error("unknown path in walk\n");
	w = newwindow(nil);
	w->settag();
	w->inc();
	x->f->w = w;
	x->f->qid.path = QID(w->id, CHDIR|Qdir);
	fc.qid = x->f->qid;
	respond(x, &fc, nil);
}

void
Xfid.open(Xfid *x)
{
	Fcall fc;
	Window *w;
	int q;

	w = x->f->w;
	if(w){
		w->lock('E');
		q = FILE(x->f->qid);
		switch(q){
		case QWaddr:
		case QWdata:
		case QWevent:
			if(w->nopen[q]++ == 0){
				if(q == QWaddr){
					w->addr = (Range)(0,0);
					w->limit = (Range)(-1,-1);
				}
				if(q==QWevent && !w->isdir && w->col!=nil){
					w->filemenu = FALSE;
					w->settag();
				}
			}
			break;
		}
		w->unlock();
	}
	fc.qid = x->f->qid;
	x->f->open = TRUE;
	respond(x, &fc, nil);
}

void
Xfid.close(Xfid *x)
{
	Fcall fc;
	Window *w;
	int q;

	w = x->f->w;
	if(w){
		w->lock('E');
		q = FILE(x->f->qid);
		switch(q){
		case QWctl:
			if(w->ctlfid!=~0 && w->ctlfid==x->f->fid){
				w->ctlfid = ~0;
				w->ctllock.unlock();
			}
			break;
		case QWdata:
			w->nomark = FALSE;
			/* fall through */
		case QWaddr:
		case QWevent:	/* BUG: do we need to shut down Xfid? */
			if(--w->nopen[q] == 0){
				if(q == QWdata)
					w->nomark = FALSE;
				if(q==QWevent && !w->isdir && w->col!=nil){
					w->filemenu = TRUE;
					w->settag();
				}
				if(q == QWevent){
					free(w->dumpstr);
					free(w->dumpdir);
					w->dumpstr = nil;
					w->dumpdir = nil;
				}
			}
			break;
		}
		w->close();
		w->unlock();
	}
	respond(x, &fc, nil);
}

void
Xfid.read(Xfid *x)
{
	Fcall fc;
	int n, q;
	uint off;
	byte buf[128];
	Window *w;

	q = FILE(x->f->qid);
	w = x->f->w;
	if(w == nil){
		fc.count = 0;
		switch(q){
		case Qcons:
		case Qlabel:
			break;
		case Qindex:
			x->indexread();
			return;
		default:
			warning(nil, "unknown qid %d\n", q);
			break;
		}
		respond(x, &fc, nil);
		return;
	}
	w->lock('F');
	if(w->col == nil){
		w->unlock();
		respond(x, &fc, Edel);
		return;
	}
	off = x->offset;
	switch(q){
	case QWaddr:
		sprint(buf, "%11d %11d ", w->addr.q0, w->addr.q1);
		goto Readbuf;

	case QWbody:
		x->utfread(&w->body, 0, w->body.file->nc);
		break;

	case QWctl:
		w->ctlprint(buf);
		goto Readbuf;

	Readbuf:
		n = strlen(buf);
		if(off > n)
			off = n;
		if(off+x->count > n)
			x->count = n-off;
		fc.count = x->count;
		fc.data = buf+off;
		respond(x, &fc, nil);
		break;

	case QWevent:
		x->eventread(w);
		break;

	case QWdata:
		/* BUG: what should happen if q1 > q0? */
		if(w->addr.q0 > w->body.file->nc){
			respond(x, &fc, Eaddr);
			break;
		}
		w->addr.q0 += x->runeread(&w->body, w->addr.q0, w->body.file->nc);
		w->addr.q1 = w->addr.q0;
		break;

	case QWtag:
		x->utfread(&w->tag, 0, w->tag.file->nc);
		break;

	default:
		sprint(buf, "unknown qid %d in read", q);
		respond(x, &fc, nil);
	}
	w->unlock();
}

void
Xfid.write(Xfid *x)
{
	Fcall fc;
	int c, cnt, qid, q, nb, nr, eval;
	byte buf[ERRLEN];
	Window *w;
	Rune *r;
	Range a;
	Text *t;
	uint q0, tq0, tq1;
	Mntdir *md;

	qid = FILE(x->f->qid);
	w = x->f->w;
	if(w){
		c = 'F';
		if(qid==QWtag || qid==QWbody)
			c = 'E';
		w->lock(c);
		if(w->col == nil){
			w->unlock();
			respond(x, &fc, Edel);
			return;
		}
	}
	x->data[x->count] = 0;
	switch(qid){
	case Qcons:
		md = x->f->mntdir;
		warning(md, "%s", x->data);
		/* fall through */
	case Qlabel:
		fc.count = x->count;
		respond(x, &fc, nil);
		break;

	case QWaddr:
		x->data[x->count] = 0;
		(r, nr) = bytetorune(x->data);
		t = &w->body;
		w->commit(t);
		(eval, nb, a) = address(t, w->limit, w->addr, r, 0, nr, rgetc, TRUE);
		free(r);
		if(nb < nr){
			respond(x, &fc, Ebadaddr);
			break;
		}
		if(!eval){
			respond(x, &fc, Eaddr);
			break;
		}
		w->addr = a;
		fc.count = x->count;
		respond(x, &fc, nil);
		break;

	case QWbody:
		t = &w->body;
		goto BodyTag;

	case QWctl:
		x->ctlwrite(w);
		break;

	case QWdata:
		t = &w->body;
		w->commit(t);
		if(w->addr.q0>t->file->nc || w->addr.q1>t->file->nc){
			respond(x, &fc, Eaddr);
			break;
		}
		r = runemalloc(x->count);
		cvttorunes(x->data, x->count, r, &nb, &nr, nil);
		if(w->nomark == FALSE){
			seq++;
			t->file->mark();
		}
		q0 = w->addr.q0;
		if(w->addr.q1 > q0){
			t->delete(q0, w->addr.q1, TRUE);
			w->addr.q1 = q0;
		}
		tq0 = t->q0;
		tq1 = t->q1;
		t->insert(q0, r, nr, TRUE);
		if(tq0 >= q0)
			tq0 += nr;
		if(tq1 >= q0)
			tq1 += nr;
		if(!t->w->noscroll)
			t->show(tq0, tq1);
		t->scrdraw();
		w->settag();
		free(r);
		w->addr.q0 += nr;
		w->addr.q1 = w->addr.q0;
		fc.count = x->count;
		respond(x, &fc, nil);
		break;

	case QWevent:
		x->eventwrite(w);
		break;

	case QWtag:
		t = &w->tag;
		goto BodyTag;

	BodyTag:
		q = x->f->nrpart;
		cnt = x->count;
		if(q > 0){
			memmove(x->data+q, x->data, cnt);
			memmove(x->data, x->f->rpart, q);
			cnt += q;
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
		if(nr > 0){
			t->w->commit(t);
			q0 = t->file->nc;
			if(qid == QWbody){
				seq++;
				t->file->mark();
				(q0, nr) = t->bsinsert(q0, r, nr, TRUE);
				if(!t->w->noscroll)
					t->show(q0+nr, q0+nr);
				t->scrdraw();
			}else
				t->insert(q0, r, nr, TRUE);
			w->settag();
			free(r);
		}
		fc.count = x->count;
		respond(x, &fc, nil);
		break;

	default:
		sprint(buf, "unknown qid %d in write", qid);
		respond(x, &fc, buf);
		break;
	}
	if(w)
		w->unlock();
}

void
Xfid.ctlwrite(Xfid *x, Window *w)
{
	Fcall fc;
	int i, m, n, nb, nr, nulls;
	Rune *r;
	byte *err, *p, *pp, *q, *e;
	int isfbuf, scrdraw, settag;
	Text *t;

	err = nil;
	e = x->data+x->count;
	scrdraw = FALSE;
	settag = FALSE;
	isfbuf = TRUE;
	if(x->count < RBUFSIZE)
		r = fbufalloc();
	else{
		isfbuf = FALSE;
		r = malloc(x->count*UTFmax+1);
	}
	x->data[x->count] = 0;
	w->tag.commit(TRUE);
	for(n=0; n<x->count; n+=m){
		p = x->data+n;
		if(strncmp(p, "lock", 4) == 0){	/* make window exclusive use */
			w->ctllock.lock();
			w->ctlfid = x->f->fid;
			m = 4;
		}else
		if(strncmp(p, "unlock", 6) == 0){	/* release exclusive use */
			w->ctlfid = ~0;
			w->ctllock.unlock();
			m = 6;
		}else
		if(strncmp(p, "clean", 5) == 0){	/* mark window 'clean', seq=0 */
			t = &w->body;
			t->eq0 = ~0;
			t->file->reset();
			t->file->mod = FALSE;
			w->dirty = FALSE;
			settag = TRUE;
			m = 5;
		}else
		if(strncmp(p, "show", 4) == 0){	/* show dot */
			t = &w->body;
			t->show(t->q0, t->q1);
			m = 4;
		}else
		if(strncmp(p, "name ", 5) == 0){	/* set file name */
			pp = p+5;
			m = 5;
			q = memchr(pp, '\n', e-pp);
			if(q==nil || q==pp){
				err = Ebadctl;
				break;
			}
			*q = 0;
			nulls = FALSE;
			cvttorunes(pp, q-pp, r, &nb, &nr, &nulls);
			if(nulls){
				err = "nulls in file name";
				break;
			}
			for(i=0; i<nr; i++)
				if(r[i] <= ' '){
					err = "bad character in file name";
					break 2;
				}
			seq++;
			w->body.file->mark();
			w->setname(r, nr);
			m += (q+1) - pp;
		}else
		if(strncmp(p, "dump ", 5) == 0){	/* set dump string */
			pp = p+5;
			m = 5;
			q = memchr(pp, '\n', e-pp);
			if(q==nil || q==pp){
				err = Ebadctl;
				break;
			}
			*q = 0;
			nulls = FALSE;
			cvttorunes(pp, q-pp, r, &nb, &nr, &nulls);
			if(nulls){
				err = "nulls in dump string";
				break;
			}
			w->dumpstr = runetobyte(r, nr);
			m += (q+1) - pp;
		}else
		if(strncmp(p, "dumpdir ", 8) == 0){	/* set dump directory */
			pp = p+8;
			m = 8;
			q = memchr(pp, '\n', e-pp);
			if(q==nil || q==pp){
				err = Ebadctl;
				break;
			}
			*q = 0;
			nulls = FALSE;
			cvttorunes(pp, q-pp, r, &nb, &nr, &nulls);
			if(nulls){
				err = "nulls in dump directory string";
				break;
			}
			w->dumpdir = runetobyte(r, nr);
			m += (q+1) - pp;
		}else
		if(strncmp(p, "delete", 6) == 0){	/* delete for sure */
			w->col->close(w, TRUE);
			m = 6;
		}else
		if(strncmp(p, "del", 3) == 0){	/* delete, but check dirty */
			if(!w->clean(TRUE)){
				err = "file dirty";
				break;
			}
			w->col->close(w, TRUE);
			m = 3;
		}else
		if(strncmp(p, "get", 3) == 0){	/* get file */
			get(&w->body, nil, nil, FALSE, XXX, nil, 0);
			m = 3;
		}else
		if(strncmp(p, "put", 3) == 0){	/* put file */
			put(&w->body, nil, nil, XXX, XXX, nil, 0);
			m = 3;
		}else
		if(strncmp(p, "dot=addr", 8) == 0){	/* set dot */
			w->body.q0 = max(0, w->addr.q0);
			w->body.q1 = min(w->body.file->nc, w->addr.q1);
			w->body.setselect(w->body.q0, w->body.q1);
			settag = TRUE;
			m = 8;
		}else
		if(strncmp(p, "addr=dot", 8) == 0){	/* set addr */
			w->addr.q0 = w->body.q0;
			w->addr.q1 = w->body.q1;
			m = 8;
		}else
		if(strncmp(p, "limit=addr", 10) == 0){	/* set limit */
			w->limit.q0 = max(0, w->addr.q0);
			w->limit.q1 = min(w->body.file->nc, w->addr.q1);
			m = 10;
		}else
		if(strncmp(p, "nomark", 6) == 0){	/* turn off automatic marking */
			w->nomark = TRUE;
			m = 6;
		}else
		if(strncmp(p, "mark", 4) == 0){	/* mark file */
			seq++;
			w->body.file->mark();
			settag = TRUE;
			m = 4;
		}else
		if(strncmp(p, "noscroll", 8) == 0){	/* turn off automatic scrolling */
			w->noscroll = TRUE;
			m = 8;
		}else
		if(strncmp(p, "cleartag", 8) == 0){	/* wipe tag right of bar */
			w->cleartag();
			settag = TRUE;
			m = 8;
		}else
		if(strncmp(p, "scroll", 6) == 0){	/* turn on automatic scrolling (writes to body only) */
			w->noscroll = FALSE;
			m = 6;
		}else{
			err = Ebadctl;
			break;
		}
		while(p[m] == '\n')
			m++;
	}

	if(isfbuf)
		fbuffree(r);
	else
		free(r);
	if(err)
		n = 0;
	fc.count = n;
	respond(x, &fc, err);
	if(settag)
		w->settag();
	if(scrdraw)
		w->body.scrdraw();
}

void
Xfid.eventwrite(Xfid *x, Window *w)
{
	Fcall fc;
	int m, n;
	Rune *r;
	byte *err, *p, *q;
	int isfbuf;
	Text *t;
	int c;
	uint q0, q1;

	rescue{
		err = Ebadevent;
		goto Out;
	}

	err = nil;
	isfbuf = TRUE;
	if(x->count < RBUFSIZE)
		r = fbufalloc();
	else{
		isfbuf = FALSE;
		r = malloc(x->count*UTFmax+1);
	}
	for(n=0; n<x->count; n+=m){
		p = x->data+n;
		w->owner = *p++;	/* disgusting */
		c = *p++;
		while(*p == ' ')
			p++;
		q0 = strtoui(p, &q, 10);
		if(q == p)
			raise;
		p = q;
		while(*p == ' ')
			p++;
		q1 = strtoui(p, &q, 10);
		if(q == p)
			raise;
		p = q;
		while(*p == ' ')
			p++;
		if(*p++ != '\n')
			raise;
		m = p-(x->data+n);
		if('a'<=c && c<='z')
			t = &w->tag;
		else if('A'<=c && c<='Z')
			t = &w->body;
		else
			raise;
		if(q0>t->file->nc || q1>t->file->nc || q0>q1)
			raise;

		switch(c){
		case 'x':
		case 'X':
			execute(t, q0, q1, TRUE, nil);
			break;
		case 'l':
		case 'L':
			look3(t, q0, q1, TRUE);
			break;
		default:
			raise;
		}

	}

    Out:
	if(isfbuf)
		fbuffree(r);
	else
		free(r);
	if(err)
		n = 0;
	fc.count = n;
	respond(x, &fc, err);
}

void
Xfid.utfread(Xfid *x, Text *t, uint q0, uint q1)
{
	Fcall fc;
	Window *w;
	Rune *r;
	byte *b, *b1;
	uint q, off, boff;
	int m, n, nr, nb;

	w = t->w;
	w->commit(t);
	off = x->offset;
	r = fbufalloc();
	b = fbufalloc();
	b1 = fbufalloc();
	n = 0;
	/* BUG: stupid code: scan from beginning */
	q = q0;
	boff = 0;
	while(q<q1 && n<x->count){
		nr = q1-q;
		if(nr > BUFSIZE/UTFmax)
			nr = BUFSIZE/UTFmax;
		t->file->read(q, r, nr);
		nb = snprint(b, BUFSIZE+1, "%.*S", nr, r);
		if(boff >= off){
			m = nb;
			if(boff+m > off+x->count)
				m = off+x->count - boff;
			memmove(b1+n, b, m);
			n += m;
		}else if(boff+nb > off){
			if(n != 0)
				error("bad count in utfrune");
			m = nb - (off-boff);
			if(m > x->count)
				m = x->count;
			memmove(b1, b+(off-boff), m);
			n += m;
		}
		boff += nb;
		q += nr;
	}
	fbuffree(r);
	fbuffree(b);
	fc.count = n;
	fc.data = b1;
	respond(x, &fc, nil);
	fbuffree(b1);
}

int
Xfid.runeread(Xfid *x, Text *t, uint q0, uint q1)
{
	Fcall fc;
	Window *w;
	Rune *r, junk;
	byte *b, *b1;
	uint q, boff;
	int i, rw, m, n, nr, nb;

	w = t->w;
	w->commit(t);
	r = fbufalloc();
	b = fbufalloc();
	b1 = fbufalloc();
	n = 0;
	q = q0;
	boff = 0;
	while(q<q1 && n<x->count){
		nr = q1-q;
		if(nr > BUFSIZE/UTFmax)
			nr = BUFSIZE/UTFmax;
		t->file->read(q, r, nr);
		nb = snprint(b, BUFSIZE+1, "%.*S", nr, r);
		m = nb;
		if(boff+m > x->count){
			i = x->count - boff;
			/* copy whole runes only */
			m = 0;
			nr = 0;
			while(m < i){
				rw = chartorune(&junk, b+m);
				if(m+rw > i)
					break;
				m += rw;
				nr++;
			}
			if(m == 0)
				break;
		}
		memmove(b1+n, b, m);
		n += m;
		boff += nb;
		q += nr;
	}
	fbuffree(r);
	fbuffree(b);
	fc.count = n;
	fc.data = b1;
	respond(x, &fc, nil);
	fbuffree(b1);
	return q-q0;
}

void
Xfid.eventread(Xfid *x, Window *w)
{
	Fcall fc;
	byte *b;
	int i, n;

	i = 0;
	x->flushed = FALSE;
	while(w->nevents == 0){
		if(i){
			if(!x->flushed)
				respond(x, &fc, "window shut down");
			return;
		}
		w->eventx = x;
		w->unlock();
		<- x->c;
		w->lock('F');
		i++;
	}

	n = w->nevents;
	if(n > x->count)
		n = x->count;
	fc.count = n;
	fc.data = w->events;
	respond(x, &fc, nil);
	b = w->events;
	w->events = strdup(w->events+n);
	free(b);
	w->nevents -= n;
}

void
Xfid.indexread(Xfid *x)
{
	Fcall fc;
	int i, j, m, n, nmax, isbuf, cnt, off;
	Window *w;
	byte *b;
	Rune *r;
	Column *c;

	row.lock();
	nmax = 0;
	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			w = c->w[i];
			nmax += Ctlsize + w->tag.file->nc*UTFmax + 1;
		}
	}
	nmax++;
	isbuf = (nmax<=RBUFSIZE);
	if(isbuf)
		b = x->buf;
	else
		b = malloc(nmax);
	r = fbufalloc();
	n = 0;
	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			w = c->w[i];
			/* only show the currently active window of a set */
			if(w->body.file->curtext != &w->body)
				continue;
			w->ctlprint(b+n);
			n += Ctlsize;
			m = min(RBUFSIZE, w->tag.file->nc);
			w->tag.file->read(0, r, m);
			m = n + snprint(b+n, nmax-n-1, "%.*S", m, r);
			while(n<m && b[n]!='\n')
				n++;
			b[n++] = '\n';
		}
	}
	row.unlock();
	off = x->offset;
	cnt = x->count;
	if(off > n)
		off = n;
	if(off+cnt > n)
		cnt = n-off;
	fc.count = cnt;
	memmove(r, b+off, cnt);
	fc.data = (byte*)r;
	if(!isbuf)
		free(b);
	respond(x, &fc, nil);
	fbuffree(r);
}
