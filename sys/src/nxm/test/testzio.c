/*
 !6c -FVTw testzio.c && 6l -o 6.testzio testzio.6
 */

//TEST:	 6.testzio <testzio.c

#include <u.h>
#include <libc.h>


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
testzp(char *)
{
	int fd[2], n;
	char buf[80];

	print("\npipe:\n");
	if(bind("#∏", "/mnt/zp", MREPL|MCREATE) < 0)
		sysfatal("bind #∏: %r");
	fd[0] = open("/mnt/zp/data", ORDWR);
	if(fd[0] < 0)
		sysfatal("open data: %r");
	fd[1] = open("/mnt/zp/data1", ORDWR);
	if(fd[1] < 1)
		sysfatal("open data1: %r");
	werrstr("");
	n = write(fd[1], "hola", 4);
	print("write: %d %r\n", n);
	werrstr("");
	n = write(fd[1], "hola", 4);
	print("write: %d %r\n", n);
	werrstr("");
	buf[0] = 0;
	n = read(fd[0], buf, sizeof buf - 1);
	if(n > 0)
		buf[n] = 0;
	print("read: %d %r '%s'\n", n, buf);
	close(fd[0]);
	close(fd[1]);
}


static void
testzp2(char *p)
{
	int fd[2], n;
	char buf[80];
	Zio io[5];
	int i;

	print("\npipe:\n");
	if(bind("#∏", "/mnt/zp", MREPL|MCREATE) < 0)
		sysfatal("bind #∏: %r");
	fd[0] = open("/mnt/zp/data", ORDWR);
	if(fd[0] < 0)
		sysfatal("open data: %r");
	fd[1] = open("/mnt/zp/data1", ORDWR);
	if(fd[1] < 1)
		sysfatal("open data1: %r");
	werrstr("");
	strcpy(p, "hola");
	io[0].data = p;
	io[0].size = strlen(io[0].data);
	io[1].data = p + 100;
	strcpy(io[1].data, "caracola");
	io[1].size = strlen(io[1].data);
	n = zwrite(fd[1], io, 2);
	print("zwrite: %d %r\n", n);
	for(i = 0; i < n; i++)
		print("io[%d].size = %ld\n", i, io[i].size);

	werrstr("");
	n = zread(fd[0], io, nelem(io), sizeof buf - 1);
	if(n > 0)
		buf[n] = 0;
	print("zread: %d %r\n", n);
	for(i = 0; i < n; i++){
		print("io[%d].size = %ld\n", i, io[i].size);
		if(io[i].size > 0){
			memmove(buf, io[i].data, io[i].size);
			buf[io[i].size] = 0;
			print("io[%d].data = '%s'\n", i, buf);
		}
	}
	close(fd[0]);
	close(fd[1]);
}


void
main(int, char*[])
{
	char *us, *ks;
	char msg[] = "Hi there!\n";
	char msg1[] = "Or not?\n";
	char buf[1024];
	Zio io[2];
	int n, i;
	int fd;

	if(bind("#g", "/mnt/seg", MREPL|MCREATE) < 0)
			sysfatal("bind #g: %r");
	us = mkseg("z", 2 * 1024 * 1024, "umsg");
	ks = mkseg("z", 2 * 1024 * 1024, "kmsg");
	print("us %#p ks %#p\n", us, ks);

	print("\nwrite from zio seg:\n");
	io[0].data = us;
	io[0].size = strlen(msg);
	strcpy(io[0].data, msg);
	io[1].data = us+0x1000;
	io[1].size = strlen(msg1);
	strcpy(io[1].data, msg1);
	werrstr("");
	n = zwrite(1, io, nelem(io));
	print("zwrite: %d %r\n", n);
	for(i = 0; i < n; i++)
		print("io[%d].size = %ld\n", i, io[i].size);

	print("\nwrite from data seg:\n");
	werrstr("");
	io[0].data = "now from the data segment\n";
	io[0].size = strlen(io[0].data);
	werrstr("");
	n = zwrite(1, io, 1);
	print("zwrite: %d %r\n", n);
	for(i = 0; i < n; i++)
		print("io[%d].size = %ld\n", i, io[i].size);

	/* read */
	print("\nread:\n");
	io[0].size = io[1].size = 0;
	fd = open("/NOTICE", OREAD);
	n = zread(fd, io, nelem(io), 50);
	close(fd);
	print("zread: %d %r\n", n);
	for(i = 0; i < n; i++){
		print("io[%d].size = %ld\n", i, io[i].size);
		if(io[i].size > 0){
			memmove(buf, io[i].data, io[i].size);
			buf[io[i].size] = 0;
			print("io[%d].data = '%s'\n", i, buf);
		}
	}

	testzp(us+0x2000);
	testzp2(us+0x3000);
	exits(nil);
}

