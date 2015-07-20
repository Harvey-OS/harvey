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

	pid = fork();
	if (pid < 0) {
		print("FAIL\n");
		exits("FAIL");
	}

	if (pid == 0) {
		char w[1];
		read(p[0], w, 1);
		//*c = 0;
		print("PASS\n");
		exits("PASS");
	}
	name = smprint("/proc/%d/mmap", pid);
	mfd = open(name, ORDWR);
	amt = write(p[0], "R", sizeof("R"));
	if (amt < 0) {
		print("FAIL\n");
		exits("FAIL");
	}
	amt = read(mfd, data, sizeof(data));
	if (amt < 0) {
		print("FAIL\n");
		exits("FAIL");
	}
	print("PASS\n");
	exits("PASS");
}
