.x
	VCTL
.i
	VCK	GO	TEST-	RESET-
	VSTART-	VSTOP
.io
	SRESET
	S0	S1	S2	S3
.o
	VBLD-
.f
	S =	S0  S1  S2  S3
	Slo =	S0  S1  S2
.e
	HBLANK = 017
	D0 = 007	D1 = 006	D2 = 004	D3 = 005
	D4 = 001	D5 = 000	D6 = 002	D7 = 003
	WAIT = 013

	hblank = (S == HBLANK)		wait = (S == WAIT)

	SRESET ~= !(!RESET- || !GO)

	S -= (~0)*SRESET
	S := (~0)*VCK

	S ~= ~(	hblank ? !VSTART- ? VSTOP ? WAIT : D0 : HBLANK :
		wait ? WAIT :
		Slo {	D0: D1, D1: D2, D2: D3, D3: D4,
			D4: D5, D5: D6, D6: D7, D7: VSTOP ? WAIT : D0})

	/*VBLD- ~= (((!RESET- || hblank) && GO) || S == D7 || !TEST-)*/

	VBLD- ~= (GO && (!RESET- || hblank || S == D7) || !TEST-)
