#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
#include "njerq.h"
#include "comm.h"
# include "defines.h"
# include "object.h"
# include "path.h"
# include "graphics.h"
#include "cache.h"

#ifndef Plan9
extern char *outname;
extern int minx,miny,maxx,maxy;
#endif

int resolution = 0;
int sawshow=0;
static ys, lx1, lx2, ly;

void
erasepageOP(void)
{
	rectf(&screen,screen.r,Zero) ;
}

void
showpageOP(void)
{
	dobitmap();
#ifdef Plan9
	/*if(dontask)done(0);*/
	if(!dontask)ckmouse(1);
#endif
	erasepageOP();
	initgraphicsOP();
#ifndef Plan9
	minx = XMAX;
	miny = YMAX;
	maxx = maxy = 0;
#ifdef HIRES
	fprintf(stderr,"finished page %d\n",pageno);
	done(0);
#endif
#endif
	fontchanged=1;
	sawshow=1;
}

void
copypageOP(void){
	dobitmap();
#ifdef Plan9
	ckmouse(1);
#endif
	fontchanged=1;
}

void
initclipOP(void)
{
	double wid, ht, cminx, cminy, cmaxx, cmaxy;
	struct	path	curpath ;

	curpath = Graphics.path ;
	rel_path(Graphics.clippath.first);
	Graphics.path.first = Graphics.path.last = NULL ;
	cmaxx = (double)Graphics.device.width;
	cmaxy = (double)Graphics.device.height;
	cminx = cminy = 0.;
#ifdef Plan9
	if(resolution==2){
		cminx += (double)p_anchor.x;
		cmaxx += (double)p_anchor.x;
		cminy += (double)p_anchor.y;
		cmaxy += (double)p_anchor.y;
	}
#endif
	if(korean|| merganthal >= 3){
		cminx = -400;
		cminy = -100;
	}
	add_element(PA_MOVETO,makepoint(cminx,cminy)) ;
	add_element(PA_LINETO,makepoint(cmaxx,cminy)) ;
	add_element(PA_LINETO,makepoint(cmaxx,cmaxy)) ;
	add_element(PA_LINETO,makepoint(cminx,cmaxy)) ;
	add_element(PA_CLOSEPATH,makepoint(cminx,cminy)) ;
	Graphics.iminx = (int)floor(cminx);
	Graphics.iminy = (int)floor(cminy);
	Graphics.imaxx = (int)floor(cmaxx);
	Graphics.imaxy = (int)floor(cmaxy);
	Graphics.clippath = Graphics.path ;
	Graphics.path = curpath ;
	Graphics.clipchanged=0;
}
char gindex;
void
do_fill(void (*caller)(void))
{
	struct	x	x[PATH_LIMIT];
	struct	element	*p ;
	double	Fmaxy,Fmaxx, Fminy,Fminx, ry, y, left, right, gtrans, g;
	int		i, nx, winding, start;
	int tx, ty;

	if ( Graphics.path.first == NULL )
		return ;
	Fminx = Fmaxx = Graphics.path.first->ppoint.x;
	Fminy = Fmaxy = Graphics.path.first->ppoint.y ;
	for ( p=Graphics.path.first ; p!=NULL ; p=p->next ){
		if ( p->ppoint.y > Fmaxy )
			Fmaxy = p->ppoint.y ;
		if ( p->ppoint.y < Fminy )
			Fminy = p->ppoint.y ;
		if ( p->ppoint.x > Fmaxx )
			Fmaxx = p->ppoint.x ;
		if ( p->ppoint.x < Fminx )
			Fminx = p->ppoint.x ;
	}
if(bbflg){
fprintf(stderr,"max %f %f min %f %f\n",Fminx,Fminy,Fmaxx,Fmaxy);
return;
}
/*	gindex = (char)(Graphics.color.brightness*4.999);*/
	g=currentgray();
	if(g == 0.)gindex = 0;
	else if(g == 1.)gindex = 16;
	else {
	gtrans = Graphics.graytab[(int)(currentgray()*(double)(GRAYLEVELS-1.))];
	gindex = floor(gtrans*((double)GRAYLEVELS-1.)/(256./17.));
/*	if(gindex == 0)gindex = 1;
	else if(gindex == 16)gindex = 15;
fprintf(stderr,"current %f gtrans %f gindex %d\n",currentgray(), gtrans, gindex);*/
	}
/*	gindex = floor(currentgray()*((double)GRAYLEVELS-1.)/(256./17.));*/
	if(gindex < 0)gindex = 0;
	if(gindex > 16)gindex = 16;
	lx1 = -1;
	Fminy = floor(Fminy);
	Fmaxy = ceil(Fmaxy) + 1.0;
	for ( y=(int)Fminy ; y<Fmaxy ; y+=1.0 ){
		ry = y + 1.0;
		nx = 0 ;
		for ( p=Graphics.path.first ; p!=NULL ; p=p->next ){
			if ( p->type == PA_MOVETO )
				continue ;
			if(p->prev->ppoint.y < y  &&  p->ppoint.y < y  ||
				p->prev->ppoint.y >= ry  &&  p->ppoint.y >= ry )
				continue ;
			x[nx++] = intercepts(p->prev,p,y);
		}
		qsort(x,nx,sizeof(*x),sxcomp) ;
		if ( caller == (void (*)(void))eofillOP )
			for ( i=0 ; i<nx ; ){
				winding=0;
				left = x[i].left;
				right = x[i].right;
				do {
					if(x[i].direction == -2)
						winding += 2;
					else winding += x[i].direction;
					if(left > x[i].left)
						left = x[i].left;
					if(right < x[i].right)
						right = x[i].right;
					i++;
				} while(winding != 0 && winding != 4 && i < nx);
				paint(nx,(int)floor(left),(int)floor(y),(int)floor(right)) ;
			}
		else {
			for ( i=0 ; i<nx ; ){
				winding = 0 ;
				left = x[i].left;
				right = x[i].right;
				do{
					winding += x[i].direction ;
					if(left > x[i].left)
						left = x[i].left;
					if(right < x[i].right)
						right = x[i].right;
					i++;
				} while ( winding != 0 && i < nx) ;
				paint(nx,(int)floor(left),(int)floor(y),(int)floor(right)) ;
			}
		}
	}
	if(lx1 >= 0)paint(2,lx1, (int)floor(y-1.),lx2);	/*trigger last one*/
}

void
paint(int pts, int x1, int y, int x2)
{
	int y1, y2;
/*fprintf(stderr,"pts %d y %d x1 %d x2 %d\n",pts,y,x1,x2);fflush(stderr);*/
	if ( y <= Graphics.iminy  ||  y > Graphics.imaxy )
		return ;
	if ( x2 < Graphics.iminx  ||  x1 > Graphics.imaxx )
		return ;
	if ( x1 < Graphics.iminx)
		x1 = Graphics.iminx;
	if ( x2 >= Graphics.imaxx)
		x2 = Graphics.imaxx;
	if(pts > 2){
		if(lx1 >= 0){
			y1 = Graphics.device.height - (ly-anchor.y);
			y2 = Graphics.device.height + 1 - (ys- anchor.y);
			if(lx1 == lx2)lx2++;
			if(y1 == y2)y2++;
			texture(&screen, Rect(lx1+anchor.x, y1, lx2+anchor.x,y2),
				pgrey[gindex], S);
#ifndef Plan9
			ckbdry(lx1, y1); ckbdry(lx2, y2);
#endif
		}
		y1 = Graphics.device.height - (y-anchor.y);
		if(x1 == x2)x2++;
		texture(&screen, Rect(x1+anchor.x, y1, x2+anchor.x,y1+1),
			pgrey[gindex],S);
#ifndef Plan9
		ckbdry(x1, y1); ckbdry(x2,y1+1);
#endif
		lx1 = -1;
	}
	else if(lx1<0){ ys = ly = y; lx1 = x1; lx2 = x2;}
	else if(x1 != lx1 || x2 != lx2){
		y1 = Graphics.device.height - (ly-anchor.y);
		y2 = Graphics.device.height + 1 - (ys - anchor.y);
		if(lx1 == lx2)lx2++;
		if(y1 == y2)y2++;
		texture(&screen, Rect(lx1+anchor.x, y1, lx2+anchor.x,y2),
			pgrey[gindex],S);
#ifndef Plan9
		ckbdry(lx1, y1); ckbdry(lx2,y2);
#endif
		ys = ly = y; lx1 = x1; lx2 = x2;
	}
	else ly = y;
}


struct x
intercepts(struct element *p0, struct element *p1, double y)
{
	double		x0, y0, x1, y1, m , ly, uy;
	double		left, right, temp;
	struct x	x;

	ly = y;
	uy = y + 1.0;

	x0 = p0->ppoint.x;
	y0 = p0->ppoint.y;
	x1 = p1->ppoint.x;
	y1 = p1->ppoint.y;
	if ( y0 < ly && y1 >= uy )
		x.direction = 2;
	else if ( y1 < ly && y0 >= uy )
		x.direction = -2;
	else if ( y0 >= ly && y0 < uy && y1 >= ly && y1 < uy )
		x.direction = 0;
	else if ( y0 < ly || y1 >= uy )
		x.direction = 1;
	else if ( y1 < ly || y0 >= uy )
		x.direction = -1;
	else pserror("internal error", "direction");

	if ( x0 > x1 ) {
		temp = x0;
		x0 = x1;
		x1 = temp;
		temp = y0;
		y0 = y1;
		y1 = temp;
	}

	if ( x.direction == 0 ) {
		x.left = x0;
		x.right = x1;
	} else if ( x0 == x1 ){
		x.left = x.right = x0;
	}
	else {
		m = (x1 - x0) / (y1 - y0);
		left = m * (ly - y0) + x0;
		right = m * (uy - y0) + x0;
		if ( left > right ) {
			x.left = right;
			x.right = left;
		}
		else {
			x.left = left;
			x.right = right;
		}
		if ( x.left < x0 ) x.left = x0;
		if ( x.right > x1 ) x.right = x1;
	}
	return(x);
}

int
sxcomp(struct x *p, struct x *q)
{
	if ( p->left >= q->right )
		return(1);
	else if ( q->left >= p->right )
		return(-1);
	else if ( p->left > q->left )
		return(1);
	else if ( q->left > p->left )
		return(-1);
	else if ( p->right > q->left )
		return(-1);
	else if ( q->right < p->right )
		return(1);
	else return(0);

}

void
framedeviceOP(void)
{
	struct	object	proc, matrix, object ;
	int		i, height, width ;

	proc = procedure() ;
	object = integer("framedevice");
	height = object.value.v_int ;
	object = integer("framedevice");
	width = object.value.v_int ;
	matrix = realmatrix(pop()) ;
 
	Graphics.device.proc = proc ;
	Graphics.device.height = height ;
	Graphics.device.width = 8 * width ;
	for ( i=0 ; i<CTM_SIZE ; i++ )
		Graphics.device.matrix[i] = matrix.value.v_array.object[i].value.v_real ;
	initmatrixOP() ;
	initclipOP() ;
}

void
nulldeviceOP(void)
{

	Graphics.device.width = 0 ;
	Graphics.device.height = 0 ;
	Graphics.device.matrix[0] = 1.0 ;
	Graphics.device.matrix[1] = 0.0 ;
	Graphics.device.matrix[2] = 0.0 ;
	Graphics.device.matrix[3] = 1.0 ;
	Graphics.device.matrix[4] = 0.0 ;
	Graphics.device.matrix[5] = 0.0 ;
	initmatrixOP() ;
	Graphics.device.proc = makearray(0,XA_EXECUTABLE) ;
	initclipOP() ;
}

void
deviceinit(void)
{
	double res;
	int dx,dy;
#ifdef Plan9
	dx = corn.x-orig.x;
	dy = corn.y-orig.y;
	if(resolution != 0){
		push(makematrix(makearray(CTM_SIZE,XA_LITERAL),
			100./72.,0.,0.,100./72.,0.,0.)) ;
		anchor.x = orig.x - p_anchor.x;
		anchor.y = orig.y + p_anchor.y;
	}
	else {
		if(800. - dx < 1100. - dy)res = dy/11.;
		else res = dx/8.;
		push(makematrix(makearray(CTM_SIZE,XA_LITERAL),
			res/72.,0.,0.,res/72.,0.,0.)) ;
		anchor.x = orig.x;
		anchor.y = orig.y;
	}
	push(makeint((dx+7)/8));
	push(makeint(dy));
#else
	push(makematrix(makearray(CTM_SIZE,XA_LITERAL),DPI/72.,0.,0.,DPI/72.,0.,0.)) ;
	push(makeint(XBYTES)) ;
	push(makeint(YMAX)) ;
#endif
	push(makearray(0,XA_EXECUTABLE)) ;
	framedeviceOP() ;
}

void
waittimeout(void)
{
	push(zero);
}

void
dummyop(void)
{
}

void
letter(void)
{
}

void
legal(void)
{
}

void
setjobtimeout(void)
{
	struct object time;
	time=integer("setjobtimeout");
}
void
jobname(void)
{
	push(cname("jobname"));
}

void
revision(void)
{
	push(zero);
}

static FILE *fp;
#ifdef FAX
#include "f_def.h"
#include "p_def.h"
struct {
	long part_ct;
	struct negconfig_st parms;
	long pg_ct;
} header = { 1, { FINE, X9600, G3_1D, G3, RICOH, A4_L, A4_W }, 1};
char seq_hdr[2]= { P_DS_HDR0, P_DS_HDR1};
struct {
	unsigned char pg_hdr[2];				/*per page*/
	unsigned char bs_hdr;
	unsigned char len_code;				/*always put out long length*/
	long pg_length;
	long unused;
} pgheader = {{P_PAGE_HDR0, P_PAGE_HDR1},P_PBS_HDR, 0x84,0,0};
char pg_trl[2]={P_PAGE_TLR0,P_PAGE_TLR1};
char seq_trl[2] = {P_DS_TLR0, P_DS_TLR1};
unsigned long length, slength;
long pg_offset;
long pg_count=0;
long leng_offset;
#ifdef AHMDAL
void
aswab(unsigned char val[])
{
	unsigned long nval=0;
	int i;
	for(i=3;i>=0;i--)
		nval = (nval <<8) |val[i];
	fwrite(&nval,4,1,fp);
}
void
vswab(unsigned long *val)
{
	fwrite(val, 4, 1, fp);
}
#else
void
vswab(unsigned char val[])
{
	unsigned long nval=0;
	int i;
	for(i=0;i<4;i++)
		nval = (nval <<8) |val[i];
	fwrite(&nval, 4, 1, fp);
}
void
aswab(unsigned long *val)
{
	fwrite(val, 4, 1, fp);
}
#endif
#endif
extern int dontoutput;
void
dobitmap(void)
{
#ifndef Plan9
	short dummy;
	Rectangle r, save;
	char noutname[20];
	if(dontoutput)return;
	if(cacheout){
		fprintf(fpcache," chars %d ",currentcache->charno);
		r.min.x = r.min.y = 0;
		r.max.x = currentcache->charbits->r.max.x;
		r.max.y = currentcache->charbits->r.max.y-3;
	}
	else if ( Fullbits ) {
		r.min.x = 0;
		r.min.y = 0;
		r.max.x = XMAX-1;
		r.max.y = YMAX-1;
	}
	else {
		if(minx<0)minx=0;
		if(miny<0)miny=0;
		r.min.x = minx;
		r.min.y = miny;
		r.max.x = maxx+64;
		r.max.y = maxy;
		if(maxx >XMAX)maxx = r.max.x = XMAX-1;
		if(maxy >YMAX)maxy = r.max.y = YMAX-1;
	}
#ifdef FAX
#ifndef SINGLE
	if(!pg_count){
		sprintf(noutname,"%s.mhs",outname);
		if((fp=fopen(noutname,"w")) == (FILE *)NULL){
			fprintf(stderr,"can't open output file %s\n", noutname);
			done(1);
		}
/*		fprintf(fp,"TYPE=ccitt-g31\nWINDOW=0 0 %d %d\nRES=200 200\n\n",
			bp->r.max.x,bp->r.max.y);*/
		aswab((unsigned char *)&header.part_ct);
		aswab((unsigned char *)&header.parms.resolution);
		aswab((unsigned char *)&header.parms.modem);
		aswab((unsigned char *)&header.parms.compression);
		aswab((unsigned char *)&header.parms.group);
		aswab((unsigned char *)&header.parms.machine_id);
		aswab((unsigned char *)&header.parms.length);
		aswab((unsigned char *)&header.parms.width);
		aswab((unsigned char *)&header.pg_ct);
/*							fwrite(&header, sizeof(header), 1, fp);*/
		fwrite(&seq_hdr[0], 1, 1, fp);
		fwrite(&seq_hdr[1],1,1,fp);
/*							fwrite(seq_hdr, sizeof(seq_hdr), 1, fp);*/
		pg_offset = sizeof(long)+sizeof(struct negconfig_st);
	}
	pg_count++;
	leng_offset = ftell(fp);
	fwrite(&pgheader.pg_hdr[0], 1, 1, fp);
	fwrite(&pgheader.pg_hdr[1], 1, 1, fp);
	fwrite(&pgheader.bs_hdr,1,1,fp);
	fwrite(&pgheader.len_code,1,1,fp);
	aswab((unsigned char *)&pgheader.pg_length);
	fwrite(&pgheader.unused,1,4,fp);
/*							fwrite(&pgheader, sizeof(pgheader), 1, fp);*/
	length = fwr_bm_g31(fp,bp);
	if(mdebug)fprintf(stderr,"mult length %ld\n",length);
	fwrite(&pg_trl[0],1,1,fp);
	fwrite(&pg_trl[1],1,1,fp);
/*							fwrite(pg_trl, sizeof(pg_trl),1,fp);*/
	fseek(fp, leng_offset+4, 0);
	length+=4;			/*include unusedbits in length*/
	vswab(&length);
	fseek(fp, 0L, 2);
#endif
#ifdef SINGLE
	sprintf(noutname,"%s.g31",outname);
	if((fp=fopen(noutname,"w")) == (FILE *)NULL){
		fprintf(stderr,"can't open output file %s\n", noutname);
		done(1);
	}
	fprintf(fp,"TYPE=ccitt-g31\nRES=200 200\nWINDOW=0 0 %d %d\n\n",
			bp->r.max.x,bp->r.max.y);
	fwr_bm_g31(fp,bp);
#endif
#endif
#ifdef HIRES
	if(!stdflag){
		sprintf(noutname,"%s.g4",outname);
		if((fp=fopen(noutname,"w")) == (FILE *)NULL){
			fprintf(stderr,"can't open output file %s\n", noutname);
			done(1);
		}
	}
	else fp=stdout;
	if(r.min.x%8)r.min.x = (r.min.x/8)*8;
	if((r.max.x-r.min.x)%8)r.max.x += (8 - (r.max.x-r.min.x)%8);
	fprintf(fp,"TYPE=ccitt-g4\nWINDOW=%d %d %d %d\nRES=300 300\n\n",
		r.min.x, r.min.y, r.max.x,r.max.y);
	fflush(fp);
	save = bp->r;
	bp->r = r;
	fwr_bm_g4(fp->fd, bp);		/*plan9*/
	bp->r = save;
#else
#ifndef FAX
	if(r.max.x >XMAX)maxx = r.max.x = XMAX-1;
	if(r.max.y >YMAX)maxy = r.max.y = YMAX-1;
	if(cacheout)putbitmap(currentcache->charbits, r, outname,1);
	else putbitmap(bp, r, outname, 1);
#endif
#endif
#endif
}
#ifdef FAX
#ifndef SINGLE
void
fixpgct(void)
{
	if(fp == NULL){
		fprintf(stderr,"no fax output generated\n");
		return;
	}
	fwrite(&seq_trl[0], 1, 1, fp);
	fwrite(&seq_trl[1],1,1,fp);
/*						fwrite(seq_trl, sizeof(seq_trl),1,fp);*/
	if(pg_count > 1){
		fseek(fp, pg_offset, 0);
		aswab((unsigned char *)&pg_count);
 	}
	fclose(fp);
	fprintf(stderr,"wrote %d pages\n",pg_count);
}
#endif
#endif
#ifndef Plan9
void
ckbdry(int x, int y)
{
	if(x < minx){
		minx = x;
	}
	if(x > maxx){
		maxx = x;
	}
	if(y < miny)
		miny = y;
	if(y > maxy)
		maxy = y;
}
#endif
