#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

void	error(char*, int);

/*
 * called by listen as rexexec rexexec net dir ...
 */
void
main(void)
{
	char *err, user[NAMELEN], buf[8192];
	int n;

	if(err = srvauth(user))
		error(err, 1);
	if(err = newns(user, 0))
		error(err, 1);
	n = read(0, buf, sizeof buf - 1);
	if(n < 0)
		error("can't read command", 1);
	buf[n] = '\0';
	putenv("service", "rx");
	execl("/bin/rc", "rc", "-lc", buf, 0);
	error("can't exec rc", 1);
}

void
error(char *msg, int syserr)
{
	if(syserr)
		fprint(2, "rexexec: %s: %r\n", msg);
	else
		fprint(2, "rexexec: %s\n", msg);
	exits(msg);
}
