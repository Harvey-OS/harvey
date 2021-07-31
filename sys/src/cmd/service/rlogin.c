#include <u.h>
#include <libc.h>

void	getstr(int, char*, int);

void
main(int argc, char **argv)
{
	char luser[128], ruser[128], term[128], err[128];
	int trusted;

	trusted = 0;
	ARGBEGIN{
	case 't':
		trusted = 1;
		break;
	}ARGEND

	getstr(0, err, sizeof(err));
	getstr(0, ruser, sizeof(ruser));
	getstr(0, luser, sizeof(luser));
	getstr(0, term, sizeof(term));
	write(0, "", 1);

	if(luser[0] == '\0')
		strncpy(luser, ruser, NAMELEN-1);
	luser[NAMELEN-1] = '\0';
	syslog(0, "telnet", "rlogind %s", luser);
	if(trusted)
		execl("/bin/aux/telnetd", "telnetd", "-n", "-t", 0);
	else
		execl("/bin/aux/telnetd", "telnetd", "-n", "-u", luser, 0);
	fprint(2, "can't exec con service: %r\n");
	exits("can't exec");
}

void
getstr(int fd, char *str, int len)
{
	char c;
	int n;

	while(--len > 0){
		n = read(fd, &c, 1);
		if(n < 0)
			return;
		if(n == 0)
			continue;
		*str++ = c;
		if(c == 0)
			break;
	}
	*str = '\0';
}
