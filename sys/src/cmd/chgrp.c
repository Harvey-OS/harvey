#include <u.h>
#include <libc.h>

int	readgid(char *);

void
main(int argc, char *argv[])
{
	int i;
	Dir dir;
	char err[ERRLEN];
	char *group;
	char *errs;

	if(argc < 2){
		fprint(2, "usage: chgrp group file ....\n");
		exits("usage");
	}
	group = argv[1];
	errs = 0;
	for(i=2; i<argc; i++){
		if(dirstat(argv[i], &dir)==-1){
			errstr(err);
			fprint(2, "chgrp: can't stat %s: %s\n", argv[i], err);
			errs = "can't stat";
			continue;
		}
		strncpy(dir.gid, group, sizeof(dir.gid));
		if(dirwstat(argv[i], &dir)==-1){
			errstr(err);
			fprint(2, "chgrp: can't wstat %s: %s\n", argv[i], err);
			errs = "can't wstat";
			continue;
		}
	}
	exits(errs);
}
