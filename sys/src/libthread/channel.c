#include <u.h>
#include <libc.h>
#include "assert.h"
#include "threadimpl.h"

static Lock chanlock;		// Central channel access lock

void
chanfree(Channel *c) {

	lock(&chanlock);
	if (c->qused == 0) {
		free(c);
	} else {
		c->freed = 1;
	}
	unlock(&chanlock);
}

int
chaninit(Channel *c, int elemsize, int elemcnt) {
	if(elemcnt < 0 || elemsize <= 0 || c == nil)
		return -1;
	c->f = 0;
	c->n = 0;
	c->freed = 0;
	c->qused = 0;
	c->s = elemcnt;
	c->e = elemsize;
	_threaddebug(DBGCHAN, "chaninit %lux", c);
	return 1;
}

Channel *
chancreate(int elemsize, int elemcnt) {
	Channel *c;

	if(elemcnt < 0 || elemsize <= 0)
		return nil;
	c = _threadmalloc(sizeof(Channel) + elemcnt * elemsize);
	if(c == nil)
		return c;
	c->f = 0;
	c->n = 0;
	c->freed = 0;
	c->qused = 0;
	c->s = elemcnt;
	c->e = elemsize;
	_threaddebug(DBGCHAN, "chancreate %lux", c);
	return c;
}

int
alt(Alt *alts) {
	Alt *a, *xa;
	Channel *c;
	uchar *v;
	int i, n, entry;
	Proc *p;
	Thread *t;

	lock(&chanlock);
	
	(*procp)->curthread->alt = alts;
	(*procp)->curthread->call = Callalt;
repeat:

	// Test which channels can proceed
	n = 1;
	a = nil;
	entry = -1;
	for (xa = alts; xa->op; xa++) {
		if (xa->op == CHANNOP) continue;
		if (xa->op == CHANNOBLK) {
			if (a == nil) {
				(*procp)->curthread->call = Callnil;
				unlock(&chanlock);
				return xa - alts;
			} else
				break;
		}

		c = xa->c;
		if ((xa->op == CHANSND && c->n < c->s) ||
			(xa->op == CHANRCV && c->n)) {
				// There's room to send in the channel
				if (nrand(n) == 0) {
					a = xa;
					entry = -1;
				}
				n++;
		} else {
			// Test for blocked senders or receivers
			for (i = 0; i < 32; i++) {
				// Is it claimed?
				if (
					(c->qused & (1 << i))
					&& xa->op == (CHANSND+CHANRCV) - c->qentry[i]->op
							// complementary op
					&& *c->qentry[i]->tag == nil
				) {
					// No
					if (nrand(n) == 0) {
						a = xa;
						entry = i;
					}
					n++;
					break;
				}
			}
		}
	}

	if (a == nil) {
		// Nothing can proceed, enqueue on all channels
		c = nil;
		for (a = alts; a->op; a++) {
			Channel *cp;

			if (a->op == CHANNOP || a->op == CHANNOBLK) continue;
			cp = a->c;
			a->tag = &c;
			for (i = 0; i < 32; i++) {
				if ((cp->qused & (1 << i)) == 0) {
					cp->qentry[i] = a;
					cp->qused |= a->q = 1 << i;
					break;
				}
			}
			threadassert(i != 32);
		}

		// And wait for the rendez vous
		unlock(&chanlock);
		if (_threadrendezvous((ulong)&c, 0) == ~0) {
			(*procp)->curthread->call = Callnil;
			return -1;
		}
		
		lock(&chanlock);

		/* We rendezed-vous on channel c, dequeue from all channels
		 * and find the Alt struct to which c belongs
		 */
		a = nil;
		for (xa = alts; xa->op; xa++) {
			Channel *xc;

			if (xa->op == CHANNOP || xa->op == CHANNOBLK) continue;
			xc = xa->c;
			xc->qused &= ~xa->q;
			if (xc == c)
				a = xa;
			
		}

		if (c->s) {
			// Buffered channel, try again
			sleep(0);
			goto repeat;
		}

		unlock(&chanlock);

		if (c->freed) chanfree(c);

		p = *procp;
		t = p->curthread;
		if (t->exiting)
			threadexits(nil);
		(*procp)->curthread->call = Callnil;
		return a - alts;
	}

	c = a->c;
	// Channel c can proceed

	if (c->s) {
		// Send or receive via the buffered channel
		if (a->op == CHANSND) {
			v = c->v + ((c->f + c->n) % c->s) * c->e;
			if (a->v)
				memmove(v, a->v, c->e);
			else
				memset(v, 0, c->e);
			c->n++;
		} else {
			if (a->v) {
				v = c->v + (c->f % c->s) * c->e;
				memmove(a->v, v, c->e);
			}
			c->n--;
			c->f++;
		}
	}
	if (entry < 0)
		for (i = 0; i < 32; i++) {
			if (
				(c->qused & (1 << i))
				&& c->qentry[i]->op == (CHANSND+CHANRCV) - a->op
				&& *c->qentry[i]->tag == nil
			) {
				// Unblock peer process
				xa = c->qentry[i];
				*xa->tag = c;

				unlock(&chanlock);
				if (_threadrendezvous((ulong)xa->tag, 0) == ~0) {
					(*procp)->curthread->call = Callnil;
					return -1;
				}
				(*procp)->curthread->call = Callnil;
				return a - alts;
			}
		}
	if (entry >= 0) {
		xa = c->qentry[entry];
		if (a->op == CHANSND) {
			if (xa->v) {
				if (a->v)
					memmove(xa->v, a->v, c->e);
				else
					memset(xa->v, 0, c->e);
			}
		} else {
			if (a->v) {
				if (xa->v)
					memmove(a->v, xa->v, c->e);
				else
					memset(a->v, 0, c->e);
			}
		}
		*xa->tag = c;

		unlock(&chanlock);
		if (_threadrendezvous((ulong)xa->tag, 0) == ~0) {
			(*procp)->curthread->call = Callnil;
			return -1;
		}
		(*procp)->curthread->call = Callnil;
		return a - alts;
	}
	unlock(&chanlock);
	yield();
	(*procp)->curthread->call = Callnil;
	return a - alts;
}

int
nbrecv(Channel *c, void *v) {
	Alt *a;
	int i;
	
	lock(&chanlock);
	if (c->qused) // There's somebody waiting
		for (i = 0; i < 32; i++) {
			a = c->qentry[i];
			if (
				(c->qused & (1 << i))
				&& a->op == CHANSND
				&& *a->tag == nil
			) {
				*a->tag = c;
				if (c->n) {
					// There's an item to receive in the buffered channel
					if (v)
						memmove(v, c->v + (c->f % c->s) * c->e, c->e);
					c->n--;
					c->f++;
				} else {
					if (v) {
						if (a->v)
							memmove(v, a->v, c->e);
						else
							memset(v, 0, c->e);
					}
				}

				unlock(&chanlock);
				if (_threadrendezvous((ulong)a->tag, 0) == ~0)
					return -1;
				return 1;
			}
		}
	if (c->n) {
		// There's an item to receive in the buffered channel
		if (v)
			memmove(v, c->v + (c->f % c->s) * c->e, c->e);
		c->n--;
		c->f++;
		unlock(&chanlock);
		yield();
		return 1;
	}
	unlock(&chanlock);
	return 0;
}

int
recv(Channel *c, void *v) {
	Alt a, *xa;
	Channel *tag;
	int i;
	Proc *p;
	Thread *t;

	
	lock(&chanlock);
retry:
	if (c->qused) // There's somebody waiting
		for (i = 0; i < 32; i++) {
			xa = c->qentry[i];
			if (
				(c->qused & (1 << i))
				&& xa->op == CHANSND
				&& *xa->tag == nil
			) {
				*xa->tag = c;
				if (c->n) {
					// There's an item to receive in the buffered channel
					if (v)
						memmove(v, c->v + (c->f % c->s) * c->e, c->e);
					c->n--;
					c->f++;
				} else {
					if (v) {
						if (xa->v)
							memmove(v, xa->v, c->e);
						else
							memset(v, 0, c->e);
					}
				}

				unlock(&chanlock);
				if (_threadrendezvous((ulong)xa->tag, 0) == ~0)
					return -1;
				return 1;
			}
	}
	if (c->n) {
		// There's an item to receive in the buffered channel
		if (v)
			memmove(v, c->v + (c->f % c->s) * c->e, c->e);
		c->n--;
		c->f++;
		unlock(&chanlock);
		yield();
		return 1;
	}
	// Unbuffered, or buffered but empty
	tag = nil;
	a.c = c;
	a.v = v;
	a.tag = &tag;
	a.op = CHANRCV;
	p = *procp;
	t = p->curthread;
	t->alt = &a;
	t->call = Callrcv;

	// enqueue on the channel
	for (i = 0; i < 32; i++)
		if ((c->qused & (1 << i)) == 0) {
			c->qentry[i] = &a;
			c->qused |= a.q = 1 << i;
			break;
		}

	unlock(&chanlock);
	if (_threadrendezvous((ulong)&tag, 0) == ~0) {
		t->call = Callnil;
		return -1;
	}
	lock(&chanlock);

	// dequeue from the channel
	c->qused &= ~a.q;
	if (c->s) goto retry;	// Buffered channels: try the queue again
	unlock(&chanlock);
	if (c->freed) chanfree(c);
	t->call = Callnil;
	if (t->exiting)
		threadexits(nil);
	return 1;
}

int
nbsend(Channel *c, void *v) {
	Alt *a;
	int i;

	lock(&chanlock);
	if (c->qused)	// Anybody waiting?
		for (i = 0; i < 32; i++) {
			a = c->qentry[i];
			if (
				(c->qused & (1 << i))
				&& a->op == CHANRCV
				&& *a->tag == nil
			) {
				*a->tag = c;
				if (c->n < c->s) {
					// There's room to send in the buffered channel
					if (v)
						memmove(c->v + ((c->f + c->n) % c->s) * c->e, v, c->e);
					else
						memset(c->v + ((c->f + c->n) % c->s) * c->e, 0, c->e);
					c->n++;
				} else {
					if (a->v) {
						if (v)
							memmove(a->v, v, c->e);
						else
							memset(a->v, 0, c->e);
					}
				}

				unlock(&chanlock);
				if (_threadrendezvous((ulong)a->tag, 0) == ~0)
					return -1;
				return 1;
			}
	}
	if (c->n < c->s) {
		// There's room to send in the buffered channel
		if (v)
			memmove(c->v + ((c->f + c->n) % c->s) * c->e, v, c->e);
		else
			memset(c->v + ((c->f + c->n) % c->s) * c->e, 0, c->e);
		c->n++;
		unlock(&chanlock);
		yield();
		return 1;
	}
	unlock(&chanlock);
	return 0;
}

int
send(Channel *c, void *v) {
	Alt a, *xa;
	Channel *tag;
	int i;
	Proc *p;
	Thread *t;

	lock(&chanlock);
retry:
	if (c->qused)	// Anybody waiting?
		for (i = 0; i < 32; i++) {
			xa = c->qentry[i];
			threadassert(!(c->qused & (1 << i)) || xa != nil);
			if (
				(c->qused & (1 << i))
				&& xa->op == CHANRCV
				&& *xa->tag == nil
			) {
				*xa->tag = c;
				if (c->n < c->s) {
					// There's room to send in the buffered channel
					if (v)
						memmove(c->v + ((c->f + c->n) % c->s) * c->e, v, c->e);
					else
						memset(c->v + ((c->f + c->n) % c->s) * c->e, 0, c->e);
					c->n++;
				} else {
					if (xa->v) {
						if (v)
							memmove(xa->v, v, c->e);
						else
							memset(xa->v, 0, c->e);
					}
				}

				unlock(&chanlock);
				if (_threadrendezvous((ulong)xa->tag, 0) == ~0)
					return -1;
				return 1;
			}
	}
	if (c->n < c->s) {
		// There's room to send in the buffered channel
		if (v)
			memmove(c->v + ((c->f + c->n) % c->s) * c->e, v, c->e);
		else
			memset(c->v + ((c->f + c->n) % c->s) * c->e, 0, c->e);
		c->n++;
		unlock(&chanlock);
		yield();
		return 1;
	}
	tag = nil;
	a.c = c;
	a.v = v;
	a.tag = &tag;
	a.op = CHANSND;
	p = *procp;
	t = p->curthread;
	t->alt = &a;
	t->call = Callsnd;

	// enqueue on the channel
	for (i = 0; i < 32; i++)
		if ((c->qused & (1 << i)) == 0) {
			c->qentry[i] = &a;
			c->qused |= a.q = 1 << i;
			break;
		}
	unlock(&chanlock);
	if (_threadrendezvous((ulong)&tag, 0) == ~0) {
		t->call = Callnil;
		return -1;
	}
	// Unbuffered channels: data was transferred; dequeue
	lock(&chanlock);
	// dequeue from the channel
	c->qused &= ~a.q;
	if (c->s) goto retry;	// Buffered channels: try the queue again
	unlock(&chanlock);
	if (c->freed) chanfree(c);
	t->call = Callnil;
	if (t->exiting)
		threadexits(nil);
	return 1;
}

int
sendul(Channel *c, ulong v) {
	threadassert(c->e == sizeof(ulong));
	return send(c, &v);
}

ulong
recvul(Channel *c) {
	ulong v;

	threadassert(c->e == sizeof(ulong));
	recv(c, &v);
	return v;
}

int
sendp(Channel *c, void *v) {
	threadassert(c->e == sizeof(void *));
	return send(c, &v);
}

void *
recvp(Channel *c) {
	void * v;

	threadassert(c->e == sizeof(void *));
	recv(c, &v);
	return v;
}

int
nbsendul(Channel *c, ulong v) {
	threadassert(c->e == sizeof(ulong));
	return nbsend(c, &v);
}

ulong
nbrecvul(Channel *c) {
	ulong v;

	threadassert(c->e == sizeof(ulong));
	if (nbrecv(c, &v) == 0)
		return 0;
	return v;
}

int
nbsendp(Channel *c, void *v) {
	threadassert(c->e == sizeof(void *));
	return nbsend(c, &v);
}

void *
nbrecvp(Channel *c) {
	void * v;

	threadassert(c->e == sizeof(void *));
	if (nbrecv(c, &v) == 0)
		return nil;
	return v;
}
