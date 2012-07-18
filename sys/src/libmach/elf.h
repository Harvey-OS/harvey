/*
 *	Definitions needed for accessing ELF headers
 */
typedef struct {
	uchar	ident[16];	/* ident bytes */
	ushort	type;		/* file type */
	ushort	machine;	/* target machine */
	int	version;	/* file version */
	ulong	elfentry;	/* start address */
	ulong	phoff;		/* phdr file offset */
	ulong	shoff;		/* shdr file offset */
	int	flags;		/* file flags */
	ushort	ehsize;		/* sizeof ehdr */
	ushort	phentsize;	/* sizeof phdr */
	ushort	phnum;		/* number phdrs */
	ushort	shentsize;	/* sizeof shdr */
	ushort	shnum;		/* number shdrs */
	ushort	shstrndx;	/* shdr string index */
} Ehdr;

typedef struct {
	u8int	ident[16];	/* ident bytes */
	u16int	type;		/* file type */
	u16int	machine;	/* target machine */
	u32int	version;	/* file version */
	u64int	elfentry;	/* start address */
	u64int	phoff;		/* phdr file offset */
	u64int	shoff;		/* shdr file offset */
	u32int	flags;		/* file flags */
	u16int	ehsize;		/* sizeof ehdr */
	u16int	phentsize;	/* sizeof phdr */
	u16int	phnum;		/* number phdrs */
	u16int	shentsize;	/* sizeof shdr */
	u16int	shnum;		/* number shdrs */
	u16int	shstrndx;	/* shdr string index */
} E64hdr;

typedef struct {
	int	type;		/* entry type */
	ulong	offset;		/* file offset */
	ulong	vaddr;		/* virtual address */
	ulong	paddr;		/* physical address */
	int	filesz;		/* file size */
	ulong	memsz;		/* memory size */
	int	flags;		/* entry flags */
	int	align;		/* memory/file alignment */
} Phdr;

typedef struct {
	u32int	type;		/* entry type */
	u32int	flags;		/* entry flags */
	u64int	offset;		/* file offset */
	u64int	vaddr;		/* virtual address */
	u64int	paddr;		/* physical address */
	u64int	filesz;		/* file size */
	u64int	memsz;		/* memory size */
	u64int	align;		/* memory/file alignment */
} P64hdr;

typedef struct {
	ulong	name;		/* section name */
	ulong	type;		/* SHT_... */
	ulong	flags;		/* SHF_... */
	ulong	addr;		/* virtual address */
	ulong	offset;		/* file offset */
	ulong	size;		/* section size */
	ulong	link;		/* misc info */
	ulong	info;		/* misc info */
	ulong	addralign;	/* memory alignment */
	ulong	entsize;	/* entry size if table */
} Shdr;

typedef struct {
	u32int	name;		/* section name */
	u32int	type;		/* SHT_... */
	u64int	flags;		/* SHF_... */
	u64int	addr;		/* virtual address */
	u64int	offset;		/* file offset */
	u64int	size;		/* section size */
	u32int	link;		/* misc info */
	u32int	info;		/* misc info */
	u64int	addralign;	/* memory alignment */
	u64int	entsize;	/* entry size if table */
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
