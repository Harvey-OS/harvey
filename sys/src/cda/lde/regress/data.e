.x
	data
.i
	ck	byteco	si
	sa0	sa9
	mod-	data-
	D0-	D1-	D2-	D3-	D4-	D5-	D6-	D7-	D8
.io
	Q0	Q1	Q2	Q3	Q4	Q5	Q6	Q7	Q8
	cparerr
.b
	dp-	d8-	qp
	s0	s1	s2	s3	s4	s5	s6	s7	s8
	X0	X1	Y0	Y1
.o
	so	null-	cmd-
.f
	slo =	s0  s1  s2  s3  s4	shi =	s5  s6  s7  s8  so
	dlo- =	dp- D0- D1- D2- D3-	dhi- =	D4- D5- D6- D7- d8-
	srlo =	si  s0  s1  s2  s3	srhi =	s4  s5  s6  s7  s8
	qlo =	qp  Q0  Q1  Q2  Q3	qhi =	Q4  Q5  Q6  Q7  Q8
	q =	    Q0  Q1  Q2  Q3		Q4  Q5  Q6  Q7  Q8
.e
	slo ~= ~(byteco ? ~dlo- : srlo)	shi ~= ~(byteco ? ~dhi- : srhi)
	slo := (~0)*ck			shi := (~0)*ck

	qlo ~= ~(byteco ? srlo : qlo)	qhi ~= ~(byteco ? srhi : qhi)
	qlo := (~0)*ck			qhi := (~0)*ck

	X0 ~= !(qp ^ Q0 ^ Q1 ^ Q2)
	X1 ~= !(Q3 ^ Q4 ^ Q5 ^ Q6)
	cparerr ~= !(X0^X1^Q7^Q8)

	null- ~= qp == 0 & q == 0
	cmd- ~= ((q&0770) == 0270) & !cparerr

	d8- ~= !mod- ? sa0 : !data- ? D8 : 1

	Y0 ~= !(D0- ^ D1- ^ D2- ^ D3-)
	Y1 ~= !(D4- ^ D5- ^ D6- ^ D7-)

	dp- ~= !mod- ? sa9 : (Y0 ^ Y1 ^ d8-)
