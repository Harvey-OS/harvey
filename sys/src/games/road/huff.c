/*
 * huffnam encode input file
 */
#include	<u.h>
#include	<libc.h>
#include	<bio.h>

typedef	struct	Sym	Sym;
typedef	struct	Tree	Tree;
typedef	struct	Prof	Prof;

struct	Sym
{
	Sym*	link;
	union
	{
		char*	name;
		int	chr;
	};
	long	count;
	long	code;
	int	nbits;
};
struct	Prof
{
	long	count;
	int	flag;
	union
	{
		Sym*	sym;
		Tree*	tree;
	};
};
struct	Tree
{
	Prof	left;
	Prof	right;
};
enum
{
	TREE	= 1,
	LEAF,
	INF	= 1 << 29,
};
Sym*	hash[100003];
Prof*	prof;
Tree*	tree;
long	ascii[256];
Sym*	asciisym[256];
char	ldbuf[20];
Biobuf*	ibuf;
Biobuf*	obuf;
long	size;

void	dofile(void);
Sym*	lookup(char*);
int	freq1(Sym**, Sym**);
int	freq2(Prof*, Prof*);
void	huff(long);
void	prtree(Prof*, int, long);
void	putcode(long, int);
void	putlong(long);

void
main(int argc, char *argv[])
{
	char file[200], *p;
	int i;

	if(argc < 1)
		fprint(2, "usage: huff files\n");
	for(i=1; i<argc; i++) {
		strcpy(file, argv[i]);
		p = utfrrune(file, '.');
		if(p == 0)
			p = utfrrune(file, 0);
		sprint(p, ".h");
		ibuf = Bopen(argv[i], OREAD);
		if(ibuf == 0) {
			fprint(2, "cannot open %s\n", argv[i]);
			continue;
		}
		obuf = Bopen(file, OWRITE);
		if(obuf == 0) {
			Bterm(ibuf);
			fprint(2, "cannot create %s\n", file);
			continue;
		}
		dofile();
		Bterm(ibuf);
		Bterm(obuf);
	}
	exits(0);
}

void
dofile(void)
{
	Prof *pp;
	char *p;
	Sym *null, *t;
	Sym **list1, **lp;
	Sym *list2, *sp;
	long v, i, nlist1, nlist2;

	for(i=0;; i++) {
		p = Brdline(ibuf, '\n');
		if(p == 0)
			break;
		sp = lookup(p);
		sp->count++;
	}

/*
 * count list1
 * put unique lines in 'null'
 */
	memset(ascii, 0, sizeof(ascii));
	sprint(ldbuf, "\n");
	null = lookup(ldbuf);
	null->count = 2;

	nlist1 = 0;
	for(i=0; i<nelem(hash); i++) {
		for(sp=hash[i]; sp; sp=sp->link) {
			/*
			 * count ascii frequencies
			 */
			for(p=sp->name; *p; p++)
				ascii[*p & 0xff]++;
			ascii['\n']++;

			/*
			 * count non unique records
			 */
			if(sp->count == 1)
				null->count++;
			else
				nlist1++;
		}
	}
	null->count -= 2;
	if(null->count < 2)
		null->count = 2;

/*
 * make list1 of lines
 */
	list1 = malloc(nlist1*sizeof(*list1));
	lp = list1;
	for(i=0; i<nelem(hash); i++)
		for(sp=hash[i]; sp; sp=sp->link)
			if(sp->count > 1)
				*lp++ = sp;

	qsort(list1, nlist1, sizeof(*list1), freq1);

/*
 * lay out structures to call huff on lines
 */
	prof = malloc(nlist1*sizeof(*prof));
	tree = malloc(nlist1*sizeof(*tree));

	lp = list1;
	pp = prof;
	for(i=0; i<nlist1; i++) {
		sp = *lp++;
		pp->sym = sp;
		pp->count = sp->count;
		pp->flag = LEAF;
		pp++;
	}

	huff(nlist1);
	free(prof);
	free(tree);

/*
 * add line delta-frequencies in %ld format.
 */
	v = 0;
	lp = list1;
	for(i=0; i<nlist1; i++) {
		sp = *lp++;

		v = sp->count - v;
		if(v < 0)
			print("oops\n");
		sprint(ldbuf, "%ld", v);
		v = sp->count;

		for(p=ldbuf; *p; p++)
			ascii[*p & 0xff]++;
	}
	ascii['\n'] += nlist1;

/*
 * count chars
 */
	nlist2 = 0;
	for(i=0; i<nelem(ascii); i++)
		if(ascii[i])
			nlist2++;

/*
 * lay out structures to call huff
 */
	list2 = malloc(nlist2*sizeof(*list2));
	prof = malloc(nlist2*sizeof(*prof));
	tree = malloc(nlist2*sizeof(*tree));

	pp = prof;
	sp = list2;
	for(i=0; i<nelem(ascii); i++)
		if(ascii[i]) {
			sp->count = ascii[i];
			sp->chr = i;

			pp->sym = sp;
			pp->count = ascii[i];
			pp->flag = LEAF;

			asciisym[i] = sp;

			pp++;
			sp++;
		}

/*
 * do huffman of chars
 */
	qsort(prof, nlist2, sizeof(*prof), freq2);

	huff(nlist2);
	free(prof);
	free(tree);


/*
 * put out character dict
 */
	size = 0;
	putlong(nlist2);					/* char dict count */

	sp = list2;
	for(i=0; i<nlist2; i++) {
		if(sp->nbits > 27)
			print("bad char code\n");
		putcode(sp->chr, 8);				/* character */
		putlong((sp->nbits << 27) | sp->code);		/* encoding */
		sp++;
	}

/*
 * put out non-unique line dict
 */
	putlong(nlist1);					/* line dict count */

	v = 0;
	lp = list1;
	for(i=0; i<nlist1; i++) {
		sp = *lp++;

		v = sp->count-v;
		if(v < 0)
			print("oops\n");
		sprint(ldbuf, "%ld", v);
		v = sp->count;

		for(p=ldbuf; *p; p++) {			/* frequency */
			t = asciisym[*p & 0xff];
			if(t == 0) {
				print("oops\n");
				exits(0);
			}
			putcode(t->code, t->nbits);
		}
		t = asciisym['\n'];
		putcode(t->code, t->nbits);

		for(p=sp->name; *p; p++) {		/* line */
			t = asciisym[*p & 0xff];
			if(t == 0) {
				print("oops\n");
				exits(0);
			}
			putcode(t->code, t->nbits);
		}
		t = asciisym['\n'];
		putcode(t->code, t->nbits);
	}

/*
 * reread input and put out line codes
 */
	Bseek(ibuf, 0, 0);

	for(i=0;; i++) {
		p = Brdline(ibuf, '\n');
		if(p == 0)
			break;
		sp = lookup(p);
		if(sp->count == 0) {
			print("oops again\n");
			continue;
		}
		if(sp->count == 1) {			/* unique line code */
			putcode(null->code, null->nbits);
			for(p=sp->name; *p; p++) {
				t = asciisym[*p & 0xff];
				if(t == 0) {
					print("oops\n");
					continue;
				}
				putcode(t->code, t->nbits);
			}
			t = asciisym['\n'];
			putcode(t->code, t->nbits);
			sp->count = 0;
			continue;
		}
		putcode(sp->code, sp->nbits);		/* non-unique line code */
	}

	putcode(null->code, null->nbits);		/* eof */
	t = asciisym['\n'];
	putcode(t->code, t->nbits);

	putcode(0, 7);					/* flush */

}

Sym*
lookup(char *name)
{
	Sym *s;
	long h;
	char *p, c0;
	long l;

	h = 0;
	for(p=name; *p != '\n';) {
		h *= 3;
		h += *p++;
	}
	*p = 0;
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
	l = (p - name) + 1;
	s = malloc(sizeof(*s));
	s->name = malloc(l);
	strcpy(s->name, name);
	s->link = hash[h];
	hash[h] = s;

	s->count = 0;
	return s;
}

int
freq1(Sym **a, Sym **b)
{
	long na, nb;

	na = (*a)->count;
	nb = (*b)->count;
	if(na > nb)
		return 1;
	if(na < nb)
		return -1;
	return strcmp((*a)->name, (*b)->name);
}

int
freq2(Prof *a, Prof *b)
{
	long na, nb;

	na = a->count;
	nb = b->count;
	if(na > nb)
		return 1;
	if(na < nb)
		return -1;
	return 0;
}

void
huff(long nprof)
{
	long c1, c2, c3, c4, t1, t2, t3;
	Tree *tp;
	Prof *l1b, *l1e, *l2b, *l2e, *pf;


	pf = prof;
	tp = tree;	/* to make new trees */

	l1b = prof;	/* sorted list of trees */
	l1e = prof;

	l2b = pf;	/* sorted list of leafs */
	l2e = prof+nprof;

	for(;;) {

		/*
		 * c1,c2 are counts in smallest trees
		 */
		c1 = INF;
		c2 = INF;
		if(l1b < l1e) {
			c1 = l1b->count;
			if(l1b+1 < l1e)
				c2 = (l1b+1)->count;
		}

		/*
		 * c3,c4 are counts in smallest leafs
		 */
		c3 = INF;
		c4 = INF;
		if(l2b < l2e) {
			c3 = l2b->count;
			if(l2b+1 < l2e)
				c4 = (l2b+1)->count;
		}

		/*
		 * termination condition, no leafs, one tree
		 */
		if(c2 == INF && c3 == INF)
			break;

		/*
		 * find smallest of 3 ways to combine
		 */
		t1 = c1+c2;	/* tree+tree */
		t2 = c1+c3;	/* tree+leaf */
		t3 = c3+c4;	/* leaf+leaf */
		if(t1 <= t2 && t1 <= t3) {
			tp->left = l1b[0];
			tp->right = l1b[1];
			l1b += 2;
		} else
		if(t2 <= t1 && t2 <= t3) {
			tp->left = l1b[0];
			tp->right = l2b[0];
			l1b++;
			l2b++;
		} else
		if(t3 <= t1 && t3 <= t2) {
			tp->left = l2b[0];
			tp->right = l2b[1];
			l2b += 2;
		} else {
			print("no smallest\n");
			break;
		}

		l1e->count = tp->left.count + tp->right.count;
		l1e->flag = TREE;
		l1e->tree = tp;
		tp++;
		l1e++;
		if(l1e > l2b) {
			print("lists overrun each other\n");
			break;
		}
	}
	prtree(l1b, 0, 0);
}

void
prtree(Prof *p, int d, long v)
{

	if(p->flag == TREE) {
		v <<= 1;
		d++;
		prtree(&p->tree->left, d, v|0);
		prtree(&p->tree->right, d, v|1);
	}
	if(d > 32-5)
		print("too deep\n");
	p->sym->code = v;
	p->sym->nbits = d;
}

void
putlong(long v)
{
	putcode(v>>16, 16);
	putcode(v, 16);
}

int	ochar = 0;
int	onb = 0;
void
putcode(long v, int nb)
{
	int x, b;

	size += nb;
	while(nb+onb >= 8) {
		x = 8-onb;
		b = v >> (nb-x);
		b &= (1 << x) - 1;
		ochar |= b;

		Bputc(obuf, ochar);
		ochar = 0;
		onb = 0;
		nb -= x;
	}
	b = v & ((1 << nb) - 1);
	ochar |= b << (8-onb-nb);
	onb += nb;
}
