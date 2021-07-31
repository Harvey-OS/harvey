#include <alef.h>
#include "ps.h"
aggr  pair { ps l, r; };

void
main()
{
	rat a, b;
	pair OO;

	a = ratmk(3, -24);
	b = ratmk(3, 8);
	print("\nexpect 1/4: ");
	ratprint(ratadd(a,b));
	print("\nexpect 0: ");
	ratprint(ratsub(a,a));
	print("\nexpect 64/3: ");
	ratprint(ratinv(ratmul(a, ratneg(b))));
	print("\nexpect 1..1: ");
	psprintn(Ones(), 20);
	print("\nexpect 1 1 1/2 1/3..1/19: ");
	psprintn(psint(Ones(), ratmk(1,1)), 20);
	print("\nexpect 0 1..1: ");
	psprintn(psxmul(Ones()), 20);
	print("\nexpect 0 0 0 1/2 1/2 ...: ");
	psprintn(pssca((1,2), 3, Ones()), 20);
	print("\nexpect -2 -4..-40: ");
	psprintn(pscmul(ratmk(-2,1), psdif(Ones())), 20);
	print("\nexpect 1 1 0..0: ");
	psprintn(pssub(Ones(), psxmul(psxmul(Ones()))), 20);
	print("\nexpect 1 1/1! 1/2!..1/9!: ");
	psprintn(psexp(psxmul(pssub(Ones(),psxmul(Ones())))),10);
	OO = psdup(psdif(Ones()));
	print("\nexpect -1..-20: ");
	psprintn(psadd(OO.l, pscmul(ratmk(-2,1),OO.r)), 20);
	print("\nexpect 1..20: ");
	psprintn(psmul(Ones(), Ones()), 20);
	print("\nexpect 1 -1 0..0: ");
	psprintn(psinv(Ones()), 20);
	print("\nexpect 1 1 2 4 8.. ");
	psprintn(pscom(Ones(),psxmul(Ones())), 20);
	print("\nexpect tan(x): ");
	psprintn(psrev(psint(pscom(Ones(),pscmul((-1,1),psmon(2))),(0,0))), 20);
	print("\nexpect 0 1 -1 1 -1.. ");
	psprintn(psrev(psxmul(Ones())), 20);
	print("\n");
	exits("");
}
