#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int filter(unsigned char *);
void map(PICFILE *, PICFILE *, int (*)(unsigned char *), int);
main(int argc, char *argv[]){
	PICFILE *in, *out;
	argc=getflags(argc, argv, "");
	if(argc!=1 && argc!=2) usage("[picture]");
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		perror("OUT");
		exits("creat output");
	}
	map(in, out, filter, NWIN);
	exits(0);
}
#define	new(t, n)	((t *)emalloc((n)*sizeof(t)))
char *emalloc(int n){
	char *v=malloc(n);
	if(v==0){
		fprint(2, "Out of space\n");
		exits("no space");
	}
	return v;
}
struct line{
	unsigned char *left;
	unsigned char *read;
	unsigned char *eread;
	unsigned char *right;
	unsigned char *cursor;
};
void getline(PICFILE *, struct line *, struct line *);
void dupline(struct line *, struct line *);
/*
 * Neighbourhood mapping from picture in to out.  Each pixel of out
 * is written with some funcion of an n by n neighbourhood of in.
 * rou is called once for each channel of each output pixel.  Its
 * argument is an n by n array of unsigned chars containing one channel
 * of the neighbourhood surrounding the corresponding input pixel.
 * Pixels on in's boundary are replicated to provide neighbours
 * for pixels close to the edge.
 * Should take a mask argument to indicate what channels to operate on.
 */
void map(PICFILE *in, PICFILE *out, int (*rou)(unsigned char *), int n){
	int j;
	unsigned char *p, *q;
	int nchan=PIC_NCHAN(in);
	int i;
	unsigned char *op, *eout, *buf, *box=new(unsigned char, n*n);
	unsigned char *outline=new(unsigned char, PIC_WIDTH(in)*nchan);
	int y;
	struct line *line, t;
	eout=outline+PIC_WIDTH(in)*nchan;
	line=new(struct line, n);
	line[0].left=buf=new(unsigned char, n*(PIC_WIDTH(in)+n-1)*nchan);
	for(i=0;i!=n;i++){
		if(i) line[i].left=line[i-1].right;
		line[i].read=line[i].left+(n/2)*nchan;
		line[i].eread=line[i].read+PIC_WIDTH(in)*nchan;
		line[i].right=line[i].left+(PIC_WIDTH(in)+n-1)*nchan;
	}
	getline(in, &line[n/2], &line[n/2]);
	for(i=0;i!=n/2;i++) dupline(&line[i], &line[n/2]);
	for(i=n/2+1;i<n-1;i++) getline(in, &line[i], &line[i-1]);
	for(y=0;y!=PIC_WIDTH(in);y++){
		getline(in, &line[n-1], &line[n-2]);
		for(i=0;i!=n;i++) line[i].cursor=line[i].left;
		for(op=outline;op!=eout;op++){
			q=box;
			for(i=0;i!=n;i++){
				p=line[i].cursor++;
				for(j=0;j!=n;j++){
					*q++=*p;
					p+=nchan;
				}
			}
			*op=(*rou)(box);
		}
		picwrite(out, (char *)outline);
		t=line[0];
		for(i=1;i!=n;i++) line[i-1]=line[i];
		line[n-1]=t;
	}
	free((char *)box);
	free((char *)outline);
	free((char *)buf);
	free((char *)line);
}
/*
 * read a line from the input picture into the buffer described by lp,
 * replicating pixels at each end.  if at eof, copy line mp.
 */
void getline(PICFILE *f, struct line *lp, struct line *mp){
	register unsigned char *p, *q;
	if(!picread(f, (char *)lp->read)) dupline(lp, mp);
	else{
		p=lp->read;
		q=p+f->nchan;
		while(p!=lp->left) *--p = *--q;
		p=lp->eread;
		q=p-f->nchan;
		while(p!=lp->right) *p++ = *q++;
	}
}
/*
 * copy line mp into lp
 */
void dupline(struct line *lp, struct line *mp){
	memcpy((char *)lp->left, (char *)mp->left, mp->right-mp->left);
}
