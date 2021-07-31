/*
 * picjoin -- horizontal picture concatenation
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NIN	150
main(int argc, char *argv[]){
	int y;
	char *buf, *bp;
	int nin, i, wid, hgt;
	PICFILE *in[NIN];
	PICFILE *out;
	argc=getflags(argc, argv, "");
	nin=argc-1;
	if(nin<1) usage("picture ...");
	if(nin>NIN){
		fprint(2, "%s: too many inputs\n", argv[0]);
		exits("many files");
	}
	wid=0;
	hgt=0;
	for(i=0;i!=nin;i++){
		in[i]=picopen_r(argv[i+1]);
		if(in[i]==0){
			picerror(argv[i+1]);
			exits("open input");
		}
		if(PIC_HEIGHT(in[i])>hgt) hgt=PIC_WIDTH(in[i]);
		wid+=PIC_WIDTH(in[i]);
		if(PIC_NCHAN(in[i])!=PIC_NCHAN(in[0])){
			fprint(2, "%s: channel number mismatch\n", argv[0]);
			exits("nchan different");
		}
	}
	buf=malloc(wid*PIC_NCHAN(in[0]));
	if(buf==0){
		fprint(2, "%s: out of space\n", argv[0]);
		exits("no space");
	}
	out=picopen_w("OUT", in[0]->type, PIC_XOFFS(in[0]), PIC_YOFFS(in[0]), wid, hgt,
		in[0]->chan, argv, in[0]->cmap);
	if(out==0){
		picerror(argv[0]);
		exits("open output");
	}
	for(y=0;y!=hgt;y++){
		bp=buf;
		for(i=0;i!=nin;i++){
			if(y<PIC_HEIGHT(in[i]))
				picread(in[i], bp);
			else if(y==PIC_HEIGHT(in[i]))
				memset(bp, 0, PIC_WIDTH(in[i])*PIC_NCHAN(in[i]));
			bp+=PIC_WIDTH(in[i])*PIC_NCHAN(in[i]);
		}
		picwrite(out, buf);
	}
	exits("");
}
