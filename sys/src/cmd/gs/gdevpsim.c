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

/* gdevpsim.c */
/* PostScript image output device */
#include "gdevprn.h"

/*
 * This driver does what the ps2image utility used to do:
 * It produces a bitmap in the form of a PostScript file that can be
 * fed to any PostScript printer.  It uses a run-length compression
 * method that executes quickly (unlike some produced by PostScript
 * drivers!).
 */

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 300
#endif
#ifndef Y_DPI
#  define Y_DPI 300
#endif

/* The device descriptor */
private dev_proc_print_page(psmono_print_page);

gx_device_printer far_data gs_psmono_device =
  prn_device(prn_std_procs, "psmono",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0, 0, 0, 0,		/* margins */
	1, psmono_print_page);

/*
 * The following setup code gets written to the PostScript file.
 * We have to break it up because the Watcom compiler has a limit of
 * 512 characters in a single token.
 */
private const char far_data *psmono_setup[] = {
"%!PS\r\n\
  /maxrep 31 def		% max repeat count\r\n\
  /maxrep1 maxrep 1 sub def\r\n\
		% Initialize the strings for filling runs.\r\n\
     /.ImageFills [\r\n\
     0 1 255\r\n\
      { maxrep string dup 0 1 maxrep1 { 3 index put dup } for\r\n\
	pop exch pop readonly\r\n\
      } for\r\n\
     ] readonly def\r\n\
","\
		% Initialize the procedure table for input dispatching.\r\n\
     /.ImageProcs [\r\n\
","\
     33 { { pop .ImageItem } } repeat\r\n\
     32 { {	% 0x21-0x40: (N-0x20) data bytes follow\r\n\
      32 sub 3 index exch 0 exch getinterval 2 index exch\r\n\
      readhexstring pop exch pop dup\r\n\
     } bind } repeat\r\n\
     31 { {	% 0x41-0x5f: repeat last data byte (N-0x40) times\r\n\
      64 sub .ImageFills 2 index dup length 1 sub get get\r\n\
      exch 0 exch getinterval\r\n\
     } bind } repeat\r\n\
     160 { { pop .ImageItem } } repeat\r\n\
     ] readonly def\r\n\
","\
		% Read one item from a compressed image.\r\n\
		% Stack contents: <buffer> <file> <previous>\r\n\
  /.ImageItem\r\n\
   { 1 index read pop dup .ImageProcs exch get exec\r\n\
   } bind def\r\n\
","\
		% Read and print an entire compressed image,\r\n\
		% scaling it uniformly in X and Y to fill the page.\r\n\
		% Arguments: <width> <height>\r\n\
","\
  /.ImageRead\r\n\
   { gsave 1 [\r\n\
     clippath pathbbox pop pop translate\r\n\
     pathbbox newpath 4 -2 roll pop pop\r\n\
     dup 3 1 roll abs 5 index exch div exch abs 6 index exch div\r\n\
     2 copy lt { exch } if pop		% (definition of max)\r\n\
     0 0 2 index neg 0 4 index 7 -1 roll mul\r\n\
     ] { .ImageItem }\r\n\
     4 index 7 add 8 idiv string currentfile ()\r\n\
     8 3 roll\r\n\
     image pop pop pop\r\n\
     grestore showpage\r\n\
   } bind def\r\n\
"};

#define data_run_code 0x20
#define max_data_run 32
#define repeat_run_code 0x40
#define max_repeat_run 31

/* Send the page to the printer. */
private void write_data_run(P4(const byte *, int, FILE *, byte));
private int
psmono_print_page(gx_device_printer *pdev, FILE *prn_stream)
{	int line_size = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
	int lnum;
	byte *line = (byte *)gs_malloc(line_size, 1, "psmono_print_page");
	byte *data;

	if ( line == 0 )
	  return_error(gs_error_VMerror);

	/* If this is the first page of the file, */
	/* write the setup code. */
	if ( gdev_prn_file_is_new(pdev) )
	  {	int i;
		for ( i = 0; i < countof(psmono_setup); i++ )
			fputs(psmono_setup[i], prn_stream);
	  }

	/* Write the .ImageRead command. */
	fprintf(prn_stream, "%d %d .ImageRead\r\n",
		pdev->width, pdev->height);

	/* Compress each scan line in turn. */
	for ( lnum = 0; lnum < pdev->height; lnum++ )
	  {	const byte *p;
		int left = line_size;
		gdev_prn_get_bits(pdev, lnum, line, &data);
		p = data;
		/* Loop invariant: p + left = data + line_size. */
		while ( left >= 4 )
		  {	/* Detect a maximal run of non-repeated data. */
			const byte *p1 = p;
			int left1 = left;
			byte b;
			int count;
			while ( left1 >= 4 && ((b = *p1) != p1[1] ||
				b != p1[2] || b != p1[3])
			      )
			  ++p1, --left1;
			if ( left1 < 4 )
			  break;		/* no repeated data left */
			write_data_run(p, (int)(p1 - p + 1), prn_stream, 0xff);
			/* Detect a maximal run of repeated data. */
			p = ++p1 + 3;
			left = --left1 - 3;
			while ( left > 0 && *p == b )
			  ++p, --left;
			for ( count = p - p1; count > max_repeat_run;
			      count -= max_repeat_run
			    )
			  {	putc(repeat_run_code + max_repeat_run,
				     prn_stream);
			  }
			putc(repeat_run_code + count, prn_stream);
		  }
		/* Write the remaining data, if any. */
		write_data_run(p, left, prn_stream, 0xff);
	  }

	/* Clean up and return. */
	fputs("\r\n", prn_stream);
	gs_free((char *)line, line_size, 1, "psmono_print_page");
	return 0;
}

/* Write a run of data on the file. */
private void
write_data_run(const byte *data, int count, FILE *f, byte invert)
{	register const byte *p = data;
	register int left = count;
	register const char _ds *hex_digits = "0123456789abcdef";
	while ( left > 0 )
	  {	int wcount = min(left, max_data_run);
		left -= wcount;
		putc(data_run_code + wcount, f);
		for ( ; wcount > 0; ++p, --wcount )
		  {	byte b = *p ^ invert;
			char c = hex_digits[b >> 4];
			putc(c, f);
			c = hex_digits[b & 0xf];
			putc(c, f);
		  }
		putc('\r', f);
		putc('\n', f);
	  }

}
