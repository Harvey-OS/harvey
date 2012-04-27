#pragma src "/sys/src/libdraw"

struct	Cursor
{
	Point	offset;
	uchar	clr[2*16];
	uchar	set[2*16];
};
