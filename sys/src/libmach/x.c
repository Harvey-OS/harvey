/*
 * 3210 definition
 */
#include <u.h>
#include <bio.h>
#include <mach.h>

/*
 * 3210 has no /proc
 */
Mach m3210 =
{
	"3210",		/* machine name */
	M3210,		/* machine type */
	0,		/* register list */
	0,		/* size of register set in bytes */
	0,		/* size of fp register set in bytes */
	0,		/* name of PC */
	0,		/* name of SP */
	0,		/* name of link register */
	"setSB",	/* static base register name */
	0,		/* value */
	0x1000,		/* page size */
	0x0,		/* kernel base */
	0,		/* kernel text mask */
	4,		/* quantization of pc */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	4,		/* szdouble */
};
