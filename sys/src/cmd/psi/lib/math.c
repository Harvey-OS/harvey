# include <u.h>
#include <libc.h>
#include "system.h"

# include "stdio.h"
# include "defines.h"
# include "object.h"

struct	object
number(char *cmd)
{
	struct	object	object ;

	object = pop() ;
	if ( !(object.type&OB_NUMBER) )
		pserror("typecheck", cmd) ;
	return(object) ;
}

struct	object
real(char *cmd)
{
	register struct	object	object ;

	object = pop() ;
	if ( object.type == OB_REAL)return(object);
	if ( object.type == OB_INT ){
		object.value.v_real = object.value.v_int ;
		object.type = OB_REAL ;
		return(object);
	}
	else {
		fprintf(stderr,"type %d ",object.type);
		pserror("typecheck", cmd) ;
	}
}

struct	object
integer(char *cmd)
{
	struct	object	object ;

	object = pop() ;
	if ( object.type != OB_INT ){
		fprintf(stderr,"type %d ",object.type);
		pserror("typecheck", cmd) ;
	}
	return(object) ;
}

void
absOP(void)
{
	struct	object	object ;

	object = number("abs") ;
	if ( object.type == OB_INT )
		if ( object.value.v_int == MAXNEG ){
			object.value.v_real = -(double)object.value.v_int ;
			object.type = OB_REAL ;
		}
		else{
			if ( object.value.v_int < 0 )
				object.value.v_int = -object.value.v_int ;
		}
	else if ( object.value.v_real < 0.0 )
		object.value.v_real = -object.value.v_real ;
	push(object) ;
}

void
addOP(void)
{
	struct	object	right, left ;

	right = number("add") ;
	left = number("add") ;
	if ( left.type == OB_INT  &&  right.type == OB_INT )
		if(left.value.v_int < 0  &&  right.value.v_int < 0
		   &&  left.value.v_int + right.value.v_int >= 0  ||
		   left.value.v_int > 0  &&  right.value.v_int > 0
		   &&  left.value.v_int + right.value.v_int <= 0){
			left.value.v_real = (double)left.value.v_int + right.value.v_int ;
			left.type = OB_REAL ;
		}
		else left.value.v_int += right.value.v_int ;
	else if ( left.type == OB_INT  &&  right.type == OB_REAL ){
		left.value.v_real = left.value.v_int + right.value.v_real ;
		left.type = OB_REAL ;
	}
	else if ( left.type == OB_REAL  &&  right.type == OB_INT )
		left.value.v_real += right.value.v_int ;
	else left.value.v_real += right.value.v_real ;
	push(left) ;
}

void
divOP(void)
{
	struct	object	left, right ;

	right = real("div") ;
	left = real("div") ;
	if ( right.value.v_real == 0.0 )
		pserror("undefinedresult", "div by 0") ;
	left.value.v_real /= right.value.v_real ;
	push(left) ;
}

void
idivOP(void)
{
	struct	object	left, right ;

	right = integer("idiv") ;
	left = integer("idiv") ;
	if ( right.value.v_int == 0 )
		pserror("undefinedresult", "idiv by 0") ;
	if ( left.value.v_int == MAXNEG  &&  right.value.v_int == -1 ){
		fprintf(stderr,"idiv values %d %d\n",left.value.v_int,right.value.v_int);
		pserror("rangecheck", "idiv") ;
	}
	left.value.v_int /= right.value.v_int ;
	push(left) ;
}

void
modOP(void)
{
	struct	object	left, right ;

	right = integer("mod") ;
	left = integer("mod") ;
	if ( right.value.v_int == 0 )
		pserror("undefinedresult", "mod of 0") ;
	left.value.v_int %= right.value.v_int ;
	push(left) ;
}

void
mulOP(void)
{
	struct	object	right, left ;

	right = number("mul") ;
	left = number("mul") ;
	if ( left.type == OB_INT  &&  right.type == OB_INT )
		if(right.value.v_int == 0  ||
		   left.value.v_int*right.value.v_int/right.value.v_int
		    == left.value.v_int)
			left.value.v_int *= right.value.v_int ;
		else{
			left.value.v_real = (double)left.value.v_int*right.value.v_int ;
			left.type = OB_REAL ;
		}
	else if ( left.type == OB_INT  &&  right.type == OB_REAL ){
		left.value.v_real = left.value.v_int * right.value.v_real ;
		left.type = OB_REAL ;
	}
	else if ( left.type == OB_REAL  &&  right.type == OB_INT )
		left.value.v_real *= right.value.v_int ;
	else left.value.v_real *= right.value.v_real ;
	push(left) ;
}

void
negOP(void)
{
	struct	object	object ;

	object = number("neg") ;
	if ( object.type == OB_INT )
		if ( object.value.v_int == MAXNEG ){
			object.value.v_real = -(double)object.value.v_int ;
			object.type = OB_REAL ;
		}
		else object.value.v_int = -object.value.v_int ;
	else object.value.v_real = -object.value.v_real ;
	push(object) ;
}

void
subOP(void)
{
	struct	object	right, left ;

	right = number("sub") ;
	left = number("sub") ;
	if ( left.type == OB_INT  &&  right.type == OB_INT )
		if(left.value.v_int < 0  &&  right.value.v_int > 0
		   && left.value.v_int - right.value.v_int >= 0  ||
		   left.value.v_int > 0  &&  right.value.v_int < 0
		   && left.value.v_int - right.value.v_int <= 0){
			left.value.v_real = (double)left.value.v_int-right.value.v_int ;
			left.type = OB_REAL ;
		}
		else left.value.v_int -= right.value.v_int ;
	else if ( left.type == OB_INT  &&  right.type == OB_REAL ){
		left.value.v_real = left.value.v_int - right.value.v_real ;
		left.type = OB_REAL ;
	}
	else if ( left.type == OB_REAL  &&  right.type == OB_INT )
		left.value.v_real -= right.value.v_int ;
	else left.value.v_real -= right.value.v_real ;
	push(left) ;
}

void
ceilingOP(void)
{
	struct	object	object ;

	object = number("ceiling") ;
	if ( object.type == OB_REAL )
		object.value.v_real = ceil(object.value.v_real) ;
	push(object) ;
}

void
floorOP(void)
{
	struct	object	object ;

	object = number("floor") ;
	if ( object.type == OB_REAL )
		object.value.v_real = floor(object.value.v_real) ;
	push(object) ;
}

void
roundOP(void)
{
	struct	object	object ;

	object = number("round") ;
	if ( object.type == OB_REAL )
		object.value.v_real = floor(object.value.v_real+0.5) ;
	push(object) ;
}

void
truncateOP(void)
{
	struct	object	object ;

	object = number("truncate") ;
	if ( object.type == OB_REAL )
		if ( object.value.v_real > 0.0 )
			object.value.v_real = floor(object.value.v_real) ;
		else
			object.value.v_real = ceil(object.value.v_real) ;
	push(object) ;
}

void
atanOP(void)
{
	struct	object	num, den ;

	den = real("atan") ;
	num = real("atan") ;
	if(num.value.v_real == 0. && den.value.v_real == 0.)
		pserror("undefinedresult", "atan - both values 0.");
	num.value.v_real = atan2(num.value.v_real,den.value.v_real) * 180 / M_PI ;
	push(num) ;
}

void
cosOP(void)
{
	struct	object	object ;

	object = real("cos") ;
	object.value.v_real = cos(object.value.v_real*M_PI/180) ;
	push(object) ;
}

void
sinOP(void)
{
	struct	object	object ;

	object = real("sin") ;
	object.value.v_real = sin(object.value.v_real*M_PI/180) ;
	push(object) ;
}


void
expOP(void)
{
	struct	object	base, exponent ;

	exponent = real("exp") ;
	base = real("exp") ;
	base.value.v_real = pow(base.value.v_real,exponent.value.v_real) ;
	if (isNaN(base.value.v_real) ){
		fprintf(stderr,"exp result not-a-number exp %f base %f\n",
		   exponent.value.v_real,base.value.v_real);
		pserror("undefinedresult", "exp") ;
	}
	push(base) ;
}

void
lnOP(void)
{
	struct	object	object ;

	object = real("ln") ;
	object.value.v_real = log(object.value.v_real) ;
	if ( isNaN(object.value.v_real) ){
		fprintf(stderr,"ln result not-a-number arg %f\n",object.value.v_real);
		pserror("undefinedresult", "ln") ;
	}
	push(object) ;
}

void
logOP(void)
{
	struct	object	object ;

	object = real("log") ;
	object.value.v_real = log10(object.value.v_real) ;
	if ( isNaN(object.value.v_real) ){
		fprintf(stderr,"log result not-a-number arg %f\n",object.value.v_real);
		pserror("undefinedresult", "log") ;
	}
	push(object) ;
}

void
sqrtOP(void)
{
	struct	object	object ;

	object = real("sqrt") ;
	if(object.value.v_real < 0.){
		fprintf(stderr,"sqrt of %f\n",object.value.v_real);
		pserror("undefinedresult",  "sqrt") ;
	}
	object.value.v_real = sqrt(object.value.v_real) ;
	push(object) ;
}

void
randOP(void)
{

	push(makeint(lrand48())) ;
}

void
srandOP(void)
{
	struct object object;
	object = integer("srand");
	srand48(object.value.v_int) ;
}

void
rrandOP(void)
{

	pserror("unimplemented", "rrand") ;
	push(makeint(0)) ;
}
