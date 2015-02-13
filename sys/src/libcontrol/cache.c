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
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Cache Cache;

struct Cache
{
	int8_t		*name;
	CCache	**cache;
	int		ncache;
};

static struct Cache imagecache = {"image"};
static struct Cache fontcache = {"font"};

static CCache*
getcacheitem(Cache *c, int8_t *name)
{
	int i;

	for(i=0; i<c->ncache; i++)
		if(c->cache[i]!=nil && strcmp(c->cache[i]->name, name)==0){
			c->cache[i]->ref++;
			return c->cache[i];
		}
	return nil;
}

static int
namecacheitem(Cache *c, void *image, int8_t *name)
{
	int i, free;
	CCache *cc;

	free = -1;
	for(i=0; i<c->ncache; i++){
		if(c->cache[i] == nil){
			free = i;
			continue;
		}
		if(strcmp(c->cache[i]->name, name) == 0){
			werrstr("%s name %q already in use", c->name, name);
			return -1;
		}
	}
	cc = ctlmalloc(sizeof(CCache));
	cc->image = image;
	cc->name = ctlstrdup(name);
	if(free >= 0){
		cc->index = free;
		c->cache[free] = cc;
	}else{
		cc->index = c->ncache;
		c->cache = ctlrealloc(c->cache, (c->ncache+1)*sizeof(CCache*));
		c->cache[c->ncache++] = cc;
	}
	cc->ref = 1;
	return 1;
}

static int
freecacheitem(Cache *c, int8_t *name)
{
	CCache	*cc;

	cc = getcacheitem(c, name);
	if(cc == nil){
		werrstr("%s name %q not in use", c->name, name);
		return -1;
	}
	cc->ref--;	/* getcacheitem increments ref */
	if(cc->ref-- == 1){
		/* client must free object itself */
		free(cc->name);
		c->cache[cc->index] = nil;
		free(cc);
	}
	return 0;
}

static void
putcacheitem(CCache *cc)
{
	if(cc == nil)
		return;
	cc->ref--;
}

static void
setcacheitemptr(Cache *c, Control *ctl, CCache **cp, int8_t *s)
{
	CCache *ci;

	ci = getcacheitem(c, s);
	if(ci == nil)
		ctlerror("%q: %s name %q not defined", ctl->name, c->name, s);
	putcacheitem(*cp);
	*cp = ci;
}

/* Images */

CImage*
_getctlimage(int8_t *name)
{
	return getcacheitem(&imagecache, name);
}

void
_putctlimage(CImage *c)
{
	putcacheitem(c);
}

int
namectlimage(Image *image, int8_t *name)
{
	return namecacheitem(&imagecache, image, name);
}

int
freectlimage(int8_t *name)
{
	return freecacheitem(&imagecache, name);
}

void
_setctlimage(Control *c, CImage **cp, int8_t *s)
{
	setcacheitemptr(&imagecache, c, cp, s);
}

/* Fonts */

CFont*
_getctlfont(int8_t *name)
{
	return getcacheitem(&fontcache, name);
}

void
_putctlfont(CFont *c)
{
	putcacheitem(c);
}

int
namectlfont(Font *font, int8_t *name)
{
	return namecacheitem(&fontcache, font, name);
}

int
freectlfont(int8_t *name)
{
	return freecacheitem(&fontcache, name);
}

void
_setctlfont(Control *c, CFont **cp, int8_t *s)
{
	setcacheitemptr(&fontcache, c, cp, s);
}
