#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

#define STRSIZE	1024
#define SYMHASH	127

static Sym *	avail;
static Sym *	stab[SYMHASH];

static ulong	hashfun(char*);
static void	moresym(void);

int	strunique;

Sym *
symfind(char *a)
{
	Sym *x;

	for(x = stab[hashfun(a) % SYMHASH]; x; x = x->next)
		if(strcmp(x->name, a) == 0)
			return x;
	return 0;
}

Sym *
symstore(char *a)
{
	Sym **bin, *x;

	bin = &stab[hashfun(a) % SYMHASH];
	for(x = *bin; x; x = x->next)
		if(strcmp(x->name, a) == 0)
			return x;
	if(avail == 0)
		moresym();
	x = avail;
	avail = x->next;
	memset(x, 0, sizeof(Sym));
	x->next = *bin;
	*bin = x;
	x->name = strunique ? a : savestr(a);
	return x;
}

static void
moresym(void)
{
	Sym *x;
	int n = SYMHASH;

	if ((x = malloc(n*sizeof(Sym))) == 0)
		error("cannot malloc sym space");
	x->next = 0;
	while (--n > 0)
		++x, x->next = x-1;
	avail = x;
}

static void
symprint1(Sym *x, void *arg)
{
	Biobuf *bp;

	bp = *(Biobuf **)arg;
	Bprint(bp, "%s %d 0x%x\n", x->name, x->type, x->ptr);
}

void
symprint(Biobuf *bp)
{
	symtraverse(symprint1, bp);
}

void
symtraverse(void (*fp)(Sym*, void*), ...)
{
	Sym **bin, *x;

	for(bin = stab; bin < stab+SYMHASH; bin++)
		for(x = *bin; x; x = x->next)
			(*fp)(x, &fp+1);
}

static ulong
hashfun(char *s)
{
	ulong a = 0, b;

	while(*s){
		a = (a << 4) + *s++;
		if(b = a&0xf0000000)
			a ^= b >> 24, a ^= b;
	}
	return a;
}

char *
savestr(char *str)	/* Place string into permanent storage. */
{
	static char *curstp; static int nchleft;
	int len;

	if ((len = strlen(str)+1) > nchleft) {
		nchleft = STRSIZE;
		while (nchleft < len)
			nchleft *= 2;
		if ((curstp = malloc(nchleft)) == 0)
			error("malloc failed in savestr");
	}
	str = strcpy(curstp, str);
	curstp += len; nchleft -= len;
	return str;
}

#define	SIZE	4096
#define	DOTDOT	(&fmt+1)

void
error(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	n = sprint(buf, "%s %d: ", infile, lineno);
	out = doprint(buf+n, buf+SIZE-n-1, fmt, DOTDOT);
	*out++ = '\n';
	write(2, buf, out-buf);
	exits("error");
}

void
warn(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	n = sprint(buf, "%s %d: ", infile, lineno);
	out = doprint(buf+n, buf+SIZE-n-1, fmt, DOTDOT);
	*out++ = '\n';
	write(2, buf, out-buf);
}
