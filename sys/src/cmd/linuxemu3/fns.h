/* error */
int mkerror(void);
#pragma varargck type "E" int
int Efmt(Fmt *e);
int sys_nosys(void);

/* linuxcall */
int linuxcall(void);

/* trap */
void inittrap(void);
void retuser(void);

/* bits */
void incref(Ref *);
int decref(Ref *);
void jumpstart(ulong addr, ulong *stack);
void jumpureg(void *ureg);
void linux_sigreturn(void);
void linux_rtsigreturn(void);

/* trace */
void inittrace(void);
void exittrace(Uproc *proc);
void clonetrace(Uproc *new, int copy);
void tprint(char *fmt, ...);
#pragma varargck argpos tprint 1
#define trace if(debug)tprint

/* proc */
void initproc(void);
void exitproc(Uproc *proc, int code, int group);
void stopproc(Uproc *proc, int code, int group);
void contproc(Uproc *proc, int code, int group);
int procfork(void (*fproc)(void *aux), void *aux, int flags);
Uproc* getproc(int tid);
Uproc* getprocn(int n);
int threadcount(int pid);
void zapthreads(void);
void setprocname(char *s);
int notifyme(int on);
void wakeme(int on);
int sleepproc(QLock *l, int flags);
Uwait* addwaitq(Uwaitq *q);
void delwaitq(Uwait *w);
int sleepq(Uwaitq *q, QLock *l, int flags);
int wakeq(Uwaitq *q, int nwake);
int requeue(Uwaitq *q1, Uwaitq *q2, int nrequeue);
int killproc(Uproc *p, Usiginfo *info, int group);
void setalarm(vlong t);

int sys_waitpid(int pid, int *pexit, int opt);
int sys_wait4(int pid, int *pexit, int opt, void *prusage);
int sys_exit(int code);
int sys_exit_group(int code);
int sys_linux_clone(int flags, void *newstack, int *parenttidptr, int *tlsdescr, void *childtidptr);
int sys_fork(void);
int sys_vfork(void);
int sys_getpid(void);
int sys_getppid(void);
int sys_gettid(void);
int sys_setpgid(int pid, int pgid);
int sys_getpgid(int pid);
int sys_setpgrp(int pid);
int sys_getpgrp(void);
int sys_getuid(void);
int sys_getgid(void);
int sys_setgid(int gid);
int sys_setuid(int uid);
int sys_setresuid(int ruid, int euid, int suid);
int sys_getresuid(int *ruid, int *euid, int *suid);
int sys_setresgid(int rgid, int egid, int sgid);
int sys_getresgid(int *rgid, int *egid, int *sgid);
int sys_setreuid(int ruid, int euid);
int sys_setregid(int rgid, int egid);
int sys_uname(void *);
int sys_personality(ulong p);
int sys_setsid(void);
int sys_getsid(int pid);
int sys_getgroups(int size, int *groups);
int sys_setgroups(int size, int *groups);

int sys_kill(int pid, int sig);
int sys_tkill(int tid, int sig);
int sys_tgkill(int pid, int tid, int sig);
int sys_rt_sigqueueinfo(int pid, int sig, void *info);

int sys_set_tid_address(int *tidptr);

int sys_sched_setscheduler(int pid, int policy, void *param);
int sys_sched_getscheduler(int pid);
int sys_sched_setparam(int pid, void *param);
int sys_sched_getparam(int pid, void *param);
int sys_sched_yield(void);

int sys_getrlimit(long resource, void *rlim);
int sys_setrlimit(long resource, void *rlim);

/* signal */
void initsignal(void);
void exitsignal(void);
void clonesignal(Uproc *new, int copyhand, int newproc);
void settty(Ufile *tty);
Ufile* gettty(void);
#pragma varargck type "S" int
int Sfmt(Fmt *f);

int wantssignal(Uproc *proc, int sig);
int ignoressignal(Uproc *proc, int sig);
int signalspending(Uproc *proc);

void handlesignals(void);
int sendsignal(Uproc *proc, Usiginfo *info, int group);

void siginfo2linux(Usiginfo *, void *);
void linux2siginfo(void *, Usiginfo *);

int sys_sigaltstack(void *stk, void *ostk);
int sys_rt_sigaction(int sig, void *pact, void *poact, int setsize);
int sys_rt_sigpending(uchar *set, int setsize);
int sys_rt_sigprocmask(int how, uchar *act, uchar *oact, int setsize);
int sys_rt_sigsuspend(uchar *set, int setsize);
int sys_sigreturn(void);
int sys_rt_sigreturn(void);

int sys_setitimer(int which, void *value, void *ovalue);
int sys_getitimer(int which, void *value);
int sys_alarm(long seconds);

/* file */
void initfile(void);
void exitfile(Uproc *proc);
void clonefile(Uproc *new, int copy);
void closexfds(void);
Ufile *procfdgetfile(Uproc *proc, int fd);
Ufile* fdgetfile(int fd);
Ufile* getfile(Ufile *file);
void putfile(Ufile *file);
int newfd(Ufile *file, int flags);
int chdirfile(Ufile *file);
int readfile(Ufile *file, void *buf, int len);
int writefile(Ufile *file, void *buf, int len);
int preadfile(Ufile *file, void *buf, int len, vlong off);
int pwritefile(Ufile *file, void *buf, int len, vlong off);
int sys_dup(int fd);
int sys_dup2(int old, int new);
int sys_fcntl(int fd, int cmd, int arg);
int sys_close(int fd);
int sys_ioctl(int fd, int cmd, void *arg);
int sys_read(int fd, void *buf, int len);
int sys_readv(int fd, void *vec, int n);
int sys_pread64(int fd, void *buf, int len, ulong off);
int sys_write(int fd, void *buf, int len);
int sys_pwrite64(int fd, void *buf, int len, ulong off);
int sys_writev(int fd, void *vec, int n);
ulong sys_lseek(int fd, ulong off, int whence);
int sys_llseek(int fd, ulong hioff, ulong looff, vlong *res, int whence);
int sys_umask(int umask);
int sys_flock(int fd, int cmd);
int sys_fsync(int fd);
int sys_fchdir(int fd);
int sys_getcwd(char *buf, int len);
int sys_fchmod(int fd, int mode);
int sys_fchown(int fd, int uid, int gid);
int sys_ftruncate(int fd, ulong size);

/* poll */
void pollwait(Ufile *f, Uwaitq *q, void *t);
int sys_poll(void *p, int nfd, long timeout);
int sys_select(int nfd, ulong *rfd, ulong *wfd, ulong *efd, void *ptv);

/* mem */
void* kmalloc(int size);
void* kmallocz(int size, int zero);
void* krealloc(void *ptr, int size);
char* kstrdup(char *s);
char* ksmprint(char *fmt, ...);
#pragma varargck argpos ksmprint 1

ulong pagealign(ulong addr);

void initmem(void);
void exitmem(void);
void clonemem(Uproc *new, int copy);
ulong procmemstat(Uproc *proc, ulong *pdat, ulong *plib, ulong *pshr, ulong *pstk, ulong *pexe);
void* mapstack(int size);
void mapdata(ulong base);
void unmapuserspace(void);
int okaddr(void *ptr, int len, int write);

ulong sys_linux_mmap(void *a);
ulong sys_mmap(ulong addr, ulong len, int prot, int flags, int fd, ulong pgoff);
int sys_munmap(ulong addr, ulong len);
ulong sys_brk(ulong bk);
int sys_mprotect(ulong addr, ulong len, int prot);
int sys_msync(ulong addr, ulong len, int flags);
ulong sys_mremap(ulong addr, ulong oldlen, ulong newlen, int flags, ulong newaddr);

int sys_futex(ulong *addr, int op, int val, void *ptime, ulong *addr2, int val3);

/* exec */
int sys_execve(char *name, char *argv[], char *envp[]);

/* time */
void inittime(void);
int sys_time(long *p);
int sys_gettimeofday(void *tvp, void *tzp);
int sys_clock_gettime(int clock, void *t);
int sys_nanosleep(void *rqp, void *rmp);
int proctimes(Uproc *p, ulong *t);
int sys_times(void *times);

/* tls */
void inittls(void);
void clonetls(Uproc *new);

int sys_set_thread_area(void *pinfo);
int sys_get_thread_area(void *pinfo);
int sys_modify_ldt(int func, void *data, int count);

/* bufproc */
void *newbufproc(int fd);
void freebufproc(void *bp);
int readbufproc(void *bp, void *data, int len, int peek, int noblock);
int pollbufproc(void *bp, Ufile *file, void *tab);
int nreadablebufproc(void *bp);

/* main */
void panic(char *msg, ...);
int onstack(long *stk, int (*func)(void *arg), void *arg);
void profme(void);

/* stat */
int ufstat(int fd, Ustat *ps);
Udirent *newdirent(char *path, char *name, int mode);

int sys_getxattr(char *path, char *name, void *value, int size);
int sys_lgetxattr(char *path, char *name, void *value, int size);
int sys_fgetxattr(int fd, char *name, void *value, int size);
int sys_setxattr(char *path, char *name, void *value, int flags, int size);
int sys_lsetxattr(char *path, char *name, void *value, int flags, int size);
int sys_fsetxattr(int fd, char *name, void *value, int size, int flags);

int sys_linux_fstat(int fd, void *st);
int sys_linux_fstat64(int fd, void *st);
int sys_linux_getdents(int fd, void *buf, int nbuf);
int sys_linux_getdents64(int fd, void *buf, int nbuf);
int sys_linux_lstat(char *path, void *st);
int sys_linux_lstat64(char *path, void *st);
int sys_linux_stat(char *path, void *st);
int sys_linux_stat64(char *path, void *st);

int sys_statfs(char *name, void *pstatfs);

/* fs */
void fsmount(Udev *dev, char *path);

char* allocpath(char *base, char *prefix, char *name);
char* fullpath(char *base, char *name);
char* shortpath(char *base, char *path);
char* fsfullpath(char *path);
char* fsrootpath(char *path);
char* basepath(char *p, char **ps);
ulong hashpath(char *s);

int fsaccess(char *path, int mode);
int fschmod(char *path, int mode);
int fschown(char *path, int uid, int gid, int link);
int fslink(char *old, char *new, int sym);
int fsmkdir(char *path, int mode);
int fsopen(char *path, int mode, int perm, Ufile **pf);
int fsreadlink(char *path, char *buf, int len);
int fsrename(char *old, char *new);
int fsstat(char *path, int link, Ustat *ps);
int fstruncate(char *path, vlong size);
int fsunlink(char *path, int rmdir);
int fsutime(char *path, int atime, int mtime);

int sys_access(char *name, int mode);
int sys_chdir(char *name);
int sys_chroot(char *name);
int sys_chmod(char *name, int mode);
int sys_chown(char *name, int uid, int gid);
int sys_creat(char *name, int perm);
int sys_lchown(char *name, int uid, int gid);
int sys_link(char *old, char *new);
int sys_open(char *name, int mode, int perm);
int sys_readlink(char *name, char *buf, int len);
int sys_rename(char *from, char *to);
int sys_rmdir(char *name);
int sys_symlink(char *old, char *new);
int sys_truncate(char *name, ulong size);
int sys_unlink(char *name);
int sys_utime(char *name, void *times);
int sys_utimes(char *name, void *tvp);
int sys_mkdir(char *name, int mode);

/* drivers */
void rootdevinit(void);
void sockdevinit(void);
int sys_linux_socketcall(int call, int *arg);
void pipedevinit(void);
int sys_pipe(int *fds);
void fddevinit(void);
void ptsdevinit(void);
void dspdevinit(void);
void miscdevinit(void);
void ptydevinit(void);
void consdevinit(void);
void procdevinit(void);

/* arch-dependent */
void clinote(struct Ureg *);
void linuxret(int);
