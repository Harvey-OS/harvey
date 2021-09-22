#include "jep.h"

#define	Scale		.206

int	lastfont;
int	lastsize;
int	efont;
int	lastadj;
int	lastcol;
int	lastdsh;
int	lastwid;
float	lastpoint;
Node*	curnode;
int	msgcount[10];

char*	adj[] =
{
	"ltxt",
	"ctxt",
	"rtxt",
};

int	getp(Node*, int, int, int);
void	setline(Biobuf *b, Node *l, int w, int d);
void	setfont(Biobuf *b, Node *l, int f, int s);
int	setadj(Node *l);

void
getpmary(void)
{
	char *p;
	int n, i, s, c;

	p = Brdline(pmin, '\n');
	if(p == 0)
		goto out;

	while(*p == ' ')
		p++;
	n = 0;
	for(;;) {
		c = *p++;
		if(c >= '0' && c <= '9') {
			n = n*16 + c - '0';
			continue;
		}
		if(c >= 'a' && c <= 'f') {
			n = n*16 + c - 'a' + 10;
			continue;
		}
		if(c >= 'A' && c <= 'F') {
			n = n*16 + c - 'A' + 10;
			continue;
		}
		break;
	}
	if(n != cksum) {
		pmary[0] = 0;
		return;
	}

	while(*p == ' ')
		p++;
	n = 0;
	while(*p >= '0' && *p <= '9')
		n = n*10 + *p++ - '0';
	pmary[0] = n;

	while(*p == ' ')
		p++;
	switch(*p++) {
	default:
		goto bad;
	case 'a':
		pmary[1] = Tarc;
		if(*p == '0') p++;
		break;
	case 's':
		pmary[1] = Tstring;
		if(*p == '0') p++;
		break;
	case 'p':
		pmary[1] = Tpoint;
		if(*p == '0') p++;
		break;
	case 'c':
		pmary[1] = Tcirc1 + *p++ - '1';
		break;
	case 'v':
		pmary[1] = Tvect1 + *p++ - '1';
		break;
	case 'l':
		pmary[1] = Tline1 + *p++ - '1';
		break;
	}

	for(i=0;; i++) {
		while(*p == ' ')
			p++;
		if(*p == '\n')
			break;
		if(*p < '0' || *p > '9')
			goto bad;
		n = 0;
		s = 0;
		if(*p == '-')
			s = *p++;
		while(*p >= '0' && *p <= '9')
			n = n*10 + *p++ - '0';
		if(s)
			n = -n;
		pmary[i+2] = n;
	}
	return;

bad:
	fprint(2, "*** %d pmin terminating\n", pmary[0]);
out:
	Bterm(pmin);
	pmin = 0;
	pmary[0] = 10000;
	return;
}

void
pmoper(Node *l, int p[], int n)
{
	int i, any;

	if(pmout) {
		Bprint(pmout, "%.8lux %4d %s", cksum, l->ord, tname[l->type]);
		for(i=0; i<n; i++)
			Bprint(pmout, " %d", p[i]);
		Bprint(pmout, "\n");
		return;
	}
	if(pmin == 0)
		return;
	while(pmary[0] < l->ord)
		getpmary();
	if(pmary[0] > l->ord)
		return;
	if(pmary[1] != l->type) {
		print("%s %d pm wrong type\n", file, l->ord);
		pmary[0] = 10000;
		return;
	}
	any = 0;
	for(i=0; i<n; i++) {
		if(pmary[i+2] != p[i])
			any++;
	}
	if(any) {
		print("%s %d pm overide %s\n", file, l->ord, aprint(l));
		print("	");
		for(i=0; i<n; i++) {
			if(pmary[i+2] == p[i])
				print(" %d", p[i]);
			else
				print(" (%d/%d)", p[i], pmary[i+2]);
			p[i] = pmary[i+2];
		}
		print("\n");
	}
}

int
xcos(int x)
{
	return x - h.minx;
}

int
xco(int x)
{
	if(x < h.minx || x > h.maxx) {
		if(x < h.minx-20 || x > h.maxx+20) {
			if(qflag || msgcount[2] == 0)
				print("%s x out of range %d (%d-%d)\n", file,
					x, h.minx, h.maxx);
			msgcount[2]++;
		}
		if(x < h.minx)
			x = h.minx;
		if(x > h.maxx)
			x = h.maxx;
	}
	return xcos(x);
}

int
ycos(int y)
{
	return h.maxy - y;
}

int
yco(int y)
{
	if(y < h.miny || y > h.maxy) {
		if(y < h.miny-20 || y > h.maxy+20) {
			if(qflag || msgcount[3] == 0)
				print("%s y out of range %d (%d-%d)\n", file,
					y, h.miny, h.maxy);
			msgcount[3]++;
		}
		if(y < h.miny)
			y = h.miny;
		if(y > h.maxy)
			y = h.maxy;
	}
	return ycos(y);
}

int
getp(Node *l, int bit, int par, int def)
{
	if(l->p.paramf & (1<<bit))
		return l->p.param[bit];
	switch(par) {
	case 1:
		while(l = l->parent)
			if(l->p.paramf & (1<<bit))
				return l->p.param[bit];
		break;
	case 2:
		while(l = l->parent)
			if(l->g.p.paramf & (1<<bit))
				return l->g.p.param[bit];
		break;
	}
	return def;
}

int
cg3(Node *l)
{
	if(l->parent != nil && l->parent->type == Tgroup3)
		return 1;
	return 0;
}

int
getcolor(Node *l)
{
	int c;
//	int xc;
	Node *p;

	// if locally specified, take that
	if(l->p.paramf & (1<<Pcol)) {
		c = l->p.param[Pcol];
		return c;
	}
	p = l->parent;
	if(p && p->type == Tgroup3) {
		c = getp(p, Pjux, 1, -1);
		if(c < 0)
			c = Black;
//		xc = Black;
//		if(p->p.paramf & (1<<Pjux))
//			xc = p->p.param[Pjux];
//		if(c != xc)
//			print("**** differ %d %d %% %s %d\n", c, xc, file, l->ord);
		return c;
	}
	c = getp(l, Pcol, 1, -1);
	if(c < 0)
		c = Black;
	return c;
}

int
getdash(Node *l)
{
	int d;

	d = getp(l, Pdsh, 1, -1);
	if(d < 0) {
		if(qflag || msgcount[6] == 0)
			print("%s %d dash not specified\n", file, l->ord);
		msgcount[6]++;
		d = 0x80;
	}
	return d;
}

int
getwidth(Node *l)
{
	int w;

	w = getp(l, Psiz, 1, -1);
	if(w < 0) {
		if(qflag || msgcount[0] == 0)
			print("%s %d size not specified\n", file, l->ord);
		msgcount[0]++;
		w = 1;
	}
	return w;
}

int
getvect(Node *l)
{
	int w;

	w = getp(l, Pfil, 1, -1);
	if(w == 1)
		return Foutline;
	if(w == 0xf)
		return Fpattern;
	if(l->parent && (l->parent->type == Tgroup3)) {
		if(l->link && l->link->parent == l->parent)
			return Finterior;
	}
	if(l->p.paramf & (1<<Psiz) && l->p.param[Psiz] > 2)
		return Foutline;
if (0) {
	w = getwidth(l);
	if(w > 1)
		return Foutline;
}
	return Fexterior;
}

int
ignore(Node *)
{
//	if(l->type == Tvect1)
//		if(getvect(l) == Fexterior)
//			return 1;
	return 0;
}

void
setcolor(Biobuf *b, Node *l, int c)
{
	if(c == Grey) {
		if(c != lastcol) {
			Bprint(b, "0.7 0.7 0.7 setrgbcolor %% %d\n", l->ord);
			lastcol = c;
		}
		return;
	}
	if(c < 0) {
		print("%s %d color not specified\n", file, l->ord);
		c = Black;
	}
	if(c >= h.ncolor) {
		print("%s %d color not in range: %d\n", file, l->ord, c);
		c = Black;
	}

	if(c != lastcol) {
		Bprint(b, "%.3f %.3f %.3f setrgbcolor %% %d\n",
			h.color[c*3+0]/255.,
			h.color[c*3+1]/255.,
			h.color[c*3+2]/255., l->ord);
		lastcol = c;
	}
}

int
domac(Biobuf *b, Node *n, int p[], int init)
{
	int x, y, d;

	d = p[2];
	if(d < 0xa0 || d > 0xce)
		return 0;

	switch(d) {
	case 0xa0:	// ctz boundary /n/jep1/charts/nzwn191.tcl
		break;
	case 0xa1:	// turd in /n/jep1/charts/kalb184.tcl
		return 0;
	case 0xa2:	// turd in /n/jep1/charts/kgvq131.tcl
		return 0;
	case 0xa6:	// nopt sector
		x = xco(n->l.vec[0].x) - xco(n->l.vec[1].x);
		y = yco(n->l.vec[0].y) - yco(n->l.vec[1].y);
		x = sqrt(x*x + y*y);
		if(init) {
			setcolor(b, n, p[0]);
			return x;
		}
		y = 30;
		Bprint(b, "%d %d moveto %% %d\n", 0, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x, -y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 0, -y, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xa7:	// turd in /n/jep1/charts/kbdr181.tcl
		return 0;
	case 0xa8:	// solid drawing for abandoned buildings
			// botched, dont understand
		if(init) {
//			setcolor(b, n, p[0]);
			setcolor(b, n, Black);
			return 20;
		}
		d = 8;
		Bprint(b, "%d %d moveto %% %d\n", 0, d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, -d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 0, -d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 0, d/2, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xa9:	// dash dot dot international border
		return 0;
	case 0xaa:	// gok
		break;
	case 0xab:	// gok
		break;
	case 0xac:	// railroad tracks
		x = xco(n->l.vec[0].x) - xco(n->l.vec[1].x);
		y = yco(n->l.vec[0].y) - yco(n->l.vec[1].y);
		x = sqrt(x*x + y*y);
		if(init) {
			setcolor(b, n, p[0]);
			return x;
		}
		setline(b, n, 5, 0x80);
		Bprint(b, "%d %d moveto %% %d\n", 0, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x, 0, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);

		setline(b, n, p[1], p[2]);
		Bprint(b, "%d %d moveto %% %d\n", 0, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x, 0, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		return 0;
	case 0xad:	// some sort of feeder /n/jep2/charts/lfbd191.tcl
		break;
	case 0xae:	// some sort of feeder /n/jep2/charts/lypr191.tcl
		break;

	case 0xb2:	// snow fence triangles
		if(init) {
			setcolor(b, n, p[0]);
			return 16;
		}
		d = 18;
		Bprint(b, "%d %d moveto %% %d\n", 0, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d/2, -d, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xb3:	// double snow fence triangles
		if(init) {
			setcolor(b, n, p[0]);
			return 16;
		}
		d = 14;
		y = 6;
		Bprint(b, "%d %d moveto %% %d\n", 0, y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d/2, y+d, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);

		Bprint(b, "%d %d moveto %% %d\n", 0, -y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, -y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d/2, -y-d, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xb4:	// gok
		break;
	case 0xb6:	// gok
		break;
	case 0xba:	// gok
		break;
	case 0xbf:	// diagonal slash
		if(init) {
			setcolor(b, n, p[0]);
			setline(b, n, 3, 0x80);
			return 16;
		}
		d = 16;
		y = 8;
		Bprint(b, "%d %d moveto %% %d\n", 0, y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, -y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		return 0;

	case 0xc2:	// gok
		break;
	case 0xc3:	// gok
		break;
	case 0xc4:	// communication sector boundary
		if(init) {
			setcolor(b, n, Black);
			return 82;
		}
		x = 10;
		y = 10;
		Bprint(b, "%d %d moveto %% %d\n", 0, y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x, y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2, y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x, y/2, n->ord);

		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x, -y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 0, -y/2, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);

		Bprint(b, "%d %d moveto %% %d\n", x+3*x/2+x+x, -y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x+x, -y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x+x, -0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x+x+3*x/2, -0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x+x+3*x/2, -y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x+x+3*x/2+x, -y/2, n->ord);

		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x+x+3*x/2+x, y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+3*x/2+x+x, y/2, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xc5:	// gok
		break;
	case 0xc6:	// arrow triangles
		if(init) {
			setcolor(b, n, Black);
			return 10;
		}
		d = 8;
		Bprint(b, "%d %d moveto %% %d\n", 0, d, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 0, -d, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xcb:	// half bushs
		if(init) {
			setcolor(b, n, Black);
			setline(b, n, 3, 0x80);
			return 78;
		}
		x = 475;
		y = 2225;
		Bprint(b, "%d %d moveto %% %d\n", 511-x, 2210-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 510-x, 2208-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 509-x, 2205-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 507-x, 2204-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 505-x, 2204-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 503-x, 2204-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 501-x, 2204-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 500-x, 2206-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 499-x, 2209-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 499-x, 2210-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 500-x, 2212-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 501-x, 2214-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		Bprint(b, "%d %d moveto %% %d\n", 520-x, 2199-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 519-x, 2197-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 518-x, 2194-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 516-x, 2194-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 514-x, 2194-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 512-x, 2195-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 511-x, 2197-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 511-x, 2199-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 511-x, 2201-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 512-x, 2203-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		Bprint(b, "%d %d moveto %% %d\n", 527-x, 2191-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 526-x, 2189-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 524-x, 2189-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 522-x, 2188-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 520-x, 2189-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 519-x, 2190-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 518-x, 2193-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 518-x, 2195-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		Bprint(b, "%d %d moveto %% %d\n", 533-x, 2189-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 530-x, 2189-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 528-x, 2190-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 526-x, 2191-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 526-x, 2194-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		Bprint(b, "%d %d moveto %% %d\n", 533-x, 2207-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 538-x, 2218-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		Bprint(b, "%d %d moveto %% %d\n", 535-x, 2192-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 538-x, 2191-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 540-x, 2190-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 543-x, 2190-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 545-x, 2192-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 547-x, 2193-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 547-x, 2196-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 547-x, 2198-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 547-x, 2200-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 546-x, 2202-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 543-x, 2204-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		Bprint(b, "%d %d moveto %% %d\n", 547-x, 2198-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 549-x, 2197-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 551-x, 2197-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 553-x, 2197-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 555-x, 2197-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 558-x, 2201-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 558-x, 2203-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 558-x, 2204-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 557-x, 2206-y, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 556-x, 2207-y, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);
		return 0;
	case 0xcc:	// gok
		break;
	case 0xcd:	// ARP line w arrowhead
		if(init) {
			setcolor(b, n, Black);
			setline(b, n, 3, 0x80);
			return 42;
		}
		x = 30;
		y = 15;
		Bprint(b, "%d %d moveto %% %d\n", 0, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+y/2, 0, n->ord);
		Bprint(b, "stroke %% %d\n", n->ord);

		Bprint(b, "%d %d moveto %% %d\n", x+y/2, 0, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+0, y/2, n->ord);

		Bprint(b, "%d %d lineto %% %d\n", x+4*y/2, 0, n->ord);

		Bprint(b, "%d %d lineto %% %d\n", x+0, -y/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", x+y/2, -0, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	case 0xce:	// visual flight track
		if(init) {
			setcolor(b, n, Black);
			return 42;
		}
		d = 10;
		Bprint(b, "%d %d moveto %% %d\n", 0, d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, d, n->ord);

		Bprint(b, "%d %d lineto %% %d\n", 7*d/2, 0, n->ord);

		Bprint(b, "%d %d lineto %% %d\n", d, -d, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", d, -d/2, n->ord);
		Bprint(b, "%d %d lineto %% %d\n", 0, -d/2, n->ord);
		Bprint(b, "closepath %% %d\n", n->ord);
		Bprint(b, "eofill %% %d\n", n->ord);
		return 0;
	}
	print("%s %d macro %.2x\n", file, n->ord, d);
	return 0;
}

int
macro(Biobuf *b, Node *n, int p[])
{
	int x0, x1, y0, y1, dx, dy;
	int d, u, c, i;
	double a;
	Node nod;

	if(n->type >= Tvect1 && n->type <= Tvect3) {
		nod.type = Tline1;
		for(i=1; i<n->v.count; i++) {
			nod.l.vec[0] = n->v.vec[i-1];
			nod.l.vec[1] = n->v.vec[i-0];
			if(!macro(b, &nod, p))
				return 0;
		}
		if(p[3] == Fexterior) {
			nod.l.vec[0] = n->v.vec[i-1];
			nod.l.vec[1] = n->v.vec[0];
			if(!macro(b, &nod, p))
				return 0;
		}
		return 1;
	}

	u = domac(b, n, p, 1);
	if(u == 0)
		return 0;

	x0 = xco(n->l.vec[0].x);
	y0 = yco(n->l.vec[0].y);
	x1 = xco(n->l.vec[1].x);
	y1 = yco(n->l.vec[1].y);
	dx = x1 - x0;
	dy = y1 - y0;

	d = sqrt(dx*dx + dy*dy);
	a = atan2(dy, dx);

	c = d/u;

	Bprint(b, "gsave %% macro %% %d\n", n->ord);
	Bprint(b, "%d %d translate %% %d\n", x0, y0, n->ord);
	Bprint(b, "%.2f rotate %% %d\n", a*180./PI, n->ord);
	for(i=0; i<c; i++) {
		if(i)
			Bprint(b, "%.4f 0 translate %% %d\n", (double)d/c, n->ord);
		domac(b, n, p, 0);
	}
	Bprint(b, "grestore %% %d\n", n->ord);
	return 1;
}

void
setline(Biobuf *b, Node *l, int w, int d)
{
	int on, off;

	if(d != lastdsh) {
		if(d == 0x80)
			Bprint(b, "[ ] 0 setdash %% %d\n", l->ord);
		else {
			on = dashc[d*2+0];
			off = dashc[d*2+1];
			if(on == 0) {
				if(qflag || msgcount[4] == 0 && d != 0)
					print("%s %d missing dash pattern %.2x\n",
						file, l->ord, d);
				msgcount[4]++;
				on = 10;
				off = 10;
			}
			if(d == 0xa9)
				Bprint(b, "[%d %d %d %d %d %d] 0 setdash %% %d\n",
					on, off, off, off, off, off, l->ord);
			else
				Bprint(b, "[%d %d] 0 setdash %% %d\n", on, off, l->ord);
		}
		lastdsh = d;
	}
	if(w != lastwid) {
		Bprint(b, "%d setlinewidth %% %d\n", w, l->ord);
		lastwid = w;
	}
}

void
setfont(Biobuf *b, Node *l, int f, int s)
{
	float sp;

	if(f < 0) {
		if(qflag || msgcount[8] == 0) {
			print("%s %d font not specified\n", file, l->ord);
			msgcount[8]++;
		}
		f = 0;
	}
	if(s < 0) {
		if(qflag || msgcount[1] == 0)
			print("%s %d size not specified\n", file, l->ord);
		msgcount[1]++;
		s = 0;
	}

	sp = s;
	if(s & 0x8000)
		sp = s | -0x8000;
	sp = -sp;

	switch(f) {
	default:
		if(qflag || msgcount[5] == 0)
			print("%s %d font not known: %.2x (%s)\n",
				file, l->ord, f, l->s.str);
		msgcount[5]++;
		f = 0;
		break;
	case 0x10:
		sp *= 1.1;	// regular
		break;
	case 0x11:
		sp *= 1.0;	// italic
		break;
	case 0x12:
		sp *= 0.8;	// bold condensed
		break;
	case 0x13:
		sp *= 0.75;	// bold
		break;
	case 0x14:
		sp *= 0.9;	// bold italic
		break;
	case 0x15:
		sp *= 0.7;	// black box
		break;
	case 0x16:
		sp *= 0.7;	// clearance regular
		break;
	case 0x17:	// clearance bold
		sp *= 0.7;
		break;
	case 0x18:	// victor airways on sid/star (sucks)
		sp *= 0.3;
		break;
	case 0x19:	// times
		sp *= 0.9;
		break;
	case 0x1c:	// jeppesen
		sp *= 1.2;
		break;
	case 0x1d:	// morse code
		sp *= 0.4;
		break;
	case 0x1f:	// footnote - black circle
	case 0x20:	// footnote - black square (use black circle)
		sp *= 1.0;
		f = 0x1f;
		break;
	case 0x1a:	// lake name
	case 0x1e:	// gok turd in /n/jep1/charts/kdtw109.tcl
	case 0x00:	// gok turd in /n/jep2/charts/lsgs111.tcl
		sp *= 1.0;
		f = 0x00;
		break;
	}
	if(sp <= 0)
		sp = 10;

	if(f ==  lastfont && s == lastsize)
		return;

	lastfont = f;
	lastsize = s;
	lastpoint = sp;

	Bprint(b, "/F%.2x findfont %.1f scalefont setfont %% %d\n", f, sp, l->ord);
}

int
setadj(Node *l)
{
	int a;
	static char str[20];

	a = getp(l, Pjux, 2, 0);
	switch(a) {
	case 0x18:
	case 0x98:	// gok /n/jep1/charts/kamw161.tcl
		return 0;
	case 0x1e:
		return 1;
	case 0x1a:
		return 2;
	}
	if(qflag || msgcount[7] == 0) {
		print("%s %d: unknown jux %.2x (%s)\n", file, l->ord, a, l->s.str);
		msgcount[7]++;
	}
	return 0;
}

void
psnode(uchar*)
{
	Node *n;
	int i, x, y, p[10];
	Biobuf *b;
	char *ss;

	b = Bopen("/tmp/jepout.ps", OWRITE);
	if(b == 0) {
		print("cant open jepout.ps\n");
		return;
	}

	for(i=0; psdat[i]; i++) {
		if(psdat[i][0] == '@')
		switch(psdat[i][1]) {
		default:
			print("bad escape\n");
			continue;
		}
		Bprint(b, "%s\n", psdat[i]);
	}

	if(h.dx > h.dy) {
		// profile
		Bprint(b, "%.2f %.2f translate\n",
			(8.5*72+h.dy*Scale)/2, (11*72-h.dx*Scale)/2);
		Bprint(b, "90 rotate\n");
	} else
		Bprint(b, "%.2f %.2f translate\n",
			(8.5*72-h.dx*Scale)/2, (11*72-h.dy*Scale)/2);
	Bprint(b, "%.4f dup scale\n", Scale);
	Bprint(b, "/Helvetica findfont 7 scalefont setfont\n");

	for(n=root; n; n = n->link) {
		curnode = n;

		switch(n->type) {
		default:
			print("%s %d ps type(%d)\n", file, n->ord, n->type);
			break;

		case Tgroup2:
		case Tgroup3:
			break;

		case Tgroup1:
			setcolor(b, n, Black);
			setfont(b, n, 0x1c, (-60) & 0xfffff);
			x = xcos((h.minx+h.maxx)/2);
			y = ycos(h.maxy+80);
			Bprint(b, "(%s) %d %d %d %s %% %d\n",
				h.nam1, 0,
				x, y,
				"ctxt", n->ord);
			break;

		case Tstring:
			p[0] = getp(n, Pcol, 3, Black);
			p[1] = getp(n, Pfnt, 2, -1);
			p[2] = getp(n, Psiz, 2, -1);
			p[3] = setadj(n);
			p[4] = ignore(n);
			pmoper(n, p, 5);

			if(p[4])
				break;
			setcolor(b, n, p[0]);
			setfont(b, n, p[1], p[2]);

			x = xco(n->s.x);
			y = yco(n->s.y);
			switch(lastfont) {
			default:
				ss = snorm(n->s.str);
				break;
			case 0x15: // black box
				y += lastpoint * .2;
				ss = sbbox(n->s.str);
				break;
			case 0x1d: // morse
				y -= lastpoint * .2;
				ss = smorse(n->s.str);
				break;
			case 0x1f: // footnote
			case 0x20: // footnote
				ss = sfoot(n->s.str);
				break;
			}
			Bprint(b, "(%s) %d %d %d %s %% %d\n",
				ss, getp(n, Prot, 2, 0),
				x, y,
				adj[p[3]], n->ord);
			break;

		case Tpoint:
			p[0] = getcolor(n);
			p[1] = getwidth(n);
			p[2] = ignore(n);
			pmoper(n, p, 3);

			if(p[2])
				break;

			setcolor(b, n, p[0]);
			setline(b, n, p[1], 0x80);
			Bprint(b, "%d %d moveto %% %d\n",
				xco(n->l.vec[0].x), yco(n->l.vec[0].y), n->ord);
			Bprint(b, "%d %d lineto %% %d\n",
				xco(n->l.vec[0].x), yco(n->l.vec[0].y), n->ord);
			Bprint(b, "stroke %% %d\n", n->ord);
			break;

		case Tvect1:
		case Tvect2:
		case Tvect3:
			p[0] = getcolor(n);
			p[1] = getwidth(n);
			p[2] = getdash(n);
			p[3] = getvect(n);
			p[4] = ignore(n);
			pmoper(n, p, 5);

			if(p[4])
				break;
			if(p[3] == Fpattern) {
				p[3] = Fexterior;
				p[0] = Grey;
			}
			if(macro(b, n, p))
				break;

			setcolor(b, n, p[0]);
			setline(b, n, p[1], p[2]);
			Bprint(b, "%d %d moveto %% %d\n",
				xco(n->v.vec[0].x), yco(n->v.vec[0].y), n->ord);
			for(i=1; i<n->v.count; i++)
				Bprint(b, "%d %d lineto %% %d\n",
					xco(n->v.vec[i].x), yco(n->v.vec[i].y),
					n->ord);
			if(n->type == Tvect2)
				Bprint(b, "closepath %% %d\n", n->ord);
			if(p[3] == Foutline)
				Bprint(b, "stroke %% %d\n", n->ord);
			if(p[3] == Fexterior)
				Bprint(b, "eofill %% %d\n", n->ord);
			break;

		case Tline1:
		case Tline2:
		case Tline3:
		case Tline4:	// prob not a line
			p[0] = getcolor(n);
			p[1] = getwidth(n);
			p[2] = getdash(n);
			p[3] = Foutline;
			p[4] = ignore(n);
			pmoper(n, p, 5);

			if(p[4])
				break;

			if(macro(b, n, p))
				break;

			setcolor(b, n, p[0]);
			setline(b, n, p[1], p[2]);
			Bprint(b, "%d %d moveto %% %d\n",
				xco(n->l.vec[0].x), yco(n->l.vec[0].y), n->ord);
			Bprint(b, "%d %d lineto %% %d\n",
				xco(n->l.vec[1].x), yco(n->l.vec[1].y), n->ord);
			Bprint(b, "stroke %% %d\n", n->ord);
			break;

		case Tcirc1:
		case Tcirc2:
		case Tcirc3:
			p[0] = getcolor(n);
			p[1] = getwidth(n);
			p[2] = getdash(n);
			p[3] = getvect(n);
			p[4] = ignore(n);
			pmoper(n, p, 5);

			if(p[4])
				break;

			setcolor(b, n, p[0]);
			setline(b, n, p[1], p[2]);
			Bprint(b, "%d %d %d 0 360 arc %% %d\n",
				xcos(n->c.x), ycos(n->c.y), n->c.r, n->ord);
			if(p[3] == Foutline)
				Bprint(b, "stroke %% %d\n", n->ord);
			if(p[3] == Fexterior)
				Bprint(b, "eofill %% %d\n", n->ord);
			break;

		case Tarc:
			p[0] = getcolor(n);
			p[1] = getwidth(n);
			p[2] = getdash(n);
			p[3] = ignore(n);
			pmoper(n, p, 4);

			if(p[3])
				break;

			setcolor(b, n, p[0]);
			setline(b, n, p[1], p[2]);
			Bprint(b, "%d %d %d %d %d arc %% %d\n",
				xcos(n->c.x), ycos(n->c.y), n->c.r,
				n->c.a1, n->c.a2, n->ord);
			Bprint(b, "stroke %% %d\n", n->ord);
			break;
		}
	}
	Bprint(b, "showpage\n");
	Bterm(b);
}

void
esc(char *p, int v)
{
	p[0] = '\\';
	p[1] = ((v>>6) & 7) + '0';
	p[2] = ((v>>3) & 7) + '0';
	p[3] = ((v>>0) & 7) + '0';
}

char*
snorm(char *s)
{
	char *p;
	int c;

	p = conbuf;
	for(;;) {
		switch(c = *s++&0xff) {
		default:
			*p++ = c;
			continue;
		case 0:
			goto out;
		case '~':	esc(p, 0330); break;	// zero with slash
		case '^':	esc(p, 0260); break;	// degree
		case '\'':	esc(p, 0222); break;	// feet
		case '|':	esc(p, 0251); break;	// copyright
		case ')':	esc(p, 0051); break;	// )
		case '(':	esc(p, 0050); break;	// (
		}
		p += 4;
	}
out:
	*p = 0;
	return conbuf;
}

char*
smorse(char *s)
{
	char *p, *q;
	int c;

	p = conbuf;
	for(;;) {
		switch(c = *s++&0xff) {
		default:
			if(c >= 'a' && c <= 'z')
				c += 'A'-'a';
			if(q = morse[c]) {
				while(c = *q++) {
					if(c == '.')
						esc(p, 0267);	// centered period
					else
						esc(p, 0255);	// hyphen
					p += 4;
				}
				if(*s == 0)
					continue;
				c = ' ';
			}
			*p++ = c;
			continue;
		case 0:
			goto out;
		}
//		p += 4;			/* not reachable */
	}
out:
	*p = 0;
	return conbuf;
}

char*
sfoot(char *s)
{
	char *p;
	int c;

	p = conbuf;
	for(;;) {
		switch(c = *s++&0xff) {
		default:
			*p++ = c;
			continue;
		case 0:
			goto out;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			esc(p, c+(0xca-'1'));	// numeral in dark circle
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
			esc(p, c+(0xca-'A'));	// numeral in dark circle
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
			esc(p, c+(0xca-'a'));	// numeral in dark circle
			break;
		}
		p += 4;
	}
out:
	*p = 0;
	return conbuf;
}

char*
sbbox(char *s)
{
	char *p;
	int c;

	p = conbuf;
	for(;;) {
		switch(c = *s++&0xff) {
		default:
			if(c >= 'a' && c <= 'z')
				c += 'A'-'a';
			*p++ = c;
			continue;
		case 0:
			goto out;
		case '.':
			*p++ = ' ';
			continue;
		}
//		p += 4;		/*  not reachable */	
	}
out:
	*p = 0;
	return conbuf;
}
