#include <alef.h>
#include <fcall.h>

byte *modes[] =
{
	"---",
	"--x",
	"-w-",
	"-wx",
	"r--",
	"r-x",
	"rw-",
	"rwx",
};

intern void
rwx(int m, byte *s)
{
	strncpy(s, modes[m], 3);
}

intern byte buf[16];

int
dirmodeconv(Printspec *v)
{
	uint m;

	m = *((uint*)v->o);

	if(m & CHDIR)
		buf[0]='d';
	else if(m & CHAPPEND)
		buf[0]='a';
	else
		buf[0]='-';
	if(m & CHEXCL)
		buf[1]='l';
	else
		buf[1]='-';
	rwx((m>>6)&7, buf+2);
	rwx((m>>3)&7, buf+5);
	rwx((m>>0)&7, buf+8);
	buf[11] = 0;

	strconv(v, buf);
	return(sizeof(uint));
}
