#include	"u.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"lib.h"

Bootconf	conf;

void
main(void)
{
	char *dev;

	consinit();
	meminit();
	mmuinit();

	dev = getenv("booted_dev");
	if (dev == 0)
		panic("get dev name");
	if (strncmp(dev, "BOOTP", 5) == 0)
		bootp(dev);
	else
		print("boot device %s not supported\n", dev);
}
