#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 *  reboot stream module definition
 */
static void rebootopen(Queue*, Stream*);
static void rebootiput(Queue*, Block*);
static void rebootoput(Queue*, Block*);
static void rebootreset(void);
Qinfo rebootinfo =
{
	rebootiput,
	rebootoput,
	rebootopen,
	0,
	"reboot",
	0
};

static void
rebootopen(Queue *q, Stream *s)
{
	USED(q);
	USED(s);
	if(strcmp(u->p->user, eve) != 0)
		error(Eperm);
}

void
rebootoput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

static void
rebootiput(Queue *q, Block *bp)
{
	if(bp->type == M_HANGUP){
		print("lost connection to fs, rebooting");
		exit(0);
	}
	PUTNEXT(q, bp);
}
