#define NVSIZE	256
#define PWDLEN	8
#define KEYLEN	(2*PWDLEN+1)
#define OFF(x)	((ulong)((Nvram *)0)->x)

typedef struct Nvram_ro	Nvram_ro;
typedef struct Nvram_rw	Nvram_rw;
typedef union Nvram	Nvram;

struct Nvram_rw
{
	uchar	checksum[1];
	uchar	revision[1];
	uchar	console[2];
	uchar	syspart[48];	/* system partition */
	uchar	osloader[18];	/* OS loader */
	uchar	osfile[28];	/* OS load filename */
	uchar	osopts[12];	/* OS load options */
	uchar	pgcolor[6];	/* color for textport background */
	uchar	lbaud[5];	/* initial baud rates for the duart */
	uchar	diskless[1];	/* are we running as a diskless station */
	uchar	oldeaddr[6];	/* old location of the ethernet address */
	uchar	tz[8];		/* timezone - number of hours behind GMT */
	uchar	ospart[48];	/* OS load partition */
	uchar	autoload[1];	/* autoload - 'Y' or 'N' */
	uchar	diagmode[2];	/* diagnostic report level/ide actions */
	uchar	netaddr[4];	/* IP address */
	uchar	nokbd[1];	/* allowed to boot without a keyboard */
	uchar	keybd[5];	/* overrides key map for the returned keyboard */
	uchar	lang[6];	/* language desired (format: en_USA) */
	uchar	pwdkey[KEYLEN];	/* encrypted key to protect manual mode */
	uchar	netkey[KEYLEN];	/* encrypted key to protect network access */
	uchar	scsirt[1];	/* no. of scsi retries when probing */
	uchar	volume[3];	/* default audio volume */
	uchar	scsiid[1];	/* host adapter id */
	uchar	sgilogo[1];	/* add the SGI product logo to the screen */
	uchar	nogui[1];	/* texport is just like a tty */
	uchar	rbaud[1];	/* encoded as x*1200 */
	uchar	free[5];
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
