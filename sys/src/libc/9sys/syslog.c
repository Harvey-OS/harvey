#include <u.h>
#include <libc.h>

static struct
{
	int	fd;
	int	consfd;
	char	name[NAMELEN];
	Dir	d;
	Dir	consd;
	Lock;
} sl =
{
	-1, -1,
};

static void
_syslogopen(void)
{
	char buf[1024];

	if(sl.fd >= 0)
		close(sl.fd);
	snprint(buf, sizeof(buf), "/sys/log/%s", sl.name);
	sl.fd = open(buf, OWRITE|OCEXEC);
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
	char *ctim, *p;
	va_list arg;
	int n;
	Dir d;
	char err[ERRLEN];

	errstr(err);
	lock(&sl);

	/*
	 *  paranoia makes us stat to make sure a fork+close
	 *  hasn't broken our fd's
	 */
	if(sl.fd < 0
	   || strcmp(sl.name, logname)!=0
	   || dirfstat(sl.fd, &d) < 0
	   || d.dev != sl.d.dev
	   || d.type != sl.d.type
	   || d.qid.path != sl.d.qid.path){
		strncpy(sl.name, logname, NAMELEN-1);
		_syslogopen();
		if(sl.fd < 0)
			cons = 1;
		sl.d = d;
	}
	if(cons)
		if(sl.consfd < 0
		   || dirfstat(sl.consfd, &d) < 0
		   || d.dev != sl.consd.dev
		   || d.type != sl.consd.type
		   || d.qid.path != sl.consd.qid.path){
			sl.consfd = open("#c/cons", OWRITE|OCEXEC);
			sl.consd = d;
		}

	if(fmt == nil){
		unlock(&sl);
		return;
	}

	ctim = ctime(time(0));
	werrstr(err);
	p = buf + snprint(buf, sizeof(buf)-1, "%s ", sysname());
	strncpy(p, ctim+4, 15);
	p += 15;
	*p++ = ' ';
	va_start(arg, fmt);
	p = doprint(p, buf+sizeof(buf)-1, fmt, arg);
	va_end(arg);
	*p++ = '\n';
	n = p - buf;

	if(sl.fd >= 0){
		seek(sl.fd, 0, 2);
		write(sl.fd, buf, n);
	}

	if(cons && sl.consfd >=0)
		write(sl.consfd, buf, n);

	unlock(&sl);
}
