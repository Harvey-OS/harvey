#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	unsigned char *c = (void *)(512ULL * 1024 * 1024 * 1024);
	int pid;
	int p[2], mfd;
	char *name;
	static char data[128];
	int amt;
	if (pipe(p) < 0) {
		print("FAIL\n");
		exits("FAIL");
	}

	print("p[%d,%d]; c is %p\n", p[0], p[1], c);
	pid = fork();
	if (pid < 0) {
		print("FAIL\n");
		exits("FAIL");
	}

	if (pid == 0) {
		char w[1];
		print("KID: WAIT\n");
		read(p[1], w, 1);
		print("KID: CONTINUE\n");
//w		*c = 0;
		print("KID: LOOP FOREVER");
		print("PASS\n");
		exits("PASS");
	}
	name = smprint("/proc/%d/mmap", pid);
	mfd = open(name, ORDWR);
	amt = write(p[0], "R", sizeof("R"));
	close(p[0]);
	close(p[1]);
	print("DONE WITH PIPES amt %d\n", amt);
	if (amt < 0) {
		print("FAIL\n");
		exits("FAIL");
	}
	amt = read(mfd, data, sizeof(data));
	print("read from mfd %d\n", amt);
	if (amt < 0) {
		print("FAIL\n");
		exits("FAIL");
	}
	print("PASS\n");
	exits("PASS");
}
