/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#define PP 1

#define CWIDTH 8
#define CMASK 0377
#define NCH 256

#define TOKENSIZE 1000
#define DEFSIZE 40
#define DEFCHAR 1000
#define STARTCHAR 100
#define STARTSIZE 256
#define CCLSIZE 1000

#define TREESIZE 1000
#define NSTATES 500
#define MAXPOS 2500
#define NTRANS 2000
#define NOUTPUT 5000

#define NACTIONS 100
#define ALITTLEEXTRA 30

#define RCCL NCH + 90
#define RNCCL NCH + 91
#define RSTR NCH + 92
#define RSCON NCH + 93
#define RNEWE NCH + 94
#define FINAL NCH + 95
#define RNULLS NCH + 96
#define RCAT NCH + 97
#define STAR NCH + 98
#define PLUS NCH + 99
#define QUEST NCH + 100
#define DIV NCH + 101
#define BAR NCH + 102
#define CARAT NCH + 103
#define S1FINAL NCH + 104
#define S2FINAL NCH + 105

#define DEFSECTION 1
#define RULESECTION 2
#define ENDSECTION 5
#define TRUE 1
#define FALSE 0

#define PC 1
#define PS 1

#ifdef DEBUG
#define LINESIZE 110
extern int yydebug;
extern int debug; /* 1 = on */
extern int charc;
#endif

#ifdef DEBUG
extern int freturn(int);
#else
#define freturn(s) s
#endif

extern int sargc;
extern char** sargv;
extern unsigned char buf[520];
extern int yyline;   /* line number of file */
extern char* yyfile; /* file name of file */
extern int sect;
extern int eof;
extern int lgatflg;
extern int divflg;
extern int funcflag;
extern int pflag;
extern int casecount;
extern int chset; /* 1 = char set modified */
extern Biobuf* fin, fout, *fother;
extern int foutopen;
extern int errorf;
extern int fptr;
extern char* cname;
extern int prev; /* previous input character */
extern int pres; /* present input character */
extern int peek; /* next input character */
extern int* name;
extern int* left;
extern int* right;
extern int* parent;
extern unsigned char** ptr;
extern unsigned char* nullstr;
extern int tptr;
extern unsigned char pushc[TOKENSIZE];
extern unsigned char* pushptr;
extern unsigned char slist[STARTSIZE];
extern unsigned char* slptr;
extern unsigned char** def, **subs, *dchar;
extern unsigned char** sname, *stchar;
extern unsigned char* ccl;
extern unsigned char* ccptr;
extern unsigned char* dp, *sp;
extern int dptr, sptr;
extern unsigned char* bptr; /* store input position */
extern unsigned char* tmpstat;
extern int count;
extern int** foll;
extern int* nxtpos;
extern int* positions;
extern int* gotof;
extern int* nexts;
extern unsigned char* nchar;
extern int** state;
extern int* sfall;              /* fallback state num */
extern unsigned char* cpackflg; /* true if state has been character packed */
extern int* atable, aptr;
extern int nptr;
extern unsigned char symbol[NCH];
extern unsigned char cindex[NCH];
extern int xstate;
extern int stnum;
extern int ccount;
extern unsigned char match[NCH];
extern unsigned char extra[NACTIONS];
extern unsigned char* pcptr, *pchar;
extern int pchlen;
extern int nstates, maxpos;
extern int yytop;
extern int report;
extern int ntrans, treesize, outsize;
extern long rcount;
extern int* verify, *advance, *stoff;
extern int scon;
extern unsigned char* psave;

extern void acompute(int);
extern void add(int**, int);
extern void allprint(int);
extern void cclinter(int);
extern void cgoto(void);
extern void cfoll(int);
extern int cpyact(void);
extern int dupl(int);
extern void error(char*, ...);
extern void first(int);
extern void follow(int);
extern int gch(void);
extern unsigned char* getl(unsigned char*);
extern void layout(void);
extern void lgate(void);
extern int lookup(unsigned char*, unsigned char**);
extern int member(int, unsigned char*);
extern void mkmatch(void);
extern int mn0(int);
extern int mn1(int, int);
extern int mnp(int, void*);
extern int mn2(int, int, uintptr);
extern void munputc(int);
extern void munputs(unsigned char*);
extern void* myalloc(int, int);
extern void nextstate(int, int);
extern int notin(int);
extern void packtrans(int, unsigned char*, int*, int, int);
extern void padd(int**, int);
extern void pccl(void);
extern void pfoll(void);
extern void phead1(void);
extern void phead2(void);
extern void pstate(int);
extern void ptail(void);
extern void sect1dump(void);
extern void sect2dump(void);
extern void statistics(void);
extern void stprt(int);
extern void strpt(unsigned char*);
extern void treedump(void);
extern int usescape(int);
extern void warning(char*, ...);
extern int yyparse(void);
extern void yyerror(char*);
