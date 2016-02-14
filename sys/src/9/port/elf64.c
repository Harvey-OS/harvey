#include	"u.h"
#include	"tos.h"
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>
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
		EM_NONE = 0, //  No
		EM_M32 = 1, //  AT&T WE
		EM_SPARC = 2, //
		EM_386 = 3, //  Intel
		EM_68K = 4, //  Motorola
		EM_88K = 5, //  Motorola
		EM_IAMCU = 6, //  Intel
		EM_860 = 7, //  Intel
		EM_MIPS = 8, //  MIPS I
		EM_S370 = 9, //  IBM System/370
		EM_MIPS_RS3_LE = 10, //  MIPS RS3000
		// reserved
		EM_PARISC = 15, //  Hewlett-Packard
		// reserved
		EM_VPP500 = 17, //  Fujitsu
		EM_SPARC32PLUS = 18, //  Enhanced instruction set
		EM_960 = 19, //  Intel
		EM_PPC = 20, //
		EM_PPC64 = 21, //  64-bit
		EM_S390 = 22, //  IBM System/390
		EM_SPU = 23, //  IBM
		// reserved
		EM_V800 = 36, //  NEC
		EM_FR20 = 37, //  Fujitsu
		EM_RH32 = 38, //  TRW
		EM_RCE = 39, //  Motorola
		EM_ARM = 40, //  ARM 32-bit architecture
		EM_ALPHA = 41, //  Digital
		EM_SH = 42, //  Hitachi
		EM_SPARCV9 = 43, //  SPARC Version
		EM_TRICORE = 44, //  Siemens TriCore embedded
		EM_ARC = 45, //  Argonaut RISC Core, Argonaut Technologies
		EM_H8_300 = 46, //  Hitachi
		EM_H8_300H = 47, //  Hitachi
		EM_H8S = 48, //  Hitachi
		EM_H8_500 = 49, //  Hitachi
		EM_IA_64 = 50, //  Intel IA-64 processor
		EM_MIPS_X = 51, //  Stanford
		EM_COLDFIRE = 52, //  Motorola
		EM_68HC12 = 53, //  Motorola
		EM_MMA = 54, //  Fujitsu MMA Multimedia
		EM_PCP = 55, //  Siemens
		EM_NCPU = 56, //  Sony nCPU embedded RISC
		EM_NDR1 = 57, //  Denso NDR1
		EM_STARCORE = 58, //  Motorola Star*Core
		EM_ME16 = 59, //  Toyota ME16
		EM_ST100 = 60, //  STMicroelectronics ST100
		EM_TINYJ = 61, //  Advanced Logic Corp. TinyJ embedded processor
		EM_X86_64 = 62, //  AMD x86-64
		EM_PDSP = 63, //  Sony DSP
		EM_PDP10 = 64, //  Digital Equipment Corp.
		EM_PDP11 = 65, //  Digital Equipment Corp.
		EM_FX66 = 66, //  Siemens FX66
		EM_ST9PLUS = 67, //  STMicroelectronics ST9+ 8/16 bit
		EM_ST7 = 68, //  STMicroelectronics ST7 8-bit
		EM_68HC16 = 69, //  Motorola MC68HC16
		EM_68HC11 = 70, //  Motorola MC68HC11
		EM_68HC08 = 71, //  Motorola MC68HC08
		EM_68HC05 = 72, //  Motorola MC68HC05
		EM_SVX = 73, //  Silicon Graphics
		EM_ST19 = 74, //  STMicroelectronics ST19 8-bit
		EM_VAX = 75, //  Digital
		EM_CRIS = 76, //  Axis Communications 32-bit embedded
		EM_JAVELIN = 77, //  Infineon Technologies 32-bit embedded
		EM_FIREPATH = 78, //  Element 14 64-bit DSP
		EM_ZSP = 79, //  LSI Logic 16-bit DSP
		EM_MMIX = 80, //  Donald Knuth's educational 64-bit
		EM_HUANY = 81, //  Harvard University machine-independent object
		EM_PRISM = 82, //  SiTera
		EM_AVR = 83, //  Atmel AVR 8-bit
		EM_FR30 = 84, //  Fujitsu
		EM_D10V = 85, //  Mitsubishi
		EM_D30V = 86, //  Mitsubishi
		EM_V850 = 87, //  NEC
		EM_M32R = 88, //  Mitsubishi
		EM_MN10300 = 89, //  Matsushita
		EM_MN10200 = 90, //  Matsushita
		EM_PJ = 91, //
		EM_OPENRISC = 92, //  OpenRISC 32-bit embedded
		EM_ARC_COMPACT = 93, //  ARC International ARCompact processor (old spelling/synonym:
		EM_XTENSA = 94, //  Tensilica Xtensa
		EM_VIDEOCORE = 95, //  Alphamosaic VideoCore
		EM_TMM_GPP = 96, //  Thompson Multimedia General Purpose
		EM_NS32K = 97, //  National Semiconductor 32000
		EM_TPC = 98, //  Tenor Network TPC
		EM_SNP1K = 99, //  Trebia SNP 1000
		EM_ST200 = 100, //  STMicroelectronics (www.st.com) ST200
		EM_IP2K = 101, //  Ubicom IP2xxx microcontroller
		EM_MAX = 102, //  MAX
		EM_CR = 103, //  National Semiconductor CompactRISC
		EM_F2MC16 = 104, //  Fujitsu
		EM_MSP430 = 105, //  Texas Instruments embedded microcontroller
		EM_BLACKFIN = 106, //  Analog Devices Blackfin (DSP)
		EM_SE_C33 = 107, //  S1C33 Family of Seiko Epson
		EM_SEP = 108, //  Sharp embedded
		EM_ARCA = 109, //  Arca RISC
		EM_UNICORE = 110, //  Microprocessor series from PKU-Unity Ltd. and MPRC of Peking
		EM_EXCESS = 111, //  eXcess: 16/32/64-bit configurable embedded
		EM_DXP = 112, //  Icera Semiconductor Inc. Deep Execution
		EM_ALTERA_NIOS2 = 113, //  Altera Nios II soft-core
		EM_CRX = 114, //  National Semiconductor CompactRISC CRX
		EM_XGATE = 115, //  Motorola XGATE embedded
		EM_C166 = 116, //  Infineon C16x/XC16x
		EM_M16C = 117, //  Renesas M16C series
		EM_DSPIC30F = 118, //  Microchip Technology dsPIC30F Digital Signal
		EM_CE = 119, //  Freescale Communication Engine RISC
		EM_M32C = 120, //  Renesas M32C series
		// reserved
		EM_TSK3000 = 131, //  Altium TSK3000
		EM_RS08 = 132, //  Freescale RS08 embedded
		EM_SHARC = 133, //  Analog Devices SHARC family of 32-bit DSP
		EM_ECOG2 = 134, //  Cyan Technology eCOG2
		EM_SCORE7 = 135, //  Sunplus S+core7 RISC
		EM_DSP24 = 136, //  New Japan Radio (NJR) 24-bit DSP
		EM_VIDEOCORE3 = 137, //  Broadcom VideoCore III
		EM_LATTICEMICO32 = 138, //  RISC processor for Lattice FPGA
		EM_SE_C17 = 139, //  Seiko Epson C17
		EM_TI_C6000 = 140, //  The Texas Instruments TMS320C6000 DSP
		EM_TI_C2000 = 141, //  The Texas Instruments TMS320C2000 DSP
		EM_TI_C5500 = 142, //  The Texas Instruments TMS320C55x DSP
		EM_TI_ARP32 = 143, //  Texas Instruments Application Specific RISC Processor, 32bit
		EM_TI_PRU = 144, //  Texas Instruments Programmable Realtime
		// reserved
		EM_MMDSP_PLUS = 160, //  STMicroelectronics 64bit VLIW Data Signal
		EM_CYPRESS_M8C = 161, //  Cypress M8C
		EM_R32C = 162, //  Renesas R32C series
		EM_TRIMEDIA = 163, //  NXP Semiconductors TriMedia architecture
		EM_QDSP6 = 164, //  QUALCOMM DSP6
		EM_8051 = 165, //  Intel 8051 and
		EM_STXP7X = 166, //  STMicroelectronics STxP7x family of configurable and extensible RISC
		EM_NDS32 = 167, //  Andes Technology compact code size embedded RISC processor
		EM_ECOG1 = 168, //  Cyan Technology eCOG1X
		EM_ECOG1X = 168, //  Cyan Technology eCOG1X
		EM_MAXQ30 = 169, //  Dallas Semiconductor MAXQ30 Core
		EM_XIMO16 = 170, //  New Japan Radio (NJR) 16-bit DSP
		EM_MANIK = 171, //  M2000 Reconfigurable RISC
		EM_CRAYNV2 = 172, //  Cray Inc. NV2 vector
		EM_RX = 173, //  Renesas RX
		EM_METAG = 174, //  Imagination Technologies META processor
		EM_MCST_ELBRUS = 175, //  MCST Elbrus general purpose hardware
		EM_ECOG16 = 176, //  Cyan Technology eCOG16
		EM_CR16 = 177, //  National Semiconductor CompactRISC CR16 16-bit
		EM_ETPU = 178, //  Freescale Extended Time Processing
		EM_SLE9X = 179, //  Infineon Technologies SLE9X
		EM_L10M = 180, //  Intel
		EM_K10M = 181, //  Intel
		// reserved(Intel)
		EM_AARCH64 = 183, //  ARM 64-bit architecture
		// reserved(ARM)
		EM_AVR32 = 185, //  Atmel Corporation 32-bit microprocessor
		EM_STM8 = 186, //  STMicroeletronics STM8 8-bit
		EM_TILE64 = 187, //  Tilera TILE64 multicore architecture
		EM_TILEPRO = 188, //  Tilera TILEPro multicore architecture
		EM_MICROBLAZE = 189, //  Xilinx MicroBlaze 32-bit RISC soft processor
		EM_CUDA = 190, //  NVIDIA CUDA
		EM_TILEGX = 191, //  Tilera TILE-Gx multicore architecture
		EM_CLOUDSHIELD = 192, //  CloudShield architecture
		EM_COREA_1ST = 193, //  KIPO-KAIST Core-A 1st generation processor
		EM_COREA_2ND = 194, //  KIPO-KAIST Core-A 2nd generation processor
		EM_ARC_COMPACT2 = 195, //  Synopsys ARCompact
		EM_OPEN8 = 196, //  Open8 8-bit RISC soft processor
		EM_RL78 = 197, //  Renesas RL78
		EM_VIDEOCORE5 = 198, //  Broadcom VideoCore V
		EM_78KOR = 199, //  Renesas 78KOR
		EM_56800EX = 200, //  Freescale 56800EX Digital Signal Controller
		EM_BA1 = 201, //  Beyond BA1 CPU
		EM_BA2 = 202, //  Beyond BA2 CPU
		EM_XCORE = 203, //  XMOS xCORE processor
		EM_MCHP_PIC = 204, //  Microchip 8-bit PIC(r)
		// reserved(Intel)
		EM_KM32 = 210, //  KM211 KM32 32-bit
		EM_KMX32 = 211, //  KM211 KMX32 32-bit
		EM_KMX16 = 212, //  KM211 KMX16 16-bit
		EM_KMX8 = 213, //  KM211 KMX8 8-bit
		EM_KVARC = 214, //  KM211 KVARC
		EM_CDP = 215, //  Paneve CDP architecture
		EM_COGE = 216, //  Cognitive Smart Memory
		EM_COOL = 217, //  Bluechip Systems
		EM_NORC = 218, //  Nanoradio Optimized
		EM_CSR_KALIMBA = 219, //  CSR Kalimba architecture
		EM_Z80 = 220, //  Zilog
		EM_VISIUM = 221, //  Controls and Data Services VISIUMcore
		EM_FT32 = 222, //  FTDI Chip FT32 high performance 32-bit RISC
		EM_MOXIE = 223, //  Moxie processor
		EM_AMDGPU = 224, //  AMD GPU
		EM_RISCV = 243, // Berkeley RISC-V

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

static struct {
	char *mach;
	int e_machine;
} elfmachs[] = {
	{"amd64", EM_X86_64},
	{"arm64", EM_AARCH64},
	{"power64", EM_PPC64},
};

static int
ispow2(uintptr_t a)
{
  return ((a != 0) && (a & (a-1)) == 0);
}

static int
overlap(uintptr_t a0, uintptr_t aend, uintptr_t b0, uintptr_t bend)
{
	uint64_t max0, minend;
	max0 = a0 > b0 ? a0 : b0;
	minend = aend < bend ? aend : bend;
	return max0 < minend;
}

/*
 *	return the number of ldsegs in rp
 */
int
elf64ldseg(Chan *c, uintptr_t *entryp, Ldseg **rp, char *mach, uint32_t minpgsz)
{
	Proc *up = externup();
	Elf64_Ehdr ehdr;
	uint16_t (*get16)(uint8_t *);
	uint32_t (*get32)(uint8_t *);
	uint64_t (*get64)(uint8_t *);
	uint8_t *phbuf, *phend;
	uint8_t *fp;
	Ldseg *ldseg;
	uint64_t entry;
	int i, j, si;

	entry = 0;
	phbuf = nil;
	ldseg = nil;
	si = 0;

	if(waserror()){
		if(ldseg != nil)
			free(ldseg);
		if(phbuf != nil)
			free(phbuf);
		nexterror();
	}

	if(c->dev->read(c, &ehdr, sizeof ehdr, 0) != sizeof ehdr){
		print("elf64ldseg: too short for header\n");
		goto done; // too short to be elf but could be something else
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
			uint32_t phnum, phentsize;
			uint16_t e_machine;

			e_machine = get16(ehdr.e_machine);
			if(mach != nil){
				for(i = 0; i < nelem(elfmachs); i++)
					if(elfmachs[i].e_machine == e_machine && !strcmp(mach, elfmachs[i].mach))
						break;
				if(i == nelem(elfmachs)){
					print("elf64ldseg: e_machine %d incorrect for host %s\n", e_machine, mach);
					error(Ebadexec);
				}
			}
			entry = get64(ehdr.e_entry);
			phoff = get16(ehdr.e_phoff);
			phnum = get16(ehdr.e_phnum);
			phentsize = get16(ehdr.e_phentsize);

			if(phentsize*phnum > minpgsz){
				print("elf64ldseg: phentsize %d phnum %d exceeds page size %d\n", phentsize, phnum, minpgsz);
				error(Ebadexec);
			}

			phbuf = malloc(phentsize*phnum);
			if(phbuf == nil){
				print("elf64ldseg: malloc fail\n");
				error(Ebadexec);
			}

			if(c->dev->read(c, phbuf, phentsize*phnum, phoff) != phentsize*phnum){
				print("elf64ldseg: read program header fail\n");
				error(Ebadexec);
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
				error(Ebadexec);
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

					if(memsz < filesz){
						print("elf64ldseg: memsz %d < filesz %d\n", memsz, filesz);
						error(Ebadexec);
					}

					if(!ispow2(align)){
						print("elf64ldseg: align 0x%x not a power of 2\n", align);
						error(Ebadexec);
					}

					if(align < minpgsz){
						print("elf64ldseg: align 0x%x < minpgsz 0x%x\n", align, minpgsz);
						error(Ebadexec);
					}

					if(offset & (align-1) != vaddr & (align-1)){
						print("elf64ldseg: va offset 0x%x != file offset 0x%x (align 0x%x)\n",
							offset & (align-1),
							vaddr & (align-1),
							align
						);
						error(Ebadexec);
					}

					ldseg[si].pgsz = align;
					ldseg[si].memsz = memsz;
					ldseg[si].filesz = filesz;
					ldseg[si].pg0fileoff = offset & ~(align-1);
					ldseg[si].pg0vaddr = vaddr & ~(align-1);
					ldseg[si].pg0off = offset & (align-1);

					si++;
				}
			}
			for(i = 0; i < si; i++){
				for(j = 0; j < si; j++){
					if(i != j){
						Ldseg *lda, *ldb;
						lda = ldseg+i;
						ldb = ldseg+j;
						if(overlap(
							lda->pg0vaddr, lda->pg0vaddr + lda->pg0off + lda->memsz,
							ldb->pg0vaddr, ldb->pg0vaddr + ldb->pg0off + ldb->memsz
						)){
							print("elf64ldseg: load segs %p:%p and %p:%p ovelap\n",
								lda->pg0vaddr, lda->pg0vaddr + lda->pg0off + lda->memsz,
								ldb->pg0vaddr, ldb->pg0vaddr + ldb->pg0off + ldb->memsz
							);
							error(Ebadexec);
						}
					}
				}
			}
		} else {
			print("elf64ldseg: not elfclass64\n");
			error(Ebadexec);
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
	poperror();
	return si;
}
