unsigned long
nptohl(void *p)
{
	unsigned char *up;
	unsigned long x;

	up = p;
	x = (up[0]<<24)|(up[1]<<16)|(up[2]<<8)|up[3];
	return x;
}
