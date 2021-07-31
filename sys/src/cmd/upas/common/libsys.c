#include "common.h"
#include <auth.h>
#include <ndb.h>

/*
 *  number of predefined fd's
 */
int nsysfile=3;

static char err[ERRLEN];

/*
 *  return the date
 */
extern char *
thedate(void)
{
	static char now[64];
	char *cp;

	strcpy(now, ctime(time(0)));
	cp = strchr(now, '\n');
	if(cp)
		*cp = 0;
	return now;
}

/*
 *  return the user id of the current user
 */
extern char *
getlog(void)
{
	static char user[64];
	int fd;
	int n;

	fd = open("/dev/user", 0);
	if(fd < 0)
		return nil;
	if((n=read(fd, user, sizeof(user)-1)) <= 0)
		return nil;
	close(fd);
	user[n] = 0;
	return user;
}

/*
 *  return the lock name
 */
static String *
lockname(char *path)
{
	String *lp;
	char *cp;

	/*
	 *  get the name of the lock file
	 */
	lp = s_new();
	cp = strrchr(path, '/');
	if(cp){
		cp++;
		s_nappend(lp, path, cp - path);
	} else {
		
		cp = path;
	}
	s_append(lp, "L.");
	s_nappend(lp, cp, NAMELEN-3);

	return lp;
}

int
syscreatelocked(char *path, int mode, int perm)
{
	return create(path, mode, CHEXCL|perm);
}

int
sysopenlocked(char *path, int mode)
{
/*	return open(path, OEXCL|mode);/**/
	return open(path, mode);		/* until system call is fixed */
}

int
sysunlockfile(int fd)
{
	return close(fd);
}

/*
 *  try opening a lock file.  If it doesn't exist try creating it.
 */
static int
openlockfile(Mlock *l)
{
	int fd;
	Dir d;
	char *p;

	fd = open(s_to_c(l->name), OWRITE);
	if(fd >= 0){
		l->fd = fd;
		return 0;
	}

	if(dirstat(s_to_c(l->name), &d) < 0){
		/* file doesn't exist */
		/* try creating it */
		fd = create(s_to_c(l->name), OWRITE, CHEXCL|0666);
		if(fd >= 0){
			if(dirfstat(fd, &d) >= 0){
				d.mode |= CHEXCL|0666;
				dirfwstat(fd, &d);
			}
			l->fd = fd;
			return 0;
		}

		/* couldn't create */
		/* do we have write access to the directory? */
		p = strrchr(s_to_c(l->name), '/');
		if(p != 0){
			*p = 0;
			fd = access(s_to_c(l->name), 2);
			*p = '/';
			if(fd < 0)
				return -1;	/* give up */
		} else {
			fd = access(".", 2);
			if(fd < 0)
				return -1;	/* give up */
		}
	}

	return 1; /* try again later */
}

#define LSECS 5*60

/*
 *  Set a lock for a particular file.  The lock is a file in the same directory
 *  and has L. prepended to the name of the last element of the file name.
 */
extern Mlock *
syslock(char *path)
{
	Mlock *l;
	int tries;

	l = mallocz(sizeof(Mlock), 1);
	if(l == 0)
		return nil;

	l->name = lockname(path);

	/*
	 *  wait LSECS seconds for it to unlock
	 */
	for(tries = 0; tries < LSECS*2; tries++){
		switch(openlockfile(l)){
		case 0:
			return l;
		case 1:
			sleep(500);
			break;
		default:
			goto noway;
		}
	}

noway:
	s_free(l->name);
	free(l);
	return nil;
}

/*
 *  like lock except don't wait
 */
extern Mlock *
trylock(char *path)
{
	Mlock *l;
	char buf[1];
	int fd;

	l = malloc(sizeof(Mlock));
	if(l == 0)
		return 0;

	l->name = lockname(path);
	if(openlockfile(l) != 0){
		s_free(l->name);
		free(l);
		return 0;
	}
	
	/* fork process to keep lock alive */
	switch(l->pid = rfork(RFPROC)){
	default:
		break;
	case 0:
		fd = l->fd;
		for(;;){
			sleep(1000*60);
			seek(fd, 0, 0);
			if(write(fd, buf, 1) < 0)
				break;
		}
		_exits(0);
	}
	return l;
}

extern void
syslockrefresh(Mlock *l)
{
	char buf[1];

	seek(l->fd, 0, 0);
	read(l->fd, buf, 1);
}

extern void
sysunlock(Mlock *l)
{
	if(l == 0)
		return;
	if(l->name){
		s_free(l->name);
	}
	if(l->fd >= 0)
		close(l->fd);
	if(l->pid > 0)
		postnote(PNPROC, l->pid, "time to die");
	free(l);
}

/*
 *  Open a file.  The modes are:
 *
 *	l	- locked
 *	a	- set append permissions
 *	r	- readable
 *	w	- writable
 *	A	- append only (doesn't exist in Bio)
 */
extern Biobuf *
sysopen(char *path, char *mode, ulong perm)
{
	int sysperm;
	int sysmode;
	int fd;
	int docreate;
	int append;
	int truncate;
	Dir d;
	Biobuf *bp;

	/*
	 *  decode the request
	 */
	sysperm = 0;
	sysmode = -1;
	docreate = 0;
	append = 0;
	truncate = 0;
 	for(; mode && *mode; mode++)
		switch(*mode){
		case 'A':
			sysmode = OWRITE;
			append = 1;
			break;
		case 'c':
			docreate = 1;
			break;
		case 'l':
			sysperm |= CHEXCL;
			break;
		case 'a':
			sysperm |= CHAPPEND;
			break;
		case 'w':
			if(sysmode == -1)
				sysmode = OWRITE;
			else
				sysmode = ORDWR;
			break;
		case 'r':
			if(sysmode == -1)
				sysmode = OREAD;
			else
				sysmode = ORDWR;
			break;
		case 't':
			truncate = 1;
			break;
		default:
			break;
		}
	switch(sysmode){
	case OREAD:
	case OWRITE:
	case ORDWR:
		break;
	default:
		if(sysperm&CHAPPEND)
			sysmode = OWRITE;
		else
			sysmode = OREAD;
		break;
	}

	/*
	 *  create file if we need to
	 */
	if(truncate)
		sysmode |= OTRUNC;
	fd = open(path, sysmode);
	if(fd < 0){
		if(dirstat(path, &d) < 0){
			if(docreate == 0)
				return 0;

			fd = create(path, sysmode, sysperm|perm);
			if(fd < 0)
				return 0;
			if(dirfstat(fd, &d) >= 0){
				d.mode |= sysperm|perm;
				dirfwstat(fd, &d);
			}
		} else
			return 0;
	}

	bp = (Biobuf*)malloc(sizeof(Biobuf));
	if(bp == 0){
		close(fd);
		return 0;
	}
	memset(bp, 0, sizeof(Biobuf));
	Binit(bp, fd, sysmode&~OTRUNC);

	if(append)
		Bseek(bp, 0, 2);
	return bp;
}

/*
 *  close the file, etc.
 */
int
sysclose(Biobuf *bp)
{
	int rv;

	rv = Bterm(bp);
	close(Bfildes(bp));
	free(bp);
	return rv;
}

/*
 *  create a file
 */
int
syscreate(char *file, int mode, ulong perm)
{
	return create(file, mode, perm);
}

/*
 *  make a directory
 */
int
sysmkdir(char *file, ulong perm)
{
	int fd;

	if((fd = create(file, OREAD, CHDIR|perm)) < 0)
		return -1;
	close(fd);
	return 0;
}

/*
 *  change the group of a file
 */
int
syschgrp(char *file, char *group)
{
	Dir d;

	if(group == 0)
		return -1;
	if(dirstat(file, &d) < 0)
		return -1;
	strncpy(d.gid, group, sizeof(d.gid));
	return dirwstat(file, &d);
}

extern int
sysdirread(int fd, Dir *d, int n)
{
	return read(fd, d, n);
}

/*
 *  read in the system name
 */
extern char *
sysname_read(void)
{
	static char name[128];
	char *cp;

	cp = getenv("site");
	if(cp == 0 || *cp == 0)
		cp = alt_sysname_read();
	if(cp == 0 || *cp == 0)
		cp = "kremvax";
	strcpy(name, cp);
	return name;
}
extern char *
alt_sysname_read(void)
{
	static char name[128];
	int n, fd;

	fd = open("/dev/sysname", OREAD);
	if(fd < 0)
		return 0;
	n = read(fd, name, sizeof(name)-1);
	close(fd);
	if(n <= 0)
		return 0;
	name[n] = 0;
	return name;
}

/*
 *  get all names
 */
extern char**
sysnames_read(void)
{
	static char **namev;
	Ndbtuple *t, *nt;
	char domain[Ndbvlen];
	int n;
	char *cp;

	if(namev)
		return namev;

	t = csgetval(0, "sys", alt_sysname_read(), "dom", domain);
	n = 0;
	for(nt = t; nt; nt = nt->entry)
		if(strcmp(nt->attr, "dom") == 0)
			n++;

	namev = (char**)malloc(sizeof(char *)*(n+3));

	if(namev){
		n = 0;
		namev[n++] = strdup(sysname_read());
		cp = alt_sysname_read();
		if(cp)
			namev[n++] = strdup(cp);
		for(nt = t; nt; nt = nt->entry)
			if(strcmp(nt->attr, "dom") == 0)
				namev[n++] = strdup(nt->val);
		namev[n] = 0;
	}
	if(t)
		ndbfree(t);

	return namev;
}

/*
 *  read in the domain name
 */
extern char *
domainname_read(void)
{
	char **namev;

	for(namev = sysnames_read(); *namev; namev++)
		if(strchr(*namev, '.'))
			return *namev;
	return 0;
}

/*
 *  return true if the last error message meant file
 *  did not exist.
 */
extern int
e_nonexistent(void)
{
	errstr(err);
	return strcmp(err, "file does not exist") == 0;
}

/*
 *  return true if the last error message meant file
 *  was locked.
 */
extern int
e_locked(void)
{
	errstr(err);
	return strcmp(err, "open/create -- file is locked") == 0;
}

/*
 *  return the length of a file
 */
extern long
sysfilelen(Biobuf *fp)
{
	Dir	d;

	if(dirfstat(Bfildes(fp), &d)<0)
		return -1;
	return d.length;
}

/*
 *  remove a file
 */
extern int
sysremove(char *path)
{
	return remove(path);
}

/*
 *  rename a file, fails unless both are in the same directory
 */
extern int
sysrename(char *old, char *new)
{
	Dir d;
	char *obase;
	char *nbase;

	obase = strrchr(old, '/');
	nbase = strrchr(new, '/');
	if(obase){
		if(nbase == 0)
			return -1;
		if(strncmp(old, new, obase-old) != 0)
			return -1;
		nbase++;
	} else {
		if(nbase)
			return -1;
		nbase = new;
	}
	if(dirstat(old, &d) < 0)
		return -1;
	memset(d.name, 0, sizeof(d.name));
	strcpy(d.name, nbase);
	return dirwstat(old, &d);
}

/*
 *  see if a file exists
 */
extern int
sysexist(char *file)
{
	Dir	d;

	return dirstat(file, &d) == 0;
}

/*
 * kill a process or process group
 */

static int
stomp(int pid, char *file)
{
	char name[64];
	int fd;

	snprint(name, sizeof(name), "/proc/%d/%s", pid, file);
	fd = open(name, 1);
	if(fd < 0)
		return -1;
	if(write(fd, "die: yankee pig dog\n", sizeof("die: yankee pig dog\n") - 1) <= 0){
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
	
}

/*
 *  kill a process
 */
extern int
syskill(int pid)
{
	return stomp(pid, "note");
	
}

/*
 *  kill a process group
 */
extern int
syskillpg(int pid)
{
	return stomp(pid, "notepg");
}

extern int
sysdetach(void)
{
	if(rfork(RFENVG|RFNAMEG|RFNOTEG) < 0) {
		werrstr("rfork failed");
		return -1;
	}
	return 0;
}

/*
 *  catch a write on a closed pipe
 */
static int *closedflag;
static int
catchpipe(void *a, char *msg)
{
	static char *foo = "sys: write on closed pipe";

	USED(a);
	if(strncmp(msg, foo, strlen(foo)) == 0){
		if(closedflag)
			*closedflag = 1;
		return 1;
	}
	return 0;
}
void
pipesig(int *flagp)
{
	closedflag = flagp;
	atnotify(catchpipe, 1);
}
void
pipesigoff(void)
{
	atnotify(catchpipe, 0);
}

void
exit(int i)
{
	char buf[32];

	if(i == 0)
		exits(0);
	snprint(buf, sizeof(buf), "%d", i);
	exits(buf);
}

/*
 *  become powerless user
 */
int
become(char **cmd, char *who)
{
	int fd;

	USED(cmd);
	if(strcmp(who, "none") == 0) {
		fd = open("#c/user", OWRITE);
		if(fd < 0 || write(fd, "none", strlen("none")) < 0) {
			werrstr("can't become none");
			return -1;
		}
		close(fd);
		if(newns("none", 0)) {
			werrstr("can't set new namespace");
			return -1;
		}
	}
	return 0;
}

static int
islikeatty(int fd)
{
	Dir d;

	if(dirfstat(fd, &d) < 0)
		return 0;
	return strcmp(d.name, "cons") == 0;
}

extern int
holdon(void)
{
	int fd;

	if(!islikeatty(0))
		return -1;

	fd = open("/dev/consctl", OWRITE);
	write(fd, "holdon", 6);

	return fd;
}

extern int
sysopentty(void)
{
	return open("/dev/cons", ORDWR);
}

extern void
holdoff(int fd)
{
	write(fd, "holdoff", 7);
	close(fd);
}

extern int
sysfiles(void)
{
	return 128;
}

/*
 *  expand a path relative to the user's mailbox directory
 *
 *  if the path starts with / or ./, don't change it
 *
 */
extern String *
mboxpath(char *path, char *user, String *to, int dot)
{
	if (dot || *path=='/' || strncmp(path, "./", 2) == 0
			      || strncmp(path, "../", 3) == 0) {
		to = s_append(to, path);
	} else {
		to = s_append(to, MAILROOT);
		to = s_append(to, "/box/");
		to = s_append(to, user);
		to = s_append(to, "/");
		to = s_append(to, path);
	}
	return to;
}

extern String *
mboxname(char *user, String *to)
{
	return mboxpath("mbox", user, to, 0);
}

extern String *
deadletter(String *to)		/* pass in sender??? */
{
	char *cp;

	cp = getlog();
	if(cp == 0)
		return 0;
	return mboxpath("dead.letter", cp, to, 0);
}

char *
homedir(char *user)
{
	USED(user);
	return getenv("home");
}

String *
readlock(String *file)
{
	char *cp;

	cp = getlog();
	if(cp == 0)
		return 0;
	return mboxpath("reading", cp, file, 0);
}

String *
username(String *from)
{
	int n;
	Biobuf *bp;
	char *p, *q;
	String *s;

	bp = Bopen("/adm/keys.who", OREAD);
	if(bp == 0)
		bp = Bopen("/adm/netkeys.who", OREAD);
	if(bp == 0)
		return 0;

	s = 0;
	n = strlen(s_to_c(from));
	for(;;) {
		p = Brdline(bp, '\n');
		if(p == 0)
			break;
		p[Blinelen(bp)-1] = 0;
		if(strncmp(p, s_to_c(from), n))
			continue;
		p += n;
		if(*p != ' ' && *p != '\t')	/* must be full match */
			continue;
		while(*p && (*p == ' ' || *p == '\t'))
				p++;
		if(*p == 0)
			continue;
		for(q = p; *q; q++)
			if(('0' <= *q && *q <= '9') || *q == '<')
				break;
		while(q > p && q[-1] != ' ' && q[-1] != '\t')
			q--;
		while(q > p && (q[-1] == ' ' || q[-1] == '\t'))
			q--;
		*q = 0;
		s = s_new();
		s_append(s, "\"");
		s_append(s, p);
		s_append(s, "\"");
		break;
	}
	Bterm(bp);
	return s;
}

char *
remoteaddr(int fd, char *dir)
{
	char buf[128], *p;
	int n;

	if(dir == 0){
		if(fd2path(fd, buf, sizeof(buf)) != 0)
			return "";

		/* parse something of the form /net/tcp/nnnn/data */
		p = strrchr(buf, '/');
		if(p == 0)
			return "";
		strncpy(p+1, "remote", sizeof(buf)-(p-buf)-2);
	} else
		snprint(buf, sizeof buf, "%s/remote", dir);
	buf[sizeof(buf)-1] = 0;

	fd = open(buf, OREAD);
	if(fd < 0)
		return "";
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n > 0){
		buf[n] = 0;
		p = strchr(buf, '!');
		if(p)
			*p = 0;
		return strdup(buf);
	}
	return "";
}

/*
 *	stub to read locked current directory - this is different in unix version
 */
int
sysreaddot(int fd, Dir *dir, long n)
{
	return dirread(fd, dir, n);

}
