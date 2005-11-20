unsigned long
getcallerpc(void *x)
{
	return (((unsigned long*)(x))[-1]);
}
