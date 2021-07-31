#include "common.h"
#include "print.h"
#include "pop3.h"
#include <pwd.h>
#include <stat.h>

uchar chal[256];

extern void
hello(void)
{
	struct stat st;

	stat("/etc/passwd", &st);
	srand(getpid()*time(0)*st.st_ino*st.st_mtime);
	sprint(chal, "<%d.%d@i.dont.really.care.com>", rand() & 0xffff, rand() & 0xffff);
	sendok("POP3 server ready %s", chal);
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
dologin(char *user)
{
	loggedin = 1;
	mailfile = s_new();
	mboxname(user, mailfile);
	if(P(s_to_c(mailfile))){
		senderr("file busy");
		exits(0);
	}
	read_mbox(s_to_c(mailfile), 0);
	return sendok("");
}

extern int
passcmd(char *arg)
{
	struct passwd *pw;
	static int tries;

	if(loggedin)
		return senderr("already authenticated");
	if(*arg == 0)
		return senderr("PASS requires argument");
	if(*user == 0)
		return senderr("USER must precede PASS");

	pw = getpwnam(user);

	if(pw == 0 || strcmp(crypt(arg, pw->pw_passwd), pw->pw_passwd) != 0){
		if(tries++ > 5){
			senderr("login failed, server exiting");
			exits(0);
		}
		return senderr("login failed");
	}

	rv = dologin(user);
	setuid(pw->pw_uid);
	return rv;
}

extern int
apopcmd(char *arg)
{
	return senderr("APOP command unimplimented");
}
