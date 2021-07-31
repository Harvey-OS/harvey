#include "common.h"
#include "print.h"
#include "pop3.h"
#include <auth.h>

Apopchalstate chs;

extern void
hello(void)
{
	if(apopchal(&chs) < 0)
		senderr("auth server not responding, try later");

	sendok("POP3 server ready %s", chs.chal);
}

extern int
usercmd(char *arg)
{
	if(loggedin)
		return senderr("already authenticated");
	if(*arg == 0)
		return senderr("USER requires argument");
	strncpy(user, arg, sizeof(user));
	user[NAMELEN-1] = 0;
	return sendok("");
}

static int
dologin(char *user, char *response)
{
	int rv;
	static int tries;

	syslog(0, "pop3", "user %s login", user);
	rv = apopreply(&chs, user, response);
	syslog(0, "pop3", "user %s response %d %s", user, rv, response);
	switch(rv){
	case 0:
		loggedin = 1;
		newns(user, 0);
		mailfile = s_new();
		mboxname(user, mailfile);
		if(P(s_to_c(mailfile))){
			senderr("file busy");
			exits(0);
		}
		read_mbox(s_to_c(mailfile), reverse);
		return sendok("");
	case -2:
		if(tries++ < 5)
			return senderr("authentication failed");
		break;
	}
	senderr("authentication failed %r, server exiting");
	exits(0);

	/* unreachable... */
	return 0;
}

extern int
passcmd(char *arg)
{
	USED(arg);
	return senderr("No passwords in the clear");
}

extern int
apopcmd(char *arg)
{
	char *resp;

	resp = nextarg(arg);
	strncpy(user, arg, sizeof(arg));
	user[NAMELEN-1] = 0;
	return dologin(arg, resp);
}

