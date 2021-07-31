#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

static	char	bbarena[10000];
static	char	bbused[10];

void*
bbmalloc(int n)
{
	char *a;
	int s, i;

	s = splhi();
	a = memchr(bbused, 0, sizeof(bbused));
	if(a) {
		i = a - bbused;
		a = bbarena + i*n;
		if(a+n <= bbarena+sizeof(bbarena)) {
			bbused[i] = 1;
			splx(s);
			return a;
		}
	}
	splx(s);
	print("too many bbmallocs\n");
	return bbarena;
}

void
bbfree(void *va, int n)
{
	int s, i;

	s = splhi();
	i = ((char*)va - bbarena) /n;
	if(i >= 0 && i < sizeof(bbused) && bbused[i]){
		bbused[i] = 0;
		splx(s);
		return;
	}
	splx(s);
	print("sanity bbfree\n");
}

void*
bbdflush(void *p, int n)
{
	return p;
}

