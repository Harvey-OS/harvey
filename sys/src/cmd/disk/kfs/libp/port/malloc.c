#include <u.h>
#include <libc.h>
#include <libp.h>

enum{
	MHUNK	= 16*1024,
	ALIGN	= 16,
};

typedef struct Pail	Pail;

struct Pail{
	ulong	size;
	Pail	*next;
};

static void	*carvealloc(ulong);
static Pail	*mpail(ulong);
static Lock	mlock;
static Lock	brklock;

static Pail	*pails;
static ulong	elem;
static char	*spare;
static ulong	mleft;

static void	*topalloc;
static ulong	base;
static ulong	alloced;
static int	calls;
static int	recalls, rehits, rehits1;
static int	frees;

void *malloc(long ln){
	Pail *p, *np;
	ulong n;

	calls++;
	n = (ulong)ln;
	alloced += n;
	n = (n + sizeof(ulong) + ALIGN - 1) & ~(ALIGN - 1);
	lock(&mlock);
	p = mpail(n);
	if(p == 0)
		return 0;
	np = p->next;
	if(np){
		p->next = np->next;
		unlock(&mlock);
		return (char *)np + sizeof(ulong);
	}
	unlock(&mlock);
	return carvealloc(n);
}

static void *carvealloc(ulong n){
	Pail *p;
	extern char end[];

	lock(&brklock);
	if(mleft < n){
		if(spare == 0){
			base = ((ulong)end + (ALIGN - 1)) & ~(ALIGN - 1);
			spare = (char *)base;
		}
		if(segbrk(spare, spare + mleft + n + MHUNK) < 0){
			unlock(&brklock);
			return 0;
		}
		mleft += n + MHUNK;
	}
	mleft -= n;
	p = (Pail *)spare;
	spare += n;
	p->size = n;
	topalloc = spare;
	unlock(&brklock);
	return (char *)p + sizeof(ulong);
}

static Pail *mpail(ulong n){
	Pail *np, *op;
	long l, r, m;
	ulong s, oelem;

top:
	l = 0;
	r = elem - 1;
	while(l <= r){
		m = (r + l) / 2;
		s = pails[m].size;
		if(s == n)
			return &pails[m];
		if(s < n)
			l = m + 1;
		else
			r = m - 1;
	}
	if((elem & 15) == 0){
		m = ((elem + 16) * sizeof(Pail) + sizeof(ulong) + ALIGN - 1)
			& ~(ALIGN - 1);
		oelem = elem;
		unlock(&mlock);
		np = carvealloc(m);
		lock(&mlock);
		if(elem != oelem)
			goto top;
		if(np == 0)
			return 0;
		op = pails;
	}else{
		np = pails;
		op = 0;
	}
	for(m = elem; m > 0 && pails[m - 1].size > n; m--)
		np[m] = pails[m - 1];
	np[m].next = 0;
	np[m].size = n;
	for(r = 0; r < m; r++)
		np[r] = pails[r];
	pails = np;
	elem++;
	if(op){
		unlock(&mlock);
		free(op);
		lock(&mlock);
	}
	goto top;
}

void free(void *v){
	Pail *p, *np;

	frees++;
	np = (Pail *)((ulong *)v - 1);
	lock(&mlock);
	p = mpail(np->size);
	np->next = p->next;
	p->next = np;
	unlock(&mlock);
}

void *realloc(void *v, long ln){
	Pail *p;
	void *x;
	ulong m, n;

	recalls++;
	n = (ulong)ln;
	if(!v)
		return malloc(n);
	p = (Pail*)((ulong *)v - 1);
	m = p->size - sizeof(ulong);
	if(n <= m){
		rehits++;
		return v;
	}
	if(spare == (char *)p + p->size){
		x = carvealloc(n - p->size);
		if(x == p + p->size + sizeof(ulong)){
			rehits1++;
			n = (n + sizeof(ulong) + ALIGN - 1) & ~(ALIGN - 1);
			p->size = n;
			return v;
		}
		free(x);
	}
	p = malloc(n);
	memmove(p, v, m);
	free(v);
	return p;
}

void mstats(void){
	Pail *p;
	int i, n;

	print("malloc: %d bytes allocated\n", alloced);
	print("areana: base = 0x%x top = 0x%x range = %d\n",
		base, topalloc, (char*)topalloc - (char*)base);
	print("%d calls to malloc %d calls to free, %d calls to realloc\n",
		calls, frees, recalls);
	print("%d reallocs in same block, %d blocks expanded\n",
		rehits, rehits1);
	print("%d different block sizes\n", elem);
	print("free list:\n");
	for(i = 0; i < elem; i++){
		n = 0;
		for(p = pails[i].next; p; p = p->next)
			n++;
		print("\t%4d byte blocks: %d\n", pails[i].size, n);
	}
}
