#include "all.h"
#include "mem.h"
#include "io.h"
#include "ureg.h"

void	uartspecial1(int, void (*)(int), int (*)(void), int);
int	uartgetc1(void);
void	uartputc1(int c);

static int
getc1(void)
{
	int i, c;

	for(i=0; i<100; i++) {
		c = uartgetc1();
		if(c) {
			return c;
		}
		delay(10);
	}
	return 0;
}

ulong
wwvtime(void)
{
	int i, c;
	char buf[20];
	Rtc t;
	ulong tim;

	uartspecial1(1, 0, 0, 300);

	for(i=0; i<10; i++)
		if(getc1() == 0)
			break;

	uartputc1('o');
	if(getc1() != 'o')
		goto bad;
	uartputc1('\r');
	if(getc1() != '\r')
		goto bad;

	for(i=0; i<18;) {
		c = getc1();
		if((c&0x7f) == '\r') {
			if(i >= 10)
				break;
			i = 0;
			continue;
		}
		buf[i++] = c & 0xf;
	}

	if(i != 15) {
		print("wwvtime: length %d\n", i);
		goto bad;
	}

	// battery low
	if(buf[14] & 0x8)
		print("wwvtime: battery low\n");

	// valid bit
	if((buf[14]&0x1) == 0) {
		print("wwvtime: time not valid\n");
		goto bad;
	}

	t.hour = buf[0]*10 + buf[1];
	t.min = buf[2]*10 + buf[3];
	t.sec = buf[4]*10 + buf[5];
		/* buf[6] is day of week mon = 1 */
	t.mday = buf[7]*10 + buf[8];
	t.mon = buf[9]*10 + buf[10];
	t.year = (buf[11]*10 + buf[12]) + 1900;
	if(t.year < 1970)
		t.year += 100;	// maybe?

	// delay is about (160/300 + 1600/186300) sec
	//	160 is bits delay
	//	300 is baudrate
	//	1600 is distance to ft collins in miles
	//	186300 is speed of light
	//	total delay 542ms

	tim = rtc2sec(&t);
	delay(1000-542);

//	print("wwvtime: %T\n", tim+1);
	return tim + 1;

bad:
	return 0;
}
