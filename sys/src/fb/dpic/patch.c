#include <u.h>
#include <libc.h>
main(void){
	int f=open("S.swab", ORDWR);
	short int n=438;
	seek(f, 0x6a0a, 0);
	write(f, &n, 2);
	seek(f, 0x6a0e, 0);
	write(f, &n, 2);
	n=246;
	seek(f, 0x785e, 0);
	write(f, &n, 2);
	seek(f, 0x7862, 0);
	write(f, &n, 2);
}
