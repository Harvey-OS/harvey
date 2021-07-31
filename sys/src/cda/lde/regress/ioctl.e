.x
	IOCTL
	PAL16L8
.tt	iiiiiiiiigi3i422222v
.i
	A2 : 1		A3 : 2		A4 : 3		A5 : 4
	BWE- : 5	BDS- : 6	PORT : 7
	HFINTEN : 8	EFINTEN : 9	HF- : 11	EF- : 13
.o
	INT : 19
	SEL- : 18	MDWS- : 17	MDOE- : 16	ACK : 15
	e12 = 112	e13 = 113	e14 = 114	e15 = 115
	e16 = 116	e17 = 117	e18 = 118	e19 = 119
.f
	asynce = e12 e13 e14 e15 e16 e17 e18 e19
.e
	asynce = 0375

	select = (PORT && A5 == 1 && A4 == 1 && A3 == 1)

	bds = select && !BDS-

	mds = bds && A2 == 0

	SEL- = !!select

	ACK = !bds

	MDWS- = !!(mds && !BWE-)

	MDOE- = !!(mds && BWE-)

	INT = !((HFINTEN && HF-) || (EFINTEN && !EF-))
