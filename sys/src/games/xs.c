#include "xs.h"

/*
 * engine for 4s, 5s, etc
 */

Cursor whitearrow = {
	{0, 0},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFC, 
	 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xF8, 0xFF, 0xFC, 
	 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFC, 
	 0xF3, 0xF8, 0xF1, 0xF0, 0xE0, 0xE0, 0xC0, 0x40, },
	{0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x06, 0xC0, 0x1C, 
	 0xC0, 0x30, 0xC0, 0x30, 0xC0, 0x38, 0xC0, 0x1C, 
	 0xC0, 0x0E, 0xC0, 0x07, 0xCE, 0x0E, 0xDF, 0x1C, 
	 0xD3, 0xB8, 0xF1, 0xF0, 0xE0, 0xE0, 0xC0, 0x40, }
};

enum
{
	CNone	= 0,
	CBounds	= 1,
	CPiece	= 2,
	NX	= 10,
	NY	= 20,
};

enum{
	TIMER,
	MOUSE,
	RESHAPE,
	KBD,
	SUSPEND,
	NALT
};

char		board[NY][NX];
Rectangle	rboard;
Point		pscore;
Point		scoresz;
int		pcsz = 32;
Point		pos;
Image	*bb, *bbmask, *bb2, *bb2mask;
Image	*whitemask;
Rectangle	br, br2;
long		points;
int		dt;
int		DY;
int		DMOUSE;
int		lastmx;
Mouse	mouse;
int		newscreen;
Channel	*timerc;
Channel	*suspc;
Channel	*mousec;
Channel	*kbdc;
Mousectl	*mousectl;
Keyboardctl	*kbdctl;
int		suspended;

void		redraw(int);

int	tsleep;

Piece *piece;

#define	NCOL	10

uchar txbits[NCOL][32]={
	{0xDD,0xDD,0xFF,0xFF,0x77,0x77,0xFF,0xFF,
	 0xDD,0xDD,0xFF,0xFF,0x77,0x77,0xFF,0xFF,
	 0xDD,0xDD,0xFF,0xFF,0x77,0x77,0xFF,0xFF,
	 0xDD,0xDD,0xFF,0xFF,0x77,0x77,0xFF,0xFF},
	{0xDD,0xDD,0x77,0x77,0xDD,0xDD,0x77,0x77,
	 0xDD,0xDD,0x77,0x77,0xDD,0xDD,0x77,0x77,
	 0xDD,0xDD,0x77,0x77,0xDD,0xDD,0x77,0x77,
	 0xDD,0xDD,0x77,0x77,0xDD,0xDD,0x77,0x77},
	{0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,
	 0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,
	 0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,
	 0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55},
	{0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,
	 0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,
	 0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,
	 0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55},
	{0x22,0x22,0x88,0x88,0x22,0x22,0x88,0x88,
	 0x22,0x22,0x88,0x88,0x22,0x22,0x88,0x88,
	 0x22,0x22,0x88,0x88,0x22,0x22,0x88,0x88,
	 0x22,0x22,0x88,0x88,0x22,0x22,0x88,0x88},
	{0x22,0x22,0x00,0x00,0x88,0x88,0x00,0x00,
	 0x22,0x22,0x00,0x00,0x88,0x88,0x00,0x00,
	 0x22,0x22,0x00,0x00,0x88,0x88,0x00,0x00,
	 0x22,0x22,0x00,0x00,0x88,0x88,0x00,0x00},
	{0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
	 0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
	 0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
	 0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00},
	{0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
	 0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
	 0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
	 0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00},
	{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	 0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	 0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	 0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC},
	{0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33,
	 0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33,
	 0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33,
	 0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33},
};

int txpix[NCOL] = {
	DYellow,	/* yellow */
	DCyan,	/* cyan */
	DGreen,	/* lime green */
	DGreyblue,	/* slate */
	DRed,	/* red */
	DGreygreen,	/* olive green */
	DBlue,	/* blue */
	0xFF55AAFF,	/* pink */
	0xFFAAFFFF,	/* lavender */
	0xBB005DFF,	/* maroon */
};

Image *tx[NCOL];

int
movemouse(void)
{
	mouse.xy = Pt(rboard.min.x + Dx(rboard)/2, rboard.min.y +Dy(rboard)/2);
	moveto(mousectl, mouse.xy);
	return mouse.xy.x;
}

int
warp(Point p, int x)
{
	if (!suspended && piece != nil) {
		x = pos.x + piece->sz.x*pcsz/2;
		if (p.y < rboard.min.y)
			p.y = rboard.min.y;
		if (p.y >= rboard.max.y)
			p.y = rboard.max.y - 1;
		moveto(mousectl, Pt(x, p.y));
	}
	return x;
}

Piece *
rotr(Piece *p)
{
	if(p->rot == 3)
		return p-3;
	return p+1;
}

Piece *
rotl(Piece *p)
{
	if(p->rot == 0)
		return p+3;
	return p-1;
}

int
collide(Point pt, Piece *p)
{
	int i;
	int c = CNone;

	pt.x = (pt.x - rboard.min.x) / pcsz;
	pt.y = (pt.y - rboard.min.y) / pcsz;
	for(i=0; i<N; i++){
		pt.x += p->d[i].x;
		pt.y += p->d[i].y;
		if(pt.x<0 || pt.x>=NX || pt.y<0 || pt.y>=NY)
			c |= CBounds;
		if(board[pt.y][pt.x])
			c |= CPiece;
	}
	return c;
}

int
collider(Point pt, Point pmax)
{
	int i, j, pi, pj, n, m;

	pi = (pt.x - rboard.min.x) / pcsz;
	pj = (pt.y - rboard.min.y) / pcsz;
	n = pmax.x / pcsz;
	m = pmax.y / pcsz + 1;
	for(i = pi; i < pi+n && i < NX; i++)
		for(j = pj; j < pj+m && j < NY; j++)
			if(board[j][i])
				return 1;
	return 0;
}

void
setpiece(Piece *p){
	int i;
	Rectangle r, r2;
	Point op, delta;

	draw(bb, bb->r, display->white, nil, ZP);
	draw(bbmask, bbmask->r, display->transparent, nil, ZP);
	br = Rect(0, 0, 0, 0);
	br2 = br;
	piece = p;
	if(p == 0)
		return;
	r.min = bb->r.min;
	for(i=0; i<N; i++){
		r.min.x += p->d[i].x*pcsz;
		r.min.y += p->d[i].y*pcsz;
		r.max.x = r.min.x + pcsz;
		r.max.y = r.min.y + pcsz;
		if(i == 0){
			draw(bb, r, display->black, nil, ZP);
			draw(bb, insetrect(r, 1), tx[piece->tx], nil, ZP);
			draw(bbmask, r, display->opaque, nil, ZP);
			op = r.min;
		}else{
			draw(bb, r, bb, nil, op);
			draw(bbmask, r, bbmask, nil, op);
		}
		if(br.max.x < r.max.x)
			br.max.x = r.max.x;
		if(br.max.y < r.max.y)
			br.max.y = r.max.y;
	}
	br.max = subpt(br.max, bb->r.min);
	delta = Pt(0,DY);
	br2.max = addpt(br.max, delta);
	r = rectaddpt(br, bb2->r.min);
	r2 = rectaddpt(br2, bb2->r.min);
	draw(bb2, r2, display->white, nil, ZP);
	draw(bb2, rectaddpt(r,delta), bb, nil, bb->r.min);
	draw(bb2mask, r2, display->transparent, nil, ZP);
	draw(bb2mask, r, display->opaque, bbmask, bb->r.min);
	draw(bb2mask, rectaddpt(r,delta), display->opaque, bbmask, bb->r.min);
}

void
drawpiece(void){
	draw(screen, rectaddpt(br, pos), bb, bbmask, bb->r.min);
	if (suspended)
		draw(screen, rectaddpt(br, pos), display->white, whitemask, ZP);
}

void
undrawpiece(void)
{
	Image *mask = nil;
	if(collider(pos, br.max))
		mask = bbmask;
	draw(screen, rectaddpt(br, pos), display->white, mask, bb->r.min);
}

void
rest(void)
{
	int i;
	Point pt;

	pt = divpt(subpt(pos, rboard.min), pcsz);
	for(i=0; i<N; i++){
		pt.x += piece->d[i].x;
		pt.y += piece->d[i].y;
		board[pt.y][pt.x] = piece->tx+16;
	}
}

int
canfit(Piece *p)
{
	static int dx[]={0, -1, 1, -2, 2, -3, 3, 4, -4};
	int i, j;
	Point z;

	j = N + 1;
	if(j >= 4){
		j = p->sz.x;
		if(j<p->sz.y)
			j = p->sz.y;
		j = 2*j-1;
	}
	for(i=0; i<j; i++){
		z.x = pos.x + dx[i]*pcsz;
		z.y = pos.y;
		if(!collide(z, p)){
			z.y = pos.y + pcsz-1;
			if(!collide(z, p)){
				undrawpiece();
				pos.x = z.x;
				return 1;
			}
		}
	}
	return 0;
}

void
score(int p)
{
	char buf[128];

	points += p;
	snprint(buf, sizeof(buf), "%.6ld", points);
	draw(screen, Rpt(pscore, addpt(pscore, scoresz)), display->white, nil, ZP);
	string(screen, pscore, display->black, ZP, font, buf);
}

void
drawsq(Image *b, Point p, int ptx){
	Rectangle r;

	r.min = p;
	r.max.x = r.min.x+pcsz;
	r.max.y = r.min.y+pcsz;
	draw(b, r, display->black, nil, ZP);
	draw(b, insetrect(r, 1), tx[ptx], nil, ZP);
}

void
drawboard(void)
{
	int i, j;

	border(screen, insetrect(rboard, -2), 2, display->black, ZP);
	draw(screen, Rect(rboard.min.x, rboard.min.y-2, rboard.max.x, rboard.min.y),
		display->white, nil, ZP);
	for(i=0; i<NY; i++)
		for(j=0; j<NX; j++)
			if(board[i][j])
				drawsq(screen, Pt(rboard.min.x+j*pcsz, rboard.min.y+i*pcsz), board[i][j]-16);
	score(0);
	if (suspended)
		draw(screen, screen->r, display->white, whitemask, ZP);
}

void
choosepiece(void)
{
	int i;

	do{
		i = nrand(NP);
		setpiece(&pieces[i]);
		pos = rboard.min;
		pos.x += nrand(NX)*pcsz;
	}while(collide(Pt(pos.x, pos.y+pcsz-DY), piece));
	drawpiece();
	flushimage(display, 1);
}

int
movepiece(void)
{
	Image *mask = nil;

	if(collide(Pt(pos.x, pos.y+pcsz), piece))
		return 0;
	if(collider(pos, br2.max))
		mask = bb2mask;
	draw(screen, rectaddpt(br2, pos), bb2, mask, bb2->r.min);
	pos.y += DY;
	flushimage(display, 1);
	return 1;
}

void
suspend(int s)
{
	suspended = s;
	if (suspended)
		setcursor(mousectl, &whitearrow);
	else
		setcursor(mousectl, nil);
	if (!suspended)
		drawpiece();
	drawboard();
	flushimage(display, 1);
}

void
pause(int t)
{
	int s;
	Alt alts[NALT+1];

	alts[TIMER].c = timerc;
	alts[TIMER].v = nil;
	alts[TIMER].op = CHANRCV;
	alts[SUSPEND].c = suspc;
	alts[SUSPEND].v = &s;
	alts[SUSPEND].op = CHANRCV;
	alts[RESHAPE].c = mousectl->resizec;
	alts[RESHAPE].v = nil;
	alts[RESHAPE].op = CHANRCV;
	// avoid hanging up those writing ong mousec and kbdc
	// so just accept it all and keep mouse up-to-date
	alts[MOUSE].c = mousec;
	alts[MOUSE].v = &mouse;
	alts[MOUSE].op = CHANRCV;
	alts[KBD].c = kbdc;
	alts[KBD].v = nil;
	alts[KBD].op = CHANRCV;
	alts[NALT].op = CHANEND;

	flushimage(display, 1);
	for(;;)
		switch(alt(alts)){
		case SUSPEND:
			if (!suspended && s) {
				suspend(1);
			} else if (suspended && !s) {
				suspend(0);
				lastmx = warp(mouse.xy, lastmx);
			}
			break;
		case TIMER:
			if(suspended)
				break;
			if((t -= tsleep) < 0)
				return;
			break;
		case RESHAPE:
			redraw(1);
			break;		
		}
}

int
horiz(void)
{
	int lev[MAXN];
	int i, j, h;
	Rectangle r;

	h = 0;
	for(i=0; i<NY; i++){
		for(j=0; board[i][j]; j++)
			if(j == NX-1){
				lev[h++] = i;
				break;
			}
	}
	if(h == 0)
		return 0;
	r = rboard;
	newscreen = 0;
	for(j=0; j<h; j++){
		r.min.y = rboard.min.y + lev[j]*pcsz;
		r.max.y = r.min.y + pcsz;
		draw(screen, r, display->white, whitemask, ZP);
		flushimage(display, 1);
	}
	for(i=0; i<3; i++){
		pause(250);
		if(newscreen){
			drawboard();
			break;
		}
		for(j=0; j<h; j++){
			r.min.y = rboard.min.y + lev[j]*pcsz;
			r.max.y = r.min.y + pcsz;
			draw(screen, r, display->white, whitemask, ZP);
		}
		flushimage(display, 1);
	}
	r = rboard;
	for(j=0; j<h; j++){
		i = NY - lev[j] - 1;
		score(250+10*i*i);
		r.min.y = rboard.min.y;
		r.max.y = rboard.min.y+lev[j]*pcsz;
		draw(screen, rectaddpt(r, Pt(0,pcsz)), screen, nil, r.min);
		r.max.y = rboard.min.y+pcsz;
		draw(screen, r, display->white, nil, ZP);
		memcpy(&board[1][0], &board[0][0], NX*lev[j]);
		memset(&board[0][0], 0, NX);
	}
	flushimage(display, 1);
	return 1;
}

void
mright(void)
{
	if(!collide(Pt(pos.x+pcsz, pos.y), piece))
	if(!collide(Pt(pos.x+pcsz, pos.y+pcsz-DY), piece)){
		undrawpiece();
		pos.x += pcsz;
		drawpiece();
		flushimage(display, 1);
	}
}

void
mleft(void)
{
	if(!collide(Pt(pos.x-pcsz, pos.y), piece))
	if(!collide(Pt(pos.x-pcsz, pos.y+pcsz-DY), piece)){
		undrawpiece();
		pos.x -= pcsz;
		drawpiece();
		flushimage(display, 1);
	}
}

void
rright(void)
{
	if(canfit(rotr(piece))){
		setpiece(rotr(piece));
		drawpiece();
		flushimage(display, 1);
	}
}

void
rleft(void)
{
	if(canfit(rotl(piece))){
		setpiece(rotl(piece));
		drawpiece();
		flushimage(display, 1);
	}
}

int fusst = 0;
int
drop(int f)
{
	if(f){
		score(5L*(rboard.max.y-pos.y)/pcsz);
		do; while(movepiece());
	}
	fusst = 0;
	rest();
	if(pos.y==rboard.min.y && !horiz())
		return 1;
	horiz();
	setpiece(0);
	pause(1500);
	choosepiece();
	lastmx = warp(mouse.xy, lastmx);
	return 0;
}

int
play(void)
{
	int i;
	Mouse om;
	int s;
	Rune r;
	Alt alts[NALT+1];

	alts[TIMER].c = timerc;
	alts[TIMER].v = nil;
	alts[TIMER].op = CHANRCV;
	alts[MOUSE].c = mousec;
	alts[MOUSE].v = &mouse;
	alts[MOUSE].op = CHANRCV;
	alts[SUSPEND].c = suspc;
	alts[SUSPEND].v = &s;
	alts[SUSPEND].op = CHANRCV;
	alts[RESHAPE].c = mousectl->resizec;
	alts[RESHAPE].v = nil;
	alts[RESHAPE].op = CHANRCV;
	alts[KBD].c = kbdc;
	alts[KBD].v = &r;
	alts[KBD].op = CHANRCV;
	alts[NALT].op = CHANEND;

	dt = 64;
	lastmx = -1;
	lastmx = movemouse();
	choosepiece();
	lastmx = warp(mouse.xy, lastmx);
	for(;;)
	switch(alt(alts)){
	case MOUSE:
		if(suspended) {
			om = mouse;
			break;
		}
		if(lastmx < 0)
			lastmx = mouse.xy.x;
		if(mouse.xy.x > lastmx+DMOUSE){
			mright();
			lastmx = mouse.xy.x;
		}
		if(mouse.xy.x < lastmx-DMOUSE){
			mleft();
			lastmx = mouse.xy.x;
		}
		if(mouse.buttons&1 && !(om.buttons&1))
			rleft();
		if(mouse.buttons&2 && !(om.buttons&2))
			if(drop(1))
				return 1;
		if(mouse.buttons&4 && !(om.buttons&4))
			rright();
		om = mouse;
		break;
	case SUSPEND:
		if (!suspended && s)
			suspend(1);
		else
		if (suspended && !s) {
			suspend(0);
			lastmx = warp(mouse.xy, lastmx);
		}
		break;
	case RESHAPE:
		redraw(1);
		break;		
	case KBD:
		if(suspended)
			break;
		switch(r){
		case 'f':
		case ';':
			mright();
			break;
		case 'a':
		case 'j':
			mleft();
			break;
		case 'd':
		case 'l':
			rright();
			break;
		case 's':
		case 'k':
			rleft();
			break;
		case ' ':
			if(drop(1))
				return 1;
			break;
		}
		break;
	case TIMER:
		if(suspended)
			break;
		dt -= tsleep;
		if(dt < 0){
			i = 1;
			dt = 16 * (points+nrand(10000)-5000) / 10000;
			if(dt >= 32){
				i += (dt-32)/16;
				dt = 32;
			}
			dt = 52-dt;
			while(i-- > 0)
				if(movepiece()==0 && ++fusst==40){
					if(drop(0))
						return 1;
					break;
				}
		}
		break;
	}
}

void
setparms(void)
{
	char buf[32];
	int fd, n;

	tsleep = 50;
	fd = open("/dev/hz", OREAD);
	if(fd < 0)
		return;
	n = read(fd, buf, sizeof buf - 1);
	close(fd);
	if(n < 0)
		return;
	buf[n] = '\0';
	tsleep = strtoul(buf, 0, 10);
	tsleep = (1000 + tsleep - 1) / tsleep;
}

void
timerproc(void *v)
{
	Channel *c;
	void **arg;

	arg = v;
	c = (Channel*)arg;

	for(;;){
		sleep(tsleep);
		send(c, nil);
	}
}

void
suspproc(void *)
{
	Mouse mouse;
	Rune r;
	int s;
	Alt alts[NALT+1];

	alts[TIMER].op = CHANNOP;
	alts[MOUSE].c = mousectl->c;
	alts[MOUSE].v = &mouse;
	alts[MOUSE].op = CHANRCV;
	alts[SUSPEND].op = CHANNOP;
	alts[RESHAPE].op = CHANNOP;
	alts[KBD].c = kbdctl->c;
	alts[KBD].v = &r;
	alts[KBD].op = CHANRCV;
	alts[NALT].op = CHANEND;

	s = 0;
	for(;;)
		switch(alt(alts)){
		case MOUSE:
			send(mousec, &mouse);
			break;
		case KBD:
			switch(r){
			case 'q':
			case 'Q':
			case 0x04:
			case 0x7F:
				threadexitsall(nil);
			default:
				if(s) {
					s = 0;
					send(suspc, &s);
				} else
					switch(r){
					case 'z':
					case 'Z':
					case 'p':
					case 'P':
					case 0x1B:
						s = 1;
						send(suspc, &s);
						break;
					default:
						send(kbdc, &r);
					}
				break;
			}
		}
}

void
redraw(int new)
{
	Rectangle r;
	long dx, dy;

	if(new && getwindow(display, Refmesg) < 0)
		sysfatal("can't reattach to window");
	r = screen->r;
	pos.x = (pos.x - rboard.min.x) / pcsz;
	pos.y = (pos.y - rboard.min.y) / pcsz;
	dx = r.max.x - r.min.x;
	dy = r.max.y - r.min.y - 2*32;
	DY = dx / NX;
	if(DY > dy / NY)
		DY = dy / NY;
	DY /= 8;
	if(DY > 4)
		DY = 4;
	pcsz = DY*8;
	DMOUSE = pcsz/3;
	if(pcsz < 8)
		sysfatal("screen too small: %d", pcsz);
	rboard = screen->r;
	rboard.min.x += (dx-pcsz*NX)/2;
	rboard.min.y += (dy-pcsz*NY)/2+32;
	rboard.max.x = rboard.min.x+NX*pcsz;
	rboard.max.y = rboard.min.y+NY*pcsz;
	pscore.x = rboard.min.x+8;
	pscore.y = rboard.min.y-32;
	scoresz = stringsize(font, "000000");
	pos.x = pos.x*pcsz + rboard.min.x;
	pos.y = pos.y*pcsz + rboard.min.y;
	if(bb){
		freeimage(bb);
		freeimage(bbmask);
		freeimage(bb2);
		freeimage(bb2mask);
	}
	bb = allocimage(display, Rect(0,0,N*pcsz,N*pcsz), screen->chan, 0, 0);
	bbmask = allocimage(display, Rect(0,0,N*pcsz,N*pcsz), GREY1, 0, 0);
	bb2 = allocimage(display, Rect(0,0,N*pcsz,N*pcsz+DY), screen->chan, 0, 0);
	bb2mask = allocimage(display, bb2->r, GREY1, 0, 0);
	if(bb==0 || bbmask==0 || bb2==0 || bb2mask==0)
		sysfatal("allocimage fail (bb)");
	draw(screen, screen->r, display->white, nil, ZP);
	drawboard();
	setpiece(piece);
	if(piece)
		drawpiece();
	lastmx = movemouse();
	newscreen = 1;
	flushimage(display, 1);
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	Image *tb;
	char buf[200];
	int i, scores;
	long starttime, endtime;

	ARGBEGIN{
	default:
		usage();
	}ARGEND
	if(argc)
		usage();

	suspended = 0;
	setparms();
	snprint(buf, sizeof(buf), "%ds", N);
	initdraw(0, 0, buf);
	mousectl = initmouse(nil, display->image);	/* BUG? */
	if(mousectl == nil)
		sysfatal("[45]s: mouse init failed: %r");
	kbdctl = initkeyboard(nil);	/* BUG? */
	if(kbdctl == nil)
		sysfatal("[45]s: keyboard init failed: %r");
	starttime = time(0);
	srand(starttime);
	snprint(buf, sizeof(buf), "/sys/games/lib/%dscores", N);
	scores = open(buf, OWRITE);
	if(scores < 0)
		sysfatal("can't open %s: %r", buf);
	tb = 0;
	if(screen->depth < 3){
		tb = allocimage(display, Rect(0,0,16,16), 0, 1, -1);
		if(tb == 0)
			sysfatal("allocimage fail (tb)");
	}
	for(i = 0; i<NCOL; i++){
		tx[i] = allocimage(display, Rect(0, 0, 16, 16), screen->chan, 1, txpix[i]);
		if(tx[i] == 0)
			sysfatal("allocimage fail (tx)");
		if(screen->depth < 3){
			loadimage(tb, tb->r, txbits[i], 32);
			draw(tx[i], tx[i]->r, tb, nil, ZP);
		}
	}
	if(tb != 0)
		freeimage(tb);

	whitemask = allocimage(display, Rect(0,0,1,1), CMAP8, 1, setalpha(DWhite, 0x7F));
	if(whitemask==0)
		sysfatal("allocimage fail (whitemask)");

	threadsetname("4s-5s");
	timerc= chancreate(sizeof(int), 0);
	proccreate(timerproc, timerc, 1024);
	suspc= chancreate(sizeof(int), 0);
	mousec= chancreate(sizeof(Mouse), 0);
	kbdc= chancreate(sizeof(Rune), 0);
	threadcreate(suspproc, nil, 1024);
	points = 0;
	memset(board, 0, sizeof(board));
	redraw(0);
	if(play()){
		endtime = time(0);
		fprint(scores, "%ld\t%s\t%lud\t%ld\n",
			points, getuser(), starttime, endtime-starttime);
	}
	threadexitsall(nil);
	exits(0);
}
