#include <u.h>
#include <libc.h>

#define DPRECSTR "0.0000004000000000125"
#define DPREC 0.0000004000000000125
#define DIEEELO 0x9ac0499f
#define DIEEEHI 0x3e9ad7f2

jmp_buf errj;
char* err;

void
catcher(void* u, char* s)
{
	err = 0;
	if(strncmp(s, "sys: fp:", 8) == 0) {
		err = s;
		notejmp(u, errj, 0);
	}
	noted(NDFLT);
}

void
tstdiv(double p)
{
	double r = 1.0;

	r /= p;
	print("1/%0.20g = %0.20g\n", p, r);
}

void
main(void)
{
	double p = DPREC;
	int d[2] = {DIEEELO, DIEEEHI};
	uint64_t dieee, q;
	dieee = *(uint64_t*)d;
	q = *(uint64_t*)&p;

	err = 0;
	notify(catcher);
	setjmp(errj);
	if(err) {
		fprint(2, "%s\n", err);
		exits("FAIL");
	}

	print("Double-precision test number: %s\n", DPRECSTR);
	print("Expected internal representation: %ullx\n", dieee);
	print("Actual internal representation: %ullx\n", q);

	if(q != dieee)
		exits("FAIL");

	tstdiv(p);
}
