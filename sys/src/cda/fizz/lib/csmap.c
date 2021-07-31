#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void
allocsig(Chip *c)
{
	Signal **s;

	s = (Signal **)f_malloc(sizeof(Signal *)*(long)(c->npins+1));
	symlook(c->name, S_CSMAP, (void *)s);
	*s = 0;
}

void
addsig(Signal *s)
{
	register Signal **ss;
	register i;

	if(s->type != NORMSIG)
		return;
	for(i = 0; i < s->n; i++){
		ss = (Signal **)symlook(s->coords[i].chip, S_CSMAP, (void *)0);
		while(*ss)
			if(s == *ss++)
				break;
		if(*ss == 0){
			*ss++ = s;
			*ss = 0;
		}
	}
}

void
csmap(void)
{
	/* set up chip to signals map */
	symtraverse(S_CHIP, allocsig);
	symtraverse(S_SIGNAL, addsig);
}

void
csstat(Chip *c)
{
	Signal *s, **ss;
	int ns = 0, tot = 0;

	ss = (Signal **)symlook(c->name, S_CSMAP, (void *)0);
	while(s = *ss++){
		ns++;
		tot += s->n;
	}
	if (ns) print("Chip %s: %d sigs, ave len=%.1f\n", c->name, ns, tot/(float)ns);
	ss = (Signal **)symlook(c->name, S_CSMAP, (void *)0);
	while(s = *ss++){
		print("%s: %d, ", s->name, s->n);
	}
	print("\n");
}
