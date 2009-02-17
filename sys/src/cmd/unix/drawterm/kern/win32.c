#include <windows.h>
#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include <libsec.h>

typedef struct Oproc Oproc;
struct Oproc {
	int tid;
	HANDLE	*sema;
};

static int tlsx = TLS_OUT_OF_INDEXES;

char	*argv0;

Proc*
_getproc(void)
{
	if(tlsx == TLS_OUT_OF_INDEXES)
		return nil;
	return TlsGetValue(tlsx);
}

void
_setproc(Proc *p)
{
	if(tlsx == TLS_OUT_OF_INDEXES){
		tlsx = TlsAlloc();
		if(tlsx == TLS_OUT_OF_INDEXES)
			panic("out of indexes");
	}
	TlsSetValue(tlsx, p);
}

void
oserror(void)
{
	oserrstr();
	nexterror();
}

void
osinit(void)
{
	Oproc *t;
	static Proc firstprocCTstore;

	_setproc(&firstprocCTstore);
	t = (Oproc*)firstprocCTstore.oproc;
	assert(t != 0);

	t->tid = GetCurrentThreadId();
	t->sema = CreateSemaphore(0, 0, 1000, 0);
	if(t->sema == 0) {
		oserror();
		panic("could not create semaphore: %r");
	}
}

void
osnewproc(Proc *p)
{
	Oproc *op;

	op = (Oproc*)p->oproc;
	op->sema = CreateSemaphore(0, 0, 1000, 0);
	if (op->sema == 0) {
		oserror();
		panic("could not create semaphore: %r");
	}
}

void
osmsleep(int ms)
{
	Sleep((DWORD) ms);
}

void
osyield(void)
{
	Sleep(0);
}

static DWORD WINAPI tramp(LPVOID vp);

void
osproc(Proc *p)
{
	DWORD tid;

	if(CreateThread(0, 0, tramp, p, 0, &tid) == 0) {
		oserror();
		panic("osproc: %r");
	}

	Sleep(0);
}

static DWORD WINAPI
tramp(LPVOID vp)
{
	Proc *p = (Proc *) vp;
	Oproc *op = (Oproc*) p->oproc;

	_setproc(p);
	op->tid = GetCurrentThreadId();
	op->sema = CreateSemaphore(0, 0, 1000, 0);
	if(op->sema == 0) {
		oserror();
		panic("could not create semaphore: %r");
	}

 	(*p->fn)(p->arg);
	ExitThread(0);
	return 0;
}

void
procsleep(void)
{
	Proc *p;
	Oproc *op;

	p = up;
	op = (Oproc*)p->oproc;
	WaitForSingleObject(op->sema, INFINITE);}

void
procwakeup(Proc *p)
{
	Oproc *op;

	op = (Oproc*)p->oproc;
	ReleaseSemaphore(op->sema, 1, 0);
}

void
random20(uchar *p)
{
	LARGE_INTEGER ti;
	int i, j;
	FILETIME ft;
	DigestState ds;
	vlong tsc;
	
	GetSystemTimeAsFileTime(&ft);
	memset(&ds, 0, sizeof ds);
	sha1((uchar*)&ft, sizeof(ft), 0, &ds);
	for(i=0; i<50; i++) {
		for(j=0; j<10; j++) {
			QueryPerformanceCounter(&ti);
			sha1((uchar*)&ti, sizeof(ti), 0, &ds);
			tsc = GetTickCount();
			sha1((uchar*)&tsc, sizeof(tsc), 0, &ds);
		}
		Sleep(10);
	}
	sha1(0, 0, p, &ds);
}

void
randominit(void)
{
}

ulong
randomread(void *v, ulong n)
{
	int i;
	uchar p[20];
	
	for(i=0; i<n; i+=20){
		random20(p);
		if(i+20 <= n)
			memmove((char*)v+i, p, 20);
		else
			memmove((char*)v+i, p, n-i);
	}
	return n;
}

long
seconds(void)
{
	return time(0);
}

ulong
ticks(void)
{
	return GetTickCount();
}

#if 0
uvlong
fastticks(uvlong *v)
{
	uvlong n;

	n = GetTickCount() * 1000 * 1000;
	if(v)
		*v = n;
	return n;
}
#endif

extern int	main(int, char*[]);


int
wstrutflen(Rune *s)
{
	int n;
	
	for(n=0; *s; n+=runelen(*s),s++)
		;
	return n;
}

int
wstrtoutf(char *s, Rune *t, int n)
{
	int i;
	char *s0;

	s0 = s;
	if(n <= 0)
		return wstrutflen(t)+1;
	while(*t) {
		if(n < UTFmax+1 && n < runelen(*t)+1) {
			*s = 0;
			return s-s0+wstrutflen(t)+1;
		}
		i = runetochar(s, t);
		s += i;
		n -= i;
		t++;
	}
	*s = 0;
	return s-s0;
}

int
win_hasunicode(void)
{
	OSVERSIONINFOA osinfo;
	int r;

	osinfo.dwOSVersionInfoSize = sizeof(osinfo);
	if(!GetVersionExA(&osinfo))
		panic("GetVersionEx failed");
	switch(osinfo.dwPlatformId) {
	default:
		panic("unknown PlatformId");
	case VER_PLATFORM_WIN32s:
		panic("Win32s not supported");
	case VER_PLATFORM_WIN32_WINDOWS:
		r = 0;
		break;
	case VER_PLATFORM_WIN32_NT:
		r = 1;
		break;
	}

	return r;
}

int
wstrlen(Rune *s)
{
	int n;

	for(n=0; *s; s++,n++)
		;
	return n;
}
static int	args(char *argv[], int n, char *p);

int APIENTRY
WinMain(HINSTANCE x, HINSTANCE y, LPSTR z, int w)
{
	int argc, n;
	char *arg, *p, **argv;
	Rune *warg;

	if(0 && win_hasunicode()){
		warg = GetCommandLineW();
		n = (wstrlen(warg)+1)*UTFmax;
		arg = malloc(n);
		wstrtoutf(arg, warg, n);
	}else
		arg = GetCommandLineA();

	/* conservative guess at the number of args */
	for(argc=4,p=arg; *p; p++)
		if(*p == ' ' || *p == '\t')
			argc++;
	argv = malloc(argc*sizeof(char*));
	argc = args(argv, argc, arg);

	mymain(argc, argv);
	ExitThread(0);
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
 * Windows socket error messages
 * There must be a way to get these strings out of the library.
 * This table is derived from the MSDN online help.
 */
static struct {
	int e;
	char *s;
} tab[] = {
	{ 10004, "interrupted function call" },
	{ 10013, "permission denied" },
	{ 10014, "bad address" },
	{ 10022, "invalid argument" },
	{ 10024, "too many open files" },
	{ 10035, "resource temporarily unavailable" },
	{ 10036, "operation now in progress" },
	{ 10037, "operation already in progress" },
	{ 10038, "socket operation on nonsocket" },
	{ 10039, "destination address required" },
	{ 10040, "message too long" },
	{ 10041, "protocol wrong type for socket" },
	{ 10042, "bad protocol option" },
	{ 10043, "protocol not supported" },
	{ 10044, "socket type not supported" },
	{ 10045, "operation not supported" },
	{ 10046, "protocol family not supported" },
	{ 10047, "address family not supported by protocol family" },
	{ 10048, "address already in use" },
	{ 10049, "cannot assign requested address" },
	{ 10050, "network is down" },
	{ 10051, "network is unreachable" },
	{ 10052, "network dropped connection on reset" },
	{ 10053, "software caused connection abort" },
	{ 10054, "connection reset by peer" },
	{ 10055, "no buffer space available" },
	{ 10056, "socket is already connected" },
	{ 10057, "socket is not connected" },
	{ 10058, "cannot send after socket shutdown" },
	{ 10060, "connection timed out" },
	{ 10061, "connection refused" },
	{ 10064, "host is down" },
	{ 10065, "no route to host" },
	{ 10067, "too many processes" },
	{ 10091, "network subsystem is unavailable" },
	{ 10092, "winsock.dll version out of range" },
	{ 10093, "wsastartup not called" },
	{ 10101, "graceful shutdown in progress" },
	{ 10109, "class type not found" },
	{ 11001, "host name not found" },
	{ 11002, "host not found (non-authoritative)" },
	{ 11003, "nonrecoverable error" },
	{ 11004, "valid name, but no data record of requested type" },
};

void
osrerrstr(char *buf, uint nbuf)
{
	char *p, *q;
	int e, i, r;

	e = GetLastError();
	r = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, nbuf, 0);
	if(r == 0){
		for(i=0; i<nelem(tab); i++)
			if(tab[i].e == e){
				strecpy(buf, buf+nbuf, tab[i].s);
				break;
			}
		if(i==nelem(tab))
			snprint(buf, nbuf, "windows error %d", e);
	}

	for(p=q=buf; *p; p++) {
		if(*p == '\r')
			continue;
		if(*p == '\n')
			*q++ = ' ';
		else
			*q++ = *p;
	}
	*q = '\0';
}

void
oserrstr(void)
{
	osrerrstr(up->errstr, ERRMAX);
}

long
showfilewrite(char *a, int n)
{
	Rune *action, *arg, *cmd, *p;
	static Rune Lopen[] = { 'o', 'p', 'e', 'n', 0 };

	cmd = runesmprint("%.*s", n, a);
	if(cmd == nil)
		error("out of memory");
	if(cmd[runestrlen(cmd)-1] == '\n')
		cmd[runestrlen(cmd)] = 0;
	p = runestrchr(cmd, ' ');
	if(p){
		action = cmd;
		*p++ = 0;
		arg = p;
	}else{
		action = Lopen;
		arg = cmd;
	}
	ShellExecute(0, 0, action, arg, 0, SW_SHOWNORMAL);
	return n;
}
