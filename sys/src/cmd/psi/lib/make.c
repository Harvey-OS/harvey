#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "dict.h"
#include "njerq.h"
#include "cache.h"

struct	object
makestring(int length)
{
	struct	object	object ;

	object.type = OB_STRING ;
	object.xattr = XA_LITERAL ;
	if ( length < 0 )
		pserror("rangecheck", "makestring size<0") ;
	if ( length > STRING_LIMIT ){
		fprintf(stderr,"length=%d\n",length);
		pserror("rangecheck", "makestring too long") ;
	}
	object.value.v_string.access = AC_UNLIMITED ;
	object.value.v_string.length = length ;
	object.value.v_string.chars = (unsigned char *)vm_alloc(length) ;
	return(object) ;
}

struct	object
makereal(double real)
{
	struct	object	object ;

	object.type = OB_REAL ;
	object.xattr = XA_LITERAL ;
	object.value.v_real = real ;
	return(object) ;
}

struct	object
makeint(int integer)
{
	struct	object	object ;

	object.type = OB_INT ;
	object.xattr = XA_LITERAL ;
	object.value.v_int = integer ;
	return(object) ;
}

struct	object
makepointer(char *p)
{
	struct	object	object ;

	object.type = OB_NONE ;
	object.xattr = XA_LITERAL ;
	object.value.v_pointer = p ;
	return(object) ;
}

struct	object
makesave(int level, struct namelist *namelist, char *cur_mem)
{
	struct	object	object ;

	object.type = OB_SAVE ;
	object.xattr = XA_LITERAL ;
	object.value.v_save = (struct save *)vm_alloc(sizeof(*object.value.v_save)) ;
	object.value.v_save->level = level ;
	object.value.v_save->namelist = namelist ;
	object.value.v_save->cur_mem = cur_mem ;
	return(object) ;
}

struct	object
makebool(enum bool bool)
{
	struct	object	object ;

	object.type = OB_BOOLEAN ;
	object.xattr = XA_LITERAL ;
	object.value.v_boolean = bool ;
	return(object) ;
}

struct	object
makenull(void)
{
	struct	object	object ;

	object.type = OB_NULL ;
	object.xattr = XA_LITERAL ;
	return(object) ;
}


struct	object
makenone(void)
{
	struct	object	object ;

	object.type = OB_NONE ;
	object.xattr = XA_LITERAL ;
	return(object) ;
}

struct	object
makearray(int length, enum xattr xattr)
{
	struct	object	array ;

	array.type = OB_ARRAY ;
	array.xattr = xattr ;
	if ( length < 0 )
		pserror("rangecheck", "makearray <0") ;
	if ( length > ARRAY_LIMIT )
		pserror("rangecheck", "makearray too big") ;
	array.value.v_array.access = AC_UNLIMITED ;
	array.value.v_array.length = length ;
	array.value.v_array.object = (struct object *)vm_alloc(length*sizeof(*array.value.v_array.object)) ;
	return(array) ;
}

struct	object
makedict(int max)
{
	struct	object	dict ;

	dict.type = OB_DICTIONARY ;
	dict.xattr = XA_LITERAL ;
	if ( max < 0 )
		pserror("rangecheck", "makedict <0") ;
	if ( max > DICTIONARY_LIMIT )
		pserror("rangecheck", "makedict too big") ;
	dict.value.v_dict = (struct dict *)vm_alloc(sizeof(*dict.value.v_dict)) ;
	dict.value.v_dict->access = AC_UNLIMITED ;
	dict.value.v_dict->cur = 0 ;
	dict.value.v_dict->max = max ;
	dict.value.v_dict->dictent = (struct dictent *)vm_alloc(max*sizeof(*dict.value.v_dict->dictent)) ;
	return(dict) ;
}

struct	object
makefile(FILE *fp, enum access access, enum xattr xattr)
{
	struct	object	object ;

	object.type = OB_FILE ;
	object.xattr = xattr ;
	object.value.v_file.access = access ;
	object.value.v_file.fp = fp ;
	return(object) ;
}

struct	object
makeoperator(void (*op)(void))
{
	struct	object	object ;

	object.type = OB_OPERATOR ;
	object.xattr = XA_EXECUTABLE ;
	object.value.v_operator = op ;
	return(object) ;
}

struct	object
makefid(void)
{
	struct	object	object ;

	object.type = OB_FONTID ;
	object.xattr = XA_LITERAL ;
	object.value.v_fontid.fontno = Fonts++;
	return(object) ;
}
