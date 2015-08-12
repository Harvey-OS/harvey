/**
 * @file oprofile.h
 *
 * API for machine-specific interrupts to interface
 * to oprofile.
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 */

#ifndef OPROFILE_H
#define OPROFILE_H

/* Each escaped entry is prefixed by ESCAPE_CODE
 * then one of the following codes, then the
 * relevant data.
 * These #defines live in this file so that arch-specific
 * buffer sync'ing code can access them.
 */
#define ESCAPE_CODE ~0UL
#define CTX_SWITCH_CODE 1
#define CPU_SWITCH_CODE 2
#define COOKIE_SWITCH_CODE 3
#define KERNEL_ENTER_SWITCH_CODE 4
#define KERNEL_EXIT_SWITCH_CODE 5
#define MODULE_LOADED_CODE 6
#define CTX_TGID_CODE 7
#define TRACE_BEGIN_CODE 8
#define TRACE_END_CODE 9
#define XEN_ENTER_SWITCH_CODE 10
#define SPU_PROFILING_CODE 11
#define SPU_CTX_SWITCH_CODE 12
#define IBS_FETCH_CODE 13
#define IBS_OP_CODE 14

/**
 * One-time exit/cleanup for the arch.
 */
void oprofile_arch_exit(void);

/**
 * Add a sample. This may be called from any context.
 */
void oprofile_add_sample(void *const regs, unsigned long event);

/**
 * Add an extended sample.  Use this when the PC is not from the regs, and
 * we cannot determine if we're in kernel mode from the regs.
 *
 * This function does perform a backtrace.
 *
 */
void oprofile_add_ext_sample(unsigned long pc, void *const regs,
			     unsigned long event, int is_kernel);

/**
 * Add an hardware sample.
 */
void oprofile_add_ext_hw_sample(unsigned long pc, Ureg *regs,
				unsigned long event, int is_kernel,
				Proc *proc);

/* Use this instead when the PC value is not from the regs. Doesn't
 * backtrace. */
void oprofile_add_pc(unsigned long pc, int is_kernel, unsigned long event);

void oprofile_add_backtrace(uintptr_t pc, uintptr_t fp);
void oprofile_add_userpc(uintptr_t pc);

/* add a backtrace entry, to be called from the ->backtrace callback */
void oprofile_add_trace(unsigned long eip);

/**
 * Add the contents of a circular buffer to the event buffer.
 */
void oprofile_put_buff(unsigned long *buf, unsigned int start,
		       unsigned int stop, unsigned int max);

unsigned long oprofile_get_cpu_buffer_size(void);
void oprofile_cpu_buffer_inc_smpl_lost(void);

/* cpu buffer functions */

struct op_sample;

struct op_entry {
	void *event;
	struct op_sample *sample;
	unsigned long size;
	unsigned long *data;
};

void oprofile_write_reserve(struct op_entry *entry,
			    Ureg *regs,
			    unsigned long pc, int code, int size);
int oprofile_add_data(struct op_entry *entry, unsigned long val);
int oprofile_add_data64(struct op_entry *entry, uint64_t val);
int oprofile_write_commit(struct op_entry *entry);

void oprofile_perf_exit(void);
char *op_name_from_perf_id(void);

struct op_sample *op_cpu_buffer_read_entry(struct op_entry *entry, int cpu);
unsigned long op_cpu_buffer_entries(int cpu);
void oprofile_cpubuf_flushone(int core, int newbuf);
void oprofile_cpubuf_flushall(int alloc);
void oprofile_control_trace(int onoff);
int oprofread(void *, int);
int oproflen(void);

#endif /* OPROFILE_H */
