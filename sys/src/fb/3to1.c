/*
 * 3to1 -- convert a 24 bit picture to 8 bits
 *	also does color-map replacement on 8 bit inputs
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NCMAP	256		/* how many color map entries? */
#define	NCOLOR	256		/* 0<=r,g,b<NCOLOR */
void initcindex(uchar [][3], int);
int fclook(int, int, int);
int clook(int, int, int);
void main(int argc, char *argv[]){
	PICFILE *in, *out;
	char *infile, *s, *inbuf, *outbuf, *ip, *op, *eop;
	int wid, hgt, nchan, offs, y, mapped, i, t;
	uchar cmap[NCMAP][3];
	uchar icmap[NCMAP][3];
	short *e, *error, herror[3];
	int ncmap;
	switch(getflags(argc, argv, "en:1[ncolor]")){
	default:
		usage("cmap [picture]");
	case 3:
		infile=argv[2];
		break;
	case 2:
		infile="IN";
		break;
	}
	if(!getcmap(argv[1], (uchar *)cmap)){
		fprint(2, "%s: can't get colormap %s\n", argv[0], argv[1]);
		exits("no cmap");
	}
	ncmap=flag['n']?atoi(flag['n'][0]):NCMAP;
	if(ncmap>NCMAP){
		fprint(2, "%s: sorry, can't handle ncolor>%d\n", argv[0], NCMAP);
		exits("range error");
	}
	in=picopen_r(infile);
	if(in==0){
		perror(infile);
		exits("no input");
	}
	s=strstr(in->chan, "rgb");
	if(s==0){
		mapped=1;
		s=strstr(in->chan, "m");
		if(s==0){
			fprint(2, "%s: can't convert CHAN=%s\n", argv[0], in->chan);
			exits("unk chan");
		}
		if(in->cmap) memmove(icmap, in->cmap, 256*3);
		else for(y=0;y!=256;y++) icmap[y][0]=icmap[y][1]=icmap[y][2]=y;
	}
	else
		mapped=0;
	offs=s-in->chan;
	wid=PIC_WIDTH(in);
	hgt=PIC_HEIGHT(in);
	nchan=PIC_NCHAN(in);
	inbuf=malloc(wid*nchan);
	outbuf=malloc(wid);
	error=malloc((wid+1)*3*sizeof(short));
	memset(error, 0, (wid+1)*3*sizeof(short));
	if(inbuf==0 || outbuf==0 || error==0){
		fprint(2, "%s: can't malloc\n", argv[0]);
		exits("no malloc");
	}
	out=picopen_w("OUT", in->type, PIC_XOFFS(in), PIC_YOFFS(in), wid, hgt, "m", 0,
		(char *)cmap);
	initcindex(cmap, ncmap);
	eop=outbuf+wid;
	for(y=0;y!=hgt;y++){
		picread(in, inbuf);
		if(flag['e']){
			if(mapped){
				for(op=outbuf,ip=inbuf+offs;op!=eop;op++,ip+=nchan)
					*op=fclook(icmap[*ip&255][0]&255,
						icmap[*ip&255][1]&255,
						icmap[*ip&255][2]&255);
			}
			else{
				for(op=outbuf,ip=inbuf+offs;op!=eop;op++,ip+=nchan)
					*op=fclook(ip[0]&255, ip[1]&255, ip[2]&255);
			}
		}
		else if(mapped){
			herror[0]=herror[1]=herror[2]=0;
			for(op=outbuf,ip=inbuf+offs,e=error+3;op!=eop;op++,ip+=nchan,e+=3){
				*op=clook((icmap[*ip&255][0]&255)+e[0],
					  (icmap[*ip&255][1]&255)+e[1],
					  (icmap[*ip&255][2]&255)+e[2]);
				for(i=0;i!=3;i++){
					t=(icmap[*ip&255][i]&255)-cmap[*op&255][i];
					e[i]=3*t/8+herror[i];
					herror[i]=t/4;
					e[i+3]+=t-(5*t/8);
				}
			}
		}
		else{
			herror[0]=herror[1]=herror[2]=0;
			for(op=outbuf,ip=inbuf+offs,e=error+3;op!=eop;op++,ip+=nchan,e+=3){
				*op=clook((ip[0]&255)+e[0],
					  (ip[1]&255)+e[1],
					  (ip[2]&255)+e[2]);
				for(i=0;i!=3;i++){
					t=(ip[i]&255)-cmap[*op&255][i];
					e[i]=3*t/8+herror[i];
					herror[i]=t/4;
					e[i+3]+=t-(5*t/8);
				}
			}
		}
		picwrite(out, outbuf);
	}
}
/*
 * Color-map reverse indexing using a bucket list
 *
 * cindex[r>>RSH][g>>GSH][b>>BSH] points to a list of colors
 * that might be the closest to (r,g,b).
 */
#define	RSH	4	/* tune this for speed */
#define	GSH	4	/* tune this for speed */
#define	BSH	4	/* tune this for speed */
#define	NR	(NCOLOR>>RSH)
#define	NG	(NCOLOR>>GSH)
#define	NB	(NCOLOR>>BSH)
typedef struct Interval Interval;
typedef struct Color Color;
struct Interval{
	int lo, hi;
};
struct Color{
	uchar r, g, b, i;
	Color *next;
};
Color *cindex[NR][NG][NB];
/*
 * Look (r,g,b) up by indexing
 * cindex and walking through the list
 * of possible closest colors
 */
int clook(int r, int g, int b){
	Color *cp, *bestp;
	int bestd, dr, dg, db, d;
	if(r<0) r=0; else if(r>=NCOLOR) r=NCOLOR-1;
	if(g<0) g=0; else if(g>=NCOLOR) g=NCOLOR-1;
	if(b<0) b=0; else if(b>=NCOLOR) b=NCOLOR-1;
	cp=cindex[r>>RSH][g>>GSH][b>>BSH];
	bestp=cp;
	if(cp->next){
		dr=cp->r-r;
		dg=cp->g-g;
		db=cp->b-b;
		bestd=dr*dr+dg*dg+db*db;
		while((cp=cp->next)!=0){
			dr=cp->r-r;
			dg=cp->g-g;
			db=cp->b-b;
			d=dr*dr+dg*dg+db*db;
			if(d<bestd){
				bestd=d;
				bestp=cp;
			}
		}
	}
	return bestp->i;
}
/*
 * Same as above, but without range check
 */
int fclook(int r, int g, int b){
	Color *cp, *bestp;
	int bestd, dr, dg, db, d;
	cp=cindex[r>>RSH][g>>GSH][b>>BSH];
	bestp=cp;
	if(cp->next){
		dr=cp->r-r;
		dg=cp->g-g;
		db=cp->b-b;
		bestd=dr*dr+dg*dg+db*db;
		while((cp=cp->next)!=0){
			dr=cp->r-r;
			dg=cp->g-g;
			db=cp->b-b;
			d=dr*dr+dg*dg+db*db;
			if(d<bestd){
				bestd=d;
				bestp=cp;
			}
		}
	}
	return bestp->i;
}
/*
 * Interval addition
 */
Interval iadd(Interval a, Interval b){
	a.lo+=b.lo;
	a.hi+=b.hi;
	return a;
}
/*
 * Interval square
 */
Interval isq(Interval a){
	int t;
	if(a.hi<=0){
		t=a.hi*a.hi;
		a.hi=a.lo*a.lo;
		a.lo=t;
	}
	else if(a.lo>=0){
		a.lo*=a.lo;
		a.hi*=a.hi;
	}
	else{
		if(-a.lo>=a.hi)
			a.hi=a.lo*a.lo;
		else
			a.hi*=a.hi;
		a.lo=0;
	}
	return a;
}
/*
 * interval ((r<<RSH)-c.r)^2 + ((g<<GSH)-c.g)^2 + ((b<<BSH)-c.b)^2
 *
 * This tells us how far c can be from any point in
 * the box (r,g,b)
 */
Interval rsq(Color c, Interval r, Interval g, Interval b){
	r.lo=(r.lo<<RSH)-c.r;
	r.hi=(r.hi<<RSH)-c.r;
	g.lo=(g.lo<<GSH)-c.g;
	g.hi=(g.hi<<GSH)-c.g;
	b.lo=(b.lo<<BSH)-c.b;
	b.hi=(b.hi<<BSH)-c.b;
	return iadd(iadd(isq(r), isq(g)), isq(b));
}
/*
 * map[ncmap] is a list of colors that might be the closest
 * color to some point in the interval box (r,g,b).
 *
 * First, we trim the list to remove colors whose minimum distance
 * to a point in (r,g,b) is larger than the maximum of some other
 * color, since those can never be the closest to any point in (r,g,b).
 *
 * If there is only one color in the list, or if there is only one point
 * in the box, we're done.
 *
 * Otherwise, we subdivide the (r,g,b) box in its longest dimension and
 * proceed recursively.
 */
void rangemap(Color map[NCMAP], int ncmap, Interval r, Interval g, Interval b){
	Interval range[NCMAP], new;
	Color submap[NCMAP], *clist;
	int i, j, k, minhi, nsubmap, dr, dg, db;
	minhi=255*255*3+1;
	for(i=0;i!=ncmap;i++){
		range[i]=rsq(map[i], r, g, b);
		if(range[i].hi<minhi)
			minhi=range[i].hi;
	}
	nsubmap=0;
	for(i=0;i!=ncmap;i++){
		if(range[i].lo<=minhi){
			submap[nsubmap]=map[i];
			nsubmap++;
		}
	}
	dr=r.hi-r.lo;
	dg=g.hi-g.lo;
	db=b.hi-b.lo;
	if(nsubmap==1 || dr*dg*db<=1){
		clist=malloc(nsubmap*sizeof(Color));
		memmove(clist, submap, nsubmap*sizeof(Color));
		for(i=1;i!=nsubmap;i++) clist[i-1].next=clist+i;
		clist[nsubmap-1].next=0;
		for(i=r.lo;i!=r.hi;i++) for(j=g.lo;j!=g.hi;j++) for(k=b.lo;k!=b.hi;k++)
			cindex[i][j][k]=clist;
	}
	else if(dr>=dg && dr>=db){
		new.lo=r.lo;
		new.hi=r.lo+dr/2;
		rangemap(submap, nsubmap, new, g, b);
		new.lo=new.hi;
		new.hi=r.hi;
		rangemap(submap, nsubmap, new, g, b);
	}
	else if(dg>=db){
		new.lo=g.lo;
		new.hi=g.lo+dg/2;
		rangemap(submap, nsubmap, r, new, b);
		new.lo=new.hi;
		new.hi=g.hi;
		rangemap(submap, nsubmap, r, new, b);
	}
	else{
		new.lo=b.lo;
		new.hi=b.lo+db/2;
		rangemap(submap, nsubmap, r, g, new);
		new.lo=new.hi;
		new.hi=b.hi;
		rangemap(submap, nsubmap, r, g, new);
	}
}
Interval rrange={0,NR}, grange={0,NG}, brange={0,NB};
void initcindex(uchar input[NCMAP][3], int ncmap){
	int i;
	Color map[NCMAP];
	for(i=0;i!=ncmap;i++){
		map[i].r=input[i][0];
		map[i].g=input[i][1];
		map[i].b=input[i][2];
		map[i].i=i;
	}
	rangemap(map, ncmap, rrange, grange, brange);
}
