/*
 * System dependent header files for research
 */
#include <u.h>
#include <libc.h>
#include <regexp.h>
#include <bio.h>
#include "../libString/String.h"

/*
 *  for the lock foutines in libsys.c
 */
typedef struct Lock	Lock;
struct Lock {
	int fd;
	String *name;
};

/*
 *  from config.c
 */
extern char *LOGROOT;	/* root of log system */
extern char *MAILROOT;	/* root of mail system */
extern char *UPASROOT;	/* root of upas system */
extern char *SMTPQROOT; /* root of smtpq directory */
extern char *SYSALIAS;	/* file system alias files are listed in */
extern char *USERALIAS;	/* file system alias files are listed in */
extern int MBOXMODE;	/* default mailbox protection mode */

/*
 *  files in libsys.c
 */
extern char	*sysname_read(void);
extern char	*alt_sysname_read(void);
extern char	*domainname_read(void);
extern char	*getlog(void);
extern char	*thedate(void);
extern Biobuf	*sysopen(char*, char*, ulong);
extern int	sysclose(Biobuf*);
extern int	sysmkdir(char*, ulong);
extern int	syschgrp(char*, char*);
extern Lock	*lock(char *);
extern void	unlock(Lock *);
extern int	e_nonexistant(void);
extern int	e_locked(void);
extern ulong	sysfilelen(Biobuf*);
extern int	sysremove(char*);
extern int	sysrename(char*, char*);
extern int	sysexist(char*);
extern int	syskill(int);
extern int	syscreate(char*, int);
extern long	sysmtime(char*);
extern Lock	*trylock(char *);
extern void	exit(int);
extern void	pipesig(int*);
extern void	pipesigoff(void);
extern void	newprocgroup(void);
extern void	becomenone(void);
extern char*	csquery(char*, char*, char*);
extern int	holdon(void);
extern void	holdoff(int);

extern int nsysfile;
extern int nofile;

/*
 *  maximum size for a file path
 */
#define MAXPATHLEN 128
