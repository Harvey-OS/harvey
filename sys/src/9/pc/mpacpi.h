/*
 * ACPI definitions
 *
 * A System Descriptor Table starts with a header of 4 bytes of signature
 * followed by 4 bytes of total table length then 28 bytes of ID information
 * (including the table checksum).
 */
typedef struct Dsdt Dsdt;
typedef struct Facp Facp;
typedef struct Hpet Hpet;
typedef struct Madt Madt;
typedef struct Mcfg Mcfg;
typedef struct Mcfgd Mcfgd;
typedef struct Rsd Rsd;

struct Dsdt {				/* Differentiated System DT */
	uchar	sdthdr[36];		/* "DSDT" + length[4] + [28] */
	uchar	db[];			/* Definition Block */
};
struct Facp {				/* Fixed ACPI DT */
	uchar	sdthdr[36];		/* "FACP" + length[4] + [28] */
	uchar	faddr[4];		/* Firmware Control Address */
	uchar	dsdt[4];		/* DSDT Address */
	uchar	pad[200];		/* total table is 244 */
};
struct Hpet {				/* High-Precision Event Timer DT */
	uchar	sdthdr[36];		/* "HPET" + length[4] + [28] */
	uchar	id[4];			/* Event Timer Block ID */
	uchar	addr[12];		/* ACPI Format Address */
	uchar	seqno;			/* Sequence Number */
	uchar	minticks[2];		/* Minimum Clock Tick */
	uchar	attr;			/* Page Protection */
};
struct Madt {				/* Multiple APIC DT */
	uchar	sdthdr[36];		/* "MADT" + length[4] + [28] */
	uchar	addr[4];		/* Local APIC Address */
	uchar	flags[4];
	uchar	structures[];
};
typedef struct Mcfg {			/* PCI Memory Mapped Config */
	uchar	sdthdr[36];		/* "MCFG" + length[4] + [28] */
	uchar	pad[8];			/* reserved */
	Mcfgd	mcfgd[];		/* descriptors */
} Mcfg;
struct Mcfgd {				/* MCFG Descriptor */
	uchar	addr[8];		/* base address */
	uchar	segno[2];		/* segment group number */
	uchar	sbno;			/* start bus number */
	uchar	ebno;			/* end bus number */
	uchar	pad[4];			/* reserved */
};
struct Rsd {				/* Root System Description * */
	uchar	signature[8];		/* "RSD PTR " */
	uchar	rchecksum;
	uchar	oemid[6];
	uchar	revision;
	uchar	raddr[4];		/* RSDT */
	uchar	length[4];
	uchar	xaddr[8];		/* XSDT */
	uchar	xchecksum;		/* XSDT */
	uchar	pad[3];			/* reserved */
};
