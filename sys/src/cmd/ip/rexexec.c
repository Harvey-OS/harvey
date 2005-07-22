#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

/*
 * called by listen as rexexec rexexec net dir ...
 */
void
main(int argc, char **argv)
{
	char buf[8192];
	int n, nn;
	AuthInfo *ai;

	ARGBEGIN{
	}ARGEND;

	ai = auth_proxy(0, auth_getkey, "proto=p9any role=server");
	if(ai == nil)
		sysfatal("auth_proxy: %r");
	if(strcmp(ai->cuid, "none") == 0)
		sysfatal("rexexec by none disallowed");
	if(auth_chuid(ai, nil) < 0)
		sysfatal("auth_chuid: %r");

	n = 0;
	do {
		nn = read(0, buf+n, 1);
		if(nn <= 0)
			sysfatal("can't read command");
		n += nn;
		if(n == sizeof buf)
			buf[n-1] = '\0';
	} while (buf[n-1] != '\0');

	putenv("service", "rx");
	execl("/bin/rc", "rc", "-lc", buf, nil);
	sysfatal("can't exec rc");
}
