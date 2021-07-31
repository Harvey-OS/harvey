#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

void	error(char*);

int nonone = 1;

/*
 * called by listen as rexexec rexexec net dir ...
 */
void
main(int argc, char **argv)
{
	char user[NAMELEN], buf[8192];
	int n, nn, nonone = 0;

	ARGBEGIN{
	case 'a':
		nonone = 0;
	}ARGEND;

	if(srvauth(0, user) < 0)
		error("srvauth");
	if(nonone && strcmp(user, "none") == 0)
		error("rexexec by none disallowed");
	if(newns(user, 0) < 0)
		error("newns");
	n = 0;
	do {
		nn = read(0, buf+n, sizeof(buf) - n);
		if(nn <= 0)
			error("can't read command");
		n += nn;
		if(n == sizeof buf)
			buf[n-1] = '\0';
	} while (buf[n-1] != '\0');

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
