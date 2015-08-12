/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>

static void
portMapPrint(Fmt *fmt, PortMap *x)
{
	fmtprint(fmt, "[%ud %ud %ud %ud]", x->prog, x->vers, x->prot, x->port);
}
static uint
portMapSize(PortMap *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4 + 4 + 4;
	return a;
}
static int
portMapPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortMap *x)
{
	if(sunUint32Pack(a, ea, &a, &x->prog) < 0)
		goto Err;
	if(sunUint32Pack(a, ea, &a, &x->vers) < 0)
		goto Err;
	if(sunUint32Pack(a, ea, &a, &x->prot) < 0)
		goto Err;
	if(sunUint32Pack(a, ea, &a, &x->port) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portMapUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortMap *x)
{
	if(sunUint32Unpack(a, ea, &a, &x->prog) < 0)
		goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->vers) < 0)
		goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->prot) < 0)
		goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->port) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portTNullPrint(Fmt *fmt, PortTNull *x)
{
	USED(x);
	fmtprint(fmt, "%s", "PortTNull");
}
static uint
portTNullSize(PortTNull *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
static int
portTNullPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
static int
portTNullUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
static void
portRNullPrint(Fmt *fmt, PortRNull *x)
{
	USED(x);
	fmtprint(fmt, "%s", "PortRNull");
}
static uint
portRNullSize(PortRNull *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
static int
portRNullPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
static int
portRNullUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
static void
portTSetPrint(Fmt *fmt, PortTSet *x)
{
	fmtprint(fmt, "PortTSet ");
	portMapPrint(fmt, &x->map);
}
static uint
portTSetSize(PortTSet *x)
{
	uint a;
	USED(x);
	a = 0 + portMapSize(&x->map);
	return a;
}
static int
portTSetPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTSet *x)
{
	if(portMapPack(a, ea, &a, &x->map) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portTSetUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTSet *x)
{
	if(portMapUnpack(a, ea, &a, &x->map) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portRSetPrint(Fmt *fmt, PortRSet *x)
{
	fmtprint(fmt, "PortRSet %ud", x->b);
}
static uint
portRSetSize(PortRSet *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	return a;
}
static int
portRSetPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRSet *x)
{
	if(sunUint1Pack(a, ea, &a, &x->b) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portRSetUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRSet *x)
{
	if(sunUint1Unpack(a, ea, &a, &x->b) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portTUnsetPrint(Fmt *fmt, PortTUnset *x)
{
	fmtprint(fmt, "PortTUnset ");
	portMapPrint(fmt, &x->map);
}
static uint
portTUnsetSize(PortTUnset *x)
{
	uint a;
	USED(x);
	a = 0 + portMapSize(&x->map);
	return a;
}
static int
portTUnsetPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTUnset *x)
{
	if(portMapPack(a, ea, &a, &x->map) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portTUnsetUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTUnset *x)
{
	if(portMapUnpack(a, ea, &a, &x->map) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portRUnsetPrint(Fmt *fmt, PortRUnset *x)
{
	fmtprint(fmt, "PortRUnset %ud", x->b);
}
static uint
portRUnsetSize(PortRUnset *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	return a;
}
static int
portRUnsetPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRUnset *x)
{
	if(sunUint1Pack(a, ea, &a, &x->b) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portRUnsetUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRUnset *x)
{
	if(sunUint1Unpack(a, ea, &a, &x->b) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portTGetportPrint(Fmt *fmt, PortTGetport *x)
{
	fmtprint(fmt, "PortTGetport ");
	portMapPrint(fmt, &x->map);
}
static uint
portTGetportSize(PortTGetport *x)
{
	uint a;
	USED(x);
	a = 0 + portMapSize(&x->map);
	return a;
}
static int
portTGetportPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTGetport *x)
{
	if(portMapPack(a, ea, &a, &x->map) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portTGetportUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTGetport *x)
{
	if(portMapUnpack(a, ea, &a, &x->map) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portRGetportPrint(Fmt *fmt, PortRGetport *x)
{
	fmtprint(fmt, "PortRGetport %ud", x->port);
}
static uint
portRGetportSize(PortRGetport *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	return a;
}
static int
portRGetportPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRGetport *x)
{
	if(sunUint32Pack(a, ea, &a, &x->port) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portRGetportUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRGetport *x)
{
	if(sunUint32Unpack(a, ea, &a, &x->port) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portTDumpPrint(Fmt *fmt, PortTDump *x)
{
	USED(x);
	fmtprint(fmt, "PortTDump");
}
static uint
portTDumpSize(PortTDump *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
static int
portTDumpPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTDump *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
static int
portTDumpUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTDump *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
static void
portRDumpPrint(Fmt *fmt, PortRDump *x)
{
	int i;

	fmtprint(fmt, "PortRDump");
	for(i = 0; i < x->nmap; i++) {
		fmtprint(fmt, " ");
		portMapPrint(fmt, &x->map[i]);
	}
}
static uint
portRDumpSize(PortRDump *x)
{
	return (5 * 4 * x->nmap) + 4;
}
static int
portRDumpPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRDump *x)
{
	int i;
	uint32_t zero, one;

	zero = 0;
	one = 1;
	for(i = 0; i < x->nmap; i++) {
		if(sunUint32Pack(a, ea, &a, &one) < 0 || portMapPack(a, ea, &a, &x->map[i]) < 0)
			goto Err;
	}
	if(sunUint32Pack(a, ea, &a, &zero) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portRDumpUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRDump *x)
{
	int i;
	u1int u1;
	PortMap *m;

	m = (PortMap *)a;
	for(i = 0;; i++) {
		if(sunUint1Unpack(a, ea, &a, &u1) < 0)
			goto Err;
		if(u1 == 0)
			break;
		if(portMapUnpack(a, ea, &a, &m[i]) < 0)
			goto Err;
	}
	x->nmap = i;
	x->map = m;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}
static void
portTCallitPrint(Fmt *fmt, PortTCallit *x)
{
	fmtprint(fmt, "PortTCallit [%ud,%ud,%ud] %ud", x->prog, x->vers, x->proc, x->count);
}
static uint
portTCallitSize(PortTCallit *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4 + 4 + sunVarOpaqueSize(x->count);
	return a;
}
static int
portTCallitPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTCallit *x)
{
	if(sunUint32Pack(a, ea, &a, &x->prog) < 0)
		goto Err;
	if(sunUint32Pack(a, ea, &a, &x->vers) < 0)
		goto Err;
	if(sunUint32Pack(a, ea, &a, &x->proc) < 0)
		goto Err;
	if(sunVarOpaquePack(a, ea, &a, &x->data, &x->count, -1) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portTCallitUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortTCallit *x)
{
	if(sunUint32Unpack(a, ea, &a, &x->prog) < 0)
		goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->vers) < 0)
		goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->proc) < 0)
		goto Err;
	if(sunVarOpaqueUnpack(a, ea, &a, &x->data, &x->count, -1) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static void
portRCallitPrint(Fmt *fmt, PortRCallit *x)
{
	fmtprint(fmt, "PortRCallit %ud %ud", x->port, x->count);
}
static uint
portRCallitSize(PortRCallit *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + sunVarOpaqueSize(x->count);
	return a;
}
static int
portRCallitPack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRCallit *x)
{
	if(sunUint32Pack(a, ea, &a, &x->port) < 0)
		goto Err;
	if(sunVarOpaquePack(a, ea, &a, &x->data, &x->count, -1) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
portRCallitUnpack(uint8_t *a, uint8_t *ea, uint8_t **pa, PortRCallit *x)
{
	if(sunUint32Unpack(a, ea, &a, &x->port) < 0)
		goto Err;
	if(sunVarOpaqueUnpack(a, ea, &a, &x->data, &x->count, -1) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}

typedef int (*P)(uint8_t *, uint8_t *, uint8_t **, SunCall *);
typedef void (*F)(Fmt *, SunCall *);
typedef uint (*S)(SunCall *);

static SunProc proc[] = {
    (P)portTNullPack, (P)portTNullUnpack, (S)portTNullSize, (F)portTNullPrint, sizeof(PortTNull),
    (P)portRNullPack, (P)portRNullUnpack, (S)portRNullSize, (F)portRNullPrint, sizeof(PortRNull),
    (P)portTSetPack, (P)portTSetUnpack, (S)portTSetSize, (F)portTSetPrint, sizeof(PortTSet),
    (P)portRSetPack, (P)portRSetUnpack, (S)portRSetSize, (F)portRSetPrint, sizeof(PortRSet),
    (P)portTUnsetPack, (P)portTUnsetUnpack, (S)portTUnsetSize, (F)portTUnsetPrint, sizeof(PortTUnset),
    (P)portRUnsetPack, (P)portRUnsetUnpack, (S)portRUnsetSize, (F)portRUnsetPrint, sizeof(PortRUnset),
    (P)portTGetportPack, (P)portTGetportUnpack, (S)portTGetportSize, (F)portTGetportPrint, sizeof(PortTGetport),
    (P)portRGetportPack, (P)portRGetportUnpack, (S)portRGetportSize, (F)portRGetportPrint, sizeof(PortRGetport),
    (P)portTDumpPack, (P)portTDumpUnpack, (S)portTDumpSize, (F)portTDumpPrint, sizeof(PortTDump),
    (P)portRDumpPack, (P)portRDumpUnpack, (S)portRDumpSize, (F)portRDumpPrint, sizeof(PortRDump),
    (P)portTCallitPack, (P)portTCallitUnpack, (S)portTCallitSize, (F)portTCallitPrint, sizeof(PortTCallit),
    (P)portRCallitPack, (P)portRCallitUnpack, (S)portRCallitSize, (F)portRCallitPrint, sizeof(PortRCallit),
};

SunProg portProg =
    {
     PortProgram,
     PortVersion,
     proc,
     nelem(proc),
};
