/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsnotify.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Notification machinery implementation */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsnotify.h"

/* GC descriptors */
private_st_gs_notify_registration();
public_st_gs_notify_list();

/* Initialize a notification list. */
void
gs_notify_init(gs_notify_list_t *nlist, gs_memory_t *mem)
{
    nlist->first = 0;
    nlist->memory = mem;
}

/* Register a client. */
int
gs_notify_register(gs_notify_list_t *nlist, gs_notify_proc_t proc,
		   void *proc_data)
{
    gs_notify_registration_t *nreg =
	gs_alloc_struct(nlist->memory, gs_notify_registration_t,
			&st_gs_notify_registration, "gs_notify_register");

    if (nreg == 0)
	return_error(gs_error_VMerror);
    nreg->proc = proc;
    nreg->proc_data = proc_data;
    nreg->next = nlist->first;
    nlist->first = nreg;
    return 0;
}

/*
 * Unregister a client.  Return 1 if the client was registered, 0 if not.
 * If proc_data is 0, unregister all registrations of that proc; otherwise,
 * unregister only the registration of that procedure with that proc_data.
 */
private void
no_unreg_proc(void *pdata)
{
}
int
gs_notify_unregister_calling(gs_notify_list_t *nlist, gs_notify_proc_t proc,
			     void *proc_data,
			     void (*unreg_proc)(P1(void *pdata)))
{
    gs_notify_registration_t **prev = &nlist->first;
    gs_notify_registration_t *cur;
    bool found = 0;

    while ((cur = *prev) != 0)
	if (cur->proc == proc &&
	    (proc_data == 0 || cur->proc_data == proc_data)
	    ) {
	    *prev = cur->next;
	    unreg_proc(cur->proc_data);
	    gs_free_object(nlist->memory, cur, "gs_notify_unregister");
	    found = 1;
	} else
	    prev = &cur->next;
    return found;
}
int
gs_notify_unregister(gs_notify_list_t *nlist, gs_notify_proc_t proc,
		     void *proc_data)
{
    return gs_notify_unregister_calling(nlist, proc, proc_data, no_unreg_proc);
}

/*
 * Notify the clients on a list.  If an error occurs, return the first
 * error code, but notify all clients regardless.
 */
int
gs_notify_all(gs_notify_list_t *nlist, void *event_data)
{
    gs_notify_registration_t *cur;
    gs_notify_registration_t *next;
    int ecode = 0;

    for (next = nlist->first; (cur = next) != 0;) {
	int code;

	next = cur->next;
	code = cur->proc(cur->proc_data, event_data);
	if (code < 0 && ecode == 0)
	    ecode = code;
    }
    return ecode;
}

/* Release a notification list. */
void
gs_notify_release(gs_notify_list_t *nlist)
{
    gs_memory_t *mem = nlist->memory;

    while (nlist->first) {
	gs_notify_registration_t *next = nlist->first->next;

	gs_free_object(mem, nlist->first, "gs_notify_release");
	nlist->first = next;
    }
}
