#include <u.h>
#include <libc.h>
#include "../boot/boot.h"

Method	method[]={
	{ "il", configil, authil, connectil,  },
	{ "tcp", configtcp, authtcp, connecttcp,  },
	{ 0 },
};
int cpuflag = 0;
void (*pword)(int, Method*) = userpasswd;
char* bootdisk = "#w1/sd1";
extern void boot(int, char**);
void
main(int argc, char **argv)
{
	boot(argc, argv);
}
int (*cfs)(int) = 0;
