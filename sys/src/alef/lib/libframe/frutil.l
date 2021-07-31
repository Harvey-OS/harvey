#include <alef.h>
#include <graphics.h>
#include <frame.h>

int
_frcanfit(Frame *f, Point pt, Frbox *b)
{
	int left, w, nr;
	char *p;
	Rune r;

	left = f->r.max.x-pt.x;
	if(b->nrune < 0)
		return b->minwid <= left;
	if(left >= b->wid)
		return b->nrune;
	nr = 0;
	for(p = b->ptr; *p; p+=w){
		r = *p;
		if(r < Runeself)
			w = 1;
		else
			w = chartorune(&r, p);
		left -= charwidth(f->font, r);
		if(left < 0)
			return nr;
		nr++;
	}
	berror("_frcanfit can't");
	return 0;
}

void
_frcklinewrap(Frame *f, Point *p, Frbox *b)
{
	int wid;

	if(b->nrune < 0)
		wid = b->minwid;
	else
		wid = b->wid;

	if(wid > f->r.max.x-p->x){
		p->x = f->left;
		p->y += f->font->height;
	}
}

void
_frcklinewrap0(Frame *f, Point *p, Frbox *b)
{
	if(_frcanfit(f, *p, b) == 0){
		p->x = f->left;
		p->y += f->font->height;
	}
}

void
_fradvance(Frame *f, Point *p, Frbox *b)
{
	if(b->nrune<0 && b->bc=='\n'){
		p->x = f->left;
		p->y += f->font->height;
	}else
		p->x += b->wid;
}

int
_frnewwid(Frame *f, Point pt, Frbox *b)
{
	int c, x;

	c = f->r.max.x;
	x = pt.x;
	if(b->nrune >= 0)
		return b->wid;
	if(b->bc == '\t'){
		if(x+b->minwid > c)
			x = pt.x = f->left;
		x += f->maxtab;
		x -= (x-f->left)%f->maxtab;
		if(x-pt.x<b->minwid || x>c)
			x = pt.x+b->minwid;
		b->wid = x-pt.x;
	}
	return b->wid;
}

void
_frclean(Frame *f, Point pt, int n0, int n1)	/* look for mergeable boxes */
{
	Frbox *b;
	int nb, c;

	c = f->r.max.x;
	for(nb=n0; nb<n1-1; nb++){
		b = &f->box[nb];
		_frcklinewrap(f, &pt, b);
		while(b[0].nrune>=0 && nb<n1-1 && b[1].nrune>=0 && pt.x+b[0].wid+b[1].wid<c){
			_frmergebox(f, nb);
			n1--;
			b = &f->box[nb];
		}
		_fradvance(f, &pt, &f->box[nb]);
	}
	for(; nb<f->nbox; nb++){
		b = &f->box[nb];
		_frcklinewrap(f, &pt, b);
		_fradvance(f, &pt, &f->box[nb]);
	}
	f->lastlinefull = 0;
	if(pt.y >= f->r.max.y)
		f->lastlinefull = 1;
}
