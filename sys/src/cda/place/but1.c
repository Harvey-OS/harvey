#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"

extern Bitmap screen;
extern Bitmap *grey;;
extern char message[];
extern Font * defont;
char * caption;

static Chip *
wchip(Point p)
{
	register Chip *c;

	if(b.chips)
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if(((c->flags&(PLACED|WSIDE))
				== (scrn.selws ? (PLACED|WSIDE) : PLACED))
					&& ptinrect(p, c->sr))
						return(c);
	return((Chip *)0);
}

void
but1(void)
{
	register Chip *c;
	Point p;
	void move(void);
	if(ptinrect(mouse.xy, scrn.map)){
		cursorswitch(&blank);
		pan();
		cursorswitch(0);
		return;
	}
	if(ptinrect(p = mouse.xy, scrn.sr) && (c = wchip(p))) {
		cursorswitch(&blank);
		select(c);
		move();
		cursorswitch(0);
	}
}

void newcaption(Chip *);

void
select(Chip * c)
{
	Crectf(&screen, c->sr, F_XOR);
	c->flags ^= SELECTED;
	newcaption(c);
}

void
newcaption(Chip * c)
{
	char buf[256];
	static char *lastc = "";
	static char *lastt = "";
	if(caption) {
		if(message[0] == 0)
			string(&screen, scrn.bname.min, defont, caption, S^D);
		free(caption);
	}
	strcpy(buf, b.name);
	if(c) {
		if(c->flags & SELECTED) {
			lastc = c->name+b.chipstr;
			lastt = c->type+b.chipstr;
		}
		else {
			for(c = b.chips; !(c->flags&EOLIST); c++)
				if((c->flags&(SELECTED|WSIDE))
					== (scrn.selws ? (SELECTED|WSIDE) : SELECTED))
						break;
			if(!(c->flags&EOLIST)) {
				lastc = c->name+b.chipstr;
				lastt = c->type+b.chipstr;
			}
			else {
				lastc = "";
				lastt = "";
			}
		}
	}
	strcat(buf, "   ");
	strcat(buf, lastc);
	strcat(buf, "   ");
	strcat(buf, lastt);
	strcat(buf, "   ");
	if(b.ncursig)
		strcat(buf, (b.sigs+b.cursig[b.ncursig - 1])->name+b.chipstr);
	caption = (char *)  malloc(1+strlen(buf));
	strcpy(caption, buf);
	if(message[0] == 0)
		string(&screen, scrn.bname.min, defont, caption, S^D);
}

void
clrselect(void)
{
	register Chip *c;

	if(b.chips)
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if((c->flags&(SELECTED|WSIDE))
				== (scrn.selws ? (SELECTED|WSIDE) : SELECTED))
					select(c);
}

void
rectselect(void)
{
	register Chip *c;
	Rectangle r;
	extern void move(void);

	r = getrect(3, &mouse);
	if(b.chips)
	for(c = b.chips; !(c->flags&EOLIST); c++)
		if(rinr(c->sr, r) &&
			((c->flags&(SELECTED|WSIDE|PLACED))
				== (scrn.selws ? (WSIDE|PLACED)
					: (PLACED))))
						select(c);
}

void
sweepdrag(void)
{
	register Chip *c;
	Rectangle r;
	extern void move(void);

	r = getrect(1, &mouse);
	if(b.chips)
	for(c = b.chips; !(c->flags&EOLIST); c++)
		if(rinr(c->sr, r) &&
			((c->flags&(SELECTED|WSIDE|PLACED))
				== (scrn.selws ? (WSIDE|PLACED)
					: (PLACED))))
						select(c);
	cursorswitch(&blank);
	move();
	cursorswitch(0);
}

void
pinsel(void)
{
	int i;
	Point pp;
	Pin * p;
	Chip * c;
	Signal *ss;	
	for( ;!mouse.buttons; mouse = emouse());
	while(button3()) mouse = emouse();
	if(ptinrect(pp = mouse.xy, scrn.sr) && (c = wchip(pp)) && (c->npins > 0)) {
		pp = pstob(pp);
		pp.x = pp.x - c->br.min.x;
		pp.y =  c->br.max.y - pp.y;
		for(p = c->pins, i = 0; i < c->npins; i++, p++){
			if(pp.x > (25 + p->pos.x)) continue;
			if(pp.x < (p->pos.x - 25)) continue;
			if(pp.y > (25 + p->pos.y)) continue;
			if(pp.y < (p->pos.y - 25)) continue;
			if(p->type >= 0) {
				for(ss = b.sigs; ss->id != p->type; ss++)
					if(ss->name == 0)
						panic("bad signal id");
				b.cursig[b.ncursig++] = ss-b.sigs;
				gdrawsig(ss-b.sigs);
			}
			return;
		}
	}
}

extern Cursor sweep;

void
setcur(NMitem *m)
{
	Chip *c = (Chip *)m->data;
	m->data = (long)draw;
	select(c);
	if(!rinr(c->br, scrn.br))
		panto(confine(sub(c->br.origin, Pt(1000, 1000)), scrn.bmax));
}

NMitem *
mschip(int i)
{
	register Chip *c;
	static NMitem m;
	static char buf[64], nam[64];

	if(b.chips){
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if((c->flags&(PLACED|WSIDE))
				== (scrn.selws ? (PLACED|WSIDE) : PLACED))
					if(i-- == 0)
						break;
		if(c->flags&EOLIST)
			c = 0;
	} else
		c = 0;
	m.data = (long)c;
	m.next = 0;
	m.hfn = setcur;
	m.dfn = 0;
	m.bfn = 0;
	if(c)
	{
		sprint(nam, "%s\240", c->name+b.chipstr);
		m.text = nam;
		m.help = c->type+b.chipstr;
	} else
		m.text = 0;
	return(&m);
}
extern Font * defont;
static Point oldp;
void
track(void)
{
	char buf[32];
	Point p;
	static Point oldp;

	p = pstob(mouse.xy);
	if(eqpt(p, oldp) || !ptinrect(mouse.xy, scrn.sr))
		return;
	sprint(buf, "%d,%d", p.x, p.y);
	rectf(&screen, scrn.coord, F_CLR); 
	string(&screen, scrn.coord.origin, defont, buf, F_XOR);
	oldp = p;
}


