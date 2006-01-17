/* Copyright (C) 1993, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: genconf.c,v 1.10 2004/12/20 22:17:39 igor Exp $ */
/* Generate configuration files */
#include "stdpre.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>		/* for calloc */
#include <string.h>

/*
 * This program reads .dev files, which contain definitions of modules,
 * and generates merged configuration files.
 *
 * A .dev file specifies a list of "resources" of various kinds (.obj/.o
 * files, other .dev files to be merged in, PostScript files, driver names,
 * compiled fonts, etc.)  that make up the module.  (This is similar in
 * spirit to the Macintosh resource fork, and to PostScript resources, but
 * has nothing to do directly with the latter.)
 *
 * A .dev file consists of a series of switches and resource names.  Most
 * switches specifies the type of the following resources, until the next
 * type switch; there are also a few other switches.
 *
 * genconf recognizes the following resource type switches in .dev files.
 * See the next section on command line switches for the meaning of
 * <name_prefix>.
 *
 *    -comp <name>
 *
 *	Adds compositor_(<name_prefix>_composite_<name>_type) to <gconfig.h>.
 *	Used for gs_composite_type_t structures, whose identity may need to
 *	passed through the command list.
 *
 *    -dev <device>
 *
 *	Adds device_(<name_prefix><device>_device) to <gconfig.h>.
 *	Used for ordinary devices.
 *
 *    -dev2 <device>
 *
 *	Adds device2_(<name_prefix><device>_device) to <gconfig.h>.
 *	Used for some newer devices with potentially different structures
 *	or conventions.
 *
 *    -emulator <emulator>
 *
 *	Adds emulator_("<emulator>",<len>), where len is the number of
 *	characters in <emulator>, to <gconfig.h>.
 *	Used for names that should appear as instances of the PostScript
 *	Emulator resource category.
 *
 *    -font <font>
 *
 *	Adds an entry to <gconfigf.h>, as described below.
 *
 *    -functiontype <fntype>
 *
 *	Adds function_type_(<fntype>,<name_prefix>build_function_<fntype>)
 *	to <gconfig.h>.
 *	Used for instances of the PostScript FunctionType resource category,
 *	and also to generate a table of procedures for processing
 *	Function dictionaries.
 *
 *    -halftone <htname>
 *
 *	Adds halftone_(<name_prefix>dht_<htname>) to <gconfig.h>.
 *	Used for halftones that will be compiled into the executable.
 *
 *    -imageclass <iclass>
 *
 *	Adds image_class_(<name_prefix>image_class_<iclass>) to <gconfig.h>.
 *	Used internally for the various cases of rendering images.
 *
 *    -imagetype <itype>
 *
 *	Adds image_type_(<itype>,<name_prefix>image_type_<itype>) to
 *	<gconfig.h>.
 *	Used for instances of the PostScript ImageType resource category,
 *	and also to generate a table of procedures for handling the various
 *	types of images.
 *
 *    -include <module>
 *
 *	Reads the file named <module> (or <module>.dev if <module> doesn't
 *	end with ".dev") as though it were part of the current .dev file.
 *	Used when one module requires the presence of another.
 *
 *    -init <initname>
 *
 *	Adds init_(<name_prefix><initname>_init) to <gconfig.h>.
 *	Used for initialization procedures to be called when Ghostscript
 *	is started.  By convention, <initname> is (almost always) the name
 *	of the source file in which the procedure is defined.
 *
 *    -iodev <iodev>
 *
 *	Adds io_device_(<name_prefix>iodev_<iodev>) to <gconfig.h>.
 *	Used for instances of the PostScript IODevice resource category.
 *
 *    -lib <lib>
 *
 *	Adds <lib> to the list of libraries to be written by -l.
 *
 *    -libpath <libpath>
 *
 *	Adds <libpath> to the list of library paths to be written by -l.
 *
 *    -link <link>
 *
 *	Adds <link> to the list of switches to be passed to the linker,
 *	to be written by -l.
 *
 *    -obj <obj>
 *
 *	Adds <obj> to the list of files to be linked, to be written by -o.
 *
 *    -oper <opername>
 *
 *	Adds oper_(<opername>_op_defs) to <gconfig.h>.
 *	Used for tables of PostScript operators.  By convention,
 *	<opername> is (usually) the name of the source file in which the
 *	table appears.
 *
 *    -plugin <plugin_name>
 *
 *	Adds plugin_(<name_prefix><plugin_name>_instantiate) to <gconfig.h>.
 *	Used for plugins to be instantiated when Ghostscript
 *	is started.  By convention, <plugin_name> is (almost always) the name
 *	of the source file in which the plugin is defined.
 *
 *    -ps <psname>
 *
 *	Adds psfile_("<psname>.ps",<len+3>), where <len> is the number of
 *	character in <psname>, to <gconfig.h>.
 *	Used for PostScript files that should be read at initialization.
 *
 * genconf recognizes the following other switches in .dev files:
 *
 *    -replace <module>
 *
 *	This switch declares that <module> has been replaced by another
 *	module (presumably the one defined by the .dev file in which the
 *	switch appears), and should be removed from consideration.
 *	Modules that can be -replaced should not -include other modules.
 *
 * genconf writes the following files if the corresponding switch is used:
 *
 *    -h <gconfig.h>
 *
 *	Writes a list of all the resources as described above.  Each
 *	sublist is surrounded by an #ifdef/#endif in case the
 *	referenced macro (e.g., device_, oper_) is undefined.
 *	Duplicates are eliminated.
 *
 *    -f <gconfigf.h>
 *
 *	Writes a list of all the -font resources, in the form
 *		font_("0.font_<name>",<name_prefix>f_<name>,zf_<name)
 *
 * Other switches specify various options and parameters:
 *
 *    -Z
 *
 *	Turns on debugging output.
 *
 *    -C [<file_prefix>]
 *
 *	Prefixes <file_prefix> to the names of all .dev files read in,
 *	and to the names of all .obj/.o files written in <gconfig.h>.
 *	The default file_prefix is the empty string.
 *	This switch should probably be removed, since nothing in the
 *	current Ghostscript makefiles uses it.
 *
 *    -e <escapechar>
 *
 *	Sets the escape character for patterns.  See below.
 *
 *    -n [<name_prefix> | -]
 *
 *	Prefixes <name_prefix>, or the empty string, to certain items in
 *	the output, as indicated above.
 *	The default name_prefix is "gs_".
 *
 *    -p[l|L][u][e] <pattern>
 *
 *	Sets the pattern (format string) used for writing certain items in
 *	the output, as indicated above.  '%' in the pattern indicates
 *	substitution of the variable data, as for printf except that the
 *	'%' is not followed by a format character -- the data are always
 *	inserted literally.  See below for more information about patterns.
 *
 *    -l[o] <lib.tr>
 *
 *	Writes the list of library paths, links, and library file names
 *	on <lib.tr>.  -lo also writes the list of object file names,
 *	as for -o. This feature is obsolete and is not longer in use.
 *      Use -ol instead.
 *
 *    -o[l] <obj.tr>
 *
 *	Writes the list object file names on <obj.tr>.  -ol also writes
 *	the list of library paths, links, and library file names as for -l.
 *
 * Usage summary:
 *
 *      genconf [-Z] [-C prefix] [-e escapechar] [-n [name_prefix | -]]
 *	  [@]xxx.dev* [-f gconfigf.h] [-h gconfig.h]
 *        [-p[l|L][u][e] pattern] [-l|o|lo|ol lib.tr]
 *
 * Patterns:
 *
 *   The default escape character is &.  When this character appears in a
 *   pattern, it acts as follows:
 *	&p produces a %;
 *	&s produces a space;
 *	&& (i.e., the escape character twice) produces a \;
 *	&- produces a -;
 *	&x, for any other character x, is an error.
 */

#define DEFAULT_FILE_PREFIX ""
#define DEFAULT_NAME_PREFIX "gs_"

#define MAX_STR 120

/* Structures for accumulating information. */
typedef struct string_item_s {
    const char *str;
    int file_index;		/* index of file containing this item */
    int index;
} string_item_t;

/* The values of uniq_mode are bit masks. */
typedef enum {
    uniq_all = 1,		/* keep all occurrences (default) */
    uniq_first = 2,		/* keep only first occurrence */
    uniq_last = 4		/* keep only last occurrence */
} uniq_mode_t;
typedef struct string_list_s {
    /* The following are set at creation time. */
    const char *list_name;	/* only for debugging */
    int max_count;
    uniq_mode_t mode;
    /* The following are updated dynamically. */
    int count;
    string_item_t *items;
} string_list_t;

#define MAX_PATTERN 60
typedef struct string_pattern_s {
    bool upper_case;
    bool drop_extn;
    char pattern[MAX_PATTERN + 1];
} string_pattern_t;
typedef struct config_s {
    int debug;
    const char *name_prefix;
    const char *file_prefix;
    /* Special "resources" */
    string_list_t file_names;
    string_list_t file_contents;
    string_list_t replaces;
    /* Real resources */
    union ru_ {
	struct nu_ {
	    /* These names must parallel config_lists below. */
	    string_list_t sorted_resources;
	    string_list_t resources;
	    string_list_t devs;	/* also includes devs2 */
	    string_list_t compositors;
	    string_list_t fonts;
	    string_list_t libs;
	    string_list_t libpaths;
	    string_list_t links;
	    string_list_t objs;
	} named;
#define NUM_RESOURCE_LISTS 9
	string_list_t indexed[NUM_RESOURCE_LISTS];
    } lists;
    string_pattern_t lib_p;
    string_pattern_t libpath_p;
    string_pattern_t obj_p;
} config_t;

/*
 * These lists grow automatically if needed, so we could start out with
 * small allocations.
 */
static const config_t init_config = {
    0,				/* debug */
    DEFAULT_NAME_PREFIX,	/* name_prefix */
    DEFAULT_FILE_PREFIX,	/* file_prefix */
    {"file name", 200},		/* file_names */
    {"file contents", 200},	/* file_contents */
    {"-replace", 50}
};
static const string_list_t init_config_lists[] = {
    {"resource", 100, uniq_first},
    {"sorted_resource", 20, uniq_first},
    {"-comp", 10, uniq_first},
    {"-dev", 100, uniq_first},
    {"-font", 50, uniq_first},
    {"-lib", 20, uniq_last},
    {"-libpath", 10, uniq_first},
    {"-link", 10, uniq_first},
    {"-obj", 500, uniq_first}
};

/* Forward definitions */
private void *mrealloc(void *, size_t, size_t);
int alloc_list(string_list_t *);
void dev_file_name(char *);
int process_replaces(config_t *);
int read_dev(config_t *, const char *);
int read_token(char *, int, const char **);
int add_entry(config_t *, char *, const char *, int);
string_item_t *add_item(string_list_t *, const char *, int);
void sort_uniq(string_list_t *, bool);
void write_list(FILE *, const string_list_t *, const char *);
void write_list_pattern(FILE *, const string_list_t *, const string_pattern_t *);
bool var_expand(char *, char [MAX_STR], const config_t *);
void add_definition(const char *, const char *, string_list_t *, bool);
string_item_t *lookup(const char *, const string_list_t *);

int
main(int argc, char *argv[])
{
    config_t conf;
    char escape = '&';
    int i;

    /* Allocate string lists. */
    conf = init_config;
    memcpy(conf.lists.indexed, init_config_lists,
	   sizeof(conf.lists.indexed));
    alloc_list(&conf.file_names);
    alloc_list(&conf.file_contents);
    alloc_list(&conf.replaces);
    for (i = 0; i < NUM_RESOURCE_LISTS; ++i)
	alloc_list(&conf.lists.indexed[i]);

    /* Initialize patterns. */
    conf.lib_p.upper_case = false;
    conf.lib_p.drop_extn = false;
    strcpy(conf.lib_p.pattern, "%s\n");
    conf.libpath_p = conf.lib_p;
    conf.obj_p = conf.lib_p;

    /* Process command line arguments. */
    for (i = 1; i < argc; i++) {
	const char *arg = argv[i];
	FILE *out;
	int lib = 0, obj = 0;

	if (*arg != '-') {
	    read_dev(&conf, arg);
	    continue;
	}
	if (i == argc - 1) {
	    fprintf(stderr, "Missing argument after %s.\n",
		    arg);
	    exit(1);
	}
	switch (arg[1]) {
	    case 'C':		/* change directory, by analogy with make */
		conf.file_prefix =
		    (argv[i + 1][0] == '-' ? "" : argv[i + 1]);
		++i;
		continue;
	    case 'e':
		escape = argv[i + 1][0];
		++i;
		continue;
	    case 'n':
		conf.name_prefix =
		    (argv[i + 1][0] == '-' ? "" : argv[i + 1]);
		++i;
		continue;
	    case 'p':
		{
		    string_pattern_t *pat;

		    switch (*(arg += 2)) {
			case 'l':
			    pat = &conf.lib_p;
			    break;
			case 'L':
			    pat = &conf.libpath_p;
			    break;
			default:
			    pat = &conf.obj_p;
			    arg--;
		    }
		    pat->upper_case = false;
		    pat->drop_extn = false;
		    if (argv[i + 1][0] == '-')
			strcpy(pat->pattern, "%s\n");
		    else {
			char *p, *q;

			for (p = pat->pattern, q = argv[++i];
			     (*p++ = *q++) != 0;
			    )
			    if (p[-1] == escape)
				switch (*q) {
				    case 'p':
					p[-1] = '%'; q++; break;
				    case 's':
					p[-1] = ' '; q++; break;
				    case '-':
					p[-1] = '-'; q++; break;
				    default:
					if (*q == escape) {
					    p[-1] = '\\'; q++; break;
					}
					fprintf(stderr,
					  "%c not followed by p|s|%c|-: &%c\n",
						escape, escape, *q);
					exit(1);
				}
			p[-1] = '\n';
			*p = 0;
		    }
		    for (;;) {
			switch (*++arg) {
			    case 'u':
				pat->upper_case = true;
				break;
			    case 'e':
				pat->drop_extn = true;
				break;
			    case 0:
				goto pbreak;
			    default:
				fprintf(stderr, "Unknown switch %s.\n", arg);
				exit(1);
			}
		    }
		  pbreak:if (pat == &conf.obj_p) {
			conf.lib_p = *pat;
			conf.libpath_p = *pat;
		    }
		    continue;
		}
	    case 'Z':
		conf.debug = 1;
		continue;
	}
	/* Must be an output file. */
	out = fopen(argv[++i], "w");
	if (out == 0) {
	    fprintf(stderr, "Can't open %s for output.\n",
		    argv[i]);
	    exit(1);
	}
	switch (arg[1]) {
	    case 'f':
		process_replaces(&conf);
		fputs("/* This file was generated automatically by genconf.c. */\n", out);
		fputs("/* For documentation, see gsconfig.c. */\n", out);
		{
		    char template[80];

		    sprintf(template,
			    "font_(\"0.font_%%s\",%sf_%%s,zf_%%s)\n",
			    conf.name_prefix);
		    write_list(out, &conf.lists.named.fonts, template);
		}
		break;
	    case 'h':
		process_replaces(&conf);
		fputs("/* This file was generated automatically by genconf.c. */\n", out);
		write_list(out, &conf.lists.named.compositors, "%s\n");
		write_list(out, &conf.lists.named.devs, "%s\n");
		sort_uniq(&conf.lists.named.resources, true);
		write_list(out, &conf.lists.named.resources, "%s\n");
		sort_uniq(&conf.lists.named.sorted_resources, false);
		write_list(out, &conf.lists.named.sorted_resources, "%s\n");
		break;
	    case 'l':
		lib = 1;
		obj = arg[2] == 'o';
		goto lo;
	    case 'o':
		obj = 1;
		lib = arg[2] == 'l';
	      lo:process_replaces(&conf);
		if (obj) {
		    sort_uniq(&conf.lists.named.objs, true);
		    write_list_pattern(out, &conf.lists.named.objs, &conf.obj_p);
		}
		if (lib) {
		    sort_uniq(&conf.lists.named.libs, true);
		    sort_uniq(&conf.lists.named.links, true);
		    write_list_pattern(out, &conf.lists.named.libpaths, &conf.libpath_p);
		    write_list_pattern(out, &conf.lists.named.links, &conf.obj_p);
		    write_list_pattern(out, &conf.lists.named.libs, &conf.lib_p);
		}
		break;
	    default:
		fclose(out);
		fprintf(stderr, "Unknown switch %s.\n", argv[i]);
		exit(1);
	}
	fclose(out);
    }

    return 0;
}

/*
 * We would like to use the real realloc, but it doesn't work on all systems
 * (e.g., some Linux versions).  Also, this procedure does the right thing
 * if old_ptr = NULL.
 */
private void *
mrealloc(void *old_ptr, size_t old_size, size_t new_size)
{
    void *new_ptr = malloc(new_size);

    if (new_ptr == NULL)
	return NULL;
    /* We have to pass in the old size, since we have no way to */
    /* determine it otherwise. */
    if (old_ptr)
	memcpy(new_ptr, old_ptr, min(old_size, new_size));
    return new_ptr;
}

/* Allocate and initialize a string list. */
int
alloc_list(string_list_t * list)
{
    list->count = 0;
    list->items =
	(string_item_t *) calloc(list->max_count, sizeof(string_item_t));
    assert(list->items != NULL);
    return 0;
}

/* If necessary, convert a .dev name to its file name. */
void
dev_file_name(char *str)
{
    int len = strlen(str);

    if (len <= 4 || strcmp(".dev", str + len - 4))
	strcat(str, ".dev");
}

/* Delete any files that are named as -replace "resources". */
int
process_replaces(config_t * pconf)
{
    char bufname[MAX_STR];
    int i;

    for (i = 0; i < pconf->replaces.count; ++i) {
	int j;

	strcpy(bufname, pconf->replaces.items[i].str);
	/* See if the file being replaced was included. */
	dev_file_name(bufname);
	for (j = 0; j < pconf->file_names.count; ++j) {
	    const char *fname = pconf->file_names.items[j].str;

	    if (!strcmp(fname, bufname)) {
		if (pconf->debug)
		    printf("Replacing file %s.\n", fname);
		/* Delete all resources associated with this file. */
		{
		    int rn;

		    for (rn = 0; rn < NUM_RESOURCE_LISTS; ++rn) {
			string_item_t *items = pconf->lists.indexed[rn].items;
			int count = pconf->lists.indexed[rn].count;
			int tn;

			for (tn = 0; tn < count; ++tn) {
			    if (items[tn].file_index == j) {
				/*
				 * Delete the item.  Since we haven't sorted
				 * the items yet, just replace this item
				 * with the last one, but make sure we don't
				 * miss scanning it.
				 */
				if (pconf->debug)
				    printf("Replacing %s %s.\n",
					 pconf->lists.indexed[rn].list_name,
					   items[tn].str);
				items[tn--] = items[--count];
			    }
			}
			pconf->lists.indexed[rn].count = count;
		    }
		}
		pconf->file_names.items[j].str = "";
		break;
	    }
	}
    }
    /* Don't process the replaces again. */
    pconf->replaces.count = 0;
    return 0;
}

/*
 * Read an entire file into memory.
 * We use the 'index' of the file_contents string_item_t to record the union
 * of the uniq_mode_ts of all (direct and indirect) items in the file.
 * Return the file_contents item for the file.
 */
private string_item_t *
read_file(config_t * pconf, const char *fname)
{
    char *cname = malloc(strlen(fname) + strlen(pconf->file_prefix) + 1);
    int i;
    FILE *in;
    int end, nread;
    char *cont;
    string_item_t *item;

    if (cname == 0) {
	fprintf(stderr, "Can't allocate space for file name %s%s.\n",
		pconf->file_prefix, fname);
	exit(1);
    }
    strcpy(cname, pconf->file_prefix);
    strcat(cname, fname);
    for (i = 0; i < pconf->file_names.count; ++i)
	if (!strcmp(pconf->file_names.items[i].str, cname)) {
	    free(cname);
	    return &pconf->file_contents.items[i];
	}
    /* Try to open the file in binary mode, to avoid the overhead */
    /* of unnecessary EOL conversion in the C library. */
    in = fopen(cname, "rb");
    if (in == 0) {
	in = fopen(cname, "r");
	if (in == 0) {
	    fprintf(stderr, "Can't read %s.\n", cname);
	    exit(1);
	}
    }
    fseek(in, 0L, 2 /*SEEK_END */ );
    end = ftell(in);
    cont = malloc(end + 1);
    if (cont == 0) {
	fprintf(stderr, "Can't allocate %d bytes to read %s.\n",
		end + 1, cname);
	exit(1);
    }
    rewind(in);
    nread = fread(cont, 1, end, in);
    fclose(in);
    cont[nread] = 0;
    if (pconf->debug)
	printf("File %s = %d bytes.\n", cname, nread);
    add_item(&pconf->file_names, cname, -1);
    item = add_item(&pconf->file_contents, cont, -1);
    item->index = 0;		/* union of uniq_mode_ts */
    return item;
}

/* Read and parse a .dev file.  Return the union of all its uniq_mode_ts. */
int
read_dev(config_t * pconf, const char *arg)
{
    string_item_t *item;
    const char *in;

#define MAX_TOKEN 256
    char *token = malloc(MAX_TOKEN + 1);
    char *category = malloc(MAX_TOKEN + 1);
    int file_index;
    int len;

    if (pconf->debug)
	printf("Reading %s;\n", arg);
    item = read_file(pconf, arg);
    if (item->index == uniq_first) {	/* Don't need to read the file again. */
	if (pconf->debug)
	    printf("Skipping duplicate file.\n");
	return uniq_first;
    }
    in = item->str;
    file_index = item - pconf->file_contents.items;
    strcpy(category, "obj");
    while ((len = read_token(token, MAX_TOKEN, &in)) > 0)
	item->index |= add_entry(pconf, category, token, file_index);
    free(category);
#undef MAX_TOKEN
    if (len < 0) {
	fprintf(stderr, "Token too long: %s.\n", token);
	exit(1);
    }
    if (pconf->debug)
	printf("Finished %s.\n", arg);
    free(token);
    return item->index;
}

/* Read a token from a string that contains the contents of a file. */
int
read_token(char *token, int max_len, const char **pin)
{
    const char *in = *pin;
    int len = 0;

    while (len < max_len) {
	char ch = *in;

	if (ch == 0)
	    break;
	++in;
	if (isspace(ch)) {
	    if (len > 0)
		break;
	    continue;
	}
	token[len++] = ch;
    }
    token[len] = 0;
    *pin = in;
    return (len >= max_len ? -1 /* token too long */ : len);
}

/* Add an entry to a configuration.  Return its uniq_mode_t. */
int
add_entry(config_t * pconf, char *category, const char *item, int file_index)
{
    if (item[0] == '-' && islower(item[1])) {	/* set category */
	strcpy(category, item + 1);
	return 0;
    } else {			/* add to current category */
	char str[MAX_STR];
	char template[80];
	const char *pat = 0;
	string_list_t *list = &pconf->lists.named.resources;

	if (pconf->debug)
	    printf("Adding %s %s;\n", category, item);
	/* Handle a few resources specially; just queue the rest. */
	switch (category[0]) {
#define IS_CAT(str) !strcmp(category, str)
	    case 'c':
		if (!IS_CAT("comp"))
		    goto err;
		pat = "compositor_(%scomposite_%%s_type)";
		list = &pconf->lists.named.compositors;
		goto pre;
	    case 'd':
		if (IS_CAT("dev"))
		    pat = "device_(%s%%s_device)";
		else if (IS_CAT("dev2"))
		    pat = "device2_(%s%%s_device)";
		else
		    goto err;
		list = &pconf->lists.named.devs;
pre:		sprintf(template, pat, pconf->name_prefix);
		pat = template;
		break;
	    case 'e':
		if (IS_CAT("emulator")) {
		    sprintf(str, "emulator_(\"%s\",%u)",
			    item, (uint)strlen(item));
		    item = str;
		    break;
		}
		goto err;
	    case 'f':
		if (IS_CAT("font")) {
		    list = &pconf->lists.named.fonts;
		    break;
		} else if (IS_CAT("functiontype")) {
		    pat = "function_type_(%%s,%sbuild_function_%%s)";
		} else
		    goto err;
		goto pre;
	    case 'h':
		if (IS_CAT("halftone")) {
		    pat = "halftone_(%sdht_%%s)";
		} else
		    goto err;
		goto pre;
	    case 'i':
		if (IS_CAT("imageclass")) {
		    list = &pconf->lists.named.sorted_resources;
		    pat = "image_class_(%simage_class_%%s)";
		} else if (IS_CAT("imagetype")) {
		    pat = "image_type_(%%s,%simage_type_%%s)";
		} else if (IS_CAT("include")) {
		    strcpy(str, item);
		    dev_file_name(str);
		    return read_dev(pconf, str);
		} else if (IS_CAT("init")) {
		    pat = "init_(%s%%s_init)";
		} else if (IS_CAT("iodev")) {
		    pat = "io_device_(%siodev_%%s)";
		} else
		    goto err;
		goto pre;
	    case 'l':
		if (IS_CAT("lib")) {
		    list = &pconf->lists.named.libs;
		    break;
		} else if (IS_CAT("libpath")) {
		    list = &pconf->lists.named.libpaths;
		    break;
		} else if (IS_CAT("link")) {
		    list = &pconf->lists.named.links;
		    break;
		}
		goto err;
	    case 'o':
		if (IS_CAT("obj")) {
		    list = &pconf->lists.named.objs;
		    strcpy(template, pconf->file_prefix);
		    strcat(template, "%s");
		    pat = template;
		    break;
		}
		if (IS_CAT("oper")) {
		    pat = "oper_(%s_op_defs)";
		    break;
		}
		goto err;
	    case 'p':
		if (IS_CAT("ps")) {
		    sprintf(str, "psfile_(\"%s.ps\",%u)",
			    item, (uint)(strlen(item) + 3));
		    item = str;
		    break;
		} else if (IS_CAT("plugin")) {
		    pat = "plugin_(%s%%s_instantiate)";
                    goto pre;
                }
		goto err;
	    case 'r':
		if (IS_CAT("replace")) {
		    list = &pconf->replaces;
		    break;
		}
		goto err;
#undef IS_CAT
	    default:
err:		fprintf(stderr, "Definition not recognized: %s %s.\n",
			category, item);
		exit(1);
	}
	if (pat) {
	    sprintf(str, pat, item, item);
	    assert(strlen(str) < MAX_STR);
	    add_item(list, str, file_index);
	} else
	    add_item(list, item, file_index);
	return list->mode;
    }
}

/* Add an item to a list. */
string_item_t *
add_item(string_list_t * list, const char *str, int file_index)
{
    char *rstr = malloc(strlen(str) + 1);
    int count = list->count;
    string_item_t *item;

    if (count >= list->max_count) {
	list->max_count <<= 1;
	if (list->max_count < 20)
	    list->max_count = 20;
	list->items =
	    (string_item_t *) mrealloc(list->items,
				     (list->max_count >> 1) *
				     sizeof(string_item_t),
				     list->max_count *
				     sizeof(string_item_t));
	assert(list->items != NULL);
    }
    item = &list->items[count];
    item->str = rstr;
    item->index = count;
    item->file_index = file_index;
    strcpy(rstr, str);
    list->count++;
    return item;
}

/*
 * Remove duplicates from a list of string_item_ts.
 * In case of duplicates, remove all but the earliest (if last = false)
 * or the latest (if last = true).
 */
private int
cmp_index(const void *p1, const void *p2)
{
    const string_item_t *const psi1 = (const string_item_t *)p1;
    const string_item_t *const psi2 = (const string_item_t *)p2;
    int cmp = psi1->index - psi2->index;

    return (cmp < 0 ? -1 : cmp > 0 ? 1 : 0);
}
private int
cmp_str(const void *p1, const void *p2)
{
    const string_item_t *const psi1 = (const string_item_t *)p1;
    const string_item_t *const psi2 = (const string_item_t *)p2;

    return strcmp(psi1->str, psi2->str);
}
void
sort_uniq(string_list_t * list, bool by_index)
{
    string_item_t *strlist = list->items;
    int count = list->count;
    const string_item_t *from;
    string_item_t *to;
    int i;
    bool last = list->mode == uniq_last;

    if (count == 0)
	return;
    qsort((char *)strlist, count, sizeof(string_item_t), cmp_str);
    for (from = to = strlist + 1, i = 1; i < count; from++, i++)
	if (strcmp(from->str, to[-1].str))
	    *to++ = *from;
	else if ((last ? from->index > to[-1].index :
		  from->index < to[-1].index)
	    )
	    to[-1] = *from;
    count = to - strlist;
    list->count = count;
    if (by_index)
	qsort((char *)strlist, count, sizeof(string_item_t), cmp_index);
}

/* Write a list of strings using a template. */
void
write_list(FILE * out, const string_list_t * list, const char *pstr)
{
    string_pattern_t pat;

    pat.upper_case = false;
    pat.drop_extn = false;
    strcpy(pat.pattern, pstr);
    write_list_pattern(out, list, &pat);
}
void
write_list_pattern(FILE * out, const string_list_t * list,
		   const string_pattern_t * pat)
{
    int i;
    char macname[40];
    int plen = strlen(pat->pattern);

    *macname = 0;
    for (i = 0; i < list->count; i++) {
	const char *lstr = list->items[i].str;
	int len = strlen(lstr);
	char *str = malloc(len + 1);
	int xlen = plen + len * 3;
	char *xstr = malloc(xlen + 1);
	char *alist;

	strcpy(str, lstr);
	if (pat->drop_extn) {
	    char *dot = str + len;

	    while (dot > str && *dot != '.')
		dot--;
	    if (dot > str)
		*dot = 0, len = dot - str;
	}
	if (pat->upper_case) {
	    char *ptr = str;

	    for (; *ptr; ptr++)
		if (islower(*ptr))
		    *ptr = toupper(*ptr);
	}
	/* We repeat str for the benefit of patterns that */
	/* need the argument substituted in more than one place. */
	sprintf(xstr, pat->pattern, str, str, str);
	/* Check to make sure the item is within the scope of */
	/* an appropriate #ifdef, if necessary. */
	alist = strchr(xstr, '(');
	if (alist != 0 && alist != xstr && alist[-1] == '_') {
	    *alist = 0;
	    if (strcmp(xstr, macname)) {
		if (*macname)
		    fputs("#endif\n", out);
		fprintf(out, "#ifdef %s\n", xstr);
		strcpy(macname, xstr);
	    }
	    *alist = '(';
	} else {
	    if (*macname) {
		fputs("#endif\n", out);
		*macname = 0;
	    }
	}
	fputs(xstr, out);
	free(xstr);
	free(str);
    }
    if (*macname)
	fputs("#endif\n", out);
}
