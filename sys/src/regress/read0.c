#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	int amt;
	char r[1];
	amt = read(0, r, 0);
	if (amt < 0) {
		print("FAIL\n");
		exits("FAIL");
	}
	print("PASS\n");
	exits("PASS");
}
