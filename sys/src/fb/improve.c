/*
 * improve -- given a colormap and a picture, compute a colormap that
 * better matches the picture's colors
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NMAP	256
struct box{
	int count;
	int map[3];
	int sum[3];
}cmap[NMAP];
int nmap;
#define	RED	0
#define	GRN	1
#define	BLU	2
void buildkd(void);
int closest(int, int, int);
main(int argc, char *argv[]){
	int x, y;
	char *p;
	struct box *bp;
	PICFILE *in;
	char *inline, cmapbuf[256*3];
	if((argc=getflags(argc, argv, "n:1[ncolor]"))!=2 && argc!=3) usage("cmap [picture]");
	nmap=flag['n']?atoi(flag['n'][0]):NMAP;
	if(nmap<=0 || NMAP<nmap) usage("cmap [picture]");
	if(!getcmap(argv[1], (unsigned char *)cmapbuf)){
		perror(argv[1]);
		exits("can't get cmap");
	}
	in=picopen_r(argc==3?argv[2]:"IN");
	if(in==0){
		perror(argc==3?argv[2]:"IN");
		exits("open input");
	}
	inline=malloc(PIC_WIDTH(in)*PIC_NCHAN(in));
	if(inline==0){
		fprint(2, "%s: out of space\n", argv[0]);
		exits("no space");
	}
	if(PIC_NCHAN(in)<3){
		fprint(2, "%s: not rgb\n", argv[1]);
		exits("not rgb");
	}
	for(bp=cmap,p=cmapbuf;bp!=&cmap[nmap];bp++,p+=3){
		bp->map[0]=p[0]&255;
		bp->map[1]=p[1]&255;
		bp->map[2]=p[2]&255;
	}
	buildkd();
	for(y=0;y!=PIC_HEIGHT(in);y++){
		picread(in, inline);
		for(x=0,p=inline;x!=PIC_WIDTH(in);x++,p+=PIC_NCHAN(in)){
			bp=&cmap[closest(p[0]&255, p[1]&255, p[2]&255)];
			bp->sum[0]+=p[0]&255;
			bp->sum[1]+=p[1]&255;
			bp->sum[2]+=p[2]&255;
			bp->count++;
		}
	}
	for(bp=cmap,p=cmapbuf;bp!=&cmap[nmap];bp++,p+=3){
		if(bp->count){
			p[0]=bp->sum[0]/bp->count;
			p[1]=bp->sum[1]/bp->count;
			p[2]=bp->sum[2]/bp->count;
		}
		else{
			p[0]=0;
			p[1]=0;
			p[2]=0;
		}
	}
	write(1, cmapbuf, sizeof cmapbuf);
	exits("");
}
struct kd{
	int dim, split;
	struct kd *left, *right;
	struct box *leaf;
}kd[2*NMAP], *ekd=kd;
int test[3];		/* test point */
struct box *closep;	/* closest colormap entry found to test point */
int closed;		/* square of distance from test point to closep */
int lo[3], hi[3];	/* space delimited by kd-tree partitions */
#define	INF	0x7FFFFFFF
int sqdist(int r, int g, int b){ return r*r+g*g+b*b; }
int overlap(void);
/*
 * Find the closest color-map entry to the test point, using
 * kd-tree nearest neighbor search algorithm from Friedman, Bentley & Finkel:
 * ``An Algorithm for Finding Best Matches in Logarithmic Expected Time,''
 * ACM Trans. on Math. Soft. 3, pp. 209-226
 *
 * N.B.: kdlook(.) and its subroutine overlap() typically account for 65% of
 * this program's run-time, split evenly between the two.  Everything else is
 * down in the noise.
 */
void kdlook(struct kd *kd){
	register t, d;
	if(kd->leaf){
		t=sqdist(test[RED]-kd->leaf->map[RED],
			test[GRN]-kd->leaf->map[GRN],
			test[BLU]-kd->leaf->map[BLU]);
		if(t<closed){
			closep=kd->leaf;
			closed=t;
		}
	}
	else{
		d=kd->dim;
		if(test[d]<kd->split){
			t=hi[d];
			hi[d]=kd->split;
			kdlook(kd->left);
			hi[d]=t;
			t=lo[d];
			lo[d]=kd->split;
			if(overlap()) kdlook(kd->right);
			lo[d]=t;
		}
		else{
			t=lo[d];
			lo[d]=kd->split;
			kdlook(kd->right);
			lo[d]=t;
			t=hi[d];
			hi[d]=kd->split;
			if(overlap()) kdlook(kd->left);
			hi[d]=t;
		}
	}
}
/*
 * Does the sphere centered on the test point and with closep on its
 * surface overlap the bound volume?
 */
int overlap(void){
	register dsq, t;
	if(test[RED]<lo[RED]){
		t=lo[RED]-test[RED];
		dsq=t*t;
	}
	else if(test[RED]>hi[RED]){
		t=hi[RED]-test[RED];
		dsq=t*t;
	}
	else dsq=0;
	if(test[GRN]<lo[GRN]){
		t=lo[GRN]-test[GRN];
		dsq+=t*t;
	}
	else if(test[GRN]>hi[GRN]){
		t=hi[GRN]-test[GRN];
		dsq+=t*t;
	}
	if(test[BLU]<lo[BLU]){
		t=lo[BLU]-test[BLU];
		dsq+=t*t;
	}
	else if(test[BLU]>hi[BLU]){
		t=hi[BLU]-test[BLU];
		dsq+=t*t;
	}
	return dsq<=closed;
}
int closest(int r, int g, int b){
	test[RED]=r;
	test[GRN]=g;
	test[BLU]=b;
	lo[RED]=lo[GRN]=lo[BLU]=-INF;
	hi[RED]=hi[GRN]=hi[BLU]=INF;
	closed=INF;
	kdlook(kd);
	return closep-cmap;
}
int kddim;
int kdcmp(struct box *a, struct box *b){
	return a->map[kddim]-b->map[kddim];
}
/*
 * build the kd-tree
 */
struct kd *makekd(struct box *p0, struct box *p1){
	register struct box *p;
	register struct kd *kp=ekd++;
	register dim;
	int lo[3], hi[3];
	if(p1==p0+1){
		kp->leaf=p0;
		return kp;
	}
	for(dim=0;dim!=3;dim++){
		lo[dim]=hi[dim]=p0->map[dim];
		for(p=p0+1;p!=p1;p++){
			if(p->map[dim]<lo[dim]) lo[dim]=p->map[dim];
			else if(p->map[dim]>hi[dim]) hi[dim]=p->map[dim];
		}
	}
	if(hi[RED]-lo[RED]>hi[GRN]-lo[GRN] && hi[RED]-lo[RED]>hi[BLU]-lo[BLU])
		kddim=RED;
	else if(hi[GRN]-lo[GRN]>hi[BLU]-lo[BLU])
		kddim=GRN;
	else
		kddim=BLU;
	qsort((char *)p0, p1-p0, sizeof(struct box), kdcmp);
	kp->dim=kddim;
	p=p0+(p1-p0)/2;
	kp->split=p->map[kddim];
	while(p!=p1 && p->map[kddim]==kp->split) p++;
	if(p==p1){
		--p;
		while(p>=p0 && p->map[kddim]==kp->split) --p;
		if(p<p0){
/*			fprint(2, "cell too tiny!\n"); */
			kp->leaf=p0;
			return kp;
		}
		kp->split=p++->map[kddim];
	}
	kp->left=makekd(p0, p);
	kp->right=makekd(p, p1);
	return kp;
}
void buildkd(void){
	register struct kd *kp;
	for(kp=kd;kp!=ekd;kp++){
		kp->left=0;
		kp->right=0;
		kp->leaf=0;
	}
	ekd=kd;
	makekd(cmap, &cmap[nmap]);
}
