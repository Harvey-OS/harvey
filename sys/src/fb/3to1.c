/*
 * Convert a 3 channel picture to the best 1 channel picture with the
 * given colormap.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NMAP	256
struct box{
	int map[3];
}cmap[NMAP];
int nmap;
#define	RED	0
#define	GRN	1
#define	BLU	2
char **picargv;
short error[4098*3];
char *progcmd;
void buildkd(void);
int closest(int r, int g, int b);
main(int argc, char *argv[]){
	int x, y;
	char *p, *q;
	short *e;
	int i, t;
	PICFILE *in, *out;
	struct box *bp;
	char inline[4096*8], outline[4096], cmapbuf[256*3];
	short herror[3];
	argc=getflags(argc, argv, "en:1[ncolor]");
	if(argc!=2 && argc!=3) usage("cmap [picture]");
	nmap=flag['n']?atoi(flag['n'][0]):NMAP;
	if(nmap<=0 || NMAP<nmap) usage("cmap [picture]");
	if(!getcmap(argv[1], (unsigned char *)cmapbuf)){
		perror(argv[1]);
		exits("no cmap");
	}
	for(bp=cmap,p=cmapbuf;bp!=&cmap[nmap];bp++,p+=3){
		bp->map[0]=p[0]&255;
		bp->map[1]=p[1]&255;
		bp->map[2]=p[2]&255;
	}
	buildkd();
	for(bp=cmap,p=cmapbuf;bp!=&cmap[nmap];bp++,p+=3){
		p[0]=bp->map[0];
		p[1]=bp->map[1];
		p[2]=bp->map[2];
	}
	in=picopen_r(argc==3?argv[2]:"IN");
	if(in==0){
		picerror(argc==3?argv[2]:"IN");
		exits("open input");
	}
	if(PIC_NCHAN(in)<3){
		fprint(2, "%s: not rgb\n", argv[1]);
		exits("not rgb");
	}
	out=picopen_w("OUT", in->type,
		PIC_XOFFS(in), PIC_YOFFS(in), PIC_WIDTH(in), PIC_HEIGHT(in),
		"m", argv, cmapbuf);
	if(out==0){
		picerror("OUT");
		exits("create output");
	}
	for(y=0;y!=PIC_HEIGHT(in);y++){
		picread(in, inline);
		if(flag['e']){
			for(x=0,p=inline,q=outline;x!=PIC_WIDTH(in);x++,p+=PIC_NCHAN(in),q++)
				*q=closest(p[0]&255, p[1]&255, p[2]&255);
		}
		else{
			herror[0]=herror[1]=herror[2]=0;
			for(x=0,p=inline,q=outline,e=error+3;
			    x!=PIC_WIDTH(in);
			    x++,p+=PIC_NCHAN(in),q++,e+=3){
				*q=closest((p[0]&255)+e[0], (p[1]&255)+e[1], (p[2]&255)+e[2]);
				for(i=0;i!=3;i++){
					t=(p[i]&255)-cmap[*q&255].map[i];
					e[i]=3*t/8+herror[i];
					herror[i]=t/4;
					e[i+3]+=t-(5*t/8);
				}
			}
		}
		picwrite(out, outline);
	}
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
	int t, d;
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
			fprint(2, "tiny cell %d\n", p1-p0);
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
