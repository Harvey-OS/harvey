/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gendev.c,v 1.7 2004/12/08 21:35:13 stefan Exp $ */
/* Generate .dev configuration files */
#include "stdpre.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>		/* for calloc */
#include <string.h>

/*
 * This program generates .dev configuration files.
 *
 * Usage:
 *      gendev [-Z] [-n [<name_prefix> | -]] [-C [<dir> | -]]
 *        (-d <devfile> | -m <modfile> | -a <modfile>)
 *        (-<category> | <item>)*
 *
 * The name_prefix is for device[2], font, init, and iodev resources.
 * ****** DOESN'T HANDLE -replace YET ******
 * ****** DOESN'T MERGE device AND device2 ******
 */

#define DEFAULT_NAME_PREFIX "gs_"
#define INITIAL_CATEGORY "obj"

typedef struct config_s {
    /* Set once */
    FILE *out;
    bool debug;
    const char *name_prefix;
    const char *file_prefix;
    ulong file_id;		/* for uniq_last detection */
    /* Updated dynamically */
#define ITEM_ID_BITS 5
    uint item_id;
    bool any_scan_items;
    bool in_res_scan;
    bool in_category;		/* in_category implies in_res_scan */
    const char *current_category;
} config;
static const config init_config =
{
    0,				/* out */
    0,				/* debug */
    DEFAULT_NAME_PREFIX,	/* name_prefix */
    "",				/* file_prefix */
    0,				/* file_id */
    0,				/* item_id */
    0,				/* any_scan_items */
    0,				/* in_res_scan */
    0,				/* in_category */
    ""				/* current_category */
};

static const char *indent_INCLUDED = "\t\t\t\t";
static const char *indent_RES_SCAN = "   ";
static const char *indent_category = "\t";
static const char *indent_SEEN = "\t    ";
static const char *indent_include = "   ";
static const char *indent_scan_item = "\t";
static const char *indent_item = "";

/* Forward definitions */
void add_entry(config *, const char *, const char *, bool);

main(int argc, char *argv[])
{
    config conf;
    int i, j;
    bool dev, append;
    const char *category = INITIAL_CATEGORY;
    const char *fnarg;
    FILE *out;
    long pos;

    conf = init_config;
    /* Process command line arguments. */
    for (i = 1; i < argc; i++) {
	const char *arg = argv[i];

	if (*arg != '-') {
	    fprintf(stderr, "-d|m|a must precede non-switches.\n", arg);
	    exit(1);
	}
	switch (arg[1]) {
	    case 'C':		/* change directory, by analogy with make */
		conf.file_prefix =
		    (argv[i + 1][0] == '-' ? "" : argv[i + 1]);
		++i;
		continue;
	    case 'n':
		conf.name_prefix =
		    (argv[i + 1][0] == '-' ? "" : argv[i + 1]);
		++i;
		continue;
	    case 'a':
		dev = false, append = true;
		break;
	    case 'd':
		dev = true, append = false;
		break;
	    case 'm':
		dev = false, append = false;
		break;
	    case 'Z':
		conf.debug = true;
		continue;
	    default:
		fprintf(stderr, "Unknown switch %s.\n", argv[i]);
		exit(1);
	}
	break;
    }
    if (i == argc - 1) {
	fprintf(stderr, "No output file name given, last argument is %s.\n",
		argv[i]);
	exit(1);
    }
    /* Must be the output file. */
    fnarg = argv[++i];
    {
	char fname[100];

	strcpy(fname, fnarg);
	strcat(fname, ".dev");
	out = fopen(fname, (append ? "a" : "w"));
	if (out == 0) {
	    fprintf(stderr, "Can't open %s for output.\n", fname);
	    exit(1);
	}
	if (!append)
	    fprintf(out,
		    "/*\n * File %s created automatically by gendev.\n */\n",
		    fname);
    }
    conf.out = out;
    pos = ftell(out);
    /* We need a separate _INCLUDED flag for each batch of definitions. */
    fprintf(out, "\n#%sifndef %s_%ld_INCLUDED\n",
	    indent_INCLUDED, fnarg, pos);
    fprintf(out, "#%s  define %s_%ld_INCLUDED\n",
	    indent_INCLUDED, fnarg, pos);
    /* Create a "unique" hash for the output file. */
    for (j = 0; fnarg[j] != 0; ++j)
	conf.file_id = conf.file_id * 41 + fnarg[j];
    conf.item_id <<= ITEM_ID_BITS;
    /* Add the real entries. */
    if (dev)
	add_entry(&conf, "dev", fnarg, false);
    for (j = i + 1; j < argc; ++j) {
	const char *arg = argv[j];

	if (arg[0] == '-')
	    category = arg + 1;
	else
	    add_entry(&conf, category, arg, false);
    }
    if (conf.in_category)
	fprintf(out, "#%sendif /* -%s */\n",
		indent_category, conf.current_category);
    /* Add the scanning entries, if any. */
    if (conf.any_scan_items) {
	if (conf.in_res_scan)
	    fprintf(out, "#%selse /* RES_SCAN */\n", indent_RES_SCAN);
	else
	    fprintf(out, "#%sifdef RES_SCAN\n", indent_RES_SCAN);
	conf.in_res_scan = true;
	category = INITIAL_CATEGORY;
	conf.item_id = 0;
	for (j = i + 1; j < argc; ++j) {
	    const char *arg = argv[j];

	    if (arg[0] == '-')
		category = arg + 1;
	    else
		add_entry(&conf, category, arg, true);
	}
    }
    if (conf.in_res_scan)
	fprintf(out, "#%sendif /* RES_SCAN */\n", indent_RES_SCAN);
    fprintf(out, "#%sendif /* !%s_%ld_INCLUDED */\n",
	    indent_INCLUDED, fnarg, pos);
    fclose(out);
    exit(0);
}

/* Add an entry to the output. */
typedef enum {
    uniq_none = 0,
    uniq_first,
    uniq_last
} uniq_mode;
void
write_item(config * pconf, const char *str, const char *category,
	   const char *item, uniq_mode mode)
{
    FILE *out = pconf->out;
    char cati[80];

    if (!pconf->in_res_scan) {
	fprintf(out, "#%sifndef RES_SCAN\n", indent_RES_SCAN);
	pconf->in_res_scan = true;
    }
    if (strcmp(pconf->current_category, category)) {
	const char *paren = strchr(str, '(');

	if (pconf->in_category)
	    fprintf(out, "#%sendif /* -%s */\n",
		    indent_category, pconf->current_category);
	fprintf(out, "#%sifdef ", indent_category);
	fwrite(str, sizeof(char), paren - str, out);

	fprintf(out, "\n");
	pconf->current_category = category;
	pconf->in_category = true;
    }
    sprintf(cati, "%s_%s_", category, item);
    switch (mode) {
	case uniq_none:
	    fprintf(out, "%s%s\n", indent_item, str);
	    break;
	case uniq_first:
	    fprintf(out, "#%sifndef %sSEEN\n", indent_SEEN, cati);
	    fprintf(out, "#%s  define %sSEEN\n", indent_SEEN, cati);
	    write_item(pconf, str, category, item, uniq_none);
	    fprintf(out, "#%sendif\n", indent_SEEN, cati);
	    break;
	case uniq_last:
	    fprintf(out, "#%sif %sSEEN == %lu\n", indent_SEEN, cati,
		    pconf->file_id + pconf->item_id++);
	    write_item(pconf, str, category, item, uniq_none);
	    fprintf(out, "#%sendif\n", indent_SEEN, cati);
	    pconf->any_scan_items = true;
	    break;
    }
}
void
write_scan_item(config * pconf, const char *str, const char *category,
		const char *item, uniq_mode mode)
{
    FILE *out = pconf->out;
    char cati[80];

    sprintf(cati, "%s_%s_", category, item);
    switch (mode) {
	case uniq_none:
	    break;
	case uniq_first:
	    break;
	case uniq_last:
	    fprintf(out, "#%sundef %sSEEN\n", indent_scan_item, cati);
	    fprintf(out, "#%s  define %sSEEN %lu\n", indent_scan_item, cati,
		    pconf->file_id + pconf->item_id++);
    }
}
void
add_entry(config * pconf, const char *category, const char *item, bool scan)
{
    char str[80];
    uniq_mode mode = uniq_first;

    if (pconf->debug && !scan)
	printf("Adding %s %s;\n", category, item);
    str[0] = 0;
    switch (category[0]) {	/* just an accelerator */
#define is_cat(str) !strcmp(category, str)
	case 'd':
	    if (is_cat("dev"))
		sprintf(str, "device_(%s%s_device)\n",
			pconf->name_prefix, item);
	    else if (is_cat("dev2"))
		sprintf(str, "device2_(%s%s_device)\n",
			pconf->name_prefix, item);
	    break;
	case 'e':
	    if (is_cat("emulator"))
		sprintf(str, "emulator_(\"%s\",%d)",
			item, strlen(item));
	    break;
	case 'f':
	    if (is_cat("font"))
		sprintf(str, "font_(\"0.font_%s\",%sf_%s,zf_%s)",
			item, pconf->name_prefix, item, item);
	    break;
	case 'i':
	    if (is_cat("include")) {
		int len = strlen(item);

		if (scan)
		    return;
		if (strcmp(pconf->current_category, category)) {
		    if (pconf->in_category) {
			fprintf(pconf->out, "#%sendif /* -%s */\n",
				indent_category, pconf->current_category);
			pconf->in_category = false;
		    }
		    pconf->current_category = category;
		}
		if (pconf->in_res_scan) {
		    fprintf(pconf->out, "#%sendif /* RES_SCAN */\n",
			    indent_RES_SCAN);
		    pconf->in_res_scan = false;
		}
		if (len < 5 || strcmp(item + len - 4, ".dev"))
		    fprintf(pconf->out, "#%sinclude \"%s.dev\"\n",
			    indent_include, item);
		else
		    fprintf(pconf->out, "#%sinclude \"%s\"\n",
			    indent_include, item);
		return;
	    } else if (is_cat("init"))
		sprintf(str, "init_(%s%s_init)", pconf->name_prefix, item);
	    else if (is_cat("iodev"))
		sprintf(str, "io_device_(%siodev_%s)", pconf->name_prefix, item);
	    break;
	case 'l':
	    if (is_cat("lib")) {
		sprintf(str, "lib_(%s)", item);
		mode = uniq_last;
	    }
	    break;
	case 'o':
	    if (is_cat("obj"))
		sprintf(str, "obj_(%s%s)", pconf->file_prefix, item);
	    else if (is_cat("oper"))
		sprintf(str, "oper_(%s_op_defs)", item);
	    break;
	case 'p':
	    if (is_cat("ps"))
		sprintf(str, "psfile_(\"%s.ps\",%d)",
			item, strlen(item) + 3);
	    break;
#undef is_cat
	default:
	    ;
    }
    if (str[0] == 0) {
	fprintf(stderr, "Unknown category %s.\n", category);
	exit(1);
    }
    if (scan)
	write_scan_item(pconf, str, category, item, mode);
    else
	write_item(pconf, str, category, item, mode);
}
