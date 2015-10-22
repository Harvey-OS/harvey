// This simple benchmark measures how long it takes to get 1M errors.
// It will be used to improve waserror performance, as that is a crucial
// bit of code in some paths and Akaros has shown how to get it back to
// Ken C-like performance levels.
#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	int i;
	uint64_t start, stop;
	char buf[1];

	(void) close(3);
	start = nsec();
	for(i = 0; i < 1000000000; i++) {
		if (read(3, buf, 1) >= 0){
			fprint(2, "Read of 3 did not fail!\n");
			exits("benchmark broken");
		}
	}
	stop = nsec();
	print("# error benchmark\n");
	print("1000000 %lld\n", stop - start);
}
