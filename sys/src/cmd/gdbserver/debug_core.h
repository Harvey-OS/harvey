/*
 * Created by: Jason Wessel <jason.wessel@windriver.com>
 *
 * Copyright (c) 2009 Wind River Systems, Inc.  All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*
 * These are the private implementation headers between the kernel
 * debugger core and the debugger front end code.
 */

/* kernel debug core data structures */
struct state {
	unsigned long	threadid;
	int		ex_vector;
	int		signo;
	int		err_code;
	int		cpu;
	int		pass_exception;
	char		*pidname;
	long		usethreadid;
	void		*gdbregs;
	int 		gdbregsize; // determined by the amount read from /proc/pid/gdbregs
};

#define DCPU_SSTEP       0x8 /* CPU is single stepping */

struct debuggerinfo_struct {
	void			*debuggerinfo;
	struct task_struct	*task;
	int			exception_state;
	int			ret_state;
	int			irq_depth;
	int			enter_kgdb;
};

extern struct debuggerinfo_struct info[];

/* kernel debug core break point routines */
extern char * dbg_remove_all_break(struct state *);
extern char * dbg_set_sw_break(struct state *, unsigned long addr);
extern char * dbg_remove_sw_break(struct state *, unsigned long addr);
extern char * dbg_activate_sw_breakpoints(struct state *);
extern char * dbg_deactivate_sw_breakpoints(struct state *);

/* polled character access to i/o module */
extern int dbg_io_get_char(void);

/* stub return value for switching between the gdbstub and kdb */
#define DBG_PASS_EVENT -12345
/* Switch from one cpu to another */
#define DBG_SWITCH_CPU_EVENT -123456
extern int dbg_switch_cpu;

/* there's lots of ambiguity about unsigned vs. signed characters in packets.
 * put simply, it's quite a mess. Further, standard library stuff all assumes signed
 * characters. We're going to assume IO packets are signed, and build on that.
 * but it's still going to be cast hell.
 */
extern char remcom_out_buffer[];
extern char remcom_in_buffer[];
/* gdbstub interface functions */
extern int gdb_serial_stub(struct state *ks, int port);
extern void gdbstub_msg_write(const char *s, int len);

// And, yeah, since packets are signed, this takes a signed.
void error_packet(char *pkt, char *error);
char *errstring(char *prefix);


/* gdbstub functions used for kdb <-> gdbstub transition */
extern char *gdbstub_state(struct state *ks, char *cmd);
extern int dbg_kdb_mode;

char *gpr(struct state *ks, int pid);
char *rmem(void *dest, int pid, uint64_t addr, int size);
char *wmem(uint64_t dest, int pid, void *addr, int size);
#define MAX_BREAKPOINTS 32768
