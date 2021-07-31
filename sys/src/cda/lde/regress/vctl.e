.x
	VCTL
	PAL16R4
.tt	iiiiiiiiigi22333322v
.i
	CK : 1		CKIN : 2	VBCK : 3	GO : 4
	TEST- : 5	HSYNC : 6	VSTART- : 7	VEND : 8
	RESET : 9	OE- : 11
	vidle- = 18
	s0 = 17		s1 = 16		s2 = 15		s3 = 14
.o
	VCK : 19	VIDLE- : 18
	S0 : 17		S1 : 16		S2 : 15		S3 : 14
	VBLD- : 13	HBLK- : 12
	e19 = 119	e18 = 118	e13 = 113	e12 = 112
.f
	s = s0 s1 s2 s3
	slo = s0 s1 s2
	S = S0 S1 S2 S3
	asynce = e19 e18 e13 e12
.u
	CK	OE-
.e
	IDLE = 017			HBLANK = 013
	D0 = 003	D1 = 002	D2 = 000	D3 = 001
	D4 = 005	D5 = 004	D6 = 006	D7 = 007

	idle = s == IDLE		hblank = s == HBLANK

	asynce = 017

	S = ~(RESET || !GO ? IDLE :
		idle ? HSYNC ? HBLANK : IDLE :
		hblank ? !VSTART- ? D0 : HBLANK :
		slo {	D0: D1, D1: D2, D2: D3, D3: D4,
			D4: D5, D5: D6, D6: D7, : VEND ? IDLE : D0 })

	VCK = !(vidle- ? !VBCK : CKIN)

	VIDLE- = !!(idle || RESET || !GO)

	VBLD- = !!(s == D0 || !TEST-)

	HBLK- = !!(hblank)
