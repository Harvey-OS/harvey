#include <u.h>
#include <libc.h>
# include <ctype.h>
#include "system.h"
# include "stdio.h"
# include "object.h"


struct	object
procedure(void)
{
	struct	object	proc ;

	proc = pop() ;
	if ( proc.xattr == XA_EXECUTABLE  &&  proc.type == OB_ARRAY )
		return(proc) ;
	fprintf(stderr,"type %d attr %d %s ",proc.type,proc.xattr,proc.value.v_string.chars);
	if(proc.type == OB_STRING)
		fprintf(stderr,"named:%o\n",proc.value.v_string.chars[0]);
	pserror("typecheck", "procedure") ;
}


int
hexcvt(int c)
{
	if ( isdigit(c) )
		return(c-'0') ;
	if ( isupper(c) )
		return(c-'A'+10) ;
	return(c-'a'+10) ;
}
