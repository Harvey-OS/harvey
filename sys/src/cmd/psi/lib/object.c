#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "dict.h"

static	struct	object	stack[OPERAND_STACK_LIMIT] ;
static	int		stacktop ;

void
operstackinit(void)
{
	stacktop = 0 ;
}

void
oper_invalid(char *mem_bound)
{
	int	i ;

	for ( i=0 ; i<stacktop ; i++ )
		invalid(stack[i],mem_bound) ;
}

void
push(struct object object)
{
	if ( stacktop == OPERAND_STACK_LIMIT ) 
		pserror("stackoverflow", "push") ;
	stack[stacktop++] = object ;
}

void
pushpoint(struct pspoint ppoint)
{
	push(makereal(ppoint.x)) ;
	push(makereal(ppoint.y)) ;
}
struct	object
pop(void)
{
	int i;
	struct object object;
	if ( stacktop <= 0 )
		pserror("stackunderflow", "pop") ;
	object = stack[stacktop];
	return(stack[--stacktop]) ;
}

void
popOP(void)
{
	pop() ;
}

void
exchOP(void)
{
	struct	object	left, right ;

	right = pop() ;
	left = pop() ;
	push(right) ;
	push(left) ;
}

void
dupOP(void)
{
	struct	object	object ;

	object = pop() ;
	push(object) ;
	push(object) ;
}

void
copyOP(void)
{
	int		i ;
	struct	object	left, right ;

	right = pop() ;
	switch(right.type){
	case OB_INT :
		if ( right.value.v_int < 0 ){
			fprintf(stderr,"copy <0 %d\n",right.value.v_int);
			pserror("rangecheck", "copy") ;
		}
		if ( right.value.v_int > stacktop ){
			fprintf(stderr,"copy too big %d\n",right.value.v_int);
			pserror("rangecheck", "copy") ;
		}
		for ( i=0 ; i<right.value.v_int ; i++ )
			push(stack[stacktop-right.value.v_int]) ;
		return;

	case OB_ARRAY :
		left = pop() ;
		if ( left.type != OB_ARRAY ){
			fprintf(stderr,"copy not array %d\n",left.value);
			pserror("typecheck", "copy array") ;
		}
		if ( right.value.v_array.length < left.value.v_array.length ){
			fprintf(stderr,"copy array too small l %d r %d\n",
			  left.value.v_array.length,right.value.v_array.length);
			pserror("rangecheck", "copy array") ;
		}
		saveitem(right.value.v_array.object,
		sizeof(right.value.v_array.object[0])*left.value.v_array.length);
		for ( i=0 ; i<left.value.v_array.length ; i++ )
			right.value.v_array.object[i] = left.value.v_array.object[i] ;
		right.value.v_array.length = left.value.v_array.length ;
		push(right) ;
		return;

	case OB_STRING :
		left = pop() ;
		if ( left.type != OB_STRING ){
			fprintf(stderr,"copy not string %d\n",left.type);
			pserror("typecheck", "copy string") ;
		}
		if ( right.value.v_string.length < left.value.v_string.length){
			fprintf(stderr,"copy sting too small %d %d\n",
			right.value.v_string.length,left.value.v_string.length);
			pserror("rangecheck", "copy string") ;
		}
		saveitem((char *)right.value.v_string.chars,
		sizeof(right.value.v_string.chars[0])*left.value.v_string.length);
		for ( i=0 ; i<left.value.v_array.length ; i++ )
			right.value.v_string.chars[i] = left.value.v_string.chars[i] ;
		right.value.v_string.length = left.value.v_string.length ;
		push(right) ;
		return;

	case OB_DICTIONARY :
		left = pop() ;
		if ( left.type != OB_DICTIONARY ){
			fprintf(stderr,"copy not dict %d\n",left.type);
			pserror("typecheck", "copy dict") ;
		}
		if ( right.value.v_dict->max < left.value.v_dict->cur ){
			fprintf(stderr,"dict too small %d %d\n",
			  right.value.v_dict->max,left.value.v_dict->cur);
			pserror("rangecheck", "copy dict") ;
		}
		if ( right.value.v_dict->cur != 0 )
			pserror("rangecheck", "copy dict") ;
		for ( i=0 ; i<left.value.v_dict->cur ; i++ )
			dictput(right.value.v_dict,left.value.v_dict->dictent[i].key,
			    left.value.v_dict->dictent[i].value) ;
		right.xattr = left.xattr ;
		SAVEITEM(right.value.v_dict->access) ;
		right.value.v_dict->access = left.value.v_dict->access ;
		push(right) ;
		return;

	default :
		pserror("typecheck", "copy") ;
	}
}

void
indexOP(void)
{
	struct	object	object ;

	object = integer("index") ;
	if ( object.value.v_int < 0 ){
		fprintf(stderr,"index<0 %d\n",object.value.v_int);
		pserror("rangecheck", "index") ;
	}
	if ( object.value.v_int >= stacktop ){
		fprintf(stderr,"index too big %d\n",object.value.v_int);
		pserror("rangecheck", "index") ;
	}
	push(stack[stacktop-object.value.v_int-1]) ;
}
void
rollOP(void)
{
	int	n, j ;
	struct object object;

	object = integer("roll");
	j = object.value.v_int ;
	object = integer("roll");
	n = object.value.v_int ;
	if ( n < 0 ){
		fprintf(stderr,"neg roll %d\n",n);
		pserror("rangecheck", "roll") ;
	}
	if ( n > stacktop ){
		fprintf(stderr,"too big %d\n",n);
		pserror("rangecheck", "roll") ;
	}
	rollrec(0,n,j) ;
}

void
rollrec(int i,int n,int j)
{
	struct	object	t ;
	int l;

	t = stack[stacktop-i-1] ;
	if ( i+1 < n )
		rollrec(i+1,n,j) ;
	l = stacktop-modulus(i-j,n)-1;
	stack[l] = t ;
}

int
modulus(int n, int m)
{
	n %= m ;
	if ( n < 0 )
		n += m ;
	return(n) ;
}

void
clearOP(void)
{
	stacktop = 0 ;
}

void
countOP(void)
{
	push(makeint(stacktop)) ;
}

void
markOP(void)
{
	struct	object	object ;
	object.type = OB_MARK ;	
	object.xattr = XA_LITERAL ;
	push(object) ;
}

void
cleartomarkOP(void)
{
	for ( stacktop-- ; stacktop>=0 ; stacktop-- )
		if ( stack[stacktop].type == OB_MARK )
			return ;
	pserror("unmatchedmark", "cleartomark") ;
}

void
counttomarkOP(void)
{
	int	i ;
	for ( i=stacktop ; i>0 ; i-- )
		if ( stack[i-1].type == OB_MARK ){
			push(makeint(stacktop-i)) ;
			return ;
		}	
	pserror("unmatchedmark", "counttomark") ;
}

void
pstackOP(void)
{
	int	i ;

	if((stacktop-1)< 0)fprintf(stderr,"stack empty\n");
	else
		for ( i=stacktop-1 ; i>=0 ; i-- )
			eqeq(stack[i],0) ;
}

void
stackOP(void)
{
	int	i, j, length ;
	unsigned char	*p ;

	if((stacktop-1)< 0)fprintf(stderr,"stack empty\n");
	else
		for ( i=stacktop-1 ; i>=0 ; i-- ){
			p = cvs(stack[i],&length) ;
			for ( j=0 ; j<length ; j++ )
				fputc(p[j],stderr) ;
			fputc('\n',stderr) ;
		}
}
