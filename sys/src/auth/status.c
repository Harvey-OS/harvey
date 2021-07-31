#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <auth.h>
#include "authsrv.h"

int	check(char*, char*, char*, char*, char*);
void	printstatus(char*, char*, char*, char*);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *u, status[32], expire[32];

	ARGBEGIN{
	}ARGEND
	argv0 = "status";

	if(argc != 1)
		usage();
	u = argv[0];
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");
	if(check(KEYDB, u, status, expire, "Plan 9"))
		printstatus(u, status, expire, "Plan 9");
	else
		print("user %s has no Plan 9 account\n");
	if(check(NETKEYDB, u, status, expire, "SecureNet"))
		printstatus(u, status, expire, "SecureNet");
	else
		print("user %s has no SecureNet account\n");
	exits(0);
}

void
printstatus(char *u, char *status, char *expire, char *where)
{
	long extime, now;

	print("%s account for %s ", where, u);
	if(strcmp(status, "disabled\n") == 0){
		print("disabled\n");
		return;
	}
	if(strcmp(expire, "unknown") == 0){
		print("enabled; expiration date unknown\n");
		return;
	}
	if(strcmp(expire, "never\n") == 0){
		print("enabled and will never expire\n");
		return;
	}
	extime = strtol(expire, 0, 10);
	now = time(0);
	if(extime <= now){
		print("expired %s", ctime(extime));
		return;
	}
	print("enabled and will expire %s", ctime(extime));
}

int
check(char *db, char *u, char *status, char *expire, char *where)
{
	char buf[sizeof KEYDB+sizeof NETKEYDB+2*NAMELEN+6];
	int fd, n;

	strcpy(status, "unknown");
	strcpy(expire, "unknown");
	sprint(buf, "%s/%s/status", db, u);
	fd = open(buf, OREAD);
	if(fd < 0)
		return 0;
	n = read(fd, status, 32);
	if(n < 0)
		fprint(2, "status: can't read %s status\n", where);
	else
		status[n] = '\0';
	close(fd);

	sprint(buf, "%s/%s/expire", db, u);
	fd = open(buf, OREAD);
	if(fd < 0){
		fprint(2, "status: can't open %s expiration date\n", where);
		return 1;
	}
	n = read(fd, expire, 32);
	if(n < 0)
		fprint(2, "can't read %s expiration\n", where);
	else
		expire[n] = '\0';
	close(fd);
	return 1;
}

void
usage(void)
{
	fprint(2, "usage: status user\n");
	exits("usage");
}
