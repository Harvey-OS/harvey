#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <auth.h>
#include "authsrv.h"

/*
 * temporary checking for terminal log on
 *
 * a -> c	challenge
 * c -> a	KC{challenge}, client
 * a -> c	OK or NO
 */
char	user[NAMELEN];

void
main(int argc, char *argv[])
{
	char *user, *ukey, chal[AUTHLEN], resp[AUTHLEN+NAMELEN];
	int i;

	USED(argc, argv);
	argv0 = "check";
	srand(getpid()*time(0));

	for(i=0; i<sizeof chal; i++)
		chal[i] = nrand(256);
	if(write(1, chal, sizeof chal) < 0)
		fail(0);
	if(read(0, resp, sizeof resp) != sizeof resp)
		fail(0);
	user = resp + AUTHLEN;
	user[NAMELEN-1] = '\0';

	if(strcmp(user, "none") != 0){
		ukey = findkey(user);
		if(!ukey){
			write(1, "NO", 2);
			syslog(0, AUTHLOG, "user not found");
			fail(0);
		}
		decrypt(ukey, resp, AUTHLEN);
		if(memcmp(chal, resp, AUTHLEN) != 0){
			write(1, "NO", 2);
			syslog(0, AUTHLOG, "bad user response");
			fail(user);
		}
	}
	write(1, "OK", 2);
	syslog(0, AUTHLOG, "user %s ok", user);
	succeed(user);
	exits(0);
}
