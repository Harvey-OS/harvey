/*
 * Program to cause a synthesized load for profiling.
 * Spawns n processes each of which would do r rounds of
 * io + compute.
 */

#include <u.h>
#include <libc.h>

enum
{
	Nprocs = 10,
	Nrounds = 4,
	Ncpu = 10000,
	Nio = 10000,
	Nstack = 10,
	Nsleep = 10,
	Bufsz = 1024,
};

#include "rnd.c"

int nprocs, nrounds, ncpu, nio, nstack, nsleep;


static void
docpu(void)
{
	int *dv;
	int i, j, x;

	dv = malloc(sizeof vals);
	for(i = 0; i < nelem(vals); i++)
		dv[i] = vals[i];
	for(i = 0; i < nelem(dv); i++)
		for(j = i; j < nelem(dv); j++)
			if(dv[i] > dv[j]){
				x = dv[i];
				dv[i] = dv[j];
				dv[j] = x;
			}
	free(dv);
}

static void
doio(char buf[], int sz)
{
	int i, p[2];
	long n;

	if(pipe(p) < 0)
		sysfatal("pipe");
	switch(fork()){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(p[1]);
		for(i = 0; i < nio; i++){
			n = readn(p[0], buf, sz);
			if(n != sz)
				sysfatal("read: %r");
		}
		close(p[0]);
		exits(nil);
	default:
		close(p[0]);
		for(i = 0; i < nio; i++){
			n = write(p[1], buf, sz);
			if(n != sz)
				sysfatal("write: %r");
		}
		close(p[1]);
		waitpid();
	}
}

static void
stk(int lvl)
{
	int i, c;
	char buf[Bufsz];

	if(lvl < Nstack){
		stk(lvl+1);
		return;
	}
	for(i = 0; i <nrounds; i++){
		for(c = 0; c < ncpu; c++)
			docpu();
		doio(buf, sizeof buf);
		sleep(nsleep);
	}
}

static void
usage(void)
{
	fprint(2, "usage: %s [-p procs] [-r nrounds] [-c ncpu]"
		" [-i nio] [-s nstack]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int p;

	nprocs = Nprocs;
	nrounds = Nrounds;
	ncpu = Ncpu;
	nio = Nio;
	nstack = Nstack;
	nsleep = Nsleep;
	ARGBEGIN{
	case 'p':
		nprocs = atoi(EARGF(usage()));
		break;
	case 'r':
		nrounds = atoi(EARGF(usage()));
		break;
	case 'c':
		ncpu = atoi(EARGF(usage()));
		break;
	case 'i':
		nio = atoi(EARGF(usage()));
		break;
	case 's':
		nstack = atoi(EARGF(usage()));
		break;
	case 'w':
		nsleep = atoi(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND
	
	if(nprocs < 2)
		nprocs = 2;
	for(p = 0; p < nprocs/2; p++){
		switch(fork()){
		case -1:
			sysfatal("fork: %r");
		case 0:
			stk(0);
			exits(nil);
		}
		docpu();
	}
	for(p = 0; p < nprocs; p++)
		waitpid();
	exits(nil);
}
