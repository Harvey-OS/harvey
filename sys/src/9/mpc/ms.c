#include <u.h>
#include <libc.h>

char	d0[1000008], d1[1000008];

void	test(int size, int align);

memtest(char *pp, int n)
{
	static int seed = 0;
	ulong *p, *ep;
	ulong x;

if(seed % 100 == 0)
print("memtest seed = %d\n", seed);

	p = (ulong*)pp;
	ep = (ulong*)(pp+n);

	x = seed;
	while(p < ep) {
		*p++ = x;
		x *= 1103515245;
	}

	x = seed;
	p = (ulong*)pp;
	ep = (ulong*)(pp+n);
	while(p < ep) {
		if(*p++ != x) {
			print("error %ux!!\n", ((char*)p) - pp);
			return;
		}
		x *= 1103515245;
	}
	seed++;
}

void
main()
{
	int j;

	memmove(d0, d1, sizeof(d0));
	
	for(j=0; j<10; j++) {
		memtest(d0, sizeof(d0));
	}

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

	exits(0);
}

void
test(int size, int align)
{
	int n;
	int t;

	t = -times(0);
	n = 10000000/size;
	while(n) {
		memmove(d0+align, d1, size);
		n--;
	}
	t += times(0);
	print("size = %d align = %d KB/s %d %d\n", size, align, 10000000/t, t);
}
