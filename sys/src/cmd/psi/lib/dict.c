#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "dict.h"
# include "operator.h"
# include "error.h"
#include "path.h"


static	struct	object	stack[DICT_STACK_LIMIT] ;
static	int		stacktop ;

void
dict_invalid(char *mem_bound)
{
	int	i ;
	for ( i=0 ; i<stacktop ; i++ )
		invalid(stack[i],mem_bound) ;
}
struct	object			/*internal called only with name or string*/
dictget(struct dict *dict, struct object key)
{
	register int i;
	register struct object object;
	register unsigned char *kptr=key.value.v_string.chars;
	if(key.type == OB_STRING){
		object = makename(key.value.v_string.chars,
			key.value.v_string.length, XA_LITERAL);
		kptr = object.value.v_string.chars;
	}
	for ( i=0 ; i<dict->cur ; i++ ){	/*changed for speed with operators*/
		if(dict->dictent[i].key.value.v_string.chars == kptr)
				return(dict->dictent[i].value);
	}
	return(none) ;
}

struct object
odictget(struct dict *dict, struct object key)
{
	register int i;

	for(i=0;i<dict->cur; i++)
		if(equal(dict->dictent[i].key,key) == TRUE)
			return(dict->dictent[i].value);
	return(none);
}

struct	object
dictstackget(struct object key, char *errorname)
{
	struct	object	object ;
	int		i ;
	for ( i=stacktop-1 ; i>=0 ; i-- ){
		object = dictget(stack[i].value.v_dict,key) ;
		if ( object.type != OB_NONE )
			return(object);		/*was break*/
	}
	if ( object.type == OB_NONE && errorname != NULL ){
		fprintf(stderr,"key %d:leng %d %s\n",key.type,key.value.v_string.length,
			key.value.v_string.chars);
		pserror(errorname, "dictstackget") ;
	}
	return(object) ;
}

void
dictstackinit(void)
{
	int		i ;
	char c;
	struct	object	systemdict , statusd, dummy, name, product, temp;
	struct	object	userdict, errordict, symbolencoding;
	struct dict *charstrings,*scharstrings;

	true = makebool(TRUE);
	false = makebool(FALSE);
	zero = makereal(0.);
	null = makenull();
	systemdict = makedict(9+Op_nel+1) ;
	Systemdict = systemdict.value.v_dict ;
	sysdictput(cname("systemdict"),&systemdict) ;
	sysdictput(cname("true"),&true) ;
	sysdictput(cname("false"),&false) ;
	sysdictput(cname("null"),&null) ;
	if(Op_nel != nNop_nel)
		pserror("operator tables different size", "dictstackinit");
	for ( i=0 ; i<Op_nel ; i++ )
		sysdictput(pname(Nop_tab[i],XA_EXECUTABLE),&Op_tab[i]) ;
	lbrak=cname("[");	/*must be after loading system dict*/
	rbrak=cname("]");
	lbrace = cname("{");
	rbrace = cname("}");

	statusd = makedict(1+Sop_nel);
	statusdict = statusd.value.v_dict;
	sysdictput(cname("statusdict"),&statusd) ;
	loaddict(statusdict, cname("manualfeed"),&false);
	product.type=OB_STRING;
	product.xattr=AC_NONE;
	product.value.v_string = sproduct;
	loaddict(statusdict, pname(NSop_tab[0],XA_EXECUTABLE), &product);
	for(i=1;i<Sop_nel; i++)
		loaddict(statusdict, pname(NSop_tab[i],XA_EXECUTABLE),&Sop_tab[i]);

	userdict = makedict(USER_DICT_LIMIT) ;
	sysdictput(cname("userdict"),&userdict) ;

	FontMatrix = cname("FontMatrix");
	FID = cname("FID");
	ImageMatrix = cname("ImageMatrix");
	FontBBox = cname("FontBBox");
	FontDirectory = cname("FontDirectory");
	FontType = cname("FontType");
	Encoding = cname("Encoding");
	Buildchar = cname("BuildChar");
	BHeight = cname("Bheight");
	Ascent = cname("Ascent");
	Chardata = cname("CharData");
	CharStrings = cname("CharStrings");
	FontInfo = cname("FontInfo");
	unknown = cname("Times-Roman");
	dummy = makeint(0);
	none = makenone();
	nullstr = cname("");
	notdef = makename((unsigned char *)".notdef",7, XA_LITERAL);
	zeropt = makepoint(0.,0.);

	errordict = makedict(NEL(error_tab)) ;
	sysdictput(cname("errordict"),&errordict) ;
	for ( i=0 ; i<NEL(error_tab) ; i++ ){
		temp = makeoperator(errorhandleOP);
		loaddict(errordict.value.v_dict,cname(error_tab[i]),
			&temp) ;
	}
	temp = makedict(FONT_DICT_LIMIT);
	sysdictput(FontDirectory,&temp) ;
	Charstrings = makedict(256);
	charstrings = Charstrings.value.v_dict;
	standardencoding = makearray(256,XA_LITERAL) ;
	sysdictput(cname("StandardEncoding"),&standardencoding) ;
	for ( i=0 ; i<256 ; i++ ){
		if(se_tab[i].length != 0){
			name = pname(se_tab[i],XA_LITERAL);
			loaddict(charstrings, name, &dummy);
		}
		else name = notdef;
		standardencoding.value.v_array.object[i] = name ;
		if(se_tab[i].length == 1){
			c = *se_tab[i].chars;
			if(c<='Z'){
				lalpha[c-'A']=name;
				xalpha[c-'A']=pckname(se_tab[i],XA_EXECUTABLE);
			}
			else {
				lalpha[c-'a'+26] = name;
				xalpha[c-'a'+26]=pckname(se_tab[i],XA_EXECUTABLE);
			}
		}
	}
	SCharstrings = makedict(256);
	scharstrings = SCharstrings.value.v_dict;
	symbolencoding = makearray(256,XA_LITERAL) ;
	sysdictput(cname("SymbolEncoding"),&symbolencoding) ;
	for ( i=0 ; i<256 ; i++ ){
		if(sye_tab[i].length != 0){
			name = pckname(sye_tab[i],XA_LITERAL);
			loaddict(scharstrings, name, &dummy);
		}
		else name = notdef;
		symbolencoding.value.v_array.object[i] = name ;
	}
	Fontinfo = makedict(2);
	temp = makeint(1000);
	loaddict(Fontinfo.value.v_dict,cname("UnderlinePosition"),&temp);
	temp = makeint(100);
	loaddict(Fontinfo.value.v_dict,cname("UnderlineThickness"),&temp);
	if(!nofontinit)initfont();
	stack[0] = systemdict ;
	stack[1] = userdict ;
	stacktop = 2 ;
}
void
dictput(struct dict *dict, struct object key, struct object object)
{
	int		i ;
	for ( i=0 ; i<dict->cur ; i++ ){
		if(dict->dictent[i].key.type == key.type &&
		 dict->dictent[i].key.value.v_string.chars == key.value.v_string.chars)
			break ;
	}
	if ( i == dict->cur ){
		if ( dict->cur >= dict->max ){
			fprintf(stderr,"dict max %d %d adding %s ",dict->max,
				key.type,key.value.v_string.chars);
			pserror("dictfull", "dictput") ;
		}
		SAVEITEM(dict->cur) ;
		dict->cur++ ;
		dict->dictent[i].key = key ;
	}
	else
		SAVEITEM(dict->dictent[i].value) ;
	dict->dictent[i].value = object ;	
}
void
sysdictput(struct object key, struct object *object)
{
	int i;
	SAVEITEM(Systemdict->cur) ;
	i=Systemdict->cur++;
	Systemdict->dictent[i].key = key ;
	Systemdict->dictent[i].value = *object ;
}	
void
loaddict(struct dict *dict, struct object key, struct object *object)
{
	int i;
	SAVEITEM(dict->cur) ;
	i=dict->cur++;
	dict->dictent[i].key = key ;
	dict->dictent[i].value = *object ;
}
void
odictput(struct dict *dict, struct object key, struct object object)
{
	int		i ;
	for ( i=0 ; i<dict->cur ; i++ ){
		if(equal(dict->dictent[i].key, key) == TRUE){
			break ;
		}
	}
	if ( i == dict->cur ){
		if ( dict->cur >= dict->max ){
			fprintf(stderr,"dict max %d %d",dict->max,key.type);
			pserror("dictfull", "odictput") ;
		}
		SAVEITEM(dict->cur) ;
		dict->cur++ ;
		dict->dictent[i].key = key ;
	}
	else
		SAVEITEM(dict->dictent[i].value) ;
	dict->dictent[i].value = object ;	
}

void
dictOP(void)
{
	struct object object;
	object = integer("dict");
	push(makedict(object.value.v_int)) ;
}

void
beginOP(void)
{
	struct	object	object ;

	object = pop() ;
	if ( object.type != OB_DICTIONARY )
		pserror("typecheck", "begin") ;
	if ( stacktop == DICT_STACK_LIMIT )
		pserror("dictstackoverflow", "begin") ;
	stack[stacktop++] = object ;
}
void
begin(struct object object)
{
	if ( stacktop == DICT_STACK_LIMIT ) 
		pserror("dictstackoverflow", "begin") ;
	stack[stacktop++] = object ;
}

void
endOP(void)
{
	if ( stacktop <= 2 )
		pserror("stackunderflow", "end") ;
	--stacktop ;
}

void
maxlengthOP(void)
{
	struct	object	object ;

	object = pop() ;
	if ( object.type != OB_DICTIONARY )
		pserror("typecheck", "maxlength") ;
	push(makeint(object.value.v_dict->max)) ;
}

void
currentdictOP(void)
{

	push(stack[stacktop-1]) ;
}

void
defOP(void)
{
	struct	object	value, key ;

	value = pop() ;
	key = pop() ;
	if(key.type == OB_NAME)dictput(stack[stacktop-1].value.v_dict,key,value) ;
	else odictput(stack[stacktop-1].value.v_dict,key,value) ;
}

void
loadOP(void)
{
	push(dictstackget(pop(),"undefined:load")) ;
}

void
knownOP(void)
{
	struct	object	key, dict, item ;

	key = pop() ;
	dict = pop() ;
	if ( dict.type != OB_DICTIONARY )
		pserror("typecheck", "known") ;
	item = dictget(dict.value.v_dict,key);
	if ( item.type == OB_NONE )
		push(false) ;
	else
		push(true) ;
}

void
whereOP(void)
{
	int		i ;
	struct	object	key, item ;

	key = pop() ;
	for ( i=stacktop-1 ; i>=0 ; i-- ){
		item = dictget(stack[i].value.v_dict,key);
		if ( item.type != OB_NONE )
			break ;
	}
	if ( i >= 0 ){
		push(stack[i]) ;
		push(true) ;
	}
	else
		push(false) ;
}

void
storeOP(void)
{
	int		i ;
	struct	object	key, value, item ;

	value = pop() ;
	key = pop() ;
	for ( i=stacktop-1 ; i>=0 ; i-- ){
		item = dictget(stack[i].value.v_dict,key);
		if ( item.type != OB_NONE )
			break ;
	}
	if ( i >= 0 )
		dictput(stack[i].value.v_dict,key,value) ;
	else
		dictput(stack[stacktop-1].value.v_dict,key,value) ;
}

void
countdictstackOP(void)
{

	push(makeint(stacktop)) ;
}

void
dictstackOP(void)
{
	int		i ;
	struct	object	array ;

	array = pop() ;
	if ( array.type != OB_ARRAY )
		pserror("typecheck", "dictstack") ;
	if ( stacktop > array.value.v_array.length )
		pserror("rangecheck", "dictstack") ;
	for ( i=0 ; i<stacktop ; i++ ){
		SAVEITEM(array.value.v_array.object[i]) ;
		array.value.v_array.object[i] = stack[i] ;
	}
	array.value.v_array.length = stacktop ;
	push(array) ;
}
