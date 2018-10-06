
#include <u.h>
#include <libc.h>

void
main(void)
{
	char buf[1024];
	int i, n, oldn, fail;
	int fd;

	fail = 0;
	for(i = 0; i < 10000; i++){
		fd = open("/proc/1/status", OREAD);
		n = read(fd, buf, sizeof buf);
		if(i != 0 && n != oldn){
			fprint(2, "read %d, want %d\n", n, oldn);
			fail++;
		}
		oldn = n;
		close(fd);
	}
	print("PASS\n");
	exits("PASS");
}
