#include <u.h>
#include <libc.h>

static int
_syslogopen(char *logname, int logfd)
{
	char buf[1024];

	if(logfd >= 0)
		close(logfd);
	snprint(buf, sizeof(buf), "/sys/log/%s", logname);
	return open(buf, OWRITE);
}

/*
 * Print
 *  sysname: time: mesg
 * on /sys/log/logname.
 * If cons or log file can't be opened, print on the system console, too.
 */
void
syslog(int cons, char *logname, char *fmt, ...)
{
	char buf[1024];
	char *ctim, *p, *ebuf, *t;
	int f, n;
	static char name[NAMELEN];
	static char sysname[NAMELEN];
	static int logfd = -1;
	static int consfd = -1;

	if(logfd<0 || strcmp(name, logname)!=0){
		strncpy(name, logname, NAMELEN-1);
		logfd = _syslogopen(logname, logfd);
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
	ctim = ctime(time(0));
	ebuf = buf+sizeof(buf)-1; /* leave room for newline */
	t = sysname;
	p = doprint(buf, ebuf, "%s ", &t);
	strncpy(p, ctim+4, 12);
	p += 12;
	*p++ = ' ';
	p = doprint(p, ebuf, fmt, (&fmt+1));
	*p++ = '\n';
	n = p - buf;
	if(logfd >= 0){
		if(seek(logfd, 0, 2) < 0 || write(logfd, buf, n) < 0){
			logfd = _syslogopen(logname, logfd);
			if(logfd >= 0){
				seek(logfd, 0, 2);
				write(logfd, buf, n);
			}
		}
	}
	if(cons && consfd >=0)
		write(consfd, buf, n);
}
