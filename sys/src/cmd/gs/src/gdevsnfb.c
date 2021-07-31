/* Copyright (C) 1989, 1990, 1991, 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevsnfb.c,v 1.1 2000/03/09 08:40:41 lpd Exp $*/
/* Sony News frame buffer driver for GhostScript */
#include "gdevprn.h"
#define prn_dev ((gx_device_printer *)dev) /* needed in 5.31 et seq */
/*#include <sys/types.h> problems with ushort! */
typedef	char *	caddr_t;
typedef	long	off_t;
#include <sys/uio.h>
#include <newsiop/framebuf.h>

/* The device descriptor */
private dev_proc_open_device(sonyfb_open);
private dev_proc_output_page(sonyfb_output_page);
private dev_proc_close_device(sonyfb_close);
private gx_device_procs sonyfb_procs =
  prn_procs(sonyfb_open, sonyfb_output_page, sonyfb_close);
gx_device_printer far_data gs_sonyfb_device =
  prn_device(sonyfb_procs, "sonyfb",
	102.4,				/* width_10ths */
	103.2,				/* height_10ths */
	100,				/* x_dpi */
	100,				/* y_dpi */
	0,0,0,0,			/* margins */
	1, 0);

private int fb_file = -1;
sPrimRect prect;

private int
sonyfb_open(gx_device *dev)
{
  sScrType stype;

  if(fb_file < 0)
    if((fb_file = open("/dev/fb", 2)) < 0)
      perror("open failed");
    else
      if(ioctl(fb_file, FBIOCGETSCRTYPE, &stype) < 0)
	perror("ioctl failed");
      else
	prect.rect = stype.visiblerect;

  return gdev_prn_open(dev);
}

private int
sonyfb_close(gx_device *dev)
{
  if(fb_file >= 0)
    {
      close(fb_file);
      fb_file = -1;
    }
  return gdev_prn_close(dev);
}

#define FRAME_WIDTH	1024

/* Send the page to the printer. */
private int
sonyfb_output_page(gx_device *dev, int num_copies, int flush)
{
  int l, i, byte_width, height;
  unsigned char *bm, *fbs, *fb;

  byte_width = (dev->width + 7) / 8;
  height = dev->height;
  bm	 = (typeof(bm))prn_dev->mem.base;

  prect.refPoint.x = 0;
  prect.refPoint.y = 0;
  prect.ptnRect = prect.rect;
  
  prect.ptnBM.type  = BM_MEM;
  prect.ptnBM.depth = 1;
  prect.ptnBM.width = (byte_width + 1) / 2;
  prect.ptnBM.rect.origin.x = 0;
  prect.ptnBM.rect.origin.y = 0;
  prect.ptnBM.rect.extent.x = byte_width * 8; /* width in 16bit words */
  prect.ptnBM.rect.extent.y = height;
  prect.ptnBM.base = (typeof(prect.ptnBM.base))bm;
  
  prect.fore_color = 1;
  prect.aux_color = 0;
  prect.planemask = FB_PLANEALL;
  prect.transp = 0;
  prect.func = BF_S;
  prect.clip = prect.rect;
  prect.drawBM.type  = BM_FB;
  prect.drawBM.depth = 1;
  prect.drawBM.width = (prect.rect.extent.x + 15) / 16;
  prect.drawBM.rect = prect.rect;
  prect.drawBM.base = 0;
  
  if(ioctl(fb_file, FBIOCRECTANGLE, &prect) < 0)
    perror("rect ioctl failed");

  return gx_finish_output_page(dev, num_copies, flush);
}
