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
extern int minx, miny, maxx, maxy;

static	struct graphics	stack[GSAVE_LEVEL_LIMIT] ;
static	int		stacktop ;
static int	save_level;

void
graphicsstackinit(void)
{
	stacktop = 0 ;
}

void
gsaveOP(void)
{
	gsave(NULL) ;
}

void
gsave(struct save *save)
{
	if ( stacktop >= GSAVE_LEVEL_LIMIT )
		pserror("limitcheck", "gsave") ;
	Graphics.save = save ;
	stack[stacktop++] = Graphics ;
	if(save != NULL)save_level=stacktop;
	if(Graphics.path.first != NULL)
		Graphics.path = ncopypath(Graphics.path.first) ;
	if(Graphics.clippath.first != NULL)
		Graphics.clippath = ncopypath(Graphics.clippath.first);

}

void
grestoreOP(void)
{
	if ( stacktop > 0 && stacktop > save_level){
		rel_path(Graphics.path.first) ;
		rel_path(Graphics.clippath.first);
		Graphics = stack[stacktop-1] ;
	}
	if ( stack[stacktop-1].save == NULL )
		--stacktop ;
}

void
grestoreallOP(void)
{
	grestoreall(NULL) ;
}

void
grestoreall(struct save *save)
{
	struct element *savel;
	struct pspoint swid, sorig;
	if(!charpathflg)rel_path(Graphics.path.first) ;
	else{
		savel = Graphics.path.first;
		swid = Graphics.width;
		sorig = Graphics.origin;
	}
	rel_path(Graphics.clippath.first);
	for ( ; stacktop>1 ; stacktop-- ){
		if ( stack[stacktop-1].save != NULL  &&
		   (save == NULL  ||  stack[stacktop-1].save == save)){
			break ;
		}
		rel_path(stack[stacktop-1].path.first) ;
		rel_path(stack[stacktop-1].clippath.first);
	}
	if ( stacktop > 0 )
		Graphics = stack[stacktop-1] ;
	if(charpathflg){	/*maybe should check for type3*/
		Graphics.path.first = savel;
		Graphics.width = swid;
		Graphics.origin = sorig;
	}
	if ( save != NULL ){
		--stacktop ;
		save_level=0;
	}
}

void
initgraphicsOP(void)
{

	initmatrixOP() ;
	Graphics.color.red = 0 ;
	Graphics.color.green = 0 ;
	Graphics.color.blue = 0 ;
	Graphics.linewidth = .5 ;
	Graphics.linecap = CAP_BUTT ;
	Graphics.linejoin = JOIN_MITER ;
	Graphics.dash.count = 0 ;
	Graphics.dash.offset = 0.0 ;
	Graphics.miterlimit = 10.0 ;
	rel_path(Graphics.path.first);
	Graphics.path.first = NULL ;
	Graphics.path.last = NULL ;
	Graphics.incharpath = 0;
	Graphics.origin.x = 0.0;
	Graphics.origin.y = 0.0;
	initclipOP() ;
}

void
setlinewidthOP(void)
{
	struct object object;
	struct pspoint p;
	object = real("setlinewidth");
	Graphics.linewidth = object.value.v_real*.5 ;
	if(Graphics.linewidth <= SMALL){
		p.x = .5;
		p.y = 0.;
		p = idtransform(p);
		Graphics.linewidth = p.x;
	}
}

void
setmiterlimitOP(void)
{
	double		limit ;
	struct object object;
	object = real("setmiterlimit");
	limit = object.value.v_real ;
	if ( limit < 1.0 )
		pserror("rangecheck", "setmiterlimit") ;
	Graphics.miterlimit = limit ;
}

void
setflatOP(void)
{
	double		flat ;
	struct object object;
	object = real("setflat");
	flat = object.value.v_real ;
	if ( flat < 0.2 )
		flat = 0.2 ;
	if ( flat > 100.0 )
		flat = 100.0 ;
	Graphics.flat = flat ;
}

void
setlinecapOP(void)
{
	struct object object;
	object = integer("setlinecap");
	Graphics.linecap = (enum linecap)object.value.v_int ;
}

void
setlinejoinOP(void)
{
	struct object object;
	object = integer("setlinejoin");
	Graphics.linejoin = (enum linejoin)object.value.v_int ;
}

void
setdashOP(void)
{
	int		i ;
	struct	dash	dash ;
	struct	object	object ;
	enum	bool	allzero ;
	object = real("setdash");
	dash.offset = object.value.v_real ;
	object = pop() ;
	if ( object.type != OB_ARRAY )
		pserror("typecheck", "setdash - array") ;
	dash.count = object.value.v_array.length ;
	if ( dash.count > DASH_LIMIT )
		pserror("limitcheck", "setdash - count") ;
	allzero = TRUE ;
	if(dash.count > 0){
		for ( i=0 ; i<dash.count ; i++ ){
			if ( object.value.v_array.object[i].type == OB_INT )
				dash.dash[i] = (double)object.value.v_array.object[i].value.v_int ;
			else if ( object.value.v_array.object[i].type == OB_REAL )
				dash.dash[i] = object.value.v_array.object[i].value.v_real ;
			else {
				fprintf(stderr,"array object not number\n");
				pserror("typecheck", "setdash") ;
			}
			if ( dash.dash[i] < 0.0 ){
				fprintf(stderr,"bad array element %f\n",dash.dash[i]);
				pserror("rangecheck", "setdash") ;
			}
			if ( dash.dash[i] != 0.0 )
				allzero = FALSE ;
		}
		if (allzero == TRUE )
			pserror("rangecheck", "setdash - all elements 0") ;
	}
	Graphics.dash = dash ;
}

void
currentmiterlimitOP(void)
{
	push(makereal(Graphics.miterlimit)) ;
}

void
currentlinewidthOP(void)
{
	push(makereal(Graphics.linewidth*2.)) ;
}

void
currentflatOP(void)
{
	push(makereal(Graphics.flat)) ;
}

void
currentlinecapOP(void)
{
	push(makeint((int)Graphics.linecap)) ;
}

void
currentlinejoinOP(void)
{
	push(makeint((int)Graphics.linejoin)) ;
}

void
currentdashOP(void)
{
	int		i ;
	struct	object	array, object ;
	
	array = makearray(Graphics.dash.count,XA_LITERAL) ;
	if(Graphics.dash.count > 0){
		for ( i=0 ; i<Graphics.dash.count ; i++ ){ /*avoid compiler bug*/
			object = makereal(Graphics.dash.dash[i]);
			array.value.v_array.object[i] = object ;
		}
	}
	push(array) ;
	push(makereal(Graphics.dash.offset)) ;
}
