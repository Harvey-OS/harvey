/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <frame.h>

Point
_frptofcharptb(Frame* f, uint32_t p, Point pt, int bn)
{
	uint8_t* s;
	Frbox* b;
	int w, l;
	Rune r;

	for(b = &f->box[bn]; bn < f->nbox; bn++, b++) {
		_frcklinewrap(f, &pt, b);
		if(p < (l = NRUNE(b))) {
			if(b->nrune > 0)
				for(s = b->ptr; p > 0; s += w, p--) {
					if((r = *s) < Runeself)
						w = 1;
					else
						w = chartorune(&r, (char*)s);
					pt.x +=
					    stringnwidth(f->font, (char*)s, 1);
					if(r == 0 || pt.x > f->r.max.x)
						drawerror(f->display,
						          "frptofchar");
				}
			break;
		}
		p -= l;
		_fradvance(f, &pt, b);
	}
	return pt;
}

Point
frptofchar(Frame* f, uint32_t p)
{
	return _frptofcharptb(f, p, f->r.min, 0);
}

Point _frptofcharnb(Frame* f, uint32_t p,
                    int nb) /* doesn't do final _fradvance to next line */
{
	Point pt;
	int nbox;

	nbox = f->nbox;
	f->nbox = nb;
	pt = _frptofcharptb(f, p, f->r.min, 0);
	f->nbox = nbox;
	return pt;
}

static Point
_frgrid(Frame* f, Point p)
{
	p.y -= f->r.min.y;
	p.y -= p.y % f->font->height;
	p.y += f->r.min.y;
	if(p.x > f->r.max.x)
		p.x = f->r.max.x;
	return p;
}

uint32_t
frcharofpt(Frame* f, Point pt)
{
	Point qt;
	int w, bn;
	uint8_t* s;
	Frbox* b;
	uint32_t p;
	Rune r;

	pt = _frgrid(f, pt);
	qt = f->r.min;
	for(b = f->box, bn = 0, p = 0; bn < f->nbox && qt.y < pt.y; bn++, b++) {
		_frcklinewrap(f, &qt, b);
		if(qt.y >= pt.y)
			break;
		_fradvance(f, &qt, b);
		p += NRUNE(b);
	}
	for(; bn < f->nbox && qt.x <= pt.x; bn++, b++) {
		_frcklinewrap(f, &qt, b);
		if(qt.y > pt.y)
			break;
		if(qt.x + b->wid > pt.x) {
			if(b->nrune < 0)
				_fradvance(f, &qt, b);
			else {
				s = b->ptr;
				for(;;) {
					if((r = *s) < Runeself)
						w = 1;
					else
						w = chartorune(&r, (char*)s);
					if(r == 0)
						drawerror(f->display,
						          "end of string in "
						          "frcharofpt");
					qt.x +=
					    stringnwidth(f->font, (char*)s, 1);
					s += w;
					if(qt.x > pt.x)
						break;
					p++;
				}
			}
		} else {
			p += NRUNE(b);
			_fradvance(f, &qt, b);
		}
	}
	return p;
}
