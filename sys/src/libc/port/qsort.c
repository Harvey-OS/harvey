#include <u.h>
#include <libc.h>

static	int	(*qscmp)(void*, void*);
static	void	(*qsexc)(void*, void*);
static	void	(*qstexc)(void*, void*, void*);
static	long	qses, nqses, qsesn;
typedef	struct	{ long x[4]; } xlong;


static	void
qs1(char *a, char *l)
{
	char *i, *j;
	char *lp, *hp;
	int c;
	long n;


start:
	if((n=l-a) <= nqses)
		return;
	n = qses * (n / (2*qses));
	hp = lp = a+n;
	i = a;
	j = l-qses;
	for(;;) {
		if(i < lp) {
			if((c = (*qscmp)(i, lp)) == 0) {
				(*qsexc)(i, lp -= qses);
				continue;
			}
			if(c < 0) {
				i += qses;
				continue;
			}
		}

loop:
		if(j > hp) {
			if((c = (*qscmp)(hp, j)) == 0) {
				(*qsexc)(hp += qses, j);
				goto loop;
			}
			if(c > 0) {
				if(i == lp) {
					(*qstexc)(i, hp += qses, j);
					i = lp += qses;
					goto loop;
				}
				(*qsexc)(i, j);
				j -= qses;
				i += qses;
				continue;
			}
			j -= qses;
			goto loop;
		}


		if(i == lp) {
			if(lp-a >= l-hp) {
				qs1(hp+qses, l);
				l = lp;
			} else {
				qs1(a, lp);
				a = hp+qses;
			}
			goto start;
		}


		(*qstexc)(j, lp -= qses, i);
		j = hp -= qses;
	}
}

static	void
qs2(char *a, char *l)
{
	char *i, *j;

	j = a;

loop:
	i = j;
	j += qses;
	if(j >= l)
		return;
	if((*qscmp)(i, j) <= 0)
		goto loop;

bub:
	(*qsexc)(i, i+qses);
	if(i <= a)
		goto loop;
	i -= qses;
	if((*qscmp)(i, i+qses) > 0)
		goto bub;
	goto loop;
}

static	void
qsexc1(void *a, void *b)
{
	long n;
	char t, *i, *j;

	i = a;
	j = b;
	n = qsesn;
	do {
		t = *i;
		*i++ = *j;
		*j++ = t;
	} while(--n);
}

static	void
qsexc4(void *a, void *b)
{
	long n;
	long t, *i, *j;

	i = a;
	j = b;
	n = qsesn;
	do {
		t = *i;
		*i++ = *j;
		*j++ = t;
	} while(--n);
}

static	void
qsexc16(void *a, void *b)
{
	long n;
	xlong t, *i, *j;

	i = a;
	j = b;
	n = qsesn;
	do {
		t = *i;
		*i++ = *j;
		*j++ = t;
	} while(--n);
}

static	void
qstexc1(void *a, void *b, void *c)
{
	long n;
	char t, *i, *j, *k;

	i = a;
	j = b;
	k = c;
	n = qsesn;
	do {
		t = *i;
		*i++ = *k;
		*k++ = *j;
		*j++ = t;
	} while(--n);
}

static	void
qstexc4(void *a, void *b, void *c)
{
	long n;
	long t, *i, *j, *k;

	i = a;
	j = b;
	k = c;
	n = qsesn;
	do {
		t = *i;
		*i++ = *k;
		*k++ = *j;
		*j++ = t;
	} while(--n);
}

static	void
qstexc16(void *a, void *b, void *c)
{
	long n;
	xlong t, *i, *j, *k;

	i = a;
	j = b;
	k = c;
	n = qsesn;
	do {
		t = *i;
		*i++ = *k;
		*k++ = *j;
		*j++ = t;
	} while(--n);
}

void
qsort(void *a, long n, long es, int (*fc)(void*, void*))
{
	long l;

	qscmp = fc;
	qses = es;
	nqses = 6 * qses;
	if(qses >= sizeof(xlong) && qses%sizeof(xlong) == 0) {
		qsexc = qsexc16;
		qstexc = qstexc16;
		qsesn = qses/sizeof(xlong);
	} else
	if(qses >= sizeof(long) && qses%sizeof(long) == 0) {
		qsexc = qsexc4;
		qstexc = qstexc4;
		qsesn = qses/sizeof(long);
	} else {
		qsexc = qsexc1;
		qstexc = qstexc1;
		qsesn = qses;
	}
	l = n * qses;
	qs1((char*)a, (char*)a+l);
	qs2((char*)a, (char*)a+l);
}
