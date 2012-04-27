#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>
#include <nfs3.h>

char*
nfs3StatusStr(Nfs3Status x)
{
	switch(x){
	case Nfs3Ok:
		return "Nfs3Ok";
	case Nfs3ErrNotOwner:
		return "Nfs3ErrNotOwner";
	case Nfs3ErrNoEnt:
		return "Nfs3ErrNoEnt";
	case Nfs3ErrNoMem:
		return "Nfs3ErrNoMem";
	case Nfs3ErrIo:
		return "Nfs3ErrIo";
	case Nfs3ErrNxio:
		return "Nfs3ErrNxio";
	case Nfs3ErrAcces:
		return "Nfs3ErrAcces";
	case Nfs3ErrExist:
		return "Nfs3ErrExist";
	case Nfs3ErrXDev:
		return "Nfs3ErrXDev";
	case Nfs3ErrNoDev:
		return "Nfs3ErrNoDev";
	case Nfs3ErrNotDir:
		return "Nfs3ErrNotDir";
	case Nfs3ErrIsDir:
		return "Nfs3ErrIsDir";
	case Nfs3ErrInval:
		return "Nfs3ErrInval";
	case Nfs3ErrFbig:
		return "Nfs3ErrFbig";
	case Nfs3ErrNoSpc:
		return "Nfs3ErrNoSpc";
	case Nfs3ErrRoFs:
		return "Nfs3ErrRoFs";
	case Nfs3ErrMLink:
		return "Nfs3ErrMLink";
	case Nfs3ErrNameTooLong:
		return "Nfs3ErrNameTooLong";
	case Nfs3ErrNotEmpty:
		return "Nfs3ErrNotEmpty";
	case Nfs3ErrDQuot:
		return "Nfs3ErrDQuot";
	case Nfs3ErrStale:
		return "Nfs3ErrStale";
	case Nfs3ErrRemote:
		return "Nfs3ErrRemote";
	case Nfs3ErrBadHandle:
		return "Nfs3ErrBadHandle";
	case Nfs3ErrNotSync:
		return "Nfs3ErrNotSync";
	case Nfs3ErrBadCookie:
		return "Nfs3ErrBadCookie";
	case Nfs3ErrNotSupp:
		return "Nfs3ErrNotSupp";
	case Nfs3ErrTooSmall:
		return "Nfs3ErrTooSmall";
	case Nfs3ErrServerFault:
		return "Nfs3ErrServerFault";
	case Nfs3ErrBadType:
		return "Nfs3ErrBadType";
	case Nfs3ErrJukebox:
		return "Nfs3ErrJukebox";
	case Nfs3ErrFprintNotFound:
		return "Nfs3ErrFprintNotFound";
	case Nfs3ErrAborted:
		return "Nfs3ErrAborted";
	default:
		return "unknown";
	}
}

static struct {
	SunStatus status;
	char *msg;
} etab[] = {
	Nfs3ErrNotOwner,	"not owner",
	Nfs3ErrNoEnt,		"directory entry not found",
	Nfs3ErrIo,			"i/o error",
	Nfs3ErrNxio,		"no such device",
	Nfs3ErrNoMem,	"out of memory",
	Nfs3ErrAcces,		"access denied",
	Nfs3ErrExist,		"file or directory exists",
	Nfs3ErrXDev,		"cross-device operation",
	Nfs3ErrNoDev,		"no such device",
	Nfs3ErrNotDir,		"not a directory",
	Nfs3ErrIsDir,		"is a directory",
	Nfs3ErrInval,		"invalid arguments",
	Nfs3ErrFbig,		"file too big",
	Nfs3ErrNoSpc,		"no space left on device",
	Nfs3ErrRoFs,		"read-only file system",
	Nfs3ErrMLink,		"too many links",
	Nfs3ErrNameTooLong,	"name too long",
	Nfs3ErrNotEmpty,	"directory not empty",
	Nfs3ErrDQuot,		"dquot",
	Nfs3ErrStale,		"stale handle",
	Nfs3ErrRemote,	"remote error",
	Nfs3ErrBadHandle,	"bad handle",
	Nfs3ErrNotSync,	"out of sync with server",
	Nfs3ErrBadCookie,	"bad cookie",
	Nfs3ErrNotSupp,	"not supported",
	Nfs3ErrTooSmall,	"too small",
	Nfs3ErrServerFault,	"server fault",
	Nfs3ErrBadType,	"bad type",
	Nfs3ErrJukebox,	"jukebox -- try again later",
	Nfs3ErrFprintNotFound,	"fprint not found",
	Nfs3ErrAborted,	"aborted",
};

void
nfs3Errstr(SunStatus status)
{
	int i;

	for(i=0; i<nelem(etab); i++){
		if(etab[i].status == status){
			werrstr(etab[i].msg);
			return;
		}
	}
	werrstr("unknown nfs3 error %d", (int)status);
}

char*
nfs3FileTypeStr(Nfs3FileType x)
{
	switch(x){
	case Nfs3FileReg:
		return "Nfs3FileReg";
	case Nfs3FileDir:
		return "Nfs3FileDir";
	case Nfs3FileBlock:
		return "Nfs3FileBlock";
	case Nfs3FileChar:
		return "Nfs3FileChar";
	case Nfs3FileSymlink:
		return "Nfs3FileSymlink";
	case Nfs3FileSocket:
		return "Nfs3FileSocket";
	case Nfs3FileFifo:
		return "Nfs3FileFifo";
	default:
		return "unknown";
	}
}

void
nfs3HandlePrint(Fmt *fmt, Nfs3Handle *x)
{
	fmtprint(fmt, "%s\n", "Nfs3Handle");
	fmtprint(fmt, "\t%s=", "handle");
	if(x->len > 64)
		fmtprint(fmt, "%.*H... (%d)", 64, x->h, x->len);
	else
		fmtprint(fmt, "%.*H", x->len, x->h);
	fmtprint(fmt, "\n");
}
uint
nfs3HandleSize(Nfs3Handle *x)
{
	uint a;
	USED(x);
	a = 0 + sunVarOpaqueSize(x->len);
	return a;
}
int
nfs3HandlePack(uchar *a, uchar *ea, uchar **pa, Nfs3Handle *x)
{
	if(x->len > Nfs3MaxHandleSize || sunUint32Pack(a, ea, &a, &x->len) < 0
	|| sunFixedOpaquePack(a, ea, &a, x->h, x->len) < 0)
		goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3HandleUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3Handle *x)
{
	uchar *ha;
	u32int n;

	if(sunUint32Unpack(a, ea, &a, &n) < 0 || n > Nfs3MaxHandleSize)
		goto Err;
	ha = a;
	a += (n+3)&~3;
	if(a > ea)
		goto Err;
	memmove(x->h, ha, n);
	x->len = n;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TimePrint(Fmt *fmt, Nfs3Time *x)
{
	fmtprint(fmt, "%s\n", "Nfs3Time");
	fmtprint(fmt, "\t%s=", "sec");
	fmtprint(fmt, "%ud", x->sec);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "nsec");
	fmtprint(fmt, "%ud", x->nsec);
	fmtprint(fmt, "\n");
}
uint
nfs3TimeSize(Nfs3Time *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	return a;
}
int
nfs3TimePack(uchar *a, uchar *ea, uchar **pa, Nfs3Time *x)
{
	if(sunUint32Pack(a, ea, &a, &x->sec) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->nsec) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TimeUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3Time *x)
{
	if(sunUint32Unpack(a, ea, &a, &x->sec) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->nsec) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3AttrPrint(Fmt *fmt, Nfs3Attr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3Attr");
	fmtprint(fmt, "\t%s=", "type");
	fmtprint(fmt, "%s", nfs3FileTypeStr(x->type));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "mode");
	fmtprint(fmt, "%ud", x->mode);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "nlink");
	fmtprint(fmt, "%ud", x->nlink);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "uid");
	fmtprint(fmt, "%ud", x->uid);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "gid");
	fmtprint(fmt, "%ud", x->gid);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "size");
	fmtprint(fmt, "%llud", x->size);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "used");
	fmtprint(fmt, "%llud", x->used);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "major");
	fmtprint(fmt, "%ud", x->major);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "minor");
	fmtprint(fmt, "%ud", x->minor);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "fsid");
	fmtprint(fmt, "%llud", x->fsid);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "fileid");
	fmtprint(fmt, "%llud", x->fileid);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "atime");
	nfs3TimePrint(fmt, &x->atime);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "mtime");
	nfs3TimePrint(fmt, &x->mtime);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "ctime");
	nfs3TimePrint(fmt, &x->ctime);
	fmtprint(fmt, "\n");
}
uint
nfs3AttrSize(Nfs3Attr *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4 + 8 + 8 + nfs3TimeSize(&x->atime) + nfs3TimeSize(&x->mtime) + nfs3TimeSize(&x->ctime);
	return a;
}
int
nfs3AttrPack(uchar *a, uchar *ea, uchar **pa, Nfs3Attr *x)
{
	int i;

	if(i=x->type, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->mode) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->nlink) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->uid) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->gid) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->size) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->used) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->major) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->minor) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->fsid) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->fileid) < 0) goto Err;
	if(nfs3TimePack(a, ea, &a, &x->atime) < 0) goto Err;
	if(nfs3TimePack(a, ea, &a, &x->mtime) < 0) goto Err;
	if(nfs3TimePack(a, ea, &a, &x->ctime) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3AttrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3Attr *x)
{
	int i;
	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->type = i;
	if(sunUint32Unpack(a, ea, &a, &x->mode) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->nlink) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->uid) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->gid) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->size) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->used) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->major) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->minor) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->fsid) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->fileid) < 0) goto Err;
	if(nfs3TimeUnpack(a, ea, &a, &x->atime) < 0) goto Err;
	if(nfs3TimeUnpack(a, ea, &a, &x->mtime) < 0) goto Err;
	if(nfs3TimeUnpack(a, ea, &a, &x->ctime) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3WccAttrPrint(Fmt *fmt, Nfs3WccAttr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3WccAttr");
	fmtprint(fmt, "\t%s=", "size");
	fmtprint(fmt, "%llud", x->size);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "mtime");
	nfs3TimePrint(fmt, &x->mtime);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "ctime");
	nfs3TimePrint(fmt, &x->ctime);
	fmtprint(fmt, "\n");
}
uint
nfs3WccAttrSize(Nfs3WccAttr *x)
{
	uint a;
	USED(x);
	a = 0 + 8 + nfs3TimeSize(&x->mtime) + nfs3TimeSize(&x->ctime);
	return a;
}
int
nfs3WccAttrPack(uchar *a, uchar *ea, uchar **pa, Nfs3WccAttr *x)
{
	if(sunUint64Pack(a, ea, &a, &x->size) < 0) goto Err;
	if(nfs3TimePack(a, ea, &a, &x->mtime) < 0) goto Err;
	if(nfs3TimePack(a, ea, &a, &x->ctime) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3WccAttrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3WccAttr *x)
{
	if(sunUint64Unpack(a, ea, &a, &x->size) < 0) goto Err;
	if(nfs3TimeUnpack(a, ea, &a, &x->mtime) < 0) goto Err;
	if(nfs3TimeUnpack(a, ea, &a, &x->ctime) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3WccPrint(Fmt *fmt, Nfs3Wcc *x)
{
	fmtprint(fmt, "%s\n", "Nfs3Wcc");
	fmtprint(fmt, "\t%s=", "haveWccAttr");
	fmtprint(fmt, "%d", x->haveWccAttr);
	fmtprint(fmt, "\n");
	switch(x->haveWccAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "wccAttr");
		nfs3WccAttrPrint(fmt, &x->wccAttr);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3WccSize(Nfs3Wcc *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->haveWccAttr){
	case 1:
		a = a + nfs3WccAttrSize(&x->wccAttr);
		break;
	}
	a = a + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	return a;
}
int
nfs3WccPack(uchar *a, uchar *ea, uchar **pa, Nfs3Wcc *x)
{
	if(sunUint1Pack(a, ea, &a, &x->haveWccAttr) < 0) goto Err;
	switch(x->haveWccAttr){
	case 1:
		if(nfs3WccAttrPack(a, ea, &a, &x->wccAttr) < 0) goto Err;
		break;
	}
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3WccUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3Wcc *x)
{
	if(sunUint1Unpack(a, ea, &a, &x->haveWccAttr) < 0) goto Err;
	switch(x->haveWccAttr){
	case 1:
		if(nfs3WccAttrUnpack(a, ea, &a, &x->wccAttr) < 0) goto Err;
		break;
	}
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
char*
nfs3SetTimeStr(Nfs3SetTime x)
{
	switch(x){
	case Nfs3SetTimeDont:
		return "Nfs3SetTimeDont";
	case Nfs3SetTimeServer:
		return "Nfs3SetTimeServer";
	case Nfs3SetTimeClient:
		return "Nfs3SetTimeClient";
	default:
		return "unknown";
	}
}

void
nfs3SetAttrPrint(Fmt *fmt, Nfs3SetAttr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3SetAttr");
	fmtprint(fmt, "\t%s=", "setMode");
	fmtprint(fmt, "%d", x->setMode);
	fmtprint(fmt, "\n");
	switch(x->setMode){
	case 1:
		fmtprint(fmt, "\t%s=", "mode");
		fmtprint(fmt, "%ud", x->mode);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "setUid");
	fmtprint(fmt, "%d", x->setUid);
	fmtprint(fmt, "\n");
	switch(x->setUid){
	case 1:
		fmtprint(fmt, "\t%s=", "uid");
		fmtprint(fmt, "%ud", x->uid);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "setGid");
	fmtprint(fmt, "%d", x->setGid);
	fmtprint(fmt, "\n");
	switch(x->setGid){
	case 1:
		fmtprint(fmt, "\t%s=", "gid");
		fmtprint(fmt, "%ud", x->gid);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "setSize");
	fmtprint(fmt, "%d", x->setSize);
	fmtprint(fmt, "\n");
	switch(x->setSize){
	case 1:
		fmtprint(fmt, "\t%s=", "size");
		fmtprint(fmt, "%llud", x->size);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "setAtime");
	fmtprint(fmt, "%s", nfs3SetTimeStr(x->setAtime));
	fmtprint(fmt, "\n");
	switch(x->setAtime){
	case Nfs3SetTimeClient:
		fmtprint(fmt, "\t%s=", "atime");
		nfs3TimePrint(fmt, &x->atime);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "setMtime");
	fmtprint(fmt, "%s", nfs3SetTimeStr(x->setMtime));
	fmtprint(fmt, "\n");
	switch(x->setMtime){
	case Nfs3SetTimeClient:
		fmtprint(fmt, "\t%s=", "mtime");
		nfs3TimePrint(fmt, &x->mtime);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3SetAttrSize(Nfs3SetAttr *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->setMode){
	case 1:
		a = a + 4;
		break;
	}
	a = a + 4;
	switch(x->setUid){
	case 1:
		a = a + 4;
		break;
	}
	a = a + 4;
	switch(x->setGid){
	case 1:
		a = a + 4;
		break;
	}
	a = a + 4;
	switch(x->setSize){
	case 1:
		a = a + 8;
		break;
	}
	a = a + 4;
	switch(x->setAtime){
	case Nfs3SetTimeClient:
		a = a + nfs3TimeSize(&x->atime);
		break;
	}
	a = a + 4;
	switch(x->setMtime){
	case Nfs3SetTimeClient:
		a = a + nfs3TimeSize(&x->mtime);
		break;
	}
	return a;
}
int
nfs3SetAttrPack(uchar *a, uchar *ea, uchar **pa, Nfs3SetAttr *x)
{
	int i;

	if(sunUint1Pack(a, ea, &a, &x->setMode) < 0) goto Err;
	switch(x->setMode){
	case 1:
		if(sunUint32Pack(a, ea, &a, &x->mode) < 0) goto Err;
		break;
	}
	if(sunUint1Pack(a, ea, &a, &x->setUid) < 0) goto Err;
	switch(x->setUid){
	case 1:
		if(sunUint32Pack(a, ea, &a, &x->uid) < 0) goto Err;
		break;
	}
	if(sunUint1Pack(a, ea, &a, &x->setGid) < 0) goto Err;
	switch(x->setGid){
	case 1:
		if(sunUint32Pack(a, ea, &a, &x->gid) < 0) goto Err;
		break;
	}
	if(sunUint1Pack(a, ea, &a, &x->setSize) < 0) goto Err;
	switch(x->setSize){
	case 1:
		if(sunUint64Pack(a, ea, &a, &x->size) < 0) goto Err;
		break;
	}
	if(i=x->setAtime, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->setAtime){
	case Nfs3SetTimeClient:
		if(nfs3TimePack(a, ea, &a, &x->atime) < 0) goto Err;
		break;
	}
	if(i=x->setMtime, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->setMtime){
	case Nfs3SetTimeClient:
		if(nfs3TimePack(a, ea, &a, &x->mtime) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3SetAttrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3SetAttr *x)
{
	int i;

	if(sunUint1Unpack(a, ea, &a, &x->setMode) < 0) goto Err;
	switch(x->setMode){
	case 1:
		if(sunUint32Unpack(a, ea, &a, &x->mode) < 0) goto Err;
		break;
	}
	if(sunUint1Unpack(a, ea, &a, &x->setUid) < 0) goto Err;
	switch(x->setUid){
	case 1:
		if(sunUint32Unpack(a, ea, &a, &x->uid) < 0) goto Err;
		break;
	}
	if(sunUint1Unpack(a, ea, &a, &x->setGid) < 0) goto Err;
	switch(x->setGid){
	case 1:
		if(sunUint32Unpack(a, ea, &a, &x->gid) < 0) goto Err;
		break;
	}
	if(sunUint1Unpack(a, ea, &a, &x->setSize) < 0) goto Err;
	switch(x->setSize){
	case 1:
		if(sunUint64Unpack(a, ea, &a, &x->size) < 0) goto Err;
		break;
	}
	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->setAtime = i;
	switch(x->setAtime){
	case Nfs3SetTimeClient:
		if(nfs3TimeUnpack(a, ea, &a, &x->atime) < 0) goto Err;
		break;
	}
	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->setMtime = i;
	switch(x->setMtime){
	case Nfs3SetTimeClient:
		if(nfs3TimeUnpack(a, ea, &a, &x->mtime) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TNullPrint(Fmt *fmt, Nfs3TNull *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "Nfs3TNull");
}
uint
nfs3TNullSize(Nfs3TNull *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfs3TNullPack(uchar *a, uchar *ea, uchar **pa, Nfs3TNull *x)
{
	USED(x);
	USED(ea);
	*pa = a;
	return 0;
}
int
nfs3TNullUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TNull *x)
{
	USED(x);
	USED(ea);
	*pa = a;
	return 0;
}
void
nfs3RNullPrint(Fmt *fmt, Nfs3RNull *x)
{
	USED(x);
	fmtprint(fmt, "%s\n", "Nfs3RNull");
}
uint
nfs3RNullSize(Nfs3RNull *x)
{
	uint a;
	USED(x);
	a = 0;
	return a;
}
int
nfs3RNullPack(uchar *a, uchar *ea, uchar **pa, Nfs3RNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
int
nfs3RNullUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RNull *x)
{
	USED(ea);
	USED(x);
	*pa = a;
	return 0;
}
void
nfs3TGetattrPrint(Fmt *fmt, Nfs3TGetattr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TGetattr");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
}
uint
nfs3TGetattrSize(Nfs3TGetattr *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle);
	return a;
}
int
nfs3TGetattrPack(uchar *a, uchar *ea, uchar **pa, Nfs3TGetattr *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TGetattrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TGetattr *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RGetattrPrint(Fmt *fmt, Nfs3RGetattr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RGetattr");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RGetattrSize(Nfs3RGetattr *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case Nfs3Ok:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	return a;
}
int
nfs3RGetattrPack(uchar *a, uchar *ea, uchar **pa, Nfs3RGetattr *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RGetattrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RGetattr *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	switch(x->status){
	case Nfs3Ok:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TSetattrPrint(Fmt *fmt, Nfs3TSetattr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TSetattr");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "attr");
	nfs3SetAttrPrint(fmt, &x->attr);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "checkCtime");
	fmtprint(fmt, "%d", x->checkCtime);
	fmtprint(fmt, "\n");
	switch(x->checkCtime){
	case 1:
		fmtprint(fmt, "\t%s=", "ctime");
		nfs3TimePrint(fmt, &x->ctime);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3TSetattrSize(Nfs3TSetattr *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + nfs3SetAttrSize(&x->attr) + 4;
	switch(x->checkCtime){
	case 1:
		a = a + nfs3TimeSize(&x->ctime);
		break;
	}
	return a;
}
int
nfs3TSetattrPack(uchar *a, uchar *ea, uchar **pa, Nfs3TSetattr *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(nfs3SetAttrPack(a, ea, &a, &x->attr) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->checkCtime) < 0) goto Err;
	switch(x->checkCtime){
	case 1:
		if(nfs3TimePack(a, ea, &a, &x->ctime) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TSetattrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TSetattr *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(nfs3SetAttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
	if(sunUint1Unpack(a, ea, &a, &x->checkCtime) < 0) goto Err;
	switch(x->checkCtime){
	case 1:
		if(nfs3TimeUnpack(a, ea, &a, &x->ctime) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RSetattrPrint(Fmt *fmt, Nfs3RSetattr *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RSetattr");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "wcc");
	nfs3WccPrint(fmt, &x->wcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RSetattrSize(Nfs3RSetattr *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + nfs3WccSize(&x->wcc);
	return a;
}
int
nfs3RSetattrPack(uchar *a, uchar *ea, uchar **pa, Nfs3RSetattr *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->wcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RSetattrUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RSetattr *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(nfs3WccUnpack(a, ea, &a, &x->wcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TLookupPrint(Fmt *fmt, Nfs3TLookup *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TLookup");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
}
uint
nfs3TLookupSize(Nfs3TLookup *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name);
	return a;
}
int
nfs3TLookupPack(uchar *a, uchar *ea, uchar **pa, Nfs3TLookup *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TLookupUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TLookup *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RLookupPrint(Fmt *fmt, Nfs3RLookup *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RLookup");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "handle");
		nfs3HandlePrint(fmt, &x->handle);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "haveAttr");
		fmtprint(fmt, "%d", x->haveAttr);
		fmtprint(fmt, "\n");
		switch(x->haveAttr){
		case 1:
			fmtprint(fmt, "\t%s=", "attr");
			nfs3AttrPrint(fmt, &x->attr);
			fmtprint(fmt, "\n");
			break;
		}
		break;
	}
	fmtprint(fmt, "\t%s=", "haveDirAttr");
	fmtprint(fmt, "%d", x->haveDirAttr);
	fmtprint(fmt, "\n");
	switch(x->haveDirAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "dirAttr");
		nfs3AttrPrint(fmt, &x->dirAttr);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RLookupSize(Nfs3RLookup *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case Nfs3Ok:
		a = a + nfs3HandleSize(&x->handle) + 4;
		switch(x->haveAttr){
		case 1:
			a = a + nfs3AttrSize(&x->attr);
			break;
		}
			break;
	}
	a = a + 4;
	switch(x->haveDirAttr){
	case 1:
		a = a + nfs3AttrSize(&x->dirAttr);
		break;
	}
	return a;
}
int
nfs3RLookupPack(uchar *a, uchar *ea, uchar **pa, Nfs3RLookup *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(sunUint1Pack(a, ea, &a, &x->haveDirAttr) < 0) goto Err;
	switch(x->haveDirAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->dirAttr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RLookupUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RLookup *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	switch(x->status){
	case Nfs3Ok:
		if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(sunUint1Unpack(a, ea, &a, &x->haveDirAttr) < 0) goto Err;
	switch(x->haveDirAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->dirAttr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TAccessPrint(Fmt *fmt, Nfs3TAccess *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TAccess");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "access");
	fmtprint(fmt, "%ud", x->access);
	fmtprint(fmt, "\n");
}
uint
nfs3TAccessSize(Nfs3TAccess *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + 4;
	return a;
}
int
nfs3TAccessPack(uchar *a, uchar *ea, uchar **pa, Nfs3TAccess *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->access) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TAccessUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TAccess *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->access) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RAccessPrint(Fmt *fmt, Nfs3RAccess *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RAccess");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "access");
		fmtprint(fmt, "%ud", x->access);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RAccessSize(Nfs3RAccess *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + 4;
		break;
	}
	return a;
}
int
nfs3RAccessPack(uchar *a, uchar *ea, uchar **pa, Nfs3RAccess *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Pack(a, ea, &a, &x->access) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RAccessUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RAccess *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Unpack(a, ea, &a, &x->access) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TReadlinkPrint(Fmt *fmt, Nfs3TReadlink *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TReadlink");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
}
uint
nfs3TReadlinkSize(Nfs3TReadlink *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle);
	return a;
}
int
nfs3TReadlinkPack(uchar *a, uchar *ea, uchar **pa, Nfs3TReadlink *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TReadlinkUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TReadlink *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RReadlinkPrint(Fmt *fmt, Nfs3RReadlink *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RReadlink");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "data");
		fmtprint(fmt, "\"%s\"", x->data);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RReadlinkSize(Nfs3RReadlink *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + sunStringSize(x->data);
		break;
	}
	return a;
}
int
nfs3RReadlinkPack(uchar *a, uchar *ea, uchar **pa, Nfs3RReadlink *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunStringPack(a, ea, &a, &x->data, -1) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RReadlinkUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RReadlink *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunStringUnpack(a, ea, &a, &x->data, -1) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TReadPrint(Fmt *fmt, Nfs3TRead *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TRead");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "offset");
	fmtprint(fmt, "%llud", x->offset);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "count");
	fmtprint(fmt, "%ud", x->count);
	fmtprint(fmt, "\n");
}
uint
nfs3TReadSize(Nfs3TRead *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + 8 + 4;
	return a;
}
int
nfs3TReadPack(uchar *a, uchar *ea, uchar **pa, Nfs3TRead *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->offset) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TReadUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TRead *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->offset) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RReadPrint(Fmt *fmt, Nfs3RRead *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RRead");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "count");
		fmtprint(fmt, "%ud", x->count);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "eof");
		fmtprint(fmt, "%d", x->eof);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "data");
		if(x->ndata <= 32)
			fmtprint(fmt, "%.*H", x->ndata, x->data);
		else
			fmtprint(fmt, "%.32H...", x->data);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RReadSize(Nfs3RRead *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + 4 + 4 + sunVarOpaqueSize(x->ndata);
		break;
	}
	return a;
}
int
nfs3RReadPack(uchar *a, uchar *ea, uchar **pa, Nfs3RRead *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Pack(a, ea, &a, &x->count) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->eof) < 0) goto Err;
		if(sunVarOpaquePack(a, ea, &a, &x->data, &x->ndata, x->count) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RReadUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RRead *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Unpack(a, ea, &a, &x->count) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->eof) < 0) goto Err;
		if(sunVarOpaqueUnpack(a, ea, &a, &x->data, &x->ndata, x->count) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
char*
nfs3SyncStr(Nfs3Sync x)
{
	switch(x){
	case Nfs3SyncNone:
		return "Nfs3SyncNone";
	case Nfs3SyncData:
		return "Nfs3SyncData";
	case Nfs3SyncFile:
		return "Nfs3SyncFile";
	default:
		return "unknown";
	}
}

void
nfs3TWritePrint(Fmt *fmt, Nfs3TWrite *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TWrite");
	fmtprint(fmt, "\t%s=", "file");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "offset");
	fmtprint(fmt, "%llud", x->offset);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "count");
	fmtprint(fmt, "%ud", x->count);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "stable");
	fmtprint(fmt, "%s", nfs3SyncStr(x->stable));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "data");
	if(x->ndata > 32)
		fmtprint(fmt, "%.32H... (%d)", x->data, x->ndata);
	else
		fmtprint(fmt, "%.*H", x->ndata, x->data);
	fmtprint(fmt, "\n");
}
uint
nfs3TWriteSize(Nfs3TWrite *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + 8 + 4 + 4 + sunVarOpaqueSize(x->ndata);
	return a;
}
int
nfs3TWritePack(uchar *a, uchar *ea, uchar **pa, Nfs3TWrite *x)
{
	int i;

	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->offset) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->count) < 0) goto Err;
	if(i=x->stable, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunVarOpaquePack(a, ea, &a, &x->data, &x->ndata, x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TWriteUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TWrite *x)
{
	int i;

	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->offset) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->count) < 0) goto Err;
	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->stable = i;
	if(sunVarOpaqueUnpack(a, ea, &a, &x->data, &x->ndata, x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RWritePrint(Fmt *fmt, Nfs3RWrite *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RWrite");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "wcc");
	nfs3WccPrint(fmt, &x->wcc);
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "count");
		fmtprint(fmt, "%ud", x->count);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "committed");
		fmtprint(fmt, "%s", nfs3SyncStr(x->committed));
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "verf");
		fmtprint(fmt, "%.*H", Nfs3WriteVerfSize, x->verf);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RWriteSize(Nfs3RWrite *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + nfs3WccSize(&x->wcc);
	switch(x->status){
	case Nfs3Ok:
		a = a + 4 + 4 + Nfs3WriteVerfSize;
		break;
	}
	return a;
}
int
nfs3RWritePack(uchar *a, uchar *ea, uchar **pa, Nfs3RWrite *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->wcc) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Pack(a, ea, &a, &x->count) < 0) goto Err;
		if(i=x->committed, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
		if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3WriteVerfSize) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RWriteUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RWrite *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(nfs3WccUnpack(a, ea, &a, &x->wcc) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Unpack(a, ea, &a, &x->count) < 0) goto Err;
		if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->committed = i;
		if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3WriteVerfSize) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
char*
nfs3CreateStr(Nfs3Create x)
{
	switch(x){
	case Nfs3CreateUnchecked:
		return "Nfs3CreateUnchecked";
	case Nfs3CreateGuarded:
		return "Nfs3CreateGuarded";
	case Nfs3CreateExclusive:
		return "Nfs3CreateExclusive";
	default:
		return "unknown";
	}
}

void
nfs3TCreatePrint(Fmt *fmt, Nfs3TCreate *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TCreate");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "mode");
	fmtprint(fmt, "%s", nfs3CreateStr(x->mode));
	fmtprint(fmt, "\n");
	switch(x->mode){
	case Nfs3CreateUnchecked:
	case Nfs3CreateGuarded:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3SetAttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	case Nfs3CreateExclusive:
		fmtprint(fmt, "\t%s=", "verf");
		fmtprint(fmt, "%.*H", Nfs3CreateVerfSize, x->verf);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3TCreateSize(Nfs3TCreate *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name) + 4;
	switch(x->mode){
	case Nfs3CreateUnchecked:
	case Nfs3CreateGuarded:
		a = a + nfs3SetAttrSize(&x->attr);
		break;
	case Nfs3CreateExclusive:
		a = a + Nfs3CreateVerfSize;
		break;
	}
	return a;
}
int
nfs3TCreatePack(uchar *a, uchar *ea, uchar **pa, Nfs3TCreate *x)
{
	int i;

	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(i=x->mode, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->mode){
	case Nfs3CreateUnchecked:
	case Nfs3CreateGuarded:
		if(nfs3SetAttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	case Nfs3CreateExclusive:
		if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3CreateVerfSize) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TCreateUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TCreate *x)
{
	int i;

	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->mode = i;
	switch(x->mode){
	case Nfs3CreateUnchecked:
	case Nfs3CreateGuarded:
		if(nfs3SetAttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	case Nfs3CreateExclusive:
		if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3CreateVerfSize) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RCreatePrint(Fmt *fmt, Nfs3RCreate *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RCreate");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "haveHandle");
		fmtprint(fmt, "%d", x->haveHandle);
		fmtprint(fmt, "\n");
		switch(x->haveHandle){
		case 1:
			fmtprint(fmt, "\t%s=", "handle");
			nfs3HandlePrint(fmt, &x->handle);
			fmtprint(fmt, "\n");
			break;
		}
		fmtprint(fmt, "\t%s=", "haveAttr");
		fmtprint(fmt, "%d", x->haveAttr);
		fmtprint(fmt, "\n");
		switch(x->haveAttr){
		case 1:
			fmtprint(fmt, "\t%s=", "attr");
			nfs3AttrPrint(fmt, &x->attr);
			fmtprint(fmt, "\n");
			break;
		}
		break;
	}
	fmtprint(fmt, "\t%s=", "dirWcc");
	nfs3WccPrint(fmt, &x->dirWcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RCreateSize(Nfs3RCreate *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case Nfs3Ok:
		a = a + 4;
		switch(x->haveHandle){
		case 1:
			a = a + nfs3HandleSize(&x->handle);
			break;
		}
		a = a + 4;
		switch(x->haveAttr){
		case 1:
			a = a + nfs3AttrSize(&x->attr);
			break;
		}
			break;
	}
	a = a + nfs3WccSize(&x->dirWcc);
	return a;
}
int
nfs3RCreatePack(uchar *a, uchar *ea, uchar **pa, Nfs3RCreate *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Pack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccPack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RCreateUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RCreate *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Unpack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccUnpack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TMkdirPrint(Fmt *fmt, Nfs3TMkdir *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TMkdir");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "attr");
	nfs3SetAttrPrint(fmt, &x->attr);
	fmtprint(fmt, "\n");
}
uint
nfs3TMkdirSize(Nfs3TMkdir *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name) + nfs3SetAttrSize(&x->attr);
	return a;
}
int
nfs3TMkdirPack(uchar *a, uchar *ea, uchar **pa, Nfs3TMkdir *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(nfs3SetAttrPack(a, ea, &a, &x->attr) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TMkdirUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TMkdir *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(nfs3SetAttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RMkdirPrint(Fmt *fmt, Nfs3RMkdir *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RMkdir");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "haveHandle");
		fmtprint(fmt, "%d", x->haveHandle);
		fmtprint(fmt, "\n");
		switch(x->haveHandle){
		case 1:
			fmtprint(fmt, "\t%s=", "handle");
			nfs3HandlePrint(fmt, &x->handle);
			fmtprint(fmt, "\n");
			break;
		}
		fmtprint(fmt, "\t%s=", "haveAttr");
		fmtprint(fmt, "%d", x->haveAttr);
		fmtprint(fmt, "\n");
		switch(x->haveAttr){
		case 1:
			fmtprint(fmt, "\t%s=", "attr");
			nfs3AttrPrint(fmt, &x->attr);
			fmtprint(fmt, "\n");
			break;
		}
		break;
	}
	fmtprint(fmt, "\t%s=", "dirWcc");
	nfs3WccPrint(fmt, &x->dirWcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RMkdirSize(Nfs3RMkdir *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case Nfs3Ok:
		a = a + 4;
		switch(x->haveHandle){
		case 1:
			a = a + nfs3HandleSize(&x->handle);
			break;
		}
		a = a + 4;
		switch(x->haveAttr){
		case 1:
			a = a + nfs3AttrSize(&x->attr);
			break;
		}
			break;
	}
	a = a + nfs3WccSize(&x->dirWcc);
	return a;
}
int
nfs3RMkdirPack(uchar *a, uchar *ea, uchar **pa, Nfs3RMkdir *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Pack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccPack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RMkdirUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RMkdir *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Unpack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccUnpack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TSymlinkPrint(Fmt *fmt, Nfs3TSymlink *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TSymlink");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "attr");
	nfs3SetAttrPrint(fmt, &x->attr);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "data");
	fmtprint(fmt, "\"%s\"", x->data);
	fmtprint(fmt, "\n");
}
uint
nfs3TSymlinkSize(Nfs3TSymlink *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name) + nfs3SetAttrSize(&x->attr) + sunStringSize(x->data);
	return a;
}
int
nfs3TSymlinkPack(uchar *a, uchar *ea, uchar **pa, Nfs3TSymlink *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(nfs3SetAttrPack(a, ea, &a, &x->attr) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->data, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TSymlinkUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TSymlink *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(nfs3SetAttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->data, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RSymlinkPrint(Fmt *fmt, Nfs3RSymlink *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RSymlink");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "haveHandle");
		fmtprint(fmt, "%d", x->haveHandle);
		fmtprint(fmt, "\n");
		switch(x->haveHandle){
		case 1:
			fmtprint(fmt, "\t%s=", "handle");
			nfs3HandlePrint(fmt, &x->handle);
			fmtprint(fmt, "\n");
			break;
		}
		fmtprint(fmt, "\t%s=", "haveAttr");
		fmtprint(fmt, "%d", x->haveAttr);
		fmtprint(fmt, "\n");
		switch(x->haveAttr){
		case 1:
			fmtprint(fmt, "\t%s=", "attr");
			nfs3AttrPrint(fmt, &x->attr);
			fmtprint(fmt, "\n");
			break;
		}
		break;
	}
	fmtprint(fmt, "\t%s=", "dirWcc");
	nfs3WccPrint(fmt, &x->dirWcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RSymlinkSize(Nfs3RSymlink *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case Nfs3Ok:
		a = a + 4;
		switch(x->haveHandle){
		case 1:
			a = a + nfs3HandleSize(&x->handle);
			break;
		}
		a = a + 4;
		switch(x->haveAttr){
		case 1:
			a = a + nfs3AttrSize(&x->attr);
			break;
		}
			break;
	}
	a = a + nfs3WccSize(&x->dirWcc);
	return a;
}
int
nfs3RSymlinkPack(uchar *a, uchar *ea, uchar **pa, Nfs3RSymlink *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Pack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccPack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RSymlinkUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RSymlink *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Unpack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccUnpack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TMknodPrint(Fmt *fmt, Nfs3TMknod *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TMknod");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "type");
	fmtprint(fmt, "%s", nfs3FileTypeStr(x->type));
	fmtprint(fmt, "\n");
	switch(x->type){
	case Nfs3FileChar:
	case Nfs3FileBlock:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3SetAttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "major");
		fmtprint(fmt, "%ud", x->major);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "minor");
		fmtprint(fmt, "%ud", x->minor);
		fmtprint(fmt, "\n");
		break;
	case Nfs3FileSocket:
	case Nfs3FileFifo:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3SetAttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3TMknodSize(Nfs3TMknod *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name) + 4;
	switch(x->type){
	case Nfs3FileChar:
	case Nfs3FileBlock:
		a = a + nfs3SetAttrSize(&x->attr) + 4 + 4;
		break;
	case Nfs3FileSocket:
	case Nfs3FileFifo:
		a = a + nfs3SetAttrSize(&x->attr);
		break;
	}
	return a;
}
int
nfs3TMknodPack(uchar *a, uchar *ea, uchar **pa, Nfs3TMknod *x)
{
	int i;

	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(i=x->type, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->type){
	case Nfs3FileChar:
	case Nfs3FileBlock:
		if(nfs3SetAttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->major) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->minor) < 0) goto Err;
		break;
	case Nfs3FileSocket:
	case Nfs3FileFifo:
		if(nfs3SetAttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TMknodUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TMknod *x)
{
	int i;

	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->type = i;
	switch(x->type){
	case Nfs3FileChar:
	case Nfs3FileBlock:
		if(nfs3SetAttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->major) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->minor) < 0) goto Err;
		break;
	case Nfs3FileSocket:
	case Nfs3FileFifo:
		if(nfs3SetAttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RMknodPrint(Fmt *fmt, Nfs3RMknod *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RMknod");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "haveHandle");
		fmtprint(fmt, "%d", x->haveHandle);
		fmtprint(fmt, "\n");
		switch(x->haveHandle){
		case 1:
			fmtprint(fmt, "\t%s=", "handle");
			nfs3HandlePrint(fmt, &x->handle);
			fmtprint(fmt, "\n");
			break;
		}
		fmtprint(fmt, "\t%s=", "haveAttr");
		fmtprint(fmt, "%d", x->haveAttr);
		fmtprint(fmt, "\n");
		switch(x->haveAttr){
		case 1:
			fmtprint(fmt, "\t%s=", "attr");
			nfs3AttrPrint(fmt, &x->attr);
			fmtprint(fmt, "\n");
			break;
		}
		break;
	}
	fmtprint(fmt, "\t%s=", "dirWcc");
	nfs3WccPrint(fmt, &x->dirWcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RMknodSize(Nfs3RMknod *x)
{
	uint a;
	USED(x);
	a = 0 + 4;
	switch(x->status){
	case Nfs3Ok:
		a = a + 4;
		switch(x->haveHandle){
		case 1:
			a = a + nfs3HandleSize(&x->handle);
			break;
		}
		a = a + 4;
		switch(x->haveAttr){
		case 1:
			a = a + nfs3AttrSize(&x->attr);
			break;
		}
			break;
	}
	a = a + nfs3WccSize(&x->dirWcc);
	return a;
}
int
nfs3RMknodPack(uchar *a, uchar *ea, uchar **pa, Nfs3RMknod *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Pack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccPack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RMknodUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RMknod *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	switch(x->status){
	case Nfs3Ok:
		if(sunUint1Unpack(a, ea, &a, &x->haveHandle) < 0) goto Err;
		switch(x->haveHandle){
		case 1:
			if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
			break;
		}
		if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
		switch(x->haveAttr){
		case 1:
			if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
			break;
		}
		break;
	}
	if(nfs3WccUnpack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TRemovePrint(Fmt *fmt, Nfs3TRemove *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TRemove");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
}
uint
nfs3TRemoveSize(Nfs3TRemove *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name);
	return a;
}
int
nfs3TRemovePack(uchar *a, uchar *ea, uchar **pa, Nfs3TRemove *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TRemoveUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TRemove *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RRemovePrint(Fmt *fmt, Nfs3RRemove *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RRemove");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "wcc");
	nfs3WccPrint(fmt, &x->wcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RRemoveSize(Nfs3RRemove *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + nfs3WccSize(&x->wcc);
	return a;
}
int
nfs3RRemovePack(uchar *a, uchar *ea, uchar **pa, Nfs3RRemove *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->wcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RRemoveUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RRemove *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(nfs3WccUnpack(a, ea, &a, &x->wcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TRmdirPrint(Fmt *fmt, Nfs3TRmdir *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TRmdir");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
}
uint
nfs3TRmdirSize(Nfs3TRmdir *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + sunStringSize(x->name);
	return a;
}
int
nfs3TRmdirPack(uchar *a, uchar *ea, uchar **pa, Nfs3TRmdir *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TRmdirUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TRmdir *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RRmdirPrint(Fmt *fmt, Nfs3RRmdir *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RRmdir");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "wcc");
	nfs3WccPrint(fmt, &x->wcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RRmdirSize(Nfs3RRmdir *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + nfs3WccSize(&x->wcc);
	return a;
}
int
nfs3RRmdirPack(uchar *a, uchar *ea, uchar **pa, Nfs3RRmdir *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->wcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RRmdirUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RRmdir *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(nfs3WccUnpack(a, ea, &a, &x->wcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TRenamePrint(Fmt *fmt, Nfs3TRename *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TRename");
	fmtprint(fmt, "\t%s=", "from");
	fmtprint(fmt, "{\n");
	fmtprint(fmt, "\t\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->from.handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->from.name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t}");
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "to");
	fmtprint(fmt, "{\n");
	fmtprint(fmt, "\t\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->to.handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->to.name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t}");
	fmtprint(fmt, "\n");
}
uint
nfs3TRenameSize(Nfs3TRename *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->from.handle) + sunStringSize(x->from.name) + nfs3HandleSize(&x->to.handle) + sunStringSize(x->to.name);
	return a;
}
int
nfs3TRenamePack(uchar *a, uchar *ea, uchar **pa, Nfs3TRename *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->from.handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->from.name, -1) < 0) goto Err;
	if(nfs3HandlePack(a, ea, &a, &x->to.handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->to.name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TRenameUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TRename *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->from.handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->from.name, -1) < 0) goto Err;
	if(nfs3HandleUnpack(a, ea, &a, &x->to.handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->to.name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RRenamePrint(Fmt *fmt, Nfs3RRename *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RRename");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "fromWcc");
	nfs3WccPrint(fmt, &x->fromWcc);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "toWcc");
	nfs3WccPrint(fmt, &x->toWcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RRenameSize(Nfs3RRename *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + nfs3WccSize(&x->fromWcc) + nfs3WccSize(&x->toWcc);
	return a;
}
int
nfs3RRenamePack(uchar *a, uchar *ea, uchar **pa, Nfs3RRename *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->fromWcc) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->toWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RRenameUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RRename *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(nfs3WccUnpack(a, ea, &a, &x->fromWcc) < 0) goto Err;
	if(nfs3WccUnpack(a, ea, &a, &x->toWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TLinkPrint(Fmt *fmt, Nfs3TLink *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TLink");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "link");
	fmtprint(fmt, "{\n");
	fmtprint(fmt, "\t\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->link.handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->link.name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t}");
	fmtprint(fmt, "\n");
}
uint
nfs3TLinkSize(Nfs3TLink *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + nfs3HandleSize(&x->link.handle) + sunStringSize(x->link.name);
	return a;
}
int
nfs3TLinkPack(uchar *a, uchar *ea, uchar **pa, Nfs3TLink *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(nfs3HandlePack(a, ea, &a, &x->link.handle) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->link.name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TLinkUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TLink *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(nfs3HandleUnpack(a, ea, &a, &x->link.handle) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->link.name, -1) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RLinkPrint(Fmt *fmt, Nfs3RLink *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RLink");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "dirWcc");
	nfs3WccPrint(fmt, &x->dirWcc);
	fmtprint(fmt, "\n");
}
uint
nfs3RLinkSize(Nfs3RLink *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	a = a + nfs3WccSize(&x->dirWcc);
	return a;
}
int
nfs3RLinkPack(uchar *a, uchar *ea, uchar **pa, Nfs3RLink *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	if(nfs3WccPack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RLinkUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RLink *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	if(nfs3WccUnpack(a, ea, &a, &x->dirWcc) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TReadDirPrint(Fmt *fmt, Nfs3TReadDir *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TReadDir");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "cookie");
	fmtprint(fmt, "%llud", x->cookie);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "verf");
	fmtprint(fmt, "%.*H", Nfs3CookieVerfSize, x->verf);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "count");
	fmtprint(fmt, "%ud", x->count);
	fmtprint(fmt, "\n");
}
uint
nfs3TReadDirSize(Nfs3TReadDir *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + 8 + Nfs3CookieVerfSize + 4;
	return a;
}
int
nfs3TReadDirPack(uchar *a, uchar *ea, uchar **pa, Nfs3TReadDir *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->cookie) < 0) goto Err;
	if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TReadDirUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TReadDir *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->cookie) < 0) goto Err;
	if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3EntryPrint(Fmt *fmt, Nfs3Entry *x)
{
	fmtprint(fmt, "%s\n", "Nfs3Entry");
	fmtprint(fmt, "\t%s=", "fileid");
	fmtprint(fmt, "%llud", x->fileid);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "cookie");
	fmtprint(fmt, "%llud", x->cookie);
	fmtprint(fmt, "\n");
}
uint
nfs3EntrySize(Nfs3Entry *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 8 + sunStringSize(x->name) + 8;
	return a;
}
int
nfs3EntryPack(uchar *a, uchar *ea, uchar **pa, Nfs3Entry *x)
{
	u1int one;

	one = 1;
	if(sunUint1Pack(a, ea, &a, &one) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->fileid) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->cookie) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3EntryUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3Entry *x)
{
	u1int one;

	memset(x, 0, sizeof *x);
	if(sunUint1Unpack(a, ea, &a, &one) < 0 || one != 1) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->fileid) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->cookie) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RReadDirPrint(Fmt *fmt, Nfs3RReadDir *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RReadDir");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "verf");
		fmtprint(fmt, "%.*H", Nfs3CookieVerfSize, x->verf);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=%ud\n", "count", x->count);
		fmtprint(fmt, "\t%s=", "eof");
		fmtprint(fmt, "%d", x->eof);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RReadDirSize(Nfs3RReadDir *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + Nfs3CookieVerfSize;
		a += x->count;
		a += 4 + 4;
		break;
	}
	return a;
}
int
nfs3RReadDirPack(uchar *a, uchar *ea, uchar **pa, Nfs3RReadDir *x)
{
	int i;
	u1int zero;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
		if(sunFixedOpaquePack(a, ea, &a, x->data, x->count) < 0) goto Err;
		zero = 0;
		if(sunUint1Pack(a, ea, &a, &zero) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->eof) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
countEntry(uchar *a, uchar *ea, uchar **pa, u32int *n)
{
	uchar *oa;
	u64int u64;
	u32int u32;
	u1int u1;

	oa = a;
	for(;;){
		if(sunUint1Unpack(a, ea, &a, &u1) < 0)
			return -1;
		if(u1 == 0)
			break;
		if(sunUint64Unpack(a, ea, &a, &u64) < 0
		|| sunUint32Unpack(a, ea, &a, &u32) < 0)
			return -1;
		a += (u32+3)&~3;
		if(a >= ea)
			return -1;
		if(sunUint64Unpack(a, ea, &a, &u64) < 0)
			return -1;
	}
	*n = (a-4) - oa;
	*pa = a;
	return 0;
}
int
nfs3RReadDirUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RReadDir *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	if(x->status == Nfs3Ok){
		if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
		x->data = a;
		if(countEntry(a, ea, &a, &x->count) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->eof) < 0) goto Err;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TReadDirPlusPrint(Fmt *fmt, Nfs3TReadDirPlus *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TReadDirPlus");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "cookie");
	fmtprint(fmt, "%llud", x->cookie);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "verf");
	fmtprint(fmt, "%.*H", Nfs3CookieVerfSize, x->verf);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "dirCount");
	fmtprint(fmt, "%ud", x->dirCount);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "maxCount");
	fmtprint(fmt, "%ud", x->maxCount);
	fmtprint(fmt, "\n");
}
uint
nfs3TReadDirPlusSize(Nfs3TReadDirPlus *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + 8 + Nfs3CookieVerfSize + 4 + 4;
	return a;
}
int
nfs3TReadDirPlusPack(uchar *a, uchar *ea, uchar **pa, Nfs3TReadDirPlus *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->cookie) < 0) goto Err;
	if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->dirCount) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->maxCount) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TReadDirPlusUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TReadDirPlus *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->cookie) < 0) goto Err;
	if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->dirCount) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->maxCount) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3EntryPlusPrint(Fmt *fmt, Nfs3Entry *x)
{
	fmtprint(fmt, "%s\n", "Nfs3EntryPlus");
	fmtprint(fmt, "\t%s=", "fileid");
	fmtprint(fmt, "%llud", x->fileid);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "name");
	fmtprint(fmt, "\"%s\"", x->name);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "cookie");
	fmtprint(fmt, "%llud", x->cookie);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	fmtprint(fmt, "\t%s=", "haveHandle");
	fmtprint(fmt, "%d", x->haveHandle);
	fmtprint(fmt, "\n");
	switch(x->haveHandle){
	case 1:
		fmtprint(fmt, "\t%s=", "handle");
		nfs3HandlePrint(fmt, &x->handle);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3EntryPlusSize(Nfs3Entry *x)
{
	uint a;
	USED(x);
	a = 0 + 8 + sunStringSize(x->name) + 8 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	a = a + 4;
	switch(x->haveHandle){
	case 1:
		a = a + nfs3HandleSize(&x->handle);
		break;
	}
	return a;
}
int
nfs3EntryPlusPack(uchar *a, uchar *ea, uchar **pa, Nfs3Entry *x)
{
	u1int u1;

	if(sunUint1Pack(a, ea, &a, &u1) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->fileid) < 0) goto Err;
	if(sunStringPack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->cookie) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	if(sunUint1Pack(a, ea, &a, &x->haveHandle) < 0) goto Err;
	switch(x->haveHandle){
	case 1:
		if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3EntryPlusUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3Entry *x)
{
	u1int u1;

	if(sunUint1Unpack(a, ea, &a, &u1) < 0 || u1 != 1) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->fileid) < 0) goto Err;
	if(sunStringUnpack(a, ea, &a, &x->name, -1) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->cookie) < 0) goto Err;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	if(sunUint1Unpack(a, ea, &a, &x->haveHandle) < 0) goto Err;
	switch(x->haveHandle){
	case 1:
		if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RReadDirPlusPrint(Fmt *fmt, Nfs3RReadDirPlus *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RReadDirPlus");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "verf");
		fmtprint(fmt, "%.*H", Nfs3CookieVerfSize, x->verf);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\tcount=%ud\n", x->count);
		fmtprint(fmt, "\t%s=", "eof");
		fmtprint(fmt, "%d", x->eof);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RReadDirPlusSize(Nfs3RReadDirPlus *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + Nfs3CookieVerfSize;
		a += x->count;
		a += 4 + 4;
		break;
	}
	return a;
}
int
nfs3RReadDirPlusPack(uchar *a, uchar *ea, uchar **pa, Nfs3RReadDirPlus *x)
{
	int i;
	u1int zero;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
		if(sunFixedOpaquePack(a, ea, &a, x->data, x->count) < 0) goto Err;
		zero = 0;
		if(sunUint1Pack(a, ea, &a, &zero) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->eof) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
static int
countEntryPlus(uchar *a, uchar *ea, uchar **pa, u32int *n)
{
	uchar *oa;
	u64int u64;
	u32int u32;
	u1int u1;
	Nfs3Handle h;
	Nfs3Attr attr;

	oa = a;
	for(;;){
		if(sunUint1Unpack(a, ea, &a, &u1) < 0)
			return -1;
		if(u1 == 0)
			break;
		if(sunUint64Unpack(a, ea, &a, &u64) < 0
		|| sunUint32Unpack(a, ea, &a, &u32) < 0)
			return -1;
		a += (u32+3)&~3;
		if(a >= ea)
			return -1;
		if(sunUint64Unpack(a, ea, &a, &u64) < 0
		|| sunUint1Unpack(a, ea, &a, &u1) < 0
		|| (u1 && nfs3AttrUnpack(a, ea, &a, &attr) < 0)
		|| sunUint1Unpack(a, ea, &a, &u1) < 0
		|| (u1 && nfs3HandleUnpack(a, ea, &a, &h) < 0))
			return -1;
	}
	*n = (a-4) - oa;
	*pa = a;
	return 0;
}
		
int
nfs3RReadDirPlusUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RReadDirPlus *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	if(x->status == Nfs3Ok){
		if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3CookieVerfSize) < 0) goto Err;
		x->data = a;
		if(countEntryPlus(a, ea, &a, &x->count) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->eof) < 0) goto Err;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TFsStatPrint(Fmt *fmt, Nfs3TFsStat *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TFsStat");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
}
uint
nfs3TFsStatSize(Nfs3TFsStat *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle);
	return a;
}
int
nfs3TFsStatPack(uchar *a, uchar *ea, uchar **pa, Nfs3TFsStat *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TFsStatUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TFsStat *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RFsStatPrint(Fmt *fmt, Nfs3RFsStat *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RFsStat");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "totalBytes");
		fmtprint(fmt, "%llud", x->totalBytes);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "freeBytes");
		fmtprint(fmt, "%llud", x->freeBytes);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "availBytes");
		fmtprint(fmt, "%llud", x->availBytes);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "totalFiles");
		fmtprint(fmt, "%llud", x->totalFiles);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "freeFiles");
		fmtprint(fmt, "%llud", x->freeFiles);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "availFiles");
		fmtprint(fmt, "%llud", x->availFiles);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "invarSec");
		fmtprint(fmt, "%ud", x->invarSec);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RFsStatSize(Nfs3RFsStat *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + 8 + 8 + 8 + 8 + 8 + 8 + 4;
		break;
	}
	return a;
}
int
nfs3RFsStatPack(uchar *a, uchar *ea, uchar **pa, Nfs3RFsStat *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint64Pack(a, ea, &a, &x->totalBytes) < 0) goto Err;
		if(sunUint64Pack(a, ea, &a, &x->freeBytes) < 0) goto Err;
		if(sunUint64Pack(a, ea, &a, &x->availBytes) < 0) goto Err;
		if(sunUint64Pack(a, ea, &a, &x->totalFiles) < 0) goto Err;
		if(sunUint64Pack(a, ea, &a, &x->freeFiles) < 0) goto Err;
		if(sunUint64Pack(a, ea, &a, &x->availFiles) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->invarSec) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RFsStatUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RFsStat *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint64Unpack(a, ea, &a, &x->totalBytes) < 0) goto Err;
		if(sunUint64Unpack(a, ea, &a, &x->freeBytes) < 0) goto Err;
		if(sunUint64Unpack(a, ea, &a, &x->availBytes) < 0) goto Err;
		if(sunUint64Unpack(a, ea, &a, &x->totalFiles) < 0) goto Err;
		if(sunUint64Unpack(a, ea, &a, &x->freeFiles) < 0) goto Err;
		if(sunUint64Unpack(a, ea, &a, &x->availFiles) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->invarSec) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TFsInfoPrint(Fmt *fmt, Nfs3TFsInfo *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TFsInfo");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
}
uint
nfs3TFsInfoSize(Nfs3TFsInfo *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle);
	return a;
}
int
nfs3TFsInfoPack(uchar *a, uchar *ea, uchar **pa, Nfs3TFsInfo *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TFsInfoUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TFsInfo *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RFsInfoPrint(Fmt *fmt, Nfs3RFsInfo *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RFsInfo");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "readMax");
		fmtprint(fmt, "%ud", x->readMax);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "readPref");
		fmtprint(fmt, "%ud", x->readPref);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "readMult");
		fmtprint(fmt, "%ud", x->readMult);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "writeMax");
		fmtprint(fmt, "%ud", x->writeMax);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "writePref");
		fmtprint(fmt, "%ud", x->writePref);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "writeMult");
		fmtprint(fmt, "%ud", x->writeMult);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "readDirPref");
		fmtprint(fmt, "%ud", x->readDirPref);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "maxFileSize");
		fmtprint(fmt, "%llud", x->maxFileSize);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "timePrec");
		nfs3TimePrint(fmt, &x->timePrec);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "flags");
		fmtprint(fmt, "%ud", x->flags);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RFsInfoSize(Nfs3RFsInfo *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + nfs3TimeSize(&x->timePrec) + 4;
		break;
	}
	return a;
}
int
nfs3RFsInfoPack(uchar *a, uchar *ea, uchar **pa, Nfs3RFsInfo *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Pack(a, ea, &a, &x->readMax) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->readPref) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->readMult) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->writeMax) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->writePref) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->writeMult) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->readDirPref) < 0) goto Err;
		if(sunUint64Pack(a, ea, &a, &x->maxFileSize) < 0) goto Err;
		if(nfs3TimePack(a, ea, &a, &x->timePrec) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->flags) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RFsInfoUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RFsInfo *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Unpack(a, ea, &a, &x->readMax) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->readPref) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->readMult) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->writeMax) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->writePref) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->writeMult) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->readDirPref) < 0) goto Err;
		if(sunUint64Unpack(a, ea, &a, &x->maxFileSize) < 0) goto Err;
		if(nfs3TimeUnpack(a, ea, &a, &x->timePrec) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->flags) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TPathconfPrint(Fmt *fmt, Nfs3TPathconf *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TPathconf");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
}
uint
nfs3TPathconfSize(Nfs3TPathconf *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle);
	return a;
}
int
nfs3TPathconfPack(uchar *a, uchar *ea, uchar **pa, Nfs3TPathconf *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TPathconfUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TPathconf *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RPathconfPrint(Fmt *fmt, Nfs3RPathconf *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RPathconf");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "haveAttr");
	fmtprint(fmt, "%d", x->haveAttr);
	fmtprint(fmt, "\n");
	switch(x->haveAttr){
	case 1:
		fmtprint(fmt, "\t%s=", "attr");
		nfs3AttrPrint(fmt, &x->attr);
		fmtprint(fmt, "\n");
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "maxLink");
		fmtprint(fmt, "%ud", x->maxLink);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "maxName");
		fmtprint(fmt, "%ud", x->maxName);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "noTrunc");
		fmtprint(fmt, "%d", x->noTrunc);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "chownRestricted");
		fmtprint(fmt, "%d", x->chownRestricted);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "caseInsensitive");
		fmtprint(fmt, "%d", x->caseInsensitive);
		fmtprint(fmt, "\n");
		fmtprint(fmt, "\t%s=", "casePreserving");
		fmtprint(fmt, "%d", x->casePreserving);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RPathconfSize(Nfs3RPathconf *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + 4;
	switch(x->haveAttr){
	case 1:
		a = a + nfs3AttrSize(&x->attr);
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		a = a + 4 + 4 + 4 + 4 + 4 + 4;
		break;
	}
	return a;
}
int
nfs3RPathconfPack(uchar *a, uchar *ea, uchar **pa, Nfs3RPathconf *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(sunUint1Pack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrPack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Pack(a, ea, &a, &x->maxLink) < 0) goto Err;
		if(sunUint32Pack(a, ea, &a, &x->maxName) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->noTrunc) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->chownRestricted) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->caseInsensitive) < 0) goto Err;
		if(sunUint1Pack(a, ea, &a, &x->casePreserving) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RPathconfUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RPathconf *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(sunUint1Unpack(a, ea, &a, &x->haveAttr) < 0) goto Err;
	switch(x->haveAttr){
	case 1:
		if(nfs3AttrUnpack(a, ea, &a, &x->attr) < 0) goto Err;
		break;
	}
	switch(x->status){
	case Nfs3Ok:
		if(sunUint32Unpack(a, ea, &a, &x->maxLink) < 0) goto Err;
		if(sunUint32Unpack(a, ea, &a, &x->maxName) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->noTrunc) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->chownRestricted) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->caseInsensitive) < 0) goto Err;
		if(sunUint1Unpack(a, ea, &a, &x->casePreserving) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3TCommitPrint(Fmt *fmt, Nfs3TCommit *x)
{
	fmtprint(fmt, "%s\n", "Nfs3TCommit");
	fmtprint(fmt, "\t%s=", "handle");
	nfs3HandlePrint(fmt, &x->handle);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "offset");
	fmtprint(fmt, "%llud", x->offset);
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "count");
	fmtprint(fmt, "%ud", x->count);
	fmtprint(fmt, "\n");
}
uint
nfs3TCommitSize(Nfs3TCommit *x)
{
	uint a;
	USED(x);
	a = 0 + nfs3HandleSize(&x->handle) + 8 + 4;
	return a;
}
int
nfs3TCommitPack(uchar *a, uchar *ea, uchar **pa, Nfs3TCommit *x)
{
	if(nfs3HandlePack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Pack(a, ea, &a, &x->offset) < 0) goto Err;
	if(sunUint32Pack(a, ea, &a, &x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3TCommitUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3TCommit *x)
{
	if(nfs3HandleUnpack(a, ea, &a, &x->handle) < 0) goto Err;
	if(sunUint64Unpack(a, ea, &a, &x->offset) < 0) goto Err;
	if(sunUint32Unpack(a, ea, &a, &x->count) < 0) goto Err;
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
void
nfs3RCommitPrint(Fmt *fmt, Nfs3RCommit *x)
{
	fmtprint(fmt, "%s\n", "Nfs3RCommit");
	fmtprint(fmt, "\t%s=", "status");
	fmtprint(fmt, "%s", nfs3StatusStr(x->status));
	fmtprint(fmt, "\n");
	fmtprint(fmt, "\t%s=", "wcc");
	nfs3WccPrint(fmt, &x->wcc);
	fmtprint(fmt, "\n");
	switch(x->status){
	case Nfs3Ok:
		fmtprint(fmt, "\t%s=", "verf");
		fmtprint(fmt, "%.*H", Nfs3WriteVerfSize, x->verf);
		fmtprint(fmt, "\n");
		break;
	}
}
uint
nfs3RCommitSize(Nfs3RCommit *x)
{
	uint a;
	USED(x);
	a = 0 + 4 + nfs3WccSize(&x->wcc);
	switch(x->status){
	case Nfs3Ok:
		a = a + Nfs3WriteVerfSize;
		break;
	}
	return a;
}
int
nfs3RCommitPack(uchar *a, uchar *ea, uchar **pa, Nfs3RCommit *x)
{
	int i;

	if(i=x->status, sunEnumPack(a, ea, &a, &i) < 0) goto Err;
	if(nfs3WccPack(a, ea, &a, &x->wcc) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunFixedOpaquePack(a, ea, &a, x->verf, Nfs3WriteVerfSize) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}
int
nfs3RCommitUnpack(uchar *a, uchar *ea, uchar **pa, Nfs3RCommit *x)
{
	int i;

	if(sunEnumUnpack(a, ea, &a, &i) < 0) goto Err; x->status = i;
	if(nfs3WccUnpack(a, ea, &a, &x->wcc) < 0) goto Err;
	switch(x->status){
	case Nfs3Ok:
		if(sunFixedOpaqueUnpack(a, ea, &a, x->verf, Nfs3WriteVerfSize) < 0) goto Err;
		break;
	}
	*pa = a;
	return 0;
Err:
	*pa = ea;
	return -1;
}

typedef int (*P)(uchar*, uchar*, uchar**, SunCall*);
typedef void (*F)(Fmt*, SunCall*);
typedef uint (*S)(SunCall*);

static SunProc proc[] = {
	(P)nfs3TNullPack, (P)nfs3TNullUnpack, (S)nfs3TNullSize, (F)nfs3TNullPrint, sizeof(Nfs3TNull),
	(P)nfs3RNullPack, (P)nfs3RNullUnpack, (S)nfs3RNullSize, (F)nfs3RNullPrint, sizeof(Nfs3RNull),
	(P)nfs3TGetattrPack, (P)nfs3TGetattrUnpack, (S)nfs3TGetattrSize, (F)nfs3TGetattrPrint, sizeof(Nfs3TGetattr),
	(P)nfs3RGetattrPack, (P)nfs3RGetattrUnpack, (S)nfs3RGetattrSize, (F)nfs3RGetattrPrint, sizeof(Nfs3RGetattr),
	(P)nfs3TSetattrPack, (P)nfs3TSetattrUnpack, (S)nfs3TSetattrSize, (F)nfs3TSetattrPrint, sizeof(Nfs3TSetattr),
	(P)nfs3RSetattrPack, (P)nfs3RSetattrUnpack, (S)nfs3RSetattrSize, (F)nfs3RSetattrPrint, sizeof(Nfs3RSetattr),
	(P)nfs3TLookupPack, (P)nfs3TLookupUnpack, (S)nfs3TLookupSize, (F)nfs3TLookupPrint, sizeof(Nfs3TLookup),
	(P)nfs3RLookupPack, (P)nfs3RLookupUnpack, (S)nfs3RLookupSize, (F)nfs3RLookupPrint, sizeof(Nfs3RLookup),
	(P)nfs3TAccessPack, (P)nfs3TAccessUnpack, (S)nfs3TAccessSize, (F)nfs3TAccessPrint, sizeof(Nfs3TAccess),
	(P)nfs3RAccessPack, (P)nfs3RAccessUnpack, (S)nfs3RAccessSize, (F)nfs3RAccessPrint, sizeof(Nfs3RAccess),
	(P)nfs3TReadlinkPack, (P)nfs3TReadlinkUnpack, (S)nfs3TReadlinkSize, (F)nfs3TReadlinkPrint, sizeof(Nfs3TReadlink),
	(P)nfs3RReadlinkPack, (P)nfs3RReadlinkUnpack, (S)nfs3RReadlinkSize, (F)nfs3RReadlinkPrint, sizeof(Nfs3RReadlink),
	(P)nfs3TReadPack, (P)nfs3TReadUnpack, (S)nfs3TReadSize, (F)nfs3TReadPrint, sizeof(Nfs3TRead),
	(P)nfs3RReadPack, (P)nfs3RReadUnpack, (S)nfs3RReadSize, (F)nfs3RReadPrint, sizeof(Nfs3RRead),
	(P)nfs3TWritePack, (P)nfs3TWriteUnpack, (S)nfs3TWriteSize, (F)nfs3TWritePrint, sizeof(Nfs3TWrite),
	(P)nfs3RWritePack, (P)nfs3RWriteUnpack, (S)nfs3RWriteSize, (F)nfs3RWritePrint, sizeof(Nfs3RWrite),
	(P)nfs3TCreatePack, (P)nfs3TCreateUnpack, (S)nfs3TCreateSize, (F)nfs3TCreatePrint, sizeof(Nfs3TCreate),
	(P)nfs3RCreatePack, (P)nfs3RCreateUnpack, (S)nfs3RCreateSize, (F)nfs3RCreatePrint, sizeof(Nfs3RCreate),
	(P)nfs3TMkdirPack, (P)nfs3TMkdirUnpack, (S)nfs3TMkdirSize, (F)nfs3TMkdirPrint, sizeof(Nfs3TMkdir),
	(P)nfs3RMkdirPack, (P)nfs3RMkdirUnpack, (S)nfs3RMkdirSize, (F)nfs3RMkdirPrint, sizeof(Nfs3RMkdir),
	(P)nfs3TSymlinkPack, (P)nfs3TSymlinkUnpack, (S)nfs3TSymlinkSize, (F)nfs3TSymlinkPrint, sizeof(Nfs3TSymlink),
	(P)nfs3RSymlinkPack, (P)nfs3RSymlinkUnpack, (S)nfs3RSymlinkSize, (F)nfs3RSymlinkPrint, sizeof(Nfs3RSymlink),
	(P)nfs3TMknodPack, (P)nfs3TMknodUnpack, (S)nfs3TMknodSize, (F)nfs3TMknodPrint, sizeof(Nfs3TMknod),
	(P)nfs3RMknodPack, (P)nfs3RMknodUnpack, (S)nfs3RMknodSize, (F)nfs3RMknodPrint, sizeof(Nfs3RMknod),
	(P)nfs3TRemovePack, (P)nfs3TRemoveUnpack, (S)nfs3TRemoveSize, (F)nfs3TRemovePrint, sizeof(Nfs3TRemove),
	(P)nfs3RRemovePack, (P)nfs3RRemoveUnpack, (S)nfs3RRemoveSize, (F)nfs3RRemovePrint, sizeof(Nfs3RRemove),
	(P)nfs3TRmdirPack, (P)nfs3TRmdirUnpack, (S)nfs3TRmdirSize, (F)nfs3TRmdirPrint, sizeof(Nfs3TRmdir),
	(P)nfs3RRmdirPack, (P)nfs3RRmdirUnpack, (S)nfs3RRmdirSize, (F)nfs3RRmdirPrint, sizeof(Nfs3RRmdir),
	(P)nfs3TRenamePack, (P)nfs3TRenameUnpack, (S)nfs3TRenameSize, (F)nfs3TRenamePrint, sizeof(Nfs3TRename),
	(P)nfs3RRenamePack, (P)nfs3RRenameUnpack, (S)nfs3RRenameSize, (F)nfs3RRenamePrint, sizeof(Nfs3RRename),
	(P)nfs3TLinkPack, (P)nfs3TLinkUnpack, (S)nfs3TLinkSize, (F)nfs3TLinkPrint, sizeof(Nfs3TLink),
	(P)nfs3RLinkPack, (P)nfs3RLinkUnpack, (S)nfs3RLinkSize, (F)nfs3RLinkPrint, sizeof(Nfs3RLink),
	(P)nfs3TReadDirPack, (P)nfs3TReadDirUnpack, (S)nfs3TReadDirSize, (F)nfs3TReadDirPrint, sizeof(Nfs3TReadDir),
	(P)nfs3RReadDirPack, (P)nfs3RReadDirUnpack, (S)nfs3RReadDirSize, (F)nfs3RReadDirPrint, sizeof(Nfs3RReadDir),
	(P)nfs3TReadDirPlusPack, (P)nfs3TReadDirPlusUnpack, (S)nfs3TReadDirPlusSize, (F)nfs3TReadDirPlusPrint, sizeof(Nfs3TReadDirPlus),
	(P)nfs3RReadDirPlusPack, (P)nfs3RReadDirPlusUnpack, (S)nfs3RReadDirPlusSize, (F)nfs3RReadDirPlusPrint, sizeof(Nfs3RReadDirPlus),
	(P)nfs3TFsStatPack, (P)nfs3TFsStatUnpack, (S)nfs3TFsStatSize, (F)nfs3TFsStatPrint, sizeof(Nfs3TFsStat),
	(P)nfs3RFsStatPack, (P)nfs3RFsStatUnpack, (S)nfs3RFsStatSize, (F)nfs3RFsStatPrint, sizeof(Nfs3RFsStat),
	(P)nfs3TFsInfoPack, (P)nfs3TFsInfoUnpack, (S)nfs3TFsInfoSize, (F)nfs3TFsInfoPrint, sizeof(Nfs3TFsInfo),
	(P)nfs3RFsInfoPack, (P)nfs3RFsInfoUnpack, (S)nfs3RFsInfoSize, (F)nfs3RFsInfoPrint, sizeof(Nfs3RFsInfo),
	(P)nfs3TPathconfPack, (P)nfs3TPathconfUnpack, (S)nfs3TPathconfSize, (F)nfs3TPathconfPrint, sizeof(Nfs3TPathconf),
	(P)nfs3RPathconfPack, (P)nfs3RPathconfUnpack, (S)nfs3RPathconfSize, (F)nfs3RPathconfPrint, sizeof(Nfs3RPathconf),
	(P)nfs3TCommitPack, (P)nfs3TCommitUnpack, (S)nfs3TCommitSize, (F)nfs3TCommitPrint, sizeof(Nfs3TCommit),
	(P)nfs3RCommitPack, (P)nfs3RCommitUnpack, (S)nfs3RCommitSize, (F)nfs3RCommitPrint, sizeof(Nfs3RCommit)
};

SunProg nfs3Prog = 
{
	Nfs3Program,
	Nfs3Version,
	proc,
	nelem(proc),
};
