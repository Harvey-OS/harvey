#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

void	error(char*);

/*
 * called by listen as rexexec rexexec net dir ...
 */
void
main(void)
{
	char *err, user[NAMELEN], buf[8192];
	int n;

	if(err = srvauth(user))
		error(err);
	if(err = newns(user, 0))
		error(err);
	n = read(0, buf, sizeof buf - 1);
	if(n < 0)
		error("can't read command");
	buf[n] = '\0';
	putenv("service", "rx");
	execl("/bin/rc", "rc", "-lc", buf, 0);
	error("can't exec rc");
}

void
error(char *msg)
{
	fprint(2, "rexexec: %s: %r\n", msg);
	exits(msg);
}
