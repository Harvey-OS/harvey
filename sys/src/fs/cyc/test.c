#include	<u.h>
#include	"comm.h"
#include	"all.h"
#include	"fns.h"
#include	"io.h"

enum
{
	SystemResetpc	= 0xF0C00048,	/* NB. These addresses are known to the rom ! */
	SystemPrcb	= 0xFFFFFF30,
	Resetreq	= 3<<8,
};

int	getchar(void);
void	recv(void);
void	txmit(void);

void
main(void)
{
	extern char edata[], end[];

	memset(edata, 0, end-edata);

	duartinit();
	

	print("Cyclone test\n");


	initvme();
	initsql();

	switch(getchar()) {
	case 'r':
		print("receiver\n");
		recv();
	case 't':
		print("transmitter\n");
		txmit();
	}
}

void
exits(char *s)
{
	print("exits: %s\n", s);
	for(;;)
		;
}

void
recv(void)
{
	uchar block[256+16];
	uchar *p;
	int i, j, blk;

	p = (uchar*)(((int)block+15) & ~0xf);

	blk = 0;
	for(;;) {
		if(FIFORCVBLK)
			continue;
		bmovefv(&(SQL->rxfifo), (ulong)p);
		for(i = 0; i < 254; i++) {
			if(p[i]+1 != p[i+1]) {
				print("recv error! off %d blk %d\n", i, blk);
				for(j = i-5; j < i+5; j++)
					print("%lux ", p[j] & 0xff);
				print("\n");
			}
		}
		blk++;
		XMIT(0x12345678);
		if((blk%10000) == 0) putc('.');
	}
}

void
txmit(void)
{
	uchar block[256+16];
	uchar *p;
	ulong c;
	int i;

	p = (uchar*)(((int)block+15) & ~0xf);
	for(i = 0; i < 256; i++)
		p[i] = i;

	for(;;) {
		while(FIFOXMTBLK)
			;
		bmovevf((ulong)p, &(SQL->txfifo));
		if(!(FIFOE)) {
			RECV(c);
			if(c != 0x12345678)
				print("reply bad 0x%lux\n", c);
		}
	}
}

void
delay(long ms)
{
	int i;

	ms = 5000*ms;
	for(i = 0; i < ms; i++)
		;
}

void
initvme(void)
{
	Vic *v;

	print("vic init\n");

	v = VIC;
	v->lbtr   = 0x49;	/* Local bus time */
	v->ttor   = 0xDE;	/* 512uS bus time out */
	v->iconf  = 0x44;
	v->ss0cr0 = 0x32;	/* A32 mode, Supervisor access only */
	v->ss0cr1 = 0x30;
	v->ss1cr0 = 0x36;	/* A24 mode, Supervisor access only */
	v->ss1cr1 = 0x30;
	v->rcr    = 0x00;
	v->amsr   = 0x8b;
	v->btdr	  = 0x02;
	v->besr   = 0;		/* Reset error status */

	/*
	 * SGI machines need AM code 9 (User Data Extentenenenended)
	 * so the processor must run in user mode 
	 */
	modpc(2, 0);		/* Clear em -> user mode */
	print("vic init done, u-mode\n");
}

void
initsql(void)
{
	Taxi *t;
	ulong i;

	t = SQL;

	t->rst1 = 0;		/* Reset taxi chips */
	delay(10);
	t->rst0 = 0;
	delay(10);
	t->rst1 = 0;
	delay(10);

	i = t->ctl;		/* Clear command bits */
	USED(i);
	i = t->csr;
	USED(i);
	t->fiforst = 0;
	print("taxi reset\n");

	i = t->fifocsr;
	if((i&Xmit_flag) == 0)
		print("taxiinit: Xmit fifo flags %lux\n", i & 0xff);

	t->txoffwr = CHUNK;	/* tx fifo threshold in words */
	/*
	 * Off by one error !
	 */
	t->rcvoffwr = CHUNK-1;
	print("taxi fifo block size %d words\n", CHUNK);
	print("csr misc =%lux\n", &t->csr);
}

void
irq5(uchar vector)
{
	VIC->irq5 = vector;
	while(VIC->irsr & (1<<5))	/* Loop until I5 is clear, so we can post */
		;
	VIC->irsr = (1<<5) | 1;		/* Post the interrupt */
}

ulong
swapl(ulong v)
{
	return	(v>>24) |
		((v>>8)&0xFF00) |
		((v<<8)&0xFF0000) |
		(v<<24);
}
