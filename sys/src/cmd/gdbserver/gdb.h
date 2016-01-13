/*
 * This provides the callbacks and functions that KGDB needs to share between
 * the core, I/O and arch-specific portions.
 *
 * Author: Amit Kale <amitkale@linsyssoft.com> and
 *         Tom Rini <trini@kernel.crashing.org>
 *
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
static int bpsize;

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

struct bkpt {
	unsigned long		bpt_addr;
	unsigned char		saved_instr[16];
	enum bptype	type;
	enum bpstate	state;
};

struct dbg_reg_def_t {
	char *name;
	int size;
	int offset;
};

extern char *dbg_get_reg(int regno, void *mem, Ureg *regs);
extern int dbg_set_reg(int regno, void *mem, Ureg *regs);


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
		      Ureg *regs);

/**
 *	arch_set_pc - Generic call back to the program counter
 *	@regs: Current &struct pt_regs.
 *  @pc: The new value for the program counter
 *
 *	This function handles updating the program counter and requires an
 *	architecture specific implementation.
 */
extern void arch_set_pc(Ureg *regs, unsigned long pc);


/* Optional functions. */
extern int validate_break_address(unsigned long addr);
extern char *arch_set_breakpoint(struct state *ks, struct bkpt *bpt);
extern char *arch_remove_breakpoint(struct state *ks, struct bkpt *bpt);

// I'm pretty sure we don't need this. In the kernel these will be defined in the arch.c file.
// I can't see the need for ops. That's some weird linux thinking artifact I guess.
/**
 * struct arch - Describe architecture specific values.
 * @gdb_bpt_instr: The instruction to trigger a breakpoint.
 * @flags: Flags for the breakpoint, currently just %HW_BREAKPOINT.
 * @set_breakpoint: Allow an architecture to specify how to set a software
 * breakpoint.
 * @remove_breakpoint: Allow an architecture to specify how to remove a
 * software breakpoint.
 * @set_hw_breakpoint: Allow an architecture to specify how to set a hardware
 * breakpoint.
 * @remove_hw_breakpoint: Allow an architecture to specify how to remove a
 * hardware breakpoint.
 * @disable_hw_break: Allow an architecture to specify how to disable
 * hardware breakpoints for a single cpu.
 * @remove_all_hw_break: Allow an architecture to specify how to remove all
 * hardware breakpoints.
 * @correct_hw_break: Allow an architecture to specify how to correct the
 * hardware debug registers.
 * @enable_nmi: Manage NMI-triggered entry to KGDB
 */
struct arch {
	unsigned char		gdb_bpt_instr[16];
	unsigned long		flags;

	int	(*set_breakpoint)(unsigned long, char *);
	int	(*remove_breakpoint)(unsigned long, char *);
	int	(*set_hw_breakpoint)(unsigned long, int, enum bptype);
	int	(*remove_hw_breakpoint)(unsigned long, int, enum bptype);
	void	(*disable_hw_break)(Ureg *regs);
	void	(*remove_all_hw_break)(void);
	void	(*correct_hw_break)(void);

};

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
char *mem2hex(char *mem, char *buf, int count);
char *hex2mem(char *buf, char *mem, int count);

extern int isremovedbreak(unsigned long addr);
extern void schedule_breakpoint(void);

extern int
handle_exception(int ex_vector, int signo, int err_code,
		      Ureg *regs);

extern int			single_step;
extern int			active;

void gdbinit(void);
