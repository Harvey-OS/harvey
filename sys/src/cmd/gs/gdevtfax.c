/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevtfax.c */
/* Plain-bits or TIFF/F fax device. */
#include "gdevprn.h"
/*#include "gdevtifs.h"*/			/* see TIFF section below */
#include "strimpl.h"
#include "scfx.h"

/* Define the device parameters. */
#define X_DPI 204
#define Y_DPI 196
#define LINE_SIZE ((X_DPI * 101 / 10 + 7) / 8)	/* max bytes per line */

/* The device descriptors */

dev_proc_open_device(gdev_fax_open);
private dev_proc_print_page(faxg3_print_page);
private dev_proc_print_page(faxg32d_print_page);
private dev_proc_print_page(faxg4_print_page);
private dev_proc_print_page(tiffg3_print_page);
private dev_proc_print_page(tiffg32d_print_page);
private dev_proc_print_page(tiffg4_print_page);

struct gx_device_tfax_s {
	gx_device_common;
	gx_prn_device_common;
		/* The following items are only for TIFF output. */
	long prev_dir;		/* file offset of previous directory offset */
	long dir_off;		/* file offset of next write */
};
typedef struct gx_device_tfax_s gx_device_tfax;

#define tfdev ((gx_device_tfax *)dev)

/* Define procedures that adjust the paper size. */
private gx_device_procs gdev_fax_std_procs =
  prn_procs(gdev_fax_open, gdev_prn_output_page, gdev_prn_close);

gx_device_tfax far_data gs_faxg3_device =
{   prn_device_std_body(gx_device_tfax, gdev_fax_std_procs, "faxg3",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, faxg3_print_page)
};

gx_device_tfax far_data gs_faxg32d_device =
{   prn_device_std_body(gx_device_tfax, gdev_fax_std_procs, "faxg32d",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, faxg32d_print_page)
};

gx_device_tfax far_data gs_faxg4_device =
{   prn_device_std_body(gx_device_tfax, gdev_fax_std_procs, "faxg4",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, faxg4_print_page)
};

gx_device_tfax far_data gs_tiffg3_device =
{   prn_device_std_body(gx_device_tfax, gdev_fax_std_procs, "tiffg3",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, tiffg3_print_page)
};

gx_device_tfax far_data gs_tiffg32d_device =
{   prn_device_std_body(gx_device_tfax, gdev_fax_std_procs, "tiffg32d",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, tiffg32d_print_page)
};

gx_device_tfax far_data gs_tiffg4_device =
{   prn_device_std_body(gx_device_tfax, gdev_fax_std_procs, "tiffg4",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,			/* margins */
	1, tiffg4_print_page)
};

/* Open the device, adjusting the paper size. */
int
gdev_fax_open(gx_device *dev)
{	if ( dev->width >= 1680 && dev->width <= 1736 )
	  {	/* Adjust width for A4 paper. */
		dev->width = 1728;
	  }
	else if ( dev->width >= 2000 && dev->width <= 2056 )
	  {	/* Adjust width for B4 paper. */
		dev->width = 2048;
	  }
	return gdev_prn_open(dev);
}

/* Send the page to the printer. */
int
gdev_fax_print_page(gx_device_printer *pdev, FILE *prn_stream,
  stream_CFE_state *ss)
{	gs_memory_t *mem = &gs_memory_default;
	const stream_template _ds *temp = &s_CFE_template;
	int code;
	stream_cursor_read r;
	stream_cursor_write w;
	int in_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	int lnum;
	byte *in;
	byte *out;
	/* If the file is 'nul', don't even do the writes. */
	bool nul = !strcmp(pdev->fname, "nul");

	/* Initialize the common part of the encoder state. */
	ss->template = temp;
	ss->memory = mem;
	/* Initialize the rest of the encoder state. */
	ss->Uncompressed = false;
	ss->Columns = pdev->width;
	ss->Rows = pdev->height;
	ss->BlackIs1 = true;
	/* Now initialize the encoder. */
	code = (*temp->init)((stream_state *)ss);
	if ( code < 0 )
	  return code;

	/* Allocate the buffers. */
	in = gs_alloc_bytes(mem, temp->min_in_size + in_size + 1, "gdev_fax_print_page(in)");
#define out_size 1000
	out = gs_alloc_bytes(mem, out_size, "gdev_fax_print_page(out)");
	if ( in == 0 || out == 0 )
	{	code = gs_note_error(gs_error_VMerror);
		goto done;
	}

	/* Set up the processing loop. */
	lnum = 0;
	r.ptr = r.limit = in - 1;
	w.ptr = out - 1;
	w.limit = w.ptr + out_size;

	/* Process the image. */
	for ( ; ; )
	{	int status;
		if_debug7('w', "[w]bitcfe: lnum=%d r=0x%lx,0x%lx,0x%lx w=0x%lx,0x%lx,0x%lx\n", lnum,
			  (ulong)in, (ulong)r.ptr, (ulong)r.limit,
			  (ulong)out, (ulong)w.ptr, (ulong)w.limit);
		status = (*temp->process)((stream_state *)ss,
					  &r, &w, lnum == pdev->height);
		if_debug7('w', "...%d, r=0x%lx,0x%lx,0x%lx w=0x%lx,0x%lx,0x%lx\n", status,
			  (ulong)in, (ulong)r.ptr, (ulong)r.limit,
			  (ulong)out, (ulong)w.ptr, (ulong)w.limit);
		switch ( status )
		{
		case 0:			/* need more input data */
		{	uint left;
			if ( lnum == pdev->height )
			  goto ok;
			left = r.limit - r.ptr;
			memcpy(in, r.ptr + 1, left);
			gdev_prn_copy_scan_lines(pdev, lnum++, in + left, in_size);
			r.limit = in + left + in_size - 1;
			r.ptr = in - 1;
		}	break;
		case 1:			/* need to write output */
			if ( !nul )
			  fwrite(out, 1, w.ptr + 1 - out, prn_stream);
			w.ptr = out - 1;
			break;
		}
	}

ok:
	/* Write out any remaining output. */
	if ( !nul )
	  fwrite(out, 1, w.ptr + 1 - out, prn_stream);

done:
	gs_free_object(mem, out, "gdev_fax_print_page(out)");
	gs_free_object(mem, in, "gdev_fax_print_page(in)");
	return code;
}

/* Print a 1-D Group 3 page. */
private int
faxg3_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	stream_CFE_state state;
	state.K = 0;
	state.EndOfLine = true;
	state.EncodedByteAlign = false;
	state.EndOfBlock = false;
	state.FirstBitLowOrder = false;
	return gdev_fax_print_page(pdev, prn_stream, &state);
}

/* Print a 2-D Group 3 page. */
private int
faxg32d_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	stream_CFE_state state;
	state.K = (pdev->y_pixels_per_inch < 100 ? 2 : 4);
	state.EndOfLine = true;
	state.EncodedByteAlign = false;
	state.EndOfBlock = false;
	state.FirstBitLowOrder = false;
	return gdev_fax_print_page(pdev, prn_stream, &state);
}

/* Print a Group 4 page. */
private int
faxg4_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	stream_CFE_state state;
	state.K = -1;
	state.EndOfLine = false;
	state.EncodedByteAlign = false;
	state.EndOfBlock = false;
	state.FirstBitLowOrder = false;
	return gdev_fax_print_page(pdev, prn_stream, &state);
}

/* ---------------- TIFF/F output ---------------- */

#include "time_.h"
#include "gscdefs.h"
#include "gdevtifs.h"

/* Define the TIFF directory we use. */
/* NB: this array is sorted by tag number (assumed below) */
typedef struct TIFF_directory_s {
	TIFF_dir_entry	SubFileType;
	TIFF_dir_entry	ImageWidth;
	TIFF_dir_entry	ImageLength;
	TIFF_dir_entry	BitsPerSample;
	TIFF_dir_entry	Compression;
	TIFF_dir_entry	Photometric;
	TIFF_dir_entry	FillOrder;
	TIFF_dir_entry	StripOffsets;
	TIFF_dir_entry	Orientation;
	TIFF_dir_entry	SamplesPerPixel;
	TIFF_dir_entry	RowsPerStrip;
	TIFF_dir_entry	StripByteCounts;
	TIFF_dir_entry	XResolution;
	TIFF_dir_entry	YResolution;
	TIFF_dir_entry	PlanarConfig;
	TIFF_dir_entry	T4T6Options;
	TIFF_dir_entry	ResolutionUnit;
	TIFF_dir_entry	PageNumber;
	TIFF_dir_entry	Software;
	TIFF_dir_entry	DateTime;
	TIFF_dir_entry	CleanFaxData;
	TIFF_ulong	diroff;			/* offset to next directory */
	TIFF_ulong	xresValue[2];		/* XResolution indirect value */
	TIFF_ulong	yresValue[2];		/* YResolution indirect value */
#define maxSoftware 40
	char		softwareValue[maxSoftware]; /* Software indirect value */
	char		dateTimeValue[20];	/* DateTime indirect value */
} TIFF_directory;
private const TIFF_directory dirTemplate = {
	{ TIFFTAG_SubFileType,	TIFF_LONG,  1, SubFileType_page },
	{ TIFFTAG_ImageWidth,	TIFF_LONG,  1 },
	{ TIFFTAG_ImageLength,	TIFF_LONG,  1 },
	{ TIFFTAG_BitsPerSample,	TIFF_SHORT, 1, 1 },
	{ TIFFTAG_Compression,	TIFF_SHORT, 1, Compression_CCITT_T4 },
	{ TIFFTAG_Photometric,	TIFF_SHORT, 1, Photometric_min_is_white },
	{ TIFFTAG_FillOrder,	TIFF_SHORT, 1, FillOrder_LSB2MSB },
	{ TIFFTAG_StripOffsets,	TIFF_LONG,  1 },
	{ TIFFTAG_Orientation,	TIFF_SHORT, 1, Orientation_top_left },
	{ TIFFTAG_SamplesPerPixel,	TIFF_SHORT, 1, 1 },
	{ TIFFTAG_RowsPerStrip,	TIFF_LONG,  1 },
	{ TIFFTAG_StripByteCounts,	TIFF_LONG,  1 },
	{ TIFFTAG_XResolution,	TIFF_RATIONAL, 1 },
	{ TIFFTAG_YResolution,	TIFF_RATIONAL, 1 },
	{ TIFFTAG_PlanarConfig,	TIFF_SHORT, 1, PlanarConfig_contig },
	{ TIFFTAG_T4Options,	TIFF_LONG,  1, 0 },
	{ TIFFTAG_ResolutionUnit,	TIFF_SHORT, 1, ResolutionUnit_inch },
	{ TIFFTAG_PageNumber,	TIFF_SHORT, 2 },
	{ TIFFTAG_Software,	TIFF_ASCII, 0 },
	{ TIFFTAG_DateTime,	TIFF_ASCII, 20 },
	{ TIFFTAG_CleanFaxData,	TIFF_SHORT, 1, CleanFaxData_clean },
	0, { 0, 1 }, { 0, 1 }, { 0 }, { 0, 0 }
};
#define	NTAGS (offset_of(TIFF_directory, diroff) / sizeof(TIFF_dir_entry))

/* Forward references */
private int tiff_begin_page(P3(gx_device_tfax *, FILE *, TIFF_directory *));
private int tiff_end_page(P2(gx_device_tfax *, FILE *));

/* Print the page. */
private int
tifff_print_page(gx_device_printer *dev, FILE *prn_stream,
  stream_CFE_state *pstate, TIFF_directory *pdir)
{	int code;
	tiff_begin_page(tfdev, prn_stream, pdir);
	pstate->FirstBitLowOrder = true;	/* decoders prefer this */
	code = gdev_fax_print_page(dev, prn_stream, pstate);
	tiff_end_page(tfdev, prn_stream);
	return code;
}
private int
tiffg3_print_page(gx_device_printer *dev, FILE *prn_stream)
{	stream_CFE_state state;
	TIFF_directory dir;
	state.K = 0;
	state.EndOfLine = true;
	state.EncodedByteAlign = true;
	state.EndOfBlock = true;
	dir = dirTemplate;
	dir.Compression.value = Compression_CCITT_T4;
	dir.T4T6Options.tag = TIFFTAG_T4Options;
	dir.T4T6Options.value = T4Options_fill_bits;
	return tifff_print_page(dev, prn_stream, &state, &dir);
}
private int
tiffg32d_print_page(gx_device_printer *dev, FILE *prn_stream)
{	stream_CFE_state state;
	TIFF_directory dir;
	state.K = (dev->y_pixels_per_inch < 100 ? 2 : 4);
	state.EndOfLine = true;
	state.EncodedByteAlign = true;
	state.EndOfBlock = true;
	dir = dirTemplate;
	dir.Compression.value = Compression_CCITT_T4;
	dir.T4T6Options.tag = TIFFTAG_T4Options;
	dir.T4T6Options.value = T4Options_2D_encoding | T4Options_fill_bits;
	return tifff_print_page(dev, prn_stream, &state, &dir);
}
private int
tiffg4_print_page(gx_device_printer *dev, FILE *prn_stream)
{	stream_CFE_state state;
	TIFF_directory dir;
	state.K = -1;
	state.EndOfLine = false;
	state.EncodedByteAlign = false;  /* no fill_bits option for T6 */
	state.EndOfBlock = true;
	dir = dirTemplate;
	dir.Compression.value = Compression_CCITT_T6;
	dir.T4T6Options.tag = TIFFTAG_T6Options;
	return tifff_print_page(dev, prn_stream, &state, &dir);
}

#undef tfdev

/* Fix up tag values on big-endian machines if necessary. */
private void
tiff_fixuptags(TIFF_dir_entry *dp, int n)
{
#if arch_is_big_endian
	for ( ; n-- > 0; dp++ )
	  switch ( dp->type )
	    {
	    case TIFF_SHORT: case TIFF_SSHORT:
	      /* We may have two shorts packed into a TIFF_ulong. */
	      dp->value = (dp->value << 16) + (dp->value >> 16); break;
	    case TIFF_BYTE: case TIFF_SBYTE:
	      dp->value <<= 24; break;
	    }
#endif
}

/* Begin a TIFF/F page. */
private int
tiff_begin_page(gx_device_tfax *tfdev, FILE *fp, TIFF_directory *pdir)
{	if (gdev_prn_file_is_new((gx_device_printer *)tfdev))
	  {	/* This is a new file; write the TIFF header. */
		static const TIFF_header hdr =
		  {
#if arch_is_big_endian
		    TIFF_magic_big_endian,
#else
		    TIFF_magic_little_endian,
#endif
		    TIFF_version_value,
		    sizeof(TIFF_header)
		  };
		fwrite((char *)&hdr, sizeof(hdr), 1, fp);
		tfdev->prev_dir = 0;
	  }
	else
	  {	/* Patch pointer to this directory from previous. */
		TIFF_ulong offset = (TIFF_ulong)tfdev->dir_off;
		fseek(fp, tfdev->prev_dir, SEEK_SET);
		fwrite((char *)&offset, sizeof(offset), 1, fp);
		fseek(fp, tfdev->dir_off, SEEK_SET);
	  }

	/* Write count of tags in directory. */
	{	TIFF_short dircount = NTAGS;
		fwrite((char *)&dircount, sizeof(dircount), 1, fp);
	}
	tfdev->dir_off = ftell(fp);

	/* Fill in directory tags and write the directory. */
	pdir->ImageWidth.value = tfdev->width;
	pdir->ImageLength.value = tfdev->height;
	pdir->StripOffsets.value = tfdev->dir_off + sizeof(TIFF_directory);
	pdir->RowsPerStrip.value = tfdev->height;
	pdir->PageNumber.value = (TIFF_ulong)tfdev->PageCount << 16;
#define set_offset(tag, varray)\
  pdir->tag.value =\
    tfdev->dir_off + offset_of(TIFF_directory, varray[0])
	set_offset(XResolution, xresValue);
	set_offset(YResolution, yresValue);
	set_offset(Software, softwareValue);
	set_offset(DateTime, dateTimeValue);
#undef set_offset
	pdir->xresValue[0] = tfdev->x_pixels_per_inch;
	pdir->yresValue[0] = tfdev->y_pixels_per_inch;
	{	char revs[10];
		strncpy(pdir->softwareValue, gs_product, maxSoftware);
		pdir->softwareValue[maxSoftware - 1] = 0;
		sprintf(revs, " %1.2f", gs_revision / 100.0);
		strncat(pdir->softwareValue, revs,
			maxSoftware - strlen(pdir->softwareValue) - 1);
		pdir->Software.count = strlen(pdir->softwareValue) + 1;
	}
	{	struct tm tms;
		time_t t;
		time(&t);
		tms = *localtime(&t);
		sprintf(pdir->dateTimeValue, "%04d:%02d:%02d %02d:%02d:%02d",
			tms.tm_year + 1900, tms.tm_mon, tms.tm_mday,
			tms.tm_hour, tms.tm_min, tms.tm_sec);
	}
	/* DateTime */
	tiff_fixuptags(&pdir->SubFileType, NTAGS);
	fwrite((char *)pdir, sizeof(*pdir), 1, fp);

	/*puteol(tfdev);*/	/****** USES EndOfBlock ****** WRONG ******/
	return 0;
}

/* End a TIFF/F page. */
private int
tiff_end_page(gx_device_tfax *tfdev, FILE *fp)
{	long dir_off;
	TIFF_ulong cc;

	dir_off = tfdev->dir_off;
	tfdev->prev_dir = tfdev->dir_off + offset_of(TIFF_directory, diroff);
	tfdev->dir_off = ftell(fp);
	/* Patch strip byte counts value. */
	cc = tfdev->dir_off - (dir_off + sizeof(TIFF_directory));
	fseek(fp, dir_off + offset_of(TIFF_directory, StripByteCounts.value), SEEK_SET);
	fwrite(&cc, sizeof(cc), 1, fp);
	return 0;
}
