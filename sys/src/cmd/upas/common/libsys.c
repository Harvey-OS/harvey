#include "common.h"
#include <auth.h>

/*
 *  number of predefined fd's
 */
int nsysfile=3;
int nofile=128;

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
		return "Liz.Bimmler";
	if((n=read(fd, user, sizeof(user)-1)) <= 0)
		strcpy(user, "Liz.Bimmler");
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

/*
 *  try opening a lock file.  If it doesn't exist try creating it.
 */
static int
openlockfile(char *path)
{
	int fd;
	Dir d;

	fd = open(path, OWRITE);
	if(fd >= 0)
		return fd;
	if(dirstat(path, &d) < 0){
		fd = create(path, OWRITE, CHEXCL|0666);
		if(fd >= 0){
			if(dirfstat(fd, &d) >= 0){
				d.mode |= CHEXCL|0666;
				dirfwstat(fd, &d);
			}
			return fd;
		}
	}
	return -1;
}

#define LSECS 5*60

/*
 *  Set a lock for a particular file.  The lock is a file in the same directory
 *  and has L. prepended to the name of the last element of the file name.
 */
extern Lock *
lock(char *path)
{
	Lock *l;
	int tries;

	l = malloc(sizeof(Lock));
	if(l == 0)
		return 0;

	/*
	 *  wait LSECS seconds for it to unlock
	 */
	l->name = lockname(path);
	for(tries = 0; tries < LSECS*2; tries++){
		l->fd = openlockfile(s_to_c(l->name));
		if(l->fd >= 0)
			return l;
		sleep(500);	/* wait 1/2 second */
	}
	
	s_free(l->name);
	free(l);
	return 0;
}

/*
 *  like lock except don't wait
 */
extern Lock *
trylock(char *path)
{
	Lock *l;

	l = malloc(sizeof(Lock));
	if(l == 0)
		return 0;

	l->name = lockname(path);
	l->fd = openlockfile(s_to_c(l->name));
	
	if(l->fd < 0){
		s_free(l->name);
		free(l);
		return 0;
	}
	return l;
}

extern void
unlock(Lock *l)
{
	if(l == 0)
		return;
	if(l->name){
		s_free(l->name);
	}
	if(l->fd >= 0)
		close(l->fd);
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
	Dir d;
	Biobuf *bp;

	/*
	 *  decode the request
	 */
	sysperm = 0;
	sysmode = -1;
	docreate = 0;
	append = 0;
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
	Binit(bp, fd, sysmode);

	/*
	 *  try opening
	 */
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

	rv = Bclose(bp);
	close(Bfildes(bp));
	free(bp);
	return rv;
}

/*
 *  create a file
 */
int
syscreate(char *file, int mode)
{
	return create(file, ORDWR, mode);
}

/*
 *  make a directory
 */
int
sysmkdir(char *file, ulong perm)
{
	int fd;

	if((fd = create(file, OREAD, 0x80000000L + perm)) < 0)
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

	if(dirstat(file, &d) < 0)
		return -1;
	strncpy(d.gid, group, sizeof(d.gid));
	return dirwstat(file, &d);
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
	if(cp == 0)
		cp = "kremvax";
	strcpy(name, cp);
	return name;
}

/*
 *  return true if the last error message meant file
 *  did not exist.
 */
extern int
e_nonexistant(void)
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
extern ulong
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
 *  kill a process
 */
extern int
syskill(int pid)
{
	char name[64];
	int fd;

	sprint(name, "/proc/%d/note", pid);
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
 *  catch a write on a closed pipe
 */
static int *closedflag;
static int
catchpipe(void *a, char *msg)
{
	static char *foo = "sys: write on closed pipe";

	USED(a);
	if(strncmp(msg, foo, strlen(foo)) == 0){
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
	sprint(buf, "%d", i);
	exits(buf);
}

/*
 *  New process group.  Divest this process of at least signals associated
 *  with other processes.  On Plan 9 fork the name space and environment
 *  variables also.
 */
void
newprocgroup(void)
{
	rfork(RFENVG|RFNAMEG|RFNOTEG);
}

/*
 *  become a powerless user
 */
void
becomenone(void)
{
	int fd;

	fd = open("#c/user", OWRITE);
	if(fd < 0 || write(fd, "none", strlen("none")) < 0)
		fprint(2, "can't become none\n");
	close(fd);
	fd = open("#c/key", OWRITE);
	if(fd >= 0){
		write(fd, "1234567", 7);
		close(fd);
	}
	if(newns("none", 0))
		fprint(2, "can't set new namespace\n");
}
