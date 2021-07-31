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
#define FONTS 1
#define MAXCH 100		/*max height/width of char bitmap*/
/*
extern struct fonttab SStbl, Rtbl, CWtbl, Btbl, Itbl, Htbl, HItbl, HBtbl;
extern int SSbits[],Rbits[],CWbits[],Bbits[], Ibits[], Hbits[], HIbits[], HBbits[];
*/
char *jwidth=JWIDTH;
char *jfont=JFONT;
char *tnames[] = { "SSn", "Rn", "CWn", "Bn", "In", "BIn", "Hn", "HIn", "HBn", "HBOn", 0};
char *pnames[] = {"Symbol", "Times-Roman", "Courier", "Times-Bold","Times-Italic", "Times-BoldItalic",
	"Helvetica", "Helvetica-Oblique", "Helvetica-Bold","Helvetica-BoldOblique"};
/*
struct fonttab *ftbl[] = { &SStbl, &Rtbl, &CWtbl, &Btbl, &Intbl, &BIntbl, &Htbl, &HItbl, &HBtbl, 0};
int *fontbits[] = {SSbits, Rbits, CWbits, Bbits, Inbits, BInbits, Hbits, HIbits, HBbits, 0};
*/
char *altnames[] = {"Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",0};
struct object names[8];
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
int mdebug = 0, dontcache, dontoutput;
int maxw, stdflag, width;
char *outname;
void main(int, char *[]), initfont(void);
int getwidths(char *, double[]);
void mkchar(struct dict *,myFont *, int, struct object, double, double[], int);
myFont *getfont(char[], double[]);
struct object getbits(myFont *, int);
int fnum;

void
main(int argc, char *argv[]){
	char **t;
	int i;
	if(argc != 2){
		fprintf(stderr,"specify font number from:\n");
		for(t=tnames, i=0;*t != 0;t++, i++)
			fprintf(stderr,"%s %d\n",*t,i);
		exit(0);
	}
	fnum = atoi(argv[1]);
	init((char *)0);
	exit(0);
}
void
initfont(void)
{
	struct dict *fdict, *chardict;
	struct object fdobj, cdictobj, chardata;
	struct object null, array;
	struct fonttab *fonttbl;
	int *fontbits;
	unsigned char *cp;
	char namebuf[50], *p, *charname;
	static myFont  *special, *roman;
	static struct object three, imagemx, fontmx, buildoper, encoding;
	static struct fstorage{
		struct object Dict;
		struct object Cdict;
		struct object Array;
	} Fontstorage[FONTS], *fptr = Fontstorage;
	static struct object cnames[256];
	double swidths[256], widths[256];
	int maxs, maxf;
	myFont *fp;
	Fontchar *fc;
	double base;
	int k, j, n, index, i;
	sprintf(namebuf,"%s/%s.24",jfont,tnames[0]);
	special = getfont(tnames[fnum], swidths);
/*	if(!getwidths(tnames[0], swidths))
		for(j=0;j<256;j++)swidths[j] = 0.;*/
	maxs = maxw;
	sprintf(namebuf,"%s/%s.24",jfont,tnames[1]);
/*	roman = getfont(namebuf, 0);
	if(!getwidths(tnames[1], widths))
		for(j=0;j<128;j++)widths[j] = 0.;*/
	maxf = maxw;
	zero = makereal(0.);
	blank = cname("question");
	for(j=0; j<FONTS; j++, fptr++){
		fptr->Dict = makedict(12);
		fptr->Cdict = makedict(256);
		fptr->Array = makearray(4, XA_LITERAL);
		fptr->Array.value.v_array.object[0] =
			fptr->Array.value.v_array.object[1] = zero;
		names[j] = cname(pnames[j]);
	}
	fptr = Fontstorage;
	three = makeint(3);
	imagemx = makematrix(makearray(CTM_SIZE, XA_LITERAL),
		1., 0.0, 0.0, -1., 0.0, 0.0);
	fontmx = makematrix(makearray(CTM_SIZE, XA_LITERAL),
		1./33., 0., 0.,1./33., 0., 0.);
	buildoper = makeoperator(BuildChar);
	encoding = dictget(Systemdict, cname("StandardEncoding"));
	for(j=0;j<128;j++){
		if(se_tab[j].length == 0)
			cnames[j] = none;
		else cnames[j] = pckname(se_tab[j],XA_LITERAL);
	}
	for(j=160;j<256;j++){
		if(se_tab[j].length ==0)
			cnames[j]=none;
		else cnames[j] = pckname(se_tab[j],XA_LITERAL);
	}
	for(k=0; k<FONTS; k++){
findex=0;
fprintf(stderr,"%s ",tnames[fnum]);
		switch(k){
		case 0: fp = special;
			maxw = maxs;
			break;
		case 1: fp = roman;
			maxw = maxf;
			break;
		default:
			sprintf(namebuf,"%s/%s.24",jfont,tnames[k]);
			/*fp = getfont(namebuf, 1);*/
			if(!getwidths(tnames[k],widths))
				for(j=0;j<128;j++)widths[j] = 0.;
		}
		if(fp == (myFont *)0){
			fprintf(stderr,"can't open file %s/%s.24\n",jfont,
				tnames[k]);
			exit(0);
		}
		fdobj = fptr->Dict;
		fdict = fdobj.value.v_dict;
		dictput(fdict, FontType, three);
		dictput(fdict, FontMatrix, fontmx);	/*fontmx*/
		if(k){
			dictput(fdict, Encoding, encoding);
			dictput(fdict, CharStrings, Charstrings);
		}
		else {
			dictput(fdict, Encoding, dictget(Systemdict, cname("SymbolEncoding")));
			dictput(fdict, CharStrings, SCharstrings);
		}
		dictput(fdict, FontInfo, Fontinfo);
		dictput(fdict, Buildchar, buildoper);
		dictput(fdict, ImageMatrix, imagemx);
fprintf(stderr,"%d %d %d\n",fp->height,maxw,fp->ascent);
		base = (double)fp->height;
		dictput(fdict, BHeight, makereal(base));
		array = fptr->Array;
		array.value.v_array.object[2] = makereal((double)maxw);
		array.value.v_array.object[3] = makereal(base);
		dictput(fdict, FontBBox, array);
		cdictobj = fptr->Cdict;
		chardict = cdictobj.value.v_dict;
		if(!k){
			for(j=0;j<256;j++){
				if(sye_tab[j].length == 0)continue;
				mkchar(chardict,fp,j,pckname(sye_tab[j],XA_LITERAL),base,swidths,j);
/*				if(rsteal[j] != 0){
					if(sye_tab[(int)rsteal[j]].length!= 0)
					 mkchar(chardict,roman,j,
						pckname(sye_tab[(int)rsteal[j]],XA_LITERAL), base,widths,(int)rsteal[j]);
					continue;
				}*/
			}
/*			for(j=160;j<256;j++){
				n = spmap[j-160];
				if(sye_tab[j].length == 0 || n == 0)continue;
				mkchar(chardict,fp,n,pckname(sye_tab[j],XA_LITERAL),base,swidths,j);
			}*/
		}
		else{
			for(j=0;j<fp->n;j++){
				if(ssteal[j] != 0)
				 mkchar(chardict,special,j,cnames[ssteal[j]],base,swidths,ssteal[j]);
				if(j < 8)index = (int)bmap[j];
				else index = j;
				if(index != 0 && cnames[index].type != OB_NONE)
				   mkchar(chardict, fp, j, cnames[index], base,widths,index);
			}
			for(j=160;j<256;j++){
				n = rmap[j-160];
				if(cnames[j].type != OB_NONE && n != 0)
					mkchar(chardict, fp, n, cnames[j], base,widths,j);
			}
		}
		dictput(fdict, Chardata, cdictobj);
		push(names[k]);
		push(fdobj);
		definefontOP();
		pop();
		if(k == 3){
			for(i=0;altnames[i] != 0;i++){
				dictput(fdict,Chardata,cdictobj);
				push(cname(altnames[i]));
				push(fdobj);
				definefontOP();
				pop();
			}
		}
		fptr++;
	}
}
void
mkchar(struct dict *chardict,myFont *fp, int j,struct object charname,
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
/*	if(fc->left&0200)sleft=fc->left - 256;
	else sleft=fc->left;*/
	sleft = fc->left;
fprintf(stderr,"%d %d %d %f %f ",fc->width,rheight,sleft,
	wid[j],base-(double)(fc->top));
#else	
fprintf(stderr,"%d %d %d %f %f ",fc->width,rheight,fc->left,
	wid[j],base-(double)(fc->top));
#endif
	chardata = makearray(6, XA_LITERAL);
	chardata.value.v_array.object[0] = makeint(fc->width);
	chardata.value.v_array.object[1] = makeint(fc->bottom-fc->top +1);
	if(wid[j] == 0.){
	chardata.value.v_array.object[2] = makereal((double)(fc->width));
	}
	else{ chardata.value.v_array.object[2] = makereal(wid[j]);
	}
	chardata.value.v_array.object[3] = makereal((double)fc->left);
	chardata.value.v_array.object[4] = makereal(base-(double)fc->top);
	chardata.value.v_array.object[5] = getbits(fp,j);
	dictput(chardict, charname, chardata);
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
	if(b == (Bitmap *)0)b=balloc(Rect(0, 0, MAXCH,MAXCH),0);
	else rectf(b, Rect(0, 0, MAXCH,MAXCH),Zero);
	fc = fp->info+c;
	height = fc->bottom - fc->top + 1;
/*if(c == 254){fprintf(stderr,"height %d bot %d top %d wid %d %d\n",height,fc->bottom,fc->top,fc->width,(fc->width+7)/8);
fprintf(stderr," x %d %d\n",fc->x,(fc+1)->x);}*/
	bitblt(b,Pt(0,0),fp->bits, Rect(fc->x,fc->top,(fc+1)->x,fc->bottom),S);
	bt = bits;
	sp = b->base;
	for(k=height;k>0;k--){
		ch = (unsigned char *)sp;
		for(j=0;j < (fc->width+7)/8; j++,ch++){
#ifdef VAX
			*bt++ = bytemap[*ch];	/* map only for vax */
#else
			*bt++ = *ch;
/*if(c == 254)fprintf(stderr,"%x ",*(bt-1));*/
#endif
		}
/*if(c == 254)fprintf(stderr,"\n");*/
		sp += b->width;
	}
	size = bt-bits;
fprintf(stderr,"%d\n",size);
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
	char buf[50];
	FILE *fin;
	int i;
	int x;
	sprintf(buf,"%s/%s.widths",jwidth,s);
	if((fin = fopen(buf,"r")) == NULL){
		/*fprintf(stderr,"can't open file %s\n",buf);*/
		return(0);
	}
	for(i=0;i<128;i++){
		fgets(buf,20,fin);
		x = atoi(buf);
		if(x)
			wid[i] = ((double)x*2.4/720.)*100.;
		else wid[i] = 0.;
		if(maxw < (int)wid[i])maxw = (int)wid[i];
	}
	fclose(fin);
	return(1);
}
