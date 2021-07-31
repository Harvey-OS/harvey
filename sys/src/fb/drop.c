#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
main(int argc, char *argv[]){
	PICFILE *f;
	Bitmap *b;
	char *name;
	switch(argc){
	case 1: name="IN"; break;
	case 2: name=argv[1]; break;
	default:
		fprint(2, "Usage: %s [picfile]\n", argv[0]);
		exits("usage");
	}
	f=picopen_r(name);
	if(f==0){
		picerror(name);
		exits("can't open picfile");
	}
	binit(0,0,0);
	b=rdpicfile(f, screen.ldepth);
	if(b==0){
		fprint(2, "%s: no space for bitmap\n", argv[0]);
		exits("no space");
	}
	bitblt(&screen, div(sub(add(screen.r.min, screen.r.max),
		Pt(PIC_WIDTH(f), PIC_HEIGHT(f))), 2), b, b->r, S);
	bflush();
	exits("");
}
