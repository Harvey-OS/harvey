#include	"all.h"

enum
{
	Npiece	= 6,
	Gone	= 64,
	Bad	= 0,
	Nopen	= 255,
};

typedef	struct	G	G;
struct	G
{
	uchar	npc;
	uchar	p[Npiece];
	uchar	l[Npiece];
};

typedef	struct	Egt	Egt;
struct	Egt
{
	char*	file;
	uchar	npc;
	uchar	pc[Npiece];
	void*	access;
};

typedef	struct	Vmm	Vmm;
struct	Vmm
{
	int	v;
	short	m1;
	short	m2;
	uchar	uniq1;
	uchar	uniq2;
};

void	convp2g(G*, ulong*, ulong*);
Egdb*	egdbinit(char*, int);
int	egdbread(Egdb*, ulong, ulong);

Egt	egt[] =
{
	"qr.qn",
		6, WKING, BKING, /**/ WQUEEN, WROOK, /**/ BQUEEN, BKNIGHT,
		nil,
	"rb.bn",
		6, WKING, BKING, /**/ WROOK, WBISHOP, /**/ BBISHOP, BKNIGHT,
		nil,
	"rb.nn",
		6, WKING, BKING, /**/ WROOK, WBISHOP, /**/ BKNIGHT, BKNIGHT,
		nil,
	"rn.nn",
		6, WKING, BKING, /**/ WROOK, WKNIGHT, /**/ BKNIGHT, BKNIGHT,
		nil,
	"rbb.q",
		6, WKING, BKING, /**/ WROOK, WBISHOP, WBISHOP, /**/ BQUEEN,
		nil,
	"qb.rr",
		6, WKING, BKING, /**/ WQUEEN, WBISHOP, /**/ BROOK, BROOK,
		nil,
	"qn.rr",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, /**/ BROOK, BROOK,
		nil,
	"rr.bb",
		6, WKING, BKING, /**/ WROOK, WROOK, /**/ BBISHOP, BBISHOP,
		nil,
	"rnn.q",
		6, WKING, BKING, /**/ WROOK, WKNIGHT, WKNIGHT, /**/ BQUEEN,
		nil,
	"qb.rb",
		6, WKING, BKING, /**/ WQUEEN, WBISHOP, /**/ BROOK, BBISHOP,
		nil,
	"qb.rn",
		6, WKING, BKING, /**/ WQUEEN, WBISHOP, /**/ BROOK, BKNIGHT,
		nil,
	"qn.rb",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, /**/ BROOK, BBISHOP,
		nil,
	"rn.bn",
		6, WKING, BKING, /**/ WROOK, WKNIGHT, /**/ BBISHOP, BKNIGHT,
		nil,
	"rn.bb",
		6, WKING, BKING, /**/ WROOK, WKNIGHT, /**/ BBISHOP, BBISHOP,
		nil,
	"qq.rb",
		6, WKING, BKING, /**/ WQUEEN, WQUEEN, /**/ BROOK, BBISHOP,
		nil,
	"qr.rb",
		6, WKING, BKING, /**/ WQUEEN, WROOK, /**/ BROOK, BBISHOP,
		nil,
	"rbn.q",
		6, WKING, BKING, /**/ WROOK, WBISHOP, WKNIGHT, /**/ BQUEEN,
		nil,
	"rr.rn",
		6, WKING, BKING, /**/ WROOK, WROOK, /**/ BROOK, BKNIGHT,
		nil,
	"rb.bb",
		6, WKING, BKING, /**/ WROOK, WBISHOP, /**/ BBISHOP, BBISHOP,
		nil,
	"rr.rb",
		6, WKING, BKING, /**/ WROOK, WROOK, /**/ BROOK, BBISHOP,
		nil,
	"qq.qr",
		6, WKING, BKING, /**/ WQUEEN, WQUEEN, /**/ BQUEEN, BROOK,
		nil,
	"qr.qr",
		6, WKING, BKING, /**/ WQUEEN, WROOK, /**/ BQUEEN, BROOK,
		nil,
	"rbb.r",
		6, WKING, BKING, /**/ WROOK, WBISHOP, WBISHOP, /**/ BROOK,
		nil,
	"rrn.q",
		6, WKING, BKING, /**/ WROOK, WROOK, WKNIGHT, /**/ BQUEEN,
		nil,
	"rrb.q",
		6, WKING, BKING, /**/ WROOK, WROOK, WBISHOP, /**/ BQUEEN,
		nil,
	"qr.qb",
		6, WKING, BKING, /**/ WQUEEN, WROOK, /**/ BQUEEN, BBISHOP,
		nil,
	"qr.rr",
		6, WKING, BKING, /**/ WQUEEN, WROOK, /**/ BROOK, BROOK,
		nil,
	"qn.rn",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, /**/ BROOK, BKNIGHT,
		nil,
	"rr.bn",
		6, WKING, BKING, /**/ WROOK, WROOK, /**/ BBISHOP, BKNIGHT,
		nil,
	"rr.nn",
		6, WKING, BKING, /**/ WROOK, WROOK, /**/ BKNIGHT, BKNIGHT,
		nil,
	"nnn.r",
		6, WKING, BKING, /**/ WKNIGHT, WKNIGHT, WKNIGHT, /**/ BROOK,
		nil,
	"bbn.r",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WKNIGHT, /**/ BROOK,
		nil,
	"bnn.r",
		6, WKING, BKING, /**/ WBISHOP, WKNIGHT, WKNIGHT, /**/ BROOK,
		nil,
	"bbb.r",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WBISHOP, /**/ BROOK,
		nil,
	"qb.qb",
		6, WKING, BKING, /**/ WQUEEN, WBISHOP, /**/ BQUEEN, BBISHOP,
		nil,
	"qb.qn",
		6, WKING, BKING, /**/ WQUEEN, WBISHOP, /**/ BQUEEN, BKNIGHT,
		nil,
	"qn.qb",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, /**/ BQUEEN, BBISHOP,
		nil,
	"qn.qn",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, /**/ BQUEEN, BKNIGHT,
		nil,
	"q.bbb",
		6, WKING, BKING, /**/ WQUEEN, /**/ BBISHOP, BBISHOP, BBISHOP,
		nil,
	"q.bnn",
		6, WKING, BKING, /**/ WQUEEN, /**/ BBISHOP, BKNIGHT, BKNIGHT,
		nil,
	"q.bbn",
		6, WKING, BKING, /**/ WQUEEN, /**/ BBISHOP, BBISHOP, BKNIGHT,
		nil,
	"q.nnn",
		6, WKING, BKING, /**/ WQUEEN, /**/ BKNIGHT, BKNIGHT, BKNIGHT,
		nil,
	"nnn.b",
		6, WKING, BKING, /**/ WKNIGHT, WKNIGHT, WKNIGHT, /**/ BBISHOP,
		nil,
	"nnn.n",
		6, WKING, BKING, /**/ WKNIGHT, WKNIGHT, WKNIGHT, /**/ BKNIGHT,
		nil,
	"qnn.q",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, WKNIGHT, /**/ BQUEEN,
		nil,
	"bbn.b",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WKNIGHT, /**/ BBISHOP,
		nil,
	"bbn.n",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WKNIGHT, /**/ BKNIGHT,
		nil,
	"bbb.b",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WBISHOP, /**/ BBISHOP,
		nil,
	"bbb.n",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WBISHOP, /**/ BKNIGHT,
		nil,
	"bnn.b",
		6, WKING, BKING, /**/ WBISHOP, WKNIGHT, WKNIGHT, /**/ BBISHOP,
		nil,
	"bnn.n",
		6, WKING, BKING, /**/ WBISHOP, WKNIGHT, WKNIGHT, /**/ BKNIGHT,
		nil,
	"bb.nn",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, /**/ BKNIGHT, BKNIGHT,
		nil,
	"qq.qq",
		6, WKING, BKING, /**/ WQUEEN, WQUEEN, /**/ BQUEEN, BQUEEN,
		nil,
	"rrr.q",
		6, WKING, BKING, /**/ WROOK, WROOK, WROOK, /**/ BQUEEN,
		nil,
	"rb.rb",
		6, WKING, BKING, /**/ WROOK, WBISHOP, /**/ BROOK, BBISHOP,
		nil,
	"rb.rn",
		6, WKING, BKING, /**/ WROOK, WBISHOP, /**/ BROOK, BKNIGHT,
		nil,
	"rn.rb",
		6, WKING, BKING, /**/ WROOK, WKNIGHT, /**/ BROOK, BBISHOP,
		nil,
	"rn.rn",
		6, WKING, BKING, /**/ WROOK, WKNIGHT, /**/ BROOK, BKNIGHT,
		nil,
	"qb.qr",
		6, WKING, BKING, /**/ WQUEEN, WBISHOP, /**/ BQUEEN, BROOK,
		nil,
	"qn.qr",
		6, WKING, BKING, /**/ WQUEEN, WKNIGHT, /**/ BQUEEN, BROOK,
		nil,
	"bbn.q",
		6, WKING, BKING, /**/ WBISHOP, WBISHOP, WKNIGHT, /**/ BQUEEN,
		nil,
	"rr.rr",
		6, WKING, BKING, /**/ WROOK, WROOK, /**/ BROOK, BROOK,
		nil,
	"nn.nn",
		6, WKING, BKING, /**/ WKNIGHT, WKNIGHT, /**/ BKNIGHT, BKNIGHT,
		nil,
	"bn.nn",
		6, WKING, BKING, /**/ WBISHOP, WKNIGHT, /**/ BKNIGHT, BKNIGHT,
		nil,
};

/* files are now under /usr/ken/eg, so this is unneeded */
void
bindum(void)
{
	int fd;

	fd = open("/srv/boot", ORDWR);
	if(fd < 0) {
		hprint(hout, "cant open #s");
		exits("open");
	}
	if(mount(fd, -1, "/n/other", 0, "other") < 0) {
		hprint(hout, "mount failed %r");
		exits("mount");
	}
}

int
inmate(void)
{
	short moves[MAXMG];

	if(battack(wkpos) && wattack(bkpos))
		return 0;
	getmv(moves);
	if(moves[0])
		return 0;
	return 1;
}

int
valof(int a)
{
	int va;

	switch(a) {
	case WKING:	va = 10; break;
	case BKING:	va =  9; break;
	case WQUEEN:	va =  8; break;
	case WROOK:	va =  7; break;
	case WBISHOP:	va =  6; break;
	case WKNIGHT:	va =  5; break;
	case BQUEEN:	va =  4; break;
	case BROOK:	va =  5; break;
	case BBISHOP:	va =  2; break;
	case BKNIGHT:	va =  1; break;
	default:
		print("valof: a = %d\n", a);
		va =  0;
		break;
	}
	return va;
}

int
egcmp(Egt *egp, G *g)
{
	int i, j, p;
	G a;

	a.npc = egp->npc;
	for(i=0; i<a.npc; i++) {
		a.p[i] = egp->pc[i];
		a.l[i] = Gone;
	}
	for(i=0; i<g->npc; i++) {
		p = g->p[i];
		for(j=0; j<a.npc; j++) {
			if(a.p[j] == p)
			if(a.l[j] == Gone) {
				a.l[j] = g->l[i];
				goto found;
			}
		}
		return 0;
	found:;
	}
	*g = a;
	return 1;
}

/*
 * weak side to move
 * look up current position
 */
int
egval1(void)
{
	int n, i, b, npc;
	G g;
	Egt *egp;
	ulong kgird, pgird;
	char file[100];

	/* for now, white is strong */
	if((mvno & 1) == 0)
		goto bad;

	npc = 0;
	for(i=0; i<64; i++) {
		b = board[i] & (BLACK|7);
		switch(b) {
		case 0:
		case BLACK:
			continue;
		case WPAWN:
		case BPAWN:
			goto bad;
		}
		if(npc >= Npiece)
			goto bad;
		if(mvno & 1) {
			g.p[npc] = b;
			g.l[npc] = i;
		} else {
			g.p[npc] = b^BLACK;
			g.l[npc] = i^070;
		}
		npc++;
	}
	g.npc = npc;

	for(egp=egt; egp<egt+nelem(egt); egp++)
		if(egp->access != nil && egcmp(egp, &g))
			goto found;
	for(egp=egt; egp<egt+nelem(egt); egp++)
		if(egcmp(egp, &g))
			goto found;

	return Bad;

found:
	if(egp->access == nil)
		egp->access = egdbinit(egp->file, mateflag);
	if(egp->access == nil) {
		hprint(hout, "cant open %s: %r\n", file);
		exits("open");
	}

	convp2g(&g, &kgird, &pgird);

	n = egdbread(egp->access, kgird, pgird);
	if(n < 0)
		goto bad;
	if(n == 0) {
		if(inmate())
			return 1;
		return 9999;
	}
	if(mateflag)
		npc = 1;
	return npc*1000 + n;

bad:
	return Bad;
}

/*
 * strong side to move
 * return min (to mate)
 */
int
egval2(void)
{
	short moves[MAXMG], m;
	int i, v, b;

	b = 10000;
	getmv(moves);
	for(i=0; m=moves[i]; i++) {
		move(m);
		v = egval1();
		if(v < b)
			b = v;
		rmove();
	}
	return b;
}

/*
 * convert ki,pi to hash
 */
void
convp2g(G *p, ulong *kg, ulong *pg)
{
	int i, l, inv, t, k;
	long h;

	k = p->l[0];
	t = eghtab[k];
	inv = (t & 060) << 4;
	i = (t & 017) << 6;

	l = inv | (t & 0300) | p->l[1];
	t = eggtab[l];
	i |= t & 077;
	*kg = egitab[i];

	l = p->l[2];
	if(l == Gone)
		l = k;
	l |= inv | (t & 0300);
	t = eggtab[l];
	h = t & 077;

	for(i=3; i<p->npc; i++) {
		l = p->l[i];
		if(l == Gone)
			l = k;
		l |= inv | (t & 0300);
		h <<= 6;
		t = eggtab[l];
		h |= t & 077;
	}
	*pg = h;
}

void
algloc(int l, char *s)
{
	s[0] = 'a' + l%8;
	s[1] = '8' - l/8;
}

void
posn(char *s)
{
	int pc, f, i;

	for(pc=WKING; pc >= WPAWN; pc--) {
		f = 1;
		for(i=0; i<64; i++) {
			if((board[i]&017) == pc) {
				if(f) {
					f = 0;
					*s++ = 'w';
					*s++ = "pnbrqk"[pc-WPAWN];
				}
				algloc(i, s);
				s += 2;
			}
		}
	}
	for(pc=BKING; pc >= BPAWN; pc--) {
		f = 1;
		for(i=0; i<64; i++) {
			if((board[i]&017) == pc) {
				if(f) {
					f = 0;
					*s++ = 'b';
					*s++ = "pnbrqk"[pc-BPAWN];
				}
				algloc(i, s);
				s += 2;
			}
		}
	}
	if(mateflag)
		*s++ = 'm';
	if(moveno & 1)
		*s++ = '.';
	*s = 0;
}

void
mvstr(int m, char *s)
{
	int fr, to;

	fr = (m >> 6) & 077;
	to = (m >> 0) & 077;
	if(fr == 0 && to == 0) {
		strcpy(s, "pass");
		return;
	}
	*s = "0PNBRQKG"[board[fr]&7];
	algloc(fr, s+1);

	if(board[to] & 7)
		s[3] = 'x';
	else
		s[3] = '-';
	algloc(to, s+4);
	s[6] = 0;
}

typedef	struct	Dep	Dep;
struct	Dep
{
	short	m;
	int	d;
};

int
dcmpl(void *va, void *vb)
{
	Dep *a, *b;
	int n;

	a = va;
	b = vb;

	n = a->d - b->d;
	if(n)
		return n;
	n = a->m - b->m;
	return n;
}

int
dcmph(void *va, void *vb)
{
	Dep *a, *b;
	int n;

	a = va;
	b = vb;

	n = b->d - a->d;
	if(n)
		return n;
	n = a->m - b->m;
	return n;
}

void
addptrs(void)
{
	short moves[MAXMG], m;
	char str1[100], str2[100];
	Dep lst[MAXMG];
	int rd, i, d, n, incheck, notindb;

	incheck = 0;
	notindb = 0;

/* bindum(); */
	rd = egval1();
	if(rd == Bad) {
		d = egval2();
		if(d == Bad)
			notindb = 1;
	}

	move(Nomove);
	if(check()) {
		incheck = 1;
		notindb = 0;
	}
	rmove();

//	hprint(hout, "<base href=http://plan9.bell-labs.com/magic/eg/>\n");
	hprint(hout, "<font size=+1>");
	if(moveno & 1)
		hprint(hout, "Black");
	else
		hprint(hout, "White");
	hprint(hout, " to move");
	if(mateflag)
		hprint(hout, " (to mate)");
	if(incheck)
		hprint(hout, " INCHECK");
	if(notindb)
		hprint(hout, " NOT IN DB");
	hprint(hout, "</font><br>\n");

	n = 0;
	if(!incheck) {
		getmv(moves);
		for(i=0; m=moves[i]; i++) {
			move(m);
			if(rd == Bad)
				d = egval1();
			else
				d = egval2();
			lst[i].m = m;
			lst[i].d = d;
			rmove();
		}
		n = i;
	}

	if(rd == Bad)
		qsort(lst, n, sizeof(lst[0]), dcmpl);
	else
		qsort(lst, n, sizeof(lst[0]), dcmph);

	move(Nomove);
	if(rd == Bad)
		d = egval1();
	else
		d = egval2();
	lst[n].m = Nomove;
	lst[n].d = d;
	rmove();
	n++;

	for(i=0; i<n; i++) {
		mvstr(lst[i].m, str1);
		move(lst[i].m);
		posn(str2);
		rmove();
		hprint(hout, "%4.4d <A HREF=/magic/eg/%s>%s</A><BR>\n",
			lst[i].d, str2, str1);
	}
}
