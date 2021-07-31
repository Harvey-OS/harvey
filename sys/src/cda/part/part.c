
#include "part.h"

Hshtab **aouts, **bouts;
int besti, bestj, obesti, obestj, ompno, olpno, mostpno, leastpno;
int total, ototal;
Depend *dpstart;
char * name;
Device * devtype;
int extra, more;
int interact, DEBUG, pflag;

void prtype(void);

void
main(int argc, char **argv)
{
	int i, niter, pkgno, cnt;
	char *s;
	int most, least;
	more = 0;
	niter = NITER;
	DEBUG = 0;
	extra = 0;
	interact = 0;
	devtype = (Device *) 0;
	while (argc>1 && argv[1][0]=='-') {
		switch(argv[1][1]) {
		case 'p':
			pflag = 1;  /* use old pin assignment */
			break;
		case 'D':
			DEBUG = 1;
			break;
		case 'N':
			niter = atoi(&argv[1][2]);
			break;
		case 'M':
			if(argv[1][2])
				more = atoi(&argv[1][2]);
			else {
				argc--;
				argv++;
				more = atoi(argv[1]);
			}
			break;
		case 'a':
			extra = atoi(&argv[1][2]);
			break;
		case 'i':
			interact = 1;
			break;
		case 't':
			if(argv[1][2]) {
				if(argv[1][2] == '?') {
					prtype();
					GOODEND;
				}
				gettype(&argv[1][2]);
			}
			else {
				argc--;
				argv++;
				if(argv[1][0] == '?') {
					prtype();
					GOODEND;
				}
				gettype(argv[1]);
			}
			break;
		default:
			fprintf(stderr,"unknown option %c\n",argv[1][1]);
			BADEND;
		}
		argc--;
		argv++;
	}
	while(argc > 1) {
		argc--;
		argv++;
		rdfile(argv[0]);
		name = argv[0];
		if(s = strrchr(name, '.')) *s = 0;
	}
	fprintf(stderr, "inputs %d outputs %d\n", hshused-noutputs, noutputs);
	if(pflag) {
		assigned();
		GOODEND;
	}
	init();
again:
	for(i = 0; i < niter; ++i) {
		most = -1000;
		least = 1000;
		ototal = total;
		total = 0;
		for(pkgno = 0; pkgno < npackages; ++pkgno) {
			cnt = assign(pkgno,0);
			total += cnt;
			fprintf(stderr, "pkg %d %d pins ", pkgno, cnt);
			if(cnt >= most) {
				mostpno = pkgno;
				most = cnt;
			}
			if(cnt < least) {
				least = cnt;
				leastpno = pkgno;
			}
		}
		if((most <= 0) && (--more < 0)) {
			printpkg();
			jstart();
			for(i = 0; i < npackages; ++i) out(i);
			jend();
			GOODEND;
		}
		if(leastpno == mostpno) break;
		if(total == ototal) {
			leastpno = pick(npackages);
			while((mostpno = pick(npackages)) == leastpno);
		}	
		fprintf(stderr, "%d %d<->%d\n", total, leastpno, mostpno);
		bwk(mostpno, leastpno);
	}
	fprintf(stderr, "no can do\n");
	printpkg();
	if(interact) {
		while((i = fgetc(stdin)) == '\n');
		if(i == 'y') {
			shuffle();
			goto again;
		}
	}
	GOODEND;
}

int
bwk(int a, int b)
{
	int i, j, k, na, nb, n, best, newbest, bi;
	na = packages[a]->type->nouts;
	nb = packages[b]->type->nouts;
	n = ((nb > na) ? na : nb);
	while(1) {
		best = assign(a,0) + assign(b,0);
/*fprintf(stderr, " i = 0 best = %d\n", best); */
		bi = 0;
		for(i = 0; i < na ; ++i) 
			packages[a]->index[i] = 0;
		for(i = 0; i < nb ; ++i) 
			packages[b]->index[i] = 0;
		for(i = 1; i <= n ; ++i) {
			newbest = improve(a,b);
/*fprintf(stderr, " i = %d best = %d\n", i, newbest);*/
			packages[a]->index[besti] = i;
			packages[b]->index[bestj] = i;
			swap(besti, bestj);
			if(newbest < best) {
				best = newbest;
				bi = i;
			}
		}
		for(i = bi+1; i <= n; ++i) {
			for(j = 0; j < na; ++j)
				if(packages[a]->index[j] == i) break;
			for(k = 0; k < nb; ++k)
				if(packages[b]->index[k] == i) break;
			swap(j,k);
		}
		if(bi == 0) {
			return(best);
		}
	}
}

void
shuffle(void)
{
/*	int times,pkg1, pkg2, pin1, pin2;
	times = npackages * 20;
	while(times--) {
		pkg1 = pick(npackages);
		while((pkg2 = pick(npackages)) == pkg1);
		if((pin1 = packages[pkg1]->oused) <
			packages[pkg1]->type->nouts) ++pin1;
		pin1 = pick(pin1);
		if((pin2 = packages[pkg2]->oused) <
			packages[pkg2]->type->nouts) ++pin2;
		pin2 = pick(pin2);
		mostpno = pkg1;
		leastpno = pkg2;
		aouts = packages[pkg1]->outputs;
		bouts = packages[pkg2]->outputs;
		swap(pin1, pin2);
	} */
}

		
int
improve(int a,int b)
{
	int best, ft, i, j, nouta, noutb;
	aouts = packages[a]->outputs;
	bouts = packages[b]->outputs;
	best = 1000;
	nouta = packages[a]->type->nouts;
	noutb = packages[b]->type->nouts;
	for(i = 0; i < nouta; ++i) {
		if(packages[a]->index[i] != 0) continue;
		for(j = 0; j < noutb; ++j) {
			if(packages[b]->index[j] != 0) continue;
			swap(i,j);
			ft = assign(a, 0) + assign(b,0);
			if(ft <= best) {
				best= ft;
				besti = i;
				bestj = j;
			}
			swap(i,j);
		}
	}
	return(best);
}

int
fit(int a, int b)
{
	int na, nb;
	na = countpins(packages[a]) - (packages[a]->type)->nios;
	if(na > 0) na *= 10;
	nb = countpins(packages[b]) - (packages[b]->type)->nios;
	if(nb > 0) nb *= 10;
	return(na + nb);
}

int aflg, bflg;
void
swap(int i,int j)
{
	Hshtab * tmp;
	tmp = aouts[i];
	aouts[i] = bouts[j];
	bouts[j] = tmp;
}

void
printpkg(void)
{
	int i,j, cnt;
	Depend *dp;
	for(i = 0; i < npackages;++i) {
		fprintf(stderr, "package %d type %s: \n", i, packages[i]->type->name);
		for(j = 0; j < packages[i]->type->nouts; ++j) {
			if(packages[i]->outputs[j] == (Hshtab *) 0) continue;
			fprintf(stderr, "\t%s\tnum minterms = %d\t%s %s %s %s\n",
				(packages[i]->outputs[j])->name,
				(packages[i]->outputs[j])->nminterm,
				((packages[i]->outputs[j])->type&CLOCKED)?
					"CLOCKED": "COMB",
				((packages[i]->outputs[j])->type & INV)?
					"INV" : "NONINV",
				((packages[i]->outputs[j])->set)?
					"SET" : "",
				((packages[i]->outputs[j])->clr)?
					"CLR" : "");
			dp = (packages[i]->outputs[j])->depends;
			cnt = 0;
			while(dp->name) {
				if(cnt == 0) fprintf(stderr, "\t\t");
				fprintf(stderr, " %s", dp->name->name);
				if((cnt = (cnt+1)%10) == 0) fprintf(stderr, "\n");
				dp = dp->next;
			}
			if(cnt != 0)  fprintf(stderr, "\n");
		}
		fprintf(stderr, "number of pins = %d\n", countpins(packages[i])); 
	}
}

void
init(void)
{
	Package * pkg;
	int  i, pkgno, j, lastchance;

	newpkg(devtype);
	for(i = 0; i < noutputs; ++i) {
	   lastchance = 0;
	   for(pkgno = 0; ;) {
		pkg = packages[pkgno];
		for(j = 0 ; j  < (pkg->type)->nouts; ++j) {
			if(pkg->outputs[j] != (Hshtab *) 0) continue;
			pkg->outputs[j]  = outputs[i];
			if(assign(pkgno, 0) == BIG) {
				pkg->outputs[j]  = (Hshtab *) 0;
				continue;
			}
			break;
		}
		if(j != (pkg->type)->nouts) break;
		if(++pkgno == npackages) {
			if(lastchance) cantdo(i);
			lastchance = 1;
			newpkg(devtype);
		}
	   }
	}
	while(extra--)
		newpkg(devtype);
        srand((int)time((long *)0));
}	

void 
cantdo(int ono)
{
	printpkg();
	fprintf(stderr, "can't assign");
	fprintf(stderr, "\t%s\tnum minterms = %d\t%s %s %s %s\n",
		(outputs[ono])->name,
		(outputs[ono])->nminterm,
		(outputs[ono]->type&CLOCKED)?
			"CLOCKED": "COMB",
		(outputs[ono]->type & INV)?
			"INV" : "NONINV",
		(outputs[ono]->set)?
			"SET" : "",
		(outputs[ono]->clr)?
			"CLR" : "");
	BADEND;
}

void
hpinit(Hshtab *hp) {
	hp->depends = (Depend *) lmalloc((long) sizeof(Depend));
	hp->depends->name = NIL(Hshtab);
	hp->minterms = (Depend *) lmalloc((long) sizeof(Depend));
	hp->minterms->name = NIL(Hshtab);
}

Package *
newpkg(Device * devtype)
{
	Package * pkg;
	int  i;
	if(npackages > NPKG) {
		fprintf(stderr, "Too many packages\n");
		BADEND;
	}
	pkg = (Package *) lmalloc((long) sizeof(Package));
	packages[npackages++] = pkg;
	for(i = 0; i < DEVNOUTPINS; ++i) {
		pkg->outputs[i] = (Hshtab *) 0;
		pkg->index[i] = 0;
	}
	if(devtype) pkg->type = devtype;
	else if(((noutputs+extra)%12) && ((noutputs+extra)%12 <= 10))
		pkg->type = &dev22v10;
	else pkg->type = &dev26v12;
	return(pkg);
}

void
swmins(Hshtab * hp, Hshtab * hp1)
{
	Depend * dp;
	Hshtab ** hpp;
	int i;
	dp = hp->minterms;
	hp->minterms = hp1->minterms;
	hp1->minterms = dp;
	i = hp->nminterm;
	hp->nminterm = hp1->nminterm;
	hp1->nminterm = i;
	hpp = hp->ins;
	hp->ins = hp1->ins;
	hp1->ins = hpp;
	i = hp->nins;
	hp->nins = hp1->nins;
	hp1->nins = i;
	hp->type ^= INV;
}

void
rdfile(char *fname)
{
	int nminterm, suffix, nins;
	FILE *fp;
	Depend *dp;
	Hshtab *hp, *hp1, *ins[NIO*3];
	char *s, *s1, buf2[100], root[100];
	char buf[BUFSIZ];

	fp = fopen(fname, "r");
	if(fp == 0) {
		fprintf(stderr, "can't open %s\n", fname);
		BADEND;
	}
	nminterm = 0;
	nins = 0;
	hp = 0;
	while(s = fgets(buf, sizeof(buf), fp)) {
		buf[strlen(buf)-1] = 0;			/* step on newline */
		if(s[0] == '.' && s[1] == 'x' && s[2]) {
			if(devtype) continue;
			nextwd(s+2, root, &suffix);
			gettype(root);
			continue;
		}
		if(s[0] == '.' && s[1] == 'I' && s[2]) {
			nminterm = 0;
			nins = 0;
			s = nextwd(s+2, root, &suffix);
			if(suffix & (SFX_SET|SFX_CLR|SFX_ENB|SFX_CLK)) {
				hp = 0;
				continue;
			}
			strcpy(buf2, "~");
			strcat(buf2, root);
			hp = lookup(buf2,1);
			if(hp->type) {
				fprintf(stderr, "two inverts for %s\n", root);
				BADEND;
			}
			hpinit(hp);
			for(s = nextwd(s, root, &suffix);
			   *root; s = nextwd(s, root, &suffix)) {
				hp1 = lookup(root,1);
				ins[nins++] = hp1;
			}
			hp->ins = (Hshtab **)
				lmalloc((long) (sizeof(Hshtab *) * nins));
			memcpy(hp->ins, ins, (sizeof(Hshtab *) * nins));
			hp->nins = nins;
			continue;
		}
		if(s[0] == '.' && s[1] == 'o' && s[2]) {
			nminterm = 0;
			nins = 0;
			s = nextwd(s+2, root, &suffix);
			hp = lookup(root, 1);
			if((hp->type & OUTPUT) == 0) {
				hpinit(hp);
				hp->type |= OUTPUT;
				outputs[noutputs++] = hp;
			}
			if(suffix&SFX_NCN)
				hp->type |= NOCONN;
			if(suffix&SFX_INV) 
				hp->type |= INV;
			if(suffix&SFX_SET) {
				strcpy(buf2, ":");
				strcat(buf2, root);
				hp1 = lookup(buf2,1);
				if(hp1->type) {
					fprintf(stderr, "two sets for %s\n", root);
					BADEND;
				}
				hp->set = hp1;
				s1 = s;
				for(s = nextwd(s, root, &suffix);
				   *root; s = nextwd(s, root, &suffix)) {
					hp1 = lookup(root,1);
					hp1->type |= INPUT;
					if(adddep(&(hp->depends), hp1, SET)) ++hp->ndepends;
				}
				s = s1;
				hp = hp->set;
				hpinit(hp);
				hp->type |= SET;
			}
					
			if(suffix&SFX_CLR) {
				strcpy(buf2, ";");
				strcat(buf2, root);
				hp1 = lookup(buf2,1);
				if(hp1->type) {
					fprintf(stderr, "two clears for %s\n", root);
					BADEND;
				}
				hp->clr = hp1;
				s1 = s;
				for(s = nextwd(s, root, &suffix);
				   *root; s = nextwd(s, root, &suffix)) {
					hp1 = lookup(root,1);
					hp1->type |= INPUT;
					if(adddep(&(hp->depends), hp1, CLR)) ++hp->ndepends;
				}
				s = s1;
				hp = hp->clr;
				hpinit(hp);
				hp->type |= CLR;
			}
					
			if(suffix&SFX_ENB) {
				hp->type |= ENABLED;
				strcpy(buf2, "#");
				strcat(buf2, root);
				hp1 = lookup(buf2,1);
				if(hp1->type) {
					fprintf(stderr, "two enables for %s\n", root);
					BADEND;
				}
				hp1->type = ENABLE;
				hp->enab = hp1;
				s1 = s;
				for(s = nextwd(s, root, &suffix);
				   *root; s = nextwd(s, root, &suffix)) {
					hp1 = lookup(root,1);
					hp1->type |= INPUT;
					if(adddep(&(hp->depends), hp1, ENABLE)) ++hp->ndepends;
				}
				s = s1;
				hp = hp->enab;
				hpinit(hp);
			}
					
			if(suffix&SFX_CLK) {
				s = nextwd(s, root, &suffix);
				if(*root == 0) {
					fprintf(stderr, "clock for %s empty string\n",
						hp->name);
					continue;
				}
				hp1 = lookup(root,1);
				hp1->type |= CLOCK;
				if(adddep(&(hp->depends), hp1, CLOCK)) ++hp->ndepends;
				hp->type |= CLOCKED;
				hp->clock = hp1;
				hp = 0;
				*s = 0;
				continue;
			}
			for(s = nextwd(s, root, &suffix);
			   *root; s = nextwd(s, root, &suffix)) {
				hp1 = lookup(root,1);
				ins[nins++] = hp1;
				hp1->type |= INPUT;
				if(adddep(&(hp->depends), hp1, INPUT)) ++hp->ndepends;
			}
			hp->ins = (Hshtab **)
					lmalloc((long) (sizeof(Hshtab *) * nins));
			memcpy(hp->ins, ins, (sizeof(Hshtab *) * nins));
			hp->nins = nins;
			continue;
		}
		else
			if(s[0] == '.')
				hp = 0;
		if(hp == 0) continue;
		for(s = nextwd(s,root,&suffix); *root; s = nextwd(s,root,&suffix)) {
			dp = getmins(root, ins, nins);
			dp->next = hp->minterms;
			hp->minterms = dp;
			hp->nminterm = ++nminterm;
		}
	}
	fclose(fp);
	for(hp = hshtab; hp < &hshtab[HSHSIZ]; hp++) {
		if(hp->name && hp->name[0] == '~') {
			hp1 = lookup(&hp->name[1], 0);
			if(!hp1) {
				fprintf(stderr, "%s has invert only\n", &hp->name[1]);
				BADEND;
			}
			if((hp->nminterm < hp1->nminterm)
			   && !(hp1->set)
			   && !(hp1->clr))
				swmins(hp1, hp);
		}
	}
}

				
Depend *
getmins(char * s, Hshtab **ins, int nins)
{
	char *s1, buf[1000];
	Depend * dp;
	int index, i, term, mask;
	for(s1 = s; *s1 && (*s1 != ':'); ++s1);
	if(*s1 == 0) {
		fprintf(stderr, "getmins: no : in %s\n", s);
		BADEND;
	}
	*s1++ = 0;
	term = atoi(s);
	mask = atoi(s1);
	s1 = buf;
	*s1 = 0;
	for(i = 0, index = 1; i < nins; ++i, index <<= 1) {
		if((index & mask) == 0) continue;
		*s1++ = ' ';
		if(!(term & index))  *s1++ = '!';
		strcpy(s1, ins[i]->name);
		while(*s1 != 0) ++s1;
	}
	dp = (Depend *) lmalloc((long) sizeof(Depend));
	dp->name = lookup(buf, 1);
	return(dp);
}	
		

char *
nextwd(char *s, char *rtp, int* sfxp)
{
	char *s0, *s1;
	s0 = s;
	*sfxp = 0;
	if(*s == 0) {
		*rtp = 0;
		return((char *) 0);
	}
	while(*s && ((*s == ' ') || (*s == '\t'))) ++s;
	while(*s && (*s != ' ') && (*s != '\t') && (*s != '\n')) 
		*rtp++ = *s++;
	while(*s == '\n') {
		*sfxp |= SFX_EOL;
		++s;
	}
	for(s1 = s - 1; s1 >= s0; --s1, --rtp) 
		switch(*s1) {
		case '~':
			*sfxp |= SFX_INV;
			break;
		case '<':
			*sfxp |= SFX_CLR;
			break;
		case '>':
			*sfxp |= SFX_SET;
			break;
		case '$':
			*sfxp |= SFX_CLK;
			break;
		case '*':
			*sfxp |= SFX_ENB;
			break;
		case '#':
			*sfxp |= SFX_NCN;
			break;
		case '\n':
			break;
		default:
			if((s1 > s0) && (*(s1-1) == '@')) {
				switch(*s1) {
			case 'i':
				*sfxp |= SFX_INV;
				break;
			case 'c':
				*sfxp |= SFX_CLR;
				break;
			case 's':
				*sfxp |= SFX_SET;
				break;
			case 'd':
				*sfxp |= SFX_CLK;
				break;
			case 'e':
				*sfxp |= SFX_ENB;
				break;
			case 'b':
				*sfxp |= SFX_NCN;
				break;
				}
				--rtp;
				--s1;
			}
			else {
		 		*rtp = 0;
				return(s);
			}
		}
	fprintf(stderr, "nextwd: error\n"); BADEND;
	return((char *) 0);
}
		
				
/*
 * hash table routine
 */
Hshtab*
lookup(char *string, int ent)
{
	Hshtab *rp;
	char *sp;
	int ihash;

	/*
	 * hash
	 * take one's complement of every other char
	 */
	ihash = 0;
	for(sp=string; *sp; sp++)
		ihash = ihash+ihash+ihash + *sp;
	if(ihash < 0)
		ihash = ~ihash;
	rp = &hshtab[ihash%HSHSIZ];


	/*
	 * lookup
	 */
	while(sp = rp->name) {
		if(strcmp(string, sp) == 0)
			return rp;
		rp++;
		if(rp >= &hshtab[HSHSIZ])
			rp = hshtab;
	}
	if(ent == 0)
		return 0;
	hshused++;
	if(hshused >= HSHSIZ) {
		fprintf(stderr, "Symbol table overflow\n");
		BADEND;
	}
	rp->name = f_strdup(string);
	return rp;
}
int
adddep(Depend **dpp, Hshtab *hp, short type)
{
	Depend *dp, *ndp;
	dp = *dpp;
	while(hp < dp->name)  {
		dpp = &(dp->next);
		dp = *dpp;
	}
	if(hp == dp->name) {
		dp->type |= type;
		return(0);
	}
	ndp = (Depend *) lmalloc((long) sizeof(Depend));
	ndp->next = dp;
	ndp->name = hp;
	ndp->type = type;
	*dpp = ndp;
	return(1);
}

Depend dps[NIO];
int npins;

int
countpins(Package *pkg)
{
	Depend *dp2;
	int i;
	register Depend **dpp1;
	register Hshtab * name2;
	npins = 0;
	dpstart = dps;
	dpstart->name = NIL(Hshtab);
	for(i = 0; i < pkg->type->nouts; ++ i) {
		dpp1 = &dpstart;
		if(pkg->outputs[i] == 0) continue;
		dp2 = pkg->outputs[i]->depends;
		enter(dpp1, pkg->outputs[i], 0);
		for(;name2 = dp2->name; dp2 = dp2->next)
			enter(dpp1, name2, 0);
	}
	return(npins);
}


int
countins(Package *pkg)
{
	Depend *dp2;
	int i;
	register Depend **dpp1;
	register Hshtab * name2;
	npins = 0;
	dpstart = dps;
	dpstart->name = NIL(Hshtab);
	for(i = 0; i < pkg->type->nouts; ++ i) {
		dpp1 = &dpstart;
		if(pkg->outputs[i] == 0) continue;
		dp2 = pkg->outputs[i]->depends;
		for(;name2 = dp2->name; dp2 = dp2->next)
			enter(dpp1, name2, dp2->type);
	}
	return(npins);
}

void
enter(Depend **dpp, Hshtab *name, short type)
{
	register Depend *dp;
	register int i;
	dp = *dpp;
	while(name < dp->name) {
		dpp = &(dp->next);
		dp = *dpp;
	}
	if(name == dp->name) {
		dp->type |= type;
		return;
	}
	if(npins >= NIO) return;
	dps[i = ++npins].next = dp;
	dps[i].name = name;
	dps[i].type = type;
	*dpp = &dps[i];
	return;
}


int
remov(Depend **dpp, Hshtab *name, short type)
{
	register Depend *dp;
	dp = *dpp;
	while(name < dp->name) {
		if(dp->name == NIL(Hshtab)) return(0);
		dpp = &(dp->next);
		dp = *dpp;
	}
	if(name == dp->name) {
		if(dp->type & type)
			dp->type &= ~ type;
		else
			return(0);
		if(dp->type == 0)
			*dpp = dp->next;
		return(1);
	}
	return(0);
}


struct pinassgn pinassgn[NIO];
void
out(int pkgno)
{
	if(assign(pkgno, 1) == BIG) return;
	outp(pkgno);
	outm(pkgno);
	outj(pkgno);
}

int
assign(int pkgno, int final)
{
	int i, j, pno, key, msk, cost;
	char buf[200];
	Package *pp;
	Depend * dpp;
	Hshtab *hp, **outputs, *hp_clock, *hp_enab, *hp1;
	cost = 0;
	hp_clock = 0;
	hp_enab = 0;
	if(!pflag) 
		for(i = 0; i < NIO; ++i) {
			pinassgn[i].type = 0;
			pinassgn[i].name = (Hshtab *) 0;
		}
	pp = packages[pkgno];
	outputs = pp->outputs;
	devtype = pp->type;
	countins(pp);
/* dpstart points to input list */
/* first assign outputs */
	for(j = 0; j < pp->type->nouts; ++j) {
		if((hp = *outputs) == (Hshtab *) 0) {
			outputs++;
			continue;
		}
		hp->mypkg = pkgno;
		msk = INPUT|OUTPUT;
		key = OUTPUT;
		if(remov(&dpstart, hp, INPUT)) key |= INPUT;
		if(remov(&dpstart, hp, ENABLE)) key |= INPUT;
		if(remov(&dpstart, hp, SET)) key |= INPUT;
		if(remov(&dpstart, hp, CLR)) key |= INPUT;
		if(hp->type & CLOCKED) {
			key |= CLOCKED;
			msk |= CLOCKED;
			if(hp_clock && (hp_clock != hp->clock)) {
				fprintf(stderr, "2 clocks %s and %s on pkq%d\n",
					hp_clock, hp->clock, pkgno);
			}
 			else hp_clock = hp->clock;
		}
		else {
			key |= COMB;
			msk |= COMB;
		}
		if(hp->type & INV) {
			key |= INV;
			msk |= INV;
		}
		else {
			key |= NONINV;
			msk |= NONINV;
		}
		if(!(pno = find(key, msk, hp->nminterm, hp))) {
			if(!(key & INPUT)) {
				key |= INPUT;  
				pno = find(key, msk, hp->nminterm, hp);
				if(!pno) 
					key &= ~INPUT;
			}
			if(!pno) {
				buf[0] = '~';
				strcpy(&buf[1], hp->name);
				if(hp1 = lookup(buf, 0)) {
					swmins(hp, hp1);
					msk ^= (INV | NONINV);
					key ^= (INV | NONINV);
					pno = find(key, msk, hp->nminterm, hp);
					if(!pno && !(key & INPUT)) {
						key |= INPUT;  
					}
				}
			}
		}
		if(!(pno = find(key, msk, hp->nminterm, hp))) {
			if(!(key & INPUT)) {
				key |= INPUT;  
				pno = find(key, msk, hp->nminterm, hp);
			}
		}
		if(pno) {
			pinassgn[pno].name = hp;
			pinassgn[pno].type = key;
if(DEBUG)
	fprintf(stderr, "output assign: %s type 0x%x key 0x%x pno = %d\n", hp->name, hp->type, key, pno);
		}
		else {
		    if(final)
			fprintf(stderr, "can't assign %s key 0x%x mask 0x%x\n", hp->name, key, msk);
		    else return(BIG);
		}
/* Global enable */
		if((hp->type & ENABLED)
		   && (devtype->pins[pno-1].type & ENABLED)) {
			hp1 = getenab(hp->enab);
			if(hp_enab && (hp_enab != hp1)) {
			   if(final)
				fprintf(stderr, "2 global enables %s and %s on pkq%d\n",
					hp_enab->name, hp->enab->name, pkgno);
			   else return(BIG);
			}
 			else hp_enab = hp1;
		}
		++outputs;
	}
/* now remaining inputs */
/* first clock, if any */
	if(hp = hp_clock) {
		if((pno = find(CLOCK, CLOCK, 0, hp)) == 0) {
		    if(final)
			fprintf(stderr, "can't assign %s clock\n", hp->name);
		    else return(BIG);
		}
		if((devtype->pins[pno-1].type & PINPUT)
			&& remov(&dpstart, hp, INPUT)) {
				remov(&dpstart, hp, CLOCK);
				pinassgn[pno].name = hp;
				pinassgn[pno].type = INPUT|CLOCK;
if(DEBUG)
	fprintf(stderr, "clk assign: %s key 0x%x pno = %d\n", hp->name, INPUT|CLOCK, pno);
		}
		else {
			pinassgn[pno].name = hp;
			pinassgn[pno].type = CLOCK;
			remov(&dpstart, hp, CLOCK);
if(DEBUG)
	fprintf(stderr, "clk assign: %s key 0x%x pno = %d\n", hp->name, CLOCK, pno);
		}
	}
/* global enables */
	if(hp = hp_enab) {
		if((pno = find(ENABLE, ENABLE, 0, hp)) == 0) {
		    if(final)
			fprintf(stderr, "can't assign %s global enable\n", hp->name);
		    else return(BIG);
		}
		if((devtype->pins[pno-1].type & PINPUT)
			&& remov(&dpstart, hp, INPUT)) {
				remov(&dpstart, hp, ENABLE);
				pinassgn[pno].name = hp;
				pinassgn[pno].type = ENABLE|INPUT;
if(DEBUG)
	fprintf(stderr, "global enab assign: %s key 0x%x pno = %d\n", hp->name, ENABLE|INPUT, pno);
		}
		else {
			remov(&dpstart, hp, ENABLE);
			pinassgn[pno].name = hp;
			pinassgn[pno].type = ENABLE;
if(DEBUG)
	fprintf(stderr, "global enab assign: %s key 0x%x pno = %d\n", hp->name, ENABLE, pno);
		}
	}
/* rest */
	for(dpp = dpstart; dpp->name; dpp = dpp->next) {
		if((dpp->type & (ENABLE | CLR | SET | INPUT)) == 0) continue;
		hp = dpp->name;
		if((pno = find(PINPUT, PINPUT, 0, hp)) == 0) {
			if(final) 
			   fprintf(stderr, "can't assign %s key 0x%x mask 0x%x\n", hp->name, dpp->type, PINPUT);
			else cost += CANTFIT;
		}
		pinassgn[pno].name = hp;
		pinassgn[pno].type = INPUT;
if(DEBUG)
	fprintf(stderr, "input assign: %s key 0x%x pno = %d\n", hp->name, dpp->type, pno);
	}
	if(final) if(pinassgn[0].type) fprintf(stderr, "not assigned %s\n", pinassgn[0].name->name);
	if(!final) while((pno=find(PINPUT, PINPUT, 0, (Hshtab *) 0)) != 0) {
		pinassgn[pno].type = INPUT;
		--cost;
	}
	return(cost);
}

void
outm(int pkgno)
{
	int i, npins, type, cflg, sflg;
	FILE *fp;
	char buf[100];
	Hshtab * hp;
	cflg = sflg = 0;
	npins = devtype->npins;
	if(npackages == 1) sprint(buf, "%s.m", name);
	else sprint(buf, "%s%2.2d.m", name, pkgno);
	fp = fopen(buf, "w");
	if(fp == 0) {
		fprintf(stderr, "can't open %s\n", buf);
		BADEND;
	}
	fprintf(fp, ".x %s\n", &devtype->name[3]);
	for(i = 1;  i <= npins; ++i) {
		if(pinassgn[i].type & OUTPUT) {
			putmins(fp, (hp = pinassgn[i].name) ,i);
			if(((type = hp->type)
			    & (CLOCKED|INV)) == CLOCKED)
				fprintf(fp, devtype->pins[i-1].clk, i,i,i,i);
			else  if((type & (CLOCKED|INV)) == 0)
				fprintf(fp, devtype->pins[i-1].cmb, i,i,i,i);
			else  if((type & (CLOCKED|INV)) == INV)
				fprintf(fp, devtype->pins[i-1].cmbinv, i,i,i,i);
			else  if((type & (CLOCKED|INV)) == CLOCKED|INV)
				fprintf(fp, devtype->pins[i-1].clkinv, i,i,i,i);
			if((devtype->pins[i-1].type & ENABLED) == 0) {
				if(hp->enab) putmins(fp, hp->enab, 100+i);
				else fprintf(fp, ".o %d\n 0:0\n", 100+i);
			}
			if(hp->clr && !(cflg++))
				putmins(fp, hp->clr, find(CLR, CLR, 0, hp->clr));
			if(hp->set && !(sflg++))
				putmins(fp, hp->set, find(SET, SET, 0, hp->set));
		}
		else if((devtype->pins[i-1].type & OUTPUT|INPUT) == OUTPUT|INPUT) 
			fprintf(fp, devtype->pins[i-1].cmb, i,i,i,i);
	}
	fclose(fp);
}
			
Hshtab *
getenab(Hshtab * hp)
{
	if(hp->nins != 1) {
		fprintf(stderr, "no enable for %s\n", hp->name);
		return((Hshtab *) 0);
	}
	else return(hp->ins[0]);
}

#define CNT 8			
void
putmins(FILE *fp, Hshtab * hp, int first)
{
	char *s, *s1, buf[100];
	int i, index, mterm, foo, mask, term, count, inv, flip;
	Depend * dp;
	Hshtab *hp0;
	flip = 0;
	fprintf(fp, ".o %d", first);
	for(i = 0; i < hp->nins; ++i) {
		fprintf(fp, " %d", findpin(hp0 = hp->ins[i]));
		if(packages[hp->mypkg]->type == &dev22v10)
			if(hp->mypkg == hp0->mypkg)
				if((hp0->type & (CLOCKED|INV)) == CLOCKED)
					flip |= 1 << i;
	}
	fprintf(fp, "\n");
	count = 0;
	for(mterm = 0, dp = hp->minterms; mterm < hp->nminterm; ++mterm, dp = dp->next) {
		i = 0;
		index = 1;
		mask = 0;
		term = 0;
		s = dp->name->name;
		for(s = nextwd(s, buf, &foo); buf[0]; s = nextwd(s, buf, &foo)) {
			if(buf[0] == '!') {
				inv = 1;
				s1 = &buf[1];
			}
			else {
				inv = 0;
				s1 = buf;
			}
			while(strcmp(s1, hp->ins[i]->name)) {
				++i;
				index <<= 1;
				if(i == hp->nins) {
					fprintf(stderr, "putmins: can't find input %s in %s",
						s1, hp->name);
					BADEND;
				}
			}
			mask |= index;
			term |= index & (inv ? flip : ~flip);
		} 
		fprintf(fp, " %d:%d", term, mask);
		if((++count)%CNT == 0) fprintf(fp, "\n");
	}
	if((count%CNT)) fprintf(fp, "\n");
} 

int
findpin(Hshtab *hp)
{
	int i;
	for(i = 1; i <= devtype->npins; ++i)
		if(pinassgn[i].name == hp) return(i);
	return(0);
}

void
outp(int pkgno)
{
	int npins;
	FILE *fp;
	char buf[100];
	struct pinassgn *pnp;
	Pin * pins;
	if(npackages == 1) sprint(buf, "%s.p", name);
	else sprint(buf, "%s%2.2d.p", name, pkgno);
	fp = fopen(buf, "w");
	if(fp == 0) {
		fprintf(stderr, "can't open %s\n", buf);
		BADEND;
	}
	pnp = &pinassgn[1];
	npins = devtype->npins;
	pins = devtype->pins;
	if(npackages == 1)
	   fprintf(fp, ".t	%s	%s %%%s\n", name, devtype->pkgtype, devtype->name);
	else
	   fprintf(fp, ".t	%s%2.2d	%s %%%s\n", name, pkgno, devtype->pkgtype, devtype->name);
	fprintf(fp, ".tt ");
	while(npins) {
		if((npackages == 1) && pnp->name && (pnp->name->type & NOCONN))
			fprintf(fp, "n");
		else 
			if(pnp->type) {
				if(pnp->type&OUTPUT) {
					if(pnp->name->type & ENABLED) fprintf(fp, "3");
					else fprintf(fp, "2");
				}
				else fprintf(fp, "i");
			}
		else {
			if(pins->type & VCC) fprintf(fp, "v");
			else if(pins->type & GND) fprintf(fp, "g");
			else if(pins->type & PMASK) fprintf(fp, "n");
		}
		npins--;
		pnp++;
		pins++;
	}
	fprintf(fp, "\n");
	pnp = &pinassgn[1];
	npins = devtype->npins;
	while(npins) {
		if((npackages == 1) && (pnp->name != (Hshtab *) 0)
		  && (pnp->name->type & NOCONN))
			fprintf(fp, "%%");
		if(pnp->type) fprintf(fp, ".tp	%s	%d\n", pnp->name->name,
				pnp-pinassgn);
		npins--;
		pnp++;
	}
	fclose(fp);
}
	
			
int
find(short key, short msk, short nm, Hshtab *hp)
{
	int bestmt, besti, npins;
	Pin *pins;
	struct pinassgn *pnp;
	bestmt = 1000;
	besti = 0;
	if(pflag) {
		pins = devtype->pins;
		npins = devtype->npins;
		pnp = &pinassgn[1];
		while(npins) {
			if(pnp->name == hp) {
				if((pins->type & msk) == key) 
					if((pins->nminterms >= nm))
						return(pnp-pinassgn);
				return(0);
			}
			npins--;
			pins++;
			pnp++;
		}
	}
	pins = devtype->pins;
	npins = devtype->npins;
	pnp = &pinassgn[1];
	while(npins) {
		if((pins->type & msk) == key) 
			if(!(pnp->name) && !(pnp->type))
				if((pins->nminterms >= nm) &&
					(pins->nminterms < bestmt)) {
						besti = pnp-pinassgn;
						bestmt = pins->nminterms;
				}
		npins--;
		pins++;
		pnp++;
	}
	return(besti);
}

#define X0 64
#define X1 160
#define X2 192
#define X3 288
#define X4 408
#define X5 504
#define X6 536
#define X7 632
#define XMID 344
#define Y0 96
#define DELTA 8
#define DELTA2 16
#define FMT0 8, 1, 4, 16
#define FMT1 1, 64, 128, 4
#define FMT2 8, 1, 4, 16
#define FMT3 1, 64, 128, 4

static int y;
FILE *fpj;

void
jstart(void)
{
	char buf[100];
	if(npackages == 1) return;
	sprint(buf, "%s.j", name);
	fpj = fopen(buf, "w");
	if(fpj == 0) {
		fprintf(stderr, "can't open %s\n", buf);
		BADEND;
	}
	y = Y0;
}


void
outj(int pkgno)
{
	int npins, ystart, yinp, youtp;
	char buf[100], buf0[100];
	struct pinassgn *pnp;

	if(npackages == 1) return;
	sprint(buf, "%s%2.2d", name, pkgno);
	sprint(buf0, "%c%2.2d", 'I', pkgno);
	y += DELTA2;
	ystart = y;
	yinp = youtp = y;
	pnp = &pinassgn[1];
	npins = devtype->npins;
	while(npins) {
		if(pnp->type & OUTPUT)
			outwire(pnp->name->name,
				youtp += DELTA2, X4, X5, FMT2);
		else if(pnp->type)
			outwire(pnp->name->name,
				yinp += DELTA2, X2, X3, FMT1);
		npins--;
		pnp++;
	}
	y = (yinp > youtp) ? yinp : youtp;
	y += DELTA2;
	fprintf(fpj, "t %s@ 2 %s@ 32 %d %d\n", buf0, buf, XMID, y);
	y += DELTA2;
	fprintf(fpj, "b %d %d %d %d\n", X3, ystart, X4, y);
} 

void
outwire(char *name, int y, int x0, int x1, int arg0, int arg1, int arg2, int arg3)
{
	fprintf(fpj, "w %d %d %d %d\n", x0, y, x1, y);
	fprintf(fpj, "t %s@ %d @ %d %d %d\n", name, arg0, arg1, x0, y);
	fprintf(fpj, "t %s@ %d @ %d %d %d\n", name, arg2, arg3, x1, y);
}

void
jend(void)
{
	int pkgno, outno;
	Depend *dp2;
	register Depend **dpp1;
	register Hshtab * name2;
	if(npackages == 1) return;
	npins = 0;
	dpstart = dps;
	dpstart->name = NIL(Hshtab);
	dpp1 = &dpstart;
	fprintf(fpj, "z %d %d %d %d\n", X0, Y0, X7, y + DELTA);
	y = Y0;
	for(pkgno = 0; pkgno < npackages; ++pkgno) {
		for(outno = 0; outno < packages[pkgno]->type->nouts; ++outno) {
			if(packages[pkgno]->outputs[outno] == (Hshtab *) 0)
				continue;
			if(!((packages[pkgno]->outputs[outno])->type & NOCONN))
			outwire((packages[pkgno]->outputs[outno])->name,
				y += DELTA2, X6, X7, FMT3);
			dp2 = (packages[pkgno]->outputs[outno])->depends;
			for(;name2 = dp2->name; dp2 = dp2->next)
				enter(dpp1, name2, 0);
		}
	}
	y = Y0;
	for(dp2 = dpstart; dp2->name; dp2 = dp2->next) {
		if(dp2->name->type & (NOCONN | OUTPUT)) continue;
		outwire(dp2->name->name,
				y += DELTA2, X0, X1, FMT0);
			
	}
}
int
pick(int n)
{
	int ret;
	ret = rand()%n;
	if(ret < 0) ret = -ret;
	return(ret);
}

void
assigned(void)
{
	char buf[100];
	int pkgno;
	FILE *pfp;
	pkgno = 0;
	sprint(buf, "%s.p", name);
	pfp = fopen(buf, "r");
	npackages = 1;
	if(pfp != 0) {
		passign(0,pfp);
		fclose(pfp);
printpkg();
		out(0);
		return;
	}
	npackages = 2;
	jstart();
	sprint(buf, "%s%2.2d.p", name, pkgno);
	pfp = fopen(buf, "r");
	while(pfp != 0) {
		passign(pkgno,pfp);
		fclose(pfp);
		npackages = 2;
		out(pkgno);
		++pkgno;
		sprint(buf, "%s%2.2d.p", name, pkgno);
		pfp = fopen(buf, "r");
	}
	npackages = pkgno;
	jend();
}

char ttline[NIO];

void
passign(int pkgno, FILE *fp) {
	Package * pkg;
	int oused;
	char *ttp;
	oused = 0;
	readp(fp);
	npackages = pkgno;
	pkg = newpkg(devtype);
	for(ttp = ttline; *ttp; ++ttp) {
		if((*ttp == '2') || (*ttp == '3') || (*ttp == '4') || (*ttp == 'b')) {
			if(oused == devtype->nouts) {
				fprintf(stderr, "too many outputs for devtype %s\n", devtype->name);
				BADEND;
			}
			pkg->outputs[oused++] = pinassgn[(ttp-ttline)+1].name;
			if(*ttp == 'b') pinassgn[(ttp-ttline)+1].name = (Hshtab *) 0;
		}
	}
}

void
readp(FILE *fp) {
	char *s;
	int foo,pinno,i,isburied;
	Hshtab * hp;
	char root[100];
	char buf[BUFSIZ];

	for(i = 0; i < NIO; ++i) {
		pinassgn[i].type = 0;
		pinassgn[i].name = (Hshtab *) 0;
	}
	while(s = fgets(buf, sizeof(buf), fp)) {
		buf[strlen(buf)-1] = 0;		/* step on newline */
		isburied = 0;
		if(*s != '.' || *(s+1) != 't') {
			if(*s++ == '%')
				if(*s != '.' || *(s+1) != 't')
					continue;
			isburied = 1;
		}
		switch(*(s = s+2)) {
		case ' ':
		case '\t':
			while(*s && (*s != '%')) ++s;
			if(*s++) {
				if((*s == 'P') && (*(s+1) == 'A') && (*(s+2) == 'L')) s += 3;
				gettype(s);
			}
			break;
		case 't':
			++s;
			while(*s && ((*s == ' ') || (*s == '\t'))) ++s;
			strcpy(ttline, s);
			break;
		case 'p':
			s = nextwd(s+1, root, &foo);
			if(*root) {
				if((hp = lookup(root, 0)) == 0) {
					fprintf(stderr, "can't lookup pin name %s\n", root);
					BADEND;
				}
				nextwd(s, root, &foo);
				pinno = atoi(root);
				if(pinno < NIO) {
					pinassgn[pinno].name = hp;
					if(isburied) ttline[pinno-1] = 'b';
				}
			}
			break;
		}
	}
}
				
			
			

/* cistrcmp -- Case Insensitive strcmp */

#define mklow(c) (((c) >= 'A' && (c) <= 'Z') ? (c) + 'a' - 'A' : (c))

int cistrcmp (char *s1, char *s2)
{
    if (s1 == NULL || s2 == NULL) {
    fprintf(stderr, "cistrcmp: Can't compare NULL string!\n");
	return -1;
    } /* if NULL */

    while (*s1 && mklow (*s1) == mklow (*s2))
	s1++, s2++;

    return (mklow (*s1) < mklow (*s2)) ? -1 : ((mklow (*s1) == mklow (*s2)) ?
	    0 : 1);
} /* cistrcmp */


void
prtype(void)
{
	int i;
	for(i =	0; types[i].name != (char *) 0; ++i)
		fprintf(stderr, "%s\n", types[i].device->name);
}

void
gettype(char * t)
{
	int i;
	for(i =	0; types[i].name != (char *) 0; ++i) 
			if(cistrcmp(t, types[i].name) == 0) {
				fprintf(stderr, "type = %s\n", types[i].device->name);
				devtype = types[i].device;
				return;
			}
	fprintf(stderr, "Unknown type %s\n", t);
	BADEND;
	return;
} /* gettype */
