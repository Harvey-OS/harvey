/*
 * Plan 9 versions of system-specific functions
 *	By convention, exported routines herein have names beginning with an
 *	upper case letter.
 */
#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"

char *Signame[] = {
	"sigexit",	"sighup",	"sigint",	"sigquit",
	"sigalrm",	"sigkill",	"sigfpe",	"sigterm",
	0
};
char *syssigname[] = {
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
char *Rcmain = "/rc/lib/rcmain";
char *Fdprefix = "/fd/";

void execfinit(void);
void execbind(void);
void execmount(void);
void execnewpgrp(void);

builtin Builtin[] = {
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

void
execnewpgrp(void)
{
	int arg;
	char *s;
	switch(count(runq->argv->words)){
	case 1:
		arg = RFENVG|RFNAMEG|RFNOTEG;
		break;
	case 2:
		arg = 0;
		for(s = runq->argv->words->next->word;*s;s++)
			switch(*s){
			default:
				goto Usage;
			case 'n':
				arg|=RFNAMEG;  break;
			case 'N':
				arg|=RFCNAMEG;
				break;
			case 'm':
				arg|=RFNOMNT;  break;
			case 'e':
				arg|=RFENVG;   break;
			case 'E':
				arg|=RFCENVG;  break;
			case 's':
				arg|=RFNOTEG;  break;
			case 'f':
				arg|=RFFDG;    break;
			case 'F':
				arg|=RFCFDG;   break;
			}
		break;
	default:
	Usage:
		pfmt(err, "Usage: %s [fnesFNEm]\n", runq->argv->words->word);
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

int
openenv(char *shortname)
{
	int f;
	io *envname;

	envname = openstr();
	pfmt(envname, "/env/%s", shortname);
	f = open((char *)envname->strp, OREAD);
	closeio(envname);
	return f;
}

int
createenv(char *pfx, char *shortname)
{
	int f;
	io *envname;

	envname = openstr();
	pfmt(envname, "/env/%s%s", pfx, shortname);
	f = Creat((char *)envname->strp);
	if (f < 0)
		pfmt(err, "rc: can't create %s: %r\n", (char *)envname->strp);
	closeio(envname);
	return f;
}

void
Vinit(void)
{
	int dir, f, len, i, n, nent;
	char *buf, *s, *name;
	var *namevar;
	word *val;
	Dir *ent;

	dir = open("/env", OREAD);
	if(dir<0){
		pfmt(err, "rc: can't open /env: %r\n");
		return;
	}
	ent = nil;
	while ((nent = dirread(dir, &ent)) > 0) {
		for(i = 0; i<nent; i++){
			len = ent[i].length;
			name = ent[i].name;
			if(len <= 0 || strncmp(name, "fn#", 3) == 0)
				continue;
			if((f = openenv(name)) < 0)
				continue;
			buf = emalloc(len+1);
			n = readn(f, buf, len);
			if (n <= 0)
				buf[0] = '\0';
			else
				buf[n] = '\0';
			val = 0;
			/* Charitably add a 0 at the end if need be */
			if(buf[len-1])
				buf[len++]='\0';
			s = buf+len-1;
			for(;;){
				while(s!=buf && s[-1]!='\0')
					--s;
				val = newword(s, val);
				if(s==buf)
					break;
				--s;
			}
			setvar(name, val);
			namevar = vlook(name);
			assert(namevar != nil);
			namevar->changed = 0;
			close(f);
			efree(buf);
		}
		free(ent);
	}
	close(dir);
}

int envdir;

void
Xrdfn(void)
{
	int f;
	Dir *e;
	static Dir *ent, *allocent;
	static int nent;

	for(;;){
		if(nent == 0){
			free(allocent);
			nent = dirread(envdir, &allocent);
			ent = allocent;
		}
		if(nent <= 0)
			break;
		while(nent){
			e = ent++;
			nent--;
			if(e->length && strncmp(e->name, "fn#", 3)==0){
				if((f = openenv(e->name)) >= 0){
					execcmds(openfd(f));
					return;
				}
			}
		}
	}
	close(envdir);
	Xreturn();
}

union code rdfns[4];

void
execfinit(void)
{
	static int first = 1;
	if(first){
		rdfns[0].i = 1;
		rdfns[1].f = Xrdfn;
		rdfns[2].f = Xjump;
		rdfns[3].i = 1;
		first = 0;
	}
	Xpopm();
	envdir = open("/env", 0);
	if(envdir<0){
		pfmt(err, "rc: can't open /env: %r\n");
		return;
	}
	start(rdfns, 1, runq->local);
}

int
Waitfor(int pid, int)
{
	thread *p;
	Waitmsg *w;
	char errbuf[ERRMAX];

	if(pid >= 0 && !havewaitpid(pid))
		return 0;

	while((w = wait()) != nil){
		/* this would otherwise go unreported by rc */
		if(strstr(w->msg, "error in demand load") != nil)
			pfmt(err, "rc: %s\n", w->msg);
		delwaitpid(w->pid);
		if(w->pid==pid){
			setstatus(w->msg);
			free(w);
			return 0;
		}
		for(p = runq->ret;p;p = p->ret)
			if(p->pid==w->pid){
				p->pid=-1;
				strcpy(p->status, w->msg);
			}
		free(w);
	}

	errstr(errbuf, sizeof errbuf);
	if(strcmp(errbuf, "interrupted")==0) return -1;
	return 0;
}

char **
mkargv(word *a)
{
	char **argv = (char **)emalloc((count(a)+2)*sizeof(char *));
	char **argp = argv+1;	/* leave one at front for runcoms */
	for(;a;a = a->next) *argp++=a->word;
	*argp = 0;
	return argv;
}

void
addenv(var *v)
{
	word *w;
	int f;
	io *fd;
	if(v->changed){
		v->changed = 0;
		if((f = createenv("", v->name)) >= 0) {
			for(w = v->val;w;w = w->next)
				write(f, w->word, strlen(w->word)+1);
			close(f);
		}
	}
	if(v->fnchanged){
		v->fnchanged = 0;
		if((f = createenv("fn#", v->name)) >= 0) {
			if(v->fn){
				fd = openfd(f);
				pfmt(fd, "fn %q %s\n", v->name, v->fn[v->pc-1].s);
				closeio(fd);
			}
			close(f);
		}
	}
}

void
updenvlocal(var *v)
{
	if(v){
		updenvlocal(v->next);
		addenv(v);
	}
}

void
Updenv(void)
{
	var *v, **h;
	for(h = gvar;h!=&gvar[NVAR];h++)
		for(v=*h;v;v = v->next)
			addenv(v);
	if(runq)
		updenvlocal(runq->local);
}

/* not used on plan 9 */
int
ForkExecute(char *file, char **argv, int sin, int sout, int serr)
{
	int pid;

	if(access(file, 1) != 0)
		return -1;
	switch(pid = fork()){
	case -1:
		return -1;
	case 0:
		if(sin >= 0)
			dup(sin, 0);
		else
			close(0);
		if(sout >= 0)
			dup(sout, 1);
		else
			close(1);
		if(serr >= 0)
			dup(serr, 2);
		else
			close(2);
		exec(file, argv);
		exits(file);
	}
	return pid;
}

void
Execute(word *args, word *path)
{
	char **argv = mkargv(args);
	char file[1024], errstr[ERRMAX+1];
	int nc;

	Updenv();
	errstr[0] = '\0';
	for(;path;path = path->next){
		nc = strlen(path->word);
		if(nc >= sizeof file - 1){	/* 1 for / */
			werrstr("path component too long");
			continue;
		}
		strcpy(file, path->word);
		if(file[0]){
			strcat(file, "/");
			nc++;
		}
		if(nc + strlen(argv[1]) >= sizeof file){
			werrstr("command name too long");
			continue;
		}
		strcat(file, argv[1]);
		exec(file, argv+1);
		/*
		 * if file exists and is executable, exec should have worked,
		 * unless it's a directory or an executable for another
		 * architecture.  in particular, if it failed due to lack of
		 * swap/vm (e.g., arg. list too long) or other allocation or
		 * i/o failure, stop searching and print the reason for failure.
		 */
		rerrstr(errstr, sizeof errstr);
		if (strstr(errstr, " allocat") != nil ||
		    strstr(errstr, " full") != nil ||
		    strstr(errstr, "i/o error") != nil)
			break;
	}
	if(errstr[0] == '\0')		/* pick up any werrstr "too long"s */
		rerrstr(errstr, sizeof errstr);
	pfmt(err, "%s: %s\n", argv[1], errstr);
	efree((char *)argv);
}

#define	NDIR	256		/* should be a better way */

int
Globsize(char *p)
{
	int isglob = 0, globlen = NDIR+1;
	for(;*p;p++){
		if(*p==GLOB){
			p++;
			if(*p!=GLOB)
				isglob++;
			globlen+=*p=='*'?NDIR:1;
		}
		else
			globlen++;
	}
	return isglob?globlen:0;
}
#define	NFD	50

struct{
	Dir	*dbuf;
	int	i;
	int	n;
}dir[NFD];

int
Opendir(char *name)
{
	Dir *db;
	int f;
	f = open(name, 0);
	if(f==-1)
		return f;
	db = dirfstat(f);
	if(db!=nil && (db->mode&DMDIR)){
		if(f<NFD){
			dir[f].i = 0;
			dir[f].n = 0;
		}
		free(db);
		return f;
	}
	free(db);
	close(f);
	return -1;
}

static int
trimdirs(Dir *d, int nd)
{
	int r, w;

	for(r=w=0; r<nd; r++)
		if(d[r].mode&DMDIR)
			d[w++] = d[r];
	return w;
}

/*
 * onlydirs is advisory -- it means you only
 * need to return the directories.  it's okay to
 * return files too (e.g., on unix where you can't
 * tell during the readdir), but that just makes
 * the globber work harder.
 */
int
Readdir(int f, void *p, int onlydirs)
{
	int n;

	if(f<0 || f>=NFD)
		return 0;
Again:
	if(dir[f].i==dir[f].n){	/* read */
		free(dir[f].dbuf);
		dir[f].dbuf = 0;
		n = dirread(f, &dir[f].dbuf);
		if(n>0){
			if(onlydirs){
				n = trimdirs(dir[f].dbuf, n);
				if(n == 0)
					goto Again;
			}
			dir[f].n = n;
		}else
			dir[f].n = 0;
		dir[f].i = 0;
	}
	if(dir[f].i == dir[f].n)
		return 0;
	strcpy(p, dir[f].dbuf[dir[f].i].name);
	dir[f].i++;
	return 1;
}

void
Closedir(int f)
{
	if(f>=0 && f<NFD){
		free(dir[f].dbuf);
		dir[f].i = 0;
		dir[f].n = 0;
		dir[f].dbuf = 0;
	}
	close(f);
}

int interrupted = 0;

void
notifyf(void*, char *s)
{
	int i;

	for (i = 0; syssigname[i]; i++)
		if (strncmp(s, syssigname[i], strlen(syssigname[i])) == 0) {
			if (strncmp(s, "sys: ", 5) != 0)
				interrupted = 1;
			goto Out;
		}
	pfmt(err, "rc: note: %s\n", s);
	noted(NDFLT);
	return;
Out:
	if (strcmp(s, "interrupt") != 0 || trap[i] == 0) {
		trap[i]++;
		ntrap++;
	}
	if (ntrap >= 32) {	/* rc is probably in a trap loop */
		pfmt(err, "rc: Too many traps (trap %s), aborting\n", s);
		abort();
	}
	noted(NCONT);
}

void
Trapinit(void)
{
	notify(notifyf);
}

void
Unlink(char *name)
{
	remove(name);
}

long
Write(int fd, void *buf, long cnt)
{
	return write(fd, buf, cnt);
}

long
Read(int fd, void *buf, long cnt)
{
	return read(fd, buf, cnt);
}

int
Executable(char *file)
{
	Dir *statbuf;
	int ret;

	statbuf = dirstat(file);
	if(statbuf == nil)
		return 0;
	ret = ((statbuf->mode&0111)!=0 && (statbuf->mode&DMDIR)==0);
	free(statbuf);
	return ret;
}

int
Creat(char *file)
{
	return create(file, 1, 0666);
}

int
Dup(int a, int b)
{
	return dup(a, b);
}

int
Dup1(int)
{
	return -1;
}

void
Exit(char *stat)
{
	Updenv();
	setstatus(stat);
	exits(truestatus()?"":getstatus());
}

int
Eintr(void)
{
	return interrupted;
}

void
Noerror(void)
{
	interrupted = 0;
}

int
Isatty(int fd)
{
	char buf[64];

	if(fd2path(fd, buf, sizeof buf) != 0)
		return 0;

	/* might be #c/cons during boot - fixed 22 april 2005, remove this later */
	if(strcmp(buf, "#c/cons") == 0)
		return 1;

	/* might be /mnt/term/dev/cons */
	return strlen(buf) >= 9 && strcmp(buf+strlen(buf)-9, "/dev/cons") == 0;
}

void
Abort(void)
{
	pfmt(err, "aborting\n");
	flush(err);
	Exit("aborting");
}

void
Memcpy(void *a, void *b, long n)
{
	memmove(a, b, n);
}

void*
Malloc(ulong n)
{
	return mallocz(n, 1);
}

int *waitpids;
int nwaitpids;

void
addwaitpid(int pid)
{
	waitpids = realloc(waitpids, (nwaitpids+1)*sizeof waitpids[0]);
	if(waitpids == 0)
		panic("Can't realloc %d waitpids", nwaitpids+1);
	waitpids[nwaitpids++] = pid;
}

void
delwaitpid(int pid)
{
	int r, w;

	for(r=w=0; r<nwaitpids; r++)
		if(waitpids[r] != pid)
			waitpids[w++] = waitpids[r];
	nwaitpids = w;
}

void
clearwaitpids(void)
{
	nwaitpids = 0;
}

int
havewaitpid(int pid)
{
	int i;

	for(i=0; i<nwaitpids; i++)
		if(waitpids[i] == pid)
			return 1;
	return 0;
}

/* avoid loading any floating-point library code */
int
_efgfmt(Fmt *)
{
	return -1;
}
