#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	Waitmsg *w;
	int i;

	for(i = 0; i < argc; i++) {
		switch(fork()) {
		case -1:
			fprint(2, "fork fail\n");
			exits("FAIL");
		case 0:
			execl("/bin/echo", "echo", argv[i], nil);
			fprint(2, "execl fail\n");
			exits("execl");
		default:
			w = wait();
			if(strcmp(w->msg, "")) {
				print("FAIL\n");
				exits("FAIL");
			}
			break;
		}
	}
	print("PASS\n");
	exits("PASS");
}
