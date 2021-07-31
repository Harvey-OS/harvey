#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
void main(int argc, char *argv[]){
	char *pname;
	int sx, sy, wid, hgt, nchan, x, y, i;
	char *ibuf, *obuf, *ip, *op;
	PICFILE *in, *out;
	switch(getflags(argc, argv, "")){
	default: usage("xscale yscale [file]");
	case 3:  pname="IN"; break;
	case 4:  pname=argv[3]; break;
	}
	sx=atoi(argv[1]);
	sy=atoi(argv[2]);
	if(sx<=0 || sy<=0){
		fprint(2, "%s: scale must be positive\n", argv[0]);
		exits("range err");
	}
	in=picopen_r(pname);
	if(in==0){
		fprint(2, "%s: can't open %s\n", argv[0], pname);
		exits("no input");
	}
	wid=PIC_WIDTH(in);
	hgt=PIC_HEIGHT(in);
	nchan=PIC_NCHAN(in);
	ibuf=malloc(wid*nchan);
	obuf=sx==1?ibuf:malloc(wid*nchan*sx);
	if(ibuf==0 || obuf==0){
		fprint(2, "%s: no malloc\n", argv[0]);
		exits("no mem");
	}
	out=picopen_w("OUT", in->type, PIC_XOFFS(in)*sx, PIC_YOFFS(in)*sy,
		PIC_WIDTH(in)*sx, PIC_HEIGHT(in)*sy, in->chan, 0, in->cmap);
	if(out==0){
		fprint(2, "%s: can't open OUT!\n", argv[0]);
		exits("no OUT");
	}
	for(y=0;y!=hgt;y++){
		picread(in, ibuf);
		if(sx!=1){
			ip=ibuf;
			op=obuf;
			for(x=0;x!=wid;x++){
				for(i=0;i!=sx;i++){
					memmove(op, ip, nchan);
					op+=nchan;
				}
				ip+=nchan;
			}
		}
		for(i=0;i!=sy;i++) picwrite(out, obuf);
	}
	exits(0);
}
