#include <u.h>
#include <libc.h>
#include "String.h"

	/*
	 * this is only called on memory allocation failures.
	 * it is stored here, to allow programs (e.g. smtpd)
	 * to override the default action.
	 */
void
s_error(char *f, char *status)
{
	
	perror(f);
	exits(status);
}
