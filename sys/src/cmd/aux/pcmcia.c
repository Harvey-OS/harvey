#include <u.h>
#include <libc.h>

enum
{
	Linktarget = 0x13,
	Funcid = 0x21,
	End =	0xff,
};

int fd;
int pos;

void	tdevice(int, int);
void	tlonglnkmfc(int, int);
void	tfuncid(int, int);
void	tcfig(int, int);
void	tentry(int, int);
void	tvers1(int, int);

void (*parse[256])(int, int) =
{
[1]		tdevice,
[6]		tlonglnkmfc,
[0x15]		tvers1,
[0x17]		tdevice,
[0x1A]		tcfig,
[0x1B]		tentry,
[Funcid]	tfuncid,
};

int hex;

void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[512];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "pcmcia: %s\n", buf);
	exits(buf);
}

int
readc(void *x)
{
	int rv;

	seek(fd, 2*pos, 0);
	pos++;
	rv = read(fd, x, 1);
	if(hex)
		print("%2.2ux ", *(uchar*)x);
	return rv;
}

int
tuple(int next, int expect)
{
	uchar link;
	uchar type;

	pos = next;
	if(readc(&type) != 1)
		return -1;
	if(type == 0xff)
		return -1;
print("type %.2uX\n", type & 0xff);

	if(expect && expect != type){
		print("expected %.2uX found %.2uX\n", 
			expect, type);
		return -1;
	}

	if(readc(&link) != 1)
		return -1;
	if(parse[type])
		(*parse[type])(type, link);
	if(link == 0xff)
		next = -1;
	else
		next = next+2+link;
	return next;
}

void
main(int argc, char *argv[])
{
	char *file;
	int next;

	ARGBEGIN{
	case 'x':
		hex = 1;
	}ARGEND;

	if(argc == 0)
		file = "#y/pcm0attr";
	else
		file = argv[0];

	fd = open(file, OREAD);
	if(fd < 0)
		fatal("opening %s: %r", file);

	for(next = 0; next >= 0;)
		next = tuple(next, 0);
}

ulong speedtab[16] =
{
[1]	250,
[2]	200,
[3]	150,
[4]	100,
};

ulong mantissa[16] =
{
[1]	10,
[2]	12,
[3]	13,
[4]	15,
[5]	20,
[6]	25,
[7]	30,
[8]	35,
[9]	40,
[0xa]	45,
[0xb]	50,
[0xc]	55,
[0xd]	60,
[0xe]	70,
[0xf]	80,
};

ulong exponent[8] =
{
[0]	1,
[1]	10,
[2]	100,
[3]	1000,
[4]	10000,
[5]	100000,
[6]	1000000,
[7]	10000000,
};

char *typetab[256] =
{
[1]	"Masked ROM",
[2]	"PROM",
[3]	"EPROM",
[4]	"EEPROM",
[5]	"FLASH",
[6]	"SRAM",
[7]	"DRAM",
[0xD]	"IO+MEM",
};

ulong
getlong(int size)
{
	uchar c;
	int i;
	ulong x;

	x = 0;
	for(i = 0; i < size; i++){
		if(readc(&c) != 1)
			break;
		x |= c<<(i*8);
	}
	return x;
}

void
tdevice(int ttype, int len)
{
	uchar id;
	uchar type;
	uchar speed, aespeed;
	uchar size;
	ulong bytes, ns;
	char *tname, *ttname;

	while(len > 0){
		if(readc(&id) != 1)
			return;
		len--;
		if(id == End)
			return;

		/* PRISM cards have a device tuple with id = size = 0. */
		if(id == 0x00){
			if(readc(&size) != 1)
				return;
			len--;
			continue;
		}

		speed = id & 0x7;
		if(speed == 0x7){
			if(readc(&speed) != 1)
				return;
			len--;
			if(speed & 0x80){
				if(readc(&aespeed) != 1)
					return;
				ns = 0;
			} else
				ns = (mantissa[(speed>>3)&0xf]*exponent[speed&7])/10;
		} else
			ns = speedtab[speed];

		type = id>>4;
		if(type == 0xE){
			if(readc(&type) != 1)
				return;
			len--;
		}
		tname = typetab[type];
		if(tname == 0)
			tname = "unknown";

		if(readc(&size) != 1)
			return;
		len--;
		bytes = ((size>>3)+1) * 512 * (1<<(2*(size&0x7)));

		if(ttype == 1)
			ttname = "device";
		else
			ttname = "attr device";
		print("%s %ld bytes of %ldns %s\n", ttname, bytes, ns, tname);
	}
}

void
tlonglnkmfc(int, int)
{
	int i, opos;
	uchar nfn, space, expect;
	int addr;

	readc(&nfn);
	for(i = 0; i < nfn; i++){
		readc(&space);
		addr = getlong(4);
		opos = pos;
		expect = Linktarget;
		while(addr > 0){
			addr = tuple(addr, expect);
			expect = 0;
		}
		pos = opos;
	}
}

static char *funcids[] = {
	"MULTI",
	"MEMORY",
	"SERIAL",
	"PARALLEL",
	"FIXED",
	"VIDEO",
	"NETWORK",
	"AIMS",
	"SCSI",
};

void
tfuncid(int, int)
{
	uchar func;

	readc(&func);
	print("Function %s\n", 
		(func >= nelem(funcids))? "unknown function": funcids[func]);
}

void
tvers1(int ttype, int len)
{
	uchar c, major, minor;
	int  i;
	char string[512];

	USED(ttype);
	if(readc(&major) != 1)
		return;
	len--;
	if(readc(&minor) != 1)
		return;
	len--;
	print("version %d.%d\n", major, minor);
	while(len > 0){
		for(i = 0; len > 0 && i < sizeof(string); i++){
			if(readc(&string[i]) != 1)
				return;
			len--;
			c = string[i];
			if(c == 0)
				break;
			if(c == 0xff){
				if(i != 0){
					string[i] = 0;
					print("\t%s<missing null>\n", string);
				}
				return;
			}
		}
		string[i] = 0;
		print("\t%s\n", string);
	}
}

void
tcfig(int ttype, int len)
{
	uchar size, rasize, rmsize;
	uchar last;
	ulong caddr;
	ulong cregs;
	int i;

	USED(ttype, len);
	if(readc(&size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(&last) != 1)
		return;
	caddr = getlong(rasize);
	cregs = getlong(rmsize);

	print("configuration registers at");
	for(i = 0; i < 16; i++)
		if((1<<i) & cregs)
			print(" (%d)0x%lux", i, caddr + i*2);
	print("\n");
}

char *intrname[16] =
{
[0]	"memory",
[1]	"I/O",
[4]	"Custom 0",
[5]	"Custom 1",
[6]	"Custom 2",
[7]	"Custom 3",
};

ulong vexp[8] =
{
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
ulong vmant[16] =
{
	10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90,
};

void
volt(char *name)
{
	uchar c;
	ulong microv;
	ulong exp;

	if(readc(&c) != 1)
		return;
	exp = vexp[c&0x7];
	microv = vmant[(c>>3)&0xf]*exp;
	while(c & 0x80){
		if(readc(&c) != 1)
			return;
		switch(c){
		case 0x7d:
			break;		/* high impedence when sleeping */
		case 0x7e:
		case 0x7f:
			microv = 0;	/* no connection */
			break;
		default:
			exp /= 10;
			microv += exp*(c&0x7f);
		}
	}
	print(" V%s %lduV", name, microv);
}

void
amps(char *name)
{
	uchar c;
	ulong amps;

	if(readc(&c) != 1)
		return;
	amps = vexp[c&0x7]*vmant[(c>>3)&0xf];
	while(c & 0x80){
		if(readc(&c) != 1)
			return;
		if(c == 0x7d || c == 0x7e || c == 0x7f)
			amps = 0;
	}
	if(amps >= 1000000)
		print(" I%s %ldmA", name, amps/100000);
	else if(amps >= 1000)
		print(" I%s %lduA", name, amps/100);
	else
		print(" I%s %ldnA", name, amps*10);
}

void
power(char *name)
{
	uchar feature;

	print("\t%s: ", name);
	if(readc(&feature) != 1)
		return;
	if(feature & 1)
		volt("nominal");
	if(feature & 2)
		volt("min");
	if(feature & 4)
		volt("max");
	if(feature & 8)
		amps("static");
	if(feature & 0x10)
		amps("avg");
	if(feature & 0x20)
		amps("peak");
	if(feature & 0x40)
		amps("powerdown");
	print("\n");
}

void
ttiming(char *name, int scale)
{
	uchar unscaled;
	ulong scaled;

	if(readc(&unscaled) != 1)
		return;
	scaled = (mantissa[(unscaled>>3)&0xf]*exponent[unscaled&7])/10;
	scaled = scaled * vexp[scale];
	print("\t%s %ldns\n", name, scaled);
}

void
timing(void)
{
	uchar c, i;

	if(readc(&c) != 1)
		return;
	i = c&0x3;
	if(i != 3)
		ttiming("max wait", i);
	i = (c>>2)&0x7;
	if(i != 7)
		ttiming("max ready/busy wait", i);
	i = (c>>5)&0x7;
	if(i != 7)
		ttiming("reserved wait", i);
}

void
range(int asize, int lsize)
{
	ulong address, len;

	address = getlong(asize);
	len = getlong(lsize);
	print("\t\t%lux - %lux\n", address, address+len);
}

char *ioaccess[4] =
{
	" no access",
	" 8bit access only",
	" 8bit or 16bit access",
	" selectable 8bit or 8&16bit access",
};

int
iospace(uchar c)
{
	int i;

	print("\tIO space %d address lines%s\n", c&0x1f, ioaccess[(c>>5)&3]);
	if((c & 0x80) == 0)
		return -1;

	if(readc(&c) != 1)
		return -1;

	for(i = (c&0xf)+1; i; i--)
		range((c>>4)&0x3, (c>>6)&0x3);
	return 0;
}

void
iospaces(void)
{
	uchar c;

	if(readc(&c) != 1)
		return;
	iospace(c);
}

void
irq(void)
{
	uchar c;
	uchar irq1, irq2;
	ushort i, irqs;

	if(readc(&c) != 1)
		return;
	if(c & 0x10){
		if(readc(&irq1) != 1)
			return;
		if(readc(&irq2) != 1)
			return;
		irqs = irq1|(irq2<<8);
	} else
		irqs = 1<<(c&0xf);
	print("\tinterrupts%s%s%s", (c&0x20)?":level":"", (c&0x40)?":pulse":"",
		(c&0x80)?":shared":"");
	for(i = 0; i < 16; i++)
		if(irqs & (1<<i))
			print(", %d", i);
	print("\n");
}

void
memspace(int asize, int lsize, int host)
{
	ulong haddress, address, len;

	len = getlong(lsize)*256;
	address = getlong(asize)*256;
	if(host){
		haddress = getlong(asize)*256;
		print("\tmemory address range 0x%lux - 0x%lux hostaddr 0x%lux\n",
			address, address+len, haddress);
	} else
		print("\tmemory address range 0x%lux - 0x%lux\n", address, address+len);
}

void
misc(void)
{
}

void
tentry(int ttype, int len)
{
	uchar c, i, feature;
	char *tname;
	char buf[16];

	USED(ttype, len);
	if(readc(&c) != 1)
		return;
	print("configuration %d%s\n", c&0x3f, (c&0x40)?" (default)":"");
	if(c & 0x80){
		if(readc(&i) != 1)
			return;
		tname = intrname[i & 0xf];
		if(tname == 0){
			tname = buf;
			sprint(buf, "type %d", i & 0xf);
		}
		print("\t%s device, %s%s%s%s\n", tname,
			(i&0x10)?" Battery status active":"",
			(i&0x20)?" Write Protect active":"",
			(i&0x40)?" Ready/Busy active":"",
			(i&0x80)?" Memory Wait required":"");
	}
	if(readc(&feature) != 1)
		return;
	switch(feature&0x3){
	case 1:
		power("Vcc");
		break;
	case 2:
		power("Vcc");
		power("Vpp");
		break;
	case 3:
		power("Vcc");
		power("Vpp1");
		power("Vpp2");
		break;
	}
	if(feature&0x4)
		timing();
	if(feature&0x8)
		iospaces();
	if(feature&0x10)
		irq();
	switch((feature>>5)&0x3){
	case 1:
		memspace(0, 2, 0);
		break;
	case 2:
		memspace(2, 2, 0);
		break;
	case 3:
		if(readc(&c) != 1)
			return;
		for(i = 0; i <= (c&0x7); i++)
			memspace((c>>5)&0x3, (c>>3)&0x3, c&0x80);
		break;
	}
	if(feature&0x80)
		misc();
}
