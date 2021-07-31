#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

typedef struct Pintab
{
	long value;
	long drill;
	struct Pintab *next;
} Pintab;

#define	NHASH	4099	/* prime please */
static Pintab *hash[NHASH];

void
pininit(void)
{
	register Pintab **s, *ss;

	for(s = hash; s < &hash[NHASH]; s++){
		for(ss = *s; ss; ss = ss->next)
			Free((char *)ss);
		*s = 0;
	}
}

long
pinlook(long sym, long install)
{
	register long h;
	register Pintab *s;

	h = sym;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s; s = s->next)
		if(s->value == sym)
			return(s->drill);
	if(install == 0)
		return(0);
	s = (Pintab *)f_malloc((long)sizeof(Pintab));
	s->value = sym;
	s->drill = install;
	s->next = hash[h];
	hash[h] = s;
	return(1);
}

void
pintraverse(void (*fn)(Pin ))
{
	register Pintab **s, *ss;
	Pin p;

	for(s = hash; s < &hash[NHASH]; s++)
		for(ss = *s; ss; ss = ss->next){
			p.p.x = ss->value%ROWLEN;
			p.p.y = ss->value/ROWLEN;
			p.drill = (char) ss->drill;
			(*fn)(p);
		}
}

void
pinstat(void)
{
	register Pintab **s, *ss;
	int n[NHASH];
	register i, j;
	int tot;
	double d;

	for(i = 0; i < NHASH; i++)
		n[i] = 0;
	for(s = hash; s < &hash[NHASH]; s++){
		for(j = 0, ss = *s; ss; ss = ss->next)
			j++;
		n[j]++;
	}
	print("pintab: N=%ld\n", NHASH);
	for(i = 0, d = 0, tot = 0; i < NHASH; i++){
		if(n[i]) print("%d of length %d\n", n[i], i);
		d += n[i]*i;
		if(i) tot += n[i];
	}
	print("len: tot=%g, ave=%g\n", d, d/tot);
}
