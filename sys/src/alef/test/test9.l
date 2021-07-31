#include <alef.h>
#include <graphics.h>

void
main()
{
	print("test\n");

	binit(nil, nil, nil);

	print("%R\n", screen.r);

	bitblt(&screen, screen.r.min, &screen, screen.r, F^S);
}
