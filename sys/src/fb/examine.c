#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
Bitmap *b;
Point corner;
int wid, hgt, nchan;
void ereshaped(Rectangle r){
	screen.r=r;
	if(!b) return;
	corner=div(sub(add(screen.r.min, screen.r.max), Pt(wid, hgt)), 2);
	bitblt(&screen, corner, b, b->r, S);
}
main(int argc, char *argv[]){
	PICFILE *f;
	char *name;
	int y, i;
	Mouse m;
	char *pic, *pv;
	switch(argc){
	case 1: name="IN"; break;
	case 2: name=argv[1]; break;
	default:
		fprint(2, "Usage: %s [picfile]\n", argv[0]);
		exits("usage");
	}
	f=picopen_r(name);
	if(f==0){
	Bad:
		picerror(name);
		exits("can't open picfile");
	}
	wid=PIC_WIDTH(f);
	hgt=PIC_HEIGHT(f);
	nchan=PIC_NCHAN(f);
	binit(0,0,argv[0]);
	einit(Emouse);
	b=rdpicfile(f, screen.ldepth);
	if(b==0){
		fprint(2, "%s: no space for bitmap\n", argv[0]);
		exits("no space");
	}
	picclose(f);
	ereshaped(screen.r);
	f=picopen_r(name);
	if(f==0) goto Bad;
	pic=malloc(wid*hgt*nchan);
	for(y=0;y!=hgt;y++)
		picread(f, pic+y*wid*nchan);
	for(;;){
		m=emouse();
		if(m.buttons&1){
			m.xy=sub(m.xy, corner);
			if(ptinrect(m.xy, Rect(0, 0, wid, hgt))){
				print("x %3d y %3d", m.xy.x+PIC_XOFFS(f),
					m.xy.y+PIC_YOFFS(f));
				pv=pic+(m.xy.y*wid+m.xy.x)*nchan;
				for(i=0;i!=nchan;i++)
					print(" %c %3d", f->chan[i], *pv++&255);
				print("\n");
			}
			do m=emouse(); while(m.buttons);
		}
		else if(m.buttons&2) ereshaped(screen.r);
		else if(m.buttons&4) exits("");
	}
}
