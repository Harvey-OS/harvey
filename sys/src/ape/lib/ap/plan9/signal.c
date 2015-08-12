/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include "sys9.h"
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <setjmp.h>

extern sigset_t _psigblocked;

static struct {
	char* msg; /* just check prefix */
	int num;
} sigtab[] = {
    {"hangup", SIGHUP},
    {"interrupt", SIGINT},
    {"quit", SIGQUIT},
    {"alarm", SIGALRM},
    {"sys: trap: illegal instruction", SIGILL},
    {"sys: trap: reserved instruction", SIGILL},
    {"sys: trap: reserved", SIGILL},
    {"sys: trap: arithmetic overflow", SIGFPE},
    {"abort", SIGABRT},
    {"sys: fp:", SIGFPE},
    {"exit", SIGKILL},
    {"die", SIGKILL},
    {"kill", SIGKILL},
    {"sys: trap: bus error", SIGSEGV},
    {"sys: trap: address error", SIGSEGV},
    {"sys: trap: TLB", SIGSEGV},
    {"sys: write on closed pipe", SIGPIPE},
    {"alarm", SIGALRM},
    {"term", SIGTERM},
    {"usr1", SIGUSR1},
    {"usr2", SIGUSR2},
};
#define NSIGTAB ((sizeof sigtab) / (sizeof(sigtab[0])))

void (*_sighdlr[MAXSIG + 1])(int, char*, Ureg*); /* 0 initialized: SIG_DFL */

/* must match signal.h: extern void (*signal(int, void (*)()))(); */
// void (*signal(int sig, void (*func)(int, char*, Ureg*)))(int, char*, Ureg*)
void (*signal(int sig, void (*func)()))()
{
	void (*oldf)(int, char*, Ureg*);

	if(sig <= 0 || sig > MAXSIG) {
		errno = EINVAL;
		return SIG_ERR;
	}
	oldf = _sighdlr[sig];
	if(sig == SIGKILL)
		return oldf; /* can't catch or ignore SIGKILL */
	_sighdlr[sig] = func;
	return oldf;
}

/* BAD CODE - see /sys/src/ape/lib/ap/$objtype/setjmp.s for real code
int
sigsetjmp(sigjmp_buf buf, int savemask)
{
        int r;

        buf[0] = savemask;
        buf[1] = _psigblocked;
        return setjmp(&buf[2]);
}
*/

/*
 * BUG: improper handling of process signal mask
 */
int
sigaction(int sig, struct sigaction* act, struct sigaction* oact)
{
	if(sig <= 0 || sig > MAXSIG || sig == SIGKILL) {
		errno = EINVAL;
		return -1;
	}
	if(oact) {
		oact->sa_handler = _sighdlr[sig];
		oact->sa_mask = _psigblocked;
		oact->sa_flags = 0;
	}
	if(act) {
		_sighdlr[sig] = act->sa_handler;
	}
	return 0;
}

/* this is registered in _envsetup */
int
_notehandler(void* u, char* msg)
{
	int i;
	void (*f)(int, char*, Ureg*);
	extern void _doatexits(void); /* in stdio/exit.c */

	if(_finishing)
		_finish(0, 0);
	for(i = 0; i < NSIGTAB; i++) {
		if(strncmp(msg, sigtab[i].msg, strlen(sigtab[i].msg)) == 0) {
			f = _sighdlr[sigtab[i].num];
			if(f == SIG_DFL || f == SIG_ERR)
				break;
			if(f != SIG_IGN) {
				_notetramp(sigtab[i].num, f, u);
				/* notetramp is machine-dependent; doesn't
				 * return to here */
			}
			_NOTED(0); /* NCONT */
			return 0;
		}
	}
	_doatexits();
	_NOTED(1); /* NDFLT */
	return 0;
}

int
_stringsig(char* nam)
{
	int i;

	for(i = 0; i < NSIGTAB; i++)
		if(strncmp(nam, sigtab[i].msg, strlen(sigtab[i].msg)) == 0)
			return sigtab[i].num;
	return 0;
}

char*
_sigstring(int sig)
{
	int i;

	for(i = 0; i < NSIGTAB; i++)
		if(sigtab[i].num == sig)
			return sigtab[i].msg;
	return "unknown signal";
}
