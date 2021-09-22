/*
 * select.c
 * Select Loop
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

/* #define DEBUG_CALLS /* */

/*
 * more select machinery
 */

#ifndef FD_SETSIZE
#define FD_SETSIZE (sizeof(fd_set) * 8)		/* fds (bits) per fd_set */
#endif

enum {
	Debugsel = 0,		/* flag: debug select usage */
};

typedef struct thread Selthread;
struct thread {
	void	(*read_func)(void *);
	void	(*write_func)(void *);
	void	(*error_func)(void *);
	void	*data;
};

Selthread threads[FD_SETSIZE];
fd_set w_read, w_error;
fd_set x_read, x_error;
static int w_max;

/*
 * more timer machinery
 */

int timer_id = 0;

struct timer {
	struct	timer *next;
	struct	timer *prev;
	ttime	interval;
	void	(*func)(void *);
	void	*data;
	int	id;
};

struct list_head timers = {&timers, &timers};

ttime
get_time(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

long
select_info(int type)
{
	int i = 0, j;
	struct cache_entry *ce;

	switch (type) {
	case CI_FILES:
		for (j = 0; j < FD_SETSIZE; j++)
			if (threads[j].read_func || threads[j].write_func ||
			    threads[j].error_func)
				i++;
		return i;
	case CI_TIMERS:
		foreach(ce, timers)
			i++;
		return i;
	default:
		internal((unsigned char *)"cache_info: bad request");
	}
	return 0;
}

/*
 * `bottom-half' machinery.
 * it's not obvious what this stuff is;
 * it looks like a mechanism for deferred
 * (but not timed nor scheduled) execution
 * of a given function with a given argument.
 */

struct bottom_half {
	struct	bottom_half *next;
	struct	bottom_half *prev;
	void	(*fn)(void *);
	void	*data;
};

struct list_head bottom_halves = { &bottom_halves, &bottom_halves };

int
register_bottom_half(void (*fn)(void *), void *data)
{
	struct bottom_half *bh;

	foreach(bh, bottom_halves)
		if (bh->fn == fn && bh->data == data)
			return 0;
	if (!(bh = mem_alloc(sizeof(struct bottom_half))))
		return -1;
	bh->fn = fn;
	bh->data = data;
	add_to_list(bottom_halves, bh);
	return 0;
}

void
unregister_bottom_half(void (*fn)(void *), void *data)
{
	struct bottom_half *bh;

retry:
	foreach(bh, bottom_halves)
		if (bh->fn == fn && bh->data == data) {
			del_from_list(bh);
			mem_free(bh);
			goto retry;
		}
}

void
check_bottom_halves(void)
{
	struct bottom_half *bh;
	void (*fn)(void *);
	void *data;

	while (!list_empty(bottom_halves)) {
		bh = bottom_halves.prev;
		fn = bh->fn;
		data = bh->data;
		del_from_list(bh);
		mem_free(bh);
#ifdef DEBUG_CALLS
		fprintf(stderr, "call: bh %p\n", fn);
#endif
		pr((*fn)(data)) {
			free_list(bottom_halves);
			break;
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "bh done\n");
#endif
	}
}

#define CHK_BH() if (!list_empty(bottom_halves)) check_bottom_halves()

/*
 * timer machinery
 */

ttime last_time;

void
check_timers(void)
{
	ttime interval = get_time() - last_time;
	struct timer *t, *tt;

	foreach(t, timers)
		t->interval -= interval;

	foreach(t, timers) {
		if (t->interval > 0)
			break;
#ifdef DEBUG_CALLS
		fprintf(stderr, "call: timer %p\n", t->func);
#endif
		pr((*t->func)(t->data)) {
			del_from_list((struct timer *)timers.next);
			break;
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "timer done\n");
#endif
		CHK_BH();
		tt = t->prev;
		del_from_list(t);
		mem_free(t);
		t = tt;
	}
	last_time += interval;
}

int
install_timer(ttime t, void (*func)(void *), void *data)
{
	struct timer *tm, *tt;

	if (!(tm = mem_alloc(sizeof(struct timer))))
		return -1;
	tm->interval = t;
	tm->func = func;
	tm->data = data;
	tm->id = timer_id++;
	foreach(tt, timers)
		if (tt->interval >= t)
			break;
	add_at_pos(tt->prev, tm);
	return tm->id;
}

void
kill_timer(int id)
{
	struct timer *tm, *tt;
	int k = 0;

	foreach(tm, timers)
		if (tm->id == id) {
			tt = tm;
			del_from_list(tm);
			tm = tm->prev;
			mem_free(tt);
			k++;
		}
	if (!k)
		internal((unsigned char *)"trying to kill nonexisting timer");
	if (k >= 2)
		internal((unsigned char *)"more timers with same id");
}

/*
 * a little select machinery (see below for more)
 */

void *
get_handler(int fd, int tp)
{
	if (fd < 0 || fd >= FD_SETSIZE) {
		internal((unsigned char *)"get_handler: handle %d >= FD_SETSIZE %d", fd, FD_SETSIZE);
		return NULL;
	}
	switch (tp) {
	case H_READ:
		return threads[fd].read_func;
	case H_WRITE:
		return threads[fd].write_func;
	case H_ERROR:
		return threads[fd].error_func;
	case H_DATA:
		return threads[fd].data;
	}
	internal((unsigned char *)"get_handler: bad type %d", tp);
	return NULL;
}

void
set_handlers(int fd, void (*read_func)(void *), void (*write_func)(void *),
	void (*error_func)(void *), void *data)
{
	if (fd < 0)
		return;
	if (fd >= FD_SETSIZE) {
		internal((uchar *)"set_handlers: fd %d >= FD_SETSIZE %d",
			fd, FD_SETSIZE);
		return;
	}
	write_func = NULL;		/* no select for write - geoff */
	threads[fd].read_func = read_func;
	threads[fd].write_func = write_func;
	threads[fd].error_func = error_func;
	threads[fd].data = data;
	if (read_func)
		FD_SET(fd, &w_read);
	else {
		FD_CLR(fd, &w_read);
		FD_CLR(fd, &x_read);
	}
/*
	if (write_func)
		FD_SET(fd, &w_write);
	else {
		FD_CLR(fd, &w_write);
		FD_CLR(fd, &x_write);
	}
 */
	if (error_func)
		FD_SET(fd, &w_error);
	else {
		FD_CLR(fd, &w_error);
		FD_CLR(fd, &x_error);
	}
	if (read_func || write_func || error_func) {
		if (fd >= w_max)
			w_max = fd + 1;
	} else if (fd == w_max - 1) {
		int i;

		for (i = fd - 1; i >= 0; i--)
			if (FD_ISSET(i, &w_read) ||
/*
			    FD_ISSET(i, &w_write) ||
 */
			    FD_ISSET(i, &w_error))
				break;
		w_max = i + 1;
	}
}

/*
 * god-awful-signal machinery
 * BSD signals are just a horrible idea
 */

#define NUM_SIGNALS	32

struct signal_handler {
	void	(*fn)(void *);
	void	*data;
	int	critical;
};

int signal_mask[NUM_SIGNALS];
volatile struct signal_handler signal_handlers[NUM_SIGNALS];

volatile int critical_section = 0;

void check_for_select_race();

void got_signal(int sig)
{
	if (sig >= NUM_SIGNALS || sig < 0) {
		error((unsigned char *)"ERROR: bad signal number: %d", sig);
		return;
	}
	if (!signal_handlers[sig].fn) return;
	if (signal_handlers[sig].critical) {
		signal_handlers[sig].fn(signal_handlers[sig].data);
		return;
	}
	signal_mask[sig] = 1;
	check_for_select_race();
}

void install_signal_handler(int sig, void (*fn)(void *), void *data, int critical)
{
	struct sigaction sa;
	if (sig >= NUM_SIGNALS || sig < 0) {
		internal((unsigned char *)"bad signal number: %d", sig);
		return;
	}
	memset(&sa, 0, sizeof sa);
	if (!fn) sa.sa_handler = SIG_IGN;
	else sa.sa_handler = got_signal;
	sigfillset(&sa.sa_mask);
	/*sa.sa_flags = SA_RESTART;*/
	if (!fn) sigaction(sig, &sa, NULL);
	signal_handlers[sig].fn = fn;
	signal_handlers[sig].data = data;
	signal_handlers[sig].critical = critical;
	if (fn) sigaction(sig, &sa, NULL);
}

int pending_alarm = 0;

void alarm_handler(void *x)
{
	pending_alarm = 0;
	check_for_select_race();
}

void check_for_select_race()
{
	if (critical_section) {
#ifdef SIGALRM
		install_signal_handler(SIGALRM, alarm_handler, NULL, 1);
#endif
		pending_alarm = 1;
#ifdef HAVE_ALARM
		/*alarm(1);*/
#endif
	}
}

void uninstall_alarm()
{
	pending_alarm = 0;
#ifdef HAVE_ALARM
	alarm(0);
#endif
}

int
check_signals(void)
{
	int i, r = 0;

	for (i = 0; i < NUM_SIGNALS; i++)
		if (signal_mask[i]) {
			struct signal_handler *sp = &signal_handlers[i];

			signal_mask[i] = 0;
			if (sp->fn) {
#ifdef DEBUG_CALLS
				fprintf(stderr, "call: signal %d -> %p\n", i, sp->fn);
#endif
				pr(sp->fn(sp->data)) return 1;
#ifdef DEBUG_CALLS
				fprintf(stderr, "signal done\n");
#endif
			}
			CHK_BH();
			r = 1;
		}
	return r;
}

void sigchld(void *p)
{
#ifndef WNOHANG
	wait(NULL);
#else
	while ((int)waitpid(-1, NULL, WNOHANG) > 0)
		continue;
#endif
}

void set_sigcld(void)
{
	install_signal_handler(SIGCHLD, sigchld, NULL, 1);
}

/*
 * select machinery
 */

volatile int terminate_loop = 0;

enum {
	Braindamage = 0,
};

typedef void (Vfvp)(void *);	/* void func. taking void pointer */
typedef void (*Pvfvp)(void *); /* pointer to void func. taking void pointer */

static int
findwork(fd_set *fds, int memboff, int maxfd1, int seln)
{
	int n = seln, i, waswork = 0;
	Selthread *thp;
	Pvfvp *funcp;

	for (i = 0; n > 0 && i++ < maxfd1; )
		if (FD_ISSET(i, fds)) {
			waswork++;
			thp = &threads[i];
			funcp = (Pvfvp *)((char *)thp + memboff);
			if (*funcp) {
				pr((**funcp)(thp->data)) continue;
			}
			n--;
			CHK_BH();
		}
	return waswork;
}

static void
dumpfds(char *msg, fd_set *fds)
{
	int i;

	fprintf(stderr, "fds %s:", msg);
	for (i = 0; i < FD_SETSIZE; i++)
		if (FD_ISSET(i, fds))
			fprintf(stderr, " %d", i);
	fprintf(stderr, "\n");
}

extern int evpipe[2];
extern int shootme;

enum {
	Minsleepus = 2000,	/* sleep at least this long; don't burn CPU */
	Maxsleepus = 1000*1000,	/* but no longer than this; update screen */
};

void
select_loop(void (*init)(void))
{
	memset(signal_mask, 0, sizeof signal_mask);
	memset(signal_handlers, 0, sizeof signal_handlers);
	FD_ZERO(&w_read);
	/* FD_ZERO(&w_write); */
	FD_ZERO(&w_error);
	w_max = 0;
	last_time = get_time();
	signal(SIGPIPE, SIG_IGN);

	if (pipe(evpipe) < 0) {
		perror("pipe");
		exit(1);
	}

	/* initialises graphics driver, among other things */
	(*init)();

	CHK_BH();
	while (!terminate_loop) {
		int n, i;
		ttime tt = 0;
		struct timeval tv;
		struct timeval *tm = NULL;

		/*
		 * way too much set up for select
		 */

		/* WTF?!  This is crap; just call it once. */
		check_timers();
		if (Braindamage) {
			check_signals();
			check_timers();
			check_timers();
			check_timers();
		}

		if (!F)
			redraw_all_terminals();
		if (!list_empty(timers)) {
			tt = ((struct timer *)timers.next)->interval + 1;
			if (tt < 0)
				tt = 0;
			if (tt > 0) {
				if (tt < Minsleepus)
					tt = Minsleepus;
				else if (tt > Maxsleepus)
					tt = Maxsleepus;
				tv.tv_sec = tt / 1000;
				tv.tv_usec = (tt % 1000) * 1000;
				tm = &tv;
			}
		}
		memcpy(&x_read,  &w_read,  sizeof(fd_set));
		memcpy(&x_error, &w_error, sizeof(fd_set));
		/* memcpy(&x_write, &w_write, sizeof(fd_set)); */
		if (terminate_loop)
			break;
		if (w_max == 0 && list_empty(timers)) {
			if (1)
				internal((uchar*)"select_loop: no more events to wait for");
			break;
		}

		critical_section = 1;
		if (check_signals()) {
			critical_section = 0;
			continue;
		}

		/*
		 * do the damn select: any work to do? maybe sleep for a bit.
		 * we no longer use select for output streams, where it
		 * isn't very useful anyway.  `errors' are actually EOF.
		 */

#ifdef DEBUG_CALLS
		fprintf(stderr, "select\n");
#endif
		if (Debugsel) {
			dumpfds("before r", &x_read);
			dumpfds("before e", &x_error);
		}
		n = loop_select(w_max, &x_read, NULL, &x_error, tm);

		if (Debugsel) {
			if (n < 0)
				perror("select");
			fprintf(stderr, "%d = select(%d,...,%ld us)\n",
				n, w_max, tt);
			if (n > 0) {
				dumpfds("after r", &x_read);
				dumpfds("after e", &x_error);
			}
		}
		/*printf("sel: %d\n", n);*/

		if (n < 0) {
#ifdef DEBUG_CALLS
			fprintf(stderr, "select err\n");
#endif
			critical_section = 0;
			uninstall_alarm();
			if (errno == EBADF) {
				/* probably just EOF, but clear the bit(s) */
				for (i = 0; i < w_max; i++) {
					struct stat stbuf;

					if (FD_ISSET(i, &w_read) &&
					    fstat(i, &stbuf) < 0 &&
					    errno == EBADF) {
						fprintf(stderr,
						    "closing %d due to EBADF\n",
							i);
						FD_CLR(i, &w_read);
						FD_CLR(i, &w_error);
					}
				}
				continue;
			}
			if (errno != EINTR) {
				perror("select");
				error((unsigned char *)"ERROR: select failed");
				sleep(1);	/* don't just burn CPU */
			}
			continue;
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "select done\n");
#endif

		/*
		 * more busy work, then figure out what work there is
		 * to do, if any, and do it.
		 */

		critical_section = 0;
		uninstall_alarm();

		if (n > 0) {
			/* these seem redundant, but i could be wrong */
			check_signals();
			check_timers();

			findwork(&x_error, offsetof(Selthread, error_func),
				w_max, n);
			findwork(&x_read,  offsetof(Selthread, read_func),
				w_max, n);
		}
		nopr();
	}
	if (shootme) {
		kill(shootme, SIGKILL);
		shootme = 0;
	}
#ifdef DEBUG_CALLS
	fprintf(stderr, "exit loop\n");
#endif
	nopr();
}

/*
 * interfacing with non-APE plan9.c
 */

void
seteventbit(int evfd)
{
	FD_SET(evfd, &w_read);
	FD_SET(evfd, &w_error);
	if (evfd >= w_max)
		w_max = evfd + 1;
}

void
clreventbit(int evfd)
{
	FD_CLR(evfd, &w_read);
	FD_CLR(evfd, &w_error);
}

int
isseteventbit(int evfd)
{
	return FD_ISSET(evfd, &x_read) || FD_ISSET(evfd, &x_error)? 1: 0;
}

/* must use aperead and apewrite in plan9.c on fds used by select */
int
aperead(int fd, void *addr, ulong size)
{
	return read(fd, addr, size);
}

int
apewrite(int fd, void *addr, ulong size)
{
	return write(fd, addr, size);
}
