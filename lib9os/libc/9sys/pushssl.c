#include <u.h>
#include <libc.h>

/*
 * Since the SSL device uses decimal file descriptors to name channels,
 * it is impossible for a user-level file server to stand in for the kernel device.
 * Thus we hard-code #D rather than use /net/ssl.
 */

int
pushssl(int fd, char *alg, char *secin, char *secout, int *cfd)
{
	char buf[8];
	char dname[64];
	int n, data, ctl;

	ctl = open("#D/ssl/clone", ORDWR);
	if(ctl < 0)
		return -1;
	n = read(ctl, buf, sizeof(buf)-1);
	if(n < 0)
		goto error;
	buf[n] = 0;
	sprint(dname, "#D/ssl/%s/data", buf);
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
