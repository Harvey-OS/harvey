struct Ureg
{
	ulong	cause;			/* trap number */
	ulong	status;			/* old status */
	ulong	cha;			/* channel address */
	ulong	chd;			/* channel data */
	ulong	chc;			/* channel control */
	union{
		ulong	pc0;
		ulong	pc;
	};
	ulong	pc1;
	ulong	pc2;
	ulong	ipc;
	ulong	ipa;
	ulong	ipb;
	ulong	q;
	ulong	alustat;
	ulong	cr;
	ulong	r64;
	ulong	sp;			/* r65 */
	ulong	r66;
	ulong	r67;
	ulong	r68;
	ulong	r69;
	ulong	r70;
	ulong	r71;
	ulong	r72;
	ulong	r73;
	ulong	r74;
	ulong	r75;
	ulong	r76;
	ulong	r77;
	ulong	r78;
	ulong	r79;
	ulong	r80;
	ulong	r81;
	ulong	r82;
	ulong	r83;
	ulong	r84;
	ulong	r85;
	ulong	r86;
	ulong	r87;
	ulong	r88;
	ulong	r89;
	ulong	r90;
	ulong	r91;
	ulong	r92;
	ulong	r93;
	ulong	r94;
	ulong	r95;
	ulong	r96;
	ulong	r97;
	ulong	r98;
	ulong	r99;
	ulong	r100;
	ulong	r101;
};
