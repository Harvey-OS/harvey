#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "object.h"

void
stringOP(void)
{
	int		length, i ;
	struct	object	string, object ;

	object = integer("string");
	length = object.value.v_int ;
	string = makestring(length) ;
	for ( i=0 ; i<length ; i++ )
		string.value.v_string.chars[i] = 0 ;
	push(string) ;	
}

void
anchorsearchOP(void)
{
	struct	object	seek, string, post, match ;

	seek = pop() ;
	if ( seek.type != OB_STRING )
		pserror("typecheck", "anchorsearch") ;
	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "anchorsearch") ;
	if(string.value.v_string.length >= seek.value.v_string.length  &&
	    memcmp(string.value.v_string.chars,seek.value.v_string.chars,
	    seek.value.v_string.length) == 0){
		post = string ;
		post.value.v_string.chars += seek.value.v_string.length ;
		post.value.v_string.length -= seek.value.v_string.length ;
		push(post) ;
		match = string ;
		match.value.v_string.length = seek.value.v_string.length ;
		push(match) ;
		push(true) ;
	}
	else{
		push(string) ;
		push(false) ;
	}
}

void
searchOP(void)
{
	int		i ;
	struct	object	seek, string, post, pre, match ;

	seek = pop() ;
	if ( seek.type != OB_STRING )
		pserror("typecheck", "search") ;
	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "search") ;

	for ( i=0 ; string.value.v_string.length-i>=seek.value.v_string.length ; i++ )
		if (memcmp(string.value.v_string.chars+i,seek.value.v_string.chars,
		    seek.value.v_string.length) == 0 ){
			post = string ;
			post.value.v_string.chars += i + seek.value.v_string.length ;
			post.value.v_string.length -= i + seek.value.v_string.length ;
			push(post) ;
			match = string ;
			match.value.v_string.chars += i ;
			match.value.v_string.length = seek.value.v_string.length ;
			push(match) ;
			pre = string ;
			pre.value.v_string.length = i ;
			push(pre) ;
			push(true) ;
			return ;
		}
	push(string) ;
	push(false) ;
}
