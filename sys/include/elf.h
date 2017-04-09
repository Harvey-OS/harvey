/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	Definitions needed for accessing ELF headers
 */
typedef struct {
	uint8_t		ident[16];	/* ident bytes */
	uint16_t	type;		/* file type */
	uint16_t	machine;	/* target machine */
	uint32_t	version;	/* file version */
	uint64_t	elfentry;	/* start address */
	uint64_t	phoff;		/* phdr file offset */
	uint64_t	shoff;		/* shdr file offset */
	uint32_t	flags;		/* file flags */
	uint16_t	ehsize;		/* sizeof ehdr */
	uint16_t	phentsize;	/* sizeof phdr */
	uint16_t	phnum;		/* number phdrs */
	uint16_t	shentsize;	/* sizeof shdr */
	uint16_t	shnum;		/* number shdrs */
	uint16_t	shstrndx;	/* shdr string index */
} E64hdr;

typedef struct {
	int	type;		/* entry type */
	uint32_t	offset;		/* file offset */
	uint32_t	vaddr;		/* virtual address */
	uint32_t	paddr;		/* physical address */
	int	filesz;		/* file size */
	uint32_t	memsz;		/* memory size */
	int	flags;		/* entry flags */
	int	align;		/* memory/file alignment */
} Phdr;

typedef struct {
	uint32_t	type;		/* entry type */
	uint32_t	flags;		/* entry flags */
	uint64_t	offset;		/* file offset */
	uint64_t	vaddr;		/* virtual address */
	uint64_t	paddr;		/* physical address */
	uint64_t	filesz;		/* file size */
	uint64_t	memsz;		/* memory size */
	uint64_t	align;		/* memory/file alignment */
} P64hdr;

typedef struct {
	uint32_t	name;		/* section name */
	uint32_t	type;		/* SHT_... */
	uint32_t	flags;		/* SHF_... */
	uint32_t	addr;		/* virtual address */
	uint32_t	offset;		/* file offset */
	uint32_t	size;		/* section size */
	uint32_t	link;		/* misc info */
	uint32_t	info;		/* misc info */
	uint32_t	addralign;	/* memory alignment */
	uint32_t	entsize;	/* entry size if table */
} Shdr;

typedef struct {
	uint32_t	name;		/* section name */
	uint32_t	type;		/* SHT_... */
	uint64_t	flags;		/* SHF_... */
	uint64_t	addr;		/* virtual address */
	uint64_t	offset;		/* file offset */
	uint64_t	size;		/* section size */
	uint32_t	link;		/* misc info */
	uint32_t	info;		/* misc info */
	uint64_t	addralign;	/* memory alignment */
	uint64_t	entsize;	/* entry size if table */
} S64hdr;

enum {
	/* Ehdr codes */
	MAG0 = 0,		/* ident[] indexes */
	MAG1 = 1,
	MAG2 = 2,
	MAG3 = 3,
	CLASS = 4,
	DATA = 5,
	VERSION = 6,

	ELFCLASSNONE = 0,	/* ident[CLASS] */
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
	ELFCLASSNUM = 3,

	ELFDATANONE = 0,	/* ident[DATA] */
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2,
	ELFDATANUM = 3,

	NOETYPE = 0,		/* type */
	REL = 1,
	EXEC = 2,
	DYN = 3,
	CORE = 4,

	NONE = 0,		/* machine */
	M32 = 1,		/* AT&T WE 32100 */
	SPARC = 2,		/* Sun SPARC */
	I386 = 3,		/* Intel 80386 */
	M68K = 4,		/* Motorola 68000 */
	M88K = 5,		/* Motorola 88000 */
	I486 = 6,		/* Intel 80486 */
	I860 = 7,		/* Intel i860 */
	MIPS = 8,		/* Mips R2000 */
	S370 = 9,		/* Amdhal	*/
	MIPSR4K = 10,		/* Mips R4000 */
	SPARC64 = 18,		/* Sun SPARC v9 */
	POWER = 20,		/* PowerPC */
	POWER64 = 21,		/* PowerPC64 */
	ARM = 40,		/* ARM */
	AMD64 = 62,		/* Amd64 */
	ARM64 = 183,		/* ARM64 */

	NO_VERSION = 0,		/* version, ident[VERSION] */
	CURRENT = 1,

	/* Phdr Codes */
	NOPTYPE = 0,		/* type */
	LOAD = 1,
	DYNAMIC = 2,
	INTERP = 3,
	NOTE = 4,
	SHLIB = 5,
	PHDR = 6,

	R = 0x4,		/* flags */
	W = 0x2,
	X = 0x1,

	/* Shdr Codes */
	Progbits = 1,	/* section types */
	Strtab = 3,
	Nobits = 8,

	Swrite = 1,	/* section attributes */
	Salloc = 2,
	Sexec = 4,
};

#define	ELF_MAG		((0x7f<<24) | ('E'<<16) | ('L'<<8) | 'F')
