#ifndef __LIB9_H
#define __LIB9_H
#if !defined(_RESEARCH_SOURCE) && !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/lib9.a"

#include <u.h>		/* ick; need Rune defined below */

#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define MRECOV	0x0008	/* perform recovery if mount channel is lost */
#define MCACHE	0x0010	/* cache some data */
#define	MMASK	0x0007	/* all bits on */

#define	FORKPG		1
#define	FORKEG		2
#define	FORKFD		4

#define	SG_RONLY	0040	/* read only */
#define	SG_CEXEC	0100	/* detach on exec */

enum
{
	RFNAMEG		= (1<<0),
	RFENVG		= (1<<1),
	RFFDG		= (1<<2),
	RFNOTEG		= (1<<3),
	RFPROC		= (1<<4),
	RFMEM		= (1<<5),
	RFNOWAIT	= (1<<6),
	RFCNAMEG	= (1<<10),
	RFCENVG		= (1<<11),
	RFCFDG		= (1<<12),
	RFREND		= (1<<13)
};

extern char *argv0;
#define	ARGBEGIN	for((argv0||(argv0=*argv)),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				Rune _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(*_args && (_args += chartorune(&_argc, _args)))\
				switch(_argc)
#define	ARGEND		SET(_argt);USED(_argt,_argc,_args);}USED(argv, argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	EARGF(x)	(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (char*)0)))

#define	ARGC()		_argc

extern	int	errstr(char*, unsigned int);
extern	int	bind(char*, char*, int);
extern	int	mount(int, int, char*, int, char*);
extern	int	unmount(char*, char*);
extern	int	rfork(int);
extern	int	segattach(int, char*, void*, unsigned long);
extern	int	segbrk(void*, void*);
extern	int	segdetach(void*);
extern	int	segflush(void*, unsigned long);
extern	int	segfree(void*, unsigned long);
extern	unsigned long	rendezvous(unsigned long, unsigned long);
extern	unsigned long	getfcr(void);
extern	unsigned long	getfsr(void);
extern	void		setfcr(unsigned long);
extern	void		setfsr(unsigned long);

#endif
