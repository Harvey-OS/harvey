#include <u.h>
#include <libc.h>
#include <bio.h>

/* automatically generated; do not edit. */
typedef struct Fibhdr Fibhdr;
struct Fibhdr {
	ushort wIdent;
	ushort nFib;
	ushort nProduct;
	ushort lid;
	short pnNext;
	uchar fDot;
	uchar fGlsy;
	uchar fComplex;
	uchar fHasPic;
	uchar cQuickSaves;
	uchar fEncrypted;
	uchar fWhichTblStm;
	uchar fReadOnlyRecommended;
	uchar fWriteReservation;
	uchar fExtChar;
	uchar fLoadOverride;
	uchar fFarEast;
	uchar fCrypto;
	ushort nFibBack;
	ulong lKey;
	uchar envr;
	uchar fMac;
	uchar fEmptySpecial;
	uchar fLoadOverridePage;
	uchar fFutureSavedUndo;
	uchar fWord97Saved;
	ushort chs;
	ushort chsTables;
	long fcMin;
	long fcMac;
	ushort csw;
};
enum { bcFibhdr = 0x22 };

/* automatically generated; do not edit. */
void
readFibhdr(Fibhdr *s, uchar *v, int nv)
{
	if(nv < bcFibhdr) sysfatal("not enough data for Fibhdr");
	s->wIdent = v[0x0] | (v[0x0+1] << 8);
	s->nFib = v[0x2] | (v[0x2+1] << 8);
	s->nProduct = v[0x4] | (v[0x4+1] << 8);
	s->lid = v[0x6] | (v[0x6+1] << 8);
	s->pnNext = v[0x8] | (v[0x8+1] << 8);
	s->fDot = ((v[0xA]) & 0x1) >> 0;
	s->fGlsy = ((v[0xA]) & 0x2) >> 1;
	s->fComplex = ((v[0xA]) & 0x4) >> 2;
	s->fHasPic = ((v[0xA]) & 0x8) >> 3;
	s->cQuickSaves = ((v[0xA]) & 0x240) >> 4;
	s->fEncrypted = ((v[0xB]) & 0x1) >> 0;
	s->fWhichTblStm = ((v[0xB]) & 0x2) >> 1;
	s->fReadOnlyRecommended = ((v[0xB]) & 0x4) >> 2;
	s->fWriteReservation = ((v[0xB]) & 0x8) >> 3;
	s->fExtChar = ((v[0xB]) & 0x16) >> 4;
	s->fLoadOverride = ((v[0xB]) & 0x32) >> 5;
	s->fFarEast = ((v[0xB]) & 0x64) >> 6;
	s->fCrypto = ((v[0xB]) & 0x128) >> 7;
	s->nFibBack = v[0xC] | (v[0xC+1] << 8);
	s->lKey = v[0xE] | (v[0xE+1] << 8)| (v[0xE+2] << 16) | (v[0xE+3] << 24);
	s->envr = v[0x12];
	s->fMac = ((v[0x13]) & 0x1) >> 0;
	s->fEmptySpecial = ((v[0x13]) & 0x2) >> 1;
	s->fLoadOverridePage = ((v[0x13]) & 0x4) >> 2;
	s->fFutureSavedUndo = ((v[0x13]) & 0x8) >> 3;
	s->fWord97Saved = ((v[0x13]) & 0x16) >> 4;
	s->chs = v[0x14] | (v[0x14+1] << 8);
	s->chsTables = v[0x16] | (v[0x16+1] << 8);
	s->fcMin = v[0x18] | (v[0x18+1] << 8)| (v[0x18+2] << 16) | (v[0x18+3] << 24);
	s->fcMac = v[0x1C] | (v[0x1C+1] << 8)| (v[0x1C+2] << 16) | (v[0x1C+3] << 24);
	s->csw = v[0x20] | (v[0x20+1] << 8);
}

void
usage(void)
{
	fprint(2, "usage: wordtext /mnt/doc/WordDocument\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Biobuf *b;
	Biobuf bout;
	uchar buf[512];
	Fibhdr f;
	int i, c, n;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	Binit(&bout, 1, OWRITE);
	b = Bopen(argv[0], OREAD);
	if(b == nil) {
		fprint(2, "couldn't open file: %r\n");
		exits("word");
	}

	n = Bread(b, buf, sizeof buf);
	if(n < sizeof buf) {
		fprint(2, "short read: %r\n");
		exits("read");
	}

	readFibhdr(&f, buf, sizeof buf);
	// printFibhdr(&f);

	Bseek(b, f.fcMin, 0);

	n = f.fcMac - f.fcMin;
	for(i=0; i<n; i++) {
		c = Bgetc(b);
		if(c < 0)
			break;

		switch(c) {
		default:
			Bputc(&bout, c);
			break;

		case '\\':	Bprint(&bout, "\\");	break;	/* field escape */
		case 7:	Bprint(&bout, "\n");		break;	/* cell, row mark */
		case 9:	Bprint(&bout, "\t");		break;	/* tab */
		case 11:	Bprint(&bout, "\n");		break;	/* hard line break */
		case 12:	Bprint(&bout, "\n\n\n\n");	break;	/* page break */
		case 13:	Bprint(&bout, "\n\n");	break;	/* paragraph end */
		case 14:				break;	/* column break */
		case 19:	Bprint(&bout, "<");		break;	/* field begin */
		case 20:	Bprint(&bout, ":");		break;	/* field sep */
		case 21:	Bprint(&bout, ">");		break;	/* field end */
		case 30:	Bprint(&bout, "-");		break;	/* non-breaking hyphen */
		case 31:				break;	/* non-required hyphen */
	/*	case 45:	Bprint(&bout, "-");		break;	/* breaking hyphen */
		case 160:	Bprint(&bout, " ");		break;	/* non-breaking space */

		/*
		 *  these are only supposed to get used when special is set, but we 
		 * never see these ascii values otherwise anyway.
		 */

		/*
		 * Empirically, some documents have sections of text where
		 * every character is followed by a zero byte.  Some have sections
		 * of text where there are no zero bytes.  Still others have both
		 * types and alternate between them.  Until we parse which 
		 * characters are ``special'', page numbers lose out.
		 */
		case 0:	/* Bprint(&bout, "<pageno>"); */	break;
		case 1:	Bprint(&bout, "<picture>");	break;
		case 2:	Bprint(&bout, "<footnote>");	break;
		case 3:	Bprint(&bout, "<footnote sep>");	break;
		case 4:	Bprint(&bout, "<footnote cont>");	break;
		case 5:	Bprint(&bout, "<animation>");	break;
		case 6:	Bprint(&bout, "<lineno>");	break;
		/* case 7:	Bprint(&bout, "<hand picture>");	break; */
		case 8:	Bprint(&bout, "<drawn object>");	break;
		case 10:	Bprint(&bout, "<abbrev date>");	break;
		/* case 11:	Bprint(&bout, "<hh:mm:ss>");	break; */
		/* case 12:	Bprint(&bout, "<section no>");	break; */
		/* case 14:	Bprint(&bout, "<Thu>");	break; */
		case 15:	Bprint(&bout, "<Thursday>");	break;
		case 16:	Bprint(&bout, "<day of month>");	break;

		case 22:	Bprint(&bout, "<hour>");	break;
		case 23:	Bprint(&bout, "<hour hh>");	break;
		case 24:	Bprint(&bout, "<minute>");	break;
		case 25:	Bprint(&bout, "<minute mm>");	break;
		case 26:	Bprint(&bout, "<seconds>");	break;
		case 27:	Bprint(&bout, "<AM/PM>");	break;
		case 28:	Bprint(&bout, "<hh:mm:ss>");	break;
		case 29:	Bprint(&bout, "<date>");	break;
	/* printable ascii begins hereish */
	/*
		case 30:	Bprint(&bout, "<mm/dd/yy>");	break;
		case 33:	Bprint(&bout, "<mm>");	break;
		case 34:	Bprint(&bout, "<yyyy>");	break;
		case 35:	Bprint(&bout, "<yy>");	break;
		case 36:	Bprint(&bout, "<Feb>");	break;
		case 37:	Bprint(&bout, "<February>");	break;
		case 38:	Bprint(&bout, "<hh:mm>");	break;
		case 39:	Bprint(&bout, "<long date>");	break;
		case 41:				break; */
		}
	}
	Bprint(&bout, "\n");
}
