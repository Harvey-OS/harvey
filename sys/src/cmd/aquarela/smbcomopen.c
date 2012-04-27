#include "headers.h"

static void
smblogprintattr(int cmd, ushort attr)
{
	if (attr & SMB_ATTR_READ_ONLY)
		smblogprint(cmd, " readonly");
	if (attr & SMB_ATTR_HIDDEN)
		smblogprint(cmd, " hidden");
	if (attr & SMB_ATTR_SYSTEM)
		smblogprint(cmd, " system");
	if (attr & SMB_ATTR_DIRECTORY)
		smblogprint(cmd, " directory");
	if (attr & SMB_ATTR_ARCHIVE)
		smblogprint(cmd, " archive");
}

static SmbFile *
openfile(SmbSession *s, SmbTree *t, char *path, ushort mode, ushort attr, ushort ofun, ulong createoptions, uvlong createsize,
	ushort *fidp, Dir **dp, ushort *actionp)
{
	int p9mode;
	int share;
	Dir *d = nil;
	int fd = -1;
	ushort action;
	SmbFile *f = nil;
	SmbSharedFile *sf = nil;
	char *fullpath = nil;
	int diropen = 0;

//smblogprint(-1, "%s A %r", path);
	p9mode = (mode >> SMB_OPEN_MODE_ACCESS_SHIFT) & SMB_OPEN_MODE_ACCESS_MASK;	
	share = (mode >> SMB_OPEN_MODE_SHARE_SHIFT) & SMB_OPEN_MODE_SHARE_MASK;	
	if (share == SMB_OPEN_MODE_SHARE_COMPATIBILITY) {
	badshare:
//smblogprint(-1, "%s SMB_OPEN_MODE_SHARE_COMPATIBILITY", path);
		smbseterror(s, ERRDOS, ERRbadshare);
		goto done;
	}
	smbstringprint(&fullpath, "%s%s", t->serv->path, path);
	d = dirstat(fullpath);
	if (d) {
		/* file exists */
		int ofunexist;
		if (d->mode & DMDIR) {
			if (createoptions & SMB_CO_FILE) {
				smbseterror(s, ERRDOS, ERRnoaccess);
				goto done;
			}
		}
		else if (createoptions & SMB_CO_DIRECTORY) {
			smbseterror(s, ERRDOS, ERRnoaccess);
			goto done;
		}
			
		sf = smbsharedfileget(d, p9mode, &share);
		if (sf == nil)
			goto badshare;
		action = 1;
		ofunexist = (ofun >> SMB_OFUN_EXIST_SHIFT) & SMB_OFUN_EXIST_MASK;
		if (ofunexist == SMB_OFUN_EXIST_FAIL) {
			smbseterror(s, ERRDOS, ERRfilexists);
			goto done;
		}
		else if (ofunexist == SMB_OFUN_EXIST_TRUNCATE) {
			if ((d->mode & DMDIR) || (p9mode != OWRITE && p9mode != ORDWR)) {
				smbseterror(s, ERRDOS, ERRbadaccess);
				goto done;
			}
			p9mode |= OTRUNC;
			action = 3;
		}
		else if (ofunexist != SMB_OFUN_EXIST_OPEN) {
			smbseterror(s, ERRDOS, ERRbadaccess);
			goto done;
		}
		if (d->mode & DMDIR)
			diropen = 1;
		else
			fd = open(fullpath, p9mode);
	}
	else {
		/* file does not exist */
		ulong p9attr;
		action = 3;
		if ((ofun & SMB_OFUN_NOEXIST_CREATE) == 0) {
			smbseterror(s, ERRDOS, ERRbadfile);
			goto done;
		}
		if (createsize != 0) {
			smbseterror(s, ERRDOS, ERRunsup);
			goto done;
		}
//smblogprint(-1, "creating: attr 0x%.4ux co 0x%.8lux\n", attr, createoptions);
		if (createoptions & SMB_CO_FILE) {
			attr &= SMB_ATTR_DIRECTORY;
			if (attr == 0)
				attr = SMB_ATTR_NORMAL;
		}
		else if (createoptions & SMB_CO_DIRECTORY) {
			attr &= ~SMB_ATTR_NORMAL;
			attr |= SMB_ATTR_DIRECTORY;
			p9mode = OREAD;
		}
//smblogprint(-1, "creating: before conversion attr 0x%.4ux\n", attr);
		p9attr = smbdosattr2plan9mode(attr);
//smblogprint(-1, "creating: after conversion p9attr 0%.uo\n", p9attr);
		fd = create(fullpath, p9mode, p9attr);
		if (fd >= 0) {
			d = dirfstat(fd);
			sf = smbsharedfileget(d, p9mode, &share);
			if (sf == nil) {
				close(fd);
				remove(path);
				goto badshare;
			}
		}
	}
//smblogprint(-1, "%s D %r", fullpath);
	if (!diropen && fd < 0) {
		smbseterror(s, ERRSRV, ERRaccess);
		goto done;
	}
	f = smbemalloc(sizeof(SmbFile));
	if (diropen) {
		f->ioallowed = 0;
		f->fd = -1;
	}
	else {
		f->ioallowed = 1;
		f->fd = fd;
	}
	f->name = smbestrdup(path);
	f->sf = sf;
	sf = nil;
	f->share = share;
	f->p9mode = p9mode;
	f->t = t;
	if (s->fidmap == nil)
		s->fidmap = smbidmapnew();
	*fidp = smbidmapadd(s->fidmap, f);
//smblogprint(h->command, "REPLY:\n t->id=0x%ux fid=%d path=%s\n", t->id, *fidp, path);
	smblogprintif(smbglobals.log.fids, "openfile: 0x%.4ux/0x%.4ux %s\n", t->id, *fidp, path);
	if (actionp)
		*actionp = action;
	if (dp) {
		*dp = d;
		d = nil;
	}
done:
	if (sf)
		smbsharedfileput(nil, sf, share);
	free(d);
	free(fullpath);
	return f;
}

SmbProcessResult
smbcomopenandx(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar andxcommand;
	ushort andxoffset, flags, mode, sattr, attr;
	ulong createtime;	
	ushort ofun;
	ulong createsize, timeout;
	char *path = nil;
	ulong andxoffsetfixupoffset;
	SmbProcessResult pr;
	ushort action;
	Dir *d = nil;
	SmbFile *f;
	SmbTree *t;
	ushort fid;

	if (!smbcheckwordcount("comopenandx", h, 15))
		return SmbProcessResultFormat;

	andxcommand = *pdata++;
	pdata++;
	andxoffset = smbnhgets(pdata); pdata += 2;
	flags = smbnhgets(pdata); pdata += 2;
	mode = smbnhgets(pdata); pdata += 2;
	sattr = smbnhgets(pdata); pdata += 2;
	attr = smbnhgets(pdata); pdata += 2;
	createtime = smbnhgetl(pdata); pdata += 4;
	ofun = smbnhgets(pdata); pdata += 2;
	createsize = smbnhgetl(pdata); pdata += 4;
	timeout = smbnhgetl(pdata); pdata += 4;
	pdata += 4;
	USED(pdata);
	if (!smbbuffergetstring(b, h, SMB_STRING_PATH, &path)) {
		pr = SmbProcessResultFormat;
		goto done;
	}

	smbloglock();
	smblogprint(h->command, "flags 0x%.4ux", flags);
	if (flags & SMB_OPEN_FLAGS_ADDITIONAL)
		smblogprint(h->command, " additional");
	if (flags & SMB_OPEN_FLAGS_OPLOCK)
		smblogprint(h->command, " oplock");
	if (flags & SMB_OPEN_FLAGS_OPBATCH)
		smblogprint(h->command, " opbatch");
	smblogprint(h->command, "\n");
	smblogprint(h->command, "mode 0x%.4ux", mode);
	switch ((mode >> SMB_OPEN_MODE_ACCESS_SHIFT) & SMB_OPEN_MODE_ACCESS_MASK) {
	case OREAD:
		smblogprint(h->command, " OREAD");
		break;
	case OWRITE:
		smblogprint(h->command, " OWRITE");
		break;
	case ORDWR:
		smblogprint(h->command, " ORDWR");
		break;
	case OEXEC:
		smblogprint(h->command, " OEXEC");
		break;
	}
	switch ((mode >> SMB_OPEN_MODE_SHARE_SHIFT) & SMB_OPEN_MODE_SHARE_MASK) {
	case SMB_OPEN_MODE_SHARE_COMPATIBILITY:
		smblogprint(h->command, " compatinility");
		break;
	case SMB_OPEN_MODE_SHARE_EXCLUSIVE:
		smblogprint(h->command, " exclusive");
		break;
	case SMB_OPEN_MODE_SHARE_DENY_WRITE:
		smblogprint(h->command, " deny write");
		break;
	case SMB_OPEN_MODE_SHARE_DENY_READOREXEC:
		smblogprint(h->command, " deny readorxec");
		break;
	case SMB_OPEN_MODE_SHARE_DENY_NONE:
		smblogprint(h->command, " deny none");
		break;
	}
	if (mode & SMB_OPEN_MODE_WRITE_THROUGH)
		smblogprint(h->command, " write through");
	smblogprint(h->command, "\n");
	smblogprint(h->command, "sattr 0x%.4ux", sattr);
	smblogprintattr(h->command, sattr);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "attr 0x%.4ux", attr);
	smblogprintattr(h->command, attr);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "createtime 0x%.8lux\n", createtime);
	smblogprint(h->command, "ofun 0x%.4ux", ofun);
	if (ofun & SMB_OFUN_NOEXIST_CREATE)
		smblogprint(h->command, " noexistscreate");
	else
		smblogprint(h->command, " noexistfail");
	switch ((ofun >> SMB_OFUN_EXIST_SHIFT) & SMB_OFUN_EXIST_MASK) {
	case SMB_OFUN_EXIST_FAIL:
		smblogprint(h->command, " existfail");
		break;
	case SMB_OFUN_EXIST_OPEN:
		smblogprint(h->command, " existopen");
		break;
	case SMB_OFUN_EXIST_TRUNCATE:
		smblogprint(h->command, " existtruncate");
		break;
	}
	smblogprint(h->command, "\n");
	smblogprint(h->command, "createsize 0x%.8lux\n", createsize);
	smblogprint(h->command, "timeout 0x%.8lux\n", timeout);
	smblogprint(h->command, "path %s\n", path);
	smblogunlock();

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		goto errordone;
	}

	f = openfile(s, t, path, mode, attr, ofun, 0, createsize, &fid, &d, &action);
	if (f == nil) {
		pr = SmbProcessResultError;
		goto done;
	}
	h->wordcount = 15;
	if (!smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand, &andxoffsetfixupoffset)
		|| !smbbufferputs(s->response, fid)
		|| !smbbufferputs(s->response, smbplan9mode2dosattr(d->mode))
		|| !smbbufferputl(s->response, smbplan9time2utime(d->mtime, s->tzoff))
		|| !smbbufferputl(s->response, smbplan9length2size32(d->length))
		|| !smbbufferputs(s->response, smbplan9mode2dosattr(d->mode)) // probbaly bogus
		|| !smbbufferputs(s->response, 0)	// all files are files
		|| !smbbufferputs(s->response, 0)	// pipe state
		|| !smbbufferputs(s->response, action)
		|| !smbbufferputl(s->response, 0)	// fileID
		|| !smbbufferputs(s->response, 0)
		|| !smbbufferputs(s->response, 0)) {	// bytecount 0
		smbfileclose(s, f);
		pr = SmbProcessResultMisc;
		goto done;
	}
	if (andxcommand != SMB_COM_NO_ANDX_COMMAND)
		pr = smbchaincommand(s, h, andxoffsetfixupoffset, andxcommand, andxoffset, b);
	else
		pr = SmbProcessResultReply;
	goto done;	
errordone:
	pr = SmbProcessResultError;
done:
	free(path);
	free(d);
	return pr;
}

SmbProcessResult
smbcomopen(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar fmt;
	char *path;
	ushort mode, attr;
	SmbTree *t;
	ushort fid;
	Dir *d = nil;
	SmbFile *f;
	SmbProcessResult pr;

	if (!smbcheckwordcount("comopen", h, 2))
		return SmbProcessResultFormat;
	mode = smbnhgets(pdata);
	attr = smbnhgets(pdata + 2);
	if (!smbbuffergetb(b, &fmt)
		|| fmt != 4
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &path)) {
		pr = SmbProcessResultFormat;
		goto done;
	}
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
	error:
		pr = SmbProcessResultError;
		goto done;
	}
	f = openfile(s, t, path, mode, attr,
		SMB_OFUN_EXIST_OPEN << SMB_OFUN_EXIST_SHIFT,
		0, 0, &fid, &d, nil);
	if (f == nil)
		goto error;
	h->wordcount = 7;
	if (!smbbufferputheader(s->response, h, &s->peerinfo)
		|| !smbbufferputs(s->response, fid)
		|| !smbbufferputs(s->response, smbplan9mode2dosattr(d->mode))
		|| !smbbufferputl(s->response, smbplan9time2utime(d->mtime, s->tzoff))
		|| !smbbufferputl(s->response, smbplan9length2size32(d->length))
		|| !smbbufferputs(s->response, 2)		// lies - this should be the actual access allowed
		|| !smbbufferputs(s->response, 0))
		pr = SmbProcessResultMisc;
	else
		pr = SmbProcessResultReply;
done:
	free(path);
	free(d);
	return pr;
}


/*
   smb_com      SMBcreate       smb_com      SMBcreate
   smb_wct      3               smb_wct      1
   smb_vwv[0]   attribute       smb_vwv[0]   file handle
   smb_vwv[1]   time low        smb_bcc      0
   smb_vwv[2]   time high
   smb_bcc      min = 2
   smb_buf[]    ASCII -- 04
                file pathname
*/

SmbProcessResult
smbcomcreate(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	int ofun, attr, mode;
	long createtime;
	char *path;
	uchar fmt;
	SmbFile *f;
	SmbTree *t;
	ushort fid;
	SmbProcessResult pr;

	path = nil;
	if (!smbcheckwordcount("comcreate", h, 3))
		return SmbProcessResultFormat;

	smblogprint(h->command, "tid=%d\n", h->tid);
	attr = smbnhgets(pdata); pdata += 2;
	createtime = smbnhgetl(pdata);
	if (!smbbuffergetb(b, &fmt) || fmt != 0x04 || 
	    !smbbuffergetstring(b, h, SMB_STRING_PATH, &path)){
		pr = SmbProcessResultError;
		goto done;
	}

	smbloglock();
	smblogprint(h->command, "path %s\n", path);
	smblogprint(h->command, "attr 0x%.4ux", attr);
	smblogprintattr(h->command, attr);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "createtime 0x%.8lux\n", createtime);
	smblogunlock();

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		pr = SmbProcessResultError;
		goto done;
	}

	mode = (ORDWR<<SMB_OPEN_MODE_ACCESS_SHIFT) | // SFS: FIXME: should be OWRITE?
		(SMB_OPEN_MODE_SHARE_EXCLUSIVE<<SMB_OPEN_MODE_SHARE_SHIFT);
	ofun = SMB_OFUN_NOEXIST_CREATE|(SMB_OFUN_EXIST_FAIL<<SMB_OFUN_EXIST_SHIFT);
	f = openfile(s, t, path, mode, attr, ofun, SMB_CO_FILE, 0, &fid, nil, nil);
	if (f == nil) {
		pr = SmbProcessResultError;
		goto done;
	}

	h->wordcount = 1;		// SFS: FIXME: unsure of this constant, maybe should be 3
	if (!smbbufferputheader(s->response, h, &s->peerinfo)
		|| !smbbufferputs(s->response, fid)
		|| !smbbufferputs(s->response, 0)){	// bytecount 0
		pr = SmbProcessResultMisc;
		goto done;
	}
	pr = SmbProcessResultReply;
	goto done;

done:
	free(path);
	return pr;
}


typedef struct SmbSblut {
	char *s;
	ulong mask;
} SmbSblut;

static SmbSblut dasblut[] = {
	{ "SMB_DA_SPECIFIC_READ_DATA", SMB_DA_SPECIFIC_READ_DATA },
	{ "SMB_DA_SPECIFIC_WRITE_DATA", SMB_DA_SPECIFIC_WRITE_DATA },
	{ "SMB_DA_SPECIFIC_APPEND_DATA", SMB_DA_SPECIFIC_APPEND_DATA },
	{ "SMB_DA_SPECIFIC_READ_EA", SMB_DA_SPECIFIC_READ_EA },
	{ "SMB_DA_SPECIFIC_WRITE_EA", SMB_DA_SPECIFIC_WRITE_EA },
	{ "SMB_DA_SPECIFIC_EXECUTE", SMB_DA_SPECIFIC_EXECUTE },
	{ "SMB_DA_SPECIFIC_DELETE_CHILD", SMB_DA_SPECIFIC_DELETE_CHILD },
	{ "SMB_DA_SPECIFIC_READ_ATTRIBUTES", SMB_DA_SPECIFIC_READ_ATTRIBUTES },
	{ "SMB_DA_SPECIFIC_WRITE_ATTRIBUTES", SMB_DA_SPECIFIC_WRITE_ATTRIBUTES },
	{ "SMB_DA_STANDARD_DELETE_ACCESS", SMB_DA_STANDARD_DELETE_ACCESS },
	{ "SMB_DA_STANDARD_READ_CONTROL_ACCESS", SMB_DA_STANDARD_READ_CONTROL_ACCESS },
	{ "SMB_DA_STANDARD_WRITE_DAC_ACCESS", SMB_DA_STANDARD_WRITE_DAC_ACCESS },
	{ "SMB_DA_STANDARD_WRITE_OWNER_ACCESS", SMB_DA_STANDARD_WRITE_OWNER_ACCESS },
	{ "SMB_DA_STANDARD_SYNCHRONIZE_ACCESS", SMB_DA_STANDARD_SYNCHRONIZE_ACCESS },
	{ "SMB_DA_GENERIC_ALL_ACCESS", SMB_DA_GENERIC_ALL_ACCESS },
	{ "SMB_DA_GENERIC_EXECUTE_ACCESS", SMB_DA_GENERIC_EXECUTE_ACCESS },
	{ "SMB_DA_GENERIC_WRITE_ACCESS", SMB_DA_GENERIC_WRITE_ACCESS },
	{ "SMB_DA_GENERIC_READ_ACCESS", SMB_DA_GENERIC_READ_ACCESS },
	{ 0 }
};

static SmbSblut efasblut[] = {
	{ "SMB_ATTR_READ_ONLY", SMB_ATTR_READ_ONLY },
	{ "SMB_ATTR_HIDDEN", SMB_ATTR_HIDDEN },
	{ "SMB_ATTR_SYSTEM", SMB_ATTR_SYSTEM },
	{ "SMB_ATTR_DIRECTORY", SMB_ATTR_DIRECTORY },
	{ "SMB_ATTR_ARCHIVE", SMB_ATTR_ARCHIVE },
	{ "SMB_ATTR_NORMAL", SMB_ATTR_NORMAL },
	{ "SMB_ATTR_COMPRESSED", SMB_ATTR_COMPRESSED },
	{ "SMB_ATTR_TEMPORARY", SMB_ATTR_TEMPORARY },
	{ "SMB_ATTR_WRITETHROUGH", SMB_ATTR_WRITETHROUGH },
	{ "SMB_ATTR_NO_BUFFERING", SMB_ATTR_NO_BUFFERING },
	{ "SMB_ATTR_RANDOM_ACCESS", SMB_ATTR_RANDOM_ACCESS },
	{ 0 }
};

static SmbSblut sasblut[] = {
	{ "SMB_SA_SHARE_READ", SMB_SA_SHARE_READ },
	{ "SMB_SA_SHARE_WRITE", SMB_SA_SHARE_WRITE },
	{ "SMB_SA_SHARE_DELETE", SMB_SA_SHARE_DELETE },
	{ "SMB_SA_NO_SHARE", SMB_SA_NO_SHARE },
	{ 0 }
};

static SmbSblut cosblut[] = {
	{ "SMB_CO_DIRECTORY", SMB_CO_DIRECTORY },
	{ "SMB_CO_WRITETHROUGH", SMB_CO_WRITETHROUGH },
	{ "SMB_CO_SEQUENTIAL_ONLY", SMB_CO_SEQUENTIAL_ONLY },
	{ "SMB_CO_FILE", SMB_CO_FILE },
	{ "SMB_CO_NO_EA_KNOWLEDGE", SMB_CO_NO_EA_KNOWLEDGE },
	{ "SMB_CO_EIGHT_DOT_THREE_ONLY", SMB_CO_EIGHT_DOT_THREE_ONLY },
	{ "SMB_CO_RANDOM_ACCESS", SMB_CO_RANDOM_ACCESS },
	{ "SMB_CO_DELETE_ON_CLOSE", SMB_CO_DELETE_ON_CLOSE },
	{ 0 }
};

static SmbSlut cdslut[] = {
	{ "SMB_CD_SUPERCEDE", SMB_CD_SUPERCEDE },
	{ "SMB_CD_OPEN", SMB_CD_OPEN },
	{ "SMB_CD_CREATE", SMB_CD_CREATE },
	{ "SMB_CD_OPEN_IF", SMB_CD_OPEN_IF },
	{ "SMB_CD_OVERWRITE", SMB_CD_OVERWRITE },
	{ "SMB_CD_OVERWRITE_IF", SMB_CD_OVERWRITE_IF },
	{ 0 }
};

static void
smbsblutlogprint(uchar cmd, SmbSblut *sblut, ulong mask)
{
	while (sblut->s) {
		if (mask && (sblut->mask & mask) || (mask == 0 && sblut->mask == 0))
			smblogprint(cmd, " %s", sblut->s);
		sblut++;
	}
}

SmbProcessResult
smbcomntcreateandx(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar andxcommand;
	ushort andxoffset;
	char *path = nil;
	SmbProcessResult pr;
	ulong namelength;
	ulong flags;
	ulong rootdirectoryfid, desiredaccess;
	uvlong allocationsize;
	ulong extfileattributes, shareaccess, createdisposition, createoptions, impersonationlevel;
	uchar securityflags;
	int p9mode;
	int sharemode;
	ushort mode;
	SmbTree *t;
	ushort ofun;
	SmbFile *f;
	ushort fid;
	Dir *d = nil;
	ushort action;
	uvlong mtime;
	ulong andxoffsetfixup;

	if (!smbcheckwordcount("comntcreateandx", h, 24))
		return SmbProcessResultFormat;

	andxcommand = *pdata++;
	pdata++;
	andxoffset = smbnhgets(pdata); pdata += 2;
	pdata++;
	namelength = smbnhgets(pdata); pdata += 2;
	flags = smbnhgetl(pdata); pdata += 4;
	rootdirectoryfid = smbnhgetl(pdata); pdata += 4;
	desiredaccess = smbnhgetl(pdata); pdata += 4;
	allocationsize = smbnhgetv(pdata); pdata += 8;
	extfileattributes = smbnhgetl(pdata); pdata += 4;
	shareaccess = smbnhgetl(pdata); pdata += 4;
	createdisposition = smbnhgetl(pdata); pdata += 4;
	createoptions = smbnhgetl(pdata); pdata += 4;
	impersonationlevel = smbnhgetl(pdata); pdata += 4;
	securityflags = *pdata++;
	USED(pdata);

	if (!smbbuffergetstring(b, h, SMB_STRING_PATH, &path)) {
		pr = SmbProcessResultFormat;
		goto done;
	}

	smblogprint(h->command, "namelength %d\n", namelength);
	smblogprint(h->command, "flags 0x%.8lux\n", flags);
	smblogprint(h->command, "rootdirectoryfid %lud\n", rootdirectoryfid);
	smblogprint(h->command, "desiredaccess 0x%.8lux", desiredaccess);
	smbsblutlogprint(h->command, dasblut, desiredaccess);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "allocationsize %llud\n", allocationsize);
	smblogprint(h->command, "extfileattributes 0x%.8lux", extfileattributes);
	smbsblutlogprint(h->command, efasblut, extfileattributes);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "shareaccess 0x%.8lux", shareaccess);
	smbsblutlogprint(h->command, sasblut, shareaccess);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "createdisposition 0x%.8lux %s\n",
		createdisposition, smbrevslut(cdslut, createdisposition));
	smblogprint(h->command, "createoptions 0x%.8lux", createoptions);
	smbsblutlogprint(h->command, cosblut, createoptions);
	smblogprint(h->command, "\n");
	smblogprint(h->command, "impersonationlevel 0x%.8lux\n", impersonationlevel);
	smblogprint(h->command, "securityflags 0x%.2ux\n", securityflags);
	smblogprint(h->command, "path %s\n", path);

	if (rootdirectoryfid != 0) {
		smblogprint(-1, "smbcomntcreateandx: fid relative not implemented\n");
		goto unimp;
	}

	if (desiredaccess & SMB_DA_GENERIC_MASK)
		switch (desiredaccess & SMB_DA_GENERIC_MASK){
		case SMB_DA_GENERIC_READ_ACCESS:
			p9mode = OREAD;
			break;
		case SMB_DA_GENERIC_WRITE_ACCESS:
			p9mode = OWRITE;
			break;
		case SMB_DA_GENERIC_ALL_ACCESS:
			p9mode = ORDWR;
			break;
		case SMB_DA_GENERIC_EXECUTE_ACCESS:
			p9mode = OEXEC;
			break;
		default:
			p9mode = OREAD;
			break;
		}
	else
	if (desiredaccess & SMB_DA_SPECIFIC_READ_DATA)
		if (desiredaccess & (SMB_DA_SPECIFIC_WRITE_DATA | SMB_DA_SPECIFIC_APPEND_DATA))
			p9mode = ORDWR;
		else
			p9mode = OREAD;
	else if (desiredaccess & (SMB_DA_SPECIFIC_WRITE_DATA | SMB_DA_SPECIFIC_APPEND_DATA))
		p9mode = ORDWR;
	else
		p9mode = OREAD;

	if (shareaccess == SMB_SA_NO_SHARE)
		sharemode = SMB_OPEN_MODE_SHARE_EXCLUSIVE;
	else if (shareaccess & (SMB_SA_SHARE_READ | SMB_SA_SHARE_WRITE) ==
		(SMB_SA_SHARE_READ | SMB_SA_SHARE_WRITE))
		sharemode = SMB_OPEN_MODE_SHARE_DENY_NONE;
	else if (shareaccess & SMB_SA_SHARE_READ)
		sharemode = SMB_OPEN_MODE_SHARE_DENY_WRITE;
	else if (shareaccess & SMB_SA_SHARE_WRITE)
		sharemode = SMB_OPEN_MODE_SHARE_DENY_READOREXEC;
	else
		sharemode = SMB_OPEN_MODE_SHARE_DENY_NONE;

	mode = (sharemode << SMB_OPEN_MODE_SHARE_SHIFT) | (p9mode << SMB_OPEN_MODE_ACCESS_SHIFT);

	switch (createdisposition) {
	default:
		smblogprint(-1, "smbcomntcreateandx: createdisposition 0x%.8lux not implemented\n", createdisposition);
		goto unimp;
	case SMB_CD_OPEN:
		ofun = SMB_OFUN_EXIST_OPEN;
		break;
	case SMB_CD_CREATE:
		ofun = SMB_OFUN_EXIST_FAIL | SMB_OFUN_NOEXIST_CREATE;
		break;
	case SMB_CD_OPEN_IF:
		ofun = SMB_OFUN_EXIST_OPEN | SMB_OFUN_NOEXIST_CREATE;
		break;
	case SMB_CD_OVERWRITE:
		ofun = SMB_OFUN_EXIST_TRUNCATE;
		break;
	case SMB_CD_OVERWRITE_IF:
		ofun = SMB_OFUN_EXIST_TRUNCATE | SMB_OFUN_NOEXIST_CREATE;
		break;
	}

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}

	f = openfile(s, t, path, mode, extfileattributes, ofun, createoptions, allocationsize, &fid, &d, &action);

	if (f == nil) {
		pr = SmbProcessResultError;
		goto done;
	}

	h->wordcount = 42;
	mtime =  smbplan9time2time(d->mtime);
	if (!smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand, &andxoffsetfixup)
		|| !smbbufferputb(s->response, 0)		// oplocks? pah
		|| !smbbufferputs(s->response, fid)
		|| !smbbufferputl(s->response, action)
		|| !smbbufferputv(s->response, mtime)
		|| !smbbufferputv(s->response, smbplan9time2time(d->atime))
		|| !smbbufferputv(s->response, mtime)
		|| !smbbufferputv(s->response, mtime)
		|| !smbbufferputl(s->response, smbplan9mode2dosattr(d->mode))
		|| !smbbufferputv(s->response, smbl2roundupvlong(d->length, smbglobals.l2allocationsize))
		|| !smbbufferputv(s->response, d->length)
		|| !smbbufferputbytes(s->response, nil, 4)
		|| !smbbufferputb(s->response, (d->qid.type & QTDIR) != 0)
		|| !smbbufferputbytes(s->response, nil, 8)
		|| !smbbufferputs(s->response, 0)) {
		pr = SmbProcessResultMisc;
		goto done;
	}

	if (andxcommand != SMB_COM_NO_ANDX_COMMAND)
		pr = smbchaincommand(s, h, andxoffsetfixup, andxcommand, andxoffset, b);
	else
		pr = SmbProcessResultReply;

	goto done;

unimp:
	pr = SmbProcessResultUnimp;

done:
	free(path);
	free(d);

	return pr;
}

