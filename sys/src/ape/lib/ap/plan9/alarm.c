#include "lib.h"
#include <unistd.h>
#include "sys9.h"

unsigned int
alarm(unsigned seconds)
{
	return _ALARM(seconds*1000);
}
