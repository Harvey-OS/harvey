/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Wrapper around the ugly sys/wait includes/ifdefs */
/* $Id$ */

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifndef POSIX_SYS_WAIT
/* Get rid of system macros (which probably use union wait) */
# undef WIFCORED
# undef WIFEXITED
# undef WEXITSTATUS
# undef WIFSIGNALED
# undef WTERMSIG
# undef WIFSTOPPED
# undef WSTOPSIG
#endif /* POSIX_SYS_WAIT */

typedef int WAIT_T;

#ifndef WIFCORED
# define WIFCORED(s)	((s) & 0x80)
#endif
#define WSTATUS(s)	(s)

#ifndef WIFEXITED
# define WIFEXITED(s)	(((s) & 0xff) == 0)
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(s)	(((s) >> 8) & 0xff)
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(s)	(((s) & 0xff) != 0 && ((s) & 0xff) != 0x7f)
#endif
#ifndef WTERMSIG
# define WTERMSIG(s)	((s) & 0x7f)
#endif
#ifndef WIFSTOPPED
# define WIFSTOPPED(s)	(((s) & 0xff) == 0x7f)
#endif
#ifndef WSTOPSIG
# define WSTOPSIG(s)	(((s) >> 8) & 0xff)
#endif

#if !defined(HAVE_WAITPID) && defined(HAVE_WAIT3)
  /* always used with p == -1 */
# define ksh_waitpid(p, s, o)	wait3((s), (o), (struct rusage *) 0)
#else /* !HAVE_WAITPID && HAVE_WAIT3 */
# define ksh_waitpid(p, s, o)	waitpid((p), (s), (o))
#endif /* !HAVE_WAITPID && HAVE_WAIT3 */
