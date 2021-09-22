#include <u.h>
#include <libc.h>
#include <ip.h>
#include "icc.h"
#include "tlv.h"

Tagstr fcpstr[] = {
	{TagFlen, "n of data bytes, excl. structural info"},
	{TagFMlen, "n of data bytes, incl. structural info"},
	{TagFd, "file descriptor byte/data coding?/max record size 2B?/N records 1B || 2 B"},
	{TagFid, "file identifier"},
	{TagDFname, "DF name"},
	{TagProp, "prop info not in ber-tlv"},
	{TagSecProp, "sec attr in prof format"},
	{TagSecEFExtra, "EF id containing extra ctl info"},
	{TagShort, "short EF id"},
	{TagLcs, "LCS"},
	{TagSecAttrRef, "sec attr ref in expanded format"},
	{TagSecAttrComp, "sec attr in compact format"},
	{TagEFSecEnv, "id of EF with sec environ templates"},
	{TacSecChan, "sec chan attr"},
	{TagSecAttrData, "sec attr template for data obj"},
	{TagSecProp2, "sec info in prop format (2)"},
	{TagEFlist, "template of pair(s) of short EF + file ref"},
	{TagPropTlv, "prop info in ber-tlv"},
	{TagSectAttrExp, "sec attr in expanded format"},
	{TagCryptId, "crypto mechanism id template"},
	{0, ""},
};

/* BUG: needs more size checking */

static int
unpacksimptlv(Tlv *t, uchar *b, int bsz)
{
	uchar *s;

	s = b;
	t->kind = TSimple;

	if(bsz < 2){
		werrstr("tlv: buffer too small %d", bsz);
		return -1;
	}
	t->tag = *b++;
	t->len = *b++;
	if(t->len == 0)
		return b - s;

	if(t->len == 0xff){
		if(bsz < b - s + 2){
			werrstr("tlv: bad len 0xff");
			return -1;
		}
		t->len = nhgets(b);
		b += 2;
	}
	t->data = nil;
	if(t->len > MaxTlvData){
		werrstr("tlv: packet greater than MaxTlvData: %lud", t->len);
		return -1;
	}
	
	if(t->len + b - s < 3){
		werrstr("tlv: bad len  %lud", t->len + b - s);
		return -1;
	}
	if(t->len > 0){
		t->data = t->datapack;
		memmove(t->data, b, t->len);
		b +=  t->len;
	}
	return b - s;
}

static int
unpackbertlv(Tlv *t, uchar *b, int bsz)
{
	uchar *s;
	uchar bb;

	s = b;
	
	t->kind = TBer;

	if(bsz < 2)
		return -1;

	t->tagclass = *b++;
	t->tag = t->tagclass & TagMsk;

	t->isconstr = t->tagclass & IsConstruct;
	t->tagclass >>= 6;

	if(bsz < b - s + 1)
		return -1;
	if(t->tag == TagMsk){
		bb = *b++;
		if( !(bb&TagIs3B) )
			t->tag = bb;
		else{
			if(bsz < b - s + 2)
				return -1;
			t->tag = ( (0x7f & bb) << 7) & (0x7f & *b++);
		}
	}
	
	t->len = *b++;

	
	if(bsz < b - s + t->len - Ber2B + 1)
			return -1;
	if(t->len > MaxBer1B)
		switch(t->len){
		case Ber2B:
			t->len = *b++;
			break;
		case Ber3B:
			t->len = nhgets(b);
			b += 2;
			break;
		case Ber4B:
			t->len = (b[1]<<16)|(b[2]<<8)|(b[3]<<0);
			b += 3;
			break;
		case Ber5B:
			t->len = nhgetl(b);
			b += 4;
			break;
		}
	if(t->len > MaxTlvData){
		werrstr("tlv: packet greater than MaxTlvData: %lud", t->len);
		return -1;
	}
	
	if(bsz < b - s + t->len)
			return -1;
	if(t->len > 0){
		t->data = t->datapack;
		memmove(t->data, b, t->len);
		b +=  t->len;
	}
	return b - s;
}

static int
packsimptlv(Tlv *t, uchar *b, int bsz)
{
	uchar *s;

	s = b;
	
	if(t->kind != TSimple){
		werrstr("tried to pack simpl a dif tlv");
		return -1;
	}
	*b++ = t->tag;
	if(t->len < 0xff)
		*b++ = t->len;
	else{
		*b++ = 0xff;
		hnputs(b, t->len);
		b += 2;
	}
	if(t->len > 0){
		memmove(b, t->data, t->len);
		b +=  t->len;
	}

	if(b - s > bsz)
		return -1;
	return b - s;
}

static int
packbertlv(Tlv *t, uchar *b, int bsz)
{
	uchar *s;
	uchar bb;

	s = b;
	if(t->kind != TSimple){
		werrstr("tried to pack ber a dif tlv");
		return -1;
	}

	if(bsz < 1)
		return -1;
	if(t->tag < TagMsk)
		bb = t->tag;
	else{
		bb = TagMsk;
	}
	bb |= (t->tagclass << 6);
	if(t->isconstr)
		bb |= IsConstruct;
	*b++ = bb;

	if(t->tag >= TagMsk){
		if( t->tag <= 127 )
			*b++ = t->tag;
		else{
			*b++ = ( 1 << 7 ) | ( (0x7f & t->tag) >> 7);
			*b++ = (0x7f & t->tag);
		}
	}
	
	if(t->len < MaxBer1B){
		*b++ = t->len;
	}
	else if(t->len > MaxBer1B && t->len < 0xff){
		*b++ = Ber2B;
		*b++ = t->len;
	}
	else if(t->len > 0xff&& t->len < 0xffff){
		*b++ = Ber3B;
		hnputs(b, t->len);
	}
	else if(t->len > 0xffff&& t->len < 0xffffff){
		*b++ = Ber4B;
		b[0] = t->len>>24;
		b[1] = t->len>>16;
		b[2] = t->len>>8;
		b += 3;
	}
	else if(t->len > 0xffffff){
		*b++ = Ber5B;
		hnputl(b, t->len);
	}

	if(t->len > MaxTlvData){
		werrstr("tlv: packet greater than MaxTlvData: %lud", t->len);
		return -1;
	}
	if(t->len > 0){
		t->data = t->datapack;
		memmove(b, t->data, t->len);
		b +=  t->len;
	}
	if(b - s > bsz)
		return -1;
	return b - s;
}

static int
unpackcomptlv(Tlv *t, uchar *b, int bsz)
{
	uchar *s;

	s = b;
	t->kind = TComp;

	if(bsz < 1){
		werrstr("tlv: buffer too small %d", bsz);
		return -1;
	}
	t->tag = 0x40 | (*b>>4);
	t->len = (*b++) & 0xf;	
	if(t->len + 1 > bsz)
	{
		werrstr("bad len on comp tlv: %lud+1 < %d", t->len, bsz);
		return -1;
	}
	if(t->len > 0){
		t->data = t->datapack;
		memmove(t->data, b, t->len);
		b +=  t->len;
	}
	return b - s;
}


static int
packcomptlv(Tlv *t, uchar *b, int bsz)
{
	uchar *s;

	s = b;
	
	if(t->kind != TComp){
		werrstr("tried to pack simpl a ber tlv");
		return -1;
	}

	if((t->len & 0xf0) || (t->tag&0xf0) != 0x40){
		werrstr("tried to compact a non compactable tlv");
		return -1;
	}
	*b++ = (t->tag << 4)|(t->len);

	if( t->len+1 > bsz ){
		werrstr("bad len on comp tlv");
		return -1;
	}

	if(t->len > 0){
		memmove(b, t->data, t->len);
		b +=  t->len;
	}

	if(b - s > bsz){
		werrstr("bad len on comp tlv");
		return -1;
	}
	return b - s;
}


int
unpacktlv(Tlv *t, uchar *b, int bsz, int kind)
{
	int res;

	if(kind == TBer)
		res = unpackbertlv(t, b, bsz);
	else if(kind == TSimple)
		res = unpacksimptlv(t, b, bsz);	
	else if(kind == TComp)
		res = unpackcomptlv(t, b, bsz);
	else
		return -1;
	return res;
}



int
packtlv(Tlv *t, uchar *b, int bsz)
{
	int res;

	if(t->kind == TBer)
		res = packbertlv(t, b, bsz);
	else if(t->kind == TSimple)
		res = packsimptlv(t, b, bsz);
	else if(t->kind == TComp)
		res = packcomptlv(t, b, bsz);
	else
		return -1;
	return res;
}

static char *
strtag(Tagstr *ts, uchar tag)
{
	Tagstr *tt;

	for(tt = ts; tt->tag != 0; tt++){
		if(tt->tag == tag)
			return tt->str;
	}
	return "bad tag";
}

void
tagpr2levels(uchar *buf, int bufsz, Tagstr *ts)
{
	Tlv t, tin;
	int l, i, res;

	unpacktlv(&t, buf, bufsz, TSimple);
	fprint(2, "TAG: %lux - ", t.tag);
	for(i = 0; i< t.len; i++){
		fprint(2, "%2.2ux ", t.data[i]);
	}
	fprint(2, "\n");
	
	for(l = 0; l< t.len; l += res){
		res = unpacktlv(&tin, t.data+l, t.len-l, TSimple);
		if(res < 0)
			sysfatal("unpacking tag: %r");
		fprint(2, "	TAG: [%s] %lux, tlen,%d - ", strtag(ts, tin.tag), tin.tag, res);
		if(tin.data != nil)
			for(i = 0; i< tin.len; i++){
				fprint(2, "%2.2ux ", tin.data[i]);
			}
		fprint(2, "\n");
	}

}
