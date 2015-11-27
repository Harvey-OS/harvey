#include <u.h>
#include <libc.h>

#include "alien/sys/outer.h"

void
main(void) {
	PRINT_MESSAGE;
	print("PASS\n");
	exits("PASS");
}
