#include <u.h>
#include <libc.h>
#include <ureg.h>
#include <mp.h>
#include <libsec.h>

#include "dat.h"
#include "fns.h"
#include "linux.h"

enum {
	Qproc,
	Qstat,
	Qcpuinfo,
	Qmeminfo,
	Quptime,
	Qloadavg,
	Qself,
	Qpid,
	Qcwd,
	Qcmdline,
	Qenviron,
	Qexe,
	Qroot,
	Qpidstat,
	Qpidstatm,
	Qstatus,
	Qmaps,
	Qfd,
	Qfd1,
	Qtask,
	Qtask1,
	Qmax,
};

static struct {
	int mode;
	char *name;
} procdevtab[] = {
	0555|S_IFDIR,	"proc",
		0444|S_IFREG,	"stat",
		0444|S_IFREG,	"cpuinfo",
		0444|S_IFREG,	"meminfo",
		0444|S_IFREG,	"uptime",
		0444|S_IFREG,	"loadavg",
		0777|S_IFLNK,	"self",
		0555|S_IFDIR,	"###",
			0777|S_IFLNK,	"cwd",
			0444|S_IFREG,	"cmdline",
			0444|S_IFREG,	"environ",
			0777|S_IFLNK,	"exe",
			0777|S_IFLNK,	"root",
			0444|S_IFREG,	"stat",
			0444|S_IFREG,	"statm",
			0444|S_IFREG,	"status",
			0444|S_IFREG,	"maps",
			0555|S_IFDIR,	"fd",
				0777|S_IFLNK,	"###",
			0555|S_IFDIR,	"task",
				0555|S_IFDIR,	"###",
};

typedef struct Procfile Procfile;
struct Procfile
{
	Ufile;
	int	q;
	int	pid;
	vlong lastoff;
	char	*data;
	int	ndata;
};

static int
path2q(char *path, int *ppid, int *pfd)
{
	int i, q, pid, fd;
	char *x;

	q = -1;
	pid = -1;
	fd = -1;
	path++;
	for(i=Qproc; i<Qmax; i++){
		if(x = strchr(path, '/'))
			*x = 0;
		if(path[0]>='0' && path[0]<='9'){
			switch(i){
			case Qpid:
			case Qtask1:
				pid = atoi(path);
				goto match;
			case Qfd1:
				fd = atoi(path);
				goto match;
			}
		}
		if(strcmp(path, procdevtab[i].name) == 0){
match:		if(x == nil){
				q = i;
				break;
			}
			if(i == Qself){	/* hack */
				pid = current->pid;
				i = Qpid;
			}
			if((procdevtab[i].mode & ~0777) == S_IFDIR){
				path = x+1;
				if(i == Qtask1)
					i = Qpid;
			}
		}
		if(x != nil)
			*x = '/';
	}
	if(ppid)
		*ppid = pid;
	if(pfd)
		*pfd = fd;
	return q;
}

/*
 * the proc device also implements the functionality
 * for /dev/std^(in out err) and /dev/fd. we just
 * rewrite the path to the names used in /proc.
 */
static char*
rewritepath(char *path)
{
	if(strcmp(path, "/dev/stdin")==0){
		path = kstrdup("/proc/self/fd/0");
	} else if(strcmp(path, "/dev/stdout")==0){
		path = kstrdup("/proc/self/fd/1");
	} else if(strcmp(path, "/dev/stderr")==0){
		path = kstrdup("/proc/self/fd/2");
	} else if(strncmp(path, "/dev/fd", 7) == 0){
		path = allocpath("/proc/self", "fd", path+7);
	} else {
		path = kstrdup(path);
	}
	return path;
}

static int
readlinkproc(char *path, char *buf, int len);

static int
openproc(char *path, int mode, int perm, Ufile **pf)
{
	char buf[256], *t;
	int n, q, pid, err;
	Procfile *f;

	err = -ENOENT;
	path = rewritepath(path);
	if((q = path2q(path, &pid, nil)) < 0)
		goto out;
	if((procdevtab[q].mode & ~0777) == S_IFLNK){
		n = readlinkproc(path, buf, sizeof(buf)-1);
		if(n > 0){
			buf[n] = 0;
			err = fsopen(buf, mode, perm, pf);
		}
		goto out;
	}
	if((mode & O_ACCMODE) != O_RDONLY){
		err = -EPERM;
		goto out;
	}
	if(q >= Qpid){
		qlock(&proctab);
		if(getproc(pid) == nil){
			qunlock(&proctab);
			goto out;
		}
		qunlock(&proctab);
	}

	/* hack */
	if(strncmp(path, "/proc/self", 10) == 0){
		t = ksmprint("/proc/%d%s", pid, path+10);
		free(path); path = t;
	}

	f = kmallocz(sizeof(*f), 1);
	f->ref = 1;
	f->mode = mode;
	f->path = path; path = nil;
	f->fd = -1;
	f->dev = PROCDEV;
	f->q = q;
	f->pid = pid;
	*pf = f;
	err = 0;

out:
	free(path);
	return err;
}

static int
closeproc(Ufile *file)
{
	Procfile *f = (Procfile*)file;

	if(f->data)
		free(f->data);
	return 0;
}

enum {
	SScpu,
	SSswitches,
	SSinterrupts,
	SSsyscalls,
	SSpagefaults,
	SStlbmisses,
	SStlbpurges,
	SSloadavg,
	SSidletime,
	SSintrtime,
	SSmax,
};

static char*
sysstat(ulong *prun, ulong *pidle, ulong *pload)
{
	char buf[1024], *p, *e, *t, *data;
	ulong dt, swtch, user, sys, load;
	static ulong run, idle, intr;
	int n, fd;

	data = nil;
	swtch = user = sys = load = 0;

	dt = (ulong)(((nsec() - boottime) * HZ) / 1000000000LL) - run;
	run += dt;

	n = 0;
	if((fd = open("/dev/sysstat", OREAD)) >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		close(fd);
	}
	if(n > 0){
		buf[n] = 0;
		p = buf;
		while(e = strchr(p, '\n')){
			char *f[SSmax];
			*e = 0;
			if(getfields(p, f, SSmax, 1, "\t ") != SSmax)
				break;

			if(p == buf){
				swtch += atoi(f[SSswitches]);

				idle += (atoi(f[SSidletime]) * dt)/100;
				intr += (atoi(f[SSintrtime]) * dt)/100;

				load = 100-atoi(f[SSidletime]);

				user = run - idle - intr;
				sys = run - user;

				data = ksmprint("cpu %lud %lud %lud %lud %lud %lud %lud\n",
					user, 0UL, sys, idle, 0UL, intr, 0UL);
			}
			t = ksmprint("%scpu%d %lud %lud %lud %lud %lud %lud %lud\n",
				data, atoi(f[SScpu]), user, 0UL, sys, idle, 0UL, intr, 0UL);
			free(data);
			data = t;

			p = e+1;
		}
		t = ksmprint("%sbtime %lud\nctxt %lud\n", data, 
				(ulong)(boottime/1000000000LL), swtch);
		free(data);
		data = t;
	}
	if(prun)
		*prun = run;
	if(pidle)
		*pidle = idle;
	if(pload)
		*pload = load;

	return data;
}

static char*
procstat(Uproc *p)
{
	return
		(p->wstate & WEXITED) ? "Z (zombie)" : 
		(p->wstate & WSTOPPED) ? "T (stopped)" :
		(p->state == nil) ? "R (running)" : "S (sleeping)";
}

static char*
procname(Uproc *p)
{
	char *s;

	p = getproc(p->pid);
	if(p == nil || p->comm == nil)
		return "";
	if(s = strrchr(p->comm, '/'))
		return s+1;
	return p->comm;
}


static void
gendata(Procfile *f)
{
	char *s, *t;
	int i, nproc, nready;
	ulong tms[4];
	Uproc *p;

	f->ndata = 0;
	if(s = f->data){
		f->data = nil;
		free(s);
	}
	s = nil;

	if(f->q >= Qpid){
		ulong vmsize, vmdat, vmlib, vmshr, vmstk, vmexe;

		qlock(&proctab);
		if((p = getproc(f->pid)) == nil){
			qunlock(&proctab);
			return;
		}
		switch(f->q){
		case Qcmdline:
			p = getproc(p->pid);
			if(p == nil || p->comm == nil)
				break;
			i = strlen(p->comm)+1;
			if(i >= p->ncomm-2)
				break;
			f->ndata = p->ncomm-i-2;
			f->data = kmalloc(f->ndata);
			memmove(f->data, p->comm + i, f->ndata);
			qunlock(&proctab);
			return;

		case Qenviron:
			break;
		case Qpidstat:
			if(proctimes(p, tms) != 0)
				memset(tms, 0, sizeof(tms));
			vmsize = procmemstat(p, nil, nil, nil, nil, nil);
			s = ksmprint(
				"%d (%s) %c %d %d %d %d %d %lud %lud "
				"%lud %lud %lud %lud %lud %ld %ld %ld %ld %ld "
				"%ld %lud %lud %ld %lud %lud %lud %lud %lud %lud "
				"%lud %lud %lud %lud %lud %lud %lud %d %d\n",
				p->tid,
				procname(p),
				procstat(p)[0],
				p->ppid, 
				p->pgid,
				p->psid,
				0,	/* tty */
				0,	/* tty pgrp */
				0UL,	/* flags */
				0UL, 0UL, 0UL, 0UL,	/* pagefault stats */
				tms[0],	/* utime */
				tms[1],	/* stime */
				tms[2],	/* cutime */
				tms[3],	/* cstime */
				0UL,	/* priority */
				0UL,	/* nice */
				0UL,	/* always 0UL */
				0UL,	/* time to next alarm */
				(ulong)(((p->starttime - boottime) * HZ) / 1000000000LL),
				vmsize,	/* vm size in bytes */
				vmsize,	/* vm working set */
				0UL,	/* rlim */
				p->codestart,
				p->codeend,
				p->stackstart,
				0UL,	/* SP */
				0UL,	/* PC */
				0UL,	/* pending signal mask */
				0UL,	/* blocked signal mask */
				0UL,	/* ignored signal mask */
				0UL,	/* catched signal mask */
				0UL,	/* wchan */
				0UL,	/* nswap */
				0UL,	/* nswap children */
				p->exitsignal,
				0);	/* cpu */
			break;
		case Qpidstatm:
			vmsize = procmemstat(p, &vmdat, &vmlib, &vmshr, &vmstk, &vmexe);
			s = ksmprint("%lud %lud %lud %lud %lud %lud %lud\n", 
				vmsize/PAGESIZE, vmsize/PAGESIZE, vmshr/PAGESIZE,
				vmexe/PAGESIZE, vmstk/PAGESIZE, vmlib/PAGESIZE, 0UL);
			break;
		case Qstatus:
			s = ksmprint(
				"Name:\t%s\n"
				"State:\t%s\n"
				"Tgid:\t%d\n"
				"Pid:\t%d\n"
				"PPid:\t%d\n"
				"Uid:\t%d\t%d\t%d\t%d\n"
				"Gid:\t%d\t%d\t%d\t%d\n"
				"FDSize:\t%d\n"
				"Threads:\t%d\n",
				procname(p),
				procstat(p),
				p->pid,
				p->tid,
				p->ppid,
				p->uid, p->uid, p->uid, p->uid,
				p->gid, p->gid, p->gid, p->gid,
				MAXFD,
				threadcount(p->pid));
			break;
		case Qmaps:
			break;
		}
		qunlock(&proctab);
	} else {
		ulong run, idle, load;

		nproc = nready = 0;
		qlock(&proctab);
		for(i=0; i<MAXPROC; i++){
			p = getprocn(i);
			if(p == nil)
				continue;
			nproc++;
			if(p->state == nil)
				nready++;
		}
		i = proctab.nextpid;
		qunlock(&proctab);

		switch(f->q){
		case Qstat:
			s = sysstat(nil, nil, nil);
			t = ksmprint(
				"%s"
				"processes %d\n"
				"procs_running %d\n"
				"procs_blocked %d\n",
				s,
				i, 
				nready,
				nproc-nready);
			free(s);
			s = t;
			break;
		case Qcpuinfo:
			break;
		case Qmeminfo:
			break;
		case Quptime:
			free(sysstat(&run, &idle, nil));
			s = ksmprint("%lud.%lud %lud.%lud\n", run/HZ, run%HZ, idle/HZ, idle%HZ);
			break;
		case Qloadavg:
			free(sysstat(nil, nil, &load));
			s = ksmprint("%lud.%lud 0 0 %d/%d %d\n", load/100, load%100, nready, nproc, i);
			break;
		}
	}

	f->data = s;
	f->ndata = s ? strlen(s) : 0;
}

static vlong
sizeproc(Ufile *file)
{
	Procfile *f = (Procfile*)file;

	if(f->data == nil)
		gendata(f);
	return f->ndata;
}

static int
readproc(Ufile *file, void *buf, int len, vlong off)
{
	Procfile *f = (Procfile*)file;
	int ret;

	if((f->data == nil) || (off != f->lastoff))
		gendata(f);
	ret = 0;
	if(f->data && (off < f->ndata)){
		ret = f->ndata - off;
		if(ret > len)
			ret = len;
		memmove(buf, f->data + off, ret);
		f->lastoff = off + ret;
	}
	return ret;
}

static int
readlinkproc(char *path, char *buf, int len)
{
	int err, q, pid, fd;
	char *data;
	Uproc *p;
	Ufile *a;

	err = -ENOENT;
	path = rewritepath(path);
	if((q = path2q(path, &pid, &fd)) < 0)
		goto out;
	data = nil;
	if(q >= Qpid){
		qlock(&proctab);
		if((p = getproc(pid)) == nil){
			qunlock(&proctab);
			goto out;
		}
		switch(q){
		case Qcwd:
			data = kstrdup(p->cwd);
			break;
		case Qexe:
			p = getproc(p->pid);
			if(p == nil || p->comm == nil)
				break;
			data = kstrdup(p->comm);
			break;
		case Qroot:
			data = kstrdup(p->root ? p->root : "/");
			break;
		case Qfd1:
			a = procfdgetfile(p, fd);
			if(a == nil || a->path == nil){
				putfile(a);
				qunlock(&proctab);
				goto out;
			}
			data = kstrdup(a->path);
			putfile(a);
			break;
		}
		qunlock(&proctab);
	} else {
		switch(q){
		case Qself:
			data = ksmprint("/proc/%d", current->pid);
			break;
		}
	}
	err = 0;
	if(data){
		err = strlen(data);
		if(err > len)
			err = len;
		memmove(buf, data, err);
		free(data);
	}
out:
	free(path);
	return err;
}

static int
readdirproc(Ufile *file, Udirent **pd)
{
	Procfile *f = (Procfile*)file;
	char buf[12];
	Uproc *p;
	Ufile *a;
	int n, i;

	n = 0;
	switch(f->q){
	case Qproc:
		for(i=f->q+1; (procdevtab[i].mode & ~0777) != S_IFDIR; i++){
			if((*pd = newdirent(f->path, procdevtab[i].name, procdevtab[i].mode)) == nil)
				break;
			pd = &((*pd)->next);
			n++;
		}
		/* no break */
	case Qtask:
		qlock(&proctab);
		for(i=0; i<MAXPROC; i++){
			p = getprocn(i);
			if(p == nil)
				continue;
			if((f->q == Qproc) && (p->pid != p->tid))
				continue;
			if((f->q == Qtask) && (p->pid != f->pid))
				continue;
			snprint(buf, sizeof(buf), "%d", p->tid);
			if((*pd = newdirent(f->path, buf, procdevtab[i].mode)) == nil)
				break;
			pd = &((*pd)->next);
			n++;
		}
		qunlock(&proctab);
		break;

	case Qpid:
		if((*pd = newdirent(f->path, procdevtab[Qtask].name, procdevtab[Qtask].mode)) == nil)
			break;
		pd = &((*pd)->next);
		n++;
		/* no break */
	case Qtask1:
		if((*pd = newdirent(f->path, procdevtab[Qfd].name, procdevtab[Qfd].mode)) == nil)
			break;
		pd = &((*pd)->next);
		n++;
		for(i=Qpid+1; (procdevtab[i].mode & ~0777) != S_IFDIR; i++){
			if((*pd = newdirent(f->path, procdevtab[i].name, procdevtab[i].mode)) == nil)
				break;
			pd = &((*pd)->next);
			n++;
		}
		break;

	case Qfd:
		qlock(&proctab);
		if((p = getproc(f->pid)) == nil){
			qunlock(&proctab);
			break;
		}
		for(i=0; i<MAXFD; i++){
			a = procfdgetfile(p, i);
			if(a == nil || a->path == nil){
				putfile(a);
				continue;
			}
			putfile(a);
			snprint(buf, sizeof(buf), "%d", i);
			if((*pd = newdirent(f->path, buf, procdevtab[Qfd1].mode)) == nil)
				break;
			pd = &((*pd)->next);
			n++;
		}
		qunlock(&proctab);
		break;
	}

	return n;
}

static int
statproc(char *path, int, Ustat *s)
{
	int q, pid, fd, uid, gid, err;
	ulong ctime;
	Uproc *p;
	Ufile *a;

	err = -ENOENT;
	path = rewritepath(path);
	if((q = path2q(path, &pid, &fd)) < 0)
		goto out;
	if(q >= Qpid){
		qlock(&proctab);
		if((p = getproc(pid)) == nil){
			qunlock(&proctab);
			goto out;
		}
		if(q == Qfd1){
			a = procfdgetfile(p, fd);
			if(a == nil || a->path == nil){
				putfile(a);
				qunlock(&proctab);
				goto out;
			}
			putfile(a);
		}
		uid = p->uid;
		gid = p->gid;
		ctime = p->starttime/1000000000LL;
		qunlock(&proctab);
	} else {
		uid = current->uid;
		gid = current->gid;
		ctime = boottime/1000000000LL;
	}
	err = 0;
	s->mode = procdevtab[q].mode;
	s->uid = uid;
	s->gid = gid;
	s->size = 0;
	s->ino = hashpath(path);
	s->dev = 0;
	s->rdev = 0;
	s->atime = s->mtime = s->ctime = ctime;
out:
	free(path);
	return err;
}

static int
fstatproc(Ufile *f, Ustat *s)
{
	return fsstat(f->path, 0, s);
};

static Udev procdev =
{
	.open = openproc,
	.read = readproc,
	.size = sizeproc,
	.readlink = readlinkproc,
	.readdir = readdirproc,
	.close = closeproc,
	.stat = statproc,
	.fstat = fstatproc,
};

void procdevinit(void)
{
	devtab[PROCDEV] = &procdev;

	fsmount(&procdev, "/proc");
	fsmount(&procdev, "/dev/fd");
	fsmount(&procdev, "/dev/stdin");
	fsmount(&procdev, "/dev/stdout");
	fsmount(&procdev, "/dev/stderr");
}
