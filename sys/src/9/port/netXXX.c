#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 *  This is a prototype file for writing a new network protocol
 */

static void nulloput(Queue*, Block*);
static void protooput(Queue*, Block*);
static void protoiput(Queue*, Block*);

static Qinfo XXXprotold =
{
	protoiput,
	protooput,
	0,
	0,
	"proto",
	0
};

static Qinfo XXXmuxld =
{
	0,
	nulloput,
	0,
	0,
	"mux"
};

static void
protooput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

static void
protoiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

static void
nulloput(Queue *q, Block *bp)
{
	freeb(bp);
}

static void XXXaddr(Chan*, char*, int);
static void XXXother(Chan*, char*, int);
static void XXXraddr(Chan*, char*, int);
static void XXXruser(Chan*, char*, int);
static int XXXclone(Chan*);
static void XXXconnect(Chan*, char*);

Network netXXX =
{
	"XXX",
	4,
	32,			/* # conversations */
	&XXXmuxld,
	&XXXprotold,
	0,			/* no listener */
	XXXclone,
	XXXconnect,
	0,			/* no announces */
	4,			/* info files */
	{ { "addr",	XXXaddr, },
	  { "other",	XXXother, },
	  { "raddr",	XXXraddr, },
	  { "ruser",	XXXruser, },
	},
};

static void
XXXaddr(Chan *c, char *buf, int len)
{
	strncpy(buf, "my address", len-1);
}

static void
XXXother(Chan *c, char *buf, int len)
{
	strncpy(buf, "other stuff", len-1);
}

static void
XXXraddr(Chan *c, char *buf, int len)
{
	strncpy(buf, "remote address", len-1);
}

static void
XXXruser(Chan *c, char *buf, int len)
{
	strncpy(buf, "remote user", len-1);
}

static int
XXXclone(Chan *c)
{
	return 5;
}

static void
XXXconnect(Chan *c, char *dest)
{
}
