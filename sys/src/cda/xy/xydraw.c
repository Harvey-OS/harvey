#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

int	debug;

Biobuf	out;

Sym *	master;

int	colors[] = {255, 2, 5<<5, 5<<2};
int	colori;

#define	MAXCOLOR	(sizeof colors/sizeof colors[0] - 1)

Clump *	slist;	/* show these layers */
Clump *	plist;	/* print these clumps */
int	rflag;
int	tflag;

void
main(int argc, char **argv)
{
	char *l, lbuf[32];
	Sym *s, *d; int i, fcode;
	Sym *lastop;

	xyinit();
	lastop = op_plus;
	slist = new(CLUMP);
	plist = new(CLUMP);
	ARGBEGIN{
	case 'D':
		++debug;
		break;
	case 'a':
		if(lastop != op_plus)
			wrclump(slist, op_plus);
		lastop = op_plus;
		wrclump(slist, getlayer(ARGF()));
		break;
	case 'c':
		colori++;
		break;
	case 's':
		if(lastop != op_minus)
			wrclump(slist, op_minus);
		lastop = op_minus;
		wrclump(slist, getlayer(ARGF()));
		break;
	case 'l':
		l = ARGF();
		sprint(lbuf, "%sA", l);
		if(lastop != op_plus)
			wrclump(slist, op_plus);
		wrclump(slist, getlayer(lbuf));
		sprint(lbuf, "%sS", l);
		wrclump(slist, op_minus);
		wrclump(slist, getlayer(lbuf));
		lastop = op_minus;
		break;
	case 'p':
		wrclump(plist, symstore(ARGF()));
		break;
	case 'r':
		++rflag;
		break;
	case 't':
		++tflag;
		break;
	}ARGEND
	Binit(&out, 1, OWRITE);

	master = xyread(argv[0]);
	if(!master)
		error("empty input file");
	if(plist->n){
		s = plist->o[0];
		if(strcmp(s->name, "*") == 0)
			plist = allclump;
	}
	for(i=0; i<plist->n; i++){
		s = plist->o[i];
		prclump(&out, s->name);
	}
	xyborder(master->name);

	if(slist->n == 0)
		prborder(&out);
	Bflush(&out);

	if(slist->n){
		s = slist->o[0];
		if(strcmp(s->name, "*") == 0)
			slist = allclump;
	}
	d = getlayer("*");
	for(i=0; i<slist->n; i++){
		s = slist->o[i];
		if(s->type != Operator)
			comborder(d, s);
	}
	for(i=0; i<slist->n; i++){
		s = slist->o[i];
		if(s->type != Operator)
			comborder(s, d);
	}
	if(rflag){
		d = getlayer("*");
		wrclump(master->ptr, d->ptr);
		displayer(d->name, (int)op_plus->ptr);
	}
	lastop = op_plus;
	for(i=0; i<slist->n; i++){
		s = slist->o[i];
		if(s->type == Operator){
			lastop = s;
			continue;
		}
		fcode = (int)lastop->ptr;
		if(rflag)
			fcode = F & ~fcode;
		displayer(s->name, fcode);
		if(lastop == op_minus && colori)
			colori = colori%MAXCOLOR + 1;	/* bnl 3/19 */
	}
	if(slist->n && tflag)
		disptexture();
	exits(0);
}
