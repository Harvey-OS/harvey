/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gsnotify.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
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
			     void (*unreg_proc)(void *pdata))
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
