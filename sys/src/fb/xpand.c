/*% cyntax % -lfb && cc -go # % -lfb
 * xpand -- adjust dynamic range
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int lo, hi, inlo, inhi;
void range(char *, long, int);
void mkmap(void);
int clamp(int);
void map(char *, long, int);
main(int argc, char *argv[]){
	char *pic;
	int y, nx, i, examine;
	long npix;
	PICFILE *in, *out;
	char *inname="IN";
	argc=getflags(argc, argv, "s");
	switch(argc){
	default:
		usage("[in] [lo hi [inlo inhi]]");
	case 2:
		inname=argv[1];
	case 1:
		lo=0;
		hi=255;
		examine=1;
		break;
	case 4:
		inname=argv[1];
	case 3:
		lo=atoi(argv[argc-2]);
		hi=atoi(argv[argc-1]);
		examine=1;
		break;
	case 6:
		inname=argv[1];
	case 5:
		lo=atoi(argv[argc-4]);
		hi=atoi(argv[argc-3]);
		inlo=atoi(argv[argc-2]);
		inhi=atoi(argv[argc-1]);
		examine=0;
		break;
	}
	in=picopen_r(inname);
	if(in==0){
		picerror(inname);
		exits("open input");
	}
	npix=PIC_WIDTH(in)*PIC_HEIGHT(in);
	pic=(char *)malloc(npix*PIC_NCHAN(in));
	if(pic==0){
		fprint(2, "%s: out of space\n", argv[0]);
		exits("no space");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		picerror("OUT");
		exits("create output");
	}
	nx=PIC_WIDTH(in)*PIC_NCHAN(in);
	for(y=0;y!=PIC_HEIGHT(in);y++)
		picread(in, pic+y*nx);
	if(flag['s']){
		if(examine) range(pic, npix*PIC_NCHAN(in), 1);
		mkmap();
		map(pic, npix*PIC_NCHAN(in), 1);
	}
	else{
		for(i=0;i!=PIC_NCHAN(in);i++){
			if(examine) range(pic+i, npix, PIC_NCHAN(in));
			mkmap();
			map(pic+i, npix, PIC_NCHAN(in));
		}
	}
	for(y=0;y!=PIC_HEIGHT(in);y++)
		picwrite(out, pic+y*nx);
	exits("");
}
char xpand[256];
/*
 * find the range of a byte array;
 */
void range(char *p, long n, int step){
	char *ep=p+n*step;
	int v;
	inlo=255;
	inhi=0;
	do{
		v=*p&255;
		if(v<inlo) inlo=v;
		if(v>inhi) inhi=v;
	}while((p+=step)!=ep);
}
/*
 * Make an expanding lookup table
 */
void mkmap(void){
	register i;
	if(inhi<inlo){
		i=inlo; inlo=inhi; inhi=i;
		i=lo; lo=hi; hi=i;
	}
	for(i=0;i!=256;i++){
		if(i<=inlo) xpand[i]=clamp(lo);
		else if(i<=inhi) xpand[i]=clamp(lo+(hi-lo)*(i-inlo)/(inhi-inlo));
		else xpand[i]=clamp(hi);
	}
}
int clamp(int a){ return a<=0?0:a<=255?a:255; }
/*
 * Look up bytes in the lookup table
 */
void map(char *p, long n, int step){
	register char *ep=p+n*step;
	do *p=xpand[*p&255]; while((p+=step)!=ep);
}
