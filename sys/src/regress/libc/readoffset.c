#include <u.h>
#include <libc.h>

// Based on TestReadAtOffset in go's test suite.
// Plan 9 pread had a bug where the channel offset we updated during a call to
// pread - this shouldn't have happened.

static char tmpfilename[128] = "readoffsetXXXXX";

static void
rmtmp(void)
{
	remove(tmpfilename);
}

static int
preadn(int fd, char *buf, int32_t nbytes, int64_t offset)
{
	int nread = 0;
	do {
		int n = pread(fd, buf, nbytes, offset);
		if (n == 0) {
			break;
		}
		buf += n;
		nbytes -= n;
		offset += n;
		nread += n;
	} while (nbytes > 0);
	return nread;
}

void
main(int argc, char *argv[])
{
	mktemp(tmpfilename);
	int fd = create(tmpfilename, ORDWR, 0666);
	if (fd < 0) {
		print("FAIL: couldn't create file: %s\n", tmpfilename);
		exits("FAIL");
	}
	atexit(rmtmp);

	const char *data = "hello, world\n";
	write(fd, data, strlen(data));
	seek(fd, 0, 0);

	int n = 0;
	char b[5] = {0};
	int bsz = sizeof(b);
	n = preadn(fd, b, bsz, 7);
	if (n != sizeof(b)) {
		print("FAIL: unexpected number of bytes pread: %d (expected %d)\n", n, bsz);
		exits("FAIL");
	}

	if (strncmp(b, "world", 5) != 0) {
		print("FAIL: unexpected string pread: '%s'\n", b);
		exits("FAIL");
	}

	n = read(fd, b, bsz);
	if (n != sizeof(b)) {
		print("FAIL: unexpected number of bytes read: %d (expected %d)\n", n, bsz);
		exits("FAIL");
	}

	if (strncmp(b, "hello", 5) != 0) {
		print("FAIL: unexpected string read: '%s'\n", b);
		exits("FAIL");
	}

	print("PASS\n");
	exits(nil);
}
