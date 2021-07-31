#include	<u.h>
#include	<libc.h>
#include	<type.h>


#define	ASGN(o)\
	settype(&t1, 0);\
	t2.o = 10;\
	t1.sc = ++t2.o;\
	t1.uc = ++t2.o;\
	t1.sh = ++t2.o;\
	t1.uh = ++t2.o;\
	t1.sl = ++t2.o;\
	t1.ul = ++t2.o;\
	t1.ff = ++t2.o;\
	t1.df = ++t2.o;\
	t1.sb = ++t2.o;\
	t1.ub = ++t2.o;\
	tsttype(&t1, &a1, "o", "");

void
preinc(void)
{
	Type a1, t1, t2;

	print("preinc test\n");
	settype(&t2, 0);

	a1.sc = 11;
	a1.uc = 12;
	a1.sh = 13;
	a1.uh = 14;
	a1.sl = 15;
	a1.ul = 16;
	a1.ff = 17;
	a1.df = 18;
	a1.sb = 19;
	a1.ub = 20;

	ASGN(sc);
	ASGN(uc);
	ASGN(sh);
	ASGN(uh);
	ASGN(sl);
	ASGN(ul);
	ASGN(ff);
	ASGN(df);
	ASGN(sb);
	ASGN(ub);

	settype(&a1, 20);
	tsttype(&t2, &a1, "", "");
}
