#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"

# define VERSION	"23.0"

void
versionOP(void)
{
	int		length ;
	struct	object	string ;

	length = strlen(VERSION) ;
	string = makestring(length) ;
	strncpy((char *)string.value.v_string.chars,VERSION,length) ;
	push(string) ;
}

void
usertimeOP(void)
{
	double buffer;
	int ibuffer;

	buffer = cputime();
	ibuffer = buffer;
	push(makeint(ibuffer));
}

void
bindOP(void)
{
	struct	object	proc ;

	proc = pop();
	if(proc.type != OB_ARRAY){
		fprintf(stderr,"bind object not array - type %o\n",proc.type);
		pserror("typecheck", "bind");
	}
	psibind(proc.value.v_array,0) ;
	push(proc) ;
}

void
psibind(struct array array, int level)
{
	int		i ;
	struct	object	object ;

	for ( i=0 ; i<array.length ; i++ )
		if ( array.object[i].xattr == XA_EXECUTABLE )
			if ( array.object[i].type == OB_NAME ){
				object = dictstackget(array.object[i],NULL) ;
				if ( object.type == OB_OPERATOR &&
					object.value.v_operator != findfontOP ){
					SAVEITEM(array.object[i]) ;
					array.object[i] = object ;
				}
			}
			else if ( array.object[i].type == OB_ARRAY  &&
			     array.object[i].value.v_array.access == AC_UNLIMITED ){
				psibind(array.object[i].value.v_array,level+1) ;
				if ( level > 0 ){
					SAVEITEM(array.object[i].value.v_array.access) ;
					array.object[i].value.v_array.access = AC_READONLY ;
				}
			}
}

void
printOP(void)
{
	struct object object;
	char buf[1024], *p;
	int i;
	object = pop();
	for(i=0, p=buf; i<object.value.v_string.length; i++)
		*p++ = object.value.v_string.chars[i];
	*p = '\0';
	fprintf(stderr,"%s",buf);
}

void
flushOP(void)
{
	fflush(stderr);
}

void
lettertray(void)
{
}

void
pagecount(void)
{
	struct object object;
	object = makeint(pageno);
	push(object);
}
