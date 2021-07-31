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

/*$Id: gsnotify.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Notification machinery */

#ifndef gsnotify_INCLUDED
#  define gsnotify_INCLUDED

#include "gsstype.h"		/* for extern_st */

/*
 * An arbitrary number of clients may register themselves to be notified
 * when an event occurs.  Duplicate registrations are not detected.  Clients
 * must unregister themselves when they are being freed (finalized), if not
 * before.  Objects that provide notification must notify clients when the
 * object is being freed (finalized): in this event, and only in this event,
 * event_data = NULL.
 */

/* Define the structure used to keep track of registrations. */
#define GS_NOTIFY_PROC(proc)\
    int proc(P2(void *proc_data, void *event_data))
typedef GS_NOTIFY_PROC((*gs_notify_proc_t));
typedef struct gs_notify_registration_s gs_notify_registration_t;
struct gs_notify_registration_s {
    gs_notify_proc_t proc;
    void *proc_data;
    gs_notify_registration_t *next;
};
#define private_st_gs_notify_registration() /* in gsnotify.c */\
  gs_private_st_ptrs2(st_gs_notify_registration, gs_notify_registration_t,\
    "gs_notify_registration_t", notify_registration_enum_ptrs,\
    notify_registration_reloc_ptrs, proc_data, next)

/* Define a notification list. */
typedef struct gs_notify_list_s {
    gs_memory_t *memory;	/* for allocating registrations */
    gs_notify_registration_t *first;
} gs_notify_list_t;
/* The descriptor is public for GC of embedded instances. */
extern_st(st_gs_notify_list);
#define public_st_gs_notify_list() /* in gsnotify.c */\
  gs_public_st_ptrs1(st_gs_notify_list, gs_notify_list_t,\
    "gs_notify_list_t", notify_list_enum_ptrs, notify_list_reloc_ptrs,\
    first)
#define st_gs_notify_list_max_ptrs 1

/* Initialize a notification list. */
void gs_notify_init(P2(gs_notify_list_t *nlist, gs_memory_t *mem));

/* Register a client. */
int gs_notify_register(P3(gs_notify_list_t *nlist, gs_notify_proc_t proc,
			  void *proc_data));

/*
 * Unregister a client.  Return 1 if the client was registered, 0 if not.
 * If proc_data is 0, unregister all registrations of that proc; otherwise,
 * unregister only the registration of that procedure with that proc_data.
 */
int gs_notify_unregister(P3(gs_notify_list_t *nlist, gs_notify_proc_t proc,
			    void *proc_data));

/* Unregister a client, calling a procedure for each unregistration. */
int gs_notify_unregister_calling(P4(gs_notify_list_t *nlist,
				    gs_notify_proc_t proc, void *proc_data,
				    void (*unreg_proc)(P1(void *pdata))));

/*
 * Notify the clients on a list.  If an error occurs, return the first
 * error code, but notify all clients regardless.
 */
int gs_notify_all(P2(gs_notify_list_t *nlist, void *event_data));

/* Release a notification list. */
void gs_notify_release(P1(gs_notify_list_t *nlist));

#endif /* gsnotify_INCLUDED */
