#include <u.h>
#include <libc.h>
#include <tos.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

static void
die(void)
{
	exits(nil);
}

static char**
readenv(void)
{
	char **env;
	int fd, n, i, c;
	Dir *d;

	if((fd = open("/env", OREAD)) < 0)
		return nil;
	n = dirreadall(fd, &d);
	close(fd);
	env = kmalloc(sizeof(env[0]) * (n + 1));
	c = 0;
	for(i=0; i<n; i++){
		char *v;
		char *k;

		k = d[i].name;

		// filter out some stuff...
		if(strncmp(k, "fn#", 3) == 0)
			continue;
		if(strcmp(k, "timezone") == 0)
			continue;
		if(strcmp(k, "0")==0)
			continue;

		if((v = getenv(d[i].name)) == nil)
			continue;
		if((env[c] = ksmprint("%s=%s", k, v)) == nil)
			continue;
		free(v);

		c++;
	}
	env[c] = 0;

	free(d);

	return env;
}

struct onstackargs
{
	long		*stk;
	void		*arg;
	int		(*func)(void *);
	int		ret;
	jmp_buf	jmp;
};

int
onstack(long *stk, int (*func)(void *), void *arg)
{
	struct onstackargs a, *args;
	jmp_buf jmp;
	long *sp;

	sp = (long*)&a;
	if((long*)sp >= stk && (long*)sp < stk+(KSTACK / sizeof(long)))
		return func(arg);

	if(args = (struct onstackargs*)setjmp(jmp)){
		args->ret = onstack(args->stk, args->func, args->arg);
		longjmp(args->jmp, 1);
	}

	sp = &stk[(KSTACK / sizeof(long))-16];
	jmp[JMPBUFSP] = (long)sp;

	memset(stk, 0, KSTACK);

	args = &a;
	args->stk = stk;
	args->func = func;
	args->arg = arg;

	if(!setjmp(args->jmp))
		longjmp(jmp, (int)args);

	return args->ret;
}

#pragma profile off

static void
proff(void (*fn)(void*), void *arg)
{
	if(_tos->prof.what == 0){
		fn(arg);
	}else{
		prof(fn, arg, 2000, _tos->prof.what);
	}
}

static void
profexitjmpfn(void *arg)
{
	/*
	 * we are now called by the profiling function on the profstack.
	 * save the current continuation so we can return here on exit.
	 */
	if(!setjmp(exitjmp))
		longjmp((long*)arg, 1); /* return from profme() */
}

static int
profmeprofstack(void *arg)
{
	proff(profexitjmpfn, arg);
	for(;;) die();
}

#pragma profile on

static long *profstack;

void
profme(void)
{
	jmp_buf j;

	if(!setjmp(j))
		onstack(profstack, profmeprofstack, j);
}


static void
vpanic(char *msg, va_list arg)
{
	char buf[32];
	int fd;

	fprint(2, "PANIC: ");
	vfprint(2, msg, arg);
	fprint(2, "\n");

	if(debug)
		abort();

	snprint(buf, sizeof(buf), "/proc/%d/notepg", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, "kill", 4);
		close(fd);
	}
	exits("panic");
}

void
panic(char *msg, ...)
{
	va_list arg;

	va_start(arg, msg);
	vpanic(msg, arg);
	va_end(arg);
}

void usage(void)
{
	fprint(2, "usage: linuxemu [-d] [-u uid] [-g gid] cmd [args]\n");
	exits("usage");
}

struct mainstack
{
	long		profstack[KSTACK / sizeof(long)];
	long		kstack[KSTACK / sizeof(long)];
	Uproc	*proc;
	jmp_buf	exitjmp;
};

void main(int argc, char *argv[])
{
	struct mainstack ms;
	int err;
	int uid, gid;
	int fd;

	fmtinstall('E', Efmt);
	fmtinstall('S', Sfmt);

	uid = 0;
	gid = 0;
	debug = 0;

	ARGBEGIN {
	case 'd':
		debug++;
		break;
	case 'u':
		uid = atoi(EARGF(usage()));
		break;
	case 'g':
		gid = atoi(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND

	if(argc < 1)
		usage();

	rootdevinit();
	procdevinit();
	ptydevinit();
	consdevinit();
	dspdevinit();
	miscdevinit();
	sockdevinit();
	pipedevinit();

	kstack = ms.kstack;
	profstack = ms.profstack;
	exitjmp = ms.exitjmp;
	pcurrent = &ms.proc;
	current = nil;

	if(setjmp(exitjmp))
		die();

	initproc();
	current->uid = uid;
	current->gid = gid;

	/* emulated console */
	sys_close(0);
	if((fd = sys_open("/dev/cons", O_RDWR, 0)) != 0)
		fprint(2, "cant open console for stdin\n");
	sys_close(1);
	if(sys_dup(fd) != 1)
		fprint(2, "cant dup stdout\n");
	sys_close(2);
	if(sys_dup(fd) != 2)
		fprint(2, "cant dup stderr\n");

	sys_fcntl(0, F_SETFD, 0);
	sys_fcntl(1, F_SETFD, 0);
	sys_fcntl(2, F_SETFD, 0);

	err = sys_execve(*argv, argv, readenv());

	fprint(2, "%s: %E\n", *argv, err);
	longjmp(exitjmp, 1);
}
