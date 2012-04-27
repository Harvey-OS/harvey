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
	Fmem		= 0x00000001,		/* mem* valid */
	Fbootdevice	= 0x00000002,		/* bootdevice valid */
	Fcmdline	= 0x00000004,		/* cmdline valid */
	Fmods		= 0x00000008,		/* mod* valid */
	Fsyms		= 0x00000010,		/* syms[] has a.out info */
	Felf		= 0x00000020,		/* syms[] has ELF info */
	Fmmap		= 0x00000040,		/* mmap* valid */
	Fdrives		= 0x00000080,		/* drives* valid */
	Fconfigtable	= 0x00000100,		/* configtable* valid */
	Fbootloadername	= 0x00000200,		/* bootloadername* valid */
	Fapmtable	= 0x00000400,		/* apmtable* valid */
	Fvbe		= 0x00000800,		/* vbe[] valid */
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

int
multiboot(u32int magic, u32int pmbi, int vflag)
{
	char *p;
	int i, n;
	Mbi *mbi;
	Mod *mod;
	MMap *mmap;
	u64int addr, len;

	if(vflag)
		print("magic %#ux pmbi %#ux\n", magic, pmbi);
	if(magic != 0x2badb002)
		return -1;

	mbi = KADDR(pmbi);
	if(vflag)
		print("flags %#ux\n", mbi->flags);
	if(mbi->flags & Fcmdline){
		p = KADDR(mbi->cmdline);
		if(vflag)
			print("cmdline <%s>\n", p);
		else
			optionsinit(p);
	}
	if(mbi->flags & Fmods){
		for(i = 0; i < mbi->modscount; i++){
			mod = KADDR(mbi->modsaddr + i*16);
			if(mod->string != 0)
				p = KADDR(mod->string);
			else
				p = "";
			if(vflag)
				print("mod %#ux %#ux <%s>\n",
					mod->modstart, mod->modend, p);
			else
				asmmodinit(mod->modstart, mod->modend, p);
		}
	}
	if(mbi->flags & Fmmap){
		mmap = KADDR(mbi->mmapaddr);
		n = 0;
		while(n < mbi->mmaplength){
			addr = (((u64int)mmap->base[1])<<32)|mmap->base[0];
			len = (((u64int)mmap->length[1])<<32)|mmap->length[0];
			switch(mmap->type){
			default:
				if(vflag)
					print("type %ud", mmap->type);
				break;
			case 1:
				if(vflag)
					print("Memory");
				else
					asmmapinit(addr, len, mmap->type);
				break;
			case 2:
				if(vflag)
					print("reserved");
				else
					asmmapinit(addr, len, mmap->type);
				break;
			case 3:
				if(vflag)
					print("ACPI Reclaim Memory");
				else
					asmmapinit(addr, len, mmap->type);
				break;
			case 4:
				if(vflag)
					print("ACPI NVS Memory");
				else
					asmmapinit(addr, len, mmap->type);
				break;
			}
			if(vflag)
				print("\n\t%#16.16llux %#16.16llux (%llud)\n",
					addr, addr+len, len);

			n += mmap->size+sizeof(mmap->size);
			mmap = KADDR(mbi->mmapaddr+n);
		}
	}
	if(vflag && (mbi->flags & Fbootloadername)){
		p = KADDR(mbi->bootloadername);
		print("bootloadername <%s>\n", p);
	}

	return 0;
}
