#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libg.h>
#include "sky.h"
#include "strings.c"

enum
{
	NNGCrec=8163,	/* number of records in the NGC catalog */
	NNGC=7840,	/* number of NGC numbers */
	NMrec=127,	/* number of M records */
	NM=110,		/* number of M numbers */
	NName=827,	/* number of prose names */
	NBayer=1517,	/* number of bayer entries */
	NSAO=258998,	/* number of SAO stars */
	MAXcon=1932,	/* maximum number of patches in a constellation */
	Ncon=88,	/* number of constellations */
	Npatch=92053,	/* highest patch number */
};

char *typetostr[] =
{
	/* List */	"can't happen",
	/* Patch */	"can't happen",
	/* SAO */	"can't happen",
	/* NGC */	"can't happen",
	/* M */		"can't happen",
	/* Constel */	"can't happen",
	/* Nonstar */	"can't happen",
	/* Star */	"star",
	/* Galaxy */	"eg",
	/* PlanetaryN */"pn",
	/* OpenCl */	"oc",
	/* GlobularCl */"gc",
	/* DiffuseN */	"dn",
	/* NebularCl */	"nc",
	/* Nonexistent */"xx",
	/* Unknown */	"??",
	/* LMC */	"can't happen",
	/* SMC */	"can't happen",
};

NGCindexrec	ngcindex[NNGCrec];
NGCindexrec	mindex[NMrec];
Namerec		name[NName];
Bayerec		bayer[NBayer];
long		con[MAXcon];
ushort		conindex[Ncon+1];
long		patchaddr[Npatch+1];

Record	*rec;
Record	*orec;
Record	*cur;

char	*dir=DIR;
int	saodb;
int	ngcdb;
int	ngcindexdb;
int	mindexdb;
int	namedb;
int	bayerdb;
int	condb;
int	conindexdb;
int	patchdb;
char	parsed[3];
long	nrec;
long	nreca;
long	norec;
long	noreca;

Bitmap	*lightgrey;
static	uchar lightgreybits[] = {
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44,
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44,
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44,
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44,
};

Biobuf	bin;
Biobuf	bout;

main(int argc, char *argv[])
{
	char *line;

	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);
	if(argc != 1)
		dir = argv[1];
	while(line = Brdline(&bin, '\n')){
		line[BLINELEN(&bin)-1] = 0;
		lookup(line, 1);
		Bflush(&bout);
	}
	return 0;
}

void
reset(void)
{
	nrec = 0;
	cur = rec;
}

void
grow(void)
{
	nrec++;
	if(nreca < nrec){
		nreca = nrec+50;
		rec = realloc(rec, nreca*sizeof(Record));
		if(rec == 0){
			fprint(2, "scat: realloc fails\n");
			exits("realloc");
		}
	}
	cur = rec+nrec-1;
}

void
copy(void)
{
	if(noreca < nreca){
		noreca = nreca;
		orec = realloc(orec, nreca*sizeof(Record));
		if(orec == 0){
			fprint(2, "scat: realloc fails\n");
			exits("realloc");
		}
	}
	memmove(orec, rec, nrec*sizeof(Record));
	norec = nrec;
}

int
eopen(char *s)
{
	char buf[128];
	int f;

	sprint(buf, "%s/%s.scat", dir, s);
	f = open(buf, 0);
	if(f<0){
		fprint(2, "scat: can't open %s\n", buf);
		exits("open");
	}
	return f;
}


void
Eread(int f, char *name, void *addr, long n)
{
	if(read(f, addr, n) != n){	/* BUG! */
		fprint(2, "scat: read error on %s\n", name);
		exits("read");
	}
}

char*
skipbl(char *s)
{
	while(*s!=0 && (*s==' ' || *s=='\t'))
		s++;
	return s;
}

char*
skipstr(char *s, char *t)
{
	while(*s && *s==*t)
		s++, t++;
	return skipbl(s);
}

/* produce little-endian long at address l */
long
Long(long *l)
{
	uchar *p;

	p = (uchar*)l;
	return (long)p[0]|((long)p[1]<<8)|((long)p[2]<<16)|((long)p[3]<<24);
}

/* produce little-endian long at address l */
int
Short(short *s)
{
	uchar *p;

	p = (uchar*)s;
	return p[0]|(p[1]<<8);
}

void
saoopen(void)
{
	long i;

	if(saodb == 0){
		saodb = eopen("sao");

		namedb = eopen("name");
		Eread(namedb, "name", name, sizeof name);
		close(namedb);
		for(i=0; i<NName; i++)
			name[i].sao = Long(&name[i].sao);

		bayerdb = eopen("bayer");
		Eread(bayerdb, "bayer", bayer, sizeof bayer);
		close(bayerdb);
		for(i=0; i<NBayer; i++)
			bayer[i].sao = Long(&bayer[i].sao);
	}
}

void
ngcopen(void)
{
	long i;

	if(ngcdb == 0){
		ngcdb = eopen("ngc");
		ngcindexdb = eopen("ngcindex");
		Eread(ngcindexdb, "ngcindex", ngcindex, sizeof ngcindex);
		close(ngcindexdb);
		for(i=0; i<NNGCrec; i++)
			ngcindex[i].ngc = Short(&ngcindex[i].ngc);
	}
}

void
patchopen(void)
{
	Biobuf *b;
	long l, m;
	char buf[100];

	if(patchdb == 0){
		patchdb = eopen("patch");
		sprint(buf, "%s/patchindex.scat", dir);
		b = Bopen(buf, OREAD);
		if(b == 0){
			fprint(2, "can't open %s\n", buf);
			exits("open");
		}
		for(m=0,l=0; l<=Npatch; l++)
			patchaddr[l] = m += Bgetc(b)*4;
		Bclose(b);
	}
}

void
mopen(void)
{
	int i;

	if(mindexdb == 0){
		mindexdb = eopen("mindex");
		Eread(mindexdb, "mindex", mindex, sizeof mindex);
		close(mindexdb);
		for(i=0; i<NMrec; i++)
			mindex[i].ngc = Short(&mindex[i].ngc);
	}
}

void
constelopen(void)
{
	int i;

	if(condb == 0){
		condb = eopen("con");
		conindexdb = eopen("conindex");
		Eread(conindexdb, "conindex", conindex, sizeof conindex);
		close(conindexdb);
		for(i=0; i<Ncon+1; i++)
			conindex[i] = Short((short*)&conindex[i]);
	}
}

void
plotopen(void)
{
	static int init;

	if(init)
		return;
	init = 1;
	binit(0, 0, "scat");
	lightgrey = balloc(Rect(0, 0, 16, 16), 0);
	wrbitmap(lightgrey, 0, 16, lightgreybits);
}

void
lowercase(char *s)
{
	for(; *s; s++)
		if('A'<=*s && *s<='Z')
			*s += 'a'-'A';
}

int
loadngc(long index, int tag)
{
	long j;

	ngcopen();
	j = index-1;
	while(ngcindex[j].ngc != index)
		j += index-ngcindex[j].ngc;
	while(ngcindex[j].ngc==index && ngcindex[j].tag!=tag)
		j++;
	if(ngcindex[j].ngc != index)
		return 0;
	grow();
	cur->type = NGC;
	cur->index = index;
	seek(ngcdb, j*sizeof(NGCrec), 0);
	Eread(ngcdb, "ngc", &cur->ngc, sizeof(NGCrec));
	cur->ngc.ngc = Short(&cur->ngc.ngc);
	cur->ngc.ra = Long(&cur->ngc.ra);
	cur->ngc.dec = Long(&cur->ngc.dec);
	cur->ngc.glat = Long(&cur->ngc.glat);
	cur->ngc.glong = Long(&cur->ngc.glong);
	cur->ngc.mag = Short(&cur->ngc.mag);
	return 1;
}

int
loadsao(int index)
{
	if(index<=0 || index>NSAO)
		return 0;
	saoopen();
	grow();
	cur->type = SAO;
	cur->index = index;
	seek(saodb, (index-1)*sizeof(SAOrec), 0);
	Eread(saodb, "sao", &cur->sao, sizeof(SAOrec));
	cur->sao.ra = Long(&cur->sao.ra);
	cur->sao.dec = Long(&cur->sao.dec);
	cur->sao.dra = Long(&cur->sao.dra);
	cur->sao.ddec = Long(&cur->sao.ddec);
	cur->sao.mag = Short(&cur->sao.mag);
	cur->sao.mpg = Short(&cur->sao.mpg);
	cur->sao.hd = Long(&cur->sao.hd);
	return 1;
}

int
loadpatch(long index)
{
	int i;

	patchopen();
	if(index<=0 || index>Npatch)
		return 0;
	grow();
	cur->type = Patch;
	cur->index = index;
	seek(patchdb, patchaddr[index-1], 0);
	cur->patch.nkey = (patchaddr[index]-patchaddr[index-1])/4;
	Eread(patchdb, "patch", cur->patch.key, cur->patch.nkey*4);
	for(i=0; i<cur->patch.nkey; i++)
		cur->patch.key[i] = Long(&cur->patch.key[i]);
	return 1;
}

int
loadtype(int t)
{
	int i;

	ngcopen();
	for(i=0; i<NNGCrec; i++)
		if(t == (ngcindex[i].m&0x3F)){
			grow();
			cur->type = NGCN;
			cur->index = ngcindex[i].ngc;
			cur->tag = ngcindex[i].tag;
		}
	return 1;
}

void
flatten(void)
{
	int i, j, notflat;
	Record *or;
	long key;

    loop:
	copy();
	reset();
	notflat = 0;
	for(i=0,or=orec; i<norec; i++,or++){
		switch(or->type){
		default:
			fprint(2, "bad type %d in >\n", or->type);
			break;

		case NGC:
		case SAO:
			grow();
			memmove(cur, or, sizeof(Record));
			break;

		case NGCN:
			loadngc(or->index, or->tag);
			notflat = 1;
			break;

		case Star:
			loadsao(or->index);
			notflat = 1;
			break;

		case PatchC:
			loadpatch(or->index);
			notflat = 1;
			break;

		case Patch:
			for(j=1; j<or->patch.nkey; j++){
				key = or->patch.key[j];
				if((key&0x3F) == SAO)
					loadsao((key>>8)&0xFFFFFF);
				else
					loadngc((key>>16)&0xFFFF, ((key&0xFF00)>>8));
			}
			break;
		}
	}
	if(notflat)
		goto loop;
}

int
ism(int index)
{
	int i;

	for(i=0; i<NMrec; i++)
		if(mindex[i].ngc == index)
			return 1;
	return 0;
}

char*
alpha(char *s, char *t)
{
	int n;

	n = strlen(t);
	if(strncmp(s, t, n)==0 && (s[n]<'a' || 'z'<s[n]))
		return skipbl(s+n);
	return 0;
	
}

char*
text(char *s, char *t)
{
	int n;

	n = strlen(t);
	if(strncmp(s, t, n)==0 && (s[n]==0 || s[n]==' ' || s[n]=='\t'))
		return skipbl(s+n);
	return 0;
	
}

int
cull(char *s, int keep)
{
	int i, j, nobj, keepthis;
	Record *or;
	char *t;
	int dogrtr, doless, dom, dosao, dongc;
	int mgrtr, mless;
	char obj[100];

	memset(obj, 0, sizeof(obj));
	nobj = 0;
	dogrtr = 0;
	doless = 0;
	dom = 0;
	dongc = 0;
	dosao = 0;
	mgrtr = mless= 0;
	for(;;){
		if(s[0] == '>'){
			dogrtr = 1;
			mgrtr = 10 * strtod(s+1, &t);
			if(mgrtr==0  && t==s+1){
				fprint(2, "bad magnitude\n");
				return 0;
			}
			s = skipbl(t);
			continue;
		}
		if(s[0] == '<'){
			doless = 1;
			mless = 10 * strtod(s+1, &t);
			if(mless==0  && t==s+1){
				fprint(2, "bad magnitude\n");
				return 0;
			}
			s = skipbl(t);
			continue;
		}
		if(t = text(s, "m")){
 			dom = 1;
			s = t;
			continue;
		}
		if(t = text(s, "sao")){
			dosao = 1;
			s = t;
			continue;
		}
		if(t = text(s, "ngc")){
			dongc = 1;
			s = t;
			continue;
		}
		for(i=0; names[i].name; i++)
			if(t = alpha(s, names[i].name)){
				if(nobj > 100){
					fprint(2, "too many object types\n");
					return 0;
				}
				obj[nobj++] = names[i].type;
				s = t;
				goto Continue;
			}
		break;
	    Continue:;
	}
	if(*s){
		fprint(2, "syntax error in object list\n");
		return 0;
	}

	flatten();
	copy();
	reset();
	if(dom)
		mopen();
	if(dosao)
		saoopen();
	if(dongc || nobj)
		ngcopen();
	for(i=0,or=orec; i<norec; i++,or++){
		keepthis = !keep;
		if(doless && or->ngc.mag <= mless)
			keepthis = keep;
		if(dogrtr && or->ngc.mag >= mgrtr)
			keepthis = keep;
		if(dom && (or->type==NGC && ism(or->ngc.ngc)))
			keepthis = keep;
		if(dongc && or->type==NGC)
			keepthis = keep;
		if(dosao && or->type==SAO)
			keepthis = keep;
		for(j=0; j<nobj; j++)
			if(or->type==NGC && or->ngc.type==obj[j])
				keepthis = keep;
		if(keepthis){
			grow();
			memmove(cur, or, sizeof(Record));
		}
	}
	return 1;
}

int
compar(void *va, void *vb)
{
	Record *a=va, *b=vb;

	if(a->type == b->type)
		return a->index - b->index;
	return a->type - b->type;
}

void
sort(void)
{
	int i;
	Record *r, *s;

	if(nrec == 0)
		return;
	qsort(rec, nrec, sizeof(Record), compar);
	r = rec+1;
	s = rec;
	for(i=1; i<nrec; i++,r++){
		if(r->type==s->type && r->index==s->index)
			continue;
		memmove(++s, r, sizeof(Record));
	}
	nrec = (s+1)-rec;
}

char	greekbuf[128];

char*
togreek(char *s)
{
	char *t;
	int i, n;
	Rune r;

	t = greekbuf;
	while(*s){
		for(i=1; i<=24; i++){
			n = strlen(greek[i]);
			if(strncmp(s, greek[i], n)==0 && (s[n]==' ' || s[n]=='\t')){
				s += n;
				t += runetochar(t, &greeklet[i]);
				goto Cont;
			}
		}
		n = chartorune(&r, s);
		for(i=0; i<n; i++)
			*t++ = *s++;
    Cont:;
	}
	*t = 0;
	return greekbuf;
}

char*
fromgreek(char *s)
{
	char *t;
	int i, n;
	Rune r;

	t = greekbuf;
	while(*s){
		n = chartorune(&r, s);
		for(i=1; i<=24; i++){
			if(r == greeklet[i]){
				strcpy(t, greek[i]);
				t += strlen(greek[i]);
				s += n;
				goto Cont;
			}
		}
		for(i=0; i<n; i++)
			*t++ = *s++;
    Cont:;
	}
	*t = 0;
	return greekbuf;
}

int
coords(int deg)
{
	int i;
	int x, y;
	Record *or;
	long dec, ra, ndec, nra;
	int rdeg;

	flatten();
	copy();
	reset();
	deg *= 2;
	for(i=0,or=orec; i<norec; i++,or++){
		dec = or->ngc.dec/(1000*60*60);
		ra = or->ngc.ra/(1000*60*60);
		rdeg = deg/cos((dec*PI)/180);
		for(y=-deg; y<=+deg; y++){
			ndec = dec*2+y;
			if(ndec/2>=90 || ndec/2<=-90)
				continue;
			/* fp errors hurt here, so we round 1' to the pole */
			if(ndec >= 0)
				ndec = ndec*500*60*60 + 60000;
			else
				ndec = ndec*500*60*60 - 60000;
			for(x=-rdeg; x<=+rdeg; x++){
				nra = ra*2+x;
				if(nra/2 < 0)
					nra += 360*2;
				if(nra/2 >= 360)
					nra -= 360*2;
				/* fp errors hurt here, so we round up 1' */
				nra = nra/2*1000*60*60 + 60000;
				loadpatch(patcha(angle(nra), angle(ndec)));
			}
		}
	}
	sort();
	return 1;
}

long	mapx0, mapy0;
long	mapra, mapdec, mapddec;
double	maps;
double	mapks0, mapks1;	/* keystoning */

void
setmap(long ramin, long ramax, long decmin, long decmax, Rectangle r)
{
	int c;

	c = 1000*60*60;
	mapra = ramax/2+ramin/2;
	mapdec = decmax/2+decmin/2;
	mapddec = decmax/2-decmin/2;
	mapx0 = (r.max.x+r.min.x)/2;
	mapy0 = (r.max.y+r.min.y)/2;
	maps = (double)Dy(r)/(double)(decmax-decmin);
	mapks1 = (double)Dx(r)/(double)(ramax-ramin);
	if(mapks1 < maps)
		maps = mapks1;
	mapks1 = cos(decmin/c * PI/180);
	mapks0 = cos(decmax/c * PI/180);
	mapks0 = (mapks1+mapks0)/2;
}

Point
map(long ra, long dec)
{
	Point p;

	p.y = mapy0 - (dec-mapdec)*maps;
	p.x = mapx0 - (ra-mapra)*maps*(mapks0+(mapks1-mapks0)*(mapdec-dec)/mapddec);
	return p;
}

int
dsize(int mag)	/* mag is 10*magnitude; return disc size */
{
	double d;

	mag += 25;	/* make mags all positive; sirius is -1.6m */
	d = (130-mag)/10;
	/* if plate scale is huge, adjust */
	if(maps < 100.0/(1000*60*60))
		d *= .7;
	if(maps < 50.0/(1000*60*60))
		d *= .7;
	return d;
}


void
plot(char *flags)
{
	int i;
	char *t;
	long x, y, c;
	int rah, ram, d1, d2;
	double r0;
	long ra, dec;
	int m;
	Point p;
	long ramin, ramax, decmin, decmax;	/* all in degrees */
	Record *r;
	Rectangle rect, r1;
	int folded;
	int nogrid = 0;

	for(;;){
		if(t = alpha(flags, "nogrid")){
			nogrid = 1;
			flags = t;
			continue;
		}
		if(*flags){
			fprint(2, "syntax error in plot\n");
			return;
		}
		break;
	}
	flatten();
	folded = 0;
	/* convert to milliarcsec */
	c = 1000*60*60;
    Again:
	ramin = 0x7FFFFFFF;
	ramax = -0x7FFFFFFF;
	decmin = 0x7FFFFFFF;
	decmax = -0x7FFFFFFF;
	for(r=rec,i=0; i<nrec; i++,r++){
		if(r->type == Patch){
			radec(r->index, &rah, &ram, &dec);
			ra = 15*rah+ram/4;
			r0 = c/cos(dec*PI/180);
			ra *= c;
			dec *= c;
			if(dec == 0)
				d1 = c, d2 = c;
			else if(dec < 0)
				d1 = c, d2 = 0;
			else
				d1 = 0, d2 = c;
		}else if(r->type==SAO || r->type==NGC){
			ra = r->ngc.ra;
			dec = r->ngc.dec;
			d1 = 0, d2 = 0, r0 = 0;
		}else
			continue;
		if(dec+d2 > decmax)
			decmax = dec+d2;
		if(dec-d1 < decmin)
			decmin = dec-d1;
		if(folded){
			ra -= 180*c;
			if(ra < 0)
				ra += 360*c;
		}
		if(ra+r0 > ramax)
			ramax = ra+r0;
		if(ra < ramin)
			ramin = ra;
	}
	if(folded){
		if(ramax-ramin > 270*c){
			fprint(2, "ra range too wide %d°\n", (ramax-ramin)/c);
			return;
		}
	}else if(ramax-ramin > 270*c){
		folded = 1;
		goto Again;
	}
	if(ramax-ramin<100 || decmax-decmin<100){
		fprint(2, "plot too small\n");
		return;
	}
	flatten();
	plotopen();
	screen.r = bscreenrect(0);
	rect = screen.r;
	rect.min.x += 16;
	bitblt(&screen, rect.min, &screen, rect, 0xF);
	rect = inset(rect, 20);
	setmap(ramin, ramax, decmin, decmax, rect);
	if(!nogrid){
		for(x=ramin; x<=ramax; x+=c)
			segment(&screen, map(x, decmin), map(x, decmax), ~0, 0);
		for(y=decmin; y<=decmax; y+=c)
			segment(&screen, map(ramin, y), map(ramax, y), ~0, 0);
	}
	for(i=0,r=rec; i<nrec; i++,r++){
		dec = r->ngc.dec;
		ra = r->ngc.ra;
		if(folded){
			ra -= 180*c;
			if(ra < 0)
				ra += 360*c;
		}
		/* xor lines and stars, clear the rest */
		if(r->type == SAO){
			m = r->sao.mag;
			if(m == UNKNOWNMAG)
				m = r->sao.mpg;
			if(m == UNKNOWNMAG)
				continue;
			m = dsize(m);
			if(m < 3)
				disc(&screen, map(ra, dec), m, ~0, 0);
			else{
				disc(&screen, map(ra, dec), m+1, ~0, 0xF);
				disc(&screen, map(ra, dec), m, ~0, 0);
			}
			continue;
		}
		switch(r->ngc.type){
		case Galaxy:
			ellipse(&screen, map(ra, dec), 4, 3, ~0, 0);
			break;

		case PlanetaryN:
			p = map(ra, dec);
			circle(&screen, p, 3, ~0, 0);
			segment(&screen, Pt(p.x, p.y+4), Pt(p.x, p.y+7), ~0, 0);
			segment(&screen, Pt(p.x, p.y-4), Pt(p.x, p.y-7), ~0, 0);
			segment(&screen, Pt(p.x+4, p.y), Pt(p.x+7, p.y), ~0, 0);
			segment(&screen, Pt(p.x-4, p.y), Pt(p.x-7, p.y), ~0, 0);
			break;

		case OpenCl:
		case NebularCl:
		case DiffuseN:
			p = map(ra, dec);
			r1.min = Pt(p.x-4, p.y-4);
			r1.max = Pt(p.x+4, p.y+4);
			if(r->ngc.type != DiffuseN)
				texture(&screen, r1, lightgrey, D&~S);
			if(r->ngc.type != OpenCl){
				bitblt(&screen, r1.min, &screen, r1, F&~D);
				r1 = inset(r1, -1);
				bitblt(&screen, r1.min, &screen, r1, F&~D);
			}
			break;

		case GlobularCl:
			p = map(ra, dec);
			circle(&screen, p, 4, ~0, 0);
			segment(&screen, Pt(p.x-3, p.y), Pt(p.x+4, p.y), ~0, 0);
			segment(&screen, Pt(p.x, p.y-3), Pt(p.x, p.y+4), ~0, 0);
			break;

		}
	}
	bflush();
}

void
lookup(char *s, int doreset)
{
	int i, j, k, tag;
	int rah, ram, deg;
	char *starts, *inputline=s, *t, *u;
	Record *r;
	long n;
	double x;

	lowercase(s);
	s = skipbl(s);

	if(*s == 0)
		goto Print;

	if(t = alpha(s, "flat")){
		if(*t){
			fprint(2, "flat takes no arguments\n");
			return;
		}
		if(nrec == 0){
			fprint(2, "no records\n");
			return;
		}
		flatten();
		goto Print;
	}

	if(t = alpha(s, "print")){
		if(*t){
			fprint(2, "print takes no arguments\n");
			return;
		}
		for(i=0,r=rec; i<nrec; i++,r++)
			prrec(r);
		return;
	}

	if(t = alpha(s, "add")){
		lookup(t, 0);
		return;
	}

	if(t = alpha(s, "sao")){
		n = strtoul(t, &u, 10);
		if(n<=0 || n>NSAO)
			goto NotFound;
		t = skipbl(u);
		if(*t){
			fprint(2, "syntax error in sao\n");
			return;
		}
		if(doreset)
			reset();
		if(!loadsao(n))
			goto NotFound;
		goto Print;
	}

	if(t = alpha(s, "ngc")){
		n = strtoul(t, &u, 10);
		if(n<=0 || n>NNGC)
			goto NotFound;
		tag = *u;
		if(tag)
			u++;
		t = skipbl(u);
		if(*t){
			fprint(2, "syntax error in ngc\n");
			return;
		}
		if('a'<=tag && tag<='z')
			tag ^= ' ';
		if(doreset)
			reset();
		if(!loadngc(n, tag))
			goto NotFound;
		goto Print;
	}

	if(t = alpha(s, "m")){
		n = strtoul(t, &u, 10);
		if(n<=0 || n>NM)
			goto NotFound;
		mopen();
		for(j=n-1; mindex[j].m<n; j++)
			;
		if(doreset)
			reset();
		while(mindex[j].m == n){
			if(mindex[j].ngc){
				grow();
				cur->type = NGCN;
				cur->index = mindex[j].ngc;
				cur->tag = mindex[j].tag;
			}
			j++;
		}
		goto Print;
	}

	for(i=1; i<=Ncon; i++)
		if(t = alpha(s, constel[i])){
			if(*t){
				fprint(2, "syntax error in constellation\n");
				return;
			}
			constelopen();
			seek(condb, 4L*conindex[i-1], 0);
			j = conindex[i]-conindex[i-1];
			Eread(condb, "con", con, 4*j);
			if(doreset)
				reset();
			for(k=0; k<j; k++){
				grow();
				cur->type = PatchC;
				cur->index = Long(&con[k]);
			}
			goto Print;
		}

	if(t = alpha(s, "expand")){
		n = 0;
		if(*t){
			if(*t<'0' && '9'<*t){
		Expanderr:
				fprint(2, "syntax error in expand\n");
				return;
			}
			n = strtoul(t, &u, 10);
			t = skipbl(u);
			if(*t)
				goto Expanderr;
		}
		coords(n);
		goto Print;
	}

	if(t = alpha(s, "plot")){
		if(nrec == 0){
			Bprint(&bout, "empty\n");
			return;
		}
		plot(t);
		return;
	}

	if(t = alpha(s, "keep")){
		if(!cull(t, 1))
			return;
		goto Print;
	}

	if(t = alpha(s, "drop")){
		if(!cull(t, 0))
			return;
		goto Print;
	}

	for(i=0; names[i].name; i++){
		if(t = alpha(s, names[i].name)){
			if(*t){
				fprint(2, "syntax error in type\n");
				return;
			}
			if(doreset)
				reset();
			loadtype(names[i].type);
			goto Print;
		}
	}

	switch(s[0]){
	case '"':
		starts = ++s;
		while(*s != '"')
			if(*s++ == 0){
				fprint(2, "bad star name\n");
				return;
			}
		*s = 0;
		j = nrec;
		if(doreset)
			reset();
		saoopen();
		starts = fromgreek(starts);
		for(i=0; i<NName; i++)
			if(equal(starts, name[i].name)){
				grow();
				rec[j].type = Star;
				rec[j].index = name[i].sao;
				strcpy(rec[j].star.name, name[i].name);
				j++;
			}
		if(parsename(starts))
			for(i=0; i<NBayer; i++)
				if(bayer[i].name[0]==parsed[0] &&
				  (bayer[i].name[1]==parsed[1] || parsed[1]==0) &&
				   bayer[i].name[2]==parsed[2]){
					grow();
					rec[j].type = Star;
					rec[j].index = bayer[i].sao;
					strncpy(rec[j].star.name, starts, sizeof(rec[j].star.name));
					j++;
				}
		if(j == 0)
			goto NotFound;
		break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		rah = strtoul(s, &t, 10);
		if(rah>=24 || *t!='h'){
	BadCoords:
			fprint(2, "bad coordinates %s\n", inputline);
			break;
		}
		s = t;
		ram = strtoul(s+1, &t, 10);
		if(ram>=60 || *t!='m')
			goto BadCoords;
		s = t+1;
		if(*s != ' '){
			x = strtod(s, &t);
			if(x<0 || x>=60 || *t!='s')
				goto BadCoords;
			s = t+1;
		}
		deg = strtol(skipbl(s), &t, 10);
		j = strlen("°");
		if(strncmp(t, "°", j) == 0)
			t += j;
		s = skipbl(t);
		if(*s != 0){
			if(*s<'0' || '9'<*s)
				goto BadCoords;
			strtod(s, &t);
			if(*t != '\'')
				goto BadCoords;
			s = t+1;
			if(*s != 0){
				if(*s<'0' || '9'<*s)
					goto BadCoords;
				strtod(s, &t);
				if(*t != '"')
					goto BadCoords;
			}
		}

		if(doreset)
			reset();
		if(abs(deg) >= 90)
			goto BadCoords;
		if(!loadpatch(patch(rah, ram, deg)))
			goto NotFound;
		break;

	default:
		fprint(2, "unknown command %s\n", inputline);
		return;
	}

    Print:
	if(nrec == 0)
		Bprint(&bout, "empty\n");
	else if(nrec <= 2)
		for(i=0; i<nrec; i++)
			prrec(rec+i);
	else
		Bprint(&bout, "%d items\n", nrec);
	return;

    NotFound:
	fprint(2, "%s not found\n", inputline);
	return;
}

int
typetab(int c)
{
	switch(c){
	case 'd':
		return DiffuseN;
	case 'e':
		return Galaxy;
	case 'g':
		return GlobularCl;
	case 'o':
		return OpenCl;
	case 'p':
		return PlanetaryN;
	case 'u':
		return Unknown;
	case 'x':
		return Nonexistent;
	case 'N':
		return NebularCl;
	case 'S':
		return SMC;
	case 'L':
		return LMC;
	case 0:
		return 0;
	default:
		return -1;
	}
}

char *mag[] =
{
	"<=7m", "<=10m", "<=13m", "dim"
};

char *ngctype[] =
{
	"??",
	"oc",
	"gc",
	"dn",
	"pn",
	"eg",
	"nc",
	"xx",
	"lmc",
	"smc",
	"can't happen"
};

char*
ngcstring(int d)
{
	static char buf[128];
	if(d>10)
		sprint(buf, "%s in %s", ngctype[d/10], ngctype[d%10]);
	else
		sprint(buf, "%s", ngctype[d]);
	return buf;
}

char*
skip(int n, char *s)
{
	while(n--)
		do; while(*s++);
	return s;
}

short	odescindex[NINDEX];
short	ndescindex[NINDEX];

void
printnames(Record *r)
{
	int i, j;

	for(i=0; i<NName; i++)	/* should be binary search! */
		if(name[i].sao == r->index)
			goto out;
    bad:
	fprint(2, "bad name\n");
	abort();
    out:
	for(j=0; j<r->sao.nname; j++){
		if(name[i+j].sao != r->index)
			goto bad;
		Bprint(&bout, " \"%s\"", togreek(name[i+j].name));
	}
}

int
equal(char *s1, char *s2)
{
	int c;

	while(*s1){
		if(*s1==' '){
			while(*s1==' ')
				s1++;
			continue;
		}
		while(*s2==' ')
			s2++;
		c=*s2;
		if('A'<=*s2 && *s2<='Z')
			c^=' ';
		if(*s1!=c)
			return 0;
		s1++, s2++;
	}
	return 1;
}

int
parsename(char *s)
{
	char *blank;
	int i;

	blank = strchr(s, ' ');
	if(blank==0 || strchr(blank+1, ' ') || strlen(blank+1)!=3)
		return 0;
	blank++;
	parsed[0] = parsed[1] = parsed[2] = 0;
	if('0'<=s[0] && s[0]<='9'){
		i = atoi(s);
		parsed[0] = i;
		if(i > 100)
			return 0;
	}else{
		for(i=1; i<=24; i++)
			if(strncmp(greek[i], s, strlen(greek[i]))==0){
				parsed[0]=100+i;
				goto out;
			}
		return 0;
	    out:
		if('0'<=s[strlen(greek[i])] && s[strlen(greek[i])]<='9')
			parsed[1]=s[strlen(greek[i])]-'0';
	}
	for(i=1; i<=88; i++)
		if(strcmp(constel[i], blank)==0){
			parsed[2] = i;
			return 1;
		}
    Return:
	return 0;
}


void
prrec(Record *r)
{
	NGCrec *n;
	SAOrec *s;
	int i, rah, ram, dec;
	long key;

	if(r) switch(r->type){
	default:
		fprint(2, "can't prrec type %d\n", r->type);
abort();
		exits("type");

	case NGC:
		n = &r->ngc;
		Bprint(&bout, "NGC%4d%c %s ", n->ngc, n->tag? n->tag: ' ', ngcstring(n->code));
		if(n->mag == UNKNOWNMAG)
			Bprint(&bout, "----");
		else
			Bprint(&bout, "%.1f", 	n->mag/10.0);
		Bprint(&bout, "\t%s %s\t%.2f %.2f\n",
			hm(angle(n->ra)),
			dm(angle(n->dec)),
			DEG(angle(n->glat)),
			DEG(angle(n->glong)));
		prdesc(n->desc, odesctab, odescindex);
		prdesc(skip(1, n->desc), ndesctab, ndescindex);
		Bprint(&bout, "\t%s\n", skip(2, n->desc));
		break;

	case SAO:
		s = &r->sao;
		Bprint(&bout, "SAO%6ld  ", r->index);
		if(s->mag==UNKNOWNMAG)
			Bprint(&bout, "---");
		else
			Bprint(&bout, "%.1f", s->mag/10.0);
		if(s->mpg==UNKNOWNMAG)
			Bprint(&bout, ",---");
		else
			Bprint(&bout, ",%.1f", s->mpg/10.0);
		Bprint(&bout, "  %s %s  %.4fs %.3f\"",
			hms(angle(s->ra)),
			dms(angle(s->dec)),
			DEG(angle(s->dra))*(4*60),
			DEG(angle(s->ddec))*(60*60));
		Bprint(&bout, "  %.3s %c %.2s %ld %d",
			s->spec, s->code, s->compid, s->hd, s->hdcode);
		if(s->name[0]){
			if(s->name[0] >= 100){
				Bprint(&bout, " \"%C", greeklet[s->name[0]-100]);
				if(s->name[1])
					Bprint(&bout, "%d", s->name[1]);
			}else
				Bprint(&bout, " %d", s->name[0]);
			Bprint(&bout, " %s\"", constel[s->name[2]]);
		}
		if(s->nname)
			printnames(r);
		Bprint(&bout, "\n");
		break;

	case Patch:
		radec(r->index, &rah, &ram, &dec);
		Bprint(&bout, "%dh%dm %d°", rah, ram, dec);
		key = r->patch.key[0];
		Bprint(&bout, " %s", constel[key&0xFF]);
		if((key>>=8) & 0xFF)
			Bprint(&bout, " %s", constel[key&0xFF]);
		if((key>>=8) & 0xFF)
			Bprint(&bout, " %s", constel[key&0xFF]);
		if((key>>=8) & 0xFF)
			Bprint(&bout, " %s", constel[key&0xFF]);
		for(i=1; i<r->patch.nkey; i++){
			key = r->patch.key[i];
			if((key&0x3F) == SAO)
				Bprint(&bout, " sao%ld", (key>>8)&0xFFFFFF);
			else
				Bprint(&bout, " ngc%ld%c(%s)", (key>>16)&0xFFFF,
					key&0xFF00? ((key&0xFF00)>>8):' ',
					typetostr[key&0x3F]);
		}
		Bprint(&bout, "\n");
		break;

	case NGCN:
		Bprint(&bout, "ngc%d%c\n", r->index, r->tag);
		break;

	case Star:
		Bprint(&bout, "sao%ld \"%s\"\n", r->index, togreek(r->star.name));
		break;

	case PatchC:
		radec(r->index, &rah, &ram, &dec);
		Bprint(&bout, "%dh%dm %d\n", rah, ram, dec);
		break;
	}
}
