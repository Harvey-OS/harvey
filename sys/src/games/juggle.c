#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#define	NSTEP	10		/* number of steps between throws */
#define	RBALL	10		/* radius of ball images */
#define	NBALL	100
int nhand=2;
int delay=20;			/* ms delay between steps */
int nball;
int maxhgt;
Rectangle win;

#define add addpt
#define sub subpt
#define inset insetrect

/*
 * pattern lists the heights of a repeating sequence of throws.
 * At time t, hand t%nhand throws.  At that time, it must
 * hold exactly one ball, unless it executes a 0 throw,
 * in which case it must hold no ball.  A throw of height h
 * at time t lands at time t+h in hand (t+h)%nhand.
 */
typedef struct Ball Ball;
struct Ball{
	int oldhand;	/* hand that previously held the ball */
	int hgt;	/* how high the throw from oldhand was */
	int time;	/* time at which ball will arrive */
	int hand;	/* hand in which ball will rest on arrival */
};
Ball ball[NBALL];
void throw(int t, int hgt){
	int hand=t%nhand;
	int i, b, n;
	b=n=0;
	for(i=0;i!=nball;i++) if(ball[i].hand==hand && ball[i].time<=t){
		n++;
		b=i;
	}
	if(hgt==0){
		if(n!=0){
			print("bad zero throw at t=%d, nball=%d\n", t, n);
			exits("bad");
		}
	}
	else if(n!=1){
		print("bad ball count at t=%d, nball=%d\n", t, n);
		exits("bad");
	}
	else{
		ball[b].oldhand=hand;
		ball[b].hgt=hgt;
		ball[b].time=t+hgt;
		ball[b].hand=(hand+hgt)%nhand;
	}
}
Point bpos(int b, int step, int t){
	Ball *bp=&ball[b];
	double dt=t-1+(step+1.)/NSTEP-(bp->time-bp->hgt);
	double hgt=(bp->hgt*dt-dt*dt)*4./(maxhgt*maxhgt);
	double alpha=(bp->oldhand+(bp->hand-bp->oldhand)*dt/bp->hgt)/(nhand-1);
	return (Point){win.min.x+(win.max.x-win.min.x)*alpha,
		       win.max.y-1+(win.min.y-win.max.y)*hgt};
}
Image *image, *disk;
void move(int t){
	int i, j;
	for(i=0;i!=NSTEP;i++){
		if(ecanmouse()) emouse();
		draw(image, inset(image->r, 3), display->white, nil, ZP);
		for(j=0;j!=nball;j++)
			draw(image, rectaddpt(disk->r, sub(bpos(j, i, t), Pt(RBALL, RBALL))),
				disk, nil, ZP);
		draw(screen, screen->r, image, nil, image->r.min);
		flushimage(display, 1);
		if(delay>0)
			sleep(delay);
	}
}

void
usage(char *name)
{
	fprint(2, "usage: %s [start] pattern\n", name);
	exits("usage");
}

void 
eresized(int new){
	if(new && getwindow(display, Refnone) < 0) {
		sysfatal("can't reattach to window");
	}
	if(image) freeimage(image);
	image=allocimage(display, screen->r, screen->chan, 0, DNofill);
	draw(image, image->r, display->black, nil, ZP);
	win=inset(screen->r, 4+2*RBALL);
}
void
main(int argc, char *argv[]){
	int sum, i, t, hgt, nstart, npattern;
	char *s, *start = nil, *pattern = nil;

	ARGBEGIN{
	default:
		usage(argv0);
	case 'd':
		s = ARGF();
		if(s == nil)
			usage(argv0);
		delay = strtol(argv[0], &s, 0);
		if(delay < 0 || s == argv[0] || *s != '\0')
			usage(argv0);
		break;
	case 'h':
		s = ARGF();
		if(s == nil)
			usage(argv0);
		nhand = strtol(argv[0], &s, 0);
		if(nhand <= 0 || s == argv[0] || *s != '\0')
			usage(argv0);
		break;
	}ARGEND
	
	switch(argc) {
	case 1: 
			start=""; 
			pattern=argv[0]; 
			break;
	case 2: 
			start=argv[0]; 
			pattern=argv[1]; 
			break;
	default: 
			usage(argv0);
	}
	sum=0;
	maxhgt=0;
	for(s=pattern;*s;s++){
		hgt=*s-'0';
		sum+=hgt;
		if(maxhgt<hgt) maxhgt=hgt;
	}
	npattern=s-pattern;
	for(s=start;*s;s++){
		hgt=*s-'0';
		if(maxhgt<hgt) maxhgt=hgt;
	}
	if(sum%npattern){
		print("%s: non-integral ball count\n",argv[0]);
		exits("partial ball");
	}
	nball=sum/npattern;
	for(i=0;i!=nball;i++){
		ball[i].oldhand=(i-nball)%nhand;
		if(ball[i].oldhand<0) ball[i].oldhand+=nhand;
		ball[i].hgt=nball;
		ball[i].time=i;
		ball[i].hand=i%nhand;
	}
	if(initdraw(nil, nil, "juggle") < 0)
		sysfatal("initdraw failed: %r");
	einit(Emouse);
	disk=allocimage(display, Rect(0, 0, 2*RBALL+1, 2*RBALL+1), screen->chan, 0, DWhite);
	fillellipse(disk, Pt(RBALL, RBALL), RBALL, RBALL, display->black, ZP);
	eresized(0);
	if(image==0){
		print("can't allocate bitmap");
		exits("no space");
	}
	for(t=0;start[t];t++){
		move(t);
		throw(t, start[t]-'0');
	}
	nstart=t;
	for(;;t++){
		move(t);
		throw(t, pattern[(t-nstart)%npattern]-'0');
	}
}
