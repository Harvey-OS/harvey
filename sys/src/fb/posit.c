/*% cc -gpo # % -lcv
 * posit -- composite a bunch of pictures
 * Bug: chan must match for all pictures
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
PICFILE **in, *out;
int nin;
unsigned char *acc, *line;
Rectangle owin;
Rectangle both(Rectangle r, Rectangle s){
	if(s.min.x<r.min.x) r.min.x=s.min.x;
	if(s.min.y<r.min.y) r.min.y=s.min.y;
	if(s.max.x>r.max.x) r.max.x=s.max.x;
	if(s.max.y>r.max.y) r.max.y=s.max.y;
	return r;
}
int rgb=0;
main(int argc, char *argv[]){
	unsigned char *ap, *lp, *end, *epix;
	int i, j, k, err=0, nchan, alfoffs, alf, noalf;
	argc=getflags(argc, argv, "");
	if(argc<2) usage("file ...");
	nin=argc-1;
	in=malloc(nin*sizeof(PICFILE *));
	if(in==0){
		fprint(2, "%s: can't malloc file pointers!\n", argv[0]);
		exits("many files");
	}
	for(i=0;i!=nin;i++){
		if((in[i]=picopen_r(argv[i+1]))==0){
			perror(argv[i+1]);
			err++;
		}
		if(strcmp(in[i]->chan, in[0]->chan)!=0){
			fprint(2, "%s: %s has CHAN=%s, and %s has CHAN=%s\n", argv[0],
				argv[1], in[0]->chan,
				argv[i+1], in[i]->chan);
			err++;
		}
	}
	if(err)
		exits("bad input");
	owin.min.x=PIC_XOFFS(in[0]);
	owin.min.y=PIC_YOFFS(in[0]);
	owin.max=add(owin.min, Pt(PIC_WIDTH(in[0]), PIC_HEIGHT(in[0])));
	for(i=1;i!=nin;i++)
		owin=both(owin, PIC_RECT(in[i]));
	nchan=PIC_NCHAN(in[0]);
	acc=(unsigned char *)malloc((owin.max.x-owin.min.x)*nchan);
	if(acc==0){
	NoMemory:
		fprint(2, "%s: out of memory\n", argv[0]);
		exits("no space");
	}
	line=(unsigned char *)malloc((owin.max.x-owin.min.x)*nchan);
	if(line==0) goto NoMemory;
	if((out=picopen_w("OUT", in[0]->type,
		owin.min.x, owin.min.y, owin.max.x-owin.min.x, owin.max.y-owin.min.y,
		in[0]->chan, argv, (char *)0))==0){
		perror(argv[0]);
		exits("create output");
	}
	noalf=0;
	for(alfoffs=0;in[0]->chan[alfoffs]!='a';alfoffs++){
		if(in[0]->chan[alfoffs]=='\0'){
			noalf++;
			break;
		}
	}
	for(i=owin.min.y;i!=owin.max.y;i++){
		end=&acc[PIC_WIDTH(out)*nchan];
		memset(acc, 0, end-acc);
		for(j=0;j!=nin;j++){
			if(PIC_YOFFS(in[j])<=i && i<PIC_YOFFS(in[j])+PIC_HEIGHT(in[j])){
				if(!picread(in[j], (char *)line)) break;
				end=&line[PIC_WIDTH(in[j])*nchan];
				ap=&acc[(PIC_XOFFS(in[j])-PIC_XOFFS(out))*nchan];
				if(noalf)
					for(lp=line;lp!=end;) *ap++ = *lp++;
				else{
					for(lp=line;lp!=end;){
						alf=255-ap[alfoffs];
						epix=ap+nchan;
						while(ap!=epix)
							*ap++ += *lp++ * alf/255;
					}
				}
			}
		}
		picwrite(out, (char *)acc);
	}
	exits("");
}
