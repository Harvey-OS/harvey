#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	int i, f;

	for(i=1; i<argc; i++){
		if(access(argv[i], 0) == 0){
			fprint(2, "mkdir: %s already exists\n", argv[i]);
			exits("exists");
		}
		f = create(argv[i], OREAD, CHDIR + 0777L);
		if(f < 0){
			fprint(2, "mkdir: can't create %s\n", argv[i]);
			perror(0);
			exits("error");
		}
		close(f);
	}
	exits(0);
}
