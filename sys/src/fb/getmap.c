#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int nbit, npix;
RGB map[256];
/* replicate (from top) value in v (n bits) until it fills a ulong */
ulong rep(ulong v, int n){
	int o;
	ulong rv;
	rv=0;
	for(o=32-n;o>=0;o-=n) rv|=v<<o;
	if(o!=-n) rv|=v>>-o;
	return rv;
}
void putcmap(uchar cmap[256*3]){
	int i, j;
	for(j=0;j!=npix;j++){
		i=3*255*j/(npix-1);
		map[npix-1-j].red=rep(cmap[i], 8);
		map[npix-1-j].green=rep(cmap[i+1], 8);
		map[npix-1-j].blue=rep(cmap[i+2], 8);
	}
	wrcolmap(&screen, map);
}
void main(int argc, char *argv[]){
	uchar cmapbuf[256*3];
	if(argc!=2){
		fprint(2, "Usage: %s colormap\n", argv[0]);
		exits("usage");
	}
	if(!getcmap(argv[1], cmapbuf)){
		fprint(2, "%s: can't find %s\n", argv[0], argv[1]);
		exits("not found");
	}
	binit(0,0,0);
	nbit=1<<screen.ldepth;
	npix=1<<nbit;
	putcmap(cmapbuf);
	exits(0);
}
