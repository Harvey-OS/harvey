#include <alef.h>
#include <bio.h>

Biobuf stdout;

void
main(void)
{
	stdout.init(1, OWRITE);
	stdout.print("hello world!\n");
}
