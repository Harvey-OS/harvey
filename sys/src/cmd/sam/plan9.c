/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "sam.h"

Rune	samname[] = L"~~sam~~";

Rune *left[]= {
	L"{[(<«",
	L"\n",
	L"'\"`",
	0
};
Rune *right[]= {
	L"}])>»",
	L"\n",
	L"'\"`",
	0
};

int8_t	RSAM[] = "sam";
int8_t	SAMTERM[] = "/bin/aux/samterm";
int8_t	HOME[] = "home";
int8_t	TMPDIR[] = "/tmp";
int8_t	SH[] = "rc";
int8_t	SHPATH[] = "/bin/rc";
int8_t	RX[] = "rx";
int8_t	RXPATH[] = "/bin/rx";
int8_t	SAMSAVECMD[] = "/bin/rc\n/sys/lib/samsave";

void
dprint(int8_t *z, ...)
{
	int8_t buf[BLOCKSIZE];
	va_list arg;

	va_start(arg, z);
	vseprint(buf, &buf[BLOCKSIZE], z, arg);
	va_end(arg);
	termwrite(buf);
}

void
print_ss(int8_t *s, String *a, String *b)
{
	dprint("?warning: %s: `%.*S' and `%.*S'\n", s, a->n, a->s, b->n, b->s);
}

void
print_s(int8_t *s, String *a)
{
	dprint("?warning: %s `%.*S'\n", s, a->n, a->s);
}

int
statfile(int8_t *name, uint32_t *dev, uint64_t *id, int32_t *time,
	 int32_t *length,
	 int32_t *appendonly)
{
	Dir *dirb;

	dirb = dirstat(name);
	if(dirb == nil)
		return -1;
	if(dev)
		*dev = dirb->type|(dirb->dev<<16);
	if(id)
		*id = dirb->qid.path;
	if(time)
		*time = dirb->mtime;
	if(length)
		*length = dirb->length;
	if(appendonly)
		*appendonly = dirb->mode & DMAPPEND;
	free(dirb);
	return 1;
}

int
statfd(int fd, uint32_t *dev, uint64_t *id, int32_t *time, int32_t *length,
       int32_t *appendonly)
{
	Dir *dirb;

	dirb = dirfstat(fd);
	if(dirb == nil)
		return -1;
	if(dev)
		*dev = dirb->type|(dirb->dev<<16);
	if(id)
		*id = dirb->qid.path;
	if(time)
		*time = dirb->mtime;
	if(length)
		*length = dirb->length;
	if(appendonly)
		*appendonly = dirb->mode & DMAPPEND;
	free(dirb);
	return 1;
}

void
notifyf(void *a, int8_t *s)
{
	USED(a);
	if(bpipeok && strcmp(s, "sys: write on closed pipe") == 0)
		noted(NCONT);
	if(strcmp(s, "interrupt") == 0)
		noted(NCONT);
	panicking = 1;
	rescue();
	noted(NDFLT);
}

int8_t*
waitfor(int pid)
{
	Waitmsg *w;
	static int8_t msg[ERRMAX];

	while((w = wait()) != nil){
		if(w->pid != pid){
			free(w);
			continue;
		}
		strecpy(msg, msg+sizeof msg, w->msg);
		free(w);
		return msg;
	}
	rerrstr(msg, sizeof msg);
	return msg;
}

void
samerr(int8_t *buf)
{
	sprint(buf, "%s/sam.err", TMPDIR);
}

void*
emalloc(uint32_t n)
{
	void *p;

	p = malloc(n);
	if(p == 0)
		panic("malloc fails");
	memset(p, 0, n);
	return p;
}

void*
erealloc(void *p, uint32_t n)
{
	p = realloc(p, n);
	if(p == 0)
		panic("realloc fails");
	return p;
}
