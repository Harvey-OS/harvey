#include <alef.h>
#include <libg.h>
#include <frame.h>

Bitmap	*frones;
intern byte fronebits[] = {
	0xFF, 0xFF, 0xFF, 0xFF,
};

void
frinit(Frame *f, Rectangle r, Font *ft, Bitmap *b)
{
	int w;

	f->font = ft;
	f->scroll = nil;
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
	if(frones == nil){
		w = (sizeof(uint)*8)>>screen.ldepth;
		frones = balloc(Rect(0, 0, w, 1), screen.ldepth);
		if(frones == nil){
			fprint(2, "frame: can't initialize textures\n");
			exits("frame textures");
		}
		wrbitmap(frones, 0, 1, fronebits);
	}
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
