#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"io.h"
#include	"../port/error.h"

/*
 * stubs to mostly avoid acpi.
 */
typedef struct Srat Srat;

enum {
	/* SRAT types */
	SRlapic = 0,	/* Local apic/sapic affinity */
	SRmem,		/* Memory affinity */
	SRlx2apic,	/* x2 apic affinity */
};

/*
 * ACPI System resource affinity table
 */
struct Srat
{
	int	type;
	Srat*	next;
	union{
		struct{
			int	dom;	/* proximity domain */
			int	apic;	/* apic id */
			int	sapic;	/* sapic id */
			int	clkdom;	/* clock domain */
		} lapic;
		struct{
			int	dom;	/* proximity domain */
			u64int	addr;	/* base address */
			u64int	len;
			int	hplug;	/* hot pluggable */
			int	nvram;	/* non volatile */
		} mem;
		struct{
			int	dom;	/* proximity domain */
			int	apic;	/* x2 apic id */
			int	clkdom;	/* clock domain */
		} lx2apic;
	};
};

static Srat*	srat;	/* System resource affinity, used by physalloc */

/*
 * Return a number identifying a color for the memory at
 * the given address (color identifies locality) and fill *sizep
 * with the size for memory of the same color starting at addr.
 */
int
memcolor(uintmem addr, uintmem *sizep)
{
	Srat *sl;

	for(sl = srat; sl != nil; sl = sl->next)
		if(sl->type == SRmem)
		if(sl->mem.addr <= addr && addr-sl->mem.addr < sl->mem.len){
			if(sizep != nil)
				*sizep = sl->mem.len - (addr - sl->mem.addr);
			if(sl->mem.dom >= NCOLOR)
				print("memcolor: NCOLOR too small");
			return sl->mem.dom % NCOLOR;
		}
	return -1;
}

/*
 * Being machno an index in sys->machptr, return the color
 * for that mach (color identifies locality).
 */
int
corecolor(int machno)
{
	Srat *sl;
	Mach *m;
	static int colors[MACHMAX];

	if(machno < 0 || machno >= MACHMAX)
		return -1;
	m = sys->machptr[machno];
	if(m == nil)
		return -1;

	if(machno >= 0 && machno < nelem(colors) && colors[machno] != 0)
		return colors[machno] - 1;

	for(sl = srat; sl != nil; sl = sl->next)
		if(sl->type == SRlapic && sl->lapic.apic == m->apicno){
			if(machno >= 0 && machno < nelem(colors))
				colors[machno] = 1 + sl->lapic.dom;
			if(sl->lapic.dom >= NCOLOR)
				print("corecolor: NCOLOR too small");
			return sl->lapic.dom % NCOLOR;
		}
	return -1;
}
