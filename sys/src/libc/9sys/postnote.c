#include <u.h>
#include <libc.h>

int
postnote(int pid, char *note)
{
	char file[128];
	int f, r;

	if(pid < 0)
		sprint(file, "/proc/%d/notepg", -pid);
	else
		sprint(file, "/proc/%d/note", pid);

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
