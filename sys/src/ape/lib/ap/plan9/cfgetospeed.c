#include <termios.h>

speed_t
cfgetospeed(const struct termios *p)
{
	return B0;
}

int
cfsetospeed(struct termios *p, speed_t s)
{
	return 0;
}

speed_t
cfgetispeed(const struct termios *p)
{
	return B0;
}

int
cfsetispeed(struct termios *p, speed_t s)
{
	return 0;
}

