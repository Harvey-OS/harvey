#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	ulong num = 1;
	int i;

	if (argc > 1)
		num = strtoul(argv[1], 0, 0);
	print("Try to malloc %ulld bytes in %ld loops\n", num*0x200000ULL, num);
	for(i = 0; i < num; i++)
		if (sbrk(0x200000) == nil){
			print("%d sbrk failed\n", i);
			break;
		}
	print("Did it\n");
	while(1);
}

/* 6c bigloop.c; 6l -o bigloop bigloop.6 */
