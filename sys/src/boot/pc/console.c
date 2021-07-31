#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

IOQ consiq;
IOQ consoq;

static int useuart;

int	debug = 0;

void
kbdchar(int c)
{
	c &= 0x7F;
	if(c == 0x10)
		warp86("\n^P\n", 0);
	if(c == 0x12)			/* control-r? */
		debug = !debug;
	consiq.putc(&consiq, c);
}

static int
consputc(void)
{
	return consoq.getc(&consoq);
}

void
kbdinit(void)
{
	i8042init();
	qinit(&consiq);
}

void
consinit(char* name, char* speed)
{
	int baud, port;

	if(name == nil || cistrcmp(name, "cga") == 0)
		return;
	port = strtoul(name, 0, 0);
	if(port < 0 || port > 1)
		return;
	if(speed == nil || (baud = strtoul(speed, 0, 0)) == 0)
		baud = 9600;

	qinit(&consoq);

	uartspecial(port, kbdchar, consputc, baud);
	useuart = 1;
	uartputs(&consoq, "\n", 1);
}

void
consdrain(void)
{
	if(useuart)
		uartdrain();
}

void
consputs(char* s, int n)
{
	cgascreenputs(s, n);
	if(useuart)
		uartputs(&consoq, s, n);
}

void
warp86(char* s, ulong)
{
	if(s != nil)
		print(s);
	spllo();
	consdrain();

	i8042reset();

	/*
	 * Often the BIOS hangs during restart if a conventional 8042
	 * warm-boot sequence is tried. The following is Intel specific and
	 * seems to perform a cold-boot, but at least it comes back.
	 * And sometimes there is no keyboard...
	 *
	 * The reset register (0xcf9) is usually in one of the bridge
	 * chips. The actual location and sequence could be extracted from
	 * ACPI but why bother, this is the end of the line anyway.
	 */
	print("Takes a licking and keeps on ticking...\n");
	*(ushort*)KADDR(0x472) = 0x1234;	/* BIOS warm-boot flag */
	outb(0xcf9, 0x02);
	outb(0xcf9, 0x06);

	for(;;)
		idle();
}

static int
getline(char *buf, int size, int timeout)
{
	int c, i=0;
	ulong start;
	char echo;

	for (;;) {
		start = m->ticks;
		do{
			/* timeout seconds to first char */
			if(timeout && ((m->ticks - start) > timeout*HZ))
				return -2;
			c = consiq.getc(&consiq);
		}while(c == -1);
		timeout = 0;

		if(c == '\r')
			c = '\n'; 		/* turn carriage return into newline */
		if(c == '\177')
			c = '\010';		/* turn delete into backspace */
		if(c == '\025')
			echo = '\n';		/* echo ^U as a newline */
		else
			echo = c;
		consputs(&echo, 1);

		if(c == '\010'){
			if(i > 0)
				i--; /* bs deletes last character */
			continue;
		}
		/* a newline ends a line */
		if (c == '\n')
			break;
		/* ^U wipes out the line */
		if (c =='\025')
			return -1;
		if(i == size)
			return size;
		buf[i++] = c;
	}
	buf[i] = 0;
	return i;
}

int
getstr(char *prompt, char *buf, int size, char *def, int timeout)
{
	int len, isdefault;
	char pbuf[PRINTSIZE];

	buf[0] = 0;
	isdefault = (def && *def);
	if(isdefault == 0){
		timeout = 0;
		sprint(pbuf, "%s: ", prompt);
	}
	else if(timeout)
		sprint(pbuf, "%s[default==%s (%ds timeout)]: ", prompt, def, timeout);
	else
		sprint(pbuf, "%s[default==%s]: ", prompt, def);
	for (;;) {
		print(pbuf);
		consdrain();
		len = getline(buf, size, timeout);
		switch(len){
		case 0:
			/* RETURN */
			if(isdefault)
				break;
			continue;
		case -1:
			/* ^U typed */
			continue;
		case -2:
			/* timeout, use default */
			consputs("\n", 1);
			len = 0;
			break;
		default:
			break;
		}
		if(len >= size){
			print("line too long\n");
			continue;
		}
		break;
	}
	if(len == 0 && isdefault)
		strncpy(buf, def, size);
	return 0;
}

int
print(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	consputs(buf, n);

	return n;
}

void
panic(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	strcpy(buf, "panic: ");
	va_start(arg, fmt);
	n = vseprint(buf+7, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\n';
	consputs(buf, n+1);

//floppymemwrite();
//splhi(); for(;;);
	if(etherdetach)
		etherdetach();
	if(sddetach)
		sddetach();

	consputs("\nPress almost any key to reset...", 32);
	spllo();
	while(consiq.getc(&consiq) == -1)
		;

	warp86(nil, 0);
}
