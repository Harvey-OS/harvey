/*% cyntax % && cc -go # % -lpicfile
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int stripe(char *p0, char *p1, int c){
	if(p0==p1){
		if(c>255){
			*p0=255;
			return c-255;
		}
		*p0=c;
		return 0;
	}
	if(c>2*255){
		*p0=255;
		*p1=255;
		return c-2*255;
	}
	*p0=c/2;
	*p1=c-c/2;
	return 0;
}
main(int argc, char *argv[]){
	char *p0, *p1, *e0;
	int y, y0, hgt, x, lwid, i, c;
	PICFILE *in, *out;
	char *inf, *lines;
	switch(getflags(argc, argv, "ld")){	/* l for light foreground, d for dark */
	case 2: inf="IN"; break;
	case 3: inf=argv[2]; break;
	default: usage("height [file]");
	}
	in=picopen_r(inf);
	if(in==0){
		picerror(inf);
		exits("open input");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	hgt=atoi(argv[1]);
	lwid=PIC_NCHAN(in)*PIC_WIDTH(in);
	lines=malloc(lwid*hgt);
	if(lines==0){
		fprint(2, "%s: out of space\n", argv[0]);
		exits("no space");
	}
	for(y=0;y!=PIC_HEIGHT(in);y+=hgt){
		if(y+hgt>PIC_HEIGHT(in)) hgt=PIC_HEIGHT(in)-y;
		for(i=0;i!=hgt;i++) picread(in, lines+i*lwid);
		for(x=0;x!=PIC_WIDTH(in);x++) for(i=0;i!=PIC_NCHAN(in);i++){
			c=0;
			for(y0=0;y0!=hgt;y0++) c+=(lines[y0*lwid+x*PIC_NCHAN(in)+i])&255;
			if(flag['l']){
				e0=&lines[x*PIC_NCHAN(in)+i];
				p0=e0+lwid*((hgt-1)/2);
				p1=e0+lwid*(hgt-1-(hgt-1)/2);
				for(;p0>=e0;p0-=lwid,p1+=lwid) c=stripe(p0, p1, c);
			}
			else{
				p0=&lines[x*PIC_NCHAN(in)+i];
				p1=p0+lwid*(hgt-1);
				for(;p0<=p1;p0+=lwid,p1-=lwid) c=stripe(p0, p1, c);
			}
		}
		for(i=0;i!=hgt;i++) picwrite(out, lines+i*lwid);
	}
	exits("");
}
