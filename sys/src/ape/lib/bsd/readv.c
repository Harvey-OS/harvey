#include <unistd.h>
#include <string.h>
#include <sys/uio.h>

int
readv(int fd, struct iovec *v, int ent)
{
	int x, i, n, len;
	char *t;
	char buf[10*1024];

	for(len = i = 0; i < ent; i++)
		len += v[i].iov_len;
	if(len > sizeof(buf))
		len = sizeof(buf);

	len = read(fd, buf, len);
	if(len <= 0)
		return len;

	for(n = i = 0; n < len && i < ent; i++){
		x = len - n;
		if(x > v[i].iov_len)
			x = v[i].iov_len;
		memmove(v[i].iov_base, buf + n, x);
		n += x;
	}

	return len;
}
