#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <mux.h>

static	Tag	*taghash[Hashsize];
static	Tag	*taglist;
static	ushort	tagalloc = Tagstart;

ushort
tagM2S(ushort tag, ushort mux)
{
	Tag *t, **h;

	t = taglist;
	if(t == 0) {
		t = malloc(sizeof(Tag));
		if(t == 0) {
			fprint(2, "tagM2S: no memory\n");
			exits("tagM2S");
		}
		memset(t, 0, sizeof(Tag));
		t->tag = tagalloc++;
		h = &taghash[Hash(t->tag)];
		t->hash = *h;
		*h = t;
	}
	else
		taglist = t->free;

	t->clienttag = tag;
	t->mux = mux;
	return t->tag;	
}

ushort
tagS2M(ushort tag, ushort *mux, int lookup)
{
	Tag *t;

	for(t = taghash[Hash(tag)]; t; t = t->hash) {
		if(tag == t->tag) {
			if(lookup == 0) {
				t->free = taglist;
				taglist = t;
			}
			*mux = t->mux;
			return t->clienttag;
		}
	}

	if(lookup)		/* Out of date flush */
		return 0;

	fprint(2, "tagS2M: unseen server tag %d\n", tag);
	exits("tagS2M");
}
