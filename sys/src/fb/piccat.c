/*
 * piccat -- vertical picture concatenation
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NIN	150
main(int argc, char *argv[]){
	int y;
	char *buf;
	int nin, i, wid, hgt, bg;
	PICFILE *in[NIN];
	PICFILE *out;
	argc=getflags(argc, argv, "b:1[background]");
	nin=argc-1;
	if(nin<1) usage("picture ...");
	if(nin>NIN){
		fprint(2, "%s: too many inputs\n", argv[0]);
		exits("too many files");
	}
	bg=flag['b']?atoi(flag['b'][0]):0;
	wid=0;
	hgt=0;
	for(i=0;i!=nin;i++){
		in[i]=picopen_r(argv[i+1]);
		if(in[i]==0){
			perror(argv[i+1]);
			exits("open input");
		}
		if(PIC_WIDTH(in[i])>wid) wid=PIC_WIDTH(in[i]);
		hgt+=PIC_HEIGHT(in[i]);
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
		perror(argv[0]);
		exits("open output");
	}
	for(i=0;i!=nin;i++){
		memset(buf, bg, wid*PIC_NCHAN(in[0]));
		for(y=0;y!=PIC_HEIGHT(in[i]);y++){
			picread(in[i], buf);
			picwrite(out, buf);
		}
	}
	exits("");
}
