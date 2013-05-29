#include <termios.h>

speed_t
cfgetospeed(const struct termios *)
{
	return B0;
}

int
cfsetospeed(struct termios *, speed_t)
{
	return 0;
}

speed_t
cfgetispeed(const struct termios *)
{
	return B0;
}

int
cfsetispeed(struct termios *, speed_t)
{
	return 0;
}

