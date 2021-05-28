#include "include.h"

#define uartputs vuartputs
#define uartgets vuartgets
#define uartputc vuartputc

static int useuart = 1;

int debug;

void
kbdinit(void)
{
}

void
consinit(char* name, char* speed)
{
	int port;

	if(name == nil || cistrcmp(name, "cga") == 0)
		return;
	port = strtoul(name, 0, 0);
	if(port < 0 || port > 1)
		return;
	USED(speed);

	uartputs("\n", 1);
}

void
consdrain(void)
{
//	if(useuart)
//		uartdrain();
}

static void
consputs(char* s, int n)
{
	if(useuart)
		uartputs(s, n);
}

void
warp86(char* s, ulong)
{
	if(s != nil)
		print(s);
	spllo();
	consdrain();
	print("Takes a licking and keeps on ticking...\n");
	splhi();
	for(;;)
		;
}

static int
getline(char *buf, int size, int timeout)
{
	int c, i=0;
//	ulong start;
	char echo;

	USED(timeout);
	for (;;) {
//		start = m->ticks;
//		do{
//			/* timeout seconds to first char */
//			if(timeout && ((m->ticks - start) > timeout*HZ))
//				return -2;
//			c = consiq.getc(&consiq);
//		}while(c == -1);
		c = vuartgetc();
//		timeout = 0;

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
		seprint(pbuf, pbuf + sizeof pbuf, "%s: ", prompt);
	}
	else if(timeout)
		seprint(pbuf, pbuf + sizeof pbuf, "%s[default==%s (%ds timeout)]: ",
			prompt, def, timeout);
	else
		seprint(pbuf, pbuf + sizeof pbuf, "%s[default==%s]: ",
			prompt, def);
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
		strecpy(buf, buf + size, def);
	return 0;
}

void
panic(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	strecpy(buf, buf + sizeof buf, "panic: ");
	va_start(arg, fmt);
	n = vseprint(buf+7, buf+sizeof(buf)-7, fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\n';
	consputs(buf, n+1);

	if (securemem) {
		n = qtmerrfmt(buf, sizeof buf);
		consputs(buf, n+1);
	}

//	splhi(); for(;;);
	if(etherdetach)
		etherdetach();

//	consputs("\nPress almost any key to reset...", 32);
	spllo();
//	while(consiq.getc(&consiq) == -1)
//		;
	vuartgetc();
	warp86(nil, 0);
}
