#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

Font *defont;

#define	LONG(x)	((long)(x))

int scmd;

void
sendcs(void)
{
	register Chip *c;
	char buf[20];
	if(b.chips){
		put1(scmd);
		if(scmd == IMPROVEN) {
			buf[0] = 0;
			getline("times: " , buf);
			putn(atoi(buf));
		}
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if((c->flags&(SELECTED|WSIDE))
				== (scrn.selws ? (SELECTED|WSIDE) : SELECTED)){
				if((scmd == IMPROVE1) || (scmd == IMPROVEF) || (scmd == IMPROVEN))
					if((c->flags&PLACE) != SOFT)
						continue;
				putn(c->id);
				if(scmd == CHUNPLACE)
					select(c);
				if(scmd == CHROT) {
					cursorswitch(&blank);
					drawchip(c);
					cursorswitch((Cursor *) 0);
					c->flags &= ~(PLACED|MAPPED);
				}
			}
		putn(0);
		if((scmd == IMPROVE1) || (scmd == IMPROVEF) || (scmd == IMPROVEN))
			bepatient("improving");
		while(rcv());
		if((scmd == IMPROVE1) || (scmd == IMPROVEF) || (scmd == IMPROVEN))
			bedamned();
	}
}

void
chstat(NMitem *m)
{
	scmd = m->data;
}

extern void shchip(void), shovly(void), shfont(void), clrselect(void);
extern void rectselect(void), kpredraw(void);
extern void bye(void), slws(void), shpin(void), zoomin(void), zoomout(void);
extern void poot(void), nameit(void);
void rfile(void), wfile(void), clrsig(void), namechip(void);
extern void move(void), showsig(void), insert(void), do2sig(void), plredraw(void);
extern void clrpin(void),  showpin(void), namesig(void), nametype(void);
extern void pinsel(void);
extern void sweepdrag(void);

void
clrall(void)
{
	clrpin();
	clrselect();
	clrsig();
}

void (*fncs[])(void)  = {
	poot,		/* 0 */
	shovly,		/* 1 */
	shfont,		/* 2 */
	clrselect,	/* 3 */
	rectselect,	/* 4 */
	clrsig,		/* 5 */
	bye,		/* 6 */
	shpin,		/* 7 */
	insert,		/* 8 */
	zoomin,		/* 9 */
	zoomout,	/* 10 */
	shchip,		/* 11 */
	do2sig,		/* 12 */
	nameit,		/* 13 */
	namechip,	/* 14 */
	plredraw,	/* 15 */
	kpredraw,	/* 16 */
	slws,		/* 17 */
	namesig,	/* 18 */
	nametype,	/* 19 */
	rfile,		/* 20 */
	wfile,		/* 21 */
	move,		/* 22 */
	showsig,	/* 23 */
	xdraw,		/* 24 */
	odraw, 		/* 25 */
	showpin,	/* 26 */
	clrpin,		/* 27 */
	clrall,		/* 28 */
	pinsel,		/* 29 */
	sweepdrag,	/* 30 */
};

#define MAXFNS 30

void newcaption(Chip *);

void
clrsig(void)
{
	b.ncursig = 0;
	draw();
	newcaption((Chip *) 0);
}

extern NMitem *mgridn(int, NMitem *), *mplanefn(int, NMitem *), *mkeepoutfn(int, NMitem *);
NMenu mgrid = { 0, mgridn };
NMenu mplane = { 0, mplanefn };
NMenu mkeepout = { 0, mkeepoutfn };
NMitem mvwi[] = {
	{ "redraw", "repaint the layer (XOR)", 0, 0, 0, 0, LONG(24) },
	{ "redraw (OR)", "repaint the layer (OR)", 0, 0, 0, 0, LONG(25) },
	{ "zoom in", "higher magnification", 0, 0, 0, 0, LONG(9) },
	{ "zoom out", "lower magnification", 0, 0, 0, 0, LONG(10) },
	{ "planes", "show signal planes", &mplane, 0, 0, 0, 0 },
	{ "keepouts", "show chip keepout regions", &mkeepout, 0, 0, 0, 0 },
	{ "set grid", "set grid resolution", &mgrid, 0, 0, 0, 0 },
	{ "", "show ruled overlay", 0, 0, 0, 0, LONG(1) },
	{ "", "show chip names or types", 0, 0, 0, 0, LONG(11) },
	{ "", "show board pins", 0, 0, 0, 0, LONG(7) },
	{ "", "sel side", 0, 0, 0, 0, LONG(17) },
	{ 0 }
};

void
vwfn(NMitem *n)
{
#define	LAST (sizeof mvwi / sizeof mvwi[0] - 2)

	USED(n);
	mvwi[LAST-3].text = scrn.showo? "no overlay" : "show overlay";
	mvwi[LAST-2].text = scrn.shown? "show chip type" : "show chip name";
	mvwi[LAST-1].text = scrn.showp? "no pins" : "show pins";
	mvwi[LAST].text = scrn.selws? "sel comp side" : "sel wire side";
}

NMenu mvw = { mvwi };
extern NMitem *mschip(int, NMitem *), *mssig(int, NMitem *);
NMenu mselc = { 0, mschip };
NMenu msels = { 0, mssig };
NMitem mnmiti[] = {
	{ "chip", "chip from keyboard", 0, 0, 0, 0, LONG(14) },
	{ "type", "type from keyboard", 0, 0, 0, 0, LONG(19) },
	{ "signal", "signal from keyboard", 0, 0, 0, 0, LONG(18) },
	{ 0 }
};
NMitem mclri[] = {
	{ "chips", "unselect chip", 0, 0, 0, 0, LONG(3) },
	{ "signals", "unselect signal", 0, 0, 0, 0, LONG(5) },
	{ "pins", "unselect pin", 0, 0, 0, 0, LONG(27) },
	{ "all", "unselect everything", 0, 0, 0, 0, LONG(28) },
	{ 0 }
};
NMenu mclr = { mclri };
NMenu mnmit = { mnmiti };
NMitem mseli[] = {
	{ "clr", "unselect", &mclr, 0, 0, 0, 0 },
	{ "rect sel", "select chips in rectangle", 0, 0, 0, 0, LONG(4) },
	{ "sig pin", "select signal for pin", 0, 0, 0, 0, LONG(29) },
	{ "nameit", "from keyboard", &mnmit, 0, 0, 0, 0 },
	{ "chip", "find chip by name", &mselc, 0, 0, 0, 0 },
	{ "signal", "find signal by name", &msels, 0, 0, 0, 0 },
	{ "signals", "show signals attached to chips", 0, 0, 0, 0, LONG(23) },
	{ "pins", "show pins attached to chips", 0, 0, 0, 0, LONG(26) },
	{ 0 }
};

NMenu msel = { mseli };
extern NMitem *minsfn(int, NMitem *), *mimpfn(int, NMitem *);
NMenu mins = { 0, minsfn };
NMenu mimp = { 0, mimpfn };
NMitem m3i[] = {
	{ "select", "select objects for screen", &msel, 0, 0, 0, 0 },
	{ "view", "affect the scrn", &mvw, vwfn, 0, 0, 0 },
	{ "insert", "insert one unplaced chip by hand", &mins, 0, 0, 0, 0 },
	{ "improve", "improve layout automagically", &mimp, 0, 0, 0, 0 },
	{ "read files", "read files", 0, 0, 0, 0, LONG(20) },
	{ "write file", "write pos file", 0, 0, 0, 0, LONG(21) },
	{ "exit", "exit place", 0, 0, 0, 0, LONG(6) },
	{ 0 }
};

NMenu m3 = { m3i };

extern Bitmap * cursor;
void
domenu(NMenu *mu, int but, Mouse *ms)
{
	register NMitem *m;

	scmd = 0;
	if(m = mhit(mu, but, 0, ms)) {
		if(scmd)
			sendcs();
		else if(m->data && (m->data <= MAXFNS))
			(*fncs[m->data])();
	}
}

void
menu3(Mouse *m)
{
	domenu(&m3, 3, m);
}

extern NMitem *mattrfn(int, NMitem *);
NMenu mattr = { 0, mattrfn };
NMitem om2i[] = {
	{ "unplace", "remove chips from the board", 0, 0, 0, chstat, CHUNPLACE },
	{ "soft", "set placement to soft", 0, 0, 0, chstat, CHSOFT },
	{ "hard", "set placement to hard", 0, 0, 0, chstat, CHHARD },
	{ "manual", "set placement to manual only", 0, 0, 0, chstat, CHMANUAL },
	{ "igrect", "toggle allow overlap status", 0, 0, 0, chstat, CHOVER },
	{ "igpins", "toggle ignore pins status", 0, 0, 0, chstat, CHIGPINS },
	{ "igname", "toggle no text on artwork", 0, 0, 0, chstat, CHNAME },
	{ "attr", "show chip attributes", &mattr, 0, 0, 0, 0 },
	{ 0 }
};
NMenu om2 = { om2i };
NMitem m2i[] = {
	{ "sweep", "sweep/move rect selection", 0, 0, 0, 0, LONG(30) },
	{ "move", "move enclosed chips", 0, 0, 0, 0, LONG(22) },
	{ "rotate", "rotate chips clockwise by 90 deg", 0, 0, 0, chstat, CHROT },
	{ "other", "other actions", &om2, 0, 0, 0, 0 },
	{ 0 }
};
NMenu m2 = { m2i };

void
menu2(Mouse *m)
{
	domenu(&m2, 2, m);
}

char rfilename[256] = "";

void
rfile(void)
{
	char buf[200];

	getline("read files: " , rfilename);
	if(rfilename[0]) {
		expand(rfilename);
		sprint(buf, "reading: %s", rfilename);
		bepatient(buf);
		put1(READFILE);
		puts(rfilename);
		while(rcv());
		bedamned();
	}
}

void
wfile(void)
{
	static char wfilename[128] = "";

	getline("write file: " , wfilename);
	if(wfilename[0]){
		put1(WRITEFILE);
		puts(wfilename);
		while(rcv());
	}
}

void
expand( char *s)
{
	char buf[100];
	char *p, *q;
	q = s;
	while(*s && ((*s == ' ') || (*s == '\t'))) ++s;
	for(p = buf; ;++p ) {
		if((*s == 0) || (*s == ' ') || (*s == '\t')) return;
		*p = *s++;
		if((*p == '.') && (*s == 0)) break;
	}
	*p = 0;
	sprint(q, "%s.wx %s.pkg %s.brd %s.pos", buf, buf, buf, buf);
}
