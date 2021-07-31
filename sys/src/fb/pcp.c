/*
 * Copy files between picture files, converting formats, clipping, etc.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
struct picture{
	char *name;		/* name of picture file */
	PICFILE *pf;		/* picture file pointer */
	Rectangle picr;		/* rectangle in file or on screen */
	Rectangle r;		/* rectangle actually copied */
	char *chan;		/* argument of CHAN= */
	int nchan;		/* strlen(chan) */
	char *buf;		/* scanline buffer */
}in, out;
char *inoutp;			/* pointer to beginning of output line in input buffer */
int readp(int), writep(int);
void conversion(char *, char *, char *);
#define	NCVTROU	10
struct cvtrou{
	void (*rou)(int, int, int);
	int i, o;
}cvtrou[NCVTROU];
struct cvtrou *ecvtrou=cvtrou;
void cvt(void);
int needcmap=0;		/* does the output file need the incoming colormap? */
main(int argc, char *argv[]){
	char *type="runcode";
	char *cmap=0;
	Rectangle win;
	int t, y;
	char *ldepth;
	char cmapbuf[256*3];
	switch(getflags(argc, argv,
	    "w:4[x0 y0 x1 y1]o:2[x y]t:1[pictype]c:1[rgbaz...]C:1[rgbaz...]l:1[ldepth]m:1[colormap]M")){
	default:
		usage("[infile [outfile]]");
	case 3:
		in.name=argv[1];
		out.name=argv[2];
		break;
	case 2:
		in.name=argv[1];
		out.name="OUT";
		break;
	case 1:
		in.name="IN";
		out.name="OUT";
		break;
	}
	if(flag['m'] && flag['M']){
		fprint(2, "%s: -m requires a color map, -M forbids one!\n", argv[0]);
		exits("usage");
	}
	in.pf=picopen_r(in.name);
	if(in.pf==0){
		perror(in.name);
		exits("open input");
	}
	if(flag['l']) ldepth=flag['l'][0];
	else ldepth=picgetprop(in.pf, "LDEPTH");
	in.r.min.x=PIC_XOFFS(in.pf);
	in.r.min.y=PIC_YOFFS(in.pf);
	in.r.max=add(in.r.min, Pt(PIC_WIDTH(in.pf), PIC_HEIGHT(in.pf)));
	in.picr=in.r;
	in.chan=in.pf->chan;
	in.nchan=PIC_NCHAN(in.pf);
	if(in.chan[0]=='?') switch(in.nchan){
	case 1: in.chan="m"; break;
	case 2: in.chan="ma"; break;
	case 3: in.chan="rgb"; break;
	case 4: in.chan="rgba"; break;
	case 5: in.chan="mz..."; break;
	case 6: in.chan="maz..."; break;
	case 7: in.chan="rgbz..."; break;
	case 8: in.chan="rgbaz..."; break;
	}
	type=in.pf->type;
	cmap=in.pf->cmap;
	if(flag['t']) type=flag['t'][0];
	if(flag['w']){
		win.min.x=atoi(flag['w'][0]);
		win.min.y=atoi(flag['w'][1]);
		win.max.x=atoi(flag['w'][2]);
		win.max.y=atoi(flag['w'][3]);
		if(win.max.x<win.min.x){ t=win.min.x; win.min.x=win.max.x; win.max.x=t; }
		if(win.max.y<win.min.y){ t=win.min.y; win.min.y=win.max.y; win.max.y=t; }
		if(win.min.x>in.r.min.x) in.r.min.x=win.min.x;
		if(win.min.y>in.r.min.y) in.r.min.y=win.min.y;
		if(win.max.x<in.r.max.x) in.r.max.x=win.max.x;
		if(win.max.y<in.r.max.y) in.r.max.y=win.max.y;
		if(in.r.max.x<=in.r.min.x || in.r.max.y<=in.r.min.y){
			fprint(2, "Empty window\n");
			exits("empty window");
		}
	}
	out.r=in.r;
	if(flag['o']) out.r=raddp(out.r, Pt(atoi(flag['o'][0]), atoi(flag['o'][1])));
	out.chan=flag['c']?flag['c'][0]:in.chan;
	out.nchan=strlen(out.chan);
	conversion(in.chan, out.chan, cmap);
	if(flag['M']) needcmap=0;
	if(flag['m']){
		needcmap=1;
		getcmap(flag['m'][0], (uchar *)cmapbuf);
		cmap=cmapbuf;
	}
	if(flag['C']){
		if(strlen(flag['C'][0])!=out.nchan){
			fprint(2, "%s: -C argument wrong length (%s vs. %s)\n", argv[0],
				flag['C'][0], out.chan);
			exits("bad -C");
		}
		out.chan=flag['C'][0];
	}
	in.buf=malloc(in.nchan*(in.picr.max.x-in.picr.min.x));
	out.buf=malloc(out.nchan*(out.r.max.x-out.r.min.x));
	if(in.buf==0 || out.buf==0){
		fprint(2, "%s: out of space\n", argv[0]);
		exits("no space");
	}
	inoutp=in.buf+(in.r.min.x-in.picr.min.x)*in.nchan;
	out.pf=picopen_w(out.name, type,
		out.r.min.x, out.r.min.y, out.r.max.x-out.r.min.x, out.r.max.y-out.r.min.y,
		out.chan, 0, needcmap?cmap:0);
	if(ldepth) picputprop(out.pf, "LDEPTH", ldepth);
	if(out.pf==0){
		perror(out.name);
		exits("create output");
	}
	for(y=out.r.min.y;y!=out.r.max.y;y++){
		if(!readp(y-out.r.min.y+in.r.min.y)){
			fprint(2, "%s: %s short (last y=%d)\n", argv[0], in.name, y-1);
			break;
		}
		cvt();
		writep(y);
	}
	exits("");
}
int readp(int y){
	if(y==in.r.min.y){
		for(y=in.picr.min.y;y!=in.r.min.y;y++)
			picread(in.pf, in.buf);
	}
	return picread(in.pf, in.buf);
}
int writep(int y){
	if(!picwrite(out.pf, out.buf)){
		perror(out.name);
		exits("write error");
	}
}
void cvt(void){
	struct cvtrou *p;
	for(p=cvtrou;p!=ecvtrou;p++)
		(*p->rou)(out.r.max.x-out.r.min.x, p->i, p->o);
}
/*
 * No transformation: zap output buffer pointer, avoid copying
 */
void ident(int npix, int i, int o){
	out.buf=inoutp;
}
void cpchan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan;
	while(p!=ep){
		*q = *p;
		p+=in.nchan;
		q+=out.nchan;
	}
}
void cp3chan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan;
	while(p!=ep){
		q[0]=p[0];
		q[1]=p[1];
		q[2]=p[2];
		p+=in.nchan;
		q+=out.nchan;
	}
}
void cp4chan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=in.buf+npix*in.nchan;
	while(p!=ep){
		q[0]=p[0];
		q[1]=p[1];
		q[2]=p[2];
		q[3]=p[3];
		p+=in.nchan;
		q+=out.nchan;
	}
}
void lumchan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan;
	while(p!=ep){
		q[0]=((p[0]&255)*299+(p[1]&255)*587+(p[2]&255)*114)/1000;
		p+=in.nchan;
		q+=out.nchan;
	}
}
char cmap[256][3];
void mapchan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan, *r;
	while(p!=ep){
		r=cmap[p[0]&255];
		q[0]=r[0];
		q[1]=r[1];
		q[2]=r[2];
		p+=in.nchan;
		q+=out.nchan;
	}
}
void maprchan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan;
	while(p!=ep){
		q[0]=cmap[p[0]&255][0];
		p+=in.nchan;
		q+=out.nchan;
	}
}
void mapgchan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan;
	while(p!=ep){
		q[0]=cmap[p[0]&255][1];
		p+=in.nchan;
		q+=out.nchan;
	}
}
void mapbchan(int npix, int i, int o){
	register char *p=inoutp+i, *q=out.buf+o, *ep=p+npix*in.nchan;
	while(p!=ep){
		q[0]=cmap[p[0]&255][2];
		p+=in.nchan;
		q+=out.nchan;
	}
}
void rgb0chan(int npix, int i, int o){
	register char *q=out.buf+o, *eq=q+npix*out.nchan;
	while(q!=eq){
		q[0]=q[1]=q[2]=0;
		q+=out.nchan;
	}
}
void zerochan(int npix, int i, int o){
	register char *q=out.buf+o, *eq=q+npix*out.nchan;
	while(q!=eq){
		*q=0;
		q+=out.nchan;
	}
}
void a255chan(int npix, int i, int o){
	register char *q=out.buf+o, *eq=q+npix*out.nchan;
	while(q!=eq){
		*q=255;
		q+=out.nchan;
	}
}
void z1chan(int npix, int i, int o){
	register char *q=out.buf+o, *eq=q+npix*out.nchan;
	while(q!=eq){
		*(float *)q = 1.;	/* could suffer alignment death */
		q+=out.nchan;
	}
}
/*
 * The input picture has channels in, and colormap *cmapp.
 * The user has asked for channels out.  Figure out what to do.
 */
struct conv{
	char *new;			/* desired channels */
	char *old;			/* available channels */
	void (*rou)(int, int, int);	/* conversion routine */
	int savecmap;			/* should we copy the colormap? */
}conv[]={
	"rgba",	"rgba",	cp4chan, 1,
	"rgb",	"rgb",	cp3chan, 1,
	"rgb",	"m",	mapchan, 0,
	"rgb",	"r",	mapchan, 0,	/* some old 1-channel pictures have CHAN=r */
	"rgb",	"",	rgb0chan,0,
	"r",	"rgb",	cpchan,  0,
	"r",	"m",	maprchan,0,
	"r",	"",	zerochan,0,
	"g",	"g",	cpchan,  0,
	"g",	"m",	mapgchan,0,
	"g",	"",	zerochan,0,		
	"b",	"b",	cpchan,  0,
	"b",	"m",	mapbchan,0,
	"b",	"",	zerochan,0,
	"a",	"a",	cpchan,  0,
	"a",	"",	a255chan,0,
	"m",	"m",	cpchan,  1,
	"m",	"rgb",	lumchan, 0,
	"m",	"r",	cpchan,	 0,	/* some old 1-channel pictures have CHAN=r */
	"m",	"",	zerochan,0,
	"z...",	"z...",	cp4chan, 0,
	"z...",	"",	z1chan,  0,
	"0",	"0",	cpchan,  0,
	"0",	"",	zerochan,0,
	0
};
void conversion(char *old, char *new, char *cmapp){
	register struct conv *cp;
	register i, o;
	if(cmapp){
		for(i=0;i!=256;i++){
			cmap[i][0]=cmapp[3*i];
			cmap[i][1]=cmapp[3*i+1];
			cmap[i][2]=cmapp[3*i+2];
		}
	}
	else{
		for(i=0;i!=256;i++) cmap[i][0]=cmap[i][1]=cmap[i][2]=i;
	}
	if(strcmp(old, new)==0){
		ecvtrou++->rou=ident;
		return;
	}
	for(o=0;new[o];){
		for(cp=conv;cp->new;cp++) if(strncmp(new+o, cp->new, strlen(cp->new))==0){
			for(i=0;old[i];i++) if(strncmp(old+i, cp->old, strlen(cp->old))==0){
				ecvtrou->i=i;
				ecvtrou->o=o;
				ecvtrou++->rou=cp->rou;
				if(cp->savecmap) needcmap=1;
				goto Found;
			}
		}
		fprint(2, "pcp: Can't convert %s to %s\n", old, new);
		exits("unknown conversion");
	Found:
		o+=strlen(cp->new);
	}
}
