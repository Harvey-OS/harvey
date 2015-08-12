/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void _assert(char *);
void accounttime(void);
void acsched(void);
void addbootfile(char *, unsigned char *, uint32_t);
Timer *addclock0link(void (*)(void), int);
int addconsdev(Queue *, void (*fn)(char *, int), int, int);
int addkbdq(Queue *, int);
int addphysseg(Physseg *);
void addwaitstat(uintptr_t pc, uint64_t t0, int type);
void addwatchdog(Watchdog *);
int adec(int *);
Block *adjustblock(Block *, int);
int ainc(int *);
void alarmkproc(void *);
Block *allocb(int);
void *alloczio(Segment *, int32_t);
int anyhigher(void);
int anyready(void);
Image *attachimage(int, Chan *, int, uintptr_t, usize);
Page *auxpage(usize);
Block *bl2mem(unsigned char *, Block *, int);
int blocklen(Block *);
void bootlinks(void);
void cachedel(Image *, uint32_t);
void cachepage(Page *, Image *);
void callwithureg(void (*)(Ureg *));
int canlock(Lock *);
int canpage(Proc *);
int canqlock(QLock *);
int canrlock(RWlock *);
Chan *cclone(Chan *);
void cclose(Chan *);
void ccloseq(Chan *);
void chanfree(Chan *);
char *chanpath(Chan *);
void checkalarms(void);
void checkb(Block *, char *);
void clearwaitstats(void);
void closeegrp(Egrp *);
void closefgrp(Fgrp *);
void closepgrp(Pgrp *);
void closergrp(Rgrp *);
void cmderror(Cmdbuf *, char *);
int cmount(Chan **, Chan *, int, char *);
Block *concatblock(Block *);
void confinit(void);
int consactive(void);
void (*consdebug)(void);
void (*consputs)(char *, int);
Block *copyblock(Block *, int);
void copypage(Page *, Page *);
void cunmount(Chan *, Chan *);
Segment *data2txt(Segment *);
uintptr_t dbgpc(Proc *);
int decrypt(void *, void *, int);
void delay(int);
void delconsdevs(void);
Proc *dequeueproc(Sched *, Schedq *, Proc *);
Chan *devattach(int, char *);
Block *devbread(Chan *, int32_t, int64_t);
int32_t devbwrite(Chan *, Block *, int64_t);
Chan *devclone(Chan *);
int devconfig(int, char *, DevConf *);
void devcreate(Chan *, char *, int, int);
void devdir(Chan *, Qid, char *, int64_t, char *, int32_t, Dir *);
int32_t devdirread(Chan *, char *, int32_t, Dirtab *, int,
		   Devgen *);
Devgen devgen;
void devinit(void);
Chan *devopen(Chan *, int, Dirtab *, int, Devgen *);
void devpermcheck(char *, int, int);
void devpower(int);
void devremove(Chan *);
void devreset(void);
void devshutdown(void);
int32_t devstat(Chan *, unsigned char *, int32_t, Dirtab *, int,
		Devgen *);
Dev *devtabget(int, int);
void devtabinit(void);
int32_t devtabread(Chan *, void *, int32_t, int64_t);
void devtabreset(void);
void devtabshutdown(void);
Walkqid *devwalk(Chan *, Chan *, char **, int, Dirtab *, int, Devgen *);
int32_t devwstat(Chan *, unsigned char *, int32_t);
int devzread(Chan *, Kzio *, int, usize, int64_t);
int devzwrite(Chan *, Kzio *, int, int64_t);
void drawactive(int);
void drawcmap(void);
void dumpaproc(Proc *);
void dumpregs(Ureg *);
void dumpstack(void);
void dumpzseg(Segment *);
Fgrp *dupfgrp(Fgrp *);
int duppage(Page *);
Segment *dupseg(Segment **, int, int);
void dupswap(Page *);
char *edfadmit(Proc *);
void edfinit(Proc *);
int edfready(Proc *);
void edfrecord(Proc *);
void edfrun(Proc *, int);
void edfstop(Proc *);
void edfyield(void);
int elf64ldseg(Chan *c, uintptr_t *entryp, Ldseg **rp, char *mach, uint32_t minpgsz);
int emptystr(char *);
int encrypt(void *, void *, int);
void envcpy(Egrp *, Egrp *);
int eqchanddq(Chan *, int, uint, Qid, int);
int eqqid(Qid, Qid);
void error(char *);
void exhausted(char *);
void exit(int);
uint64_t fastticks(uint64_t *);
uint64_t fastticks2ns(uint64_t);
uint64_t fastticks2us(uint64_t);
int fault(uintptr_t, uintptr_t, int);
void fdclose(int, int);
Chan *fdtochan(int, int, int, int);
int findmount(Chan **, Mhead **, int, uint, Qid);
int fixfault(Segment *, uintptr_t, int, int, int);
void fmtinit(void);
void forceclosefgrp(void);
void free(void *);
void freeb(Block *);
void freeblist(Block *);
int freebroken(void);
void freepte(Segment *, Pte *);
void getcolor(uint32_t, uint32_t *, uint32_t *, uint32_t *);
char *getconfenv(void);
int getpgszi(uint32_t);
Segment *getzkseg(void);
void gotolabel(Label *);
int haswaitq(void *);
void hexdump(void *v, int length);
void pahexdump(uintptr_t pa, int len);
void hnputl(void *, uint);
void hnputs(void *, uint16_t);
void hnputv(void *, uint64_t);
int32_t hostdomainwrite(char *, int32_t);
int32_t hostownerwrite(char *, int32_t);
void hzsched(void);
Block *iallocb(int);
void iallocsummary(void);
void initmark(Watermark *, int, char *);
void ilock(Lock *);
void initimage(void);
int iprint(char *, ...);
void isdir(Chan *);
int iseve(void);
int islo(void);
Segment *isoverlap(Proc *, uintptr_t, usize);
int isphysseg(char *);
void iunlock(Lock *);
void ixsummary(void);
int kbdcr2nl(Queue *, int);
int kbdgetmap(int, int *, int *, Rune *);
int kbdputc(Queue *, int);
void kbdputmap(uint16_t, uint16_t, Rune);
void kickpager(int, int);
void killbig(char *);
void kproc(char *, void (*)(void *), void *);
void kprocchild(Proc *, void (*)(void *), void *);
void (*kproftimer)(uintptr_t);
void ksetenv(char *, char *, int);
void kstrcpy(char *, char *, int);
void kstrdup(char **, char *);
int32_t latin1(Rune *, int);
int lock(Lock *);
void log(Log *, int, char *, ...);
void logclose(Log *);
char *logctl(Log *, int, char **, Logflag *);
void logn(Log *, int, void *, int);
void logopen(Log *);
int32_t logread(Log *, void *, uint32_t, int32_t);
Page *lookpage(Image *, uint32_t);
Cmdtab *lookupcmd(Cmdbuf *, Cmdtab *, int);
void mallocinit(void);
int32_t mallocreadsummary(Chan *, void *, int32_t, int32_t);
void mallocsummary(void);
Block *mem2bl(unsigned char *, int);
void (*mfcinit)(void);
void (*mfcopen)(Chan *);
int (*mfcread)(Chan *, unsigned char *, int, int64_t);
void (*mfcupdate)(Chan *, unsigned char *, int, int64_t);
void (*mfcwrite)(Chan *, unsigned char *, int, int64_t);
void mfreeseg(Segment *, uintptr_t, int);
void microdelay(int);
uint64_t mk64fract(uint64_t, uint64_t);
void mkqid(Qid *, int64_t, uint32_t, int);
void mmuflush(void);
void mmuput(uintptr_t, Page *, uint);
void mmurelease(Proc *);
void mmuswitch(Proc *);
Chan *mntauth(Chan *, char *);
usize mntversion(Chan *, uint32_t, char *, usize);
void mountfree(Mount *);
uint64_t ms2fastticks(uint32_t);
#define MS2NS(n) (((int64_t)(n)) * 1000000LL)
uint32_t ms2tk(uint32_t);
void mul64fract(uint64_t *, uint64_t, uint64_t);
void muxclose(Mnt *);
void (*mwait)(void *);
Chan *namec(char *, int, int, int);
void nameerror(char *, char *);
Chan *newchan(void);
int newfd(Chan *);
Mhead *newmhead(Chan *);
Mount *newmount(Mhead *, Chan *, int, char *);
Page *newpage(int, Segment **, uintptr_t, usize, int);
Path *newpath(char *);
Pgrp *newpgrp(void);
Proc *newproc(void);
Rgrp *newrgrp(void);
Segment *newseg(int, uintptr_t, uint64_t);
void newzmap(Segment *);
void nexterror(void);
void notemark(Watermark *, int);
uint nhgetl(void *);
uint16_t nhgets(void *);
uint64_t nhgetv(void *);
void nixprepage(int);
int nrand(int);
uint64_t ns2fastticks(uint64_t);
int okaddr(uintptr_t, int32_t, int);
int openmode(int);
Block *packblock(Block *);
Block *padblock(Block *, int);
void pagechainhead(Page *);
void pageinit(void);
uint32_t pagenumber(Page *);
uint64_t pagereclaim(Image *);
void pagersummary(void);
void pageunchain(Page *);
void panic(char *, ...);
Cmdbuf *parsecmd(char *a, int n);
void pathclose(Path *);
uint32_t perfticks(void);
void pexit(char *, int);
Page *pgalloc(usize, int);
void pgfree(Page *);
void pgrpcpy(Pgrp *, Pgrp *);
void pgrpnote(uint32_t, char *, int32_t, int);
uintmem physalloc(uint64_t, int *, void *);
void physdump(void);
void physfree(uintmem, uint64_t);
void physinit(uintmem, uint64_t);
void *phystag(uintmem);
void pio(Segment *, uintptr_t, uint32_t, Page **, int);
void portmwait(void *);
int postnote(Proc *, int, char *, int);
int pprint(char *, ...);
int preempted(void);
void prflush(void);
void printinit(void);
uint32_t procalarm(uint32_t);
void procctl(Proc *);
void procdump(void);
int procfdprint(Chan *, int, int, char *, int);
void procflushseg(Segment *);
void procinit0(void);
void procpriority(Proc *, int, int);
void procrestore(Proc *);
void procsave(Proc *);
void (*proctrace)(Proc *, int, int64_t);
void proctracepid(Proc *);
void procwired(Proc *, int);
void psdecref(Proc *);
Proc *psincref(int);
int psindex(int);
void psinit(int);
Pte *ptealloc(Segment *);
Pte *ptecpy(Segment *, Pte *);
int pullblock(Block **, int);
Block *pullupblock(Block *, int);
Block *pullupqueue(Queue *, int);
void putimage(Image *);
void putmhead(Mhead *);
void putpage(Page *);
void putseg(Segment *);
void putstrn(char *, int);
void putswap(Page *);
int pwait(Waitmsg *);
void qaddlist(Queue *, Block *);
Block *qbread(Queue *, int);
int32_t qbwrite(Queue *, Block *);
int32_t qibwrite(Queue *, Block *);
Queue *qbypass(void (*)(void *, Block *), void *);
int qcanread(Queue *);
void qclose(Queue *);
int qconsume(Queue *, void *, int);
Block *qcopy(Queue *, int, uint32_t);
int qdiscard(Queue *, int);
void qflush(Queue *);
void qfree(Queue *);
int qfull(Queue *);
Block *qget(Queue *);
void qhangup(Queue *, char *);
int qisclosed(Queue *);
int qiwrite(Queue *, void *, int);
int qlen(Queue *);
void qlock(QLock *);
void qnoblock(Queue *, int);
Queue *qopen(int, int, void (*)(void *), void *);
int qpass(Queue *, Block *);
int qpassnolim(Queue *, Block *);
int qproduce(Queue *, void *, int);
void qputback(Queue *, Block *);
int32_t qread(Queue *, void *, int);
Block *qremove(Queue *);
void qreopen(Queue *);
void qsetlimit(Queue *, int);
void qunlock(QLock *);
int qwindow(Queue *);
int qwrite(Queue *, void *, int);
int rand(void);
void randominit(void);
uint32_t randomread(void *, uint32_t);
uint32_t urandomread(void *, uint32_t);
void rdb(void);
int readnum(uint32_t, char *, uint32_t, uint32_t, int);
int32_t readstr(int32_t, char *, int32_t, char *);
void ready(Proc *);
int32_t readzio(Kzio[], int, void *, int32_t);
void reboot(void *, void *, int32_t);
void rebootcmd(int, char **);
void relocateseg(Segment *, uintptr_t);
void renameuser(char *, char *);
void resched(char *);
void resrcwait(char *);
int return0(void *);
void rlock(RWlock *);
int32_t rtctime(void);
int runac(Mach *m, void (*func)(void), int flushtlb, void *a,
	  int32_t n);
void runlock(RWlock *);
Proc *runproc(void);
void sched(void);
void scheddump(void);
void schedinit(void);
int32_t seconds(void);
Segment *seg(Proc *, uintptr_t, int);
void segclock(uintptr_t);
Sem *segmksem(Segment *, int *);
void segpage(Segment *, Page *);
char *seprintmark(char *, char *, Watermark *);
uintmem segppn(Segment *, uintmem);
int semadec(int *);
int semainc(int *);
char *seprintpagestats(char *, char *);
char *seprintphysstats(char *, char *);
int setcolor(uint32_t, uint32_t, uint32_t, uint32_t);
void setkernur(Ureg *, Proc *);
int setlabel(Label *);
void setregisters(Ureg *, char *, char *, int);
char *skipslash(char *);
void sleep(Rendez *, int (*)(void *), void *);
void *smalloc(uint32_t);
char *srvname(Chan *);
void startwaitstats(int);
int swapcount(uint32_t);
void swapinit(void);
void syscallfmt(int, ...);
void sysretfmt(int, Ar0 *, uint64_t, uint64_t, ...);
void sysrforkchild(Proc *, Proc *);
void timeradd(Timer *);
void timerdel(Timer *);
void timerintr(Ureg *, int64_t);
void timerset(uint64_t);
void timersinit(void);
uint32_t tk2ms(uint32_t);
#define TK2MS(x) ((x) * (1000 / HZ))
uint64_t tod2fastticks(int64_t);
int64_t todget(int64_t *);
void todinit(void);
void todset(int64_t, int64_t, int);
void todsetfreq(int64_t);
Block *trimblock(Block *, int, int);
void tsleep(Rendez *, int (*)(void *), void *, int32_t);
Uart *uartconsole(int, char *);
int uartctl(Uart *, char *);
int uartgetc(void);
void uartkick(void *);
void uartputc(int);
void uartputs(char *, int);
void uartrecv(Uart *, char);
int uartstageoutput(Uart *);
void unbreak(Proc *);
void uncachepage(Page *);
void unlock(Lock *);
void userinit(void);
uintptr_t userpc(Ureg *);
int32_t userwrite(char *, int32_t);
void *validaddr(void *, int32_t, int);
void validname(char *, int);
char *validnamedup(char *, int);
void validstat(unsigned char *, usize);
void *vmemchr(void *, int, int);
Proc *wakeup(Rendez *);
int walk(Chan **, char **, int, int, int *);
void wlock(RWlock *);
void wunlock(RWlock *);
/* xalloc */
void *xalloc(uint32_t);
void *xallocz(uint32_t, int);
void xfree(void *);
void xhole(uintmem, uint32_t);
void xinit(void);
int xmerge(void *, void *);
void *xspanalloc(uint32_t, int, uint32_t);
void xsummary(void);
/* end xalloc */
void yield(void);
uintptr_t zgetaddr(Segment *);
void zgrow(Segment *);
int ziofmt(Fmt *);
int zputaddr(Segment *, uintptr_t);
uint32_t ms(void);

#pragma varargck argpos iprint 1
#pragma varargck argpos panic 1
#pragma varargck argpos pprint 1

/* profiling. */
void oprofile_control_trace(int onoff);
int oprofread(void *va, int n);
void oprof_alarm_handler(Ureg *u);
void oprofile_add_backtrace(uintptr_t pc, uintptr_t fp);
void oprofile_add_userpc(uintptr_t pc);
int alloc_cpu_buffers(void);
