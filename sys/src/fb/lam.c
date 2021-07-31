/*% cyntax % -lfb && cc -go # % -lfb
 * lam -- stick a bunch of pictures together.
 * Bug: nchan must match for all pictures
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
Rectangle runion(Rectangle r, Rectangle s){
	if(s.min.x<r.min.x) r.min.x=s.min.x;
	if(s.min.y<r.min.y) r.min.y=s.min.y;
	if(s.max.x>r.max.x) r.max.x=s.max.x;
	if(s.max.y>r.max.y) r.max.y=s.max.y;
	return r;
}
main(int argc, char *argv[]){
	PICFILE **inf, **inp, **einf, *otf;
	Rectangle outr;
	int y;
	char win[512];
	unsigned char *bp, *buf, *ebuf;
	argc=getflags(argc, argv, "");
	if(argc<2) usage("input ...");
	inf=malloc((argc-1)*sizeof(PICFILE *));
	if(inf==0){
		fprint(2, "can't malloc %d file pointers\n", argc-1);
		exits("too many files");
	}
	for(einf=inf;einf-inf!=argc-1;einf++){
		*einf=picopen_r(argv[einf-inf+1]);
		if(*einf==0){
			perror(argv[einf-inf+1]);
			exits("open input");
		}
		if(einf==inf){
			outr.min.x=PIC_XOFFS(*einf);
			outr.min.y=PIC_YOFFS(*einf);
			outr.max.x=PIC_XOFFS(*einf)+PIC_WIDTH(*einf);
			outr.max.y=PIC_YOFFS(*einf)+PIC_HEIGHT(*einf);
		}
		else{
			if(PIC_NCHAN(*einf)!=einf[-1]->nchan){
				fprint(2, "%s: NCHAN must match for all input pictures\n", argv[0]);
				exits("nchan mismatch");
			}
			outr=runion(outr, PIC_RECT(*einf));
		}
	}
	buf=(unsigned char *)malloc(inf[0]->nchan*(outr.max.x-outr.min.x));
	if(buf==0){
		fprint(2, "%s: can't get space\n", argv[0]);
		exits("no space");
	}
	ebuf=buf+PIC_NCHAN(inf[0])*(outr.max.x-outr.min.x);
	otf=picopen_w("OUT", "runcode", outr.min.x, outr.min.y,
		outr.max.x-outr.min.x, outr.max.y-outr.min.y, picgetprop(inf[0], "CHAN"),
		argv, (char *)0);
	if(otf==0){
		perror(argv[0]);
		exits("create output");
	}
	for(y=outr.min.y;y!=outr.max.y;y++){
		for(bp=buf;bp!=ebuf;)
			*bp++=0;
		for(inp=inf;inp!=einf;inp++)
			if(PIC_YOFFS(*inp)<=y && y<PIC_YOFFS(*inp)+PIC_HEIGHT(*inp))
				picread(*inp, (char *)(buf+(*inp)->nchan*
					(PIC_XOFFS(*inp)-outr.min.x)));
		picwrite(otf, (char *)buf);
	}
	exits("");
}
