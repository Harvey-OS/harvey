#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include <cda/sim.h>

#ifdef M386
#define SPACE	12
#define HIGH	8
#else
#define SPACE	16
#define HIGH	10
#endif
#define HEIGHT(n,v,d)	screen.r.min.y + SPACE*(n+1) - (d ? HIGH/2 : (HIGH*v))
#define NCELL	1000
Cell *_cell[NCELL],*_disp[NCELL],*_trig[NCELL],*_edge[NCELL];
int n_cell,n_disp,n_trig,n_edge;
int gflag = 1;
int tflag = 1;
int vflag;		/* draw vertical lines */
int pflag;		/* force half-periods */
int picflag;

void
binde(char *s, Cell *c)
{
	char *tail;
	if (tail = strchr(s, '_')) {
		if (tail[1] == '0')
			strcpy(tail, "-");
		else if (tail[1] == '1')
			strcpy(tail, "+");
	}
	c->name = s;
	_cell[n_cell] = c;
	if (++n_cell >= NCELL)
		exits("not enough sim _cells");
}

int
setup(void (*f)(char *,int), char *s, int v)
{
	int i,n,dig;
	char *num,*tail,buf[20],cvs[10];
	if (num = strchr(s,'[')) {
		*num++ = 0;
		tail = strchr(num,']');
		*tail++ = 0;
		n = atoi(num);
		dig = (n > 10) ? 2 : 1;
		for (i = 0; i < n; i++) {
			sprint(cvs,"%%s%%%d.%dd%%s",dig,dig);
			sprint(buf,cvs,s,i,tail);
			(*f)(buf,(v>>i)&1);
		}
		return n;
	}
	else {
		(*f)(s,v);
		return 1;
	}
}

showtext(char *s, Point p)
{
	static int warm;
	if (picflag) {
		if (!warm) {
			print(".PS\n");
			warm = 1;
		}
		print("\"%s\" with .nw at %d,%d\n",s,p.x,-p.y);
	}
	else
		string(&screen,p,font,s,S|D);
}

void
preen(char *s, int junk)
{
	int i;
	Cell *c;
	for (i = 0; i < n_cell; i++)
		if (strcmp(s,_cell[i]->name) == 0) {
			c = _cell[i];
			c->disp = 1;
			_disp[n_disp] = c;
			if (gflag) {
				c->y = HEIGHT(n_disp,c->s,0);
				c->ox = screen.r.min.x + 80;
				showtext(c->name,Pt(screen.r.min.x+40,HEIGHT(n_disp,0,0)-font->ascent));
			}
			n_disp++;
			return;
		}
	print("undefined: %s\n",s);
}

void
initval(char *s,int v)
{
	int i;
	for (i = 0; i < n_cell; i++)
		if (strcmp(s,_cell[i]->name) == 0) {
			_cell[i]->m = _cell[i]->s = v;
			return;
		}
	print("undefined: %s\n",s);
}

void
initvert(char *s, int v)
{
	int i;
	for (i = 0; i < n_cell; i++)
		if (strcmp(s,_cell[i]->name) == 0) {
			_edge[n_edge] = _cell[i];
			_edge[n_edge]->vert = v;
			n_edge++;
			return;
		}
	print("undefined: %s\n",s);
}

int
vert(void)
{
	int i, v = 0;
	Cell *c;
	for (i = 0; i < n_edge; i++) {
		c = _edge[i];
		if (c->prev == (1-c->vert) && c->s == c->vert)
			v |= 1;
		c->prev = c->s;
	}
	return v;
}

void
period(char *s,int p)
{
	int i;
	for (i = 0; i < n_cell; i++)
		if (strcmp(s,_cell[i]->name) == 0) {
			_cell[i]->period = p;
			_cell[i]->drive = 1;
			return;
		}
	print("undefined: %s\n",s);
}

void
_trigval(char *s,int v)
{
	int i;
	Cell *c;
	for (i = 0; i < n_cell; i++) {
		c = _cell[i];
		if (strcmp(s,_cell[i]->name) == 0) {
			_trig[n_trig] = c;
			n_trig++;
			c->trig = v;
			return;
		}
	}
	print("undefined: %s\n",s);
}

int
yet(void)
{
	int i;
	for (i = 0; i < n_trig; i++) {
		if (_trig[i]->s != _trig[i]->trig)
			return 0;
	}
	tflag = 1;
	return 1;
}

int greyval[] = {1, 1, 4, 64};

gline(Point p, Point q, int grey)
{
	if (picflag)
		print("line%s from %d,%d to %d,%d\n",grey ? " dotted 5" : "",p.x,-p.y,q.x,-q.y);
	else
		segment(&screen,p,q,grey ? greyval[screen.ldepth] : 255,S|D);
}

line(Point p, Point q)
{
	gline(p, q, 0);
}

inline(void)
{
	int i,c;
	char *s,*p,buf[1000];
	if ((s = fgets(buf, 1000, stdin)) == 0)
		return 1;
	while (*s && *s != '\n') {
		for (p = s; *p != '=' && *p != ':'; p++)
			;
		*p++ = 0;
		for (i = 0; i < n_cell; i++)
			if (strcmp(s,_cell[i]->name) == 0)
				switch (c = *p++) {
				case '0':
				case '1':
					_cell[i]->m = c - '0';
					_cell[i]->drive = 1;
					break;
				case '-':
					_cell[i]->drive = 0;
					break;
				}
		for (s = p; *s == ' '; s++)
			;
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int i,t,p,n,inited=0;
	int simtime;
	int x,dh;
	Cell *c;
	init();
	for (i = 1; i < argc;)
		if (argv[i][0] == '-')
			switch (argv[i][1]) {
			case 'd':		/* display these variables */
				if (gflag && !inited) {
					binit(0,0,argv[0]);
					screen.r = inset(screen.r,4);
					n = screen.r.max.x - screen.r.min.x;
					if (!picflag)
						bitblt(&screen,screen.r.min,&screen,screen.r,0);
					inited = 1;
				}
				for (i++, dh = 0; i < argc && argv[i][0] != '-'; i++)
					dh += setup(preen,argv[i],0);
				break;
			case 'i':
				for (i++; i < argc && argv[i][0] != '-'; i += 2)
					setup(initval,argv[i],atoi(argv[i+1]));
				break;
			case 'g':
				gflag ^= 1;
				i++;
				break;
			case 'p':
				pflag = 1;	/* we drive ourselves */
				for (i++; i < argc && argv[i][0] != '-'; i += 2)
					period(argv[i],atoi(argv[i+1]));
				break;
			case 'n':
				n = atoi(argv[i+1]);
				i += 2;
				break;
			case 's':	/* uhh, 's' for no screen? */
				picflag ^= 1;
				i++;
				break;
			case 't':
				for (i++; i < argc && argv[i][0] != '-'; i += 2)
					setup(_trigval,argv[i],atoi(argv[i+1]));
				tflag = 0;
				break;
			case 'v':
				vflag ^= 1;
				for (i++; i < argc && argv[i][0] != '-'; i += 2)
					dh += setup(initvert, argv[i], atoi(argv[i+1]));
				break;
			}
		else {
			print("illegal: %s\n",argv[i]);
			i++;
		}
	for (t = 0; n == 0 || simtime < n; t++) {
		simtime++;
		if (pflag) {
			for (i = 0; i < n_cell; i++)
				if ((p = _cell[i]->period) && (t%p) == 0) {
					_cell[i]->m ^= 1;
					/*_cell[i]->s = _cell[i]->m;*/
				}
		}
		else
			if (inline())
				break;
		do; while (settle() && !pflag);	/* settle if stdin */
		if (!tflag && !yet()) {
			simtime--;
			continue;
		}
		if (vflag && gflag && vert()) {
				x = screen.r.min.x + simtime + 80;
				gline(Pt(x,screen.r.min.y), Pt(x,HEIGHT(n_disp+1,0,0)), 1);
		}
		for (i = 0; i < n_disp && (c = _disp[i]); i++)
			if (gflag) {
				c->oy = c->y;
				c->y = HEIGHT(i,c->s,c->dis);
				x = screen.r.min.x + simtime + 80;
				if (c->oy != c->y) {
					line(Pt(c->ox,c->oy),Pt(x,c->oy));
					line(Pt(x,c->oy),Pt(x,c->y));
					c->ox = x;
				}
			}
			else {
				print(_disp[i]->name);
				print("%c",_disp[i]->drive ? '=' : ':');
				if (_disp[i]->dis)
					print("- ");
				else
					print("%d ", _disp[i]->s);
			}
		if (!gflag)
			print("\n");
	}
	if (gflag)
		for (i = 0; i < n_disp && (c = _disp[i]); i++)
			if (x > c->ox)
				line(Pt(c->ox,c->oy),Pt(x,c->y));
	if (picflag)
		print(".PE\n");
	exits(0);
}
