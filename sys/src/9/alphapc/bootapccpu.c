#include <u.h>
#include <libc.h>
#include "../boot/boot.h"

Method	method[]={
	{ "il", configil, authil, connectil,  },
	{ 0 },
};
int cpuflag = 1;
void (*pword)(int, Method*) = key;
char* rootdir = "/root";
char* bootdisk = "#w/sd0";
extern void boot(int, char**);
void
main(int argc, char **argv)
{
	boot(argc, argv);
}
int (*cfs)(int) = 0;
