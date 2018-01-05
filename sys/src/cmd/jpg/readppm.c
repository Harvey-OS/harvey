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
#include <bio.h>
#include <draw.h>
#include <ctype.h>
#include "imagefile.h"

Rawimage *readppm(Biobuf*, Rawimage*);

/*
 * fetch a non-comment character.
 */
static
int
Bgetch(Biobufhdr *b)
{
	int c;

	for(;;) {
		c = Bgetc(b);
		if(c == '#') {
			while((c = Bgetc(b)) != Beof && c != '\n')
				;
		}
		return c;
	}
}

/*
 * fetch a nonnegative decimal integer.
 */
static
int
Bgetint(Biobufhdr *b)
{
	int c;
	int i;

	while((c = Bgetch(b)) != Beof && !isdigit(c))
		;
	if(c == Beof)
		return -1;

	i = 0;
	do {
		i = i*10 + (c-'0');
	} while((c = Bgetch(b)) != Beof && isdigit(c));

	return i;
}

static
int
Bgetdecimalbit(Biobufhdr *b)
{
	int c;
	while((c = Bgetch(b)) != Beof && c != '0' && c != '1')
		;
	if(c == Beof)
		return -1;
	return c == '1';
}

static int bitc, nbit;

static
int
Bgetbit(Biobufhdr *b)
{
	if(nbit == 0) {
		nbit = 8;
		bitc = Bgetc(b);
		if(bitc == -1)
			return -1;
	}
	nbit--;
	return (bitc >> nbit) & 0x1;
}

static
void
Bflushbit(Biobufhdr*)
{
	nbit = 0;
}


Rawimage**
readpixmap(int fd, int colorspace)
{
	Rawimage **array, *a;
	Biobuf b;
	char buf[ERRMAX];
	int i;
	char *e;

	USED(colorspace);
	if(Binit(&b, fd, OREAD) < 0)
		return nil;

	werrstr("");
	e = "out of memory";
	if((array = malloc(sizeof *array)) == nil)
		goto Error;
	if((array[0] = malloc(sizeof *array[0])) == nil)
		goto Error;
	memset(array[0], 0, sizeof *array[0]);

	for(i=0; i<3; i++)
		array[0]->chans[i] = nil;

	e = "bad file format";
	switch(Bgetc(&b)) {
	case 'P':
		Bungetc(&b);
		a = readppm(&b, array[0]);
		break;
	default:
		a = nil;
		break;
	}
	if(a == nil)
		goto Error;
	array[0] = a;

	return array;

Error:
	if(array)
		free(array[0]);
	free(array);

	errstr(buf, sizeof buf);
	if(buf[0] == 0)
		strcpy(buf, e);
	errstr(buf, sizeof buf);

	return nil;
}

typedef struct Pix	Pix;
struct Pix {
	char magic;
	int	maxcol;
	int	(*fetch)(Biobufhdr*);
	int	nchan;
	int	chandesc;
	int	invert;
	void	(*flush)(Biobufhdr*);
};

static Pix pix[] = {
	{ '1', 1, Bgetdecimalbit, 1, CY, 1, nil },	/* portable bitmap */
	{ '4', 1, Bgetbit, 1, CY, 1, Bflushbit },	/* raw portable bitmap */
	{ '2', 0, Bgetint, 1, CY, 0, nil },	/* portable greymap */
	{ '5', 0, Bgetc, 1, CY, 0, nil },	/* raw portable greymap */
	{ '3', 0, Bgetint, 3, CRGB, 0, nil },	/* portable pixmap */
	{ '6', 0, Bgetc, 3, CRGB, 0, nil },	/* raw portable pixmap */
	{ 0 },
};

Rawimage*
readppm(Biobuf *b, Rawimage *a)
{
	int i, ch, wid, ht, r, c;
	int maxcol, nchan, invert;
	int (*fetch)(Biobufhdr*);
	uchar *rgb[3];
	char buf[ERRMAX];
	char *e;
	Pix *p;

	e = "bad file format";
	if(Bgetc(b) != 'P')
		goto Error;

	c = Bgetc(b);
	for(p=pix; p->magic; p++)
		if(p->magic == c)
			break;
	if(p->magic == 0)
		goto Error;


	wid = Bgetint(b);
	ht = Bgetint(b);
	if(wid <= 0 || ht <= 0)
		goto Error;
	a->r = Rect(0,0,wid,ht);

	maxcol = p->maxcol;
	if(maxcol == 0) {
		maxcol = Bgetint(b);
		if(maxcol <= 0)
			goto Error;
	}

	e = "out of memory";
	for(i=0; i<p->nchan; i++)
		if((rgb[i] = a->chans[i] = malloc(wid*ht)) == nil)
			goto Error;
	a->nchans = p->nchan;
	a->chanlen = wid*ht;
	a->chandesc = p->chandesc;

	e = "error reading file";

	fetch = p->fetch;
	nchan = p->nchan;
	invert = p->invert;
	for(r=0; r<ht; r++) {
		for(c=0; c<wid; c++) {
			for(i=0; i<nchan; i++) {
				if((ch = (*fetch)(b)) < 0)
					goto Error;
				if(invert)
					ch = maxcol - ch;
				*rgb[i]++ = (ch * 255)/maxcol;
			}
		}
		if(p->flush)
			(*p->flush)(b);
	}

	return a;

Error:
	errstr(buf, sizeof buf);
	if(buf[0] == 0)
		strcpy(buf, e);
	errstr(buf, sizeof buf);

	for(i=0; i<3; i++)
		free(a->chans[i]);
	free(a->cmap);
	return nil;
}
