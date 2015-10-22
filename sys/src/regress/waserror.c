// Ensure that many calls to waserror:
// -- return an error
// -- complete
// Since we're going to improve waserror
#include <u.h>
#include <libc.h>

void
main(void)
{
	char buf[1];
	int i, fail;
	// Just to be sure.
	if (close(3) >= 0) {
		print("waserror: close of 3 did not get an error\n");
		exits("FAIL");
	}

	for(i = 0; i < 100000000; i++) {
		if (read(3, buf, 1) >= 0){
			print("waserror: read of 3 did not get an error\n");
			exits("FAIL");
		}
	}
	print("PASS\n");
	exits(nil);
}
