#include <u.h>
#include <libc.h>

static char *nextmem = 0;
static long sizemem = 0;

#define	HUNK	16384
#define	STRSIZE	4096

static void
getmem(void)
{
	char *h;

	h = (char *) sbrk(HUNK);
	if(h == (char *)-1){
		fprint(2, "getmem didn't\n");
		abort();
	}
	if(nextmem == 0)
		nextmem = h;
	sizemem += HUNK;
}

char *
lmalloc(long n)
{
	char *addr;

	n = (n+3)&~3;
	while(sizemem < n)
		getmem();
	addr = nextmem;
	nextmem += n;
	sizemem -= n;
	return(addr);
}

char *
f_strdup(char *s)
{
	register char *ss, *sss;
	long n;

	if(sizemem < STRSIZE)
		getmem();
	n = (char *) memchr(s, 0, STRSIZE)-s+1;
	memcpy(nextmem, s, (int)n);
	s = nextmem;
	n = (n+3)&~3;
	nextmem += n;
	sizemem -= n;
	if(sizemem < 0){
		fprint(2, "strdup screwed up %ld\n", n);
		abort();
	}
	return(s);
}
