/*
 * read and convert original tiger database
 * files. output is a single ascii file.
 */
#include	<all.h>

char*	fnames[] =
{
	"pub",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f41",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f42",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f43",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f44",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f45",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f47",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f48",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f4a",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f4i",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f4p",
	"/n/juke/census90_%s/%s/%s/tgr%s%s.f4r",
	0
};

void
main(int argc, char *argv[])
{
	int i;
	long n;
	char buf[50];
	Dir dir;

	xrec1 = 1000;
	xrec2 = 1000;
	xrec3 = 1000;
	xrec4 = 1000;
	xrec5 = 1000;
	xrec6 = 1000;
	xrec7 = 1000;
	xrec8 = 1000;
	xreca = 1000;
	xreci = 1000;
	xrecp = 1000;
	xrecr = 1000;
	xrecx = 62000;

	if(argc == 2) {
		dofile("pub");	/* has state+county names */
		dofile(argv[1]);
		goto out;
	}

	if(argc != 4) {
		print("argcount\n");
		exits("xx");
	}

	for(i=0; fnames[i]; i++) {
		sprint(buf, fnames[i],
			argv[1],		/* cd name */
			argv[2],		/* state number */
			argv[3],		/* county number */
			argv[2],		/* state number */
			argv[3]);		/* county number */
		dir.length = 0;
		dirstat(buf, &dir);
		n = dir.length;

		switch(buf[strlen(buf)-1]) {
		case '1':
			xrec1 = n/230 + 10;
			break;
		case '2':
			xrec2 = n/210 + 10;
			break;
		case '3':
			xrec3 = n/113 + 10;
			break;
		case '4':
			xrec4 = n/60 + 10;
			break;
		case '5':
			xrec5 = n/54 + 10;
			break;
		case '6':
			xrec6 = n/10 + 10;
			break;
		case '7':
			xrec7 = n/76 + 10;
			break;
		case '8':
			xrec8 = n/38 + 10;
			break;
		case 'a':
			xreca = n/100 + 10;
			break;
		case 'i':
			xreci = n/54 + 10;
			break;
		case 'p':
			xrecp = n/46 + 10;
			break;
		case 'r':
			xrecr = n/48 + 10;
			break;
		}
	}

	for(i=0; fnames[i]; i++) {
		sprint(buf, fnames[i],
			argv[1],		/* cd name */
			argv[2],		/* state number */
			argv[3],		/* county number */
			argv[2],		/* state number */
			argv[3]);		/* county number */
		dofile(buf);
	}

out:
	debrief();
	output();
	exits(0);
}

void
dofile(char* file)
{
	Biobuf*	bp;
	char *p;
	long nrec;
	int c;

	bp = Bopen(file, 0);
	if(bp == 0) {
		fprint(2, "cant open %s\n", file);
		return;
	}

	nrec = 0;
	for(;;) {
		p = Brdline(bp, '\n');
		if(p == 0)
			break;
		nrec++;
		c = p[0];
		switch(c) {
		default:
			if(recs[c] == 0) {
				print("unknown record: '%c'\n", c);
				recs[c] = 1;
			}
			break;

		case ' ':	/* blank record */
			break;
		case '1':	/* basic data record */
			decode1(p);
			break;
		case '2':	/* shape coordinate points */
			decode2(p);
			break;
		case '3':	/* additional decennial census geographic area codes */
			decode3(p);
			break;
		case '4':	/* index to alternate feature names */
			decode4(p);
			break;
		case '5':	/* feature name list */
			decode5(p);
			break;
		case '6':	/* additional address range and zip code data */
			decode6(p);
			break;
		case '7':	/* landmark features */
			decode7(p);
			break;
		case '8':	/* area landmarks */
			decode8(p);
			break;
		case 'A':	/* additional polygon geographic area codes */
			decodea(p);
			break;
		case 'I':	/* area boundaries */
			decodei(p);
			break;
		case 'P':	/* polygon location */
			decodep(p);
			break;
		case 'R':	/* record number range */
			decoder(p);
			break;
		case 'X':	/* fipps codes */
			decodex(p);
			break;
		}
	}
	Bclose(bp);
}

Sym*
lookup(char *name)
{
	Sym *s;
	long h;
	char *p, c0;
	long l;

	h = 0;
	for(p=name; *p;) {
		h *= 3;
		h += *p++;
	}
	if(h < 0)
		h = ~h;
	h %= nelem(hash);
	c0 = name[0];
	for(s = hash[h]; s; s = s->link) {
		if(s->name[0] != c0)
			continue;
		if(strcmp(s->name, name) == 0)
			return s;
	}
	s = malloc(sizeof(*s));

	l = (p - name) + 1;
	s->name = malloc(l);

	strcpy(s->name, name);
	s->link = hash[h];
	hash[h] = s;

	s->count = 0;
	return s;
}

char*	runtab[] =
{
	"]aá",
	"]eé",
	"]ií",
	"]oó",
	"]uú",

	"]AÁ",
	"]EÉ",
	"]IÍ",
	"]OÓ",
	"]UÚ",

	"#nñ",
	"[uü",
	0
};

Sym*
rec(char *p, int an, int size, int beg)
{
	int i, c, bf, ef;
	char ir[100], *r;
	Sym *s;

	p += beg-1;
	bf = 0;
	ef = 0;
	r = ir;
	while(size > 0) {
		c = *p++;
		size--;
		if(c == '[' || c == ']' || c == '#') {
			if(ir == r)
				goto no;
			if(bf)
				goto no;
			for(i=0; runtab[i]; i++)
				if(runtab[i][0] == c && runtab[i][1] == r[-1]) {
					strcpy(r-1, &runtab[i][2]);
					r = strchr(r, 0);
					break;
				}
			if(runtab[i] == 0) {
				print("unknown spanish escape %c %c\n", c, r[-1]);
				goto no;
			}
			continue;
		}
	no:
		if(c == ' ') {
			bf++;
			continue;
		}
		if(bf) {
			if(ir != r)
				*r++ = ' ';
			bf = 0;
		}
		*r++ = c;
		if((c >= '0' && c <= '9') || c == '+' || c == '-')
			continue;
		if(an == A)
			continue;
		ef++;
	}
	*r = 0;
	if(ef)
		print("%d %s\n", beg, ir);
	s = lookup(ir);
	s->count++;
	return s;
}

long
lat(char *p, int an, int size, int beg)
{
	int c, ef, nf;
	long l;

	USED(an);
	p += beg-1;
	ef = 0;
	nf = 0;
	l = 0;
	while(size > 0) {
		c = *p++;
		size--;
		if(c == ' ' || c == '+')
			continue;
		if(c == '-') {
			nf++;
			continue;
		}
		if(c >= '0' && c <= '9') {
			l = l*10 + (c-'0');
			continue;
		}
		ef++;
	}
	if(nf)
		l = 360000000 - l;
	return l;
}

long
num(char *p, int an, int size, int beg)
{
	int c, ef;
	long l;

	USED(an);
	p += beg-1;
	ef = 0;
	l = 0;
	while(size > 0) {
		c = *p++;
		size--;
		if(c == ' ')
			continue;
		if(c >= '0' && c <= '9') {
			l = l*10 + (c-'0');
			continue;
		}
		ef++;
	}
	return l;
}

int
r1cmp(Rec1 *a, Rec1 *b)
{
	if(a->tlid > b->tlid)
		return 1;
	if(a->tlid < b->tlid)
		return -1;
	return 0;
}

int
r2cmp(Rec2 *a, Rec2 *b)
{
	if(a->tlid > b->tlid)
		return 1;
	if(a->tlid < b->tlid)
		return -1;
	if(a->rtsq > b->rtsq)
		return -1;
	if(a->rtsq < b->rtsq)
		return 1;
	return 0;
}

int
r4cmp(Rec4 *a, Rec4 *b)
{
	if(a->tlid > b->tlid)
		return 1;
	if(a->tlid < b->tlid)
		return -1;
	if(a->rtsq > b->rtsq)
		return -1;
	if(a->rtsq < b->rtsq)
		return 1;
	return 0;
}

int
r5cmp(Rec5 *a, Rec5 *b)
{
	if(a->feat > b->feat)
		return 1;
	if(a->feat < b->feat)
		return -1;
	return 0;
}

int
r6cmp(Rec6 *a, Rec6 *b)
{
	if(a->tlid > b->tlid)
		return 1;
	if(a->tlid < b->tlid)
		return -1;
	if(a->rtsq > b->rtsq)
		return -1;
	if(a->rtsq < b->rtsq)
		return 1;
	return 0;
}

int
r7cmp1(Rec7 *a, Rec7 *b)
{
	if(a->laname > b->laname)
		return 1;
	if(a->laname < b->laname)
		return -1;
	if(a->cfcc > b->cfcc)
		return 1;
	if(a->cfcc < b->cfcc)
		return -1;
	return 0;
}

int
r7cmp2(Rec7 *a, Rec7 *b)
{
	if(a->land > b->land)
		return 1;
	if(a->land < b->land)
		return -1;
	return 0;
}

int
racmp(Reca *a, Reca *b)
{
	if(a->cenid > b->cenid)
		return 1;
	if(a->cenid < b->cenid)
		return -1;
	if(a->polyid > b->polyid)
		return 1;
	if(a->polyid < b->polyid)
		return -1;
	return 0;
}

int
rpcmp(Recp *a, Recp *b)
{
	if(a->cenid > b->cenid)
		return 1;
	if(a->cenid < b->cenid)
		return -1;
	if(a->polyid > b->polyid)
		return 1;
	if(a->polyid < b->polyid)
		return -1;
	return 0;
}

int
rxcmp(Recx *a, Recx *b)
{
	if(a->fips > b->fips)
		return 1;
	if(a->fips < b->fips)
		return -1;
	if(a->state > b->state)
		return 1;
	if(a->state < b->state)
		return -1;
	if(a->type > b->type)
		return 1;
	if(a->type < b->type)
		return -1;
	return 0;
}

Rec1*
blook1(long tlid)
{
	Rec1 *bot, *top, *new;

	bot = rec1;
	top = bot + nrec1-1;

	while(bot <= top) {
		new = bot + (top - bot)/2;
		if(new->tlid == tlid)
			return new;
		if(new->tlid < tlid)
			bot = new + 1;
		else
			top = new - 1;
	}
	print("blook1 miss %ld\n", tlid);
	return 0;
}

Rec5*
blook5(long feat)
{
	Rec5 *bot, *top, *new;

	bot = rec5;
	top = bot + nrec5-1;

	while(bot <= top) {
		new = bot + (top - bot)/2;
		if(new->feat == feat)
			return new;
		if(new->feat < feat)
			bot = new + 1;
		else
			top = new - 1;
	}
	print("blook5 miss %ld\n", feat);
	return 0;
}

Rec7*
blook7(long land)
{
	Rec7 *bot, *top, *new;

	bot = rec7;
	top = bot + nrec7-1;

	while(bot <= top) {
		new = bot + (top - bot)/2;
		if(new->land == land)
			return new;
		if(new->land < land)
			bot = new + 1;
		else
			top = new - 1;
	}
	print("blook7 miss %ld\n", land);
	return 0;
}

Recp*
blookp(long cenid, long polyid)
{
	Recp *bot, *top, *new;

	bot = recp;
	top = bot + nrecp-1;

	while(bot <= top) {
		new = bot + (top - bot)/2;
		if(new->cenid == cenid)
		if(new->polyid == polyid)
			return new;
		if(new->cenid < cenid ||
		  (new->cenid == cenid && new->polyid < polyid))
				bot = new + 1;
		else
			top = new - 1;
	}
	print("blookp miss %ld %ld\n", cenid, polyid);
	return 0;
}

Recx*
blookx(long fips, int state, int type)
{
	Recx *bot, *top, *new;
	char buf[20];

	bot = recx;
	top = bot + nrecx-1;

	while(bot <= top) {
		new = bot + (top - bot)/2;
		if(new->fips == fips)
		if(new->state == state)
		if(new->type == type)
			return new;
		if(new->fips < fips ||
		  (new->fips == fips &&
			(new->state < state ||
			(new->state == state && new->type < type))))
				bot = new + 1;
		else
			top = new - 1;
	}
	for(new=missedx; new; new=new->link) {
		if(new->fips == fips)
		if(new->state == state)
		if(new->type == type)
			return new;
	}
	print("blookx miss %ld %d %d\n", fips, state, type);
	new = malloc(sizeof(*new));
	new->link = missedx;
	missedx = new;

	sprint(buf, "%ld", fips);
	new->fips = fips;
	new->state = state;
	new->type = type;
	new->name = lookup(buf);
	return new;
}

void
debrief(void)
{
	long v, i;
	int j;
	Sym *s;
	Addl *a;
	Rec1 *r1;
	Rec2 *r2;
	Rec4 *r4;
	Rec5 *r5;
	Rec6 *r6;
	Rec7 *r7;
	Rec8 *r8;
	Reca *ra;
	Reci *ri;
	Recp *rp;
	Recr *rr;
	Recx *rx;

/*
 * clear
 */
	for(i=nrec1,r1=rec1; i; i--,r1++) {
		r1->rec2 = 0;
		r1->rec5 = 0;
		r1->rec6 = 0;
		r1->recpl = 0;
		r1->recpr = 0;
	}
	for(i=nrecp,rp=recp; i; i--,rp++) {
		rp->rec7 = 0;
		rp->reca = 0;
	}
	for(i=nrec7,r7=rec7; i; i--,r7++)
		r7->sequence = 0;

/*
 * sort
 */
	qsort(rec2, nrec2, sizeof(Rec2), r2cmp);
	qsort(rec4, nrec4, sizeof(Rec4), r4cmp);
	qsort(rec6, nrec6, sizeof(Rec6), r6cmp);

/*
 * unique
 */
	qsort(rec1, nrec1, sizeof(Rec1), r1cmp);
	for(i=nrec1-1,r1=rec1; i>0; i--,r1++)
		if(r1->tlid == (r1+1)->tlid)
			print("rec1 dup %ld\n", r1->tlid);

	qsort(rec5, nrec5, sizeof(Rec5), r5cmp);
	for(i=nrec5-1,r5=rec5; i>0; i--,r5++)
		if(r5->feat == (r5+5)->feat)
			print("rec5 dup %ld\n", r5->feat);

	qsort(rec7, nrec7, sizeof(Rec7), r7cmp1);
	for(i=nrec7-1,r7=rec7; i>0; i--,r7++)
		if(r7->laname == (r7+1)->laname)
			if(r7->cfcc == (r7+1)->cfcc)
				(r7+1)->sequence = r7->sequence+1;

	qsort(rec7, nrec7, sizeof(Rec7), r7cmp2);
	for(i=nrec7-1,r7=rec7; i>0; i--,r7++)
		if(r7->land == (r7+1)->land)
			print("rec7 dup %ld\n", r7->land);

	qsort(reca, nreca, sizeof(Reca), racmp);
	for(i=nreca-1,ra=reca; i>0; i--,ra++)
		if(ra->cenid == (ra+1)->cenid)
		if(ra->polyid == (ra+1)->polyid)
			print("reca dup %ld %ld\n", ra->cenid, ra->polyid);

	qsort(recp, nrecp, sizeof(Recp), rpcmp);
	for(i=nrecp-1,rp=recp; i>0; i--,rp++)
		if(rp->cenid == (rp+1)->cenid)
			if(rp->polyid == (rp+1)->polyid)
				print("recp dup %ld %ld\n", rp->cenid, rp->polyid);

	qsort(recx, nrecx, sizeof(Recx), rxcmp);
	for(i=nrecx-1,rx=recx; i>0; i--,rx++)
		if(rx->fips == (rx+1)->fips)
		if(rx->state == (rx+1)->state)
		if(rx->type == (rx+1)->type)
			print("recx dup %ld %d %d %s %s\n",
				rx->fips, rx->state, rx->type,
				rx->name->name, (rx+1)->name->name);

/*
 * version
 */
	s = rec1->version;
	for(i=nrec1,r1=rec1; i>0; i--,r1++)
		if(s != r1->version)
			print("	versions differ %s %s\n", s->name, r1->version->name);
	for(i=nrec2,r2=rec2; i>0; i--,r2++)
		if(s != r2->version)
			print("	versions differ %s %s\n", s->name, r2->version->name);
#ifdef REC3
	for(i=nrec3,r3=rec3; i>0; i--,r3++)
		if(s != r3->version)
			print("	versions differ %s %s\n", s->name, r3->version->name);
#endif
	for(i=nrec4,r4=rec4; i>0; i--,r4++)
		if(s != r4->version)
			print("	versions differ %s %s\n", s->name, r4->version->name);
	for(i=nrec6,r6=rec6; i>0; i--,r6++)
		if(s != r6->version)
			print("	versions differ %s %s\n", s->name, r6->version->name);
	for(i=nrec7,r7=rec7; i>0; i--,r7++)
		if(s != r7->version)
			print("	versions differ %s %s\n", s->name, r7->version->name);
	for(i=nrec8,r8=rec8; i>0; i--,r8++)
		if(s != r8->version)
			print("	versions differ %s %s\n", s->name, r8->version->name);
	for(i=nreca,ra=reca; i>0; i--,ra++)
		if(s != ra->version)
			print("	versions differ %s %s\n", s->name, ra->version->name);
	for(i=nreci,ri=reci; i>0; i--,ri++)
		if(s != ri->version)
			print("	versions differ %s %s\n", s->name, ri->version->name);
	for(i=nrecp,rp=recp; i>0; i--,rp++)
		if(s != rp->version)
			print("	versions differ %s %s\n", s->name, rp->version->name);
	for(i=nrecr,rr=recr; i>0; i--,rr++)
		if(s != rr->version)
			print("	versions differ %s %s\n", s->name, rr->version->name);

/*
 * addl lat/lng -- link rec2 onto rec1
 */
	for(i=nrec2,r2=rec2; i>0; i--,r2++) {
		r1 = blook1(r2->tlid);
		if(r1 == 0)
			continue;
		a = malloc(sizeof(*a));
		a->rec = r2;
		a->link = r1->rec2;
		r1->rec2 = a;
	}

/*
 * alternate names -- link type4/5 onto type1
 */
	for(i=nrec4,r4=rec4; i>0; i--,r4++) {
		r1 = blook1(r4->tlid);
		if(r1 == 0)
			continue;
		for(j=4; j>=0; j--) {
			v = r4->feat[j];
			if(v == 0)
				continue;
			r5 = blook5(v);
			if(r5 == 0)
				print("missed blook5 r4\n");
			a = malloc(sizeof(*a));
			a->rec = r5;
			a->link = r1->rec5;
			r1->rec5 = a;
		}
	}

/*
 * addl addr/zip -- link rec6 onto rec1
 */
	for(i=nrec6,r6=rec6; i>0; i--,r6++) {
		r1 = blook1(r6->tlid);
		if(r1 == 0)
			continue;
		a = malloc(sizeof(*a));
		a->rec = r6;
		a->link = r1->rec6;
		r1->rec6 = a;
	}

/*
 * rec8 -- link rec7 onto recp
 */
	for(i=nrec8,r8=rec8; i>0; i--,r8++) {
		r7 = blook7(r8->land);
		if(r7 == 0)
			continue;
		rp = blookp(r8->cenid, r8->polyid);
		if(rp == 0)
			continue;
		if(r7->lat != 0 && r7->lng != 0)
			print("r7 both point and area\n");
		if(rp->rec7)
			print("rec7 not unique %ld %ld\n", rp->rec7->land, r7->land);
		rp->rec7 = r7;
	}

/*
 * reci -- link reci onto rec1
 */
	for(i=nreci,ri=reci; i>0; i--,ri++) {
		r1 = blook1(ri->tlid);
		if(r1 == 0)
			continue;
		if(ri->cenidl != 0 && ri->polyl != 0) {
			rp = blookp(ri->cenidl, ri->polyl);
			if(rp != 0) {
				if(r1->recpl)
					print("dup recpl\n");
				r1->recpl = rp;
			}
		}
		if(ri->cenidr != 0 && ri->polyr != 0) {
			rp = blookp(ri->cenidr, ri->polyr);
			if(rp != 0) {
				if(r1->recpr)
					print("dup recpr\n");
				r1->recpr = rp;
			}
		}
	}

/*
 * recp -- link reca onto recp
 */
	for(i=nreca,ra=reca; i>0; i--,ra++) {
		rp = blookp(ra->cenid, ra->polyid);
		if(rp != 0) {
			if(rp->reca)
				print("dup reca\n");
			rp->reca = ra;
		}
	}
}

void
doleft(Rec1 *r1, Side *s)
{
	Rec6 *r6;
	Rec7 *r7;
	Reca *ra;
	Recp *rp;
	Recx *rx;
	Addl *a;

	if(r1->countyl != 0 && r1->countyl != r1->countyr) {
		rx = blookx(r1->countyl, r1->statel, 2);
		s->type = 'y';
		s->param = rx->name->name;
		s++;
	}
	if(r1->fraddl != null) {
		s->type = 'a';
		s->param = r1->fraddl->name;
		s++;
	}
	if(r1->toaddl != null) {
		s->type = 'b';
		s->param = r1->toaddl->name;
		s++;
	}

	/*
	 * addl address range
	 */
	for(a=r1->rec6; a; a=a->link) {
		r6 = a->rec;
		if(r6->fraddl != null) {
			s->type = 'a';
			s->param = r6->fraddl->name;
			s++;
		}
		if(r6->toaddl != null) {
			s->type = 'b';
			s->param = r6->toaddl->name;
			s++;
		}
	}

	/*
	 * zip
	 */
	if(r1->zipl != 0) {
		s->type = 'z';
		s->paraml = r1->zipl;
		s++;
	}

	/*
	 * addl zip
	 */
	for(a=r1->rec6; a; a=a->link) {
		r6 = a->rec;
		if(r6->zipl != 0) {
			s->type = 'z';
			s->paraml = r6->zipl;
			s++;
		}
	}

	/*
	 * fips codes
	 */
	if(r1->fmcdl) {
		rx = blookx(r1->fmcdl, r1->statel, 3);
		s->type = 'm';
		s->param = rx->name->name;
		s++;
	}
/*
	if(r1->fsmcdl) {
		rx = blookx(r1->fsmcdl, r1->statel, 4);
		s->type = 'm';
		s->param = rx->name->name;
		s++;
	}
*/
	if(r1->fpll) {
		rx = blookx(r1->fpll, r1->statel, 5);
		s->type = 'l';
		s->param = rx->name->name;
		s++;
	}

	/*
	 * landmark
	 */
	rp = r1->recpl;
	r7 = 0;
	if(rp)
		r7 = rp->rec7;
	if(r7 != 0) {
		if(r7->cfcc != null) {
			s->type = 'c';
			s->param = r7->cfcc->name;
			s++;
		}
		if(r7->laname != null) {
			s->type = 'n';
			s->param = r7->laname->name;
			s++;
		}
		if(r7->sequence != 0) {
			s->type = 's';
			s->paraml = r7->sequence;
			s++;
		}
	}

	/*
	 * area
	 */
	ra = 0;
	if(rp)
		ra = rp->reca;
	if(ra != 0) {
		if(ra->fmcd) {
			rx = blookx(ra->fmcd, ra->state, 3);
			s->type = 'm';
			s->param = rx->name->name;
			s++;
		}
		if(ra->fpl) {
			rx = blookx(ra->fpl, ra->state, 5);
			s->type = 'l';
			s->param = rx->name->name;
			s++;
		}
	}
	s->type = 0;
}

void
doright(Rec1 *r1, Side *s)
{
	Rec6 *r6;
	Rec7 *r7;
	Reca *ra;
	Recp *rp;
	Recx *rx;
	Addl *a;

	if(r1->countyr != 0 && r1->countyl != r1->countyr) {
		rx = blookx(r1->countyr, r1->stater, 2);
		s->type = 'y';
		s->param = rx->name->name;
		s++;
	}
	if(r1->fraddr != null) {
		s->type = 'a';
		s->param = r1->fraddr->name;
		s++;
	}
	if(r1->toaddr != null) {
		s->type = 'b';
		s->param = r1->toaddr->name;
		s++;
	}

	/*
	 * addl address range
	 */
	for(a=r1->rec6; a; a=a->link) {
		r6 = a->rec;
		if(r6->fraddr != null) {
			s->type = 'a';
			s->param = r6->fraddr->name;
			s++;
		}
		if(r6->toaddr != null) {
			s->type = 'b';
			s->param = r6->toaddr->name;
			s++;
		}
	}

	/*
	 * zip
	 */
	if(r1->zipr != 0) {
		s->type = 'z';
		s->paraml = r1->zipr;
		s++;
	}

	/*
	 * addl zip
	 */
	for(a=r1->rec6; a; a=a->link) {
		r6 = a->rec;
		if(r6->zipr != 0) {
			s->type = 'z';
			s->paraml = r6->zipr;
			s++;
		}
	}

	/*
	 * fips codes
	 */
	if(r1->fmcdr != 0) {
		rx = blookx(r1->fmcdr, r1->stater, 3);
		s->type = 'm';
		s->param = rx->name->name;
		s++;
	}
/*
	if(r1->fsmcdr) {
		rx = blookx(r1->fsmcdr, r1->stater, 4);
		s->type = 'm';
		s->param = rx->name->name;
		s++;
	}
*/
	if(r1->fplr) {
		rx = blookx(r1->fplr, r1->stater, 5);
		s->type = 'l';
		s->param = rx->name->name;
		s++;
	}

	/*
	 * landmark
	 */
	rp = r1->recpr;
	r7 = 0;
	if(rp)
		r7 = rp->rec7;
	if(r7 != 0) {
		if(r7->cfcc != null) {
			s->type = 'c';
			s->param = r7->cfcc->name;
			s++;
		}
		if(r7->laname != null) {
			s->type = 'n';
			s->param = r7->laname->name;
			s++;
		}
		if(r7->sequence != 0) {
			s->type = 's';
			s->paraml = r7->sequence;
			s++;
		}
	}

	/*
	 * area
	 */
	ra = 0;
	if(rp)
		ra = rp->reca;
	if(ra != 0) {
		if(ra->fmcd) {
			rx = blookx(ra->fmcd, ra->state, 3);
			s->type = 'm';
			s->param = rx->name->name;
			s++;
		}
		if(ra->fpl) {
			rx = blookx(ra->fpl, ra->state, 5);
			s->type = 'l';
			s->param = rx->name->name;
			s++;
		}
	}
	s->type = 0;
}

int
cvtype(int type)
{
	switch(type) {
	case 'y':	return 1;
	case 'c':	return 2;
	case 'n':	return 3;
	case 's':	return 4;
	case 'a':	return 5;
	case 'b':	return 6;
	case 'z': 	return 7;
	case 'm':	return 8;
	case 'l':	return 9;
	}
	print("gok: %c\n", type);
	return 100;
}

int
sidecmp(Side *a, Side *b)
{
	int t1, t2;

	t1 = cvtype(a->type);
	t2 = cvtype(b->type);
	if(t1 > t2)
		return 1;
	if(t1 < t2)
		return -1;
	if(a->paraml > b->paraml)
		return 1;
	if(a->paraml < b->paraml)
		return -1;
	if(a->side > b->side)
		return 1;
	if(a->side < b->side)
		return -1;
	return 0;
}

void
doside(int lr)
{
	int f;
	Side *s;

	f = 0;
	for(s=side; s->type; s++) {
		if(s->del)
			continue;
		if(s->side != lr)
			continue;
		if(f == 0) {
			if(lr == 'l')
				Bprint(&obuf, "2\n");
			else
				Bprint(&obuf, "3\n");
			f = 1;
		}
		switch(s->type) {
		case 'y':
			Bprint(&obuf, "y%s\n", s->param);
			break;
		case 'c':
			Bprint(&obuf, "c%s\n", s->param);
			break;
		case 'n':
			Bprint(&obuf, "n%s\n", s->param);	/* botch, need suffix */
			break;
		case 's':
			break;
		case 'a':
			Bprint(&obuf, "a%s\n", s->param);
			break;
		case 'b':
			Bprint(&obuf, "b%s\n", s->param);
			break;
		case 'z':
			Bprint(&obuf, "z%.5ld\n", s->paraml);
			break;
		case 'm':
			Bprint(&obuf, "m%s\n", s->param);
			break;
		case 'l':
			Bprint(&obuf, "l%s\n", s->param);
			break;
		}
	}
}

void
dosides(Rec1 *r1)
{
	Side *s, *s1;

	doleft(r1, side);
	for(s=side; s->type; s++) {
		s->side = 'l';
		s->del = 0;
	}
	doright(r1, s);
	for(; s->type; s++) {
		s->side = 'r';
		s->del = 0;
	}

	qsort(side, s-side, sizeof(*s), sidecmp);
	for(s=side; s->type; s++) {
		for(s1=s+1; s1->type; s1++) {
			if(s->type != s1->type)
				break;
			if(s->paraml != s1->paraml)
				break;
			s1->del = 1;
			if(s->side != s1->side)
				s->del = 1;
		}
	}

	doside('l');
	doside('r');
}

void
output(void)
{
	long i, t, T;
	int f, j;
	Addl *a;
	Rec1 *r1;
	Rec2 *r2;
	Rec5 *r5;
	Rec7 *r7;
	Recr *rr;
	Recx *rx;

	null = lookup("");
	f = create("out", OWRITE, 0666);
	if(f < 0) {
		print("cant open out\n");
		return;
	}
	Binit(&obuf, f, OWRITE);

/*
 * county -- type r
 */
	if(nrecr >= 1) {
		Bprint(&obuf, "0\n");
		rr = recr;
		rx = blookx(rr->state, 0, 1);
		Bprint(&obuf, "s%s\n", rx->name->name);
		print("%.2d %.3d %s", rr->state, rr->county, rx->name->name);

		rx = blookx(rr->county, rr->state, 2);
		Bprint(&obuf, "y%s\n", rx->name->name);
		print(", %s\n", rx->name->name);
	} else {
		print("nrecr = %ld\n", nrecr);
		rr = 0;
	}

/*
 * lines -- type 1
 */
	for(i=nrec1,r1=rec1; i; i--,r1++) {
	/*
	 * check that state/county are same
	 */
		if(rr) {
		if(r1->statel != 0)
		if(rr->state != r1->statel)
			print("%ld -- statel = %ld\n", r1->cfcc, r1->statel);
		if(r1->countyl != 0)
		if(rr->county != r1->countyl)
			print("%ld -- countyl = %ld\n", r1->cfcc, r1->countyl);
		if(r1->stater != 0)
		if(rr->state != r1->stater)
			print("%ld -- stater = %ld\n", r1->cfcc, r1->stater);
		if(r1->countyr != 0)
		if(rr->county != r1->countyr)
			print("%ld -- countyr = %ld\n", r1->cfcc, r1->countyr);
		}

	/*
	 * names
	 */
		Bprint(&obuf, "1\n");
		if(r1->cfcc != null)
			Bprint(&obuf, "c%s\n", r1->cfcc->name);
		if(r1->fename != null) {
			if(r1->fedirp != null)
				Bprint(&obuf, "n%s %s", r1->fedirp->name, r1->fename->name);
			else
				Bprint(&obuf, "n%s", r1->fename->name);
			if(r1->fedirs != null)
				Bprint(&obuf, " %s", r1->fedirs->name);
			if(r1->fetype != null)
				Bprint(&obuf, " %s", r1->fetype->name);
			Bprint(&obuf, "\n");
		}

	/*
	 * addl names
	 */
		for(a=r1->rec5; a; a=a->link) {
			r5 = a->rec;
			if(r5->fename != null) {
				if(r5->fedirp != null)
					Bprint(&obuf, "n%s %s", r5->fedirp->name, r5->fename->name);
				else
					Bprint(&obuf, "n%s", r5->fename->name);
				if(r5->fedirs != null)
					Bprint(&obuf, " %s", r5->fedirs->name);
				if(r5->fetype != null)
					Bprint(&obuf, " %s", r5->fetype->name);
				Bprint(&obuf, "\n");
			}
		}
	/*
	 * left/right
	 */
		dosides(r1);
	/*
	 * first lat
	 */
		T = r1->frlat;
		Bprint(&obuf, "t%ld\n", T);

	/*
	 * addl lat
	 */
		for(a=r1->rec2; a; a=a->link) {
			r2 = a->rec;
			for(j=0; j<10; j++) {
				if(r2->lat[j] == 0 || r2->lng[j] == 0)
					continue;
				t = r2->lat[j] - T;
				T = r2->lat[j];
				if(t >= 32768 || t <= -32768)
					Bprint(&obuf, "t%ld\n", T);
				else
					Bprint(&obuf, "d%ld\n", t);
			}
		}
	/*
	 * last lat
	 */
		t = r1->tolat - T;
		T = r1->tolat;
		if(t >= 32768 || t <= -32768)
			Bprint(&obuf, "t%ld\n", T);
		else
			Bprint(&obuf, "d%ld\n", t);
	/*
	 * first lng
	 */
		T = r1->frlng;
		Bprint(&obuf, "g%ld\n", T);

	/*
	 * addl lng
	 */
		for(a=r1->rec2; a; a=a->link) {
			r2 = a->rec;
			for(j=0; j<10; j++) {
				if(r2->lat[j] == 0 || r2->lng[j] == 0)
					continue;
				t = r2->lng[j] - T;
				T = r2->lng[j];
				if(t >= 32768 || t <= -32768)
					Bprint(&obuf, "g%ld\n", T);
				else
					Bprint(&obuf, "d%ld\n", t);
			}
		}
	/*
	 * last lng
	 */
		t = r1->tolng - T;
		T = r1->tolng;
		if(t >= 32768 || t <= -32768)
			Bprint(&obuf, "g%ld\n", T);
		else
			Bprint(&obuf, "d%ld\n", t);
	}

/*
 * landmarks
 */
	for(i=nrec7,r7=rec7; i>0; i--,r7++) {
		if(r7->lat != 0 && r7->lng != 0) {
			Bprint(&obuf, "4\n");

		/*
		 * names
		 */
			if(r7->cfcc != null)
				Bprint(&obuf, "c%s\n", r7->cfcc->name);
			if(r7->laname != null)
				Bprint(&obuf, "n%s\n", r7->laname->name);
		/*
		 * lat+lng
		 */
			Bprint(&obuf, "t%ld\n", r7->lat);
			Bprint(&obuf, "g%ld\n", r7->lng);
		}
	}

	Bclose(&obuf);
	close(f);
}
