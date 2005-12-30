#include <u.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <pwd.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <authsrv.h>
#include <libsec.h>
#include "drawterm.h"

#undef socket
#undef connect
#undef getenv
#undef access

char*
getuser(void)
{
	static char user[64];
	struct passwd *pw;

	pw = getpwuid(getuid());
	if(pw == nil)
		return "none";
	strecpy(user, user+sizeof user, pw->pw_name);
	return user;
}
/*
 * Absent other hints, it works reasonably well to use
 * the X11 display name as the name space identifier.
 * This is how sam's B has worked since the early days.
 * Since most programs using name spaces are also using X,
 * this still seems reasonable.  Terminal-only sessions
 * can set $NAMESPACE.
 */
static char*
nsfromdisplay(void)
{
	char *disp, *p;

	if((disp = getenv("DISPLAY")) == nil){
		werrstr("$DISPLAY not set");
		return nil;
	}

	/* canonicalize: xxx:0.0 => xxx:0 */
	p = strrchr(disp, ':');
	if(p){
		p++;
		while(isdigit((uchar)*p))
			p++;
		if(strcmp(p, ".0") == 0)
			*p = 0;
	}

	return smprint("/tmp/ns.%s.%s", getuser(), disp);
}

char*
getns(void)
{
	char *ns;

	ns = getenv("NAMESPACE");
	if(ns == nil)
		ns = nsfromdisplay();
	if(ns == nil){
		werrstr("$NAMESPACE not set, %r");
		return nil;
	}
	return ns;
}

int
dialfactotum(void)
{
	int fd;
	struct sockaddr_un su;
	char *name;
	
	name = smprint("%s/factotum", getns());

	if(name == nil || access(name, 0) < 0)
		return -1;
	memset(&su, 0, sizeof su);
	su.sun_family = AF_UNIX;
	if(strlen(name)+1 > sizeof su.sun_path){
		werrstr("socket name too long");
		return -1;
	}
	strcpy(su.sun_path, name);
	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		werrstr("socket: %r");
		return -1;
	}
	if(connect(fd, (struct sockaddr*)&su, sizeof su) < 0){
		werrstr("connect %s: %r", name);
		close(fd);
		return -1;
	}

	return lfdfd(fd);
}

