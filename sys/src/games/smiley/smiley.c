#include <u.h>
#include <libc.h>
#include <libg.h>
#include "fns.h"

/*
 * A program of historical importance.
 */

#define	XMAX		800
int	YMAX		=1024;
int	END		=960;

#define	DEAD		1
#define	SCOWLEY		036
#define	SMILEY		037
#define	BOMB		035
#define	ACTIVE		0200
#define	MASK		0177
#define	DELTAY		8
#define	DELTAX		16
#define	MAXM		8
#define	CHARWIDTH	32
#define	MAXSMILEYS	63
#define	MAXBOMBS	5
#define	MAXTHINGS	(1+MAXSMILEYS+2+MAXBOMBS+1)
#define	FIRSTSCOWLEY	0
#define	FIRSTSMILEY	1
#define	FIRSTBOMB	(MAXSMILEYS+2)
#define	FRONT		(CHARWIDTH*(2+MAXSMILEYS/(XMAX/(2*CHARWIDTH))))
struct thing{
	int	x;
	int	y;
	char	type;
}thing[MAXTHINGS];
struct thing *frontthing[XMAX/CHARWIDTH];
#define	grouch		thing[FIRSTSCOWLEY]
#define	FUZZINESS	4
#define	TIMETOFUZZ	4000
#define	SELECTLIM	190
#define	DELTALIM	4
#define	GLOATTIME	4000
int	fuzzmask;
int	timetofuzz;
int	selectlim;
int	timetosel;
int	score;
int	calib;
Rectangle Jrect;

void
main(void)
{
	int i, x;
	struct thing *t;

	calibrate();
	binit(0, "/lib/font/bit/smiley/0.font", 0);
	einit(Ekeyboard);
	if(screen.r.max.y < YMAX)
		YMAX = 900;
	if(screen.r.max.y < YMAX)
		YMAX = 800;
	END = YMAX - 64;
	Jrect = Rect(0, 0, XMAX, YMAX);
Loop:
	bitblt(&screen, Pt(0,0), &screen, Jrect, F);
	initthings();
	jxystring(250, 2, "NUKE THE SMILEYS");
	drawthings();
	exinit();
	while(chartyped())
		kbdchar();	/* Clear the keyboard */
	fuzzmask=~(-1<<FUZZINESS);
	timetofuzz=TIMETOFUZZ;
	selectlim=SELECTLIM;
	timetosel=SELECTLIM;
	score=0;
	for(;;){
		if(kbdchar()==' ')
			break;
		rand();
	}
	select();
	while(thing[FIRSTSMILEY].type){
		buzz();
		if(timetosel--<=0){
			if(selectlim>0)
				timetosel=(selectlim-=DELTALIM)/2+
					   rand()%selectlim;
			select();
		}
		if(timetofuzz--<=0){
			fuzzmask>>=1;
			timetofuzz=TIMETOFUZZ;
		}
		if(chartyped())
			process(kbdchar());
		for(t= &thing[FIRSTSMILEY]; t->type; t++){
			if(t->type&ACTIVE){
				if(advance(t))
					goto Loop;
				if(t->y >= END-CHARWIDTH)
					kill(t);
			}
		}
		bombmove();
		exmove();
		bflush();
	}
	allover(SCOWLEY);
	for(x=0; x<20; x++){
		for(i=5000; --i; );
		bitblt(&screen, Jrect.min, &screen, Jrect, F&~D);
		for(i=5000; --i; );
		bitblt(&screen, Jrect.min, &screen, Jrect, F&~D);
	}
	goto Loop;
}

void
ereshaped(Rectangle r)
{
	USED(r);
}

void
bombmove(void)
{
	struct thing *t;
	for(t= &thing[FIRSTBOMB]; t->type; t++)
		if(t->y>2*CHARWIDTH){
			attack(t);
			move(t, t->x, t->y-DELTAY/2);
		}else
			kill(t);
}

void
initthings(void)
{
	int x, y, i;
	struct thing *thingp=thing;

	thingp->x=500;
	thingp->y=END;
	thingp++->type=SCOWLEY;
	y=FRONT-CHARWIDTH;
	x=0;
	for(i=0; i<XMAX/CHARWIDTH; i++)
		frontthing[i]=0;
	for(i=0; i<MAXSMILEYS; i++){
		thingp->x=x;
		thingp->y=y;
		thingp->type=SMILEY;
		if(frontthing[x/CHARWIDTH]==0)
			frontthing[x/CHARWIDTH]=thingp;
		thingp++;
		if((x+=2*CHARWIDTH)>=XMAX){
			x=0;
			if(((y/CHARWIDTH)&1)==0)
				x+=CHARWIDTH;
			y-=CHARWIDTH;
		}
	}
	while(thingp<&thing[MAXTHINGS]){
		thingp->x=0;
		thingp->y=0;
		thingp->type=0;
		thingp++;
	}
}
void
drawthings(void)
{
	struct thing *thingp;
	for(thingp=thing; thingp->type; thingp++)
		if(thingp->type!=DEAD)
			jxychar(thingp->x, thingp->y, thingp->type);
}

void
move(struct thing *t, int x, int y)
{
	register type=t->type&0177;
	static pause=0;
	jxychar(t->x, t->y, type);
	jxychar(x, y, type);
	buzz();
	t->x=x;
	t->y=y;
	if(++pause>5){
		bflush();
		sleep(16);
		pause=0;
	}
}

int
advance(struct thing *t)
{
	register m;
	if(t->y<FRONT)
		move(t, t->x, t->y+DELTAY);
	else{
		m=(10*(grouch.x-t->x))/(grouch.y-t->y);
		if(m>MAXM)
			m=MAXM;
		else if(m<-MAXM)
			m= -MAXM;
		move(t, t->x+(m*DELTAY)/8+fuzz(), t->y+DELTAY);
		if(t->y>=END-CHARWIDTH && abs(t->x-grouch.x)<CHARWIDTH/2){
			allover(SMILEY);
			return(1);
		}
	}
	return(0);
}

int
fuzz(void)
{
	return((rand()&fuzzmask)-fuzzmask/2);
}

void
kill(struct thing *victim)
{
	register i;
	struct thing *t;
	jxychar(victim->x, victim->y, victim->type&MASK);
	for(i=0; i<XMAX/CHARWIDTH; i++){
		if(frontthing[i]==victim){
			victim->type|=ACTIVE;	/* For now */
			frontthing[i]=0;
			for(t= &thing[FIRSTSMILEY]; t->type; t++)
				if((t->type&ACTIVE)==0 && i==t->x/CHARWIDTH){
					frontthing[i]=t;
					break;
				}
		}
		if(frontthing[i]>victim)
			frontthing[i]--;
	}
	do{
		*victim = *(victim+1);
	}while(victim++->type);
}

void
process(int c)
{
	struct thing *t;
	switch(c){
	case ' ':	/* Shoot */
		for(t= &thing[FIRSTBOMB]; t->type; t++)
			;
		if(t>&thing[FIRSTBOMB+MAXBOMBS-1])
			return;
		t->x=grouch.x;
		t->y=grouch.y-CHARWIDTH;
		t->type=BOMB|ACTIVE;
		jxychar(t->x, t->y, BOMB);
		attack(t);
		break;
	case 0xF3:	/* Left */
	case ',':
		if(grouch.x>=DELTAX)
			move(&grouch, grouch.x-DELTAX, END);
		break;
	case 0xF4:	/* Right */
	case '.':
		if(grouch.x<XMAX-DELTAX)
			move(&grouch, grouch.x+DELTAX, END);
		break;
	}
}
char scorestring[]="??? POINTS";

void
allover(int who)
{
	int x, y;

	while(exmove()){
		bombmove();
		bflush();
		buzz();
	}
	bitblt(&screen, Jrect.min, &screen, Jrect, F);
	for(y=0; y<YMAX; y+=CHARWIDTH){
		for(x=0; x<XMAX; x+=CHARWIDTH){
			jxychar(x, y, who);
		}
		bflush();
	}
	if(who==SMILEY){
		bitblt(&screen, Pt(10*CHARWIDTH, 14*CHARWIDTH),
			&screen, Rect(10*CHARWIDTH, 14*CHARWIDTH, (10+4)*CHARWIDTH, (14+4)*CHARWIDTH), F);
		jxystring(10*CHARWIDTH+25, 14*CHARWIDTH-6, "HAVE");
		jxystring(10*CHARWIDTH+53, 15*CHARWIDTH-6, "A");
		jxystring(10*CHARWIDTH+16, 16*CHARWIDTH-6, "HAPPY");
		jxystring(10*CHARWIDTH+35, 17*CHARWIDTH-6, "DAY");
	}
	bitblt(&screen, Pt(9*CHARWIDTH, 20*CHARWIDTH),
		&screen, Rect(9*CHARWIDTH, 20*CHARWIDTH, (9+6)*CHARWIDTH, (20+1)*CHARWIDTH), F);
	if(who==SCOWLEY)
		score+=10;	/* Bonus */
	scorestring[0]='0'+score/100;
	scorestring[1]='0'+(score/10)%10;
	scorestring[2]='0'+score%10;
	if(scorestring[0]=='0'){
		scorestring[0]=' ';
		if(scorestring[1]=='0')
			scorestring[1]=' ';
	}
	jxystring(9*CHARWIDTH, 20*CHARWIDTH-6, scorestring);
	gloat();
}

void
gloat(void){
	bflush();
	sleep(GLOATTIME);
}

void
select(void){
	int i, stop;
	struct thing *t, *this;
	stop=(rand()>>6)%(XMAX/CHARWIDTH);
	i=stop;
	do{
		this=frontthing[i];
		if(this==0 || this->type==0)
			goto Cont;
		if((this->type&ACTIVE)==0){
			frontthing[i]=0;
			this->type|=ACTIVE;
			for(t= &thing[FIRSTSMILEY]; t->type&&t<&thing[FIRSTBOMB-1]; t++)
				if((t->type&ACTIVE)==0 && i==t->x/CHARWIDTH){
					frontthing[i]=t;
					return;
				}
			return;
		}
	   Cont:
		if(++i>=XMAX/CHARWIDTH)
			i=0;
	}while(i!=stop);
}

void
attack(struct thing *t)
{
	struct thing *target;
	for(target= &thing[FIRSTSMILEY]; target->type && target<&thing[FIRSTBOMB-1]; target++)
		if(abs(t->x-target->x)<CHARWIDTH/2 &&
		   abs(t->y-target->y)<CHARWIDTH/2){
			score++;
			if(target->type&ACTIVE)
				score++;	/* Bonus */
			exstart(target->x+CHARWIDTH/2, target->y+CHARWIDTH/2);
			kill(t);
			kill(target);
			return;
		}
}

void
jxystring(int x, int y, char *p)
{
	string(&screen, Pt(x, y), font, p, D^S);
}

void
jxychar(int x, int y, int c)
{
	char s[2];
	s[1]=0;
	s[0]=c;
	string(&screen, Pt(x, y), font, s, D^S);
}

int
chartyped(void){
	return ecankbd();
}

int
kbdchar(void)
{
	return ekbd();
}

void
buzz(void)
{
	int i;

	for(i=calib; i>=0; --i)
		;
}

void
calibrate(void)
{
	int fd;
	int i;
	char buf[12];

	fd = open("/dev/cputime", OREAD);
	if(fd < 0){
		calib = 5000;
		return;
	}
	for(i=2*1000*1000; i>=0; --i)
		;
	read(fd, buf, 12);
	i = atoi(buf);
	if(i == 0)
		calib = 5000;
	else
		calib = 5000*1000/i;
}
