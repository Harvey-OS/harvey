#include "agent.h"

static int
_confirm(char *what, char *keyname)
{
	/* try for a new acme window (can't) */

	/* try for a new rio window */
	
	/* try to print a message to /dev/cons */

	/* give up */
}

int
confirm(char *what, char *keyname)
{
	int rv;

	rv = _confirm(what, keyname);

	/* log result */

	return rv;
}
