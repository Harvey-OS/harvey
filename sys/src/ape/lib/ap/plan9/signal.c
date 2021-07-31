#include "lib.h"
#include "sys9.h"
#include <signal.h>
#include <errno.h>
#include <string.h>

static struct {
	char	*msg;	/* just check prefix */
	int	num;
} sigtab[] = {
	{"hangup",				SIGHUP},
	{"interrupt",				SIGINT},
	{"quit",				SIGQUIT},
	{"alarm",				SIGALRM},
	{"sys: trap: illegal instruction",	SIGILL},
	{"sys: trap: reserved instruction",	SIGILL},
	{"sys: trap: reserved",			SIGILL},
	{"abort",				SIGABRT},
	{"sys: fp:",				SIGFPE},
	{"exit",				SIGKILL},
	{"die",					SIGKILL},
	{"kill",				SIGKILL},
	{"sys: trap: bus error",		SIGSEGV},
	{"sys: trap: address error",		SIGSEGV},
	{"sys: trap: TLB",			SIGSEGV},
	{"sys: write on closed pipe",		SIGPIPE},
	{"alarm",				SIGALRM},
	{"term",				SIGTERM},
	{"usr1",				SIGUSR1},
	{"usr2",				SIGUSR2},
};
#define NSIGTAB ((sizeof sigtab)/(sizeof (sigtab[0])))

#define MAXSIG SIGUSR2

void	(*_sighdlr[MAXSIG+1])(int); /* 0 initialized: SIG_DFL */

void
(*signal(int sig, void (*func)(int)))(int)
{
	void(*oldf)(int);

	if(sig <= 0 || sig > MAXSIG){
		errno = EINVAL;
		return SIG_ERR;
	}
	oldf = _sighdlr[sig];
	if(sig == SIGKILL)
		return oldf;	/* can't catch or ignore SIGKILL */
	_sighdlr[sig] = func;
	return oldf;
}

/* this is registered in _envsetup */

int
_notehandler(void *u, char *msg)
{
	int i;
	void(*f)(int);

	if(_finishing)
		_finish(0, 0);
	for(i = 0; i<NSIGTAB; i++){
		if(strncmp(msg, sigtab[i].msg, strlen(sigtab[i].msg)) == 0){
			f = _sighdlr[sigtab[i].num];
			if(f == SIG_DFL || f == SIG_ERR)
				break;
			if(f != SIG_IGN)
				(*f)(sigtab[i].num);
			_NOTED(0); /* NCONT */
			return;
		}
	}
	_NOTED(1); /* NDFLT */
}

int
_stringsig(char *nam)
{
	int i;

	for(i = 0; i<NSIGTAB; i++)
		if(strncmp(nam, sigtab[i].msg, strlen(sigtab[i].msg)) == 0)
			return sigtab[i].num;
	return 0;
}

char *
_sigstring(int sig)
{
	int i;

	for(i=0; i<NSIGTAB; i++)
		if(sigtab[i].num == sig)
			return sigtab[i].msg;
	return "unknown signal";
}

void
_sigclear(void)
{
	memset(_sighdlr, 0, sizeof(_sighdlr)); /* all are SIG_DFL again */
}
