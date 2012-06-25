#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Linuxcall Linuxcall;

struct Linuxcall
{
	char	*name;
	void	*func;
	int	(*stub)(Ureg *, void *);
};

static int fcall0(Ureg *, void *func){return ((int (*)(void))func)();}
static int fcall1(Ureg *u, void *func){return ((int (*)(int))func)(u->bx);}
static int fcall2(Ureg *u, void *func){return ((int (*)(int, int))func)(u->bx, u->cx);}
static int fcall3(Ureg *u, void *func){return ((int (*)(int, int, int))func)(u->bx, u->cx, u->dx);}
static int fcall4(Ureg *u, void *func){return ((int (*)(int, int, int, int))func)(u->bx, u->cx, u->dx, u->si);}
static int fcall5(Ureg *u, void *func){return ((int (*)(int, int, int, int, int))func)(u->bx, u->cx, u->dx, u->si, u->di);}
static int fcall6(Ureg *u, void *func){return ((int (*)(int, int, int, int, int, int))func)(u->bx, u->cx, u->dx, u->si, u->di, u->bp);}

#include "linuxcalltab.out"

static Linuxcall nocall = {
	.name = "nosys",
	.func = sys_nosys,
	.stub = fcall0,
};

