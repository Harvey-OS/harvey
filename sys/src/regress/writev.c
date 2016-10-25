#include <u.h>
#include <libc.h>

struct iovec
{
	void *iov_base;	/* BSD uses caddr_t (1003.1g requires void *) */
	size_t iov_len; /* Must be size_t (1003.1g) */
};


/* dead simple write. Since many iovecs are short it's unlikely the
 * inefficiency is that big a deal. we can replace it later with something better.
 */
int32_t twritev(int fd, const struct iovec *iov, int iovcnt)
{
	int32_t ret = 0;
	int amt, i;

	for(i = 0; i < iovcnt; i++) {
		int want = iov[i].iov_len;
		amt = write(fd, iov[i].iov_base, want);
		if (amt < 0)
			return -1; // incredible API.
		ret += amt;
		if (amt < want)
			break;
	}
	return ret;
	
}

struct iovec t[4] = {
	{"this", 4},
	{"is", 2},
	{"a", 1},
	{"test", 4},
};

int main(int argc, char *argv[])
{
	int fd;
	fd = create("/tmp/x", OWRITE, 0600);
	if (fd < 0)
		sysfatal("%r");
	if (twritev(fd, t, 4) < 11)
		sysfatal("short write: %r");
	print("PASS\n");
	exits("PASS");
	return 0;
}
	
