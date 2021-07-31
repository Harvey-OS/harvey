#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
main(int argc, char *argv[]){
	PICFILE *f;
	if(getflags(argc, argv, "")!=1) usage("");
	binit(0,0,0);
	f=picopen_w("OUT", "runcode", screen.r.min.x, screen.r.min.y,
		screen.r.max.x-screen.r.min.x, screen.r.max.y-screen.r.min.y,
		"m", argv, 0);
	if(f==0){
		perror(argv[0]);
		exits("can't open picfile");
	}
	if(wrpicfile(f, &screen)<0){
		fprint(2, "%s: can't save\n", argv[0]);
		exits("output error");
	}
	exits("");
}
