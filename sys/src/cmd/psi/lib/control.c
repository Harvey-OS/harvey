#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "object.h"
# include "dict.h"

void
forallOP(void)
{
	struct	object	proc, object ;

	proc = procedure() ;
	object = pop() ;
	execpush(makeoperator(loopnoop)) ;
	execpush(proc) ;
	execpush(object) ;
	switch(object.type){
	case OB_ARRAY :
		execpush(makeoperator(arrayforall)) ;
		return;

	case OB_DICTIONARY :
		execpush(makeint(0)) ;
		execpush(makeoperator(dictforall)) ;
		return;
		
	case OB_STRING :
		execpush(makeoperator(stringforall)) ;
		return;

	default :
		pserror("typecheck", "forall") ;
	}
}

void
forOP(void)
{
	struct	object	increment ;

	execpush(makeoperator(loopnoop)) ;
	execpush(pop()) ;
	execpush(pop()) ;
	increment = pop() ;
	execpush(increment) ;
	if ( increment.type == OB_REAL )
		execpush(real("for")) ;
	else
		execpush(integer("for")) ;
	execpush(makeoperator(xfor)) ;
}

void
repeatOP(void)
{
	struct	object	count ;

	execpush(makeoperator(loopnoop)) ;
	execpush(pop()) ;
	count = integer("repeat") ;
	if ( count.value.v_int < 0 )
		pserror("rangecheck", "repeat") ;
	execpush(count) ;
	execpush(makeoperator(xrepeat)) ;
}

void
loopOP(void)
{

	execpush(makeoperator(loopnoop)) ;
	execpush(pop()) ;
	execpush(makeoperator(xloop)) ;
}

void
ifOP(void)
{
	struct	object	proc, bool ;

	proc = procedure() ;
	bool = pop() ;
	if ( bool.type != OB_BOOLEAN )
		pserror("typecheck", "if") ;
	if ( bool.value.v_boolean == TRUE )
		execpush(proc) ;
}

void
ifelseOP(void)
{
	struct	object	proc1, proc2, bool ;

	proc2 = procedure() ;
	proc1 = procedure() ;
	bool = pop() ;
	if ( bool.type != OB_BOOLEAN )
		pserror("typecheck", "ifelse") ;
	if ( bool.value.v_boolean == TRUE )
		execpush(proc1) ;
	else
		execpush(proc2) ;
}
