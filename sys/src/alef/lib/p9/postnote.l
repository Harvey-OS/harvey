#include <alef.h>

int
postnote(int group, int pid, byte *note)
{
	int f, r;
	byte file[128];

	rescue {
		if(f >= 0)
			close(f);
		return -1;
	}
	f = -1;
	switch(group) {
	case PNPROC:
		sprint(file, "/proc/%d/note", pid);
		break;
	case PNGROUP:
		sprint(file, "/proc/%d/notepg", pid);
		break;
	default:
		raise;
	}

	f = open(file, OWRITE);
	if(f < 0)
		raise;

	r = strlen(note);
	if(write(f, note, r) != r)
		raise;
	close(f);
	return 0;
}
