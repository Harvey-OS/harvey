#include "jep.h"

/*
   Pict		x
   Group	x
   Region	x
   Object	x

   ArcChordPie	x
   Curve	x
   Ellipse	x
   RoundRect	x

   Point	x
   Poly		x
   Rect		x
   Text		x
 */

char*	deffile	= "last.tcl";
int	newsize;
int	filsize;

void
main(int argc, char *argv[])
{
	int i;

	newsize = 0;
	filsize = 0;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	case 'a':
		aflag++;
		break;
	case 'v':
		vflag++;
		break;
	case 'p':
		pflag++;
		break;
	case 'd':
		if(hereh == 0) {
			herel = 1;
			hereh = 10000;
		}
		dflag++;
		break;
	case 'h':
		herel = hereh;
		hereh = atoi(ARGF());
		if(herel == 0)
			herel = hereh;
		dflag++;
		break;
	case 's':
		sflag++;
		break;
	case 'f':
		fflag++;
		break;
	case 'y':
		xflag++;
		break;
	case 't':
		type = ARGF();
		dflag++;
		break;
	case 'x':
		pxflag = strtoul(ARGF(), 0, 16);
		dflag++;
		break;
	case 'm':
		pmout = Bopen("/tmp/jepout.pm", OWRITE);
		break;
	case 'n':
		newsize = atoi(ARGF());
		break;
	case 'q':
		qflag++;
		break;
	case 'z':
		filsize = atol(ARGF());
		break;
	} ARGEND

	if(pmout == 0)
		pmin = Bopen("jepout.pm", OREAD);

	for(i=0; i<argc; i++) {
		file = argv[i];
		jep();
	}
	if(argc == 0) {
		file = deffile;
		jep();
	}
	exits(0);
}

void
jep(void)
{
	uchar *fil;
	int f, len, n;

	fil = 0;

	f = open(file, OREAD);
	if(f < 0) {
		print("cannot open %s: %r\n", file);
		goto out;
	}
	len = seek(f, 0, 2);
	fil = mal(len+100);
	if(fil == 0) {
		print("cannot malloc %s: %d\n", file, len);
		goto out;
	}
	base = fil;
	seek(f, 0, 0);
	n = read(f, fil, len);
	if(n != len) {
		print("short read %s asked %d got %d: %r\n", file, len, n);
		goto out;
	}

	cksum = checksum(fil, len);
	root = parse(fil, len);
	prep(h.nam1, file);
	prep(h.nam2, h.name);

	/* if last name is AP, make it APT */
	strcat(h.nam2, "T");
	if(strcmp(h.nam1, h.nam2) != 0)
		prep(h.nam2, h.name);
	print("%s %.8lux\n", h.nam1, cksum);

	fixgroups();
	bounds(fil);
	if(aflag)
		allnode(fil);
	if(pflag)
		prnode(fil);
	if(dflag)
		drnode(fil);
	psnode(fil);

out:
	if(f >= 0)
		close(f);
	if(fil != 0)
		free(fil);
}

void*
mal(int n)
{
	void *v;

	v = malloc(n);
	if(v != 0)
		memset(v, 0, n);
	return v;
}

Node*
parse(uchar *p, int len)
{
	Node *l, *m, *r;
	int t, f, f1, f2, i, ord;
	uchar *p0, *pe;

	p0 = doheader(p, &len);
	pe = p+len;
	p = p0;
	ord = 0;
	r = mal(sizeof(*l));
	m = r;
	r->type = Tgroup1;
	r->p = h.p1;
	r->g.p = h.p2;
	r->g.name = h.name;
	r->g.count = h.count;
	r->ord = ord;

loop:
	p0 = p;
	l = mal(sizeof(*l));
	ord++;
	l->ord = ord;
	l->beg = p0 - base;

	t = *p++;
	l->op = t;
	f = 0;

	if(xflag && xcount == 0)
		setparam(&l->p, Px, *p++);
	else
		xcount--;

	if(t & 0x40) {
		f |= *p++;
		t &= ~0x40;
	}
	if(vflag)
		print("%.4lux t=%.2x f=%.2x x=%d.%d\n", l->beg, l->op, f, xflag, xcount);

	switch(t) {
	default:
		print("%s %d unknown opcode %d\n", file, l->ord, t);
		exits(0);

	case 0x1:
		p = getparam(p, f, &l->p);
		l->type = Tpoint;

		l->l.vec[0].x = signp(p+0);
		l->l.vec[0].y = signp(p+2);
		p += 4;
		break;

	case 0x2:
		p = getparam(p, f, &l->p);
		l->type = Tline1;

		for(i=0; i<2; i++) {
			l->l.vec[i].x = signp(p+0);
			l->l.vec[i].y = signp(p+2);
			p += 4;
		}
		break;

	case 0x3:
		p = getparam(p, f, &l->p);
		l->type = Tvect1;

		l->v.count = p[0]|(p[1]<<8);
		p += 2;

		l->v.vec = mal(l->v.count * sizeof(l->v.vec[0]));
		for(i=0; i<l->v.count; i++) {
			l->v.vec[i].x = signp(p+0);
			l->v.vec[i].y = signp(p+2);
			p += 4;
		}
		break;

	case 0x4:
		p = getparam(p, f, &l->p);
		l->type = Tvect2;

		l->v.count = p[0]|(p[1]<<8);
		p += 2;

		l->v.vec = mal(l->v.count * sizeof(l->v.vec[0]));
		for(i=0; i<l->v.count; i++) {
			l->v.vec[i].x = signp(p+0);
			l->v.vec[i].y = signp(p+2);
			p += 4;
		}
		break;

	case 0x5:
		p = getparam(p, f, &l->p);
		l->type = Tvect3;

		l->v.count = p[0]|(p[1]<<8);
		p += 2;

		l->v.vec = mal(l->v.count * sizeof(l->v.vec[0]));
		for(i=0; i<l->v.count; i++) {
			l->v.vec[i].x = signp(p+0);
			l->v.vec[i].y = signp(p+2);
			p += 4;
		}
		break;

	case 0x14:
		p = getparam(p, f, &l->p);
		l->type = Tgroup3;

		l->v.count = p[0]|(p[1]<<8);
		xcount = l->v.count;
		p += 2;
		break;

	case 0x83:
		p = getparam(p, f, &l->p);
		l->type = Tcirc2;
		for(i=0; i<2; i++) {
			l->l.vec[i].x = signp(p+0);
			l->l.vec[i+2].x = l->l.vec[i].x;
			l->l.vec[i].y = signp(p+2);
			l->l.vec[i+2].y = l->l.vec[i].y;
			p += 4;
		}
		docirc(l);
		break;

	case 0x85:
		p = getparam(p, f, &l->p);
		l->type = Tcirc3;
		for(i=0; i<2; i++) {
			l->l.vec[i].x = signp(p+0);
			l->l.vec[i+2].x = l->l.vec[i].x;
			l->l.vec[i].y = signp(p+2);
			l->l.vec[i+2].y = l->l.vec[i].y;
			p += 4;
		}
		docirc(l);
		break;

	case 0x8:
		p = getparam(p, f, &l->p);
		l->type = Tcirc1;
		l->c.r = (p[4]|(p[5]<<8)) / 2;
		l->c.x = signp(p+0) + l->c.r;
		l->c.y = signp(p+2) + l->c.r;
		p += 6;
		break;

	case 0x9:
		p = getparam(p, f, &l->p);
		l->type = Tline4;	// new
		for(i=0; i<2; i++) {
			l->l.vec[i].x = signp(p+0);
			l->l.vec[i].y = signp(p+2);
			p += 4;
		}
		break;

	case 0xa:
		p = getparam(p, f, &l->p);
		l->type = Tarc;

		for(i=0; i<4; i++) {
			l->l.vec[i].x = signp(p+0);
			l->l.vec[i].y = signp(p+2);
			p += 4;
		}
		doarc(l);
		break;

	case 0xd:
		p = getparam(p, f, &l->p);
		l->type = Tstring;

		l->s.x = signp(p+0);
		l->s.y = signp(p+2);
		p += 4;

		i = *p++;
		l->s.str = mal(i+1);
		memmove(l->s.str, p, i);
		p += i;
		break;

	case 0xf:
		f1 = 0;
		f2 = 0;
		if(f & ~0xd0)
			print("unknown group flag %.2x\n", f);
		if(f & 0x80)
			f1 = *p++;
		if(f & 0x40)
			f2 = *p++;
		if(f & 0x10) {
			i = *p++;
			l->g.name = mal(i+1);
			memcpy(l->g.name, p, i);
			p += i;
		}
		if(f1)
			p = getparam(p, f1, &l->p);
		if(f2)
			p = getparam(p, f2, &l->g.p);
		l->type = Tgroup2;
		l->g.count = p[0]|(p[1]<<8);
		p += 2;
		break;
	}
	if(p > pe) {
		print("%.4lux t=%.2x f=%.2x pastend\n", l->beg, l->op, f);
		if(!vflag)
			exits(0);
		return r;
	}
	l->end = p-base;
	if(m == 0)
		r = l;
	else
		m->link = l;
	m = l;
	if(p == pe)
		return r;
	goto loop;
}

void
setparam(Param *p, int i, uint v)
{
	p->paramf |= 1<<i;
	p->param[i] = v;
}

uchar*
getparam(uchar *p, int f, Param *m)
{
	int i, j, n, v;

	for(i=0; i<8; i++)
	if(f & (0x80>>i))
	switch("1211x121"[i]) {
	default:
		print("bad param \n");
//		print("%.4lux t=%.2x f=%.2x b=%.2x bad param %c\n",
//			l->beg, l->op, f, f & (0x80>>i), v[i]);
		break;
	case '1':
		v = p[0];
		if(vflag)
			print("%.4ux b=%.2x param 1 %.2x\n",
				(int)(p-base), f & (0x80>>i), v);
		setparam(m, i, v);
		p += 1;
		break;
	case '2':
		v = p[0] | (p[1]<<8);
		if(vflag)
			print("%.4ux b=%.2x param 2 %.4ux\n",
				(int)(p-base), f & (0x80>>i), v);
		setparam(m, i, v);
		p += 2;
		break;
	case 'x':
		v = p[0] | (p[1]<<8);
		setparam(m, i, v);
		if(v != 0xf) {
			if(vflag)
				print("%.4ux b=%.2x param 2 %.4ux\n",
				(int)(p-base), f & (0x80>>i), v);
			p += 2;
			break;
		}
		switch(p[2]) {
		default:
			print("%s %d unknown fil pattern %.2ux\n", file, (int)(p-base), p[2]);
			n = filsize;
			break;
		case 0xc8:
		case 0x8c:
			n = 7;
			break;
		case 0x98:
		case 0xcc:
			n = 9;
			break;
		case 0xb8:
		case 0xd8:
			n = 11;
			break;
		case 0xf8:
			n = 13;
			break;
		}
		memmove(m->fil, p+2, n);
		if(vflag) {
			print("%.4ux b=%.2x param 2 %.4ux [%d]",
				(int)(p-base), f & (0x80>>i), v, n);
			for(j=0; j<n; j++)
				print(" %.2x", m->fil[j]);
			print("\n");
		}
		p += 2+n;
		break;
	}
	return p;
}

int
dist(int x1, int y1, int x2, int y2)
{
	return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)) + .5;
}

double
angl(int x1, int y1, int x2, int y2)
{

	return atan2((y2-y1), -(x2-x1)) * 180/PI + 180 + .5;
}

void
doarc(Node *l)
{
	int x,x0,x1,x2,x3;
	int y,y0,y1,y2,y3;
	double a1,a2;

	x0 = l->l.vec[0].x;
	x1 = l->l.vec[1].x;
	x2 = l->l.vec[2].x;
	x3 = l->l.vec[3].x;

	y0 = l->l.vec[0].y;
	y1 = l->l.vec[1].y;
	y2 = l->l.vec[2].y;
	y3 = l->l.vec[3].y;

	x = (x0+x1)/2;
	y = (y0+y1)/2;
	a1 = angl(x,y, x2,y2);
	a2 = angl(x,y, x3,y3);
	if(a1 >= a2)
		a2 += 360;

	l->c.x = x;
	l->c.y = y;
	l->c.r = dist(x,y, x2,y2);
	l->c.a1 = a1;
	l->c.a2 = a2;
}

void
docirc(Node *l)
{
	int x,x0,x1;
	int y,y0,y1;

	x0 = l->l.vec[0].x;
	x1 = l->l.vec[1].x;

	y0 = l->l.vec[0].y;
	y1 = l->l.vec[1].y;

	x = (x0+x1)/2;
	y = (y0+y1)/2;

	l->c.x = x;
	l->c.y = y;
	l->c.r = (x1-x0)/2;
	if((x1-x0) != (y1-y0)) {
//		print("%s %d circ assert failed\n", file, l->ord);
//		make into and elipse
//		/n/jep1/charts/mmtj111.tcl 7 circ assert failed
//		/n/jep1/charts/kmkgapt.tcl 30 circ assert failed
//		/n/jep1/charts/kmkgapt.tcl 31 circ assert failed
//		/n/jep1/charts/krno191.tcl 861 circ assert failed
//		/n/jep2/charts/egae161.tcl 32 circ assert failed

	}
}

void
fixgroups(void)
{
	Node *n, *g;
	int c;

	c = 1;
	g = 0;
	for(n=root; n; n = n->link) {
		if(c == 0) {
//			print("%s %d too little count\n", file n->ord);
			c = 1;
		}
		switch(n->type) {
		case Tgroup1:
		case Tgroup2:
		case Tgroup3:
			n->parent = g;
			n->g.oldcount = c-1;
			g = n;
			c = n->g.count;
			break;

		default:
			n->parent = g;
			c = c-1;
			break;
		}
		while(c == 0 && g != 0) {
			c = g->g.oldcount;
			g = g->parent;
		}
	}
//	if(c != 0 || g != 0)
//		print("too much count: %d\n", c);
}

int
signp(uchar *p)
{
	int v;

	v = p[0] | (p[1]<<8);
	if(v & 0x8000)
		v |= -0x8000;
	return v;
}

void
defcolor(int r, int g, int b)
{
	int n;

	n = h.ncolor * 3;
	h.color[n+0] = r;
	h.color[n+1] = g;
	h.color[n+2] = b;
	h.ncolor++;
}

uchar*
doheader(uchar *fil, int *len)
{
	int i, f0, f1, f2, f3, f4, n, j;
	uchar *p;

	h.ncolor = 0;
	defcolor(0x00, 0x00, 0x00); // black
	defcolor(0xff, 0xff, 0xff); // white

	p = fil;
	p += 3;		// gok

	f0 = *p++;
	if(f0 & 0x80) {
		f0 &= ~0x80;
		*len -= 6;
	}
	if(f0 & ~0x83)
		xflag = 1;

	p += 2;		// gok

	memmove(h.name, p, 8);
	p += 8;

	f1 = *p++;
	f2 = *p++;
	f3 = 0;
	if(f2 & 0x80)
		f3 = *p++;
	f4 = 0;
	if(f2 & 0x40)
		f4 = *p++;

	if(vflag)
		print("%s %d f0=%.2x f1=%.2x f2=%.2x f3=%.2x f4=%.2x\n",
			file, (int)(p-fil),
			f0, f1, f2, f3, f4);

	if(f1 & 0x04)
		p += 4;	// gok

	if(f1 & 0x02)
		p += 1;	// gok

	if(f1 & 0x1) {
		n = *p++;
		for(i=0; i<n; i++) {
			defcolor(p[2], p[1], p[0]);
			p += 3;
		}
	}

	h.minbx = signp(p);
	p += 2;
	h.minby = signp(p);
	p += 2;
	h.maxbx = signp(p);
	p += 2;
	h.maxby = signp(p);
	p += 2;

	/* encaptulated data */
	h.nencap = 0;
	if(f2 & 0x08) {
		n = p[0]|(p[1]<<8);		// number of groups
		p += 2;
		h.nencap = n;
		h.encap = mal(n*sizeof(*h.encap));
		for(i=0; i<n; i++) {
			j = p[0] | (p[1]<<8);
			p += 2;
			h.encap[i].gok = j;

			j = p[0] | (p[1]<<8);
			p += 2;
			if(p+j >= fil + *len) {
				print("eof in encaptulation\n");
				exits(0);
			}
			h.encap[i].data = mal(j);
			h.encap[i].ndata = j;
			memmove(h.encap[i].data, p, j);
			p += j;
/*
print("%s %d-%d %.4x", file, i, j, h.encap[i].gok);
for(j=0; j<h.encap[i].ndata; j++)
print(" %.2x", h.encap[i].data[j]);
print("\n");
*/
		}
	}

	if(f3)
		p = getparam(p, f3, &h.p1);
	if(f4)
		p = getparam(p, f4, &h.p2);

	h.count = p[0]|(p[1]<<8);
	p += 2;

	return p;
}

void
prep(char *p0, char *q)
{
	char *p;
	int c;

	p = p0;
	while(c = *q++) {
		if(c == '.')
			break;
		if(c == '/') {
			p = p0;
			continue;
		}
		if(c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		*p++ = c;
	}
	*p = 0;
}

ulong
checksum(uchar *p, int len)
{
	ulong s;
	int i;

	s = 0;
	for(i=0; i<len; i++)
		s = s*2000000767UL + p[i]*2000001049UL;
	return s;
}

void
bx(int x)
{
	if(x > h.maxx)
		h.maxx = x;
	if(x < h.minx)
		h.minx = x;
}

void
by(int y)
{
	if(y > h.maxy)
		h.maxy = y;
	if(y < h.miny)
		h.miny = y;
}

void
bounds(uchar*)
{
	int i, c;
	Node *n;

	h.maxx = -10000;
	h.maxy = -10000;
	h.minx = +10000;
	h.miny = +10000;

	for(n=root; n; n = n->link) {
		switch(n->type) {
		case Tstring:
			bx(n->s.x);
			by(n->s.y);
			break;
		case Tpoint:
			bx(n->l.vec[0].x);
			by(n->l.vec[0].y);
			break;
		case Tline1:
		case Tline2:
		case Tline3:
		case Tline4:
			for(i=0; i<2; i++) {
				bx(n->l.vec[i].x);
				by(n->l.vec[i].y);
			}
			break;
		case Tgroup1:
		case Tgroup2:
		case Tgroup3:
			break;
		case Tvect1:
		case Tvect2:
		case Tvect3:
			c = n->v.count;
			for(i=0; i<c; i++) {
				bx(n->v.vec[i].x);
				by(n->v.vec[i].y);
			}
			break;
		}
	}
	h.dx = h.maxx - h.minx;
	h.dy = h.maxy - h.miny;

	if(vflag) {
		print("b %d>x>%d %d>y>%d\n",
			h.minbx, h.maxbx,
			h.minby, h.maxby);
		print("a %d>x>%d %d>y>%d\n",
			h.minx, h.maxx,
			h.miny, h.maxy);
	}
}
