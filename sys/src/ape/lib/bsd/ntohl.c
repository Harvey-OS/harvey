unsigned long
ntohl(int x)
{
	unsigned long n;
	unsigned char *p;

	n = x;
	p = (unsigned char*)&n;
	return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}

unsigned long
htonl(unsigned long h)
{
	unsigned long n;
	unsigned char *p;

	p = (unsigned char*)&n;
	p[0] = h>>24;
	p[1] = h>>16;
	p[2] = h>>8;
	p[3] = h;
	return n;
}

unsigned short
ntohs(int x)
{
	unsigned short n;
	unsigned char *p;

	n = x;
	p = (unsigned char*)&n;
	return (p[0]<<8)|p[1];
}

unsigned short
htons(unsigned short h)
{
	unsigned short n;
	unsigned char *p;

	p = (unsigned char*)&n;
	p[0] = h>>8;
	p[1] = h;
	return n;
}
