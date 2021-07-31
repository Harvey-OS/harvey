#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int getchnum(char *chan, int c){
	char *cp;
	cp=strchr(chan, c);
	if(cp==0){
		fprint(2, "%s: no channel %c in %s\n", cmdname, c, chan);
		exits("no channel");
	}
	return cp-chan;
}
void main(int argc, char *argv[]){
	uchar *buf, *bp, *cp;
	char *input, *chan;
	PICFILE *in, *out;
	int chnum, val, x, y, nx, ny, nchan, yinc;
	Rectangle r;
	switch(getflags(argc, argv, "sc:1[channel] v:1[value]")){
	default: usage("[picture]");
	case 1: input="IN"; break;
	case 2: input=argv[1]; break;
	}
	in=picopen_r(input);
	if(in==0){
		perror(input);
		exits("no picopen");
	}
	chan=in->chan;
	if(flag['v']) val=atoi(flag['v'][0]); else val=0;
	if(flag['c']) chnum=getchnum(chan, flag['c'][0][0]);
	else if(strchr(chan, 'a')) chnum=getchnum(chan, 'a');
	else if(strchr(chan, 'm')) chnum=getchnum(chan, 'm');
	else chnum=0;
	nx=PIC_WIDTH(in);
	ny=PIC_HEIGHT(in);
	nchan=PIC_NCHAN(in);
	r=Rect(nx,ny,0,0);
	if(flag['s']){
		buf=malloc(nx*ny*nchan);
		yinc=nx*nchan;
	}
	else{
		buf=malloc(nx*nchan);
		yinc=0;
	}
	if(buf==0){
		fprint(2, "%s: can't malloc!\n", argv[0]);
		exits("no mem");
	}
	for(y=0,bp=buf;y!=ny;y++,bp+=yinc){
		if(!picread(in, bp)) break;
		for(x=0,cp=bp+chnum;x!=nx;x++,cp+=nchan) if(*cp!=val){
			if(y<r.min.y) r.min.y=y;
			if(y>=r.max.y) r.max.y=y+1;
			if(x<r.min.x) r.min.x=x;
			if(x>=r.max.x) r.max.x=x+1;
		}
	}
	if(r.min.x>r.max.x) r.min.x=r.max.x;
	if(r.min.y>r.max.y) r.min.y=r.max.y;
	if(flag['s']){
		out=picopen_w("OUT", in->type, PIC_XOFFS(in), PIC_YOFFS(in),
			r.max.x-r.min.x, r.max.y-r.min.y, in->chan, 0, in->cmap);
		if(out==0){
			perror("OUT");
			exits("can't picopen_w");
		}
		bp=buf+(r.min.y*nx+r.min.x)*nchan;
		for(y=r.min.y;y!=r.max.y;y++,bp+=yinc)
			picwrite(out, bp);
	}
	else{
		r=raddp(r, Pt(PIC_XOFFS(in), PIC_YOFFS(in)));
		print("%d %d %d %d\n", r.min.x, r.min.y, r.max.x, r.max.y);
	}
	exits(0);
}
