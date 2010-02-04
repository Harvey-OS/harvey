#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

static	int	copyData(int ffd, int tfd, MbLock *ml);
static	MbLock	mLock =
{
	.fd = -1
};

static char curDir[MboxNameLen];

void
resetCurDir(void)
{
	curDir[0] = '\0';
}

int
myChdir(char *dir)
{
	if(strcmp(dir, curDir) == 0)
		return 0;
	if(dir[0] != '/' || strlen(dir) > MboxNameLen)
		return -1;
	strcpy(curDir, dir);
	if(chdir(dir) < 0){
		werrstr("mychdir failed: %r");
		return -1;
	}
	return 0;
}

int
cdCreate(char *dir, char *file, int mode, ulong perm)
{
	if(myChdir(dir) < 0)
		return -1;
	return create(file, mode, perm);
}

Dir*
cdDirstat(char *dir, char *file)
{
	if(myChdir(dir) < 0)
		return nil;
	return dirstat(file);
}

int
cdExists(char *dir, char *file)
{
	Dir *d;

	d = cdDirstat(dir, file);
	if(d == nil)
		return 0;
	free(d);
	return 1;
}

int
cdDirwstat(char *dir, char *file, Dir *d)
{
	if(myChdir(dir) < 0)
		return -1;
	return dirwstat(file, d);
}

int
cdOpen(char *dir, char *file, int mode)
{
	if(myChdir(dir) < 0)
		return -1;
	return open(file, mode);
}

int
cdRemove(char *dir, char *file)
{
	if(myChdir(dir) < 0)
		return -1;
	return remove(file);
}

/*
 * open the one true mail lock file
 */
MbLock*
mbLock(void)
{
	int i;

	if(mLock.fd >= 0)
		bye("mail lock deadlock");
	for(i = 0; i < 5; i++){
		mLock.fd = openLocked(mboxDir, "L.mbox", OREAD);
		if(mLock.fd >= 0)
			return &mLock;
		sleep(1000);
	}
	return nil;
}

void
mbUnlock(MbLock *ml)
{
	if(ml != &mLock)
		bye("bad mail unlock");
	if(ml->fd < 0)
		bye("mail unlock when not locked");
	close(ml->fd);
	ml->fd = -1;
}

void
mbLockRefresh(MbLock *ml)
{
	char buf[1];

	seek(ml->fd, 0, 0);
	read(ml->fd, buf, 1);
}

int
mbLocked(void)
{
	return mLock.fd >= 0;
}

char*
impName(char *name)
{
	char *s;
	int n;

	if(cistrcmp(name, "inbox") == 0)
		if(access("msgs", AEXIST) == 0)
			name = "msgs";
		else
			name = "mbox";
	n = strlen(name) + STRLEN(".imp") + 1;
	s = binalloc(&parseBin, n, 0);
	if(s == nil)
		return nil;
	snprint(s, n, "%s.imp", name);
	return s;
}

/*
 * massage the mailbox name into something valid
 * eliminates all .', and ..',s, redundatant and trailing /'s.
 */
char *
mboxName(char *s)
{
	char *ss;

	ss = mutf7str(s);
	if(ss == nil)
		return nil;
	cleanname(ss);
	return ss;
}

char *
strmutf7(char *s)
{
	char *m;
	int n;

	n = strlen(s) * MUtf7Max + 1;
	m = binalloc(&parseBin, n, 0);
	if(m == nil)
		return nil;
	if(encmutf7(m, n, s) < 0)
		return nil;
	return m;
}

char *
mutf7str(char *s)
{
	char *m;
	int n;

	/*
	 * n = strlen(s) * UTFmax / (2.67) + 1
	 * UTFMax / 2.67 == 3 / (8/3) == 9 / 8
	 */
	n = strlen(s);
	n = (n * 9 + 7) / 8 + 1;
	m = binalloc(&parseBin, n, 0);
	if(m == nil)
		return nil;
	if(decmutf7(m, n, s) < 0)
		return nil;
	return m;
}

void
splitr(char *s, int c, char **left, char **right)
{
	char *d;
	int n;

	n = strlen(s);
	d = binalloc(&parseBin, n + 1, 0);
	if(d == nil)
		parseErr("out of memory");
	strcpy(d, s);
	s = strrchr(d, c);
	if(s != nil){
		*left = d;
		*s++ = '\0';
		*right = s;
	}else{
		*right = d;
		*left = d + n;
	}
}

/*
 * create the mailbox and all intermediate components
 * a trailing / implies the new mailbox is a directory;
 * otherwise, it's a file.
 *
 * return with the file open for write, or directory open for read.
 */
int
createBox(char *mbox, int dir)
{
	char *m;
	int fd;

	fd = -1;
	for(m = mbox; *m; m++){
		if(*m == '/'){
			*m = '\0';
			if(access(mbox, AEXIST) < 0){
				if(fd >= 0)
					close(fd);
				fd = cdCreate(mboxDir, mbox, OREAD, DMDIR|0775);
				if(fd < 0)
					return -1;
			}
			*m = '/';
		}
	}
	if(dir)
		fd = cdCreate(mboxDir, mbox, OREAD, DMDIR|0775);
	else
		fd = cdCreate(mboxDir, mbox, OWRITE, 0664);
	return fd;
}

/*
 * move one mail folder to another
 * destination mailbox doesn't exist.
 * the source folder may be a directory or a mailbox,
 * and may be in the same directory as the destination,
 * or a completely different directory.
 */
int
moveBox(char *from, char *to)
{
	Dir *d;
	char *fd, *fe, *td, *te, *fimp;

	splitr(from, '/', &fd, &fe);
	splitr(to, '/', &td, &te);

	/*
	 * in the same directory: try rename
	 */
	d = cdDirstat(mboxDir, from);
	if(d == nil)
		return 0;
	if(strcmp(fd, td) == 0){
		nulldir(d);
		d->name = te;
		if(cdDirwstat(mboxDir, from, d) >= 0){
			fimp = impName(from);
			d->name = impName(te);
			cdDirwstat(mboxDir, fimp, d);
			free(d);
			return 1;
		}
	}

	/*
	 * directory copy is too hard for now
	 */
	if(d->mode & DMDIR)
		return 0;
	free(d);

	return copyBox(from, to, 1);
}

/*
 * copy the contents of one mailbox to another
 * either truncates or removes the source box if it succeeds.
 */
int
copyBox(char *from, char *to, int doremove)
{
	MbLock *ml;
	char *fimp, *timp;
	int ffd, tfd, ok;

	if(cistrcmp(from, "inbox") == 0)
		if(access("msgs", AEXIST) == 0)
			from = "msgs";
		else
			from = "mbox";

	ml = mbLock();
	if(ml == nil)
		return 0;
	ffd = openLocked(mboxDir, from, OREAD);
	if(ffd < 0){
		mbUnlock(ml);
		return 0;
	}
	tfd = createBox(to, 0);
	if(tfd < 0){
		mbUnlock(ml);
		close(ffd);
		return 0;
	}

	ok = copyData(ffd, tfd, ml);
	close(ffd);
	close(tfd);
	if(!ok){
		mbUnlock(ml);
		return 0;
	}

	fimp = impName(from);
	timp = impName(to);
	if(fimp != nil && timp != nil){
		ffd = cdOpen(mboxDir, fimp, OREAD);
		if(ffd >= 0){
			tfd = cdCreate(mboxDir, timp, OWRITE, 0664);
			if(tfd >= 0){
				copyData(ffd, tfd, ml);
				close(tfd);
			}
			close(ffd);
		}
	}
	cdRemove(mboxDir, fimp);
	if(doremove)
		cdRemove(mboxDir, from);
	else
		close(cdOpen(mboxDir, from, OWRITE|OTRUNC));
	mbUnlock(ml);
	return 1;
}

/*
 * copies while holding the mail lock,
 * then tries to copy permissions and group ownership
 */
static int
copyData(int ffd, int tfd, MbLock *ml)
{
	Dir *fd, td;
	char buf[BufSize];
	int n;

	for(;;){
		n = read(ffd, buf, BufSize);
		if(n <= 0){
			if(n < 0)
				return 0;
			break;
		}
		if(write(tfd, buf, n) != n)
			return 0;
		mbLockRefresh(ml);
	}
	fd = dirfstat(ffd);
	if(fd != nil){
		nulldir(&td);
		td.mode = fd->mode;
		if(dirfwstat(tfd, &td) >= 0){
			nulldir(&td);
			td.gid = fd->gid;
			dirfwstat(tfd, &td);
		}
	}
	return 1;
}
