/*
 * Plan 9 versions of system-specific functions
 *	By convention, exported routines herein have names beginning with an
 *	upper case letter.
 */
#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"
#include "getflags.h"
char *Signame[]={
	"sigexit",	"sighup",	"sigint",	"sigquit",
	"sigalrm",	"sigkill",	"sigfpe",	"sigterm",
	0
};
char *syssigname[]={
	"exit",		/* can't happen */
	"hangup",
	"interrupt",
	"quit",		/* can't happen */
	"alarm",
	"kill",
	"sys: fp: ",
	"term",
	0
};
char Rcmain[]="/rc/lib/rcmain";
char Fdprefix[]="/fd/";
void execfinit(void);
void execbind(void);
void execmount(void);
void execnewpgrp(void);
builtin Builtin[]={
	"cd",		execcd,
	"whatis",	execwhatis,
	"eval",		execeval,
	"exec",		execexec,	/* but with popword first */
	"exit",		execexit,
	"shift",	execshift,
	"wait",		execwait,
	".",		execdot,
	"finit",	execfinit,
	"flag",		execflag,
	"rfork",	execnewpgrp,
	0
};
void execnewpgrp(void){
	int arg;
	char *s;
	switch(count(runq->argv->words)){
	case 1: arg=RFENVG|RFNAMEG|RFNOTEG; break;
	case 2:
		arg=0;
		for(s=runq->argv->words->next->word;*s;s++) switch(*s){
		default:
			goto Usage;
		case 'n': arg|=RFNAMEG;  break;
		case 'N': arg|=RFCNAMEG; break;
		case 'e': arg|=RFENVG;   break;
		case 'E': arg|=RFCENVG;  break;
		case 's': arg|=RFNOTEG;  break;
		case 'f': arg|=RFFDG;    break;
		case 'F': arg|=RFCFDG;   break;
		}
		break;
	default:
	Usage:
		pfmt(err, "Usage: %s [fnesFNE]\n", runq->argv->words->word);
		setstatus("rfork usage");
		poplist();
		return;
	}
	if(rfork(arg)==-1){
		pfmt(err, "rc: %s failed\n", runq->argv->words->word);
		setstatus("rfork failed");
	}
	else
		setstatus("");
	poplist();
}
void Vinit(void){
	int dir=open("#e", 0), f, len;
	word *val;
	char *buf, *s;
	Dir ent;
	char envname[NAMELEN+6];
	if(dir<0){
		pfmt(err, "rc: can't open #e\n");
		return;
	}
	while(dirread(dir, &ent, sizeof ent)==sizeof ent){
		len=ent.length;
		if(len && strncmp(ent.name, "fn#", 3)!=0){
			strcpy(envname, "#e/");
			strcat(envname, ent.name);
			if((f=open(envname, 0))>=0){
				buf=emalloc((int)len+1);
				read(f, buf, (long)len);
				val=0;
				/* Charitably add a 0 at the end if need be */
				if(buf[len-1]) buf[len++]='\0';
				s=buf+len-1;
				for(;;){
					while(s!=buf && s[-1]!='\0') --s;
					val=newword(s, val);
					if(s==buf) break;
					--s;
				}
				setvar(ent.name, val);
				vlook(ent.name)->changed=0;
				close(f);
				efree(buf);
			}
		}
	}
	close(dir);
}
#ifdef old_execfinit
void Xrdfn(void){}
void execfinit(void){
	Xpopm();
}
#else
int envdir;
void Xrdfn(void){
	int f, len;
	Dir ent;
	char envname[NAMELEN+6];
	while(dirread(envdir, &ent, sizeof ent)==sizeof ent){
		len=ent.length;
		if(len && strncmp(ent.name, "fn#", 3)==0){
			strcpy(envname, "#e/");
			strcat(envname, ent.name);
			if((f=open(envname, 0))>=0){
				execcmds(openfd(f));
				return;
			}
		}
	}
	close(envdir);
	Xreturn();
}
union code rdfns[4];
void execfinit(void){
	static int first=1;
	if(first){
		rdfns[0].i=1;
		rdfns[1].f=Xrdfn;
		rdfns[2].f=Xjump;
		rdfns[3].i=1;
		first=0;
	}
	Xpopm();
	envdir=open("#e", 0);
	if(envdir<0){
		pfmt(err, "rc: can't open #e\n");
		return;
	}
	start(rdfns, 1, runq->local);
}
#endif
int Waitfor(int pid, int persist){
	thread *p;
	Waitmsg w;
	int cpid;
	char errbuf[ERRLEN];
	while((cpid=wait(&w))>=0){
		if(pid==cpid){
			setstatus(w.msg);
			return 0;
		}
		for(p=runq->ret;p;p=p->ret)
			if(p->pid==cpid){
				p->pid=-1;
				strcpy(p->status, w.msg);
			}
	}
	errstr(errbuf);
	if(strcmp(errbuf, "interrupted")==0) return -1;
	return 0;
}
char **mkargv(word *a)
{
	char **argv=(char **)emalloc((count(a)+2)*sizeof(char *));
	char **argp=argv+1;	/* leave one at front for runcoms */
	for(;a;a=a->next) *argp++=a->word;
	*argp=0;
	return argv;
}
void addenv(var *v)
{
	char envname[NAMELEN+9];
	word *w;
	int f;
	io *fd;
	if(v->changed){
		v->changed=0;
		strcpy(envname, "#e/");
		strcat(envname, v->name);
		if((f=Creat(envname))<0)
			pfmt(err, "rc: can't open %s\n", envname);
		else{
			for(w=v->val;w;w=w->next)
				write(f, w->word, strlen(w->word)+1L);
			close(f);
		}
	}
	if(v->fnchanged){
		v->fnchanged=0;
		strcpy(envname, "#e/fn#");
		strcat(envname, v->name);
		if((f=Creat(envname))<0)
			pfmt(err, "rc: can't open %s\n", envname);
		else{
			if(v->fn){
				fd=openfd(f);
				pfmt(fd, "fn %s %s\n", v->name, v->fn[v->pc-1].s);
				closeio(fd);
			}
			close(f);
		}
	}
}
void updenvlocal(var *v)
{
	if(v){
		updenvlocal(v->next);
		addenv(v);
	}
}
void Updenv(void){
	var *v, **h;
	for(h=gvar;h!=&gvar[NVAR];h++)
		for(v=*h;v;v=v->next)
			addenv(v);
	if(runq) updenvlocal(runq->local);
}
void Execute(word *args, word *path)
{
	char **argv=mkargv(args);
	char file[1024];
	int nc;
	Updenv();
	for(;path;path=path->next){
		nc=strlen(path->word);
		if(nc<1024){
			strcpy(file, path->word);
			if(file[0]){
				strcat(file, "/");
				nc++;
			}
			if(nc+strlen(argv[1])<1024){
				strcat(file, argv[1]);
				exec(file, argv+1);
			}
			else werrstr("command name too long");
		}
	}
	errstr(file);
	pfmt(err, "%s: %s\n", argv[1], file);
	efree((char *)argv);
}
int Globsize(char *p)
{
	ulong isglob=0, globlen=NAMELEN+1;
	for(;*p;p++){
		if(*p==GLOB){
			p++;
			if(*p!=GLOB) isglob++;
			globlen+=*p=='*'?NAMELEN:1;
		}
		else
			globlen++;
	}
	return isglob?globlen:0;
}
#define NFD	50
#define	NDBUF	32
struct{
	char	*buf;
	int	n;
}dir[NFD];
int Opendir(char *name)
{
	Dir db;
	int f;
	f=open(name, 0);
	if(f==-1)
		return f;
	if(dirfstat(f, &db)!=-1 && (db.mode&0x80000000)){
		if(f<NFD && dir[f].buf){
			free(dir[f].buf);
			dir[f].buf=0;
		}
		return f;
	}
	close(f);
	return -1;
}
int Readdir(int f, char *p)
{
	char dirent[DIRLEN];
	int n;
	if(f<0 || f>=NFD){
   slow:
		while(read(f, dirent, sizeof dirent)==sizeof dirent){
			strcpy(p, dirent);
			return 1;
		}
		return 0;
	}
	if(dir[f].buf==0){	/* allocate */
		dir[f].buf=malloc(NDBUF*DIRLEN);
		if(dir[f].buf==0)
			goto slow;
		dir[f].n=0;
	}
	if(dir[f].n==0){	/* read */
		memset(dir[f].buf, 0, NDBUF*DIRLEN);
		n = read(f, dir[f].buf, NDBUF*DIRLEN);
		if(n>0 && n<NDBUF*DIRLEN){
			memmove(dir[f].buf+NDBUF*DIRLEN-n, dir[f].buf, n);
			dir[f].n=NDBUF-n/DIRLEN;
		}else
			dir[f].n=0;
	}
	if(dir[f].buf[dir[f].n*DIRLEN]==0)
		return 0;
	strcpy(p, &dir[f].buf[dir[f].n*DIRLEN]);
	dir[f].n++;
	if(dir[f].n==NDBUF)
		dir[f].n=0;
	return 1;
}
void Closedir(int f){
	if(f>=0 && f<NFD && dir[f].buf){
		free(dir[f].buf);
		dir[f].buf=0;
	}
	close(f);
}
int interrupted = 0;
void
notifyf(void *a, char *s)
{
	int i;
	for(i=0;syssigname[i];i++) if(strncmp(s, syssigname[i], strlen(syssigname[i]))==0){
		if(strncmp(s, "sys: ", 5)!=0) interrupted=1;
		goto Out;
	}
	pfmt(err, "rc: note: %s\n", s);
	noted(NDFLT);
	return;
Out:
	if(strcmp(s, "interrupt")!=0 || trap[i]==0){
		trap[i]++;
		ntrap++;
	}
	if(ntrap>=32){	/* rc is probably in a trap loop */
		pfmt(err, "rc: Too many traps (trap %s), aborting\n", s);
		abort();
	}
	noted(NCONT);
}
void Trapinit(void){
	notify(notifyf);
}
void Unlink(char *name)
{
	remove(name);
}
long Write(int fd, char *buf, long cnt)
{
	return write(fd, buf, (long)cnt);
}
long Read(int fd, char *buf, long cnt)
{
	int n;

	return read(fd, buf, cnt);
}
long Seek(int fd, long cnt, int whence)
{
	return seek(fd, cnt, whence);
}
int Executable(char *file)
{
	Dir statbuf;

	return dirstat(file, &statbuf)!=-1 && (statbuf.mode&0111)!=0 && (statbuf.mode&CHDIR)==0;
}
int Creat(char *file)
{
	return create(file, 1, 0666L);
}
int Dup(int a, int b){
	return dup(a, b);
}
int Dup1(int a){
	return -1;
}
void Exit(char *stat)
{
	Updenv();
	setstatus(stat);
	exits(truestatus()?"":getstatus());
}
int Eintr(void){
	return interrupted;
}
void Noerror(void){
	interrupted=0;
}
int Isatty(int fd){
	Dir d1, d2;

	if(dirfstat(0, &d1)==-1) return 0;
	if(strncmp(d1.name, "ptty", 4)==0) return 1;	/* fwd complaints to philw */
	if(dirstat("/dev/cons", &d2)==-1) return 0;
	return d1.type==d2.type&&d1.dev==d2.dev&&d1.qid.path==d2.qid.path;
}
void Abort(void){
	pfmt(err, "aborting\n");
	flush(err);
	Exit("aborting");
}
void Memcpy(char *a, char *b, long n)
{
	memmove(a, b, (long)n);
}
void *Malloc(ulong n){
	return malloc(n);
}
char *Geterrstr(void){
	static char error[ERRLEN];
	error[0]='\0';
	errstr(error);
	return error;
}
