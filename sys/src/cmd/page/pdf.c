/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pdf.c
 *
 * pdf file support for page
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <bio.h>
#include "page.h"

typedef struct PDFInfo	PDFInfo;
struct PDFInfo {
	GSInfo;
	Rectangle *pagebbox;
};

static Image*	pdfdrawpage(Document *d, int page);
static char*	pdfpagename(Document*, int);

char *pdfprolog =
#include "pdfprolog.c"
	;

Rectangle
pdfbbox(GSInfo *gs)
{
	char *p;
	char *f[4];
	Rectangle r;

	r = Rect(0,0,0,0);
	waitgs(gs);
	gscmd(gs, "/CropBox knownoget {} {[0 0 0 0]} ifelse PAGE==\n");
	p = Brdline(&gs->gsrd, '\n');
	p[Blinelen(&gs->gsrd)-1] ='\0';
	if(p[0] != '[')
		return r;
	if(tokenize(p+1, f, 4) != 4)
		return r;
	r = Rect(atoi(f[0]), atoi(f[1]), atoi(f[2]), atoi(f[3]));
	waitgs(gs);
	return r;
}

Document*
initpdf(Biobuf *b, int argc, char **argv, uint8_t *buf, int nbuf)
{
	Document *d;
	PDFInfo *pdf;
	char *p;
	char *fn;
	char fdbuf[20];
	int fd;
	int i, npage;
	Rectangle bbox;

	if(argc > 1) {
		fprint(2, "can only view one pdf file at a time\n");
		return nil;
	}

	fprint(2, "reading through pdf...\n");
	if(b == nil){	/* standard input; spool to disk (ouch) */
		fd = spooltodisk(buf, nbuf, &fn);
		sprint(fdbuf, "/fd/%d", fd);
		b = Bopen(fdbuf, OREAD);
		if(b == nil){
			fprint(2, "cannot open disk spool file\n");
			wexits("Bopen temp");
		}
	}else
		fn = argv[0];

	/* sanity check */
	Bseek(b, 0, 0);
	if(!(p = Brdline(b, '\n')) && !(p = Brdline(b, '\r'))) {
		fprint(2, "cannot find end of first line\n");
		wexits("initps");
	}
	if(strncmp(p, "%PDF-", 5) != 0) {
		werrstr("not pdf");
		return nil;
	}

	/* setup structures so one free suffices */
	p = emalloc(sizeof(*d) + sizeof(*pdf));
	d = (Document*) p;
	p += sizeof(*d);
	pdf = (PDFInfo*) p;

	d->extra = pdf;
	d->b = b;
	d->drawpage = pdfdrawpage;
	d->pagename = pdfpagename;
	d->fwdonly = 0;

	if(spawngs(pdf, "-dDELAYSAFER") < 0)
		return nil;

	gscmd(pdf, "%s", pdfprolog);
	waitgs(pdf);

	setdim(pdf, Rect(0,0,0,0), ppi, 0);
	gscmd(pdf, "(%s) (r) file { DELAYSAFER { .setsafe } if } stopped pop pdfopen begin\n", fn);
	gscmd(pdf, "pdfpagecount PAGE==\n");
	p = Brdline(&pdf->gsrd, '\n');
	npage = (p != nil? atoi(p): 0);
	if(npage < 1) {
		fprint(2, "no pages?\n");
		return nil;
	}
	d->npage = npage;
	d->docname = argv[0];

	gscmd(pdf, "Trailer\n");
	bbox = pdfbbox(pdf);

	pdf->pagebbox = emalloc(sizeof(Rectangle)*npage);
	for(i=0; i<npage; i++) {
		gscmd(pdf, "%d pdfgetpage\n", i+1);
		pdf->pagebbox[i] = pdfbbox(pdf);
		if(Dx(pdf->pagebbox[i]) <= 0)
			pdf->pagebbox[i] = bbox;
	}
	return d;
}

static Image*
pdfdrawpage(Document *doc, int page)
{
	PDFInfo *pdf = doc->extra;
	Image *im;

	gscmd(pdf, "%d DoPDFPage\n", page+1);
	im = readimage(display, pdf->gsdfd, 0);
	if(im == nil) {
		fprint(2, "fatal: readimage error %r\n");
		wexits("readimage");
	}
	waitgs(pdf);
	return im;
}

static char*
pdfpagename(Document*, int page)
{
	static char str[15];
	sprint(str, "p %d", page+1);
	return str;
}
