#include <u.h>
#include <libc.h>

int
pushssl(int fd, char *alg, char *secin, char *secout, int *cfd)
{
	char buf[8];
	char dname[64];
	int n, data, ctl;

	ctl = open("/net/ssl/clone", ORDWR);
	if(ctl < 0 && access("/net/ssl", 0) < 0){
		bind("#D", "/net", MAFTER);
		ctl = open("/net/ssl/clone", ORDWR);
		if(ctl < 0)
			return -1;
	}
	n = read(ctl, buf, sizeof(buf)-1);
	if(n < 0)
		goto error;
	buf[n] = 0;
	sprint(dname, "/net/ssl/%s/data", buf);
	data = open(dname, ORDWR);
	if(data < 0)
		goto error;
	if(fprint(ctl, "fd %d", fd) < 0 ||
	   fprint(ctl, "secretin %s", secin) < 0 ||
	   fprint(ctl, "secretout %s", secout) < 0 ||
	   fprint(ctl, "alg %s", alg) < 0){
		close(data);
		goto error;
	}
	close(fd);
	if(cfd != 0)
		*cfd = ctl;
	else
		close(ctl);
	return data;
error:
	close(ctl);
	return -1;
}
