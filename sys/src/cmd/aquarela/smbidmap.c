/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

#define INITIALCHUNKSIZE 10

typedef struct Entry {
	void *p;
	int32_t freechain;
} Entry;

struct SmbIdMap {
	Entry *array;
	uint32_t entries;
	int32_t freeindex;
};

SmbIdMap *
smbidmapnew(void)
{
	SmbIdMap *m;
	m = smbemallocz(sizeof(SmbIdMap), 1);
	m->freeindex = -1;
	return m;
}

void
smbidmapremovebyid(SmbIdMap *m, int32_t id)
{
	if (m == nil)
		return;
	assert(id > 0);
	id--;
	assert(id >= 0 && id < m->entries);
	assert(m->array[id].freechain == -2);
	m->array[id].freechain = m->freeindex;
	m->freeindex = id;
}

void
smbidmapremove(SmbIdMap *m, void *thing)
{
	int32_t id;
	if (m == nil)
		return;
	id = *(int32_t *)thing;
	smbidmapremovebyid(m, id);
}

void
smbidmapremoveif(SmbIdMap *m, int (*f)(void *p, void *arg), void *arg)
{
	int i;
	if (m == nil)
		return;
	for (i = 0; i < m->entries; i++)
		if (m->array[i].freechain == -2 && (*f)(m->array[i].p, arg))
			smbidmapremovebyid(m, i + 1);
}

static void
grow(SmbIdMap *m)
{
	int32_t x;
	int32_t oldentries = m->entries;
	if (m->entries == 0)
		m->entries = INITIALCHUNKSIZE;
	else
		m->entries *= 2;
	smberealloc((void *)&m->array, sizeof(Entry) * m->entries);
	for (x = m->entries - 1; x >= oldentries; x--) {
		m->array[x].freechain = m->freeindex;
		m->freeindex = x;
	}
}

int32_t
smbidmapadd(SmbIdMap *m, void *p)
{
	int32_t i;
	if (m->freeindex < 0)
		grow(m);
	i = m->freeindex;
	m->freeindex = m->array[i].freechain;
	m->array[i].freechain = -2;
	m->array[i].p = p;
	*(int32_t *)p = i + 1;
	return i + 1;
}

void *
smbidmapfind(SmbIdMap *m, int32_t id)
{
	if (m == nil)
		return nil;
	if (id <= 0)
		return nil;
	id--;
	if (id < 0 || id > m->entries || m->array[id].freechain != -2)
		return nil;
	return m->array[id].p;
}

void
smbidmapfree(SmbIdMap **mp, SMBIDMAPAPPLYFN *freefn, void *magic)
{
	SmbIdMap *m = *mp;
	if (m) {
		int32_t i;
		if (freefn) {
			for (i = 0; i < m->entries; i++)
				if (m->array[i].freechain == -2)
					(*freefn)(magic, m->array[i].p);
		}
		free(m->array);
		free(m);
		*mp = nil;
	}
}

void
smbidmapapply(SmbIdMap *m, SMBIDMAPAPPLYFN *applyfn, void *magic)
{
	if (m) {
		int32_t i;
		for (i = 0; i < m->entries; i++)
			if (m->array[i].freechain == -2)
				(*applyfn)(magic, m->array[i].p);
	}
}
