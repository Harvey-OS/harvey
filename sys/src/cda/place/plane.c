#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

extern Bitmap screen;
extern Font *defont;

void
planecolours(void)
{
	register i, j = 3;

	for(i = 0; i < 6; i++)
		if(b.planemap[i])
			b.planemap[i] = j++;
}

void
drawcolourmap(void)
{
	register i, j;
	Point p, w, s;
	char buf[20];

	for(i = j = 0; i < 6; i++)
		if(b.planemap[i])
			j++;
	if(j == 0)
		return;
	w.x = strwidth(defont, "signo x")+32;
	w.x = ((w.x+99)/100)*100;
	w.y = defont->height+16;
	w.y = ((w.y+31)/32)*32;
	p.x = scrn.panel.corner.x - w.x;
	p.y = scrn.panel.origin.y;
	for(i = 0; i < 6; i++)
		if(b.planemap[i]){
			wrbitmap(colour, 0, 16, &colourbits[b.planemap[i]*32]);
			texture(&screen, raddp(Rpt(Pt(0, 0), w), p), colour, F_XOR);
			sprint(buf, "signo %d", i);
			s = add(p, div(w, 2));
			s.x -= strwidth(defont, buf)/2;
			s.y -= defont->height/2;
			string(&screen, s, defont, buf, F_STORE);
			p.y += w.y;
			if(p.y+w.y > scrn.panel.corner.y){
				p.y = scrn.panel.origin.y;
				p.x -= w.x;
			}
		}
}

static void
plall(NMitem *m)
{
	register i;

	for(i = 0; i < 6; i++)
		if(b.planemap[i] > 0)
			b.planemap[i] = -b.planemap[i];
	m->data = (long)draw;
}

static void
plnone(NMitem *m)
{
	register i;

	for(i = 0; i < 6; i++)
		if(b.planemap[i] < 0)
			b.planemap[i] = -b.planemap[i];
	m->data = (long)draw;
}

static int plwho;

void
plredraw(void)
{
	if(b.planemap[plwho] < 0){
		drawplane(plwho);
		b.planemap[plwho] = -b.planemap[plwho];
	} else {
		b.planemap[plwho] = -b.planemap[plwho];
		drawplane(plwho);
	}
}

static void
plone(NMitem *m)
{
	plwho = m->data;
	m->data = (long) 15;
}

NMitem *
mplanefn(int i)
{
	register n;
	static NMitem m;
	static char buf[64], nam[64];

	m.next = 0;
	m.hfn = 0;
	m.dfn = 0;
	m.bfn = 0;
	m.text = nam;
	m.help = 0;
	if(b.nplanes == 0){
		if(i)
			m.text = 0;
		else {
			strcpy(nam, "no planes");
		}
		return(&m);
	}
	m.help = buf;
	switch(i -= 1)
	{
	case -1:
		strcpy(nam, "all");
		strcpy(buf, "show all planes XORed");
		m.hfn = plall;
		break;
	case 0:
		strcpy(nam, "none");
		strcpy(buf, "undraw all planes");
		m.hfn = plnone;
		break;
	default:
		for(n = 0; n < 6; n++)
			if(b.planemap[n])
				if(--i == 0)
					break;
		if(i == 0){
			sprint(nam, "%csig %d\240", b.planemap[n]<0? '*':' ', n);
			sprint(buf, "%s show special signal %d", b.planemap[n]<0? "don't":"do", n);
			m.hfn = plone;
			m.data = n;
		} else
			m.text = 0;
		break;
	}
	return(&m);
}
