/*% cc -go # % libfb.a
 * transpose -- transpose a picture, and do other grid symmetries
 * Flags are:
 *	-d	reflect through descending diagonal (default)
 *	-a	reflect through ascending diagonal
 *	-v	reflect through vertical midline
 *	-h	reflect through horizontal midline
 *	-r	rotate right (clockwise 90 degrees)
 *	-l	rotate left (counterclockwise)
 *	-u	rotate upside down
 *	-i	identity transformation (for completeness only)
 *	-o x y	translate by (x,y). without -o, input and output have the same upper-left corner.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
char flagnames[]="vhadrlui";
void transline(char *in, int nin, int nchan, char *out, int oskip){
	register char *p, *ep, *q;
	p=in;
	for(;nin!=0;--nin){
		ep=p+nchan;
		q=out;
		while(p!=ep) *q++ = *p++;
		out+=oskip;
	}
}
main(int argc, char *argv[]){
	PICFILE *in, *out;
	char *frame, *line, *fp;
	int y, dfp, skip, kind='d', nset=0;
	int iwid, ihgt, nchan, owid, ohgt, dx, dy;
	if((argc=getflags(argc, argv, "vhadrluio:2[x y]"))!=1 && argc!=2) usage("[picture]");
	if(flag['o']){
		dx=atoi(flag['o'][0]);
		dy=atoi(flag['o'][1]);
	}
	else
		dx=dy=0;
	for(y=0;flagnames[y];y++) if(flag[flagnames[y]]){
		kind=flagnames[y];
		nset++;
	}
	if(nset>1){
		fprint(2, "%s: can have at most one flag set\n", argv[0]);
		exits("bad flags");
	}
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	iwid=PIC_WIDTH(in);
	ihgt=PIC_HEIGHT(in);
	nchan=PIC_NCHAN(in);
	frame=malloc(iwid*ihgt*nchan);
	if(frame==0){
		fprint(2, "%s: no space for image buffer\n", argv[0]);
		exits("no space");
	}
	line=malloc(iwid*nchan);
	if(line==0){
		fprint(2, "%s: no space for line buffer\n", argv[0]);
		exits("no space");
	}
	switch(kind){
	case 'a':
		owid=ihgt;
		ohgt=iwid;
		fp=frame+(owid*ohgt-1)*nchan;
		dfp=-nchan;
		skip=-nchan*owid;
		break;
	case 'd':
		owid=ihgt;
		ohgt=iwid;
		fp=frame;
		dfp=nchan;
		skip=nchan*owid;
		break;
	case 'h':
		owid=iwid;
		ohgt=ihgt;
		fp=frame+owid*(ohgt-1)*nchan;
		dfp=-owid*nchan;
		skip=nchan;
		break;
	case 'v':
		owid=iwid;
		ohgt=ihgt;
		fp=frame+(owid-1)*nchan;
		dfp=owid*nchan;
		skip=-nchan;
		break;
	case 'r':
		owid=ihgt;
		ohgt=iwid;
		fp=frame+(owid-1)*nchan;
		dfp=-nchan;
		skip=owid*nchan;
		break;
	case 'l':
		owid=ihgt;
		ohgt=iwid;
		fp=frame+owid*(ohgt-1)*nchan;
		dfp=nchan;
		skip=-owid*nchan;
		break;
	case 'u':
		owid=iwid;
		ohgt=ihgt;
		fp=frame+(owid*ohgt-1)*nchan;
		dfp=-owid*nchan;
		skip=-nchan;
		break;
	case 'i':
		owid=iwid;
		ohgt=ihgt;
		fp=frame;
		dfp=nchan*owid;
		skip=nchan;
		break;
	}
	out=picopen_w("OUT", in->type, PIC_XOFFS(in)+dx, PIC_YOFFS(in)+dy, owid, ohgt,
		in->chan, argv, in->cmap);
	if(out==0){
		perror("OUT");
		exits("create output");
	}
	for(y=0;y!=ihgt;y++){
		picread(in, line);
		transline(line, iwid, nchan, fp, skip);
		fp+=dfp;
	}
	for(y=0;y!=PIC_HEIGHT(out);y++)
		picwrite(out, frame+y*PIC_WIDTH(out)*PIC_NCHAN(out));
	exits("");
}
