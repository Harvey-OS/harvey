enum {
	Ehdr32sz	= 52,
	Phdr32sz	= 32,
	Shdr32sz	= 40,

	Ehdr64sz	= 64,
	Phdr64sz	= 56,
	Shdr64sz	= 64,
};

/* from /sys/src/libmach/elf.h */
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
	PT_LOAD = 1,
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

	Swrite = 1,	/* section attributes (flags) */
	Salloc = 2,
	Sexec = 4,
};

typedef void (*Putl)(long);

void	elf32(int mach, int bo, int addpsects, void (*putpsects)(Putl));
void	elf32phdr(void (*putl)(long), ulong type, ulong off, ulong vaddr,
	ulong paddr, ulong filesz, ulong memsz, ulong prots, ulong align);
void	elf32shdr(void (*putl)(long), ulong name, ulong type, ulong flags,
	ulong vaddr, ulong off, ulong sectsz, ulong link, ulong addnl,
	ulong align, ulong entsz);

void	elf64(int mach, int bo, int addpsects, void (*putpsects)(Putl));
void	elf64phdr(void (*putl)(long), void (*putll)(vlong), ulong type,
	uvlong off, uvlong vaddr, uvlong paddr, uvlong filesz, uvlong memsz,
	ulong prots, uvlong align);
void	elf64shdr(void (*putl)(long), void (*putll)(vlong), ulong name,
	ulong type, uvlong flags, uvlong vaddr, uvlong off, uvlong sectsz,
	ulong link, ulong addnl, uvlong align, uvlong entsz);
