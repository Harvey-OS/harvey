/*
 * Piece editor for plumb
 */
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <libg.h>
#define	SIZE	48
#define	MAG	10
#define	BORDER	4
#define	NVAL	4
struct{
	char *name;
	Bitmap *bitmap;
}piece[]={
" 0 N/E", 0,
" 1 N/S", 0,
" 2 N/W", 0,
" 3 E/S", 0,
" 4 E/W", 0,
" 5 S/W", 0,
" 6 N/S+E/W", 0,
" 7 N/E one way", 0,
" 8 N/S one way", 0,
" 9 N/W one way", 0,
"10 E/N one way", 0,
"11 E/S one way", 0,
"12 E/W one way", 0,
"13 S/N one way", 0,
"14 S/E one way", 0,
"15 S/W one way", 0,
"16 W/N one way", 0,
"17 W/E one way", 0,
"18 W/S one way", 0,
"19 start N", 0,
"20 start E", 0,
"21 start S", 0,
"22 start W", 0,
"23 finish N", 0,
"24 finish E", 0,
"25 finish S", 0,
"26 finish W", 0,
"27 N/S reservoir", 0,
"28 E/W reservoir", 0,
"29 N/S bonus", 0,
"30 E/W bonus", 0,
"31 obstacle", 0,
"32 empty", 0,
};
#define	NPIECE	(sizeof piece/sizeof piece[0])
Bitmap *flat[NVAL];
char pix[SIZE][SIZE];
char curval=3;
enum{ Paint, Fill, Relief}mode=Paint;
Rectangle bigr, smallr;
void rectf(Bitmap *b, Rectangle r, Fcode f){
	bitblt(b, r.min, b, r, f);
}
void myborder(Bitmap *b, Rectangle r, int n, Fcode f){
	if(n<0)
		border(b, inset(r, n), -n, f);
	else
		border(b, r, n, f);
}
void show(Point p){
	int v=pix[p.y][p.x];
	bitblt(&screen, add(bigr.min, mul(p, MAG)), flat[v], flat[v]->r, S);
	bitblt(&screen, add(smallr.min, p), flat[v], Rect(1,1,2,2), S);
}
char pattern[16][16]={
3,0,0,0,3,0,0,0,3,0,0,0,3,0,0,0,
0,3,0,0,0,0,0,0,0,3,0,0,0,0,0,0,
0,0,3,0,0,0,0,0,0,0,3,0,0,0,0,0,
0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,0,
3,0,0,0,3,0,0,0,3,0,0,0,3,0,0,0,
0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,3,
0,0,0,0,0,0,3,0,0,0,0,0,0,0,3,0,
0,0,0,0,0,3,0,0,0,0,0,0,0,3,0,0,
3,0,0,0,3,0,0,0,3,0,0,0,3,0,0,0,
0,3,0,0,0,0,0,0,0,3,0,0,0,0,0,0,
0,0,3,0,0,0,0,0,0,0,3,0,0,0,0,0,
0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,0,
3,0,0,0,3,0,0,0,3,0,0,0,3,0,0,0,
0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,3,
0,0,0,0,0,0,3,0,0,0,0,0,0,0,3,0,
0,0,0,0,0,3,0,0,0,0,0,0,0,3,0,0,
};
void set(Point p, char v){
	if(v==4) v=pattern[p.y%16][p.x%16];
	if(pix[p.y][p.x]!=v){
		pix[p.y][p.x]=v;
		show(p);
	}
}
char region[SIZE][SIZE];
void getregion1(Point p, char v){
	if(pix[p.y][p.x]!=v || region[p.y][p.x]) return;
	region[p.y][p.x]=1;
	if(p.x!=SIZE-1) getregion1(Pt(p.x+1, p.y), v);
	if(p.x!=0)      getregion1(Pt(p.x-1, p.y), v);
	if(p.y!=SIZE-1) getregion1(Pt(p.x, p.y+1), v);
	if(p.y!=0)      getregion1(Pt(p.x, p.y-1), v);
}
void getregion(Point p){
	int x, y;
	for(y=0;y!=SIZE;y++) for(x=0;x!=SIZE;x++) region[y][x]=0;
	getregion1(p, pix[p.y][p.x]);
}
void fill(Point p, char v){
	int x, y;
	getregion(p);
	for(y=0;y!=SIZE;y++)
		for(x=0;x!=SIZE;x++)
			if(region[y][x])
				set(Pt(x, y), v);
}
int inregion(int x, int y){
	return 0<=x && x<SIZE && 0<=y && y<SIZE && region[y][x];
}
void relief(Point p){
	int x, y;
	getregion(p);
	for(y=0;y!=SIZE;y++) for(x=0;x!=SIZE;x++) if(inregion(x, y)){
		if(!inregion(x, y-1) || !inregion(x-1, y))
			set(Pt(x, y), 0);
		else if(!inregion(x, y+1) || !inregion(x+1, y))
			set(Pt(x, y), 2);
	}
}
void ereshaped(Rectangle r){
	int x, y;
	screen.r=r;
	smallr.min=add(screen.r.min, Pt(3*BORDER, 3*BORDER));
	smallr.max=add(smallr.min, Pt(SIZE, SIZE));
	bigr.min=add(smallr.max, Pt(3*BORDER, 3*BORDER));
	bigr.max=add(bigr.min, Pt(MAG*SIZE, MAG*SIZE));
	if(!ptinrect(add(bigr.max, Pt(BORDER-1, BORDER-1)), screen.r)){
		fprintf(stderr, "window too small, must be at least %d x %d\n",
			bigr.max.x-screen.r.min.x+BORDER-1,
			bigr.max.y-screen.r.min.y+BORDER-1);
		exits("window too big");
	}
	rectf(&screen, screen.r, 0);
	border(&screen, screen.r, BORDER, F);
	myborder(&screen, screen.r, BORDER, F);
	for(y=0;y!=SIZE;y++) for(x=0;x!=SIZE;x++) show(Pt(x, y));
}
void initflats(void){
	int i, x, y;
	for(i=0;i!=NVAL;i++){
		flat[i]=balloc(Rect(0, 0, MAG, MAG), 1);
		for(y=1;y!=MAG-1;y++) for(x=1;x!=MAG-1;x++)
			point(flat[i], Pt(x, y), i, S);
		for(x=0;x!=MAG;x++){
			point(flat[i], Pt(x, 0), i^2, S);
			point(flat[i], Pt(0, x), i^2, S);
			point(flat[i], Pt(x, MAG-1), i^2, S);
			point(flat[i], Pt(MAG-1, x), i^2, S);
		}
	}
}
char *mbut2[]={
	"white",
	"light",
	"dark",
	"black",
	"pattern",
	"paint",
	"fill",
	"relief",
	0
};
Menu menu2={mbut2};
char *mbut3[]={
	"next",
	"prev",
	"write",
	"exit",
	0
};
Menu menu3={mbut3};
void see(Bitmap *b){
	uchar buf[SIZE][SIZE/4];
	int x, y;
	rdbitmap(b, 0, SIZE, (uchar *)buf);
	for(y=0;y!=SIZE;y++) for(x=0;x!=SIZE;x++)
		set(Pt(x, y), (buf[y][x/4]>>(6-x%4*2))&3);
}
void save(Bitmap *b){
	bitblt(b, b->r.min, &screen, smallr, S);
}
void input(char *name){
	int i;
	Bitmap *b;
	int fd=open(name, OREAD);
	if(fd<0){
	Bad:
		fprintf(stderr, "Bad input %s\n", name);
		exits("bad input");
	}
	for(i=0;i!=NPIECE;i++){
		b=rdbitmapfile(fd);
		if(!b || !eqrect(b->r, Rect(0, 0, SIZE, SIZE)) || b->ldepth!=1)
			goto Bad;
		piece[i].bitmap=b;
	}
	close(fd);
}
void output(char *name){
	int i;
	int fd=create(name, OWRITE, 0666);
	if(fd<0){
		fprintf(stderr, "Can't create %s\n", name);
		exits("bad output");
	}
	for(i=0;i!=NPIECE;i++)
		wrbitmapfile(fd, piece[i].bitmap);
	close(fd);
}
void main(int argc, char *argv[]){
	Mouse m;
	int i=0;
	if(argc!=3){
		fprintf(stderr, "Usage: %s input output\n", argv[0]);
		exits("usage");
	}
	binit(0, 0, 0);
	einit(Emouse|Ekeyboard);
	initflats();
	ereshaped(screen.r);
	input(argv[1]);
	see(piece[i].bitmap);
	for(;;){
		m=emouse();
		switch(m.buttons){
		case 1:
			if(ptinrect(m.xy, bigr)){
				switch(mode){
				case Fill:
					fill(div(sub(m.xy, bigr.min), MAG), curval);
					do m=emouse(); while(m.buttons);
					break;
				case Paint:
					set(div(sub(m.xy, bigr.min), MAG), curval);
					break;
				case Relief:
					relief(div(sub(m.xy, bigr.min), MAG));
					do m=emouse(); while(m.buttons);
					break;
				}
			}
			break;
		case 2:
			switch(menuhit(2, &m, &menu2)){
			case 0: curval=0; break;
			case 1: curval=1; break;
			case 2: curval=2; break;
			case 3: curval=3; break;
			case 4: curval=4; break;
			case 5: mode=Paint; break;
			case 6: mode=Fill; break;
			case 7: mode=Relief; break;
			}
			break;
		case 4:
			switch(menuhit(3, &m, &menu3)){
			case 0:
				if(i<NPIECE-1){
					save(piece[i].bitmap);
					see(piece[++i].bitmap);
				}
				break;
			case 1: 
				if(i>0){
					save(piece[i].bitmap);
					see(piece[--i].bitmap);
				}
				break;
			case 2:
				save(piece[i].bitmap);
				output(argv[2]);
				break;
			case 3:
				save(piece[i].bitmap);
				output(argv[2]);
				exits(0);
			}
			break;
		}
	}
}
