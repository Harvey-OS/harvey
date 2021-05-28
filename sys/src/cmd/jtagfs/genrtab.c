#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "debug.h"

static uchar
reverse(uchar c)
{
	uchar rc;
	int i, j, ns;

	rc = 0;
	for(i = 0, j = 1; i < 8; i++, j <<= 1){
		ns = (7 - 2*i);
		if( ns < 0){
			ns = -ns;
			rc |= (c&j) >> ns;
		}
		else
			rc |= (c&j) << ns;
	}
	return rc;
}


void
main(int , char *[])
{
	int i;

	print("/* Generated automatically, *DO NOT EDIT* */\n\n");
	print("#include <u.h>\n");
	print("#include <libc.h>\n\n");
	print("uchar rtab[256]=\n");
	print("{\n\t");
	for(i = 0; i < 256; i++){
		print("%#2.2ux, ", reverse(i));
		if(i != 0 && (i + 1) % 8 == 0)
			print("\n\t");
	}
	print("\n};");
	exits(nil);
}

