#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

static char *paqfile;

void
configembed(Method *m)
{
	if(*sys == '/' || *sys == '#'){
		/*
		 *  if the user specifies the disk in the boot cmd or
		 * 'root is from' prompt, use it
		 */
		paqfile = sys;
	} else if(m->arg){
		/*
		 *  a default is supplied when the kernel is made
		 */
		paqfile = m->arg;
	}
}

int
connectembed(void)
{
	int i, p[2];
	Dir *dir;
	char **arg, **argp;

	dir = dirstat("/boot/paqfs");
	if(dir == nil)
		return -1;
	free(dir);

	dir = dirstat(paqfile);
	if(dir == nil || dir->mode & DMDIR)
		return -1;
	free(dir);

	print("paqfs...");
	if(bind("#c", "/dev", MREPL) < 0)
		fatal("bind #c");
	if(bind("#p", "/proc", MREPL) < 0)
		fatal("bind #p");
	if(pipe(p)<0)
		fatal("pipe");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		arg = malloc((bargc+5)*sizeof(char*));
		argp = arg;
		*argp++ = "/boot/paqfs";
		*argp++ = "-iv";
		*argp++ = paqfile;
		for(i=1; i<bargc; i++)
			*argp++ = bargv[i];
		*argp = 0;

		dup(p[0], 0);
		dup(p[1], 1);
		close(p[0]);
		close(p[1]);
		exec("/boot/paqfs", arg);
		fatal("can't exec paqfs");
	default:
		break;
	}
	waitpid();

	close(p[1]);
	return p[0];
}
