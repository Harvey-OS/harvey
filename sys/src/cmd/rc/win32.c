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
char Rcmain[]="/rc/lib/rcmain";
char Fdprefix[]="/fd/";
void execfinit(void);
void execbind(void);

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
	0
};

void
Vinit(void)
{
	int dir, f, len;
	word *val;
	char *buf, *s;
	Dir *ent;
	int i, nent;
	char envname[256];
	dir = open("/env", OREAD);
	if(dir<0){
		pfmt(err, "rc: can't open /env: %r\n");
		return;
	}
	ent = nil;
	for(;;){
		nent = dirread(dir, &ent);
		if(nent <= 0)
			break;
		for(i = 0; i<nent; i++){
			len = ent[i].length;
			if(len && strncmp(ent[i].name, "fn#", 3)!=0){
				snprint(envname, sizeof envname, "/env/%s", ent[i].name);
				if((f = open(envname, 0))>=0){
					buf = emalloc((int)len+1);
					read(f, buf, (long)len);
					val = 0;
					/* Charitably add a 0 at the end if need be */
					if(buf[len-1])
						buf[len++]='\0';
					s = buf+len-1;
					for(;;){
						while(s!=buf && s[-1]!='\0') --s;
						val = newword(s, val);
						if(s==buf)
							break;
						--s;
					}
					setvar(ent[i].name, val);
					vlook(ent[i].name)->changed = 0;
					close(f);
					efree(buf);
				}
			}
		}
		free(ent);
	}
	close(dir);
}
int envdir;

void
Xrdfn(void)
{
	int f, len;
	static Dir *ent, *allocent;
	static int nent;
	Dir *e;
	char envname[256];

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
			len = e->length;
			if(len && strncmp(e->name, "fn#", 3)==0){
				snprint(envname, sizeof envname, "/env/%s", e->name);
				if((f = open(envname, 0))>=0){
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
Waitfor(int pid, int persist)
{
	thread *p;
	Waitmsg *w;
	char errbuf[ERRMAX];

	while((w = wait()) != nil){
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

char*
*mkargv(word *a)
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
	char envname[256];
	word *w;
	int f;
	io *fd;
	if(v->changed){
		v->changed = 0;
		snprint(envname, sizeof envname, "/env/%s", v->name);
		if((f = Creat(envname))<0)
			pfmt(err, "rc: can't open %s: %r\n", envname);
		else{
			for(w = v->val;w;w = w->next)
				write(f, w->word, strlen(w->word)+1L);
			close(f);
		}
	}
	if(v->fnchanged){
		v->fnchanged = 0;
		snprint(envname, sizeof envname, "/env/fn#%s", v->name);
		if((f = Creat(envname))<0)
			pfmt(err, "rc: can't open %s: %r\n", envname);
		else{
			if(v->fn){
				fd = openfd(f);
				pfmt(fd, "fn %s %s\n", v->name, v->fn[v->pc-1].s);
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

int
ForkExecute(char *file, char **argv, int sin, int sout, int serr)
{
	int pid;

{int i;
fprint(2, "forkexec %s", file);
for(i = 0; argv[i]; i++)fprint(2, " %s", argv[i]);
fprint(2, " %d %d %d\n", sin, sout, serr);
}
	if(access(file, 1) != 0)
		return -1;
fprint(2, "forking\n");
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
fprint(2, "execing\n");
		exec(file, argv);
fprint(2, "exec: %r\n");
		exits(file);
	}
	return pid;
}

void
Execute(word *args, word *path)
{
	char **argv = mkargv(args);
	char file[1024];
	int nc;
	Updenv();
	for(;path;path = path->next){
		nc = strlen(path->word);
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
	rerrstr(file, sizeof file);
	pfmt(err, "%s: %s\n", argv[1], file);
	efree((char *)argv);
}
#define	NDIR	256		/* shoud be a better way */

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
#define	NDBUF	32
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
Readdir(int f, char *p, int onlydirs)
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
	for(i = 0;syssigname[i];i++) if(strncmp(s, syssigname[i], strlen(syssigname[i]))==0){
		if(strncmp(s, "sys: ", 5)!=0) interrupted = 1;
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
Write(int fd, char *buf, long cnt)
{
	return write(fd, buf, (long)cnt);
}

long
Read(int fd, char *buf, long cnt)
{
	return read(fd, buf, cnt);
}

long
Seek(int fd, long cnt, long whence)
{
	return seek(fd, cnt, whence);
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
	return create(file, 1, 0666L);
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
	Dir *d1, *d2;
	int ret;

	d1 = dirfstat(fd);
	if(d1 == nil)
		return 0;
	if(strncmp(d1->name, "ptty", 4)==0){	/* fwd complaints to philw */
		free(d1);
		return 1;
	}
	d2 = dirstat("/dev/cons");
	if(d2 == nil){
		free(d1);
		return 0;
	}
	ret = (d1->type==d2->type&&d1->dev==d2->dev&&d1->qid.path==d2->qid.path);
	free(d1);
	free(d2);
	return ret;
}

void
Abort(void)
{
	pfmt(err, "aborting\n");
	flush(err);
	Exit("aborting");
}

void
Memcpy(char *a, char *b, long n)
{
	memmove(a, b, (long)n);
}

void*
Malloc(ulong n)
{
	return malloc(n);
}
