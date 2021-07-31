#ifndef __LIB9_H
#define __LIB9_H
#ifndef _PLAN9_SOURCE
    This header file is an extension to ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/lib9.a"


#define	MORDER	0x0003	/* mask for bits defining order of mounting */
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */
#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MMASK	0x0007	/* all bits on */

#define	FORKPG		1
#define	FORKEG		2
#define	FORKFD		4

extern	int	errstr(char*);
extern	int	bind(char*, char*, int);
extern	int	mount(int, char*, int, char*, char*);
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
