#include <u.h>
#include <libc.h>

static struct
{
	int	fd;
	int	consfd;
	char	*name;
	Dir	*d;
	Dir	*consd;
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

static int
eqdirdev(Dir *a, Dir *b)
{
	return a != nil && b != nil &&
		a->dev == b->dev && a->type == b->type &&
		a->qid.path == b->qid.path;
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
	Dir *d;
	char err[ERRMAX];

	err[0] = '\0';
	errstr(err, sizeof err);
	lock(&sl);

	/*
	 *  paranoia makes us stat to make sure a fork+close
	 *  hasn't broken our fd's
	 */
	d = dirfstat(sl.fd);
	if(sl.fd < 0 || sl.name == nil || strcmp(sl.name, logname) != 0 ||
	   !eqdirdev(d, sl.d)){
		free(sl.name);
		sl.name = strdup(logname);
		if(sl.name == nil)
			cons = 1;
		else{
			free(sl.d);
			sl.d = nil;
			_syslogopen();
			if(sl.fd < 0)
				cons = 1;
			else
				sl.d = dirfstat(sl.fd);
		}
	}
	free(d);
	if(cons){
		d = dirfstat(sl.consfd);
		if(sl.consfd < 0 || !eqdirdev(d, sl.consd)){
			free(sl.consd);
			sl.consd = nil;
			sl.consfd = open("#c/cons", OWRITE|OCEXEC);
			if(sl.consfd >= 0)
				sl.consd = dirfstat(sl.consfd);
		}
		free(d);
	}

	if(fmt == nil){
		unlock(&sl);
		return;
	}

	ctim = ctime(time(0));
	p = buf + snprint(buf, sizeof(buf)-1, "%s ", sysname());
	strncpy(p, ctim+4, 15);
	p += 15;
	*p++ = ' ';
	errstr(err, sizeof err);
	va_start(arg, fmt);
	p = vseprint(p, buf+sizeof(buf)-1, fmt, arg);
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
