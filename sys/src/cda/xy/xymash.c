#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

int	debug;

Biobuf	out;

Sym *	master;

Clump *	slist;	/* show these layers */
Clump *	plist;	/* print these clumps */
int	rflag;
int	flashit;

void
main(int argc, char **argv)
{
	char *l, lbuf[32];
	Sym *s, *d; int i, fcode;
	Sym *lastop; Rect *rp;

	fmtinstall('G', gerbconv);
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
	case 'f':
		flashit = strtol(ARGF(), 0, 0);
		break;
	case 'r':
		++rflag;
		break;
	}ARGEND
	Binit(&out, 1, OWRITE);

	master = xyread(argv[0]);
	if(!master)
		error("empty input file");
	xyborder(master->name);

	if(slist->n == 0)
		prborder(&out);
	Bflush(&out);

	if(plist->n){
		s = plist->o[0];
		if(strcmp(s->name, "*") == 0)
			plist = allclump;
	}
	if(slist->n){
		s = slist->o[0];
		if(strcmp(s->name, "*") == 0)
			slist = allclump;
	}
	for(i=0; i<plist->n; i++){
		s = plist->o[i];
		prclump(&out, s->name);
	}
	if(rflag){
		d = getlayer("*");
		for(i=0; i<slist->n; i++){
			s = slist->o[i];
			if(s->type != Operator)
				comborder(d, s);
		}
		/*probject(&out, d->ptr);*/
		wrclump(master->ptr, d->ptr);
		gerberin(d->name, (int)op_plus->ptr);
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
			fcode = S ^ fcode;
		gerberin(s->name, fcode);
	}
	gerberout();
	exits(0);
}

enum
{
	NONE	= -1000,
};

int
gerbconv(void *o, Fconv *fp)
{
	char buf[16];
	char *p = buf;

	USED(fp);
	p += sprint(p, "%.5d", *(long *)o);
	while(--p > buf && *p == '0')	/* sic dixit jhc */
		*p = 0;
	fp->f1 = 0;
	fp->f2 = NONE;
	fp->f3 = 0;
	strconv(buf, fp);
	return sizeof(long);
}

static int widths[] = {
	1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 0
};

void
gerberaper(int apsel)
{
	static int prevap;

	if(apsel != prevap){
		Bprint(&out, "G54D%d*\n", apsel);
		prevap = apsel;
	}
}

void
gerberrect(Rectangle r)
{
	int dx, dy, dr, w, apsel, xscan, n;
	int x, y, xmin, ymin, xmax, ymax;

	dx = Dx(r);
	dy = Dy(r);
	if(dx <= 0 || dy <= 0)
		error("bad rect %d %d %d %d",
			r.min.x, r.min.y, r.max.x, r.max.y);
	if(dx < dy){
		dr = dx;
		xscan = 0;
	}else{
		dr = dy;
		xscan = 1;
	}
	for(apsel=0; widths[apsel]; apsel++){
		if(widths[apsel] > dr)
			break;
		w = apsel;
	}
	apsel = w;
	w = widths[apsel];
	apsel += 10;
	if(debug)
		Bprint(&out, "R %d %d %d %d (%d) %d*%d\n",
			r.min.x, r.min.y, r.max.x, r.max.y, dr, (dr+w-1)/w, w);
	gerberaper(apsel);
	ymin = r.min.y + w/2;
	ymax = r.max.y - w/2 - 1;
	xmin = r.min.x + w/2;
	xmax = r.max.x - w/2 - 1;
	while(flashit){
		n = (dx+w-1)/w;
		n *= (dy+w-1)/w;
		if(n > flashit)
			break;
		for(y=ymin;;){
			for(x=xmin;;){
				Bprint(&out, "X%GY%GD03*\n", x, y);
				if(x >= xmax)
					break;
				x += w;
				if(x > xmax)
					x = xmax;
			}
			if(y >= ymax)
				break;
			y += w;
			if(y > ymax)
				y = ymax;
		}
		return;
	}
	if(xscan){
		for(y=ymin;;){
			if(xmin == xmax){
				Bprint(&out, "X%GY%GD03*\n", xmin, y);
			}else if(xmax-xmin <= w){
				Bprint(&out, "X%GY%GD03*\n", xmin, y);
				Bprint(&out, "X%GY%GD03*\n", xmax, y);
			}else{
				Bprint(&out, "X%GY%GD02*\n", xmin, y);
				Bprint(&out, "X%GY%GD01*\n", xmax, y);
			}
			if(y >= ymax)
				break;
			y += w;
			if(y > ymax)
				y = ymax;
		}
	}else{
		for(x=xmin;;){
			if(ymin == xmax){
				Bprint(&out, "X%GY%GD03*\n", x, ymin);
			}else if(ymax-ymin <= w){
				Bprint(&out, "X%GY%GD03*\n", x, ymin);
				Bprint(&out, "X%GY%GD03*\n", x, ymax);
			}else{
				Bprint(&out, "X%GY%GD02*\n", x, ymin);
				Bprint(&out, "X%GY%GD01*\n", x, ymax);
			}
			if(x >= xmax)
				break;
			x += w;
			if(x > xmax)
				x = xmax;
		}
	}
}

void
gerberpath(Path *pp)
{
	int apsel; Point *xp, *ep;

	if(pp->n <= 0)
		error("empty path");
	apsel = pp->width + 20;
	if(apsel < 20 || apsel > 99)
		error("bad path width %d", pp->width);
	gerberaper(apsel);
	xp = pp->edge;
	ep = &pp->edge[pp->n];
	Bprint(&out, "X%GY%GD02*\n", xp->x, xp->y);
	for(; xp < ep; xp++)
		Bprint(&out, "X%GY%GD01*\n", xp->x, xp->y);
}
