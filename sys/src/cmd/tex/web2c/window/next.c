/* next.c: display interface for the NeXT, joe@rilgp.tamri.com.

This device code requires a second program ("DrawingServant") to
act as the interface between sm and the WindowServer.  The second program
just serves to listen for commands on a pipe, but otherwise
send everything else with a DPSPrintf to the WindowServer.  The servant can
also do things like implement it's own event loop and handle things
like printing, saving .eps  and the like.  This seems (to me) to be the
best way to put a multiplatform program into a NeXT application.

You can get DrawingServant from sonata.cc.purdue.edu.
*/

/* with not too much work, the client side of things probably 
could be on another machine */
#define EXTERN extern
#include "../mfd.h"

#ifdef NEXTWIN		/* the whole file */
#define DRAWSERVER "DrawingServant"
#define DEFWIDTH 400	/* default width and height */
#define DEFHEIGHT 500

/*these default values are taken from plain.mf
and are used if we can't see anything better */
static int nextheight=DEFHEIGHT;
static int nextwidth=DEFWIDTH;
static int outpipe[2],inpipe[2];
static int pid;
static int nextscreenlooksOK = 0;
char outstring[1024];	/* the longest string pushed though a pipe */
/* these are used a lot, so macro-ize these two lines */
#define SENDPS write(outpipe[1],outstring,strlen(outstring)+1)
#define GETACK do{\
		read(inpipe[0],outstring,sizeof(outstring)-1);\
		} while(strncmp(outstring,"Ok",2))
#ifdef read
#undef read
#endif

int mf_next_initscreen()
{
	int i;
	void mf_next_closescreen();
	/* strings for height, width, in and out pipes */
	char hstr[20],wstr[20],instr[20],outstr[20];
	
	/* I should figure out how to use screen_rows and screen_cols
	to size the window. what I think I need is one of leftcol,rightcol
	toprow and botrow.  Let's find the first which is non-zero,
	at least until someone tells me what the real answer is.*/

	for(i=0;i<16;i++) {
		if((leftcol[i]-rightcol[i]) && (toprow[i]-botrow[i])) {
			nextwidth = rightcol[i]-leftcol[i];
			nextheight = botrow[i]-toprow[i];
			break;
		}
	}

	/* fork a process and assign some pipes.  return if unsuccessful */
	if( pipe(outpipe)== -1)
			return 0;
	if( pipe(inpipe)== -1)
			return 0;
	if( (pid=fork())== -1)
			return 0;

	if(pid==0) {
		/* things done by the child. we pass it height,width and
		input and output pipes */
		sprintf(hstr,"h %d ",nextheight);
		sprintf(wstr,"w %d ",nextwidth);
		sprintf(outstr,"i %d",outpipe[0]);
		sprintf(instr,"o %d",inpipe[1]);
		execl(DRAWSERVER,DRAWSERVER,hstr,wstr,instr,outstr,nil);
		exit(0);
	}
	sprintf(outstring,"initgraphics\n");
	SENDPS;
	GETACK;
	nextscreenlooksOK = 1;

	/* The prior version used a hacked version of uexit to kill the
	server...at the urging of karl berry, here is a more legit way to
	kill the server */
	atexit(*mf_next_closescreen);

	return 1;
}
/*
 *	void updatescreen;
 * 	does nothing
 *
 */
void mf_next_updatescreen()
{
}
/*
 *	void blankrectangle(int left,int right,int top,int bottom);
 *
 *		blank out a port of the screen.
 */
void mf_next_blankrectangle P4C(screencol, left,
                                screencol, right,
                                screenrow, top,
                                screenrow, bottom)
{

	if(left==0 && top==nextheight && right==nextwidth && bottom==0 ) {
		 /* clear and forgets PS strings */
		sprintf(outstring,"DSclear");
	} else {
		sprintf(outstring,
			" 1 setgray %d %d %d %d rectfill 0 setgray \n",
			left+1,top+1,right,bottom+1);
	}
		SENDPS;
		GETACK;
}

/*
 *	void paintrow(int row,int init_color,int transition_vector,
 *						int vector_size);
 *
 *		Paint "row" starting with color "init_color", up to next
 *		transition specified by "transition_vector", switch colors,
 *		and continue for "vector_size" transitions.
 */
void mf_next_paintrow P4C(screenrow,   row,
                          pixelcolor,  init_color,
                          transspec,   transition_vector,
                          screencol,   vector_size)
{
	int i,whereami;
	if(init_color) {
		init_color = 1;
	} else {
		init_color = 0;
	}
	whereami = 0;

	for(i=0;i<vector_size;i++) {
		if(init_color) {
			sprintf(outstring+whereami,
				"newpath %d %d moveto %d %d lineto stroke ",
					transition_vector[i],nextheight-row,
					transition_vector[i+1],nextheight-row);
			whereami = strlen(outstring);
			/* buffering is good.  perhaps. */
			if(whereami > 500) {
				SENDPS;
				GETACK;
				*outstring = 0;
				whereami = 0;
			}
		}
		init_color = 1-init_color;
	}
	if(whereami) {
		SENDPS;
		GETACK;
	}
}
/* this isn't part of the online display routines.  We need it to
kill DrawingServant.  This is called during exit */
void mf_next_closescreen()
{
	if(nextscreenlooksOK) {
		sprintf(outstring,"DSquit");
		SENDPS;
	}
}

#else
int next_dummy;
#endif	/* NEXTWIN */
