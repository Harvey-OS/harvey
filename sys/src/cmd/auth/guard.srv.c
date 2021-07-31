#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "authsrv.h"

/*
 * c -> a	client
 * a -> c	challenge prompt
 * c -> a	KC'{challenge}
 * a -> c	OK or NO
 */

void	catchalarm(void*, char*);
void	getraddr(char*);

char	user[NAMELEN];
char	raddr[128];

void
main(int argc, char *argv[])
{
	char ukey[DESKEYLEN], resp[32], buf[NETCHLEN];
	long chal;
	int n;

	ARGBEGIN{
	}ARGEND;

	strcpy(raddr, "unknown");
	if(argc >= 1)
		getraddr(argv[argc-1]);

	argv0 = "guard";
	srand(getpid()*time(0));
	notify(catchalarm);

	/*
	 * read the host and client and get their keys
	 */
	if(readarg(0, user, sizeof user) < 0)
		fail(0);

	/*
	 * challenge-response
	 */
	chal = lnrand(MAXNETCHAL);
	sprint(buf, "challenge: %lud\nresponse: ", chal);
	n = strlen(buf) + 1;
	if(write(1, buf, n) != n){
		syslog(0, AUTHLOG, "g-fail %r replying to server");
		exits("replying to server");
	}
	alarm(3*60*1000);
	if(readarg(0, resp, sizeof resp) < 0)
		fail(0);
	alarm(0);

	if(!findkey(NETKEYDB, user, ukey) || !netcheck(ukey, chal, resp)){
		if(!findkey(KEYDB, user, ukey) || !netcheck(ukey, chal, resp)){
			write(1, "NO", 2);
			syslog(0, AUTHLOG, "g-fail bad response %s", raddr);
			fail(user);
		}
	}
	write(1, "OK", 2);
	syslog(0, AUTHLOG, "g-ok %s %s", user, raddr);
	succeed(user);
	exits(0);
}

void
catchalarm(void *x, char *msg)
{
	USED(x, msg);
	syslog(0, AUTHLOG, "user response timed out");
	fail(0);
}

void
getraddr(char *dir)
{
	int n, fd;
	char *cp;
	char file[3*NAMELEN];

	snprint(file, sizeof(file), "%s/remote", dir);
	fd = open(file, OREAD);
	if(fd < 0)
		return;
	n = read(fd, raddr, sizeof(raddr)-1);
	close(fd);
	if(n <= 0)
		return;
	raddr[n] = 0;
	cp = strchr(raddr, '\n');
	if(cp)
		*cp = 0;
	cp = strchr(raddr, '!');
	if(cp)
		*cp = 0;
}
