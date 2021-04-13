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
	u8		ident[16];	/* ident bytes */
	u16	type;		/* file type */
	u16	machine;	/* target machine */
	u32	version;	/* file version */
	u64	elfentry;	/* start address */
	u64	phoff;		/* phdr file offset */
	u64	shoff;		/* shdr file offset */
	u32	flags;		/* file flags */
	u16	ehsize;		/* sizeof ehdr */
	u16	phentsize;	/* sizeof phdr */
	u16	phnum;		/* number phdrs */
	u16	shentsize;	/* sizeof shdr */
	u16	shnum;		/* number shdrs */
	u16	shstrndx;	/* shdr string index */
} E64hdr;

typedef struct {
	int		type;		/* entry type */
	u32	offset;		/* file offset */
	u32	vaddr;		/* virtual address */
	u32	paddr;		/* physical address */
	int		filesz;		/* file size */
	u32	memsz;		/* memory size */
	int		flags;		/* entry flags */
	int		align;		/* memory/file alignment */
} Phdr;

typedef struct {
	u32	type;		/* entry type */
	u32	flags;		/* entry flags */
	u64	offset;		/* file offset */
	u64	vaddr;		/* virtual address */
	u64	paddr;		/* physical address */
	u64	filesz;		/* file size */
	u64	memsz;		/* memory size */
	u64	align;		/* memory/file alignment */
} P64hdr;

typedef struct {
	u32	name;		/* section name */
	u32	type;		/* SHT_... */
	u32	flags;		/* SHF_... */
	u32	addr;		/* virtual address */
	u32	offset;		/* file offset */
	u32	size;		/* section size */
	u32	link;		/* misc info */
	u32	info;		/* misc info */
	u32	addralign;	/* memory alignment */
	u32	entsize;	/* entry size if table */
} Shdr;

typedef struct {
	u32	name;		/* section name */
	u32	type;		/* SHT_... */
	u64	flags;		/* SHF_... */
	u64	addr;		/* virtual address */
	u64	offset;		/* file offset */
	u64	size;		/* section size */
	u32	link;		/* misc info */
	u32	info;		/* misc info */
	u64	addralign;	/* memory alignment */
	u64	entsize;	/* entry size if table */
} S64hdr;

typedef struct {
	u32	st_name;	/* Symbol name */
	u8		st_info;	/* Type and Binding attributes */
	u8		st_other;	/* Reserved */
	u16	st_shndx;	/* Section table index */
	u64	st_value;	/* Symbol value */
	u64	st_size;	/* Size of object (e.g., common) */
} E64Sym;


typedef struct Sym {
	i64		value;
	u32		sig;
	char		type;
	char		*name;
} Sym;

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
	SHT_PROGBITS = 1,	/* section types */
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_NOBITS = 8,

	Swrite = 1,		/* section attributes */
	Salloc = 2,
	Sexec = 4,

	STB_LOCAL = 0,		/* Symbol bindings */
	STB_GLOBAL = 1,
	STB_WEAK = 2,
	STB_LOOS = 10,
	STB_HIOS = 12,
	STB_LOPROC = 13,
	STB_HIPROC = 15,

	STT_NOTYPE = 0, 	/* Symbol types */
	STT_OBJECT = 1,
	STT_FUNC = 2,
	STT_SECTION = 3,
	STT_FILE = 4,
	STT_LOOS = 10,
	STT_HIOS = 12,
	STT_LOPROC = 13,
	STT_HIPROC = 15,

	SHN_UNDEF = 0,
};

#define	ELF_MAG		((0x7f<<24) | ('E'<<16) | ('L'<<8) | 'F')
