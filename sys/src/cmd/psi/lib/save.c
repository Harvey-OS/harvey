#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
# include "object.h"
# include "defines.h"
#include "njerq.h"
#include "cache.h"
#include "dict.h"
# define MIN_MEM	2048
# define ARENASIZE	100000

struct	namelist
{
	struct	pstring	pstring ;
	struct namelist	*next ;
} ;

struct	saveitem
{
	char		*adrs ;
	int		size ;
	char		*value ;
	struct saveitem	*next ;
} ;

static	struct saveitem	*stack[SAVE_LEVEL_LIMIT] ;
static	int		stacktop ;
static	struct namelist	*namelist ;
static	char		*max_mem ;
static	char		*cur_mem ;
char		*beg_mem ;

void
savestackinit(void)
{
	char *x;
	x = malloc(30000);		/*space for stdio to alloc*/
	if(!x){
		fprintf(stderr,"can't malloc\n");
		pserror("internal","savestackinit");
	}
	free(x) ;		/*space for stdio to alloc*/
#ifndef Plan9
	x = malloc(CACHE_SPACE);
	if(!x){
		fprintf(stderr,"can't malloc cache\n");
		pserror("internal","savestackinit");
	}
	free(x);	/*space for font cache bitmaps*/
#endif
	beg_mem = sbrk(0) ;
	max_mem = cur_mem = beg_mem ;
	stacktop = 0 ;
	namelist = NULL ;
}

void
saveOP(void)
{
	struct	object	save ;
	if ( stacktop >= SAVE_LEVEL_LIMIT )
		pserror("limitcheck", "save") ;
	stack[stacktop] = NULL ;
	save = makesave(stacktop++,namelist,cur_mem) ;
	gsave(save.value.v_save) ;
	push(save) ;
}

void
saveitem(char *adrs, int size)
{
	register int i;
	register struct saveitem	*save ;

	if ( stacktop == 0 )
		return ;
	i=stacktop-1;
	for ( save=stack[i] ; save!=NULL ; save=save->next )
		if ( save->adrs == adrs )
			return ;
	save = (struct saveitem *)vm_alloc(sizeof(*save)) ;
	save->adrs = adrs ;
	save->size = size ;
	save->value = vm_alloc(size) ;
	memmove(save->value,adrs,size) ;
	save->next = stack[i] ;
	stack[i] = save ;
}

void
restoreOP(void)
{
	struct	object	save ;
	struct saveitem	*p ;
	save = pop() ;
	if ( save.type != OB_SAVE ){
		fprintf(stderr,"type %o\n",save.type);
		pserror("typecheck", "restore") ;
	}
	dict_invalid(save.value.v_save->cur_mem) ;
	exec_invalid(save.value.v_save->cur_mem) ;
	oper_invalid(save.value.v_save->cur_mem) ;

	for ( ; stacktop>save.value.v_save->level ; stacktop-- )
		for ( p=stack[stacktop-1] ; p!=NULL ; p=p->next )
			memmove(p->adrs,p->value,p->size) ;
	namelist = save.value.v_save->namelist ;
	cur_mem = save.value.v_save->cur_mem ;
	fontchanged = 1;
	grestoreall(save.value.v_save) ;
}

struct	object
makename(unsigned char *p, int length, enum xattr xattr)
{
	struct namelist	*listptr ;
	struct	object	object ;
	if ( length > NAME_LIMIT )
		pserror("limitcheck", "makename") ;
	for ( listptr=namelist ; listptr!=NULL ; listptr=listptr->next )
		if (length == listptr->pstring.length  &&
			memcmp(p,listptr->pstring.chars,length) == 0 )
			goto found ;
	listptr = (struct namelist *)vm_alloc(sizeof(*listptr)) ;
	listptr->pstring.length = length ;
	listptr->pstring.chars =(unsigned char *)memmove(vm_alloc(length),p,length);
	listptr->next = namelist ;
	namelist = listptr ;
found:
	object.type = OB_NAME ;
	object.xattr = xattr ;
	object.value.v_string = listptr->pstring ;
	return(object) ;
}
struct object
pckname(struct pstring pstr, enum xattr xattr)
{
	struct namelist	*listptr ;
	struct	object	object ;

	for ( listptr=namelist ; listptr!=NULL ; listptr=listptr->next )
		if (pstr.length == listptr->pstring.length  &&
			memcmp(pstr.chars,listptr->pstring.chars,pstr.length)== 0){
			pstr=listptr->pstring;
			goto found ;
	}
	listptr = (struct namelist *)vm_alloc(sizeof(*listptr)) ;
	listptr->pstring = pstr;
	listptr->next = namelist ;
	namelist = listptr ;
found:
	object.type = OB_NAME ;
	object.xattr = xattr;
	object.value.v_string = pstr ;
	return(object) ;
}

struct object
pname(struct pstring pstr, enum xattr xattr)
{
	struct object object;
	struct namelist *listptr;

	listptr = (struct namelist *)vm_alloc(sizeof(*listptr)) ;
	listptr->next = namelist ;
	listptr->pstring = pstr;
	namelist = listptr ;
	object.type = OB_NAME ;
	object.xattr = xattr;
	object.value.v_string = pstr;
	return(object) ;
}
char	*
vm_alloc(int n)
{
	int	amount ;
	char	*ret ;
	n = (n+3) & ~3 ;
	ret = cur_mem ;
	if ( cur_mem + n > max_mem ){
		if ( n < MIN_MEM )
			amount = MIN_MEM ;
		else{
			amount = n ;
		}
		if ( brk(max_mem+amount) == -1 )
			pserror("VMerror", "vm_alloc") ;
		max_mem += amount ;
	}
	cur_mem += n ;
	return(ret) ;	
}

void
vmstatusOP(void)
{
	push(makeint(stacktop)) ;
	push(makeint(cur_mem-beg_mem)) ;
	push(makeint(max_mem-beg_mem)) ;
}

void
invalid(struct object object, char *mem_bound)
{
	char	*p ;

	switch(object.type){
	case OB_ARRAY :
		p = (char *)object.value.v_array.object ;
		break ;

	case OB_DICTIONARY :
		p = (char *)object.value.v_dict ;
		break ;

	case OB_FILE :
		return ;

	case OB_NAME :
		p = (char *)object.value.v_string.chars ;
		break ;

	case OB_SAVE :
		p = (char *)object.value.v_save->cur_mem ;
		break ;

	case OB_STRING :
		p = (char *)object.value.v_string.chars ;
		break ;

	default :
		return ;
	}
	if ( p >= mem_bound )
		pserror("invalidrestore", "invalid") ;
}
