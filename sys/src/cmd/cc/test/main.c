#include	<u.h>
#include	<libc.h>
#include	<type.h>

static	int	ntest;

void
main(int argc, char *argv[])
{

	USED(argc);
	USED(argv);
	ntest = 0;
	assign();
	preinc();
	postinc();
	predec();
	postdec();
	add();	sub();	mul();	div();	mod();
	and();	or();	xor();
	lsh();	rsh();
	if(ntest != 954)
		print("ntest = %d\n", ntest);
	exits(0);
}

/*
 * sets all the fields
 */
void
settype(Type *t, int x)
{
	t->sc = x;
	t->uc = x;
	t->sh = x;
	t->uh = x;
	t->sl = x;
	t->ul = x;
	t->ff = (float)x;
	t->df = (double)x;
	t->sb = x;
	t->ub = x;
}

/*
 * checks only the integers
 */
void
tsttype1(Type *t1, Type *t2, char *f1, char *f2)
{
	int error;

	error = 0;
	if(t1->sc != t2->sc) {
		print("sc is %d sb %d\n", t1->sc, t2->sc);
		error++;
	}
	if(t1->uc != t2->uc) {
		print("uc is %d sb %d\n", t1->uc, t2->uc);
		error++;
	}
	if(t1->sh != t2->sh) {
		print("sh is %d sb %d\n", t1->sh, t2->sh);
		error++;
	}
	if(t1->uh != t2->uh) {
		print("uh is %d sb %d\n", t1->uh, t2->uh);
		error++;
	}
	if(t1->sl != t2->sl) {
		print("sl is %d sb %d\n", t1->sl, t2->sl);
		error++;
	}
	if(t1->ul != t2->ul) {
		print("ul is %d sb %d\n", t1->ul, t2->ul);
		error++;
	}
	if(t1->sb != t2->sb) {
		print("sb is %d sb %d\n", t1->sb, t2->sb);
		error++;
	}
	if(t1->ub != t2->ub) {
		print("ub is %d sb %d\n", t1->ub, t2->ub);
		error++;
	}
	if(error)
		print("	t1=%s t2=%s\n", f1, f2);
	ntest++;
}

/*
 * checks all the fields
 */
void
tsttype(Type *t1, Type *t2, char *f1, char *f2)
{
	int error;

	tsttype1(t1, t2, f1, f2);
#ifndef	NOFLOAT
	error = 0;
	if(t1->ff != t2->ff) {
		print("ff is %g sb %g\n", t1->ff, t2->ff);
		error++;
	}
	if(t1->df != t2->df) {
		print("df is %g sb %g\n", t1->df, t2->df);
		error++;
	}
	if(error)
		print("	t1=%s t2=%s\n", f1, f2);
#endif
}
