/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* genconf.c */
/* Generate configuration files */
#include "stdpre.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>		/* for calloc */
#include <string.h>

/*
 * This program generates a set of configuration files.
 * Almost everything it does could be done by a shell program, except that
 * (1) Unix shells are not compatible from one system to another,
 * (2) the DOS shell is not equal to the task,
 * (3) the VMS shell is radically different from all others.
 *
 * Usage:
 *	genconf [-Z] [@]xxx.dev* [-f gconfigf.h] [-h gconfig.h]
 *	  [-p[l|L][u][e] pattern] [-l|o|lo|ol out.tr]
 * &s in a pattern produces a space; && produces a \; &x, for any other
 * character x, produces x.
 */

/* This program has to use K&R C, so it will work with old compilers. */
/* Unfortunately, we have to hack up the function definitions by hand. */

/* Structures for accumulating information */
typedef struct string_item_s {
	char *str;
	int index;
} string_item;
typedef struct string_list_s {
	int max_count, count;
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
	string_list resources;
	string_list devs;
	string_list fonts;
	string_list headers;
	string_list libs;
	string_list libpaths;
	string_list objs;
	string_pattern lib_p;
	string_pattern libpath_p;
	string_pattern obj_p;
} config;
/* We allocate all of these with small sizes, since */
/* we can reallocate them later if needed. */
static const config init_config = {
	0, { 20 }, { 20 }, { 50 }, { 20 }, { 10 }, { 10 }, { 100 }
};

/* Forward definitions */
int alloc_list(P1(string_list *));
void parse_affix(P2(char *, const char *));
int read_dev(P2(config *, const char *));
int read_token(P3(char *, int, FILE *));
int add_entry(P3(config *, char *, const char *));
void sort_uniq(P1(string_list *));
void write_list(P3(FILE *, const string_list *, const char *));
void write_list_pattern(P3(FILE *, const string_list *, const string_pattern *));

main(argc, argv) int argc; char *argv[];
{	config conf;
	int i;

	/* Allocate string lists. */
	conf = init_config;
	alloc_list(&conf.resources);
	alloc_list(&conf.devs);
	alloc_list(&conf.fonts);
	alloc_list(&conf.headers);
	alloc_list(&conf.libs);
	alloc_list(&conf.libpaths);
	alloc_list(&conf.objs);

	/* Initialize patterns. */
	conf.lib_p.upper_case = false;
	conf.lib_p.drop_extn = false;
	strcpy(conf.lib_p.pattern, "%s\n");
	conf.libpath_p = conf.lib_p;
	conf.obj_p = conf.lib_p;

	/* Process command line arguments. */
	for ( i = 1; i < argc; i++ )
	{	const char *arg = argv[i];
		FILE *out;
		int lib = 0, obj = 0;
		if ( *arg != '-' )
		{	read_dev(&conf, arg);
			continue;
		}
		if ( i == argc - 1 )
		{	fprintf(stderr, "Missing argument after %s.\n",
				arg);
			exit(1);
		}
		switch ( arg[1] )
		{
		case 'p':
		{	string_pattern *pat;
			switch ( *(arg += 2) )
			{
			case 'l': pat = &conf.lib_p; break;
			case 'L': pat = &conf.libpath_p; break;
			default: pat = &conf.obj_p; arg--;
			}
			pat->upper_case = false;
			pat->drop_extn = false;
			if ( argv[i + 1][0] == '-' )
				strcpy(pat->pattern, "%s\n");
			else
			{	char *p, *q;
				for ( p = pat->pattern, q = argv[++i];
				      (*p++ = *q++) != 0;
				    )
				 if ( p[-1] == '&' )
				  switch ( *q )
				{
				case 's': p[-1] = ' '; q++; break;
				case '&': p[-1] = '\\'; q++; break;
				default: p--;
				}
				p[-1] = '\n';
				*p = 0;
			}
			for ( ; ; )
			  switch ( *++arg )
			{
			case 'u': pat->upper_case = true; break;
			case 'e': pat->drop_extn = true; break;
			case 0: goto pbreak;
			default:
				fprintf(stderr, "Unknown switch %s.\n", arg);
				exit(1);
			}
pbreak:			if ( pat == &conf.obj_p )
			{	conf.lib_p = *pat;
				conf.libpath_p = *pat;
			}
			continue;
		case 'Z':
			conf.debug = 1;
			continue;
		}
		}
		/* Must be an output file. */
		out = fopen(argv[++i], "w");
		if ( out == 0 )
		{	fprintf(stderr, "Can't open %s for output.\n",
				argv[i]);
			exit(1);
		}
		switch ( arg[1] )
		{
		case 'f':
			fputs("/* This file was generated automatically by genconf.c. */\n", out);
			fputs("/* For documentation, see gsconfig.c. */\n", out);
			write_list(out, &conf.fonts,
				   "font_(\"0.font_%s\",gsf_%s,zf_%s)\n");
			break;
		case 'h':
			fputs("/* This file was generated automatically by genconf.c. */\n", out);
			write_list(out, &conf.devs, "device_(gs_%s_device)\n");
			sort_uniq(&conf.resources);
			write_list(out, &conf.resources, "%s\n");
			write_list(out, &conf.headers, "#include \"%s\"\n");
			break;
		case 'l':
			lib = 1;
			obj = arg[2] == 'o';
			goto lo;
		case 'o':
			obj = 1;
			lib = arg[2] == 'l';
lo:			if ( obj )
			{	sort_uniq(&conf.objs);
				write_list_pattern(out, &conf.objs, &conf.obj_p);
			}
			if ( lib )
			{	write_list_pattern(out, &conf.libpaths, &conf.libpath_p);
				write_list_pattern(out, &conf.libs, &conf.lib_p);
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
alloc_list(list) string_list *list;
{	list->count = 0;
	list->items =
	  (string_item *)calloc(list->max_count, sizeof(string_item));
	assert(list->items != NULL);
	return 0;
}

/* Read and parse a .dev file. */
int
read_dev(pconf, arg) config *pconf; const char *arg;
{	FILE *in;
#define max_token 256
	char *token = malloc(max_token + 1);
	int len;
	if ( pconf->debug )
		printf("Reading %s;\n", arg);
	if ( arg[0] == '@' )
	{	in = fopen(arg + 1, "r");
		if ( in == 0 )
		{	fprintf(stderr, "Can't read %s.\n", arg + 1);
			exit(1);
		}
		while ( (len = read_token(token, max_token, in)) > 0 )
			read_dev(pconf, token);
	}
	else
	{	char *category = malloc(max_token + 1);
		in = fopen(arg, "r");
		if ( in == 0 )
		{	fprintf(stderr, "Can't read %s.\n", arg);
			exit(1);
		}
		strcpy(category, "obj");
		while ( (len = read_token(token, max_token, in)) > 0 )
			add_entry(pconf, category, token);
		free(category);
	}
#undef max_token
	fclose(in);
	if ( len < 0 )
	{	fprintf(stderr, "Token too long: %s.\n", token);
		exit(1);
	}
	if ( pconf->debug )
		printf("Finished %s.\n", arg);
	free(token);
	return 0;
}

/* Read a token from a file. */
int
read_token(token, max_len, in) char *token; int max_len; FILE *in;
{	int len = 0;
	while ( len < max_len )
	{	char ch = fgetc(in);
		token[len] = 0;
		if ( feof(in) )
			return len;
		if ( isspace(ch) )
		{	if ( len > 0 )
				return len;
			continue;
		}
		token[len++] = ch;
	}
	return -1;	/* token too long */
}

/* Add an entry to a configuration. */
int
add_entry(pconf, category, item) config *pconf; char *category; const char *item;
{	if ( item[0] == '-' )		/* set category */
		strcpy(category, item + 1);
	else				/* add to current category */
	{
#define max_str 120
		char str[max_str];
		const char *pat = "%s";
		char *rstr;
		string_list *list = &pconf->resources;
		int count;
		if ( pconf->debug )
			printf("Adding %s %s;\n", category, item);
		/* Handle a few resources specially: */
		if ( !strcmp(category, "dev") )
			list = &pconf->devs;
		else if ( !strcmp(category, "font") )
			list = &pconf->fonts;
		else if ( !strcmp(category, "header") )
			list = &pconf->headers;
		else if ( !strcmp(category, "include") )
		{	strcpy(str, item);
			strcat(str, ".dev");
			read_dev(pconf, str);
			return 0;
		}
		else if ( !strcmp(category, "lib") )
			list = &pconf->libs;
		else if ( !strcmp(category, "libpath") )
			list = &pconf->libpaths;
		else if ( !strcmp(category, "obj") )
			list = &pconf->objs;
		/* Now handle all other resources. */
		else if ( !strcmp(category, "iodev") )
			pat = "io_device_(gs_iodev_%s)";
		else if ( !strcmp(category, "oper") )
			pat = "oper_(%s_op_defs)";
		else if ( !strcmp(category, "ps") )
			pat = "psfile_(\"%s.ps\")";
		else
		{	fprintf(stderr, "Unknown category %s.\n", category);
			exit(1);
		}
		sprintf(str, pat, item);
		assert(strlen(str) < max_str);
		rstr = malloc(strlen(str) + 1);
		strcpy(rstr, str);
		count = list->count;
		if ( count >= list->max_count )
		  {	list->max_count <<= 1;
			list->items =
			  (string_item *)realloc(list->items,
						 list->max_count *
						 sizeof(string_item));
			assert(list->items != NULL);
		  }
		list->items[count].index = count;
		list->items[count].str = rstr;
		list->count++;
	}
	return 0;
}

/* Sort and uniq an array of string items. */
/* In case of duplicates, remove all but the earliest. */
#define psi1 ((const string_item *)p1)
#define psi2 ((const string_item *)p2)
int
cmp_index(p1, p2) const void *p1; const void *p2;
{	int cmp = psi1->index - psi2->index;
	return (cmp < 0 ? -1 : cmp > 0 ? 1 : 0);
}
int
cmp_str(p1, p2) const void *p1; const void *p2;
{	int cmp = strcmp(psi1->str, psi2->str);
	return (cmp != 0 ? cmp : cmp_index(p1, p2));
}
#undef psi1
#undef psi2
void
sort_uniq(list) string_list *list;
{	string_item *strlist = list->items;
	int count = list->count;
	const string_item *from;
	string_item *to;
	int i;
	if ( count == 0 )
		return;
	qsort((char *)strlist, count, sizeof(string_item), cmp_str);
	for ( from = to = strlist + 1, i = 1; i < count; from++, i++ )
	  if ( strcmp(from->str, to[-1].str) )
	    *to++ = *from;
	count = to - strlist;
	list->count = count;
	qsort((char *)strlist, count, sizeof(string_item), cmp_index);
}

/* Write a list of strings using a template. */
void
write_list(out, list, pstr) FILE *out; const string_list *list; const char *pstr;
{	string_pattern pat;
	pat.upper_case = false;
	pat.drop_extn = false;
	strcpy(pat.pattern, pstr);
	write_list_pattern(out, list, &pat);
}
void
write_list_pattern(out, list, pat) FILE *out; const string_list *list; const string_pattern *pat;
{	int i;
	char macname[40];
	int plen = strlen(pat->pattern);
	*macname = 0;
	for ( i = 0; i < list->count; i++ )
	{	const char *lstr = list->items[i].str;
		int len = strlen(lstr);
		char *str = malloc(len + 1);
		int xlen = plen + len * 3;
		char *xstr = malloc(xlen + 1);
		char *alist;
		strcpy(str, lstr);
		if ( pat->drop_extn )
		{	char *dot = str + len;
			while ( dot > str && *dot != '.' ) dot--;
			if ( dot > str ) *dot = 0, len = dot - str;
		}
		if ( pat->upper_case )
		{	char *ptr = str;
			for ( ; *ptr; ptr++ )
			  if ( islower(*ptr) ) *ptr = toupper(*ptr);
		}
		/* We repeat str for the benefit of patterns that */
		/* need the argument substituted in more than one place. */
		sprintf(xstr, pat->pattern, str, str, str);
		/* Check to make sure the item is within the scope of */
		/* an appropriate #ifdef, if necessary. */
		alist = strchr(xstr, '(');
		if ( alist != 0 && alist != xstr && alist[-1] == '_' )
		{	*alist = 0;
			if ( strcmp(xstr, macname) )
			{	if (*macname )
					fputs("#endif\n", out);
				fprintf(out, "#ifdef %s\n", xstr);
				strcpy(macname, xstr);
			}
			*alist = '(';
		}
		else
		{	if ( *macname )
			{	fputs("#endif\n", out);
				*macname = 0;
			}
		}
		fputs(xstr, out);
		free(xstr);
		free(str);
	}
	if ( *macname )
		fputs("#endif\n", out);
}
