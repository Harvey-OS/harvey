#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
/*
 * Usage: dumppic [-t type] [-h nheader] file xsize ysize chan
 */
void main(int argc, char *argv[]){
	int ifd, nx, ny, nc, y, n, remain;
	unsigned char *buf;
	PICFILE *out;
	if(getflags(argc, argv, "t:1[type]h:1[nheader]n:1[name]")!=5)
		usage("file xsize ysize chan");
	ifd=open(argv[1], OREAD);
	if(ifd==-1){
		fprint(2, "%s: can't open %s\n", argv[0], argv[1]);
		exits("no open");
	}
	if(flag['h']) seek(ifd, atoi(flag['h'][0]), 0);
	nx=atoi(argv[2]);
	ny=atoi(argv[3]);
	nc=strlen(argv[4]);
	buf=malloc(nx*nc);
	if(buf==0){
		fprint(2, "%s: can't malloc\n", argv[0]);
		exits("no space");
	}
	out=picopen_w("OUT", flag['t']?flag['t'][0]:"runcode", 0, 0, nx, ny, argv[4], 0, 0);
	if(flag['n']) picputprop(out, "NAME", flag['n'][0]);
	else picputprop(out, "NAME", argv[1]);
	for(y=0;y!=ny;y++){
		for(remain=nx*nc;remain!=0;remain-=n){
			n=read(ifd, buf+nx*nc-remain, remain);
			if(n<=0){
				fprint(2, "%s: %s short (hgt=%d)\n", argv[0], argv[1], y);
				exits("short input");
			}
		}
		picwrite(out, buf);
	}
}
