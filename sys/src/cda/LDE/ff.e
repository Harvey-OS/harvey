.i
	clk
	a[4]
	load
.m	reg
	d[4]
	ck
	q[4]
.b
	x:reg
.o
	out
.e
	out = a
	x.d = load ? a : (x.q + 1)
	x.ck = clk
