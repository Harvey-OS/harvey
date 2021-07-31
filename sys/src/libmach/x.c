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
	"3210",
	M3210,		/* machine type */
	0,		/* register list */
	0,
	0,
	0,
	0,
	0,
	0,		
	0x1000,		/* page size */
	0x0,		/* kernel base */
	0,		/* kernel text mask */
	0,		/* offset of ksp in /proc/proc */
	4,		/* correction to ksp value */
	4,		/* offset of kpc in /proc/proc */
	0,		/* correction to kpc value */
	0,		/* offset in ublk of sys call # */
	4,		/* quantization of pc */
	"setSB",	/* static base register name */
	0,		/* value */
	4,		/* szaddr */
	4,		/* szreg */
	4,		/* szfloat */
	4,		/* szdouble */
};
