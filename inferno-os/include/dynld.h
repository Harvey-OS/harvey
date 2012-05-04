/* uncomment these lines when installed in /sys/include */
/*
#pragma src "/sys/src/libdynld"
#pragma	lib	"libdynld.a"
*/

typedef struct Dynobj Dynobj;
typedef struct Dynsym Dynsym;

struct Dynobj
{
	ulong	size;		/* total size in bytes */
	ulong	text;		/* bytes of text */
	ulong	data;		/* bytes of data */
	ulong	bss;		/* bytes of bss */
	uchar*	base;	/* start of text, data, bss */
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
	ulong	sig;
	ulong	addr;
	char	*name;
};

extern Dynsym*	dynfindsym(char*, Dynsym*, int);
extern void	dynfreeimport(Dynobj*);
extern void*	dynimport(Dynobj*, char*, ulong);
extern int	dynloadable(void*, long (*r)(void*,void*,long), vlong(*sk)(void*,vlong,int));
extern Dynobj*	dynloadfd(int, Dynsym*, int, ulong);
extern Dynobj*	dynloadgen(void*, long (*r)(void*,void*,long), vlong (*s)(void*,vlong,int), void (*e)(char*), Dynsym*, int, ulong);
extern long	dynmagic(void);
extern void	dynobjfree(Dynobj*);
extern char*	dynreloc(uchar*, ulong, int, Dynsym**, int);
extern int	dyntabsize(Dynsym*);

extern Dynsym	_exporttab[];	/* created by linker -x (when desired) */
