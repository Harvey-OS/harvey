#include <u.h>
#include <libc.h>
#include "system.h"
#ifndef Plan9
#ifndef FAX
#include "stdio.h"
#include "njerq.h"
#include "object.h"
/*
 * bitmap write code.  Stolen from Tom Killian and reworked for 5620 and jx.
 */
#define SHORTSIZE	16
#define sendword(w, f)	(putc(w&255, f), putc((w>>8)&255, f))
#undef XMAX
#define XMAX 10000
static Word raster[XMAX/WORDSIZE], buffer[XMAX/WORDSIZE];
static Bitmap bbuffer={ 0, 0, XMAX, 1, XMAX/WORDSIZE,buffer};
extern FILE *db;
static int ctype, count; static short *p1, *endraster;
extern unsigned char bytemap[256];	/* &^#%$&*@^$! */
extern int mdebug;
int Ifile;


/*
 * copy a rectangle from the given bitmap into a file on the host
 */
void
putbitmap(Bitmap *bp, Rectangle r, char *fn, int btype)
{
	register i, nrasters, rastwid, nrword;
	FILE *f;
	int k, j;
#ifdef VAX
	char *px;
	unsigned char z1;
#endif
	Rectangle rrast;
	if((f=fopen(fn, "w")) == (FILE *)NULL){
		fprintf(stderr,"can't open bitmap file %s\n", fn);
		done(1);
	}
	r.max.y += 1;
	r.max.x += 1;
	nrasters=r.max.y-r.min.y;
	i=r.max.x-r.min.x;
	if(nrasters<0 || i<0){
		fprintf(stderr,"no bitmap to write\n");
		return;
	}
	rastwid=(i+SHORTSIZE-1)/SHORTSIZE;
	nrword=(i+WORDSIZE-1)/WORDSIZE;
	endraster=(short *)raster+rastwid-1;
	if(cacheout)fprintf(fpcache, "words %d\n",nrword);
	else fprintf(stderr,"bitmap from %d %d to %d %d\n",r.min.x,r.min.y,r.max.x,r.max.y);
	i|=0x8000;
	if(!cacheout){
	if(!btype){
		sendword(nrasters, f);
		sendword(i, f);
	}
	else {
		sendword(0, f);
		sendword(r.min.x, f);
		sendword(r.min.y, f);
		sendword(r.max.x, f);
		sendword(r.max.y, f);
	}
	}
	rectf(&bbuffer, bbuffer.r, Zero);
	for(i=0;i<nrword;i++)
		raster[i]=0;
	rrast=r;
	j=0;
	for(;rrast.min.y<r.max.y;rrast.min.y++){
		rrast.max.y=rrast.min.y+1;
		bitblt(&bbuffer, Pt(0,0), bp, rrast, S);
#ifdef VAX
		px = (char *)buffer;
		for(i=0;i<nrword*4;i++){
			z1 = *px;
			*px = bytemap[z1];
			px++;
		}
#endif
#ifdef SUN
		for(i=0;i<nrword;i++){
			Word t = buffer[i];
			buffer[i] = (((t>>24) & 0xff) << 16)
				| ((t>>16) & 0xff) << 24
				| ((t>>8) & 0xff)
				| (((t) & 0xff)<<8);
		}
#endif
		if(cacheout){
			fwrite(buffer, sizeof(int *), nrword, f);
		}
		else {
		for(i=0;i<nrword;i++){
			raster[i] ^= buffer[i];
		}
		putrast(f);
		for(i=0;i<nrword;i++)
			raster[i]=buffer[i];
		buffer[i-1]=0;
		}
	}
	r.max.y -= 1;
	r.max.x -= 1;
	fflush(f);
}
void
putrast(FILE *f)
{
	register short *p2;
	p1=p2=(short *)raster;
	do{
		if(p1>=p2){
			p2=p1+1;
			count=2;
			ctype = *p1==*p2;

		}
		else if((*p2==*(p2+1))==ctype){
			if(++count>=127){
				putbits(f);
				p1=p2+2;
			}
			else
				p2++;

		}
		else if(ctype){
			putbits(f);
			p1=p2+1;
			ctype=0;

		}
		else{
			count--;
			putbits(f);
			p1=p2;
			ctype=1;
		}
	}while(p2<endraster);
	if(p1<=endraster){
		if(p2>endraster)
			count--;
		putbits(f);
	}
}
void
putbits(FILE *f)
{
	int c, i;
	unsigned char *px;
	c=count;
	if(ctype){
		c+=128;
		count=1;
	}
	putc(c, f);
#ifdef AHMDAL
	fwrite(p1, sizeof(short) , count, f);
#endif
#ifdef VAX
	px = (unsigned char *)p1;
	for(i=0;i < count * (sizeof *p1); i++, px++){
		putc((*px&0xff), f);
	}
#endif
#ifdef SUN
	px = (unsigned char *)p1;
	for(i=0;i < count; i++){
		int c[2];
		c[0] = *px++;
		c[1] = *px++;
		putc((c[1]&0xff), f);
		putc((c[0]&0xff), f);
	}
#endif
}
#endif
#endif
