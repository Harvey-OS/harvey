/*
 * rotate an image 180° in O(log Dx + log Dy) /dev/draw writes,
 * using an extra buffer same size as the image.
 * 
 * the basic concept is that you can invert an array by inverting
 * the top half, inverting the bottom half, and then swapping them.
 * the code does this slightly backwards to ensure O(log n) runtime.
 * (If you do it wrong, you can get O(log² n) runtime.)
 * 
 * This is usually overkill, but it speeds up slow remote
 * connections quite a bit.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <event.h>
#include "page.h"

int ndraw = 0;
enum {
	Xaxis = 0,
	Yaxis = 1,
};

Image *mtmp;

void
writefile(char *name, Image *im, int gran)
{
	static int c = 100;
	int fd;
	char buf[200];

	snprint(buf, sizeof buf, "%d%s%d", c++, name, gran);
	fd = create(buf, OWRITE, 0666);
	if(fd < 0)
		return;	
	writeimage(fd, im, 0);
	close(fd);
}

void
moveup(Image *im, Image *tmp, int a, int b, int c, int axis)
{
	Rectangle range;
	Rectangle dr0, dr1;
	Point p0, p1;

	if(a == b || b == c)
		return;

	draw(tmp, tmp->r, im, nil, im->r.min);

	switch(axis){
	case Xaxis:
		range = Rect(a, im->r.min.y,  c, im->r.max.y);
		dr0 = range;
		dr0.max.x = dr0.min.x+(c-b);
		p0 = Pt(b, im->r.min.y);

		dr1 = range;
		dr1.min.x = dr1.max.x-(b-a);
		p1 = Pt(a, im->r.min.y);
		break;
	case Yaxis:
		range = Rect(im->r.min.x, a,  im->r.max.x, c);
		dr0 = range;
		dr0.max.y = dr0.min.y+(c-b);
		p0 = Pt(im->r.min.x, b);

		dr1 = range;
		dr1.min.y = dr1.max.y-(b-a);
		p1 = Pt(im->r.min.x, a);
		break;
	}
	draw(im, dr0, tmp, nil, p0);
	draw(im, dr1, tmp, nil, p1);
}

void
interlace(Image *im, Image *tmp, int axis, int n, Image *mask, int gran)
{
	Point p0, p1;
	Rectangle r0, r1;

	r0 = im->r;
	r1 = im->r;
	switch(axis) {
	case Xaxis:
		r0.max.x = n;
		r1.min.x = n;
		p0 = (Point){gran, 0};
		p1 = (Point){-gran, 0};
		break;
	case Yaxis:
		r0.max.y = n;
		r1.min.y = n;
		p0 = (Point){0, gran};
		p1 = (Point){0, -gran};
		break;
	}

	draw(tmp, im->r, im, display->opaque, im->r.min);
	gendraw(im, r0, tmp, p0, mask, mask->r.min);
	gendraw(im, r0, tmp, p1, mask, p1);
}

/*
 * Halve the grating period in the mask.
 * The grating currently looks like 
 * ####____####____####____####____
 * where #### is opacity.
 *
 * We want
 * ##__##__##__##__##__##__##__##__
 * which is achieved by shifting the mask
 * and drawing on itself through itself.
 * Draw doesn't actually allow this, so 
 * we have to copy it first.
 *
 *     ####____####____####____####____ (dst)
 * +   ____####____####____####____#### (src)
 * in  __####____####____####____####__ (mask)
 * ===========================================
 *     ##__##__##__##__##__##__##__##__
 */
int
nextmask(Image *mask, int axis, int maskdim)
{
	Point δ;

	δ = axis==Xaxis ? Pt(maskdim,0) : Pt(0,maskdim);
	draw(mtmp, mtmp->r, mask, nil, mask->r.min);
	gendraw(mask, mask->r, mtmp, δ, mtmp, divpt(δ,-2));
//	writefile("mask", mask, maskdim/2);
	return maskdim/2;
}

void
shuffle(Image *im, Image *tmp, int axis, int n, Image *mask, int gran,
	int lastnn)
{
	int nn, left;

	if(gran == 0)
		return;
	left = n%(2*gran);
	nn = n - left;

	interlace(im, tmp, axis, nn, mask, gran);
//	writefile("interlace", im, gran);
	
	gran = nextmask(mask, axis, gran);
	shuffle(im, tmp, axis, n, mask, gran, nn);
//	writefile("shuffle", im, gran);
	moveup(im, tmp, lastnn, nn, n, axis);
//	writefile("move", im, gran);
}

void
rot180(Image *im)
{
	Image *tmp;
	Image *mask;
	Rectangle rmask;
	int gran;

	tmp = xallocimage(display, im->r, im->chan, 0, DNofill);
	if(tmp == nil)
		return;

	for(gran=1; gran<Dx(im->r); gran *= 2)
		;
	gran /= 4;

	rmask.min = ZP;
	rmask.max = (Point){2*gran, 100};

	mask = xallocimage(display, rmask, GREY1, 1, DTransparent);
	mtmp = xallocimage(display, rmask, GREY1, 1, DTransparent);
	rmask.max.x = gran;
	draw(mask, rmask, display->opaque, nil, ZP);
//	writefile("mask", mask, gran);
	shuffle(im, tmp, Xaxis, Dx(im->r), mask, gran, 0);
	freeimage(mask);
	freeimage(mtmp);

	for(gran=1; gran<Dy(im->r); gran *= 2)
		;
	gran /= 4;
	rmask.max = (Point){100, 2*gran};
	mask = xallocimage(display, rmask, GREY1, 1, DTransparent);
	mtmp = xallocimage(display, rmask, GREY1, 1, DTransparent);
	rmask.max.y = gran;
	draw(mask, rmask, display->opaque, nil, ZP);
	shuffle(im, tmp, Yaxis, Dy(im->r), mask, gran, 0);
	freeimage(mask);
	freeimage(mtmp);
	freeimage(tmp);
}
