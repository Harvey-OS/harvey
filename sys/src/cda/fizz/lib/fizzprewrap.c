#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

extern void chkvsig(Vsig *, int), chknet(char *),  chkroute(char *);

int
fizzprewrap(void)
{
	int i;
	extern long nnet;

	symtraverse(S_TT, ttclean);
	symtraverse(S_SIGNAL, putnet);
	if(f_nerrs)
		return(f_nerrs);
	for(i = 0; i < 6; i++)
		if(f_b->v[i].name || f_b->v[i].npins)
			chkvsig(&f_b->v[i], i);
	nnet = 0;
	symtraverse(S_SIGNAL, chknet);
	if(f_nerrs)
		return(f_nerrs);
	symtraverse(S_SIGNAL, chkroute);
	if(f_b->align[0].x < 0){
		f_b->align[0] = f_b->nw;
		f_b->align[1] = f_b->sw;
		f_b->align[2] = f_b->se;
		f_b->align[3] = f_b->ne;
	}
	return(f_nerrs);
}
