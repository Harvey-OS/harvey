#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

char	d0[1000008], d1[1000008];

static void	test(int size, int align);


void
memspeed()
{
	int j;

	spllo();

	memmove(d0, d1, sizeof(d0));
	
	for(j = 0; j<8; j++)
		test(12, j);
	for(j = 0; j<8; j++)
		test(100, j);
	for(j = 0; j<8; j++)
		test(400, j);
	for(j = 0; j<8; j++)
		test(1000, j);
	for(j = 0; j<8; j++)
		test(10000, j);
	for(j = 0; j<8; j++)
		test(100000, j);
	for(j = 0; j<8; j++)
		test(1000000, j);
}

static void
test(int size, int align)
{
	int n;
	int t;

	t = -TK2MS(m->ticks);
	n = 10000000/size;
	while(n) {
		memmove(d0+align, d1, size);
		n--;
	}
	t += TK2MS(m->ticks);
	print("size = %d align = %d KB/s %d %d\n", size, align, 10000000/t, t);
}

