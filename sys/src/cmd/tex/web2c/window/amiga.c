/*
 * AMIGA.C: This is the Graphics Window interface to MetaFont for online
 * displays on the Commodore Amiga.  This file was written by Edmund Mergl
 * for his Amiga port of TeX 3.1 and MetaFont 2.7 from the original Unix
 * sources (see Fish Disks 611-616).
 *
 * Some modifications and improvements were made since 1993 by Andreas and
 * Stefan Scherer, Abt-Wolf-Strasse 17, 96215 Lichtenfels, Germany.  The
 * most recent entries from top to bottom always superseed their ancestors,
 * just read this as a bit of history:
 *
 *   August 12, 1993:
 *
 *   - We use the `memory hack', so the values for `screenwidth'
 *     and `screendepth' are to be found in `mfmemory.config'.  The
 *     display height should be set to an InterLace value, this will
 *     be dealt with by `lacefactor' (see below).
 *
 *   - The display window is no longer resizeable, because no
 *     `screenupdate' is called and the picture is destroyed.
 *
 *   - Based on ideas by Stefan Becker used in his famous MetaFont port
 *     to the Amiga of 1991 (see Fish Disk 486) there is a variable
 *     `scalefactor', which can be set to a (small) positiv integer in
 *     `mfmemory.config'.  This results in a reduced size of the displayed
 *     characters, so that large font pictures can be drawn on
 *     small-sized screens (although some distortions are possible).
 *
 *   - By replacing `Move' and `Draw' with `WritePixelLine' and friends
 *     the speed of the display was heavily increased.
 *
 *   - The display screen is totally controlled by `screenwidth' and
 *     `screendepth' from `mfmemory.config', when you set `screen_rows'
 *     and `screen_cols' both to the maximum value `4095' in your local
 *     device modes file.  MetaFont takes the smaller of these pairs.
 *
 *   - `blankrectangle' *always* set the complete display window to the
 *     background color, not only the area specified by `left', `right',
 *     `top', and `bottom'.  These MetaFont coordinates are now obeyed.
 *
 *   September 11, 1993:
 *
 *   - The changes for `Computers and Typesetting -- Volume A-E' provided
 *     by Addison-Wesley Publishing Company resulted in MetaFont 2.71.
 *     This caused a change in the window title.
 *
 *   October 22, 1993:
 *
 *   - The official version 2.71 has the same window title.  :-)
 *
 *   January 28, 1994:
 *
 *   - Although SAS/C has an AutoOpen feature for Amiga libraries,
 *     it is better to define the library bases explicitly.
 *
 *   February 15, 1995:
 *
 *   - Here's my new address:
 *     Andreas Scherer, Rolandstraﬂe 16, 52070 Aachen, Germany.
 *     <scherer@genesis.informatik.rwth-aachen.de> (Internet).
 *   - Make `amiga.c' compliant with the syntax of the other modules
 *     in the Web2C 6.1 distribution.
 *
 *   February 19, 1995:
 *
 *   - `close_all()' must not have arguments, because we want to
 *     link it into the `atexit()' function list.
 *   - To distinguish this version from the old distribution, the
 *     window title is modified.
 *
 *   March 23, 1995:
 *
 *   - MetaFont has the new version number 2.718.
 *
 *   April 9, 1995:
 *
 *   - rename `screenheight' to `screendepth' for consistency.
 *
 *   October 10, 1995:
 *
 *   - Yet another change in my address, now it's a UNIX CIP pool:
 *     <scherer@physik.rwth-aachen.de>
 *   - Cleanup for the new web2c release.
 */

#define EXTERN extern

#include "../mfd.h"

#ifdef AMIGAWIN /* Whole file */

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct Screen *Scr;
struct Window *Win;

struct BitMap tempbm;
struct RastPort temprp;

UBYTE *linearray;

int scaledwidth  = 0;
int scaledheight = 0;

#ifndef scalefactor
#define scalefactor 1
#endif

/*
 * The configuration `mf.mcf' of AmiWeb2C holds values for `screenwidth'
 * and `screendepth' that always refer to an InterLace screen.  In case
 * you open the display on a non-InterLaced screen, only half the lines
 * are shown to you.  This is automatically done by reading the screen
 * information and setting `lacefactor'.
 */
int lacefactor;

#define BACKGROUNDCOLOR 0
#define PENCOLOR        1

#define PUBSCREEN 0

/*
 * Allocating memory for the array used in `WritePixelLine' needs
 * WORD-aligned arguments.
 */
#define ALIGN_SIXTEEN_BIT(A) ((((A)+15)/16)*16)
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

/*
 * CLOSE_ALL: Clean up on MetaFont termination.  This routine has to be
 * called just before leaving the program, which is done automatically
 * when it gets entered into the `atexit' function list.  (See below.)
 */
void close_all(void)
{
  if (linearray)        free(linearray);
  if (tempbm.Planes[0]) FreeRaster(tempbm.Planes[0], scaledwidth, 1);
  if (Win)              CloseWindow(Win);
  if (GfxBase)          CloseLibrary((struct Library *)GfxBase);
  if (IntuitionBase)    CloseLibrary((struct Library *)IntuitionBase);

} /* close_all() */

/*
 * INITSCREEN: Initialize the physical display window on the WorkBench
 * screen of the Commodore Amiga.  Open the necessary libraries and open
 * the window according to `screenwidth' and `screendepth' as given in
 * the configuration file `mf.mcf'.  For this purpose, the
 * `OpenWindowTagList' routine was replaced with `OpenWindowTags'.
 * The `WFLG_SIZEGADGET' window flag has been removed; resizing the
 * window kills the picture and never triggered an `updatescreen'.
 */
boolean mf_amiga_initscreen(void)
{
#ifdef DEBUG
  printf("\ninitscreen()\n");
#endif

  /*
   * Make sure the resources are correctly returned to the system
   * when leaving the program.
   */
  if(atexit(&close_all)) { /* Can't happen. */
    fprintf(stderr, "Exit trap failure! (close_all)\n");
    uexit(1);
    }

  if (!(IntuitionBase = (struct IntuitionBase *)
    OpenLibrary((unsigned char *)"intuition.library",(unsigned long)37L)))
  {
    fprintf(stderr,"\nCan't open intuition library. V37 required.\n");
    close_all();
    return(FALSE);
  }

  if (!(GfxBase = (struct GfxBase *)
    OpenLibrary((unsigned char *)"graphics.library",(unsigned long)37L)))
  {
    fprintf(stderr,"\nCan't open intuition library. V37 required.\n");
    close_all();
    return(FALSE);
  }

  if (!(Scr = LockPubScreen(PUBSCREEN)))
  {
    fprintf(stderr,
      "\nCan't get lock on public screen (%d).\n", PUBSCREEN);
    close_all();
    return(FALSE);
  }

  scaledwidth  = screenwidth/scalefactor;
  scaledheight = screendepth/scalefactor;

  lacefactor  = ((Scr->ViewPort.Modes & LACE) ? 1 : 2);

  if (!(Win = (struct Window *)OpenWindowTags( NULL,
    WA_InnerWidth,  scaledwidth,
    WA_InnerHeight, scaledheight/lacefactor,
    WA_Flags,       WFLG_DRAGBAR|WFLG_DEPTHGADGET,
    WA_AutoAdjust,  FALSE,
    WA_Title,       (ULONG)" MetaFont V2.718 Online Display",
    WA_PubScreen,   (ULONG)Scr,
    TAG_DONE)))
  {

    fprintf(stderr,
       "\nCan't open online display window at size %d times %d.\n"
#ifdef VARMEM
       "Change your configuration file.\n",
#else
       "If you really absolutely need more capacity,\n"
       "you can ask a wizard to enlarge me.\n",
#endif
    screenwidth,screendepth);

    close_all();
    return(FALSE);
  }

  InitBitMap(&tempbm, 1, scaledwidth, 1);

  tempbm.Planes[0] = NULL;

  if(!(tempbm.Planes[0] = (PLANEPTR)AllocRaster(scaledwidth, 1)))
     return(FALSE);

  InitRastPort(&temprp);

  temprp.BitMap = &tempbm;

  if(!(linearray = (UBYTE *)
    calloc(ALIGN_SIXTEEN_BIT(scaledwidth),sizeof(UBYTE))))
    return(FALSE);

  UnlockPubScreen(NULL, Scr);

#ifdef DEBUG
   printf("initscreen() ok\n");
#endif

   return(TRUE);

} /* mf_amiga_initscreen() */

/*
 * UPDATESCREEN: I really don't know what this function is supposed
 * to do.
 */
void mf_amiga_updatescreen(void)
{
#ifdef DEBUG
  printf("updatescreen()\n");
#endif
} /* mf_amiga_update_screen() */

/*
 * BLANKRECTANGLE: Reset the drawing rectangle bounded by
 * ([left,right],[top,bottom]) to the background color.
 */
void mf_amiga_blankrectangle(screencol left, screencol right,
  screenrow top, screenrow bottom)
{
#ifdef DEBUG
  printf("blankrectangle() - left: %d right: %d top: %d bottom: %d\n",
    left, right, top, bottom);
#endif

  SetAPen (Win->RPort, BACKGROUNDCOLOR);
  RectFill(Win->RPort,
    (SHORT)(Win->BorderLeft + left/scalefactor),
    (SHORT)(Win->BorderTop + top/(scalefactor*lacefactor)),
    (SHORT)(Win->BorderLeft + right/scalefactor - 1),
    (SHORT)(Win->BorderTop + bottom/(scalefactor*lacefactor) - 1));
} /* mf_amiga_blankrectangle() */

/*
 * PAINTROW: Paint a `row' starting with color `init_color', up to the
 * next transition specified by `transition_vector', switch colors, and
 * continue for `vector_size' transitions.  It now only
 * draws into the display window when a new raster line will be affected.
 * A lot of pre-drawing and inter-drawing calculations and conditionals
 * are evaluated to prepare a fast and clean drawing under the most
 * circumstances to be expected on the Amiga.  InterLace/Lace is taken
 * care of and the sizes are set appropriately not to clobber the borders.
 * (Don't ask me why this works; at least on my machine with my config.)
 */
void mf_amiga_paintrow(screenrow row, pixelcolor init_color,
  transspec transition_vector, screencol vector_size)
{
  int start, scaledstart, scaledrow, scaledlaced;
  register int i=0, j=0, col, color;

#ifdef DEBUG
    printf("paintrow() - vector size: %d\n", vector_size);
#endif

  color       = ((0 == init_color) ? BACKGROUNDCOLOR : PENCOLOR);
  scaledlaced = scalefactor * lacefactor;
  start       = *transition_vector++;
  scaledstart = start / scalefactor;
  scaledrow   = row / scaledlaced;

  ReadPixelLine8(Win->RPort, Win->BorderLeft + scaledstart,
    Win->BorderTop + scaledrow, scaledwidth - scaledstart,
    linearray, &temprp);

  if(scalefactor>1) {

    do {
      for(col = *transition_vector++; i<col-start; i++) {
        linearray[j] |= color;
        if((i%scalefactor)==0) j++;
      }

      color = PENCOLOR - color;

#ifdef DEBUG
      printf("move col: %d row: %d ", col, row);
      printf("draw col: %d row: %d\n", (*transition_vector)-1, row);
#endif

    } while (--vector_size);

    if(i >= col-start) linearray[++j] |= color;

  } else {

    do {
      for(col = *transition_vector++; j<col-start; j++) {
         linearray[j] |= color;
         }

      color = PENCOLOR - color;

#ifdef DEBUG
      printf("move col: %d row: %d ", col, row);
      printf("draw col: %d row: %d\n", (*transition_vector)-1, row);
#endif

    } while (--vector_size);
  }

  if( (  Win->BorderTop + scaledrow ) <
      (  Win->Height - Win->BorderBottom ) )
    WritePixelLine8(Win->RPort, Win->BorderLeft + scaledstart,
      Win->BorderTop + scaledrow,
      min(Win->Width - Win->BorderLeft - Win->BorderRight - scaledstart,j),
      linearray, &temprp);

} /* mf_amiga_paintrow() */

#else
int amiga_dummy;
#endif /* AMIGAWIN */
