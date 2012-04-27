#include	<u.h>
#include	<libc.h>
#include	"compat.h"
#include	"error.h"

#include	"errstr.h"

ulong	kerndate;
Proc	**privup;
char	*eve;
extern void *mainmem;

void
_assert(char *fmt)
{
	panic("assert failed: %s", fmt);
}

int
errdepth(int ed)
{
	if(ed >= 0 && up->nerrlab != ed)
		panic("unbalanced error depth: expected %d got %d\n", ed, up->nerrlab);
	return up->nerrlab;
}

void
newup(char *name)
{
	up = smalloc(sizeof(Proc));
	up->user = eve;
	strncpy(up->name, name, KNAMELEN-1);
	up->name[KNAMELEN-1] = '\0';
}

void
kproc(char *name, void (*f)(void *), void *a)
{
	int pid;

	pid = rfork(RFPROC|RFMEM|RFNOWAIT);
	switch(pid){
	case -1:
		panic("can't make new thread: %r");
	case 0:
		break;
	default:
		return;
	}

	newup(name);
	if(!waserror())
		(*f)(a);
	_exits(nil);
}

void
kexit(void)
{
	_exits(nil);
}

void
initcompat(void)
{
	rfork(RFREND);
	privup = privalloc();
	kerndate = seconds();
	eve = getuser();
	newup("main");
}

int
openmode(ulong o)
{
	o &= ~(OTRUNC|OCEXEC|ORCLOSE);
	if(o > OEXEC)
		error(Ebadarg);
	if(o == OEXEC)
		return OREAD;
	return o;
}

void
panic(char *fmt, ...)
{
	char buf[512];
	char buf2[512];
	va_list va;

	va_start(va, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, va);
	va_end(va);
	sprint(buf2, "panic: %s\n", buf);
	write(2, buf2, strlen(buf2));

	exits("error");
}

void*
smalloc(ulong n)
{
	void *p;

	p = mallocz(n, 1);
	if(p == nil)
		panic("out of memory");
	setmalloctag(p, getcallerpc(&n));
	return p;
}

long
seconds(void)
{
	return time(nil);
}

void
error(char *err)
{
	strncpy(up->error, err, ERRMAX);
	nexterror();
}

void
nexterror(void)
{
	longjmp(up->errlab[--up->nerrlab], 1);
}

int
readstr(ulong off, char *buf, ulong n, char *str)
{
	int size;

	size = strlen(str);
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, str+off, n);
	return n;
}

void
_rendsleep(void* tag)
{
	void *value;

	for(;;){
		value = rendezvous(tag, (void*)0x22a891b8);
		if(value == (void*)0x7f7713f9)
			break;
		if(tag != (void*)~0)
			panic("_rendsleep: rendezvous mismatch");
	}
}

void
_rendwakeup(void* tag)
{
	void *value;

	for(;;){
		value = rendezvous(tag, (void*)0x7f7713f9);
		if(value == (void*)0x22a891b8)
			break;
		if(tag != (void*)~0)
			panic("_rendwakeup: rendezvous mismatch");
	}
}

void
rendsleep(Rendez *r, int (*f)(void*), void *arg)
{
	lock(&up->rlock);
	up->r = r;
	unlock(&up->rlock);

	lock(r);

	/*
	 * if condition happened, never mind
	 */
	if(up->intr || f(arg)){
		unlock(r);
		goto Done;
	}

	/*
	 * now we are committed to
	 * change state and call scheduler
	 */
	if(r->p)
		panic("double sleep");
	r->p = up;
	unlock(r);

	_rendsleep(r);

Done:
	lock(&up->rlock);
	up->r = 0;
	if(up->intr){
		up->intr = 0;
		unlock(&up->rlock);
		error(Eintr);
	}
	unlock(&up->rlock);
}

int
rendwakeup(Rendez *r)
{
	Proc *p;
	int rv;

	lock(r);
	p = r->p;
	rv = 0;
	if(p){
		r->p = nil;
		_rendwakeup(r);
		rv = 1;
	}
	unlock(r);
	return rv;
}

void
rendintr(void *v)
{
	Proc *p;

	p = v;
	lock(&p->rlock);
	p->intr = 1;
	if(p->r)
		rendwakeup(p->r);
	unlock(&p->rlock);
}

void
rendclearintr(void)
{
	lock(&up->rlock);
	up->intr = 0;
	unlock(&up->rlock);
}

