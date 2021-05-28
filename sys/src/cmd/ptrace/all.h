#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include <ptrace.h>

#include <draw.h>
#include <cursor.h>
#include <keyboard.h>
#include <mouse.h>
#include <plumb.h>

typedef struct St St;
typedef struct Proc Proc;

enum
{
	Dump = 1,
	Plot = 2,

	Stack = 32*1024,
	Incr = 4096,

	Wx = 718,		/* default window sizes */
	Wy = 400,
	Wid = 3,		/* line width */
	Scroll = 10,	/* fraction of screen in scrolls */
};

struct St
{
	PTraceevent;
	int state;		/* Sready, Srun, Ssleep */
	char *name;

	/* help for browsing */
	int x;			/* min x in screen when shown */
	int pnext;		/* next event index in graph[] for this proc */
};

struct Proc
{
	int pid;
	int* state;		/* indexes in graph[] for state changes */
	int nstate;

	int row;		/* used to plot this process */
	int s0, send;	/* slice in state[] shown */

	int *pnextp;		/* help for building St.next list */
};

#define NS(x)	((vlong)x)
#define US(x)	(NS(x) * 1000ULL)
#define MS(x)	(US(x) * 1000ULL)
#define S(x)	(MS(x) * 1000ULL)

#pragma	varargck	type	"T"		vlong
#pragma	varargck	type	"G"		St*

extern Biobuf *bout;
extern int what, verb, newwin;
extern St **graph;
extern int ngraph;
extern Proc *proc;
extern int nproc;

/*
 | c/f2p st.c
 */
extern int	Gfmt(Fmt *f);
extern int	Tfmt(Fmt *f);
extern void	makeprocs(void);
extern St*	pgraph(Proc *p, int i);
extern void	readall(char *f, int isdev);
