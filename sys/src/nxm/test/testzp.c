/*
 !6c -FVTw testzp.c && 6l -o 6.testzp testzp.6
 */



#include <u.h>
#include <libc.h>

enum{N = 10 * 1024};

void*
mkseg(char *nm, usize size, char *type)
{
	char sname[50], name[50], cname[50], buf[50];
	int fd;
	void *p;
	char *rp;
	uintptr va;
	long n;

	seprint(sname, sname+sizeof sname, "%s%s%d", nm, type, getpid());
	seprint(name, name+sizeof name, "/mnt/seg/%s", sname);
	seprint(cname, cname+sizeof cname, "%s/ctl", name);

	fd = create(name, OREAD, 0640|DMDIR);
	if(fd < 0)
		sysfatal("create seg: %r");
	close(fd);
	fd = open(cname, ORDWR);
	if(fd < 0)
		sysfatal("open ctl: %r");
	if(fprint(fd, "%s 0 %uld\n", type, size) < 0)
		sysfatal("write ctl: %r");
	n = pread(fd, buf, sizeof buf - 1, 0LL);
	if(n <= 0)
		sysfatal("read ctl: %r");
	buf[n] = 0;
	if(strncmp(buf+1, "msg ", 4) != 0)
		sysfatal("wrong ctl data");
	va = strtoull(buf+5, &rp, 0);
	size = strtoull(rp, 0, 0);
	p = (void*)va;
	p = segattach(SG_CEXEC, sname, p, size);
	if((uintptr)p == ~0)
		sysfatal("segattach: %r");
	close(fd);
	return p;
}

static void
testp(void)
{
	int fd[2], n;
	int i;
	vlong t0, t1;
	char buf[8*1024];

	print("\npipe:\n");
	pipe(fd);
	switch(fork()){
	case -1:
		sysfatal("fork");
	case 0:
		close(fd[0]);
		t0 = nsec();
		for(i = 0; i < N; i++){
			if(write(fd[1], buf, sizeof buf) != sizeof buf)
				sysfatal("write: %r");
		}
		t1 = nsec();
		close(fd[1]);
		sleep(3 * 1000);	/* do not interfere */
		print("write: %lld µs\n", (t1-t0)/1000L);
		exits(nil);
	default:
		close(fd[1]);
		t0 = nsec();
		for(i = 0;; i++){
			n = read(fd[0], buf, sizeof buf);
			if(n < 0)
				sysfatal("zread: %r");
			if(n == 0)
				break;
		}
		t1 = nsec();
		print("read: %lld µs\n", (t1-t0)/1000LL);
		t1 = (t1-t0)/1000LL;
		print("read ok (%d reads %lld bytes/µs)\n", i,
			8LL*1024LL*(vlong)i/t1);
		exits(nil);
	}
}


static void
testzp(char *p)
{
	int fd[2], n;
	Zio io[5];
	int i;
	vlong t0, t1;

	print("\npipe:\n");
	if(zp(fd) < 0)
		sysfatal("zp: %r");
	switch(fork()){
	case -1:
		sysfatal("fork");
	case 0:
		close(fd[0]);
		t0 = nsec();
		for(i = 0; i < N; i++){
			io[0].data = p;
			io[0].size = 8*1024;
			if(zwrite(fd[1], io, 1) != 1 ||
				io[0].size != 8*1024)
				sysfatal("zwrite: %r");
		}
		t1 = nsec();
		close(fd[1]);
		sleep(3 * 1000);	/* do not interfere */
		print("zwrite: %lld µs\n", (t1-t0)/1000L);
		exits(nil);
	default:
		close(fd[1]);
		t0 = nsec();
		for(i = 0;; i++){
			n = zread(fd[0], io, nelem(io), 8*1024);
			if(n < 0)
				sysfatal("zread: %r");
			if(n == 0 || io[0].size == 0)
				break;
		}
		t1 = nsec();
		print("zread: %lld µs\n", (t1-t0)/1000LL);
		t1 = (t1-t0)/1000LL;
		print("zread ok (%d 8K reads %lld bytes/µs)\n",
			i, 8LL*1024LL*(vlong)i/t1);
		testp();
		exits(nil);
	}
}


void
main(int, char*[])
{
	char *us, *ks;

	if(bind("#g", "/mnt/seg", MREPL|MCREATE) < 0)
			sysfatal("bind #g: %r");
	us = mkseg("z", 2 * 1024 * 1024, "umsg");
	ks = mkseg("z", 2 * 1024 * 1024, "kmsg");
	print("us %#p ks %#p\n", us, ks);

	testzp(us);
	exits(nil);
}

