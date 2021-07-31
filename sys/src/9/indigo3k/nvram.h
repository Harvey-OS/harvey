#define NVSIZE	128
#define PWDLEN	8
#define KEYLEN	(2*PWDLEN+1)
#define OFF(x)	((ulong)((Nvram *)0)->x)

typedef struct Nvram_ro	Nvram_ro;
typedef struct Nvram_rw	Nvram_rw;
typedef union Nvram	Nvram;

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
	uchar	pad1[NVSIZE-6];
	uchar	enet[6];	/* ethernet address */
};

union Nvram
{
	Nvram_rw;
	Nvram_ro;
};
