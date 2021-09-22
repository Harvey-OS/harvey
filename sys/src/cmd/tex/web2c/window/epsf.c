/* 
 * epsf.c -- Encapsulated PostScript window server.
 * Copyright (C) 1998  Mathias Herberts <herberts@infini.fr>
 *
 * These functions generate an Encapsulated PostScript File
 * representing the graphics normally shown online. They are
 * selected by setting MFTERM to epsf.
 *
 * The name of the file defaults to metafont.eps but can be
 * changed by setting the MFEPSF environment variable.
 *
 * The file is closed when the program exits.
 */

/* 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



#define	EXTERN		extern

#include "../mfd.h"

#ifdef EPSFWIN

#include <stdio.h>

static FILE * psout = NULL;

/*
 * Arrays epsf_{left,right,top,bottom} hold the coordinates of
 * open windows.
 *
 * When a new window is opened, i.e. when blankrectangle is
 * called, we check the arrays to see if the blank rectangle
 * clears an open window. If it is the case a showpage and an %%EOF is
 * appended To the PostScript file and a new header is output thus
 * making it easy to separate the pages.
 *
 * The last page of the file is not terminated by a showpage and
 * an %%EOF, the user should add them himself.
 *
 */

static screencol epsf_left[16];
static screencol epsf_right[16];
static screenrow epsf_top[16];
static screenrow epsf_bottom[16];

static short epsf_window = 0;
static unsigned int epsf_page = 1;

void
mf_epsf_header ()
{
  fprintf (psout, "%%!PS-Adobe-3.0 EPSF-3.0\n");
  fprintf (psout, "%%%%BoundingBox: -1 -1 %d %d\n", screenwidth, screendepth);
  fprintf (psout, "%%%%Creator: METAFONT\n");
  fprintf (psout, "%%%%Page: %d %d\n\n", epsf_page, epsf_page);
  fprintf (psout, "1 dup scale\n");
  fprintf (psout, "1 setlinewidth\n\n");

  epsf_page++;
}

boolean
mf_epsf_initscreen P1H(void)
{
  if (getenv ("MFEPSF") != (char *) NULL)
    {
      psout = fopen ((char *) getenv ("MFEPSF"), "w");
    }
  else
    {
      psout = fopen ("metafont.eps", "w");
    }
   
   if (psout == (FILE *) NULL)
    {
      return 0;
    }
  else
    {
      mf_epsf_header ();
      epsf_window = 0;
      return 1;
    }
}

void
mf_epsf_updatescreen P1H(void)
{
  fflush (psout);
}

void
mf_epsf_blankrectangle P4C(screencol, left,
			  screencol, right,
			  screenrow, top,
			  screenrow, bottom)
{
  int i;

  for (i = 0; i < epsf_window; i++)
    {
      if (! ((right - 1 < epsf_left[i]) || (epsf_right[i] < left)) ) /* new window is neither left nor right of window i */
	if ( ! ((top > epsf_bottom[i]) || (epsf_top[i] > bottom - 1))) /* new window is neither below nor above window i */
	  {	    
	    fprintf (psout, "\nshowpage\n%%%%EOF\n");
	    mf_epsf_header ();
	    epsf_window = 0;
	    break;
	  }
    }
  
  epsf_left[epsf_window] = left;
  epsf_right[epsf_window] = right - 1;
  epsf_top[epsf_window] = top;
  epsf_bottom[epsf_window] = bottom - 1;

  epsf_window++;

  fprintf (psout, "1 setgray %d %d %d %d rectfill 0 setgray\n", left, screendepth - 1 - bottom, right - left, bottom - top);

  fflush (psout);
}


void
mf_epsf_paintrow P4C(screenrow, row,
		    pixelcolor, init_color,
		    transspec, transition_vector,
		    screencol, vector_size)
{
  int col;
  int color;

  color = (init_color == 0) ? 1 : 0;

  do
    {
      col = *transition_vector++;

      if (!color)
	{
	  fprintf (psout, "newpath %d %d moveto %d %d lineto stroke\n", 
		   col, 
		   screendepth - 1 - row,
		   *transition_vector,
		   screendepth - 1 - row);
	}

      color = (color == 0) ? 1 : 0;
    }
  while (--vector_size > 0);
}


#else /* !EPSFWIN */

int epsf_dummy;

#endif /* !EPSFWIN */
