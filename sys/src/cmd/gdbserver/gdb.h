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
	GDB_AX,			/* 0 */
	GDB_BX,			/* 1 */
	GDB_CX,			/* 2 */
	GDB_DX,			/* 3 */
	GDB_SI,			/* 4 */
	GDB_DI,			/* 5 */
	GDB_BP,			/* 6 */
	GDB_SP,			/* 7 */
	GDB_R8,			/* 8 */
	GDB_R9,			/* 9 */
	GDB_R10,		/* 10 */
	GDB_R11,		/* 11 */
	GDB_R12,		/* 12 */
	GDB_R13,		/* 13 */
	GDB_R14,		/* 14 */
	GDB_R15,		/* 15 */
	GDB_PC,			/* 16 */
	GDB_PS,			/* 17 */
	GDB_CS,			/* 18 */
	GDB_SS,			/* 19 */
	GDB_DS,			/* 20 */
	GDB_ES,			/* 21 */
	GDB_FS,			/* 22 */
	GDB_GS,			/* 23 */
};

// Again, this is very gdb-specific.
extern char* regstrs[];

#define GDB_ORIG_AX		57
#define DBG_MAX_REG_NUM		24
/* 17 64 bit regs and 5 32 bit regs */
#define NUMREGBYTES		((17 * 8) + (5 * 4))

struct bkpt {
  unsigned long bpt_addr;
  unsigned char saved_instr[16];
  enum bptype	type;
  enum bpstate	state;
};

extern uint64_t arch_get_pc(struct state *ks);

extern char *dbg_get_reg(int regno, void *mem, uintptr_t *regs);
extern int dbg_set_reg(int regno, void *mem, uintptr_t *regs);


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
extern int
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
extern void arch_set_pc(uintptr_t *regs, unsigned long pc);


/* Optional functions. */
extern int validate_break_address(unsigned long addr);
extern char *arch_set_breakpoint(struct state *ks, struct bkpt *bpt);
extern char *arch_remove_breakpoint(struct state *ks, struct bkpt *bpt);


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
	void			(*write_char) (uint8_t);
	void			(*flush) (void);
	int			(*init) (void);
	void			(*pre_exception) (void);
	void			(*post_exception) (void);
	int			is_console;
};

int hex2long(char **ptr, unsigned long *long_val);
char *mem2hex(unsigned char *mem, char *buf, int count);
char *hex2mem(char *buf, unsigned char *mem, int count);
void gdb_cmd_reg_get(struct state *ks);
void gdb_cmd_reg_set(struct state *ks);
uint64_t arch_get_reg(struct state *ks, int regnum);

extern int isremovedbreak(unsigned long addr);
extern void schedule_breakpoint(void);

extern int
handle_exception(int ex_vector, int signo, int err_code,
		 uintptr_t *regs);
