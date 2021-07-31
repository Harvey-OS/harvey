#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <auth.h>
#include "authsrv.h"

char	user[NAMELEN];
void	catchalarm(void*, char*);

/*
 * c -> a	client, host, KH{RXschal, hchal}
 * a -> c	challenge
 * c -> a	KC'{challenge}
 * a -> c	KH{RXstick, hchal, client, KC}
 */
void
main(int argc, char *argv[])
{
	char uandh[2*NAMELEN+AUTHLEN], *user, *host, *ukey, *unkey, *hkey, hchal[AUTHLEN];
	char resp[32], tick[AUTHLEN+NAMELEN+DESKEYLEN], buf[NETCHLEN];
	long chal;
	int i, n;

	USED(argc, argv);
	argv0 = "chal";
	srand(getpid()*time(0));
	notify(catchalarm);

	/*
	 * read the host and client and get their keys
	 */
	if(read(0, uandh, sizeof uandh) != sizeof uandh){
		syslog(0, AUTHLOG, "can't read user name");
		fail(0);
	}
	user = uandh;
	host = uandh+NAMELEN;
	user[NAMELEN-1] = host[NAMELEN-1] = '\0';

	/*
	 * challenge-response
	 */
	chal = lnrand(MAXNETCHAL);
	sprint(buf, "%lud", chal);
	n = strlen(buf) + 1;
	if(write(1, buf, n) != n){
		syslog(0, AUTHLOG, "error: %r replying to server");
		exits("error replying");
	}

	alarm(3*60*1000);
	if(readarg(0, resp, sizeof resp) < 0)
		fail(0);
	alarm(0);

	ukey = findkey(user);
	unkey = findnetkey(user);
	if(!ukey){
		syslog(0, AUTHLOG, "user not found");
		fail(0);
	}
	if((!unkey || !netcheck(unkey, chal, resp)) && !netcheck(ukey, chal, resp)){
		syslog(0, AUTHLOG, "bad user response");
		write(1, "NO", 2);
		fail(user);
	}
	hkey = findkey(host);
	if(!hkey || !ishost(host)){
		syslog(0, AUTHLOG, "host not found");
		fail(0);
	}
	memmove(hchal, uandh+2*NAMELEN, AUTHLEN);
	decrypt(hkey, hchal, AUTHLEN);
	if(hchal[0] != RXschal){
		syslog(0, AUTHLOG, "bad host challenge");
		fail(0);
	}
	hchal[0] = RXstick;

	syslog(0, AUTHLOG, "user %s authenticated on %s", user, host);
	write(1, "OK", 2);
	succeed(user);

	/*
	 * write back the ticket
	 */
	i = 0;
	memmove(&tick[i], hchal, AUTHLEN);
	i += AUTHLEN;
	strncpy(&tick[i], user, NAMELEN);
	i += NAMELEN;
	memmove(&tick[i], ukey, DESKEYLEN);
	i += DESKEYLEN;
	encrypt(hkey, tick, i);
	if(write(1, tick, i) != i){
		syslog(0, AUTHLOG, "error: %r replying to server");
		exits("error replying");
	}
	exits(0);
}

void
catchalarm(void *x, char *msg)
{
	USED(x, msg);
	syslog(0, AUTHLOG, "user response timed out");
	fail(0);
}
