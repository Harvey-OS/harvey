#include <u.h>
#include <libc.h>
# include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "dict.h"
# include "path.h"
#include "njerq.h"
# include "graphics.h"
#include "cache.h"

struct pspoint texwidth;
int texfont;

void
definefontOP(void)
{
	struct	object	font, key, junk, junk1 ;
	font = fontcheck("definefont") ;
	key = pop() ;
	junk = dictget(Systemdict,FontDirectory);
	if(key.type == OB_NAME || key.type == OB_STRING)
		junk1 =dictget(junk.value.v_dict,key);
	else junk1 = odictget(junk.value.v_dict,key);	/*tex's key is a dict!!!*/
	if(junk1.type != OB_NONE){
		push(junk1);
		return;
	}
	SAVEITEM(font.value.v_dict->access) ;
	font.value.v_dict->access = AC_READONLY ;
	dictput(font.value.v_dict,FID,makefid()) ;
	dictput(junk.value.v_dict,key,font) ;
	push(font) ;
}

struct	object
fontcheck(char *caller)
{
	int		i ;
	struct	object	font, object, junk ;

	font = pop() ;
	if ( font.type != OB_DICTIONARY ){
		fprintf(stderr,"font.type %d %s %d\n",font.type,font.value.v_string.chars,
			font.xattr);
		pserror("typecheck", caller) ;
	}
	realmatrix(dictget(font.value.v_dict,FontMatrix)) ;
	junk = dictget(font.value.v_dict,FontType);
	if ( junk.type != OB_INT ){
		fprintf(stderr,"fonttype %d ",
			junk.type);
		pserror("invalidfont", caller) ;
	}

	object = dictget(font.value.v_dict,FontBBox) ;
	if ( object.type != OB_ARRAY )
		pserror("invalidfont FontBBox", caller) ;
	if ( object.value.v_array.length != 4 ){
		fprintf(stderr,"length %d ",object.value.v_array.length);
		pserror("invalidfont length", caller) ;
	}
	for ( i=0 ; i<object.value.v_array.length ; i++ )
		if(object.value.v_array.object[i].type == OB_INT ||
			 object.value.v_array.object[i].type == OB_REAL )continue;
		else {
		fprintf(stderr,"type %d ",object.value.v_array.object[i].type);
			pserror("invalidfont box obj", caller) ;
		}

	object = dictget(font.value.v_dict,Encoding) ;
	if ( object.type != OB_ARRAY ){
		fprintf(stderr,"type %d ",object.type);
		pserror("invalidfont encoding type", caller) ;
	}
	for ( i= 0 ; i<object.value.v_array.length ; i++ )
		if ( object.value.v_array.object[i].type == OB_NAME ||
			 object.value.v_array.object[i].type == OB_NULL )continue;
		else pserror("invalidfont encoding obj", caller) ;
	return(font) ;
}
static fct=0;
static char namebuf[7000];
static struct {
char *fontnam, *filenam;
int length;
}ftbl[240], *fptr, *fend;
void
findfontOP(void)
{
	struct	object	key, font, junk, file,fname;
	int i, j, saveLine_no, first, previous;
	static int loaded=0;
	static int ptable[52];
	char buf[50], *b, c, *str, *s, buf1[50];
	FILE *f;
	key = pop();
	junk = dictget(Systemdict,FontDirectory);
	font =dictget(junk.value.v_dict,key);
	if(font.type !=  OB_NONE ){
		push(font);
		return;
	}
	if(!searchflg)goto ret;
	if(!loaded){
		loaded++;
		f = fopen("/usr/llc/Newfonts/fonts/Name.tbl","r");
		if(f == (FILE *)NULL){
			fprintf(stderr,"can't open Name.tbl\n");
			goto ret;
		}
		fptr = ftbl;
		b = namebuf;
		fptr->fontnam = b;
		first = 0;
		while((c = fgetc(f)) != EOF){
			if(c == ' '){
				*b++ = '\0';
				fptr->filenam = b;
			}
			else if(c == '\n'){
				fptr->length = b - fptr->filenam;
				*b++ = '\0';
				(++fptr)->fontnam = b;
				first = 0;
			}
			else {
				*b++ = c;
				if(!first++ && c != previous){
					if(c < 'Z')
						ptable[c-'A'] = fptr-ftbl;
					else ptable[c-'a' + 26] = fptr-ftbl;
					previous = c;
				}
			}
		}
		fend = fptr;
		fclose(f);
	}
	str = (char *)key.value.v_string.chars;
	if(*str < 'Z')i = *str - 'A';
	else i = *str - 'a' + 26;
	i = ptable[i];
	for(fptr = &ftbl[i]; fptr <= fend; fptr++){
		b=fptr->fontnam;
		if(*str != *b){
			goto ret;
		}
		for(s=str; *b != '\0'; s++, b++)
			if(*s != *b)break;
		if((*b == '\0') && (s-str == key.value.v_string.length))break;
	}
	sprintf(buf1,"/usr/llc/Newfonts/fonts/%s",fptr->filenam);
	if((f=fopen(buf1, "r")) == (FILE *)NULL){
		fprintf(stderr,"%s can't open %s\n", str, buf1);
		goto ret;
	}
	fname = makestring(24 + fptr->length);
	for(b=buf1, i=0;*b != '\0'; b++)
		fname.value.v_string.chars[i++] = *b;
	saveLine_no = Line_no;
	currentfileOP();
	push(fname);
	runOP();
	execute();
	file=pop();
	currentfile=file.value.v_file.fp;
	Line_no = saveLine_no;
	font =dictget(junk.value.v_dict,key);
ret:	if(font.type ==  OB_NONE )
		font = dictget(junk.value.v_dict, unknown);
	push(font);
}

void
scalefontOP(void)
{
	dupOP() ;
	push(makearray(CTM_SIZE,XA_LITERAL)) ;
	scaleOP() ;
	makefontOP() ;
}

void
setfontOP(void)
{
	struct object font, junk;
	int i;
	font = fontcheck("setfont");
	Graphics.font = font.value.v_dict ;
	junk = dictget(Graphics.font, FontType);
	current_type = junk.value.v_int;
	junk = dictget(Graphics.font, cname("BitMaps"));
	if(junk.type != OB_NONE)
		texfont=1;
	else {
		junk = dictget(Graphics.font, cname("CharDict"));
		if(junk.type != OB_NONE)
			texfont = 1;
		else texfont=0;
	}
	if(cacheout||((merganthal||korean)&&current_type == 3)){
		if(merganthal||korean){
			fprintf(stderr,"begin font\nfontname "); fflush(stderr);
			fpcache = stderr;
		}
		junk = dictget(Graphics.font,n_FontName);
		if(junk.type != OB_NONE){
		for(i=0;i<junk.value.v_string.length; i++)
			putc(junk.value.v_string.chars[i],fpcache);
		if(merganthal||korean)putc('\n',fpcache);
		else putc(' ', fpcache);
		}
		else fprintf(stderr,"name not found\n");
		fflush(fpcache);
	}
	fontchanged = 1;
}

void
currentfontOP(void)
{

	struct	object	font ;

	font.type = OB_DICTIONARY ;
	font.xattr = XA_LITERAL ;
	font.value.v_dict = Graphics.font ;
	push(font) ;
}

void
makefontOP(void)
{
	struct	object	font, fontprime, fontptr, matrix, gmatrix;
	struct	object	Fontmatrix;

	matrix = pop() ;
	font = fontcheck("makefont") ;
	fontprime = makedict(font.value.v_dict->max) ;
	push(font) ;
	push(fontprime) ;
	copyOP() ;
	dupOP() ;
	dupOP() ;
	push(FontMatrix) ;
	getOP() ;
	push(matrix) ;
	exchOP() ;
	push(makearray(CTM_SIZE,XA_LITERAL)) ;
	concatmatrixOP() ;
	push(FontMatrix) ;
	exchOP() ;
	putOP() ;
}

void
showOP(void)
{
	struct	object	pstring;

	pstring = pop() ;
	if ( pstring.type != OB_STRING ){
		fprintf(stderr,"type %d ",pstring.type);
		pserror("typecheck", "show") ;
	}
	show(zeropt,-1,zeropt,none,pstring) ;
}

void
ashowOP(void)
{
	struct pspoint a ;
	struct	object	pstring;
	pstring = pop() ;
	if ( pstring.type != OB_STRING )
		pserror("typecheck", "ashow") ;
	a = poppoint("ashow");
	show(zeropt,-1,a,none,pstring) ;
}

void
widthshowOP(void)
{
	int		charcode ;
	struct pspoint		c ;
	struct	object	pstring,object;

	pstring = pop() ;
	if ( pstring.type != OB_STRING )
		pserror("typecheck", "widthshow") ;
	object = integer("widthshow");
	charcode = object.value.v_int ;
	c = poppoint("widthshow");
	show(c,charcode,zeropt,none,pstring) ;
}

void
awidthshowOP(void)
{
	int		charcode ;
	struct pspoint		c, a ;
	struct	object	pstring, object;
	pstring = pop() ;
	if ( pstring.type != OB_STRING )
		pserror("typecheck", "awidthshow") ;
	a = poppoint("awidthshow");
	object = integer("awidthshow");
	charcode = object.value.v_int ;
	c = poppoint("awidthshow");
	show(c,charcode,a,none,pstring) ;
}

void
kshowOP(void)
{
	struct	object	proc, pstring;

	pstring = pop() ;
	if ( pstring.type != OB_STRING )
		pserror("typecheck", "kshow") ;
	proc = procedure() ;
	show(zeropt,-1,zeropt,proc,pstring) ;
}

void
show(struct pspoint c, int charcode, struct pspoint a, struct object proc,
	struct object pstring)
{
	int		i,j;
	struct	pspoint	width;
	struct	object	*encoding , intchar, junk;
	struct object	charstring;
	struct	dict	*charstrings ;
	unsigned char *ss;
	struct object	fobj;
	a = dtransform(a);
	c = dtransform(c);
	if(microsoft){
		a.y = 0.;
		c.y = 0.;
	}
	show_save();
	if(fontchanged){
		j = findfno();
	}
	ss = pstring.value.v_string.chars;
	for ( i=0 ; i<pstring.value.v_string.length ; i++,ss++ ){
		if ( current_type == 3 ){
			gsaveOP() ;
			texcode = *ss;			/*kluge for tex lies*/
			intchar = makeint((int)*ss);
 if(mdebug||bbflg)fprintf(stderr,"char %c %x ",*ss&0177,*ss);
			if(merganthal||korean){
			   if(texcode == 040)fprintf(stderr,"code sp %o %d\n", texcode,texcode);
			   else if(texcode == 011)fprintf(stderr,"code ht %o %d\n", texcode,texcode);
			   else if(texcode == 012)fprintf(stderr,"code nl %o %d\n", texcode,texcode);
			   else fprintf(stderr,"code %c %o %d\n",texcode,texcode,texcode);
			}
			push(intchar) ;
			if(!putcachech()){
				currentfontOP();
				push(intchar);
				execpush(dictget(Graphics.font,Buildchar));
				execute();
				if(cacheit>0){
					push(intchar);
					mcachechar();
				}
				else Graphics.width = texwidth;		/*kluge if didn't cache tex*/
				cacheit = 0;
				if(fontchanged)j=findfno();
			}
			width = add_point(Graphics.width, a) ;
			if ( charcode == (int)*ss )
				width = add_point(width,c);
			grestoreOP() ;
			width = idtransform(width);
			push(makereal(width.x)) ;
			push(makereal(width.y)) ;
			translateOP() ;
		} else if ( current_type == 1 ) {
			gsaveOP();
			intchar = makeint((int)*ss);
/*fprintf(stderr,"type1 char %c %x\n",*ss&0177,*ss);*/
			push(intchar) ;
			if(!putcachech()){
				ExecCharString(Graphics.font, (int)*ss, 0, false);
				if(cacheit>0){
					push(intchar);
					mcachechar();
				}
				cacheit = 0;
				if(fontchanged)j=findfno();
			}
			width = add_point(Graphics.width, a);
			if ( charcode == (int)*ss )
				width = add_point(width, c);
			grestoreOP();
			width = idtransform(width);
			push(makereal(width.x));
			push(makereal(width.y));
			translateOP();
		}
		else{
		  junk = dictget(Graphics.font,Encoding);
		  encoding = junk.value.v_array.object;
		  junk = dictget(Graphics.font,cname("CharStrings"));
		  charstrings = junk.value.v_dict ;
			execpush(dictget(charstrings,encoding[(int)*ss])) ;
			execute() ;
		}
		if(proc.type != OB_NONE && i < pstring.value.v_string.length-1 ){
			show_restore() ;
			push(makeint((int)*ss)) ;
			push(makeint((int)*(ss+1))) ;
			execpush(proc) ;
			execute() ;
			show_save() ;
			j=findfno();
		}
	}		
	show_restore() ;
}
void
show_save(void)
{
	struct object matrix;
	if(Graphics.font == (struct dict *)0){
		pserror("no current font defined", "show_save");
	}
	gsaveOP() ;
	Graphics.inshow = 1 ;
	currentpointOP() ;
	translateOP() ;
	currentpointOP() ;
	newpathOP() ;
	movetoOP() ;
	matrix = dictget(Graphics.font,FontMatrix);
	push(dictget(Graphics.font,FontMatrix)) ;
	concatOP() ;
}

void
show_restore(void)
{
	struct	pspoint	ppoint;

	push(zero) ;
	push(zero) ;
	movetoOP() ;
	ppoint = currentpoint() ;
	grestoreOP() ;
	add_element(PA_MOVETO,ppoint) ;
}
void
charpath(struct object pstring, struct object bool)
{
	unsigned char *ss;
	int i;
	struct object intchar;
	struct path new;
	struct pspoint width;
	charpathflg = 1;
	new = ncopypath(Graphics.path.first);
	rel_path(Graphics.path.first);
	ss = pstring.value.v_string.chars;
	show_save();
	for(i=0;i<pstring.value.v_string.length; i++, ss++){
		gsaveOP();
		if(current_type == 1)
			ExecCharString(Graphics.font, (int)*ss, 1, bool);
		else {
			currentfontOP();
			intchar = makeint((int)*ss);
			if(merganthal){
			   if(*ss == 040)fprintf(stderr,"code sp %o %d\n", *ss,*ss);
			   else if(*ss == 011)fprintf(stderr,"code ht %o %d\n", *ss,*ss);
			   else if(*ss == 012)fprintf(stderr,"code nl %o %d\n", *ss,*ss);
			   else fprintf(stderr,"code %c %o %d\n",*ss,*ss,*ss);
			}
			push(intchar);
			execpush(dictget(Graphics.font,Buildchar));
			execute();
/*			if(bool.value.v_boolean == TRUE){
				flattenpathOP();
				strokepathOP();
			}needs to look at painttype*/
		}
		appendpath(&new, Graphics.path, Graphics.origin );
		newpathOP();
		width = Graphics.width;
		grestoreOP();
		width = idtransform(width);
		push(makereal(width.x));
		push(makereal(width.y));
		translateOP();
	}
	show_restore();
	Graphics.path = new;
	charpathflg = 0;
/*	if(bool.value.v_boolean == TRUE)
		strokepathOP();*/
/*printpath("charpath", Graphics.path, 1000);*/
}

void
setcharwidthOP(void)
{
	struct object object;
	if ( !Graphics.inshow )
		pserror("undefined font", "setcharwidth") ;
	Graphics.width = texwidth = dtransform(poppoint("setcharwidth"));
}

void
setcachedeviceOP(void)
{
	struct	object	font, matrix, cfont, object;
	struct pspoint width, ur, ll, nur, nll, w;
	struct cachefont *cache;
	int i;
	static ct=0;

	if ( !Graphics.inshow )
		pserror("undefined font", "setcachedevice") ;
	object = real("setcachedevice");	/*because of dumb 3b compiler*/
	ur.y = object.value.v_real;
	object = real("setcachedevice");
	ur.x = object.value.v_real;
	object = real("setcachedevice");
	ll.y = object.value.v_real;
	object = real("setcachedevice");
	ll.x = object.value.v_real;
	width = poppoint("setcachedevice");
	Graphics.width = texwidth = dtransform(width);
	if((merganthal) && current_type == 3){
		w = _transform(width);
		fprintf(stderr,"width %d\n",(int)w.x);
	}
	nur = dtransform(ur);
	nll = dtransform(ll);
	if(cacheout)fprintf(stderr," %d %d %d %d",(int)ur.x,(int)ur.y,(int)ll.x,(int)ll.y);
	if((i=getbytes(nur.x,nur.y,nll.x,nll.y)) <= blimit){
		if(findfno() < 0){
			putfcache(dontcache,ur.y,ur.x,ll.y,ll.x);
		}
		cache = currentcache;
		if(cache == (struct cachefont *)0||dontcache){
			/*if(ct++ < 5)fprintf(stderr,"too many chars in cache %d\n",cache->charno);*/
			cacheit = -1;
			return;
		}
		else if(cache->charno>=CACHE_CLIMIT-2){
			/*if(ct++ < 5)fprintf(stderr,"too many chars in cache %d\n",cache->charno);*/
			cacheit = -1;
		}
		else if(cache->internal)cacheit = 1;
		else cacheit = 2;
		cache->cachec[cache->charno].gwidth = Graphics.width.x;
		cache->cachec[cache->charno].gheight = Graphics.width.y;
	}
	else {
		if(mdebug && !korean)fprintf(stderr,"char too big for cache %d\n",i);
		cacheit = -1;
		fh = fw = 0;
		forigin.x = forigin.y = 0;
	}
	savex = G[4];	/*in case BuildChar changes it*/
	savey = G[5];
}

void
stringwidthOP(void)
{
	struct object junk1,junk2;
	struct pspoint ppoint;
	gsaveOP() ;
	Graphics.device.width = 0 ;
	Graphics.device.height = 0 ;
	newpathOP() ;
	push(zero) ;
	push(zero) ;
	movetoOP() ;
	showOP() ;
	currentpointOP() ;
	grestoreOP() ;
}
