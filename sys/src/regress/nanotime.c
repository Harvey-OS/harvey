#include <u.h>
#include <libc.h>

int verbose = 0;
void
main(void)
{
	int ret = 0; // success
	vlong start, end;
	char *msg;

	start = nsec();
	sleep(1);
	end = nsec();

	if (end <= start)
		ret = 1;

	if (verbose)
		print("nanotime: start %llx, end %llx\n", start, end);
	if(ret){
		msg = smprint("nanotime: FAIL: start %llx end %llx",
			start, end);
		print("%s\n", msg);
		exits(msg);
	}
}
