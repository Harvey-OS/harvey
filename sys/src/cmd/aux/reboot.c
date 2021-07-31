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
	int fd;
	char buf[DIRLEN];
	char file[128];
	char *p;

	switch(rfork(RFPROC|RFNOWAIT|RFNOTEG|RFCFDG)){
	case 0:
		break;
	default:
		exits(0);
	}

	notify(ding);
	if(argc > 1)
		strcpy(file, argv[1]);
	else{
		p = readenv("cputype", buf, sizeof(buf));
		if(p == 0)
			exits(0);
		file[0] = 0;
		strcat(file, "/");
		strcat(file, p);
		strcat(file, "/init");
	}

	fd = open(file, OREAD);

	//  the logic here is to make a request every 5 minutes.
	//  If the request alarms out, that's OK, the file server
	//  may just be busy.  If the request fails for any other
	//  reason, it's probably because the connection went
	//  away so reboot.
	for(;;){
		alarm(1000*60*5);
		alarmed = 0;
		if(fstat(fd, buf) < 0)
			if(!alarmed)
				reboot();
		alarm(0);
		sleep(60*1000);
	}
}
