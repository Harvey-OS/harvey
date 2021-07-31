#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <mux.h>

int
convM2S(char *ap, Fcall *f, int n)
{
	uchar *p;
	ushort mux;

	p = (uchar*)ap;
	GET_CHAR(type);
	if(f->type == Tmux) {
		_mountismux = 1;
		mux = p[0]|(p[1]<<8);
		p += 2;
		GET_CHAR(type);
	}
	else
		mux = 0;

	GET_SHORT(tag);
	f->tag = tagM2S(f->tag, mux);
	switch(f->type)
	{
	default:
		return 0;

	case Tnop:
		break;

	case Tsession:
		break;

	case Tflush:
		GET_SHORT(oldtag);
		f->oldtag = tagS2M(f->oldtag, &mux, 1);
		break;

	case Tattach:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_STRING(uname, sizeof(f->uname));
		GET_STRING(aname, sizeof(f->aname));
		GET_STRING(auth, sizeof(f->auth));
		break;

	case Tauth:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_STRING(uname, sizeof(f->uname));
		GET_STRING(chal, 8+NAMELEN);
		break;

	case Tclone:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_SHORT(newfid);
		f->newfid = mapM2S(f->newfid, mux);
		break;

	case Twalk:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_STRING(name, sizeof(f->name));
		break;

	case Topen:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_CHAR(mode);
		break;

	case Tcreate:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_STRING(name, sizeof(f->name));
		GET_LONG(perm);
		GET_CHAR(mode);
		break;

	case Tread:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		VGET_LONG(offset);
		GET_SHORT(count);
		break;

	case Twrite:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		VGET_LONG(offset);
		GET_SHORT(count);
		p++;	/* pad(1) */
		f->data = (char*)p; p += f->count;
		break;

	case Tclunk:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		break;

	case Tremove:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		break;

	case Tstat:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		break;

	case Twstat:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_STRING(stat, sizeof(f->stat));
		break;

	case Tclwalk:
		GET_SHORT(fid);
		f->fid = mapM2S(f->fid, mux);
		GET_SHORT(newfid);
		GET_STRING(name, sizeof(f->name));
		break;
	}
	if((uchar*)ap+n == p)
		return n;
	return 0;
}
