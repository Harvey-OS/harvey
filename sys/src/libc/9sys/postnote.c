#include <u.h>
#include <libc.h>

int
postnote(int group, int pid, char *note)
{
	char file[128];
	int f, r;

	switch(group) {
	case PNPROC:
		sprint(file, "/proc/%d/note", pid);
		break;
	case PNGROUP:
		sprint(file, "/proc/%d/notepg", pid);
		break;
	default:
		return -1;
	}

	f = open(file, OWRITE);
	if(f < 0)
		return -1;

	r = strlen(note);
	if(write(f, note, r) != r) {
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
