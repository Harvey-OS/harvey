#ifndef __ERROR_H
#define __ERROR_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in pure ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libv.a"

#ifdef __cplusplus
extern "C" {
#endif

extern char *_progname;		/* program name */
extern void _perror(char *);	/* perror but with _progname */

#ifdef __cplusplus
}
#endif

#endif /* _ERROR_H */
