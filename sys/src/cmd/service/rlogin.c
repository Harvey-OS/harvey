#include <u.h>
#include <libc.h>

void	getstr(int, char*, int);

void
main(int argc, char **argv)
{
	char luser[128];
	char ruser[128];
	char term[128];
	char err[128];

	USED(argc, argv);

	getstr(0, err, sizeof(err));
	getstr(0, ruser, sizeof(ruser));
	getstr(0, luser, sizeof(luser));
	getstr(0, term, sizeof(term));
	write(0, "", 1);

	if(luser[0] == '\0')
		strncpy(luser, ruser, NAMELEN-1);
	luser[NAMELEN-1] = '\0';
	execl("/bin/aux/telnetd", "telnetd", "-n", "-u", luser, 0);
	fprint(2, "can't exec con service: %r\n");
	exits("can't exec");
}

void
getstr(int fd, char *str, int len)
{
	char c;
	int n;

	while(len) {
		n = read(fd, &c, 1);
		if(n < 0)
			return;
		if(n == 0)
			continue;
		*str++ = c;
		if(c == 0)
			break;
		len--;
	}
}
