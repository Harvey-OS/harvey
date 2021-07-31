/* setup file */

#ifdef _POSIX_SOURCE
#define HAVE_SGTTY 0
#else
#ifndef NO_SGTTY
#define HAVE_SGTTY 1			/* host has sgtty.h and ioctl.h */
#endif
#endif

#include        <stdio.h>
#include        <ctype.h>
#ifdef _POSIX_SOURCE
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	<time.h>
#else
#ifdef MSC
#include        <string.h>
#include	<stdlib.h>	/* for type declarations */
#include	<io.h>		/* for type declarations */
#else
#include        <strings.h>
#endif
#endif

#if HAVE_SGTTY
#include        <sys/ioctl.h>
#include        <sgtty.h>
#endif

#define	MAXLEN	65535		/* maximum length of document */
#define	MAXWORD	250		/* maximum word length */
#define	MAXLINE	500		/* maximum line length */
#define	MAXDEF	200		/* maximum number of defines */

#ifndef _POSIX_SOURCE
extern char *malloc();
#endif
#ifdef IN_TR		/* can only declare globals once */
#else
extern
#endif
int math_mode,		/* math mode status */
    de_arg,		/* .de argument */
    IP_stat,		/* IP status */
    QP_stat,		/* QP status */
    TP_stat;		/* TP status */

#ifdef IN_TR		/* can only declare globals once */
#else
extern
#endif
struct defs {
	char *def_macro;
	char *replace;
	int illegal;
} def[MAXDEF];

#ifdef IN_TR		/* can only declare globals once */
#else
extern
#endif
struct mydefs {
	char *def_macro;
	char *replace;
	int illegal;
	int arg_no;
	int par;		/* if it impiles (or contains) a par break */
} mydef[MAXDEF];

#ifdef IN_TR		/* can only declare globals once */
#else
extern
#endif
struct measure {
	char old_units[MAXWORD];	float old_value;
	char units[MAXWORD];		float value;
	char def_units[MAXWORD];	/* default units */
	int def_value;			/* default value: 0 means take last one */
} linespacing, indent, tmpind, space, vspace;

char*	alternate(char*, char*, char*);
int	CAP_GREEK(char*);
char*	do_table(char*,char*,int*);
char*	end_env(char*);
void	envoke_stat(int);
char*	flip(char*,char*);
char*	flip_twice(char*,char*,char*);
int	get_arg(char*,char*,int);
void	get_brace_arg(char*,char*);
int	get_defword(char*,char*,int*);
int	get_line(char*,char*,int );
int	get_multi_line(char*,char*);
int	get_mydef(char*,char*);
int	get_N_lines(char *,char *,int);
int	get_no_math(char*,char*);
char*	get_over_arg(char*,char*);
int	get_ref(char*,char*);
int	get_string(char*,char*,int );
void	get_size(char*,struct measure *);
int	get_sub_arg(char*,char*);
int	get_table_entry(char*,char*,int);
int	get_till_space(char*,char*);
int	getdef(char*,char*);
int	getword(char*,char*);
void	GR_to_Greek(char*,char*);
int	is_def(char*);
int	is_flip(char*);
int	is_forbid(char*);
int	is_mathcom(char*,char*);
int	is_mydef(char *);
int	is_troff_mac(char*,char*,int*,int*);
int	main(int ,char **);
void	parse_units(char*,int*,float*,int*);
void	scrbuf(FILE*,FILE*);
int	similar(char*);
char*	skip_line(char*);
int	skip_white(char*);
char*	strapp(char*,char*);
void	tmpbuf(FILE*,char*);
void	troff_tex(char *,char *,int, int);
