#include	<u.h>
#include	<libc.h>
#include	<type.h>


#define	ASGN(o)\
	settype(&t1, 0);\
	t1.sc = t2.o;\
	t1.uc = t2.o;\
	t1.sh = t2.o;\
	t1.uh = t2.o;\
	t1.sl = t2.o;\
	t1.ul = t2.o;\
	t1.ff = t2.o;\
	t1.df = t2.o;\
	t1.sb = t2.o;\
	t1.ub = t2.o;\
	tsttype(&t1, &t2, "o", "");

void
assign(void)
{
	Type t1, t2;

	print("assign test\n");
	settype(&t2, 10);
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
}
