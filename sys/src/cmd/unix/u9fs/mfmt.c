#include "u.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "libc.h"
#include "9p.h"
#include "stdio.h"


void	error(char*);

struct{
	int	type;
	char	*name;
}mname[] = {
	Tnop,		"Tnop",
	Rnop,		"Rnop",
	Tsession,	"Tsession",
	Rsession,	"Rsession",
	Terror,		"Terror",
	Rerror,		"Rerror",
	Tflush,		"Tflush",
	Rflush,		"Rflush",
	Tattach,	"Tattach",
	Rattach,	"Rattach",
	Tclone,		"Tclone",
	Rclone,		"Rclone",
	Twalk,		"Twalk",
	Rwalk,		"Rwalk",
	Topen,		"Topen",
	Ropen,		"Ropen",
	Tcreate,	"Tcreate",
	Rcreate,	"Rcreate",
	Tread,		"Tread",
	Rread,		"Rread",
	Twrite,		"Twrite",
	Rwrite,		"Rwrite",
	Tclunk,		"Tclunk",
	Rclunk,		"Rclunk",
	Tremove,	"Tremove",
	Rremove,	"Rremove",
	Tstat,		"Tstat",
	Rstat,		"Rstat",
	Twstat,		"Twstat",
	Rwstat,		"Rwstat",
	Tclwalk,	"Tclwalk",
	Rclwalk,	"Rclwalk",
	Tauth,		"Tauth",
	Rauth,		"Rauth",
	0,		0
};

char*
mfmt(Fcall *f)
{
	int i;
	char *n;
	static char buf[512];

	for(i=0; mname[i].name; i++)
		if(f->type == mname[i].type){
			strcpy(buf, mname[i].name);
			n = buf+strlen(buf);
			switch(f->type){
			case Tnop:
			case Rnop:
			case Tsession:
			case Rsession:
			case Terror:
			case Rflush:
				break;
			case Rerror:
				sprintf(n, " tag %d ename %s", f->tag, f->ename);
				break;
			case Tflush:
				sprintf(n, " tag %d oldtag %d", f->tag, f->oldtag);
				break;
			case Tattach:
				sprintf(n, " tag %d fid %d uname %s aname %s auth %s",
					f->tag, f->fid, f->uname, f->aname, f->auth);
				break;
			case Rattach:
			case Rwalk:
			case Rclwalk:
			case Ropen:
			case Rcreate:
				sprintf(n, " tag %d fid %d qid 0x%x.0x%x",
					f->tag, f->fid, f->qid.path, f->qid.vers);
				break;
			case Tclone:
				sprintf(n, " tag %d fid %d newfid %d",
					f->tag, f->fid, f->newfid);
				break;
			case Twalk:
				sprintf(n, " tag %d fid %d name %s", f->tag, f->fid, f->name);
				break;
			case Topen:
				sprintf(n, " tag %d fid %d mode 0x%x",
					f->tag, f->fid, f->mode);
				break;
			case Tcreate:
				sprintf(n, " tag %d fid %d name %s perm 0x%x mode 0x%x",
					f->tag, f->fid, f->name, f->perm, f->mode);
				break;
			case Tread:
				sprintf(n, " tag %d fid %d offset %ld count %ld",
					f->tag, f->fid, f->offset, f->count);
				break;
			case Rread:
				sprintf(n, " tag %d fid %d count %ld +data...",
					f->tag, f->fid, f->count);
				break;
			case Twrite:
				sprintf(n, " tag %d fid %d offset %ld count %ld +data...",
					f->tag, f->fid, f->offset, f->count);
				break;
			case Rwrite:
				sprintf(n, " tag %d fid %d count %ld",
					f->tag, f->fid, f->count);
				break;
			case Tclunk:
			case Rclunk:
			case Rremove:
			case Rwstat:
			case Rclone:
			case Tremove:
			case Tstat:
				sprintf(n, " tag %d fid %d", f->tag, f->fid);
				break;
			case Rstat:
			case Twstat:
				sprintf(n, " tag %d fid %d + dir", f->tag, f->fid);
				break;
			case Tclwalk:
				sprintf(n, " tag %d fid %d newfid %d name %s",
					f->tag, f->fid, f->newfid, f->name);
				break;
			case Tauth:
			case Rauth:	/* no idea */
				break;
			default:
				error("mfmt");
			}
			return buf;
		}
	sprintf(buf, "mtype 0x%x", f->type);
	return buf;
}
