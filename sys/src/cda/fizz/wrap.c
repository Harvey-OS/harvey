#include	<u.h>
#include	<libc.h>
#include	<cda/fizz.h>

/*
	have to define
	NOTCONP		not connected, plated hole
	NOTCONU		not connected, unplated hole
	WIRED		wired pin
	THERMALn	pin connected to plane n
*/
#define	clr_r	42		/* radius for thermals/clears */
#define	pad_r	36		/* radius for pads (lands) */
#define	pad_w	10
#define	t_r	27		/* radius of inner cutout */
#define	t_w	(clr_r-t_r)	/* width of annulus */
#define	t_c	20		/* gap for thermal relief */
#define	t_l	(t_w+t_r-t_c/2)
int pad_o = pad_r;		/* settable for opad */

long wwraplen, wwires;
extern int oneok;
extern Signal *maxsig;
Board b;
int verbose = 0;
int buvia = 0;
int brflag = 0;
int hnflag = 0;
int netno, numterms;
int cbflag = 0;
int cbfd;
int wrapz = 0;

char bestbuf[256];
char root[256] = "";
Rectangle datumr;
void getbholes(void);
void int_ch(void);
void xywrap(void);
void wirewrap(int dotsp, int donoconn);
void prnoc(Chip *c);
void prxynoc(Chip *c);
void prpnoc(Chip *c);
unsigned short pthit(Point p, char *sig);
void prdrill(Chip *c);
void prnoc(Chip *c);
void cntholes(Chip *c);
void printholes(void);
void getholes(Chip *c);
void mwclumps(void);
void buviaclumps(void);
void xyrect(unsigned short layers, Rectangle r);
void xyclear(char *s);
void xyopad(char *s);
void xypad(char *s);
void xythermal(char *s);
void datums(Pin *p);
void arty(char *sig);
Drillsz * getdrill(char letter);
void wired(Signal *s);
void sumnet(Signal *s);
int sys_sort(char *);
void wrapspecials(Signal *s);

void
main(int argc, char **argv)
{
	int n;
	extern long nph;
	extern int optind;
	int dotsp = 1;
	int donoconn = 0;
	int wwrap = 1;
	extern char *optarg;

	oneok = 0;
	while((n = getopt(argc, argv, "3novcxr:s:bjthz")) != -1)
		switch(n)
		{
		case '3':	dotsp = 0; break;
		case 'n':	donoconn = 1; break;
		case 'o':	oneok = 1; break;
		case 'v':	verbose = 1; break;
		case 'c':	scale = 10; break;
		case 'x':	wwrap = 0; break;
		case 'r':	strcpy(root, optarg); break;
		case 'z':	wrapz = 1; break;
		case 's':	pad_o = atoi(optarg); break;
		case 'b':	buvia = 1; break;
		case 'j':	wwrap = 0; brflag = 1; break; /* make .br file, jhc style */
		case 't':	cbflag = 1; break; /* make file for cb router */
		case 'h':	hnflag = 1; break; /* make .hn file for jhc */
		case '?':	break;
		}
	fizzinit();
	f_init(&b);
	if(wwrap)
		vbest(bestbuf);
	setroot(argv, optind, argc);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(2, "%s: %d errors\n", *argv, n);
			exits("crack");
		}
	if(verbose){
		if(b.name)
			fprint(2, "Board %s:\n", b.name);
		else
			fprint(2, "Warning: no board name\n");
	}
	fizzplane(&b);
	if(n = fizzplace()){
		fprint(2, "%d chips unplaced\n", n);
		exits("unplaced");
	}
	if(wwrap)
		wirewrap(dotsp, donoconn);
	else
		xywrap();
	exits(0);
}

void
wirewrap(int dotsp, int donoconn)
{
	int i,n;
	char cbfile[100];
	extern long nph;
	extern int optind;

	if(n = fizzprewrap()){
		fprint(2, "%d prewrap errors\n", n);
		exits("prewrap"); 
	}
	if(cbflag) {
		sprint(cbfile, "%s.cbnam", root);
			if((cbfd = create(cbfile, OWRITE, 0666)) < 0){
			perror(cbfile);
			exits("create");
		}
		int_ch();
		symtraverse(S_CHIP, cntholes);
		{int i;
		for(i = 0; i < b.ndrillsz; ++i)
			if(b.drillsz[i].count) {
				b.drillsz[i].pos = (Point *)
				   f_malloc(b.drillsz[i].count*(long)sizeof(Point));
				b.drillsz[i].pos += b.drillsz[i].count;
				}
		}
		getbholes();
		symtraverse(S_CHIP, getholes);
		printholes();
		netno = 0;
		numterms = 0;
	}
	symtraverse(S_SIGNAL, netlen);
	if(maxsig && ((maxsig->type & VSIG) != VSIG) && (maxsig->n >= MAXNET)){
		fprint(1, "net %s is too big (%d>=%d)\n", maxsig->name,
			maxsig->n, MAXNET);
		exits("bignet");
	}
	wwires = 0;
	wwraplen = 0;
	routedefault(dotsp? RTSP : RMST);
	if (wrapz) symtraverse(S_SIGNAL, wrapspecials);
	symtraverse(S_SIGNAL, wrap);
	if(cbflag == 0) {
		fprint(1, "x -1 %s 0 %p 0 0 0 %p 0 0 H___HDR\n", f_b->name ? f_b->name : "bogus_name",
			SC(f_b->prect.min), SC(f_b->prect.max));
		fprint(1, "c 4 - %d %p 1 1 1 %p 2 2 H___HDR\n",
			(abs(f_b->align[0].x-f_b->align[1].x)+abs(f_b->align[0].y-f_b->align[1].y))/scale,
			SC(f_b->align[0]), SC(f_b->align[1]));
		fprint(1, "c 8 - %d %p 3 3 3 %p 4 4 H___HDR\n",
			(abs(f_b->align[2].x-f_b->align[3].x)+abs(f_b->align[2].y-f_b->align[3].y))/scale,
			SC(f_b->align[2]), SC(f_b->align[3]));
	}
	symtraverse(S_SIGNAL, prwrap);
	symtraverse(S_SIGNAL, netlen);
	if(verbose){
		fprint(2, "largest distance to vb pin: %s\n", bestbuf);
		fprint(2, "%ld wires, total length %.2fi, longest net %s has %d wires\n",
			wwires/2, wwraplen/(float)INCH, maxsig ? maxsig->name : "",
			maxsig ? maxsig->n : 0);
	}
	if(donoconn)
		symtraverse(S_CHIP, prnoc);
	if(cbflag) {
		fprint(1, "\nEND OF NET INFORMATION");
		fprint(1, "\nTOTAL NUMBER OF TERMINALS =%6d\n", numterms);
	}
}

void
prnoc(Chip *c)
{
	register j;

	for(j = 0; j < c->npins; j++)
		if((c->netted[j] == 0) || (c->netted[j]&VBPIN))
			fprint(1, "n 1 noconnect 0 %p %s %d _.%c 0/0 x 0 x.A\n",
				SC(c->pins[j].p), c->name, j+c->pmin, c->pins[j].drill);
}

void
setroot(char **av, int b, int e)
{
	char buf[256];
	register char *s;

	if(root[0])
		return;
	buf[0] = 0;
	for(buf[0] = 0; b < e; av++, b++){
		s = strchr(*av, '.');
		if(s == 0) continue;
		*s = 0;
		if(buf[0] == 0)
			strcpy(buf, *av);
		else if(strcmp(buf, *av)){
			*s = '.';
			strcpy(buf, "a");
			break;
		}
		*s = '.';
	}
	strcpy(root, buf);
}

#define		S_ARTDONE	30	/* layer done */

#define		xy(a,b)		fprint(xyfd, " INC %s %d,%d\n", a, (b).x, (b).y)
#define		xyy(a,b,c)	fprint(xyfd, " INC %s %d,%d %s\n", a, (b).x, (b).y, c)

int xyfd;
int hnfd;
int kpfd;
long ncnum;
long ndrill;

void
xywrap(void)
{
	int n;
	extern long nph;
	extern int optind;
	extern char *optarg;
	int i;
	char buf[4096];
	char *tmp = "/tmp/fizzXXXXXX";
	char xyfile[256], hnfile[256], kpfile[256];
	register Plane *p;

	sprint(xyfile, "%s.xym", root);
	sprint(hnfile, "%s.gn", root);
	sprint(kpfile, "%s.br", root);
	cutout(&b);
	fizzplane(&b);
	if((xyfd = create(xyfile, OWRITE, 0666)) < 0){
		perror(xyfile);
		exits("create");
	}
	if(hnflag && ((hnfd = create(hnfile, OWRITE, 0666)) < 0)){
		perror(hnfile);
		exits("create");
	}
	if(brflag) 
		if((kpfd = create(kpfile, OWRITE, 0666)) < 0){
			perror(kpfile);
			exits("create");
		}
	if(buvia)
		buviaclumps();
	else
		mwclumps();
	fprint(xyfd, "GOO CLUMP\n");
	datums(b.datums);
	mktemp(tmp);
	close(create(tmp, OWRITE, 0600));
	if((n = open(tmp, 2)) < 0){
		perror(tmp);
		exits("open");
	}
	if(dup(n, 1) < 0){
		perror("dup");
		exits("dup");
	}
	close(n);
	/*
		traverse the special signal nets and do as many as we
		can by artwork. any we can't get put into SUBVSIG nets
		and will be wrapped as normal.
	*/
	symtraverse(S_TT, ttclean);
	symtraverse(S_SIGNAL, putnet);
	if(f_nerrs)
		exits("?errors");
	for(i = 0; i < b.nplanes; i++)
		xyrect(b.planes[i].layer, b.planes[i].r);
	for(i = 0; i < b.nplanes; i++)
		if(symlook(b.planes[i].sig, S_ARTDONE, (void *)0) == 0){
			arty(b.planes[i].sig);
			symlook(b.planes[i].sig, S_ARTDONE, (void *)1);
		}
	for(i = 0; i < 6; i++)
		if(f_b->v[i].name || f_b->v[i].npins)
			chkvsig(&f_b->v[i], i);
	routedefault(RMST3);
	symtraverse(S_SIGNAL, wrap);
	symtraverse(S_SIGNAL, prwrap);
	if(verbose){
		fprint(2, "%ld wires, total length %.2fi\n",
			wwires, wwraplen/(float)INCH);
	}
	ncnum = 0;
	if(brflag) {
		fprint(kpfd, "b 2 ICON0 A\n");
		fprint(kpfd, "I %s=AM1 NOCOMCODE\n", b.name);
		fprint(kpfd, "O 0 0 0 1 0 0\n");
		for(i = 0, p = b.keepouts; i < b.nkeepouts; i++, p++) 
		fprint(kpfd, "RW%c %d %d %d %d %d\n", (p->sense > 0) ? '+' : '-',
				p->layer,
				p->r.min.x, p->r.min.y,
				p->r.max.x, p->r.max.y);
		/*symtraverse(S_CHIP, prkeepout);*/
		fprint(kpfd, "RV 1 %d %d %d %d 0\n",datumr.min.x,datumr.min.y,datumr.max.x, datumr.max.y);
		fprint(kpfd, "RV 2 %d %d %d %d 1\n",datumr.min.x,datumr.min.y,datumr.max.x, datumr.max.y);
		fprint(kpfd, "RC 3 %d %d %d %d\n",datumr.min.x,datumr.min.y,datumr.max.x, datumr.max.y);
		for(i = 0; i < b.ndrillsz; ++i) 
			fprint(kpfd, "d %c %d %c\n", b.drillsz[i].letter,
				 b.drillsz[i].dia, b.drillsz[i].type);
	}
	symtraverse(S_CHIP, prxynoc);
	symtraverse(S_SIGNAL, wired);
	if(hnflag) {
		fprint(hnfd, "b %d %d %d %d\n", b.prect.min.x, b.prect.min.y,
			b.prect.max.x, b.prect.max.y);
		symtraverse(S_SIGNAL, sumnet);
		fprint(hnfd, "N %d ~noconn\n", ncnum);
		ndrill = 0;
		symtraverse(S_CHIP, prpnoc);
		fprint(hnfd, "N %d ~drill\n", ndrill);
		symtraverse(S_CHIP, prdrill);
	}
	/*
		do special for jhc; universe - adds
	*/
	for(i = 0; i < b.nplanes; i++)
		if(b.planes[i].layer&3)
			b.planes[i].sense = -1;
	if(b.nplanes == 0)
		b.planes = (Plane *)malloc(sizeof(Plane));
	b.nplanes += 2;
	b.planes = (Plane *)Realloc((char *)b.planes, b.nplanes*sizeof(Plane));
	b.planes[b.nplanes-2].r = b.planes[b.nplanes-1].r = datumr;
	b.planes[b.nplanes-2].sense = b.planes[b.nplanes-1].sense = 1;
	b.planes[b.nplanes-2].layer = LAYER(0);
	b.planes[b.nplanes-2].sig = 0;
	b.planes[b.nplanes-1].layer = LAYER(1);
	b.planes[b.nplanes-1].sig = 0;
	b.layer[0] = "LGCOMP";
	b.layer[1] = "LVCOMP";
	fizzplane(&b);
	for(i = 0; i < b.nplanes; i++)
		if(b.planes[i].layer&3)
			xyrect(b.planes[i].layer, b.planes[i].r);
	fprint(xyfd, " END CLUMP\n");
/*	Fflush(1);*/
	if(sys_sort(tmp) == -1){
		fprint(2, "can't sort\n");
		exits("sort");
	}
	close(1);
	if(hnflag) {
		if((n = open(tmp, 0)) < 0){
			perror(tmp);
			exits("open");
		}
/*		fflush(hnfd);*/
		while((i = read(n, buf, sizeof buf)) > 0)
			write(hnfd, buf, i);
	}
	exits(0);
}

void
prxynoc(Chip *c)
{
	register j;
	Drillsz *ds = (Drillsz *)0;
	char	letter;

	for(j = 0; j < c->npins; j++)
		if((c->netted[j] == 0) || (c->netted[j]&VBPIN)) {
			ncnum++;
			letter = c->pins[j].drill;
			if(ds == (Drillsz *)0 || ds->letter != letter)
				ds = getdrill(letter);
			if(ds == (Drillsz *)0) {
				fprint(2, "can't find drill %c\n",letter);
				exits("drill");
			}
			if(ds->xydef) {
				char buf[32];
				sprint(buf, "%sNC", ds->xydef);
				xy(buf, c->pins[j].p);
			}
			else {
				if(c->pins[j].drill == 'U')
					xy("NOTCONU", c->pins[j].p);
				else
					xy("NOTCONP", c->pins[j].p);
			}
		}
}

void
prpnoc(Chip *c)
{
	register j;

	for(j = 0; j < c->npins; j++)
		if((c->netted[j] == 0) || (c->netted[j]&VBPIN)){
			fprint(hnfd, "%c %d %d %s.%d\n", c->pins[j].drill,
				c->pins[j].p.x, c->pins[j].p.y, c->name, j+c->pmin);
		}
	ndrill += c->type->pkg->ndrills;
}

void
prdrill(Chip *c)
{
	register j;
	register Package *p = c->type->pkg;

	for(j = 0; j < p->ndrills; j++)
		fprint(hnfd, "%c %d %d %s.%d\n", p->drills[j].drill,
			p->drills[j].p.x+c->pt.x, p->drills[j].p.y+c->pt.y, c->name, j);
}

unsigned short
pthit(Point p, char *sig)
{
	register i;
	register unsigned short hit;

	for(i = 0; i < b.nplanes; i++)
		if((strcmp(sig, b.planes[i].sig) == 0) && fg_ptinrect(p, b.planes[i].r)){
			hit = b.planes[i].layer;
			for(i = 0; i < MAXLAYER; i++)
				if(hit & LAYER(i))
					return(i+1);
		}
	return(0);
}

void
arty(char *sig)
{
	register Pin *p;
	register i;
	register Signal *s;
	register Chip *c;
	int nxt;
	unsigned short hit;
	int checkit;
	char letter;
	Drillsz *ds = (Drillsz *)0;
	char buf[32];

	checkit = symlook(sig, S_VSIG, (void *)0) != 0;
	s = (Signal *)symlook(sig, S_SIGNAL, (void *)0);
	if(s == 0)
		return;
	for(nxt = i = 0, p = s->pins; i < s->n; i++, p++){
		if(checkit){
			c = (Chip *)symlook(s->coords[i].chip, S_CHIP, (void *)0);
			if(*c->type->tt && (c->type->tt[i] >= 'A') && (c->type->tt[i] <= 'Z'))
				hit = 0;
			else
				hit = pthit(p->p, sig);
		} else
			hit = pthit(p->p, sig);
		if(hit) {
			letter = p->drill;
			if(ds == (Drillsz *)0 || ds->letter != letter)
				ds = getdrill(letter);
			if(ds == (Drillsz *)0) {
				fprint(2, "can't find drill %c\n",letter);
				exits("drill");
			}
			if(ds->xydef)
				sprint(buf, "%sPC%d", ds->xydef, hit-1);
			else
				sprint(buf, "THERMAL%d", hit-1);
			xy(buf, p->p);
		} else {
			s->coords[nxt] = s->coords[i];
			s->pins[nxt++] = *p;
		}
	}
	if(verbose){
		if(s->n)
			fprint(2, "%s: wiring %d pts\n", sig, nxt);
	}
	s->n = nxt;
}

void
xyrect(unsigned short layers, Rectangle r)
{
	register i;

	for(i = 0; i < MAXLAYER; i++)
		if(b.layers&LAYER(i))
			if(layers&LAYER(i))
				fprint(xyfd, " RECT %sA %d,%d,%d,%d\n", b.layer[i],
					r.min.x, r.min.y, r.max.x, r.max.y);
}

void
wired(Signal *s)
{
	register j, chk;
	register Pin *p;
	register Coord *c;
	int hit;
	char letter;
	Drillsz *ds = (Drillsz *)0;
	char buf[32];

	chk = s->type == SUBVSIG;
	for(p = s->pins, c = s->coords, j = 0; j < s->n; j++, p++, c++) {
		letter = p->drill;
		if(ds == (Drillsz *)0 || ds->letter != letter)
			ds = getdrill(letter);
		if(ds == (Drillsz *)0) {
			fprint(2, "can't find drill %c\n",letter);
			exits("drill");
		}
		if(chk && symlook(c->chip, S_VSIG, (void *)0) && (hit = pthit(p->p, c->chip))){
			if(ds->xydef)
				sprint(buf, "%sPC%d", ds->xydef, hit-1);
			else
				sprint(buf, "THERMAL%d", hit-1);
			xy(buf, p->p);
		}
		else {
			if(ds->xydef)
				sprint(buf, "%sWIR", ds->xydef);
			else
				strcpy(buf, "WIRED");
		}
		xy(buf, p->p);
	}
}

void
sumnet(Signal *s)
{
	register j;
	register Pin *p;

	fprint(hnfd, "N %d %s\n", s->n, s->name);
	for(p = s->pins, j = 0; j < s->n; j++, p++)
		fprint(hnfd, "%c %d %d %C\n", p->drill, p->p.x, p->p.y, s->coords[j]);
}

void
mwclumps(void)
{
	register i, j;

	fprint(xyfd, "PAD CLUMP\n");
	xyopad("LC");
	xyopad("LW");
	fprint(xyfd, " END CLUMP\n");
	fprint(xyfd, "NOTCONP CLUMP\n");
	xyclear("LG");
	xyclear("LV");
	fprint(xyfd, " INC PAD 0,0\n");
	fprint(xyfd, " END CLUMP\n");
	fprint(xyfd, "NOTCONU CLUMP\n");
	xypad("LW");
	xyclear("LG");
	xyclear("LV");
	xypad("LC");
	fprint(xyfd, " END CLUMP\n");
	fprint(xyfd, "WIRED CLUMP\n");
	xyclear("LG");
	xyclear("LV");
	fprint(xyfd, " INC PAD 0,0\n");
	fprint(xyfd, " END CLUMP\n");
	for(j = 0; j < MAXLAYER; j++)
		if(b.layers&LAYER(j)){
			fprint(xyfd, "THERMAL%d CLUMP\n", j);
			for(i = 0; i < MAXLAYER; i++)
				if(b.layers&LAYER(i))
					if(i == j)
						xythermal(b.layer[i]);
					else
						xyclear(b.layer[i]);
			fprint(xyfd, " INC PAD 0,0\n");
			fprint(xyfd, " END CLUMP\n");
		}
}

void
buviaclumps(void)
{
}

void
xyclear(char *s)
{
	fprint(xyfd, " RECT %sS %d,%d %d,%d\n", s, -clr_r, -clr_r, clr_r, clr_r);
}

void
xyopad(char *s)
{
	fprint(xyfd, " RECT %sA %d,%d %d,%d\n", s, -pad_o, -pad_o, pad_o, pad_o);
/*	fprint(xyfd, " RING %sA 0,0 %d\n", s, pad_o);
	fprint(xyfd, " RING %sS 0,0 %d\n", s, pad_o/2);	/* bnl */
}

void
xypad(char *s)
{
	int pad_z = 2*pad_r+pad_w;

	fprint(xyfd, " RECT %sA %d,%d %d,%d\n", s, -pad_r, -pad_r, pad_r, pad_r);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s, -pad_r, pad_r, pad_z, pad_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s, pad_r, -pad_r-pad_w, pad_w, pad_z);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s, -pad_r-pad_w, -pad_r-pad_w, pad_z, pad_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s, -pad_r-pad_w, -pad_r, pad_w, pad_z);
}

void
xythermal(char *s)
{
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,-t_w-t_r,t_r,t_l,t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,-t_w-t_r,t_r+t_w-t_l,t_w,t_l-t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,-t_w-t_r,-t_r,t_w,t_l-t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,-t_w-t_r,-t_r-t_w,t_l,t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,t_r+t_w-t_l,t_r,t_l,t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,t_r,t_r+t_w-t_l,t_w,t_l-t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,t_r+t_w-t_l,-t_r-t_w,t_l,t_w);
	fprint(xyfd, " RECTI %sS %d,%d %d,%d\n", s,t_r,-t_r,t_w,t_l-t_w);
}

void
datums(Pin *p)
{
	register i, j;
	Rectangle r;

	r.min = r.max = p->p;
	for(i = 0; i < 3; i++, p++){
		if(p->p.x < r.min.x) r.min.x = p->p.x;
		if(p->p.x > r.max.x) r.max.x = p->p.x;
		if(p->p.y < r.min.y) r.min.y = p->p.y;
		if(p->p.y > r.max.y) r.max.y = p->p.y;
		switch(p->drill)
		{
		case 0:
			return;
		case '/':
			for(j = 0; j < MAXLAYER; j++)
				if(b.layer[j]){
					fprint(xyfd, " RECTI %sA %d,%d 50,50\n",
						b.layer[j], p->p.x, p->p.y);
					fprint(xyfd, " RECTI %sA %d,%d 50,50\n",
						b.layer[j], p->p.x-50, p->p.y-50);
				}
			break;
		case '\\':
			for(j = 0; j < MAXLAYER; j++)
				if(b.layer[j]){
					fprint(xyfd, " RECTI %sA %d,%d 50,50\n",
						b.layer[j], p->p.x-50, p->p.y);
					fprint(xyfd, " RECTI %sA %d,%d 50,50\n",
						b.layer[j], p->p.x, p->p.y-50);
				}
			break;
		}
	}
/*	for(j = 0; j < MAXLAYER; j++)
		if(b.layer[j]) {
			fprint(xyfd, " PATH %sA,12 %d,%d %d,%d %d,%d\n",
				b.layer[j], r.min.x, r.min.y,
				r.max.x, r.min.y, r.max.x, r.max.y);
			fprint(xyfd, " PATH %sA,12 %d,%d %d,%d %d,%d\n",
				b.layer[j], r.max.x, r.max.y,
				r.min.x, r.max.y, r.min.x, r.min.y);
		}
	fprint(xyfd, " PATH LVS,12 %d,%d %d,%d %d,%d\n",
		r.min.x, r.min.y, r.max.x, r.min.y, r.max.x, r.max.y);
	fprint(xyfd, " PATH LVS,12 %d,%d %d,%d %d,%d\n",
		r.max.x, r.max.y, r.min.x, r.max.y, r.min.x, r.min.y);
	fprint(xyfd, " PATH LGS,12 %d,%d %d,%d %d,%d\n",
		r.min.x, r.min.y, r.max.x, r.min.y, r.max.x, r.max.y);
	fprint(xyfd, " PATH LGS,12 %d,%d %d,%d %d,%d\n",
		r.max.x, r.max.y, r.min.x, r.max.y, r.min.x, r.min.y); */
	datumr = r;
}

Drillsz *
getdrill(char letter)
{
	Drillsz *ds = b.drillsz;
	Drillsz *end = &b.drillsz[b.ndrillsz];

	for(; ds < end; ds++)
		if(ds->letter == letter)
			return ds;
	return (Drillsz *) 0;
}
void
int_ch(void)
{
	register j;
	Drillsz *ds;
	for(ds = b.drillsz; ds < &b.drillsz[b.ndrillsz]; ds++)
		ds->count == 0;
	for(j = 0, ds = b.drillsz; j < b.ndrills; j++) {
 		if(b.drills[j].drill != ds->letter)
			if((ds = getdrill(b.drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", b.drills[j].drill);
				exits("drill");
			}
		++ds->count;
	}
}

void
cntholes(Chip *c)
{
	register j;
	Drillsz *ds = b.drillsz;
	for(j = 0; j < c->npins; j++) {
		if(c->pins[j].drill != ds->letter)
			if((ds = getdrill(c->pins[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->pins[j].drill);
				exits("drill");
			}
		++ds->count;
	}
	for(j = 0; j < c->ndrills; j++) {
 		if(c->drills[j].drill != ds->letter)
			if((ds = getdrill(c->drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->drills[j].drill);
				exits("drill");
			}
		++ds->count;
	}
}
void
getbholes(void)
{
	register j;
	Drillsz *ds;
	for(j = 0, ds = b.drillsz; j < b.ndrills; j++) {
 		if(b.drills[j].drill != ds->letter)
			if((ds = getdrill(b.drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", b.drills[j].drill);
				exits("drill");
			}
		*(--ds->pos) = b.drills[j].p;
	}
}

void
getholes(Chip *c)
{
	register j;
	Drillsz *ds = b.drillsz;
	for(j = 0; j < c->npins; j++) {
		if(c->pins[j].drill != ds->letter)
			if((ds = getdrill(c->pins[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->pins[j].drill);
				exits("drill");
			}
		*(--ds->pos) = c->pins[j].p;
	}
	for(j = 0; j < c->ndrills; j++) {
		if(c->drills[j].drill != ds->letter)
			if((ds = getdrill(c->drills[j].drill)) == 0) {
				fprint(2, "can't find drill %c\n", c->drills[j].drill);
				exits("drill");
			}
		*(--ds->pos) = c->drills[j].p;
	}
}
void
printholes(void)
{
	int j,i,n;
	Point * pt;
	fprint(1, "DRILL INFORMATION FOR %s", b.name);
	for(i = 0; i < b.ndrillsz; ++i) {
		if(b.drillsz[i].type != 'a') continue;
		pt = b.drillsz[i].pos;
		for(j = 0; j < b.drillsz[i].count; ++j, ++pt) {
			if(j%5 == 0)
				fprint(1, "\n%5d", b.drillsz[i].dia);
			fprint(1, " %6d%6d", pt->x,
				pt->y);
		}
	}
	fprint(1, "\n**********PLATED THROUGH HOLES**************");
	fprint(1, "\n             HOLESIZE            TOTAL DRILL");
	for(i = 0, n = 0; i < b.ndrillsz; ++i) {
		if((b.drillsz[i].type != 'a')
			|| (b.drillsz[i].count == 0)) continue;
		fprint(1, "\n%15d%29d", (int) b.drillsz[i].dia,
			(int) b.drillsz[i].count);
		n += b.drillsz[i].count;
	}
	fprint(1,"\nPLATED THROUGH-TOTAL HOLE =%17d\n", n);
	for(i = 0; i < b.ndrillsz; ++i) {
		if(b.drillsz[i].type == 'a') continue;
		pt = b.drillsz[i].pos;
		for(j = 0; j < b.drillsz[i].count; ++j, ++pt) {
			if(j%5 == 0)
				fprint(1, "\n%5d", (int) b.drillsz[i].dia);
			fprint(1, " %6d%6d",  pt->x,
				pt->y);
		}
	}
	fprint(1, "\n******************NON-PLATED THROUGH HOLES*******************");
	fprint(1, "\n             HOLESIZE            TOTAL DRILL");
	for(i = 0, n = 0; i < b.ndrillsz; ++i) {
		if((b.drillsz[i].type == 'a')
			|| (b.drillsz[i].count == 0)) continue;
		fprint(1, "\n%16d%20d", (int) b.drillsz[i].dia,
			(int) b.drillsz[i].count);
		n += b.drillsz[i].count;
	}
	fprint(1,"\nNON-PLATED THROUGH-TOTAL HOLE=%10d", n);
	fprint(1,"\nTHE NET INFORMATION BEGINS");
	fprint(1,"\nMAINTAIN THE SEQUENCE IN NETS MARKED WITH \"*\"");
}

int
sys_sort(char *fname)
{
	int pid;
	switch(pid = fork()) {
		case 0:
			execl("/bin/sort", "sort", "-n", "+3", "-o", fname, fname, (char *) 0);
			return(-1);
		case -1:
			return(-1);
		default:
			while(wait((struct Waitmsg *) 0) != pid);
			return(0);
	}
}

void
wrapspecials(Signal *s)
{
	if ((s -> type & (VSIG|NOVPINS)) == (VSIG|NOVPINS))
		s -> type = NORMSIG;
} /* end wrapspecials */
