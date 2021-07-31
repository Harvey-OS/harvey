#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	char *p;
	long secs;

	if(argc>1){
		for(secs = atol(argv[1]); secs > 0; secs--)
			sleep(1000);
		/*
		 * no floating point because it is useful to
		 * be able to run sleep when bootstrapping
		 * a machine.
		 */
		if(p = strchr(argv[1], '.')){
			p++;
			switch(strlen(p)){
			case 0:
				break;
			case 1:
				sleep(atoi(p)*100);
				break;
			case 2:
				sleep(atoi(p)*10);
				break;
			default:
				p[3] = 0;
				sleep(atoi(p));
				break;
			}
		}
	}
	exits(0);
}
