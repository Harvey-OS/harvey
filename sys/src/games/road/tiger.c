#include	<u.h>
#include	<libc.h>
#include	<bio.h>

enum
{
	TBOTH	= 1,
	TLEFT,
	TNONE,

	INF	= 1 << 29,
	MAXC1	= 12,
	MAXC2	= 12,
};

typedef	struct	Code1	Code1;
typedef	struct	Code2	Code2;

struct	Code1
{
	char*	val;
	Code1*	left;		/* used by huff */
	union
	{
		Code1*	link;
		Code1*	right;	/* used by huff */
	};
	union
	{
		long	count;	/* used by huff */
		long	code;
	};
	char	flag;		/* used by huff */
	char	nbits;
};
struct	Code2
{
	Code2*	link;
	long	code;
	short	val;
	char	nbits;
};

static	Code1*	tdecode1;
static	Code2*	tdecode2;
static	int	mdecode1;
static	int	mdecode2;
static	long	mask1;
static	long	mask2;
static	Code1**	decode1;
static	Code2**	decode2;
static	long	nlist1;
static	long	nlist2;
static	char	string[1000];
static	Biobuf	ibuf;
static	int	fid	= -1;

static	int	nb;
static	ulong	ul;
static	int	eofc;

static	void	huff(long);
static	void	prtree(Code1*, int, long);
static	long	getlong(void);
static	void	getstring(void);
static	void	error(char*);

int	Tinit(char*);
void*	Trdline(void);
int	Tclose(void);

int
Tinit(char *name)
{
	long v, i, b, n;
	Code1 *a1;
	Code2 *a2;

	nb = 0;
	ul = 0;
	eofc = 0;

	Tclose();
	fid = open(name, OREAD);
	if(fid < 0)
		return 1;
	Binit(&ibuf, fid, OREAD);

	nlist2 = getlong();
	if(nlist2 <= 0 || nlist2 >= (1<<8))
		goto bad;
	tdecode2 = malloc(nlist2*sizeof(*tdecode2));
	if(tdecode2 == 0)
		goto bad;
	mdecode2 = 0;
	a2 = tdecode2;
	for(i=0; i<nlist2; i++) {
		a2->val = Bgetc(&ibuf);
		v = getlong();
		a2->nbits = (v >> 27) & ((1<<5)-1);
		a2->code = v & ((1<<27)-1);
		if(a2->nbits > mdecode2)
			mdecode2 = a2->nbits;
		a2++;
	}

	if(mdecode2 > MAXC2)
		mdecode2 = MAXC2;
	v = 1<<mdecode2;
	mask2 = v-1;
	decode2 = malloc(v*sizeof(*decode2));
	if(decode2 == 0)
		goto bad;
	a2 = tdecode2+nlist2;
	for(i=0; i<nlist2; i++) {
		a2--;
		n = a2->nbits;
		if(n > mdecode2) {
			b = a2->code >> (n - mdecode2);
			n = mdecode2;
		} else
			b = a2->code << (mdecode2 - n);
		v = 1 << (mdecode2 - n);
		while(v > 0) {
			a2->link = decode2[b];
			decode2[b] = a2;
			b++;
			v--;
		}
	}

	nlist1 = getlong();
	if(nlist1 <= 0 || nlist1 >= (1<<27))
		goto bad;
	tdecode1 = malloc(nlist1*sizeof(*tdecode1));

	if(tdecode1 == 0)
		goto bad;

	b = 0;
	a1 = tdecode1;
	for(i=0; i<nlist1; i++) {
		getstring();
		v = atol(string) + b;
		getstring();
		a1->count = v;
		n = strlen(string)+1;
		a1->val = malloc(n);
		memmove(a1->val, string, n);
		b = v;
		a1++;
	}

	huff(nlist1);

	a1 = tdecode1;
	mdecode1 = 0;
	for(i=0; i<nlist1; i++) {
		if(a1->nbits > mdecode1)
			mdecode1 = a1->nbits;
		a1++;
	}

	if(mdecode1 > MAXC1)
		mdecode1 = MAXC1;
	v = 1<<mdecode1;
	mask1 = v-1;
	decode1 = malloc(v*sizeof(*decode1));
	if(decode1 == 0)
		goto bad;
	a1 = tdecode1+nlist1;
	for(i=0; i<nlist1; i++) {
		a1--;
		n = a1->nbits;
		if(n > mdecode1) {
			b = a1->code >> (n - mdecode1);
			n = mdecode1;
		} else
			b = a1->code << (mdecode1 - n);
		v = 1 << (mdecode1 - n);
		while(v > 0) {
			a1->link = decode1[b];
			decode1[b] = a1;
			b++;
			v--;
		}
	}
	if(eofc > 4)
		goto bad;

	return 0;

bad:
	Tclose();
	return 1;
}

void*
Trdline(void)
{
	char *l;
	long v;
	Code1 *a;
	int c, n;

	while(nb < 24) {
		c = Bgetc(&ibuf);
		if(c < 0) {
			c = 0;
			eofc++;
			if(eofc > 4)
				return 0;
		}
		ul = (ul<<8) | c;
		nb += 8;
	}

	v = (ul >> (nb - mdecode1)) & mask1;
	for(a=decode1[v]; a; a=a->link) {
		n = a->nbits;
		v = (ul >> (nb - n)) & ((1<<n) - 1);
		if(v == a->code)
			goto found;
	}
	error("bad1");
	return 0;

found:

	nb -= n;

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

	if(fid >= 0) {
		Bterm(&ibuf);
		close(fid);
		fid = -1;
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
	long v;
	int c, n;

	s = string;
	for(;;) {
		while(nb < 24) {
			c = Bgetc(&ibuf);
			if(c < 0) {
				c = 0;
				eofc++;
				if(eofc > 4)
					goto out;
			}
			ul = (ul<<8) | c;
			nb += 8;
		}

		v = (ul >> (nb - mdecode2)) & mask2;
		for(a=decode2[v]; a; a=a->link) {
			n = a->nbits;
			v = (ul >> (nb - n)) & ((1<<n) - 1);
			if(v == a->code)
				goto found;
		}
		error("bad2");
		return;

	found:
		nb -= n;
		c = a->val;
		if(c == '\n')
			break;
		*s++ = c;
	}

out:
	*s = 0;
}

static
void
huff(long nprof)
{
	long c1, c2, c3, c4, t1, t2, t3;
	Code1 *l1b, *l1e, *l2b, *l2e;

	if(nprof == 1) {
		tdecode1->code = 0;
		tdecode1->nbits = 0;
		return;
	}


	l1b = tdecode1;	/* sorted list of trees */
	l1e = tdecode1;

	l2b = tdecode1;	/* sorted list of leafs */
	l2e = tdecode1+nprof;

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
			l1e->left = &l1b[0];
			l1e->right = &l1b[1];
			l1e->flag = TBOTH;
			l1b += 2;
		} else
		if(t2 <= t1 && t2 <= t3) {
			l1e->left = &l1b[0];
			l1e->right = &l2b[0];
			l1e->flag = TLEFT;
			l1b++;
			l2b++;
		} else {
			l1e->left = &l2b[0];
			l1e->right = &l2b[1];
			l1e->flag = TNONE;
			l2b += 2;
		}

		l1e->count = l1e->left->count + l1e->right->count;
		l1e++;

		if(l1e > l2b)
			error("overrun");
	}
	prtree(l1b, 0, 0);
}

static
void
prtree(Code1 *p, int d, long v)
{

	v <<= 1;
	d++;
	if(d > 32-5)
		error("deep");

	switch(p->flag) {
	case TBOTH:
		prtree(p->left, d, v|0);
		prtree(p->right, d, v|1);
		return;
	case TLEFT:
		prtree(p->left, d, v|0);
		p->right->code = v|1;
		p->right->nbits = d;
		return;
	case TNONE:
		p->left->code = v|0;
		p->left->nbits = d;
		p->right->code = v|1;
		p->right->nbits = d;
		return;
	}
	error("tree");
}

static
long
getlong(void)
{
	long l;

	l = Bgetc(&ibuf)<<24;
	l |= Bgetc(&ibuf)<<16;
	l |= Bgetc(&ibuf)<<8;
	l |= Bgetc(&ibuf);
	return l;
}

static
void
error(char *s)
{

	fprint(2, "exp: %s\n", s);
	exits(s);
}
