// Lml 22 driver

#define MJPG_VERSION "LML33 v0.2"
#define NLML 2

// The following values can be modified to tune/set default behaviour of the
// driver.

// The number of uS delay in I2C state transitions (probably >= 10)
#define I2C_DELAY 50

// The amount of spinning to do before the I2C bus is timed out
#define I2C_TIMEOUT 10000000

// The amount of spinning to do before the guest bus is timed out
#define GUEST_TIMEOUT 10000000

// The amount of spinning to do before the polling of the still
// transfer port is aborted.
#define STILL_TIMEOUT 1000000

// The following number is the maximum number of cards permited. Each
// card found is mapped to a device minor number starting from 0.
#define MAX_CARDS 1

// The following are the datastructures needed by the device.
#define I2C_BUS		0x044
// which bit of I2C_BUS is which
#define I2C_SCL		1
#define I2C_SDA		2
#define INTR_JPEGREP	0x08000000
#define INTR_GIRQ0	0x20000000
#define INTR_STAT	0x03c

// A Device records the properties of the various card types supported.
typedef struct {
	int	number;		// The H33_CARDTYPE_ assigned
	char	*card_name;	// A string name
	int	zr060addr;	// Which guest bus address for the ZR36060
} Device;

// The remainder of the #defs are constants which should not need changing.

// The PCI vendor and device ids of the zoran chipset on the dc30
// these really belong in pci.h
#define VENDOR_ZORAN		0x11de
#define ZORAN_36057		0x6057
#define ZORAN_36067		ZORAN_36057

#define BT819Addr 0x8a
#define BT856Addr 0x88

#define NBUF 4

#define FRAGM_FINAL_B 1
#define STAT_BIT 1

typedef struct	HdrFragment		HdrFragment;
typedef struct	FrameHeader		FrameHeader;
typedef union	Fragment		Fragment;
typedef struct	FragmentTable		FragmentTable;
typedef struct	CodeData		CodeData;

//If we're on the little endian architecture, then 0xFF, 0xD8 byte sequence is
#define MRK_SOI	0xD8FF
#define MRK_APP3	0xE3FF
#define APP_NAME	"LML"

struct FrameHeader {	// Don't modify this struct, used by h/w
	ushort mrkSOI;
	ushort mrkAPP3;
	ushort lenAPP3;
	char nm[4];
	ushort frameNo;
	vlong ftime;
	ulong frameSize;
	ushort frameSeqNo;
	ushort SOIfiller;
};

#define FRAGSIZE (128*1024)

union Fragment {
	FrameHeader	fh;
	char		fb[FRAGSIZE];
};

struct HdrFragment {
	uchar	hdr[sizeof(FrameHeader)];
	Fragment;
};

struct FragmentTable {	// Don't modify this struct, used by h/w
	ulong		addr;		// Physical address
	ulong		leng;
};

struct CodeData {	// Don't modify this struct, used by h/w
	ulong		pamjpg;		// Physical addr of statCom[0]
	ulong		pagrab;		// Physical addr of grab buffer
	ulong		statCom[4];	// Physical addresses of fragdescs
	FragmentTable	fragdesc[4];
	HdrFragment	frag[4];
};

enum{
	Codedatasize = (sizeof(CodeData) + BY2PG - 1) & ~(BY2PG - 1),
	Grabdatasize = (730 * 568 * 2 * 2 + BY2PG - 1) & ~(BY2PG - 1),
};

#define POST_OFFICE		0x200
#define POST_PEND		0x02000000
#define POST_TIME		0x01000000
#define POST_DIR		0x00800000

#define GID060	0
