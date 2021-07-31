#include <signal.h>

/*
 * BUG: only handle one case: SIGUSR1, interpreted
 * as async io notification (non-standard, a al BSD SIGIO).
 * Flag field is ignored.
 */

extern void	(*_sigiohdlr)(int);
extern sigset_t	_psigblocked;

int
sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	if(oact){
		oact->sa_handler = _sigiohdlr;
		oact->sa_mask = _psigblocked;
		oact->sa_flags = 0;
	}
	if(act){
		if(sig == SIGUSR1){
			_sigiohdlr = act->sa_handler;
			_psigblocked |= act->sa_mask;
			if(act->sa_handler==SIG_ERR ||
			   act->sa_handler==SIG_IGN)
				act->sa_handler = SIG_DFL;
		}
	}
	return 0;
}
