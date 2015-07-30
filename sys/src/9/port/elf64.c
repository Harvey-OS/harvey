#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"getput.h"

typedef struct Elf64_Ehdr Elf64_Ehdr;
typedef struct Elf64_Phdr Elf64_Phdr;

enum {
	EI_MAG0 = 0,	// File identification
	EI_MAG1 = 1,
	EI_MAG2 = 2,
	EI_MAG3 = 3,
	EI_CLASS = 4,	// File class
		ELFCLASS32 = 1,	// 32-bit objects
		ELFCLASS64 = 2,	// 64-bit objects
	EI_DATA = 5,	// Data encoding
		ELFDATA2LSB = 1,	// Object file data structures are littleendian
		ELFDATA2MSB = 2,	// Object file data structures are bigendian
	EI_VERSION = 6,	// File version
	EI_OSABI = 7,	// OS/ABI identification
		ELFOSABI_SYSV = 0,	// System V ABI
		ELFOSABI_HPUX = 1,
	EI_ABIVERSION = 8,	// ABI version
	EI_PAD = 9,	// Start of padding bytes
	EI_NIDENT = 16,	// Size of e_ident[]

	ET_NONE = 0,	// No file type
	ET_REL = 1,	// Relocatable object file
	ET_EXEC = 2,	// Executable file
	ET_DYN = 3,	// Shared object file
	ET_CORE = 4,	// Core file

	PT_NULL = 0,	// Unused entry
	PT_LOAD = 1,	// Loadable segment
	PT_DYNAMIC = 2,	// Dynamic linking tables
	PT_INTERP = 3,	// Program interpreter path name
	PT_NOTE = 4,	// Note sections
	PT_SHLIB = 5,	// Reserved
	PT_PHDR = 6,	// Program header table

	PF_X = 0x1,	// Execute permission
	PF_W = 0x2,	// Write permission
	PF_R = 0x4,	// Read permission
};

struct Elf64_Ehdr {
	uint8_t e_ident[16];	/* ELF identification */
	uint8_t e_type[2];	/* Object file type */
	uint8_t e_machine[2];	/* Machine type */
	uint8_t e_version[4];	/* Object file version */
	uint8_t e_entry[8];	/* Entry point address */
	uint8_t e_phoff[8];	/* Program header offset */
	uint8_t e_shoff[8];	/* Section header offset */
	uint8_t e_flags[4];	/* Processor-specific flags */
	uint8_t e_ehsize[2];	/* ELF header size */
	uint8_t e_phentsize[2];	/* Size of program header entry */
	uint8_t e_phnum[2];	/* Number of program header entries */
	uint8_t e_shentsize[2];	/* Size of section header entry */
	uint8_t e_shnum[2];	/* Number of section header entries */
	uint8_t e_shstrndx[2];	/* Section name string table index */
};

struct Elf64_Phdr {
	uint8_t p_type[4];	/* Type of segment */
	uint8_t p_flags[4];	/* Segment attributes */
	uint8_t p_offset[8];	/* Offset in file */
	uint8_t p_vaddr[8];	/* Virtual address in memory */
	uint8_t p_paddr[8];	/* Reserved */
	uint8_t p_filesz[8];	/* Size of segment in file */
	uint8_t p_memsz[8];	/* Size of segment in memory */
	uint8_t p_align[8];	/* Alignment of segment */
};

int
elf64ldseg(Chan *c, uintptr_t *entryp, Ldseg **rp)
{
	Elf64_Ehdr ehdr;
	uint16_t (*get16)(uint8_t *);
	uint32_t (*get32)(uint8_t *);
	uint64_t (*get64)(uint8_t *);
	uint8_t *phbuf, *phend;
	uint8_t *fp;
	Ldseg *ldseg;
	uint64_t entry;
	int si;

	entry = 0;
	phbuf = nil;
	ldseg = nil;
	si = -1;

	if(c->dev->read(c, &ehdr, sizeof ehdr, 0) != sizeof ehdr){
		print("elf64ldseg: pread fail\n");
		goto done;
	}

	fp = ehdr.e_ident;
	if(fp[EI_MAG0] == '\x7f' && fp[EI_MAG1] == 'E' && fp[EI_MAG2] == 'L' && fp[EI_MAG3] == 'F'){

		if(fp[EI_DATA] == ELFDATA2LSB){
			get16 = get16le;
			get32 = get32le;
			get64 = get64le;
		} else if(fp[EI_DATA] == ELFDATA2MSB){
			get16 = get16be;
			get32 = get32be;
			get64 = get64be;
		}

		if(fp[EI_CLASS] == ELFCLASS64){
			int64_t phoff;
			int32_t phnum, phentsize;

			si = 0; /* return >= 0 if we got this far */
			entry = get64(ehdr.e_entry);
			phoff = get16(ehdr.e_phoff);
			phnum = get16(ehdr.e_phnum);
			phentsize = get16(ehdr.e_phentsize);

			phbuf = malloc(phentsize*phnum);
			if(phbuf == nil){
				print("elf64ldseg: malloc fail\n");
				goto done;
			}

			if(c->dev->read(c, phbuf, phentsize*phnum, phoff) != phentsize*phnum){
				print("elf64ldseg: pread fail\n");
				goto done;
			}

			si = 0;
			phend = phbuf + phentsize*phnum;
			for(fp = phbuf; fp < phend; fp += phentsize){
				Elf64_Phdr *phdr;
				phdr = (Elf64_Phdr*)fp;
				if(get32(phdr->p_type) == PT_LOAD)
					si++;
			}
			ldseg = malloc(si * sizeof ldseg[0]);
			if(ldseg == nil){
				print("elf64ldseg: malloc fail\n");
				goto done;
			}

			si = 0;
			for(fp = phbuf; fp < phend; fp += phentsize){
				Elf64_Phdr *phdr;

				phdr = (Elf64_Phdr*)fp;
				if(get32(phdr->p_type) == PT_LOAD){
					uint64_t offset, vaddr, align, filesz, memsz;
					uint32_t flags;

					flags = get32(phdr->p_flags);	/* Segment attributes */
					offset = get64(phdr->p_offset);	/* Offset in file */
					vaddr = get64(phdr->p_vaddr);	/* Virtual address in memory */
					filesz = get64(phdr->p_filesz);	/* Size of segment in file */
					memsz = get64(phdr->p_memsz);	/* Size of segment in memory */
					align = get64(phdr->p_align);	/* Alignment of segment */

					ldseg[si].type = SG_LOAD;
					if((flags & PF_R) != 0)
						ldseg[si].type |= SG_READ;
					if((flags & PF_W) != 0)
						ldseg[si].type |= SG_WRITE;
					if((flags & PF_X) != 0)
						ldseg[si].type |= SG_EXEC;

					ldseg[si].pgsz = align;
					ldseg[si].memsz = memsz;
					ldseg[si].filesz = filesz;
					ldseg[si].pg0faddr = offset & ~(align-1);
					ldseg[si].pg0vaddr = vaddr & ~(align-1);
					ldseg[si].pg0off = offset & (align-1);
					si++;
				}
			}
		}
	}
done:
	if(phbuf != nil)
		free(phbuf);
	if(rp != nil){
		*rp = ldseg;
	} else if(ldseg != nil){
		free(ldseg);
	}
	if(entryp != nil)
		*entryp = entry;

	return si;
}
