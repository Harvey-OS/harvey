#define NSIG	32

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define		K_INTOVF 1	/* integer overflow */
#define		K_INTDIV 2	/* integer divide by zero */
#define		K_FLTOVF 3	/* floating overflow */
#define		K_FLTDIV 4	/* floating/decimal divide by zero */
#define		K_FLTUND 5	/* floating underflow */
#define		K_DECOVF 6	/* decimal overflow */
#define		K_SUBRNG 7	/* subscript out of range */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGKIL	9
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */

#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define SIGTINT	23	/* to pgrp on every input character if LINTRUP */
#define	SIGXCPU	24	/* exceeded CPU time limit */
#define	SIGXFSZ	25	/* exceeded file size limit */
#define	SIGLAB	26	/* file label changed; secure unix only (not reset) */
