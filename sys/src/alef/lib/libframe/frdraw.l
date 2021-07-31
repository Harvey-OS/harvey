#include <alef.h>
#include <libg.h>
#include <frame.h>

void
_frredraw(Frame *f, Point pt)
{
	Frbox *b;
	int nb;

	nb = 0;
	for(b=f->box; nb < f->nbox; nb++) {
		_frcklinewrap(f, &pt, b);
		if(b->nrune >= 0)
			string(f->b, pt, f->font, (byte *)b->ptr, S);
		pt.x += b->wid;
		b++;
	}
}

Point
_frdraw(Frame *f, Point pt)
{
	Frbox *b;
	int nb, n;

	b=f->box;
	for(nb=0; nb < f->nbox; nb++){
		_frcklinewrap0(f, &pt, b);
		if(pt.y == f->r.max.y){
			f->nchars -= _frstrlen(f, nb);
			_frdelbox(f, nb, f->nbox-1);
			break;
		}
		if(b->nrune > 0){
			n = _frcanfit(f, pt, b);
			if(n == 0)
				berror("draw: _frcanfit==0");
			if(n != b->nrune){
				_frsplitbox(f, nb, n);
				b = &f->box[nb];
			}
			pt.x += b->wid;
		}else{
			if(b->bc == '\n') {
				pt.x = f->left;
				pt.y += f->font->height;
			}
			else
				pt.x += _frnewwid(f, pt, b);
		}
		b++;
	}
	return pt;
}

int
_frstrlen(Frame *f, int nb)
{
	int n, nrune;

	for(n=0; nb<f->nbox; nb++) {
		nrune = f->box[nb].nrune;
		if(nrune < 0)
			nrune = 1;
		n += nrune;
	}
	return n;
}
