#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

typedef struct BMW BMW;
typedef struct SFW SFW;
typedef struct FTW FTW;

struct BMW
{
	Bitmap;
	Window	*w;
};

struct SFW
{
	Subfont;
	int	ref;
	ulong	qid[2];
};

struct FTW
{
	int	id;
	int	height;
	int	nwidth;
	int	sized;
	uchar	*width;
	Window	*w;
};

char	*bitbuf;
int	nbitbuf;
int	nbmw;
BMW	*bmw;
int	nsfw;
SFW	*sfw;
int	nftw;
FTW	*ftw;

void
bitbufm(int cnt)
{
	if(nbitbuf<cnt){
		nbitbuf = cnt;
		bitbuf = erealloc(bitbuf, cnt);
	}
}

int
refsubf(Window *w, int id, int ref)
{
	SFW *s;
	uchar *p;
	int i;

	if(id >= w->nsubf){
		w->subf = erealloc(w->subf, (id+10)*sizeof w->subf[0]);
		memset(w->subf+w->nsubf, 0, (id+10-w->nsubf)*sizeof w->subf[0]);
		w->nsubf = id+10;
	}
	w->subf[id] += ref;
	if(id >= nsfw){
		sfw = erealloc(sfw, (id+10)*sizeof sfw[0]);
		memset(sfw+nsfw, 0, (id+10-nsfw)*sizeof sfw[0]);
		for(i=nsfw; i<id+10; i++)
			sfw[i].qid[0] = ~0;
		nsfw = id+10;
	}
	s = &sfw[id];
	s->ref += ref;
	if(id>0 && ref && s->ref==0){
		/* tough call: hold or release? memory argument says release */
		free(s->info);
		s->info = 0;
		s->qid[0] = ~0;
		p = bneed(3);
		p[0] = 'g';
		BPSHORT(p+1, id);
	}
	return s->ref;
}

int
subfindex(Window *w, int id)
{
	int ref;

	if(id<0 || id>=nsfw || id>=w->nsubf)
		return -1;
	ref = w->subf[id];
	if(ref<0 || ref>sfw[id].ref)
		error("subfindex ref count");
	if(ref == 0)
		return -1;
	return id;
}

void
bitopen(Window *w)
{
	w->bitopen = 1;
	w->reshaped = 0;
	w->bitinit = 0;
	refsubf(w, 0, 1);	/* 0th subfont is open */
}

int
bitpacksubf(uchar *p, Subfont *sf)
{
	int j;
	Fontchar *i;

	sprint((char*)p, "%11d %11d %11d ", sf->n, sf->height, sf->ascent);
	p += 3*12;
	for(i=sf->info,j=0; j<=sf->n; j++,i++,p+=6){
		BPSHORT(p, i->x);
		p[2] = i->top;
		p[3] = i->bottom;
		p[4] = i->left;
		p[5] = i->width;
	}
	return 3*12 + (sf->n+1)*6;
}

void
bitunpacksubf(Subfont *sf, uchar *p)
{
	int j;
	Fontchar *i;

	for(i=sf->info,j=0; j<=sf->n; j++,i++,p+=6){
		i->x = BGSHORT(p);
		i->top = p[2];
		i->bottom = p[3];
		i->left = p[4];
		i->width = p[5];
	}
}

int
bitread(Window *w, int cnt, int *err)
{
	uchar *p;
	SFW *sf;
	int j;

	bitbufm(cnt);
	p = (uchar*)bitbuf;
	if(w->bitinit){
		/*
		 * init:
		 *	'I'		1
		 *	ldepth		1
		 * 	rectangle	16
		 * 	clip rectangle	16
		 *	subfont info	3*12
		 *	fontchars	6*(defont->n+1)
		 */
		if(cnt < 34){
	ReturnEshort:
			*err = Eshort;
			return 0;
		}
		p[0] = 'I';
		p[1] = screen.ldepth;
		BPLONG(p+2, w->l->r.min.x);
		BPLONG(p+6, w->l->r.min.y);
		BPLONG(p+10, w->l->r.max.x);
		BPLONG(p+14, w->l->r.max.y);
		BPLONG(p+18, w->l->clipr.min.x);
		BPLONG(p+22, w->l->clipr.min.y);
		BPLONG(p+26, w->l->clipr.max.x);
		BPLONG(p+30, w->l->clipr.max.y);
		if(cnt >= 34+3*12+6*(subfont->n+1))
			cnt = 34+bitpacksubf(p+34, subfont);
		else
			cnt = 34;
		w->bitinit = 0;
		return cnt;
	}
	if(w->bitid > 0){
		/*
		 * allocate:
		 *	'A'		1
		 *	bitmap id	2
		 */
		if(cnt < 3)
			goto ReturnEshort;
		p[0] = 'A';
		BPSHORT(p+1, w->bitid);
		w->bitid = -1;
		return 3;
	}
	if(w->fontid > 0){
		/*
		 * font allocate:
		 *	'N'		1
		 *	bitmap id	2
		 */
		if(cnt < 3)
			goto ReturnEshort;
		p[0] = 'N';
		BPSHORT(p+1, w->fontid);
		w->fontid = -1;
		return 3;
	}
	if(w->subfontid > 0){
		/*
		 * subfont allocate:
		 *	'K'		1
		 *	bitmap id	2
		 */
		if(cnt < 3)
			goto ReturnEshort;
		p[0] = 'K';
		BPSHORT(p+1, w->subfontid);
		w->subfontid = -1;
		return 3;
	}
	if(w->cachesfid > 0){
		/*
		 * cached subfont:
		 *	'J'		1
		 *	subfont id	2
		 *	font info	3*12
		 *	fontchars	6*(subfont->n+1)
		 */
		j = subfindex(w, w->cachesfid);
		if(j < 0){
			*err = Efont;
			return 0;
		}
		sf = &sfw[j];
		if(cnt < 3+3*12+6*(sf->n+1))
			goto ReturnEshort;
		p[0] = 'J';
		BPSHORT(p+1, j);
		w->cachesfid = -1;
		return 3+bitpacksubf(p+3, sf);
	}
	if(w->rdbmbuf){
		/* read:
		 *	data		bytewidth*(maxy-miny)
		 */
		if(cnt < w->rdbml)
			goto ReturnEshort;
		memmove(bitbuf, w->rdbmbuf, w->rdbml);
		free(w->rdbmbuf);
		w->rdbmbuf = 0;
		return w->rdbml;
	}
	return 0;
}

int
bitwrite(Window *w, char *buf, int n, int *err)
{
	uchar *p;
	schar *sp;
	uchar x[32];
	int i, m, id, did, sid;
	long miny, maxy, ws, l, t, nc;
	long v;
	ulong q0, q1;
	Bitmap *s, *d;
	FTW *ft;
	SFW *sf;
	BMW *b;
	Rectangle r;
	Point pt, pt1, pt2;
	Layer *dl;

	m = n;
	while(m > 0)
		switch(buf[0]){
		default:
	ReturnEbit:
			*err = Ebit;
			goto Return0;

		case 'a':
			/*
			 * allocate:
			 *	'a'		1
			 *	ldepth		1
			 *	Rectangle	16
			 * next read returns allocated bitmap id
			 */
			if(m < 18)
				goto ReturnEbit;
			if(!bwrite()){	/* gotta tell alloc fail from other problems */
	ReturnEsbit:
				*err = Esbit;
				goto Return0;
			}
			p = bneed(18);
			memmove(p, buf, 18);
			m -= 18;
			v = *(p+1);
			r.min.x = BGLONG(p+2);
			r.min.y = BGLONG(p+6);
			r.max.x = BGLONG(p+10);
			r.max.y = BGLONG(p+14);
			if(!bwrite()){
	ReturnEballoc:
				*err = Eballoc;
				return 0;
			}
			if(read(bitbltfd, x, 3)!=3 || x[0]!='A')
				goto ReturnEsbit;
			id = x[1] | (x[2]<<8);
			if(id >= nbmw){
				bmw = erealloc(bmw, (id+10)*sizeof bmw[0]);
				memset(bmw+nbmw, 0, (id+10-nbmw)*sizeof bmw[0]);
				nbmw = id+10;
			}
			b = &bmw[id];
			if(b->w)
				error("reallocated bitmap");
			b->w = w;
			b->r = r;
			b->clipr = r;
			b->ldepth = v;
			b->id = id;
			b->cache = 0;
			w->bitid = id;
			buf += 18;
			break;

		case 'b':
			/*
			 * bitblt
			 *	'b'		1
			 *	dst id		2
			 *	dst Point	8
			 *	src id		2
			 *	src Rectangle	16
			 *	code		2
			 */
			if(m < 31)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0){
	ReturnEbitmap:
				*err = Ebitmap;
				goto Return0;
			}
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			sid = BGSHORT(p+11);
			if(sid < 0)
				goto ReturnEbitmap;
			if(sid>0 && (sid>=nbmw || bmw[sid].w!=w))
				goto ReturnEbitmap;
			if(sid==0 || did==0){
				pt.x = BGLONG(p+3);
				pt.y = BGLONG(p+7);
				r.min.x = BGLONG(p+13);
				r.min.y = BGLONG(p+17);
				r.max.x = BGLONG(p+21);
				r.max.y = BGLONG(p+25);
				v = BGSHORT(p+29);
				if(sid==0)
					s = w->l;
				else
					s = &bmw[sid];
				if(did==0)
					d = w->l;
				else
					d = &bmw[did];
				bitblt(d, pt, s, r, v);
			}else{
				p = bneed(31);
				memmove(p, buf, 31);
			}
			m -= 31;
			buf += 31;
			break;

		case 'c':
			/*
			 * cursorswitch
			 *	'c'		1
			 * nothing more: return to arrow; else
			 * 	Point		8
			 *	clr		32
			 *	set		32
			 */
			if(!bwrite())
				goto ReturnEsbit;
			if(m == 1){
				w->cursorp = 0;
				m -= 1;
				buf += 1;
			}else{
				if(m < 73)
					goto ReturnEbit;
				p = (uchar*) buf;
				w->cursor.offset.x = BGLONG(p+1);
				w->cursor.offset.y = BGLONG(p+5);
				memmove(w->cursor.clr, p+9, 2*16);
				memmove(w->cursor.set, p+41, 2*16);
				w->cursorp = &w->cursor;
				m -= 73;
				buf += 73;
			}
			checkcursor(1);
			break;

		case 'e':
			/*
			 * polysegment
			 *
			 *	'e'		1
			 *	id		2
			 *	pt		8
			 *	value		1
			 *	code		2
			 *	n		2
			 *	pts		2*n
			 */
			if(m < 16)
				goto ReturnEbit;
			p = (uchar*)buf;
			l = BGSHORT(p+14);
			if(m < 16+2*l)
				goto ReturnEbit;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbitmap;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			if(did == 0){
				Point *q;
				q = emalloc((l+1)*sizeof(Point));
				pt1.x = BGLONG(p+3);
				pt1.y = BGLONG(p+7);
				q[0] = pt1;
				sp = (schar*)(p+16);
				for(i=1; i<=l; i++){
					pt1.x += sp[0];
					pt1.y += sp[1];
					q[i] = pt1;
					sp += 2;
				}
				t = p[11];
				v = BGSHORT(p+12);
				polysegment(w->l, l+1, q, t, v);
				free(q);
			}else{
				p = bneed(16+2*l);
				memmove(p, buf, 16+2*l);
			}
			l = 16+2*l;
			buf += l;
			m -= l;
			break;

		case 'f':
			/*
			 * free
			 *	'f'		1
			 *	id		2
			 */
			if(m < 3)
				goto ReturnEbit;
			did = BGSHORT((uchar*)buf+1);
			if(did<=0 || nbmw<=did)
				goto ReturnEbitmap;
			b = &bmw[did];
			if(b->w != w)
				goto ReturnEbitmap;
			b->w = 0;
			p = bneed(3);
			memmove(p, buf, 3);
			m -= 3;
			buf += 3;
			break;

		case 'g':
			/*
			 * free subfont
			 *	'g'		1
			 *	id		2
			 */
			if(m < 3)
				goto ReturnEbit;
			did = subfindex(w, BGSHORT((uchar*)buf+1));
			if(did < 0){
	ReturnEsubfont:
				*err = Esubfont;
				goto Return0;
			}
			refsubf(w, did, -1);
			m -= 3;
			buf += 3;
			break;

		case 'h':
			/*
			 * free font
			 *	'h'		1
			 *	id		2
			 */
			if(m < 3)
				goto ReturnEbit;
			did = BGSHORT((uchar*)buf+1);
			if(did<=0 || nftw<=did){
	ReturnEfont:
				*err = Efont;
				goto Return0;
			}
			ft = &ftw[did];
			if(ft->w != w)
				goto ReturnEsubfont;
			free(ft->width);
			ft->nwidth = 0;
			ft->width = 0;
			ft->sized = 0;
			ft->w = 0;
			p = bneed(3);
			memmove(p, buf, 3);
			m -= 3;
			buf += 3;
			break;

		case 'i':
			/*
			 * init
			 *	'i'		1
			 */
			w->bitinit = 1;
			m -= 1;
			buf += 1;
			break;

		case 'j':
			/*
			 * subfont cache lookup
			 *	'j'		1
			 *	qid		8
			 */
			if(m < 9)
				goto ReturnEbit;
			p = (uchar*)buf;
			q0 = BGLONG(p+1);
			q1 = BGLONG(p+5);
			if(q0 == ~0){
		ReturnEsfnc:
				*err = Esfnotcached;
				goto Return0;
			}
			sf = &sfw[0];
			for(id=0; id<nsfw; id++,sf++)
				if(sf->qid[0]==q0 && sf->qid[1]==q1)
					goto Cachefound;
			if(!bwrite())
				goto ReturnEsbit;
			p = bneed(9);
			memmove(p, buf, m);
			if(!bwrite())
				goto ReturnEsfnc;
			bitbufm(1+2+3*12+6*1000);
			p = (uchar*)bitbuf;
			l = read(bitbltfd, p, 1+2+3*12+6*1000);
			if(l < 2+3*12)
				goto ReturnEsbit;
			if(p[0] != 'J')
				goto ReturnEsbit;
			id = BGSHORT(p+1);
			p += 3;
			if(refsubf(w, id, 1) == 1){
				sf = &sfw[id];
				sf->n = atoi((char*)p+0);
				sf->height = atoi((char*)p+12);
				sf->ascent = atoi((char*)p+24);
				sf->qid[0] = q0;
				sf->qid[1] = q1;
				sf->id = id;
				sf->info = emalloc(sizeof(Fontchar)*(sf->n+1));
				bitunpacksubf(sf, p+36);
			}else if(sfw[id].qid[0] != q0)
				error("subfont cache mismatch");
			w->cachesfid = id;
			m -= 9;
			buf += 9;
			break;

		Cachefound:
			w->cachesfid = id;
			refsubf(w, id, 1);
			m -= 9;
			buf += 9;
			break;

		case 'k':
			/*
			 * allocate subfont
			 *	'k'		1
			 *	n		2
			 *	height		1
			 *	ascent		1
			 *	bitmap id	2
			 *	qid		8
			 *	fontchars	6*n
			 * next read returns allocated subfont id
			 */
			if(m < 15)
				goto ReturnEbit;
			p = (uchar*)buf;
			v = BGSHORT(p+1);
			nc = 15+6*(v+1);
			if(m < nc)
				goto ReturnEbit;
			l = p[3];
			t = p[4];
			sid = BGSHORT(p+5);
			if(sid < 0)
				goto ReturnEbitmap;
			if(sid>0 && (sid>=nbmw || bmw[sid].w!=w))
				goto ReturnEbitmap;
			p = bneed(nc);
			memmove(p, buf, nc);
			m -= nc;
			if(!bwrite())
				goto ReturnEsbit;
			if(read(bitbltfd, x, 3)!=3 || x[0]!='K')
				goto ReturnEsbit;
			/* bitmap is now part of font; release locally */
			bmw[sid].w = 0;
			id = x[1] | (x[2]<<8);
			q0 = BGLONG(p+7);
			q1 = BGLONG(p+11);
			if(refsubf(w, id, 1) == 1){
				sf = &sfw[id];
				sf->n = v;
				sf->height = l;
				sf->ascent = t;
				sf->qid[0] = q0;
				sf->qid[1] = q1;
				sf->id = id;
				sf->info = emalloc(sizeof(Fontchar)*(v+1));
				bitunpacksubf(sf, (uchar*)buf+15);
				buf += nc;
			}else if(sfw[id].qid[0] != q0)
				error("subfont cache mismatch");
			w->subfontid = id;
			break;

		case 'l':
			/*
			 * line segment
			 *
			 *	'l'		1
			 *	id		2
			 *	pt1		8
			 *	pt2		8
			 *	value		1
			 *	code		2
			 */
			if(m < 22)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbitmap;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			if(did == 0){
				pt1.x = BGLONG(p+3);
				pt1.y = BGLONG(p+7);
				pt2.x = BGLONG(p+11);
				pt2.y = BGLONG(p+15);
				t = p[19];
				v = BGSHORT(p+20);
				segment(w->l, pt1, pt2, t, v);
			}else{
				p = bneed(22);
				memmove(p, buf, 22);
			}
			m -= 22;
			buf += 22;
			break;

		case 'm':
			/*
			 * read colormap
			 *
			 *	'm'		1
			 *	id		2
			 */
			if(m < 3)
				goto ReturnEbit;
			p = (uchar*)buf;
			sid = BGSHORT(p+1);
			if(sid < 0)
				goto ReturnEbitmap;
			if(sid>0 && (sid>=nbmw || bmw[sid].w!=w))
				goto ReturnEbitmap;
			if(sid == 0)
				t = screen.ldepth;
			else
				t = bmw[sid].ldepth;
			t = 12*(1 << (1 << t));
			if(!bwrite())
				goto ReturnEsbit;
			p = bneed(3);
			memmove(p, buf, 3);
			m -= 3;
			buf += 3;
			if(!bwrite())
				goto ReturnEsbit;
			w->rdbmbuf = emalloc(t);
			w->rdbml = t;
			if(read(bitbltfd, w->rdbmbuf, t) != t)
				goto ReturnEsbit;
			break;

		case 'n':
			/*
			 * allocate font
			 *	'n'		1
			 *	height		1
			 *	ascent		1
			 *	ldepth		2
			 *	ncache		2
			 * next read returns allocated font id
			 */
			if(m < 7)
				goto ReturnEbit;
			p = bneed(7);
			memmove(p, buf, 7);
			if(!bwrite())
				goto ReturnEsbit;
			if(read(bitbltfd, x, 3)!=3 || x[0]!='N')
				goto ReturnEsbit;
			id = x[1] | (x[2]<<8);
			if(id >= nftw){
				ftw = erealloc(ftw, (id+10)*sizeof ftw[0]);
				memset(ftw+nftw, 0, (id+10-nftw)*sizeof ftw[0]);
				nftw = id+10;
			}
			ft = &ftw[id];
			if(ft->w)
				error("reallocated font");
			ft->nwidth = BGSHORT((uchar*)buf+5);
			if(ft->nwidth > MAXFCACHE)
				goto ReturnEsbit;
			ft->w = w;
			ft->height = buf[1];
			ft->width = emalloc(ft->nwidth*sizeof(ft->width[0]));
			memset(ft->width, 0, ft->nwidth*sizeof(ft->width[0]));
			ft->id = id;
			w->fontid = id;
			buf += 7;
			m -= 7;
			break;

		case 'p':
			/*
			 * point
			 *
			 *	'p'		1
			 *	id		2
			 *	pt		8
			 *	value		1
			 *	code		2
			 */
			if(m < 14)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbitmap;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			if(did == 0){
				pt1.x = BGLONG(p+3);
				pt1.y = BGLONG(p+7);
				t = p[11];
				v = BGSHORT(p+12);
				point(w->l, pt1, t, v);
			}else{
				p = bneed(14);
				memmove(p, buf, 14);
			}
			m -= 14;
			buf += 14;
			break;

		case 'q':
			/*
			 * set clip rectangle
			 *	'q'		1
			 *	dst id		2
			 *	rect		16
			 */
			if(m < 19)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbitmap;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			r.min.x = BGLONG(p+3);
			r.min.y = BGLONG(p+7);
			r.max.x = BGLONG(p+11);
			r.max.y = BGLONG(p+15);
			if(did == 0)
				d = w->l;
			else
				d = &bmw[did];
			if(rectclip(&r, d->r)){
				d->clipr = r;
				if(did){
					p = bneed(19);
					memmove(p, buf, 19);
				}
			}
			m -= 19;
			buf += 19;
			break;

		case 'r':
			/*
			 * read
			 *	'r'		1
			 *	dst id		2
			 *	miny		4
			 *	maxy		4
			 */
			if(m < 11)
				goto ReturnEbit;
			p = (uchar*)buf;
			sid = BGSHORT(p+1);
			if(sid < 0)
				goto ReturnEbitmap;
			if(sid>0 && (sid>=nbmw || bmw[sid].w!=w))
				goto ReturnEbitmap;
			if(sid == 0) /* use cache */
				s = w->l->cache;
			else
				s = &bmw[sid];
			miny = BGLONG(p+3);
			maxy = BGLONG(p+7);
			if(miny>maxy || miny<s->r.min.y || maxy>s->r.max.y)
				goto ReturnEbit;
			ws = 1<<(3-s->ldepth);	/* pixels per byte */
			/* set l to number of bytes of incoming data per scan line */
			if(s->r.min.x >= 0)
				l = (s->r.max.x+ws-1)/ws - s->r.min.x/ws;
			else{	/* make positive before divide */
				t = (-s->r.min.x)+ws-1;
				t = (t/ws)*ws;
				l = (t+s->r.max.x+ws-1)/ws;
			}
			if(sid == 0 && w->l->vis == Visible){
				/* update cache for given area */
				r = s->r;
				r.min.y = miny;
				r.max.y = maxy;
				bitblt(s, r.min, w->l->cover->layer, r, S);
			}
			if(!bwrite())
				goto ReturnEsbit;
			p = bneed(11);
			memmove(p, buf, 11);
			BPSHORT(p+1, s->id);
			m -= 11;
			buf += 11;
			if(!bwrite())
				goto ReturnEsbit;
			l *= maxy-miny;
			w->rdbmbuf = emalloc(l);
			w->rdbml = l;
			if(read(bitbltfd, w->rdbmbuf, l) != l)
				goto ReturnEsbit;
			break;

		case 's':
			/*
			 * string
			 *
			 *	's'		1
			 *	id		2
			 *	pt		8
			 *	font id		2
			 *	code		2
			 *	n		2
			 * 	cache indexes	2*n (not null terminated)
			 */
			if(m < 17)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbitmap;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			v = BGSHORT(p+11);
			ft = &ftw[v];
			if(v>0 && (v>=nftw || ftw[v].w!=w))
				goto ReturnEfont;
			t = BGSHORT(p+15);
			l = 17+2*t;
			if(l<0 || m<l)
				goto ReturnEbit;
			if(did == 0)
				lcstring(w->l, ft->height, ft->nwidth, ft->width, p, t);
			else{
				p = bneed(l);
				memmove(p, buf, l);
			}
			m -= l;
			buf += l;
			break;

		case 't':
			/*
			 * texture
			 *	't'		1
			 *	dst id		2
			 *	rect		16
			 *	src id		2
			 *	fcode		2
			 */
			if(m < 23)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbitmap;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbitmap;
			if(did == 0){
				r.min.x = BGLONG(p+3);
				r.min.y = BGLONG(p+7);
				r.max.x = BGLONG(p+11);
				r.max.y = BGLONG(p+15);
				sid = BGSHORT(p+19);
				if(sid <= 0)
					goto ReturnEbitmap;
				if(sid>0 && (sid>=nbmw || bmw[sid].w!=w))
					goto ReturnEbitmap;
				s = &bmw[sid];
				v = BGSHORT(p+21);
				texture(w->l, r, s, v);
			}else{
				p = bneed(23);
				memmove(p, buf, 23);
			}
			m -= 23;
			buf += 23;
			break;

		case 'v':
			/*
			 * clear font cache and bitmap.
			 * must leave font unchanged if error.
			 *
			 *	'v'		1
			 *	id		2
			 *	ncache		2
			 *	width		2
			 */
			if(m < 7)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			l = BGSHORT(p+3);
			ft = &ftw[did];
			if(did<0 || did>=nftw || ft->w!=w)
				return Efont;
			if(!bwrite())
				goto ReturnEsbit;
			p = bneed(7);
			memmove(p, buf, 7);
			if(!bwrite())
				goto ReturnEballoc;
			if(l != ft->nwidth){
				free(ft->width);
				ft->nwidth = l;
				ft->width = emalloc(l*sizeof(ft->width[0]));
			}
			memset(ft->width, 0, l*sizeof(ft->width[0]));
			ft->sized = 1;
			m -= 7;
			buf += 7;
			break;

		case 'w':
			/*
			 * write
			 *	'w'		1
			 *	dst id		2
			 *	miny		4
			 *	maxy		4
			 *	data		bytewidth*(maxy-miny)
			 */
			if(m < 11)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			if(did < 0)
				goto ReturnEbit;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbit;
			if(did == 0)	/* use cache */
				d = w->l->cache;
			else
				d = &bmw[did];
			miny = BGLONG(p+3);
			maxy = BGLONG(p+7);
			if(miny>maxy || miny<d->r.min.y || maxy>d->r.max.y)
				goto ReturnEbit;
			ws = 1<<(3-d->ldepth);	/* pixels per byte */
			/* set l to number of bytes of incoming data per scan line */
			if(d->r.min.x >= 0)
				l = (d->r.max.x+ws-1)/ws - d->r.min.x/ws;
			else{	/* make positive before divide */
				t = (-d->r.min.x)+ws-1;
				t = (t/ws)*ws;
				l = (t+d->r.max.x+ws-1)/ws;
			}
			t = 11 + l*(maxy-miny);
			if(m < t)
				goto ReturnEbit;
			p = bneed(t);
			memmove(p, buf, t);
			BPSHORT(p+1, d->id);
			if(did == 0){
				/* if to screen, udpate from cache */
				dl = w->l;
				r = dl->r;
				r.min.y = miny;
				r.max.y = maxy;
				if(dl->vis==Visible)
					bitblt(dl->cover->layer, r.min, dl->cache, r, S);
				else if(dl->vis==Obscured)
					layerop(lupdate, r, dl, 0);
			}
			m -= t;
			buf += t;
			break;

		case 'x':
			/*
			 * cursorset
			 *
			 *	'x'		1
			 *	pt		8
			 */
			if(m < 9)
				goto ReturnEbit;
			p = (uchar*)buf;
			pt1.x = BGLONG(p+1);
			pt1.y = BGLONG(p+5);
			if(input==w && ptinrect(pt1, w->l->r)){
				p = bneed(9);
				memmove(p, buf, 9);
				/* flush will happen at end of bitwrite */
			}
			m -= 14;
			buf += 14;
			break;

		case 'y':
			/*
			 * load font from subfont
			 *
			 *	'y'		1
			 *	id		2
			 *	cache pos	2
			 *	subfont id	2
			 *	subfont index	2
			 */
			if(m < 9)
				goto ReturnEbit;
			p = (uchar*)buf;
			did = BGSHORT(p+1);
			ft = &ftw[did];
			if(did<0 || did>=nftw || ft->w!=w)
				return Efont;
			if(ft->sized == 0)
				return Ebitmap;
			l = BGSHORT(p+3);
			if(l >= ft->nwidth)
				goto ReturnEbit;
			did = subfindex(w, BGSHORT(p+5));
			if(did < 0)
				return Esubfont;
			if(did == 0)
				sf = (SFW*)subfont;
			else
				sf = &sfw[did];
			t = BGSHORT(p+7);
			if(t<0 || t>=sf->n)
				goto ReturnEbit;
			ft->width[l] = sf->info[t].width;
			p = bneed(9);
			memmove(p, buf, 9);
			if(did == 0)
				BPSHORT(p+5, subfont->id);
			m -= 9;
			buf += 9;
			break;

		case 'z':
			/*
			 * write the colormap
			 *
			 *	'z'		1
			 *	id		2
			 *	map		12*(2**bitmapdepth)
			 */
			if(m < 3)
				goto ReturnEbit;
			did = BGSHORT(((uchar*)buf)+1);
			if(did < 0)
				goto ReturnEbit;
			if(did>0 && (did>=nbmw || bmw[did].w!=w))
				goto ReturnEbit;
			t = (did == 0)? screen.ldepth : bmw[did].ldepth;
			t = 3 + 12*(1 << (1 << t));
			if(m < t)
				goto ReturnEbit;
			p = bneed(t);
			memmove(p, buf, t);
			m -= t;
			buf += t;
			break;
		}

	if(!bwrite())
		goto ReturnEsbit;
	return n;

    Return0:
	bflush();
	return 0;
}

int
bitwinread(Window *w, ulong offset, int cnt)
{
	Bitmap *b;
	long px, t, l;

	if(w->deleted)
		return -1;
	if(offset == 0){
		if(w->l == 0)
			return -1;
		if(w->rdwindow == 0){
			b = w->l->cache;
			px = 1<<(3-b->ldepth);	/* pixels per byte */
			/* set l to number of bytes of data per scan line */
			if(b->r.min.x >= 0)
				l = (b->r.max.x+px-1)/px - b->r.min.x/px;
			else{	/* make positive before divide */
				t = (-b->r.min.x)+px-1;
				t = (t/px)*px;
				l = (t+b->r.max.x+px-1)/px;
			}
			w->rdwl = 5*12 + l*Dy(b->r);
			w->rdwindow = emalloc(w->rdwl);
			sprint((char*)w->rdwindow, "%11d %11d %11d %11d %11d ",
				b->ldepth, b->r.min.x, b->r.min.y, b->r.max.x, b->r.max.y);
			if(w->l->vis == Visible)
				bitblt(b, b->r.min, w->l, b->r, S);
			rdbitmap(b, b->r.min.y, b->r.max.y, w->rdwindow+5*12);
		}
	}
	if(offset > w->rdwl)
		return -1;
	if(cnt+offset > w->rdwl)
		cnt = w->rdwl-offset;
	bitbufm(cnt);
	memmove(bitbuf, w->rdwindow+offset, cnt);
	return cnt;
}

void
bitclose(Window *w)
{
	uchar *b;
	int id;
	FTW *ft;

	bflush();
	if(w->l)
		w->l->clipr = w->clipr;
	w->lastbar = Rect(0, 0, 0, 0);
	for(id=1; id<nbmw; id++)
		if(w == bmw[id].w){
			b = bneed(3);
			b[0] = 'f';
			b[1] = id;
			b[2] = id>>8;
			bmw[id].w = 0;
		}
	for(id=0; id<w->nsubf; id++)
		if(w->subf[id])
			refsubf(w, id, -w->subf[id]);
	free(w->subf);
	w->nsubf = 0;
	w->subf = 0;
	for(id=1; id<nftw; id++){
		ft = &ftw[id];
		if(w == ft->w){
			b = bneed(3);
			b[0] = 'h';
			b[1] = id;
			b[2] = id>>8;
			free(ft->width);
			ft->nwidth = 0;
			ft->w = 0;
		}
	}
	if(w->hold)
		w->cursorp = &whitearrow;
	else
		w->cursorp = 0;
	bflush();
	checkcursor(0);
}

void
bitwclose(Window *w)
{
	if(w->rdwindow){
		free(w->rdwindow);
		w->rdwindow = 0;
		w->rdwl = 0;
	}
}
