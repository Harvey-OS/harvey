#include "lib9.h"
#undef getwd
#include <unistd.h>

char *
infgetwd(char *buf, int size)
{
	return getcwd(buf, size);
}
