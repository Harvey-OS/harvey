#include <alef.h>
#include <graphics.h>
#include <frame.h>

void
frinit(Frame *f, Rectangle r, Font *ft, Bitmap *b)
{
	f->font = ft;
	f->maxtab = 8*charwidth(ft, '0');
	f->nbox = 0;
	f->nalloc = 0;
	f->nchars = 0;
	f->nlines = 0;
	f->p0 = 0;
	f->p1 = 0;
	f->box = nil;
	f->lastlinefull = 0;
	frsetrects(f, r, b);
}

void
frsetrects(Frame *f, Rectangle r, Bitmap *b)
{
	f->b = b;
	f->entire = r;
	f->r = r;
	f->r.max.y -= (r.max.y-r.min.y)%f->font->height;
	f->left = r.min.x+1;
	f->maxlines = (r.max.y-r.min.y)/f->font->height;
}

void
frclear(Frame *f)
{
	if(f->nbox)
		_frdelbox(f, 0, f->nbox-1);

	if(f->box)
		free(f->box);

	f->box = nil;
}
