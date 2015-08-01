/**
 * @file cpu_buffer.c
 *
 * @remark Copyright 2002-2009 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 * @author Barry Kasindorf <barry.kasindorf@amd.com>
 * @author Robert Richter <robert.richter@amd.com>
 *
 * Each CPU has a local buffer that stores PC value/event
 * pairs. We also log context switches when we notice them.
 * Eventually each CPU's buffer is processed into the global
 * event buffer by sync_buffer().
 *
 * We use a local buffer for two reasons: an NMI or similar
 * interrupt cannot synchronise, and high sampling rates
 * would lead to catastrophic global synchronisation if
 * a global buffer was used.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include        "cpu_buffer.h"
#include <oprofile.h>

#define OP_BUFFER_FLAGS	0
int num_cpus = 8; // FIXME -- where do we get this.

/* we allocate an array of these and set the pointer in mach */
struct oprofile_cpu_buffer *op_cpu_buffer;

/* this one queue is used by #K to get all events. */
static Queue *opq;

/* this is run from core 0 for all cpu buffers. */
//static void wq_sync_buffer(void);
unsigned long oprofile_cpu_buffer_size = 65536;
unsigned long oprofile_backtrace_depth = 16;

#define DEFAULT_TIMER_EXPIRE (HZ / 10)
static int work_enabled;

/*
 * Resets the cpu buffer to a sane state.
 *
 * reset these to invalid values; the next sample collected will
 * populate the buffer with proper values to initialize the buffer
 */
static inline void op_cpu_buffer_reset(int cpu)
{
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];

	cpu_buf->last_is_kernel = -1;
	cpu_buf->last_proc = nil;
	//print_func_exit();
}

/* returns the remaining free size of data in the entry */
static inline
	int op_cpu_buffer_add_data(struct op_entry *entry, unsigned long val)
{
	//print_func_entry();
	if (!entry->size) {
		//print_func_exit();
		return 0;
	}
	*entry->data = val;
	entry->size--;
	entry->data++;
	//print_func_exit();
	return entry->size;
}

/* returns the size of data in the entry */
static inline int op_cpu_buffer_get_size(struct op_entry *entry)
{
	//print_func_entry();
	//print_func_exit();
	return entry->size;
}

/* returns 0 if empty or the size of data including the current value */
static inline
	int op_cpu_buffer_get_data(struct op_entry *entry, unsigned long *val)
{
	//print_func_entry();
	int size = entry->size;
	if (!size) {
		//print_func_exit();
		return 0;
	}
	*val = *entry->data;
	entry->size--;
	entry->data++;
	//print_func_exit();
	return size;
}

unsigned long oprofile_get_cpu_buffer_size(void)
{
	//print_func_entry();
	//print_func_exit();
	return oprofile_cpu_buffer_size;
}

void oprofile_cpu_buffer_inc_smpl_lost(void)
{
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];

	cpu_buf->sample_lost_overflow++;
	//print_func_exit();
}

void free_cpu_buffers(void)
{
	//print_func_entry();
	free(op_cpu_buffer);
	/* we can just leave the queue set up; it will then always return EOF */
	//print_func_exit();
}

#define RB_EVENT_HDR_SIZE 4

int alloc_cpu_buffers(void)
{
	//print_func_entry();
	/* should probably start using waserror() here. The fail stuff just gets
	 * ugly.
	 */
	int i;
	unsigned long buffer_size = oprofile_cpu_buffer_size;
	/* this can get called lots of times. Things might have been freed.
	 * So be careful.
	 */
	/* what limit? No idea. */
	if (!opq)
		opq = qopen(1024, 0, nil, nil);
	if (!opq)
		goto fail;

	/* we *really* don't want to block. Losing data is better. */
	qnoblock(opq, 1);
	if (!op_cpu_buffer) {
		op_cpu_buffer = smalloc(sizeof(*op_cpu_buffer) * num_cpus);
		if (!op_cpu_buffer)
			goto fail;

		for (i = 0; i < num_cpus; i++) {
			struct oprofile_cpu_buffer *b = &op_cpu_buffer[i];
			b->last_proc = nil;
			b->last_is_kernel = -1;
			b->tracing = 0;
			b->buffer_size = buffer_size;
			b->sample_received = 0;
			b->sample_lost_overflow = 0;
			b->backtrace_aborted = 0;
			b->sample_invalid_eip = 0;
			b->cpu = i;
			b->fullqueue = qopen(1024, Qmsg, nil, nil);
			b->emptyqueue = qopen(1024, Qmsg, nil, nil);
		}
	}

	//print_func_exit();
	return 0;

fail:
	free_cpu_buffers();
	//print_func_exit();
	panic("alloc_cpu_buffers");
	return -1;
}

void start_cpu_work(void)
{
	//print_func_entry();

	work_enabled = 1;
	//print_func_exit();
}

void end_cpu_work(void)
{
	//print_func_entry();
	work_enabled = 0;
	//print_func_exit();
}

/* placeholder. Not used yet.
 */
void flush_cpu_work(void)
{
	//print_func_entry();

	//struct oprofile_cpu_buffer *b = &op_cpu_buffer[machp()->machno];

	//print_func_exit();
}

/* Not used since we're not doing per-cpu buffering yet.
 */

struct op_sample *op_cpu_buffer_read_entry(struct op_entry *entry, int cpu)
{
	//print_func_entry();
	//print_func_exit();
	return nil;
}

static Block *op_cpu_buffer_write_reserve(struct oprofile_cpu_buffer *cpu_buf,
	struct op_entry *entry, int size)
{
	//print_func_entry();
	// Block *b; this gets some bizarre gcc set but not used error.

	int totalsize = sizeof(struct op_sample) +
		size * sizeof(entry->sample->data[0]);

	Block *b = cpu_buf->block;
	/* we might have run out. */
	if ((! b) || (b->lim - b->wp) < size) {
		if (b){
			qibwrite(opq, b);
		}
		/* For now. Later, we will grab a block off the
		 * emptyblock queue.
		 */
		cpu_buf->block = b = iallocb(oprofile_cpu_buffer_size);
		if (!b) {
			print("%s: fail\n", __func__);
			//print_func_exit();
			return nil;
		}
	}
	entry->sample = (void *)b->wp;
	entry->size = size;
	entry->data = entry->sample->data;

	b->wp += totalsize;
	//print_func_exit();
	return b;

}

static int
op_add_code(struct oprofile_cpu_buffer *cpu_buf, unsigned long backtrace,
			int is_kernel, Proc *proc)
{
	Proc *up = machp()->externup;
	//print_func_entry();
	// Block *b; this gets some bizarre gcc set but not used error. 	Block *b;
	struct op_entry entry;
	unsigned long flags;
	int size;

	flags = 0;

	if (waserror()) {
		poperror();
		print("%s: failed\n", __func__);
		//print_func_exit();
		return 1;
	}

	if (backtrace)
		flags |= TRACE_BEGIN;

	/* notice a switch from user->kernel or vice versa */
	is_kernel = ! !is_kernel;
	if (cpu_buf->last_is_kernel != is_kernel) {
		cpu_buf->last_is_kernel = is_kernel;
		flags |= KERNEL_CTX_SWITCH;
		if (is_kernel)
			flags |= IS_KERNEL;
	}

	/* notice a proc switch */
	if (cpu_buf->last_proc != proc) {
		cpu_buf->last_proc = proc;
		flags |= USER_CTX_SWITCH;
	}

	if (!flags) {
		poperror();
		/* nothing to do */
		//print_func_exit();
		return 0;
	}

	if (flags & USER_CTX_SWITCH)
		size = 1;
	else
		size = 0;

	op_cpu_buffer_write_reserve(cpu_buf, &entry, size);

	entry.sample->eip = ESCAPE_CODE;
	entry.sample->event = flags;

	if (size)
		op_cpu_buffer_add_data(&entry, (unsigned long)proc);

	poperror();
	//print_func_exit();
	return 0;
}

static inline int
op_add_sample(struct oprofile_cpu_buffer *cpu_buf,
			  unsigned long pc, unsigned long event)
{
	Proc *up = machp()->externup;
	//print_func_entry();
	struct op_entry entry;
	struct op_sample *sample;
	// Block *b; this gets some bizarre gcc set but not used error. 	Block *b;

	if (waserror()) {
		poperror();
		print("%s: failed\n", __func__);
		//print_func_exit();
		return 1;
	}

	op_cpu_buffer_write_reserve(cpu_buf, &entry, 0);

	sample = entry.sample;
	sample->eip = pc;
	sample->event = event;
	poperror();
	//print_func_exit();
	return 0;
}

/*
 * This must be safe from any context.
 *
 * is_kernel is needed because on some architectures you cannot
 * tell if you are in kernel or user space simply by looking at
 * pc. We tag this in the buffer by generating kernel enter/exit
 * events whenever is_kernel changes
 */
static int
log_sample(struct oprofile_cpu_buffer *cpu_buf, unsigned long pc,
		   unsigned long backtrace, int is_kernel, unsigned long event,
		   Proc *proc)
{
	//print_func_entry();
	Proc *tsk = proc ? proc : machp()->externup;
	cpu_buf->sample_received++;

	if (pc == ESCAPE_CODE) {
		cpu_buf->sample_invalid_eip++;
		//print_func_exit();
		return 0;
	}

	/* ah, so great. op_add* return 1 in event of failure.
	 * this function returns 0 in event of failure.
	 * what a cluster.
	 */
	lock(&cpu_buf->lock);
	if (op_add_code(cpu_buf, backtrace, is_kernel, tsk))
		goto fail;

	if (op_add_sample(cpu_buf, pc, event))
		goto fail;
	unlock(&cpu_buf->lock);

	//print_func_exit();
	return 1;

fail:
	cpu_buf->sample_lost_overflow++;
	//print_func_exit();
	return 0;
}

static inline void oprofile_begin_trace(struct oprofile_cpu_buffer *cpu_buf)
{
	//print_func_entry();
	cpu_buf->tracing = 1;
	//print_func_exit();
}

static inline void oprofile_end_trace(struct oprofile_cpu_buffer *cpu_buf)
{
	//print_func_entry();
	cpu_buf->tracing = 0;
	//print_func_exit();
}

void oprofile_cpubuf_flushone(int core, int newbuf)
{
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf;
	cpu_buf = &op_cpu_buffer[core];
	lock(&cpu_buf->lock);
	if (cpu_buf->block) {
		print("Core %d has data\n", core);
		qibwrite(opq, cpu_buf->block);
		print("After qibwrite in %s, opq len %d\n", __func__, qlen(opq));
	}
	if (newbuf)
		cpu_buf->block = iallocb(oprofile_cpu_buffer_size);
	else
		cpu_buf->block = nil;
	unlock(&cpu_buf->lock);
	//print_func_exit();
}

void oprofile_cpubuf_flushall(int alloc)
{
	//print_func_entry();
	int core;

	for(core = 0; core < num_cpus; core++) {
		oprofile_cpubuf_flushone(core, alloc);
	}
	//print_func_exit();
}

void oprofile_control_trace(int onoff)
{
	//print_func_entry();
	int core;
	struct oprofile_cpu_buffer *cpu_buf;

	for(core = 0; core < num_cpus; core++) {
		cpu_buf = &op_cpu_buffer[core];
		cpu_buf->tracing = onoff;

		if (onoff) {
			print("Enable tracing on %d\n", core);
			continue;
		}

		/* halting. Force out all buffers. */
		oprofile_cpubuf_flushone(core, 0);
	}
	//print_func_exit();
}

static inline void
__oprofile_add_ext_sample(unsigned long pc,
						  void /*struct pt_regs */ *const regs,
						  unsigned long event, int is_kernel, Proc *proc)
{
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];
	unsigned long backtrace = oprofile_backtrace_depth;

	/*
	 * if log_sample() fail we can't backtrace since we lost the
	 * source of this event
	 */
	if (!log_sample(cpu_buf, pc, backtrace, is_kernel, event, proc))
		/* failed */
	{
		//print_func_exit();
		return;
	}

	if (!backtrace) {
		//print_func_exit();
		return;
	}
#if 0
	oprofile_begin_trace(cpu_buf);
	oprofile_ops.backtrace(regs, backtrace);
	oprofile_end_trace(cpu_buf);
#endif
	//print_func_exit();
}

void oprofile_add_ext_hw_sample(unsigned long pc,
				Ureg *regs,
				unsigned long event, int is_kernel,
				Proc *proc)
{
	//print_func_entry();
	__oprofile_add_ext_sample(pc, regs, event, is_kernel, proc);
	//print_func_exit();
}

void oprofile_add_ext_sample(unsigned long pc,
							 void /*struct pt_regs */ *const regs,
							 unsigned long event, int is_kernel)
{
	//print_func_entry();
	__oprofile_add_ext_sample(pc, regs, event, is_kernel, nil);
	//print_func_exit();
}

void oprofile_add_sample(void /*struct pt_regs */ *const regs,
						 unsigned long event)
{
	//print_func_entry();
	int is_kernel;
	unsigned long pc;

	if (regs) {
		is_kernel = 0;	// FIXME!user_mode(regs);
		pc = 0;	// FIXME profile_pc(regs);
	} else {
		is_kernel = 0;	/* This value will not be used */
		pc = ESCAPE_CODE;	/* as this causes an early return. */
	}

	__oprofile_add_ext_sample(pc, regs, event, is_kernel, nil);
	//print_func_exit();
}

/*
 * Add samples with data to the ring buffer.
 *
 * Use oprofile_add_data(&entry, val) to add data and
 * oprofile_write_commit(&entry) to commit the sample.
 */
void
oprofile_write_reserve(struct op_entry *entry,
		       Ureg *regs,
		       unsigned long pc, int code, int size)
{
	Proc *up = machp()->externup;
	//print_func_entry();
	struct op_sample *sample;
	// Block *b; this gets some bizarre gcc set but not used error. 	Block *b;
	int is_kernel = 0;			// FIXME!user_mode(regs);
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];

	if (waserror()) {
		print("%s: failed\n", __func__);
		poperror();
		goto fail;
	}
	cpu_buf->sample_received++;

	/* no backtraces for samples with data */
	if (op_add_code(cpu_buf, 0, is_kernel,machp()->externup))
		goto fail;

	op_cpu_buffer_write_reserve(cpu_buf, entry, size + 2);
	sample = entry->sample;
	sample->eip = ESCAPE_CODE;
	sample->event = 0;	/* no flags */

	op_cpu_buffer_add_data(entry, code);
	op_cpu_buffer_add_data(entry, pc);
	poperror();
	//print_func_exit();
	return;
fail:
	entry->event = nil;
	cpu_buf->sample_lost_overflow++;
	//print_func_exit();
}

int oprofile_add_data(struct op_entry *entry, unsigned long val)
{
	//print_func_entry();
	if (!entry->event) {
		//print_func_exit();
		return 0;
	}
	//print_func_exit();
	return op_cpu_buffer_add_data(entry, val);
}

int oprofile_add_data64(struct op_entry *entry, uint64_t val)
{
	//print_func_entry();
	if (!entry->event) {
		//print_func_exit();
		return 0;
	}
	if (op_cpu_buffer_get_size(entry) < 2)
		/*
		 * the function returns 0 to indicate a too small
		 * buffer, even if there is some space left
		 */
	{
		//print_func_exit();
		return 0;
	}
	if (!op_cpu_buffer_add_data(entry, (uint32_t) val)) {
		//print_func_exit();
		return 0;
	}
	//print_func_exit();
	return op_cpu_buffer_add_data(entry, (uint32_t) (val >> 32));
}

int oprofile_write_commit(struct op_entry *entry)
{
	//print_func_entry();
	/* not much to do at present. In future, we might write the Block
	 * to opq.
	 */
	//print_func_exit();
	return 0;
}

void oprofile_add_pc(unsigned long pc, int is_kernel, unsigned long event)
{
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];
	log_sample(cpu_buf, pc, 0, is_kernel, event, nil);
	//print_func_exit();
}

void oprofile_add_trace(unsigned long pc)
{
	if (! op_cpu_buffer)
		return;
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];

	if (!cpu_buf->tracing) {
		//print_func_exit();
		return;
	}

	/*
	 * broken frame can give an eip with the same value as an
	 * escape code, abort the trace if we get it
	 */
	if (pc == ESCAPE_CODE)
		goto fail;
	if (op_add_sample(cpu_buf, pc, fastticks2ns(rdtsc())))
		goto fail;

	//print_func_exit();
	return;
fail:
	print("%s: fail. Turning of tracing on cpu %d\n", machp()->machno);
	cpu_buf->tracing = 0;
	cpu_buf->backtrace_aborted++;
	//print_func_exit();
	return;
}

/* Format for samples:
 * first word:
 * high 8 bits is ee, which is an invalid address on amd64.
 * next 8 bits is protocol version
 * next 16 bits is unused, MBZ. Later, we can make it a packet type.
 * next 16 bits is core id
 * next 8 bits is unused
 * next 8 bits is # PCs following. This should be at least 1, for one EIP.
 *
 * second word is time in ns.
 *
 * Third and following words are PCs, there must be at least one of them.
 */
void oprofile_add_backtrace(uintptr_t pc, uintptr_t fp)
{
	/* version 1. */
	uint64_t descriptor = 0xee01ULL<<48;
	if (! op_cpu_buffer)
		return;
	//print_func_entry();
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];

	if (!cpu_buf->tracing) {
		//print_func_exit();
		return;
	}

	struct op_entry entry;
	struct op_sample *sample;
	// Block *b; this gets some bizarre gcc set but not used error. 	Block *b;
	uint64_t event = fastticks2ns(rdtsc());

	uintptr_t bt_pcs[oprofile_backtrace_depth];

	int nr_pcs;
	nr_pcs = backtrace_list(pc, fp, bt_pcs, oprofile_backtrace_depth);

	/* write_reserve always assumes passed-in-size + 2.
	 * backtrace_depth should always be > 0.
	 */
	if (!op_cpu_buffer_write_reserve(cpu_buf, &entry, nr_pcs))
		return;

	/* we are changing the sample format, but not the struct
	 * member names yet. Later, assuming this works out.
	 */
	descriptor |= (machp()->machno << 16) | nr_pcs;
	sample = entry.sample;
	sample->eip = descriptor;
	sample->event = event;
	memmove(sample->data, bt_pcs, sizeof(uintptr_t) * nr_pcs);

	//print_func_exit();
	return;
}

void oprofile_add_userpc(uintptr_t pc)
{
	struct oprofile_cpu_buffer *cpu_buf;
	uint32_t pcoreid = machp()->machno;
	struct op_entry entry;
	// Block *b; this gets some bizarre gcc set but not used error.
	uint64_t descriptor = (0xee01ULL << 48) | (pcoreid << 16) | 1;

	if (!op_cpu_buffer)
		return;
	cpu_buf = &op_cpu_buffer[pcoreid];
	if (!cpu_buf->tracing)
		return;
	/* write_reserve always assumes passed-in-size + 2.  need room for 1 PC. */
	Block *b = op_cpu_buffer_write_reserve(cpu_buf, &entry, 1);
	if (!b)
		return;
	entry.sample->eip = descriptor;
	entry.sample->event = fastticks2ns(rdtsc());
	/* entry.sample->data == entry.data */
	assert(entry.sample->data == entry.data);
	*entry.sample->data = pc;
}

int
oproflen(void)
{
	return qlen(opq);
}

/* return # bytes read, or 0 if profiling is off, or block if profiling on and no data.
 */
int
oprofread(void *va, int n)
{
	int len = qlen(opq);
	struct oprofile_cpu_buffer *cpu_buf = &op_cpu_buffer[machp()->machno];
	if (len == 0) {
		if (cpu_buf->tracing == 0)
			return 0;
	}

	len = qread(opq, va, n);
	return len;
}
