#include <u.h>
#include <libg.h>
#include <frame.h>

int
frdelete(Frame *f, ulong p0, ulong p1)
{
	Point pt0, pt1, ppt0;
	Frbox *b;
	int n0, n1, n;
	Rectangle r;
	int nn0;

	if(p0>=f->nchars || p0==p1 || f->b==0)
		return 0;
	if(p1 > f->nchars)
		p1 = f->nchars;
	n0 = _frfindbox(f, 0, (unsigned long)0, p0);
	n1 = _frfindbox(f, n0, p0, p1);
	pt0 = _frptofcharnb(f, p0, n0);
	pt1 = frptofchar(f, p1);
	if(f->p0!=p0 || f->p1!=p1)	/* likely they ARE equal */
		frselectp(f, F&~D);	/* can do better some day */
	frselectf(f, pt0, pt1, 0);
	if(n0 == f->nbox)
		berror("off end in frdelete");
	nn0 = n0;
	ppt0 = pt0;
	_frfreebox(f, n0, n1-1);
	f->modified = 1;

	/*
	 * Invariants:
	 *  pt0 points to beginning, pt1 points to end
	 *  n0 is box containing beginning of stuff being deleted
	 *  n1, b are box containing beginning of stuff to be kept after deletion
	 *  region between pt0 and pt1 is clear
	 */
	b = &f->box[n1];
	while(pt1.x!=pt0.x && n1<f->nbox){
		_frcklinewrap0(f, &pt0, b);
		_frcklinewrap(f, &pt1, b);
		if(b->nrune > 0){
			n = _frcanfit(f, pt0, b);
			if(n==0)
				berror("_frcanfit==0");
			if(n != b->nrune){
				_frsplitbox(f, n1, n);
				b = &f->box[n1];
			}
			r.min = pt1;
			r.max = pt1;
			r.max.x += b->wid;
			r.max.y += f->font->height;
			bitblt(f->b, pt0, f->b, r, S);
			if(pt0.y == pt1.y)
				r.min.x = r.max.x-(pt1.x-pt0.x);
			bitblt(f->b, r.min, f->b, r, 0);
		}
		_fradvance(f, &pt1, b);
		pt0.x += _frnewwid(f, pt0, b);
		f->box[n0++] = f->box[n1++];
		b++;
	}
	if(pt1.y != pt0.y){
		Point pt2;

		pt2 = _frptofcharptb(f, 32767, pt1, n1);
		if(pt2.y > f->r.max.y)
			berror("frptofchar in frdelete");
		if(n1 < f->nbox){
			int q0, q1, q2;

			q0 = pt0.y+f->font->height;
			q1 = pt1.y+f->font->height;
			q2 = pt2.y+f->font->height;
			bitblt(f->b, pt0, f->b, Rect(pt1.x, pt1.y, f->r.max.x, q1), S);
			bitblt(f->b, Pt(f->r.min.x, q0), f->b, Rect(f->r.min.x, q1, f->r.max.x, q2), S);
			frselectf(f, Pt(pt2.x, pt2.y-(pt1.y-pt0.y)), pt2, 0);
		}else
			frselectf(f, pt0, pt2, 0);
	}
	_frclosebox(f, n0, n1-1);
	if(nn0>0 && f->box[nn0-1].nrune>=0 && ppt0.x-f->box[nn0-1].wid>=(int)f->left){
		--nn0;
		ppt0.x -= f->box[nn0].wid;
	}
	_frclean(f, ppt0, nn0, n0<f->nbox-1? n0+1 : n0);
	if(f->p1 > p1)
		f->p1 -= p1-p0;
	else if(f->p1 > p0)
		f->p1 = p0;
	if(f->p0 > p1)
		f->p0 -= p1-p0;
	else if(f->p0 > p0)
		f->p0 = p0;
	frselectp(f, F&~D);
	f->nchars -= p1-p0;
	pt0 = frptofchar(f, f->nchars);
	n = f->nlines;
	f->nlines = (pt0.y-f->r.min.y)/f->font->height+(pt0.x>f->left);
	return n - f->nlines;
}
