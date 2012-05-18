#include <u.h>
#include <libc.h>

/*
 !	6c -FTVw usemkill.c && 6l -o usemkill usemkill.6
 */


//TEST: 6.usemkill & { sleep 3 ; slay 6.usemkill|rc}
int s = 0;

void
main(int , char *[])
{

	semdebug = 1;
	print("down...\n");
	downsem(&s, 0);
	print("could down...\n");
	exits(nil);
}
