#include <u.h>
#include <libc.h>

void
main(int ac, char **av)
{
	char buf[256];
	char *np;

	if(ac != 3){
		fprint(2, "uk2uk takes 2 arguments\n");
		exits("bad args");
	}
	buf[0] = 0;
	while(np = strrchr(av[1], '.')){
		*np++ = 0;
		strcat(buf, np);
		strcat(buf, ".");
	}
	strcat(buf, av[1]);
	strcat(buf, av[2]);
	print("%s\n", buf);
	exits(0);
}
