/* Copyright (C) 1995, 1996 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/*$Id: gdevstc3.c,v 1.1 2000/03/09 08:40:41 lpd Exp $*/
/* Epson Stylus-Color Printer-Driver */

/***
     This file holds the sample-implementation of a RGB-algorithm for
     the stcolor-driver. It is available via

          gs -sDEVICE=stcolor -sDithering=gsrgb ...

     Actually this is no dithering-algorithm, it lets ghostscript do the job.
     This achieved, by requesting BYTE-Values between 0 and 1 to be delivered,
     which causes a depth of 1-Bit by default.

 ***/

/*
 * gdevstc.h holds all the includes and the driver-specific definitions, so
 * it is the only include you need. To add a new algorthim, STC_MODI in
 * gdevstc.h should be extended. (see the instructions there)
 */

#include "gdevstc.h"

/*
 * the routine required.
 */

/*ARGSUSED*/
int 
stc_gsrgb(stcolor_device *sdev,int npixel,byte *ip,byte *buf,byte *out)
{

   int   error = 0;

/*
 * There are basically 3 Types of calls:
 * npixel < 0    => initialize buf, if this is required
 *                  (happens only if requested)
 * npixel > 0    => process next scanline, if the flag STC_WHITE is set, then
 *                  in == NULL signals, that the basic-driver has decided
 *                  that this scanline is white. (Useful for really dithering
 *                  drivers)
 */

/* ============================================================= */
   if(npixel > 0) {  /* npixel >  0 -> scanline-processing       */
/* ============================================================= */

      int p;

/*
 *    simply merge the color-values into a single byte 
 *    (RED, GREEN, BLUE are defined in gdevstc.h)
 */
      for(p = 0; p < npixel; ++p,++out) { /* loop over pixels */

         *out = 0;
         if(*ip++) *out |= RED;
         if(*ip++) *out |= GREEN;
         if(*ip++) *out |= BLUE;

      }                                   /* loop over pixels */

/* ============================================================= */
   } else {          /* npixel <= 0 -> initialisation            */
/* ============================================================= */
/*
 * besides buffer-Initialisation, one may check the parameters in
 * the algorithm-table of the driver.
 */

/*    we didn't check for the white-calls above, so they would cause errors */
      if(sdev->stc.dither->flags & STC_WHITE)              error = -1;

/*    if we're not setup for bytes, this is an error too */
      if((sdev->stc.dither->flags & STC_TYPE) != STC_BYTE) error = -2;

/*    and rgb-mode is the only supported mode */
      if(sdev->color_info.num_components != 3)             error = -3;

/*    we can't deal with ghostscript-data directly. */
      if(sdev->stc.dither->flags & STC_DIRECT)             error = -4;

/* ============================================================= */
   } /* scanline-processing or initialisation */
/* ============================================================= */

   return error;
}
