void print(unsigned);

unsigned char buf[8];

void
main(void)
{
	unsigned n;
	unsigned char *p = buf;

	n = p[0] | p[1]<<8;
	print(n);
	n++;
	print(n);
	n++;
	USED(n);
}
