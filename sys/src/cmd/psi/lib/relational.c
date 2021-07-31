#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "object.h"

# define COMP(l,r,t)	( l.type==r.type  &&  l.value.t == r.value.t )

enum	bool
equal(struct object left, struct object right)
{
	switch(left.type){
	case OB_BOOLEAN :
		if ( COMP(left,right,v_boolean) )
			return(TRUE);
		return(FALSE);

	case OB_INT :
		if ( right.type == OB_INT ){
			if ( left.value.v_int == right.value.v_int )
				return(TRUE);
		}
		else if ( right.type == OB_REAL )
			if ( (double)left.value.v_int == right.value.v_real )
				return(TRUE);
		return(FALSE);

	case OB_NAME :
		if ( right.type == OB_NAME ){
			if ( COMP(left,right,v_string.chars) )
				return(TRUE);
		}
		else if ( right.type == OB_STRING )
			if ( comp_str(left.value.v_string,right.value.v_string) == 0 )
				return(TRUE);
		return(FALSE);

	case OB_REAL :
		if ( right.type == OB_REAL ){
			if ( left.value.v_real == right.value.v_real )
				return(TRUE);
		}
		else if ( right.type == OB_INT )
			if ( left.value.v_real == (double)right.value.v_int )
				return(TRUE);
		return(FALSE);


	case OB_STRING :
		if ( right.type == OB_STRING  ||  right.type == OB_NAME )
			if ( comp_str(left.value.v_string,right.value.v_string) == 0 )
				return(TRUE);
		return(FALSE);

	case OB_ARRAY :
		if ( COMP(left,right,v_array.object)  &&
		     left.value.v_array.length == right.value.v_array.length )
			return(TRUE);
		return(FALSE);

	case OB_DICTIONARY :
		if ( COMP(left,right,v_dict) )
			return(TRUE);
		return(FALSE);

	case OB_FILE :
		if ( COMP(left,right,v_file.fp) )
			return(TRUE);
		return(FALSE);

	case OB_FONTID :
		pserror("unimplemented", "equal") ;

	case OB_MARK :
		if ( right.type == OB_MARK )
			return(TRUE);
		return(FALSE);

	case OB_NULL :
		if ( right.type == OB_NULL )
			return(TRUE);
		return(FALSE);

	case OB_OPERATOR :
		if ( COMP(left,right,v_operator) )
			return(TRUE);
		return(FALSE);

	case OB_SAVE :
		if ( COMP(left,right,v_save->level) )
			return(TRUE);
		return(FALSE);

	default :
		fprintf(stderr,"left %d right %d\n",left.type,right.type);
		error("bad case in equal");
	}
}

void
eqOP(void)
{
	push(makebool(equal(pop(),pop()))) ;
}

void
neOP(void)
{
	enum	bool	val ;

	val = equal(pop(),pop()) ;
	if ( val == TRUE )
		push(false) ;
	else
		push(true) ;
}

void
geOP(void)
{
	struct	object	left, right ;

	right = pop() ;
	left = pop() ;
	push(makebool(!!(compare(left,right)>=0))) ;
}

void
leOP(void)
{
	struct	object	left, right;

	right = pop() ;
	left = pop() ;
	push(makebool(!!(compare(left,right)<=0))) ;
}
void
gtOP(void)
{
	struct	object	left, right;

	right = pop() ;
	left = pop() ;
	push(makebool(!!(compare(left,right)>0))) ;
}


void
ltOP(void)
{
	struct	object	left, right;

	right = pop() ;
	left = pop() ;
	push(makebool(!!(compare(left,right)<0))) ;
}

void
andOP(void)
{
	struct	object	left, right;

	right = pop() ;
	left = pop() ;
	switch(left.type){
	case OB_BOOLEAN :
		if(right.type != OB_BOOLEAN)
			pserror("typecheck", "and") ;
		if(left.value.v_boolean == TRUE  &&  right.value.v_boolean == TRUE)
			push(true);
		else push(false);
		return;

	case OB_INT :
		if ( right.type != OB_INT )
			pserror("typecheck", "and") ;
		push(makeint(left.value.v_int&right.value.v_int)) ;
		return;
	default :
		pserror("typecheck", "and") ;

	}
}

void
orOP(void)
{
	struct	object	left, right;

	right = pop() ;
	left = pop() ;
	switch(left.type){
	case OB_BOOLEAN :
		if ( right.type != OB_BOOLEAN )
			pserror("typecheck", "or") ;
		if ( left.value.v_boolean == TRUE || right.value.v_boolean == TRUE)
			push(true);
		else push(false);
		return;

	case OB_INT :
		if ( right.type != OB_INT )
			pserror("typecheck", "or") ;
		push(makeint(left.value.v_int|right.value.v_int)) ;
		return;

	default :
		pserror("typecheck", "or") ;
	}
}

void
xorOP(void)
{
	struct	object	left, right;

	right = pop() ;
	left = pop() ;
	switch(left.type){
	case OB_BOOLEAN :
		if ( right.type != OB_BOOLEAN )
			pserror("typecheck", "xor") ;
		if ( (left.value.v_boolean == TRUE) != (right.value.v_boolean == TRUE) )
			push(true);
		else push(false);
		return;

	case OB_INT :
		if ( right.type != OB_INT )
			pserror("typecheck", "xor") ;
		push(makeint(left.value.v_int^right.value.v_int)) ;
		return;

	default :
		pserror("typecheck", "xor") ;
	}
}

void
notOP(void)
{
	struct	object	object;

	object = pop() ;
	switch(object.type){
	case OB_BOOLEAN :
		if ( object.value.v_boolean == FALSE )
			push(true);
		else push(false);
		return;

	case OB_INT :
		push(makeint(~object.value.v_int));
		return;

	default :
		pserror("typecheck", "not") ;
	}
}

void
bitshiftOP(void)
{
	unsigned int	val;
	struct	object	object, shift;

	shift = pop() ;
	object = pop() ;
	if ( object.type != OB_INT  ||  shift.type != OB_INT )
		pserror("typecheck", "bitshift") ;
	val = (unsigned int)object.value.v_int ;
#ifdef I386
	/* deal with inability to shift by 32 */
	if ( shift.value.v_int >= 32 || shift.value.v_int <= -32 )
		val = 0;
        else
#endif
	if ( shift.value.v_int < 0 )
		val >>= -shift.value.v_int ;
	else
		val <<= shift.value.v_int ;
	object.value.v_int = (int)val ;
	push(object) ;
}

int
comp_str(struct pstring s1,struct pstring s2)
{
	int	i ;

	for ( i=0 ; i<s1.length&&i<s2.length ; i++ )
		if ( s1.chars[i] != s2.chars[i] )
			break ;
	if ( i < s1.length  &&  i < s2.length )
		return((int)s1.chars[i]-(int)s2.chars[i]) ;
	else return(s1.length-s2.length) ;
}

int
comp_int(int x, int y)
{
	if ( x < y )
		return(-1) ;
	else if ( x > y )
		return(1) ;
	else return(0) ;
}

int
comp_real(double x, double y)
{
	if ( x < y )
		return(-1) ;
	else if ( x > y )
		return(1) ;
	else return(0) ;
}

int
compare(struct object left, struct object right)
{

	switch(left.type){
	case OB_INT :
		if ( right.type == OB_INT )
			return(comp_int(left.value.v_int,right.value.v_int));
		else if ( right.type == OB_REAL )
			return(comp_real((double)left.value.v_int,right.value.v_real));
		else pserror("typecheck", "compare");

	case OB_REAL :
		if ( right.type == OB_REAL )
			return(comp_real(left.value.v_real,right.value.v_real));
		else if ( right.type == OB_INT )
			return(comp_real(left.value.v_real,(double)right.value.v_int));
		else pserror("typecheck", "compare");

	case OB_NAME :
	case OB_STRING :
		if ( right.type == OB_STRING || right.type == OB_NAME )
			return(comp_str(left.value.v_string,right.value.v_string));
		else pserror("typecheck", "compare");

	default :
		pserror("typecheck", "compare") ;
	}
}
