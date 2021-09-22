/*
 * process inbound multiboot table, including memory map and options.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

typedef struct Mbi Mbi;
struct Mbi {
	u32int	flags;
	u32int	memlower;
	u32int	memupper;
	u32int	bootdevice;
	u32int	cmdline;
	u32int	modscount;
	u32int	modsaddr;
	u32int	syms[4];
	u32int	mmaplength;
	u32int	mmapaddr;
	u32int	driveslength;
	u32int	drivesaddr;
	u32int	configtable;
	u32int	bootloadername;
	u32int	apmtable;
	u32int	vbe[6];
};

enum {						/* flags */
	Fmem		= 0x0001,		/* mem* valid */
	Fbootdevice	= 0x0002,		/* bootdevice valid */
	Fcmdline	= 0x0004,		/* cmdline valid */
	Fmods		= 0x0008,		/* mod* valid */
	Fsyms		= 0x0010,		/* syms[] has a.out info */
	Felf		= 0x0020,		/* syms[] has ELF info */
	Fmmap		= 0x0040,		/* mmap* valid */
	Fdrives		= 0x0080,		/* drives* valid */
	Fconfigtable	= 0x0100,		/* configtable* valid */
	Fbootloadername	= 0x0200,		/* bootloadername* valid */
	Fapmtable	= 0x0400,		/* apmtable* valid */
	Fvbe		= 0x0800,		/* vbe[] valid */
};

typedef struct Mod Mod;
struct Mod {
	u32int	modstart;
	u32int	modend;
	u32int	string;
	u32int	reserved;
};

typedef struct MMap MMap;
struct MMap {
	u32int	size;
	u32int	base[2];
	u32int	length[2];
	u32int	type;
};

static void
rdmbimmap(Mbi *mbi, int vflag)
{
	int n;
	MMap *mmap;
	u64int addr, len;

	mmap = KADDR(mbi->mmapaddr);
	n = 0;
	while(n < mbi->mmaplength){
		addr = (((u64int)mmap->base[1])<<32)|mmap->base[0];
		len = (((u64int)mmap->length[1])<<32)|mmap->length[0];
		if (vflag) {
			print("%#16.16llux %#16.16llux %15,llud ",
				addr, addr+len, len);
			switch(mmap->type){
			default:
				print("type %ud", mmap->type);
				break;
			case AsmNONE:
				print("none");
				break;
			case AsmDEV:
				print("device");
				break;
			case AsmMEMORY:
				print("Memory");
				break;
			case AsmRESERVED:
				print("reserved");
				break;
			case AsmACPIRECLAIM:
				print("ACPI Reclaim Memory");
				break;
			case AsmACPINVS:
				print("ACPI NVS Memory");
				break;
			}
			print("\n");
		} else
			switch(mmap->type){
			case AsmMEMORY:
			case AsmRESERVED:
			case AsmACPIRECLAIM:
			case AsmACPINVS:
				asmmapinit(addr, len, mmap->type);
				break;
			}

		n += mmap->size + sizeof(mmap->size);
		mmap = KADDR(mbi->mmapaddr + n);
	}
}

/* if vflag, print what would be done, but don't do it. */
int
multiboot(u32int magic, u32int pmbi, int vflag)
{
	char *p;
	int i;
	Mbi *mbi;
	Mod *mod;

	if(vflag)
		print("magic %#ux pmbi %#ux\n", magic, pmbi);
	if(magic != 0x2badb002 || pmbi == 0)
		return -1;

	mbi = KADDR(pmbi);
	if(vflag)
		print("flags %#ux\n", mbi->flags);
	if(mbi->flags & Fcmdline){
		p = KADDR(mbi->cmdline);
		if(vflag)
			print("cmdline <%s>\n", p);
		else
			mboptinit(p);
	}
	if(mbi->flags & Fmods)
		for(i = 0; i < mbi->modscount; i++){
			mod = KADDR(mbi->modsaddr + i*16);
			p = (mod->string != 0? KADDR(mod->string): "");
			if(vflag)
				print("mod %#ux %#ux <%s>\n",
					mod->modstart, mod->modend, p);
			else
				asmmodinit(mod->modstart, mod->modend, p);
		}
	if(mbi->flags & Fmmap)
		rdmbimmap(mbi, vflag);
	if(vflag && mbi->flags & Fbootloadername)
		print("bootloadername <%s>\n", KADDR(mbi->bootloadername));
	return 0;
}
