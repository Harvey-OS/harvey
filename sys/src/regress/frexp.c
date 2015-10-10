#include <u.h>
#include <libc.h>

#define DEEPS	".5*2^-1073"
#define DENEPS	"-.5*2^-1073"
#define DEPINF	"+Inf*2^0"
#define DENAN "NaN*2^0"

jmp_buf errj;
char *err;
int fail = 0;

void
catcher(void *u, char *s)
{
	err = 0;
	if(strncmp(s, "sys: fp:", 8) == 0){
		err = s;
		notejmp(u, errj, 0);
	}
	noted(NDFLT);
}

void
decomp(double d, int *exp, char *s)
{
	double m;

	m = frexp(d, exp);
	fprint(2, "Expected decomposition: %s\n", s);
	fprint(2, "Actual decomposition: %g*2^%d\n", m, *exp);

	if(isNaN(d)){
		if(*exp != 0 || !isNaN(m))
			fail = 1;
		return;
	}
	if(isInf(d, 1)){
		if(*exp != 0 || !isInf(m, 1))
			fail = 1;
		return;
	}
	if(fabs(d - ldexp(m, *exp)) > 4e-16)
		fail = 1;
}

void
main(void)
{
	double eps, neps;
	int exp;

	err = 0;
	notify(catcher);
	setjmp(errj);
	if(err){
		print("FAIL: %s\n", err);
		exits("FAIL");
	}

	eps = ldexp(1, -1074);
	neps = ldexp(-1, -1074);

	fprint(2, "Smallest Positive Double: %g\n", eps);
	decomp(eps, &exp, DEEPS);

	fprint(2, "Largest Negative Double: %g\n", neps);
	decomp(neps, &exp, DENEPS);

	fprint(2, "Positive infinity: %g\n", Inf(1));
	decomp(Inf(1), &exp, DEPINF);

	fprint(2, "NaN: %g\n", NaN());
	decomp(NaN(), &exp, DENAN);

	if(fail){
		print("FAIL\n");
		exits("FAIL");
	}
	print("PASS\n");
	exits(nil);
}
