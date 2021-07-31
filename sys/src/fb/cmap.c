/*
 * Map an rgb image through the given color map
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
main(int argc, char *argv[]){
	char *p, *ep;
	unsigned char cmap[256][3];
	char *inname, *line;
	PICFILE *in, *out;
	int rgboffs, i;
	char *chan;
	switch(getflags(argc, argv, "")){
	case 3: inname=argv[2]; break;
	case 2: inname="IN"; break;
	default:usage("colormap [picture]");
	}
	if(!getcmap(argv[1], (unsigned char *)cmap)){
		fprint(2, "%s: can't get color map %s\n", argv[0], argv[1]);
		exits("get cmap");
	}
	in=picopen_r(inname);
	if(in==0){
		picerror(inname);
		exits("open input");
	}
	line=malloc(PIC_WIDTH(in)*PIC_NCHAN(in));
	if(line==0){
		fprint(2, "%s: no space\n", argv[0]);
		exits("no space");
	}
	chan=in->chan;
	for(rgboffs=0;strcmp(chan+rgboffs, "rgb")!=0;rgboffs++){
		if(chan[rgboffs]=='\0'){
			fprint(2, "%s: %s not rgb (CHAN=%s)\n", argv[0], inname, chan);
			exits("not rgb");
		}
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		picerror(argv[0]);
		exits("create output");
	}
	for(i=0;i!=PIC_HEIGHT(in);i++){
		picread(in, line);
		ep=line+rgboffs+PIC_WIDTH(in)*PIC_NCHAN(in);
		for(p=line+rgboffs;p!=ep;p+=PIC_NCHAN(in)){
			p[0]=cmap[p[0]&255][0];
			p[1]=cmap[p[1]&255][1];
			p[2]=cmap[p[2]&255][2];
		}
		picwrite(out, line);
	}
	exits("");
}
