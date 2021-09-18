#include "lib9.h"

void
setbinmode(void)
{
	_setmode(0, _O_BINARY);
	_setmode(1, _O_BINARY);
	_setmode(2, _O_BINARY);
}
