#include <windows.h>
#include "lib9.h"
#include "sys.h"
#include "error.h"

typedef struct Ttbl	Ttbl;
typedef struct Thread	Thread;

enum {
	Nthread = 100,
	Nerrlbl = 30,
};

struct Thread {
	int	tid;
	int	tindex;

	Thread	*qnext;		/* for qlock */

	int	nerrlbl;
	Jump	errlbl[Nerrlbl];

	char	error[ERRLEN];

	void	(*f)(void *);
	void	*a;

	HANDLE	*sema;

	/* for sleep/wakeup/intr */
	Lock	rlock;
	Rendez	*r;
	int	intr;
};

struct Ttbl {
	Lock	lk;
	int	nthread;
	int	nalloc;
	Thread	*t[Nthread];
	Thread	*free;
};

char	*argv0;
_declspec(thread)       Thread *CT;

static Ttbl	tt;

static Thread	*threadalloc(void);
static void	threadfree(Thread *t);
static DWORD WINAPI tramp(LPVOID p);
static void	threadsleep(void);
static void	threadwakeup(Thread *t);

void
threadinit(void)
{
	Thread *t;

	t = threadalloc();
	assert(t != 0);

	CT = t;
	t->tid = GetCurrentThreadId();
	t->sema = CreateSemaphore(0, 0, 1000, 0);
	if(t->sema == 0) {
		oserror();
		fatal("could not create semaphore: %r");
	}
}


int
thread(char *name, void (*f)(void *), void *a)
{
	Thread *t;
	int tid;

	if((t = threadalloc()) == 0)
		return -1;

	t->f = f;
	t->a = a;

	if(CreateThread(0, 0, tramp, t, 0, &tid) == 0) {
		oserror();
		fatal("createthread %s: %r", name);
	}

	return 0;	/* for the compiler */
}

void
threadexit(void)
{
	if(tt.nthread == 1)
		exits("normal");

	threadfree(CT);
	
	ExitThread(0);
}

static DWORD WINAPI
tramp(LPVOID p)
{
	CT = (Thread*)p;

	CT->sema = CreateSemaphore(0, 0, 1000, 0);
	if(CT->sema == 0) {
		oserror();
		fatal("could not create semaphore: %r");
	}

 	(*CT->f)(CT->a);
	ExitThread(0);
	return 0;
}


int
gettid(void)
{
	return CT->tid;
}

int
gettindex(void)
{
	return CT->tindex;
}

int
errstr(char *s)
{
	char tmp[ERRLEN];

	strncpy(tmp, s, ERRLEN);
	strncpy(s, CT->error, ERRLEN);
	strncpy(CT->error, tmp, ERRLEN);

	return 0;
}

char *
threaderr(void)
{
	return CT->error;
}

static void
threadsleep(void)
{
	WaitForSingleObject(CT->sema, INFINITE);
}

static void
threadwakeup(Thread *t)
{
	ReleaseSemaphore(t->sema, 1, 0);
}

static Thread *
threadalloc(void)
{
	Thread *t;

	lock(&tt.lk);
	tt.nthread++;
	if((t = tt.free) != 0) {
		tt.free = t->qnext;
		unlock(&tt.lk);
		return t;
	}

	if(tt.nalloc == Nthread) {
		unlock(&tt.lk);
		return 0;
	}
	t = tt.t[tt.nalloc] = mallocz(sizeof(Thread));
	t->tindex = tt.nalloc;
	tt.nalloc++;
	unlock(&tt.lk);
	return t;
}

static void
threadfree(Thread *t)
{
	t->tid = -1;

	lock(&tt.lk);
	tt.nthread--;
	t->qnext = tt.free;
	tt.free = t;
	unlock(&tt.lk);
}



void
winfatal(char *s, int err)
{
	int e, r;
	char buf[100], buf2[256];

	e = GetLastError();
	
	if (err) {
		r = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buf, sizeof(buf), 0);

		if(r == 0)
			sprint(buf, "windows error %d", s, e);
		sprint(buf2, "%s: %s", s, buf);
	} else
		strcpy(buf2, s);
	MessageBox(0, buf2, "fatal error", MB_OK);
abort();
	ExitProcess(0);
}

long
p9sleep(long t)
{
	Sleep(t);
	return 0;
}

void
szero(void *p, long n)
{
	memset(p, 0, n);
}

void *
sbrk(long n)
{
	HGLOBAL h;
	void *p;

	h = GlobalAlloc(GMEM_FIXED, n);
	if(h == 0)
		return (void*)-1;

	p = GlobalLock(h);
	memset(p, 0, n);

	return p;
}

void
_exits(char *s)
{
	if(s == 0)
		s = "";

	if(*s != 0)
		MessageBox(0, s, "exits", MB_OK);
	ExitProcess(0);
}

void
nexterror(void)
{
	int n;
	Thread *t;

	t = CT;
	
	n = --t->nerrlbl;
	if(n < 0)
		fatal("error: %r");
	longjmp(t->errlbl[n].buf, 1);
}

void
error(char *fmt, ...)
{
	char buf[ERRLEN];
	va_list va;

	if(fmt) {
		va_start(va, fmt);
		doprint(buf, buf+sizeof(buf), fmt, va);
		va_end(va);
		errstr(buf);
	}
	nexterror();
}

void
poperror(void)
{
	Thread *t;

	t = CT;
	if(t->nerrlbl <= 0)
		fatal("error stack underflow");
	t->nerrlbl--;
}

Jump*
pm_waserror(void)
{
	Thread *t;
	int n;

	t = CT;
	n = t->nerrlbl++;
	if(n >= Nerrlbl)
		fatal("error stack underflow");
	return &t->errlbl[n];
}

void*
curthread(void)
{
	return CT;
}

void
oserror(void)
{
	int e, r, i;
	char buf[200];

	e = GetLastError();
	
	r = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), 0);

	if(r == 0)
		sprint(buf, "windows error %d", e);


	for(i = strlen(buf)-1; i>=0 && buf[i] == '\n' || buf[i] == '\r'; i--)
		buf[i] = 0;

	errstr(buf);
}

static void
queue(Thread **first, Thread **last)
{
	Thread *t;
	Thread *ct;

	ct = CT;

	t = *last;
	if(t == 0)
		*first = ct;
	else
		t->qnext = ct;
	*last = ct;
	ct->qnext = 0;
}

static Thread *
dequeue(Thread **first, Thread **last)
{
	Thread *t;

	t = *first;
	if(t == 0)
		return 0;
	*first = t->qnext;
	if(*first == 0)
		*last = 0;
	return t;
}

void
qlock(Qlock *q)
{
	lock(&q->lk);

	if(q->hold == 0) {
		q->hold = CT;
		unlock(&q->lk);
		return;
	}
	
	/*	
	 * the current implementation of RWLock does not allow the following assert
	assert(q->hold != CT);
	 */

	queue((Thread**)&q->first, (Thread**)&q->last);
	unlock(&q->lk);
	threadsleep();
}

int
canqlock(Qlock *q)
{
	lock(&q->lk);
	if(q->hold == 0) {
		q->hold = CT;
		unlock(&q->lk);
		return 1;
	}
	unlock(&q->lk);
	return 0;
}

void
qunlock(Qlock *q)
{
	Thread *t;

	lock(&q->lk);
	/*
	 * Can't assert this because of RWlock
	assert(q->hold == CT);
	 */
	t = dequeue((Thread**)&q->first, (Thread**)&q->last);
	if(t) {
		q->hold = t;
		unlock(&q->lk);
		threadwakeup(t);
	} else {
		q->hold = 0;
		unlock(&q->lk);
	}
}

int
holdqlock(Qlock *q)
{
	return q->hold == CT;
}

void
rendsleep(Rendez *r, int (*f)(void*), void *arg)
{
	lock(&CT->rlock);
	CT->r = r;
	unlock(&CT->rlock);

	lock(&r->l);

	/*
	 * if condition happened, never mind
	 */
	if(CT->intr || f(arg)){
		unlock(&r->l);
		goto Done;
	}

	/*
	 * now we are committed to
	 * change state and call scheduler
	 */
	if(r->t)
		fatal("double sleep");
	r->t = CT;
	unlock(&r->l);

	threadsleep();

Done:
	lock(&CT->rlock);
	CT->r = 0;
	if(CT->intr) {
		CT->intr = 0;
		unlock(&CT->rlock);
		error(Eintr);
	}
	unlock(&CT->rlock);
}

void
rendwakeup(Rendez *r)
{
	Thread *t;

	lock(&r->l);
	t = r->t;
	if(t) {
		r->t = 0;
		threadwakeup(t);
	}
	unlock(&r->l);
}

void
intr(Thread *t)
{
	lock(&t->rlock);
	t->intr = 1;
	if(t->r)
		rendwakeup(t->r);
	unlock(&t->rlock);
}

void
clearintr(void)
{
	lock(&CT->rlock);
	CT->intr = 0;
	unlock(&CT->rlock);
}

int
ticks(void)
{
	return GetTickCount();
}

void
osfillproc(Proc *p)
{
	p->uid = p->gid = -1;
}

int
canlock(int *p)
{	
	int v;
	
	_asm {
		mov	eax, p
		mov	ebx, 1
		xchg	ebx, [eax]
		mov	v, ebx
	}

	return !v;
}

extern int	main(int, char*[]);
static int	args(char *argv[], int n, char *p);

int PASCAL
WinMain(HANDLE hInst, HANDLE hPrev, LPSTR arg, int nshow)
{
	int argc, n;
	char *p, **argv;

	/* conservative guess at the number of args */
	for(argc=5,p=arg; *p; p++)
		if(*p == ' ' || *p == '\t')
			argc++;

	argv = malloc(argc*sizeof(char*));
	argc = args(argv+1, argc, arg);
	argc++;
	argv[0] = "drawterm";
	main(argc, argv);
	threadexit();
	return 0;
}

/*
 * Break the command line into arguments
 * The rules for this are not documented but appear to be the following
 * according to the source for the microsoft C library.
 * Words are seperated by space or tab
 * Words containing a space or tab can be quoted using "
 * 2N backslashes + " ==> N backslashes and end quote
 * 2N+1 backslashes + " ==> N backslashes + literal "
 * N backslashes not followed by " ==> N backslashes
 */
static int
args(char *argv[], int n, char *p)
{
	char *p2;
	int i, j, quote, nbs;

	for(i=0; *p && i<n-1; i++) {
		while(*p == ' ' || *p == '\t')
			p++;
		quote = 0;
		argv[i] = p2 = p;
		for(;*p; p++) {
			if(!quote && (*p == ' ' || *p == '\t'))
				break;
			for(nbs=0; *p == '\\'; p++,nbs++)
				;
			if(*p == '"') {
				for(j=0; j<(nbs>>1); j++)
					*p2++ = '\\';
				if(nbs&1)
					*p2++ = *p;
				else
					quote = !quote;
			} else {
				for(j=0; j<nbs; j++)
					*p2++ = '\\';
				*p2++ = *p;
			}
		}
		/* move p up one to avoid pointing to null at end of p2 */
		if(*p)
			p++;
		*p2 = 0;	
	}
	argv[i] = 0;

	return i;
}

/*
 * Override iprint
 */
int drawdebug;
int
iprint(char *fmt,...)
{
	char buf[128];
	int n;
	va_list va;

	va_start(va, fmt);
	n = doprint(buf, buf+sizeof buf, fmt, va) - buf;
	va_end(va);
	if(n > 0 && buf[n-1] == '\n')
		buf[n-1] = 0;
	MessageBox(0, buf, "iprint", MB_OK);
	return n;
}
