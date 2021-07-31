#include	<u.h>
#include	<libc.h>
#include	<stdio.h>
#include	<cda/fizz.h>
#include	"host.h"
#include	"proto.h"

static short nindex;
static short nchips;
static short nsigs;
extern char mydir[];
void typefn2(Type *);
static void sigfn1(Signal *);
static void sigfn2(Signal *);
void chipfn1(Chip *);
void chipfn2(Chip *);
void sendchip(Chip *, int );
void chipfn3(Chip *);
void typefn1(Type *);
static void sigfn3(Signal *);
Rectangle boundr(void);
static void allocsig(Chip *);
static void addsig(Signal *);
void readin(char **);
void chdump(Chip *c);

void
chipfn1(Chip *c)
{
	char buf[64];

	sprint(buf, "%d", ++nchips);
	(void)symlook(f_strdup(buf), S_CHID, (void *)c);
}

void
chipfn2(Chip *c)
{
	c->nam = nindex;
	nindex += strlen(c->name)+1;
	sendchip(c, -1);
}

void
sendchip(Chip *c, int n)
{
	if(n >= 0){
		put1(CHIP);
		putn(n);
	}
	put1(c->flags);
	putr(c->r);
	putp(c->pt);
/*if (dbfp) fprint(dbfp, "%s@%d/%d, %r\n", c->name, c->pt.x, c->pt.y, c->r);/**/
	putn((int)c->nam);
	putn((int)c->type->nam);
}

void
chipfn3(Chip *c)
{
	putstr(c->name);
}

void
typefn1(Type *t)
{
	t->nam = nindex;
	nindex += strlen(t->name)+1;
}

void
typefn2(Type *t)
{
	putstr(t->name);
}

static void
sigfn1(Signal *s)
{
	char buf[8];

	if(s->type != NORMSIG)
		return;
	s->nam = nindex;
	nindex += strlen(s->name)+1;
	s->num = nsigs++;
	sprint(buf, "%d", s->num);
	(void)symlook(f_strdup(buf), S_SID, (void *)s);
	s->done = 0;
}

static void
sigfn2(Signal *s)
{
	if(s->type != NORMSIG)
		return;
	putstr(s->name);
}

static void
sigfn3(Signal *s)
{
	if(s->type != NORMSIG)
		return;
	putn((int)s->nam);
	putn(s->n);
}

#define MAX(a,b) ((a) > (b)) ? (a) : (b)
Rectangle
boundr(void)
{
	Rectangle r;

if (dbfp) fprintf(dbfp, "Bounds: (%d, %d) - (%d, %d)\n", b.prect.min.x, b.prect.min.y,
			b.prect.max.x, b.prect.max.y);
	r.min.x = MAX(0, b.prect.min.x-INCH/2);
	r.min.y = MAX(0, b.prect.min.y-INCH/2);
	r.max.x = MAX(r.min.x+6*INCH, b.prect.max.x+INCH/2);
	r.max.y = MAX(r.min.y+6*INCH, b.prect.max.y+INCH/2);
	return(r);
}

static void
allocsig(Chip *c)
{
	Signal **s;

	s = (Signal **)f_malloc(sizeof(Signal *)*(long)(c->npins+1));
	symlook(c->name, S_CSMAP, (void *)s);
	*s = 0;
}

static void
addsig(Signal *s)
{
	register Signal **ss;
	register i;

	if(s->type != NORMSIG)
		return;
	for(i = 0; i < s->n; i++){
		ss = (Signal **)symlook(s->coords[i].chip, S_CSMAP, (void *)0);
		while(*ss)
			if(s == *ss++)
				break;
		if(*ss == 0){
			*ss++ = s;
			*ss = 0;
		}
	}
		
}

void
readin(char **argv)
{
	int n, fd;
	Pinhole *p;
	Rectangle r;
	char buf[100];

if (dbfp) fprintf(dbfp, "readin:");
	sprint(buf, "%s/fizz.errs", mydir);
	fd = create(buf, OWRITE, 0666);
	dup(fd, 2);
	close(fd);
	fizzinit();
	f_init(&b);
	f_b = &b;
	while(*argv){
		if(**argv == '/') sprint(buf, "%s", *argv++);
		else sprint(buf, "%s/%s", mydir, *argv++);
if (dbfp) fprintf(dbfp, "\n\t%s", buf);
		if(n = f_crack(buf, &b)){
			fprint(2, "%s: %d errors\n", buf, n);
			put1(ERROR);
			putstr("input errors: ");
			putstr(buf);
		}
	}
if (dbfp) fprintf(dbfp, "\n");
	if(fizzplace() < 0){
		put1(ERROR);
		putstr("input errors");
	}
	if(fizzprewrap()){
		put1(ERROR);
		putstr("input errors");
	}
	cutout(&b);
	fizzplane(&b);
	put1(BBOUNDS);
	putr(boundr());
	put1(BOARDNAME);
	putstr(b.name? b.name : "");
	for(p = b.pinholes, n = 0; p; p = p->next)
		n++;
	put1(PINHOLES);
	putn(n);
	for(p = b.pinholes; p; p = p->next){
		r.min = p->o;
		r.max.x = p->len.x + p->o.x;
		r.max.y = p->len.y + p->o.y;
		putr(r);
		putp(p->sp);
	}
	pkgclass((void *)dbfp);
	/* set up chip to signals map */
	csmap();
	nindex = 1;		/* initial null */
	nchips = 0;
	nsigs = 0;
	symtraverse(S_TYPE, typefn1);
	symtraverse(S_CHIP, chipfn1);
	put1(CHIPS);
	putn(nchips);
	symtraverse(S_CHIP, chipfn2);
	symtraverse(S_SIGNAL, sigfn1);
	put1(CHIPSTR);
	putn(nindex);
	put1(0);		/* initial null */
	symtraverse(S_TYPE, typefn2);
	symtraverse(S_CHIP, chipfn3);
	symtraverse(S_SIGNAL, sigfn2);
	put1(SIGS);
	putn(nsigs);
	symtraverse(S_SIGNAL, sigfn3);
	if(b.nplanes){
		put1(PLANES);
		putn(b.nplanes);
if (dbfp) fprintf(dbfp, "%d planes\n", b.nplanes);
		for(n = 0; n < b.nplanes; n++){
			putn(layerof(b.planes[n].layer));
			putr(b.planes[n].r);
		}
	}
if (dbfp) fprintf(dbfp, "%d keepouts\n", b.nkeepouts);
	if(b.nkeepouts){
		put1(KEEPOUTS);
		putn(b.nkeepouts);
if (dbfp) fprintf(dbfp, "%d keepouts\n", b.nkeepouts);
		for(n = 0; n < b.nkeepouts; n++){
			putn(layerof(b.keepouts[n].layer));
			putr(b.keepouts[n].r);
		}
	}
	put1(DRAW);
	restartimp();
}

void
rfile(char *s)
{
	char *ptrs[100], *p;
	int i, n;
	n = sizeof ptrs/sizeof ptrs[0];
	for(p = s, i = 0; ; p++) {
		if((*p == 0) || (*p == ' ') || (*p == '\t')) {
			ptrs[i++] = s;
			if(*p == 0) break;
			*p++ = 0;
if (dbfp) fprintf(dbfp, "%s\n", s);
			while((*p == ' ') || (*p == '\t')) ++p;
			if((i >= n) || (*p == 0)) break;
			s = p;
		}
	}
	ptrs[i] = (char *) 0;
if (dbfp) while(--i >= 0) fprintf(dbfp, "%s\n", ptrs[i]);
	readin(ptrs);
}

static FILE *ofp;

void
chdump(Chip *c)
{
	fprintf(ofp, "\t%s %d/%d %d %d\n", c->name, c->pt.x, c->pt.y, c->rotation, c->flags);
}

void
wfile(char *s)
{
	char buf1[256];
	char buf[BUFSIZ];

	sprint(buf, "%s/%s", mydir, s);
if (dbfp) fprintf(dbfp, "writing file '%s'\n", s);
	if((ofp = fopen(buf, "w")) == 0){
		perror(buf);
		put1(ERROR);
		putstr("creat failed");
		fflush(stdout);
		return;
	}
	fprintf(ofp, "Positions{\n");
	symtraverse(S_CHIP, chdump);
	fprintf(ofp, "}\n");
	fflush(ofp);
	fclose(ofp);
}

Chip *
cofn(int n)
{
	register Chip *c;
	char buf[64];

	sprint(buf, "%d", n);
	c = (Chip *)symlook(buf, S_CHID, (void *)0);
	if(c == 0){
		sprint(buf, "cofn(%d) botch", n);
		quit();
	}
	return(c);
}
