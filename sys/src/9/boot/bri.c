#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

void
configbri(Method *mp)
{
	int devno = 0;
	int fd, n, chan;
	int pfd[2];
	char dbuf[8], sbuf[8], rbuf[8];
	char buf[128], file[3*NAMELEN];
	char *p;

	fd = connectlocal();
	if(fd < 0)
		fatal("no local file system");
	if(mount(fd, "/", MAFTER|MCREATE, "", "") < 0)
		fatal("can't mount kfs");
	close(fd);

	if(pipe(pfd)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		sprint(dbuf, "%d", devno);
		sprint(sbuf, "%d", pfd[0]);
		sprint(rbuf, "%d", pfd[1]);
		execl("/68020/bin/isdn/briserver", "briserver",
			"-d", dbuf, "-s", sbuf, rbuf, 0);
		fatal("can't exec briserver");
	default:
		break;
	}
	close(pfd[1]);
	if(mount(pfd[0], "/dev", MAFTER, "", "") < 0)
		fatal("can't mount /dev/bri0");
	close(pfd[0]);

	sprint(file, "/dev/bri%d/dial", devno);
	do{
		fd = open(file, ORDWR);
		if(fd < 0)
			fatal(file);
		memset(buf, 0, sizeof(buf));
		outin("Number please ", buf, sizeof(buf));
		fprint(fd, "c/0 %s\n", buf);
		n = read(fd, buf, sizeof buf-1);
		close(fd);
		if(n <= 0)
			fatal("EOF from dialer");
		buf[n] = 0;
		write(1, buf, n);
	}while(strncmp(buf, "connected", 9) != 0);
	p = strchr(buf, '/');
	if(p == 0)
		fatal("bad reply from bri server");
	else
		p++;
	chan = strtoul(p, 0, 0);
	if(chan < 1 || chan > 2)
		fatal("bad channel from bri server");

	sprint(file, "#H%d/ctl", 2*devno+chan-1);
	fd = open(file, ORDWR);
	if(fd < 0)
		fatal(file);
	sendmsg(fd, "hdlc on");
	sendmsg(fd, "recven");
	sendmsg(fd, "push dkmux");
	sendmsg(fd, mp->arg);
	close(fd);
}

int
authbri(void)
{
	return dkauth();
}

int
connectbri(void)
{
	return dkconnect();
}
