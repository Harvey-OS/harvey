#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

extern Bitmap screen;
extern Bitmap *grey;;
static Chip *toinsert;
static Point icorr, lpmin;

static Point
snappy(Point p)
{
	return(pbtos(sub(lpmin = snap(add(icorr, pstob(p))), icorr)));
}

static void
chd(Rectangle r)
{
	Rectangle sr;
	Point p;

	sr = rbtos(toinsert->br);
	p = sub(r.origin, sr.origin);
	rectf(&screen, raddp(rbtos(Rect(-50, -50, 50, 50)), p), F_XOR);
	rectf(&screen, r, F_XOR);
	track();
}

void
insert(void)
{
	Rectangle r, rr;
	Point p;

	while(1) {
		mouse = emouse();
		if(mouse.buttons&7) break;
	}
	if((mouse.buttons&4) == 0){
		while(1) {
		 	mouse = emouse();
			if((mouse.buttons&7) == 0) break;
		}
		toinsert = 0;
		return;
	}
	r = toinsert->br;
	icorr = Pt(0,0);
	p = pbtos(snap(pstob(mouse.xy)));
	r = rbtos(raddp(r, pstob(p)));
	cursorswitch(&blank);
	rr = pan_it(3, r, scrn.sr, snappy, snappy, chd);
	cursorswitch(0);
	p = snap(pstob(rr.origin));
	put1(CHINSERT);
	putn(toinsert->id);
	putn(scrn.selws);
	putp(lpmin);
	while(rcv());
}

char insname[100];
#define MATCH 1
#define STAR 2
int
match(char * template, char * chip)
{
	char *p, *q, *save;
	int result;
	for(p = template, q = chip; (*q && *p); ++q, ++p) {
		save = p;
		result = 0;
		switch(*p) {
			case '.':
				result = MATCH;
				if(*(p+1) == '*') {
					result |= STAR;
					p += 1;
				} 
				break;
			case '[' :
				result = sqbrack(&p, *q);
				break;
			default:
				if(*p == *q) {
					result = MATCH;
					if(*(p+1) == '*') {
						result |= STAR;
						p += 1;
					}
				} 
		}
		if(result == 0) return(0);
		if(result == MATCH) continue;
		if(result == STAR) --q;
		if(result == (STAR|MATCH))
			if(match(save, q+1)) return(MATCH);
	}
	if(*q || *p) return(0);
	return(MATCH);
}

int
sqbrack(char **pp, int c)
{
	char *p, comp, star;
	int end, i;
	p = *pp;
	comp = (p[1]== '^') ? MATCH : 0;
	if(comp) ++p;
	for(end = 0 ; ; ++end) {
		if(p[end] == 0) return(0);
		if(p[end] == ']') break;
	}
	star = (p[end+1] == '*') ? STAR : 0;
	*pp += (end + comp + (star ? 1 : 0));
	if(p[2] == '-') {
		if(end != 4) return(0);
		if((p[1] > c) || (p[3] < c)) return(star|comp);
		*pp += 4;
		return((star|MATCH)^comp);
	}
	for(i = 1; i < end; ++i) {
		if(p[i] == c) {
			*pp += end;
			return((star|MATCH)^comp);
		}
	}
	return(star|comp);
}

extern 	short chsig[];
extern int errfd;


void
nametype(void)
{
	register Chip *c, *lastc;
	char buf[100];
	lastc = (Chip *) 0;
	insname[0] = 0;
	getline("type name: ", insname);
	if((insname[0] == 0) || (b.chips == 0)) return;
	for(c = b.chips; !(c->flags&EOLIST); c++) {
		if((c->flags&(PLACED|WSIDE)) != 
			(scrn.selws ? (PLACED|WSIDE) : PLACED)) continue;
		if(match(insname, c->type+b.chipstr)) {
			sprint(buf, "select: %s", c->type+b.chipstr);
			bepatient(buf);
			select(c);
			lastc = c;
			bedamned();
		}
	}
	if(lastc)
		if(!rinr(lastc->br, scrn.br))
			panto(confine(sub(lastc->br.origin, Pt(1000, 1000)), scrn.bmax));
}

void
namesig(void)
{
	register Signal *ss;
	int n;
	char buf[100];
	n = 0;
	insname[0] = 0;
	getline("sig name: ", insname);
	if((insname[0] == 0) || (b.sigs == 0)) return;
	for(ss = b.sigs; ss->name; ss++)
		if(match(insname, ss->name+b.chipstr)) {
fprint(errfd, "matched: %s %s\n", insname, ss->name+b.chipstr);
			sprint(buf, "select: %s", ss->name+b.chipstr);
			bepatient(buf);
			chsig[n++] = ss->id;
			bedamned();
		}
	chsig[n] = -1;
	if(n) getsigs(chsig);
}


void
namechip(void)
{
	register Chip *c, *lastc;
	char buf[100];
	lastc = (Chip *) 0;
	insname[0] = 0;
	getline("chip name: ", insname);
	if((insname[0] == 0) || (b.chips == 0)) return;
	for(c = b.chips; !(c->flags&EOLIST); c++) {
		if((c->flags&(PLACED|WSIDE)) != 
			(scrn.selws ? (PLACED|WSIDE) : PLACED)) continue;
		if(match(insname, c->name+b.chipstr)) {
			sprint(buf, "select: %s", c->name+b.chipstr);
			bepatient(buf);
			select(c);
			lastc = c;
			bedamned();
		}
	}
	if(lastc)
		if(!rinr(lastc->br, scrn.br))
			panto(confine(sub(lastc->br.origin, Pt(1000, 1000)), scrn.bmax));
}

void
nameit(void)
{
	register Chip *c;
	char buf[100];

	insname[0] = 0;
	getline("chip name: ", insname);
	if((insname[0] == 0) || (b.chips == 0)) return;
	for(c = b.chips; !(c->flags&EOLIST); c++) {
		if(c->flags&PLACED) continue;
		if(match(insname, c->name+b.chipstr)) {
			toinsert = c;
			sprint(buf, "place: %s", c->name+b.chipstr);
			bepatient(buf);
			insert();
			bedamned();
			if(toinsert == 0) return;
		}
	}
}

static void
setins(NMitem *m)
{
	toinsert = (Chip *)m->data;
	m->data = (long) 8;
}

NMitem *
minsfn(int i)
{
	register Chip *c;
	register j;
	static NMitem m;
	static char buf[64], nam[64];

	if(i == 0) {
		m.data = (long) 13;
		m.next = 0;
		m.hfn = 0;
		m.dfn = 0;
		m.bfn = 0;
		m.text = "nameit";
		m.help = "name from keyboard";
		return(&m);
	}
		
	if(b.chips)
		for(c = b.chips, j = 1; !(c->flags&EOLIST); c++)
			if(!(c->flags&PLACED) && (i == j++))
				goto found;
	c = 0;
found:
	m.data = (long)c;
	m.next = 0;
	m.hfn = setins;
	m.dfn = 0;
	m.bfn = 0;
	if(c)
	{
		sprint(nam, "%s\240", c->name+b.chipstr);	/* this should go away */
		m.text = nam;
		m.help = c->type+b.chipstr;
	} else
		m.text = 0;
	return(&m);
}
