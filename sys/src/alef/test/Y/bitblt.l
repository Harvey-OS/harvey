#include <alef.h>
#include <fcall.h>
#include <libg.h>
#include <frame.h>
#include "y.h"

byte *ebadb	= "bad bitblt request";
byte *epcol	= "bad bitblt protocol";
byte *emap 	= "operation on invalid bitmap";
byte *eballoc	= "out of bitmap memory";

aggr Fonttab
{
	int	id;
	int	ncache;
	int	height;
	Window	*w;
};

int	nftable;
Fonttab	*ftable;

aggr Sfonttab
{
	int	users;
	Qid;
	Subfont;
};

int	 nsftable;
Sfonttab *sftable;

void
setsub()
{
	int fd;
	Bitmap *b;

	strwidth(font, " ");
	if(font->sub[0]->name) {
		fd = open(font->sub[0]->name, OREAD);
		if(fd < 0)
			error("open subfont: %r");

		b = rdbitmapfile(fd);
		if(b == nil)
			error("rdbitmap subfont: %r");

		subfont = rdsubfontfile(fd, b);
		if(subfont == nil)
			error("read subfont");
		close(fd);
		return;
	}
	subfont = font->subf[0].f;

	nsftable = 10;
	sftable = malloc(nsftable*sizeof(Sfonttab));
	memmove(&sftable[0].Subfont, subfont, sizeof(Subfont));
	sftable[0].users = 1;
}

int
newbit(Window *w)
{
	int i;

	for(i = 1; i < w->maxbit; i++)
		if(w->bits[i] == nil)
			break;

	if(i >= w->maxbit) {
		i = w->maxbit+1;
		w->maxbit += 10;
		w->bits = realloc(w->bits, w->maxbit*sizeof(Bitmap*));
	}

	return i;
}

Bitmap**
getbit(Window *w, byte *p)
{
	int i;
	Bitmap **b;

	i = BGSHORT(p);
	if(i == 0)
		return &w->cache;
	if(i < 0 || i > w->maxbit)
		return nil;
	b = &w->bits[i];
	if(*b == nil)
		return nil;
	return b;
}

Rectangle
getr(byte *p)
{
	Rectangle r;

	r.min.x = BGLONG(p);
	p += 4;
	r.min.y = BGLONG(p);
	p += 4;
	r.max.x = BGLONG(p);
	p += 4;
	r.max.y = BGLONG(p);

	return r;
}

void
bitwrite(Window *w, Ioreq *i)
{
	Fcall thdr;
	Rectangle src;
	byte tpa[32];
	Sfonttab *sf;
	Fonttab *f;
	uint q0, q1;
	Bitmap **s, **d, *b;
	byte *e, *p, *end, *xp;
	int v, nc, idx, l, t, miny, maxy, words;

	p = i->fcall.data;
	end = p+i->fcall.count;
	e = nil;
	w->mid = 0;

	while(p < end) {
		switch(p[0]) {
		default:
			e = ebadb;
			break;
	
		case 'a':
			idx = newbit(w);
			src = getr(p+2);
			b = balloc(src, p[1]);
			if(b == nil) {
				e = eballoc;
				break;
			}
			w->bits[idx] = b; 
			w->rid = idx;
			w->mid = 'A';
			p += 18;
			break;
	
		case 'b':
			d = getbit(w, p+1);
			s = getbit(w, p+11);
			if(d == nil || s == nil) {
				e = emap;
				break;
			}
			xp = bneed(31);
			memmove(xp, p, 31);
			BPSHORT(xp+1, (*d)->id);
			BPSHORT(xp+11, (*s)->id);
			p += 31;
			break;

		case 'c':
			t = end-p;
			if(t == 1) {
				w->dcursor = 1;
				p++;
				cuset(nil);
				break;
			}
			if(t < 73) {
				e = epcol;
				break;
			}
			w->offset.x = BGLONG(p+1);
			w->offset.y = BGLONG(p+5);
			memmove(w->clr, p+9, 2*16);
			memmove(w->set, p+41, 2*16);
			w->dcursor = 0;
			p += 73;
			w->chgcurs = 1;
			cuset(nil);
			break;

		case 'f':
			s = getbit(w, p+1);
			if(s == nil) {
				e = emap;
				break;
			}
			bfree(*s);
			*s = nil;
			p += 3;
			break;

		case 'i':
			w->mid = 'I';
			p++;
			break;

		case 'j':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 9)
				raise;

			q0 = BGLONG(p+1);
			q1 = BGLONG(p+5);
			p += 9;
			/*
			 * Should ask the system to see if this is already cached
			 */
			for(t = 0; t < nsftable; t++) {
				sf = &sftable[t];
				if(sf->path == q0 && sf->vers == q1) {
					w->rid = sf->id;
					w->mid = 'J';
					break 2;
				}
			}
			raise;
	
		case 'k':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 15)
				raise;
			v = BGSHORT(p+1);
			nc = 15+6*(v+1);
			if(end-p < nc)
				raise;

			l = p[3];
			t = p[4];
			s = getbit(w, p+5);
			if(s == nil) {
				e = emap;
				break;
			}
			b = *s;

			BPSHORT(p+5, b->id);
			xp = bneed(nc);
			memmove(xp, p, nc);
			if(!bwrite())
				raise;

			if(read(bitbltfd, tpa, 3) !=3 || tpa[0]!='K')
				raise;

			idx = BGSHORT(tpa+1);
			w->rid = idx;
			w->mid = 'K';
			free(b);
			*s = nil;

			if(idx >= nsftable) {
				sftable = realloc(sftable, idx*2*sizeof(Sfonttab));
				nsftable = idx*2;
			}

			q0 = BGLONG(p+7);
			q1 = BGLONG(p+11);

			sf = &sftable[idx];
			if(sf->users == 0) {
				sf->users = 1;
				sf->n = v;
				sf->height = l;
				sf->ascent = t;
				sf->path = q0;
				sf->vers = q1;
				sf->id = idx;
				sf->info = malloc(sizeof(Fontchar)*(v+1));
				_unpackinfo(sf->info, p+15, v);
			}
			else
			if(q0 != sf->path)
				error("sfont cache QID");
			p += 15+nc;
			break;

		case 'l':
			if(end-p < 22) {
				e = epcol;
				break;
			}
			d = getbit(w, p+1);
			if(d == nil) {
				e = emap;
				break;
			}
			xp = bneed(22);
			memmove(xp, p, 22);
			BPSHORT(xp+1, (*d)->id);
			p += 22;
			break;

		case 'n':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 7)
				raise;

			xp = bneed(7);
			memmove(xp, p, 7);
			if(!bwrite())
				raise;

			if(read(bitbltfd, tpa, 3) !=3 || tpa[0] != 'N')
				raise;

			idx = BGSHORT(tpa+1);
			if(idx >= nftable){
				ftable = realloc(ftable, 2*idx*sizeof(Fonttab));
				nftable = idx*2;
			}
			f = &ftable[idx];
			if(f->w)
				error("reuse of font");

			f->ncache = BGSHORT(p+5);
			if(f->ncache > MAXFCACHE)
				raise;
			f->w = w;
			f->height = p[1];
			f->id = idx;
			w->rid = idx;
			w->mid = 'N';
			p += 7;
			break;

		case 'p':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 14)
				raise;

			d = getbit(w, p+1);
			if(*d == nil) {
				e = emap;
				break;
			}
			xp = bneed(14);
			memmove(xp, p, 14);
			idx = (*d)->id;
			BPSHORT(xp+1, idx);
			p += 14;
			break;
			
		case 'q':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 19)
				raise;
			
			d = getbit(w, p+1);
			if(*d == nil) {
				e = emap;
				break;
			}
			w->clip = getr(p+3);
			p += 19;
			break;

		case 's':
			rescue {
				e = epcol;
				break;
			};
			t = end-p;
			if(t < 17)
				raise;

			d = getbit(w, p+1);
			if(d == nil) {
				e = emap;
				break;
			}
			idx = BGSHORT(p+11);
			f = &ftable[idx];
			if(idx < 0 || idx > nftable || f->w != w)
				raise;

			nc = BGSHORT(p+15)*2 + 17;
			if(t < nc)
				raise;

			xp = bneed(nc);
			memmove(xp, p, nc);
			BPSHORT(xp+1, (*d)->id);
			p += nc;
			break;
			
		case 't':
			if(end-p < 23) {
				e = epcol;
				break;
			}
			d = getbit(w, p+1);
			s = getbit(w, p+19);
			if(d == nil || s == nil) {
				e = emap;
				break;
			}
			xp = bneed(23);
			memmove(xp, p, 23);
			BPSHORT(xp+1, (*d)->id);
			BPSHORT(xp+19, (*s)->id);
			p += 23;
			break;

		case 'v':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 7)
				raise;

			idx = BGSHORT(p+1);
			f = &ftable[idx];
			if(idx < 0 || idx > nftable || f->w != w)
				raise;

			if(!bwrite())
				raise;
			xp = bneed(7);
			memmove(xp, p, 7);
			if(!bwrite())
				raise;

			t = BGSHORT(p+3);
			f->ncache = t;
			p += 7;
			break;
			
		case 'w':
			rescue {
				e = epcol;
				break;
			}
			if(end-p < 11)
				raise;
			d = getbit(w, p+1);
			if(d == nil) {
				e = emap;
				break;
			}
			b = *d;
			miny = BGLONG(p+3);
			maxy = BGLONG(p+7);
			if(miny > maxy || miny < b->r.min.y || maxy > b->r.max.y)
				raise;
			words = 1<<(3-b->ldepth);
			if(b->r.min.x >= 0)
				l = (b->r.max.x+words-1)/words - b->r.min.x/words;
			else {
				t = (-b->r.min.x)+words-1;
				t = (t/words)*words;
				l = (t+b->r.max.x+words-1)/words;
			}
			t = 11 + l*(maxy-miny);
			if(end-p < t)
				raise;
			wrbitmap(b, miny, maxy, p+11);
			p += t;
			break;

		case 'x':
			w->mse.xy = (Point)(BGLONG(p+1), BGLONG(p+5));
			cursorset(w->mse.xy);
			p += 9;
			break;

		case 'y':
			rescue {
				e = epcol;
				break;
			}
			idx = BGSHORT(p+1);
			f = &ftable[idx];
			if(idx < 0 || idx > nftable || f->w != w)
				raise;
			l = BGSHORT(p+3);
			if(l < 0 || l > f->ncache)
				raise;
			idx = BGSHORT(p+5);
			if(idx < 0 || idx > nsftable)
				raise;
			sf = &sftable[idx];
			if (sf->users == 0)
				raise;
			l = BGSHORT(p+7);
			if(l < 0 || l >= sf->n)
				raise;
			
			xp = bneed(9);
			memmove(xp, p, 9);
			p += 9;
			break;
		}
		if(e != nil)
			break;
	}

	thdr.count = i->fcall.count;
	reply(&i->fcall, &thdr, e);
}

void
bitread(Window *w, Ioreq *i)
{
	byte *p;
	Fcall thdr;
	Subfont *sf;
	Fontchar *info;
	int cnt, dat, j;

	p = i->buf;
	thdr.data = p;
	cnt = i->fcall.count;

	rescue {
		reply(&i->fcall, &thdr, epcol);
		return;
	};

	p[0] = w->mid;
	switch(w->mid) {
	case 0:
		raise;

	case 'J':
		BPSHORT(p+1, w->rid);
		sf = &sftable[w->rid].Subfont;
		sprint(p+3, "%11d %11d %11d ", sf->n, sf->height, sf->ascent);
		dat = 36+3+((sf->n+1)*6);
		if(cnt < dat)
			raise;

		p += 36+3;
		info = sf->info;
		for(j=0; j <= sf->n; j++) {
			BPSHORT(p, info->x);
			p[2] = info->top;
			p[3] = info->bottom;
			p[4] = info->left;
			p[5] = info->width;
			info++;
			p += 6;
		}
		thdr.count = dat;
		break;

	case 'I':
		if(cnt < 18)
			raise;

		p[1] = screen.ldepth;
		BPLONG(p+2, w->win.min.x);
		BPLONG(p+6, w->win.min.y);
		BPLONG(p+10, w->win.max.x);
		BPLONG(p+14, w->win.max.y);
		dat = 18;
		BPLONG(p+18, w->clip.min.x);
		BPLONG(p+22, w->clip.min.y);
		BPLONG(p+26, w->clip.max.x);
		BPLONG(p+30, w->clip.max.y);
		dat += 16;
		if(cnt >= dat+36+6*(subfont->n+1)){
			p += dat;
			sprint(p, "%11d %11d %11d ",
					subfont->n, subfont->height, subfont->ascent);
			p += 36;
			info = subfont->info;
			for(j=0; j <= subfont->n; j++) {
				BPSHORT(p, info->x);
				p[2] = info->top;
				p[3] = info->bottom;
				p[4] = info->left;
				p[5] = info->width;
				info++;
				p += 6;
			}
			cnt = dat+3*12+6*(subfont->n+1);
		}else
			cnt = dat;

		thdr.count = cnt;
		break;

	default:
		if(cnt < 3)
			raise;

		BPSHORT(p+1, w->rid);
		thdr.count = 3;
		break;
	}
	w->mid = 0;
	reply(&i->fcall, &thdr, nil);
}

int
winbits(Window *w, byte *buf, uint offset, int cnt)
{
	Bitmap *b;
	int t, l, px;

	if(w->closed)
		return -1;

	if(offset == 0) {
		if(w->cache == nil)
			return -1;

		if(w->window == nil) {
			b = w->cache;
			px = 1<<(3-b->ldepth);

			if(b->r.min.x >= 0)
				l = (b->r.max.x+px-1)/px - b->r.min.x/px;
			else {
				t = (-b->r.min.x)+px-1;
				t = (t/px)*px;
				l = (t+b->r.max.x+px-1)/px;
			}

			w->wsize = 56+l*Dy(b->r);
			w->window = malloc(w->wsize);

			sprint(w->window, "%11d %11d %11d %11d %11d ",
			b->ldepth, b->r.min.x, b->r.min.y, b->r.max.x, b->r.max.y);

			rdbitmap(b, b->r.min.y, b->r.max.y, w->window+56);
		}
	}
	if(offset > w->wsize)
		return -1;

	if(cnt+offset > w->wsize)
		cnt = w->wsize-offset;

	memmove(buf, w->window+offset, cnt);
	return cnt;
}

void
winread(Window *w, Ioreq *i)
{
	int n;
	Fcall thdr;

	n = winbits(w, i->buf, i->fcall.offset, i->fcall.count);
	thdr.data = i->buf;
	thdr.count = n;

	reply(&i->fcall, &thdr, nil);		
}
