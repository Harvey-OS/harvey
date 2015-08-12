/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void addrule(char*, Word*, char*, Word*, int, int, char*);
void addrules(Word*, Word*, char*, int, int, char*);
void addw(Word*, char*);
int assline(Biobuf*, Bufblock*);
uint32_t atimeof(int, char*);
void atouch(char*);
void bufcpy(Bufblock*, char*, int);
Envy* buildenv(Job*, int);
void catchnotes(void);
char* charin(char*, char*);
int chgtime(char*);
void clrmade(Node*);
char* copyq(char*, Rune, Bufblock*);
void delete(char*);
void delword(Word*);
int dorecipe(Node*);
void dumpa(char*, Arc*);
void dumpj(char*, Job*, int);
void dumpn(char*, Node*);
void dumpr(char*, Rule*);
void dumpv(char*);
void dumpw(char*, Word*);
int escapetoken(Biobuf*, Bufblock*, int, int);
void execinit(void);
int execsh(char*, char*, Bufblock*, Envy*);
void Exit(void);
char* expandquote(char*, Rune, Bufblock*);
void expunge(int, char*);
void freebuf(Bufblock*);
void front(char*);
Node* graph(char*);
void growbuf(Bufblock*);
void initenv(void);
void insert(Bufblock*, int);
void ipop(void);
void ipush(void);
void killchildren(char*);
void* Malloc(int);
char* maketmp(void);
int match(char*, char*, char*);
void mk(char*);
uint32_t mkmtime(char*, int);
uint32_t mtime(char*);
Arc* newarc(Node*, Rule*, char*, Resub*);
Bufblock* newbuf(void);
Job* newjob(Rule*, Node*, char*, char**, Word*, Word*, Word*, Word*);
Word* newword(char*);
int nextrune(Biobuf*, int);
int nextslot(void);
void nproc(void);
void nrep(void);
int outofdate(Node*, Arc*, int);
void parse(char*, int, int);
int pipecmd(char*, Envy*, int*);
void prusage(void);
void rcopy(char**, Resub*, int);
void readenv(void);
void* Realloc(void*, int);
void rinsert(Bufblock*, Rune);
char* rulecnt(void);
void run(Job*);
void setvar(char*, void*);
char* shname(char*);
void shprint(char*, Envy*, Bufblock*);
Word* stow(char*);
void subst(char*, char*, char*, int);
void symdel(char*, int);
void syminit(void);
Symtab* symlook(char*, int, void*);
void symstat(void);
void symtraverse(int, void (*)(Symtab*));
void timeinit(char*);
uint32_t timeof(char*, int);
void touch(char*);
void update(int, Node*);
void usage(void);
Word* varsub(char**);
int waitfor(char*);
int waitup(int, int*);
Word* wdup(Word*);
int work(Node*, Node*, Arc*);
char* wtos(Word*, int);
