/*
 * swar -- jerq space war
 * td&rob 84.01.01
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
int xc, yc;
#define	NOBJ	(1+2+6*2)
#define	MSPEED	(V/32)				/* speed of missile relative to ship */
#define	SZ	512				/* maximum scaled coordinate */
#define	V	256				/* velocity scale factor */
#define	G	(V*11585)			/* 11585 is SZ**(3./2.) */
#define	ALIVE	1
#define	DEAD	2
#define	SUN	3
#define	BOOM	4
#define	HYPER	5
#define	SLEEP	40				/* ms per iteration */
#define	TUBA	(2000/SLEEP)			/* iterations until born again */
#define	HYTIME	((600+rand()%600)/SLEEP)	/* iterations in hyperspace */
char *starbits[]={
#include "deathstar.icon"
};
Bitmap *stardwg;
Rectangle starrect={0, 0, 16, 16};
char *p0bits[]={
#include "player0.icon"
};
Bitmap *p0dwg;
Rectangle p0rect={0, 0, 44, 44};
char *p1bits[]={
#include "player1.icon"
};
Bitmap *p1dwg;
Rectangle p1rect={0, 0, 44, 44};
char *misbits[]={
#include "missile.icon"
};
Bitmap *misdwg;
Rectangle misrect={0, 0, 32, 32};
char *boombits[]={
#include "boom.icon"
};
Bitmap *boomdwg;
Rectangle boomrect={0, 0, 64, 64};
struct obj{
	int x, y;
	int vx, vy;		/* scaled by V */
	int orientation;
	int state;
	int diameter;
	Bitmap *bp;
	int curdwg;
#define	wrapped	timer
	int timer;
}obj[NOBJ], iobj[]={
	{0, 0, 0, 0, 0, SUN, 16},
	{ 300, 0, 0,  5*V, 8, ALIVE, 11, 0, 0, TUBA},
	{-300, 0, 0, -5*V, 0, ALIVE, 11, 0, 0, TUBA},
	{0, 0, 0, 0, 0, ALIVE, 8},
};
#define	ATT	(&obj[0])
#define	P0	(&obj[1])
#define	P1	(&obj[2])
int score[3];
#define	NORIENTATION	16
struct dv{
	int x, y;
}dv[NORIENTATION]={
#include "accel.h"
};
int xc, yc, size;
void kbdplayer(int c);
void hyper(struct obj *o);
void right(struct obj *o);
void left(struct obj *o);
void jerk(struct obj *o);
int isqrt(int x);
void fire(struct obj *o);
void initobj(struct obj *o);
void deathto(struct obj *o, int doboom);
void boom(struct obj *o);
void shards(struct obj *o);
void move(struct obj *o);
void blot(struct obj *o, int dwg);
void collide(struct obj *o, struct obj *p);
void newscore(struct obj *o, struct obj *p);
void drawscore(char *str, int sc, int where);
void doscore(void);
Bitmap *initbitmap(char *bits[], Rectangle r);
#define	sq(x)	((x)*(x))
#define	muldiv(a, b, c)	((a)*(b)/(c))
int min(int a, int b){return a<b?a:b;}
int abs(int a){return a<0?-a:a;}
void ereshaped(Rectangle r){ USED(r); exits("reshaped"); }
void main(void){
	struct obj *o, *p;
	binit(0,0,0);
	einit(Ekeyboard|Emouse);
	iobj[0].bp=stardwg=initbitmap(starbits, starrect);
	iobj[1].bp=p0dwg=initbitmap(p0bits, p0rect);
	iobj[2].bp=p1dwg=initbitmap(p1bits, p1rect);
	iobj[3].bp=misdwg=initbitmap(misbits, misrect);
	boomdwg=initbitmap(boombits, boomrect);
	xc=(screen.r.min.x+screen.r.max.x)/2;
	yc=(screen.r.min.y+screen.r.max.y)/2;
	size=min(screen.r.max.x-screen.r.min.x,
		screen.r.max.y-screen.r.min.y)/2;
	bitblt(&screen, screen.r.min, &screen, screen.r, F);
	for(o=obj;o<=P1;o++)
		initobj(o);
	for(;o!=&obj[NOBJ];o++)
		o->state=DEAD;
	doscore();
	for(;;){
		for(o=obj;o!=&obj[NOBJ];o++){
			switch(o->state){
			case ALIVE:
			case SUN:
				for(p=o+1;p!=&obj[NOBJ];p++)
					if(p->state!=DEAD)
						collide(o, p);
				if(o>P1)
					left(o);
				move(o);
				break;
			case HYPER:
				if(--o->timer==0){
					blot(o, o->curdwg);
					o->state=ALIVE;
					if(rand()%4==0){
						deathto(o, 1);
						newscore(ATT, o);
					}
				}else
					move(o);
				break;
			case DEAD:
				if((o==P0 || o==P1) && --o->timer==0)
					initobj(o);
				break;
			case BOOM:
				shards(o);
				move(o);
				break;
			}
		}
		bflush();
		while(ecanmouse()) emouse();
		if(ecankbd()) kbdplayer(ekbd());
		sleep(SLEEP);
	}
}
void kbdplayer(int c){
	switch(c){
	case 'k': left(P0);	break;
	case 'o': jerk(P0);	break;
	case ';': right(P0);	break;
	case 'l': fire(P0);	break;
	case '.':
	case ',': hyper(P0);	break;
	case 'a': left(P1);	break;
	case 'w': jerk(P1);	break;
	case 'd': right(P1);	break;
	case 's': fire(P1);	break;
	case 'z':
	case 'x': hyper(P1);	break;
	case 'Q': exits("");	break;
	}
}
void hyper(struct obj *o){
	if(o->state!=ALIVE)
		return;
	o->state=HYPER;
	o->timer=HYTIME;
	blot(o, o->curdwg);
}
void right(struct obj *o){
	if(++o->orientation==NORIENTATION)
		o->orientation=0;
}
void left(struct obj *o){
	if(--o->orientation<0)
		o->orientation=NORIENTATION-1;
}
void jerk(struct obj *o){
	o->vx+=dv[o->orientation].x/2;
	o->vy+=dv[o->orientation].y/2;
}
int isqrt(int x){
	int s, u;
	if(x<=0)
		return(0);
	if(x>=32768L*(32768L/4))
		return(2*isqrt(x/4));	/* avoid overflow */
	for(s=2, u=4;u<x;s+=s, u*=4);
	while((u=((x+s*s)/s)>>1)<s)
		s=u;
	return(s);
}
void fire(struct obj *o){
	struct obj *m;
	int vx, vy, vl;
	if(o->state!=ALIVE)
		return;
	for(m=o+2;m<&obj[NOBJ];m+=2)
		if(m->state==DEAD){
			initobj(m);
			m->state=ALIVE;
			vl=isqrt(sq(o->vx)+sq(o->vy));
			if(vl==0)
				vl=V;
			vx=muldiv(vl, dv[o->orientation].x, V);
			vy=muldiv(vl, dv[o->orientation].y, V);
			m->x=o->x+muldiv(vx, (o->diameter+m->diameter), vl);
			m->y=o->y+muldiv(vy, (o->diameter+m->diameter), vl);
			m->vx=o->vx+MSPEED*dv[o->orientation].x;
			m->vy=o->vy+MSPEED*dv[o->orientation].y;
			blot(m, m->orientation);
			return;
		}
}
void initobj(struct obj *o){
	*o=(o>P1)?iobj[P1-obj+1]:iobj[o-obj];
	if(o<=P1)
		blot(o, o->orientation);
}
void deathto(struct obj *o, int doboom){
	o->state=DEAD;
	blot(o, o->curdwg);
	if(doboom)
		boom(o);
}
void boom(struct obj *o){
	o->state=BOOM;
	o->bp=boomdwg;
	o->diameter=boomdwg->r.max.x/4;
	blot(o, o->orientation=0);
}
void shards(struct obj *o){
	if(++o->orientation==16){
		blot(o, o->curdwg);
		o->state=DEAD;
		o->timer=TUBA;
	}
}
void move(struct obj *o){
	int r32;
	int x, y;
	if(o->state==DEAD || o->state==SUN)
		return;
	r32=o->x*o->x+o->y*o->y;
	if(r32!=0){
		r32*=isqrt(r32);	/* pow(r, 3./2.) */
		if(r32!=0){
			o->vx-=G*o->x/r32;
			o->vy-=G*o->y/r32;
		}
	}
	x=o->x+o->vx/V;
	y=o->y+o->vy/V;
	if(x<-SZ || SZ<x){
		if(o>P1 && o->wrapped){
    Death:
			deathto(o, 0);
			return;
		}
		if(x<-SZ) x+=2*SZ; else x-=2*SZ;
		o->wrapped++;
	}
	if(y<-SZ || SZ<y){
		if(o>P1 && o->wrapped)
			goto Death;
		if(y<-SZ) y+=2*SZ; else y-=2*SZ;
		o->wrapped++;
	}
	if(o->state!=HYPER)
		blot(o, o->curdwg);
	o->x=x, o->y=y;
	if(o->state!=HYPER)
		blot(o, o->orientation);
}
#define	BLOTSIZE	5
#define	rescale(x)	muldiv(x, size, SZ)
void blot(struct obj *o, int dwg){
	int dx=dwg%4*o->diameter, dy=dwg/4*o->diameter;
	bitblt(&screen, Pt(rescale(o->x)+xc-o->diameter/2, rescale(o->y)+yc-o->diameter/2),
		o->bp, Rect(dx, dy, dx+o->diameter, dy+o->diameter), S^D);
	o->curdwg=dwg;
}
void collide(struct obj *o, struct obj *p){
	int doneboom;
	/* o<p always */
	if(o->state!=HYPER && p->state!=HYPER
	&& sq(rescale(o->x-p->x))+sq(rescale(o->y-p->y))<
		sq(o->diameter+p->diameter)/4){
		newscore(o, p);
		if(doneboom=o->state==ALIVE)
			deathto(o, 1);
		if(p->state==ALIVE)
			deathto(p, !doneboom || (o==P0 && p==P1));
	}
}
void newscore(struct obj *o, struct obj *p){
	doscore();
	/* o<p always */
	score[2]++;
	if(o==P0 || p==P0)
		score[1]++;
	if(o==P1 || p==P1)
		score[0]++;
	doscore();
}
void drawscore(char *str, int sc, int where){
	static char buf[16];
	char *p=buf+6;
	int s;
	do; while(*p++=*str++);
	p=buf+6;
	s=abs(sc);
	do{
		*--p=s%10+'0';
		s/=10;
	}while(s);
	if(sc<0)
		*--p='-';
	if(where==2)
		s=screen.r.min.x+20;
	else if(where==1)
		s=(screen.r.min.x+screen.r.max.x-strwidth(font, p))/2;
	else
		s=screen.r.max.x-strwidth(font, p)-20;
	string(&screen, Pt(s, screen.r.min.y+5), font, p, S^D);
}
void doscore(void){
	drawscore(" MCI", score[0], 0);
	drawscore(" AT&T", score[2], 1);
	drawscore(" SPRINT", score[1], 2);
}
Bitmap *initbitmap(char *bits[], Rectangle r){
	Point p;
	Bitmap *b=balloc(r, screen.ldepth);
	char *bit;
	if(b==0){
		fprint(2, "swar: can't balloc\n");
		exits("balloc");
	}
	for(p.y=r.min.y;p.y!=r.max.y;p.y++){
		bit=bits[p.y-r.min.y];
		for(p.x=r.min.x;p.x!=r.max.x;p.x++){
			while(*bit==' ') bit++;
			point(b, p, *bit++=='x'?~0:0, S);
		}
	}
	return b;
}
