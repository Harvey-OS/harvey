typedef struct Proc Proc;
typedef struct Mach Mach;
struct Mach {
	int	machno;
	Proc*	p;
};
struct Proc {
	int	a;
	int b;
};
extern register Proc *up;
extern register Mach *m;
Proc proc;

void
sync(void)
{
}

void
f(Proc *p)
{
	up->a = up->b;
	up->a = 5;
	up = p;
	m->p = up;
	m = (void*)0xFFF;
	sync();
	m->p = up;
}
