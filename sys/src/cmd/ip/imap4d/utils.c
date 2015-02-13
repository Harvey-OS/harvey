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
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

/*
 * reverse string [s:e) in place
 */
void
strrev(int8_t *s, int8_t *e)
{
	int c;

	while(--e > s){
		c = *s;
		*s++ = *e;
		*e = c;
	}
}

int
isdotdot(int8_t *s)
{
	return s[0] == '.' && s[1] == '.' && (s[2] == '/' || s[2] == '\0');
}

int
issuffix(int8_t *suf, int8_t *s)
{
	int n;

	n = strlen(s) - strlen(suf);
	if(n < 0)
		return 0;
	return strcmp(s + n, suf) == 0;
}

int
isprefix(int8_t *pre, int8_t *s)
{
	return strncmp(pre, s, strlen(pre)) == 0;
}

int
ciisprefix(int8_t *pre, int8_t *s)
{
	return cistrncmp(pre, s, strlen(pre)) == 0;
}

int8_t*
readFile(int fd)
{
	Dir *d;
	int32_t length;
	int8_t *s;

	d = dirfstat(fd);
	if(d == nil)
		return nil;
	length = d->length;
	free(d);
	s = binalloc(&parseBin, length + 1, 0);
	if(s == nil || read(fd, s, length) != length)
		return nil;
	s[length] = '\0';
	return s;
}

/*
 * create the imap tmp file.
 * it just happens that we don't need multiple temporary files.
 */
int
imapTmp(void)
{
	int8_t buf[ERRMAX], name[MboxNameLen];
	int tries, fd;

	snprint(name, sizeof(name), "/mail/box/%s/mbox.tmp.imp", username);
	for(tries = 0; tries < LockSecs*2; tries++){
		fd = create(name, ORDWR|ORCLOSE|OCEXEC, DMEXCL|0600);
		if(fd >= 0)
			return fd;
		errstr(buf, sizeof buf);
		if(cistrstr(buf, "locked") == nil)
			break;
		sleep(500);
	}
	return -1;
}

/*
 * open a file which might be locked.
 * if it is, spin until available
 */
int
openLocked(int8_t *dir, int8_t *file, int mode)
{
	int8_t buf[ERRMAX];
	int tries, fd;

	for(tries = 0; tries < LockSecs*2; tries++){
		fd = cdOpen(dir, file, mode);
		if(fd >= 0)
			return fd;
		errstr(buf, sizeof buf);
		if(cistrstr(buf, "locked") == nil)
			break;
		sleep(500);
	}
	return -1;
}

int
fqid(int fd, Qid *qid)
{
	Dir *d;

	d = dirfstat(fd);
	if(d == nil)
		return -1;
	*qid = d->qid;
	free(d);
	return 0;
}

uint32_t
mapInt(NamedInt *map, int8_t *name)
{
	int i;

	for(i = 0; map[i].name != nil; i++)
		if(cistrcmp(map[i].name, name) == 0)
			break;
	return map[i].v;
}

int8_t*
estrdup(int8_t *s)
{
	int8_t *t;

	t = emalloc(strlen(s) + 1);
	strcpy(t, s);
	return t;
}

void*
emalloc(uint32_t n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		bye("server out of memory");
	setmalloctag(p, getcallerpc(&n));
	return p;
}

void*
ezmalloc(uint32_t n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		bye("server out of memory");
	setmalloctag(p, getcallerpc(&n));
	memset(p, 0, n);
	return p;
}

void*
erealloc(void *p, uint32_t n)
{
	p = realloc(p, n);
	if(p == nil)
		bye("server out of memory");
	setrealloctag(p, getcallerpc(&p));
	return p;
}
