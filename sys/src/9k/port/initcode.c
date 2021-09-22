/*
 * IMPORTANT!  DO NOT ADD LIBRARY CALLS TO THIS FILE.
 * The entire text image must fit on one page
 * (and there's no data segment, so any read/write data must be on the stack).
 */

#include <u.h>
#include <libc.h>

enum {
	Alignmagic = 0x12345678,
};

char cons[] = "#c/cons";
char boot[] = "/boot/boot";
char dev[] = "/dev";
char c[] = "#c";
char e[] = "#e";
char ec[] = "#ec";
char s[] = "#s";
char srv[] = "/srv";
char env[] = "/env";

static int alignmagic = Alignmagic;
static char unaligned[] = "initcode data seg unaligned!\n";

void
startboot(char *argv0, char **argv)
{
	char buf[200];

	USED(argv0);
	if (alignmagic != Alignmagic)
		write(2, unaligned, sizeof unaligned - 1);
	/*
	 * open the console here so that /boot/boot,
	 * which could be a shell script, can inherit the open fds.
	 */
	close(0);
	close(1);
	close(2);
	open(cons, OREAD);
	open(cons, OWRITE);
	open(cons, OWRITE);
	if (alignmagic != Alignmagic)
		write(2, unaligned, sizeof unaligned - 1);
	bind(c, dev, MAFTER);
	bind(ec, env, MAFTER);
	bind(e, env, MCREATE|MAFTER);
	bind(s, srv, MREPL|MCREATE);
	exec(boot, argv);

	rerrstr(buf, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	_exits(buf);
}
