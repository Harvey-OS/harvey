#include	<u.h>
#include	<libc.h>
#include	<type.h>

#define	TEST1(o1,o2)\
	settype(&t1, 0);\
	t1.sc = t2.o1 | t3.o2;\
	t1.uc = t2.o1 | t3.o2;\
	t1.sh = t2.o1 | t3.o2;\
	t1.uh = t2.o1 | t3.o2;\
	t1.sl = t2.o1 | t3.o2;\
	t1.ul = t2.o1 | t3.o2;\
	t1.sb = t2.o1 | t3.o2;\
	t1.ub = t2.o1 | t3.o2;\
	tsttype1(&t1, &a1, "o1", "o2");

#define	TEST(o)\
	TEST1(o,sc);\
	TEST1(o,uc);\
	TEST1(o,sh);\
	TEST1(o,uh);\
	TEST1(o,sl);\
	TEST1(o,ul);\
	TEST1(o,sb);\
	TEST1(o,ub);

#define	TESTa(o)\
	settype(&t1, A);\
	t1.sc |= t2.o;\
	t1.uc |= t2.o;\
	t1.sh |= t2.o;\
	t1.uh |= t2.o;\
	t1.sl |= t2.o;\
	t1.ul |= t2.o;\
	t1.sb |= t2.o;\
	t1.ub |= t2.o;\
	tsttype1(&t1, &a1, "o", "");

#define	A	12
#define	B	18
#define	C	(A|B)

void
or(void)
{
	Type t1, t2, t3;
	Type a1;

	print("or test\n");
	settype(&t2, A);
	settype(&t3, B);
	settype(&a1, C);
	
	TEST(sc);
	TEST(uc);
	TEST(sh);
	TEST(uh);
	TEST(sl);
	TEST(ul);
	TEST(sb);
	TEST(ub);

	settype(&a1, A);
	tsttype1(&t2, &a1, "", "");
	settype(&a1, B);
	tsttype1(&t3, &a1, "", "");

	print("orass test\n");
	settype(&t2, B);
	settype(&a1, C);
	
	TESTa(sc);
	TESTa(uc);
	TESTa(sh);
	TESTa(uh);
	TESTa(sl);
	TESTa(ul);
	TESTa(sb);
	TESTa(ub);

	settype(&a1, B);
	tsttype1(&t2, &a1, "", "");
}
