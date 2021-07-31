#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "path.h"
#ifndef Plan9
#include "njerq.h"
#endif
# include "graphics.h"
# include "dict.h"

static	struct	object	stack[EXEC_STACK_LIMIT] ;
static	int		stacktop ;

void
execstackinit(void)
{

	stack[0] = makefile(stdin,AC_READONLY,XA_EXECUTABLE) ;
	currentfile = stdin;
	stacktop = 1 ;
}

void
exec_invalid(char *mem_bound)
{
	int	i ;

	for ( i=0 ; i<stacktop ; i++ )
		invalid(stack[i],mem_bound) ;
}

void
execpush(struct object object)
{
	if ( stacktop == EXEC_STACK_LIMIT )
		pserror("execstackoverflow", "execpush") ;
	stack[stacktop++] = object ;

}

void
execute(void)
{
	int		old_stacktop ;
	register struct	object	object ;
	int	offset ;
	FILE	*sp ;

	old_stacktop = stacktop ;
	while ( stacktop >= old_stacktop ){
		if(prolog){
			if(pageflag==1 || skipflag==2)
			if(pageno != page || skipflag){
				skiptopage();
				pageflag=0;
				/*pageflag++;*/
				skipflag = 0;
			}
		}
		object = stack[stacktop-1] ;
		if ( object.xattr == XA_LITERAL ){
			push(object) ;
			stacktop-- ;
		}
		else switch(object.type){
		case OB_ARRAY :
			switch(object.value.v_array.length){
			case 0 :
				stacktop-- ;
				break ;
			case 1 :
				if(object.value.v_array.object[0].type ==
				 OB_ARRAY&&object.value.v_array.object[0].xattr ==
				 XA_EXECUTABLE){
					push(object.value.v_array.object[0]) ;
					stacktop-- ;
				}
				else stack[stacktop-1]=object.value.v_array.object[0];
				break ;
			default :
				stack[stacktop-1].value.v_array.object++ ;
				stack[stacktop-1].value.v_array.length-- ;
				if(stack[stacktop-1].value.v_array.object[-1].type
				   == OB_ARRAY  &&
				   stack[stacktop-1].value.v_array.object[-1].xattr
				   == XA_EXECUTABLE)
				  push(stack[stacktop-1].value.v_array.object[-1]);
				else execpush(stack[stacktop-1].value.v_array.object[-1]) ;
				break ;
			}
			break ;

		case OB_FILE :
			if(restart){
				prolog=1;
				restart=0;
			}
			object = scanner(stack[stacktop-1].value.v_file.fp) ;
			if ( object.type == OB_NONE ){
				fclose(stack[stacktop-1].value.v_file.fp) ;
				stacktop-- ;
			}
			else{
				if ( object.type == OB_ARRAY  &&
				    object.xattr == XA_EXECUTABLE )
					push(object) ;
				else execpush(object) ;
			}
			break ;

		case OB_BOOLEAN :
		case OB_DICTIONARY :
		case OB_FONTID :
		case OB_INT :
		case OB_MARK :
		case OB_REAL :
		case OB_SAVE :
			push(object) ;
			stacktop-- ;
			break ;

		case OB_NAME :
			stack[stacktop-1] = dictstackget(object,"undefined:execute") ;
			break ;

		case OB_OPERATOR :
			stacktop-- ;
			(*object.value.v_operator)() ;
			break ;


		case OB_NULL :
			break ;

		case OB_STRING :
			sp = sopen(stack[stacktop-1].value.v_string) ;
			object = scanner(sp) ;
			if ( object.type == OB_NONE ){
				f_close(sp) ;
				stacktop-- ;
			}
			else {
				offset = stell(sp) ;
				stack[stacktop-1].value.v_string.chars += offset ;
				stack[stacktop-1].value.v_string.length -= offset ;	
				if ( object.type == OB_ARRAY  &&
				    object.xattr == XA_EXECUTABLE )
					push(object) ;
				else execpush(object) ;
			}
			break ;

		default :
		fprintf(stderr,"execute type %d\n",object.type);
			error("bad case in execute") ;
		}
	}
}

void
execOP(void)
{
	struct object object;
	object = pop();
	execpush(object) ;
}

void
arrayforall(void)
{

	if ( stack[stacktop-1].value.v_array.length == 0 )
		stacktop -= 3 ;
	else {
		stack[stacktop-1].value.v_array.object++ ;
		stack[stacktop-1].value.v_array.length-- ;
		push(stack[stacktop-1].value.v_array.object[-1]) ;
		stacktop++ ;
		execpush(stack[stacktop-3]) ;
	}
}

void
stringforall(void)
{

	if ( stack[stacktop-1].value.v_string.length == 0 )
		stacktop -= 3 ;
	else {
		stack[stacktop-1].value.v_string.chars++ ;
		stack[stacktop-1].value.v_string.length-- ;
		push(makeint((int)stack[stacktop-1].value.v_string.chars[-1])) ;
		stacktop++ ;
		execpush(stack[stacktop-3]) ;
	}
}

void
dictforall(void)
{

	if ( stack[stacktop-1].value.v_int == stack[stacktop-2].value.v_dict->cur )
		stacktop -= 4 ;
	else {
	 push(stack[stacktop-2].value.v_dict->dictent[stack[stacktop-1].value.v_int].key) ;
	 push(stack[stacktop-2].value.v_dict->dictent[stack[stacktop-1].value.v_int].value) ;
		stack[stacktop-1].value.v_int++ ;
		stacktop++ ;
		execpush(stack[stacktop-4]) ;
	}
}

void
xfor(void)
{
	enum	bool	up ;
	struct object object;

	push(stack[stacktop-2]) ;
	push(makeint(0)) ;
	gtOP() ;
	object = pop();
	up = object.value.v_boolean ;
	push(stack[stacktop-1]) ;
	push(stack[stacktop-3]) ;
	if ( up == TRUE )
		gtOP() ;
	else
		ltOP() ;
	object = pop();
	if ( object.value.v_boolean == TRUE )
		stacktop -= 5 ;
	else {
		push(stack[stacktop-1]) ;
		push(stack[stacktop-1]) ;
		push(stack[stacktop-2]) ;
		addOP() ;
		stack[stacktop-1] = pop() ;
		stacktop++ ;
		execpush(stack[stacktop-5]) ;
	}
}

void
xrepeat(void)
{

	if ( stack[stacktop-1].value.v_int-- == 0 )
		stacktop -= 3 ;
	else {
		stacktop++ ;
		execpush(stack[stacktop-3]) ;
	}
}

void
xloop(void)
{

	stacktop++ ;
	execpush(stack[stacktop-2]) ;
}

void
execstackOP(void)
{
	int		i ;
	struct	object	array ;

	array = pop() ;
	if ( array.type != OB_ARRAY )
		pserror("typecheck", "execstack") ;
	if ( array.value.v_array.length < stacktop )
		pserror("rangecheck", "execstack") ;
	for ( i=0 ; i<stacktop ; i++ ){
		SAVEITEM(array.value.v_array.object[i]) ;
		array.value.v_array.object[i] = stack[i] ;
	}
	array.value.v_array.length = stacktop ;
	push(array) ;
}

void
exitOP(void)
{
	int		i ;

	for ( i=stacktop-1 ; i>=0 ; i-- ){
		if ( stack[i].type == OB_FILE )
			pserror("invalidexit", "exit") ;
		if ( stack[i].type == OB_OPERATOR )
			if ( (void (*)(void))stack[i].value.v_operator == loopnoop )
				break ;
			else if ( (void (*)(void))stack[i].value.v_operator == xstopped )
				pserror("invalidexit", "exit") ;
	}
	if ( i >= 0 )
		stacktop = i ;
	else {
		fprintf(stderr,"i=%d\n",i);
		warning("no enclosing looping context") ;
		execpush(makeoperator(quitOP)) ;
	}
}

void
loopnoop(void)
{
	error("loopnoop executed") ;
}

void
quitOP(void)
{
	error("quit") ;
}

void
countexecstackOP(void)
{
	push(makeint(stacktop)) ;
}

void
xpathforall(void)
{
	struct	element	*p, *next ;
	struct	object	proc ;

	p = (struct element *)stack[stacktop-1].value.v_pointer ;
	if ( p == NULL )
		stacktop -= 6 ;
	else {	
		switch(p->type){
		case PA_MOVETO :
			proc = stack[stacktop-2] ;
			push(makereal(p->ppoint.x)) ;
			push(makereal(p->ppoint.y)) ;
			itransformOP() ;
			next = p->next ;
			break ;

		case PA_LINETO :
			proc = stack[stacktop-3] ;
			push(makereal(p->ppoint.x)) ;
			push(makereal(p->ppoint.y)) ;
			itransformOP() ;
			next = p->next ;
			break ;

		case PA_CURVETO :
			proc = stack[stacktop-4] ;
			push(makereal(p->ppoint.x)) ;
			push(makereal(p->ppoint.y)) ;
			itransformOP() ;
			push(makereal(p->next->ppoint.x)) ;
			push(makereal(p->next->ppoint.y)) ;
			itransformOP() ;
			push(makereal(p->next->next->ppoint.x)) ;
			push(makereal(p->next->next->ppoint.y)) ;
			itransformOP() ;
			next = p->next->next->next ;
			break ;

		case PA_CLOSEPATH :
			proc = stack[stacktop-5] ;
			next = p->next ;
			break ;

		default :
			error("bad case in xpathforall") ;
		}
		stack[stacktop-1].value.v_pointer = (char *)next ;
		stacktop++ ;
		execpush(proc) ;
	}
}

void
currentfileOP(void)
{
	int		i ;
	for ( i=stacktop-1 ; i>=0 ; i-- )
		if ( stack[i].type == OB_FILE )
			break ;
	if ( i < 0 )
		push(makefile(NULL,AC_NONE,XA_LITERAL)) ;
	else
		push(stack[i]) ;
}

void
stoppedOP(void)
{

	execpush(makeoperator(xstopped)) ;
	execpush(pop()) ;
}

void
xstopped(void)
{

	push(false) ;
}

void
stopOP(void)
{
	int		i ;

	for ( i=stacktop-1 ; i>=0 ; i-- ){
		if ( stack[i].type == OB_OPERATOR  &&
		  (void (*)(void))stack[i].value.v_operator == xstopped )
			break ;
		if ( stack[i].type == OB_FILE )
			fclose(stack[i].value.v_file.fp) ;
	}
	if ( i < 0 )
		execpush(makeoperator(quitOP)) ;
	else {
		push(true) ;
		stacktop = i ;
	}
}

void
startOP(void)
{
}
