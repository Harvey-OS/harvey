#include <u.h>
#include <libc.h>

void
main(void)
{
	char buf[DIRLEN];
	char pathname[512];
	char *cp;

	cp = "?";
	if(stat(".", buf) == 0){
		if(buf[0] == '/'){
			if(getwd(pathname, sizeof(pathname))){
				cp = strrchr(pathname, '/');
				if(cp == 0)
					cp = pathname;
				else
					cp++;
			}
		} else
			cp = buf;
	}
	write(1, cp, strlen(cp));
	exits(0);
}
