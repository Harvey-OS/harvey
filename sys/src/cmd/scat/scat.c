#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libg.h>
#include "sky.h"
#include "strings.c"

enum
{
	NNGC=7840,	/* number of NGC numbers [1..NNGC] */
	NIC = 5386,	/* number of IC numbers */
	NNGCrec=NNGC+NIC,	/* number of records in the NGC catalog (including IC's, starting at NNGC */
	NMrec=122,	/* number of M records */
	NM=110,		/* number of M numbers */
	NAbell=2712,	/* number of records in the Abell catalog */
	NName=1000,	/* number of prose names; estimated maximum (read from editable text file) */
	NBayer=1517,	/* number of bayer entries */
	NSAO=258998,	/* number of SAO stars */
	MAXcon=1932,	/* maximum number of patches in a constellation */
	Ncon=88,	/* number of constellations */
	Npatch=92053,	/* highest patch number */
};

char		ngctype[NNGCrec];
Mindexrec	mindex[NMrec];
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
int	abelldb;
int	ngctypedb;
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
nameopen(void)
{
	Biobuf b;
	int i;
	char *l, *p;

	if(namedb == 0){
		namedb = eopen("name");
		Binit(&b, namedb, OREAD);
		for(i=0; i<NName; i++){
			l = Brdline(&b, '\n');
			if(l == 0)
				break;
			p = strchr(l, '\t');
			if(p == 0){
		Badformat:
				Bprint(&bout, "warning: name.scat bad format; line %d\n", i+1);
				break;
			}
			*p++ = 0;
			strcpy(name[i].name, l);
			if(strncmp(p, "ngc", 3) == 0)
				name[i].ngc = atoi(p+3);
			else if(strncmp(p, "ic", 2) == 0)
				name[i].ngc = atoi(p+2)+NNGC;
			else if(strncmp(p, "sao", 3) == 0)
				name[i].sao = atoi(p+3);
			else if(strncmp(p, "abell", 5) == 0)
				name[i].abell = atoi(p+5);
			else
				goto Badformat;
		}
		if(i == NName)
			Bprint(&bout, "warning: too many names in name.scat (max %d); extra ignored\n", NName);
		close(namedb);

		bayerdb = eopen("bayer");
		Eread(bayerdb, "bayer", bayer, sizeof bayer);
		close(bayerdb);
		for(i=0; i<NBayer; i++)
			bayer[i].sao = Long(&bayer[i].sao);
	}
}

void
saoopen(void)
{
	if(saodb == 0){
		nameopen();
		saodb = eopen("sao");
	}
}

void
ngcopen(void)
{
	if(ngcdb == 0){
		nameopen();
		ngcdb = eopen("ngc2000");
		ngctypedb = eopen("ngc2000type");
		Eread(ngctypedb, "ngctype", ngctype, sizeof ngctype);
		close(ngctypedb);
	}
}

void
abellopen(void)
{
	/* nothing extra to do with abell: it's directly indexed by number */
	if(abelldb == 0)
		abelldb = eopen("abell");
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
		Bterm(b);
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
loadngc(long index)
{
	long j;

	ngcopen();
	j = (index-1)*sizeof(NGCrec);
	grow();
	cur->type = NGC;
	cur->index = index;
	seek(ngcdb, j, 0);
	Eread(ngcdb, "ngc", &cur->ngc, sizeof(NGCrec));
	cur->ngc.ngc = Short(&cur->ngc.ngc);
	cur->ngc.ra = Long(&cur->ngc.ra);
	cur->ngc.dec = Long(&cur->ngc.dec);
	cur->ngc.diam = Long(&cur->ngc.diam);
	cur->ngc.mag = Short(&cur->ngc.mag);
	return 1;
}

int
loadabell(long index)
{
	long j;

	abellopen();
	j = index-1;
	grow();
	cur->type = Abell;
	cur->index = index;
	seek(abelldb, j*sizeof(Abellrec), 0);
	Eread(abelldb, "abell", &cur->abell, sizeof(Abellrec));
	cur->abell.abell = Short(&cur->abell.abell);
	if(cur->abell.abell != index){
		fprint(2, "bad format in abell catalog\n");
		exits("abell");
	}
	cur->abell.ra = Long(&cur->abell.ra);
	cur->abell.dec = Long(&cur->abell.dec);
	cur->abell.glat = Long(&cur->abell.glat);
	cur->abell.glong = Long(&cur->abell.glong);
	cur->abell.rad = Long(&cur->abell.rad);
	cur->abell.mag10 = Short(&cur->abell.mag10);
	cur->abell.pop = Short(&cur->abell.pop);
	cur->abell.dist = Short(&cur->abell.dist);
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
		if(t == (ngctype[i])){
			grow();
			cur->type = NGCN;
			cur->index = i+1;
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
			fprint(2, "bad type %d in flatten\n", or->type);
			break;

		case Abell:
		case NGC:
		case SAO:
			grow();
			memmove(cur, or, sizeof(Record));
			break;

		case NGCN:
			loadngc(or->index);
			notflat = 1;
			break;

		case NamedSAO:
			loadsao(or->index);
			notflat = 1;
			break;

		case NamedNGC:
			loadngc(or->index);
			notflat = 1;
			break;

		case NamedAbell:
			loadabell(or->index);
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
				else if((key&0x3F) == Abell)
					loadabell((key>>8)&0xFFFFFF);
				else
					loadngc((key>>16)&0xFFFF);
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
	int dogrtr, doless, dom, dosao, dongc, doabell;
	int mgrtr, mless;
	char obj[100];

	memset(obj, 0, sizeof(obj));
	nobj = 0;
	dogrtr = 0;
	doless = 0;
	dom = 0;
	dongc = 0;
	dosao = 0;
	doabell = 0;
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
		if(t = text(s, "abell")){
			doabell = 1;
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
	if(doabell)
		abellopen();
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
		if(doabell && or->type==Abell)
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
	maps = ((double)Dy(r))/(double)(decmax-decmin);
	mapks1 = cos(((double)decmin)/c * PI/180);
	mapks0 = cos(((double)decmax)/c * PI/180);
	if(mapks0 > mapks1)
		mapks1 = mapks0;
	mapks1 = ((double)Dx(r))/(double)(ramax-ramin) / mapks1;
	if(mapks1 < maps)
		maps = mapks1;
	mapks1 = cos(((double)decmin)/c * PI/180);
	mapks0 = cos(((double)decmax)/c * PI/180);
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
	int ra, dec;
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
		if(r->type == Abell){
			ellipse(&screen, add(map(ra, dec), Pt(-3, 2)), 2, 1, ~0, 0);
			ellipse(&screen, add(map(ra, dec), Pt(3, 2)), 2, 1, ~0, 0);
			ellipse(&screen, add(map(ra, dec), Pt(0, -2)), 1, 2, ~0, 0);
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
pplate(char *flags)
{
	int i;
	long c;
	int na, rah, ram, d1, d2;
	double r0;
	int ra, dec;
	long ramin, ramax, decmin, decmax;	/* all in degrees */
	Record *r;
	int folded;
	Angle racenter, deccenter, rasize, decsize, a[4];
	Picture *pic;

	rasize = -1.0;
	decsize = -1.0;
	na = 0;
	for(;;){
		while(*flags==' ')
			flags++;
		if(('0'<=*flags && *flags<='9') || *flags=='+' || *flags=='-'){
			if(na >= 3)
				goto err;
			a[na++] = getra(flags);
			while(*flags && *flags!=' ')
				flags++;
			continue;
		}
		if(*flags){
	err:
			Bprint(&bout, "syntax error in plate\n");
			return;
		}
		break;
	}
	switch(na){
	case 0:
		break;
	case 1:
		rasize = a[0];
		decsize = rasize;
		break;
	case 2:
		rasize = a[0];
		decsize = a[1];
		break;
	case 3:
	case 4:
		racenter = a[0];
		deccenter = a[1];
		rasize = a[2];
		if(na == 4)
			decsize = a[3];
		else
			decsize = rasize;
		if(rasize<0.0 || decsize<0.0){
			Bprint(&bout, "negative sizes\n");
			return;
		}
		goto done;
	}
	folded = 0;
	/* convert to milliarcsec */
	c = 1000*60*60;
    Again:
	if(nrec == 0){
		Bprint(&bout, "empty\n");
		return;
	}
	ramin = 0x7FFFFFFF;
	ramax = -0x7FFFFFFF;
	decmin = 0x7FFFFFFF;
	decmax = -0x7FFFFFFF;
	for(r=rec,i=0; i<nrec; i++,r++){
		if(r->type == Patch){
			radec(r->index, &rah, &ram, &dec);
			ra = 15*rah+ram/4;
			r0 = c/cos(RAD(dec));
			ra *= c;
			dec *= c;
			if(dec == 0)
				d1 = c, d2 = c;
			else if(dec < 0)
				d1 = c, d2 = 0;
			else
				d1 = 0, d2 = c;
		}else if(r->type==SAO || r->type==NGC || r->type==Abell){
			ra = r->ngc.ra;
			dec = r->ngc.dec;
			d1 = 0, d2 = 0, r0 = 0;
		}else if(r->type==NGCN){
			loadngc(r->index);
			continue;
		}else if(r->type==NamedSAO){
			loadsao(r->index);
			continue;
		}else if(r->type==NamedNGC){
			loadngc(r->index);
			continue;
		}else if(r->type==NamedAbell){
			loadabell(r->index);
			continue;
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
	if(!folded && ramax-ramin>270*c){
		folded = 1;
		goto Again;
	}
	racenter = angle(ramin+(ramax-ramin)/2);
	deccenter = angle(decmin+(decmax-decmin)/2);
	if(rasize<0 || decsize<0){
		rasize = angle(ramax-ramin)*cos(deccenter);
		decsize = angle(decmax-decmin);
	}
    done:
	if(DEG(rasize)>1.1 || DEG(decsize)>1.1){
		Bprint(&bout, "plate too big: %s", ms(rasize));
		Bprint(&bout, " x %s\n", ms(decsize));
		Bprint(&bout, "trimming to 30'x30'\n");
		rasize = RAD(0.5);
		decsize = RAD(0.5);
	}
	Bprint(&bout, "%s %s ", hms(racenter), dms(deccenter));
	Bprint(&bout, "%s", ms(rasize));
	Bprint(&bout, " x %s\n", ms(decsize));
	Bflush(&bout);
	flatten();
	pic = image(racenter, deccenter, rasize, decsize);
	if(pic == 0)
		return;
	Bprint(&bout, "plate %s locn %d %d %d %d\n", pic->name, pic->minx, pic->miny, pic->maxx, pic->maxy);
	Bflush(&bout);
	display(pic);
}

void
lookup(char *s, int doreset)
{
	int i, j, k;
	int rah, ram, deg;
	char *starts, *inputline=s, *t, *u;
	Record *r;
	long n;
	double x;
	Angle ra;

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
		t = skipbl(u);
		if(*t){
			fprint(2, "syntax error in ngc\n");
			return;
		}
		if(doreset)
			reset();
		if(!loadngc(n))
			goto NotFound;
		goto Print;
	}

	if(t = alpha(s, "ic")){
		n = strtoul(t, &u, 10);
		if(n<=0 || n>NIC)
			goto NotFound;
		t = skipbl(u);
		if(*t){
			fprint(2, "syntax error in ic\n");
			return;
		}
		if(doreset)
			reset();
		if(!loadngc(n+NNGC))
			goto NotFound;
		goto Print;
	}

	if(t = alpha(s, "abell")){
		n = strtoul(t, &u, 10);
		if(n<=0 || n>NAbell)
			goto NotFound;
		if(doreset)
			reset();
		if(!loadabell(n))
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

	if(t = alpha(s, "plate")){
		pplate(t);
		return;
	}

	if(t = alpha(s, "gamma")){
		while(*t==' ')
			t++;
		u = t;
		x = strtod(t, &u);
		if(u > t)
			gam.gamma = x;
		print("%.2f\n", gam.gamma);
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
		if(doreset)
			reset();
		j = nrec;
		saoopen();
		starts = fromgreek(starts);
		for(i=0; i<NName; i++)
			if(equal(starts, name[i].name)){
				grow();
				if(name[i].sao){
					rec[j].type = NamedSAO;
					rec[j].index = name[i].sao;
				}
				if(name[i].ngc){
					rec[j].type = NamedNGC;
					rec[j].index = name[i].ngc;
				}
				if(name[i].abell){
					rec[j].type = NamedAbell;
					rec[j].index = name[i].abell;
				}
				strcpy(rec[j].named.name, name[i].name);
				j++;
			}
		if(parsename(starts))
			for(i=0; i<NBayer; i++)
				if(bayer[i].name[0]==parsed[0] &&
				  (bayer[i].name[1]==parsed[1] || parsed[1]==0) &&
				   bayer[i].name[2]==parsed[2]){
					grow();
					rec[j].type = NamedSAO;
					rec[j].index = bayer[i].sao;
					strncpy(rec[j].named.name, starts, sizeof(rec[j].named.name));
					j++;
				}
		if(j == 0){
			*s = '"';
			goto NotFound;
		}
		break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		strtoul(s, &t, 10);
		if(*t != 'h'){
	BadCoords:
			fprint(2, "bad coordinates %s\n", inputline);
			break;
		}
		ra = DEG(getra(s));
		while(*s && *s!=' ' && *s!='\t')
			s++;
		rah = ra/15;
		ra = ra-rah*15;
		ram = ra*4;
		deg = strtol(s, &t, 10);
		if(t == s)
			goto BadCoords;
		/* degree sign etc. is optional */
		if(*t == L'°')
			deg = DEG(getra(s));
		if(doreset)
			reset();
		if(abs(deg)>=90 || rah>=24)
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

char *ngctypes[] =
{
[Galaxy] 		"Gx",
[PlanetaryN]	"Pl",
[OpenCl]		"OC",
[GlobularCl]	"Gb",
[DiffuseN]		"Nb",
[NebularCl]	"C+N",
[Asterism]		"Ast",
[Knot]		"Kt",
[Triple]		"***",
[Double]		"D*",
[Single]		"*",
[Uncertain]	"?",
[Nonexistent]	"-",
[Unknown]	" ",
[PlateDefect]	"PD",
};

char*
ngcstring(int d)
{
	if(d<Galaxy || d>PlateDefect)
		return "can't happen";
	return ngctypes[d];
}

short	descindex[NINDEX];

void
printnames(Record *r)
{
	int i, ok, done;

	done = 0;
	for(i=0; i<NName; i++){	/* stupid linear search! */
		ok = 0;
		if(r->type==SAO && r->index==name[i].sao)
			ok = 1;
		if(r->type==NGC && r->ngc.ngc==name[i].ngc)
			ok = 1;
		if(r->type==Abell && r->abell.abell==name[i].abell)
			ok = 1;
		if(ok){
			if(done++ == 0)
				Bprint(&bout, "\t");
			Bprint(&bout, " \"%s\"", togreek(name[i].name));
		}
	}
	if(done)
		Bprint(&bout, "\n");
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

char*
dist_grp(int dg)
{
	switch(dg){
	default:
		return "unknown";
	case 1:
		return "13.3-14.0";
	case 2:
		return "14.1-14.8";
	case 3:
		return "14.9-15.6";
	case 4:
		return "15.7-16.4";
	case 5:
		return "16.5-17.2";
	case 6:
		return "17.3-18.0";
	case 7:
		return ">18.0";
	}
}

char*
rich_grp(int dg)
{
	switch(dg){
	default:
		return "unknown";
	case 0:
		return "30-40";
	case 1:
		return "50-79";
	case 2:
		return "80-129";
	case 3:
		return "130-199";
	case 4:
		return "200-299";
	case 5:
		return ">=300";
	}
}

void
prrec(Record *r)
{
	NGCrec *n;
	SAOrec *s;
	Abellrec *a;
	int i, rah, ram, dec, nn;
	long key;

	if(r) switch(r->type){
	default:
		fprint(2, "can't prrec type %d\n", r->type);
		exits("type");

	case NGC:
		n = &r->ngc;
		if(n->ngc <= NNGC)
			Bprint(&bout, "NGC%4d ", n->ngc);
		else
			Bprint(&bout, "IC%4d ", n->ngc-NNGC);
		Bprint(&bout, "%s ", ngcstring(n->type));
		if(n->mag == UNKNOWNMAG)
			Bprint(&bout, "----");
		else
			Bprint(&bout, "%.1f%c", n->mag/10.0, n->magtype);
		Bprint(&bout, "\t%s %s\t%c%.1f'\n",
			hm(angle(n->ra)),
			dm(angle(n->dec)),
			n->diamlim,
			DEG(angle(n->diam))*60.);
		prdesc(n->desc, desctab, descindex);
		printnames(r);
		break;

	case Abell:
		a = &r->abell;
		Bprint(&bout, "Abell%4d  %.1f %.2f° %dMpc", a->abell, a->mag10/10.0,
			DEG(angle(a->rad)), a->dist);
		Bprint(&bout, "\t%s %s\t%.2f %.2f\n",
			hm(angle(a->ra)),
			dm(angle(a->dec)),
			DEG(angle(a->glat)),
			DEG(angle(a->glong)));
		Bprint(&bout, "\tdist grp: %s  rich grp: %s  %d galaxies/°²\n",
			dist_grp(a->distgrp),
			rich_grp(a->richgrp),
			a->pop);
		printnames(r);
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
		Bprint(&bout, "\n");
		printnames(r);
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
			switch(key&0x3F){
			case SAO:
				Bprint(&bout, " SAO%ld", (key>>8)&0xFFFFFF);
				break;
			case Abell:
				Bprint(&bout, " Abell%ld", (key>>8)&0xFFFFFF);
				break;
			default:	/* NGC */
				nn = (key>>16)&0xFFFF;
				if(nn > NNGC)
					Bprint(&bout, " IC%ld", nn-NNGC);
				else
					Bprint(&bout, " NGC%ld", nn);
				Bprint(&bout, "(%s)", ngcstring(key&0x3F));
				break;
			}
		}
		Bprint(&bout, "\n");
		break;

	case NGCN:
		if(r->index <= NNGC)
			Bprint(&bout, "NGC%d\n", r->index);
		else
			Bprint(&bout, "IC%d\n", r->index-NNGC);
		break;

	case NamedSAO:
		Bprint(&bout, "SAO%ld \"%s\"\n", r->index, togreek(r->named.name));
		break;

	case NamedNGC:
		if(r->index <= NNGC)
			Bprint(&bout, "NGC%ld \"%s\"\n", r->index, togreek(r->named.name));
		else
			Bprint(&bout, "IC%ld \"%s\"\n", r->index-NNGC, togreek(r->named.name));
		break;

	case NamedAbell:
		Bprint(&bout, "Abell%ld \"%s\"\n", r->index, togreek(r->named.name));
		break;

	case PatchC:
		radec(r->index, &rah, &ram, &dec);
		Bprint(&bout, "%dh%dm %d\n", rah, ram, dec);
		break;
	}
}
