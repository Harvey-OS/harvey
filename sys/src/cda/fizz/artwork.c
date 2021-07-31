#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

void achip(Chip *), schipc(Chip *), schipw(Chip *), pdefn(Chip *), pchip(Chip *),
ptype(Chip *);
void rect(Rectangle, char *);
void xystr(char *, Rectangle, char * , int);

void
main(int argc, char **argv)
{
	Board b;
	int n;
	int aflag = 0;
	int sflag = 0;
	int tflag = 0;
	int cflag = 0;
	extern int optind;

	while((n = getopt(argc, argv, "astc")) != -1)
		switch(n)
		{
		case 'a':	aflag = 1; break;
		case 's':	sflag = 1; break;
		case 't':	tflag = 1; break;
		case 'c':	cflag = 1; break;
		case '?':	break;
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(1, "%s: %d errors\n", *argv, n);
			exit(1);
		}
	fizzplane(&b);
	fizzplace();
	if(fizzprewrap())
		exit(1);
	if(aflag){
		if(b.xydef){
			fprint(1, "%s", b.xydef);
			b.xydef = 0;
		}
		symtraverse(S_CHIP, pdefn);
		fprint(1, "CHIPXY CLUMP\n");
		if(b.xyid)
			fprint(1, " INC %s\n", b.xyid);
		symtraverse(S_CHIP, achip);
		fprint(1, " END CLUMP\n");
	}
	/* WARNING: schipc screws around with the rectangles for a chip!! */
	if(sflag){
		fprint(1, "SILKW CLUMP\n");
		symtraverse(S_CHIP, schipw);
		fprint(1, " END CLUMP\n");
		fprint(1, "SILKC CLUMP\n");
		symtraverse(S_CHIP, schipc);
		fprint(1, " END CLUMP\n");
		fprint(1, "SILK CLUMP\n INC SILKC\n INC SILKW\n");
		fprint(1, " END CLUMP\n");
	}
	if(cflag) {
		fprint(1, "POS CLUMP\n");
		symtraverse(S_CHIP, pchip);
		fprint(1, " END CLUMP\n");
	}
	if(tflag) {
		fprint(1, "POS CLUMP\n");
		symtraverse(S_CHIP, ptype);
		fprint(1, " END CLUMP\n");
	}

	exit(0);
}

void
pdefn(Chip *c)
{
	if(c->type->pkg->xydef){
		fprint(1, "%s", c->type->pkg->xydef);
		c->type->pkg->xydef = 0;
	}
}

void
achip(Chip *c)
{
	Point xyoff;
	if(c->type->pkg->xyid) {
		xyoff = c->type->pkg->xyoff;
		xyrot(&xyoff, c->rotation);
		fprint(1, " INC %s %d,%d,,%d\n", c->type->pkg->xyid, xyoff.x+c->pt.x, xyoff.y+c->pt.y, c->rotation*90);
	}
}

#define		BORDER		(INCH/20)
#define		WIRE		"SLWA"
#define		COMP		"SLCA"
#define		POS		"POSXY"
void
schipw(Chip *c)
{
	Rectangle r;
	register i;
	Point p;

	r.max.x = r.max.y = -1;
	r.min.x = r.min.y = 1000000;
	if(!c->npins && !c->ndrills) return;
#ifdef	not_me_buddy
	this seems a lot of work to get the wrong answer.
	if you wanna do this, check with andrew

	for(i = 0; i < c->npins; i++){
		p = c->pins[i].p;
		if(p.x < r.min.x) r.min.x = p.x;
		if(p.x > r.max.x) r.max.x = p.x;
		if(p.y < r.min.y) r.min.y = p.y;
		if(p.y > r.max.y) r.max.y = p.y;
	}
	for(i = 0; i < c->ndrills; i++){
		p = c->drills[i].p;
		if(p.x < r.min.x) r.min.x = p.x;
		if(p.x > r.max.x) r.max.x = p.x;
		if(p.y < r.min.y) r.min.y = p.y;
		if(p.y > r.max.y) r.max.y = p.y;
	}
	if((r.max.x == -1) || (r.max.y == -1) || (r.min.x == 1000000)
				|| (r.min.y == 1000000)) return;
	r.min.x -= BORDER;
	r.min.y -= BORDER;
	r.max.x += BORDER;
	r.max.y += BORDER;
	c->r = r;
	/* reflect by hand */
#endif
	r = c->r;
	i = -r.min.x;
	r.min.x = -r.max.x;
	r.max.x = i;
	if((c->flags&IGBOX)==0)
		rect(r, WIRE);
	if((c->flags&IGNAME)==0)
		xystr(c->name, r, WIRE, c->rotation);
	if((c->flags&IGPIN1)==0){
		r.min.x = -c->pt.x-BORDER;
		r.min.y = c->pt.y-BORDER;
		r.max.x = -c->pt.x+BORDER;
		r.max.y = c->pt.y+BORDER;
		rect(r, WIRE);
	}
}

void
ptype(Chip *c)
{
	Rectangle r;
	int x;
	Point p;
	r = c->r;
	if(c->flags&WSIDE) {
		/* reflect by hand */
		x = -r.min.x;
		r.min.x = -r.max.x;
		r.max.x = x;
		x = -1;
	}
	else x = 1;
	rect(r, POS);
	xystr(c->type->name, r, POS, c->rotation);
	r.min.x = x*c->pt.x-BORDER;
	r.min.y = c->pt.y-BORDER;
	r.max.x = x*c->pt.x+BORDER;
	r.max.y = c->pt.y+BORDER;
	rect(r, POS);
}


void
pchip(Chip *c)
{
	Rectangle r;
	int x;
	Point p;
	r = c->r;
	if(c->flags&WSIDE) {
		/* reflect by hand */
		x = -r.min.x;
		r.min.x = -r.max.x;
		r.max.x = x;
		x = -1;
	}
	else x = 1;
	rect(r, POS);
	xystr(c->name, r, POS, c->rotation);
	r.min.x = x*c->pt.x-BORDER;
	r.min.y = c->pt.y-BORDER;
	r.max.x = x*c->pt.x+BORDER;
	r.max.y = c->pt.y+BORDER;
	rect(r, POS);
}
void
schipc(Chip *c)
{
	Rectangle r;
	Point p;

	r = c->r;
	if((c->flags&IGBOX)==0)
		rect(r, COMP);
	if((c->flags&IGNAME)==0)
		xystr(c->type->name, r, COMP, c->rotation);
	if((c->flags&IGPIN1)==0){
		r.min.x = c->pt.x-BORDER;
		r.min.y = c->pt.y-BORDER;
		r.max.x = c->pt.x+BORDER;
		r.max.y = c->pt.y+BORDER;
		rect(r, COMP);
	}
}

char trtab[256] =
{
	0,   '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	' ', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', ',', '-', '.', '*',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '*', '*', '*', '*', '*',
	'*', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '*', '*', '*', '*', '-',
	'*', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
	'*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
};

void
xytr(char *from, char *to)
{
	while(*to++ = trtab[*from++]);
}

#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))

#define	H(D)	MIN((10*((r.max.D-r.min.D - 80-2*BORDER)/nch))/8, 80)

void
xystr(char *s, Rectangle r, char *layer, int rot)
{
	int height, l, nch;
	char buf[256];
	/*
		you specify height; width is 8/10*height
		ref point is center of first char
	*/
	xytr(s, buf);
	s = buf;
	nch = strlen(s);
	if(H(x) < H(y)){
		if((rot&1) == 0)
			rot--;
	}else if(H(x) > H(y)){
		if(rot&1)
			rot--;
	}
	rot = (rot+4)%4;/**/
	if(rot&1)
		rot = 3;
	else
		rot = 0;
	if(rot&1){	/* rotate */
		height = H(y)*((rot&2)? -1:1);
		l = (2*height*(nch-1))/5;
		fprint(1, " TEXT %s %d %d,%d,,%d, (%s)\n", layer, abs(height),
			(r.min.x+r.max.x)/2, (r.max.y+r.min.y)/2 - l,
			90*rot, s);
	} else {
		height = H(x)*((rot&2)? -1:1);
		l = (2*height*(nch-1))/5;
		fprint(1, " TEXT %s %d %d,%d,,%d, (%s)\n", layer, abs(height),
			(r.max.x+r.min.x)/2 - l, (r.min.y+r.max.y)/2,
			90*rot, s);
	}
}

void
rect(Rectangle r, char *layer)
{
	fprint(1, " PATH %s,12 %d,%d %d,%d %d,%d\n", layer,
		r.min.x, r.min.y, r.min.x, r.max.y, r.max.x, r.max.y);
	fprint(1, " PATH %s,12 %d,%d %d,%d %d,%d\n", layer,
		r.max.x, r.max.y, r.max.x, r.min.y, r.min.x, r.min.y);
}
