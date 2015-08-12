/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void Abort(void);
void Closedir(int);
int Creat(char *);
int Dup(int, int);
int Dup1(int);
int Eintr(void);
int Executable(char *);
void Execute(word *, word *);
void Exit(char *);
int ForkExecute(char *, char **, int, int, int);
int Globsize(char *);
int Isatty(int);
void Memcpy(void *, void *, int32_t);
void Noerror(void);
int Opendir(char *);
int32_t Read(int, void *, int32_t);
int Readdir(int, void *, int);
int32_t Seek(int, int32_t, int32_t);
void Trapinit(void);
void Unlink(char *);
void Updenv(void);
void Vinit(void);
int Waitfor(int, int);
int32_t Write(int, void *, int32_t);
void addwaitpid(int);
int advance(void);
int back(int);
void cleanhere(char *);
void codefree(code *);
int compile(tree *);
char *list2str(word *);
int count(word *);
void deglob(void *);
void delwaitpid(int);
void dotrap(void);
void freenodes(void);
void freewords(word *);
void globlist(void);
int havewaitpid(int);
int idchr(int);
void inttoascii(char *, int32_t);
void kinit(void);
int mapfd(int);
int match(void *, void *, int);
int matchfn(void *, void *);
char **mkargv(word *);
void clearwaitpids(void);
void panic(char *, int);
void pathinit(void);
void poplist(void);
void popword(void);
void pprompt(void);
void pushlist(void);
void pushredir(int, int, int);
void pushword(char *);
void readhere(void);
word *searchpath(char *);
void setstatus(char *);
void setvar(char *, word *);
void skipnl(void);
void start(code *, int, var *);
int truestatus(void);
void usage(char *);
int wordchr(int);
void yyerror(char *);
int yylex(void);
int yyparse(void);
