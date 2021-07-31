#ifndef __PCONS_H
#define __PCONS_H
#ifndef _PCONS_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/libap.a"

extern int pcons(void);		/* pseudo-console mounting function */
extern int pushtty(void);	/* push tty discipline on fds 0, 1, 2 */

#endif
