#include <alef.h>
#include <bio.h>
#include <net.h>


void
hnputs(byte *ptr, usint val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}

void
hnputl(byte *ptr, uint val)
{
	ptr[0] = val>>24;
	ptr[1] = val>>16;
	ptr[2] = val>>8;
	ptr[3] = val;
}

uint
nhgetl(byte *ptr)
{
	return ((ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3]);
}

usint
nhgets(byte *ptr)
{
	return ((ptr[0]<<8) | ptr[1]);
}
