typedef struct Method	Method;
struct Method
{
	char	*name;
	void	(*config)(Method*);
	int	(*connect)(void);
	char	*arg;
};
enum
{
	Statsz=	256,
	Nbarg=	16,
};

#define dprint(...) if(debugboot) fprint(2, __VA_ARGS__); else USED(debugboot)

extern char*	bootdisk;		/* defined in ../$arch/boot$CONF.c */
extern char*	rootdir;
extern int	(*cfs)(int);
/*
 * flag: start factotum with -S not -u, time out boot prompts,
 * invoke init -c not -t (run cpurc not termrc).
 */
extern int	cpuflag;
extern char	cputype[];
extern int	debugboot;
extern int	fflag;
extern int	kflag;
extern Method	method[];		/* defined in ../$arch/boot$CONF.c */
extern void	(*pword)(int, Method*);
extern char	sys[];
extern uchar	hostkey[];
extern uchar	statbuf[Statsz];
extern int	bargc;
extern char	*bargv[Nbarg];
extern int	pcload;
extern int	promptwait;

/* libc equivalent */
extern void	authentication(int);
extern int	cache(int);
extern char*	checkkey(Method*, char*, char*);
extern int	chmod(char *file, int mode);
extern void	fatal(char*);
extern void	getpasswd(char*, int);
extern void	key(int, Method*);
extern int	mountusbparts(void);
extern int	outin(char*, char*, int);
extern int	plumb(char*, char*, int*, char*);
extern int	readfile(char*, char*, int);
extern int	readparts(void);
extern long	readn(int, void*, long);
extern int	run(char *file, ...);
extern int	runv(char **argv);
extern int	sendmsg(int, char*);
extern void	setenv(char*, char*);
extern void	settime(int, int, char*);
extern void	srvcreate(char*, int);
extern void	usbinit(int post);
extern void	warning(char*);
extern int	writefile(char*, char*, int);
extern void	boot(int, char **);
extern void	doauthenticate(int, Method*);
extern int	old9p(int);
extern int	parsefields(char*, char**, int, char*);
extern int	fetchmissing(char*);
extern char*	archname(char*);
extern char*	basename(char*);
extern char*	bootname(char*);

/* methods */
extern void	configtcp(Method*);
extern int	connecttcp(void);

extern void	configlocal(Method*);
extern int	connectlocal(void);

extern void	configpaq(Method*);
extern int	connectpaq(void);

extern void	configembed(Method*);
extern int	connectembed(void);

extern void	configip(int, char**, int);

/* hack for passing authentication address */
extern char	*authaddr;

extern void	catchint(void *a, char *note);

#define USB	/* pc still needs it */
