/*
 *  title:    uni.c
 *  abstract: Uniterm Terminal Window interface for Metafont.
 *  author:   T.R.Hageman, Groningen, The Netherlands.
 *  created:  May 1990
 *  modified:
 *  description:
 *    Uniterm (Simon Poole's terminal emulator for the Atari ST)
 *    emulates a `smart' Tektronix 4014 graphics terminal,
 *    and allows selective erasing of the graphics screen.
 *    (I do not know whether this is a standard feature of smart Teks
 *     or an invention of Mr. Poole)
 *
 *    This file is offered as an alternative to the "standard"
 *    tektronix driver (which I find rather slow...)
 *
 *    {{a possible way to improve the standard TEK driver would be to
 *      remember the (merged) transition lists instead converting to
 *      a bit map and back again.}}
 *---*/

#define EXTERN extern
#include "../mfd.h"

#ifdef UNITERMWIN             /* Whole file */

#define WIDTH 1024
#define HEIGHT        780
/*
 *    Send a vector to the graphics terminal
 *    (in a slightly optimized way).
 */
static void
sendvector(x, y)
register unsigned     x, y;
{
      static int      Hi_Y, Lo_Y, Hi_X;       /* remembered values */
      register int    Lo_Y_sent = 0;
      register int    t;
#ifdef DEBUG
      if (x >= WIDTH)                 /* clip... */
              x = WIDTH - 1;
      if (y >= HEIGHT)
              y = HEIGHT - 1;
#endif
      /*
       * Send Hi_Y only if it has changed.
       */
      if ((t = 0x20|(y >> 5)) != Hi_Y) {
              Hi_Y = t, putchar(t);
      }
      /*
       * Likewise, send Lo_Y only if it has changed.
       * (and remember that it has been sent)
       */
      if ((t = 0x60|(y & 0x1f)) != Lo_Y) {
              Lo_Y_sent = 1;
              Lo_Y = t, putchar(t);
      }
      /*
       * A slight complication here. If Hi_X has changed,
       * we must send Lo_Y too, but only if we didn't already send it.
       */
      if ((t = 0x20|(x >> 5)) != Hi_X) {
              if (!Lo_Y_sent)
                      putchar(Lo_Y);
              Hi_X = t, putchar(t);
      }
      /*
       * Lo_X is always sent, so don't bother to remember it.
       */
      t = 0x40|(x & 0x1f), putchar(t);
}
/*
 *    Tektronix has origin in lower-left corner, whereas MetaFont
 *    has its origin in the upper-left corner.
 *    The next macro hides this.
 */
#define VECTOR(col,row) sendvector((unsigned)(col),(unsigned)(HEIGHT-1-(row)))
/*
 *    GS              - `Dark' vectors are in fact invisible, i.e., a move.
 *                      (Also switches from text- to graphics screen.)
 */
#define DARK()                putchar('\35')
/*
 *    CAN             - Switch from graphics- to text screen.
 */
#define TEXT_SCREEN() putchar('\30')
/*
 *    ESC STX(ETX)    - Enable(disable) block-fill mode.
 */
#define BLOCK(on)     (putchar('\33'),putchar(2+!(on)))
/*
 *    ESC / 0(1) d    - Set black(white) ink.
 */
#define INK(on)               (putchar('\33'), putchar('\57'), \
                       putchar('\61'-(on)), putchar('\144'))
/*
 *    US              - Switch to `alpha mode'
 */
#define ALPHA_MODE()  putchar('\37')
/*
 *    ESC FF          - clear graphics&alpha screen.
 */
#define ALPHA_CLS();  (putchar('\33'), putchar('\14'))
mf_uniterm_initscreen()
{
      ALPHA_CLS();
      TEXT_SCREEN();
      return 1;
}
mf_uniterm_updatescreen()
{
      DARK();
      VECTOR(0,HEIGHT-1);
      fflush(stdout);
      TEXT_SCREEN();          /*  switch to text mode */
}
mf_uniterm_blankrectangle(left, right, top, bottom)
screencol     left, right;
screenrow     top, bottom;
{
      if (top==0 && left==0 && bottom>=HEIGHT-1 && right>=WIDTH-1) {
              ALPHA_CLS();
              return;
      }
      DARK();
      VECTOR(left, top);
      BLOCK(1);                       /* setup block erase mode */
      INK(0);                         /* use white ink */
      VECTOR(right-1, bottom-1);      /* this draws the block */
      BLOCK(0);                       /* back to (black) linedraw mode */
      INK(1);                         /* black ink */
}
mf_uniterm_paintrow(row, init_color, transition_vector, vector_size)
screenrow             row;
pixelcolor            init_color;
register transspec    transition_vector;
register screencol    vector_size;
{
      register int            blank = !init_color;
#if 0
      /* This is the basic */
      DARK();
      VECTOR(*transition_vector++, row);      /* move to first transition */
      do {
              INK(blank ^= 1);
              VECTOR(*transition_vector++ - 1, row);
      } while (--vector_size > 0);
#endif
      register screencol      col;
      /* However, we optimize the amount of output a bit by blanking
         out the row first (since each INK command takes 4 bytes) */
      DARK();
      if (blank) {
              VECTOR(transition_vector[((vector_size-1)&~1)+1] - 1, row);
              INK(0);
              VECTOR(*transition_vector++, row);
              INK(1);
              if (vector_size==1)
                      return;
      }
      else {
              if (vector_size > 1) {
                      VECTOR(transition_vector[vector_size & ~1] - 1, row);
                      INK(0);
                      VECTOR(transition_vector[1], row);
                      INK(1);
                      DARK();
              }
              VECTOR(*transition_vector++, row);
      }
      do {
              col = *transition_vector++;
              if ((blank ^= 1) == 0)
                      DARK();         /* white -> black; move to first black pixel */
              else
                      col--;          /* black -> white; blacken to col-1 */
              VECTOR(col, row);
      } while (--vector_size > 0);
}
#else
int   tek_dummy;
#endif /* UNITERMWIN */
