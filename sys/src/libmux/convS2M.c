#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <mux.h>

int
convS2M(Fcall *f, char *ap)
{
	uchar *p;
	ushort fid, tag, mux;

	tag = tagS2M(f->tag, &mux, 0);

	p = (uchar*)ap;
	if(_mountismux) {
		*p++ = Tmux;
		PUT_2DATA(mux);
	}

	PUT_CHAR(type);
	PUT_2DATA(tag);
	switch(f->type)
	{
	default:
		return 0;

	case Rnop:
		break;

	case Rsession:
		break;

	case Rerror:
		PUT_STRING(ename, sizeof(f->ename));
		break;

	case Rflush:
		break;

	case Rattach:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_LONG(qid.path);
		PUT_LONG(qid.vers);
		break;

	case Rauth:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_STRING(chal, 8+8+7+7);
		break;

	case Rclone:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		break;

	case Rwalk:
	case Rclwalk:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_LONG(qid.path);
		PUT_LONG(qid.vers);
		break;

	case Ropen:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_LONG(qid.path);
		PUT_LONG(qid.vers);
		break;

	case Rcreate:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_LONG(qid.path);
		PUT_LONG(qid.vers);
		break;

	case Rread:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_SHORT(count);
		p++;	/* pad(1) */
		PUT_STRING(data, f->count);
		break;

	case Rwrite:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_SHORT(count);
		break;

	case Rclunk:
		fid = mapS2M(f->fid, 1);
		PUT_2DATA(fid);
		break;

	case Rremove:
		fid = mapS2M(f->fid, 1);
		PUT_2DATA(fid);
		break;

	case Rstat:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		PUT_STRING(stat, sizeof(f->stat));
		break;

	case Rwstat:
		fid = mapS2M(f->fid, 0);
		PUT_2DATA(fid);
		break;
	}
	return p - (uchar*)ap;
}
