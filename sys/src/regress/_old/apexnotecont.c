
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
print("a\n");
	p->restorepc = u->ip;
print("b\n");
	p->sig = sig;
print("c\n");
	p->hdlr = hdlr;
print("d\n");
	p->u = u;
print("e\n");
	nstack++;
print("f\n");
	u->ip = (uint64_t) notecont;
print("g\n");
	noted(2);	/* NSAVE: clear note but hold state */
}

static void
notecont(struct Ureg *u, char *s)
{
	Pcstack *p;
	void(*f)(int, char*, struct Ureg*);

print("notecont u is %p, msg %s\n", u, s);
print("A\n");
	p = &pcstack[nstack-1];
print("B\n");
	f = p->hdlr;
print("C\n");
	u->ip = p->restorepc;
print("D\n");
	nstack--;
print("E\n");
	print("notecont: let's call f at %p\n", f);
print("F\n");
	(*f)(p->sig, s, u);
print("G\n");
	print("notecont: back ...\n");
print("H\n");
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
	print("HANDLER: msg %s\n", msg);
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
