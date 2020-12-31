/* eatmem consumes memory until there is no more.
 *
 * It tests whether there are problems in the new vm.
 */
#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	while (1) {
		// If the kernel crashes, we won't print "PASS" :-)
		mallocz(0x1048576*16, 1);
	}
	print("PASS\n");
	exits(nil);
}
