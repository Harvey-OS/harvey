/*
 * dump a 9660 cd image for a little while.
 * for debugging.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>

Biobuf *b;

#pragma varargck type "s" uchar*
#pragma varargck type "L" uchar*
#pragma varargck type "B" uchar*
#pragma varargck type "N" uchar*
#pragma varargck type "T" uchar*
#pragma varargck type "D" uchar*

typedef struct Voldesc Voldesc;
struct Voldesc {
	uchar	magic[8];	/* 0x01, "CD001", 0x01, 0x00 */
	uchar	systemid[32];	/* system identifier */
	uchar	volumeid[32];	/* volume identifier */
	uchar	unused[8];	/* character set in secondary desc */
	uchar	volsize[8];	/* volume size */
	uchar	charset[32];
	uchar	volsetsize[4];	/* volume set size = 1 */
	uchar	volseqnum[4];	/* volume sequence number = 1 */
	uchar	blocksize[4];	/* logical block size */
	uchar	pathsize[8];	/* path table size */
	uchar	lpathloc[4];	/* Lpath */
	uchar	olpathloc[4];	/* optional Lpath */
	uchar	mpathloc[4];	/* Mpath */
	uchar	ompathloc[4];	/* optional Mpath */
	uchar	rootdir[34];	/* root directory */
	uchar	volsetid[128];	/* volume set identifier */
	uchar	publisher[128];
	uchar	prepid[128];	/* data preparer identifier */
	uchar	applid[128];	/* application identifier */
	uchar	notice[37];	/* copyright notice file */
	uchar	abstract[37];	/* abstract file */
	uchar	biblio[37];	/* bibliographic file */
	uchar	cdate[17];	/* creation date */
	uchar	mdate[17];	/* modification date */
	uchar	xdate[17];	/* expiration date */
	uchar	edate[17];	/* effective date */
	uchar	fsvers;		/* file system version = 1 */
};

void
dumpbootvol(void *a)
{
	Voldesc *v;

	v = a;
	print("magic %.2ux %.5s %.2ux %2ux\n",
		v->magic[0], v->magic+1, v->magic[6], v->magic[7]);
	if(v->magic[0] == 0xFF)
		return;

	print("system %.32T\n", v->systemid);
	print("volume %.32T\n", v->volumeid);
	print("volume size %.4N\n", v->volsize);
	print("charset %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux %.2ux\n",
		v->charset[0], v->charset[1], v->charset[2], v->charset[3],
		v->charset[4], v->charset[5], v->charset[6], v->charset[7]);
	print("volume set size %.2N\n", v->volsetsize);
	print("volume sequence number %.2N\n", v->volseqnum);
	print("logical block size %.2N\n", v->blocksize);
	print("path size %.4L\n", v->pathsize);
	print("lpath loc %.4L\n", v->lpathloc);
	print("opt lpath loc %.4L\n", v->olpathloc);
	print("mpath loc %.4B\n", v->mpathloc);
	print("opt mpath loc %.4B\n", v->ompathloc);
	print("rootdir %D\n", v->rootdir);
	print("volume set identifier %.128T\n", v->volsetid);
	print("publisher %.128T\n", v->publisher);
	print("preparer %.128T\n", v->prepid);
	print("application %.128T\n", v->applid);
	print("notice %.37T\n", v->notice);
	print("abstract %.37T\n", v->abstract);
	print("biblio %.37T\n", v->biblio);
	print("creation date %.17s\n", v->cdate);
	print("modification date %.17s\n", v->mdate);
	print("expiration date %.17s\n", v->xdate);
	print("effective date %.17s\n", v->edate);
	print("fs version %d\n", v->fsvers);
}

typedef struct Cdir Cdir;
struct Cdir {
	uchar	len;
	uchar	xlen;
	uchar	dloc[8];
	uchar	dlen[8];
	uchar	date[7];
	uchar	flags;
	uchar	unitsize;
	uchar	gapsize;
	uchar	volseqnum[4];
	uchar	namelen;
	uchar	name[1];	/* chumminess */
};
#pragma varargck type "D" Cdir*

int
Dfmt(Fmt *fmt)
{
	char buf[128];
	Cdir *c;

	c = va_arg(fmt->args, Cdir*);
	if(c->namelen == 1 && c->name[0] == '\0' || c->name[0] == '\001') {
		snprint(buf, sizeof buf, ".%s dloc %.4N dlen %.4N",
			c->name[0] ? "." : "", c->dloc, c->dlen);
	} else {
		snprint(buf, sizeof buf, "%.*T dloc %.4N dlen %.4N", c->namelen, c->name,
			c->dloc, c->dlen);
	}
	fmtstrcpy(fmt, buf);
	return 0;
}

typedef struct Path Path;
struct Path {
	uchar	namelen;
	uchar	xlen;
	uchar	dloc[4];
	uchar	parent[2];
	uchar	name[1];	/* chumminess */
};
#pragma varargck type "P" Path*

char longc, shortc;
void
bigend(void)
{
	longc = 'B';
}

void
littleend(void)
{
	longc = 'L';
}

int
Pfmt(Fmt *fmt)
{
	char xfmt[128], buf[128];
	Path *p;

	p = va_arg(fmt->args, Path*);
	sprint(xfmt, "data=%%.4%c up=%%.2%c name=%%.*T (%%d)", longc, longc);
	snprint(buf, sizeof buf, xfmt, p->dloc, p->parent, p->namelen, p->name, p->namelen);
	fmtstrcpy(fmt, buf);
	return 0;
}

ulong
big(void *a, int n)
{
	uchar *p;
	ulong v;
	int i;

	p = a;
	v = 0;
	for(i=0; i<n; i++)
		v = (v<<8) | *p++;
	return v;
}

ulong
little(void *a, int n)
{
	uchar *p;
	ulong v;
	int i;

	p = a;
	v = 0;
	for(i=0; i<n; i++)
		v |= (*p++<<(i*8));
	return v;
}

/* numbers in big or little endian. */
int
BLfmt(Fmt *fmt)
{
	ulong v;
	uchar *p;
	char buf[20];

	p = va_arg(fmt->args, uchar*);

	if(!(fmt->flags&FmtPrec)) {
		fmtstrcpy(fmt, "*BL*");
		return 0;
	}

	if(fmt->r == 'B')
		v = big(p, fmt->prec);
	else
		v = little(p, fmt->prec);

	sprint(buf, "0x%.*lux", fmt->prec*2, v);
	fmt->flags &= ~FmtPrec;
	fmtstrcpy(fmt, buf);
	return 0;
}

/* numbers in both little and big endian */
int
Nfmt(Fmt *fmt)
{
	char buf[100];
	uchar *p;

	p = va_arg(fmt->args, uchar*);

	sprint(buf, "%.*L %.*B", fmt->prec, p, fmt->prec, p+fmt->prec);
	fmt->flags &= ~FmtPrec;
	fmtstrcpy(fmt, buf);
	return 0;
}

int
asciiTfmt(Fmt *fmt)
{
	char *p, buf[256];
	int i;

	p = va_arg(fmt->args, char*);
	for(i=0; i<fmt->prec; i++)
		buf[i] = *p++;
	buf[i] = '\0';
	for(p=buf+strlen(buf); p>buf && p[-1]==' '; p--)
		;
	p[0] = '\0';
	fmt->flags &= ~FmtPrec;
	fmtstrcpy(fmt, buf);
	return 0;
}

int
runeTfmt(Fmt *fmt)
{
	Rune buf[256], *r;
	int i;
	uchar *p;

	p = va_arg(fmt->args, uchar*);
	for(i=0; i*2+2<=fmt->prec; i++, p+=2)
		buf[i] = (p[0]<<8)|p[1];
	buf[i] = L'\0';
	for(r=buf+i; r>buf && r[-1]==L' '; r--)
		;
	r[0] = L'\0';
	fmt->flags &= ~FmtPrec;
	return fmtprint(fmt, "%S", buf);
}

void
ascii(void)
{
	fmtinstall('T', asciiTfmt);
}

void
joliet(void)
{
	fmtinstall('T', runeTfmt);
}

void
getsect(uchar *buf, int n)
{
	if(Bseek(b, n*2048, 0) != n*2048 || Bread(b, buf, 2048) != 2048)
		sysfatal("reading block %ux", n);
}

void
pathtable(Voldesc *v, int islittle)
{
	int i, j, n, sz, addr;
	ulong (*word)(void*, int);
	uchar x[2048], *p, *ep;
	Path *pt;

	print(">>> entire %s path table\n", islittle ? "little" : "big");
	littleend();
	if(islittle) {
		littleend();
		word = little;
	}else{
		bigend();
		word = big;
	}
	sz = little(v->pathsize, 4);	/* little, not word */
	addr = word(islittle ? v->lpathloc : v->mpathloc, 4);
	j = 0;
	n = 1;
	while(sz > 0){
		getsect(x, addr);
		p = x;
		ep = x+sz;
		if(ep > x+2048)
			ep = x+2048;
		for(i=0; p<ep; i++) {
			pt = (Path*)p;
			if(pt->namelen==0)
				break;
			print("0x%.4x %4d+%-4ld %P\n", n, j, p-x, pt);
			n++;
			p += 8+pt->namelen+(pt->namelen&1);
		}
		sz -= 2048;
		addr++;
		j++;
	}
}

void
dump(void *root)
{
	Voldesc *v;
	Cdir *c;
	long rootdirloc;
	uchar x[2048];
	int i;
	uchar *p;

	dumpbootvol(root);
	v = (Voldesc*)root;
	c = (Cdir*)v->rootdir;
	rootdirloc = little(c->dloc, 4);
	print(">>> sed 5q root directory\n");
	getsect(x, rootdirloc);
	p = x;
	for(i=0; i<5 && (p-x)<little(c->dlen, 4); i++) {
		print("%D\n", p);
		p += ((Cdir*)p)->len;
	}

	pathtable(v, 1);
	pathtable(v, 0);
}

void
main(int argc, char **argv)
{
	uchar root[2048], jroot[2048];

	if(argc != 2)
		sysfatal("usage: %s file", argv[0]);

	b = Bopen(argv[1], OREAD);
	if(b == nil)
		sysfatal("bopen %r");

	fmtinstall('L', BLfmt);
	fmtinstall('B', BLfmt);
	fmtinstall('N', Nfmt);
	fmtinstall('D', Dfmt);
	fmtinstall('P', Pfmt);

	getsect(root, 16);
	ascii();
	dump(root);

	getsect(jroot, 17);
	joliet();
	dump(jroot);
	exits(0);
}
