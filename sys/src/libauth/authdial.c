#include <u.h>
#include <libc.h>
#include <auth.h>

int
authdial(char *service)
{
	return dial(netmkaddr("$auth", 0, service), 0, 0, 0);
}
