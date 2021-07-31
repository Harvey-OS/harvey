#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "dict.h"
#include "path.h"
#ifndef Plan9
#include "njerq.h"
#endif
#include "graphics.h"

void
lengthOP(void)
{
	int		length ;
	struct	object	object ;

	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		length = object.value.v_array.length ;
		break ;

	case OB_DICTIONARY :
		length = object.value.v_dict->cur ;
		break ;
	case OB_NAME:
	case OB_STRING :
		length = object.value.v_string.length ;
		break ;

	default :
		fprintf(stderr,"type %o\n",object.type);
		pserror("typecheck", "length") ;
	}
	push(makeint(length)) ;
}

void
arrayOP(void)
{
	int		i, length ;
	struct	object	array, null, object ;

	object = integer("array");	/*to avoid compiler bug*/
	length = object.value.v_int ;
	array = makearray(length,XA_LITERAL) ;
	null = makenull() ;
	for ( i=0 ; i<length ; i++ )
		array.value.v_array.object[i] = null ;
	push(array) ;
}

void
endarrayOP(void)
{
	int		i ;
	struct	object	object , mark;

	counttomarkOP() ;
	arrayOP() ;
	object = pop() ;
	for ( i=object.value.v_array.length-1 ; i>=0 ; i-- )
		object.value.v_array.object[i] = pop() ;
	mark=pop();
	if ( mark.type != OB_MARK )
		error("expecting mark in endarray") ;
	push(object) ;
}

void
putOP(void)
{
	struct	object	source, index, dest ;

	source = pop() ;
	index = pop() ;
	dest = pop() ;
	switch(dest.type){
	case OB_ARRAY :
		if ( index.type != OB_INT )
			pserror("typecheck", "put array") ;
		if ( index.value.v_int < 0  ||
			index.value.v_int >= dest.value.v_array.length ){
			fprintf(stderr,"val %d dest leng %d\n",index.value.v_int,
			   dest.value.v_array.length);
			pserror("rangecheck", "put array") ;
		}
		SAVEITEM(dest.value.v_array.object[index.value.v_int]) ;
		dest.value.v_array.object[index.value.v_int] = source ;
		return;

	case OB_STRING :
		if ( source.type != OB_INT )
			pserror("typecheck", "put string") ;
		if ( source.value.v_int < 0  ||  source.value.v_int > 255 ){
			fprintf(stderr,"source %d\n",source.value.v_int);
			pserror("rangecheck", "put string") ;
		}
		if ( index.type != OB_INT )
			pserror("typecheck", "put string - index type") ;
		if ( index.value.v_int < 0  ||
			index.value.v_int >= dest.value.v_string.length ){
			fprintf(stderr,"val %d dest leng %d too small\n",index.value.v_int,
			   dest.value.v_array.length);
			pserror("rangecheck", "put string") ;
		}
		SAVEITEM(dest.value.v_string.chars[index.value.v_int]) ;
		dest.value.v_string.chars[index.value.v_int] = source.value.v_int ;
		return;

	case OB_DICTIONARY :
		if(decryptf||Graphics.incharpath)dictput(dest.value.v_dict,index,source) ;
		else odictput(dest.value.v_dict,index,source) ;
		return;

	default :
		pserror("typecheck", "put") ;
	}
}

void
getOP(void)
{
	struct	object	target, locator, object ;
	int index;

	locator = pop() ;
	target = pop() ;
	switch(target.type){
	case OB_ARRAY :
		if ( locator.type != OB_INT )
			pserror("typecheck", "get array") ;
		index = locator.value.v_int ;
		if ( index < 0  ||  index >= target.value.v_array.length ){
			fprintf(stderr,"bad index %d leng %d\n",index,
				target.value.v_array.length);
			pserror("rangecheck", "get array") ;
		}
		push(target.value.v_array.object[index]) ;
		return;

	case OB_STRING :
		if ( locator.type != OB_INT )
			pserror("typecheck", "get string") ;
		index = locator.value.v_int ;
		if ( index < 0  ||  index >= target.value.v_string.length ){
			fprintf(stderr,"bad index %d leng %d\n",index,
				target.value.v_string.length);
			pserror("rangecheck", "get string") ;
		}
		push(makeint((int)target.value.v_string.chars[index])) ;
		return;

	case OB_DICTIONARY :
		if(decryptf||Graphics.incharpath)
			object = dictget(target.value.v_dict,locator) ;
		else object = odictget(target.value.v_dict,locator) ;
		if ( object.type == OB_NONE ){
			if(locator.type == OB_INT)
				fprintf(stderr,"dict loctype %d val %d\n",locator.type,
					locator.value.v_int);
			else fprintf(stderr,"dict loctype %d val %s\n",locator.type,
				locator.value.v_string.chars);
			pserror("undefined", "get dict") ;
		}
		push(object) ;
		return;

	default :
		pserror("typecheck", "get") ;
	}
}

void
getintervalOP(void)
{
	int		i, count, index ;
	struct	object	object, subarray ;

	object=integer("getinterval");
	count = object.value.v_int ;
	object=integer("getinterval");
	index = object.value.v_int ;
	object = pop() ;
	switch(object.type){
	case OB_ARRAY :
		if ( index < 0  ||  index >= object.value.v_array.length ){
			fprintf(stderr,"bad index %d leng %d\n",
				index, object.value.v_array.length);
			pserror("rangecheck", "getinterval array");
		}
		if(count < 0 || index+count > object.value.v_array.length){
			fprintf(stderr,"bad count %d leng %d\n",
				count, object.value.v_array.length);
			pserror("rangecheck", "getinterval array") ;
		}
		subarray = object ;
		subarray.value.v_array.object += index ;
		subarray.value.v_array.length = count ;
		break ;

	case OB_STRING :
		if ( index < 0  ||  index >= object.value.v_string.length ){
			fprintf(stderr,"bad index %d leng %d\n",index,
				object.value.v_array.length);
			pserror("rangecheck", "getinterval string");
		}
		if(count< 0 || index+count > object.value.v_string.length){
			fprintf(stderr,"bad count %d index %d leng %d\n",
				count, index,object.value.v_string.length);
			pserror("rangecheck", "getinterval string");
		}
		subarray = object;
		subarray.value.v_string.chars += index ;
		subarray.value.v_string.length = count ;
		break ;

	default :
		pserror("typecheck", "getinterval") ;
	}
	push(subarray) ;
}

void
putintervalOP(void)
{
	int		index, i ;
	struct	object	source, dest, object ;

	source = pop() ;
	object = integer("putinterval");
	index = object.value.v_int ;
	dest = pop() ;
	if ( source.type != dest.type )
		pserror("typecheck", "putinterval types different") ;

	switch(dest.type){
	case OB_ARRAY :
		if(source.value.v_array.length <= 0)break;
		if ( index < 0  ||  index >= dest.value.v_array.length ){
			fprintf(stderr,"bad index %d arr_leng %d\n",index,
				dest.value.v_array.length);
			pserror("rangecheck", "putinterval array");
		}
		if ( index+source.value.v_array.length > dest.value.v_array.length){
			fprintf(stderr,"dest too small %d %d\n",
				index+source.value.v_array.length,
				dest.value.v_array.length);
			pserror("rangecheck", "putinterval array");
		}
		for ( i=0 ; i<source.value.v_array.length ; i++ ){
			SAVEITEM(dest.value.v_array.object[index+i]) ;
			dest.value.v_array.object[index+i] = source.value.v_array.object[i] ;
		}
		return;

	case OB_STRING :
		if ( index < 0  ||  index >= dest.value.v_string.length ){
			fprintf(stderr,"bad index %d st_leng %d\n", index,
				dest.value.v_string.length);
			pserror("rangecheck", "putinterval string") ;
		}
		if ( index+source.value.v_string.length > dest.value.v_string.length ){
			fprintf(stderr,"dest too small %d %d\n",
				index+source.value.v_string.length,
				dest.value.v_string.length);
			pserror("rangecheck", "putinterval string") ;
		}
		for ( i=0 ; i<source.value.v_string.length ; i++ ){
			SAVEITEM(dest.value.v_string.chars[index+i]) ;
			dest.value.v_string.chars[index+i] = source.value.v_string.chars[i] ;
		}
		return;

	default :
		pserror("typecheck", "putinterval") ;
	}
}

void
aloadOP(void)
{
	int		i ;
	struct	object	array ;

	array = pop() ;
	if ( array.type != OB_ARRAY )
		pserror("typecheck", "aload not array") ;
	for ( i=0 ; i<array.value.v_array.length ; i++ )
		push(array.value.v_array.object[i]) ;
	push(array) ;
}

void
astoreOP(void)
{
	int		i ;
	struct	object	array ;

	array = pop() ;
	if ( array.type != OB_ARRAY )
		pserror("typecheck", "astore not array") ;
	for ( i=0 ; i<array.value.v_array.length ; i++ ){
		SAVEITEM(array.value.v_array.object[array.value.v_array.length-i-1]) ;
		array.value.v_array.object[array.value.v_array.length-i-1] = pop();
	}
	push(array) ;
}
