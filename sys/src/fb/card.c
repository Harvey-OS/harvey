/*
 * card -- make a picture file filled with a given color
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
void clrline(char *bp, int r, int g, int b, int a, char *chan, int wid){
	char *ebuf=bp+strlen(chan)*wid, *cp;
	int m=(299*r+587*g+114*b)/1000;
	while(bp!=ebuf){
		for(cp=chan;*cp;cp++) switch(*cp){
		default:  *bp++=0; break;
		case 'r': *bp++=r; break;
		case 'g': *bp++=g; break;
		case 'b': *bp++=b; break;
		case 'a': *bp++=a; break;
		case 'm': *bp++=m; break;
		}
	}
}
main(int argc, char *argv[]){
	int r, g, b, a, y;
	int x0, y0, wid, hgt;
	char *line, *chan;
	PICFILE *out;
	switch(getflags(argc, argv, "c:1[rgba]w:4[x0 y0 x1 y1]")){
	default:
		usage("red [green blue] [alpha]");
	case 5:
		chan="rgba";
		r=atoi(argv[1]);
		g=atoi(argv[2]);
		b=atoi(argv[3]);
		a=atoi(argv[4]);
		break;
	case 4:
		chan="rgb";
		r=atoi(argv[1]);
		g=atoi(argv[2]);
		b=atoi(argv[3]);
		a=255;
		break;
	case 3:
		chan="ma";
		r=g=b=atoi(argv[1]);
		a=atoi(argv[2]);
		break;
	case 2:
		chan="m";
		r=g=b=atoi(argv[1]);
		a=255;
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
	if(flag['c']) chan=flag['c'][0];
	line=malloc(wid*strlen(chan));
	if(line==0){
		fprint(2, "%s: out of memory\n", argv[0]);
		exits("no space");
	}
	clrline(line, r, g, b, a, chan, wid);
	out=picopen_w("OUT", "runcode", x0, y0, wid, hgt, chan, (char **)0, (char *)0);
	if(out==0){
		picerror(argv[0]);
		exits("create output");
	}
	for(y=0;y!=hgt;y++) picwrite(out, line);
	exits("");
}
