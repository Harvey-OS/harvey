#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	ulong num = 1;
	uvlong size;
	u8int *c;

	if (argc > 1)
		num = strtoul(argv[1], 0, 0);
	size = num * 0x200000ULL;
	print("Try to malloc %ulld bytes\n", size);
	c = mallocz(size, 1);
	print("Did it\n");
	while(1);
}

/* 6c big.c; 6l -o big big.6 */
