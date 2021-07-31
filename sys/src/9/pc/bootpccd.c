#include <u.h>
#include <libc.h>
#include "../boot/boot.h"

Method	method[]={
	{ "local", configlocal, authlocal, connectlocal,  },
	{ "il", configil, authil, connectil,  },
	{ "tcp", configtcp, authtcp, connecttcp,  },
	{ "9600", config9600, auth9600, connect9600,  },
	{ "19200", config19200, auth19200, connect19200,  },
	{ "incon", configincon, authincon, connectincon, "config 1 16 restart dk 256" },
	{ 0 },
};
int cpuflag = 0;
void (*pword)(int, Method*) = userpasswd;
char* bootdisk = "#w/sd0";
extern void boot(int, char**);
void
main(int argc, char **argv)
{
	boot(argc, argv);
}
int (*cfs)(int) = 0;
