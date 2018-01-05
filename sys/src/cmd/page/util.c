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
#include <draw.h>
#include <event.h>
#include <bio.h>
#include "page.h"

void*
emalloc(int sz)
{
	void *v;
	v = malloc(sz);
	if(v == nil) {
		fprint(2, "out of memory allocating %d\n", sz);
		wexits("mem");
	}
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, int sz)
{
	v = realloc(v, sz);
	if(v == nil) {
		fprint(2, "out of memory allocating %d\n", sz);
		wexits("mem");
	}
	return v;
}

char*
estrdup(char *s)
{
	char *t;
	if((t = strdup(s)) == nil) {
		fprint(2, "out of memory in strdup(%.10s)\n", s);
		wexits("mem");
	}
	return t;
}

int
opentemp(char *template)
{
	int fd, i;
	char *p;

	p = estrdup(template);
	fd = -1;
	for(i=0; i<10; i++){
		mktemp(p);
		if(access(p, 0) < 0 && (fd=create(p, ORDWR|ORCLOSE, 0400)) >= 0)
			break;
		strcpy(p, template);
	}
	if(fd < 0){
		fprint(2, "couldn't make temporary file\n");
		wexits("Ecreat");
	}
	strcpy(template, p);
	free(p);

	return fd;
}

/*
 * spool standard input to /tmp.
 * we've already read the initial in bytes into ibuf.
 */
int
spooltodisk(uint8_t *ibuf, int in, char **name)
{
	uint8_t buf[8192];
	int fd, n;
	char temp[40];

	strcpy(temp, "/tmp/pagespoolXXXXXXXXX");
	fd = opentemp(temp);
	if(name)
		*name = estrdup(temp);

	if(write(fd, ibuf, in) != in){
		fprint(2, "error writing temporary file\n");
		wexits("write temp");
	}

	while((n = read(stdinfd, buf, sizeof buf)) > 0){
		if(write(fd, buf, n) != n){
			fprint(2, "error writing temporary file\n");
			wexits("write temp0");
		}
	}
	seek(fd, 0, 0);
	return fd;
}

/*
 * spool standard input into a pipe.
 * we've already ready the first in bytes into ibuf
 */
int
stdinpipe(uint8_t *ibuf, int in)
{
	uint8_t buf[8192];
	int n;
	int p[2];
	if(pipe(p) < 0){
		fprint(2, "pipe fails: %r\n");
		wexits("pipe");
	}

	switch(rfork(RFMEM|RFPROC|RFFDG)){
	case -1:
		fprint(2, "fork fails: %r\n");
		wexits("fork");
	default:
		close(p[1]);
		return p[0];
	case 0:
		break;
	}

	close(p[0]);
	write(p[1], ibuf, in);
	while((n = read(stdinfd, buf, sizeof buf)) > 0)
		write(p[1], buf, n);

	_exits(0);
	return -1;	/* not reached */
}

/* try to update the label, but don't fail on any errors */
void
setlabel(char *label)
{
	char *s;
	int fd;

	s = smprint("%s/label", display->windir);
	if (s == nil)
		return;
	fd = open(s, OWRITE);
	free(s);
	if(fd >= 0){
		write(fd, label, strlen(label));
		close(fd);
	}
	werrstr("");
}
