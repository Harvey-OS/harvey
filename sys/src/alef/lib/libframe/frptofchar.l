#include <alef.h>
#include <graphics.h>
#include <frame.h>

Point
_frptofcharptb(Frame *f, uint p, Point pt, int bn)
{
	char *s;
	Frbox *b;
	int w, l;
	Rune r;

	for(b = &f->box[bn]; bn < f->nbox; bn++){
		_frcklinewrap(f, &pt, b);
		l = b->nrune;
		if(l < 0)
			l = 1;
		if(p < l){
			if(b->nrune > 0)
				for(s = b->ptr; p > 0; s += w){
					r = *s;
					if(r < Runeself)
						w = 1;
					else
						w = chartorune(&r, (char*)s);
					pt.x += charwidth(f->font, r);
					if(r==0 || pt.x>f->r.max.x)
						berror("frptofchar");
					p--;
				}
			break;
		}
		p -= l;
		_fradvance(f, &pt, b);
		b++;
	}
	return pt;
}

Point
frptofchar(Frame *f, uint p)
{
	return _frptofcharptb(f, p, Pt(f->left, f->r.min.y), 0);
}

Point
_frptofcharnb(Frame *f, uint p, int nb)	/* doesn't do final _fradvance to next line */
{
	Point pt;
	int nbox;

	nbox = f->nbox;
	f->nbox = nb;
	pt = _frptofcharptb(f, p, Pt(f->left, f->r.min.y), 0);
	f->nbox = nbox;
	return pt;
}

intern
Point
_frgrid(Frame *f, Point p)
{
	p.y -= f->r.min.y;
	p.y -= p.y%f->font->height;
	p.y += f->r.min.y;
	if(p.x > f->r.max.x)
		p.x = f->r.max.x;
	return p;
}

uint
frcharofpt(Frame *f, Point pt)
{
	Point qt;
	int w, bn, nrune;
	char *s;
	Frbox *b;
	uint p;
	Rune r;

	pt = _frgrid(f, pt);
	qt.x = f->left;
	qt.y = f->r.min.y;

	b=f->box;
	bn=0;
	for(p=0; bn<f->nbox && qt.y<pt.y; bn++){
		_frcklinewrap(f, &qt, b);
		if(qt.y >= pt.y)
			break;
		_fradvance(f, &qt, b);
		nrune = b->nrune;
		if(nrune < 0)
			nrune = 1;
		p += nrune;
		b++;
	}

	for(; bn<f->nbox && qt.x<=pt.x; bn++){
		_frcklinewrap(f, &qt, b);
		if(qt.y > pt.y)
			break;
		if(qt.x+b->wid > pt.x){
			if(b->nrune < 0)
				_fradvance(f, &qt, b);
			else{
				s = b->ptr;
				for(;;){
					r = *s;
					if(r < Runeself)
						w = 1;
					else
						w = chartorune(&r, (char*)s);
					if(r == 0)
						berror("end of string in frcharofpt");
					s += w;
					qt.x += charwidth(f->font, r);
					if(qt.x > pt.x)
						break;
					p++;
				}
			}
		}else{
			nrune = b->nrune;
			if(nrune < 0)
				nrune = 1;
			p += nrune;
			_fradvance(f, &qt, b);
		}
		b++;
	}
	return p;
}
