#include <stdio.h>

#ifdef PLAN9
#define exit(x) exits(x ? "abnormal exit" : (char *) 0)
#endif

/*
 * i/o
 */

#define	INUSE 01
#define	NEEDED 02
#define	DEFINED 04

FILE *fi;		/* input file(s) */

/*
 * symbol table
 */
#define HSHSIZ 419	/* a prime */
#define NCHARS 23
struct hshtab {
	char name[NCHARS];
	char flag;
	struct hshtab *frwd;
	struct hshtab *bkwd;
};
struct hshtab hshtab[HSHSIZ];
struct hshtab *lookup(char *);
int rline(void);
void parse(void);
void diag(void);
 
extern char	line[];
extern char	*fname;
extern char	*largv[];
extern int	largc;
extern int	nline;

