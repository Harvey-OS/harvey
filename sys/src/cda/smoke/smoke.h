
/*
 * General useful macros
 */

#undef signal
#include <u.h>
#include <libc.h>

#define STRLEN 128 /* Maximum string length (I hope) */
#define MAXPIN 256 /* Maximum pin number */
#define MAXPERNET 512 /* maximum number of pins on a net (I hope) */
#define MAXTYPES 1024 /* maximum number of types (I hope) [geez rae!] */
#define MAXFILES 64 /* Maximum number of files (I hope) */
#define INSTANCESIZE 1023
#define SIGNALSIZE 1023
#define ICSIZE 1023

#define NULL_STRUCT(type) (struct type *) 0
#define alloc_struct(type) (struct type *) calloc(1, sizeof (struct type))

#define loop for(;;)
#define Abs(x) ((x < 0) ? -x : x)

#define TRUE -1
#define FALSE 0

#ifndef NULL
#define NULL (char *) 0
#endif

/*
 * Data structures for the static checker
 */

struct pin
{
	struct pin *instance_chain;
	struct instance *instance;
	struct signal *signal;
	int pin_type_vector;
	int pin_number;
	unsigned char fileindex;
};

struct bucket
{
	char *name;
	struct bucket *next;
	union
	{
		struct signal *signal;
		struct instance *instance;
		struct chip *chip;
	} pointer;
};

struct instance
{
	struct chip *chip;
	struct bucket *bucket;
	struct instance *next;
	struct pin **pins;
};

struct chip
{
	struct chip *next, *chain;
	struct bucket *bucket;
	struct instance *first_instance;
	struct pin_definitions *pin_definitions;
};

struct pin_definitions
{
	int pin_count;
	char *pin_types;
	char **pin_names;
};

struct signal
{
	struct signal *next;
	struct pin *first_pin;
	struct bucket *bucket;
	int pin_count;
	int type;
};

/*
 * pin types
 */

#define OUTPUT_PIN 0x01
#define INPUT_PIN 0x02
#define BI_PIN 0x03
#define SOURCE_PIN 0x04
#define SINK_PIN 0x08
#define OC_PIN 0x10
#define TRISTATE_PIN 0x20
#define ANALOG_PIN 0x40
#define TOTEM_POLE_PIN 0x80
#define POWER_PIN 0x100
#define GROUND_PIN 0x200
#define SWITCH_PIN 0x400
#define TERMINATOR_PIN 0x800
#define ECL_PIN 0x1000
#define PULLUP_PIN 0x2000
#define IGNORE_PIN 0x4000
#define PLUS_PIN 0x8000
#define MINUS_PIN 0x10000
#define CURRENT_PIN 0x20000
#define DIGITAL_PIN 0x40000
#define FULL_PIN 0x7ffff

/*
 * signal types
 */

#define NO_TYPE 0x0
#define MACRO_SIGNAL 0x1
#define IGNORE_SIGNAL 0x2

/*
 * stolen from plan 9
 */

#ifndef PLAN9
#define	ARGBEGIN	for((argv0? 0: (argv0=*argv)),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt, _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				while(*_args) switch(_argc=*_args++)
#define	ARGEND		}
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc
char *argv0;
#endif
