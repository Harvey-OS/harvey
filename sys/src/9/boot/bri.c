#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

void
configbri(Method *mp)
{
	char trbuf[TICKREQLEN];
	int devno = 0;
	int fd, n, chan, i;
	int pfd[2];
	char dbuf[8], sbuf[8], rbuf[8];
	char *argv[16];
	char buf[128], file[3*NAMELEN];
	char dialstr[32];
	char *p;
	char *dbg, *dkptr;

	if(p = strchr(mp->arg, ';')){	/* assign = */
		dbg = mp->arg;
		*p++ = 0;
		dkptr = p;
	}else{
		dbg = 0;
		dkptr = mp->arg;
	}
	fd = connectlocal();
	if(fd < 0)
		fatal("no local file system");
	if(fsession(fd, trbuf) < 0)
		fatal("fsession failed on #s/kfs");
	if(mount(fd, "/", MAFTER|MCREATE, "") < 0)
		fatal("can't mount kfs");
	close(fd);

	if(pipe(pfd)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		i = 0;
		argv[i++] = "briserver";
		if(dbg){
			argv[i++] = "-D";
			argv[i++] = dbg;
		}
		argv[i++] = "-d";
		argv[i++] = dbuf;
		sprint(dbuf, "%d", devno);
		argv[i++] = "-s";
		argv[i++] = sbuf;
		sprint(sbuf, "%d", pfd[0]);
		argv[i++] = rbuf;
		sprint(rbuf, "%d", pfd[1]);
		argv[i] = 0;
		exec("/68020/bin/isdn/briserver", argv);
		fatal("can't exec briserver");
	default:
		break;
	}
	close(pfd[1]);
	if(fsession(pfd[0], trbuf) < 0)
		fatal("fsession failed on /dev/bri0");
	if(mount(pfd[0], "/dev", MAFTER, "") < 0)
		fatal("can't mount /dev/bri0");
	close(pfd[0]);

	sprint(file, "/dev/bri%d/dial", devno);
	do{
		fd = open(file, ORDWR);
		if(fd < 0)
			fatal(file);
		memset(dialstr, 0, sizeof(dialstr));
		outin(0, "Number please ", dialstr, sizeof(dialstr));
		fprint(fd, "c/0 %s\n", dialstr);
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
	sprint(file, "#J%d/b1xb2", devno);
	fd = open(file, OWRITE);
	if(fd < 0)
		fatal(file);
	sendmsg(fd, (chan==2) ? "0" : "1");
	close(fd);
	chan = 2;

	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		execl("/68020/bin/isdn/dkbrimon", "dkbrimon", dialstr, 0);
		fatal("can't exec dkbrimon");
	default:
		break;
	}
	sprint(file, "#H%d/ctl", 2*devno+chan-1);
	fd = open(file, ORDWR);
	if(fd < 0)
		fatal(file);
	sendmsg(fd, "hdlc on");
	sendmsg(fd, "recven");
	sendmsg(fd, "push dkmux");
	sendmsg(fd, dkptr);
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
