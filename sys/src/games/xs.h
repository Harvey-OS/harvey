/*
 * engine for 4s, 5s, etc
 * need to define N and pieces table before including this
 */

enum
{
	CNone	= 0,
	CBounds	= 1,
	CPiece	= 2,
	Etimer	= 4,
	NX	= 10,
	NY	= 20,
};

#define	NP	(sizeof pieces/sizeof(Piece))

char		board[NY][NX];
Rectangle	rboard;
Point		pscore;
int		pcsz = 32;
Point		pos;
Bitmap		*bb, *bb2;
Point		bp;
Rectangle	br, br2;
long		points;
int		dt;
int		DY;
int		DMOUSE;
int		lastmx;
int		newscreen;

int	tsleep;

Piece *piece, *bpiece;

uchar txbits[8][32]={
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
	{0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	 0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	 0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
	 0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC},
	{0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33,
	 0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33,
	 0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33,
	 0xCC,0xCC,0xCC,0xCC,0x33,0x33,0x33,0x33},
};

RGB txcol[8] = {
	{0x00000000,	0x00000000,	0xFFFFFFFF},
	{0xFFFFFFFF,	0x00000000,	0x00000000},
	{0xFFFFFFFF,	0x00000000,	0xFFFFFFFF},
	{0x00000000,	0xFFFFFFFF,	0x00000000},
	{0x00000000,	0xFFFFFFFF,	0xFFFFFFFF},
	{0xBCBCBCBC,	0x11111111,	0x8F8F8F8F},
	{0xFFFFFFFF,	0xFFFFFFFF,	0x00000000},
	{0xFFFF0000,	0x0000FFFFF,	0x0000FFFF},
};

Bitmap *tx[8];

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

void
rest(void)
{
	int i;
	Point pt;

	pt = div(sub(pos, rboard.min), pcsz);
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
	if(N > 4){
		if(j);
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
				pos.x = z.x;
				return 1;
			}
		}
	}
	return 0;
}

void
restore(void)
{
	bitblt(&screen, bp, bb, br, DxorS);
}

void
backup(void)
{
	bp = pos;
}

void
draw(Bitmap *b, Point p){
	int i;
	Rectangle or, r;

	bitblt(bb, bb->r.min, bb, bb->r, 0);
	br.min = br.max = r.min = bb->r.min;
	for(i=0; i<N; i++){
		r.min.x += piece->d[i].x*pcsz;
		r.min.y += piece->d[i].y*pcsz;
		r.max.x = r.min.x + pcsz;
		r.max.y = r.min.y + pcsz;
		if(i == 0){
			bitblt(bb, r.min, bb, r, 0xF);
			texture(bb, inset(r, 1), tx[piece->tx], S);
			or = r;
		}else
			bitblt(bb, r.min, bb, or, S);
		if(br.max.x < r.max.x)
			br.max.x = r.max.x;
		if(br.max.y < r.max.y)
			br.max.y = r.max.y;
	}
	bitblt(b, p, bb, br, DxorS);
}

void
score(int p)
{
	char buf[128];

	points += p;
	sprint(buf, "%.6d", points);
	string(&screen, pscore, font, buf, S);
}

void
drawsq(Bitmap *b, Point p, int ptx){
	Rectangle r;

	r.min = p;
	r.max.x = r.min.x+pcsz;
	r.max.y = r.min.y+pcsz;
	bitblt(b, r.min, b, r, 0xF);
	texture(b, inset(r, 1), tx[ptx], S);
}

void
drawboard(void)
{
	int i, j;

	border(&screen, inset(rboard, -2), 2, 0xF);
	bitblt(&screen, Pt(rboard.min.x, rboard.min.y-2),
		&screen, Rect(rboard.min.x, rboard.min.y-2, rboard.max.x, rboard.min.y), 0);
	for(i=0; i<NY; i++)
		for(j=0; j<NX; j++)
			if(board[i][j])
				drawsq(&screen, Pt(rboard.min.x+j*pcsz, rboard.min.y+i*pcsz), board[i][j]-16);
	score(0);
}

void
choosepiece(void)
{
	int i;

	do{
		i = nrand(NP);
		piece = &pieces[i];
		pos = rboard.min;
		pos.x += nrand(NX)*pcsz;
	}while(collide(Pt(pos.x, pos.y+pcsz-DY), piece));
	backup();
	draw(&screen, pos);
	bflush();
}

int
movepiece(void)
{
	Point p;

	if(collide(Pt(pos.x, pos.y+pcsz), piece))
		return 0;
	if(bpiece != piece){
		br2 = br;
		br2.max.y += DY;
		bitblt(bb2, bb2->r.min, bb2, br2, 0);
		p = bb2->r.min;
		bitblt(bb2, p, bb, br, S);
		p.y += DY;
		bitblt(bb2, p, bb, br, DxorS);
		bpiece = piece;
	}
	bitblt(&screen, bp, bb2, br2, DxorS);
	pos.y += DY;
	bp = pos;
	bflush();
	return 1;
}

void
pause(int t)
{
	Event e;

	while(ecanread(Etimer))
		eread(Etimer, &e);
	while(eread(Etimer, &e))
		if((t -= tsleep) < 0)
			break;
}

int
horiz(void)
{
	int lev[N];
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
		bitblt(&screen, r.min, &screen, r, 0);
		bflush();
	}
	for(i=0; i<6; i++){
		pause(500);
		if(newscreen){
			drawboard();
			break;
		}
		for(j=0; j<h; j++){
			r.min.y = rboard.min.y + lev[j]*pcsz;
			r.max.y = r.min.y + pcsz;
			bitblt(&screen, r.min, &screen, r, 0xF&~D);
		}
		bflush();
	}
	r = rboard;
	for(j=0; j<h; j++){
		i = NY - lev[j] - 1;
		score(250+10*i*i);
		r.min.y = rboard.min.y;
		r.max.y = rboard.min.y+lev[j]*pcsz;
		bitblt(&screen, Pt(r.min.x, r.min.y+pcsz), &screen, r, S);
		r.max.y = rboard.min.y+pcsz;
		bitblt(&screen, rboard.min, &screen, r, 0);
		memcpy(&board[1][0], &board[0][0], NX*lev[j]);
		memset(&board[0][0], 0, NX);
	}
	bflush();
	return 1;
}

void
mright(void)
{
	if(!collide(Pt(pos.x+pcsz, pos.y), piece))
	if(!collide(Pt(pos.x+pcsz, pos.y+pcsz-DY), piece)){
		restore();
		pos.x += pcsz;
		backup();
		bitblt(&screen, bp, bb, br, DxorS);
		bflush();
	}
}

void
mleft(void)
{
	if(!collide(Pt(pos.x-pcsz, pos.y), piece))
	if(!collide(Pt(pos.x-pcsz, pos.y+pcsz-DY), piece)){
		restore();
		pos.x -= pcsz;
		backup();
		bitblt(&screen, bp, bb, br, DxorS);
		bflush();
	}
}

void
rright(void)
{
	if(canfit(rotr(piece))){
		restore();
		piece = rotr(piece);
		backup();
		draw(&screen, pos);
		bflush();
	}
}

void
rleft(void)
{
	if(canfit(rotl(piece))){
		restore();
		piece = rotl(piece);
		backup();
		draw(&screen, pos);
		bflush();
	}
}

int fusst = 0;
int
drop(int f)
{
	Event e;

	if(f){
		score(5L*(rboard.max.y-pos.y)/pcsz);
		do; while(movepiece());
	}
	fusst = 0;
	rest();
	if(pos.y==rboard.min.y && !horiz())
		return 1;
	horiz();
	piece = 0;
	pause(1500);
	choosepiece();
	while(ecanread(Emouse)){
		eread(Emouse, &e);
		lastmx = e.mouse.xy.x;
	}
	return 0;
}


int
play(void)
{
	int i;
	Mouse om;
	Event e;

	/* flush the pipe */
	while(ecanread(~0))	event(&e);
	dt = 64;
	choosepiece();
	for(;;)
	switch(event(&e)){
	case Emouse:
		if(lastmx < 0)
			lastmx = e.mouse.xy.x;
		if(e.mouse.xy.x > lastmx+DMOUSE){
			mright();
			lastmx = e.mouse.xy.x;
		}
		if(e.mouse.xy.x < lastmx-DMOUSE){
			mleft();
			lastmx = e.mouse.xy.x;
		}
		if(e.mouse.buttons&1 && !(om.buttons&1))
			rleft();
		if(e.mouse.buttons&2 && !(om.buttons&2))
			if(drop(1))
				return 1;
		if(e.mouse.buttons&4 && !(om.buttons&4))
			rright();
		om = e.mouse;
		break;		
	case Ekeyboard:
		switch(e.kbdc){
		case 'q':
		case 'Q':
		case 0x04:
			return 0;
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
	case Etimer:
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
	return 0;		/* not reached */
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
main(int argc, char *argv[])
{
	Bitmap *tb;
	char buf[5*NAMELEN];
	int i, scores;
	long starttime, endtime;
	ulong pix;

	ARGBEGIN{
	}ARGEND

	setparms();
	sprint(buf, "%d", N);
	binit(0, 0, buf);
	starttime = time(0);
	srand(starttime);
	sprint(buf, "/sys/games/lib/%dscores", N);
	scores = open(buf, OWRITE);
	if(scores < 0){
		fprint(2, "can't open %s\n", buf);
		exits("scores file");
	}
	tb = 0;
	if(screen.ldepth != 0){
		if(screen.ldepth == 3)
			tb = balloc(Rect(0,0,1,1),3);
		else
			tb = balloc(Rect(0,0,16,16),0);
		if(tb == 0){
			fprint(2, "balloc fail\n");
			exits("balloc");
		}
	}
	for(i = 0; i<sizeof(tx)/sizeof(tx[0]); i++){
		tx[i] = balloc(Rect(0, 0, 16, 16), screen.ldepth);
		if(screen.ldepth == 0)
			wrbitmap(tx[i], 0, 16, txbits[i]);
		else if(screen.ldepth == 3){
			pix = rgbpix(&screen, txcol[i]);
			point(tb, Pt(0,0), pix, S);
			texture(tx[i], tx[i]->r, tb, S);
		}else{
			wrbitmap(tb, 0, 16, txbits[i]);
			bitblt(tx[i], Pt(0,0), tb, Rect(0, 0, 16, 16), S);
		}
	}
	if(screen.ldepth != 0)
		bfree(tb);
	einit(Ekeyboard|Emouse);
	etimer(Etimer, tsleep);
	points = 0;
	memset(board, 0, sizeof(board));
	ereshaped(screen.r);
	if(play()){
		endtime = time(0);
		fprint(scores, "%ld\t%s\t%lud\t%ld\n",
			points, getuser(), starttime, endtime-starttime);
	}
	exits(0);
}

void
ereshaped(Rectangle r)
{
	long dx, dy;

	screen.r = r;
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
	if(pcsz < 8){
		fprint(2, "screen too small: %d\n", pcsz);
		exits("screen");
	}
	rboard = screen.r;
	rboard.min.x += (dx-pcsz*NX)/2;
	rboard.min.y += (dy-pcsz*NY)/2+32;
	rboard.max.x = rboard.min.x+NX*pcsz;
	rboard.max.y = rboard.min.y+NY*pcsz;
	pscore.x = rboard.min.x+8;
	pscore.y = rboard.min.y-32;
	pos.x = pos.x*pcsz + rboard.min.x;
	pos.y = pos.y*pcsz + rboard.min.y;
	if(bb){
		bfree(bb);
		bfree(bb2);
	}
	bb = balloc(Rpt(rboard.min, add(rboard.min, Pt(N*pcsz, N*pcsz))), screen.ldepth);
	bb2 = balloc(Rpt(rboard.min, add(rboard.min, Pt(N*pcsz, N*pcsz+DY))), screen.ldepth);
	if(bb==0 || bb2==0){
		fprint(2, "balloc fail\n");
		exits("balloc");
	}
	bitblt(&screen, screen.r.min, &screen, screen.r, 0);
	drawboard();
	if(piece)
		draw(&screen, pos);
	bp = pos;
	bpiece = 0;
	lastmx = -1;
	newscreen = 1;
	bflush();
}
