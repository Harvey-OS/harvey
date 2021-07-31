#include "stdinc.h"

#include "9.h"

int
authRead(Fid* afid, void* data, int count)
{
	AuthInfo *ai;
	AuthRpc *rpc;

	if((rpc = afid->rpc) == nil)
		return -1;

	switch(auth_rpc(rpc, "read", nil, 0)){
	default:
		return -1;
	case ARdone:
		if((ai = auth_getinfo(rpc)) == nil)
			break;
		if(ai->cuid == nil || *ai->cuid == '\0'){
			auth_freeAI(ai);
			break;
		}
		assert(afid->cuname == nil);
		afid->cuname = vtStrDup(ai->cuid);
		auth_freeAI(ai);
		if(Dflag)
			fprint(2, "authRead cuname %s\n", afid->cuname);
		assert(afid->uid == nil);
		if((afid->uid = uidByUname(afid->cuname)) == nil)
			break;
		return 0;
	case ARok:
		if(count < rpc->narg)
			break;
		memmove(data, rpc->arg, rpc->narg);
		return rpc->narg;
	case ARphase:
		break;
	}
	return -1;
}

int
authWrite(Fid* afid, void* data, int count)
{
	assert(afid->rpc != nil);
	if(auth_rpc(afid->rpc, "write", data, count) != ARok)
		return -1;
	return count;
}

int
authCheck(Fcall* t, Fid* fid, Fs* fsys)
{
	Fid *afid;
	uchar buf[1];

	/*
	 * Can't lookup with FidWlock here as there may be
	 * protocol to do. Use a separate lock to protect altering
	 * the auth information inside afid.
	 */
	if((afid = fidGet(fid->con, t->afid, 0)) == nil){
		/*
		 * If no authentication is asked for, allow
		 * "none" provided the connection has already
		 * been authenticatated.
		 */
		if(strcmp(fid->uname, unamenone) == 0 && fid->con->aok){
			if((fid->uid = uidByUname(fid->uname)) == nil)
				return 0;
			return 1;
		}

		/*
		 * The console is allowed to attach without
		 * authentication.
		 */
		if(!fid->con->isconsole)
			return 0;
		if((fid->uid = uidByUname(fid->uname)) == nil)
			return 0;
		return 1;
	}

	/*
	 * Check valid afid;
	 * check uname and aname match.
	 */
	if(!(afid->qid.type & QTAUTH)){
		fidPut(afid);
		return 0;
	}
	if(strcmp(afid->uname, fid->uname) != 0 || afid->fsys != fsys){
		fidPut(afid);
		return 0;
	}

	vtLock(afid->alock);
	if(afid->cuname == nil){
		if(authRead(afid, buf, 0) != 0 || afid->cuname == nil){
			vtUnlock(afid->alock);
			fidPut(afid);
			return 0;
		}
	}
	vtUnlock(afid->alock);

	assert(fid->uid == nil);
	if((fid->uid = uidByUname(afid->cuname)) == nil){
		fidPut(afid);
		return 0;
	}

	vtMemFree(fid->uname);
	fid->uname = vtStrDup(afid->cuname);
	fidPut(afid);

	/*
	 * Allow "none" once the connection has been authenticated.
	 */
	fid->con->aok = 1;

	return 1;
}
