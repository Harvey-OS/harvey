#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
#include "defines.h"
# include "object.h"
#include "dict.h"
#include "path.h"
#include "graphics.h"

void
error(char *msg)
{
	fprintf(stderr,"%s: line %d: %s\n",Prog_name,Line_no, msg) ;
	dobitmap();
	done(1) ;
}

void
pserror(char *errorname, char *cmd)
{
	struct object dict;
	fprintf(stderr,"%s, line %d command %s\n",errorname, Line_no, cmd) ;
	if(Systemdict != 0){
	dict = dictget(Systemdict,cname("errordict"));
	if(dict.type != OB_NONE)
		execpush(dictget(dict.value.v_dict,cname(errorname))) ;
	else fprintf(stderr,"errordict undefined - big problems\n");
	}
	dobitmap();
	done(1) ;
}

void
warning(char *msg)
{
	fprintf(stderr,"%s: %s\n",Prog_name, msg) ;
}

void
errorhandleOP(void)
{
	fprintf(stderr,"yep, got an error\n") ;
	done(1) ;
}
void
trapOP(void)
{
	static int trap=0;
	struct object object;
	object = pop();
	if(object.type == OB_REAL)fprintf(stderr,"trap %f\n",object.value.v_real);
	else if(object.type == OB_INT)fprintf(stderr,"trap %d\n",object.value.v_int);
	else fprintf(stderr,"hit trap %d\n",trap++);
}
extern int rel, used, ct_el;
void
debug(void)
{
	struct object object;
	double toggle;
	int i;
	object = pop();
	if((toggle = object.value.v_real) == 0.)mdebug=0;
	else mdebug=toggle;
	fprintf(stderr,"%%in debug %d ",mdebug);
if(mdebug){
fprintf(stderr,"used %d rel %d ct_el %d\n",used, rel, ct_el);
}
	push(filename);
	printOP();
	fprintf(stderr,"\n");
}
