#include "mplot.h"
Bitmap *offscreen=&screen;
/*
 * Clear the window from x0, y0 to x1, y1 (inclusive) to color c
 */
void m_clrwin(int x0, int y0, int x1, int y1, int c){
	int y, hgt;
	x1++;
	y1++;
	if(c<=0)
		bitblt(offscreen, Pt(x0, y0), offscreen, Rect(x0, y0, x1, y1), Zero);
	else if(c>=(2<<screen.ldepth)-1)
		bitblt(offscreen, Pt(x0, y0), offscreen, Rect(x0, y0, x1, y1), F);
	else{
		segment(offscreen, Pt(x0, y0), Pt(x1, y0), c, S);
		for(y=y0+1,hgt=1;y<y1;y+=hgt,hgt*=2){
			if(y+hgt>y1) hgt=y1-y;
			bitblt(offscreen, Pt(x0, y), offscreen, Rect(x0, y0, x1, y0+hgt), S);
		}
	}
}
/*
 * Draw text between pointers p and q with first character centered at x, y.
 * Use color c.  Centered if cen is non-zero, right-justified if right is non-zero.
 * Returns the y coordinate for any following line of text.
 * Bug: color is ignored.
 */
int m_text(int x, int y, char *p, char *q, int c, int cen, int right){
	Point tsize;
	USED(c);
	*q='\0';
	tsize=strsize(font, p);
	if(cen) x -= tsize.x/2;
	else if(right) x -= tsize.x;
	string(offscreen, Pt(x, y-tsize.y/2), font, p, S|D);
	return y+tsize.y;
}
/*
 * Draw the vector from x0, y0 to x1, y1 in color c.
 * Clipped by caller
 */
void m_vector(int x0, int y0, int x1, int y1, int c){
	if(c<0) c=0;
	if(c>(1<<(1<<screen.ldepth))-1) c=(2<<screen.ldepth)-1;
	segment(offscreen, Pt(x0, y0), Pt(x1, y1), c, S);
}
Rectangle scr;
int scrset=0;
char *scanint(char *s, int *n){
	while(*s<'0' || '9'<*s){
		if(*s=='\0'){
			fprint(2, "plot: bad -Wxmin,ymin,xmax,ymax\n");
			exits("bad arg");
		}
		s++;
	}
	*n=0;
	while('0'<=*s && *s<='9'){
		*n=*n*10+*s-'0';
		s++;
	}
	return s;
}
void setwindow(char *s){
	s=scanint(s, &scr.min.x);
	s=scanint(s, &scr.min.y);
	s=scanint(s, &scr.max.x);
	scanint(s, &scr.max.y);
	scrset=1;
}
Rectangle getscr(void){
	int fd;
	char buf[12*5];
	fd=open("/dev/screen", OREAD);
	if(fd==-1) fd=open("/mnt/term/dev/screen", OREAD);
	if(fd==-1) return Rect(0,0,1024,1024);
	if(read(fd, buf, sizeof buf)!=sizeof buf){
		fprint(2, "Can't read /dev/screen: %r\n");
		exits("screen read");
	}
	return Rect(atoi(buf+12), atoi(buf+24), atoi(buf+36), atoi(buf+48));
}
char *rdenv(char *name){
	char *v;
	int fd, size;
	fd=open(name, OREAD);
	if(fd<0) return 0;
	size=seek(fd, 0, 2);
	v=malloc(size+1);
	if(v==0){
		fprint(2, "Can't malloc: %r\n");
		exits("no mem");
	}
	seek(fd, 0, 0);
	read(fd, v, size);
	v[size]=0;
	close(fd);
	return v;
}
void winit(void (*errfun)(char *), char *font, char *label, Rectangle r){
	char *srv, *mntsrv;
	char spec[100];
	int srvfd, cons, pid;
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFENVG|RFNOTEG|RFNOWAIT)){
	case -1:
		fprint(2, "Can't fork: %r\n");
		exits("no fork");
	case 0:
		break;
	default:
		exits(0);
	}
	srv=rdenv("/env/8½srv");
	if(srv==0){
		free(srv);
		mntsrv=rdenv("/mnt/term/env/8½srv");
		srv=malloc(strlen(mntsrv)+10);
		sprint(srv, "/mnt/term%s", mntsrv);
		free(mntsrv);
		pid=0;		/* 8½srv can't send notes to remote processes! */
	}
	else pid=getpid();
	srvfd=open(srv, ORDWR);
	free(srv);
	if(srvfd==-1){
		fprint(2, "Can't open %s: %r\n", srv);
		exits("no srv");
	}
	sprint(spec, "N%d,%d,%d,%d,%d\n", pid, r.min.x, r.min.y, r.max.x, r.max.y);
	if(mount(srvfd, "/mnt/8½", 0, spec)==-1){
		fprint(2, "Can't mount: %r\n");
		exits("no mount");
	}
	close(srvfd);
	bind("/mnt/8½", "/dev", MBEFORE);
	cons=open("/dev/cons", OREAD);
	if(cons==-1){
	NoCons:
		fprint(2, "Can't open /dev/cons: %r");
		exits("no cons");
	}
	dup(cons, 0);
	close(cons);
	cons=open("/dev/cons", OWRITE);
	if(cons==-1) goto NoCons;
	dup(cons, 1);
	dup(cons, 2);
	close(cons);
	binit(errfun, font, label);
}
/*
 * Startup initialization
 */
void m_initialize(char *s){
	static int first=1;
	int dx, dy;
	USED(s);
	if(first){
		if(!scrset){
			scr=getscr();
			scr.min=div(sub(add(scr.min, scr.max), Pt(520, 520)), 2);
			scr.max=add(scr.min, Pt(520, 520));
		}
		winit(0,0,0,scr);
		clipminx=mapminx=screen.r.min.x+4;
		clipminy=mapminy=screen.r.min.y+4;
		clipmaxx=mapmaxx=screen.r.max.x-5;
		clipmaxy=mapmaxy=screen.r.max.y-5;
		dx=clipmaxx-clipminx;
		dy=clipmaxy-clipminy;
		if(dx>dy){
			mapminx+=(dx-dy)/2;
			mapmaxx=mapminx+dy;
		}
		else{
			mapminy+=(dy-dx)/2;
			mapmaxy=mapminy+dx;
		}
		first=0;
	}
}
/*
 * Clean up when finished
 */
void m_finish(void){
	m_swapbuf();
}
void m_swapbuf(void){
	if(offscreen!=&screen)
		bitblt(&screen, offscreen->r.min, offscreen, offscreen->r, S);
	bflush();
}
void m_dblbuf(void){
	if(offscreen==&screen){
		offscreen=balloc(inset(screen.r, 4), screen.ldepth);
		if(offscreen==0){
			fprintf(stderr, "Can't double buffer\n");
			offscreen=&screen;
		}
	}
}
