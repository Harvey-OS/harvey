#include "headers.h"

typedef struct RapTableEntry RapTableEntry;

struct RapTableEntry {
	char *name;
	SmbProcessResult (*procedure)(SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata);
};

typedef int INFOSIZEFN(ushort level, void *data);
typedef int INFOPUTFN(SmbBuffer *b, ushort level, void *data);
typedef int INFOPUTSTRINGSFN(SmbBuffer *b, ushort level, int instance, void *data);
typedef void *INFOENUMERATEFN(void *magic, int i);

typedef struct InfoMethod {
	INFOSIZEFN *size;
	INFOPUTFN *put;
	INFOPUTSTRINGSFN *putstrings;
	INFOENUMERATEFN *enumerate;
} InfoMethod;

static int
serverinfosize(ushort level, void *data)
{
	SmbServerInfo *si = data;
	switch (level) {
	case 0:
		return 16;
	case 1:
		return 26 + smbstrlen(si->remark);
	default:
		return 0;
	}
}

static int
serverinfoput(SmbBuffer *b, ushort level, void *data)
{
	SmbServerInfo *si = data;
	if (!smbbufferputstrn(b, si->name, 16, 1))
		return 0;
	if (level > 0) {
		if (!smbbufferputb(b, si->vmaj)
			|| !smbbufferputb(b, si->vmin)
			|| !smbbufferputl(b, si->stype)
			|| !smbbufferputl(b, 0))
			return 0;
	}
	if (level > 1)
		return 0;
	return 1;
}

static int
serverinfoputstrings(SmbBuffer *b, ushort level, int instance, void *data)
{
	SmbServerInfo *si = data;
	if (level == 1) {
		if (!smbbufferfixupabsolutel(b, instance * 26 + 22)
			|| !smbbufferputstring(b, nil, SMB_STRING_ASCII, si->remark))
			return 0;
	}
	return 1;
}

static void *
serverinfoenumerate(void *magic, int i)
{
	if (magic) {
		SmbServerInfo **si = magic;
		return si[i];
	}
	if (i == 0)
		return &smbglobals.serverinfo;
	return nil;
}

InfoMethod serverinfo = {
	serverinfosize,
	serverinfoput,
	serverinfoputstrings,
	serverinfoenumerate,
};

static int
shareinfosize(ushort level, void *data)
{
	SmbService *serv = data;
	switch (level) {
	case 0:
		return 13;
	case 1:
		return 20 + smbstrlen(serv->remark);
	case 2:
		return 40 + smbstrlen(serv->remark) + smbstrlen(serv->path);
	default:
		return 0;
	}
}

static int
shareinfoput(SmbBuffer *b, ushort level, void *data)
{
	SmbService *serv = data;
	if (!smbbufferputstrn(b, serv->name, 13, 0))
		return 0;
	if (level > 0) {
		if (!smbbufferputb(b, 0)
			|| !smbbufferputs(b, serv->stype)
			|| !smbbufferputl(b, 0))
			return 0;
	}
	if (level > 1) {
		if (!smbbufferputs(b, 7)
			|| !smbbufferputs(b, -1)
			|| !smbbufferputs(b, serv->ref)
			|| !smbbufferputl(b, 0)
			|| !smbbufferfill(b, 0, 10))
			return 0;
	}
	if (level > 2)
		return 0;
	return 1;
}

static int
shareinfoputstrings(SmbBuffer *b, ushort level, int instance, void *data)
{
	SmbService *serv = data;
	switch (level) {
	case 0:
		break;
	case 1:
		if (!smbbufferfixupabsolutel(b, instance * 20 + 16)
			|| !smbbufferputstring(b, nil, SMB_STRING_ASCII, serv->remark))
			return 0;
		break;
	case 2:
		if (!smbbufferfixupabsolutel(b, instance * 40 + 16)
			|| !smbbufferputstring(b, nil, SMB_STRING_ASCII, serv->remark)
			|| !smbbufferfixupabsolutel(b, instance * 40 + 26)
			|| !smbbufferputstring(b, nil, SMB_STRING_ASCII, serv->path))
			return 0;
		break;
	default:
		return 0;
	}
	return 1;
}

static void *
shareinfoenumerate(void *, int i)
{
	SmbService *serv;
	for (serv = smbservices; i-- > 0 && serv; serv = serv->next)
		;
	return serv;
}

static InfoMethod shareinfo = {
	shareinfosize,
	shareinfoput,
	shareinfoputstrings,
	shareinfoenumerate,
};

static SmbProcessResult
thingfill(SmbBuffer *outparam, SmbBuffer *outdata, InfoMethod *m, ushort level, void *magic)
{
	int sentthings, totalthings;
	int i;
	int totalbytes;

	sentthings = 0;
	totalbytes = 0;
	for (i = 0; ; i++) {
		int len;
		void *thing = (*m->enumerate)(magic, i);
		if (thing == nil)
			break;
		len = (*m->size)(level, thing);
		if (totalbytes + len <= smbbufferspace(outdata)) {
			assert((*m->put)(outdata, level, thing));
			sentthings++;
		}
		totalbytes += len;
	}
	totalthings = i;
	for (i = 0; i < sentthings; i++) {
		void *thing = (*m->enumerate)(magic, i);
		assert(thing);
		assert((*m->putstrings)(outdata, level, i, thing));
	}
	if (!smbbufferputs(outparam, sentthings < totalthings ? SMB_RAP_ERROR_MORE_DATA : SMB_RAP_NERR_SUCCESS)
		|| !smbbufferputs(outparam, 0)
		|| !smbbufferputs(outparam, totalthings)
		|| !smbbufferputs(outparam, sentthings))
		return SmbProcessResultFormat;
	return SmbProcessResultReply;
}

static SmbProcessResult
onethingfill(SmbBuffer *outparam, SmbBuffer *outdata, InfoMethod *m, ushort level, void *thing)
{
	int moredata;
	int totalbytes = (*m->size)(level, thing);
	if (totalbytes <= smbbufferspace(outdata)) {
		assert((*m->put)(outdata, level, thing));
		assert((*m->putstrings)(outdata, level, 0, thing));
		moredata = 0;
	}
	else
		moredata = 1;
	if (!smbbufferputs(outparam, moredata ? SMB_RAP_ERROR_MORE_DATA : SMB_RAP_NERR_SUCCESS)
		|| !smbbufferputs(outparam, 0)
		|| !smbbufferputs(outparam, totalbytes))
		return SmbProcessResultFormat;
	return SmbProcessResultReply;
}

static SmbProcessResult
netshareenum(SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata)
{
	ushort level;

	/* WrLeh */
	/* ushort sLevel, RCVBUF pbBuffer, RCVBUFLEN cbBuffer, ENTCOUNT pcEntriesRead, ushort *pcTotalAvail */

	if (!smbbuffergets(inparam, &level))
		return SmbProcessResultFormat;
	
	smblogprintif(smbglobals.log.rap2, "netshareenum(%lud, %lud)\n",
		level, smbbufferwritespace(outdata));

	if (level != 1)
		return SmbProcessResultFormat;

	return thingfill(outparam, outdata, &shareinfo, level, nil);
}

static SmbProcessResult
netserverenum2(SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata)
{
	ushort level, rbl;
	char *domain;
	ulong servertype;
	SmbProcessResult pr;
	SmbServerInfo *si[3];
	SmbServerInfo domainsi;
	int entries;

	/* WrLehDz
	 * ushort sLevel, RCVBUF pbBuffer, RCVBUFLEN cbBuffer, ENTCOUNT pcEntriesRead, ushort *pcTotalAvail,
	 * ulong fServerType, char *pszDomain
	*/

	if (!smbbuffergets(inparam, &level)
		|| !smbbuffergets(inparam, &rbl)
		|| !smbbuffergetl(inparam, &servertype)
		|| !smbbuffergetstr(inparam, 0, &domain)) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}
	
	smblogprintif(smbglobals.log.rap2, "netserverenum2(%lud, %lud, 0x%.8lux, %s)\n",
		level, smbbufferwritespace(outdata), servertype, domain);

	if (level > 1)
		goto fmtfail;

	if (servertype == 0xffffffff)
		servertype &= ~(SV_TYPE_DOMAIN_ENUM | SV_TYPE_LOCAL_LIST_ONLY);

	if ((servertype & SV_TYPE_LOCAL_LIST_ONLY) != 0 && (servertype & SV_TYPE_DOMAIN_ENUM) == 0) 
		servertype = SV_TYPE_ALL & ~(SV_TYPE_DOMAIN_ENUM);

	entries = 0;

	if ((servertype & SV_TYPE_SERVER) != 0
		&& (domain[0] == 0 || cistrcmp(domain, smbglobals.primarydomain) == 0)) {
		si[entries++] = &smbglobals.serverinfo;
	}

	if ((servertype & SV_TYPE_DOMAIN_ENUM) != 0) {
		/* there's only one that I know about */
		memset(&domainsi, 0, sizeof(domainsi));
		domainsi.name = smbglobals.primarydomain;
		domainsi.stype = SV_TYPE_DOMAIN_ENUM;
		si[entries++] = &domainsi;
	}
	si[entries] = 0;

	pr = thingfill(outparam, outdata, &serverinfo, level, si);
			
done:
	free(domain);
	return pr;
}

static SmbProcessResult
netsharegetinfo(SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata)
{
	char *netname;
	ushort level;
	SmbProcessResult pr;
	SmbService *serv;

	/*
	 * zWrLh
	 * char *pszNetName, ushort sLevel, RCVBUF pbBuffer, RCVBUFLEN cbBuffer, ushort *pcbTotalAvail
	*/

	if (!smbbuffergetstrinline(inparam, &netname)
		|| !smbbuffergets(inparam, &level)) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}
	
	smblogprintif(smbglobals.log.rap2, "netsharegetinfo(%s, %lud, %lud)\n",
		netname, level, smbbufferwritespace(outdata));

	if (level > 2)
		goto fmtfail;

	for (serv = smbservices; serv; serv = serv->next)
		if (cistrcmp(serv->name, netname) == 0)
			break;

	if (serv == nil) {
		smblogprint(-1, "netsharegetinfo: service %s unimplemented\n", netname);
		pr = SmbProcessResultUnimp;
		goto done;
	}

	pr = onethingfill(outparam, outdata, &shareinfo, level, serv);

done:
	return pr;
}

static SmbProcessResult
netservergetinfo(SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata)
{
	ushort level;
	SmbProcessResult pr;

	/* WrLh
	 * ushort sLevel, RCVBUF pbBuffer, RCVBUFLEN cbBuffer, ushort *pcbTotalAvail
	*/

	if (!smbbuffergets(inparam, &level)) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}
	
	smblogprintif(smbglobals.log.rap2, "netservergetinfo(%lud, %lud)\n",
		level, smbbufferwritespace(outdata));

	if (level > 1)
		goto fmtfail;

	pr = onethingfill(outparam, outdata, &shareinfo, level, &smbglobals.serverinfo);
			
done:
	return pr;
}

static SmbProcessResult
netwkstagetinfo(SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata)
{
	ushort level;
	ushort usefulbytes;
	SmbProcessResult pr;
	int moredata;

	/* WrLh
	 * ushort sLevel, RCVBUF pbBuffer, RCVBUFLEN cbBuffer, ushort *pcbTotalAvail
	*/

	if (!smbbuffergets(inparam, &level)) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}
	
	smblogprintif(smbglobals.log.rap2, "netwkstagetinfo(%lud, %lud)\n",
		level, smbbufferwritespace(outdata));

	if (level != 10)
		goto fmtfail;

	usefulbytes = 22 + smbstrlen(smbglobals.serverinfo.name) + smbstrlen(getuser())
		+ 3 * smbstrlen(smbglobals.primarydomain);

	moredata = usefulbytes > smbbufferwritespace(outdata);

	assert(smbbufferputl(outdata, 0));
	assert(smbbufferputl(outdata, 0));
	assert(smbbufferputl(outdata, 0));
	assert(smbbufferputb(outdata, smbglobals.serverinfo.vmaj));
	assert(smbbufferputb(outdata, smbglobals.serverinfo.vmin));
	assert(smbbufferputl(outdata, 0));
	assert(smbbufferputl(outdata, 0));
	assert(smbbufferfixupabsolutel(outdata, 0));
	assert(smbbufferputstring(outdata, nil, SMB_STRING_ASCII, smbglobals.serverinfo.name));
	assert(smbbufferfixupabsolutel(outdata, 4));
	assert(smbbufferputstring(outdata, nil, SMB_STRING_ASCII, getuser()));
	assert(smbbufferfixupabsolutel(outdata, 8));
	assert(smbbufferputstring(outdata, nil, SMB_STRING_ASCII, smbglobals.primarydomain));
	assert(smbbufferfixupabsolutel(outdata, 14));
	assert(smbbufferputstring(outdata, nil, SMB_STRING_ASCII, smbglobals.primarydomain));
	assert(smbbufferfixupabsolutel(outdata, 18));
	assert(smbbufferputstring(outdata, nil, SMB_STRING_ASCII, smbglobals.primarydomain));

	if (!smbbufferputs(outparam, moredata ? SMB_RAP_ERROR_MORE_DATA : SMB_RAP_NERR_SUCCESS)
		|| !smbbufferputs(outparam, 0)
		|| !smbbufferputs(outparam, usefulbytes)) {
		pr = SmbProcessResultFormat;
		goto done;
	}
	
	pr = SmbProcessResultReply;
			
done:
	return pr;
}

static RapTableEntry raptable[] = {
[RapNetShareGetInfo] { "NetShareGetInfo", netsharegetinfo },
[RapNetShareEnum] { "NetShareEnum", netshareenum },
[RapNetServerGetInfo] {"NetServerGetInfo", netservergetinfo },
[RapNetWkstaGetInfo] { "NetWkstaGetInfo", netwkstagetinfo },
[RapNetServerEnum2] { "NetServerEnum2", netserverenum2 },
};

SmbProcessResult
smbrap2(SmbSession *s)
{
	char *pstring;
	char *dstring;
	ushort pno;
	RapTableEntry *e;
	SmbProcessResult pr;
	SmbBuffer *inparam;

	inparam = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(inparam, &pno)
		|| !smbbuffergetstrinline(inparam, &pstring)
		|| !smbbuffergetstrinline(inparam, &dstring)) {
		smblogprintif(smbglobals.log.rap2, "smbrap2: not enough parameters\n");
		pr = SmbProcessResultFormat;
		goto done;
	}
	if (pno > nelem(raptable) || raptable[pno].name == nil) {
		smblogprint(-1, "smbrap2: unsupported procedure %ud\n", pno);
		pr = SmbProcessResultUnimp;
		goto done;
	}
	e = raptable + pno;
	pr = (*e->procedure)(inparam, s->transaction.out.parameters, s->transaction.out.data);
done:
	smbbufferfree(&inparam);
	return pr;
}
