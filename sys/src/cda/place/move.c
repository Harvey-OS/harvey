#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

extern Bitmap *grey;;
extern Bitmap screen;

/*
	only move NMOVE !!
*/
#define		NMOVE		100

static Chip *ch[NMOVE], **nch;
static Point icorr;

static Point
snappy(Point p)
{
	return(sub(snap(add(icorr, pstob(p))), icorr));
}

static
void
chd(Rectangle r)
{
	Point p;
	register Chip **cp;

	p = sub(r.origin, ch[0]->br.origin);
	rectf(&screen, rbtos(raddp(Rect(-50, -50, 50, 50), add(ch[0]->pmin, p))), F_XOR);
	rectf(&screen, rbtos(r), F_XOR);
	for(cp = ch+1; cp < nch; cp++)
		Crectf(&screen, rbtos(raddp((*cp)->br, p)), F_XOR);
	track();
}

int moved;

void
move(void)
{
	Rectangle r;
	Point op;
	register Chip *c;
	extern int errfd;

	if(b.chips == 0)
		return;
	for(c = b.chips, nch = ch; !(c->flags&EOLIST); c++)
		if(((c->flags&(SELECTED|WSIDE))
			== (scrn.selws ? (SELECTED|WSIDE) : SELECTED))
				&& (nch < &ch[NMOVE]))
						*nch++ = c;
	if(nch == ch)
		return;
	cursorswitch(&blank);
	r = ch[0]->br;
	op = r.origin;
	icorr = Pt(0,0);
	icorr = sub(snap(ch[0]->pmin), op);
	r = pan_it(mouse.buttons, r, scrn.br, snappy, pbtos, chd);
	op = sub(op, r.origin);
	cursorswitch((Cursor *) 0);
	put1(CHMOVE);
	while(--nch >= ch) {
		putn((*nch)->id);
		if (moved)
			select(*nch);
	}
	putn(0);
	putp(op);		/* why use this?? */
	while(rcv());
}
