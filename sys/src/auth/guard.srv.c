#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <auth.h>
#include "authsrv.h"

/*
 * c -> a	client
 * a -> c	challenge prompt
 * c -> a	KC'{challenge}
 * a -> c	OK or NO
 */

void	catchalarm(void*, char*);

char	user[NAMELEN];

void
main(int argc, char *argv[])
{
	char *ukey, resp[32], buf[NETCHLEN];
	long chal;
	int n;

	USED(argc, argv);
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
		syslog(0, AUTHLOG, "error: %r replying to server");
		exits("replying to server");
	}
	alarm(3*60*1000);
	if(readarg(0, resp, sizeof resp) < 0)
		fail(0);
	alarm(0);

	ukey = findnetkey(user);
	if(!ukey){
		write(1, "NO", 2);
		syslog(0, AUTHLOG, "user not found");
		fail(0);
	}
	if(!netcheck(ukey, chal, resp)){
		ukey = findkey(user);
		if(!ukey || !netcheck(ukey, chal, resp)){
			write(1, "NO", 2);
			syslog(0, AUTHLOG, "user %s: bad response", user);
			fail(user);
		}
	}
	write(1, "OK", 2);
	syslog(0, AUTHLOG, "user %s ok", user);
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
