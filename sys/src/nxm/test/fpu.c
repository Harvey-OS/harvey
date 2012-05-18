#include <u.h>
#include <libc.h>

/*
 !	6c -FTVw fpu.c && 6l -o fpu fpu.6
 */

int s = 0;

void
main(int , char *[])
{
	double r;

	r = 3.1415926;
	r /= 2;
	if(r != 0.0)
		print("hola\n");
	print("2nd\n");
	r /= 2;
	if(r+1.9 != 0.0)
		print("caracola\n");
	exits(nil);
}
