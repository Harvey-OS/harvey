#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#define	nelem(x)	(sizeof(x)/sizeof(x)[0])
typedef	struct	Sym	Sym;
typedef	struct	Code1	Code1;
typedef	struct	Code2	Code2;
typedef	struct	Tree	Tree;
typedef	struct	Prof	Prof;

struct	Code1
{
	char*	val;
	long	code;
	int	nbits;
};
struct	Code2
{
	long	val;
	long	code;
	int	nbits;
};
struct	Prof
{
	long	count;
	int	flag;
	union
	{
		Tree*	tree;
		Code1*	sym;
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

static	Prof*	prof;
static	Tree*	tree;
static	Code1*	tdecode1;
static	Code2*	tdecode2;
static	int	mdecode1;
static	int	mdecode2;
static	long	mask1;
static	long	mask2;
static	long*	decode1;
static	uchar*	decode2;
static	long	nlist1;
static	long	nlist2;
static	char	string[1000];
static	Biobuf*	ibuf;

static	int	nb;
static	ulong	ul;
static	int	eofc;

static	void	huff(long);
static	void	prtree(Prof*, int, long);
static	long	getlong(void);
static	void	getstring(void);
static	void	error(char*);

char*	tiger(char*);
int	Tinit(char*);
void*	Trdline(void);
int	Tclose(void);

int
Tinit(char *name)
{
	long v, i, b;
	Code1 *a1;
	Code2 *a2;
	Prof *p;

	nb = 0;
	ul = 0;
	eofc = 0;

	Tclose();
	ibuf = Bopen(tiger(name), OREAD);
	if(ibuf == 0)
		return 1;

	nlist2 = getlong();
	tdecode2 = malloc(nlist2*sizeof(*tdecode2));
	mdecode2 = 0;
	a2 = tdecode2;
	for(i=0; i<nlist2; i++) {
		a2->val = Bgetc(ibuf);
		v = getlong();
		a2->nbits = (v >> 27) & ((1<<5)-1);
		a2->code = v & ((1<<27)-1);
		if(a2->nbits > mdecode2)
			mdecode2 = a2->nbits;
		a2++;
	}

	v = 1<<mdecode2;
	mask2 = v-1;
	decode2 = malloc(v*sizeof(*decode2));
	a2 = tdecode2;
	for(i=0; i<nlist2; i++) {
		v = 1 << (mdecode2 - a2->nbits);
		b = a2->code << (mdecode2 - a2->nbits);
		while(v > 0) {
			decode2[b] = i;
			b++;
			v--;
		}
		a2++;
	}

	nlist1 = getlong();
	tdecode1 = malloc(nlist1*sizeof(*tdecode1));
	prof = malloc(nlist1*sizeof(*prof));
	tree = malloc(nlist1*sizeof(*tree));

	p = prof;
	b = 0;
	a1 = tdecode1;
	for(i=0; i<nlist1; i++) {
		getstring();
		v = atol(string) + b;
		getstring();
		p->count = v;
		p->sym = a1;
		p->flag = LEAF;
		a1->val = malloc(strlen(string)+1);
		strcpy(a1->val, string);
		b = v;
		a1++;
		p++;
	}

	huff(nlist1);

	a1 = tdecode1;
	mdecode1 = 0;
	for(i=0; i<nlist1; i++) {
		if(a1->nbits > mdecode1)
			mdecode1 = a1->nbits;
		a1++;
	}

	v = 1<<mdecode1;
	mask1 = v-1;
	decode1 = malloc(v*sizeof(*decode1));
	a1 = tdecode1;
	for(i=0; i<nlist1; i++) {
		v = 1 << (mdecode1 - a1->nbits);
		b = a1->code << (mdecode1 - a1->nbits);
		while(v > 0) {
			decode1[b] = i;
			b++;
			v--;
		}
		a1++;
	}

	return 0;
}

void*
Trdline(void)
{
	char *l;
	long v;
	ulong lul;
	Code1 *a;
	int c, lnb;

	lul = ul;
	lnb = nb;

	while(lnb < 24) {
		c = Bgetc(ibuf);
		if(c < 0) {
			c = 0;
			eofc++;
			if(eofc > 4) {
				error("eof");
				return 0;
			}
		}
		lul = (lul<<8) | c;
		lnb += 8;
	}
	v = (lul >> (lnb - mdecode1)) & mask1;
	a = &tdecode1[decode1[v]];

	nb = lnb - a->nbits;
	ul = lul;

	l = a->val;
	if(*l == 0) {
		getstring();
		l = string;
		if(*l == 0)
			return 0;
	}
	return l;
}

int
Tclose(void)
{
	long i;
	Code1 *a1;

	if(ibuf) {
		Bclose(ibuf);
		ibuf = 0;
	}
	if(tdecode2) {
		free(tdecode2);
		tdecode2 = 0;
	}
	if(decode2) {
		free(decode2);
		decode2 = 0;
	}
	if(tdecode1) {
		a1 = tdecode1;
		for(i=0; i<nlist1; i++) {
			free(a1->val);
			a1++;
		}
		free(tdecode1);
		tdecode1 = 0;
	}
	if(prof) {
		free(prof);
		prof = 0;
	}
	if(tree) {
		free(tree);
		tree = 0;
	}
	if(decode1) {
		free(decode1);
		decode1 = 0;
	}
	return 0;
}

static
void
getstring(void)
{
	char *s;
	Code2 *a;
	ulong lul;
	long v;
	int c, lnb;

	lul = ul;
	lnb = nb;

	s = string;
	for(;;) {
		while(lnb < 24) {
			c = Bgetc(ibuf);
			if(c < 0) {
				c = 0;
				eofc++;
				if(eofc > 4) {
					error("eof");
					goto out;
				}
			}
			lul = (lul<<8) | c;
			lnb += 8;
		}

		v = (lul >> (lnb - mdecode2)) & mask2;
		a = &tdecode2[decode2[v]];
		lnb -= a->nbits;
		c = a->val;
		if(c == '\n')
			break;
		*s++ = c;
	}

out:
	*s = 0;

	nb = lnb;
	ul = lul;
}

static
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
		} else
			error("smallest");

		l1e->count = tp->left.count + tp->right.count;
		l1e->flag = TREE;
		l1e->tree = tp;
		tp++;
		l1e++;
		if(l1e > l2b)
			error("overrun");
	}
	prtree(l1b, 0, 0);
}

static
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
		error("deep");
	p->sym->code = v;
	p->sym->nbits = d;
}

static
long
getlong(void)
{
	long l;

	l = Bgetc(ibuf)<<24;
	l |= Bgetc(ibuf)<<16;
	l |= Bgetc(ibuf)<<8;
	l |= Bgetc(ibuf);
	return l;
}

static
void
error(char *s)
{

	fprint(2, "exp: %s\n", s);
	exits(s);
}
