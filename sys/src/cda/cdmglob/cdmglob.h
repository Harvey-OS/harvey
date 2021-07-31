#include <u.h>
#include <libc.h>
#define Strsame(x,y)	((*(x) == *(y)) && !strcmp ((x), (y)))

#define HASH	257

#define S_IN	1	/* input */
#define S_OUT	2	/* output */
#define S_GLOB	4	/* global */

typedef struct {
	char	*s_name;
	short	s_type;		/* S_IN, etc. */
	short	s_hash;		/* hash chain link */
} Sig;


typedef struct {
	char	*p_name;
	Sig	**p_sigs;	/* signals within macro on pin */
	short	p_nsigs;	/* # of p_sigs */
	short	p_num;		/* optional (0) number */
	short	p_hash;		/* hash chain link */
	char	p_dir;		/* direction: S_IN | S_OUT */
} Pin;


typedef struct CALL	Call;


typedef struct {
	char	*m_name;	/* type name */
	char	*m_file;	/* file name where first defined */
	Pin	*m_pins;	/* connection points */
	Pin	**m_psort;	/* sorted list of pointers into m_pins */
	Sig	*m_sigs;	/* signals used within macro */
	Sig	**m_argv;	/* "formal parameters" */
	Call	*m_calls;	/* what this macro calls */
	Call	**m_chash;	/* instance name hash table */
	short	*m_shash;	/* signal name hash table */
	short	*m_phash;	/* pin name hash table */
	short	m_nsigs;	/* # of m_sigs */
	short	m_npins;	/* # of m_pins */
	short	m_argc;		/* # of m_argv */
	short	m_out;		/* m_argv index of first output (for kdm) */
	char	m_dcl;		/* seen a .t line for this chip */
	char	m_def;		/* seen a .w file for this macro */
	char	m_use;		/* called (or printed) */
	char	m_undef;	/* undefined macro */
} Macro;


struct CALL {
	char	*c_name;	/* instance name */
	Macro	*c_type;	/* macro type instantiated */
	Call	*c_next;	/* links from m_calls */
	Sig	**c_argv;	/* "actual parameters" */
	Call	*c_hash;	/* links from m_chash */
	short	c_argc;		/* # of c_argv, == c_type->m_argc */
	char	c_rename;	/* c_name = first output name (for kdm) */

	/*
	 * Bourne again!  why put the argument list in one place 
	 * when you can splatter it across one or more files?
	 * that way you don't have to check for actual/formal
	 * parameter agreement right away.
	 */
	char	*c_file;
	short	c_lineno;
};


#define alloc(v,n,t)	((v) = (t *) doalloc ((unsigned)(n) * sizeof (t)))

#include "template.h"
