/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

int readline(char* bud, int len);

/*
int
plumb(char *dir, char *dest, int *efd, char *here)
{
	char buf[128];
	char name[128];
	int n;

	sprint(name, "%s/clone", dir);
	efd[0] = open(name, ORDWR);
	if(efd[0] < 0)
		return -1;
	n = read(efd[0], buf, sizeof(buf)-1);
	if(n < 0){
		close(efd[0]);
		return -1;
	}
	buf[n] = 0;
	sprint(name, "%s/%s/data", dir, buf);
	if(here){
		sprint(buf, "announce %s", here);
		if(sendmsg(efd[0], buf) < 0){
			close(efd[0]);
			return -1;
		}
	}
	sprint(buf, "connect %s", dest);
	if(sendmsg(efd[0], buf) < 0){
		close(efd[0]);
		return -1;
	}
	efd[1] = open(name, ORDWR);
	if(efd[1] < 0){
		close(efd[0]);
		return -1;
	}
	return efd[1];
}
 */

int
sendmsg(int fd, char *msg)
{
	int n;

	n = strlen(msg);
	if(write(fd, msg, n) != n)
		return -1;
	return 0;
}

void
warning(char *s)
{
	char buf[ERRMAX];

	buf[0] = '\0';
	errstr(buf, sizeof buf);
	fprint(2, "boot: %s: %s\n", s, buf);
}

void
fatal(char *s)
{
	char buf[ERRMAX];

	buf[0] = '\0';
	errstr(buf, sizeof buf);
	fprint(2, "boot: %s: %s\n", s, buf);
	exits(0);
}

int
readfile(char *name, char *buf, int len)
{
	int f, n;

	buf[0] = 0;
	f = open(name, OREAD);
	if(f < 0){
		fprint(2, "readfile: cannot open %s (%r)\n", name);
		return -1;
	}
	n = read(f, buf, len-1);
	if(n >= 0)
		buf[n] = 0;
	close(f);
	return 0;
}

int
writefile(char *name, char *buf, int len)
{
	int f, n;

	f = open(name, OWRITE);
	if(f < 0)
		return -1;
	n = write(f, buf, len);
	close(f);
	return (n != len) ? -1 : 0;
}

void
setenv(char *name, char *val)
{
	int f;
	char ename[64];

	snprint(ename, sizeof ename, "#e/%s", name);
	f = create(ename, 1, 0666);
	if(f < 0){
		fprint(2, "create %s: %r\n", ename);
		return;
	}
	write(f, val, strlen(val));
	close(f);
}

void
srvcreate(char *name, int fd)
{
	char *srvname;
	int f;
	char buf[64];

	srvname = strrchr(name, '/');
	if(srvname)
		srvname++;
	else
		srvname = name;

	snprint(buf, sizeof buf, "#s/%s", srvname);
	f = create(buf, 1, 0666);
	if(f < 0)
		fatal(buf);
	sprint(buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		fatal("write");
	close(f);
}

void
catchint(void *a, char *note)
{
	USED(a);
	if(strcmp(note, "alarm") == 0)
		noted(NCONT);
	noted(NDFLT);
}

int
outin(char *prompt, char *def, int len)
{
	int n;
	char buf[256];

	if(len >= sizeof buf)
		len = sizeof(buf)-1;

	if(cpuflag){
		notify(catchint);
		alarm(15*1000);
	} 
	print("%s[%s]: ", prompt, *def ? def : "no default");
	memset(buf, 0, sizeof buf);
	n = readline(buf, sizeof buf);

	if(cpuflag){
		alarm(0);
		notify(0);
	}

	if(n < 0){
		return 1;
	}
	if(n > 1){
		buf[n-1] = 0;
		strcpy(def, buf);
	}
	return n;
}
