#include <u.h>
#include <libc.h>
#include <bio.h>
#include "imap4d.h"

int
cistrcmp(char *s1, char *s2)
{
	int c1, c2;

	while(*s1){
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';

		if(c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';

		if(c1 != c2)
			return c1 - c2;
	}
	return -*s2;
}

int
cistrncmp(char *s1, char *s2, int n)
{
	int c1, c2;

	while(*s1 && n-- > 0){
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';

		if(c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';

		if(c1 != c2)
			return c1 - c2;
	}
	if(n == 0)
		return 0;
	return -*s2;
}

char*
cistrstr(char *s, char *sub)
{
	int c, csub, n;

	csub = *sub;
	if(csub == '\0')
		return s;
	if(csub >= 'A' && csub <= 'Z')
		csub -= 'A' - 'a';
	sub++;
	n = strlen(sub);
	for(; c = *s; s++){
		if(c >= 'A' && c <= 'Z')
			c -= 'A' - 'a';
		if(c == csub && cistrncmp(s+1, sub, n) == 0)
			return s;
	}
	return nil;
}

/*
 * reverse string [s:e) in place
 */
void
strrev(char *s, char *e)
{
	int c;

	while(--e > s){
		c = *s;
		*s++ = *e;
		*e = c;
	}
}

int
isdotdot(char *s)
{
	return s[0] == '.' && s[1] == '.' && (s[2] == '/' || s[2] == '\0');
}

int
issuffix(char *suf, char *s)
{
	int n;

	n = strlen(s) - strlen(suf);
	if(n < 0)
		return 0;
	return strcmp(s + n, suf) == 0;
}

int
isprefix(char *pre, char *s)
{
	return strncmp(pre, s, strlen(pre)) == 0;
}

int
ciisprefix(char *pre, char *s)
{
	return cistrncmp(pre, s, strlen(pre)) == 0;
}

char*
readFile(int fd)
{
	Dir d;
	char *s;

	if(dirfstat(fd, &d) < 0)
		return nil;
	s = canAlloc(&parseCan, d.length + 1, 0);
	if(s == nil)
		return nil;
	if(read(fd, s, d.length) != d.length)
		return 0;
	s[d.length] = '\0';
	return s;
}

/*
 * create the imap tmp file
 */
int
imapTmp(void)
{
	char buf[ERRLEN], name[5*NAMELEN];
	int tries, fd;

	snprint(name, sizeof(name), "/mail/box/%s/mbox.tmp.imp", username);
	for(tries = 0; tries < LockSecs*2; tries++){
		fd = create(name, ORDWR|ORCLOSE|OCEXEC, CHEXCL|0600);
		if(fd >= 0)
			return fd;
		errstr(buf);
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
openLocked(char *dir, char *file, int mode)
{
	char buf[ERRLEN];
	int tries, fd;

	for(tries = 0; tries < LockSecs*2; tries++){
		fd = cdOpen(dir, file, mode);
		if(fd >= 0)
			return fd;
		errstr(buf);
		if(cistrstr(buf, "locked") == nil)
			break;
		sleep(500);
	}
	return -1;
}

ulong
mapInt(NamedInt *map, char *name)
{
	int i;

	for(i = 0; map[i].name != nil; i++)
		if(cistrcmp(map[i].name, name) == 0)
			break;
	return map[i].v;
}

char*
estrdup(char *s)
{
	char *t;

	t = emalloc(strlen(s) + 1);
	strcpy(t, s);
	return t;
}

void*
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		bye("server out of memory");
	return p;
}

void*
ezmalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		bye("server out of memory");
	memset(p, 0, n);
	return p;
}

void*
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(p == nil)
		bye("server out of memory");
	return p;
}
