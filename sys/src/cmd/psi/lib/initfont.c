#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
#include "stdio.h"
#include "defines.h"
#include "object.h"
#include "dict.h"
#include "njerq.h"
#include "font.h"
#include "finit.h"

#define FONTS 10
#define MAXCH 50		/*max height/width of char bitmap*/

extern struct fonttab SSntbl[], Rntbl[], CWntbl[], Bntbl[], Intbl[], BIntbl[], Hntbl[], HIntbl[], HBntbl[], HBOntbl[];
extern int SSnbits[],Rnbits[],CWnbits[],Bnbits[], Inbits[], BInbits[], Hnbits[], HInbits[], HBnbits[], HBOnbits[];
extern int SSnfconst[], Rnfconst[],CWnfconst[],Bnfconst[],Infconst[],BInfconst[],Hnfconst[],
	HInfconst[], HBnfconst[], HBOnfconst[];

char *tnames[] = { "SSn", "Rn", "CWn", "Bn", "In", "BIn", "Hn", "HIn", "HBn", "HBOn", 0};
char *pnames[] = {"Symbol", "Times-Roman", "Courier", "Times-Bold","Times-Italic", "Times-BoldItalic",
	"Helvetica", "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique"};

struct fonttab *ftbl[] = { SSntbl, Rntbl, CWntbl, Bntbl, Intbl, BIntbl, Hntbl, HIntbl, HBntbl, HBOntbl, 0};
int *fbits[] = {SSnbits, Rnbits, CWnbits, Bnbits, Inbits, BInbits, Hnbits, HInbits, HBnbits, HBOnbits, 0};
int *fconst[] = {SSnfconst, Rnfconst,CWnfconst,Bnfconst,Infconst,BInfconst,Hnfconst,HInfconst,
	HBnfconst, HBOnfconst};

char *altnames[] = {"Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",0};
struct object names[FONTS];

extern unsigned char bytemap[256];

void
initfont(void)
{
	struct dict *fdict, *chardict;
	struct object fdobj, cdictobj, chardata;
	struct object null, array, temp;
	struct fonttab **fonttbl;
	int *fontbits, *fontconst;
	char *p, *charname;
	static struct object three, imagemx, fontmx, buildoper, encoding;
	static struct fstorage{
		struct object Dict;
		struct object Cdict;
		struct object Array;
	} Fontstorage[FONTS], *fptr = Fontstorage;
	static struct object cnames[256];
	double base;
	int k, j, n, index, i;

	blank = cname("question");
	n_FontName = cname("FontName");
	for(j=0; j<FONTS; j++, fptr++){
		fptr->Dict = makedict(14);
		fptr->Cdict = makedict(256);
		fptr->Array = makearray(4, XA_LITERAL);
		fptr->Array.value.v_array.object[0] =
			fptr->Array.value.v_array.object[1] = zero;
		fptr->Array.xattr = XA_EXECUTABLE;
		names[j] = cname(pnames[j]);
	}
	fptr = Fontstorage;
	three = makeint(3);
	imagemx = makematrix(makearray(CTM_SIZE, XA_LITERAL),
		1., 0.0, 0.0, -1., 0.0, 0.0);
	fontmx = makematrix(makearray(CTM_SIZE, XA_LITERAL),
		1./33.33333, 0., 0.,1./33.33333, 0., 0.);
	buildoper = makeoperator(BuildChar);
	encoding = dictget(Systemdict, cname("StandardEncoding"));
	for(j=0;j<256;j++){
		if(se_tab[j].length == 0)
			cnames[j] = none;
		else cnames[j] = pckname(se_tab[j],XA_LITERAL);
	}
	for(k=0; k<FONTS; k++){
		fontbits = fbits[k];
		fonttbl = &ftbl[k];
		fontconst = fconst[k];
		fdobj = fptr->Dict;
		fdict = fdobj.value.v_dict;
		loaddict(fdict, FontType, &three);
		loaddict(fdict, FontMatrix, &fontmx);	/*fontmx*/
		if(k){
			loaddict(fdict, Encoding, &encoding);
			loaddict(fdict, CharStrings, &Charstrings);
		}
		else {
			temp = dictget(Systemdict, cname("SymbolEncoding"));
			loaddict(fdict, Encoding, &temp);
			loaddict(fdict, CharStrings, &SCharstrings);
		}
		loaddict(fdict, FontInfo, &Fontinfo);
		loaddict(fdict, Buildchar, &buildoper);
		loaddict(fdict, ImageMatrix, &imagemx);
		base = (double)*fontconst;
		temp = makereal(base);
		loaddict(fdict, BHeight, &temp);
		temp = makereal((double)*(fontconst+2));
		loaddict(fdict, Ascent, &temp);
		array = fptr->Array;
		array.value.v_array.object[2] = makereal((double)*(fontconst+1));
		array.value.v_array.object[3] = makereal(base);
		loaddict(fdict, FontBBox, &array);
		cdictobj = fptr->Cdict;
		chardict = cdictobj.value.v_dict;
		if(!k){
			for(j=0;j<128;j++){
				if(sye_tab[j].length > 0)
				mkchar(chardict, pckname(sye_tab[j],XA_LITERAL),*fonttbl+j,fontbits);
				else mkchar(chardict, none, *fonttbl+j,fontbits);
			}
			for(j=160; j<256; j++){
				if(sye_tab[j].length > 0)
				mkchar(chardict, pckname(sye_tab[j],XA_LITERAL),*fonttbl+j,fontbits);
				else mkchar(chardict, none, *fonttbl+j,fontbits);
			}
		}
		else {
			for(j=0;j<256;j++)
				mkchar(chardict,cnames[j], *fonttbl+j, fontbits);
		}
		loaddict(fdict, Chardata, &cdictobj);
		loaddict(fdict, n_FontName, &names[k]);
		push(names[k]);
		push(fdobj);
		definefontOP();
		pop();
		if(k == 2){
			for(i=0;altnames[i] != 0;i++){
				push(cname(altnames[i]));
				push(fdobj);
				definefontOP();
				pop();
			}
		}
		fptr++;
	}
	for(i=1;i<32; i++)
		encoding.value.v_array.object[i] = notdef;
	for(i=128; i<159; i++)
		encoding.value.v_array.object[i] = notdef;
	for(i=209; i<225; i++)
		encoding.value.v_array.object[i] = notdef;
	for(i=228; i<232; i++)
		encoding.value.v_array.object[i] = notdef;

}
void
mkchar(struct dict *chardict, struct object charname, struct fonttab *ft,
	int bits[])
{
	struct object chardata;
	if(!ft->width)return;
	chardata = makearray(6, XA_LITERAL);
	chardata.value.v_array.object[0] = makeint(ft->width);
	chardata.value.v_array.object[1] = makeint(ft->height);
	if(ft->fwid == 0.)
		chardata.value.v_array.object[2] = makereal((double)(ft->width));
	else chardata.value.v_array.object[2] = makereal(ft->fwid);
	chardata.value.v_array.object[3] = makereal((double)ft->left);
	chardata.value.v_array.object[4] = makereal(ft->basetotop);
	chardata.value.v_array.object[5] = makebits(bits,ft->index,ft->size);
	loaddict(chardict, charname, &chardata);
}
struct object
makebits(int *bits, int bindex, int size)
{
	struct	object	object ;

	object.type = OB_STRING ;
	object.xattr = XA_LITERAL ;
	if ( size < 0 )
		pserror("rangecheck", "makebits size<0") ;
	if ( size > STRING_LIMIT ){
		fprintf(stderr,"size %d\n",size);
		pserror("rangecheck", "makebits size>limit") ;
	}
	object.value.v_string.access = AC_UNLIMITED ;
	object.value.v_string.length = size ;
	object.value.v_string.chars = (unsigned char *)(bits+bindex) ;
	return(object) ;
}
