/* Copyright (C) 1991, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: zcontext.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Display PostScript context operators */
#include "memory_.h"
#include "ghost.h"
#include "gp.h"			/* for usertime */
#include "oper.h"
#include "gsexit.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gxalloc.h"
#include "gxstate.h"		/* for copying gstate stack */
#include "stream.h"		/* for files.h */
#include "files.h"
#include "idict.h"
#include "igstate.h"
#include "icontext.h"
#include "interp.h"
#include "isave.h"
#include "istruct.h"
#include "dstack.h"
#include "estack.h"
#include "ostack.h"
#include "store.h"

/*
 * Define the rescheduling interval.  A value of max_int effectively
 * disables scheduling.  The only reason not to make this const is to
 * allow it to be changed during testing.
 */
private int reschedule_interval = 100;

/* Scheduling hooks in interp.c */
extern int (*gs_interp_reschedule_proc)(P1(i_ctx_t **));
extern int (*gs_interp_time_slice_proc)(P1(i_ctx_t **));
extern int gs_interp_time_slice_ticks;

/* Context structure */
typedef enum {
    cs_active,
    cs_done
} ctx_status;
typedef struct gs_context_s gs_context;
typedef struct gs_scheduler_s gs_scheduler_t;

/*
 * If several contexts share local VM, then if any one of them has done an
 * unmatched save, the others are not allowed to run.  We handle this by
 * maintaining the following invariant:
 *      When control reaches the point in the scheduler that decides
 *      what context to run next, then for each group of contexts
 *      sharing local VM, if the save level for that VM is non-zero,
 *      saved_local_vm is only set in the context that has unmatched
 *      saves.
 * We maintain this invariant as follows: when control enters the
 * scheduler, if a context was running, we set its saved_local_vm flag
 * to (save_level > 0).  When selecting a context to run, we ignore
 * contexts where saved_local_vm is false and the local VM save_level > 0.
 */
struct gs_context_s {
    gs_context_state_t state;	/* (must be first for subclassing) */
    /* Private state */
    gs_scheduler_t *scheduler;
    ctx_status status;
    ulong index;
    bool detach;		/* true if a detach has been */
				/* executed for this context */
    bool saved_local_vm;	/* (see above) */
    gs_context *next;		/* next context with same status */
				/* (active, waiting on same lock, */
				/* waiting on same condition, */
				/* waiting to be destroyed) */
    gs_context *joiner;		/* context waiting on a join */
				/* for this one */
    gs_context *table_next;	/* hash table chain */
};

/* GC descriptor */
private 
CLEAR_MARKS_PROC(context_clear_marks)
{
    gs_context *const pctx = vptr;

    (*st_context_state.clear_marks)
	(&pctx->state, sizeof(pctx->state), &st_context_state);
}
private 
ENUM_PTRS_BEGIN(context_enum_ptrs)
ENUM_PREFIX(st_context_state, 4);
ENUM_PTR3(0, gs_context, scheduler, next, joiner);
ENUM_PTR(3, gs_context, table_next);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(context_reloc_ptrs)
RELOC_PREFIX(st_context_state);
RELOC_PTR3(gs_context, scheduler, next, joiner);
RELOC_PTR(gs_context, table_next);
RELOC_PTRS_END
gs_private_st_complex_only(st_context, gs_context, "context",
	     context_clear_marks, context_enum_ptrs, context_reloc_ptrs, 0);

/* Context list structure */
typedef struct ctx_list_s {
    gs_context *head;
    gs_context *tail;
} ctx_list;

/* Condition structure */
typedef struct gs_condition_s {
    ctx_list waiting;	/* contexts waiting on this condition */
} gs_condition;
gs_private_st_ptrs2(st_condition, gs_condition, "conditiontype",
     condition_enum_ptrs, condition_reloc_ptrs, waiting.head, waiting.tail);

/* Lock structure */
typedef struct gs_lock_s {
    ctx_list waiting;	/* contexts waiting for this lock, */
    /* must be first for subclassing */
    gs_context *holder;	/* context holding the lock, if any */
} gs_lock;
gs_private_st_suffix_add1(st_lock, gs_lock, "locktype",
			  lock_enum_ptrs, lock_reloc_ptrs, st_condition, holder);

/* Global state */
/*typedef struct gs_scheduler_s gs_scheduler_t; *//* (above) */
struct gs_scheduler_s {
    gs_context *current;
    long usertime_initial;	/* usertime when current started running */
    ctx_list active;
    gs_context *dead;
#define CTX_TABLE_SIZE 19
    gs_context *table[CTX_TABLE_SIZE];
};

/* Structure definition */
gs_private_st_composite(st_scheduler, gs_scheduler_t, "gs_scheduler",
			scheduler_enum_ptrs, scheduler_reloc_ptrs);
private ENUM_PTRS_BEGIN(scheduler_enum_ptrs)
{
    index -= 4;
    if (index < CTX_TABLE_SIZE)
	ENUM_RETURN_PTR(gs_scheduler_t, table[index]);
    return 0;
}
ENUM_PTR3(0, gs_scheduler_t, current, active.head, active.tail);
ENUM_PTR(3, gs_scheduler_t, dead);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(scheduler_reloc_ptrs)
{
    RELOC_PTR3(gs_scheduler_t, current, active.head, active.tail);
    RELOC_PTR(gs_scheduler_t, dead);
    {
	int i;

	for (i = 0; i < CTX_TABLE_SIZE; ++i)
	    RELOC_PTR(gs_scheduler_t, table[i]);
    }
}
RELOC_PTRS_END

/* Forward references */
private int context_create(P5(gs_scheduler_t *, gs_context **,
			      const gs_dual_memory_t *,
			      const gs_context_state_t *, bool));
private long context_usertime(P0());
private int context_param(P3(const gs_scheduler_t *, os_ptr, gs_context **));
private void context_destroy(P1(gs_context *));
private void stack_copy(P4(ref_stack_t *, const ref_stack_t *, uint, uint));
private int lock_acquire(P2(os_ptr, gs_context *));
private int lock_release(P1(ref *));

/* Internal procedures */
private void
context_load(gs_scheduler_t *psched, gs_context *pctx)
{
    if_debug1('"', "[\"]loading %ld\n", pctx->index);
    if ( pctx->state.keep_usertime )
      psched->usertime_initial = context_usertime();
    context_state_load(&pctx->state);
}
private void
context_store(gs_scheduler_t *psched, gs_context *pctx)
{
    if_debug1('"', "[\"]storing %ld\n", pctx->index);
    context_state_store(&pctx->state);
    if ( pctx->state.keep_usertime )
      pctx->state.usertime_total +=
        context_usertime() - psched->usertime_initial;
}

/* List manipulation */
private void
add_last(ctx_list *pl, gs_context *pc)
{
    pc->next = 0;
    if (pl->head == 0)
	pl->head = pc;
    else
	pl->tail->next = pc;
    pl->tail = pc;
}

/* ------ Initialization ------ */

private int ctx_initialize(P1(i_ctx_t **));
private int ctx_reschedule(P1(i_ctx_t **));
private int ctx_time_slice(P1(i_ctx_t **));
private int
zcontext_init(i_ctx_t *i_ctx_p)
{
    /* Complete initialization after the interpreter is entered. */
    gs_interp_reschedule_proc = ctx_initialize;
    gs_interp_time_slice_proc = ctx_initialize;
    gs_interp_time_slice_ticks = 0;
    return 0;
}
/*
 * The interpreter calls this procedure at the first reschedule point.
 * It completes context initialization.
 */
private int
ctx_initialize(i_ctx_t **pi_ctx_p)
{
    i_ctx_t *i_ctx_p = *pi_ctx_p; /* for gs_imemory */
    gs_ref_memory_t *imem = iimemory_system;
    gs_scheduler_t *psched =
	gs_alloc_struct((gs_memory_t *) imem, gs_scheduler_t,
			&st_scheduler, "gs_scheduler");

    psched->current = 0;
    psched->active.head = psched->active.tail = 0;
    psched->dead = 0;
    memset(psched->table, 0, sizeof(psched->table));
    /* Create an initial context. */
    if (context_create(psched, &psched->current, &gs_imemory, *pi_ctx_p, true) < 0) {
	lprintf("Can't create initial context!");
	gs_abort();
    }
    psched->current->scheduler = psched;
    /* Hook into the interpreter. */
    *pi_ctx_p = &psched->current->state;
    gs_interp_reschedule_proc = ctx_reschedule;
    gs_interp_time_slice_proc = ctx_time_slice;
    gs_interp_time_slice_ticks = reschedule_interval;
    return 0;
}

/* ------ Interpreter interface to scheduler ------ */

/* When an operator decides it is time to run a new context, */
/* it returns o_reschedule.  The interpreter saves all its state in */
/* memory, calls ctx_reschedule, and then loads the state from memory. */
private int
ctx_reschedule(i_ctx_t **pi_ctx_p)
{
    gs_context *current = (gs_context *)*pi_ctx_p;
    gs_scheduler_t *psched = current->scheduler;

#ifdef DEBUG
    if (*pi_ctx_p != &current->state) {
	lprintf2("current->state = 0x%lx, != i_ctx_p = 0x%lx!\n",
		 (ulong)&current->state, (ulong)*pi_ctx_p);
    }
#endif
    /* If there are any dead contexts waiting to be released, */
    /* take care of that now. */
    while (psched->dead != 0) {
	gs_context *next = psched->dead->next;

	if (current == psched->dead) {
	    if_debug1('"', "[\"]storing dead %ld\n", current->index);
	    context_state_store(&current->state);
	    current = 0;
	}
	context_destroy(psched->dead);
	psched->dead = next;
    }
    /* Update saved_local_vm.  See above for the invariant. */
    if (current != 0)
	current->saved_local_vm =
	    current->state.memory.space_local->saved != 0;
    /* Run the first ready context, taking the 'save' lock into account. */
    {
	gs_context *prev = 0;
	gs_context *ready;

	for (ready = psched->active.head;;
	     prev = ready, ready = ready->next
	    ) {
	    if (ready == 0) {
		if (current != 0)
		    context_store(psched, current);
		lprintf("No context to run!");
		return_error(e_Fatal);
	    }
	    /* See above for an explanation of the following test. */
	    if (ready->state.memory.space_local->saved != 0 &&
		!ready->saved_local_vm
		)
		continue;
	    /* Found a context to run. */
	    {
		gs_context *next = ready->next;

		if (prev)
		    prev->next = next;
		else
		    psched->active.head = next;
		if (!next)
		    psched->active.tail = prev;
	    }
	    break;
	}
	if (ready == current)
	    return 0;		/* no switch */
	/*
	 * Save the state of the current context in psched->current,
	 * if any context is current.
	 */
	if (current != 0)
	    context_store(psched, current);
	psched->current = ready;
	/* Load the state of the new current context. */
	context_load(psched, ready);
	/* Switch the interpreter's context state pointer. */
	*pi_ctx_p = &ready->state;
    }
    return 0;
}

/* If the interpreter wants to time-slice, it saves its state, */
/* calls ctx_time_slice, and reloads its state. */
private int
ctx_time_slice(i_ctx_t **pi_ctx_p)
{
    gs_scheduler_t *psched = ((gs_context *)*pi_ctx_p)->scheduler;

    if (psched->active.head == 0)
	return 0;
    if_debug0('"', "[\"]time-slice\n");
    add_last(&psched->active, psched->current);
    return ctx_reschedule(pi_ctx_p);
}

/* ------ Context operators ------ */

/* - currentcontext <context> */
private int
zcurrentcontext(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const gs_context *current = (const gs_context *)i_ctx_p;

    push(1);
    make_int(op, current->index);
    return 0;
}

/* <context> detach - */
private int
zdetach(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const gs_scheduler_t *psched = ((gs_context *)i_ctx_p)->scheduler;
    gs_context *pctx;
    int code;

    if ((code = context_param(psched, op, &pctx)) < 0)
	return code;
    if_debug2('\'', "[']detach %ld, status = %d\n",
	      pctx->index, pctx->status);
    if (pctx->joiner != 0 || pctx->detach)
	return_error(e_invalidcontext);
    switch (pctx->status) {
	case cs_active:
	    pctx->detach = true;
	    break;
	case cs_done:
	    context_destroy(pctx);
    }
    pop(1);
    return 0;
}

private int
    do_fork(P6(i_ctx_t *i_ctx_p, os_ptr op, const ref * pstdin,
	       const ref * pstdout, uint mcount, bool local)),
    values_older_than(P4(const ref_stack_t * pstack, uint first, uint last,
			 int max_space));
private int
    fork_done(P1(i_ctx_t *)),
    fork_done_with_error(P1(i_ctx_t *)),
    finish_join(P1(i_ctx_t *)),
    reschedule_now(P1(i_ctx_t *));

/* <mark> <obj1> ... <objN> <proc> .fork <context> */
/* <mark> <obj1> ... <objN> <proc> <stdin|null> <stdout|null> */
/*   .localfork <context> */
private int
zfork(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint mcount = ref_stack_counttomark(&o_stack);
    ref rnull;

    if (mcount == 0)
	return_error(e_unmatchedmark);
    make_null(&rnull);
    return do_fork(i_ctx_p, op, &rnull, &rnull, mcount, false);
}
private int
zlocalfork(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint mcount = ref_stack_counttomark(&o_stack);
    int code;

    if (mcount == 0)
	return_error(e_unmatchedmark);
    code = values_older_than(&o_stack, 1, mcount - 1, avm_local);
    if (code < 0)
	return code;
    code = do_fork(i_ctx_p, op - 2, op - 1, op, mcount - 2, true);
    if (code < 0)
	return code;
    op = osp;
    op[-2] = *op;
    pop(2);
    return code;
}

/* Internal procedure to actually do the fork operation. */
private int
do_fork(i_ctx_t *i_ctx_p, os_ptr op, const ref * pstdin, const ref * pstdout,
	uint mcount, bool local)
{
    gs_context *pcur = (gs_context *)i_ctx_p;
    gs_scheduler_t *psched = pcur->scheduler;
    stream *s;
    gs_dual_memory_t dmem;
    gs_context *pctx;
    ref old_userdict, new_userdict;
    int code;

    check_proc(*op);
    if (iimemory_local->save_level)
	return_error(e_invalidcontext);
    if (r_has_type(pstdout, t_null)) {
	code = zget_stdout(i_ctx_p, &s);
	if (code < 0)
	    return code;
	pstdout = &ref_stdio[1];
    } else
	check_read_file(s, pstdout);
    if (r_has_type(pstdin, t_null)) {
	code = zget_stdin(i_ctx_p, &s);
	if (code < 0)
	    return code;
	pstdin = &ref_stdio[0];
    } else
	check_read_file(s, pstdin);
    dmem = gs_imemory;
    if (local) {
	/* Share global VM, private local VM. */
	ref *puserdict;
	uint userdict_size;
	gs_raw_memory_t *parent = iimemory_local->parent;
	gs_ref_memory_t *lmem;
	gs_ref_memory_t *lmem_stable;

	if (dict_find_string(systemdict, "userdict", &puserdict) <= 0 ||
	    !r_has_type(puserdict, t_dictionary)
	    )
	    return_error(e_Fatal);
	old_userdict = *puserdict;
	userdict_size = dict_maxlength(&old_userdict);
	lmem = ialloc_alloc_state(parent, iimemory_local->chunk_size);
	lmem_stable = ialloc_alloc_state(parent, iimemory_local->chunk_size);
	if (lmem == 0 || lmem_stable == 0) {
	    gs_free_object(parent, lmem_stable, "do_fork");
	    gs_free_object(parent, lmem, "do_fork");
	    return_error(e_VMerror);
	}
	lmem->space = avm_local;
	lmem_stable->space = avm_local;
	lmem->stable_memory = (gs_memory_t *)lmem_stable;
	dmem.space_local = lmem;
	code = context_create(psched, &pctx, &dmem, &pcur->state, false);
	if (code < 0) {
	    /****** FREE lmem ******/
	    return code;
	}
	/*
	 * Create a new userdict.  PostScript code will take care of
	 * the rest of the initialization of the new context.
	 */
	code = dict_alloc(lmem, userdict_size, &new_userdict);
	if (code < 0) {
	    context_destroy(pctx);
	    /****** FREE lmem ******/
	    return code;
	}
    } else {
	/* Share global and local VM. */
	code = context_create(psched, &pctx, &dmem, &pcur->state, false);
	if (code < 0) {
	    /****** FREE lmem ******/
	    return code;
	}
	/*
	 * Copy the gstate stack.  The current method is not elegant;
	 * in fact, I'm not entirely sure it works.
	 */
	{
	    int n;
	    const gs_state *old;
	    gs_state *new;

	    for (n = 0, old = igs; old != 0; old = gs_state_saved(old))
		++n;
	    for (old = pctx->state.pgs; old != 0; old = gs_state_saved(old))
		--n;
	    for (; n > 0 && code >= 0; --n)
		code = gs_gsave(pctx->state.pgs);
	    if (code < 0) {
/****** FREE lmem & GSTATES ******/
		return code;
	    }
	    for (old = igs, new = pctx->state.pgs;
		 old != 0 /* (== new != 0) */  && code >= 0;
		 old = gs_state_saved(old), new = gs_state_saved(new)
		)
		code = gs_setgstate(new, old);
	    if (code < 0) {
/****** FREE lmem & GSTATES ******/
		return code;
	    }
	}
    }
    pctx->state.language_level = i_ctx_p->language_level;
    pctx->state.dict_stack.min_size = idict_stack.min_size;
    pctx->state.dict_stack.userdict_index = idict_stack.userdict_index;
    pctx->state.stdio[0] = *pstdin;
    pctx->state.stdio[1] = *pstdout;
    pctx->state.stdio[2] = pcur->state.stdio[2];
    /* Initialize the interpreter stacks. */
    {
	ref_stack_t *dstack = (ref_stack_t *)&pctx->state.dict_stack;
	uint count = ref_stack_count(&d_stack);
	uint copy = (local ? min_dstack_size : count);

	ref_stack_push(dstack, copy);
	stack_copy(dstack, &d_stack, copy, count - copy);
	if (local) {
	    /* Substitute the new userdict for the old one. */
	    long i;

	    for (i = 0; i < copy; ++i) {
		ref *pdref = ref_stack_index(dstack, i);

		if (obj_eq(pdref, &old_userdict))
		    *pdref = new_userdict;
	    }
	}
    }
    {
	ref_stack_t *estack = (ref_stack_t *)&pctx->state.exec_stack;

	ref_stack_push(estack, 3);
	/* fork_done must be executed in both normal and error cases. */
	make_mark_estack(estack->p - 2, es_other, fork_done_with_error);
	make_oper(estack->p - 1, 0, fork_done);
	*estack->p = *op;
    }
    {
	ref_stack_t *ostack = (ref_stack_t *)&pctx->state.op_stack;
	uint count = mcount - 2;

	ref_stack_push(ostack, count);
	stack_copy(ostack, &o_stack, count, osp - op + 1);
    }
    pctx->state.binary_object_format = pcur->state.binary_object_format;
    add_last(&psched->active, pctx);
    pop(mcount - 1);
    op = osp;
    make_int(op, pctx->index);
    return 0;
}

/*
 * Check that all values being passed by fork or join are old enough
 * to be valid in the environment to which they are being transferred.
 */
private int
values_older_than(const ref_stack_t * pstack, uint first, uint last,
		  int next_space)
{
    uint i;

    for (i = first; i <= last; ++i)
	if (r_space(ref_stack_index(pstack, (long)i)) >= next_space)
	    return_error(e_invalidaccess);
    return 0;
}

/* This gets executed when a context terminates normally. */
/****** MUST DO ALL RESTORES ******/
/****** WHAT IF invalidrestore? ******/
private int
fork_done(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_context *pcur = (gs_context *)i_ctx_p;
    gs_scheduler_t *psched = pcur->scheduler;

    if_debug2('\'', "[']done %ld%s\n", pcur->index,
	      (pcur->detach ? ", detached" : ""));
    /*
     * Clear the context's dictionary, execution and graphics stacks
     * now, to retain as little as possible in case of a garbage
     * collection or restore.  We know that fork_done is the
     * next-to-bottom entry on the execution stack.
     */
    ref_stack_pop_to(&d_stack, min_dstack_size);
    pop_estack(&pcur->state, ref_stack_count(&e_stack) - 1);
    gs_grestoreall(igs);
    /*
     * If there are any unmatched saves, we need to execute restores
     * until there aren't.  An invalidrestore is possible and will
     * result in an error termination.
     */
    if (iimemory_local->save_level) {
	ref *prestore;

	if (dict_find_string(systemdict, "restore", &prestore) <= 0) {
	    lprintf("restore not found in systemdict!");
	    return_error(e_Fatal);
	}
	if (pcur->detach) {
	    ref_stack_clear(&o_stack);	/* help avoid invalidrestore */
	    op = osp;
	}
	push(1);
	make_tv(op, t_save, saveid, alloc_save_current_id(&gs_imemory));
	push_op_estack(fork_done);
	++esp;
	ref_assign(esp, prestore);
	return o_push_estack;
    }
    if (pcur->detach) {
	/*
	 * We would like to free the context's memory, but we can't do
	 * it yet, because the interpreter still has references to it.
	 * Instead, queue the context to be freed the next time we
	 * reschedule.  We can, however, clear its operand stack now.
	 */
	ref_stack_clear(&o_stack);
	context_store(psched, pcur);
	pcur->next = psched->dead;
	psched->dead = pcur;
	psched->current = 0;
    } else {
	gs_context *pctx = pcur->joiner;

	pcur->status = cs_done;
	/* Schedule the context waiting to join this one, if any. */
	if (pctx != 0)
	    add_last(&psched->active, pctx);
    }
    return o_reschedule;
}
/*
 * This gets executed when the stack is being unwound for an error
 * termination.
 */
private int
fork_done_with_error(i_ctx_t *i_ctx_p)
{
/****** WHAT TO DO? ******/
    return fork_done(i_ctx_p);
}

/* <context> join <mark> <obj1> ... <objN> */
private int
zjoin(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_context *current = (gs_context *)i_ctx_p;
    gs_scheduler_t *psched = current->scheduler;
    gs_context *pctx;
    int code;

    if ((code = context_param(psched, op, &pctx)) < 0)
	return code;
    if_debug2('\'', "[']join %ld, status = %d\n",
	      pctx->index, pctx->status);
    /*
     * It doesn't seem logically necessary, but the Red Book says that
     * the context being joined must share both global and local VM with
     * the current context.
     */
    if (pctx->joiner != 0 || pctx->detach || pctx == current ||
	pctx->state.memory.space_global !=
	current->state.memory.space_global ||
	pctx->state.memory.space_local !=
	current->state.memory.space_local ||
	iimemory_local->save_level != 0
	)
	return_error(e_invalidcontext);
    switch (pctx->status) {
	case cs_active:
	    /*
	     * We need to re-execute the join after the joined
	     * context is done.  Since we can't return both
	     * o_push_estack and o_reschedule, we push a call on
	     * reschedule_now, which accomplishes the latter.
	     */
	    check_estack(2);
	    push_op_estack(finish_join);
	    push_op_estack(reschedule_now);
	    pctx->joiner = current;
	    return o_push_estack;
	case cs_done:
	    {
		const ref_stack_t *ostack =
		    (ref_stack_t *)&pctx->state.op_stack;
		uint count = ref_stack_count(ostack);

		push(count);
		{
		    ref *rp = ref_stack_index(&o_stack, count);

		    make_mark(rp);
		}
		stack_copy(&o_stack, ostack, count, 0);
		context_destroy(pctx);
	    }
    }
    return 0;
}

/* Finish a deferred join. */
private int
finish_join(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_context *current = (gs_context *)i_ctx_p;
    gs_scheduler_t *psched = current->scheduler;
    gs_context *pctx;
    int code;

    if ((code = context_param(psched, op, &pctx)) < 0)
	return code;
    if_debug2('\'', "[']finish_join %ld, status = %d\n",
	      pctx->index, pctx->status);
    if (pctx->joiner != current)
	return_error(e_invalidcontext);
    pctx->joiner = 0;
    return zjoin(i_ctx_p);
}

/* Reschedule now. */
private int
reschedule_now(i_ctx_t *i_ctx_p)
{
    return o_reschedule;
}

/* - yield - */
private int
zyield(i_ctx_t *i_ctx_p)
{
    gs_context *current = (gs_context *)i_ctx_p;
    gs_scheduler_t *psched = current->scheduler;

    if (psched->active.head == 0)
	return 0;
    if_debug0('"', "[\"]yield\n");
    add_last(&psched->active, current);
    return o_reschedule;
}

/* ------ Condition and lock operators ------ */

private int
    monitor_cleanup(P1(i_ctx_t *)),
    monitor_release(P1(i_ctx_t *)),
    await_lock(P1(i_ctx_t *));
private void
     activate_waiting(P1(ctx_list * pcl));

/* - condition <condition> */
private int
zcondition(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_condition *pcond =
	ialloc_struct(gs_condition, &st_condition, "zcondition");

    if (pcond == 0)
	return_error(e_VMerror);
    pcond->waiting.head = pcond->waiting.tail = 0;
    push(1);
    make_istruct(op, a_all, pcond);
    return 0;
}

/* - lock <lock> */
private int
zlock(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_lock *plock = ialloc_struct(gs_lock, &st_lock, "zlock");

    if (plock == 0)
	return_error(e_VMerror);
    plock->holder = 0;
    plock->waiting.head = plock->waiting.tail = 0;
    push(1);
    make_istruct(op, a_all, plock);
    return 0;
}

/* <lock> <proc> monitor - */
private int
zmonitor(i_ctx_t *i_ctx_p)
{
    gs_context *current = (gs_context *)i_ctx_p;
    os_ptr op = osp;
    gs_lock *plock;
    gs_context *pctx;
    int code;

    check_stype(op[-1], st_lock);
    check_proc(*op);
    plock = r_ptr(op - 1, gs_lock);
    pctx = plock->holder;
    if_debug1('\'', "[']monitor 0x%lx\n", (ulong) plock);
    if (pctx != 0) {
	if (pctx == current ||
	    (iimemory_local->save_level != 0 &&
	     pctx->state.memory.space_local ==
	     current->state.memory.space_local)
	    )
	    return_error(e_invalidcontext);
    }
    /*
     * We push on the e-stack:
     *      The lock object
     *      An e-stack mark with monitor_cleanup, to release the lock
     *        in case of an error
     *      monitor_release, to release the lock in the normal case
     *      The procedure to execute
     */
    check_estack(4);
    code = lock_acquire(op - 1, current);
    if (code != 0) {		/* We didn't acquire the lock.  Re-execute this later. */
	push_op_estack(zmonitor);
	return code;		/* o_reschedule */
    }
    *++esp = op[-1];
    push_mark_estack(es_other, monitor_cleanup);
    push_op_estack(monitor_release);
    *++esp = *op;
    pop(2);
    return code;
}
/* Release the monitor lock when unwinding for an error or exit. */
private int
monitor_cleanup(i_ctx_t *i_ctx_p)
{
    int code = lock_release(esp);

    --esp;
    return code;
}
/* Release the monitor lock when the procedure completes. */
private int
monitor_release(i_ctx_t *i_ctx_p)
{
    --esp;
    return monitor_cleanup(i_ctx_p);
}

/* <condition> notify - */
private int
znotify(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_condition *pcond;

    check_stype(*op, st_condition);
    pcond = r_ptr(op, gs_condition);
    if_debug1('"', "[\"]notify 0x%lx\n", (ulong) pcond);
    pop(1);
    op--;
    if (pcond->waiting.head == 0)	/* nothing to do */
	return 0;
    activate_waiting(&pcond->waiting);
    return zyield(i_ctx_p);
}

/* <lock> <condition> wait - */
private int
zwait(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_lock *plock;
    gs_context *pctx;
    gs_condition *pcond;

    check_stype(op[-1], st_lock);
    plock = r_ptr(op - 1, gs_lock);
    check_stype(*op, st_condition);
    pcond = r_ptr(op, gs_condition);
    if_debug2('"', "[\"]wait lock 0x%lx, condition 0x%lx\n",
	      (ulong) plock, (ulong) pcond);
    pctx = plock->holder;
    if (pctx == 0 || pctx != pctx->scheduler->current ||
	(iimemory_local->save_level != 0 &&
	 (r_space(op - 1) == avm_local || r_space(op) == avm_local))
	)
	return_error(e_invalidcontext);
    check_estack(1);
    lock_release(op - 1);
    add_last(&pcond->waiting, pctx);
    push_op_estack(await_lock);
    return o_reschedule;
}
/* When the condition is signaled, wait for acquiring the lock. */
private int
await_lock(i_ctx_t *i_ctx_p)
{
    gs_context *current = (gs_context *)i_ctx_p;
    os_ptr op = osp;
    int code = lock_acquire(op - 1, current);

    if (code == 0) {
	pop(2);
	return 0;
    }
    /* We didn't acquire the lock.  Re-execute the wait. */
    push_op_estack(await_lock);
    return code;		/* o_reschedule */
}

/* Activate a list of waiting contexts, and reset the list. */
private void
activate_waiting(ctx_list * pcl)
{
    gs_context *pctx = pcl->head;
    gs_context *next;

    for (; pctx != 0; pctx = next) {
	gs_scheduler_t *psched = pctx->scheduler;

	next = pctx->next;
	add_last(&psched->active, pctx);
    }
    pcl->head = pcl->tail = 0;
}

/* ------ Miscellaneous operators ------ */

/* - usertime <int> */
private int
zusertime_context(i_ctx_t *i_ctx_p)
{
    gs_context *current = (gs_context *)i_ctx_p;
    gs_scheduler_t *psched = current->scheduler;
    os_ptr op = osp;
    long utime = context_usertime();

    push(1);
    if (!current->state.keep_usertime) {
	/*
	 * This is the first time this context has executed usertime:
	 * we must track its execution time from now on.
	 */
	psched->usertime_initial = utime;
	current->state.keep_usertime = true;
    }
    make_int(op, current->state.usertime_total + utime -
	     psched->usertime_initial);
    return 0;
}

/* ------ Internal procedures ------ */

/* Create a context. */
private int
context_create(gs_scheduler_t * psched, gs_context ** ppctx,
	       const gs_dual_memory_t * dmem,
	       const gs_context_state_t *i_ctx_p, bool copy_state)
{
    gs_ref_memory_t *mem = dmem->space_local;
    gs_context *pctx;
    int code;
    long ctx_index;
    gs_context **pte;

    pctx = gs_alloc_struct((gs_memory_t *) mem, gs_context, &st_context,
			   "context_create");
    if (pctx == 0)
	return_error(e_VMerror);
    if (copy_state) {
	pctx->state = *i_ctx_p;
    } else {
	gs_context_state_t *pctx_st = &pctx->state;

	code = context_state_alloc(&pctx_st, systemdict, dmem);
	if (code < 0) {
	    gs_free_object((gs_memory_t *) mem, pctx, "context_create");
	    return code;
	}
    }
    ctx_index = gs_next_ids(1);
    pctx->scheduler = psched;
    pctx->status = cs_active;
    pctx->index = ctx_index;
    pctx->detach = false;
    pctx->saved_local_vm = false;
    pctx->next = 0;
    pctx->joiner = 0;
    pte = &psched->table[ctx_index % CTX_TABLE_SIZE];
    pctx->table_next = *pte;
    *pte = pctx;
    *ppctx = pctx;
    if (gs_debug_c('\'') | gs_debug_c('"'))
	dlprintf2("[']create %ld at 0x%lx\n", ctx_index, (ulong) pctx);
    return 0;
}

/* Check a context ID.  Note that we do not check for context validity. */
private int
context_param(const gs_scheduler_t * psched, os_ptr op, gs_context ** ppctx)
{
    gs_context *pctx;
    long index;

    check_type(*op, t_integer);
    index = op->value.intval;
    if (index < 0)
	return_error(e_invalidcontext);
    pctx = psched->table[index % CTX_TABLE_SIZE];
    for (;; pctx = pctx->table_next) {
	if (pctx == 0)
	    return_error(e_invalidcontext);
	if (pctx->index == index)
	    break;
    }
    *ppctx = pctx;
    return 0;
}

/* Read the usertime as a single value. */
private long
context_usertime(void)
{
    long secs_ns[2];

    gp_get_usertime(secs_ns);
    return secs_ns[0] * 1000 + secs_ns[1] / 1000000;
}

/* Destroy a context. */
private void
context_destroy(gs_context * pctx)
{
    gs_ref_memory_t *mem = pctx->state.memory.space_local;
    gs_scheduler_t *psched = pctx->scheduler;
    gs_context **ppctx = &psched->table[pctx->index % CTX_TABLE_SIZE];

    while (*ppctx != pctx)
	ppctx = &(*ppctx)->table_next;
    *ppctx = (*ppctx)->table_next;
    if (gs_debug_c('\'') | gs_debug_c('"'))
	dlprintf3("[']destroy %ld at 0x%lx, status = %d\n",
		  pctx->index, (ulong) pctx, pctx->status);
    if (!context_state_free(&pctx->state))
	gs_free_object((gs_memory_t *) mem, pctx, "context_destroy");
}

/* Copy the top elements of one stack to another. */
/* Note that this does not push the elements: */
/* the destination stack must have enough space preallocated. */
private void
stack_copy(ref_stack_t * to, const ref_stack_t * from, uint count,
	   uint from_index)
{
    long i;

    for (i = (long)count - 1; i >= 0; --i)
	*ref_stack_index(to, i) = *ref_stack_index(from, i + from_index);
}

/* Acquire a lock.  Return 0 if acquired, o_reschedule if not. */
private int
lock_acquire(os_ptr op, gs_context * pctx)
{
    gs_lock *plock = r_ptr(op, gs_lock);

    if (plock->holder == 0) {
	plock->holder = pctx;
	return 0;
    }
    add_last(&plock->waiting, pctx);
    return o_reschedule;
}

/* Release a lock.  Return 0 if OK, e_invalidcontext if not. */
private int
lock_release(ref * op)
{
    gs_lock *plock = r_ptr(op, gs_lock);
    gs_context *pctx = plock->holder;

    if (pctx != 0 && pctx == pctx->scheduler->current) {
	plock->holder = 0;
	activate_waiting(&plock->waiting);
	return 0;
    }
    return_error(e_invalidcontext);
}

/* ------ Initialization procedure ------ */

/* We need to split the table because of the 16-element limit. */
const op_def zcontext1_op_defs[] = {
    {"0condition", zcondition},
    {"0currentcontext", zcurrentcontext},
    {"1detach", zdetach},
    {"2.fork", zfork},
    {"1join", zjoin},
    {"4.localfork", zlocalfork},
    {"0lock", zlock},
    {"2monitor", zmonitor},
    {"1notify", znotify},
    {"2wait", zwait},
    {"0yield", zyield},
		/* Note that the following replace prior definitions */
		/* in the indicated files: */
    {"0usertime", zusertime_context},	/* zmisc.c */
    op_def_end(0)
};
const op_def zcontext2_op_defs[] = {
		/* Internal operators */
    {"0%fork_done", fork_done},
    {"1%finish_join", finish_join},
    {"0%monitor_cleanup", monitor_cleanup},
    {"0%monitor_release", monitor_release},
    {"2%await_lock", await_lock},
    op_def_end(zcontext_init)
};
