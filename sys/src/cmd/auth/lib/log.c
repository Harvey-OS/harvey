#include <u.h>
#include <libc.h>
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"

static void
record(char *db, char *user, char *msg)
{
	char buf[Maxpath];
	int fd;

	snprint(buf, sizeof buf, "%s/%s/log", db, user);
	fd = open(buf, OWRITE);
	if(fd < 0)
		return;
	write(fd, msg, strlen(msg));
	close(fd);
	return;
}

void
logfail(char *user)
{
	if(!user)
		return;
	record(KEYDB, user, "bad");
	record(NETKEYDB, user, "bad");
}

void
succeed(char *user)
{
	if(!user)
		return;
	record(KEYDB, user, "good");
	record(NETKEYDB, user, "good");
}

void
fail(char *user)
{
	logfail(user);
	exits("failure");
}
