#include	"rework.h"

class Pin
{
private:
	long pt;
	Net *net;
	Pin *next;
	friend Pintab;
};

Pintab::Pintab(int n)
{
	npins = n;
	pins = new Pin *[n];
	while(--n >= 0)
		pins[n] = 0;
}

Net *
Pintab::install(long pt, Net *value)
{
	register long h;
	register Pin *s;

	h = pt%npins;
	for(s = pins[h]; s; s = s->next)
		if(s->pt == pt){
			if(value)
				s->net = value;
			return(s->net);
		}
	if(value){
		s = new Pin;
		s->pt = pt;
		s->net = value;
		s->next = pins[h];
		pins[h] = s;
	}
	return(value);
}

void
Pintab::stats()
{
	register Pin **s, *ss;
	int *n;
	register i, j;
	int tot;
	double d = 0;

	n = new int[NPTS];
	for(i = 0; i < npins; i++)
		n[i] = 0;
	for(s = pins; s < &pins[npins]; s++){
		for(j = 0, ss = *s; ss; ss = ss->next)
			j++;
		n[j]++;
		d += j;
	}
	fprintf(stdout, "%g pts, N=%ld\n", d, npins);
	for(i = 1, tot = 0; i < NPTS; i++){
		if(n[i]){
			fprintf(stdout, "%d of length %d\n", n[i], i);
			tot += n[i];
		}
	}
	fprintf(stdout, "ave len = %g\n", d/tot);
}
