/* Copyright (C) 1990, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevprn.c */
/* Generic printer driver support */
#include "gdevprn.h"
#include "gp.h"
#include "gsparam.h"

/* Define the scratch file name prefix for mktemp */
#define SCRATCH_TEMPLATE gp_scratch_file_name_prefix

/* Internal routine for opening a scratch file */
private int
open_scratch(char *fname, FILE **pfile)
{	char fmode[4];
	strcpy(fmode, "w+");
	strcat(fmode, gp_fmode_binary_suffix);
	*pfile = gp_open_scratch_file(SCRATCH_TEMPLATE, fname, fmode);
	if ( *pfile == NULL )
	   {	eprintf1("Could not open the scratch file %s.\n", fname);
		return_error(gs_error_invalidfileaccess);
	   }
	return 0;
}

/* ------ Standard device procedures ------ */

/* Macros for casting the pdev argument */
#define ppdev ((gx_device_printer *)pdev)
#define pmemdev ((gx_device_memory *)pdev)
#define pcldev ((gx_device_clist *)pdev)

/* Define the old-style printer procedure vector. */
gx_device_procs prn_std_procs =
  prn_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close);

/* Initialize a printer device. */
int
gdev_prn_initialize(register gx_device *pdev, const char _ds *dname,
  dev_proc_print_page_t print_page)
{	int code = gdev_initialize(pdev);
	pdev->dname = dname;
	set_dev_proc(pdev, open_device, gdev_prn_open);
	set_dev_proc(pdev, output_page, gdev_prn_output_page);
	set_dev_proc(pdev, close_device, gdev_prn_close);
	set_dev_proc(pdev, map_rgb_color, gdev_prn_map_rgb_color);
	set_dev_proc(pdev, map_color_rgb, gdev_prn_map_color_rgb);
	set_dev_proc(pdev, get_params, gdev_prn_get_params);
	set_dev_proc(pdev, put_params, gdev_prn_put_params);
	ppdev->print_page = print_page;
	ppdev->max_bitmap = PRN_MAX_BITMAP;
	ppdev->use_buffer_space = PRN_BUFFER_SPACE;
	ppdev->fname[0] = 0;
	ppdev->file = 0;
	ppdev->file_is_new = false;
	ppdev->ccfname[0] = 0;
	ppdev->ccfile = 0;
	ppdev->cbfname[0] = 0;
	ppdev->cbfile = 0;
	ppdev->buffer_space = 0;
	ppdev->buf = 0;
	return code;
}

/* Further initialize a printer device for color. */
void
gdev_prn_init_color(gx_device *pdev, int depth,
  dev_proc_map_rgb_color_t map_rgb_color,
  dev_proc_map_color_rgb_t map_color_rgb)
{	pdev->color_info.num_components = (depth > 1 ? 3 : 1);
	pdev->color_info.depth = (depth > 1 && depth < 8 ? 8 : depth);
	pdev->color_info.max_gray = (depth >= 8 ? 255 : 1);
	pdev->color_info.max_color = (depth >= 8 ? 255 : depth > 1 ? 1 : 0);
	pdev->color_info.dither_grays = (depth >= 8 ? 5 : 2);
	pdev->color_info.dither_colors = (depth >= 8 ? 5 : depth > 1 ? 2 : 0);
	set_dev_proc(pdev, map_rgb_color, map_rgb_color);
	set_dev_proc(pdev, map_color_rgb, map_color_rgb);
}

/* Open a generic printer device. */
/* Specific devices may wish to extend this. */
int
gdev_prn_open(gx_device *pdev)
{	const gx_device_memory *mdev =
	  gdev_mem_device_for_bits(pdev->color_info.depth);
	ulong mem_space;
	byte *base = 0;
	char *left = 0;
	int code = 0;

	if ( mdev == 0 )
	  return_error(gs_error_rangecheck);
	memset(ppdev->skip, 0, sizeof(ppdev->skip));
	ppdev->orig_procs = pdev->std_procs;
	ppdev->file = ppdev->ccfile = ppdev->cbfile = NULL;
	mem_space = gdev_mem_bitmap_size(pmemdev);
	if ( mem_space >= ppdev->max_bitmap ||
	     mem_space != (uint)mem_space ||	/* too big to allocate */
	     (base = (byte *)gs_malloc((uint)mem_space, 1, "printer buffer(open)")) == 0 ||	/* can't allocate */
	     (PRN_MIN_MEMORY_LEFT != 0 &&
	      (left = gs_malloc(PRN_MIN_MEMORY_LEFT, 1, "printer memory left")) == 0)	/* not enough left */
	   )
	{	/* Buffer the image in a command list. */
		uint space;
		int code;
		/* Release the buffer if we allocated it. */
		if ( base != 0 )
		  gs_free((char *)base, (uint)mem_space, 1,
			  "printer buffer(open)");
		for ( space = ppdev->use_buffer_space; ; )
		{	base = (byte *)gs_malloc(space, 1,
						 "command list buffer(open)");
			if ( base != 0 ) break;
			if ( (space >>= 1) < PRN_MIN_BUFFER_SPACE )
			  return_error(gs_error_VMerror);	/* no hope */
		}
open_c:		pcldev->data = base;
		pcldev->data_size = space;
		pcldev->target = pdev;
		gs_make_mem_device(&pcldev->mdev, mdev, 0, 0, pdev);
		ppdev->buf = base;
		ppdev->buffer_space = space;
		/* Try opening the command list, to see if we allocated */
		/* enough buffer space. */
		code = (*gs_clist_device_procs.open_device)((gx_device *)pcldev);
		if ( code < 0 )
		{	/* If there wasn't enough room, and we haven't */
			/* already shrunk the buffer, try enlarging it. */
			if ( code == gs_error_limitcheck &&
			     space >= ppdev->use_buffer_space
			   )
			{	gs_free((char *)base, space, 1,
					"command list buffer(retry open)");
				space <<= 1;
				base = (byte *)gs_malloc(space, 1,
					 "command list buffer(retry open)");
				ppdev->buf = base;
				if ( base != 0 )
				  goto open_c;
			}
			/* Fall through with code < 0 */
		}
		if ( code < 0 ||
		     (code = open_scratch(ppdev->ccfname, &ppdev->ccfile)) < 0 ||
		     (code = open_scratch(ppdev->cbfname, &ppdev->cbfile)) < 0
		   )
		{	/* Clean up before exiting */
			gdev_prn_close(pdev);
			return code;
		}
		pcldev->cfile = ppdev->ccfile;
		pcldev->bfile = ppdev->cbfile;
		pcldev->bfile_end_pos = 0;
		ppdev->std_procs = gs_clist_device_procs;
	}
	else
	{	/* Render entirely in memory. */
		/* Release the leftover memory. */
		gs_free(left, PRN_MIN_MEMORY_LEFT, 1,
			"printer memory left");
		ppdev->buffer_space = 0;
		pmemdev->base = base;
		ppdev->std_procs = mdev->std_procs;
		ppdev->std_procs.get_page_device =
		  gx_page_device_get_page_device;
	}
	/* Synthesize the procedure vector. */
	/* Rendering operations come from the memory or clist device, */
	/* non-rendering come from the printer device. */
#define copy_proc(p) set_dev_proc(ppdev, p, ppdev->orig_procs.p)
	copy_proc(get_initial_matrix);
	copy_proc(output_page);
	copy_proc(close_device);
	copy_proc(map_rgb_color);
	copy_proc(map_color_rgb);
	copy_proc(get_params);
	copy_proc(put_params);
	copy_proc(map_cmyk_color);
	copy_proc(get_xfont_procs);
	copy_proc(get_xfont_device);
	copy_proc(map_rgb_alpha_color);
	copy_proc(get_page_device);
#undef copy_proc
	code = (*dev_proc(pdev, open_device))(pdev);
	if ( code < 0 )
	  return code;
	if ( ppdev->OpenOutputFile )
	  code = gdev_prn_open_printer(pdev, 1);
	return code;

}

/* Get parameters.  Printer devices add several more parameters */
/* to the default set. */
int
gdev_prn_get_params(gx_device *pdev, gs_param_list *plist)
{	int code = gx_default_get_params(pdev, plist);
	gs_param_string ofns;
	if ( code < 0 ) return code;
	code = (ppdev->NumCopies_set ?
		param_write_int(plist, "NumCopies", &ppdev->NumCopies) :
		param_write_null(plist, "NumCopies"));
	if ( code < 0 ) return code;
	code = param_write_bool(plist, "OpenOutputFile", &ppdev->OpenOutputFile);
	if ( code < 0 ) return code;
	code = param_write_long(plist, "BufferSpace", &ppdev->use_buffer_space);
	if ( code < 0 ) return code;
	code = param_write_long(plist, "MaxBitmap", &ppdev->max_bitmap);
	if ( code < 0 ) return code;
	ofns.data = (const byte *)ppdev->fname,
	  ofns.size = strlen(ppdev->fname),
	  ofns.persistent = false;
	return param_write_string(plist, "OutputFile", &ofns);
}

/* Put parameters. */
/* Note that setting the buffer sizes closes the device. */
int
gdev_prn_put_params(gx_device *pdev, gs_param_list *plist)
{	int ecode = 0;
	int code;
	const char _ds *param_name;
	int nci = ppdev->NumCopies;
	bool oof = ppdev->OpenOutputFile;
	bool ncnull = false;
	long bsl = ppdev->use_buffer_space;
	long mbl = ppdev->max_bitmap;
	gs_param_string ofs;

	switch ( code = param_read_int(plist, (param_name = "NumCopies"), &nci) )
	{
	case 0:
		if ( nci < 0 )
		  ecode = gs_error_rangecheck;
		else
		  break;
		goto nce;
	default:
		if ( (code = param_read_null(plist, param_name)) == 0 )
		  {	ncnull = true;
			break;
		  }
		ecode = code;	/* can't be 1 */
nce:		param_signal_error(plist, param_name, ecode);
	case 1:
		break;
	}

	switch ( code = param_read_bool(plist, (param_name = "OpenOutputFile"), &oof) )
	{
	default:
		ecode = code;
		param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
		break;
	}

	switch ( code = param_read_long(plist, (param_name = "BufferSpace"), &bsl) )
	{
	case 0:
		if ( bsl < 10000 )
		  ecode = gs_error_rangecheck;
		else
		  break;
		goto bse;
	default:
		ecode = code;
bse:		param_signal_error(plist, param_name, ecode);
	case 1:
		break;
	}

	switch ( code = param_read_long(plist, (param_name = "MaxBitmap"), &mbl) )
	{
	case 0:
		if ( mbl < 10000 )
		  ecode = gs_error_rangecheck;
		else
		  break;
		goto mbe;
	default:
		ecode = code;
mbe:		param_signal_error(plist, param_name, ecode);
	case 1:
		break;
	}

	switch ( code = param_read_string(plist, (param_name = "OutputFile"), &ofs) )
	{
	case 0:
		if ( ofs.size >= prn_fname_sizeof )
		  ecode = gs_error_limitcheck;
		else
		  break;
		goto ofe;
	default:
		ecode = code;
ofe:		param_signal_error(plist, param_name, ecode);
	case 1:
		ofs.data = 0;
		break;
	}

	if ( ecode < 0 )
	  return ecode;
	code = gx_default_put_params(pdev, plist);
	if ( code < 0 )
	  return code;

	if ( nci != ppdev->NumCopies )
	  {	ppdev->NumCopies = nci;
		ppdev->NumCopies_set = true;
	  }
	else if ( ncnull )
	  ppdev->NumCopies_set = false;
	ppdev->OpenOutputFile = oof;
	if ( bsl != ppdev->use_buffer_space )
	  {	if ( pdev->is_open )
		  gs_closedevice(pdev);
		ppdev->use_buffer_space = bsl;
	  }
	if ( mbl != ppdev->max_bitmap )
	  {	if ( pdev->is_open )
		  gs_closedevice(pdev);
		ppdev->max_bitmap = mbl;
	  }
	if ( ofs.data != 0 )
	  {	/* Close the file if it's open. */
		if ( ppdev->file != NULL && ppdev->file != stdout )
		  gp_close_printer(ppdev->file, ppdev->fname);
		ppdev->file = NULL;
		memcpy(ppdev->fname, ofs.data, ofs.size);
		ppdev->fname[ofs.size] = 0;
	  }

	/* If the device is open and OpenOutputFile is true, */
	/* open the OutputFile now.  (If the device isn't open, */
	/* this will happen when it is opened.) */
	if ( pdev->is_open && oof )
	  {	code = gdev_prn_open_printer(pdev, 1);
		if ( code < 0 )
		  return code;
	  }

	return 0;
}

/* Generic routine to send the page to the printer. */
int
gdev_prn_output_page(gx_device *pdev, int num_copies, int flush)
{	int code, outcode, closecode, errcode;

	code = gdev_prn_open_printer(pdev, 1);
	if ( code < 0 )
	  return code;

	/* Print the accumulated page description. */
	outcode = (*ppdev->print_page_copies)(ppdev, ppdev->file, num_copies);
	errcode = (ferror(ppdev->file) ? gs_error_ioerror : 0);
	closecode = gdev_prn_close_printer(pdev);

	if ( ppdev->buffer_space ) /* reinitialize clist for writing */
	  code = (*gs_clist_device_procs.output_page)(pdev, num_copies, flush);

	if ( outcode < 0 )
	  return outcode;
	if ( errcode < 0 )
	  return_error(errcode);
	if ( closecode < 0 )
	  return closecode;
	return code;
}

/* Print multiple copies of a page by calling print_page multiple times. */
int
gx_default_print_page_copies(gx_device_printer *pdev, FILE *prn_stream,
  int num_copies)
{	int i = num_copies;
	int code = 0;
	while ( code >= 0 && i-- > 0 )
	  code = (*pdev->print_page)(pdev, prn_stream);
	return code;
}

/* Generic closing for the printer device. */
/* Specific devices may wish to extend this. */
int
gdev_prn_close(gx_device *pdev)
{	if ( ppdev->ccfile != NULL )
	{	fclose(ppdev->ccfile);
		ppdev->ccfile = NULL;
		unlink(ppdev->ccfname);
	}
	if ( ppdev->cbfile != NULL )
	{	fclose(ppdev->cbfile);
		ppdev->cbfile = NULL;
		unlink(ppdev->cbfname);
	}
	if ( ppdev->buffer_space != 0 )
	{	/* Free the buffer */
		gs_free((char *)ppdev->buf, (uint)ppdev->buffer_space, 1,
			"command list buffer(close)");
	}
	else
	{	/* Free the memory device bitmap */
		gs_free((char *)pmemdev->base,
			(uint)gdev_mem_bitmap_size(pmemdev),
			1, "printer buffer(close)");
	}
	if ( ppdev->file != NULL )
	{	if ( ppdev->file != stdout )
		   gp_close_printer(ppdev->file, ppdev->fname);
		ppdev->file = NULL;
	}
	pdev->std_procs = ppdev->orig_procs;
	return 0;
}

/* Map colors.  This is different from the default, because */
/* printers write black, not white. */

gx_color_index
gdev_prn_map_rgb_color(gx_device *dev, gx_color_value r, gx_color_value g,
  gx_color_value b)
{	/* Map values >= 1/2 to 0, < 1/2 to 1. */
	return ((r | g | b) > gx_max_color_value / 2 ?
		(gx_color_index)0 : (gx_color_index)1);
}

int
gdev_prn_map_color_rgb(gx_device *dev, gx_color_index color,
  gx_color_value prgb[3])
{	/* Map 0 to max_value, 1 to 0. */
	prgb[0] = prgb[1] = prgb[2] = -((gx_color_value)color ^ 1);
	return 0;
}

/* ------ Driver services ------ */

/* Open the current page for printing. */
int
gdev_prn_open_printer(gx_device *pdev, int binary_mode)
{	char *fname = ppdev->fname;
	char pfname[prn_fname_sizeof + 10];
	char *pnpos = strchr(fname, '%');
	if ( pnpos != NULL )
	{	long count1 = ppdev->PageCount + 1;
		sprintf(pfname, fname,
			(pnpos[1] == 'l' ? count1 : (int)count1));
		fname = pfname;
	}
	if ( ppdev->file == NULL )
	{	if ( !strcmp(fname, "-") )
			ppdev->file = stdout;
		else
		{	ppdev->file = gp_open_printer(fname, binary_mode);
			if ( ppdev->file == NULL )
				return_error(gs_error_invalidfileaccess);
		}
		ppdev->file_is_new = true;
	}
	else
		ppdev->file_is_new = false;
	return 0;
}

/* Copy a scan line from the buffer to the printer. */
int
gdev_prn_get_bits(gx_device_printer *pdev, int y, byte *str, byte **actual_data)
{	int code = (*dev_proc(pdev, get_bits))((gx_device *)pdev, y, str, actual_data);
	uint line_size = gdev_prn_raster(pdev);
	int last_bits = -(pdev->width * pdev->color_info.depth) & 7;
	if ( code < 0 ) return code;
	if ( last_bits != 0 )
	{	byte *dest = (actual_data != 0 ? *actual_data : str);
		dest[line_size - 1] &= 0xff << last_bits;
	}
	return 0;
}
/* Copy scan lines to a buffer.  Return the number of scan lines, */
/* or <0 if error. */
int
gdev_prn_copy_scan_lines(gx_device_printer *pdev, int y, byte *str, uint size)
{	uint line_size = gdev_prn_raster(pdev);
	int count = size / line_size;
	int i;
	byte *dest = str;
	count = min(count, pdev->height - y);
	for ( i = 0; i < count; i++, dest += line_size )
	{	int code = gdev_prn_get_bits(pdev, y + i, dest, NULL);
		if ( code < 0 ) return code;
	}
	return count;
}

/* Close the current page. */
int
gdev_prn_close_printer(gx_device *pdev)
{	if ( strchr(ppdev->fname, '%') )	/* file per page */
	   {	gp_close_printer(ppdev->file, ppdev->fname);
		ppdev->file = NULL;
	   }
	return 0;
}
