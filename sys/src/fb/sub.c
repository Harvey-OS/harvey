/*
 * sub -- subtract a pair of images
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
main(int argc, char *argv[]){
	PICFILE *a, *b, *out;
	unsigned char *abuf, *bbuf, *outbuf, *ap, *bp, *outp, *eout;
	int y, nc, ny;
	argc=getflags(argc, argv, "");
	if(argc!=3) usage("file file");
	a=picopen_r(argv[1]);
	b=picopen_r(argv[2]);
	out=picopen_w("OUT", PIC_SAMEARGS(a));
	if(a==0 || b==0 || out==0){
		if(a==0) perror(argv[1]);
		if(b==0) perror(argv[2]);
		if(out==0) perror("OUT");
		exits("no input");
	}
	if(PIC_WIDTH(a)!=PIC_WIDTH(b)
	|| PIC_HEIGHT(a)!=PIC_HEIGHT(b)
	|| PIC_NCHAN(a)!=PIC_NCHAN(b)){
		fprint(2, "%s: input dimensions don't match\n", argv[0]);
		exits("conformability");
	}
	nc=PIC_WIDTH(a)*PIC_NCHAN(a);
	ny=PIC_HEIGHT(a);
	abuf=malloc(nc);
	bbuf=malloc(nc);
	outbuf=malloc(nc);
	if(abuf==0 || bbuf==0 || outbuf==0){
		fprint(2, "%s: can't malloc\n", argv[0]);
		exits("no mem");
	}
	eout=outbuf+nc;
	for(y=0;y!=ny;y++){
		picread(a, abuf);
		picread(b, bbuf);
		for(ap=abuf,bp=bbuf,outp=outbuf;outp!=eout;ap++,bp++,outp++)
			*outp=*ap>*bp?*ap-*bp:*bp-*ap;
		picwrite(out, outbuf);
	}
	exits(0);
}
