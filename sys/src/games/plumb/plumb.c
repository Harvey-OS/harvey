/*
 * Plumb -- a pipe-fitting game
 * Bugs:
 *	must tweak timing
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
int score;
/*
 * Per-piece data
 */
/*
 * Directions
 */
#define	NORTH	0	/* exit north */
#define	EAST	1	/* exit east */
#define	SOUTH	2	/* exit south */
#define	WEST	3	/* exit west */
#define	START	4	/* begin block */
#define	NDIR	5
#define	END	5	/* finish block */
#define	ILL	6	/* entry not allowed (illegal) */
/*
 * Classes (END and START are classes, as well as directions)
 */
#define	CURVE	0		/* curved player pieces */
#define	STR8	1		/* straight player pieces, pun -- 8==AIGHT */
#define	CROSS	2		/* the cross piece */
#define	ONEWAY	3		/* one-way pieces */
#define	NPLAYER	4		/* pieces placed by player have class<NPLAYER */
/*	START	4 */		/* start block */
/*	END	5 */		/* end block */
#define	BLOCK	6		/* blocked squares */
#define	RES	7		/* reservoirs */
#define	BONUS	8		/* bonus pipe */
#define	NCLASS	9
#define	EMPTY	9		/* empty squares */
struct piece{
	int exit[NDIR];		/* exit direction, for each entry */
	int class;		/* What kind of piece is this */
	int s0, s1;		/* score for each passage through */
	Bitmap *image;
}piece[];
#define	NPIECE	(sizeof piece/sizeof piece[0])
struct piece piece[]={
/*	NORTH,  EAST, SOUTH,  WEST, START,  class,  s0,  s1, image */
	 EAST, NORTH,   ILL,   ILL,   ILL,  CURVE, 100,   0, 0, /*  0 N/E */
	SOUTH,   ILL, NORTH,   ILL,   ILL,   STR8, 100,   0, 0, /*  1 N/S */
	 WEST,   ILL,   ILL, NORTH,   ILL,  CURVE, 100,   0, 0, /*  2 N/W */
	  ILL, SOUTH,  EAST,   ILL,   ILL,  CURVE, 100,   0, 0, /*  3 E/S */
	  ILL,  WEST,   ILL,  EAST,   ILL,   STR8, 100,   0, 0, /*  4 E/W */
	  ILL,   ILL,  WEST, SOUTH,   ILL,  CURVE, 100,   0, 0, /*  5 S/W */
	SOUTH,  WEST, NORTH,  EAST,   ILL,  CROSS, 100, 400, 0, /*  6 N/S+E/W */
	 EAST,   ILL,   ILL,   ILL,   ILL, ONEWAY, 200,   0, 0, /*  7 N/E one way */
	SOUTH,   ILL,   ILL,   ILL,   ILL, ONEWAY, 200,   0, 0, /*  8 N/S one way */
	 WEST,   ILL,   ILL,   ILL,   ILL, ONEWAY, 200,   0, 0, /*  9 N/W one way */
	  ILL, NORTH,   ILL,   ILL,   ILL, ONEWAY, 200,   0, 0, /* 10 E/N one way */
	  ILL, SOUTH,   ILL,   ILL,   ILL, ONEWAY, 200,   0, 0, /* 11 E/S one way */
	  ILL,  WEST,   ILL,   ILL,   ILL, ONEWAY, 200,   0, 0, /* 12 E/W one way */
	  ILL,   ILL, NORTH,   ILL,   ILL, ONEWAY, 200,   0, 0, /* 13 S/N one way */
	  ILL,   ILL,  EAST,   ILL,   ILL, ONEWAY, 200,   0, 0, /* 14 S/E one way */
	  ILL,   ILL,  WEST,   ILL,   ILL, ONEWAY, 200,   0, 0, /* 15 S/W one way */
	  ILL,   ILL,   ILL, NORTH,   ILL, ONEWAY, 200,   0, 0, /* 16 W/N one way */
	  ILL,   ILL,   ILL,  EAST,   ILL, ONEWAY, 200,   0, 0, /* 17 W/E one way */
	  ILL,   ILL,   ILL, SOUTH,   ILL, ONEWAY, 200,   0, 0, /* 18 W/S one way */
	  ILL,   ILL,   ILL,   ILL, NORTH,  START,   0,   0, 0, /* 19 start N */
	  ILL,   ILL,   ILL,   ILL,  EAST,  START,   0,   0, 0, /* 20 start E */
	  ILL,   ILL,   ILL,   ILL, SOUTH,  START,   0,   0, 0, /* 21 start S */
	  ILL,   ILL,   ILL,   ILL,  WEST,  START,   0,   0, 0, /* 22 start W */
	  END,   ILL,   ILL,   ILL,   ILL,    END, 500,   0, 0, /* 23 finish N */
	  ILL,   END,   ILL,   ILL,   ILL,    END, 500,   0, 0, /* 24 finish E */
	  ILL,   ILL,   END,   ILL,   ILL,    END, 500,   0, 0, /* 25 finish S */
	  ILL,   ILL,   ILL,   END,   ILL,    END, 500,   0, 0, /* 26 finish W */
	SOUTH,   ILL, NORTH,   ILL,   ILL,    RES, 200,   0, 0, /* 27 N/S reservoir*/
	  ILL,  WEST,   ILL,  EAST,   ILL,    RES, 200,   0, 0, /* 28 E/W reservoir*/
	SOUTH,   ILL, NORTH,   ILL,   ILL,  BONUS, 200,   0, 0, /* 29 N/S bonus */
	  ILL,  WEST,   ILL,  EAST,   ILL,  BONUS, 200,   0, 0, /* 30 E/W bonus */
	  ILL,   ILL,   ILL,   ILL,   ILL,  BLOCK,   0,   0, 0, /* 31 obstacle */
	  ILL,   ILL,   ILL,   ILL,   ILL,  EMPTY,   0,   0, 0, /* 32 empty */
};
#define	empty	(&piece[NPIECE-1])
#define	SIZE	48	/* width or height of square pieces */
#define	RAD	4	/* radius of pipes, must be even and divide RESRAD, SIZE */
#define	RESRAD	18	/* radius of reservoir */
/*
 * Per-level data
 */
int level;		/* current game level */
int goodelay;		/* delay before goo moves */
int igoospeed;		/* initial goo speed */
int goospeed;		/* how fast the goo flows */
int ibonusdist;		/* initial bonus distance */
int bonusdist;		/* distance before bonuses accrue */
int dist[NPLAYER];	/* distribution of pieces given to player */
struct piece *distp[NPLAYER];
#define	NUPCOMING	100
struct piece *upcoming[NUPCOMING];
struct piece **upcomingp=upcoming, **eupcoming=upcoming;
int init[NCLASS];	/* initial number of pieces of each class */
/*
 * Per-board data
 */
#define	BORDER	4
#define	NX	10
#define	NY	7
struct square{
	struct piece *piece;
	int nentry;
	int mark;
}board[NY][NX];
Rectangle boardr, stackr, scorer;
#define	NSTACK	5
struct piece *stack[NSTACK];
/*
 * animation goodies
 */
int busy;		/* should not respond to the mouse */
int goox, gooy;		/* square currently containing goo */
struct square *goosquare;
int goodir;		/* where did the goo enter? (index in piece->exit) */
int goodist;		/* how far through the square has the goo gone? */
#define	FAST	10	/* speed of fast goo */
#define	RESDELAY	200	/* reservoir filling delay */
#define	NAGENDA	5
struct agenda{
	int time;		/* in msec, as read from /dev/cputime */
	int (*f)(void);
}agenda[NAGENDA];
int running;		/* level still active */
int now;		/* time of last event */
int currtime=0;		/* time of last call to getrtime */
rectf(Bitmap *b, Rectangle r, Fcode f){
	bitblt(b, r.min, b, r, f);
}
myborder(Bitmap *b, Rectangle r, int n, Fcode f){
	if(n<0)
		border(b, inset(r, n), -n, f);
	else
		border(b, r, n, f);
}
message(char *s){
	char buf[100];
	sprintf(buf, "Level %2d.  Score %6d.  Dist %2d.  %s",
		level, score, bonusdist, s);
	string(&screen, add(scorer.min, Pt(1, 1)), font, buf, S);
	rectf(&screen,
		Rpt(add(scorer.min, Pt(1+strwidth(font, buf), 1)), scorer.max), 0);
			
}
addscore(int n, int mult){
	if(mult && (bonusdist==0 || goospeed==FAST)) n*=2;
	score+=n;
	if(score<0) score=0;
	message("");
}
/*
 * Get current real time
 */
int getrtime(void){
	char buf[12*6];
	static int timefd=-1;
	int n;
	if(timefd==-1){
		timefd=open("/dev/cputime", OREAD);
		if(timefd<0){
			perror("open /dev/cputime");
			exits("cputime");
		}
	}
	seek(timefd, 0L, 0);
	if((n=read(timefd, buf, sizeof buf))!=sizeof buf){
		perror("read /dev/cputime");
		exits("read cputime");
	}
	currtime=atoi(buf+24);
	return currtime;
}
/*
 * Schedule f to be called at given time
 */
sched(int time, int (*f)(void)){
	struct agenda *p, *q;
	if(time<now) fprintf(stderr, "time %d < now %d\n", time, now);
	for(p=agenda;p!=&agenda[NAGENDA] && p->f && p->time<=time;p++);
	if(p==&agenda[NAGENDA]){
		fprintf(stderr, "agenda full!\n");
		exits("agenda full");
	}
	for(q=&agenda[NAGENDA-1];q!=p;--q) q[0]=q[-1];
	p->time=time;
	p->f=f;
}
/*
 * If f is on the schedule, reschedule it for the given time
 */
resched(int time, int (*f)(void)){
	struct agenda *p;
	for(p=agenda;p!=&agenda[NAGENDA] && p->f;p++){
		if(p->f==f){
			for(;p!=&agenda[NAGENDA-1] && p[1].f;p++)
				p[0]=p[1];
			if(p!=&agenda[NAGENDA-1])
				p->f=0;
			sched(time, f);
			break;
		}
	}
}
/*
 * Call all functions scheduled to be called before current time
 */
fugit(void){
	struct agenda *p;
	int (*f)(void);
	int future;
	if(!agenda[0].f) return;
	future=getrtime();
	while(agenda[0].f && agenda[0].time<future){
		now=agenda[0].time;
		f=agenda[0].f;
		for(p=&agenda[1];p!=&agenda[NAGENDA] && p->f;p++)
			p[-1]=p[0];
		p[-1].f=0;
		(*f)();
	}
	bflush();
}
putpiece(int x, int y, struct piece *p){
	board[y][x].piece=p;
	bitblt(&screen, add(boardr.min, Pt(x*SIZE, y*SIZE)),
			p->image, p->image->r, S);
}
/*
 * Should run explosion here
 */
#define	NEXPLODE	30	/* # of cycles in explosion */
#define	NDESTROY	10	/* # of cycldes to destroy unused pieces */
#define	XDELAY	33
int explodex, explodey;
struct piece *explodepiece;
explode(void){
	Point p;
	if(--busy==0){
		putpiece(explodex, explodey, explodepiece);
		return;
	}
	p=add(boardr.min, Pt(explodex*SIZE, explodey*SIZE));
	rectf(&screen, Rpt(p, add(p, Pt(SIZE, SIZE))), F&~D);
	sched(now+XDELAY, explode);
}
Point position[]={
	SIZE/2, 0,	/* NORTH */
	SIZE, SIZE/2,	/* EAST */
	SIZE/2, SIZE,	/* SOUTH */
	0, SIZE/2,	/* WEST */
	SIZE/2, SIZE/2,	/* START */
	SIZE/2, SIZE/2,	/* END */
};
fillgoo(int from, int to, int alpha){
	Point p, dp;
	p=add(position[from], add(boardr.min, Pt(goox*SIZE, gooy*SIZE)));
	dp=sub(position[to], position[from]);
	p=add(p, div(mul(dp, alpha), SIZE/2));
	rectf(&screen, Rpt(sub(p, Pt(RAD/2, RAD/2)), add(p, Pt(RAD/2, RAD/2))), 0);
}
Bitmap *resdisk[RESRAD];
mkresdisks(void){
	int i, x, y, xsq;
	Bitmap *b;
	for(i=0;i!=RESRAD;i++){
		b=resdisk[i]=balloc(Rect(0, 0, SIZE, SIZE), 1);
		for(y=0;y!=SIZE/2;y++){
			xsq=i*i-y*y;
			if(xsq>0){
				for(x=1;x*x<xsq;x++);
				segment(b, Pt(SIZE/2-x, SIZE/2-y),
					Pt(SIZE/2+x, SIZE/2-y), 3, S);
				segment(b, Pt(SIZE/2-x, SIZE/2+y),
					Pt(SIZE/2+x, SIZE/2+y), 3, S);
			}
		}
	}
}
fillres(int alpha){
	bitblt(&screen, add(boardr.min, Pt(goox*SIZE, gooy*SIZE)),
		resdisk[alpha], resdisk[alpha]->r, D&~S);
}
movegoo(void){
	int delay=goospeed;
	switch(goosquare->piece->class){
	default:
		fprintf(stderr, "Bad piece class %d in animate\n",
			goosquare->piece->class);
		exits("bad class");
	case EMPTY:
	case BLOCK:
		goto Stop;
	case CURVE:
	case STR8:
	case CROSS:
	case ONEWAY:
	case BONUS:
		if(goodist<SIZE/2) fillgoo(goodir, END, goodist);
		else fillgoo(END, goosquare->piece->exit[goodir], goodist-SIZE/2);
		goodist+=RAD/2;
		if(goodist==SIZE) goto DoneSquare;
		break;
	case RES:
		if(goodist<SIZE/2){
			fillgoo(goodir, END, goodist);
			goodist+=RAD/2;
		}
		else if(goodist<SIZE/2+RESRAD){
			fillres(goodist-SIZE/2);
			if(goospeed!=FAST) delay=RESDELAY;
			goodist++;
		}
		else{
			fillgoo(END, goosquare->piece->exit[goodir],
				goodist-SIZE/2-RESRAD);
			goodist+=RAD/2;
		}
		if(goodist==SIZE+RESRAD) goto DoneSquare;
		break;
	case START:
		fillgoo(END, goosquare->piece->exit[goodir], goodist);
		goodist+=RAD/2;
		if(goodist==SIZE/2) goto DoneSquare;
		break;
	case END:
		fillgoo(goodir, END, goodist);
		goodist+=RAD/2;
		if(goodist==SIZE/2) goto DoneSquare;
		break;
	}
	sched(now+delay, movegoo);
	return;
Stop:
	running=0;
	return;
DoneSquare:
	/*
	 * Finished with this square, score it and move to the next square
	 */
	if(bonusdist) --bonusdist;
	switch(goosquare->nentry){
	case 1: addscore(goosquare->piece->s0, 1); break;
	case 2: addscore(goosquare->piece->s1, 1); break;
	}
	switch(goosquare->piece->exit[goodir]){
	case END: goto Stop;
	case NORTH: if(gooy--==0)  goto Stop; goodir=SOUTH; break;
	case EAST:  if(++goox==NX) goto Stop; goodir=WEST;  break;
	case SOUTH: if(++gooy==NY) goto Stop; goodir=NORTH; break;
	case WEST:  if(goox--==0)  goto Stop; goodir=EAST;  break;
	}
	goosquare=&board[gooy][goox];
	if(goosquare->piece->exit[goodir]==ILL) goto Stop;
	goosquare->nentry++;
	goodist=0;
	sched(now+goospeed, movegoo);
}
fillin(Bitmap *b, int edge){
	switch(edge){
	case NORTH:
		rectf(b, Rect(SIZE/2-RAD, 0, SIZE/2+RAD, SIZE/2+RAD), F);
		break;
	case EAST:
		rectf(b, Rect(SIZE/2-RAD, SIZE/2-RAD, SIZE, SIZE/2+RAD), F);
		rectf(b, Rect(SIZE-RAD, SIZE/2-3*RAD/2,
				SIZE, SIZE/2+3*RAD/2), F);
		break;
	case SOUTH:
		rectf(b, Rect(SIZE/2-RAD, SIZE/2-RAD, SIZE/2+RAD, SIZE), F);
		rectf(b, Rect(SIZE/2-3*RAD/2, SIZE-RAD,
				SIZE/2+3*RAD/2, SIZE), F);
		break;
	case WEST:
		rectf(b, Rect(0, SIZE/2-RAD, SIZE/2+RAD, SIZE/2+RAD), F);
		break;
	}
}
pieceimages(int version){
	struct piece *p;
	int f;
	char name[100];
	static int first=1;
	static int lastversion=-1;
	if(version==lastversion) return;
	sprintf(name, "/sys/games/lib/plumb/pieces.%d", version);
	f=open(name, OREAD);
	if(f<0){
		if(first){
			perror(name);
			exits("Can't read pieces");
		}
		return;
	}
	first=0;
	lastversion=version;
	for(p=piece;p!=&piece[NPIECE];p++){
		if(p->image) bfree(p->image);
		p->image=rdbitmapfile(f);
	}
	close(f);
}
/*
 * An entry in the level file contains:
 * Level BonusDist Delay Speed PieceVersion
 *	CURVE STR8 CROSS ONEWAY
 *	END BLOCK RES BONUS
 */
FILE *param;
levelparams(int really){
	int i, piecenum, total;
	if(param==0){
		param=fopen("/sys/games/lib/plumb/levels", "r");
		if(param==0){
			perror("/sys/games/lib/plumb/levels");
			exits("can't open levels");
		}
	}
	if(fscanf(param, "%d", &i)==1){
		if(i!=level){
			fprintf(stderr,
				"parameter phase error, level %d!=%d\n", level, i);
			exits("parameter phase");
		}
		if(fscanf(param, "%d%d%d%d",
			&ibonusdist, &goodelay, &igoospeed, &piecenum)!=4){
		Bad:
			fprintf(stderr, "parameter error, level %d\n", level);
			exits("bad levels");
		}
		total=0;
		for(i=CURVE;i!=NPLAYER;i++){
			if(fscanf(param, "%d", &dist[i])!=1)
				goto Bad;
			distp[i]=piece;
			total+=dist[i];
		}
		if(total==0 || total>NUPCOMING)
			goto Bad;
		for(i=END;i!=NCLASS;i++)
			if(fscanf(param, "%d", &init[i])!=1)
				goto Bad;
	}
	if(really) pieceimages(piecenum);
	init[START]=1;
}
replacable(int x, int y){
	struct square *p;
	if(y<0 || NY<=y || x<0 || NX<=x) return 0;
	p=&board[y][x];
	if(p->nentry) return 0;
	switch(p->piece->class){
	case CURVE:
	case STR8:
	case CROSS:
	case ONEWAY:
	case EMPTY:
		return 1;
	default:
		return 0;
	}
}
okdir(int x, int y, int dir){
	switch(dir){
	case NORTH: --y; break;
	case EAST:  x++; break;
	case SOUTH: y++; break;
	case WEST:  --x; break;
	default:    return 1;
	}
	return 1<=x && x<NX-1 && 1<y && y<NY-1 && board[y][x].piece->class==EMPTY;
}
floodboard(int x, int y){
	if(x<0 || NX<=x || y<0 || NY<=y
	|| board[y][x].piece!=empty || board[y][x].mark) return;
	board[y][x].mark=1;
	floodboard(x+1, y);
	floodboard(x-1, y);
	floodboard(x, y-1);
	floodboard(x, y+1);
}
/*
 * Check that no exit is blocked and that the
 * empty cells form a single 4-connected component.
 */
okboard(void){
	int x, y, x0=-1, y0=-1, i;
	struct piece *p;
	for(y=0;y!=NY;y++) for(x=0;x!=NX;x++){
		board[y][x].mark=0;
		p=board[y][x].piece;
		if(p==empty){
			x0=x;
			y0=y;
		}
		for(i=0;i!=NDIR;i++) if(p->exit[i]!=ILL){
			if(!okdir(x, y, i)) return 0;
			if(!okdir(x, y, p->exit[i])) return 0;
		}
	}
	if(x0==-1){
		fprintf(stderr, "Board full!\n");
		exits("board full");
	}
	floodboard(x0, y0);
	for(y=0;y!=NY;y++) for(x=0;x!=NX;x++)
		if(board[y][x].piece==empty && !board[y][x].mark) return 0;
	return 1;
}
struct piece *stackpiece(void){
	struct piece *p, *v, **pp;
	int i, j, k;
	if(upcomingp==eupcoming){
		eupcoming=upcomingp=upcoming;
		for(i=0;i!=NPLAYER;i++) for(j=0;j!=dist[i];j++){
			do{
				if(++distp[i]==&piece[NPIECE])
					distp[i]=piece;
			}while(distp[i]->class!=i);
			++eupcoming;
			pp=upcoming+nrand(eupcoming-upcoming);
			eupcoming[-1]=*pp;
			*pp=distp[i];
		}
	}
	p=*upcomingp++;
	v=stack[0];
	for(i=1;i!=NSTACK;i++)
		stack[i-1]=stack[i];
	stack[NSTACK-1]=p;
	bitblt(&screen, add(stackr.min, Pt(0, SIZE)),
		&screen, Rpt(stackr.min, sub(stackr.max, Pt(0, SIZE))), S);
	bitblt(&screen, stackr.min, p->image, p->image->r, S);
	return v;
}
clearboard(void){
	int x, y, i, j, n, ntry, nrestart=0;
	struct piece *p, *q;
Restart:
	if(nrestart++==20){
		fprintf(stderr, "plumb: clearboard failed\n");
		exits("can't init");
	}
	for(y=0;y!=NY;y++) for(x=0;x!=NX;x++){
		board[y][x].nentry=0;
		board[y][x].piece=empty;
	}
	for(i=0;i!=NCLASS;i++) for(j=0;j!=init[i];j++){
		n=0;
		q=0;
		for(p=piece;p!=&piece[NPIECE];p++)
			if(p->class==i && nrand(++n)==0)
				q=p;
		if(q==0){
			fprintf(stderr, "No piece of class %d\n", i);
			exits("bad class");
		}
		for(ntry=0;;ntry++){
			if(ntry==200)
				goto Restart;
			x=nrand(NX);
			y=nrand(NY);
			if(board[y][x].piece==empty){
				board[y][x].piece=q;
				if(okboard()) break;
				board[y][x].piece=empty;
			}
		}
		if(q->class==START){
			goox=x;
			gooy=y;
			goodir=START;
			goosquare=&board[gooy][goox];
		}
	}
	for(y=0;y!=NY;y++) for(x=0;x!=NX;x++) putpiece(x, y, board[y][x].piece);
}
place(Point p){
	Point cell;
	cell=div(sub(p, boardr.min), SIZE);
	if(replacable(cell.x, cell.y)){
		if(board[cell.y][cell.x].piece!=empty){
			addscore(-50, 0);
			explodex=cell.x;
			explodey=cell.y;
			explodepiece=stackpiece();
			busy=NEXPLODE;
			putpiece(cell.x, cell.y, empty);
			disc(&screen, add(mul(cell, SIZE),
				add(boardr.min, Pt(SIZE/2, SIZE/2))),
				SIZE/2, ~0, S);
			sched(now, explode);
		}
		else
			putpiece(cell.x, cell.y, stackpiece());
	}
}
newlevel(void){
	int i;
	levelparams(1);
	clearboard();
	for(i=0;i!=NSTACK;i++) stackpiece();
	now=getrtime();
	bonusdist=ibonusdist;
	goospeed=igoospeed;
	goodist=0;
	running=1;
	busy=0;
	sched(now+goodelay, movegoo);
}
void ereshaped(Rectangle r){
	int x, y, i;
	screen.r=r;
	scorer.min=add(screen.r.min, Pt(3*BORDER, 3*BORDER));
	scorer.max=add(scorer.min, Pt((NX+2)*SIZE, font->height+2));
	stackr.min=add(scorer.min, Pt(0, font->height+2+3*BORDER));
	stackr.max=add(stackr.min, Pt(SIZE, NSTACK*SIZE));
	boardr.min=add(stackr.min, Pt(2*SIZE, 0));
	boardr.max=add(boardr.min, Pt(NX*SIZE, NY*SIZE));
	if(!ptinrect(add(boardr.max, Pt(BORDER-1, BORDER-1)), screen.r)){
		fprintf(stderr, "window too small, must be at least %d x %d\n",
			boardr.max.x-boardr.min.x+BORDER-1,
			boardr.max.y-boardr.min.y+BORDER-1);
		exits("window too small");
	}
	rectf(&screen, screen.r, 0);
	border(&screen, screen.r, BORDER, F);
	myborder(&screen, screen.r, BORDER, F);
	myborder(&screen, scorer, -BORDER, F);
	myborder(&screen, stackr, -BORDER, F);
	myborder(&screen, boardr, -BORDER, F);
	for(i=0;i!=NSTACK;i++) if(stack[i])
		bitblt(&screen, add(stackr.min, Pt(0, i*SIZE)),
			stack[i]->image, stack[i]->image->r, S);
	for(y=0;y!=NY;y++) for(x=0;x!=NX;x++) if(board[y][x].piece)
		bitblt(&screen, add(boardr.min, Pt(x*SIZE, y*SIZE)),
			board[y][x].piece->image,
			board[y][x].piece->image->r, S);
}
cleanscreen(void){
	rectf(&screen, screen.r, 0);
	border(&screen, screen.r, BORDER, F);
}
outscore(void){
	char buf[100];
	int n;
	int f;
	if(score==0) return;
	f=open("/dev/user", OREAD);
	if(f<0 || (n=read(f, buf, sizeof buf))<=0){
		strcpy(buf, "nobody");
		n=6;
	}
	sprintf(buf+n, " %d %d\n", score, level);
	close(f);
	f=open("/sys/games/lib/plumb/scores", OWRITE);
	write(f, buf, strlen(buf));
	close(f);
}
/*
 * int menuhit(int but, Mouse *m, Menu *menu)
 */
char *but2[]={
	"fast",
	0
};
Menu m2={but2};
menu2(Mouse *m){
	switch(menuhit(2, m, &m2)){
	case 0:
		goospeed=FAST;
		resched(now, movegoo);
		break;
	}
}
char *but3[]={
	"exit",
	0
};
Cursor confirmcursor={
	0, 0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0x00, 0x0E, 0x07, 0x1F, 0x03, 0x17, 0x73, 0x6F,
	0xFB, 0xCE, 0xDB, 0x8C, 0xDB, 0xC0, 0xFB, 0x6C,
	0x77, 0xFC, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
	0x94, 0xA6, 0x63, 0x3C, 0x63, 0x18, 0x94, 0x90,
};
confirm(int b){
	Mouse down, up;
	cursorswitch(&confirmcursor);
	do down=emouse(); while(!down.buttons);
	do up=emouse(); while(up.buttons);
	cursorswitch(0);
	return down.buttons==(1<<(b-1));
}
Menu m3={but3};
menu3(Mouse *m){
	switch(menuhit(3, m, &m3)){
	case 0:
		if(confirm(3)){
			outscore();
			cleanscreen();
			exits("");
		}
	}
}
/*
 * return a mouse event, preempting button 3 hits so
 * that the player can always exit.
 */
Mouse emouse3(void){
	Mouse m;
	for(;;){
		m=emouse();
		if(m.buttons!=4) break;
		menu3(&m);
	}
	return m;
}
waitclick(void){
	do;while(emouse3().buttons==0);
	do;while(emouse3().buttons!=0);
}
main(int argc, char *argv[]){
	int i, x, y, doublecross, firstlevel;
	Mouse oldm, m;
	char buf[100];
	srand(time(0));
	binit(0, 0, 0);
	einit(Emouse|Ekeyboard);
	if(argc>1){
		firstlevel=atoi(argv[1]);
		if(firstlevel<=0){
			fprintf(stderr, "%s: start level must be positive\n",
				argv[0]);
			exits("level<1");
		}
	}
	else
		firstlevel=1;
	ereshaped(screen.r);
	mkresdisks();
	m.buttons=0;
	for(;;){
		score=0;
		if(param){
			fclose(param);
			param=0;
		}
		for(level=1;level!=firstlevel;level++) levelparams(0);
		for(;;level++){
			newlevel();
			addscore(0, 0);	/* to force redisplay */
			bflush();
			while(ecanmouse()) emouse3();
			do{
				if(ecanmouse()){
					oldm=m;
					m=emouse3();
					if(oldm.buttons==0){
						if(m.buttons==1 && !busy && goospeed!=FAST)
							place(m.xy);
						else if(m.buttons==2)
							menu2(&m);
					}
				}
				fugit();
			}while(running);
			while(busy) fugit();
			doublecross=0;
			for(y=0;y!=NY;y++) for(x=0;x!=NX;x++){
				if(board[y][x].piece->class<NPLAYER){
					switch(board[y][x].nentry){
					case 0:
						addscore(-100, 0);
						explodex=x;
						explodey=y;
						explodepiece=empty;
						busy=NDESTROY;
						putpiece(x, y, empty);
						sched(now, explode);
						while(busy) fugit();
						break;
					case 2:
						doublecross++;
						break;
					}
				}
			}
			doublecross=doublecross/5*5;
			if(doublecross){
				sprintf(buf, "Bonus for %d crosses", doublecross);
				message(buf);
				bflush();
				sleep(2000);
				addscore(2000+1000*doublecross, 0);
			}
			while(ecanmouse()) emouse3();
			if(bonusdist) break;
			message("Click to continue");
			waitclick();
		}
		outscore();
		message("Game over [click]");
		waitclick();
		score=0;
		message("New game ...");
	}
}
