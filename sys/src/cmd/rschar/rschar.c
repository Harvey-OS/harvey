#include <u.h>
#include <libc.h>
#include <libg.h>
#include "rsclass.h"

typedef struct Panel	Panel;
typedef struct Rset	Rset;

struct Rset
{
	ushort	index;
	ushort	size;
	ushort	strokes[1];
};

struct Panel
{
	int	n;
	Rune	r[1];	
};

extern int	nrsets;
extern Rset *	rsets[];
extern uchar	rsclass[];

int	bdown(Mouse*);
void	bsline(void);
void	clearline(void);
void	incrline(Rune);
void	makespanels(void);
char*	menu3gen(int);
void	putline(void);
void	showline(void);
void	showradical(void);
void	showrpanel(void);
void	showspanel(void);
void	showstroke(void);

Menu menu3 = { 0, menu3gen, 0 };

#define	CMARG	1

int	debug;
int	jflag;
char *	fname;
int	ufd;
int	spacewidth;
int	dxy;
int	base;
int	radical;
int	stroke;
Point	porg;
Point	prad;
Point	pstr;
Panel **panels;
int	npanels;
int	ipanel;
Rectangle sleft;
Rectangle sright;
Point	pline;
char *	sname = "/dev/snarf";
int	sfd;
char	lbuf[256];

void
usage(void)
{
	fprint(2, "usage: %s [-bgj] [font]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	Panel *sp;
	Event e;
	Point p;
	int b, k, s;

	ARGBEGIN{
	case 'b':
		jflag |= BIG5;
		break;
	case 'g':
		jflag |= GB2312;
		break;
	case 'j':
		jflag |= JIS208;
		break;
	case 's':
		sname = ARGF();
		break;
	case 'D':
		++debug;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc > 1)
		usage();
	if(argc > 0)
		fname = argv[0];

	sfd = open(sname, OREAD);
	if(sfd >= 0){
		b = read(sfd, lbuf, sizeof lbuf-1);
		close(sfd);
		if(b >= 0)
			lbuf[b] = 0;
	}
	binit(0, fname, argv0);
	dxy = font->height + 2*CMARG;
	spacewidth = strwidth(font, "0");
	radical = -1;
	stroke = -1;
	ereshaped(bscreenrect(0));
	einit(Emouse|Ekeyboard);
	for(;;)switch(event(&e)){
	case Emouse:
		if(e.mouse.buttons&4){
			k = jflag;
			s = menuhit(3, &e.mouse, &menu3);
			switch(s){
			case -1:
				break;
			case 0:
				if(jflag != 0)
					jflag = 0;
				break;
			default:
				b = 1<<(s-1);
				if(jflag != b)
					jflag ^= b;
				break;
			}
			if(jflag != k && radical >= 0){
				ipanel = 0;
				showradical();
			}
			break;
			
		}
		b = bdown(&e.mouse);
		if(b == 0)
			break;
		p = div(sub(e.mouse.xy, porg), dxy);
		if(e.mouse.xy.x >= porg.x && p.x < 14 &&
		   e.mouse.xy.y >= porg.y && p.y < 17){
			k = 17*p.x + p.y;
			if(k >= nrsets)
				break;
			radical = k;
			stroke = -1;
			ipanel = 0;
			showradical();
			break;
		}
		p = div(sub(e.mouse.xy, pstr), dxy);
		if(e.mouse.xy.x >= pstr.x &&
		   e.mouse.xy.y >= pstr.y && p.y < 17){
			if(radical < 0 || radical >= nrsets)
				break;
			if(ipanel < 0 || ipanel >= npanels)
				break;
			sp = panels[ipanel];
			k = 17*p.x + p.y;
			if(k > sp->n || sp->r[k] < RMIN)
				break;
			stroke = sp->r[k];
			showstroke();
			if(b == 2)
				incrline(stroke);
			break;
		}
		if(ipanel > 0 && ptinrect(e.mouse.xy, sleft)){
			--ipanel;
			showspanel();
			break;
		}
		if(ipanel < npanels-1 && ptinrect(e.mouse.xy, sright)){
			++ipanel;
			showspanel();
			break;
		}
		break;
	case Ekeyboard:
		switch(e.kbdc){
		case 0x04:		/* ctl-D */
			exits(0);
			break;
		case 0x08:		/* bs */
			bsline();
			break;
		case 0x15:		/* ctl-U */
			clearline();
			break;
		default:
			incrline(e.kbdc);
			break;
		}
		break;
	}
}

char *
menu3gen(int k)
{
	static char *entries[] = {
		0, "Big Five", "GB 2312-80", "JIS X 0208-1990"
	};
	char buf[64];
	int bit;

	if(k < 0 || k >= nelem(entries))
		return 0;
	if(k == 0){
		if(jflag == 0)
			return "[all char's shown]";
		return "show all char's";
	}
	if(jflag == 0){
		sprint(buf, "select %s", entries[k]);
	}else{
		bit = 1<<(k-1);
		if(jflag == bit)
			sprint(buf, "[%s selected]", entries[k]);
		else if(jflag & bit)
			sprint(buf, "hide %s (shown)", entries[k]);
		else
			sprint(buf, "show %s", entries[k]);
	}
	return buf;
}

void
ereshaped(Rectangle r)
{
	screen.r = r;
	r = inset(r, 5);
	bitblt(&screen, r.min, &screen, r, Zero);
	r.min.x += 16;
	clipr(&screen, r);
	showrpanel();
	if(radical >= 0)
		showradical();
	if(stroke >= 0)
		showstroke();
	showline();
}

void
showrpanel(void)
{
	Point org, p;
	char buf[16];
	int i, j, n, r;

	prad = screen.clipr.min;
	org = add(prad, Pt(2*spacewidth, (3*dxy)/2));
	r = 0;
	for(i=0; i<14; i++){
		p = add(org, mul(Pt(i, 0), dxy));
		for(j=0; j<17; j++, r++, p.y+=dxy){
			if(r >= nrsets)
				break;
			n = runetochar(buf, &rsets[r]->strokes[0]);
			buf[n] = 0;
			string(&screen, p, font, buf, S);
			if(i == 0 && j == 0)
				porg = p;
		}
	}
	pline.x = screen.clipr.min.x;
	pline.y = org.y + 18*dxy;
	pstr.x = org.x + 15*dxy;
	pstr.y = org.y;
}

void
showspanel(void)
{
	Panel *sp;
	Point p;
	char buf[16];
	int i, j, k, n;
	Rune c;

	if(ipanel < 0 || ipanel >= npanels)
		return;
	sp = panels[ipanel];
	bitblt(&screen, pstr, &screen, Rpt(pstr, screen.clipr.max), Zero);
	for(k=i=0; k<sp->n; i++){
		p = add(pstr, mul(Pt(i, 0), dxy));
		for(j=0; k<sp->n && j<17; j++, p.y+=dxy){
			c = sp->r[k++];
			if(c == 0)
				continue;
			if(c < RMIN){
				n = sprint(buf, "%d", c);
				string(&screen, Pt(p.x+(2-n)*spacewidth/2, p.y),
					font, buf, S);
			}else{
				
				n = runetochar(buf, &c);
				buf[n] = 0;
				string(&screen, p, font, buf, S);
			}
		}
	}
	sleft.min = add(pstr, Pt(4*dxy, -(3*dxy)/2));
	if(ipanel > 0){
		sleft.max = string(&screen, sleft.min, font, "⇐", S);
		sleft.max.y += font->height;
	}else{
		sleft.max = add(sleft.min, Pt(dxy, dxy));
		bitblt(&screen, sleft.min, &screen, sleft, Zero);
	}
	sright.min = add(sleft.min, Pt(2*dxy, 0));
	if(ipanel < npanels-1){
		sright.max = string(&screen, sright.min, font, "⇒", S);
		sright.max.y += font->height;
	}else{
		sright.max = add(sright.min, Pt(dxy, dxy));
		bitblt(&screen, sright.min, &screen, sright, Zero);
	}
}

void
makespanels(void)
{
	Rset *rp;
	ushort *up, *us, *uend;
	Panel *sp;
	int size, i, j, k, sc;

	for(i=0; i<npanels; i++)
		free(panels[i]);
	npanels = 1;
	panels = realloc(panels, sizeof(Panel*));
	size = 17*((screen.clipr.max.x - pstr.x)/dxy);
	sp = calloc(1, sizeof(Panel)+(size-1)*sizeof(Rune));
	panels[0] = sp;
	rp = rsets[radical];
	up = rp->strokes;
	uend = up + rp->size;
	us = 0;
	i = 0;
	sc = -1;
	while(up < uend){
		if(i >= size){
			k = sp->n;
			if(k > 0)
				up = us;
			else
				sp->n = size;
			panels = realloc(panels, (npanels+1)*sizeof(Panel*));
			sp = calloc(1, sizeof(Panel)+(size-1)*sizeof(Rune));
			panels[npanels++] = sp;
			i = 0;
			if(k == 0 && *up >= RMIN)
				sp->r[i++] = sc;
		}
		if(*up < RMIN){
			sc = *up;
			sp->n = i;
			us = up;
			j = i%17;
			if(j >= 15)
				i += 17-j;
			else if(j != 0)
				i++;
			if(i < size)
				sp->r[i++] = *up++;
			continue;
		}
		if(!jflag || (rsclass[*up-RMIN] & jflag))
			sp->r[i++] = *up;
		++up;
	}
	sp->n = i;
}

void
showradical(void)
{
	Rectangle r;
	Point p;
	char key[32];
	Rset *rp;

	r.min = prad;
	r.max.x = screen.clipr.max.x;
	r.max.y = prad.y+font->height;
	bitblt(&screen, prad, &screen, r, Zero);
	rp = rsets[radical];
	sprint(key, "%3d %.4ux %C", rp->index, rp->strokes[0], rp->strokes[0]);
	p = string(&screen, prad, font, key, S);
	makespanels();
	showspanel();
}

void
showstroke(void)
{
	char key[32];

	sprint(key, "%.4ux %C ", stroke, stroke);
	string(&screen, sub(pstr, Pt(spacewidth, (3*dxy)/2)), font, key, S);
}

void
showline(void)
{
	Rectangle r;

	r.min = pline;
	r.max.x = screen.clipr.max.x;
	r.max.y = pline.y+font->height;
	bitblt(&screen, pline, &screen, r, Zero);
	if(lbuf[0])
		string(&screen, pline, font, lbuf, S);
}

void
putline(void)
{
	int n;

	sfd = create(sname, OWRITE, 0666);
	if(sfd < 0)
		return;
	n = strlen(lbuf);
	if(n > 0)
		write(sfd, lbuf, n);
	close(sfd);
	showline();
}

void
clearline(void)
{
	lbuf[0] = 0;
	putline();
}

void
bsline(void)
{
	int c, n;

	n = strlen(lbuf);
	while(n > 0){
		c = *(uchar *)&lbuf[--n];
		lbuf[n] = 0;
		if((c & 0xc0) != 0x80)
			break;
	}
	putline();
}

void
incrline(Rune r)
{
	int n;

	if(r == 0)
		return;
	n = strlen(lbuf);
	if(n+UTFmax+1 >= sizeof lbuf)
		return;
	n += runetochar(lbuf+n, &r);
	lbuf[n] = 0;
	putline();
}

int
bdown(Mouse *m)
{
	static int ob;
	int b = 0;

	if(m->buttons == ob)
		return 0;
	if((m->buttons&1) && !(ob&1))
		b |= 1;
	if((m->buttons&2) && !(ob&2))
		b |= 2;
	if((m->buttons&4) && !(ob&4))
		b |= 4;
	ob = m->buttons;
	switch(b){
	case 1:	return 1;
	case 2:	return 2;
	case 4:	return 3;
	}
	return 0;
}
