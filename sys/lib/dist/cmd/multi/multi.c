#include <u.h>
#include <libc.h>

#include "multiproto.h"
struct {
	char *name; 
	void (*fn)(int, char**);
} mains[] =
{
#include "multi.h"
};

void
main(int argc, char **argv)
{
	int i;
	char *cmd, *p;
	
	if(argc == 1){
		fprint(2, "usage: multi cmd args...\n");
		exits("usage");
	}
	
	cmd = argv[1];
	if(p = strrchr(cmd, '/'))
		cmd = p+1;
	argv++;
	argc--;

	for(i=0; i<nelem(mains); i++){
		if(strcmp(cmd, mains[i].name) == 0){
			mains[i].fn(argc, argv);
			return;
		}
	}
	fprint(2, "multi: no such cmd %s\n", cmd);
	exits("no cmd");
}
