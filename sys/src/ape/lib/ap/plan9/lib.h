#include <sys/types.h>
#include <sys/limits.h>
#include <fcntl.h>

/* be sure to change _fdinfo[] init in _buf if you change this */
typedef struct Fdinfo{
	unsigned long	flags;
	unsigned long	oflags;
	/*
	 * the following are if flags&FD_BUFFERED
	 */
	char		*name;
	short		n;	/* bytes valid in buf */
	char		eof;	/* if true, eof after current buffer exhausted */
	char		*buf;	/* malloc'd region to hold chars */
	char		*next;	/* next place to fetch from buf */
	char		*end;	/* just after end of allocated buf */
	int		pid;	/* pid of read/write proc */
} Fdinfo;

/* #define FD_CLOEXEC 1 is in fcntl.h */

#define FD_ISOPEN	0x2
#define FD_BUFFERED	0x4
#define FD_ISPIPE	0x20

#define READMAX		8192	/* max through a pipe */

extern Fdinfo	_fdinfo[OPEN_MAX];
extern int	_finishing;
extern int	_sessleader;
extern void	(*_sighdlr[])(int);
extern char	*_sigstring(int);
extern int	_stringsig(char *);
extern long	_psigblocked;
extern int	_startbuf(int);
extern int	_servebuf(int);
extern void	_sigclear(void);
extern void	_finish(int, char *);
extern void	_newcpid(int);
extern void	_delcpid(int);
extern char	*_ultoa(char *, unsigned long);
extern int	_notehandler(void *, char *);
extern void	_syserrno(void);
extern int	_getpw(int *, char **, char **);
