#ifndef __DYNLD_H
#define	__DYNLD_H
#if !defined(_PLAN9_SOURCE)
    This header file is an extension to ANSI/POSIX
#endif

#pragma src "/sys/src/ape/lib/dynld"
#pragma lib "/$M/lib/ape/libdynld.a"

typedef struct Dynobj Dynobj;
typedef struct Dynsym Dynsym;

struct Dynobj
{
	unsigned long	size;		/* total size in bytes */
	unsigned long	text;		/* bytes of text */
	unsigned long	data;		/* bytes of data */
	unsigned long	bss;		/* bytes of bss */
	unsigned char*	base;	/* start of text, data, bss */
	int	nexport;
	Dynsym*	export;	/* export table */
	int	nimport;
	Dynsym**	import;	/* import table */
};

/*
 * this structure is known to the linkers
 */
struct Dynsym
{
	unsigned long	sig;
	unsigned long	addr;
	char	*name;
};

extern Dynsym*	dynfindsym(char*, Dynsym*, int);
extern void	dynfreeimport(Dynobj*);
extern void*	dynimport(Dynobj*, char*, unsigned long);
extern int	dynloadable(void*, long (*r)(void*,void*,long), long long(*sk)(void*,long long,int));
extern Dynobj*	dynloadfd(int, Dynsym*, int, unsigned long);
extern Dynobj*	dynloadgen(void*, long (*r)(void*,void*,long), long long (*s)(void*,long long,int), void (*e)(char*), Dynsym*, int, unsigned long);
extern long	dynmagic(void);
extern void	dynobjfree(Dynobj*);
extern char*	dynreloc(unsigned char*, unsigned long, int, Dynsym**, int);
extern int	dyntabsize(Dynsym*);

extern Dynsym	_exporttab[];	/* created by linker -x (when desired) */

#endif
