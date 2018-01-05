/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <cursor.h>
#include <event.h>
#include <bio.h>
#include <plumb.h>
#include <ctype.h>
#include <keyboard.h>
#include "page.h"

typedef struct Cached Cached;
struct Cached
{
	Document *doc;
	int page;
	int angle;
	Image *im;
};

static Cached cache[5];

static Image*
questionmark(void)
{
	static Image *im;

	if(im)
		return im;
	im = xallocimage(display, Rect(0,0,50,50), GREY1, 1, DBlack);
	if(im == nil)
		return nil;
	string(im, ZP, display->white, ZP, display->defaultfont, "?");
	return im;
}

void
cacheflush(void)
{
	int i;
	Cached *c;

	for(i=0; i<nelem(cache); i++){
		c = &cache[i];
		if(c->im)
			freeimage(c->im);
		c->im = nil;
		c->doc = nil;
	}
}

static Image*
_cachedpage(Document *doc, int angle, int page, char *ra)
{
	int i;
	Cached *c, old;
	Image *im, *tmp;
	static int lastpage = -1;

	if((page < 0 || page >= doc->npage) && !doc->fwdonly)
		return nil;

Again:
	for(i=0; i<nelem(cache); i++){
		c = &cache[i];
		if(c->doc == doc && c->angle == angle && c->page == page){
			if(chatty) fprint(2, "cache%s hit %d\n", ra, page);
			goto Found;
		}
		if(c->doc == nil)
			break;
	}

	if(i >= nelem(cache))
		i = nelem(cache)-1;
	c = &cache[i];
	if(c->im)
		freeimage(c->im);
	c->im = nil;
	c->doc = nil;
	c->page = -1;

	if(chatty) fprint(2, "cache%s load %d\n", ra, page);
	im = doc->drawpage(doc, page);
	if(im == nil){
		if(doc->fwdonly)	/* end of file */
			wexits(0);
		im = questionmark();
		if(im == nil){
		Flush:
			if(i > 0){
				cacheflush();
				goto Again;
			}
			fprint(2, "out of memory: %r\n");
			wexits("memory");
		}
		return im;
	}

	if(im->r.min.x != 0 || im->r.min.y != 0){
		/* translate to 0,0 */
		tmp = xallocimage(display, Rect(0, 0, Dx(im->r), Dy(im->r)), im->chan, 0, DNofill);
		if(tmp == nil){
			freeimage(im);
			goto Flush;
		}
		drawop(tmp, tmp->r, im, nil, im->r.min, S);
		freeimage(im);
		im = tmp;
	}

	switch(angle){
	case 90:
		im = rot90(im);
		break;
	case 180:
		rot180(im);
		break;
	case 270:
		im = rot270(im);
		break;
	}
	if(im == nil)
		goto Flush;

	c->doc = doc;
	c->page = page;
	c->angle = angle;
	c->im = im;

Found:
	if(chatty) fprint(2, "cache%s mtf %d @%d:", ra, c->page, i);
	old = *c;
	memmove(cache+1, cache, (c-cache)*sizeof cache[0]);
	cache[0] = old;
	if(chatty){
		for(i=0; i<nelem(cache); i++)
			fprint(2, " %d", cache[i].page);
		fprint(2, "\n");
	}
	if(chatty) fprint(2, "cache%s return %d %p\n", ra, old.page, old.im);
	return old.im;
}

Image*
cachedpage(Document *doc, int angle, int page)
{
	static int lastpage = -1;
	static int rabusy;
	Image *im;
	int ra;

	if(doc->npage < 1)
		return display->white;

	im = _cachedpage(doc, angle, page, "");
	if(im == nil)
		return nil;

	/* readahead */
	ra = -1;
	if(!rabusy){
		if(page == lastpage+1)
			ra = page+1;
		else if(page == lastpage-1)
			ra = page-1;
	}
	lastpage = page;
	if(ra >= 0){
		rabusy = 1;
		switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			rabusy = 0;
			break;
		case 0:
			lockdisplay(display);
			_cachedpage(doc, angle, ra, "-ra");
			rabusy = 0;
			unlockdisplay(display);
			_exits(nil);
		default:
			break;
		}
	}
	return im;
}
