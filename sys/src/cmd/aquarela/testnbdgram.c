#include "headers.h"

static int
deliver(void *, NbDgram *s)
{
	SmbHeader h;
	uchar *pdata;
	ushort bytecount;
	SmbBuffer *b;
	char *errmsg;
	SmbTransaction transaction;
	int rv;
//	int x;
	char *comment, *servername;
	unsigned char opcode, updatecount, versionminor, versionmajor;
	ulong periodicity, signature, type;

	errmsg = nil;
	comment = nil;
	servername = nil;
//	nbdumpdata(s->datagram.data, s->datagram.length);
	b = smbbufferinit(s->datagram.data, s->datagram.data, s->datagram.length);
	if (!smbbuffergetandcheckheader(b, &h, SMB_COM_TRANSACTION, 0, &pdata, &bytecount, &errmsg)) {
		print("ignored: %s\n", errmsg);
		goto done;
	}
	memset(&transaction, 0, sizeof(transaction));
	rv = smbtransactiondecodeprimary(&transaction, &h, pdata, b, &errmsg);
	if (rv < 0) {
		print("transaction decode fail: %s\n", errmsg);
		goto done;
	}
	if (rv == 0) {
		print("transaction too big\n");
		goto done;
	}
/*
	print("name: %s\n", transaction.in.name);
	print("setup:");
	for (x = 0; x < transaction.in.scount; x++)
		print(" 0x%.4ux", transaction.in.setup[x]);
	print("\n");
	print("parameters:\n");
	nbdumpdata(transaction.in.parameters, transaction.in.tpcount);
	print("data:\n");
	nbdumpdata(transaction.in.data, transaction.in.tdcount);
*/
	if (strcmp(transaction.in.name, "\\MAILSLOT\\BROWSE") != 0) {
		print("not a supported mailslot\n");
		goto done;
	}
	
	if (!smbbuffergetb(b, &opcode)) {
		print("not enough data for opcode\n");
		goto done;
	}

	if (opcode != 1) {
		print("not a supported mailslot opcode %d\n", opcode);
		goto done;
	}
	
	if (!smbbuffergetb(b, &updatecount)
		|| !smbbuffergetl(b, &periodicity)
		|| !smbbuffergetstrn(b, 16, &servername)
		|| !smbbuffergetb(b, &versionmajor)
		|| !smbbuffergetb(b, &versionminor)
		|| !smbbuffergetl(b, &type)
		|| !smbbuffergetl(b, &signature)
		|| !smbbuffergetstr(b, &comment)) {
		print("mailslot parse failed\n");
		goto done;
	}
/*
 * not advisable to check this! Netgear printservers send 0x55aa
	if ((signature & 0xffff0000) != 0xaa550000) {
		print("wrong signature\n");
		goto done;
	}
*/
	print("%s: period %ludms version %d.%d type 0x%.8lux browserversion %d.%d comment %s\n",
		servername, periodicity, versionmajor, versionminor, type, (signature >> 8) & 0xff, signature & 0xff, comment);
done:
	free(errmsg);
	free(comment);
	free(servername);
	smbtransactionfree(&transaction);
	smbbufferfree(&b);
	return 1;
}

void
threadmain(int, char **)
{
	char *e;
	NbDgramSendParameters p;
	nbinit();
	smbglobalsguess(1);
	nbmknamefromstringandtype(p.to, smbglobals.primarydomain, 0x1d);
	e = nbdgramlisten(p.to, deliver, nil);
	if (e) {
		print("listen failed: %s\n", e);
		threadexitsall("net");
	}
	p.type = NbDgramDirectUnique;
	for (;;) {
		if (!smbbrowsesendhostannouncement(smbglobals.serverinfo.name, 3 * 60 * 1000,
			SV_TYPE_SERVER,
			"Testing testing", &e)) {
			print("hostannounce failed: %s\n", e);
		}
		sleep(60 * 1000);
	}
}
