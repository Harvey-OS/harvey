#include <u.h>
#include <libc.h>

#define	 SFX_INV 1
#define	 SFX_ENB 2
#define	 SFX_SET 4
#define	 SFX_RESET 8
#define	 SFX_CLK  16
#define	 SFX_EOL  32
#define  SFX_NCN  64

#define BIG 10000
#define CANTFIT 10
#define BUFSZ 1000
#define	HSHSIZ	2000
#define NIO	1250

#define NPKG	10
#define NITER	5
#define DEVNOUTPINS	1000

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
	short	type; /* see _TYPEs below */
	short	nminterms;
};

struct device {
	short	next;
	short	npins;
	short	nouts;
	short	nios;
	Pin	*pins;
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

#define	NIL(type)	((type*)0)

#define STRLEN 128
#define FALSE 0
#define TRUE (!0)
#define MAXPIN 10000
#define MAXARRAY 10
#define MAXCELLS 25

#define USED_FLAG 0x1
#define COMPLEMENT_FIRST 0x2
#define DOUBLE_FLAG 0x4
#define PIN_FLAG 0x8
#define FUSE_FLAG 0x10
#define OCCUPIED_FLAG 0x200

#define NO_TYPE 0x0
#define SET_TYPE 0x1
#define RESET_TYPE 0x2
#define CLOCK_TYPE 0x4
#define BURIED_TYPE 0x8
#define ENABLE_TYPE 0x10
#define CLOCKED_TYPE 0x20
#define COMB_TYPE 0x40
#define INPUT_TYPE 0x80
#define OUTPUT_TYPE 0x100
#define ENABLED_TYPE 0x200
#define VCC_TYPE 0x400
#define GND_TYPE 0x800
#define INTERNAL_TYPE 0x1000
#define EXTERNAL_TYPE 0x2000
#define INVERTED_TYPE 0x4000
#define NONINVERTED_TYPE 0x8000

#define PINPUT INPUT_TYPE

struct ARRAY {
	char *name;
	int offset;
	int max_inputs;
	int max_outputs;
};

struct CELL {
	char *name;
	int mask;
	int type;
	int enable;
	int clock;
	int clocked; /* a.k.a. registered */
	int reset;
	int set;
	int invert;
};

struct PIN {
	unsigned short int flags;
	unsigned short int properties;
	unsigned short int max_term;
	short int input_line;
	short int output_line;
	short int index;
	short int first, last;
	short int macrocell;
};

#define maximum(x,y) ((x) > (y)) ? (x) : (y)

#define BADEND exit(1)
#define GOODEND exit(0)
