/*
 * SUN NFSv3 Mounter.  See RFC 1813
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>
#include <nfs3.h>

void
nfsMount3TNullPrint(Fmt *fmt, NfsMount3TNull *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3TNull");
}
uint
nfsMount3TNullSize(NfsMount3TNull *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3TNullPack(uchar *a, uchar *ea, uchar **pa, NfsMount3TNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3TNullUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3TNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3RNullPrint(Fmt *fmt, NfsMount3RNull *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3RNull");
}
uint
nfsMount3RNullSize(NfsMount3RNull *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3RNullPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3RNullUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3TMntPrint(Fmt *fmt, NfsMount3TMnt *x)
{
	fmtprint(fmt, "%s\n", "NfsMount3TMnt");
	fmtprint(fmt, "\t%s=", "path");
	fmtprint(fmt, "\"%s\"", x->path);
	fmtprint(fmt, "\n");
}
uint
nfsMount3TMntSize(NfsMount3TMnt *x)
{
	uint a;
	USED(x);
	a = 0 + sunStringSize(x->path);
	return a;
}
int
nfsMount3TMntPack(uchar *a, uchar *ea, uchar **pa, NfsMount3TMnt *x)
{
	if(sunStringPack(a, ea, &a, &x->path, 1024) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount3TMntUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3TMnt *x)
{
	if(sunStringUnpack(a, ea, &a, &x->path, 1024) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfsMount3RMntPrint(Fmt *fmt, NfsMount3RMnt *x)
{
	fmtprint(fmt, "%s\n", "NfsMount3RMnt");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%ud", x->status);
	fmtprint(fmt, "\n");
	switch(x->status){
	case 0:
		fmtprint(fmt, "\t%s=", "handle");
		fmtprint(fmt, "%.*H", x->len, x->handle);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfsMount3RMntSize(NfsMount3RMnt *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case 0:
		a = a + sunVarOpaqueSize(x->len);
		a = a + 4 + 4 * x->nauth;
		break;
	}
	a = a;
	return a;
}
uint
nfsMount1RMntSize(NfsMount3RMnt *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case 0:
		a = a + NfsMount1HandleSize;
		break;
	}
	return a;
}

int
nfsMount3RMntPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RMnt *x)
{
	int i;
	if(sunUint32Pack(a, ea, &a, &x->status) < 0) goto Err;
	switch(x->status){
	case 0:
		if(sunVarOpaquePack(a, ea, &a, &x->handle, &x->len, NfsMount3MaxHandleSize) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->nauth) < 0) goto Err;
		for(i=0; i<x->nauth; i++)
			if(sunUint32Pack(a, ea, &a, &x->auth[i]) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount1RMntPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RMnt *x)
{
	if(sunUint32Pack(a, ea, &a, &x->status) < 0) goto Err;
	switch(x->status){
	case 0:
		if(x->len != NfsMount1HandleSize)
			goto Err;
		if(sunFixedOpaquePack(a, ea, &a, x->handle, NfsMount1HandleSize) < 0) goto Err;
		if(x->nauth != 0)
			goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount1RMntUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RMnt *x)
{
	if(sunUint32Unpack(a, ea, &a, &x->status) < 0) goto Err;
	switch(x->status){
	case 0:
		x->len = NfsMount1HandleSize;
		x->nauth = 0;
		x->auth = 0;
		if(sunFixedOpaqueUnpack(a, ea, &a, x->handle, NfsMount1HandleSize) < 0) goto Err;
		if(x->nauth != 0)
			goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}

int
nfsMount3RMntUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RMnt *x)
{
	int i;

	if(sunUint32Unpack(a, ea, &a, &x->status) < 0) goto Err;
	switch(x->status){
	case 0:
		if(sunVarOpaqueUnpack(a, ea, &a, &x->handle, &x->len, NfsMount3MaxHandleSize) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->nauth) < 0) goto Err;
		x->auth = (u32int*)a;
		for(i=0; i<x->nauth; i++)
			if(sunUint32Unpack(a, ea, &a, &x->auth[i]) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfsMount3TDumpPrint(Fmt *fmt, NfsMount3TDump *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3TDump");
}
uint
nfsMount3TDumpSize(NfsMount3TDump *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3TDumpPack(uchar *a, uchar *ea, uchar **pa, NfsMount3TDump *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3TDumpUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3TDump *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3EntryPrint(Fmt *fmt, NfsMount3Entry *x)
{
	fmtprint(fmt, "%s\n", "NfsMount3Entry");
	fmtprint(fmt, "\t%s=", "host");
	fmtprint(fmt, "\"%s\"", x->host);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "path");
	fmtprint(fmt, "\"%s\"", x->path);
	fmtprint(fmt, "\n");
}
uint
nfsMount3EntrySize(NfsMount3Entry *x)
{
	uint a;
	USED(x);
	a = 0 + sunStringSize(x->host) + sunStringSize(x->path);
	return a;
}
int
nfsMount3EntryPack(uchar *a, uchar *ea, uchar **pa, NfsMount3Entry *x)
{
	u1int one;

	one = 1;
	if(sunUint1Pack(a, ea, &a, &one) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->host, 255) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->path, 1024) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount3EntryUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3Entry *x)
{
	u1int one;

	if(sunUint1Unpack(a, ea, &a, &one) < 0 || one != 1) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->host, NfsMount3MaxNameSize) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->path, NfsMount3MaxPathSize) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfsMount3RDumpPrint(Fmt *fmt, NfsMount3RDump *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3RDump");
}
uint
nfsMount3RDumpSize(NfsMount3RDump *x)
{
	uint a;
	USED(x);
	a = 0;
	a += x->count;
	a += 4;
	return a;
}
int
nfsMount3RDumpPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RDump *x)
{
	u1int zero;

	zero = 0;
	if(a+x->count > ea) goto Err;
	memmove(a, x->data, x->count);
	a += x->count;
	if(sunUint1Pack(a, ea, &a, &zero) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount3RDumpUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RDump *x)
{
	int i;
	uchar *oa;
	u1int u1;
	u32int u32;

	oa = a;
	for(i=0;; i++){
		if(sunUint1Unpack(a, ea, &a, &u1) < 0)
			goto Err;
		if(u1 == 0)
			break;
		if(sunUint32Unpack(a, ea, &a, &u32) < 0
		|| u32 > NfsMount3MaxNameSize
		|| (a+=u32) >= ea
		|| sunUint32Unpack(a, ea, &a, &u32) < 0
		|| u32 > NfsMount3MaxPathSize
		|| (a+=u32) >= ea)
			goto Err;
	}
	x->count = (a-4) - oa;
	x->data = oa;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfsMount3TUmntPrint(Fmt *fmt, NfsMount3TUmnt *x)
{
	fmtprint(fmt, "%s\n", "NfsMount3TUmnt");
	fmtprint(fmt, "\t%s=", "path");
	fmtprint(fmt, "\"%s\"", x->path);
	fmtprint(fmt, "\n");
}
uint
nfsMount3TUmntSize(NfsMount3TUmnt *x)
{
	uint a;
	USED(x);
	a = 0 + sunStringSize(x->path);
	return a;
}
int
nfsMount3TUmntPack(uchar *a, uchar *ea, uchar **pa, NfsMount3TUmnt *x)
{
	if(sunStringPack(a, ea, &a, &x->path, 1024) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount3TUmntUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3TUmnt *x)
{
	if(sunStringUnpack(a, ea, &a, &x->path, 1024) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfsMount3RUmntPrint(Fmt *fmt, NfsMount3RUmnt *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3RUmnt");
}
uint
nfsMount3RUmntSize(NfsMount3RUmnt *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3RUmntPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RUmnt *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3RUmntUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RUmnt *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3TUmntallPrint(Fmt *fmt, NfsMount3TUmntall *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3TUmntall");
}
uint
nfsMount3TUmntallSize(NfsMount3TUmntall *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3TUmntallPack(uchar *a, uchar *ea, uchar **pa, NfsMount3TUmntall *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3TUmntallUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3TUmntall *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3RUmntallPrint(Fmt *fmt, NfsMount3RUmntall *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3RUmntall");
}
uint
nfsMount3RUmntallSize(NfsMount3RUmntall *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3RUmntallPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RUmntall *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3RUmntallUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RUmntall *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3TExportPrint(Fmt *fmt, NfsMount3TExport *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3TExport");
}
uint
nfsMount3TExportSize(NfsMount3TExport *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfsMount3TExportPack(uchar *a, uchar *ea, uchar **pa, NfsMount3TExport *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfsMount3TExportUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3TExport *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfsMount3RExportPrint(Fmt *fmt, NfsMount3RExport *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "NfsMount3RExport");
	fmtprint(fmt, "\n");
}
uint
nfsMount3RExportSize(NfsMount3RExport *x)
{
	uint a;
	USED(x);
	a = 0;
	a += x->count;
	a += 4;	/* end of export list */
	return a;
}
int
nfsMount3RExportPack(uchar *a, uchar *ea, uchar **pa, NfsMount3RExport *x)
{
	u1int zero;

	zero = 0;
	if(a+x->count > ea) goto Err;
	memmove(a, x->data, x->count);
	a += x->count;
	if(sunUint1Pack(a, ea, &a, &zero) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfsMount3RExportUnpack(uchar *a, uchar *ea, uchar **pa, NfsMount3RExport *x)
{
	int ng, ne;
	uchar *oa;
	u1int u1;
	u32int u32;

	oa = a;
	ng = 0;
	for(ne=0;; ne++){
		if(sunUint1Unpack(a, ea, &a, &u1) < 0)
			goto Err;
		if(u1 == 0)
			break;
		if(sunUint32Unpack(a, ea, &a, &u32) < 0
		|| (a += (u32+3)&~3) >= ea)
			goto Err;
		for(;; ng++){
			if(sunUint1Unpack(a, ea, &a, &u1) < 0)
				goto Err;
			if(u1 == 0)
				break;
			if(sunUint32Unpack(a, ea, &a, &u32) < 0
			|| (a += (u32+3)&~3) >= ea)
				goto Err;
		}
	}
	x->data = oa;
	x->count = (a-4) - oa;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
uint
nfsMount3ExportGroupSize(uchar *a)
{
	int ng;
	u1int have;
	u32int n;

	a += 4;
	sunUint32Unpack(a, a+4, &a, &n);
	a += (n+3)&~3;
	ng = 0;
	for(;;){
		sunUint1Unpack(a, a+4, &a, &have);
		if(have == 0)
			break;
		ng++;
		sunUint32Unpack(a, a+4, &a, &n);
		a += (n+3)&~3;
	}
	return ng;
}
int
nfsMount3ExportUnpack(uchar *a, uchar *ea, uchar **pa, char **gp, char ***pgp, NfsMount3Export *x)
{
	int ng;
	u1int u1;

	if(sunUint1Unpack(a, ea, &a, &u1) < 0 || u1 != 1) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->path, NfsMount3MaxPathSize) < 0) goto Err;
	x->g = gp;
	ng = 0;
	for(;;){
		if(sunUint1Unpack(a, ea, &a, &u1) < 0) goto Err;
		if(u1 == 0)
			break;
		if(sunStringUnpack(a, ea, &a, &gp[ng++], NfsMount3MaxNameSize) < 0) goto Err;
	}
	x->ng = ng;
	*pgp = gp+ng;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}
uint
nfsMount3ExportSize(NfsMount3Export *x)
{
	int i;
	uint a;

	a = 4 + sunStringSize(x->path);
	for(i=0; i<x->ng; i++)
		a += 4 + sunStringSize(x->g[i]);
	a += 4;
	return a;
}
int
nfsMount3ExportPack(uchar *a, uchar *ea, uchar **pa, NfsMount3Export *x)
{
	int i;
	u1int u1;

	u1 = 1;
	if(sunUint1Pack(a, ea, &a, &u1) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->path, NfsMount3MaxPathSize) < 0) goto Err;
	for(i=0; i<x->ng; i++){
		if(sunUint1Pack(a, ea, &a, &u1) < 0) goto Err;
		if(sunStringPack(a, ea, &a, &x->g[i], NfsMount3MaxNameSize) < 0) goto Err;
	}
	u1 = 0;
	if(sunUint1Pack(a, ea, &a, &u1) < 0) goto Err;
	*pa = a;
	return 0;

Err:
	*pa = ea;
	return -1;
}

typedef int (*P)(uchar*, uchar*, uchar**, SunCall*);
typedef void (*F)(Fmt*, SunCall*);
typedef uint (*S)(SunCall*);

static SunProc proc3[] = {
	(P)nfsMount3TNullPack, (P)nfsMount3TNullUnpack, (S)nfsMount3TNullSize, (F)nfsMount3TNullPrint, sizeof(NfsMount3TNull),
	(P)nfsMount3RNullPack, (P)nfsMount3RNullUnpack, (S)nfsMount3RNullSize, (F)nfsMount3RNullPrint, sizeof(NfsMount3RNull),
	(P)nfsMount3TMntPack, (P)nfsMount3TMntUnpack, (S)nfsMount3TMntSize, (F)nfsMount3TMntPrint, sizeof(NfsMount3TMnt),
	(P)nfsMount3RMntPack, (P)nfsMount3RMntUnpack, (S)nfsMount3RMntSize, (F)nfsMount3RMntPrint, sizeof(NfsMount3RMnt),
	(P)nfsMount3TDumpPack, (P)nfsMount3TDumpUnpack, (S)nfsMount3TDumpSize, (F)nfsMount3TDumpPrint, sizeof(NfsMount3TDump),
	(P)nfsMount3RDumpPack, (P)nfsMount3RDumpUnpack, (S)nfsMount3RDumpSize, (F)nfsMount3RDumpPrint, sizeof(NfsMount3RDump),
	(P)nfsMount3TUmntPack, (P)nfsMount3TUmntUnpack, (S)nfsMount3TUmntSize, (F)nfsMount3TUmntPrint, sizeof(NfsMount3TUmnt),
	(P)nfsMount3RUmntPack, (P)nfsMount3RUmntUnpack, (S)nfsMount3RUmntSize, (F)nfsMount3RUmntPrint, sizeof(NfsMount3RUmnt),
	(P)nfsMount3TUmntallPack, (P)nfsMount3TUmntallUnpack, (S)nfsMount3TUmntallSize, (F)nfsMount3TUmntallPrint, sizeof(NfsMount3TUmntall),
	(P)nfsMount3RUmntallPack, (P)nfsMount3RUmntallUnpack, (S)nfsMount3RUmntallSize, (F)nfsMount3RUmntallPrint, sizeof(NfsMount3RUmntall),
	(P)nfsMount3TExportPack, (P)nfsMount3TExportUnpack, (S)nfsMount3TExportSize, (F)nfsMount3TExportPrint, sizeof(NfsMount3TExport),
	(P)nfsMount3RExportPack, (P)nfsMount3RExportUnpack, (S)nfsMount3RExportSize, (F)nfsMount3RExportPrint, sizeof(NfsMount3RExport),
};

static SunProc proc1[] = {
	(P)nfsMount3TNullPack, (P)nfsMount3TNullUnpack, (S)nfsMount3TNullSize, (F)nfsMount3TNullPrint, sizeof(NfsMount3TNull),
	(P)nfsMount3RNullPack, (P)nfsMount3RNullUnpack, (S)nfsMount3RNullSize, (F)nfsMount3RNullPrint, sizeof(NfsMount3RNull),
	(P)nfsMount3TMntPack, (P)nfsMount3TMntUnpack, (S)nfsMount3TMntSize, (F)nfsMount3TMntPrint, sizeof(NfsMount3TMnt),
	(P)nfsMount1RMntPack, (P)nfsMount1RMntUnpack, (S)nfsMount1RMntSize, (F)nfsMount3RMntPrint, sizeof(NfsMount3RMnt),
	(P)nfsMount3TDumpPack, (P)nfsMount3TDumpUnpack, (S)nfsMount3TDumpSize, (F)nfsMount3TDumpPrint, sizeof(NfsMount3TDump),
	(P)nfsMount3RDumpPack, (P)nfsMount3RDumpUnpack, (S)nfsMount3RDumpSize, (F)nfsMount3RDumpPrint, sizeof(NfsMount3RDump),
	(P)nfsMount3TUmntPack, (P)nfsMount3TUmntUnpack, (S)nfsMount3TUmntSize, (F)nfsMount3TUmntPrint, sizeof(NfsMount3TUmnt),
	(P)nfsMount3RUmntPack, (P)nfsMount3RUmntUnpack, (S)nfsMount3RUmntSize, (F)nfsMount3RUmntPrint, sizeof(NfsMount3RUmnt),
	(P)nfsMount3TUmntallPack, (P)nfsMount3TUmntallUnpack, (S)nfsMount3TUmntallSize, (F)nfsMount3TUmntallPrint, sizeof(NfsMount3TUmntall),
	(P)nfsMount3RUmntallPack, (P)nfsMount3RUmntallUnpack, (S)nfsMount3RUmntallSize, (F)nfsMount3RUmntallPrint, sizeof(NfsMount3RUmntall),
	(P)nfsMount3TExportPack, (P)nfsMount3TExportUnpack, (S)nfsMount3TExportSize, (F)nfsMount3TExportPrint, sizeof(NfsMount3TExport),
	(P)nfsMount3RExportPack, (P)nfsMount3RExportUnpack, (S)nfsMount3RExportSize, (F)nfsMount3RExportPrint, sizeof(NfsMount3RExport),
};

SunProg nfsMount3Prog = 
{
	NfsMount3Program,
	NfsMount3Version,
	proc3,
	nelem(proc3),
};

SunProg nfsMount1Prog =
{
	NfsMount1Program,
	NfsMount1Version,
	proc1,
	nelem(proc1),
};
