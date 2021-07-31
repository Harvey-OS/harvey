#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Plan9		= 1,
	Securenet	= 2,
};

Tm	getdate(char*);
int	setexpire(char*, char*, char*, char*);
void	usage(void);

void
usage(void)
{
	fprint(2, "usage: expire [-pn] user yyyymmdd|never\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Tm date;
	char *u, expire[32];
	ulong time;
	int change, which;

	change = 0;
	which = 0;
	ARGBEGIN{
	case 'p':
		which |= Plan9;
		break;
	case 'n':
		which |= Securenet;
		break;
	default:
		usage();
	}ARGEND
	argv0 = "expire";

	if(argc != 2)
		usage();
	u = argv[0];
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");
	time = 0;
	if(strcmp(argv[1], "never") == 0){
		strcpy(expire, "never");
	}else{
		date = getdate(argv[1]);
		time = tm2sec(date);
		if(time == 0)
			strcpy(expire, "never");
		else
			sprint(expire, "%lud", time);
	}

	if(!which)
		which |= Plan9 | Securenet;
	if(which & Plan9)
		change += setexpire(KEYDB, u, expire, "Plan 9");
	if(which & Securenet)
		change += setexpire(NETKEYDB, u, expire, "SecureNet");
	if(!change){
		fprint(2, "couldn't set an expiration date for %s\n", u);
		exits("expire failed");
	}
	if(strcmp(expire, "never") == 0)
		print("user %s will never expire\n");
	else
		print("user %s will expire at %s: %s", u, expire, ctime(time));
	syslog(0, AUTHLOG, "user %s: expire at %s", u, expire);
	exits(0);
}

/*
 * get the date in the format yyyymmdd
 */
Tm
getdate(char *d)
{
	Tm date;
	int i;

	date.year = date.mon = date.mday = 0;
	date.hour = date.min = date.sec = 0;
	for(i = 0; i < 8; i++)
		if(!isdigit(d[i]))
			return date;
	date.year = (d[0]-'0')*1000 + (d[1]-'0')*100 + (d[2]-'0')*10 + d[3]-'0';
	date.year -= 1900;
	d += 4;
	date.mon = (d[0]-'0')*10 + d[1]-'0';
	d += 2;
	date.mday = (d[0]-'0')*10 + d[1]-'0';
	return date;
}

int
setexpire(char *db, char *u, char *expire, char *where)
{
	char buf[sizeof KEYDB+sizeof NETKEYDB+2*NAMELEN+6];
	int fd;

	sprint(buf, "%s/%s/expire", db, u);
	fd = open(buf, OWRITE);
	if(fd < 0 || write(fd, expire, strlen(expire)) != strlen(expire)){
		print("can't set %s expiration date\n", where);
		return 0;
	}
	close(fd);
	return 1;
}
