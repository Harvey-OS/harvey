#include <u.h>
#include <libc.h>
#include <stdio.h>

#define	 SFX_INV 1
#define	 SFX_ENB 2
#define	 SFX_SET 4
#define	 SFX_CLR 8
#define	 SFX_CLK  16
#define	 SFX_EOL  32
#define  SFX_NCN  64

#define BIG 10000
#define CANTFIT 10
#define BUFSZ 1000
#define	HSHSIZ	2000
#define NIO	125
#define DEVNAME	"PAL26V12"
#define DEVCLKPIN	1
#define DEVNOUTPINS	12
#define DEVNPINS	26

/* types */
#define OUTPUT 1
#define INPUT 2
#define CLOCK 4
#define VCC 8

#define GND 16
#define CLOCKED 32
#define ENABLED 64
#define	COMB	128

#define CLR	256
#define SET	512
#define NOCONN  1024
#define INV	2048

#define NONINV	4096
#define ENABLE	8192
#define PINPUT 16384

#define PMASK	0xfcff

#define CCIO CLOCKED|COMB|INPUT|OUTPUT|INV|NONINV|PINPUT

#define NPKG	10
#define NITER	5

typedef	struct	hshtab	Hshtab;
typedef struct  depend	Depend;
typedef struct	package	Package;
typedef struct  device Device;
typedef struct  pin Pin;

struct pinassgn {
	short type;
	Hshtab * name;
};

struct pin {
	short type; /* INPUT OUTPUT CLOCK VCC GND */
	short	nminterms;
	char * clk;
	char * cmb;
	char * clkinv;
	char * cmbinv;
};

struct device {
	char	*name;
	char	*pkgtype;
	short	npins;
	short	nouts;
	short	nios;
	Pin	*pins;
};
/* one = 0:0 */
#define p22v10clk ".o 2%2.2d\n 0:0\n.o 3%2.2d\n"
#define p22v10cmb ".o 2%2.2d\n 0:0\n.o 3%2.2d\n 0:0\n"
#define p22v10clkinv ".o 2%2.2d\n.o 3%2.2d\n"
#define p22v10cmbinv ".o 2%2.2d\n.o 3%2.2d\n 0:0\n"

Pin p18v10[] = {
	PINPUT|CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 9 */
	GND, 0,		"",	"",	"",	"",	/* 10 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 11 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 12 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 13 */
	CCIO, 10,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 14 */
	CCIO, 10,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 15 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 16 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 17 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 18 */
	CCIO, 8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 19 */
	VCC, 0,	"",	"",	"",	"",	/* 24 */
	CLR, 0,	"",	"",	"",	"",	/* 25 */
	SET, 0,	"",	"",	"",	""	/* 26 */
};

Pin p22v10[] = {
	PINPUT|CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	PINPUT, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	GND, 0,		"",	"",	"",	"",	/* 12 */
	PINPUT, 0,	"",	"",	"",	"",	/* 13 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 14 */
	CCIO, 10,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 15 */
	CCIO, 12,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 16 */
	CCIO, 14,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 17 */
	CCIO, 16,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 18 */
	CCIO, 16,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 19 */
	CCIO, 14,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 20 */
	CCIO, 12,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 21 */
	CCIO, 10,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 22 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 23 */
	VCC, 0,	"",	"",	"",	"",	/* 24 */
	CLR, 0,	"",	"",	"",	"",	/* 25 */
	SET, 0,	"",	"",	"",	""	/* 26 */
};

Device	dev18v10 = {
		"PAL18V10",
		"DIP20	10	20",
		22,
		10,
		18,
		p18v10
};

Device	dev22v10 = {
		"PAL22V10",
		"DIP2403	12	24",
		26,
		10,
		22,
		p22v10
};

Pin p26cv12[] = {
	PINPUT|CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	VCC,   0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	PINPUT, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	PINPUT, 0,	"",	"",	"",	"",	/* 12 */
	PINPUT, 0,	"",	"",	"",	"",	/* 13 */
	PINPUT, 0,	"",	"",	"",	"",	/* 14 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 15 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 16 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 17 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 18 */
	CCIO, 10,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 19 */
	CCIO, 12,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 20 */
	GND, 0,	"",	"",	"",	"",	/* 21 */
	CCIO, 12,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 22 */
	CCIO, 10,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 23 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 24 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 25 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 26 */
	CCIO,  8,p22v10clk, p22v10cmb, p22v10clkinv, p22v10cmbinv, 	/* 27 */
	PINPUT, 0,	"",	"",	"",	"",	/* 28 */
	CLR, 0,	"",	"",	"",	"",	/* 29 */
	SET, 0,	"",	"",	"",	""	/* 30 */
};

Device	dev26cv12 = {
		"PAL26CV12",
		"DIP2803	21	7",
		28,
		12,
		26,
		p26cv12
};

Pin p20l8[] = {
	PINPUT, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	PINPUT, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	GND, 0,	"",	"",	"",	"",	/* 12 */
	PINPUT, 0,	"",	"",	"",	"",	/* 13 */
	PINPUT, 0,	"",	"",	"",	"",	/* 14 */
	OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 15 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 16 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 17 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 18 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 19 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 20 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 21 */
	OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 22 */
	PINPUT, 0,	"",	"",	"",	"",	/* 23 */
	VCC,   0,	"",	"",	"",	"",	/* 24 */
};

Device	dev20l8 = {
		"PAL20L8",
		"DIP2403	12	24",
		24,
		8,
		22,
		p20l8
};

Pin p16l8[] = {
	PINPUT, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	GND, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 12 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 13 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 14 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 15 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 16 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 17 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 18 */
	OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 19 */
	VCC,   0,	"",	"",	"",	"",	/* 20 */
};

Device	dev16l8 = {
		"PAL16L8",
		"DIP20	10	20",
		20,
		8,
		18,
		p16l8
};

Pin p48n22[] = {
	PINPUT, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	VCC,   0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	PINPUT, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	PINPUT, 0,	"",	"",	"",	"",	/* 12 */
	PINPUT, 0,	"",	"",	"",	"",	/* 13 */
	GND,	0,	"",	"",	"",	"",	/* 14 */
	GND,	0,	"",	"",	"",	"",	/* 15 */
	PINPUT, 0,	"",	"",	"",	"",	/* 16 */
	PINPUT, 0,	"",	"",	"",	"",	/* 17 */
	PINPUT, 0,	"",	"",	"",	"",	/* 18 */
	PINPUT, 0,	"",	"",	"",	"",	/* 19 */
	PINPUT, 0,	"",	"",	"",	"",	/* 20 */
	VCC,   0,	"",	"",	"",	"",	/* 21 */
	PINPUT, 0,	"",	"",	"",	"",	/* 22 */
	PINPUT, 0,	"",	"",	"",	"",	/* 23 */
	PINPUT, 0,	"",	"",	"",	"",	/* 24 */
	PINPUT, 0,	"",	"",	"",	"",	/* 25 */
	PINPUT, 0,	"",	"",	"",	"",	/* 26 */
	PINPUT, 0,	"",	"",	"",	"",	/* 27 */
	PINPUT, 0,	"",	"",	"",	"",	/* 28 */
	PINPUT, 0,	"",	"",	"",	"",	/* 29 */
	PINPUT, 0,	"",	"",	"",	"",	/* 30 */
	PINPUT, 0,	"",	"",	"",	"",	/* 31 */
	GND, 	0,	"",	"",	"",	"",	/* 32 */
	PINPUT, 0,	"",	"",	"",	"",	/* 33 */
	PINPUT, 0,	"",	"",	"",	"",	/* 34 */
	PINPUT, 0,	"",	"",	"",	"",	/* 35 */
	PINPUT, 0,	"",	"",	"",	"",	/* 36 */
	PINPUT, 0,	"",	"",	"",	"",	/* 37 */
	VCC,   0,	"",	"",	"",	"",	/* 38 */
	PINPUT, 0,	"",	"",	"",	"",	/* 39 */
	PINPUT, 0,	"",	"",	"",	"",	/* 40 */
	PINPUT, 0,	"",	"",	"",	"",	/* 41 */
	PINPUT, 0,	"",	"",	"",	"",	/* 42 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"",	/* 43 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"",	/* 44 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 45 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 46 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 47 */
	GND, 	0,	"",	"",	"",	"",	/* 48 */
	GND, 	0,	"",	"",	"",	"",	/* 49 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 50 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 51 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 52 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"",	/* 53 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 54 */
	VCC,   0,	"",	"",	"",	"",	/* 55 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 56 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 57 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 58 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 59 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 60 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 61 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 62 */
	OUTPUT|COMB|INV, 1,	"",	"",	"",	"", 	/* 63 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 12,	"",	"",	"",	"", 	/* 64 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 65 */
	GND, 0,	"",	"",	"",	"",	/* 66 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 67 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"", 	/* 68 */
};

Device	dev48n22 = {
		"PAL48N22",
		"PLCC68",
		68,
		22,
		48,
		p48n22
};

Pin p16r6[] = {
	CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	GND, 0,	"",	"",	"",	"",	/* 10 */
	ENABLE, 0,	"",	"",	"",	"",	/* 11 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 12 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 13 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 14 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 15 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 16 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 17 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 18 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 19 */
	VCC,   0,	"",	"",	"",	"",	/* 20 */
};

Device	dev16r6 = {
		"PAL16R6",
		"DIP20	10	20",
		20,
		8,
		18,
		p16r6
};

Pin p20r4[] = {
	CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	PINPUT, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	GND, 0,	"",	"",	"",	"",	/* 12 */
	ENABLE, 0,	"",	"",	"",	"",	/* 13 */
	PINPUT, 0,	"",	"",	"",	"",	/* 14 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 15 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 16 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 17 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 18 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 19 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 20 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 21 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 22 */
	PINPUT, 0,	"",	"",	"",	"",	/* 23 */
	VCC,   0,	"",	"",	"",	"",	/* 24 */
};

Device	dev20r4 = {
		"PAL20R4",
		"DIP2403	12	24",
		24,
		8,
		22,
		p20r4
};

Pin p16r4[] = {
	CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	GND, 0,	"",	"",	"",	"",	/* 10 */
	ENABLE, 0,	"",	"",	"",	"",	/* 11 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 12 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 13 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 14 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 15 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 16 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 17 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 18 */
	PINPUT|INPUT|OUTPUT|COMB|INV, 7,	"",	"",	"",	"",	/* 19 */
	VCC,   0,	"",	"",	"",	"",	/* 20 */
};

Device	dev16r4 = {
		"PAL16R4",
		"DIP20	10	20",
		20,
		8,
		18,
		p16r4
};

Pin p16r8[] = {
	CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	PINPUT, 0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	GND, 0,	"",	"",	"",	"",	/* 10 */
	ENABLE, 0,	"",	"",	"",	"",	/* 11 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 12 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 13 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"",	/* 14 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 15 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 16 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 17 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 18 */
	INPUT|OUTPUT|CLOCKED|INV|ENABLED, 8,	"",	"",	"",	"", 	/* 19 */
	VCC,   0,	"",	"",	"",	"",	/* 20 */
};

Device	dev16r8 = {
		"PAL16R8",
		"DIP20	10	20",
		20,
		8,
		18,
		p16r8
};

#define p26v12clk ".o 2%2.2d\n 0:0\n.o 3%2.2d\n.o 4%2.2d\n 0:0\n.o 5%2.2d\n"
#define p26v12cmb ".o 2%2.2d\n 0:0\n.o 3%2.2d\n 0:0\n.o 4%2.2d\n 0:0\n.o 5%2.2d\n 0:0\n"
#define p26v12clkinv ".o 2%2.2d\n.o 3%2.2d\n.o 4%2.2d\n 0:0\n.o 5%2.2d\n 0:0\n"
#define p26v12cmbinv ".o 2%2.2d\n.o 3%2.2d\n 0:0\n.o 4%2.2d\n 0:0\n.o 5%2.2d\n 0:0\n"

Pin p26v12[] = {
	PINPUT|CLOCK, 0,	"",	"",	"",	"",	/* 1 */
	PINPUT, 0,	"",	"",	"",	"",	/* 2 */
	PINPUT, 0,	"",	"",	"",	"",	/* 3 */
	PINPUT, 0,	"",	"",	"",	"",	/* 4 */
	PINPUT, 0,	"",	"",	"",	"",	/* 5 */
	PINPUT, 0,	"",	"",	"",	"",	/* 6 */
	VCC,   0,	"",	"",	"",	"",	/* 7 */
	PINPUT, 0,	"",	"",	"",	"",	/* 8 */
	PINPUT, 0,	"",	"",	"",	"",	/* 9 */
	PINPUT, 0,	"",	"",	"",	"",	/* 10 */
	PINPUT, 0,	"",	"",	"",	"",	/* 11 */
	PINPUT, 0,	"",	"",	"",	"",	/* 12 */
	PINPUT, 0,	"",	"",	"",	"",	/* 13 */
	PINPUT, 0,	"",	"",	"",	"",	/* 14 */
	CCIO,  8,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 15 */
	CCIO,  8,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 16 */
	CCIO, 10,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 17 */
	CCIO, 12,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 18 */
	CCIO, 14,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 19 */
	CCIO, 16,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 20 */
	GND, 0,	"",	"",	"",	"",	/* 21 */
	CCIO, 16,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 22 */
	CCIO, 14,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 23 */
	CCIO, 12,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 24 */
	CCIO, 10,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 25 */
	CCIO,  8,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 26 */
	CCIO,  8,p26v12clk, p26v12cmb, p26v12clkinv, p26v12cmbinv, 	/* 27 */
	PINPUT, 0,	"",	"",	"",	"",	/* 28 */
	CLR, 0,	"",	"",	"",	"",	/* 29 */
	SET, 0,	"",	"",	"",	""	/* 30 */
};

Device	dev26v12 = {
		"PAL26V12",
		"DIP2803	21	7",
		30,
		12,
		26,
		p26v12
};


struct package
{
	Device * type;
	Hshtab *outputs[DEVNOUTPINS];
	char index[DEVNOUTPINS];
};

struct depend 
{
	Depend	*next;
	short	type;
	Hshtab	*name;
};

struct hshtab
{
	short	type;
	short	mypkg;
	char	*name;
	short	ndepends;
	Depend *depends;
	short	nminterm;
	Depend *minterms;
	Hshtab **ins;
	short	nins;
	Hshtab *clock;
	Hshtab *enab;
	Hshtab *clr;
	Hshtab *set;
};

Hshtab	hshtab[HSHSIZ];
int hshused;

Package *packages[NPKG];
int	npackages;

Hshtab *outputs[NIO];
int	noutputs;

struct
{
	char *name;
	Device * device;
} types[] = {

		"22V10",	&dev22v10,
		"26CV12",	&dev26cv12,
		"18V10",	&dev18v10,
		"26V12",	&dev26v12,
		"16R8",		&dev16r8,
		"16R6",		&dev16r6,
		"16R4",		&dev16r4,
		"20R4",		&dev20r4,
		"16L8",		&dev16l8,
		"48N22",	&dev48n22,
		"20L8",		&dev20l8,
		(char *) 0,	(Device *) 0
	};

#define	NIL(type)	((type*)0)

#ifndef PLAN9
void srand(long);
long time(long *);
int rand(void);
#define GOODEND exit(0)
#define BADEND exit(1)
#define OREAD 0
#define OWRITE 1
#define create($1,$2,$3) creat($1,$3)
#else
void srand(int);
int time(long *);
int rand(void);
#define GOODEND exits("");
#define BADEND exits("ERROR");
#endif
void putmins(FILE*, Hshtab *, int);
int findpin(Hshtab *);
void rdfile(char *);
char *nextwd(char *, char *, int *);
void out(int);
Hshtab* lookup(char *, int);
int adddep(Depend **, Hshtab *, short);
char * f_strdup(char *);
char * lmalloc(long);
void outp(int);
void init(void);
Package * newpkg(Device *);
void printpkg(void);
int countpins(Package *);
void enter(Depend **, Hshtab *, short);
int remov(Depend **, Hshtab *, short);
int issuf(char *, char *);
int improve(int,int);
void swap(int,int);
void uswap(int,int);
int fit(int,int);
int find(short, short, short, Hshtab *);
void outm(int);
void jstart(void);
void outj(int );
void outwire(char *, int, int, int, int, int, int, int);
void jend(void);
int pick(int);
void shuffle(void);
int bwk(int, int);
Hshtab *getenab(Hshtab *);
void gettype(char *);
int assign(int, int);
void assigned(void);
void passign(int, FILE*);
void readp(FILE*);
Depend *getmins(char *, Hshtab **, int);
void cantdo(int);
