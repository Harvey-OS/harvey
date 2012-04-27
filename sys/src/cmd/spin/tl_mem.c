/***** tl_spin: tl_mem.c *****/

/* Copyright (c) 1995-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

/* Based on the translation algorithm by Gerth, Peled, Vardi, and Wolper, */
/* presented at the PSTV Conference, held in 1995, Warsaw, Poland 1995.   */

#include "tl.h"

#if 1
#define log(e, u, d)	event[e][(int) u] += (long) d;
#else
#define log(e, u, d)
#endif

#define A_LARGE		80
#define A_USER		0x55000000
#define NOTOOBIG	32768

#define POOL		0
#define ALLOC		1
#define FREE		2
#define NREVENT		3

extern	unsigned long All_Mem;
extern	int tl_verbose;

union M {
	long size;
	union M *link;
};

static union M *freelist[A_LARGE];
static long	req[A_LARGE];
static long	event[NREVENT][A_LARGE];

void *
tl_emalloc(int U)
{	union M *m;
  	long r, u;
	void *rp;

	u = (long) ((U-1)/sizeof(union M) + 2);

	if (u >= A_LARGE)
	{	log(ALLOC, 0, 1);
		if (tl_verbose)
		printf("tl_spin: memalloc %ld bytes\n", u);
		m = (union M *) emalloc((int) u*sizeof(union M));
		All_Mem += (unsigned long) u*sizeof(union M);
	} else
	{	if (!freelist[u])
		{	r = req[u] += req[u] ? req[u] : 1;
			if (r >= NOTOOBIG)
				r = req[u] = NOTOOBIG;
			log(POOL, u, r);
			freelist[u] = (union M *)
				emalloc((int) r*u*sizeof(union M));
			All_Mem += (unsigned long) r*u*sizeof(union M);
			m = freelist[u] + (r-2)*u;
			for ( ; m >= freelist[u]; m -= u)
				m->link = m+u;
		}
		log(ALLOC, u, 1);
		m = freelist[u];
		freelist[u] = m->link;
	}
	m->size = (u|A_USER);

	for (r = 1; r < u; )
		(&m->size)[r++] = 0;

	rp = (void *) (m+1);
	memset(rp, 0, U);
	return rp;
}

void
tfree(void *v)
{	union M *m = (union M *) v;
	long u;

	--m;
	if ((m->size&0xFF000000) != A_USER)
		Fatal("releasing a free block", (char *)0);

	u = (m->size &= 0xFFFFFF);
	if (u >= A_LARGE)
	{	log(FREE, 0, 1);
		/* free(m); */
	} else
	{	log(FREE, u, 1);
		m->link = freelist[u];
		freelist[u] = m;
	}
}

void
a_stats(void)
{	long	p, a, f;
	int	i;

	printf(" size\t  pool\tallocs\t frees\n");
	for (i = 0; i < A_LARGE; i++)
	{	p = event[POOL][i];
		a = event[ALLOC][i];
		f = event[FREE][i];

		if(p|a|f)
		printf("%5d\t%6ld\t%6ld\t%6ld\n",
			i, p, a, f);
	}
}
