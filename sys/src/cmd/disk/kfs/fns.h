#include "portfns.h"

long	belong(char *);
Chan*	chaninit(char*);
void	check(Filsys *, long);
int 	cmd_exec(char*);
void	consserve(void);
void	confinit(void);
int	fsinit(int, int);
void	*ialloc(ulong);
int	nextelem(void);
long	number(int, int);
Device	scsidev(char*);
int	skipbl(int);
void	startproc(void (*)(void), char *);
void	syncproc(void);
void	syncall(void);

int	fprint(int, char*, ...);
void	wreninit(Device);
int	wrencheck(Device);
void	wrenream(Device);
long	wrensize(Device);
long	wrensuper(Device);
long	wrenroot(Device);
int	wrenread(Device, long, void *);
int	wrenwrite(Device, long, void *);

/*
 * macros for compat with bootes
 */
#define	localfs			1

#define devgrow(d, s)	0
#define nofree(d, a)	0
#define isro(d)		0

#define	superaddr(d)		((*devcall[d.type].super)(d))
#define	getraddr(d)		((*devcall[d.type].root)(d))
#define devsize(d)		((*devcall[d.type].size)(d))
#define	devwrite(d, a, v)	((*devcall[d.type].write)(d, a, v))
#define	devread(d, a, v)	((*devcall[d.type].read)(d, a, v))
