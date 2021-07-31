/*
 * Compute luminance
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
char *ochan(char *chan){
	if(chan[0]=='r' && chan[1]=='g' && chan[2]=='b'){
		chan[2]='m';	/* Ooh, careful! */
		return chan+2;
	}
	return chan;
}
main(int argc, char *argv[]){
	PICFILE *in, *out;
	char *inbuf, *outbuf, *end, *p, *q, *next;
	char lum[256];
	int i, y;
	if((argc=getflags(argc, argv, ""))!=1 && argc!=2) usage("[picture]");
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		picerror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	inbuf=malloc(PIC_NCHAN(in)*PIC_WIDTH(in));
	if(inbuf==0){
	NoSpace:
		fprint(2, "%s: out of space\n", argv[0]);
		exits("no space");
	}
	out=picopen_w("OUT", in->type,
		PIC_XOFFS(in), PIC_YOFFS(in), PIC_WIDTH(in), PIC_HEIGHT(in),
		ochan(in->chan), argv, (char *)0);
	outbuf=malloc(PIC_NCHAN(out)*PIC_WIDTH(out));
	if(outbuf==0) goto NoSpace;
	if(out==0){
		picerror("OUT");
		exits("create output");
	}
	end=inbuf+PIC_NCHAN(in)*PIC_WIDTH(in);
	if(PIC_NCHAN(in)==PIC_NCHAN(out)){
		if(in->cmap){
			for(i=0;i!=256;i++)
				lum[i]=((in->cmap[3*i]&255)*299
					+(in->cmap[3*i+1]&255)*587
					+(in->cmap[3*i+2]&255)*114)/1000;
		}
		else{
			for(i=0;i!=256;i++) lum[i]=i;
		}
		for(y=0;y!=PIC_HEIGHT(in);y++){
			picread(in, inbuf);
			for(p=inbuf;p!=end;p+=PIC_NCHAN(in)) *p=lum[*p&255];
			picwrite(out, inbuf);
		}
	}
	else{
		for(y=0;y!=PIC_HEIGHT(in);y++){
			picread(in, inbuf);
			p=inbuf;
			q=outbuf;
			while(p!=end){
				next=q+PIC_NCHAN(out);
				*q++=((p[0]&255)*299+(p[1]&255)*587+(p[2]&255)*114)/1000;
				p+=3;
				while(q!=next) *q++ = *p++;
			}
			picwrite(out, outbuf);
		}
	}
	exits("");
}
