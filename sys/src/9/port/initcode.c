#include <u.h>
#include <libc.h>

char cons[] = "#c/cons";
char boot[] = "/boot/boot";
char dev[] = "/dev";
char c[] = "#c";
char e[] = "#e";
char ec[] = "#ec";
char s[] = "#s";
char srv[] = "/srv";
char env[] = "/env";

void
main(void)
{
	open(cons, OREAD);
	open(cons, OWRITE);
	open(cons, OWRITE);
	bind(c, dev, MAFTER);
	bind(ec, env, MAFTER);
	bind(e, env, MCREATE|MAFTER);
	bind(s, srv, MREPL|MCREATE);
	execl(boot, boot, nil);
	for(;;);
}
