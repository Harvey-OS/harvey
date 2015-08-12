/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	BusCBUS = 0, /* Corollary CBUS */
	BusCBUSII,   /* Corollary CBUS II */
	BusEISA,     /* Extended ISA */
	BusFUTURE,   /* IEEE Futurebus */
	BusINTERN,   /* Internal bus */
	BusISA,      /* Industry Standard Architecture */
	BusMBI,      /* Multibus I */
	BusMBII,     /* Multibus II */
	BusMCA,      /* Micro Channel Architecture */
	BusMPI,      /* MPI */
	BusMPSA,     /* MPSA */
	BusNUBUS,    /* Apple Macintosh NuBus */
	BusPCI,      /* Peripheral Component Interconnect */
	BusPCMCIA,   /* PC Memory Card International Association */
	BusTC,       /* DEC TurboChannel */
	BusVL,       /* VESA Local bus */
	BusVME,      /* VMEbus */
	BusXPRESS,   /* Express System Bus */
};

#define MKBUS(t, b, d, f) (((t) << 24) | (((b)&0xFF) << 16) | (((d)&0x1F) << 11) | (((f)&0x07) << 8))
#define BUSFNO(tbdf) (((tbdf) >> 8) & 0x07)
#define BUSDNO(tbdf) (((tbdf) >> 11) & 0x1F)
#define BUSBNO(tbdf) (((tbdf) >> 16) & 0xFF)
#define BUSTYPE(tbdf) ((tbdf) >> 24)
#define BUSBDF(tbdf) ((tbdf)&0x00FFFF00)
#define BUSUNKNOWN (-1)

/*
 * PCI support code.
 */
enum {		       /* type 0 and type 1 pre-defined header */
       PciVID = 0x00,  /* vendor ID */
       PciDID = 0x02,  /* device ID */
       PciPCR = 0x04,  /* command */
       PciPSR = 0x06,  /* status */
       PciRID = 0x08,  /* revision ID */
       PciCCRp = 0x09, /* programming interface class code */
       PciCCRu = 0x0A, /* sub-class code */
       PciCCRb = 0x0B, /* base class code */
       PciCLS = 0x0C,  /* cache line size */
       PciLTR = 0x0D,  /* latency timer */
       PciHDT = 0x0E,  /* header type */
       PciBST = 0x0F,  /* BIST */

       PciBAR0 = 0x10, /* base address */
       PciBAR1 = 0x14,

       PciINTL = 0x3C, /* interrupt line */
       PciINTP = 0x3D, /* interrupt pin */
};

enum { /* type 0 pre-defined header */
       PciBAR2 = 0x18,
       PciBAR3 = 0x1C,
       PciBAR4 = 0x20,
       PciBAR5 = 0x24,
       PciCIS = 0x28,   /* cardbus CIS pointer */
       PciSVID = 0x2C,  /* subsystem vendor ID */
       PciSID = 0x2E,   /* cardbus CIS pointer */
       PciEBAR0 = 0x30, /* expansion ROM base address */
       PciMGNT = 0x3E,  /* burst period length */
       PciMLT = 0x3F,   /* maximum latency between bursts */
};

enum {			/* type 1 pre-defined header */
       PciPBN = 0x18,   /* primary bus number */
       PciSBN = 0x19,   /* secondary bus number */
       PciUBN = 0x1A,   /* subordinate bus number */
       PciSLTR = 0x1B,  /* secondary latency timer */
       PciIBR = 0x1C,   /* I/O base */
       PciILR = 0x1D,   /* I/O limit */
       PciSPSR = 0x1E,  /* secondary status */
       PciMBR = 0x20,   /* memory base */
       PciMLR = 0x22,   /* memory limit */
       PciPMBR = 0x24,  /* prefetchable memory base */
       PciPMLR = 0x26,  /* prefetchable memory limit */
       PciPUBR = 0x28,  /* prefetchable base upper 32 bits */
       PciPULR = 0x2C,  /* prefetchable limit upper 32 bits */
       PciIUBR = 0x30,  /* I/O base upper 16 bits */
       PciIULR = 0x32,  /* I/O limit upper 16 bits */
       PciEBAR1 = 0x28, /* expansion ROM base address */
       PciBCR = 0x3E,   /* bridge control register */
};

typedef struct Pcidev Pcidev;
struct Pcidev {
	int tbdf;     /* type+bus+device+function */
	uint16_t vid; /* vendor ID */
	uint16_t did; /* device ID */
	uint8_t rid;  /* revision ID */

	struct {
		uint32_t bar; /* base address */
		int size;
	} mem[6];

	uint8_t intl; /* interrupt line */
	uint16_t ccru;

	Pcidev *list;
	Pcidev *bridge; /* down a bus */
	Pcidev *link;   /* next device on this bno */
};
