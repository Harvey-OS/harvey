#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
Bitmap *rdpicfile(PICFILE *f, int ldepth){
	int wid=PIC_WIDTH(f);
	int hgt=PIC_HEIGHT(f);
	int nchan=PIC_NCHAN(f);
	int w=1<<ldepth;
	int nval=1<<w;
	Bitmap *b;
	int y;
	char *buf, *p;
	int lum, value, shift, pv;
	uchar data[1024], *dp;
	int *error, *ep, *eerror, cerror;
	buf=malloc(wid*nchan);
	if(buf==0) return 0;
	error=malloc((wid+2)*sizeof(int));
	eerror=error+wid+1;
	if(error==0){
		free(buf);
		return 0;
	}
	b=balloc(Rect(0, 0, wid, hgt), ldepth);
	if(b==0){
		free(buf);
		free(error);
		return 0;
	}
	switch(nchan){
	case 3:
	case 4:
	case 7:
	case 8:
		lum=1;
		break;
	default:
		lum=0;
	}
	for(ep=error;ep<=eerror;ep++) *ep=0;
	for(y=0;y!=hgt;y++){
		cerror=0;
		picread(f, buf);
		p=buf;
		dp=data-1;
		shift=0;
		for(ep=error+1;ep!=eerror;ep++){
			shift-=w;
			if(shift<0){
				shift+=8;
				*++dp=0;
			}
			if(lum)
				value=((p[0]&255)*299+(p[1]&255)*587+(p[2]&255)*114)/1000;
			else
				value=p[0]&255;
			value+=ep[0]/16;
			pv=value<=0?0:value<255?value*nval/256:nval-1;
			p+=nchan;
			*dp|=(nval-1-pv)<<shift;
			value-=pv*255/(nval-1);
			ep[1]+=value*7;
			ep[-1]+=value*3;
			ep[0]=cerror+value*5;
			cerror=value;
		}
		wrbitmap(b, y, y+1, data);
	}
	free(buf);
	free(error);
	return b;
}
