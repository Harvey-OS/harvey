#include	<u.h>
#include	<libc.h>
#include	<type.h>


#define	ASGN(o)\
	settype(&t1, 0);\
	t2.o = 10;\
	t1.sc = t2.o++;\
	t1.uc = t2.o++;\
	t1.sh = t2.o++;\
	t1.uh = t2.o++;\
	t1.sl = t2.o++;\
	t1.ul = t2.o++;\
	t1.ff = t2.o++;\
	t1.df = t2.o++;\
	t1.sb = t2.o++;\
	t1.ub = t2.o++;\
	tsttype(&t1, &a1, "o", "");

void
postinc(void)
{
	Type a1, t1, t2;

	print("postinc test\n");
	settype(&t2, 0);

	a1.sc = 10;
	a1.uc = 11;
	a1.sh = 12;
	a1.uh = 13;
	a1.sl = 14;
	a1.ul = 15;
	a1.ff = (float)16;
	a1.df = (double)17;
	a1.sb = 18;
	a1.ub = 19;
	
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
