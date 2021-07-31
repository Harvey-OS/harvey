#include <stdio.h>
#include "site.h"

#define	TRUE	1
#define	FALSE	0

#ifndef	BSD
#include <string.h>
#define	rindex	strrchr
#define	index	strchr
#else
#include <strings.h>
#endif

#ifdef SYSV
#define bcopy(s,d,n)	memcpy((d),(s),(n))
#define bcmp(s1,s2,n)	memcmp((s1),(s2),(n))
#define bzero(s,n)	memset((s),0,(n))
#endif

#ifdef __GNUC__
#define alloca __builtin_alloca
#endif

#ifndef	ANSI
#ifdef	SYSV
extern sprintf();
#else
extern char *sprintf();
#endif
#endif

#define max_line_length (78)
#define max_strings (20000)
#define hash_prime (101)
#define sym_table_size (3000)
#define unused (271828)
#define ex_32 (2)
#define ex_real (3)
#define max(a,b) ((a>b)?a:b)

extern int indent;
extern int line_pos;
extern int last_brace;
extern int block_level;
extern int ii;
extern int last_tok;

extern char safe_string[80];
extern char var_list[200];
extern char field_list[200];
extern char last_id[80];
extern char z_id[80];
extern char next_temp[];

extern long last_i_num;
extern int ii, l_s;
extern long lower_bound, upper_bound;
extern FILE *fopen();
extern FILE *std;
extern int pf_count;

#include "symtab.h"

extern char strings[max_strings];
extern int hash_list[hash_prime];
extern short global;
#ifdef	MS_DOS
extern struct sym_entry huge sym_table[sym_table_size];
#else
extern struct sym_entry sym_table[sym_table_size];
#endif
extern int next_sym_free, next_string_free;
extern int mark_sym_free, mark_string_free;

#ifdef	FLEX
extern char *yytext;
#else	/* LEX */
#ifdef	HP
extern unsigned char yytext[];
#else /* Not HP */
extern char yytext[];
#endif	/* HP */
#endif	/* LEX */

#ifdef	ANSI
extern void exit(int);
extern int yyparse(void);
void find_next_temp(void);
void normal(void);
void new_line(void);
void indent_line(void);
void my_output(char *);
void semicolon(void);
void yyerror(char *);
int hash(char *);
int search_table(char *);
int add_to_table(char *);
void remove_locals(void);
void mark(void);
void initialize(void);
void main(int,char * *);
#else
void find_next_temp(), normal(), new_line(), indent_line(), my_output();
void semicolon(), yyerror(), remove_locals(), mark(), initialize();
#endif
