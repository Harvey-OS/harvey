/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

#define	Nwork	16

int localdirstat(int8_t*, Dir*);
int ismatch(int8_t*);
void conflict(int8_t*, int8_t*, ...);
void error(int8_t*, ...);
int isdir(int8_t*);

void worker(int fdf, int fdt, int8_t *from, int8_t *to);
int64_t	nextoff(void);
void	failure(void *, int8_t *note);

QLock	lk;
int64_t	off;

int errors;
int nconf;
int donothing;
int verbose;
int8_t **match;
int nmatch;
int tempspool = 1;
int safeinstall = 1;
int8_t *lroot;
int8_t *rroot;
Db *clientdb;
int skip;
int douid;
int8_t *mkname(int8_t*, int, int8_t*, int8_t*);
int8_t localbuf[10240];
int8_t remotebuf[10240];
int copyfile(int8_t*, int8_t*, int8_t*, Dir*, int, int*);
uint32_t maxnow;
int maxn;
int8_t *timefile;
int timefd;
int samecontents(int8_t*, int8_t*);

Db *copyerr;

typedef struct Res Res;
struct Res
{
	int8_t c;
	int8_t *name;
};

Res *res;
int nres;

void 
addresolve(int c, int8_t *name)
{
	if(name[0] == '/')
		name++;
	res = erealloc(res, (nres+1)*sizeof res[0]);
	res[nres].c = c;
	res[nres].name = name;
	nres++;
}

int
resolve(int8_t *name)
{
	int i, len;

	for(i=0; i<nres; i++){
		len = strlen(res[i].name);
		if(len == 0)
			return res[i].c;
		if(strncmp(name, res[i].name, len) == 0 && (name[len]=='/' || name[len] == 0))
			return res[i].c;
	}
	return '?';
}

void
readtimefile(void)
{
	int n;
	int8_t buf[24];

	if((timefd = open(timefile, ORDWR)) < 0
	&& (timefd = create(timefile, ORDWR|OEXCL, 0666)) < 0)
		return;

	n = readn(timefd, buf, sizeof buf);
	if(n < sizeof buf)
		return;

	maxnow = atoi(buf);
	maxn = atoi(buf+12);
}

void
writetimefile(void)
{
	int8_t buf[24+1];

	snprint(buf, sizeof buf, "%11lud %11d ", maxnow, maxn);
	pwrite(timefd, buf, 24, 0);
}

static void membogus(int8_t**);

void
addce(int8_t *local)
{
	int8_t e[ERRMAX];
	Dir d;

	memset(&d, 0, sizeof d);
	rerrstr(e, sizeof e);
	d.name = atom(e);
	d.uid = "";
	d.gid = "";
	insertdb(copyerr, atom(local), &d);
}

void
delce(int8_t *local)
{
	removedb(copyerr, local);
}

void
chat(int8_t *f, ...)
{
	Fmt fmt;
	int8_t buf[256];
	va_list arg;

	if(!verbose)
		return;

	fmtfdinit(&fmt, 1, buf, sizeof buf);
	va_start(arg, f);
	fmtvprint(&fmt, f, arg);
	va_end(arg);
	fmtfdflush(&fmt);
}

void
usage(void)
{
	fprint(2, "usage: replica/applylog [-cnSstuv] [-T timefile] clientdb clientroot serverroot [path ...]\n");
	exits("usage");
}

int
notexists(int8_t *path)
{
	int8_t buf[ERRMAX];

	if(access(path, AEXIST) >= 0)
		return 0;
	
	rerrstr(buf, sizeof buf);
	if(strstr(buf, "entry not found") || strstr(buf, "not exist"))
		return 1;

	/* some other error, like network hangup */
	return 0;
}

int
prstopped(int skip, int8_t *name)
{
	if(!skip) {
		fprint(2, "stopped updating log apply time because of %s\n",
			name);
		skip = 1;
	}
	return skip;
}

void
main(int argc, char **argv)
{ 
	char *f[10], *local, *name, *remote, *s, *t, verb;
	int fd, havedb, havelocal, i, k, n, nf, resolve1, skip;
	int checkedmatch1, checkedmatch2, 
		checkedmatch3, checkedmatch4;
	ulong now;
	Biobuf bin;
	Dir dbd, ld, nd, rd;
	Avlwalk *w;
	Entry *e;

	membogus(argv);
	quotefmtinstall();
	ARGBEGIN{
	case 's':
	case 'c':
		i = ARGC();
		addresolve(i, EARGF(usage()));
		break;
	case 'n':
		donothing = 1;
		verbose = 1;
		break;
	case 'S':
		safeinstall = 0;
		break;
	case 'T':
		timefile = EARGF(usage());
		break;
	case 't':
		tempspool = 0;
		break;
	case 'u':
		douid = 1;
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
	}ARGEND

	if(argc < 3)
		usage();

	if(timefile)
		readtimefile();

	lroot = argv[1];
	if(!isdir(lroot))
		sysfatal("bad local root directory");
	rroot = argv[2];
	if(!isdir(rroot))
		sysfatal("bad remote root directory");

	match = argv+3;
	nmatch = argc-3;
	for(i=0; i<nmatch; i++)
		if(match[i][0] == '/')
			match[i]++;

	if((clientdb = opendb(argv[0])) == nil)
		sysfatal("opendb %q: %r", argv[2]);
	
	copyerr = opendb(nil);

	skip = 0;
	Binit(&bin, 0, OREAD);
	for(; s=Brdstr(&bin, '\n', 1); free(s)){
		t = estrdup(s);
		nf = tokenize(s, f, nelem(f));
		if(nf != 10 || strlen(f[2]) != 1){
			skip = 1;
			fprint(2, "warning: skipping bad log entry <%s>\n", t);
			free(t);
			continue;
		}
		free(t);
		now = strtoul(f[0], 0, 0);
		n = atoi(f[1]);
		verb = f[2][0];
		name = f[3];
		if(now < maxnow || (now==maxnow && n <= maxn))
			continue;
		local = mkname(localbuf, sizeof localbuf, lroot, name);
		if(strcmp(f[4], "-") == 0)
			f[4] = f[3];
		remote = mkname(remotebuf, sizeof remotebuf, rroot, f[4]);
		rd.name = f[4];
		rd.mode = strtoul(f[5], 0, 8);
		rd.uid = f[6];
		rd.gid = f[7];
		rd.mtime = strtoul(f[8], 0, 10);
		rd.length = strtoll(f[9], 0, 10);
		havedb = finddb(clientdb, name, &dbd)>=0;
		havelocal = localdirstat(local, &ld)>=0;

		resolve1 = resolve(name);

		/*
		 * if(!ismatch(name)){
		 *	skip = 1;
		 *	continue;
		 * }
		 * 
		 * This check used to be right here, but we want
		 * the time to be able to move forward past entries
		 * that don't match and have already been applied.
		 * So now every path below must checked !ismatch(name)
		 * before making any changes to the local file
		 * system.  The fake variable checkedmatch
		 * tracks whether !ismatch(name) has been checked.
		 * If the compiler doesn't produce any used/set
		 * warnings, then all the paths should be okay.
		 * Even so, we have the asserts to fall back on.
		 */
		switch(verb){
		case 'd':	/* delete file */
			delce(local);
			if(!havelocal)	/* doesn't exist; who cares? */
				break;
			if(access(remote, AEXIST) >= 0)	/* got recreated! */
				break;
			if(!ismatch(name)){
				skip = prstopped(skip, name);
				continue;
			}
			SET(checkedmatch1);
			if(!havedb){
				if(resolve1 == 's')
					goto DoRemove;
				else if(resolve1 == 'c')
					goto DoRemoveDb;
				conflict(name, "locally created; will not remove");
				skip = 1;
				continue;
			}
			assert(havelocal && havedb);
			if(dbd.mtime > rd.mtime)		/* we have a newer file than what was deleted */
				break;
			if(samecontents(local, remote) > 0){	/* going to get recreated */
				chat("= %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
				break;
			}
			if(!(dbd.mode&DMDIR) && (dbd.mtime != ld.mtime || dbd.length != ld.length)){	/* locally modified since we downloaded it */
				if(resolve1 == 's')
					goto DoRemove;
				else if(resolve1 == 'c')
					break;
				conflict(name, "locally modified; will not remove");
				skip = 1;
				continue;
			}
		    DoRemove:
			USED(checkedmatch1);
			assert(ismatch(name));
			chat("d %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
			if(donothing)
				break;
			if(remove(local) < 0){
				error("removing %q: %r", name);
				skip = 1;
				continue;
			}
		    DoRemoveDb:
			USED(checkedmatch1);
			assert(ismatch(name));
			removedb(clientdb, name);
			break;

		case 'a':	/* add file */
			if(!havedb){
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch2);
				if(!havelocal)
					goto DoCreate;
				if((ld.mode&DMDIR) && (rd.mode&DMDIR))
					break;
				if(samecontents(local, remote) > 0){
					chat("= %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
					goto DoCreateDb;
				}
				if(resolve1 == 's')
					goto DoCreate;
				else if(resolve1 == 'c')
					goto DoCreateDb;
				conflict(name, "locally created; will not overwrite");
				skip = 1;
				continue;
			}
			assert(havedb);
			if(dbd.mtime >= rd.mtime)	/* already created this file; ignore */
				break;
			if(havelocal){
				if((ld.mode&DMDIR) && (rd.mode&DMDIR))
					break;
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch2);
				if(samecontents(local, remote) > 0){
					chat("= %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
					goto DoCreateDb;
				}
				if(dbd.mtime==ld.mtime && dbd.length==ld.length)
					goto DoCreate;
				if(resolve1=='s')
					goto DoCreate;
				else if(resolve1 == 'c')
					goto DoCreateDb;
				conflict(name, "locally modified; will not overwrite");
				skip = 1;
				continue;
			}
			if(!ismatch(name)){
				skip = prstopped(skip, name);
				continue;
			}
			SET(checkedmatch2);
		    DoCreate:
			USED(checkedmatch2);
			assert(ismatch(name));
			if(notexists(remote)){
				addce(local);
				/* no skip=1 */
				break;;
			}
			chat("a %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
			if(donothing)
				break;
			if(rd.mode&DMDIR){
				fd = create(local, OREAD, DMDIR);
				if(fd < 0 && isdir(local))
					fd = open(local, OREAD);
				if(fd  < 0){
					error("mkdir %q: %r", name);
					skip = 1;
					continue;
				}
				nulldir(&nd);
				nd.mode = rd.mode;
				if(dirfwstat(fd, &nd) < 0)
					fprint(2, "warning: cannot set mode on %q\n", local);
				nulldir(&nd);
				nd.gid = rd.gid;
				if(dirfwstat(fd, &nd) < 0)
					fprint(2, "warning: cannot set gid on %q\n", local);
				if(douid){
					nulldir(&nd);
					nd.uid = rd.uid;
					if(dirfwstat(fd, &nd) < 0)
						fprint(2, "warning: cannot set uid on %q\n", local);
				}
				close(fd);
				rd.mtime = now;
			}else{
				if(copyfile(local, remote, name, &rd, 1, &k) < 0){
					if(k)
						addce(local);
					skip = 1;
					continue;
				}
			}
		    DoCreateDb:
			USED(checkedmatch2);
			assert(ismatch(name));
			insertdb(clientdb, name, &rd);
			break;
			
		case 'c':	/* change contents */
			if(!havedb){
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch3);
				if(resolve1 == 's')
					goto DoCopy;
				else if(resolve1=='c')
					goto DoCopyDb;
				if(samecontents(local, remote) > 0){
					chat("= %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
					goto DoCopyDb;
				}
				if(havelocal)
					conflict(name, "locally created; will not update");
				else
					conflict(name, "not replicated; will not update");
				skip = 1;
				continue;
			}
			if(dbd.mtime >= rd.mtime)		/* already have/had this version; ignore */
				break;
			if(!ismatch(name)){
				skip = prstopped(skip, name);
				continue;
			}
			SET(checkedmatch3);
			if(!havelocal){
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(resolve1 == 's')
					goto DoCopy;
				else if(resolve1 == 'c')
					break;
				conflict(name, "locally removed; will not update");
				skip = 1;
				continue;
			}
			assert(havedb && havelocal);
			if(dbd.mtime != ld.mtime || dbd.length != ld.length){
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(samecontents(local, remote) > 0){
					chat("= %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
					goto DoCopyDb;
				}
				if(resolve1 == 's')
					goto DoCopy;
				else if(resolve1 == 'c')
					goto DoCopyDb;
				conflict(name, "locally modified; will not update [%llud %lud -> %llud %lud]", dbd.length, dbd.mtime, ld.length, ld.mtime);
				skip = 1;
				continue;
			}
		    DoCopy:
			USED(checkedmatch3);
			assert(ismatch(name));
			if(notexists(remote)){
				addce(local);
				/* no skip=1 */
				break;
			}
			chat("c %q\n", name);
			if(donothing)
				break;
			if(copyfile(local, remote, name, &rd, 0, &k) < 0){
				if(k)
					addce(local);
				skip = 1;
				continue;
			}
		    DoCopyDb:
			USED(checkedmatch3);
			assert(ismatch(name));
			if(!havedb){
				if(havelocal)
					dbd = ld;
				else
					dbd = rd;
			}
			dbd.mtime = rd.mtime;
			dbd.length = rd.length;
			insertdb(clientdb, name, &dbd);
			break;			

		case 'm':	/* change metadata */
			if(!havedb){
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch4);
				if(resolve1 == 's'){
					USED(checkedmatch4);
					SET(checkedmatch2);
					goto DoCreate;
				}
				else if(resolve1 == 'c')
					goto DoMetaDb;
				if(havelocal)
					conflict(name, "locally created; will not update metadata");
				else
					conflict(name, "not replicated; will not update metadata");
				skip = 1;
				continue;
			}
			if(!(dbd.mode&DMDIR) && dbd.mtime > rd.mtime)		/* have newer version; ignore */
				break;
			if((dbd.mode&DMDIR) && dbd.mtime > now)
				break;
			if(havelocal && (!douid || strcmp(ld.uid, rd.uid)==0) && strcmp(ld.gid, rd.gid)==0 && ld.mode==rd.mode)
				break;
			if(!havelocal){
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch4);
				if(resolve1 == 's'){
					USED(checkedmatch4);
					SET(checkedmatch2);
					goto DoCreate;
				}
				else if(resolve1 == 'c')
					break;
				conflict(name, "locally removed; will not update metadata");
				skip = 1;
				continue;
			}
			if(!(dbd.mode&DMDIR) && (dbd.mtime != ld.mtime || dbd.length != ld.length)){	/* this check might be overkill */
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch4);
				if(resolve1 == 's' || samecontents(local, remote) > 0)
					goto DoMeta;
				else if(resolve1 == 'c')
					break;
				conflict(name, "contents locally modified (%s); will not update metadata to %s %s %luo",
					dbd.mtime != ld.mtime ? "mtime" :
					dbd.length != ld.length ? "length" : 
					"unknown",
					rd.uid, rd.gid, rd.mode);
				skip = 1;
				continue;
			}
			if((douid && strcmp(ld.uid, dbd.uid)!=0) || strcmp(ld.gid, dbd.gid)!=0 || ld.mode!=dbd.mode){
				if(notexists(remote)){
					addce(local);
					/* no skip=1 */
					break;
				}
				if(!ismatch(name)){
					skip = prstopped(skip, name);
					continue;
				}
				SET(checkedmatch4);
				if(resolve1 == 's')
					goto DoMeta;
				else if(resolve1 == 'c')
					break;
				conflict(name, "metadata locally changed; will not update metadata to %s %s %luo", rd.uid, rd.gid, rd.mode);
				skip = 1;
				continue;
			}
			if(!ismatch(name)){
				skip = prstopped(skip, name);
				continue;
			}
			SET(checkedmatch4);
		    DoMeta:
			USED(checkedmatch4);
			assert(ismatch(name));
			if(notexists(remote)){
				addce(local);
				/* no skip=1 */
				break;
			}
			chat("m %q %luo %q %q %lud\n", name, rd.mode, rd.uid, rd.gid, rd.mtime);
			if(donothing)
				break;
			nulldir(&nd);
			nd.gid = rd.gid;
			nd.mode = rd.mode;
			if(douid)
				nd.uid = rd.uid;
			if(dirwstat(local, &nd) < 0){
				error("dirwstat %q: %r", name);
				skip = 1;
				continue;
			}
		    DoMetaDb:
			USED(checkedmatch4);
			assert(ismatch(name));
			if(!havedb){
				if(havelocal)
					dbd = ld;
				else
					dbd = rd;
			}
			if(dbd.mode&DMDIR)
				dbd.mtime = now;
			dbd.gid = rd.gid;
			dbd.mode = rd.mode;
			if(douid)
				dbd.uid = rd.uid;
			insertdb(clientdb, name, &dbd);
			break;
		}
		if(!skip && !donothing){
			maxnow = now;
			maxn = n;
		}
	}

	w = avlwalk(copyerr->avl);
	while(e = (Entry*)avlnext(w))
		error("copying %q: %s\n", e->name, e->d.name);

	if(timefile)
		writetimefile();
	if(nconf)
		exits("conflicts");

	if(errors)
		exits("errors");
	exits(nil);
}


int8_t*
mkname(int8_t *buf, int nbuf, int8_t *a, int8_t *b)
{
	if(strlen(a)+strlen(b)+2 > nbuf)
		sysfatal("name too long");

	strcpy(buf, a);
	if(a[strlen(a)-1] != '/')
		strcat(buf, "/");
	strcat(buf, b);
	return buf;
}

int
isdir(int8_t *s)
{
	uint32_t m;
	Dir *d;

	if((d = dirstat(s)) == nil)
		return 0;
	m = d->mode;
	free(d);
	return (m&DMDIR) != 0;
}

void
conflict(int8_t *name, int8_t *f, ...)
{
	int8_t *s;
	va_list arg;

	va_start(arg, f);
	s = vsmprint(f, arg);
	va_end(arg);

	fprint(2, "! %s: %s\n", name, s);
	free(s);

	nconf++;
}

void
error(int8_t *f, ...)
{
	int8_t *s;
	va_list arg;

	va_start(arg, f);
	s = vsmprint(f, arg);
	va_end(arg);
	fprint(2, "error: %s\n", s);
	free(s);
	errors = 1;
}

int
ismatch(int8_t *s)
{
	int i, len;

	if(nmatch == 0)
		return 1;
	for(i=0; i<nmatch; i++){
		len = strlen(match[i]);
		if(len == 0)
			return 1;
		if(strncmp(s, match[i], len) == 0 && (s[len]=='/' || s[len] == 0))
			return 1;
	}
	return 0;
}

int
localdirstat(int8_t *name, Dir *d)
{
	static Dir *d2;

	free(d2);
	if((d2 = dirstat(name)) == nil)
		return -1;
	*d = *d2;
	return 0;
}

enum { DEFB = 8192 };

static int
cmp1(int fd1, int fd2)
{
	int8_t buf1[DEFB];
	int8_t buf2[DEFB];
	int n1, n2;
	
	for(;;){
		n1 = readn(fd1, buf1, DEFB);
		n2 = readn(fd2, buf2, DEFB);
		if(n1 < 0 || n2 < 0)
			return -1;
		if(n1 != n2)
			return 0;
		if(n1 == 0)
			return 1;
		if(memcmp(buf1, buf2, n1) != 0)
			return 0;
	}
}

static int
copy1(int fdf, int fdt, int8_t *from, int8_t *to)
{
	int i, n, rv, pid[Nwork];
	Waitmsg *w;

	n = 0;
	off = 0;
	for(i=0; i<Nwork; i++){
		switch(pid[n] = rfork(RFPROC|RFMEM)){
		case 0:
			notify(failure);
			worker(fdf, fdt, from, to);
		case -1:
			break;
		default:
			n++;
			break;
		}
	}
	if(n == 0){
		fprint(2, "cp: rfork: %r\n");
		return -1;
	}

	rv = 0;
	while((w = wait()) != nil){
		if(w->msg[0]){
			rv = -1;
			for(i=0; i<n; i++)
				if(pid[i] > 0)
					postnote(PNPROC, pid[i], "failure");
		}
		free(w);
	}
	return rv;
}

void
worker(int fdf, int fdt, int8_t *from, int8_t *to)
{
	int8_t buf[DEFB], *bp;
	int32_t len, n;
	int64_t o;

	len = sizeof(buf);
	bp = buf;
	o = nextoff();

	while(n = pread(fdf, bp, len, o)){
		if(n < 0){
			fprint(2, "reading %s: %r\n", from);
			_exits("bad");
		}
		if(pwrite(fdt, buf, n, o) != n){
			fprint(2, "writing %s: %r\n", to);
			_exits("bad");
		}
		bp += n;
		o += n;
		len -= n;
		if(len == 0){
			len = sizeof buf;
			bp = buf;
			o = nextoff();
		}
	}
	_exits(nil);
}

int64_t
nextoff(void)
{
	int64_t o;

	qlock(&lk);
	o = off;
	off += DEFB;
	qunlock(&lk);

	return o;
}

void
failure(void*, int8_t *note)
{
	if(strcmp(note, "failure") == 0)
		_exits(nil);
	noted(NDFLT);
}


static int
opentemp(int8_t *template)
{
	int fd, i;
	int8_t *p;

	p = estrdup(template);
	fd = -1;
	for(i=0; i<10; i++){
		mktemp(p);
		if((fd=create(p, ORDWR|OEXCL|ORCLOSE, 0000)) >= 0)
			break;
		strcpy(p, template);
	}
	if(fd < 0)
		return -1;
	strcpy(template, p);
	free(p);
	return fd;
}

static int
copytotemp(int8_t *remote, int rfd, Dir *d0)
{
	int tfd;
	int8_t tmp[32];
	Dir *d1;

	strcpy(tmp, "/tmp/replicaXXXXXXXX");
	tfd = opentemp(tmp);
	if(tfd < 0)
		return -1;
	if(copy1(rfd, tfd, remote, tmp) < 0 || (d1 = dirfstat(rfd)) == nil){
		close(tfd);
		return -1;
	}

	if(d0->qid.path != d1->qid.path
	|| d0->qid.vers != d1->qid.vers
	|| d0->mtime != d1->mtime
	|| d0->length != d1->length){
		/* file changed underfoot; go around again */
		free(d1);
		close(tfd);
		return -2;
	}
	free(d1);
	return tfd;
}

int
copyfile(int8_t *local, int8_t *remote, int8_t *name, Dir *d, int dowstat,
	int *printerror)
{
	Dir *d0, *dl;
	Dir nd;
	int rfd, tfd, wfd, didcreate;
	int8_t tmp[32], *p, *safe;
	int8_t err[ERRMAX];

	do {
		*printerror = 0;
		if((rfd = open(remote, OREAD)) < 0)
			return -1;

		d0 = dirfstat(rfd);
		if(d0 == nil){
			close(rfd);
			return -1;
		}
		*printerror = 1;
		if(!tempspool){
			tfd = rfd;
			goto DoCopy;
		}

		tfd = copytotemp(remote, rfd, d0);
		close(rfd);
		if (tfd < 0) {
			free(d0);
			if (tfd == -1)
				return -1;
		}
	} while(tfd == -2);
	if(seek(tfd, 0, 0) != 0){
		close(tfd);
		free(d0);
		return -1;
	}

DoCopy:
	/*
	 * clumsy but important hack to do safeinstall-like installs.
	 */
	p = strchr(name, '/');
	if(safeinstall && p && strncmp(p, "/bin/", 5) == 0 && access(local, AEXIST) >= 0){
		/* 
		 * remove bin/_targ
		 */
		safe = emalloc(strlen(local)+2);
		strcpy(safe, local);
		p = strrchr(safe, '/')+1;
		memmove(p+1, p, strlen(p)+1);
		p[0] = '_';
		remove(safe);	/* ignore failure */

		/*
		 * rename bin/targ to bin/_targ
		 */
		nulldir(&nd);
		nd.name = p;
		if(dirwstat(local, &nd) < 0)
			fprint(2, "warning: rename %s to %s: %r\n", local, p);
	}

	didcreate = 0;
	if((dl = dirstat(local)) == nil){
		if((wfd = create(local, OWRITE, 0)) >= 0){
			didcreate = 1;
			goto okay;
		}
		goto err;
	}else{
		if((wfd = open(local, OTRUNC|OWRITE)) >= 0)
			goto okay;
		rerrstr(err, sizeof err);
		if(strstr(err, "permission") == nil)
			goto err;
		nulldir(&nd);
		/*
		 * Assume the person running pull is in the appropriate
		 * groups.  We could set 0666 instead, but I'm worried
		 * about leaving the file world-readable or world-writable
		 * when it shouldn't be.
		 */
		nd.mode = dl->mode | 0660;
		if(nd.mode == dl->mode)
			goto err;
		if(dirwstat(local, &nd) < 0)
			goto err;
		if((wfd = open(local, OTRUNC|OWRITE)) >= 0){
			nd.mode = dl->mode;
			if(dirfwstat(wfd, &nd) < 0)
				fprint(2, "warning: set mode on %s to 0660 to open; cannot set back to %luo: %r\n", local, nd.mode);
			goto okay;
		}
		nd.mode = dl->mode;
		if(dirwstat(local, &nd) < 0)
			fprint(2, "warning: set mode on %s to %luo to open; open failed; cannot set mode back to %luo: %r\n", local, nd.mode|0660, nd.mode);
		goto err;
	}
		
err:
	close(tfd);
	free(d0);
	free(dl);
	return -1;

okay:
	free(dl);
	if(copy1(tfd, wfd, tmp, local) < 0){
		close(tfd);
		close(wfd);
		free(d0);
		return -1;
	}
	close(tfd);
	if(didcreate || dowstat){
		nulldir(&nd);
		nd.mode = d->mode;
		if(dirfwstat(wfd, &nd) < 0)
			fprint(2, "warning: cannot set mode on %s\n", local);
		nulldir(&nd);
		nd.gid = d->gid;
		if(dirfwstat(wfd, &nd) < 0)
			fprint(2, "warning: cannot set gid on %s\n", local);
		if(douid){
			nulldir(&nd);
			nd.uid = d->uid;
			if(dirfwstat(wfd, &nd) < 0)
				fprint(2, "warning: cannot set uid on %s\n", local);
		}
	}
	d->mtime = d0->mtime;
	d->length = d0->length;
	nulldir(&nd);
	nd.mtime = d->mtime;
	if(dirfwstat(wfd, &nd) < 0)
		fprint(2, "warning: cannot set mtime on %s\n", local);
	free(d0);

	close(wfd);
	return 0;
}

int
samecontents(int8_t *local, int8_t *remote)
{
	Dir *d0, *d1;
	int rfd, tfd, lfd, ret;

	/* quick check: sizes must match */
	d1 = nil;
	if((d0 = dirstat(local)) == nil || (d1 = dirstat(remote)) == nil){
		free(d0);
		free(d1);
		return -1;
	}
	if(d0->length != d1->length){
		free(d0);
		free(d1);
		return 0;
	}

	do {
		if((rfd = open(remote, OREAD)) < 0)
			return -1;
		d0 = dirfstat(rfd);
		if(d0 == nil){
			close(rfd);
			return -1;
		}

		tfd = copytotemp(remote, rfd, d0);
		close(rfd);
		free(d0);
		if (tfd == -1)
			return -1;
	} while(tfd == -2);
	if(seek(tfd, 0, 0) != 0){
		close(tfd);
		return -1;
	}

	/*
	 * now compare
	 */
	if((lfd = open(local, OREAD)) < 0){
		close(tfd);
		return -1;
	}
	
	ret = cmp1(lfd, tfd);
	close(lfd);
	close(tfd);
	return ret;
}

/*
 * Applylog might try to overwrite itself.
 * To avoid problems with this, we copy ourselves
 * into /tmp and then re-exec.
 */
int8_t *rmargv0;

static void
rmself(void)
{
	remove(rmargv0);
}

static int
genopentemp(int8_t *template, int mode, int perm)
{
	int fd, i;
	int8_t *p;	

	p = estrdup(template);
	fd = -1;
	for(i=0; i<10; i++){
		mktemp(p);
		if(access(p, 0) < 0 && (fd=create(p, mode, perm)) >= 0)
			break;
		strcpy(p, template);
	}
	if(fd < 0)
		sysfatal("could not create temporary file");

	strcpy(template, p);
	free(p);

	return fd;
}

static void
membogus(int8_t **argv)
{
	int n, fd, wfd;
	int8_t template[50], buf[1024];

	if(strncmp(argv[0], "/tmp/_applylog_", 1+3+1+1+8+1)==0) {
		rmargv0 = argv[0];
		atexit(rmself);
		return;
	}

	if((fd = open(argv[0], OREAD)) < 0)
		return;

	strcpy(template, "/tmp/_applylog_XXXXXX");
	if((wfd = genopentemp(template, OWRITE, 0700)) < 0)
		return;

	while((n = read(fd, buf, sizeof buf)) > 0)
		if(write(wfd, buf, n) != n)
			goto Error;

	if(n != 0)
		goto Error;

	close(fd);
	close(wfd);

	argv[0] = template;
	exec(template, argv);
	fprint(2, "exec error %r\n");

Error:
	close(fd);
	close(wfd);
	remove(template);
	return;
}
