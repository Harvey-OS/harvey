/* Copyright (C) 1993, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: genconf.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Generate configuration files */
#include "stdpre.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>		/* for calloc */
#include <string.h>

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

/*
 * This program generates a set of configuration files.
 * Usage:
 *      genconf [-Z] [-e escapechar] [-n [name_prefix | -]] [@]xxx.dev*
 *        [-f gconfigf.h] [-h gconfig.h]
 *        [-p[l|L][u][e] pattern] [-l|o|lo|ol out.tr]
 * The default escape character is &.  When this character appears in a
 * pattern, it acts as follows:
 *	&p produces a %;
 *	&s produces a space;
 *	&& (i.e., the escape character twice) produces a \;
 *	&- produces a -;
 *	&x, for any other character x, is an error.
 */

#define DEFAULT_NAME_PREFIX "gs_"

#define MAX_STR 120

/* Structures for accumulating information. */
typedef struct string_item_s {
    const char *str;
    int file_index;		/* index of file containing this item */
    int index;
} string_item;

/* The values of uniq_mode are bit masks. */
typedef enum {
    uniq_all = 1,		/* keep all occurrences (default) */
    uniq_first = 2,		/* keep only first occurrence */
    uniq_last = 4		/* keep only last occurrence */
} uniq_mode;
typedef struct string_list_s {
    /* The following are set at creation time. */
    const char *list_name;	/* only for debugging */
    int max_count;
    uniq_mode mode;
    /* The following are updated dynamically. */
    int count;
    string_item *items;
} string_list;

#define max_pattern 60
typedef struct string_pattern_s {
    bool upper_case;
    bool drop_extn;
    char pattern[max_pattern + 1];
} string_pattern;
typedef struct config_s {
    int debug;
    const char *name_prefix;
    const char *file_prefix;
    /* Special "resources" */
    string_list file_names;
    string_list file_contents;
    string_list replaces;
    /* Real resources */
    union ru_ {
	struct nu_ {
	    string_list sorted_resources;
#define c_sorted_resources lists.named.sorted_resources
	    string_list resources;
#define c_resources lists.named.resources
	    string_list devs;	/* also includes devs2 */
#define c_devs lists.named.devs
	    string_list fonts;
#define c_fonts lists.named.fonts
	    string_list libs;
#define c_libs lists.named.libs
	    string_list libpaths;
#define c_links lists.named.links
	    string_list links;
#define c_libpaths lists.named.libpaths
	    string_list objs;
#define c_objs lists.named.objs
	} named;
#define NUM_RESOURCE_LISTS 8
	string_list indexed[NUM_RESOURCE_LISTS];
    } lists;
    string_pattern lib_p;
    string_pattern libpath_p;
    string_pattern obj_p;
} config;

/* These lists grow automatically if needed, so we could start out with */
/* small allocations. */
static const config init_config =
{
    0,				/* debug */
    DEFAULT_NAME_PREFIX,	/* name_prefix */
    "",				/* file_prefix */
    {"file name", 200},		/* file_names */
    {"file contents", 200},	/* file_contents */
    {"-replace", 50}
};
static const string_list init_config_lists[] =
{
    {"resource", 100, uniq_first},
    {"sorted_resource", 20, uniq_first},
    {"-dev", 100, uniq_first},
    {"-font", 50, uniq_first},
    {"-lib", 20, uniq_last},
    {"-libpath", 10, uniq_first},
    {"-link", 10, uniq_first},
    {"-obj", 500, uniq_first}
};

/* Forward definitions */
int alloc_list(P1(string_list *));
void dev_file_name(P1(char *));
int process_replaces(P1(config *));
int read_dev(P2(config *, const char *));
int read_token(P3(char *, int, const char **));
int add_entry(P4(config *, char *, const char *, int));
string_item *add_item(P3(string_list *, const char *, int));
void sort_uniq(P2(string_list *, bool));
void write_list(P3(FILE *, const string_list *, const char *));
void write_list_pattern(P3(FILE *, const string_list *, const string_pattern *));
bool var_expand(P3(char *, char [MAX_STR], const config *));
void add_definition(P4(const char *, const char *, string_list *, bool));
string_item *lookup(P2(const char *, const string_list *));

int
main(int argc, char *argv[])
{
    config conf;
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
		    string_pattern *pat;

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
		    write_list(out, &conf.c_fonts, template);
		}
		break;
	    case 'h':
		process_replaces(&conf);
		fputs("/* This file was generated automatically by genconf.c. */\n", out);
		write_list(out, &conf.c_devs, "%s\n");
		sort_uniq(&conf.c_resources, true);
		write_list(out, &conf.c_resources, "%s\n");
		sort_uniq(&conf.c_sorted_resources, false);
		write_list(out, &conf.c_sorted_resources, "%s\n");
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
		    sort_uniq(&conf.c_objs, true);
		    write_list_pattern(out, &conf.c_objs, &conf.obj_p);
		}
		if (lib) {
		    sort_uniq(&conf.c_libs, true);
		    sort_uniq(&conf.c_links, true);
		    write_list_pattern(out, &conf.c_libpaths, &conf.libpath_p);
		    write_list_pattern(out, &conf.c_links, &conf.obj_p);
		    write_list_pattern(out, &conf.c_libs, &conf.lib_p);
		}
		break;
	    default:
		fclose(out);
		fprintf(stderr, "Unknown switch %s.\n", argv[i]);
		exit(1);
	}
	fclose(out);
    }

    exit(0);
}

/* Allocate and initialize a string list. */
int
alloc_list(string_list * list)
{
    list->count = 0;
    list->items =
	(string_item *) calloc(list->max_count, sizeof(string_item));
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
process_replaces(config * pconf)
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
			string_item *items = pconf->lists.indexed[rn].items;
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

/* Read an entire file into memory. */
/* We use the 'index' of the file_contents string_item to record the union */
/* of the uniq_modes of all (direct and indirect) items in the file. */
/* Return the file_contents item for the file. */
private string_item *
read_file(config * pconf, const char *fname)
{
    char *cname = malloc(strlen(fname) + strlen(pconf->file_prefix) + 1);
    int i;
    FILE *in;
    int end, nread;
    char *cont;
    string_item *item;

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
    item->index = 0;		/* union of uniq_modes */
    return item;
}

/* Read and parse a .dev file. */
/* Return the union of all its uniq_modes. */
int
read_dev(config * pconf, const char *arg)
{
    string_item *item;
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

/* Add an entry to a configuration. */
/* Return its uniq_mode. */
int
add_entry(config * pconf, char *category, const char *item, int file_index)
{
    if (item[0] == '-' && islower(item[1])) {	/* set category */
	strcpy(category, item + 1);
	return 0;
    } else {			/* add to current category */
	char str[MAX_STR];
	char template[80];
	const char *pat = 0;
	string_list *list = &pconf->c_resources;

	if (pconf->debug)
	    printf("Adding %s %s;\n", category, item);
	/* Handle a few resources specially; just queue the rest. */
	switch (category[0]) {
#define IS_CAT(str) !strcmp(category, str)
	    case 'd':
		if (IS_CAT("dev"))
		    pat = "device_(%s%%s_device)";
		else if (IS_CAT("dev2"))
		    pat = "device2_(%s%%s_device)";
		else
		    goto err;
		list = &pconf->c_devs;
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
		    list = &pconf->c_fonts;
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
		    list = &pconf->c_sorted_resources;
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
		    list = &pconf->c_libs;
		    break;
		} else if (IS_CAT("libpath")) {
		    list = &pconf->c_libpaths;
		    break;
		} else if (IS_CAT("link")) {
		    list = &pconf->c_links;
		    break;
		}
		goto err;
	    case 'o':
		if (IS_CAT("obj")) {
		    list = &pconf->c_objs;
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
string_item *
add_item(string_list * list, const char *str, int file_index)
{
    char *rstr = malloc(strlen(str) + 1);
    int count = list->count;
    string_item *item;

    if (count >= list->max_count) {
	list->max_count <<= 1;
	if (list->max_count < 20)
	    list->max_count = 20;
	list->items =
	    (string_item *) mrealloc(list->items,
				     (list->max_count >> 1) *
				     sizeof(string_item),
				     list->max_count *
				     sizeof(string_item));
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

/* Remove duplicates from a list of string_items. */
/* In case of duplicates, remove all but the earliest (if last = false) */
/* or the latest (if last = true). */
#define psi1 ((const string_item *)p1)
#define psi2 ((const string_item *)p2)
private int
cmp_index(const void *p1, const void *p2)
{
    int cmp = psi1->index - psi2->index;

    return (cmp < 0 ? -1 : cmp > 0 ? 1 : 0);
}
private int
cmp_str(const void *p1, const void *p2)
{
    return strcmp(psi1->str, psi2->str);
}
#undef psi1
#undef psi2
void
sort_uniq(string_list * list, bool by_index)
{
    string_item *strlist = list->items;
    int count = list->count;
    const string_item *from;
    string_item *to;
    int i;
    bool last = list->mode == uniq_last;

    if (count == 0)
	return;
    qsort((char *)strlist, count, sizeof(string_item), cmp_str);
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
	qsort((char *)strlist, count, sizeof(string_item), cmp_index);
}

/* Write a list of strings using a template. */
void
write_list(FILE * out, const string_list * list, const char *pstr)
{
    string_pattern pat;

    pat.upper_case = false;
    pat.drop_extn = false;
    strcpy(pat.pattern, pstr);
    write_list_pattern(out, list, &pat);
}
void
write_list_pattern(FILE * out, const string_list * list, const string_pattern * pat)
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
