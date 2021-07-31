#include "u.h"
#include "lib.h"
#include "fns.h"
#include "dat.h"
#include "mem.h"

void
main(void)
{
	void (*f)(void);
	ulong *kernel;

	print("inflating kernel\n");

	kernel = (ulong*)(0xc0200000+20*1024);
	if(gunzip((uchar*)0xc0008000, 2*1024*1024, (uchar*)kernel, 512*1024) > 0){
		f = (void (*)(void))0xc0008010;
		draincache();
	} else {
		print("inflation failed\n");
		f = nil;
	}
	(*f)();
}

void
exit(void)
{

	void (*f)(void);

	delay(1000);

	print("it's a wonderful day to die\n");
	f = nil;
	(*f)();
}

void
delay(int ms)
{
	int i;

	while(ms-- > 0){
		for(i = 0; i < 1000; i++)
			;
	}
}
