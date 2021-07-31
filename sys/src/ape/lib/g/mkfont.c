#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libg.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef wchar_t Rune;

enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80,		/* decoding error in UTF */
};

/*
 * Cobble fake font using existing subfont
 */
Font*
mkfont(Subfont *subfont, Rune min)
{
	Font *font;
	uchar *gbuf;
	Cachefont *c;

	font = malloc(sizeof(Font));
	if(font == 0)
		return 0;
	memset(font, 0, sizeof(Font));
	font->name = gstrdup("<synthetic>");
	font->ncache = NFCACHE+NFLOOK;
	font->nsubf = NFSUBF;
	font->cache = malloc(font->ncache * sizeof(font->cache[0]));
	font->subf = malloc(font->nsubf * sizeof(font->subf[0]));
	if(font->name==0 || font->cache==0 || font->subf==0){
    Err1:
		free(font->name);
		free(font->cache);
		free(font->subf);
		free(font->sub);
		free(font);
		return 0;
	}
	memset(font->cache, 0, font->ncache*sizeof(font->cache[0]));
	memset(font->subf, 0, font->nsubf*sizeof(font->subf[0]));
	font->height = subfont->height;
	font->ascent = subfont->ascent;
	font->ldepth = screen.ldepth;
	gbuf = bneed(7);
	gbuf[0] = 'n';
	gbuf[1] = font->height;
	gbuf[2] = font->ascent;
	BPSHORT(gbuf+3, font->ldepth);
	BPSHORT(gbuf+5, font->ncache);
	if(!bwrite())
		goto Err1;
	if(read(bitbltfd, (char *)gbuf, 3)!=3 || gbuf[0]!='N')
		goto Err1;
	font->id = gbuf[1] | (gbuf[2]<<8);
	font->age = 1;
	font->sub = malloc(sizeof(Cachefont*));
	if(font->sub == 0){
Err2:
		gbuf = bneed(3);
		gbuf[0] = 'h';
		gbuf[1] = font->height;
		gbuf[2] = font->ascent;
		BPSHORT(gbuf+1, font->id);
		goto Err1;
	}
	c = malloc(sizeof(Cachefont));
	if(c == 0)
		goto Err2;
	font->nsub = 1;
	font->sub[0] = c;
	c->min = min;
	c->max = min+subfont->n-1;
	c->offset = 0;
	c->name = 0;	/* noticed by freeup() and agefont() */
	c->abs = 1;
	font->subf[0].age = 0;
	font->subf[0].cf = c;
	font->subf[0].f = subfont;
	return font;
}
