#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

static void
configdkconc(char *ctl, char *arg)
{
	int cfd;
	char buf[1024], *p;
/*
	if(fork() == 0){
		cfd = open("#c/klog", OREAD);
		while((n = read(cfd, buf, sizeof buf)) > 0)
			if(write(1, buf, n) < 0)
				break;
		_exits(0);
	}
*/
	p = strchr(arg, ';');
	cfd = open(ctl, ORDWR);
	if(arg == 0 || p == 0 || cfd < 0){
		sprint(buf, "opening %s", ctl);
		fatal(buf);
	}
	*p++ = 0;
	sendmsg(cfd, "push conc");
	sendmsg(cfd, arg);
	close(cfd);
	ctl = "#Kconc/conc/0/ctl";
	cfd = open(ctl, ORDWR);
	if(cfd < 0){
		sprint(buf, "opening %s", ctl);
		fatal(buf);
	}
	sendmsg(cfd, "push dkmux");
	sendmsg(cfd, p);
	close(cfd);
}

static void
configdatakit(char *ctl, char *arg)
{
	int cfd;
	char buf[64];

	cfd = open(ctl, ORDWR);
	if(arg == 0 || cfd < 0){
		sprint(buf, "opening %s", ctl);
		fatal(buf);
	}
	sendmsg(cfd, "push dkmux");
	sendmsg(cfd, arg);
	close(cfd);
}

void
confighs(Method *mp)
{
	configdatakit("#h/ctl", mp->arg);
}

int
authhs(void)
{
	return dkauth();
}

int
connecths(void)
{
	return dkconnect();
}

void
configincon(Method *mp)
{
	configdatakit("#i/ctl", mp->arg);
}

int
authincon(void)
{
	return dkauth();
}

int
connectincon(void)
{
	return dkconnect();
}

void
configcincon(Method *mp)
{
	configdkconc("#i/ctl", mp->arg);
}

int
authcincon(void)
{
	return dkauth();
}

int
connectcincon(void)
{
	return dkconnect();
}
