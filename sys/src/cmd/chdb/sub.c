#include	"ddb.h"

void
searchposit(Xor* xp, long lb, long hb, long ng, int inv)
{
	long i, k1, k2, k;
	ulong xa, xb, *mw, mask;
	Game g;
	short *gp;
	int j, from, to, mov, typ, p, f, init;
	Str *s, tmp;

	init = 0;
	for(k=0; k<xp->cursize; k+=2)
		if(xp->words[k+0] == initpxora)
		if(xp->words[k+1] == initpxorb)
			init++;
	mw = &xp->words[xp->cursize];
	mask = xp->mask;
	ngames = ng;
	s = &str[lb];
	for(i=lb; i<hb; i++,s++) {
		f = 0;
		if(init) {
			s->position = 0;
			f = 1;
		}
		decode(&g, s);
		ginit(&initp);
		gp = g.moves;
		xa = initpxora;
		xb = initpxorb;
		for(j=0; j<g.nmoves; j++) {
			xa ^= btmxora;
			xb ^= btmxorb;
			mov = *gp++;
			from = (mov>>6) & 077;
			to = (mov>>0) & 077;

			p = board[to] & 017;
			xa ^= xora[(p<<6) | to];		/* old to */
			xb ^= xorb[(p<<6) | to];

			p = board[from] & 017;
			xa ^= xora[(p<<6) | from];		/* old from */
			xb ^= xorb[(p<<6) | from];

			xa ^= xora[(p<<6) | to];		/* new to */
			xb ^= xorb[(p<<6) | to];
			xa ^= xora[from];			/* new from */
			xb ^= xorb[from];

			board[to] = p;
			board[from] = 0;

			typ = (mov>>12) & 017;
			switch(typ) {
			case TNPRO:
				xa ^= xora[(p<<6) | to];	/* old to */
				xb ^= xorb[(p<<6) | to];
				p = WKNIGHT | (p&BLACK);
				xa ^= xora[(p<<6) | to];	/* new to */
				xb ^= xorb[(p<<6) | to];
				board[to] = p;
				break;
			case TBPRO:
				xa ^= xora[(p<<6) | to];	/* old to */
				xb ^= xorb[(p<<6) | to];
				p = WBISHOP | (p&BLACK);
				xa ^= xora[(p<<6) | to];	/* new to */
				xb ^= xorb[(p<<6) | to];
				board[to] = p;
				break;
			case TRPRO:
				xa ^= xora[(p<<6) | to];	/* old to */
				xb ^= xorb[(p<<6) | to];
				p = WROOK | (p&BLACK);
				xa ^= xora[(p<<6) | to];	/* new to */
				xb ^= xorb[(p<<6) | to];
				board[to] = p;
				break;
			case TQPRO:
				xa ^= xora[(p<<6) | to];	/* old to */
				xb ^= xorb[(p<<6) | to];
				p = WQUEEN | (p&BLACK);
				xa ^= xora[(p<<6) | to];	/* new to */
				xb ^= xorb[(p<<6) | to];
				board[to] = p;
				break;
			case TOO:
				from = to+1;
				to--;
				goto mv;
			case TOOO:
				from = to-2;
				to++;
			mv:
				p = board[to] & 017;
				xa ^= xora[(p<<6) | to];		/* old to */
				xb ^= xorb[(p<<6) | to];

				p = board[from] & 017;
				xa ^= xora[(p<<6) | from];		/* old from */
				xb ^= xorb[(p<<6) | from];

				xa ^= xora[(p<<6) | to];		/* new to */
				xb ^= xorb[(p<<6) | to];
				xa ^= xora[from];			/* new from */
				xb ^= xorb[from];

				board[to] = p;
				board[from] = 0;
				break;
			case TENPAS:
				if(j & 1)
					to -= 8;
				else
					to += 8;
				p = board[to] & 017;
				xa ^= xora[(p<<6) | to];		/* old to */
				xb ^= xorb[(p<<6) | to];
				xa ^= xora[to];				/* new to */
				xb ^= xorb[to];
				board[to] = 0;
				break;
			}
			if(mw[(xa>>5) & mask] & (1 << (xa&31))) {
				k1 = 0;
				k2 = xp->cursize;
				for(;;) {
					k = ((k1+k2) / 4) * 2;
					if(k <= k1)
						break;
					if(xa > xp->words[k+0]) {
						k1 = k;
						continue;
					}
					if(xa < xp->words[k+0]) {
						k2 = k;
						continue;
					}
					if(xb >= xp->words[k+1]) {
						k1 = k;
						continue;
					}
					k2 = k;
				}
				if(xa == xp->words[k+0])
				if(xb == xp->words[k+1]) {
					s->position = j+1;
					f = 1;
				}
			}
		}
		if(inv)
			f = !f;
		if(f) {
			tmp = *s;
			*s = str[ngames];
			str[ngames] = tmp;
			ngames++;
		}
	}
}

void
searchre(Reprog *re, long afield, long lb, long hb, long ng, int inv)
{
	Str *s, tmp;
	long i, field, j;
	Game g;
	char *fld;
	int f;

	if(afield == 0)
		afield = (1<<Bywhite) | (1<<Byblack);
	ngames = ng;
	s = &str[lb];
	for(i=lb; i<hb; i++,s++) {
		f = 0;
		decode(&g, s);
		field = afield;
		while(field) {
			j = field & ~(field-1);
			field &= ~j;
			switch(j) {
			default:
				continue;
			case 1<<Bywhite:
				fld = g.white;
				break;
			case 1<<Byblack:
				fld = g.black;
				break;
			case 1<<Byevent:
				fld = g.event;
				break;
			case 1<<Byopening:
				fld = g.opening;
				break;
			case 1<<Bydate:
				fld = g.date;
				break;
			case 1<<Byfile:
				fld = g.file;
				break;
			case 1<<Byresult:
				fld = g.result;
				break;
			}
			f = regexec(re, fld, 0, 0);
			if(f)
				break;
		}
		if(inv)
			f = !f;
		if(f) {
			tmp = *s;
			*s = str[ngames];
			str[ngames] = tmp;
			ngames++;
		}
	}
}

void
searchmark(int ch, long lb, long hb, long ng, int inv)
{
	Str *s, tmp;
	long i;
	ulong b;

	b = genmark(ch);
	ngames = ng;
	s = &str[lb];
	if(inv) {
		for(i=lb; i<hb; i++,s++)
			if(!(s->mark & b)) {
				tmp = *s;
				*s = str[ngames];
				str[ngames] = tmp;
				ngames++;
			}
	} else {
		for(i=lb; i<hb; i++,s++)
			if(s->mark & b) {
				tmp = *s;
				*s = str[ngames];
				str[ngames] = tmp;
				ngames++;
			}
	}
}

void
searchgp(ushort *gp, long lb, long hb, long ng, int inv)
{
	Str *s, tmp;
	long i;

	ngames = ng;
	s = &str[lb];
	if(inv) {
		for(i=lb; i<hb; i++,s++)
			if(s->gp != gp) {
				tmp = *s;
				*s = str[ngames];
				str[ngames] = tmp;
				ngames++;
			}
	} else {
		for(i=lb; i<hb; i++,s++)
			if(s->gp == gp) {
				tmp = *s;
				*s = str[ngames];
				str[ngames] = tmp;
				ngames++;
			}
	}
}

void
searchall(ushort *gp, long lb, long hb, long ng, int inv)
{
	Str *s, tmp;
	long i;

	USED(gp);
	ngames = ng;
	s = &str[lb];
	if(!inv)
		for(i=lb; i<hb; i++,s++) {
			tmp = *s;
			*s = str[ngames];
			str[ngames] = tmp;
			ngames++;
		}
}

void
search(Node *n, long lb, long hb, long ng, int inv)
{

	switch(n->op) {
	default:
		yyerror("unknown search");
		break;
	case Nnull:
		break;
	case Nmark:
		searchmark(n->mark, lb, hb, ng, inv);
		break;
	case Nrex:
		searchre(n->rex, n->field, lb, hb, ng, inv);
		break;
	case Ngame:
		searchgp(n->gp, lb, hb, ng, inv);
		break;
	case Nall:
		searchall(n->gp, lb, hb, ng, inv);
		break;
	case Nposit:
		searchposit(n->xor, lb, hb, ng, inv);
		break;

	case Nand:
		if(inv)
			goto or;
	and:
		search(n->left, lb, hb, ng, inv);
		search(n->right, ng, ngames, ng, inv);
		break;
	case Nor:
		if(inv)
			goto and;
	or:
		search(n->left, lb, hb, ng, inv);
		search(n->right, ngames, hb, ngames, inv);
		break;
	case Nsub:
		search(n->left, lb, hb, ng, inv);
		search(n->right, ng, ngames, ng, !inv);
		break;
	case Nnot:
		search(n->left, lb, hb, ng, !inv);
		break;
	}
}

void
setmark(int ch)
{
	long i;
	Str *s;
	ulong b;

	b = genmark(ch);
	s = str;
	for(i=0; i<ngames; i++,s++)
		s->mark |= b;
	b = ~b;
	for(; i<tgames; i++,s++)
		s->mark &= b;
}

long
rellast(void)
{
	long i;
	Str *s;

	s = str;
	for(i=0; i<ngames; i++,s++)
		if(s->gp == lastgame)
			return i;
	return 0;
}

void
regerror(char *s)
{
	char buf[STRSIZE];

	sprint(buf, "reg: %s", s);
	yyerror(buf);
}

Node*
new(int op)
{
	Node *n;

	n = nodes+nodi;
	nodi++;
	if(nodi >= NODESIZE) {
		yyerror("too many nodes");
		n = nodes;
	}
	memset(n, 0, sizeof(Node));
	n->op = op;
	return n;
}

void
freenodes(void)
{
	int i;
	Node *n;

	n = &nodes[0];
	for(i=0; i<nodi; i++,n++) {
		if(n->op == Nrex)
			if(n->rex)
				free(n->rex);
		if(n->op == Nposit)
			if(n->xor)
				free(n->xor);
	}
}

void
xorinit(void)
{
	int i;

	for(i=0; i<nelem(xora); i++) {
		xora[i] = lrand() >> 8;
		xorb[i] = lrand() >> 8;
		xora[i] ^= lrand() << 8;
		xorb[i] ^= lrand() << 8;
	}
	for(i=0; i<64; i++) {
		xora[(010<<6)|i] = xora[(000<<6)|i];
		xorb[(010<<6)|i] = xorb[(000<<6)|i];
	}
	btmxora = lrand() >> 8;
	btmxorb = lrand() >> 8;
	btmxora ^= lrand() << 8;
	btmxorb ^= lrand() << 8;

	ginit(&initp);
	initpxora = posxorw();
	initpxorb = xposxor;

	for(i=0; i<sizeof(xpc); i++) {
		xpc[i] = 0;
		if(i & 7)
			xpc[i] = (i&017) ^ BLACK;
	}
	for(i=0; i<sizeof(xbd); i++)
		xbd[i] = i^070;
}

ulong
posxorw(void)
{
	int i, j;
	ulong xa, xb;

	xa = 0;
	xb = 0;
	for(i=0; i<64; i++) {
		j = board[i] & 017;
		j = (j<<6) | i;
		xa ^= xora[j];
		xb ^= xorb[j];
	}
	if(moveno & 1) {
		xa ^= btmxora;
		xb ^= btmxorb;
	}
	xposxor = xb;
	return xa;
}

ulong
posxorb(void)
{
	int i, j;
	ulong xa, xb;

	xa = 0;
	xb = 0;
	for(i=0; i<64; i++) {
		j = xpc[board[i]];
		j = (j<<6) | xbd[i];
		xa ^= xora[j];
		xb ^= xorb[j];
	}
	if(!(moveno & 1)) {
		xa ^= btmxora;
		xb ^= btmxorb;
	}
	xposxor = xb;
	return xa;
}

#define	FLAG	1

Xor*
xoradd(Xor *xp, int d)
{
	short mv[MAXMG];
	int i, m;
	long k;

	getmv(mv);
	for(i=0; m = mv[i]; i++) {
		move(m);
		if(FLAG || d == 0) {
			k = xp->cursize + 4;
			if(k > xp->maxsize) {
				xp->maxsize += 1000;
				xp = realloc(xp, sizeof(Xor) + xp->maxsize*sizeof(ulong));
			}
			xp->words[k-4] = posxorw();
			xp->words[k-3] = xposxor;
			xp->words[k-2] = posxorb();
			xp->words[k-1] = xposxor;
			xp->cursize = k;
		}
		if(d > 0)
			xp = xoradd(xp, d-1);
		rmove();
	}
	return xp;
}

int
xcmp(ulong *a, ulong *b)
{

	if(*a > *b)
		return 1;
	if(*a < *b)
		return -1;
	a++; b++;
	if(*a > *b)
		return 1;
	if(*a < *b)
		return -1;
	return 0;
}

int
ycmp(ulong *a, ulong *b)
{
	ushort* g1, *g2;

	if(*a > *b)
		return 1;
	if(*a < *b)
		return -1;
	a++; b++;
	if(*a > *b)
		return 1;
	if(*a < *b)
		return -1;
	a++; b++;
	g1 = (*(Str**)a)->gp;
	g2 = (*(Str**)b)->gp;
	if(g1 < g2)
		return 1;
	if(g1 > g2)
		return -1;
	return 0;
}

Xor*
xorset(int d)
{
	Xor *xp;
	ulong x, *mw;
	long i, j, sz;

	xp = malloc(sizeof(Xor));
	xp->maxsize = nelem(xp->words);
	xp->cursize = 0;
	if(FLAG || d == 0) {
		xp->words[0] = posxorb();
		xp->words[1] = xposxor;
		xp->words[2] = posxorw();
		xp->words[3] = xposxor;
		xp->cursize = 4;
	}
	if(d > 0)
		xp = xoradd(xp, d-1);

	qsort(xp->words, xp->cursize/2, 2*sizeof(ulong), xcmp);
	j = 0;
	for(i=2; i<xp->cursize; i+=2)
		if(xp->words[j+0] != xp->words[i+0] ||
		   xp->words[j+1] != xp->words[i+1]) {
			j += 2;
			xp->words[j+0] = xp->words[i+0];
			xp->words[j+1] = xp->words[i+1];
		}
	xp->cursize = j+2;

	for(sz=1; sz; sz<<=1)
		if(sz >= xp->cursize)
			break;
	if(xp->cursize+sz > xp->maxsize) {
		xp->maxsize = xp->cursize+sz;
		xp = realloc(xp, sizeof(Xor) + xp->maxsize*sizeof(ulong));
	}

	mw = &xp->words[xp->cursize];
	memset(mw, 0, sz);

	sz--;
	xp->mask = sz;
	for(i=xp->cursize-2; i>=0; i-=2) {
		x = xp->words[i];
		mw[((x>>5) & sz)] |= 1 << (x&31);
	}

	sprint(chars, "     %ld positions", xp->cursize/4);
	prline(8);
	return xp;
}

int
bputs(Biobuf *b, char *s)
{
	int t;

	t = strlen(s);
	Bwrite(b, s, t);
	return t;
}

void
dowrite(int ch, char *name)
{
	int f, j, k, m;
	Biobuf buf;
	long i, n, t, nw;
	Str *s;
	ushort *sp;
	Game g;

	while(*name == ' ')
		name++;

	switch(ch) {
	default:
		yyerror("unknown write type");
		return;
	case 'a':	/* ascii */
		if(*name == 0)
			name = "wa.out";
		break;
	case 'n':	/* nroff */
		if(*name == 0)
			name = "wn.out";
		break;
	case 'm':	/* binary */
		if(*name == 0)
			name = "wm.out";
		break;
	}
	f = create(name, OWRITE, 0666);
	if(f < 0) {
		yyerror("cannot create file");
		return;
	}

	chessinit('G', 0, 0);	/* always use ascii for writing */
	Binit(&buf, f, OWRITE);
	s = &str[1];
	i = 1;
	if(ngames == 1) {
		s--;
		i--;
	}
	t = 0;
	switch(ch) {
	case 'a':
		for(; i<ngames; i++,s++) {
			decode(&g, s);
			if(g.nmoves <= 0)
				continue;
			ginit(&initp);
			t += bputs(&buf, "file: ");
			t += bputs(&buf, g.file);
			t += bputs(&buf, "\nwhite: ");
			t += bputs(&buf, g.white);
			t += bputs(&buf, "\nblack: ");
			t += bputs(&buf, g.black);
			t += bputs(&buf, "\nevent: ");
			t += bputs(&buf, g.event);
			if(*g.date) {
				t += bputs(&buf, " ");
				t += bputs(&buf, g.date);
			}
			t += bputs(&buf, "\nopening: ");
			t += bputs(&buf, g.opening);
			t += bputs(&buf, "\nresult: ");
			t += bputs(&buf, g.result);
			t += Bprint(&buf, "\n%G", g.moves[0]);
			for(j=1; j<g.nmoves; j++) {
				move(g.moves[j-1]);
				m = g.moves[j];
				if(printcol >= 60)
					t += Bprint(&buf, "\n%G", m);
				else
					t += Bprint(&buf, " %G", m);
			}
			if(printcol > 0)
				t += Bprint(&buf, "\n");
		}
		break;

	case 'n':
		t += Bprint(&buf, ".na\n");
		for(; i<ngames; i++,s++) {
			decode(&g, s);
			if(g.nmoves <= 0)
				continue;
			ginit(&initp);
			t += Bprint(&buf, ".sp\n.ne 6\n.ce 100\n%s -- %s\n",
				g.white, g.black);
			t += Bprint(&buf, "%s %s\n",
				g.event, g.date);
			t += Bprint(&buf, "%s\n",
				g.opening);
			t += Bprint(&buf, ".ce 0\n.sp\n1 %G",
				g.moves[0]);
			for(j=1; j<g.nmoves; j++) {
				move(g.moves[j-1]);
				m = g.moves[j];
				if(printcol >= 60) {
					if(j & 1)
						n = Bprint(&buf, "\n%G", m);
					else
						n = Bprint(&buf, "\n%d %G", j/2+1, m);
				} else {
					if(j & 1)
						n = Bprint(&buf, " %G", m);
					else
						n = Bprint(&buf, " %d %G", j/2+1, m);
				}
				t += n;
			}
			if(printcol >= 60)
				t += Bprint(&buf, "\n%s\n", g.result);
			else
				t += Bprint(&buf, " %s\n", g.result);
		}
		break;

	case 'm':
		for(; i<ngames; i++,s++) {
			sp = s->gp;
			m = *sp++;
			Bputc(&buf, m>>8);
			Bputc(&buf, m);
			nw = Bwrite(&buf, sp, 2*m);
			if(nw != 2*m) {
				yyerror("write error");
				break;
			}
			t += (m + 1)*2;
			sp += m;
			m = *sp++;
			Bputc(&buf, m>>8);
			Bputc(&buf, m);
			for(j=0; j<m; j++) {
				k = *sp++;
				Bputc(&buf, k>>8);
				Bputc(&buf, k);
			}
			t += (m + 1)*2;
		}
		break;
	}

	Bflush(&buf);
	close(f);
	sprint(chars, "     %s: %ld chars", name, t);
	prline(8);
	chessinit('G', cinitf1, cinitf2);
	forcegame(gameno);
}

void
dodupl(void)
{
	ulong *posn, *p;
	long i, b;
	int j, n;
	Str *s;
	Game g;

	posn = malloc(3*sizeof(ulong)*tgames);
	if(posn == 0) {
		yyerror("no memory");
		return;
	}
	b = ~genmark('1');
	p = posn;
	s = &str[0];
	for(i=0; i<tgames; i++,s++) {
		s->mark &= b;
		decode(&g, s);
		if(g.nmoves < 10)
			s->mark |= ~b;
		ginit(&initp);
		n = g.nmoves/2;
		for(j=0; j<n; j++)
			move(g.moves[j]);
		*p++ = posxorw();
		n = g.nmoves;
		for(; j<n; j++)
			move(g.moves[j]);
		*p++ = posxorw();
		*p++ = (ulong)s;
	}
	qsort(posn, tgames, 3*sizeof(ulong), ycmp);

	b = genmark('1');
	p = posn;
	for(i=1; i<tgames; i++) {
		if(p[0] == p[0+3])
		if(p[1] == p[1+3]) {
			s = (Str*)p[2];
			decode(&g, s);
			s->position = g.nmoves;
			s->mark |= b;
		}
		p += 3;
	}
	free(posn);
}

ulong
genmark(int ch)
{

	if(ch >= '0' && ch <= '9')
		return 1 << (ch-'0');
	if(ch >= 'a' && ch <= 'z')
		return 1 << 10 + (ch-'a');
	return 0;
}
