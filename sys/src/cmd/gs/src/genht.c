/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: genht.c,v 1.5 2005/10/04 06:30:02 ray Exp $ */
/* Generate C code for compiling halftones into ROM. */
#include "malloc_.h"
#include "stdio_.h"
#include "string_.h"
#include "gscdefs.h"
#include "gsmemory.h"
#include "gxbitmap.h"
#include "gxhttile.h"
#include "gxtmap.h"
#include "gxdht.h"
#include "gxdhtres.h"
#include "strimpl.h"
#include "sstring.h"

/*
 * This program translates PostScript halftone resources into C data
 * structures that can then be compiled into executables and put in shared,
 * read-only memory.  The syntax of the resource file is a subset of
 * PostScript, tightly constrained so that it can be parsed easily.  Blank
 * lines and PostScript comments are ignored, but otherwise each halftone
 * in the file (there may be more than one) must follow this format,
 * where ... indicates arbitrary text:
    (optionally:)
	/halftone_name ...
	/HalftoneType 5
    (zero or more times:)
	/halftone_name ...
	/HalftoneType 3
	/Width xxx
	/Height xxx
	/Thresholds ...
	--hex data terminated by a >--
	(zero or more times:)
	    /halftone_name 1 index
    (finally)
    ... defineresource
 * Lines that don't follow the above syntax may appear anywhere in the file
 * except in the middle of the hex data.  Width and Height must precede
 * Thresholds, but otherwise the 4 parameters may appear in any order.
 * White space at the beginning or end of a line is ignored, and any
 * amount of white space may separate the parameter name from its value.
 *
 * We envision that this format will eventually be extended to cover
 * HalftoneType 16 halftones with a single rectangle, allowing 16-bit
 * threshold values.
 */

/* Read a source file into memory. */
private char *
read_file(FILE *in, char *cname)
{
    int len, nread;
    char *cont;

    fseek(in, 0L, 2 /*SEEK_END*/);
    len = ftell(in);
    cont = malloc(len + 1);
    if (cont == 0) {
	fprintf(stderr, "Can't allocate %d bytes to read %s.\n",
		len + 1, cname);
	return 0;
    }
    rewind(in);
    nread = fread(cont, 1, len, in);
    cont[nread] = 0;
    return cont;
}

/* Parse a Halftone resource file into memory. */
private bool
parse_line(char **pstr, char **pline)
{
    char *str = *pstr;

top:
    while (*str && strchr(" \t\r\n", *str)) /* trim leading space */
	++str;
    if (*str == 0) {
	*pline = 0;
	return false;
    }
    *pline = str;
    while (*str && !strchr("\r\n", *str)) /* find EOL */
	++str;
    while (str > *pline && strchr(" \t", str[-1])) /* trim trailing space */
	--str;
    *str = 0;
    *pstr = str + 1;
    return true;
}
private int
parse_halftone(gx_device_halftone_resource_t *phtr, byte **pThresholds,
	       char **pprefix, char **pcont)
{
    char *str;
    char *line;
    char *rname = 0;
    int HalftoneType = -1;
    int Width = -1, Height = -1;
    byte *Thresholds = 0;
    stream_AXD_state ss;

    /* Parse the file. */
    for (str = *pcont; parse_line(&str, &line);) {
	char *end;
	char *prefix;
	char terminator;

	if (line[0] == '%')
	    continue;
	if (strlen(line) >= 14 &&
	    !strcmp(line + strlen(line) - 14, "defineresource")
	    )
	    break;
	if (line[0] != '/')
	    continue;
	if (strlen(line) >= 8 &&
	    !strcmp(line + strlen(line) - 8, " 1 index")
	    )
	    continue;
	end = ++line;
	while (*end && !strchr(" \t<", *end)) /* find end of name */
	    ++end;
	terminator = *end;
	*end = 0;
	if (rname == 0) { /* first name is halftone name */
	    rname = malloc(strlen(line) + 1);
	    strcpy(rname, line);
	    continue;
	}
	if (terminator == 0)		/* name alone */
	    continue;
	++end;
	if (!strcmp(line, "HalftoneType")) {
	    if (sscanf(end, "%d", &HalftoneType) != 1) {
		fprintf(stderr, "Invalid HalftoneType syntax: %s\n", line - 1);
		return -1;
	    }
	    switch (HalftoneType) {
	    case 3:
		break;
	    case 5:
		if (*pprefix)
		    free(*pprefix);
		*pprefix = rname;
		rname = 0;
		break;
	    default:
		fprintf(stderr, "Invalid HalftoneType: %s\n", end);
		return -1;
	    }
	    continue;
	} else if (!strcmp(line, "Width")) {
	    if (sscanf(end, "%d", &Width) != 1 ||
		Width <= 0 || Width > 0x4000
		) {
		fprintf(stderr, "Invalid Width: %s\n", end);
		return -1;
	    }
	} else if (!strcmp(line, "Height")) {
	    if (sscanf(end, "%d", &Height) != 1 ||
		Height <= 0 || Height > 0x4000
		) {
		fprintf(stderr, "Invalid Height: %s\n", end);
		return -1;
	    }
	} else if (!strcmp(line, "Thresholds")) {
	    uint ignore;
	    uint num_levels = 256;
	    uint num_bits = Width * Height;
	    char *eol = end + strlen(end); /* skip rest of line */
	    stream_cursor_read r;
	    stream_cursor_write w;

	    if (Width < 0 || Height < 0) {
		fprintf(stderr, "Width and Height must precede Thresholds.\n");
		return -1;
	    }
	    phtr->num_levels = num_levels;
	    phtr->levels =
		malloc(num_levels * sizeof(*phtr->levels));
	    phtr->bit_data =
		malloc(num_bits * sizeof(ushort));
	    Thresholds = malloc(num_bits);
	    s_AXD_init_inline(&ss);
	    r.ptr = (const byte *)eol;
	    r.limit = (const byte *)eol + strlen(eol + 1);
	    w.ptr = Thresholds - 1;
	    w.limit = w.ptr + num_bits;
	    s_AXD_template.process((stream_state *)&ss, &r, &w, true);
	    str = (char *)r.ptr + 1;
	    break;
	}
    }

    /* Check for successful parsing. */
    if (rname == 0)
	return 1;		/* end of definitions */
    if (HalftoneType < 0)
	fprintf(stderr, "HalftoneType not found.\n");
    if (Width < 0)
	fprintf(stderr, "Width not found.\n");
    if (Height < 0)
	fprintf(stderr, "Height not found.\n");
    if (Thresholds == 0)
	fprintf(stderr, "Thresholds not found.\n");
    if (rname == 0 || Thresholds == 0)
	return -1;
    phtr->rname = rname;
    phtr->HalftoneType = HalftoneType;
    phtr->Width = Width;
    phtr->Height = Height;
    *pThresholds = Thresholds;
    *pcont = str;
    return 0;
}

/* Write a halftone as a C procedure. */
private int
write_halftone(FILE *out, gx_device_halftone_resource_t *phtr,
	       const char *prefix, int index)
{
    int num_bits = phtr->Width * phtr->Height;
    int i;

    /* Write the initial comment. */
    fputs("\n/* ", out);
    if (prefix)
	fprintf(out, "%s.", prefix);
    fprintf(out, "%s */\n", phtr->rname);

    /* Write the levels array. */
    fprintf(out, "static const unsigned int levels_%d[] = {", index);
    for (i = 0; i < phtr->num_levels; ++i) {
	if (i % 10 == 0)
	    fputs("\n", out);
	fprintf(out, "%5u,", phtr->levels[i]);
    }
    fputs("\n0};\n", out);

    /* Write the bit_data array. */
    fprintf(out, "static const unsigned short bit_data_%d[] = {", index);
    for (i = 0; i < num_bits; ++i) {
	if (i % 10 == 0)
	    fputs("\n", out);
	fprintf(out, "%5u,", ((const ushort *)phtr->bit_data)[i]);
    }
    fputs("\n0};\n", out);

    /* Write the top-level structure. */
    fprintf(out, "static const gx_device_halftone_resource_t res_%d = {\n    \"%s\", %d, %d, %d, %d, levels_%d, bit_data_%d, %u\n};\n",
	    index, phtr->rname, phtr->HalftoneType, phtr->Width, phtr->Height,
	    phtr->num_levels, index, index,
	    ht_order_procs_short.bit_data_elt_size);

    return 0;
}

/* Main program */
int
main(int argc, char *argv[])
{
    char *iname;
    FILE *in;
    char *oname;
    FILE *out;
    int code;
    char *cont;
    char *line;
    gx_device_halftone_resource_t res;
    char *prefix = 0;
    byte *Thresholds;
    gx_ht_order order;
    int index, i;

    if (argc != 3) {
	fprintf(stderr, "Usage: genht ht_res.ps ht_data.c\n");
	exit(1);
    }
    iname = argv[1];
    oname = argv[2];
    in = fopen(iname, "rb");
    if (in == 0) {
	in = fopen(iname, "r");
	if (in == 0) {
	    fprintf(stderr, "Can't read %s.\n", iname);
	    exit(1);
	}
    }
    cont = read_file(in, iname);
    if (cont == 0)
	exit(1);
    fclose(in);
    out = fopen(oname, "w");
    if (out == 0) {
	fprintf(stderr, "Can't open %s for output.\n", oname);
	exit(1);
    }
    fprintf(out, "/*\n * This file %s was generated from %s by genht.\n * Do not edit this file.\n *\n", oname, iname);
    /* Copy initial comments from the input file. */
    while (parse_line(&cont, &line) && line[0] == '%')
	if (line[1] != '!')
	    fprintf(out, " * %s\n", line + 1);
    cont[-1] = '\n';
    cont = line;
    fputs(" */\n#include \"gxdhtres.h\"\n", out);
    for (index = 0;
	 (code = parse_halftone(&res, &Thresholds, &prefix, &cont)) == 0;
	 ++index) {
	order.width = res.Width;
	order.num_levels = res.num_levels;
	order.levels = (uint *)res.levels;
	order.num_bits = res.Width * res.Height;
	order.bit_data = (void *)res.bit_data;
	ht_order_procs_short.construct_order(&order, Thresholds);
	write_halftone(out, &res, prefix, index);
    }
    if (prefix == 0)
	prefix = res.rname;
    fputs("/* Check the prototype. */\n", out);
    fprintf(out, "DEVICE_HALFTONE_RESOURCE_PROC(gs_dht_%s);\n", prefix);
    fputs("\nconst gx_device_halftone_resource_t *const *\n", out);
    fprintf(out, "gs_dht_%s(void)\n{\n    static const gx_device_halftone_resource_t *const res[] = {\n\t",
	    prefix);
    for (i = 0; i < index; ++i)
	fprintf(out, "&res_%d, ", i);
    fputs("0\n    };\n    return res;\n}\n", out);
    fclose(out);
    if (code < 0)
	exit(1);
    return 0;
}

/* Stubs */
ENUM_PTRS_BEGIN_PROC(gs_no_struct_enum_ptrs)
{
    return 0;
    ENUM_PTRS_END_PROC
}
RELOC_PTRS_BEGIN(gs_no_struct_reloc_ptrs)
{
}
RELOC_PTRS_END
public_st_stream_state();
void
gx_ht_complete_threshold_order(gx_ht_order *porder)
{
}

/*
 * In order to avoid a linking step, we #include the required files here
 * rather than compiling them separately.
 */
#include "gxhtbit.c"
#include "scantab.c"
#include "sstring.c"

const gx_dht_proc gx_device_halftone_list[] = { 0 };
