/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Queue Queue;
#pragma incomplete Queue
Queue *_vtqalloc(void);
int _vtqsend(Queue*, void*);
void *_vtqrecv(Queue*);
void _vtqhangup(Queue*);
void *_vtnbqrecv(Queue*);
void _vtqdecref(Queue*);
Queue *_vtqincref(Queue*);
