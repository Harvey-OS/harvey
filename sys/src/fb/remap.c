/*
 * Remap an rgb image to use a given color map
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int abs(int a){ return a<0?-a:a; }
/*
 * This should sort and use binary search
 */
void cvtmap(unsigned char inmap[256][3], unsigned char outmap[256][3], unsigned char remap[256][3]){
	register i, j, k, err, target, index, v;
	for(i=0;i!=256;i++){
		for(j=0;j!=3;j++){
			target=inmap[i][j];
			v=outmap[0][j];
			index=0;
			err=abs(v-target);
			for(k=1;k!=256;k++){
				if(abs(outmap[k][j]-target)<err){
					v=outmap[k][j];
					index=k;
					err=abs(v-target);
				}
			}
			remap[i][j]=index;
		}
	}
}
main(int argc, char *argv[]){
	char *p, *ep;
	unsigned char cmap[256][3], monomap[256][3], remap[256][3];
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
		perror(inname);
		exits("open input");
	}
	line=malloc(PIC_WIDTH(in)*PIC_NCHAN(in));
	if(line==0){
		fprint(2, "%s: no space\n", argv[0]);
		exits("no space");
	}
	chan=in->chan;
	for(rgboffs=0;strncmp(chan+rgboffs, "rgb", 3)!=0;rgboffs++){
		if(chan[rgboffs]=='\0'){
			fprint(2, "%s: %s not rgb (CHAN=%s)\n", argv[0], inname, chan);
			exits("bad chan");
		}
	}
	out=picopen_w("OUT", in->type,
		PIC_XOFFS(in), PIC_YOFFS(in), PIC_WIDTH(in), PIC_HEIGHT(in),
		"m", argv, (char *)cmap);
	if(out==0){
		perror(argv[0]);
		exits("create output");
	}
	if(in->cmap==0){
		for(i=0;i!=256;i++) monomap[i][0]=monomap[i][1]=monomap[i][2]=i;
		cvtmap(monomap, cmap, remap);
	}
	else
		cvtmap((unsigned char (*)[3])in->cmap, cmap, remap);
	for(i=0;i!=PIC_HEIGHT(in);i++){
		picread(in, line);
		ep=line+rgboffs+PIC_WIDTH(in)*PIC_NCHAN(in);
		for(p=line+rgboffs;p!=ep;p+=PIC_NCHAN(in)){
			p[0]=remap[p[0]&255][0];
			p[1]=remap[p[1]&255][1];
			p[2]=remap[p[2]&255][2];
		}
		picwrite(out, line);
	}
	exits("");
}
