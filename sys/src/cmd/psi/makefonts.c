#include <u.h>
#include <libc.h>
#include <libg.h>
#include "system.h"
#include "stdio.h"
#include "defines.h"
#include "object.h"
#include "dict.h"
#include "njerq.h"
#include "font.h"
#include "map.h"
#define MAXCH 100		/*max height/width of char bitmap*/
char *tnames[] = { "SSn", "Rn", "Bn", "In", "BIn", "Hn", "HIn", "HBn", "HBOn","CWn", 0};
extern struct dict *Systemdict;
extern struct pstring se_tab[], sye_tab[];
#ifdef VAX
extern unsigned char bytemap[256];
#endif
extern struct object FontMatrix, ImageMatrix, FontBBox, Encoding, Buildchar,
	FontType, BHeight, Chardata, none, FID, FontDirectory, CharStrings,
	SCharstrings, Charstrings, FontInfo, Fontinfo;
struct object blank, zero;
int findex;
int  dontcache, dontoutput;
int maxw, stdflag, width;
char *outname;
void main(int, char *[]), initfont(void);
int getwidths(char *, double[]);
void mkchar(myFont *, int, struct object, double, double[], int);
myFont *getfont(char[], int);
struct object getbits(myFont *, int);
int fnum;
double widths[256];

void
main(int argc, char *argv[]){
	char **t;
	int i;
	if(argc == 2 && *argv[1] == '-'){
		fprintf(stderr,"specify font number from:\n");
		for(t=tnames, i=0;*t != 0;t++, i++)
			fprintf(stderr,"%s %d\n",*t,i);
		exits("error");
	}
	if(argc == 2 && *argv[1] >= '0' && *argv[1] <= '9')
		fnum = atoi(argv[1]);
	else fnum = -1;
	init((char *)0);
	exits("");
}
void
initfont(void)
{
	struct fonttab *fonttbl;
	int *fontbits;
	unsigned char *cp;
	char namebuf[50], *p, *charname;
	static struct object cnames[256];
	myFont *fp;
	Fontchar *fc;
	double base;
	int k, j, n, index, i;
	for(j=0;j<256;j++){
		if(se_tab[j].length != 0)
			cnames[j] = pckname(se_tab[j],XA_LITERAL);
	}
	if(fnum < 0)k=0;
	else k=fnum;
	for(; tnames[k] != 0 && k <= fnum; k++){
 findex=0;
 fprintf(stderr,"%s ",tnames[k]);
		sprintf(namebuf,"%s.16.1",tnames[k]);
		fp = getfont(namebuf, 0);
		getwidths(tnames[k],widths);
		if(fp == (myFont *)0){
			fprintf(stderr,"can't open file %s.16.1\n", tnames[k]);
			exits("error");
		}
		base = (double)fp->height;
 fprintf(stderr,"%d %d %d\n",fp->height,maxw,fp->ascent);
		if(!k){
			for(j=0;j<256;j++){
				if(sye_tab[j].length != 0)
				mkchar(fp,j,pckname(sye_tab[j],XA_LITERAL),base,widths,j);
			}
		}
		else{
			for(j=0;j<256;j++){
				if(se_tab[j].length != 0)
				mkchar(fp,j,cnames[j],base,widths,j);
			}
		}
	}
}
void
mkchar(myFont *fp, int j,struct object charname,
 double base, double wid[], int rind)
{
	Fontchar *fc;
	struct object chardata;
	int sleft, rheight;

	fc = fp->info + j;
	if(fc->width == 0){
		return;
	}
	rheight = fc->bottom-fc->top+1;
	if(base-(double)(fc->top) < rheight)
		rheight = (int)(base-(double)(fc->top));
 fprintf(stderr,"char %d %s index %d ",rind,charname.value.v_string.chars,findex);
#ifdef AHMDAL
	if(fc->left&0200)sleft=fc->left - 256;
	else sleft=fc->left;
	sleft = fc->left;
 fprintf(stderr,"%d %d %d %f %f ",fc->width+abs(sleft),rheight,sleft,
	wid[j],base-(double)(fc->top));
#else	
 fprintf(stderr,"%d %d %d %f %f ",fc->width+abs(fc->left),rheight,fc->left,
	wid[j],base-(double)(fc->top));
#endif
	getbits(fp,j);
}
int junk=0;
struct object
getbits(myFont *fp, int c)
{
	unsigned char bits[500], *bt;
	unsigned char *ch;
	struct object obj;
	int j, k,size,height,l=0;
	unsigned int cbits;
	unsigned char  *cx;
	static Bitmap *b;
	Fontchar *fc;
	Word *sp;
	int sleft;
	if(b == (Bitmap *)0)b=balloc(Rect(0, 0, MAXCH,MAXCH),0);
	else rectf(b, Rect(0, 0, MAXCH,MAXCH),Zero);
	fc = fp->info+c;
	height = fc->bottom - fc->top + 1;
	bitblt(b,Pt(0,0),fp->bits, Rect(fc->x,fc->top,(fc+1)->x,fc->bottom),S);
	bt = bits;
	sp = b->base;
#ifdef AHMDAL
	if(fc->left&0200)sleft = fc->left-256;
	else sleft = fc->left;
	sleft = fc->left;
#else
	sleft = fc->left;
#endif
	for(k=height;k>0;k--){
		ch = (unsigned char *)sp;
		for(j=0;j < (int)(fc->width+abs(sleft)+7)/8; j++,ch++){
#ifdef VAX
			*bt++ = bytemap[*ch];	/* map only for vax */
#else
			*bt++ = *ch;
		}
		sp += b->width;
	}
	size = bt-bits;
 fprintf(stderr,"%d %d %d\n",size,(fc+1)->x-fc->x,(int)(fc->width+abs(fc->left)+7)/8);
	obj = makestring(size);
	ch =obj.value.v_string.chars;
	cx =(unsigned char *) &cbits;
	for(k=j=0, bt=bits;k<size;k++){
		*ch++ = *bt++;
		*cx++ = *(ch-1);
 j++;
 if(!(j%4)){l++;findex++;
 if(!(l%6))fprintf(stderr,"\n0x%x,",cbits);
 else fprintf(stderr,"0x%x,", cbits); j=0;cx =(unsigned char *) &cbits;}
	}
 if(j%4){fprintf(stderr,"0x%x,\n",cbits);findex++;}
 else fprintf(stderr,"\n");
	return(obj);
}
int
getwidths(char *s, double wid[])
{
	char buf[100];
	FILE *fin;
	int i;
	int x;
	sprintf(buf,"%s.widths.24",s,s);
	if((fin = fopen(buf,"r")) == NULL){
		fprintf(stderr,"can't open file %s\n",buf);
		return(0);
	}
	while(1){
		if(fgets(buf,50,fin) == 0)break;
		i=atoi(buf);
		wid[i] = atof(buf+3);
		if(maxw < wid[i])maxw = wid[i];
	}
	fclose(fin);
	return(1);
}
