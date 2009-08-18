#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

#define	PCRES	8

struct COUNTER
{
	char 	*name;		/* function name */
	long	time;		/* ticks spent there */
};

void
error(int perr, char *s)
{
	fprint(2, "tprof: %s", s);
	if(perr){
		fprint(2, ": ");
		perror(0);
	}else
		fprint(2, "\n");
	exits(s);
}

int
compar(void *va, void *vb)
{
	struct COUNTER *a, *b;

	a = va;
	b = vb;
	if(a->time < b->time)
		return -1;
	if(a->time == b->time)
		return 0;
	return 1;
}
void
main(int argc, char *argv[])
{
	int fd;
	long i, j, k, n;
	Dir *d;
	char *name;
	ulong *data;
	ulong tbase, sum;
	long delta;
	Symbol s;
	Biobuf outbuf;
	Fhdr f;
	struct COUNTER *cp;
	char filebuf[128], *file;

	if(argc != 2 && argc != 3)
		error(0, "usage: tprof pid [binary]");
	/*
	 * Read symbol table
	 */
	if(argc == 2){
		file = filebuf;
		snprint(filebuf, sizeof filebuf, "/proc/%s/text", argv[1]);
	}else
		file = argv[2];

	fd = open(file, OREAD);
	if(fd < 0)
		error(1, file);

	if (!crackhdr(fd, &f))
		error(1, "read text header");
	if (f.type == FNONE)
		error(0, "text file not an a.out");
	machbytype(f.type);
	if (syminit(fd, &f) < 0)
		error(1, "syminit");
	close(fd);
	/*
	 * Read timing data
	 */
	file = smprint("/proc/%s/profile", argv[1]);
	fd = open(file, OREAD);
	if(fd < 0)
		error(1, file);
	free(file);
	d = dirfstat(fd);
	if(d == nil)
		error(1, "stat");
	n = d->length/sizeof(data[0]);
	if(n < 2)
		error(0, "data file too short");
	data = malloc(d->length);
	if(data == 0)
		error(1, "malloc");
	if(read(fd, data, d->length) < 0)
		error(1, "text read");
	close(fd);

	for(i=0; i<n; i++)
		data[i] = machdata->swal(data[i]);

	delta = data[0]-data[1];
	print("total: %ld\n", data[0]);
	if(data[0] == 0)
		exits(0);
	if (!textsym(&s, 0))
		error(0, "no text symbols");
	tbase = s.value & ~(mach->pgsize-1);	/* align down to page */
	print("TEXT %.8lux\n", tbase);
	/*
	 * Accumulate counts for each function
	 */
	cp = 0;
	k = 0;
	for (i = 0, j = (s.value-tbase)/PCRES+2; j < n; i++) {
		name = s.name;		/* save name */
		if (!textsym(&s, i))	/* get next symbol */
			break;
		sum = 0;
		while (j < n && j*PCRES < s.value-tbase)
			sum += data[j++];
		if (sum) {
			cp = realloc(cp, (k+1)*sizeof(struct COUNTER));
			if (cp == 0)
				error(1, "realloc");
			cp[k].name = name;
			cp[k].time = sum;
			k++;
		}
	}
	if (!k)
		error(0, "no counts");
	cp[k].time = 0;			/* "etext" can take no time */
	/*
	 * Sort by time and print
	 */
	qsort(cp, k, sizeof(struct COUNTER), compar);
	Binit(&outbuf, 1, OWRITE);
	Bprint(&outbuf, "    ms      %%   sym\n");
	while(--k>=0)
		Bprint(&outbuf, "%6ld\t%3lld.%lld\t%s\n",
				cp[k].time,
				100LL*cp[k].time/delta,
				(1000LL*cp[k].time/delta)%10,
				cp[k].name);
	exits(0);
}
