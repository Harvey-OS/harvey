typedef struct Method	Method;
struct Method
{
	char	*name;
	void	(*config)(Method*);
	int	(*auth)(void);
	int	(*connect)(void);
	char	*arg;
};

extern char*	bootdisk;
extern char*	rootdir;
extern int	(*cfs)(int);
extern int	cpuflag;
extern char	cputype[];
extern int	fflag;
extern int	kflag;
extern int pushfcall(int);
extern Method	method[];
extern void	(*pword)(int, Method*);
extern char	sys[];
extern uchar	hostkey[];
extern char	username[NAMELEN];
enum
{
	Nbarg=	32,
};
extern int	bargc;
extern char	*bargv[Nbarg];

/* libc equivalent */
extern int	cache(int);
extern char*	checkkey(Method*, char*, char*);
extern void	fatal(char*);
extern void	getpasswd(char*, int);
extern void	key(int, Method*);
extern void glendakey(int, Method*);
extern int	nop(int);
extern int	outin(char*, char*, int);
extern int	plumb(char*, char*, int*, char*);
extern int	readfile(char*, char*, int);
extern long	readn(int, void*, long);
extern int	sendmsg(int, char*);
extern void	setenv(char*, char*);
extern void	settime(int);
extern void	srvcreate(char*, int);
extern void	userpasswd(int, Method*);
extern void	warning(char*);
extern int	writefile(char*, char*, int);
extern void	boot(int, char **);
extern void	doauthenticate(int, Method*);
extern int	parsefields(char*, char**, int, char*);

/* methods */
extern void	configil(Method*);
extern int	authil(void);
extern int	connectil(void);
extern void	configtcp(Method*);
extern int	authtcp(void);
extern int	connecttcp(void);
extern void	configlocal(Method*);
extern int	authlocal(void);
extern int	connectlocal(void);
extern void	configsac(Method*);
extern int	authsac(void);
extern int	connectsac(void);
