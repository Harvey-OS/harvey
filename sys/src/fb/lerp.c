/*
 * lerp -- linear combinations of pictures
 * Bug: chan must match for all pictures
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NIN	150		/* maximum number of input files */
PICFILE *in[NIN], *out;
#define	SHIFT	16		/* non-portable (ints may be too short to avoid overflow) */
#define	SCALE	(1<<SHIFT)
int lerp[NIN];
int nin;
unsigned char *line;
int *acc;
Rectangle owin;
Rectangle both(Rectangle r, Rectangle s){
	if(s.min.x<r.min.x) r.min.x=s.min.x;
	if(s.min.y<r.min.y) r.min.y=s.min.y;
	if(s.max.x>r.max.x) r.max.x=s.max.x;
	if(s.max.y>r.max.y) r.max.y=s.max.y;
	return r;
}
main(int argc, char *argv[]){
	int *ap;
	unsigned char *lp, *end, *epix;
	int i, j, k, err=0, nchan, t, total;
	double alf;
	argc=getflags(argc, argv, "");
	if(argc<2) usage("file fraction ... [file]");
	nin=argc/2;
	if(nin>NIN){
		fprint(2, "%s: sorry, too many files\n", argv[0]);
		exits("too many files");
	}
	total=0;
	for(i=0;i!=nin;i++){
		if((in[i]=picopen_r(argv[2*i+1]))==0){
			picerror(argv[i+1]);
			err++;
		}
		if(strcmp(in[i]->chan, in[0]->chan)!=0){
			fprint(2, "%s: %s has CHAN=%s, and %s has CHAN=%s\n", argv[0],
				argv[1], in[0]->chan,
				argv[i+1], in[i]->chan);
			err++;
		}
		if(argc>2*i+2){
			lerp[i]=atof(argv[2*i+2])*SCALE;
			total+=lerp[i];
		}
		else
			lerp[i]=SCALE-total;
	}
	if(err)
		exits("bad input");
	owin.min.x=PIC_XOFFS(in[0]);
	owin.min.y=PIC_YOFFS(in[0]);
	owin.max=add(owin.min, Pt(PIC_WIDTH(in[0]), PIC_HEIGHT(in[0])));
	for(i=1;i!=nin;i++)
		owin=both(owin, PIC_RECT(in[i]));
	nchan=PIC_NCHAN(in[0]);
	acc=(int *)malloc((owin.max.x-owin.min.x)*nchan*sizeof(int));
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
		picerror(argv[0]);
		exits("create output");
	}
	for(i=owin.min.y;i!=owin.max.y;i++){
		memset(acc, 0, PIC_WIDTH(out)*nchan*sizeof(int));
		for(j=0;j!=nin;j++){
			if(PIC_YOFFS(in[j])<=i && i<=PIC_YOFFS(in[j])+PIC_HEIGHT(in[j])){
				if(!picread(in[j], (char *)line)) break;
				end=&line[PIC_WIDTH(in[j])*nchan];
				ap=&acc[(PIC_XOFFS(in[j])-PIC_XOFFS(out))*nchan];
				alf=lerp[j];
				for(lp=line;lp!=end;)
/*				was:	*ap++ += *lp++ * alf;	but 8c died */
					*ap++ += (int)(*lp++ * alf);
			}
		}
		ap=acc;
		end=&line[PIC_WIDTH(out)*nchan];
		for(lp=line;lp!=end;){
			t=*ap++;
			*lp++ = t<0?0:t<255*SCALE?t>>SHIFT:255;
		}
		picwrite(out, (char *)line);
	}
	exits("");
}
