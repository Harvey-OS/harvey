#include <u.h>
#include <libc.h>

/*
 * Print
 *  sysname: time: user: [prog: ]msg[: errmsg]
 * on /sys/log/logname, creating it (append only) if it doesn't exist.
 * Print the prog if argv0 is not 0.
 * If cons or log file can't be opened, print on the system console, too.
 */
void
syslog(int cons, char *logname, char *fmt, ...)
{
	char buf[1024];
	char *ctim, *p, *ebuf, *t;
	int f, n;
	static char user[NAMELEN];
	static char name[NAMELEN];
	static char sysname[NAMELEN];
	static int logfd = -1;
	static int consfd = -1;

	if(logfd<0 || strcmp(name, logname)!=0){
		strncpy(name, logname, NAMELEN-1);
		if(logfd >= 0)
			close(logfd);
		doprint(buf, buf+sizeof(buf), "/sys/log/%s", &logname);
		logfd = open(buf, OWRITE);
		if(logfd < 0)
			cons = 1;
	}
	if(cons && consfd<0)
		consfd = open("#c/cons", OWRITE);
	if(sysname[0] == 0){
		strcpy(sysname, "gnot");
		f = open("/env/sysname", OREAD);
		if(f >= 0){
			read(f, sysname, NAMELEN-1);
			close(f);
		}
	}
	if(user[0] == 0){
		f = open("/dev/user", OREAD);
		if(f >= 0){
			if(read(f, user, NAMELEN-1) <= 0)
				strcpy(user, "ditzel");
			close(f);
		}
	}
	ctim = ctime(time(0));
	ebuf = buf+sizeof(buf)-1; /* leave room for newline */
	t = sysname;
	p = doprint(buf, ebuf, "%s: ", &t);
	strncpy(p, ctim+4, 15);
	p += 15;
	t = user;
	if(user[0])
		p = doprint(p, ebuf, ": %s: ", &t);
	if(argv0)
		p = doprint(p, ebuf, "%s: ", &argv0);
	p = doprint(p, ebuf, fmt, (&fmt+1));
	*p++ = '\n';
	n = p - buf;
	if(logfd >= 0){
		seek(logfd, 0, 2);	/* in case it's not append-only */
		write(logfd, buf, n);
	}
	if(cons && consfd >=0)
		write(consfd, buf, n);
}
