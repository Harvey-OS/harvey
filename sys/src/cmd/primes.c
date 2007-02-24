#include <u.h>
#include <libc.h>
#include <bio.h>

double big = 9.007199254740992e15;

int pt[] =
{
	  2,  3,  5,  7, 11, 13, 17, 19, 23, 29,
	 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
	 73, 79, 83, 89, 97,101,103,107,109,113,
	127,131,137,139,149,151,157,163,167,173,
	179,181,191,193,197,199,211,223,227,229,
};
double wheel[] =
{
	10, 2, 4, 2, 4, 6, 2, 6, 4, 2,
	 4, 6, 6, 2, 6, 4, 2, 6, 4, 6,
	 8, 4, 2, 4, 2, 4, 8, 6, 4, 6,
	 2, 4, 6, 2, 6, 6, 4, 2, 4, 6,
	 2, 6, 4, 2, 4, 2,10, 2,
};
uchar table[1000];
uchar bittab[] =
{
	1, 2, 4, 8, 16, 32, 64, 128,
};

enum {
	ptsiz	= nelem(pt),
	whsiz	= nelem(wheel),
	tabsiz	= nelem(table),
	tsiz8	= tabsiz*8,
};

void	mark(double nn, long k);

void
usage(void)
{
	fprint(2, "usage: %s [start [finish]]\n", argv0);
	exits("limits");
}

void
ouch(void)
{
	fprint(2, "limits exceeded\n");
	exits("limits");
}

void
main(int argc, char *argv[])
{
	int i;
	double k, temp, v, limit, nn;
	char *l;
	Biobuf bin;

	ARGBEGIN{
	default:
		usage();
		break;
	}ARGEND;

	nn = 0;
	limit = big;
	switch (argc) {
	case 0:
		Binit(&bin, 0, OREAD);
		while ((l = Brdline(&bin, '\n')) != nil) {
			if (*l == '\n')
				continue;
			nn = atof(l);
			if(nn < 0)
				sysfatal("negative start");
			break;
		}
		Bterm(&bin);
		break;
	case 2:
		limit = atof(argv[1]);
		if(limit < nn)
			exits(0);
		if(limit > big)
			ouch();
		/* fallthrough */
	case 1:
		nn = atof(argv[0]);
		break;
	default:
		usage();
		break;
	}

	if(nn < 0 || nn > big)
		ouch();
	if(nn == 0)
		nn = 1;

	if(nn < 230) {
		for(i=0; i<ptsiz; i++) {
			if(pt[i] < nn)
				continue;
			if(pt[i] > limit)
				exits(0);
			print("%d\n", pt[i]);
//			if(limit >= big)
//				exits(0);
		}
		nn = 230;
	}

	modf(nn/2, &temp);
	nn = 2*temp + 1;
/*
 *	clear the sieve table.
 */
	for(;;) {
		for(i = 0; i < tabsiz; i++)
			table[i] = 0;
/*
 *	run the sieve.
 */
		v = sqrt(nn+tsiz8);
		mark(nn, 3);
		mark(nn, 5);
		mark(nn, 7);
		for(i = 0, k = 11; k <= v; k += wheel[i]) {
			mark(nn, k);
			i++;
			if(i >= whsiz)
				i = 0;
		}
/*
 *	now get the primes from the table
 *	and print them.
 */
		for(i = 0; i < tsiz8; i += 2) {
			if(table[i>>3] & bittab[i&07])
				continue;
			temp = nn + i;
			if(temp > limit)
				exits(0);
			print("%.0f\n", temp);
//			if(limit >= big)
//				exits(0);
		}
		nn += tsiz8;
	}
}

void
mark(double nn, long k)
{
	double t1;
	long j;

	modf(nn/k, &t1);
	j = k*t1 - nn;
	if(j < 0)
		j += k;
	for(; j < tsiz8; j += k)
		table[j>>3] |= bittab[j&07];
}
