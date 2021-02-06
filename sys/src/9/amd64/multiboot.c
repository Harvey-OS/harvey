/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

typedef struct Mbi Mbi;
struct Mbi {
	u32 flags;
	u32 memlower;
	u32 memupper;
	u32 bootdevice;
	u32 cmdline;
	u32 modscount;
	u32 modsaddr;
	u32 syms[4];
	u32 mmaplength;
	u32 mmapaddr;
	u32 driveslength;
	u32 drivesaddr;
	u32 configtable;
	u32 bootloadername;
	u32 apmtable;
	u32 vbe[6];
};

enum {				     /* flags */
       Fmem = 0x00000001,	     /* mem* valid */
       Fbootdevice = 0x00000002,     /* bootdevice valid */
       Fcmdline = 0x00000004,	     /* cmdline valid */ 
       Fmods = 0x00000008,	     /* mod* valid */
       Fsyms = 0x00000010,	     /* syms[] has a.out info */
       Felf = 0x00000020,	     /* syms[] has ELF info */
       Fmmap = 0x00000040,	     /* mmap* valid */
       Fdrives = 0x00000080,	     /* drives* valid */
       Fconfigtable = 0x00000100,    /* configtable* valid */
       Fbootloadername = 0x00000200, /* bootloadername* valid */
       Fapmtable = 0x00000400,	     /* apmtable* valid */
       Fvbe = 0x00000800,	     /* vbe[] valid */
};

typedef struct Mod Mod;
struct Mod {
	u32 modstart;
	u32 modend;
	u32 string;
	u32 reserved;
};

typedef struct MMap MMap;
struct MMap {
	u32 size;
	u32 base[2];
	u32 length[2];
	u32 type;
};

// TODO rename and recomment
// ModDir maps MBI Mod structs to Dir structs after being added to devarch
typedef struct ModDir ModDir;
struct ModDir {
	Qid *qid;
	void *data;
	size_t len;
};
enum {
	MaxNumModDirs = 4,
};
static int nummoddirs = 0;
static ModDir moddirs[MaxNumModDirs] = {0};

static int
mbpamtype(int acpitype)
{
	switch(acpitype){
	case 1:
		return PamMEMORY;
	case 2:
		return PamRESERVED;
	case 3:
		return PamACPI;
	case 4:
		return PamPRESERVE;
	case 5:
		return PamUNUSABLE;
	default:
		print("multiboot: unknown memory type %d", acpitype);
		break;
	}
	return PamNONE;
}

static const char *
mbtypename(int type)
{
	switch(type){
	case 1:
		return "Memory";
	case 2:
		return "Reserved";
	case 3:
		return "ACPI Reclaim Memory";
	case 4:
		return "ACPI NVS Memory";
	default:
		break;
	}
	return "(unknown)";
}

static i32
mbmoduleread(Chan *c, void *buf, i32 len, i64 off)
{
	ModDir *mod = nil;
	for (int i = 0; i < nummoddirs; i++) {
		if (moddirs[i].qid->path == c->qid.path) {
			mod = &moddirs[i];
			break;
		}
	}
	if (!mod) {
		error("mbmoduleread: module not found");
	}

	if (off < 0 || off > mod->len) {
		error("mbmoduleread: offset out of bounds");
	}
	print("mbmoduleread: mod->data: %p mod->len: %d buf: %p, len: %d, off: %lld\n", mod->data, mod->len, buf, len, off);
	if (off + len > mod->len) {
		len = mod->len - off;
		print("mbmoduleread: len adjust %d\n", len);
	}
	memmove(buf, mod->data + off, len);
	return len;
}

int
multiboot(u32 magic, u32 pmbi, int vflag)
{
	char *p;
	int i, n;
	Mbi *mbi;
	Mod *mod;
	MMap *mmap;
	u64 addr, len;

	if(vflag)
		print("Multiboot: magic %#x pmbi %#x\n", magic, pmbi);
	if(magic != 0x2badb002) {
		print("Multiboot: magic is not set, no multiboot tables\n");
		return -1;
	}

	mbi = KADDR(pmbi);
	if(vflag)
		print("Multiboot: flags %#x\n", mbi->flags);
	if(mbi->flags & Fcmdline){
		p = KADDR(mbi->cmdline);
		if(vflag)
			print("Multiboot: cmdline <%s>\n", p);
		else
			optionsinit(p);
	}
	if(mbi->flags & Fmmap){
		mmap = KADDR(mbi->mmapaddr);
		n = 0;
		while(n < mbi->mmaplength){
			addr = (((u64)mmap->base[1]) << 32) | mmap->base[0];
			len = (((u64)mmap->length[1]) << 32) | mmap->length[0];
			if(vflag){
				print("%s (%u)", mbtypename(mmap->type), mmap->type);
			} else {
				pamapinsert(addr, len, mbpamtype(mmap->type));
			}
			switch(mmap->type){
			// There is no consistency in which type of e820 segment RSDP is stored in.
			case 3:
			case 4:
				if(vflag)
					print("Multiboot: Would check for RSD from %p to %p:", KADDR(addr), KADDR(addr) + len);
				break;
			}
			if(vflag)
				print("\t%#16.16llx %#16.16llx (%llu)\n",
				      addr, addr + len, len);

			n += mmap->size + sizeof(mmap->size);
			mmap = KADDR(mbi->mmapaddr + n);
		}
	}
	if(mbi->flags & Fmods){
		for(i = 0; i < mbi->modscount; i++){
			mod = KADDR(mbi->modsaddr + i * 16);
			p = "";
			if(mod->string != 0)
				p = KADDR(mod->string);
			if(vflag)
				print("Multiboot: mod %#x %#x <%s>\n",
				      mod->modstart, mod->modend, p);
			else {
				u64 modstart = mod->modstart;
				usize len = mod->modend - mod->modstart;
				pamapinsert(modstart, len, PamMODULE);

				if (nummoddirs < MaxNumModDirs) {
					Dirtab *dirtab = addarchfile(p, 0444, mbmoduleread, nil);
					moddirs[nummoddirs].qid = &dirtab->qid;
					moddirs[nummoddirs].data = (void*)modstart;
					moddirs[nummoddirs].len = len;
					nummoddirs++;
				} else {
					print("Multiboot: can't add module %s to devarch - too many modules (%d)\n", p, MaxNumModDirs);
				}
			}
		}
	}
	if(vflag && (mbi->flags & Fbootloadername)){
		p = KADDR(mbi->bootloadername);
		print("Multiboot: bootloadername <%s>\n", p);
	}

	return 0;
}
