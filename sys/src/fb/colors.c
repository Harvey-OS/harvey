#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
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
		pid=0;			/* 8½srv can't send notes to remote processes! */
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
int nbit, npix;
Bitmap *pixel;
Rectangle crect[256];
void ereshaped(Rectangle newr){
	int x, y, i, n, nx, ny;
	Rectangle r, b;
	screen.r=newr;
	n=1<<(1<<screen.ldepth);
	nx=1<<((1<<screen.ldepth)/2);
	ny=n/nx;
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	r=inset(screen.r, 5);
	r.min.y+=20;
	b.max.y=r.min.y;
	for(i=n-1,y=0;y!=ny;y++){
		b.min.y=b.max.y;
		b.max.y=r.min.y+(r.max.y-r.min.y)*(y+1)/ny;
		b.max.x=r.min.x;
		for(x=0;x!=nx;x++,--i){
			point(pixel, Pt(0,0), i, S);
			b.min.x=b.max.x;
			b.max.x=r.min.x+(r.max.x-r.min.x)*(x+1)/nx;
			crect[i]=inset(b, 1);
			texture(&screen, crect[i], pixel, S);
		}
	}
	bflush();
}
char *buttons[]={"exit", 0};
Menu menu={buttons};
void main(int argc, char *argv[]){
	Rectangle scr;
	Mouse m;
	int i, n, prev;
	char buf[100];
	RGB cmap[256];
	if(argc!=1){
		fprint(2, "Usage: %s\n", argv[0]);
		exits("usage");
	}
	scr=getscr();
	winit(0,0,0, raddp(Rect(-141,-161,141,141), div(add(scr.min, scr.max), 2)));
	einit(Emouse);
	pixel=balloc(Rect(0,0,1,1), screen.ldepth);
	if(pixel==0){
		fprint(2, "%s: can't balloc\n", argv[0]);
		exits("no balloc!");
	}
	ereshaped(screen.r);
	prev=-1;
	for(;;){
		m=emouse();
		switch(m.buttons){
		case 1:
			rdcolmap(&screen, cmap);
			while(m.buttons){
				n=1<<(1<<screen.ldepth);
				prev=-1;
				for(i=0;i!=n;i++) if(i!=prev && ptinrect(m.xy, crect[i])){
					sprint(buf, "index %3d r %3d g %3d b %3d",
						i,
						cmap[i].red>>24,
						cmap[i].green>>24,
						cmap[i].blue>>24);
					string(&screen, add(screen.r.min, Pt(2,2)),
						font, buf, S);
					prev=i;
					break;
				}
				m=emouse();
			}
			break;
		case 4:
			switch(menuhit(3, &m, &menu)){
			case 0:	exits(0);
			}
		}
	}
}
