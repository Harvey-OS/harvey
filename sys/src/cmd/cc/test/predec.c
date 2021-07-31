#include	<u.h>
#include	<libc.h>
#include	<type.h>


#define	ASGN(o)\
	settype(&t1, 0);\
	t2.o = 10;\
	t1.sc = --t2.o;\
	t1.uc = --t2.o;\
	t1.sh = --t2.o;\
	t1.uh = --t2.o;\
	t1.sl = --t2.o;\
	t1.ul = --t2.o;\
	t1.ff = --t2.o;\
	t1.df = --t2.o;\
	t1.sb = --t2.o;\
	t1.ub = --t2.o;\
	tsttype(&t1, &a1, "o", "");

void
predec(void)
{
	Type a1, t1, t2;

	print("predec test\n");
	settype(&t2, 0);

	a1.sc = 9;
	a1.uc = 8;
	a1.sh = 7;
	a1.uh = 6;
	a1.sl = 5;
	a1.ul = 4;
	a1.ff = (float)3;
	a1.df = (double)2;
	a1.sb = 1;
	a1.ub = 0;
	
	ASGN(sc);
	ASGN(uc);
	ASGN(sh);
	ASGN(uh);
	ASGN(sl);
	ASGN(ul);
	ASGN(ff);
	ASGN(df);

	settype(&a1, 0);
	tsttype(&t2, &a1, "", "");
}
