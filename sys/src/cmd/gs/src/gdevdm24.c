/* Copyright (C) 1992, 1996, 1997 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

#include "gdevprn.h"


/*$Id: gdevdm24.c,v 1.3 2001/08/01 00:48:23 stefan911 Exp $*/
/* High-res 24Dot-matrix printer driver */

/* Supported printers 
 *  NEC P6 and similar, implemented by Andreas Schwab (schwab@ls5.informatik.uni-dortmund.de)
 *  Epson LQ850, implemented by Christian Felsch (felsch@tu-harburg.d400.de)
 */

/* Driver for NEC P6 */
private dev_proc_print_page (necp6_print_page);
const gx_device_printer far_data gs_necp6_device =
  prn_device (prn_std_procs, "necp6",
	      DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	      360, 360,
	      0, 0, 0.5, 0,	/* margins */
	      1, necp6_print_page);


/* Driver for Epson LQ850 */
/* I've tested this driver on a BJ300 with LQ850 emulation and there it produce correct 360x360dpi output. */
private dev_proc_print_page (lq850_print_page);
const gx_device_printer gs_lq850_device =
  prn_device (prn_std_procs, "lq850",
	      DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	      360, 360,
	      0, 0, 0.5, 0,	/* margins */
	      1, lq850_print_page);

/* ------ Internal routines ------ */

/* Forward references */
private void dot24_output_run (P4 (byte *, int, int, FILE *));
private void dot24_improve_bitmap (P2 (byte *, int));

/* Send the page to the printer. */
private int
dot24_print_page (gx_device_printer *pdev, FILE *prn_stream, char *init_string, int init_len)
{
  int xres = pdev->x_pixels_per_inch;
  int yres = pdev->y_pixels_per_inch;
  int x_high = (xres == 360);
  int y_high = (yres == 360);
  int bits_per_column = (y_high ? 48 : 24);
  uint line_size = gdev_prn_raster (pdev);
  uint in_size = line_size * bits_per_column;
  byte *in = (byte *) gs_malloc (in_size, 1, "dot24_print_page (in)");
  uint out_size = ((pdev->width + 7) & -8) * 3;
  byte *out = (byte *) gs_malloc (out_size, 1, "dot24_print_page (out)");
  int y_passes = (y_high ? 2 : 1);
  int dots_per_space = xres / 10;	/* pica space = 1/10" */
  int bytes_per_space = dots_per_space * 3;
  int skip = 0, lnum = 0, ypass;

  /* Check allocations */
  if (in == 0 || out == 0)
    {
      if (out)
	gs_free ((char *) out, out_size, 1, "dot24_print_page (out)");
      if (in)
	gs_free ((char *) in, in_size, 1, "dot24_print_page (in)");
      return_error (gs_error_VMerror);
    }

  /* Initialize the printer and reset the margins. */
  fwrite (init_string, init_len - 1, sizeof (char), prn_stream);
  fputc ((int) (pdev->width / pdev->x_pixels_per_inch * 10) + 2,
	 prn_stream);

  /* Print lines of graphics */
  while (lnum < pdev->height)
    {
      byte *inp;
      byte *in_end;
      byte *out_end;
      byte *out_blk;
      register byte *outp;
      int lcnt;

      /* Copy 1 scan line and test for all zero. */
      gdev_prn_copy_scan_lines (pdev, lnum, in, line_size);
      if (in[0] == 0
	  && !memcmp ((char *) in, (char *) in + 1, line_size - 1))
	{
	  lnum++;
	  skip += 2 - y_high;
	  continue;
	}

      /* Vertical tab to the appropriate position. */
      while ((skip >> 1) > 255)
	{
	  fputs ("\033J\377", prn_stream);
	  skip -= 255 * 2;
	}

      if (skip)
	{
	  if (skip >> 1)
	    fprintf (prn_stream, "\033J%c", skip >> 1);
	  if (skip & 1)
	    fputc ('\n', prn_stream);
	}

      /* Copy the rest of the scan lines. */
      if (y_high)
	{
	  inp = in + line_size;
	  for (lcnt = 1; lcnt < 24; lcnt++, inp += line_size)
	    if (!gdev_prn_copy_scan_lines (pdev, lnum + lcnt * 2, inp,
					   line_size))
	      {
		memset (inp, 0, (24 - lcnt) * line_size);
		break;
	      }
	  inp = in + line_size * 24;
	  for (lcnt = 0; lcnt < 24; lcnt++, inp += line_size)
	    if (!gdev_prn_copy_scan_lines (pdev, lnum + lcnt * 2 + 1, inp,
					   line_size))
	      {
		memset (inp, 0, (24 - lcnt) * line_size);
		break;
	      }
	}
      else
	{
	  lcnt = 1 + gdev_prn_copy_scan_lines (pdev, lnum + 1, in + line_size,
					       in_size - line_size);
	  if (lcnt < 24)
	    /* Pad with lines of zeros. */
	    memset (in + lcnt * line_size, 0, in_size - lcnt * line_size);
	}

      for (ypass = 0; ypass < y_passes; ypass++)
	{
	  out_end = out;
	  inp = in;
	  if (ypass)
	    inp += line_size * 24;
	  in_end = inp + line_size;

	  for (; inp < in_end; inp++, out_end += 24)
	    {
	      memflip8x8 (inp, line_size, out_end, 3);
	      memflip8x8 (inp + line_size * 8, line_size, out_end + 1, 3);
	      memflip8x8 (inp + line_size * 16, line_size, out_end + 2, 3);
	    }
	  /* Remove trailing 0s. */
	  while (out_end - 3 >= out && out_end[-1] == 0
		 && out_end[-2] == 0 && out_end[-3] == 0)
	    out_end -= 3;

	  for (out_blk = outp = out; outp < out_end;)
	    {
	      /* Skip a run of leading 0s. */
	      /* At least 10 are needed to make tabbing worth it. */

	      if (outp[0] == 0 && outp + 12 <= out_end
		  && outp[1] == 0 && outp[2] == 0
		  && outp[3] == 0 && outp[4] == 0 && outp[5] == 0
		  && outp[6] == 0 && outp[7] == 0 && outp[8] == 0
		  && outp[9] == 0 && outp[10] == 0 && outp[11] == 0)
		{
		  byte *zp = outp;
		  int tpos;
		  byte *newp;
		  outp += 12;
		  while (outp + 3 <= out_end
			 && outp[0] == 0 && outp[1] == 0 && outp[2] == 0)
		    outp += 3;
		  tpos = (outp - out) / bytes_per_space;
		  newp = out + tpos * bytes_per_space;
		  if (newp > zp + 10)
		    {
		      /* Output preceding bit data. */
		      /* only false at beginning of line */
		      if (zp > out_blk)
			{
			  if (x_high)
			    dot24_improve_bitmap (out_blk, (int) (zp - out_blk));
			  dot24_output_run (out_blk, (int) (zp - out_blk),
					  x_high, prn_stream);
			}
		      /* Tab over to the appropriate position. */
		      fprintf (prn_stream, "\033D%c%c\t", tpos, 0);
		      out_blk = outp = newp;
		    }
		}
	      else
		outp += 3;
	    }
	  if (outp > out_blk)
	    {
	      if (x_high)
		dot24_improve_bitmap (out_blk, (int) (outp - out_blk));
	      dot24_output_run (out_blk, (int) (outp - out_blk), x_high,
			      prn_stream);
	    }

	  fputc ('\r', prn_stream);
	  if (ypass < y_passes - 1)
	    fputc ('\n', prn_stream);
	}
      skip = 48 - y_high;
      lnum += bits_per_column;
    }

  /* Eject the page and reinitialize the printer */
  fputs ("\f\033@", prn_stream);
  fflush (prn_stream);

  gs_free ((char *) out, out_size, 1, "dot24_print_page (out)");
  gs_free ((char *) in, in_size, 1, "dot24_print_page (in)");

  return 0;
}

/* Output a single graphics command. */
private void
dot24_output_run (byte *data, int count, int x_high, FILE *prn_stream)
{
  int xcount = count / 3;
  fputc (033, prn_stream);
  fputc ('*', prn_stream);
  fputc ((x_high ? 40 : 39), prn_stream);
  fputc (xcount & 0xff, prn_stream);
  fputc (xcount >> 8, prn_stream);
  fwrite (data, 1, count, prn_stream);
}

/* If xdpi == 360, the P6 / LQ850 cannot print adjacent pixels.  Clear the
   second last pixel of every run of set pixels, so that the last pixel
   is always printed.  */
private void
dot24_improve_bitmap (byte *data, int count)
{
  int i;
  register byte *p = data + 6;

      for (i = 6; i < count; i += 3, p += 3)
	{
	  p[-6] &= ~(~p[0] & p[-3]);
	  p[-5] &= ~(~p[1] & p[-2]);
	  p[-4] &= ~(~p[2] & p[-1]);
	}
      p[-6] &= ~p[-3];
      p[-5] &= ~p[-2];
      p[-4] &= ~p[-1];

}


private int
necp6_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
  char necp6_init_string [] = "\033@\033P\033l\000\r\034\063\001\033Q";
  
  return dot24_print_page(pdev, prn_stream, necp6_init_string, sizeof(necp6_init_string));
}


private int
lq850_print_page(gx_device_printer *pdev, FILE *prn_stream)
{
  char lq850_init_string [] = "\033@\033P\033l\000\r\033\053\001\033Q";
  
  return dot24_print_page(pdev, prn_stream, lq850_init_string, sizeof(lq850_init_string));
}

