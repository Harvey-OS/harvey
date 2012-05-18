/*
 !6c x.c && 6l -o 6.x x.6
 */

//TEST:	6.execac -c 2 6.testx

#include <u.h>
#include <libc.h>

u64int zzz = 0xdeadbeebdeadbeebull;

void f(void);
void
f(void)
{
	if(write(2, "Pi\n", 3) == 3)
		write(2, "3\n", 2);
	write(2, "Pi\n", 3);
}


void
main(int, char*[])
{
	int x;
	int *y;


	if(zzz == 0xdeadbeebdeadbeebull)
		write(2, "Ok2\n", 4);
	f();
	y = &x;
	*y = 3;
	write(2, "hola", 4);
	if(*y == 3)
		write(2, "ok\n", 3);
	exits("xxx");
}

