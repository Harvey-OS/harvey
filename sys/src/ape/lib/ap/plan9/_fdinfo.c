#define  _BSDTIME_EXTENSION
#include "lib.h"
#include <sys/stat.h>
#include <stdlib.h>
#include "sys9.h"
#include <string.h>

extern int errno;
Fdinfo _fdinfo[OPEN_MAX];

/*
   called from _envsetup, either with the value of the environment
   variable _fdinfo (from s to se-1), or with s==0 if there was no _fdinfo
*/
static void
defaultfdinit(void)
{
	int i;
	Fdinfo *fi;

	for(i = 0; i <= 2; i++) {
		fi = &_fdinfo[i];
		fi->flags = FD_ISOPEN;
		fi->oflags = (i == 0)? O_RDONLY : O_WRONLY;
		if(_isatty(i))
			fi->flags |= FD_ISTTY;
	}
}

static int
readprocfdinit(void)
{
	/* construct info from /proc/$pid/fd */
	char buf[8192];
	Fdinfo *fi;
	int fd, pfd, pid, n, tot, m;
	char *s, *nexts;

	memset(buf, 0, sizeof buf);
	pfd = _OPEN("#c/pid", 0);
	if(pfd < 0)
		return -1;
	if(_PREAD(pfd, buf, 100, 0) < 0){
		_CLOSE(pfd);
		return -1;
	}
	_CLOSE(pfd);
	pid = strtoul(buf, 0, 10);
	strcpy(buf, "#p/");
	_ultoa(buf+3, pid);
	strcat(buf, "/fd");
	pfd = _OPEN(buf, 0);
	if(pfd < 0)
		return -1;
	memset(buf, 0, sizeof buf);
	tot = 0;
	for(;;){
		n = _PREAD(pfd, buf+tot, sizeof buf-tot, tot);
		if(n <= 0)
			break;
		tot += n;
	}
	_CLOSE(pfd);
	if(n < 0)
		return -1;
	buf[sizeof buf-1] = '\0';
	s = strchr(buf, '\n');	/* skip current directory */
	if(s == 0)
		return -1;
	s++;
	m = 0;
	for(; s && *s; s=nexts){
		nexts = strchr(s, '\n');
		if(nexts)
			*nexts++ = '\0';
		errno = 0;
		fd = strtoul(s, &s, 10);
		if(errno != 0)
			return -1;
		if(fd >= OPEN_MAX)
			continue;
		if(fd == pfd)
			continue;
		fi = &_fdinfo[fd];
		fi->flags = FD_ISOPEN;
		while(*s == ' ' || *s == '\t')
			s++;
		if(*s == 'r'){
			m |= 1;
			s++;
		}
		if(*s == 'w'){
			m |= 2;
		}
		if(m==1)
			fi->oflags = O_RDONLY;
		else if(m==2)
			fi->oflags = O_WRONLY;
		else
			fi->oflags = O_RDWR;
		if(strlen(s) >= 9 && strcmp(s+strlen(s)-9, "/dev/cons") == 0)
			fi->flags |= FD_ISTTY;
	}
	return 0;
}

static void
sfdinit(int usedproc, char *s, char *se)
{
	Fdinfo *fi;
	unsigned long fd, fl, ofl;
	char *e;

	while(s < se){
		fd = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		fl = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		ofl = strtoul(s, &e, 10);
		if(s == e)
			break;
		s = e;
		if(fd < OPEN_MAX){
			fi = &_fdinfo[fd];
			if(usedproc && !(fi->flags&FD_ISOPEN))
				continue;	/* should probably ignore all of $_fdinit */
			fi->flags = fl;
			fi->oflags = ofl;
			if(_isatty(fd))
				fi->flags |= FD_ISTTY;
		}
	}

}

void
_fdinit(char *s, char *se)
{
	int i, usedproc;
	Fdinfo *fi;
	struct stat sbuf;

	usedproc = 0;
	if(readprocfdinit() == 0)
		usedproc = 1;
else
_WRITE(2, "FAILED\n", 7);
	if(s)
		sfdinit(usedproc, s, se);
	if(!s && !usedproc)
		defaultfdinit();

	for(i = 0; i < OPEN_MAX; i++) {
		fi = &_fdinfo[i];
		if(fi->flags&FD_ISOPEN){
			if(fstat(i, &sbuf) >= 0) {
				fi->uid = sbuf.st_uid;
				fi->gid = sbuf.st_gid;
			}
		}
	}
}

