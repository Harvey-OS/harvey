#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NSTEP	10		/* number of steps between throws */
#define	DELAY	20		/* ms delay between steps */
#define	RBALL	10		/* radius of ball images */
#define	NBALL	100
int nhand=2;
int nball;
int maxhgt;
Rectangle win;
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
Bitmap *image, *disk;
void move(int t){
	int i, j;
	for(i=0;i!=NSTEP;i++){
		if(ecanmouse()) emouse();
		bitblt(image, add(image->r.min, Pt(3, 3)), image, inset(image->r, 3), Zero);
		for(j=0;j!=nball;j++)
			bitblt(image, sub(bpos(j, i, t), Pt(RBALL, RBALL)),
				disk, disk->r, S|D);
		bitblt(&screen, image->r.min, image, image->r, S);
		bflush();
		sleep(DELAY);
	}
}
void ereshaped(Rectangle r){
	if(image) bfree(image);
	screen.r=r;
	image=balloc(screen.r, screen.ldepth);
	bitblt(image, image->r.min, image, image->r, F);
	win=inset(screen.r, 4+2*RBALL);
}
void
main(int argc, char *argv[]){
	int sum, i, t, hgt, nstart, npattern;
	char *s, *start, *pattern;
	switch(getflags(argc, argv, "h:1[hands]")){
	default: usage("[start] pattern");
	case 2: start=""; pattern=argv[1]; break;
	case 3: start=argv[1]; pattern=argv[2]; break;
	}
	if(flag['h']) nhand=atoi(flag['h'][0]);
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
	binit(0,0,0);
	einit(Emouse);
	disk=balloc(Rect(0, 0, 2*RBALL+1, 2*RBALL+1), screen.ldepth);
	disc(disk, Pt(RBALL, RBALL), RBALL, ~0, S);
	ereshaped(screen.r);
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
