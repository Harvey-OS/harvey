#include <u.h>
#include <libc.h>

void
reboot(void)
{
	int fd;
	fd = open("/dev/reboot", OWRITE);
	if(fd >= 0)
		write(fd, "reboot", 6);
	exits(0);
}

char*
readenv(char *name, char *buf, int n)
{
	char *ans;
	int f;
	char ename[200];

	ans = buf;
	ename[0] = 0;
	strcat(ename, "/env/");
	strcat(ename, name);
	f = open(ename, OREAD);
	if(f < 0)
		return 0;
	n = read(f, ans, n-1);
	if(n < 0)
		ans = 0;
	else
		ans[n] = 0;
	close(f);
	return ans;
}

int alarmed;

void
ding(void*, char*msg)
{
	if(strstr(msg, "alarm")){
		alarmed = 1;
		noted(NCONT);
	}
	noted(NDFLT);
}

void
main(int argc, char **argv)
{
	int fd, logfd, pfxlen;
	char buf[256], file[128], mylog[128];
	char *p;
	Dir *d;

	if(argc > 1)
		strecpy(file, file+sizeof file, argv[1]);
	else{
		p = readenv("cputype", buf, sizeof buf);
		if(p == 0)
			exits(0);
		file[0] = 0;
		strcat(file, "/");
		strcat(file, p);
		strcat(file, "/lib");
	}
	if (access(file, AREAD) < 0)
		sysfatal("%s not readable: %r", file);

	logfd = -1;
	strncpy(mylog, "/sys/log/up/", sizeof mylog);
	pfxlen = strlen(mylog);
	p = readenv("sysname", mylog + pfxlen, sizeof mylog - pfxlen);

	switch(rfork(RFPROC|RFNOWAIT|RFNOTEG|RFCFDG)){
	case 0:
		break;
	default:
		exits(0);
	}

	notify(ding);
	fd = open(file, OREAD);
	if (fd < 0)
		exits("no file");
	if (p)
		logfd = create(mylog, AWRITE, 0666);

	//  the logic here is to make a request every 5 minutes.
	//  If the request alarms out, that's OK, the file server
	//  may just be busy.  If the request fails for any other
	//  reason, it's probably because the connection went
	//  away so reboot.
	for(;;){
		alarm(1000*60);
		alarmed = 0;

		d = dirfstat(fd);
		free(d);
		if(d == nil)
			if(!alarmed)
				reboot();
		alarm(0);

		if (logfd >= 0)
			pwrite(logfd, "\n", 1, 0);  /* evidence of running */
		sleep(60*1000*5);
	}
}
