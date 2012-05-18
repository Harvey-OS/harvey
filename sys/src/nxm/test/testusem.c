#include <u.h>
#include <libc.h>

/*
 * NIX optimistic semaphores testing.
 !	6c -FTVw usem.c && 6l -o 6.usem usem.6
 */

int Nprocs = 5;
int verb;

#define vprint	if(verb)print

typedef void (*Testf)(int, int*);

long ts;	/* Plan 9 semaphore for testing */

void
downer(int id, int *s)
{
	vprint("%d: down...\n", id);

	/*
	 * non blocking
	while(downsem(s, 1) < 0)
		sleep(1);
	 */

	/*
	 * blocking
	 */
	downsem(s, 0);

	vprint("%d: downed\n", id);
	semrelease(&ts, 1);
	exits(nil);
}

void
uper(int id, int *s)
{
	vprint("%d: up...\n", id);
	upsem(s);
	vprint("%d: uped\n", id);
	semrelease(&ts, 1);
	exits(nil);
}

void
alter(int id, int *s)
{

	vprint("%d: alt...\n", id);
	altsems(&s, 1);
	vprint("%d: alted\n", id);
	semrelease(&ts, 1);
	exits(nil);
}

int nprocs;

void
semtest(void)
{
	int i, t;
	int *s;
	struct{
		Testf f;
		int fact;
	} tests[] = {
		{uper, 2},
		{downer, 1},
		{alter, 1},
	};

	s = mallocz(sizeof *s, 1);
	nprocs = 0;
	for(t = 0; t < nelem(tests); t++)
		switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("fork");
		case 0:
			for(i = 0; i < tests[t].fact*Nprocs; i++)
				switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
				case -1:
					sysfatal("fork");
				case 0:
					tests[t].f(ainc(&nprocs), s);
					break;
				}
			exits(nil);
			break;
		}

	for(t = 0; t < nelem(tests); t++)
		for(i = 0; i < tests[t].fact*Nprocs; i++)
			semacquire(&ts, 1);
	semstats();
	print("val = %d\n", *s);
	free(s);
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

	semtest();
	exits(nil);
}
