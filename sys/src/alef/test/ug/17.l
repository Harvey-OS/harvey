#include	<alef.h>

intern int fd = -1;

void
t(void)
{
	print("time: %lux\n", time());
}

int
time(void)
{
	int t;
	byte b[20];

	memset(b, 0, sizeof(b));
	!{
		if(fd < 0)
			fd = open("/dev/time", OREAD|OCEXEC);
		if(fd >= 0) {
			seek(fd, 0, 0);
			read(fd, b, sizeof(b));
		}
	}
	t = atoi(b);
	return t;
}

void
main(void)
{
	proc t(), t(); t();

	t();
}
