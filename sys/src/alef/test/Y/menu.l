#include <alef.h>
#include <fcall.h>
#include <graphics.h>
#include <frame.h>
#include "y.h"

intern char	**lastm;
intern int	lsel;

void
menuserver(chan(Mesg) r)
{
	int n;
	Mesg m;

	for(;;) {
		m = <-r;

		n = domenu(m.m, m.menu);
		if(n < 0)
			continue;

		lastm = m.menu;
		lsel = n;

		m.sel = n+1;
		m.rchan <-= m;
	}
}

int
domenu(Point ps, char **menu)
{
	Mesg m;
	Point p;
	Window *w;
	Rectangle nh, h, r, box;
	int i, n, x, y, sel, fnd;

	x = 0;
	y = 0;
	for(i = 0; menu[i]; i++) {
		p = strsize(font, menu[i]);
		if(p.x > x)
			x = p.x;
		if(p.y > y)
			y = p.y;
	}
	p = strsize(font, " ");
	if(x < p.x*6)
		x = p.x*6;

	x += 10;
	y += 4;
	m.m = ps;
	ps.x -= x/2;
	if(lastm == menu)
		ps.y -= (lsel*y) + y/2;
	else
		ps.y -= y/2;
	r.min = ps;
	r.max = add(r.min, Pt(x, y*i+4));
	box.min = r.min;
	box.max = add(r.max, Pt(4, 4));

	w = malloc(sizeof(Window));
	if(w == nil)
		return -1;
	w->cache = balloc(box, 0);
	if(w->cache == nil)
		return -1;

	w->win = box;
	w->top = 1;
	pushw(w);


	bitblt(w->cache, box.min, &screen, box, S);
	bitblt(w->cache, add(r.min, Pt(4, 4)), &screen, r, F);
	bitblt(w->cache, r.min, &screen, r, Zero);
	border(w->cache, r, 1, F);
	p = add(r.min, Pt(5, 2));

	for(n = 0; n < i; n++) {
		string(w->cache, add(p, Pt(1, 1)), font, menu[n], S);
		p.y += y;
	}

	p = add(r.min, Pt(3, 2));

	m.type = MsgMouse;
	m.mb = 1;
	sel = -2;
	for(;;) {
		switch(m.type) {
		case MsgRefresh:
			clntref(w, &m);
			break;

		case MsgMouse:
			if(m.mb == 0)
				break 2;

			fnd = 0;
			nh.min = p;
			nh.max = Pt(r.max.x-3, p.y+y);
			for(n = 0; n < i; n++) {
				if(ptinrect(m, nh)) {
					if(sel >= 0)
						border(w->cache, h, 2, Zero);
					border(w->cache, nh, 2, F);
					h = nh;
					sel = n;
					fnd = 1;
					break;
				}
				nh.min.y += y;
				nh.max.y += y;
			}
			if(fnd == 0 && sel != -2) {
				border(w->cache, h, 2, Zero);
				sel = -1;
			}
			windref(w);
			break;

		default:
			w->out <-= m;
			break;
		}
		m = <-w->in;
	}
	popw(w);
	return sel;
}
