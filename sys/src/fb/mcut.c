/*
 * use the median cut algorithm to extract a colormap from an rgb picture
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define NR	32
#define NG	32
#define	NB	32
int count[NR][NG][NB];
#define	NMAP	256
struct box{
	int lo[3], hi[3], count;
}cmap[NMAP]={0, 0, 0, NR, NG, NB, 0}, *emap=cmap+1;
int nmap;
#define	RED	0
#define	GRN	1
#define	BLU	2
void shrink(struct box *);
int splitbig(void);
void outcmap(void);
main(int argc, char *argv[]){
	int x, y;
	char *p;
	PICFILE *in;
	char *inline;
	if((argc=getflags(argc, argv, "n:1[ncolor]"))!=1 && argc!=2) usage("[picfile]");
	nmap=flag['n']?atoi(flag['n'][0]):NMAP;
	if(nmap<=0 || NMAP<nmap) usage("[picfile]");
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		picerror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	if(PIC_NCHAN(in)<3){
		fprint(2, "%s: not rgb\n", argv[1]);
		exits("not rgb");
	}
	inline=malloc(PIC_WIDTH(in)*PIC_NCHAN(in));
	if(inline==0){
		fprint(2, "%s: out of space\n");
		exits("no space");
	}
	for(y=0;y!=PIC_HEIGHT(in);y++){
		picread(in, inline);
		for(x=0,p=inline;x!=PIC_WIDTH(in);x++,p+=PIC_NCHAN(in)){
			count[(p[0]&255)*(NR-1)/255][(p[1]&255)*(NR-1)/255]
				[(p[2]&255)*(NR-1)/255]++;
		}
	}
	cmap[0].count=PIC_WIDTH(in)*PIC_HEIGHT(in);
	shrink(cmap);
	do;while(emap!=&cmap[nmap] && splitbig());
	outcmap();
}
void shrink(struct box *bp){
	register r, g, b;
#define rfwd for(r=bp->lo[0];r!=bp->hi[0];r++)
#define gfwd for(g=bp->lo[1];g!=bp->hi[1];g++)
#define bfwd for(b=bp->lo[2];b!=bp->hi[2];b++)
#define rback for(r=bp->hi[0];r!=bp->lo[0];--r)
#define gback for(g=bp->hi[1];g!=bp->lo[1];--g)
#define bback for(b=bp->hi[2];b!=bp->lo[2];--b)
	rfwd gfwd bfwd if(count[r][g][b]) goto lo0Out;
lo0Out:
	bp->lo[0]=r;
	rback gfwd bfwd if(count[r-1][g][b]) goto hi0Out;
hi0Out:
	bp->hi[0]=r;
	gfwd bfwd rfwd if(count[r][g][b]) goto lo1Out;
lo1Out:
	bp->lo[1]=g;
	gback bfwd rfwd if(count[r][g-1][b]) goto hi1Out;
hi1Out:
	bp->hi[1]=g;
	bfwd rfwd gfwd if(count[r][g][b]) goto lo2Out;
lo2Out:
	bp->lo[2]=b;
	bback rfwd gfwd if(count[r][g][b-1]) goto hi2Out;
hi2Out:
	bp->hi[2]=b;
}
void divide(struct box box, struct box *lp, struct box *rp){
	register r, g, b, m=0;
	*lp=*rp=box;
	r=box.hi[0]-box.lo[0];
	g=box.hi[1]-box.lo[1];
	b=box.hi[2]-box.lo[2];
	if(r>g && r>b){
		r=box.lo[0];
		do{
			for(g=box.lo[1];g!=box.hi[1];g++)
				for(b=box.lo[2];b!=box.hi[2];b++)
					m+=count[r][g][b];
		}while(++r!=box.hi[0]-1 && 2*m<box.count);
		lp->hi[0]=rp->lo[0]=r;
	}
	else if(g>b){
		g=box.lo[1];
		do{
			for(b=box.lo[2];b!=box.hi[2];b++)
				for(r=box.lo[0];r!=box.hi[0];r++)
					m+=count[r][g][b];
		}while(++g!=box.hi[1]-1 && 2*m<box.count);
		lp->hi[1]=rp->lo[1]=g;
	}
	else{
		b=box.lo[2];
		do{
			for(r=box.lo[0];r!=box.hi[0];r++)
				for(g=box.lo[1];g!=box.hi[1];g++)
					m+=count[r][g][b];
		}while(++b!=box.hi[2]-1 && 2*m<box.count);
		lp->hi[2]=rp->lo[2]=b;
	}
	lp->count=m;
	rp->count=box.count-m;
}
int splitbig(void){
	register struct box *bp, *big=0;
	register size=0;
	struct box left, right;
	for(bp=cmap;bp!=emap;bp++)
		if((bp->hi[0]-bp->lo[0])*(bp->hi[1]-bp->lo[1])
			*(bp->hi[2]-bp->lo[2])>1
		&& bp->count>size){
			size=bp->count;
			big=bp;
		}
	if(big){
		divide(*big, &left, &right);
		shrink(&left);
		shrink(&right);
		*big=left;
		*emap++=right;
		return 1;
	}
	return 0;
}
int lumcmp(char *a, char *b){
	return (a[0]&255)*30+(a[1]&255)*59+(a[2]&255)*11
		-(b[0]&255)*30-(b[1]&255)*59-(b[2]&255)*11;
}
void outcmap(void){
	register struct box *bp;
	register r, g, b, racc, gacc, bacc, den, rgb;
	char cmapbuf[NMAP*3], *cp=cmapbuf;;
	for(bp=cmap;bp!=&cmap[nmap];bp++){
		racc=gacc=bacc=den=0;
		for(r=bp->lo[0];r!=bp->hi[0];r++)
			for(g=bp->lo[1];g!=bp->hi[1];g++)
				for(b=bp->lo[2];b!=bp->hi[2];b++){
					rgb=count[r][g][b];
					if(rgb){
						racc+=r*rgb;
						gacc+=g*rgb;
						bacc+=b*rgb;
						den+=rgb;
					}
				}
		if(den){
			*cp++=racc*255/((NR-1)*den);
			*cp++=gacc*255/((NG-1)*den);
			*cp++=bacc*255/((NB-1)*den);
		}
		else{
			*cp++=0;
			*cp++=0;
			*cp++=0;
		}
	}
	qsort((char *)cmapbuf, nmap, 3, lumcmp);
	for(;cp!=&cmapbuf[256*3];cp++) *cp=0;
	write(1, cmapbuf, sizeof cmapbuf);
}
