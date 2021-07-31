#include <u.h>
#include <libc.h>
#include "symtab.h"
#include <cda/fizz.h>

#define	NHASH	4099	/* prime please */
#define	HASHMUL	79L	/* this is a good value */
static Symtab *hash[NHASH];

void
syminit(void)
{
	register Symtab **s, *ss;

	for(s = hash; s < &hash[NHASH]; s++){
		for(ss = *s; ss; ss = ss->next)
			Free((char *)ss);
		*s = 0;
	}
}

void *
symlook(char *sym, int space, void *install)
{
	register long h;
	register char *p;
	register Symtab *s;

	for(p = sym, h = space; *p; h += *p++)
		h *= HASHMUL;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s; s = s->next)
		if((s->space == space) && (strcmp(s->name, sym) == 0)){
			if(install)
				s->value = install;
			return(s->value);
		}
	if(install){
		s = (Symtab *)f_malloc((long)sizeof(Symtab));
		s->space = space;
		s->name = sym;
		s->value = install;
		s->next = hash[h];
		hash[h] = s;
	}
	return(install);
}

void
symdel(char *sym, int space)
{
	register long h;
	register char *p;
	register Symtab *s, *ls;

	for(p = sym, h = space; *p; h += *p++)
		h *= HASHMUL;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h], ls = 0; s; ls = s, s = s->next)
		if((s->space == space) && (strcmp(s->name, sym) == 0)){
			if(ls)
				ls->next = s->next;
			else
				hash[h] = s->next;
			Free((char *)s);
		}
}

void
symtraverse(int space, void (*fn)(void *))
{
	register Symtab **s, *ss;

	for(s = hash; s < &hash[NHASH]; s++)
		for(ss = *s; ss; ss = ss->next)
			if(ss->space == space)
				(*fn)(ss->value);
}

void
symstat(void)
{
	register Symtab **s, *ss;
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
	print("N=%ld mul=%ld\n", NHASH, HASHMUL);
	for(i = 0, d = 0, tot = 0; i < NHASH; i++){
		if(n[i]) fprint(1, "%d of length %d\n", n[i], i);
		d += n[i]*i;
		if(i) tot += n[i];
	}
	print("ave len = %g\n", d/tot);
}

void
symdump(char *sym, int space)
{
	register long h;
	register char *p;
	register Symtab *s;

	for(p = sym, h = space; *p; h += *p++)
		h *= HASHMUL;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	print("symdump(%s):\n", sym);
	for(s = hash[h]; s; s = s->next)
		print("\t%s: space=%d value=%ld\n", s->name, s->space, s->value);
}
