#pragma	lib	"libworker.a"
#pragma	src	"/sys/src/libworker"

/* Fake kernel environment in user space */

#define	NERR		64

#define	waserror()	setjmp(*_waserror())
#define report		if(!Reporting){}else _report
#define reportdata	if(!Reporting){}else _reportdata

typedef struct Mach		Mach;
typedef struct Debugs		Debugs;

struct Mach {
	int	nerrlab;
	jmp_buf errlab[NERR];
	void	*aux;
	void	*worker;
};

struct Debugs {
	char	*name;
	ulong	val;
};

void	error(char *err);
void	ferror(char *fmt, ...);		/* formatted error */
void	nexterror(void);
void	poperror(void);
void	silenterror(char *fmt, ...);
jmp_buf	*_waserror(void);

void	_report(ulong, char *, ...);
void	_reportdata(ulong, char*, void*, int, int);
void	reportdiscard(void);
void	reporting(char *);
void	reportinit(int);

#pragma	varargck	argpos	_report	3

long	(*µs)(void);

extern int Reporting;
extern Debugs debugs[];
