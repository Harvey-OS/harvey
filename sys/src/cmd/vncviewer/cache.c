#include "vnc.h"

enum {
	NHASH = 13,
	HASHLEN=1,
};

static int nalloc;

typedef struct Chash Chash;
struct Chash {
	Color c;
	Image *im;
	Chash *next;
};

static Chash *hash[NHASH];

static Image*
movetofront(Chash **head, Chash *h)
{
	h->next = *head;
	*head = h;
	return h->im;
}

static Image*
getcolorhash(Chash **head, Color c)
{
	Chash **p, **lastp;
	Chash *h;
	int nnode;

	for(lastp=nil, p=head, nnode=0; *p; lastp=p, p=&(*p)->next, nnode++) {
		if((*p)->c == c) {
			h = *p;
			*p = h->next;
			h->next = nil;
			return movetofront(head, h);
		}
	}

	/* color not found */
	lockdisplay(display);

	if(nnode >= HASHLEN) {	/* reuse last node on list */
		h = *lastp;
		*lastp = nil;
		assert(h->next == nil);
	} else {
		h = malloc(sizeof(*h));
		h->im = allocimage(display, Rect(0,0,1,1), display->chan, 1, DNofill);
		nalloc++;
		assert(nalloc <= NHASH*HASHLEN);
	}
	h->c = c;

	loadimage(h->im, Rect(0,0,1,1), (uchar*)&c, display->depth/8);

	unlockdisplay(display);

	return movetofront(head, h);
}

Image*
colorimage(Color c)
{
	return getcolorhash(&hash[c%NHASH], c);
}


