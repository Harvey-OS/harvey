#include <alef.h>

intern byte	syslogname[NAMELEN];
intern byte	sysname[NAMELEN];
intern int		logfd = -1;
intern int		consfd = -1;
intern QLock	sysloglock;

/*
 * Print
 *  sysname: time: mesg
 * on /sys/log/logname.
 * If cons or log file can't be opened, print on the system console, too.
 */
void
syslog(int cons, byte *logname, byte *fmt, ...)
{
	byte buf[1024];
	byte *ctim, *p, *ebuf, *t;
	int f, n;

	sysloglock.lock();
	if(logfd<0 || strcmp(syslogname, logname)!=0){
		strncpy(syslogname, logname, NAMELEN-1);
		if(logfd >= 0)
			close(logfd);
		snprint(buf, sizeof(buf), "/sys/log/%s", logname);
		logfd = open(buf, OWRITE|OCEXEC);
		if(logfd < 0)
			cons = 1;
	}
	if(cons && consfd<0)
		consfd = open("#c/cons", OWRITE|OCEXEC);
	if(sysname[0] == 0){
		strcpy(sysname, "gnot");
		f = open("/env/sysname", OREAD);
		if(f >= 0){
			if(read(f, sysname, NAMELEN-1) <= 0)
				strcpy(sysname, "gnot");
			close(f);
		}
	}
	ctim = ctime(time());
	if(fmt != nil){
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
			seek(logfd, 0, 2);	/* in case it's not append-only */
			write(logfd, buf, n);
		}
		if(cons && consfd >=0)
			write(consfd, buf, n);
	}
	sysloglock.unlock();
}
