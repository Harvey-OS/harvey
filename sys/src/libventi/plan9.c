#include <u.h>
#include <libc.h>
#include <venti.h>

enum
{
	QueuingW,	/* queuing for write lock */
	QueuingR,	/* queuing for read lock */
};


typedef struct Thread Thread;

struct Thread {
	int pid;
	int ref;
	char *error;
	int state;
	Thread *next;
};

struct VtLock {
	Lock lk;
	Thread *writer;		/* thread writering write lock */
	int readers;		/* number writering read lock */
	Thread *qfirst;
	Thread *qlast;
};

struct VtRendez {
	VtLock *lk;
	Thread *wfirst;
	Thread *wlast;
};

enum {
	ERROR = 0,
};

enum {
	IdealAlignment = 32,
	ChunkSize 	= 128*1024,
};

static Thread **vtRock;

static void	vtThreadInit(void);
static void	threadSleep(Thread*);
static void	threadWakeup(Thread*);

int
vtThread(void (*f)(void*), void *rock)
{
	int tid;

	tid = rfork(RFNOWAIT|RFMEM|RFPROC);
	switch(tid){
	case -1:
		vtOSError();
		return -1;
	case 0:
		break;
	default:
		return tid;
	}
	vtAttach();
	(*f)(rock);
	vtDetach();
	_exits(0);
	return 0;
}

static Thread *
threadLookup(void)
{
	return *vtRock;
}

void
vtAttach(void)
{
	int pid;
	Thread *p;
	static int init;
	static Lock lk;

	lock(&lk);
	if(!init) {
		rfork(RFREND);
		vtThreadInit();
		init = 1;
	}
	unlock(&lk);

	pid = getpid();
	p = *vtRock;
	if(p != nil && p->pid == pid) {
		p->ref++;
		return;
	}
	p = vtMemAllocZ(sizeof(Thread));
	p->ref = 1;
	p->pid = pid;
	*vtRock = p;
}

void
vtDetach(void)
{
	Thread *p;

	p = *vtRock;
	assert(p != nil);
	p->ref--;
	if(p->ref == 0) {
		vtMemFree(p->error);
		vtMemFree(p);
		*vtRock = nil;
	}
}

char *
vtGetError(void)
{
	char *s;

	if(ERROR)
		fprint(2, "vtGetError: %s\n", threadLookup()->error);
	s = threadLookup()->error;
	if(s == nil)
		return "unknown error";
	return s;
}

char*
vtSetError(char* fmt, ...)
{
	Thread *p;
	va_list args;

	p = threadLookup();

	va_start(args, fmt);
	vtMemFree(p->error);
	p->error = vsmprint(fmt, args);
	va_end(args);
	if(ERROR)
		fprint(2, "vtSetError: %s\n", p->error);
	werrstr("%s", p->error);
	return p->error;
}

char *
vtOSError(void)
{
	Thread *p = threadLookup();
	char s[100];

	rerrstr(s, sizeof(s));
	vtMemFree(p->error);
	p->error = vtStrDup(s);

	if(ERROR)
		fprint(2, "vtOSError: %s\n", s);
	return p->error;
}

int
vtFdRead(int fd, uchar *buf, int n)
{
	n = read(fd, buf, n);
	if(n < 0) {
		vtOSError();
		return -1;
	}
	if(n == 0) {
		vtSetError("unexpected EOF");
		return 0;
	}
	return n;
}

int
vtFdWrite(int fd, uchar *buf, int n)
{
	int nn;
	
	nn = write(fd, buf, n);
	if(nn < 0) {
		vtOSError();
		return 0;
	}
	if(n != nn) {
		vtSetError("truncated write");
		return 0;
	}
	return 1;
}

void
vtFdClose(int fd)
{
	close(fd);
}

static void
vtThreadInit(void)
{
	static Lock lk;

	lock(&lk);
	if(vtRock != nil) {
		unlock(&lk);
		return;
	}
	vtRock = privalloc();
	if(vtRock == nil)
		vtFatal("can't allocate thread-private storage");
	unlock(&lk);
}

VtLock*
vtLockAlloc(void)
{
	return vtMemAllocZ(sizeof(VtLock));
}

void
vtLockInit(VtLock **p)
{
	static Lock lk;

	lock(&lk);
	if(*p != nil)
		*p = vtLockAlloc();
	unlock(&lk);
}

void
vtLockFree(VtLock *p)
{
	if(p == nil)
		return;
	assert(p->writer == nil);
	assert(p->readers == 0);
	assert(p->qfirst == nil);
	vtMemFree(p);
}

VtRendez*
vtRendezAlloc(VtLock *p)
{
	VtRendez *q;

	q = vtMemAllocZ(sizeof(VtRendez));
	q->lk = p;
	return q;
}

void
vtRendezFree(VtRendez *q)
{
	if(q == nil)
		return;
	assert(q->wfirst == nil);
	vtMemFree(q);
}

int
vtCanLock(VtLock *p)
{
	Thread *t;

	lock(&p->lk);
	t = *vtRock;
	if(p->writer == nil && p->readers == 0) {
		p->writer = t;
		unlock(&p->lk);
		return 1;
	}
	unlock(&p->lk);
	return 0;
}


void
vtLock(VtLock *p)
{
	Thread *t;

	lock(&p->lk);
	t = *vtRock;
	if(p->writer == nil && p->readers == 0) {
		p->writer = t;
		unlock(&p->lk);
		return;
	}

	/*
	 * venti currently contains code that assume locks can be passed between threads :-(
	 * assert(p->writer != t);
	 */

	if(p->qfirst == nil)
		p->qfirst = t;
	else
		p->qlast->next = t;
	p->qlast = t;
	t->next = nil;
	t->state = QueuingW;
	unlock(&p->lk);

	threadSleep(t);
	assert(p->writer == t && p->readers == 0);
}

int
vtCanRLock(VtLock *p)
{
	lock(&p->lk);
	if(p->writer == nil && p->qfirst == nil) {
		p->readers++;
		unlock(&p->lk);
		return 1;
	}
	unlock(&p->lk);
	return 0;
}

void
vtRLock(VtLock *p)
{
	Thread *t;

	lock(&p->lk);
	t = *vtRock;
	if(p->writer == nil && p->qfirst == nil) {
		p->readers++;
		unlock(&p->lk);
		return;
	}

	/*
	 * venti currently contains code that assumes locks can be passed between threads
	 * assert(p->writer != t);
	 */
	if(p->qfirst == nil)
		p->qfirst = t;
	else
		p->qlast->next = t;
	p->qlast = t;
	t->next = nil;
	t->state = QueuingR;
	unlock(&p->lk);

	threadSleep(t);
	assert(p->writer == nil && p->readers > 0);
}

void
vtUnlock(VtLock *p)
{
	Thread *t, *tt;

	lock(&p->lk);
	/*
	 * venti currently has code that assumes lock can be passed between threads :-)
 	 * assert(p->writer == *vtRock);
	 */
 	assert(p->writer != nil);   
	assert(p->readers == 0);
	t = p->qfirst;
	if(t == nil) {
		p->writer = nil;
		unlock(&p->lk);
		return;
	}
	if(t->state == QueuingW) {
		p->qfirst = t->next;
		p->writer = t;
		unlock(&p->lk);
		threadWakeup(t);
		return;
	}

	p->writer = nil;
	while(t != nil && t->state == QueuingR) {
		tt = t;
		t = t->next;
		p->readers++;
		threadWakeup(tt);
	}
	p->qfirst = t;
	unlock(&p->lk);
}

void
vtRUnlock(VtLock *p)
{
	Thread *t;

	lock(&p->lk);
	assert(p->writer == nil && p->readers > 0);
	p->readers--;
	t = p->qfirst;
	if(p->readers > 0 || t == nil) {
		unlock(&p->lk);
		return;
	}
	assert(t->state == QueuingW);
	
	p->qfirst = t->next;
	p->writer = t;
	unlock(&p->lk);

	threadWakeup(t);
}

int
vtSleep(VtRendez *q)
{
	Thread *s, *t, *tt;
	VtLock *p;

	p = q->lk;
	lock(&p->lk);
	s = *vtRock;
	/*
	 * venti currently contains code that assume locks can be passed between threads :-(
	 * assert(p->writer != s);
	 */
	assert(p->writer != nil);
	assert(p->readers == 0);
	t = p->qfirst;
	if(t == nil) {
		p->writer = nil;
	} else if(t->state == QueuingW) {
		p->qfirst = t->next;
		p->writer = t;
		threadWakeup(t);
	} else {
		p->writer = nil;
		while(t != nil && t->state == QueuingR) {
			tt = t;
			t = t->next;
			p->readers++;
			threadWakeup(tt);
		}
	}

	if(q->wfirst == nil)
		q->wfirst = s;
	else
		q->wlast->next = s;
	q->wlast = s;
	s->next = nil;
	unlock(&p->lk);

	threadSleep(s);
	assert(p->writer == s);
	return 1;
}

int
vtWakeup(VtRendez *q)
{
	Thread *t;
	VtLock *p;

	/*
	 * take off wait and put on front of queue
	 * put on front so guys that have been waiting will not get starved
	 */
	p = q->lk;
	lock(&p->lk);	
	/*
	 * venti currently has code that assumes lock can be passed between threads :-)
 	 * assert(p->writer == *vtRock);
	 */
	assert(p->writer != nil);
	t = q->wfirst;
	if(t == nil) {
		unlock(&p->lk);
		return 0;
	}
	q->wfirst = t->next;
	if(p->qfirst == nil)
		p->qlast = t;
	t->next = p->qfirst;
	p->qfirst = t;
	t->state = QueuingW;
	unlock(&p->lk);

	return 1;
}

int
vtWakeupAll(VtRendez *q)
{
	int i;
	
	for(i=0; vtWakeup(q); i++)
		;
	return i;
}

static void
threadSleep(Thread *t)
{
	if(rendezvous((ulong)t, 0x22bbdfd6) != 0x44391f14)
		sysfatal("threadSleep: rendezvous failed: %r");
}

static void
threadWakeup(Thread *t)
{
	if(rendezvous((ulong)t, 0x44391f14) != 0x22bbdfd6)
		sysfatal("threadWakeup: rendezvous failed: %r");
}

void
vtMemFree(void *p)
{
	if(p == 0)
		return;
	free(p);
}


void *
vtMemAlloc(int size)
{
	void *p;

	p = malloc(size);
	if(p == 0)
		vtFatal("vtMemAlloc: out of memory");
	return p;
}

void *
vtMemAllocZ(int size)
{
	void *p = vtMemAlloc(size);
	memset(p, 0, size);
	return p;
}

void *
vtMemRealloc(void *p, int size)
{
	if(p == nil)
		return vtMemAlloc(size);
	p = realloc(p, size);
	if(p == 0)
		vtFatal("vtRealloc: out of memory");
	return p;
}


void *
vtMemBrk(int n)
{
	static Lock lk;
	static uchar *buf;
	static int nbuf;
	static int nchunk;
	int align, pad;
	void *p;

	if(n >= IdealAlignment)
		align = IdealAlignment;
	else if(n > 8)
		align = 8;
	else	
		align = 4;

	lock(&lk);
	pad = (align - (ulong)buf) & (align-1);
	if(n + pad > nbuf) {
		buf = vtMemAllocZ(ChunkSize);
		nbuf = ChunkSize;
		pad = (align - (ulong)buf) & (align-1);
		nchunk++;
	}

	assert(n + pad <= nbuf);	
	
	p = buf + pad;
	buf += pad + n;
	nbuf -= pad + n;
	unlock(&lk);

	return p;
}

void
vtThreadSetName(char *name)
{
	int fd;
	char buf[32];

	sprint(buf, "/proc/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, name, strlen(name));
		close(fd);
	}
}
