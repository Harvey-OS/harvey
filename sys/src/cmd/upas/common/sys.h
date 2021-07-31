/*
 * System dependent header files for research
 */

#include <u.h>
#include <libc.h>
#include <regexp.h>
#include <bio.h>
#include "String.h"

/*
 *  for the lock routines in libsys.c
 */
typedef struct Mlock	Mlock;
struct Mlock {
	int fd;
	int pid;
	String *name;
};

/*
 *  from config.c
 */
extern char *MAILROOT;	/* root of mail system */
extern char *UPASLOG;	/* log directory */
extern char *UPASLIB;	/* upas library directory */
extern char *UPASBIN;	/* upas binary directory */
extern char *UPASTMP;	/* temporary directory */
extern char *SHELL;	/* path name of shell */
extern char *POST;	/* path name of post server addresses */
extern int MBOXMODE;	/* default mailbox protection mode */

/*
 *  files in libsys.c
 */
extern char	*sysname_read(void);
extern char	*alt_sysname_read(void);
extern char	*domainname_read(void);
extern char	**sysnames_read(void);
extern char	*getlog(void);
extern char	*thedate(void);
extern Biobuf	*sysopen(char*, char*, ulong);
extern int	sysopentty(void);
extern int	sysclose(Biobuf*);
extern int	sysmkdir(char*, ulong);
extern int	syschgrp(char*, char*);
extern Mlock	*syslock(char *);
extern void	sysunlock(Mlock *);
extern void	syslockrefresh(Mlock *);
extern int	e_nonexistent(void);
extern int	e_locked(void);
extern long	sysfilelen(Biobuf*);
extern int	sysremove(char*);
extern int	sysrename(char*, char*);
extern int	sysexist(char*);
extern int	syskill(int);
extern int	syskillpg(int);
extern int	syscreate(char*, int, ulong);
extern Mlock	*trylock(char *);
extern void	exit(int);
extern void	pipesig(int*);
extern void	pipesigoff(void);
extern int	holdon(void);
extern void	holdoff(int);
extern int	syscreatelocked(char*, int, int);
extern int	sysopenlocked(char*, int);
extern int	sysunlockfile(int);
extern int	sysfiles(void);
extern int 	become(char**, char*);
extern int	sysdetach(void);
extern int	sysdirreadall(int, Dir**);
extern String	*username(String*);
extern char*	remoteaddr(int, char*);
extern int	creatembox(char*, char*);

extern String	*readlock(String*);
extern char	*homedir(char*);
extern String	*mboxname(char*, String*);
extern String	*deadletter(String*);

/*
 *  maximum size for a file path
 */
#define MAXPATHLEN 128
