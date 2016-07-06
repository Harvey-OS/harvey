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
#include <venti.h>

int
vtfcallfmt(Fmt *f)
{
	VtFcall *t;

	t = va_arg(f->args, VtFcall*);
	if(t == nil){
		fmtprint(f, "<nil fcall>");
		return 0;
	}
	switch(t->msgtype){
	default:
		return fmtprint(f, "%c%d tag %u", "TR"[t->msgtype&1], t->msgtype>>1, t->tag);
	case VtRerror:
		return fmtprint(f, "Rerror tag %u error %s", t->tag, t->error);
	case VtTping:
		return fmtprint(f, "Tping tag %u", t->tag);
	case VtRping:
		return fmtprint(f, "Rping tag %u", t->tag);
	case VtThello:
		return fmtprint(f, "Thello tag %u vers %s uid %s strength %d crypto %d:%.*H codec %d:%.*H", t->tag,
			t->version, t->uid, t->strength, t->ncrypto, t->ncrypto, t->crypto,
			t->ncodec, t->ncodec, t->codec);
	case VtRhello:
		return fmtprint(f, "Rhello tag %u sid %s rcrypto %d rcodec %d", t->tag, t->sid, t->rcrypto, t->rcodec);
	case VtTgoodbye:
		return fmtprint(f, "Tgoodbye tag %u", t->tag);
	case VtRgoodbye:
		return fmtprint(f, "Rgoodbye tag %u", t->tag);
	case VtTauth0:
		return fmtprint(f, "Tauth0 tag %u auth %.*H", t->tag, t->nauth, t->auth);
	case VtRauth0:
		return fmtprint(f, "Rauth0 tag %u auth %.*H", t->tag, t->nauth, t->auth);
	case VtTauth1:
		return fmtprint(f, "Tauth1 tag %u auth %.*H", t->tag, t->nauth, t->auth);
	case VtRauth1:
		return fmtprint(f, "Rauth1 tag %u auth %.*H", t->tag, t->nauth, t->auth);
	case VtTread:
		return fmtprint(f, "Tread tag %u score %V blocktype %d count %d", t->tag, t->score, t->blocktype, t->count);
	case VtRread:
		return fmtprint(f, "Rread tag %u count %d", t->tag, packetsize(t->data));
	case VtTwrite:
		return fmtprint(f, "Twrite tag %u blocktype %d count %d", t->tag, t->blocktype, packetsize(t->data));
	case VtRwrite:
		return fmtprint(f, "Rwrite tag %u score %V", t->tag, t->score);
	case VtTsync:
		return fmtprint(f, "Tsync tag %u", t->tag);
	case VtRsync:
		return fmtprint(f, "Rsync tag %u", t->tag);
	}
}
