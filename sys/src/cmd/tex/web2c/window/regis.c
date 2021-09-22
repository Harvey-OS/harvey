/* Regis terminal window interface for Metafont, joe@rilgp.tamri.com.
   screen_rows is 480; screen_cols is 800. */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef REGISWIN		/* Whole file */

#define ESCAPE		27

/* set this up in one of two ways.  if defined, display is white characters
on a black background; undefined is the opposite */
#undef WRITEWHITE

/* 
 *	int init_screen 
 *		Put screen in graphics mode:<ESC>Pp
 *		Write no or both planes for the background black:S(I0)
 *		Write both or no planes for the forground white:W(I3)
 *		Erase screen:S(E)
 *		Return to alpha mode: <ESC>\ 
 *
 *		Assuming that the speed limitation is a serial line to the
 *		terminal, we want to define macros for the most common
 *		character combinations.
 *		Define macros for ,+0]P[ (call this "p") and
 *		,+0]V[ (this one is called "v").
 *		drawing a line is 4+(2 to 6) characters
 *		We always return true.
 */

int mf_regis_initscreen()
{
#ifdef WRITEWHITE
	printf("%cPpS(I0)W(I3)S(E)%c",ESCAPE,ESCAPE);
#else
	printf("%cPpS(I3)W(I0)S(E)%c",ESCAPE,ESCAPE);
#endif
	printf("%cPp@:p,+0]P[@;@:v,+0]V[@;",ESCAPE);
	return 1;
}
/*
 *	procedure updatescreen;
 *
 */
void mf_regis_updatescreen()
{
}
 /*	void blankrectangle(int left,int right,int top,int bottom);
 *
 *		Go to graphics mode: <ESC>Pp
 *		Move to lower left: P[%d,%d]
 *		Write no or both planes: W(I0)
 *		Turn on shading: W(S1)
 *		Vector to lower right, upper right, upper left, lower left: V's
 *		Turn off shading: W(S0)
 *		Write both or no planes: W(I3)
 *		Return to alpha mode: <ESC>\ 
 */
void mf_regis_blankrectangle P4C(screencol, left,
                                 screencol, right,
                                 screenrow, top,
                                 screenrow, bottom)
{
	printf(
#ifdef WRITEWHITE
	"%cPpP[%d,%d]W(I0)W(S1)V[%d,%d]V[%d,%d]V[%d,%d]V[%d,%d]W(S0)W(I3)%c\\",
#else
	"%cPpP[%d,%d]W(I3)W(S1)V[%d,%d]V[%d,%d]V[%d,%d]V[%d,%d]W(S0)W(I0)%c\\",
#endif
		ESCAPE,left,bottom,right,bottom,right,top,left,top,
		left,bottom,ESCAPE);
}

/*
 *	void paintrow(int row, int init_color, int* transition_vector,
 *					int vector_size);
 *		Paint "row" starting with color "init_color", up to next
 *		transition specified by "transition_vector", switch colors,
 *		and continue for "vector_size" transitions.
 */
void mf_regis_paintrow P4C(screenrow,   row,
                           pixelcolor,  init_color,
                           transspec,   transition_vector,
                           screencol,   vector_size)
{
	int i;
	if(init_color) {
		init_color = 1;
	} else {
		init_color = 0;
	}
	printf("%cPpP[0,%d]P[",ESCAPE,row);
	for(i=0;i<vector_size;i++) {
		if(init_color)
		printf("%d@v%d@p",transition_vector[i],
						transition_vector[i+1]);
		init_color = 1-init_color;
	}
	printf("+0,+0]%c\\",ESCAPE);
}

#else
int regis_dummy;
#endif	/* REGISWIN */
