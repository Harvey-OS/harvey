#include <sys/types.h>
#include <lib9.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <fmt.h>

typedef unsigned int u32int;
typedef unsigned long long u64int;

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

extern	ulong	getcallerpc(void*);
extern	void*	mallocz(ulong, int);
extern	void	setmalloctag(void*, ulong);

extern int  dec16(uchar *, int, char *, int);
extern int  enc16(char *, int, uchar *, int);
extern int  dec32(uchar *, int, char *, int);
extern int  enc32(char *, int, uchar *, int);
extern int  dec64(uchar *, int, char *, int);
extern int  enc64(char *, int, uchar *, int);

extern	vlong	nsec(void);

extern void sysfatal(char*, ...);
