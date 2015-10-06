#include <u.h>
#include <libc.h>

// Change-Id: I1cfd642d29a3955499b3507f56f0449d1710700e
// sys/src/nxm/port/devcons.c:
//	make reads on #c/sysstat return only as many bytes as
//	requested

void
main(void)
{
	int ret; // 0 = pass, 1 = fail
	int fd, n;
	char buf[1];

	fd = open("/dev/sysstat", OREAD);
	if(fd < 0){
		print("FAIL: couldn't open /dev/sysstat: %r\n");
		exits("FAIL");
	}

	ret = 0;
	for(;;){
		n = read(fd, buf, sizeof(buf));
		if(n <= 0)
			break;
		if(n > sizeof(buf))
			ret = 1;
	}
	close(fd);

	if(ret){
		print("FAIL: %d bytes read from /dev/sysstat\n", n);
		exits("FAIL");
	}
	print("PASS\n");
	exits(nil);
}
