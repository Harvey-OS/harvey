#include <u.h>
#include <libc.h>

#define STRLEN 128
#define FALSE 0
#define TRUE (!0)
#define MAXPIN 10000
#define MAXARRAY 10
#define MAXCELLS 25

#define STX 002
#define ETX 003

#define AND_SECTION 1
#define OR_SECTION 2
#define FUSE_SECTION 3
#define SWITCH_SECTION 4
#define INPUT_SECTION 5
#define OUTPUT_SECTION 6

#define NIMP 100 /* maximum number of implicants */
#define NINPUTS 100 /* maximum number of inputs */

#define maximum(x,y) ((x) > (y)) ? (x) : (y)

struct	imp
{
	long	val;
	long	mask;
} imp[NIMP];

#define NTERM 32 /* == sizeof long */

#define USED_FLAG 0x1
#define COMPLEMENT_FIRST 0x2
#define DOUBLE_FLAG 0x4
#define PIN_FLAG 0x8
#define FUSE_FLAG 0x10
#define INTERNAL_FLAG 0x80
#define EXTERNAL_FLAG 0x100
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
