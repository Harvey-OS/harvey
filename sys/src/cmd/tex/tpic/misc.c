#include	<stdio.h>
#include	<string.h>
#include	"pic.h"
#include	"y.tab.h"

setdir(n)	/* set direction (hvmode) from LEFT, RIGHT, etc. */
	int n;
{
	switch (n) {
	case UP:	hvmode = U_DIR; break;
	case DOWN:	hvmode = D_DIR; break;
	case LEFT:	hvmode = L_DIR; break;
	case RIGHT:	hvmode = R_DIR; break;
	}
 	return(hvmode);
}

curdir()	/* convert current dir (hvmode) to RIGHT, LEFT, etc. */
{
	switch (hvmode) {
	case R_DIR:	return RIGHT;
	case L_DIR:	return LEFT;
	case U_DIR:	return UP;
	case D_DIR:	return DOWN;
	}
	ERROR "can't happen curdir" FATAL;
	return 0;
}

double getcomp(p, t)	/* return component of a position */
	obj *p;
	int t;
{
	switch (t) {
	case DOTX:
		return p->o_x;
	case DOTY:
		return p->o_y;
	case DOTWID:
		switch (p->o_type) {
		case BOX:
		case BLOCK:
		case TEXT:
			return p->o_val[0];
		case CIRCLE:
		case ELLIPSE:
			return 2 * p->o_val[0];
		case LINE:
		case ARROW:
			return p->o_val[0] - p->o_x;
		case PLACE:
			return 0;
		}
	case DOTHT:
		switch (p->o_type) {
		case BOX:
		case BLOCK:
		case TEXT:
			return p->o_val[1];
		case CIRCLE:
		case ELLIPSE:
			return 2 * p->o_val[1];
		case LINE:
		case ARROW:
			return p->o_val[1] - p->o_y;
		case PLACE:
			return 0;
		}
	case DOTRAD:
		switch (p->o_type) {
		case CIRCLE:
		case ELLIPSE:
			return p->o_val[0];
		}
	}
	ERROR "you asked for a weird dimension or position" WARNING;
	return 0;
}

double	exprlist[100];
int	nexpr	= 0;

exprsave(f)
	double f;
{
	exprlist[nexpr++] = f;
}

char *sprintgen(fmt)
	char *fmt;
{
	char buf[1000];

	sprintf(buf, fmt, exprlist[0], exprlist[1], exprlist[2], exprlist[3], exprlist[4]);
	nexpr = 0;
	free(fmt);
	return tostring(buf);
}

makefattr(type, sub, f)	/* double attr */
	int type, sub;
	double f;
{
	YYSTYPE val;
	val.f = f;
	makeattr(type, sub, val);
}

makeoattr(type, o)	/* obj* attr */
	obj *o;
{
	YYSTYPE val;
	val.o = o;
	makeattr(type, 0, val);
}

makeiattr(type, i)	/* int attr */
	int i;
{
	YYSTYPE val;
	val.i = i;
	makeattr(type, 0, val);
}

maketattr(sub, p)	/* text attribute: takes two */
	char *p;
{
	YYSTYPE val;
	val.p = p;
	makeattr(TEXTATTR, sub, val);
}

addtattr(sub)		/* add text attrib to existing item */
{
	attr[nattr-1].a_sub |= sub;
}

makevattr(p)	/* varname attribute */
	char *p;
{
	YYSTYPE val;
	val.p = p;
	makeattr(VARNAME, 0, val);
}

makeattr(type, sub, val)	/* add attribute type and val */
	int type, sub;
	YYSTYPE val;
{
	if (type == 0 && val.i == 0) {	/* clear table for next stat */
		nattr = 0;
		return;
	}
	if (nattr >= nattrlist)
		attr = (Attr *) grow((char *)attr, "attr", nattrlist += 100, sizeof(Attr));
	dprintf("attr %d:  %d %d %d\n", nattr, type, sub, val.i);
	attr[nattr].a_type = type;
	attr[nattr].a_sub = sub;
	attr[nattr].a_val = val;
	nattr++;
}

printexpr(f)	/* print expression for debugging */
	double f;
{
	printf("%g\n", f);
}

printpos(p)	/* print position for debugging */
	obj *p;
{
	printf("%g, %g\n", p->o_x, p->o_y);
}

char *tostring(s)
	register char *s;
{
	register char *p;

	p = malloc(strlen(s)+1);
	if (p == NULL)
		ERROR "out of space in tostring on %s", s FATAL;
	strcpy(p, s);
	return(p);
}

obj *makepos(x, y)	/* make a position cell */
	double x, y;
{
	obj *p;

	p = makenode(PLACE, 0);
	p->o_x = x;
	p->o_y = y;
	return(p);
}

obj *makebetween(f, p1, p2)	/* make position between p1 and p2 */
	double f;
	obj *p1, *p2;
{
	obj *p;

	dprintf("fraction = %.2f\n", f);
	p = makenode(PLACE, 0);
	p->o_x = p1->o_x + f * (p2->o_x - p1->o_x);
	p->o_y = p1->o_y + f * (p2->o_y - p1->o_y);
	return(p);
}

obj *getpos(p, corner)	/* find position of point */
	obj *p;
	int corner;
{
	double x, y;

	whatpos(p, corner, &x, &y);
	return makepos(x, y);
}

whatpos(p, corner, px, py)	/* what is the position (no side effect) */
	obj *p;
	int corner;
	double *px, *py;
{
	double x, y, x1, y1;
	extern double sqrt();

	dprintf("whatpos %o %d %d\n", p, p->o_type, corner);
	x = p->o_x;
	y = p->o_y;
	if (p->o_type != PLACE) {
		x1 = p->o_val[0];
		y1 = p->o_val[1];
	}
	switch (p->o_type) {
	case PLACE:
		break;
	case BOX:
	case BLOCK:
	case TEXT:
		switch (corner) {
		case NORTH:	y += y1 / 2; break;
		case SOUTH:	y -= y1 / 2; break;
		case EAST:	x += x1 / 2; break;
		case WEST:	x -= x1 / 2; break;
		case NE:	x += x1 / 2; y += y1 / 2; break;
		case SW:	x -= x1 / 2; y -= y1 / 2; break;
		case SE:	x += x1 / 2; y -= y1 / 2; break;
		case NW:	x -= x1 / 2; y += y1 / 2; break;
		case START:
			if (p->o_type == BLOCK)
				return whatpos(objlist[(int)p->o_val[2]], START, px, py);
		case END:
			if (p->o_type == BLOCK)
				return whatpos(objlist[(int)p->o_val[3]], END, px, py);
		}
		break;
	case ARC:
		switch (corner) {
		case START:
			if (p->o_attr & CW_ARC) {
				x = p->o_val[2]; y = p->o_val[3];
			} else {
				x = x1; y = y1;
			}
			break;
		case END:
			if (p->o_attr & CW_ARC) {
				x = x1; y = y1;
			} else {
				x = p->o_val[2]; y = p->o_val[3];
			}
			break;
		}
		if (corner == START || corner == END)
			break;
		x1 = y1 = sqrt((x1-x)*(x1-x) + (y1-y)*(y1-y));
		/* Fall Through! */
	case CIRCLE:
	case ELLIPSE:
		switch (corner) {
		case NORTH:	y += y1; break;
		case SOUTH:	y -= y1; break;
		case EAST:	x += x1; break;
		case WEST:	x -= x1; break;
		case NE:	x += 0.707 * x1; y += 0.707 * y1; break;
		case SE:	x += 0.707 * x1; y -= 0.707 * y1; break;
		case NW:	x -= 0.707 * x1; y += 0.707 * y1; break;
		case SW:	x -= 0.707 * x1; y -= 0.707 * y1; break;
		}
		break;
	case LINE:
	case SPLINE:
	case ARROW:
		switch (corner) {
		case START:	break;	/* already in place */
		case END:	x = x1; y = y1; break;
		default: /* change! */
		case CENTER:	x = (x+x1)/2; y = (y+y1)/2; break;
		case NORTH:	if (y1 > y) { x = x1; y = y1; } break;
		case SOUTH:	if (y1 < y) { x = x1; y = y1; } break;
		case EAST:	if (x1 > x) { x = x1; y = y1; } break;
		case WEST:	if (x1 < x) { x = x1; y = y1; } break;
		}
		break;
	case MOVE:
		/* really ought to be same as line... */
		break;
	}
	dprintf("whatpos returns %g %g\n", x, y);
	*px = x;
	*py = y;
	return 1;
}

obj *gethere()	/* make a place for curx,cury */
{
	dprintf("gethere %g %g\n", curx, cury);
	return(makepos(curx, cury));
}

obj *getlast(n, t)	/* find n-th previous occurrence of type t */
	int n, t;
{
	int i, k;
	obj *p;

	k = n;
	for (i = nobj-1; i >= 0; i--) {
		p = objlist[i];
		if (p->o_type == BLOCKEND) {
			i = p->o_val[4];
			continue;
		}
		if (p->o_type != t)
			continue;
		if (--k > 0)
			continue;	/* not there yet */
		dprintf("got a last of x,y= %g,%g\n", p->o_x, p->o_y);
		return(p);
	}
	ERROR "there is no %dth last", n WARNING;
	return(NULL);
}

obj *getfirst(n, t)	/* find n-th occurrence of type t */
	int n, t;
{
	int i, k;
	obj *p;

	k = n;
	for (i = 0; i < nobj; i++) {
		p = objlist[i];
		if (p->o_type == BLOCK && t != BLOCK) {	/* skip whole block */
			i = p->o_val[5] + 1;
			continue;
		}
		if (p->o_type != t)
			continue;
		if (--k > 0)
			continue;	/* not there yet */
		dprintf("got a first of x,y= %g,%g\n", p->o_x, p->o_y);
		return(p);
	}
	ERROR "there is no %dth ", n WARNING;
	return(NULL);
}

double getblkvar(p, s)	/* find variable s2 in block p */
	obj *p;
	char *s;
{
	YYSTYPE y, getblk();

	y = getblk(p, s);
	return y.f;
}

obj *getblock(p, s)	/* find variable s in block p */
	obj *p;
	char *s;
{
	YYSTYPE y, getblk();

	y = getblk(p, s);
	return y.o;
}

YYSTYPE getblk(p, s)	/* find union type for s in p */
	obj *p;
	char *s;
{
	static YYSTYPE bug;
	struct symtab *stp;

	if (p->o_type != BLOCK) {
		ERROR ".%s is not in that block", s WARNING;
		return(bug);
	}
	for (stp = p->o_symtab; stp != NULL; stp = stp->s_next)
		if (strcmp(s, stp->s_name) == 0) {
			dprintf("getblk %s found x,y= %g,%g\n",
				s, (stp->s_val.o)->o_x, (stp->s_val.o)->o_y);
			return(stp->s_val);
		}
	ERROR "there is no .%s in that []", s WARNING;
	return(bug);
}

obj *fixpos(p, x, y)
	obj *p;
	double x, y;
{
	dprintf("fixpos returns %g %g\n", p->o_x + x, p->o_y + y);
	return makepos(p->o_x + x, p->o_y + y);
}

obj *addpos(p, q)
	obj *p, *q;
{
	dprintf("addpos returns %g %g\n", p->o_x+q->o_x, p->o_y+q->o_y);
	return makepos(p->o_x+q->o_x, p->o_y+q->o_y);
}

obj *subpos(p, q)
	obj *p, *q;
{
	dprintf("subpos returns %g %g\n", p->o_x-q->o_x, p->o_y-q->o_y);
	return makepos(p->o_x-q->o_x, p->o_y-q->o_y);
}

obj *makenode(type, n)
	int type, n;
{
	obj *p;
	extern char *calloc();

	p = (obj *) calloc(1, sizeof(obj) + (n-1)*sizeof(ofloat));
	if (p == NULL)
		ERROR "out of space in makenode" FATAL;
	p->o_type = type;
	p->o_count = n;
	p->o_nobj = nobj;
	p->o_mode = hvmode;
	p->o_x = curx;
	p->o_y = cury;
	p->o_nt1 = ntext1;
	p->o_nt2 = ntext;
	ntext1 = ntext;	/* ready for next caller */
	if (nobj >= nobjlist)
		objlist = (obj **) grow((char *) objlist, "objlist",
			nobjlist *= 2, sizeof(obj *));
	objlist[nobj++] = p;
	return(p);
}

extreme(x, y)	/* record max and min x and y values */
	double x, y;
{
	if (x > xmax)
		xmax = x;
	if (y > ymax)
		ymax = y;
	if (x < xmin)
		xmin = x;
	if (y < ymin)
		ymin = y;
}
