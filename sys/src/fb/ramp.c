#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
main(int argc, char *argv[]){
	int r0, g0, b0, r1, g1, b1;
	char *line, *chan;
	int x, y, x0, y0, wid, hgt;
	int r, g, b;
	PICFILE *f;
	argc=getflags(argc, argv, "w:4[x0 y0 x1 y1]v");
	switch(argc){
	default:
		usage("[[leftcolor] rightcolor]");
		break;
	case 1:
		r0=0;
		r1=255;
		chan="m";
		break;
	case 2:
		r0=0;
		r1=atoi(argv[1]);
		chan="m";
		break;
	case 3:
		r0=atoi(argv[1]);
		r1=atoi(argv[2]);
		chan="m";
		break;
	case 4:
		r0=g0=b0=0;
		r1=atoi(argv[1]);
		g1=atoi(argv[2]);
		b1=atoi(argv[3]);
		chan="rgb";
		break;
	case 7:
		r0=atoi(argv[1]);
		g0=atoi(argv[2]);
		b0=atoi(argv[3]);
		r1=atoi(argv[4]);
		g1=atoi(argv[5]);
		b1=atoi(argv[6]);
		chan="rgb";
		break;
	}
	if(flag['w']){
		x0=atoi(flag['w'][0]);
		y0=atoi(flag['w'][1]);
		wid=atoi(flag['w'][2])-x0;
		hgt=atoi(flag['w'][3])-y0;
	}
	else{
		x0=0;
		y0=0;
		wid=1280;
		hgt=1024;
	}
	f=picopen_w("OUT", "runcode", x0, y0, wid, hgt, chan, 0, 0);
	switch(strlen(chan)){
	case 1:
		line=malloc(wid);
		if(line==0){
			fprint(2, "%s: no space\n", argv[0]);
			exits("no space");
		}
		if(flag['v']){
			for(y=0;y!=hgt;y++){
				memset(line, r0+(r1-r0)*y/(hgt-1), wid);
				picwrite(f, line);
			}
		}
		else{
			for(x=0;x!=wid;x++) line[x]=r0+(r1-r0)*x/(wid-1);
			for(y=0;y!=hgt;y++) picwrite(f, line);
		}
		break;
	case 3:
		line=malloc(3*wid);
		if(line==0){
			fprint(2, "%s: no space\n", argv[0]);
			exits("no space");
		}
		if(flag['v']){
			for(y=0;y!=hgt;y++){
				r=r0+(r1-r0)*y/(hgt-1);
				g=g0+(g1-g0)*y/(hgt-1);
				b=b0+(b1-b0)*y/(hgt-1);
				for(x=0;x!=wid;x++){
					line[3*x]=r;
					line[3*x+1]=g;
					line[3*x+2]=b;
				}
				picwrite(f, line);
			}
		}
		else{
			for(x=0;x!=wid;x++){
				line[3*x]=r0+(r1-r0)*x/(wid-1);
				line[3*x+1]=g0+(g1-g0)*x/(wid-1);
				line[3*x+1]=b0+(b1-b0)*x/(wid-1);
			}
			for(y=0;y!=hgt;y++) picwrite(f, line);
		}
		break;
	}
	exits(0);
}
