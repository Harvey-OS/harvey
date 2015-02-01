/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

extern int havesecstore(char *addr, char *owner);
extern char *secstore;
extern char secstorebuf[65536];
extern char *secstorefetch(char *addr, char *owner, char *passwd);
extern char *authserver;
extern char *readcons(char *prompt, char *def, int secret);
extern int exportfs(int, int);
extern char *user;
extern char *getkey(char*, char*);
extern char *findkey(char**, char*);
extern int dialfactotum(void);
extern char *getuser(void);
extern void cpumain(int, char**);
