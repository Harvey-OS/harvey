#include "lib9.h"
#include "auth.h"


int
authdial(void)
{
	char na[256];

	netmkaddr(na, authaddr, "tcp", "567");

	return dial(na, 0, 0, 0);
}
