#include <u.h>
#include <libc.h>
#include <auth.h>
#include <../boot/boot.h>

/*
 *  pretend to get info out of nvram.
 */
void
glendakey(int, Method*)
{
	char key[DESKEYLEN];

	passtokey(key, "boyd");

	/* set host's key */
	if(writefile("#c/key", key, DESKEYLEN) < 0)
		warning("#c/key");

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", "glenda", 6) < 0)
		warning("#c/hostowner");

	if(writefile("#c/hostdomain", "plan9", 5) < 0)
		warning("#c/hostdomain");
}
