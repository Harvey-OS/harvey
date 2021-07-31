/*
 * bkrom
 * classifies the location of the
 * black king.
 * 0-6 manhattan distance to center
 * 7 orig square
 * 8-11 k-side
 * 12-15 q-side
 */
.x
	bkrom	74S287
.i
	wkx0 wkx1 wkx2
	wky0 wky1 wky2
	GND1 GND2 GND3
.o
	kb0 kb1 kb2 kb3
.f
	kx = wkx0 wkx1 wkx2
	ky = wky0 wky1 wky2
	kb = kb0 kb1 kb2 kb3
.u
	GND1 GND2 GND3
.e
	xd = (kx) { 3, 2, 1, 0, 0, 1, 2, 3 }
	yd = (ky) { 3, 2, 1, 0, 0, 1, 2, 3 }
	d = xd + yd
	kb =
		(ky == 6)?
			(kx) { 12, 13, d, d, d, d, 8, 9 }:
		(ky == 7)?
			(kx) { 14, 15, d, d, 7, d, 10, 11 }:
		d
