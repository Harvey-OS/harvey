
#include <u.h>
#include <libc.h>
#include <ureg.h>


#define RET 0xc3

/* A stack to hold pcs when signals nest */
#define MAXSIGSTACK 20
typedef struct Pcstack Pcstack;
static struct Pcstack {
	int sig;
	void (*hdlr)(int, char*, struct Ureg*);
	uint64_t restorepc;
	struct Ureg *u;
} pcstack[MAXSIGSTACK];

static int nstack = 0;

static void notecont(struct Ureg*, char*);

static void
_notetramp(int sig, void (*hdlr)(int, char*, struct Ureg*), struct Ureg *u)
{
	Pcstack *p;

	if(nstack >= MAXSIGSTACK)
		noted(1);	/* nesting too deep; just do system default */
	p = &pcstack[nstack];
	p->restorepc = u->ip;
	p->sig = sig;
	p->hdlr = hdlr;
	p->u = u;
	nstack++;
	u->ip = (uint64_t) notecont;
	noted(2);	/* NSAVE: clear note but hold state */
}

static void
notecont(struct Ureg *u, char *s)
{
	Pcstack *p;
	void(*f)(int, char*, struct Ureg*);

	p = &pcstack[nstack-1];
	f = p->hdlr;
	u->ip = p->restorepc;
	nstack--;
	(*f)(p->sig, s, u);
	noted(3);	/* NRSTR */
}

static void fu(int i, char *s, struct Ureg *u)
{
	print("fu: %d, %s, %p\n", i, s, u);
}
	    
/* this is registered in _envsetup */
static void
handler(void *u, char *msg)
{
	_notetramp(1, fu, u);
	noted(0); /* NCONT */
}

void
main(void)
{
	void (*f)(void) = nil;
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}

	f();
	print("FAIL");
	exits("FAIL");
}
