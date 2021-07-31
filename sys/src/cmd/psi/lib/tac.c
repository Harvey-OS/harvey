#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "dict.h"
# include "operator.h"


void
typeOP(void)
{
	char		*typename ;
	struct	object	object, junk ;
	int stype;
	
	junk=pop();
	switch((stype=(junk.type))){
	case OB_ARRAY :
		typename = "arraytype" ;
		break ;

	case OB_BOOLEAN :
		typename = "booleantype" ;
		break ;

	case OB_DICTIONARY :
		typename = "dicttype" ;
		break ;

	case OB_FILE :
		typename = "filetype" ;
		break ;

	case OB_FONTID :
		typename = "fontidtype" ;
		break ;

	case OB_INT :
		typename = "integertype" ;
		break ;

	case OB_MARK :
		typename = "marktype" ;
		break ;

	case OB_NAME :
		typename = "nametype" ;
		break ;

	case OB_NULL :
		typename = "nulltype" ;
		break ;

	case OB_OPERATOR :
		typename = "operatortype" ;
		break ;

	case OB_REAL :
		typename = "realtype" ;
		break ;

	case OB_SAVE :
		typename = "savetype" ;
		break ;

	case OB_STRING :
		typename = "stringtype" ;
		break ;

	default :
		fprintf(stderr,"type %d\n",stype);
		error("bad switch in type") ;
	}
	object = cname(typename) ;
	object.xattr = XA_EXECUTABLE ;
	push(object) ;	
}

void
cvlitOP(void)
{
	struct	object	object ;

	object = pop() ;
	object.xattr = XA_LITERAL ;
	push(object) ;
}

void
cvxOP(void)
{
	struct	object	object ;

	object = pop() ;
	object.xattr = XA_EXECUTABLE ;
	push(object) ;
}

void
cviOP(void)
{
	struct	object	object, junk ;

	object = pop() ;
	if ( object.type == OB_STRING ){
		push(object) ;
		tokenOP() ;
		junk=pop();
		if ( junk.value.v_boolean == FALSE )
			pserror("typecheck", "cvi") ;
		object = pop() ;
		pop() ;
	}
	switch(object.type){
	case OB_INT :
		break ;

	case OB_REAL :
		if ( object.value.v_real < MAXNEG  ||  object.value.v_real > INTEGER )
			pserror("rangecheck", "cvi") ;
		if ( object.value.v_real < 0.0 )
			object.value.v_int = (int)ceil(object.value.v_real) ;
		else
			object.value.v_int = (int)floor(object.value.v_real) ;
		object.type = OB_INT ;
		break ;

	default :
		pserror("typecheck", "cvi") ;
	}
	push(object) ;
}

void
cvrOP(void)
{
	struct	object	object, junk ;

	object = pop() ;
	if ( object.type == OB_STRING ){
		push(object) ;
		tokenOP() ;
		junk=pop();
		if ( junk.value.v_boolean == FALSE )
			pserror("typecheck", "cvr") ;
		object = pop() ;
		pop() ;
	}
	switch(object.type){
	case OB_INT :
		object.type = OB_REAL ;
		object.value.v_real = (double)object.value.v_int ;
		break ;

	case OB_REAL :
		break ;

	default :
		pserror("typecheck", "cvr") ;
	}
	push(object) ;
}

void
cvrsOP(void)
{
	int		n, radix ;
	unsigned char	*p, *q ;
	struct	object	object, junk, string ;

	string = pop() ;
	junk=integer("cvrs");
	radix = junk.value.v_int ;
	object = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "cvrs") ;
	if ( radix < 2  ||  radix > 36 )
		pserror("rangecheck", "cvrs radix") ;

	switch(object.type){
	case OB_REAL :
		push(object) ;
		cviOP() ;
		object = pop() ;
	case OB_INT :
		n = object.value.v_int ;
		p = string.value.v_string.chars ;
		q = p + string.value.v_string.length ;
		if ( radix == 10  &&  n < 0 ){
			if ( p == q )
				pserror("rangecheck", "cvrs") ;
			*p++ = '-' ;
			n = -n ;
		}
		p = radconv(n,radix,p,q) ;
		break ;

	default :
		pserror("typecheck", "cvrs") ;
	}
	string.value.v_string.length = p - string.value.v_string.chars ;
	push(string) ;
}

unsigned char *
radconv(unsigned int n, int radix, unsigned char *p, unsigned char *q)
{
	if ( n >= radix )
		p = radconv(n/radix,radix,p,q) ;
	if ( p == q )
		pserror("rangecheck", "radconv") ;
	*p++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[n%radix] ;
	return(p) ;
}

void
cvnOP(void)
{
	struct	object	string ;

	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "cvn") ;
	push(makename(string.value.v_string.chars,string.value.v_string.length,string.xattr)) ;
}

void
cvsOP(void)
{
	int		length ;
	unsigned char	*p ;
	struct	object	string ;

	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "cvs") ;
	p = cvs(pop(),&length) ;
	if ( length > string.value.v_string.length ){
		fprintf(stderr,"val %s\n",p);
		fprintf(stderr,"length %d string length %d ",length,string.value.v_string.length);
		pserror("rangecheck", "cvs") ;
	}
	string.value.v_string.length = length ;
	saveitem(string.value.v_string.chars,length) ;
	memmove(string.value.v_string.chars,p,length) ;
	push(string) ;
}

unsigned char *
cvs(struct object object, int *length)
{
	int			i ;
	unsigned char		*p ;
	static unsigned char	buf[100] ;
	
	switch(object.type){
	case OB_BOOLEAN :
		if ( object.value.v_boolean == TRUE )
			p = (unsigned char *)"true" ;
		else p = (unsigned char *)"false" ;
		*length = strlen((char *)p) ;
		break ;

	case OB_INT :
		p = buf ;
		sprintf((char *)p,"%d",object.value.v_int) ;
		*length = strlen((char *)p);
		break ;

	case OB_NAME :
	case OB_STRING :
		*length = object.value.v_string.length ;
		p = object.value.v_string.chars ;
		break ;

	case OB_OPERATOR :
		for ( i=0 ; i<Op_nel ; i++ )
			if ( object.value.v_operator == (void (*)(void))Op_tab[i].v_op )
				break ;
		if ( i == Op_nel )
			error("missing operator") ;
		p = buf ;
		sprintf((char *)p,"--%s--",Nop_tab[i].chars) ;
		*length = strlen((char *)p);
		break ;
	
	case OB_REAL :
		p = buf ;
		sprintf((char *)p,"%lf",object.value.v_real) ;	/*was %.1lf*/
		*length = strlen((char *)p);
		break ;

	case OB_ARRAY :
	case OB_DICTIONARY :
	case OB_FILE :
	case OB_FONTID :
	case OB_MARK :
	case OB_NULL :
	case OB_SAVE :
		p = (unsigned char *)"--nostringval--" ;
		*length = strlen((char *)p) ;
		break ;

	default :
		error("bad case in cvs") ;
	}
	return(p) ;
}

void
xcheckOP(void)
{
	struct object junk;

	junk=pop();
	if ( junk.xattr == XA_EXECUTABLE )
		push(true) ;
	else push(false) ;
}

void
rcheckOP(void)
{
	enum	access	access ;
	struct	object	object ;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		access = object.value.v_array.access ;
		break ;

	case OB_DICTIONARY :
		access = object.value.v_dict->access ;
		break ;

	case OB_FILE :
		access = object.value.v_file.access ;
		break ;

	case OB_STRING :
		access = object.value.v_string.access ;
		break ;

	default :
		pserror("typecheck", "rcheck") ;
	}
	if ( access == AC_UNLIMITED  ||  access == AC_READONLY )
		push(true) ;
	else push(false) ;
}

void
wcheckOP(void)
{
	enum	access	access ;
	struct	object	object ;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		access = object.value.v_array.access ;
		break ;

	case OB_DICTIONARY :
		access = object.value.v_dict->access ;
		break ;

	case OB_FILE :
		access = object.value.v_file.access ;
		break ;

	case OB_STRING :
		access = object.value.v_string.access ;
		break ;

	default :
		pserror("typecheck", "wcheck") ;
	}
	if ( access == AC_UNLIMITED )
		push(true) ;
	else push(false) ;
}

void
setaccess(enum access access)
{
	enum	access	curaccess ;
	struct	object	object ;

	object = pop() ;

	switch(object.type){
	case OB_ARRAY :
		curaccess = object.value.v_array.access ;
		break ;

	case OB_DICTIONARY :
		curaccess = object.value.v_dict->access ;
		break ;

	case OB_FILE :
		curaccess = object.value.v_file.access ;
		break ;

	case OB_STRING :
		curaccess = object.value.v_string.access ;
		break ;

	default :
		pserror("typecheck", "setaccess") ;
	}
	if ( (int)access > (int)curaccess )
		pserror("invalidaccess", "setaccess") ;
	switch(object.type){
	case OB_ARRAY :
		object.value.v_array.access = access ;
		break ;

	case OB_DICTIONARY :
		if ( access == AC_EXECUTEONLY )
			pserror("invalidaccess", "setaccess") ;
		SAVEITEM(object.value.v_dict->access) ;
		object.value.v_dict->access = access ;
		break ;

	case OB_FILE :
		object.value.v_file.access = access ;
		break ;

	case OB_STRING :
		object.value.v_string.access = access ;
		break ;
	}
	push(object) ;
}

void
noaccessOP(void)
{
	setaccess(AC_NONE) ;
}

void
readonlyOP(void)
{
	setaccess(AC_READONLY) ;
}

void
executeonlyOP(void)
{
	setaccess(AC_EXECUTEONLY) ;
}
