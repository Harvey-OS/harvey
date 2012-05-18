#include <u.h>
#include <libc.h>
#include <tube.h>

/*
 * NIX tubes testing.
 !	6c -FTVw utube.c && 6l -o 6.utube utube.6
 */

int Nprocs = 5;


typedef void (*Testf)(int, Tube*);

long ts;	/* Plan 9 semaphore for testing */
int verb;


#define vprint	if(verb)print

void
trecver(int id, Tube *t)
{
	ulong n;

	vprint("%d: recv...\n", id);
	trecv(t, &n);
	vprint("%d: recved: %uld\n", id, n);
	semrelease(&ts, 1);
	exits(nil);
}

void
tsender(int id, Tube *t)
{
	ulong n;

	n = id;
	vprint("%d: send...\n", id);
	tsend(t, &n);
	vprint("%d: sent\n", id);
	semrelease(&ts, 1);
	exits(nil);
}

void
talter(int id, Tube *t)
{
	ulong n;
	Talt alts[] = {
		{t, &n, TRCV},
	};

	n = id;
	vprint("%d: alt...\n", id);
	if(talt(alts, nelem(alts)) != 0)
		abort();
	vprint("%d: alted: %uld\n", id, n);
	semrelease(&ts, 1);
	exits(nil);
}

void
talterduo(int id, Tube *t)
{
	ulong n;
	Tube *t2;
	ulong n2;
	Talt alts[] = {
		{t, &n, TRCV},
		{nil, &n2, TRCV},
	};

	t2 = newtube(sizeof(ulong), 1);
	alts[1].t = t2;

	switch(rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("fork");
	case 0:
		sleep(1);
		tsender(-id, t2);
		exits(nil);
	}
	n = id;
	vprint("%d: alt...\n", id);
	switch(talt(alts, nelem(alts))){
	case 0:
		break;
	case 1:
		vprint("%d: alting...\n", id);
		trecv(t, &n);
		break;
	default:
		sysfatal("alt");
	}
	vprint("%d: alted: %uld\n", id, n);
	semrelease(&ts, 1);
	exits(nil);
}

long nprocs;

void
tubetest(void)
{
	int i, j, t;
	long n;
	Tube *tb;
	struct{
		Testf f;
		int fact;
	} tests[] = {
		{tsender, 1},
		{trecver, 1},
		{tsender, 1},
		{talter, 1},

//		{tsender, 1},
//		{trecver, 1},
//		{tsender, 1},
//		{talterduo, 1},
	};

	tb = newtube(sizeof(ulong), Nprocs/2);
	for(t = 0; t < nelem(tests); t++)
		switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("fork");
		case 0:
			n = 0;
			for(j = 0; j < t; j++)
				n += tests[j].fact*Nprocs;
			for(i = 0; i < tests[t].fact*Nprocs; i++)
				switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
				case -1:
					sysfatal("fork");
				case 0:
					tests[t].f(n+i, tb);
					break;
				}
			exits(nil);
			break;
		}

	for(t = 0; t < nelem(tests); t++)
		for(i = 0; i < tests[t].fact*Nprocs; i++)
			semacquire(&ts, 1);
	semstats();
	print("hole %d msg %d\n", tb->nhole, tb->nmsg);
}

void
usage(void)
{
	fprint(2, "usage: %s [-v] [-n n]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	ARGBEGIN{
	case 'v':
		verb++;
		break;
	case 'n':
		Nprocs = atoi(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND;
	if(argc != 0)
		usage();

	tubetest();
	exits(nil);
}
