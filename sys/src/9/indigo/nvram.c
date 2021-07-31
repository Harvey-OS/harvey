#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

typedef struct Nvram_ro	Nvram_ro;
typedef struct Nvram_rw	Nvram_rw;
typedef union Nvram	Nvram;

/*
 *	The National nmc93cs46 is arranged as 64 16-bit registers.
 *	Bytes are stored in big-endian order in each short.
 */
#define NVMAX	128
#define PWDLEN	8
#define KEYLEN	(2*PWDLEN+1)

struct Nvram_rw
{
	uchar	bootmode[1];	/* autoboot/command mode on reset */
	uchar	state[1];	/* current validity of tod clock and nvram */
	uchar	netaddr[16];	/* ip address in `.' format */
	uchar	lbaud[5];	/* initial baud rates */
	uchar	rbaud[5];
	uchar	console[1];	/* what consoles are enabled at power-up */
	uchar	scolor[6];	/* initial screen color */
	uchar	pcolor[6];	/* initial page color */
	uchar	lcolor[6];	/* initial logo color */
	uchar	pad0[2];
	uchar	checksum[1];
	uchar	diskless[1];
	uchar	nokbd[1];	/* may boot sans kbd */
	uchar	bootfile[50];	/* program to load on autoboot */
	uchar	pwdkey[KEYLEN];	/* encrypted key to protect manual mode */
	uchar	volume[3];	/* default audio volume */
};

struct Nvram_ro
{
	uchar	pad1[NVMAX-6];
	uchar	enet[6];	/* ethernet address */
};

union Nvram
{
	Nvram_rw;
	Nvram_ro;
};

#define	OFF(x)	((ulong)((Nvram *)0)->x)
#define	LEN(x)	(sizeof ((Nvram *)0)->x)

/* Bits in CPU_AUX_CONTROL register */

#define SER_TO_CPU	0x10		/* Data from serial memory */
#define CPU_TO_SER	0x08		/* Data to serial memory */
#define SERCLK		0x04		/* Serial Clock */
#define	CONSOLE_CS	0x02		/* EEPROM (nvram) chip select */
#define	CONSOLE_LED	0x01		/* Console led */
#define	NVRAM_PRE	0x01		/* EEPROM (nvram) PRE pin signal */

/* Control opcodes  */

#define SER_READ	0xc000		/* serial memory read */
#define SER_WEN		0x9800		/* write enable before prog modes */
#define SER_WRITE	0xa000		/* serial memory write */
#define SER_WDS		0x8000		/* disable all programming */

#define	DEV	IO(uchar, CPU_AUX_CONTROL)

static uchar console_led;

static void
cs_on(void)
{
	uchar *dev = DEV;

	console_led = *dev & CONSOLE_LED;

	*dev &= ~CPU_TO_SER;
	*dev &= ~SERCLK;
	*dev &= ~NVRAM_PRE;
	Xdelay(1);
	*dev |= CONSOLE_CS;
	*dev |= SERCLK;
}

static void
cs_off(void)
{
	uchar *dev = DEV;

	*dev &= ~SERCLK;
	*dev &= ~CONSOLE_CS;
	*dev |= NVRAM_PRE;
	*dev |= SERCLK;

	*dev = (*dev & ~CONSOLE_LED) | console_led;
}

/*
 *	Find out number of address bits in the nvram.
 */
static int
nvbits(void)
{
	static int nbits;
	int i;
	uchar *dev = DEV;

	if(nbits)
		return nbits;

	cs_on();
	/* shift in read command.  assume 3 bits opcode, 6 bits address */
	for(i=0; i<9; i++){
		if((SER_READ << i) & 0x8000)
			*dev |= CPU_TO_SER;
		else
			*dev &= ~CPU_TO_SER;
		*dev &= ~SERCLK;
		*dev |= SERCLK;
	}
	if(*dev & SER_TO_CPU){
		nbits = 11;
		*dev &= ~CPU_TO_SER;
		*dev &= ~SERCLK;
		*dev |= SERCLK;
		*dev &= ~SERCLK;
		*dev |= SERCLK;
	}else
		nbits = 9;
	/* shift out data */
	for(i=0; i<16; i++){
		*dev &= ~SERCLK;
		*dev |= SERCLK;
	}
	cs_off();
	return nbits;
}

/*
 *	Clock in the nvram command and the register number.  For the
 *	National Semiconductor nvram chip the op code is 3 bits and
 *	the address is 6 or 8 bits. 
 */
static void
cmd(int cmd, int reg, int nbits)
{
	uchar *dev = DEV;
	ushort ser_cmd;
	int i;

	ser_cmd = cmd | (reg << (16 - nbits));	/* left justified */
	for(i=0; i<nbits; i++, ser_cmd<<=1){
		if(ser_cmd & 0x8000)
			*dev |= CPU_TO_SER;
		else
			*dev &= ~CPU_TO_SER;
		*dev &= ~SERCLK;
		*dev |= SERCLK;
	}
	*dev &= ~CPU_TO_SER;	/* see data sheet timing diagram */
}

/*
 *	After write/erase commands, we must wait for the command to complete.
 *	Write cycle time is 10 ms max (~5 ms nom); we timeout after
 *	NVDELAY_TIME * NVDELAY_LIMIT = 20 ms
 */
#define NVDELAY_TIME	100	/* 100 us delay times */
#define NVDELAY_LIMIT	200	/* 200 delay limit */

static int
hold(void)
{
	uchar *dev = DEV;
	int error = 0;
	int timeout = NVDELAY_LIMIT;

	cs_on();
	while(!(*dev & SER_TO_CPU) && timeout--)
		Xdelay(NVDELAY_TIME);
	if (!(*dev & SER_TO_CPU))
		error = -1;
	cs_off();
	return error;
}

/*
 *	Read a 16-bit register from non-volatile memory.
 */
static int
getnv(int reg)
{
	uchar *dev = DEV;
	ushort data = 0;
	int nbits = nvbits();
	int i;

	cs_on();
	cmd(SER_READ, reg, nbits);
	/* clock the data out of serial mem */
	for(i=0; i<16; i++){
		*dev &= ~SERCLK;
		*dev |= SERCLK;
		data <<= 1;
		data |= (*dev & SER_TO_CPU) ? 1 : 0;
	}
	cs_off();
	return data;
}


/*
 *	Write a 16-bit word into non-volatile memory.
 */
static int
setnv(int reg, int val)
{
	uchar *dev = DEV;
	int error;
	int i;
	int nbits = nvbits();

	cs_on();
	cmd(SER_WEN, 0, nbits);	
	cs_off();

	cs_on();
	cmd(SER_WRITE, reg, nbits);

	/*
	 * clock the data into serial mem 
	 */
	for(i=0; i<16; i++, val<<=1){
		if(val & 0x8000)
			*dev |= CPU_TO_SER;
		else
			*dev &= ~CPU_TO_SER;
		*dev &= ~SERCLK;
		*dev |= SERCLK;
	}
	*dev &= ~CPU_TO_SER;	/* see data sheet timing diagram */
	
	cs_off();
	error = hold();

	cs_on();
	cmd(SER_WDS, 0, nbits);
	cs_off();

	return error;
}

static int
nvchecksum(void)
{
	int reg, checksum;
	ushort val;

	checksum = 0xa5;	/* seed it */
	for(reg = 0; reg < NVMAX/2; reg++){
		val = getnv(reg);
		if(reg == (OFF(checksum) / 2)){	/* skip the checksum byte */
			if(OFF(checksum) & 0x01)
				checksum ^= val >> 8;
			else
				checksum ^= val & 0xff;
		}else
			checksum ^= (val >> 8) ^ (val & 0xff);
		/* 8-bit rotate */
		checksum = (checksum << 1) | ((checksum&0x80) != 0);
	}
	return checksum&0xff;
}

static Nvram	nvram;
static int	inited;

/*
 *	Write to non-volatile memory.
 */
int
putnvram(ulong offset, uchar *buf, int len)
{
	ushort curval;
	uchar checksum;
	int i, offset_save; 

	if(offset >= NVMAX || len <= 0 || offset+len > NVMAX)
		return -1;
	inited = 0;
	offset_save = offset;

	if(offset % 2){
		curval  = getnv(offset/2);
		curval &= 0xff00;
		curval |= *buf++;
		if(setnv(offset/2, curval))
			return -1;
		offset++;
		len--;
	}

	for(i=0; i<len/2; i++){
		curval = *buf++ << 8;
		curval |= *buf++;
		if(setnv(offset/2 + i, curval))
			return -1;
	}

	if(len % 2){
		curval = getnv((offset + len) / 2);
		curval &= 0x00ff;
		curval |= *buf << 8;
		if(setnv((offset + len) / 2, curval))
			return -1;
	}

	if(offset_save != OFF(checksum)){
		checksum = nvchecksum();
		return putnvram(OFF(checksum), &checksum, 1);
	}
	return 0;
}

static void *
readnvram(void)
{
	ushort *data;
	int i;

	if(!inited){
		inited = 1;
		data = (ushort *)&nvram;
		for(i=0; i<NVMAX/2; i++)
			*data++ = getnv(i);
	}
	return &nvram;
}

int
getnvram(ulong offset, void *buf, int len)
{
	uchar *v = readnvram();

	if(offset >= NVMAX || len <= 0)
		return 0;
	if(offset+len > NVMAX)
		len = NVMAX-offset;
	memmove(buf, &v[offset], len);
	return len;
}

void
getnveaddr(void *p)
{
	Nvram *v = readnvram();

	memmove(p, v->enet, sizeof v->enet);
}
