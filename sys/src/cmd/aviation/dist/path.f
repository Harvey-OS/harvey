c
c main program
c
	RAD = 3.14159265359 / 180.0
c
c test case object1 is MMU
c           object2 is SBJ vor
c           object3 is Caldwell (CDW)
c results should be d1=0.00 d2=7.59
c
	call path(
     x   40.79925*RAD, 74.41528*RAD,
     x   40.58292*RAD, 74.74214*RAD,
     x   40.87511*RAD, 74.28178*RAD,
     x   d1, d2)
	print 100, d1, d2
c
c test case object1 is MMU
c           object2 is SBJ vor
c           object3 is Kupper (47N)
c results should be d1=17.10 d2=6.96
c
	call path(
     x   40.79925*RAD, 74.41528*RAD,
     x   40.58292*RAD, 74.74214*RAD,
     x   40.52500*RAD, 74.59778*RAD,
     x   d1, d2)
	print 100, d1, d2
c
c test case object1 is MMU
c           object2 is SBJ vor
c           object3 is Trenton (TTN)
c results should be d1=19.77 d2=18.67
c
	call path(
     x   40.79925*RAD, 74.41528*RAD,
     x   40.58292*RAD, 74.74214*RAD,
     x   40.27711*RAD, 74.81400*RAD,
     x   d1, d2)
	print 100, d1, d2

100	format(2f10.5)
	end

c
c subroutine path
c using a great circle route from object1 to object2
c find the point along the path of nearest approach to object3
c return great circle distance from object1 to this point (d1)
c and great circle distance from object3 to this point (d2)
c
	subroutine path(lat1, lng1, lat2, lng2, lat3, lng3, d1, d2)
	real lat1, lat2, lat3
	real lng1, lng2, lng3
	real d1, d2
	real c1, c2, c3
	real s1, s2, s3
	real c12, c13, c23
	real s13, s12, s23
	real a1, a2

	RADE = 3444.054
	c1 = sin(lat1)
	c2 = sin(lat2)
	c3 = sin(lat3)

	s1 = sqrt(1.0 - c1*c1)
	s2 = sqrt(1.0 - c2*c2)
	s3 = sqrt(1.0 - c3*c3)

	c12 = c1*c2 + s1*s2*cos(lng1 - lng2)
	c13 = c1*c3 + s1*s3*cos(lng1 - lng3)
	c23 = c2*c3 + s2*s3*cos(lng2 - lng3)
	s12 = sqrt(1.0 - c12*c12)

	if(c13 .lt. c12*c23) goto 100
	a1 = c23 - c12*c13
	s13 = sqrt(1.0 - c13*c13)
	if(a1 .lt. 0.0) goto 200
	a2 = s12 * s13
	if(a2 .gt. 1.0e-10) a1 = a1 / a2
	a1 = sqrt(1.0 - a1*a1) * s13
	a2 = sqrt(1.0 - a1*a1)
	a1 = atan2(a1, a2)
	if(a2 .gt. 1.0e-10) a2 = c13 / a2
	d1 = atan2(sqrt(1.0 - a2*a2), a2)*RADE
	d2 = a1*RADE
	return

c
c point is after object2
c return d1=distance object1 to object2
c return d2=distance object2 to object3
c
100	s23 = sqrt(1.0 - c23*c23)
	d1 = atan2(s12, c12)*RADE
	d2 = atan2(s23, c23)*RADE
	return

c
c point is before object1
c return d1=distance object1 to object1 (0.0)
c return d2=distance object1 to object3
c
200	d1 = 0.0
	d2 = atan2(s13, c13)*RADE
	return
	end
