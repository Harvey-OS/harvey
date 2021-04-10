/*
 * This provides the callbacks and functions that KGDB needs to share between
 * the core, I/O and arch-specific portions.
 *
 * Author: Amit Kale <amitkale@linsyssoft.com> and
 *         Tom Rini <trini@kernel.crashing.org>
 *
 * Copyright (C) 2008 Wind River Systems, Inc. *
 * 2001-2004 (c) Amit S. Kale and 2003-2005 (c) MontaVista Software, Inc.
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

/* and, since gdb can't do error strings, well, we have this bullshit. */
#define Einval "22"
#define Enoent "02"
#define Eio "05"

extern int connected;

extern int			setting_breakpoint;
extern int			cpu_doing_single_step;

extern struct task_struct	*usethread;
extern struct task_struct	*contthread;
extern char breakpoint[], ebreakpoint[];

enum bptype {
	BP_BREAKPOINT = 0,
	BP_HARDWARE_BREAKPOINT,
	BP_WRITE_WATCHPOINT,
	BP_READ_WATCHPOINT,
	BP_ACCESS_WATCHPOINT,
	BP_POKE_BREAKPOINT,
};

enum bpstate {
	BP_UNDEFINED = 0,
	BP_REMOVED,
	BP_SET,
	BP_ACTIVE
};

/*
 * Copyright (C) 2001-2004 Amit S. Kale

 */

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers at least NUMREGBYTES*2 are needed for register packets
 * Longer buffer is needed to list all threads
 */
#define BUFMAX			1024

/*
 *  Note that this register image is in a different order than
 *  the register image that Linux produces at interrupt time.
 *
 *  Linux's register image is defined by struct pt_regs in ptrace.h.
 *  Just why GDB uses a different order is a historical mystery.
 */

/* this is very x86_64 specific. Later, all code that messes with such things
 * needs to be in amd64.c
 * Please don't add an #ifdef here. Please
 */
enum regnames {
	GDB_AX,			// 0	GP Regs
	GDB_FIRST_GP_REG = GDB_AX,
	GDB_BX,			// 1
	GDB_CX,			// 2
	GDB_DX,			// 3
	GDB_SI,			// 4
	GDB_DI,			// 5
	GDB_BP,			// 6
	GDB_SP,			// 7
	GDB_R8,			// 8
	GDB_R9,			// 9
	GDB_R10,		// 10
	GDB_R11,		// 11
	GDB_R12,		// 12
	GDB_R13,		// 13
	GDB_R14,		// 14
	GDB_R15,		// 15
	GDB_PC,			// 16
	GDB_PS,			// 17
	GDB_CS,			// 18
	GDB_SS,			// 19
	GDB_DS,			// 20
	GDB_ES,			// 21
	GDB_FS,			// 22
	GDB_GS,			// 23	End of GP regs
	GDB_LAST_GP_REG = GDB_GS,

	GDB_STMM0,		// 24	Start of FP regs
	GDB_STMM1,		// 25
	GDB_STMM2,		// 26
	GDB_STMM3,		// 27
	GDB_STMM4,		// 28
	GDB_STMM5,		// 29
	GDB_STMM6,		// 30
	GDB_STMM7,		// 31
	GDB_FCTRL,		// 32
	GDB_FCW = GDB_FCTRL,
	GDB_FSTAT,		// 33
	GDB_FSW = GDB_FSTAT,
	GDB_FTAG,		// 34
	GDB_FTW = GDB_FTAG,
	GDB_FISEG,		// 35
	GDB_FPU_CS = GDB_FISEG,
	GDB_FIOFF,		// 36
	GDB_IP = GDB_FIOFF,
	GDB_FOSEG,		// 37
	GDB_FPU_DS = GDB_FOSEG,
	GDB_FOOFF,		// 38
	GDB_DP = GDB_FOOFF,
	GDB_FOP,		// 39
	GDB_XMM0,		// 40
	GDB_XMM1,		// 41
	GDB_XMM2,		// 42
	GDB_XMM3,		// 43
	GDB_XMM4,		// 44
	GDB_XMM5,		// 45
	GDB_XMM6,		// 46
	GDB_XMM7,		// 47
	GDB_XMM8,		// 48
	GDB_XMM9,		// 49
	GDB_XMM10,		// 50
	GDB_XMM11,		// 51
	GDB_XMM12,		// 52
	GDB_XMM13,		// 53
	GDB_XMM14,		// 54
	GDB_XMM15,		// 55
	GDB_MXCSR,		// 56	End of FP regs
	GDB_ORIGRAX,		// 57
	GDB_FSBASE,		// 58
	GDB_GSBASE,		// 59
	GDB_MAX_REG,		
};

typedef struct Reg {
	int	idx;
	char*	name;
	int	unsupported;	// Register not supported - return reg value as 0
	int	size;
	int	offset;
} Reg;

extern Reg gdbregs[];

struct bkpt {
  unsigned long bpt_addr;
  unsigned char saved_instr[16];
  enum bptype	type;
  enum bpstate	state;
};

void gdb_init_regs(void);
char *gdb_hex_reg_helper(GdbState *ks, int regnum, char *out);

u64 arch_get_pc(GdbState *ks);

char *dbg_get_reg(int regno, void *mem, uintptr_t *regs);
int dbg_set_reg(int regno, void *mem, uintptr_t *regs);


/**
 *	arch_handle_exception - Handle architecture specific GDB packets.
 *	@vector: The error vector of the exception that happened.
 *	@signo: The signal number of the exception that happened.
 *	@err_code: The error code of the exception that happened.
 *	@remcom_in_buffer: The buffer of the packet we have read.
 *	@remcom_out_buffer: The buffer of %BUFMAX bytes to write a packet into.
 *	@regs: The &struct pt_regs of the current process.
 *
 *	This function MUST handle the 'c' and 's' command packets,
 *	as well packets to set / remove a hardware breakpoint, if used.
 *	If there are additional packets which the hardware needs to handle,
 *	they are handled here.  The code should return -1 if it wants to
 *	process more packets, and a %0 or %1 if it wants to exit from the
 *	kgdb callback.
 */
int
arch_handle_exception(int vector, int signo, int err_code,
		      char *remcom_in_buffer,
		      char *remcom_out_buffer,
		      uintptr_t*regs);

/**
 *	arch_set_pc - Generic call back to the program counter
 *	@regs: Current &struct pt_regs.
 *  @pc: The new value for the program counter
 *
 *	This function handles updating the program counter and requires an
 *	architecture specific implementation.
 */
void arch_set_pc(uintptr_t *regs, unsigned long pc);


/* Optional functions. */
int validate_break_address(unsigned long addr);
char *arch_set_breakpoint(GdbState *ks, struct bkpt *bpt);
char *arch_remove_breakpoint(GdbState *ks, struct bkpt *bpt);


// Leave this for now but we should probably just use the chan abstraction
// for this nonsense.
/**
 * struct io - Describe the interface for an I/O driver to talk with KGDB.
 * @name: Name of the I/O driver.
 * @read_char: Pointer to a function that will return one char.
 * @write_char: Pointer to a function that will write one char.
 * @flush: Pointer to a function that will flush any pending writes.
 * @init: Pointer to a function that will initialize the device.
 * @pre_exception: Pointer to a function that will do any prep work for
 * the I/O driver.
 * @post_exception: Pointer to a function that will do any cleanup work
 * for the I/O driver.
 * @is_console: 1 if the end device is a console 0 if the I/O device is
 * not a console
 */
struct io {
	const char		*name;
	int			(*read_char) (void);
	void			(*write_char) (u8);
	void			(*flush) (void);
	int			(*init) (void);
	void			(*pre_exception) (void);
	void			(*post_exception) (void);
	int			is_console;
};

int hex2long(char **ptr, unsigned long *long_val);
char *mem2hex(unsigned char *mem, char *buf, int count);
char *zerohex(char *buf, int count);
char *hex2mem(char *buf, unsigned char *mem, int count);
void gdb_cmd_reg_get(GdbState *ks);
void gdb_cmd_reg_set(GdbState *ks);
u64 arch_get_reg(GdbState *ks, int regnum);
Reg *gdb_get_reg_by_name(char *reg);
Reg *gdb_get_reg_by_id(int id);

int isremovedbreak(unsigned long addr);
void schedule_breakpoint(void);

int handle_exception(int ex_vector, int signo, int err_code, uintptr_t *regs);
