/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

#define	PCRES	8

struct COUNTER
{
	char 	*name;		/* function name */
	int32_t	time;		/* ticks spent there */
};

void
error(int perr, char *s)
{
	fprint(2, "kprof: %s", s);
	if(perr){
		fprint(2, ": ");
		perror(0);
	}else
		fprint(2, "\n");
	exits(s);
}

int
compar(const void *va, const void *vb)
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
	int32_t i, j, k, n;
	char *name;
	uint32_t *data;
	int64_t tbase;
	uint32_t sum;
	int32_t delta;
	Symbol s;
	Biobuf outbuf;
	Fhdr f;
	Dir *d;
	struct COUNTER *cp;

	if(argc != 3)
		error(0, "usage: kprof text data");
	/*
	 * Read symbol table
	 */
	fd = open(argv[1], OREAD);
	if(fd < 0)
		error(1, argv[1]);
	if (!crackhdr(fd, &f))
		error(1, "read text header");
	if (f.type == FNONE)
		error(0, "text file not an a.out");
	if (syminit(fd, &f) < 0)
		error(1, "syminit");
	close(fd);
	/*
	 * Read timing data
	 */
	fd = open(argv[2], OREAD);
	if(fd < 0)
		error(1, argv[2]);
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
		data[i] = beswal(data[i]);
	delta = data[0]-data[1];
	print("total: %ld	in kernel text: %ld	outside kernel text: %ld\n",
		data[0], delta, data[1]);
	if(data[0] == 0)
		exits(0);
	if (!textsym(&s, 0))
		error(0, "no text symbols");

	tbase = mach->kbase;
	if(tbase != s.value & ~0xFFF)
		print("warning: kbase %.8llux != tbase %.8llux\n",
			tbase, s.value&~0xFFF);
	print("KTZERO %.8llux PGSIZE %dKb\n", tbase, mach->pgsize/1024);
	/*
	 * Accumulate counts for each function
	 */
	cp = 0;
	k = 0;
	for (i = 0, j = 2; j < n; i++) {
		name = s.name;		/* save name */
		if (!textsym(&s, i))	/* get next symbol */
			break;
		s.value -= tbase;
		s.value /= PCRES;
		sum = 0;
		while (j < n && j < s.value)
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
	Bprint(&outbuf, "ms	  %%	sym\n");
	while(--k>=0)
		Bprint(&outbuf, "%ld\t%3lld.%lld\t%s\n",
				cp[k].time,
				100LL*cp[k].time/delta,
				(1000LL*cp[k].time/delta)%10,
				cp[k].name);
	exits(0);
}
