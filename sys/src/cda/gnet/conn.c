#include <stdio.h>
#include "geom.h"
#include "thing.h"
#include "text.h"
#include "wire.h"
#include "conn.h"

void Conn::nets(WireList *w)
{
	register i;
	register Wire **wp;
	for (i = 0, wp = (Wire **) w->a; i < w->n; i++, wp++)
		if ((*wp)->contains(pin->p))
		/* if (pin->p == (*wp)->P || pin->p == (*wp)->Q) */
			net = (*wp)->net;
}

void Conn::put(FILE *ouf)
{
	if (net == 0)
		fprintf(stderr,"unconnected pin %s,%s\n",pin->t->s,pin->s->s);
	else
		fprintf(ouf,"	%s	%s,%s	%% %d %d %d\n",net->s->s,pin->t->s,pin->s->s,pin->style,pin->p.x,pin->p.y);
}

void Conn::putm(FILE *ouf)
{
	if (net == 0)
		fprintf(stderr,"unconnected macro pin %s\n",pin->s->s);
	else if (io == OUT)
		fprintf(ouf,".m	%s	%s	>\n",pin->s->s,net->s->s);
	else
		fprintf(ouf,".m	%s	%s	<\n",pin->s->s,net->s->s);
}

void Conn::fixup(char *suf)
{
	pin->s->append(suf);
}

void ConnList::nets(WireList *w)
{
	register i;
	for (i = 0; i < n; i++)
		conn(i)->nets(w);
}
