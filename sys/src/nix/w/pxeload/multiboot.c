#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

MMap mmap[20];
int nmmap;

Mbi multibootheader;

void
mkmultiboot(void)
{
	if(nmmap != 0){
		multibootheader.cmdline = PADDR(BOOTLINE);
		multibootheader.flags |= Fcmdline;
		multibootheader.mmapaddr = PADDR(mmap);
		multibootheader.mmaplength = nmmap*sizeof(MMap);
		multibootheader.flags |= Fmmap;
		if(vflag)
			print("&multibootheader %#p\n", &multibootheader);
	}
}
