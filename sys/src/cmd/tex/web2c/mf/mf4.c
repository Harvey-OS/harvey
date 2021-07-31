#define EXTERN extern
#include "mfd.h"

void diaground ( ) 
{halfword p, q, pp  ; 
  scaled b, a, bb, aa, d, c, dd, cc  ; 
  scaled penedge  ; 
  fraction alpha, beta  ; 
  scaled nexta  ; 
  boolean allsafe  ; 
  integer k  ; 
  scaled firstx, firsty  ; 
  p = curspec ; 
  curroundingptr = 0 ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    if ( mem [ p ] .hhfield .b1 != mem [ q ] .hhfield .b1 ) 
    {
      if ( mem [ q ] .hhfield .b1 > 4 ) 
      b = - (integer) mem [ q + 1 ] .cint ; 
      else b = mem [ q + 1 ] .cint ; 
      if ( abs ( mem [ q ] .hhfield .b1 - mem [ p ] .hhfield .b1 ) == 4 ) 
      if ( ( abs ( mem [ q + 1 ] .cint - mem [ q + 5 ] .cint ) < 655 ) || ( 
      abs ( mem [ q + 1 ] .cint + mem [ q + 3 ] .cint ) < 655 ) ) 
      {
	if ( curpen == 3 ) 
	penedge = 0 ; 
	else if ( curpathtype == 0 ) 
	switch ( mem [ q ] .hhfield .b1 ) 
	{case 1 : 
	case 5 : 
	  penedge = compromise ( mem [ mem [ mem [ curpen + 1 ] .hhfield .v.RH 
	  ] .hhfield .lhfield + 1 ] .cint , - (integer) mem [ mem [ mem [ 
	  curpen + 4 ] .hhfield .v.RH ] .hhfield .lhfield + 1 ] .cint ) ; 
	  break ; 
	case 4 : 
	case 8 : 
	  penedge = - (integer) compromise ( mem [ mem [ mem [ curpen + 1 ] 
	  .hhfield .v.RH ] .hhfield .lhfield + 1 ] .cint , - (integer) mem [ 
	  mem [ mem [ curpen + 4 ] .hhfield .v.RH ] .hhfield .lhfield + 1 ] 
	  .cint ) ; 
	  break ; 
	case 6 : 
	case 2 : 
	  penedge = compromise ( mem [ mem [ mem [ curpen + 2 ] .hhfield .v.RH 
	  ] .hhfield .lhfield + 1 ] .cint , - (integer) mem [ mem [ mem [ 
	  curpen + 3 ] .hhfield .v.RH ] .hhfield .lhfield + 1 ] .cint ) ; 
	  break ; 
	case 7 : 
	case 3 : 
	  penedge = - (integer) compromise ( mem [ mem [ mem [ curpen + 2 ] 
	  .hhfield .v.RH ] .hhfield .lhfield + 1 ] .cint , - (integer) mem [ 
	  mem [ mem [ curpen + 3 ] .hhfield .v.RH ] .hhfield .lhfield + 1 ] 
	  .cint ) ; 
	  break ; 
	} 
	else if ( mem [ q ] .hhfield .b1 <= 4 ) 
	penedge = mem [ mem [ mem [ curpen + mem [ q ] .hhfield .b1 ] .hhfield 
	.v.RH ] .hhfield .lhfield + 1 ] .cint ; 
	else penedge = - (integer) mem [ mem [ mem [ curpen + mem [ q ] 
	.hhfield .b1 ] .hhfield .v.RH ] .hhfield .lhfield + 1 ] .cint ; 
	if ( odd ( mem [ q ] .hhfield .b1 ) ) 
	a = goodval ( b , penedge + ( curgran ) / 2 ) ; 
	else a = goodval ( b - 1 , penedge + ( curgran ) / 2 ) ; 
      } 
      else a = b ; 
      else a = b ; 
      beforeandafter ( b , a , q ) ; 
    } 
    p = q ; 
  } while ( ! ( p == curspec ) ) ; 
  if ( curroundingptr > 0 ) 
  {
    p = nodetoround [ 0 ] ; 
    firstx = mem [ p + 1 ] .cint ; 
    firsty = mem [ p + 2 ] .cint ; 
    before [ curroundingptr ] = before [ 0 ] ; 
    nodetoround [ curroundingptr ] = nodetoround [ 0 ] ; 
    do {
	after [ curroundingptr ] = after [ 0 ] ; 
      allsafe = true ; 
      nexta = after [ 0 ] ; 
      {register integer for_end; k = 0 ; for_end = curroundingptr - 1 ; if ( 
      k <= for_end) do 
	{
	  a = nexta ; 
	  b = before [ k ] ; 
	  nexta = after [ k + 1 ] ; 
	  aa = nexta ; 
	  bb = before [ k + 1 ] ; 
	  if ( ( a != b ) || ( aa != bb ) ) 
	  {
	    p = nodetoround [ k ] ; 
	    pp = nodetoround [ k + 1 ] ; 
	    if ( aa == bb ) 
	    {
	      if ( pp == nodetoround [ 0 ] ) 
	      unskew ( firstx , firsty , mem [ pp ] .hhfield .b1 ) ; 
	      else unskew ( mem [ pp + 1 ] .cint , mem [ pp + 2 ] .cint , mem 
	      [ pp ] .hhfield .b1 ) ; 
	      skew ( curx , cury , mem [ p ] .hhfield .b1 ) ; 
	      bb = curx ; 
	      aa = bb ; 
	      dd = cury ; 
	      cc = dd ; 
	      if ( mem [ p ] .hhfield .b1 > 4 ) 
	      {
		b = - (integer) b ; 
		a = - (integer) a ; 
	      } 
	    } 
	    else {
		
	      if ( mem [ p ] .hhfield .b1 > 4 ) 
	      {
		bb = - (integer) bb ; 
		aa = - (integer) aa ; 
		b = - (integer) b ; 
		a = - (integer) a ; 
	      } 
	      if ( pp == nodetoround [ 0 ] ) 
	      dd = firsty - bb ; 
	      else dd = mem [ pp + 2 ] .cint - bb ; 
	      if ( odd ( aa - bb ) ) 
	      if ( mem [ p ] .hhfield .b1 > 4 ) 
	      cc = dd - ( aa - bb + 1 ) / 2 ; 
	      else cc = dd - ( aa - bb - 1 ) / 2 ; 
	      else cc = dd - ( aa - bb ) / 2 ; 
	    } 
	    d = mem [ p + 2 ] .cint ; 
	    if ( odd ( a - b ) ) 
	    if ( mem [ p ] .hhfield .b1 > 4 ) 
	    c = d - ( a - b - 1 ) / 2 ; 
	    else c = d - ( a - b + 1 ) / 2 ; 
	    else c = d - ( a - b ) / 2 ; 
	    if ( ( aa < a ) || ( cc < c ) || ( aa - a > 2 * ( bb - b ) ) || ( 
	    cc - c > 2 * ( dd - d ) ) ) 
	    {
	      allsafe = false ; 
	      after [ k ] = before [ k ] ; 
	      if ( k == curroundingptr - 1 ) 
	      after [ 0 ] = before [ 0 ] ; 
	      else after [ k + 1 ] = before [ k + 1 ] ; 
	    } 
	  } 
	} 
      while ( k++ < for_end ) ; } 
    } while ( ! ( allsafe ) ) ; 
    {register integer for_end; k = 0 ; for_end = curroundingptr - 1 ; if ( k 
    <= for_end) do 
      {
	a = after [ k ] ; 
	b = before [ k ] ; 
	aa = after [ k + 1 ] ; 
	bb = before [ k + 1 ] ; 
	if ( ( a != b ) || ( aa != bb ) ) 
	{
	  p = nodetoround [ k ] ; 
	  pp = nodetoround [ k + 1 ] ; 
	  if ( aa == bb ) 
	  {
	    if ( pp == nodetoround [ 0 ] ) 
	    unskew ( firstx , firsty , mem [ pp ] .hhfield .b1 ) ; 
	    else unskew ( mem [ pp + 1 ] .cint , mem [ pp + 2 ] .cint , mem [ 
	    pp ] .hhfield .b1 ) ; 
	    skew ( curx , cury , mem [ p ] .hhfield .b1 ) ; 
	    bb = curx ; 
	    aa = bb ; 
	    dd = cury ; 
	    cc = dd ; 
	    if ( mem [ p ] .hhfield .b1 > 4 ) 
	    {
	      b = - (integer) b ; 
	      a = - (integer) a ; 
	    } 
	  } 
	  else {
	      
	    if ( mem [ p ] .hhfield .b1 > 4 ) 
	    {
	      bb = - (integer) bb ; 
	      aa = - (integer) aa ; 
	      b = - (integer) b ; 
	      a = - (integer) a ; 
	    } 
	    if ( pp == nodetoround [ 0 ] ) 
	    dd = firsty - bb ; 
	    else dd = mem [ pp + 2 ] .cint - bb ; 
	    if ( odd ( aa - bb ) ) 
	    if ( mem [ p ] .hhfield .b1 > 4 ) 
	    cc = dd - ( aa - bb + 1 ) / 2 ; 
	    else cc = dd - ( aa - bb - 1 ) / 2 ; 
	    else cc = dd - ( aa - bb ) / 2 ; 
	  } 
	  d = mem [ p + 2 ] .cint ; 
	  if ( odd ( a - b ) ) 
	  if ( mem [ p ] .hhfield .b1 > 4 ) 
	  c = d - ( a - b - 1 ) / 2 ; 
	  else c = d - ( a - b + 1 ) / 2 ; 
	  else c = d - ( a - b ) / 2 ; 
	  if ( b == bb ) 
	  alpha = 268435456L ; 
	  else alpha = makefraction ( aa - a , bb - b ) ; 
	  if ( d == dd ) 
	  beta = 268435456L ; 
	  else beta = makefraction ( cc - c , dd - d ) ; 
	  do {
	      mem [ p + 1 ] .cint = takefraction ( alpha , mem [ p + 1 ] 
	    .cint - b ) + a ; 
	    mem [ p + 2 ] .cint = takefraction ( beta , mem [ p + 2 ] .cint - 
	    d ) + c ; 
	    mem [ p + 5 ] .cint = takefraction ( alpha , mem [ p + 5 ] .cint - 
	    b ) + a ; 
	    mem [ p + 6 ] .cint = takefraction ( beta , mem [ p + 6 ] .cint - 
	    d ) + c ; 
	    p = mem [ p ] .hhfield .v.RH ; 
	    mem [ p + 3 ] .cint = takefraction ( alpha , mem [ p + 3 ] .cint - 
	    b ) + a ; 
	    mem [ p + 4 ] .cint = takefraction ( beta , mem [ p + 4 ] .cint - 
	    d ) + c ; 
	  } while ( ! ( p == pp ) ) ; 
	} 
      } 
    while ( k++ < for_end ) ; } 
  } 
} 
void znewboundary ( p , octant ) 
halfword p ; 
smallnumber octant ; 
{halfword q, r  ; 
  q = mem [ p ] .hhfield .v.RH ; 
  r = getnode ( 7 ) ; 
  mem [ r ] .hhfield .v.RH = q ; 
  mem [ p ] .hhfield .v.RH = r ; 
  mem [ r ] .hhfield .b0 = mem [ q ] .hhfield .b0 ; 
  mem [ r + 3 ] .cint = mem [ q + 3 ] .cint ; 
  mem [ r + 4 ] .cint = mem [ q + 4 ] .cint ; 
  mem [ r ] .hhfield .b1 = 0 ; 
  mem [ q ] .hhfield .b0 = 0 ; 
  mem [ r + 5 ] .cint = octant ; 
  mem [ q + 3 ] .cint = mem [ q ] .hhfield .b1 ; 
  unskew ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint , mem [ q ] .hhfield .b1 
  ) ; 
  skew ( curx , cury , octant ) ; 
  mem [ r + 1 ] .cint = curx ; 
  mem [ r + 2 ] .cint = cury ; 
} 
halfword zmakespec ( h , safetymargin , tracing ) 
halfword h ; 
scaled safetymargin ; 
integer tracing ; 
{/* 22 30 */ register halfword Result; halfword p, q, r, s  ; 
  integer k  ; 
  boolean chopped  ; 
  smallnumber o1, o2  ; 
  boolean clockwise  ; 
  integer dx1, dy1, dx2, dy2  ; 
  integer dmax, del  ; 
  curspec = h ; 
  if ( tracing > 0 ) 
  printpath ( curspec , 558 , true ) ; 
  maxallowed = 268402687L - safetymargin ; 
  p = curspec ; 
  k = 1 ; 
  chopped = false ; 
  do {
      if ( abs ( mem [ p + 3 ] .cint ) > maxallowed ) 
    {
      chopped = true ; 
      if ( mem [ p + 3 ] .cint > 0 ) 
      mem [ p + 3 ] .cint = maxallowed ; 
      else mem [ p + 3 ] .cint = - (integer) maxallowed ; 
    } 
    if ( abs ( mem [ p + 4 ] .cint ) > maxallowed ) 
    {
      chopped = true ; 
      if ( mem [ p + 4 ] .cint > 0 ) 
      mem [ p + 4 ] .cint = maxallowed ; 
      else mem [ p + 4 ] .cint = - (integer) maxallowed ; 
    } 
    if ( abs ( mem [ p + 1 ] .cint ) > maxallowed ) 
    {
      chopped = true ; 
      if ( mem [ p + 1 ] .cint > 0 ) 
      mem [ p + 1 ] .cint = maxallowed ; 
      else mem [ p + 1 ] .cint = - (integer) maxallowed ; 
    } 
    if ( abs ( mem [ p + 2 ] .cint ) > maxallowed ) 
    {
      chopped = true ; 
      if ( mem [ p + 2 ] .cint > 0 ) 
      mem [ p + 2 ] .cint = maxallowed ; 
      else mem [ p + 2 ] .cint = - (integer) maxallowed ; 
    } 
    if ( abs ( mem [ p + 5 ] .cint ) > maxallowed ) 
    {
      chopped = true ; 
      if ( mem [ p + 5 ] .cint > 0 ) 
      mem [ p + 5 ] .cint = maxallowed ; 
      else mem [ p + 5 ] .cint = - (integer) maxallowed ; 
    } 
    if ( abs ( mem [ p + 6 ] .cint ) > maxallowed ) 
    {
      chopped = true ; 
      if ( mem [ p + 6 ] .cint > 0 ) 
      mem [ p + 6 ] .cint = maxallowed ; 
      else mem [ p + 6 ] .cint = - (integer) maxallowed ; 
    } 
    p = mem [ p ] .hhfield .v.RH ; 
    mem [ p ] .hhfield .b0 = k ; 
    if ( k < 255 ) 
    incr ( k ) ; 
    else k = 1 ; 
  } while ( ! ( p == curspec ) ) ; 
  if ( chopped ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 562 ) ; 
    } 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 563 ; 
      helpline [ 2 ] = 564 ; 
      helpline [ 1 ] = 565 ; 
      helpline [ 0 ] = 566 ; 
    } 
    putgeterror () ; 
  } 
  quadrantsubdivide () ; 
  if ( internal [ 36 ] > 0 ) 
  xyround () ; 
  octantsubdivide () ; 
  if ( internal [ 36 ] > 65536L ) 
  diaground () ; 
  p = curspec ; 
  do {
      lab22: q = mem [ p ] .hhfield .v.RH ; 
    if ( p != q ) 
    {
      if ( mem [ p + 1 ] .cint == mem [ p + 5 ] .cint ) 
      if ( mem [ p + 2 ] .cint == mem [ p + 6 ] .cint ) 
      if ( mem [ p + 1 ] .cint == mem [ q + 3 ] .cint ) 
      if ( mem [ p + 2 ] .cint == mem [ q + 4 ] .cint ) 
      {
	unskew ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint , mem [ q ] 
	.hhfield .b1 ) ; 
	skew ( curx , cury , mem [ p ] .hhfield .b1 ) ; 
	if ( mem [ p + 1 ] .cint == curx ) 
	if ( mem [ p + 2 ] .cint == cury ) 
	{
	  removecubic ( p ) ; 
	  if ( q != curspec ) 
	  goto lab22 ; 
	  curspec = p ; 
	  q = p ; 
	} 
      } 
    } 
    p = q ; 
  } while ( ! ( p == curspec ) ) ; 
  turningnumber = 0 ; 
  p = curspec ; 
  q = mem [ p ] .hhfield .v.RH ; 
  do {
      r = mem [ q ] .hhfield .v.RH ; 
    if ( ( mem [ p ] .hhfield .b1 != mem [ q ] .hhfield .b1 ) || ( q == r ) ) 
    {
      newboundary ( p , mem [ p ] .hhfield .b1 ) ; 
      s = mem [ p ] .hhfield .v.RH ; 
      o1 = octantnumber [ mem [ p ] .hhfield .b1 ] ; 
      o2 = octantnumber [ mem [ q ] .hhfield .b1 ] ; 
      switch ( o2 - o1 ) 
      {case 1 : 
      case -7 : 
      case 7 : 
      case -1 : 
	goto lab30 ; 
	break ; 
      case 2 : 
      case -6 : 
	clockwise = false ; 
	break ; 
      case 3 : 
      case -5 : 
      case 4 : 
      case -4 : 
      case 5 : 
      case -3 : 
	{
	  dx1 = mem [ s + 1 ] .cint - mem [ s + 3 ] .cint ; 
	  dy1 = mem [ s + 2 ] .cint - mem [ s + 4 ] .cint ; 
	  if ( dx1 == 0 ) 
	  if ( dy1 == 0 ) 
	  {
	    dx1 = mem [ s + 1 ] .cint - mem [ p + 5 ] .cint ; 
	    dy1 = mem [ s + 2 ] .cint - mem [ p + 6 ] .cint ; 
	    if ( dx1 == 0 ) 
	    if ( dy1 == 0 ) 
	    {
	      dx1 = mem [ s + 1 ] .cint - mem [ p + 1 ] .cint ; 
	      dy1 = mem [ s + 2 ] .cint - mem [ p + 2 ] .cint ; 
	    } 
	  } 
	  dmax = abs ( dx1 ) ; 
	  if ( abs ( dy1 ) > dmax ) 
	  dmax = abs ( dy1 ) ; 
	  while ( dmax < 268435456L ) {
	      
	    dmax = dmax + dmax ; 
	    dx1 = dx1 + dx1 ; 
	    dy1 = dy1 + dy1 ; 
	  } 
	  dx2 = mem [ q + 5 ] .cint - mem [ q + 1 ] .cint ; 
	  dy2 = mem [ q + 6 ] .cint - mem [ q + 2 ] .cint ; 
	  if ( dx2 == 0 ) 
	  if ( dy2 == 0 ) 
	  {
	    dx2 = mem [ r + 3 ] .cint - mem [ q + 1 ] .cint ; 
	    dy2 = mem [ r + 4 ] .cint - mem [ q + 2 ] .cint ; 
	    if ( dx2 == 0 ) 
	    if ( dy2 == 0 ) 
	    {
	      if ( mem [ r ] .hhfield .b1 == 0 ) 
	      {
		curx = mem [ r + 1 ] .cint ; 
		cury = mem [ r + 2 ] .cint ; 
	      } 
	      else {
		  
		unskew ( mem [ r + 1 ] .cint , mem [ r + 2 ] .cint , mem [ r ] 
		.hhfield .b1 ) ; 
		skew ( curx , cury , mem [ q ] .hhfield .b1 ) ; 
	      } 
	      dx2 = curx - mem [ q + 1 ] .cint ; 
	      dy2 = cury - mem [ q + 2 ] .cint ; 
	    } 
	  } 
	  dmax = abs ( dx2 ) ; 
	  if ( abs ( dy2 ) > dmax ) 
	  dmax = abs ( dy2 ) ; 
	  while ( dmax < 268435456L ) {
	      
	    dmax = dmax + dmax ; 
	    dx2 = dx2 + dx2 ; 
	    dy2 = dy2 + dy2 ; 
	  } 
	  unskew ( dx1 , dy1 , mem [ p ] .hhfield .b1 ) ; 
	  del = pythadd ( curx , cury ) ; 
	  dx1 = makefraction ( curx , del ) ; 
	  dy1 = makefraction ( cury , del ) ; 
	  unskew ( dx2 , dy2 , mem [ q ] .hhfield .b1 ) ; 
	  del = pythadd ( curx , cury ) ; 
	  dx2 = makefraction ( curx , del ) ; 
	  dy2 = makefraction ( cury , del ) ; 
	  del = takefraction ( dx1 , dy2 ) - takefraction ( dx2 , dy1 ) ; 
	  if ( del > 4684844L ) 
	  clockwise = false ; 
	  else if ( del < -4684844L ) 
	  clockwise = true ; 
	  else clockwise = revturns ; 
	} 
	break ; 
      case 6 : 
      case -2 : 
	clockwise = true ; 
	break ; 
      case 0 : 
	clockwise = revturns ; 
	break ; 
      } 
      while ( true ) {
	  
	if ( clockwise ) 
	if ( o1 == 1 ) 
	o1 = 8 ; 
	else decr ( o1 ) ; 
	else if ( o1 == 8 ) 
	o1 = 1 ; 
	else incr ( o1 ) ; 
	if ( o1 == o2 ) 
	goto lab30 ; 
	newboundary ( s , octantcode [ o1 ] ) ; 
	s = mem [ s ] .hhfield .v.RH ; 
	mem [ s + 3 ] .cint = mem [ s + 5 ] .cint ; 
      } 
      lab30: if ( q == r ) 
      {
	q = mem [ q ] .hhfield .v.RH ; 
	r = q ; 
	p = s ; 
	mem [ s ] .hhfield .v.RH = q ; 
	mem [ q + 3 ] .cint = mem [ q + 5 ] .cint ; 
	mem [ q ] .hhfield .b0 = 0 ; 
	freenode ( curspec , 7 ) ; 
	curspec = q ; 
      } 
      p = mem [ p ] .hhfield .v.RH ; 
      do {
	  s = mem [ p ] .hhfield .v.RH ; 
	o1 = octantnumber [ mem [ p + 5 ] .cint ] ; 
	o2 = octantnumber [ mem [ s + 3 ] .cint ] ; 
	if ( abs ( o1 - o2 ) == 1 ) 
	{
	  if ( o2 < o1 ) 
	  o2 = o1 ; 
	  if ( odd ( o2 ) ) 
	  mem [ p + 6 ] .cint = 0 ; 
	  else mem [ p + 6 ] .cint = 1 ; 
	} 
	else {
	    
	  if ( o1 == 8 ) 
	  incr ( turningnumber ) ; 
	  else decr ( turningnumber ) ; 
	  mem [ p + 6 ] .cint = 0 ; 
	} 
	mem [ s + 4 ] .cint = mem [ p + 6 ] .cint ; 
	p = s ; 
      } while ( ! ( p == q ) ) ; 
    } 
    p = q ; 
    q = r ; 
  } while ( ! ( p == curspec ) ) ; 
  while ( mem [ curspec ] .hhfield .b0 != 0 ) curspec = mem [ curspec ] 
  .hhfield .v.RH ; 
  if ( tracing > 0 ) 
  if ( internal [ 36 ] <= 0 ) 
  printspec ( 559 ) ; 
  else if ( internal [ 36 ] > 65536L ) 
  printspec ( 560 ) ; 
  else printspec ( 561 ) ; 
  Result = curspec ; 
  return(Result) ; 
} 
void zendround ( x , y ) 
scaled x ; 
scaled y ; 
{y = y + 32768L - ycorr [ octant ] ; 
  x = x + y - xcorr [ octant ] ; 
  m1 = floorunscaled ( x ) ; 
  n1 = floorunscaled ( y ) ; 
  if ( x - 65536L * m1 >= y - 65536L * n1 + zcorr [ octant ] ) 
  d1 = 1 ; 
  else d1 = 0 ; 
} 
void zfillspec ( h ) 
halfword h ; 
{halfword p, q, r, s  ; 
  if ( internal [ 10 ] > 0 ) 
  beginedgetracing () ; 
  p = h ; 
  do {
      octant = mem [ p + 3 ] .cint ; 
    q = p ; 
    while ( mem [ q ] .hhfield .b1 != 0 ) q = mem [ q ] .hhfield .v.RH ; 
    if ( q != p ) 
    {
      endround ( mem [ p + 1 ] .cint , mem [ p + 2 ] .cint ) ; 
      m0 = m1 ; 
      n0 = n1 ; 
      d0 = d1 ; 
      endround ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint ) ; 
      if ( n1 - n0 >= movesize ) 
      overflow ( 539 , movesize ) ; 
      move [ 0 ] = d0 ; 
      moveptr = 0 ; 
      r = p ; 
      do {
	  s = mem [ r ] .hhfield .v.RH ; 
	makemoves ( mem [ r + 1 ] .cint , mem [ r + 5 ] .cint , mem [ s + 3 ] 
	.cint , mem [ s + 1 ] .cint , mem [ r + 2 ] .cint + 32768L , mem [ r + 
	6 ] .cint + 32768L , mem [ s + 4 ] .cint + 32768L , mem [ s + 2 ] 
	.cint + 32768L , xycorr [ octant ] , ycorr [ octant ] ) ; 
	r = s ; 
      } while ( ! ( r == q ) ) ; 
      move [ moveptr ] = move [ moveptr ] - d1 ; 
      if ( internal [ 35 ] > 0 ) 
      smoothmoves ( 0 , moveptr ) ; 
      movetoedges ( m0 , n0 , m1 , n1 ) ; 
    } 
    p = mem [ q ] .hhfield .v.RH ; 
  } while ( ! ( p == h ) ) ; 
  tossknotlist ( h ) ; 
  if ( internal [ 10 ] > 0 ) 
  endedgetracing () ; 
} 
void zdupoffset ( w ) 
halfword w ; 
{halfword r  ; 
  r = getnode ( 3 ) ; 
  mem [ r + 1 ] .cint = mem [ w + 1 ] .cint ; 
  mem [ r + 2 ] .cint = mem [ w + 2 ] .cint ; 
  mem [ r ] .hhfield .v.RH = mem [ w ] .hhfield .v.RH ; 
  mem [ mem [ w ] .hhfield .v.RH ] .hhfield .lhfield = r ; 
  mem [ r ] .hhfield .lhfield = w ; 
  mem [ w ] .hhfield .v.RH = r ; 
} 
halfword zmakepen ( h ) 
halfword h ; 
{/* 30 31 45 40 */ register halfword Result; smallnumber o, oo, k  ; 
  halfword p  ; 
  halfword q, r, s, w, hh  ; 
  integer n  ; 
  scaled dx, dy  ; 
  scaled mc  ; 
  q = h ; 
  r = mem [ q ] .hhfield .v.RH ; 
  mc = abs ( mem [ h + 1 ] .cint ) ; 
  if ( q == r ) 
  {
    hh = h ; 
    mem [ h ] .hhfield .b1 = 0 ; 
    if ( mc < abs ( mem [ h + 2 ] .cint ) ) 
    mc = abs ( mem [ h + 2 ] .cint ) ; 
  } 
  else {
      
    o = 0 ; 
    hh = 0 ; 
    while ( true ) {
	
      s = mem [ r ] .hhfield .v.RH ; 
      if ( mc < abs ( mem [ r + 1 ] .cint ) ) 
      mc = abs ( mem [ r + 1 ] .cint ) ; 
      if ( mc < abs ( mem [ r + 2 ] .cint ) ) 
      mc = abs ( mem [ r + 2 ] .cint ) ; 
      dx = mem [ r + 1 ] .cint - mem [ q + 1 ] .cint ; 
      dy = mem [ r + 2 ] .cint - mem [ q + 2 ] .cint ; 
      if ( dx == 0 ) 
      if ( dy == 0 ) 
      goto lab45 ; 
      if ( abvscd ( dx , mem [ s + 2 ] .cint - mem [ r + 2 ] .cint , dy , mem 
      [ s + 1 ] .cint - mem [ r + 1 ] .cint ) < 0 ) 
      goto lab45 ; 
      if ( dx > 0 ) 
      octant = 1 ; 
      else if ( dx == 0 ) 
      if ( dy > 0 ) 
      octant = 1 ; 
      else octant = 2 ; 
      else {
	  
	dx = - (integer) dx ; 
	octant = 2 ; 
      } 
      if ( dy < 0 ) 
      {
	dy = - (integer) dy ; 
	octant = octant + 2 ; 
      } 
      else if ( dy == 0 ) 
      if ( octant > 1 ) 
      octant = 4 ; 
      if ( dx < dy ) 
      octant = octant + 4 ; 
      mem [ q ] .hhfield .b1 = octant ; 
      oo = octantnumber [ octant ] ; 
      if ( o > oo ) 
      {
	if ( hh != 0 ) 
	goto lab45 ; 
	hh = q ; 
      } 
      o = oo ; 
      if ( ( q == h ) && ( hh != 0 ) ) 
      goto lab30 ; 
      q = r ; 
      r = s ; 
    } 
    lab30: ; 
  } 
  if ( mc >= 268402688L ) 
  goto lab45 ; 
  p = getnode ( 10 ) ; 
  q = hh ; 
  mem [ p + 9 ] .cint = mc ; 
  mem [ p ] .hhfield .lhfield = 0 ; 
  if ( mem [ q ] .hhfield .v.RH != q ) 
  mem [ p ] .hhfield .v.RH = 1 ; 
  {register integer for_end; k = 1 ; for_end = 8 ; if ( k <= for_end) do 
    {
      octant = octantcode [ k ] ; 
      n = 0 ; 
      h = p + octant ; 
      while ( true ) {
	  
	r = getnode ( 3 ) ; 
	skew ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint , octant ) ; 
	mem [ r + 1 ] .cint = curx ; 
	mem [ r + 2 ] .cint = cury ; 
	if ( n == 0 ) 
	mem [ h ] .hhfield .v.RH = r ; 
	else if ( odd ( k ) ) 
	{
	  mem [ w ] .hhfield .v.RH = r ; 
	  mem [ r ] .hhfield .lhfield = w ; 
	} 
	else {
	    
	  mem [ w ] .hhfield .lhfield = r ; 
	  mem [ r ] .hhfield .v.RH = w ; 
	} 
	w = r ; 
	if ( mem [ q ] .hhfield .b1 != octant ) 
	goto lab31 ; 
	q = mem [ q ] .hhfield .v.RH ; 
	incr ( n ) ; 
      } 
      lab31: r = mem [ h ] .hhfield .v.RH ; 
      if ( odd ( k ) ) 
      {
	mem [ w ] .hhfield .v.RH = r ; 
	mem [ r ] .hhfield .lhfield = w ; 
      } 
      else {
	  
	mem [ w ] .hhfield .lhfield = r ; 
	mem [ r ] .hhfield .v.RH = w ; 
	mem [ h ] .hhfield .v.RH = w ; 
	r = w ; 
      } 
      if ( ( mem [ r + 2 ] .cint != mem [ mem [ r ] .hhfield .v.RH + 2 ] .cint 
      ) || ( n == 0 ) ) 
      {
	dupoffset ( r ) ; 
	incr ( n ) ; 
      } 
      r = mem [ r ] .hhfield .lhfield ; 
      if ( mem [ r + 1 ] .cint != mem [ mem [ r ] .hhfield .lhfield + 1 ] 
      .cint ) 
      dupoffset ( r ) ; 
      else decr ( n ) ; 
      if ( n >= 255 ) 
      overflow ( 578 , 255 ) ; 
      mem [ h ] .hhfield .lhfield = n ; 
    } 
  while ( k++ < for_end ) ; } 
  goto lab40 ; 
  lab45: p = 3 ; 
  if ( mc >= 268402688L ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 572 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 573 ; 
      helpline [ 0 ] = 574 ; 
    } 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 575 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 576 ; 
      helpline [ 1 ] = 577 ; 
      helpline [ 0 ] = 574 ; 
    } 
  } 
  putgeterror () ; 
  lab40: if ( internal [ 6 ] > 0 ) 
  printpen ( p , 571 , true ) ; 
  Result = p ; 
  return(Result) ; 
} 
halfword ztrivialknot ( x , y ) 
scaled x ; 
scaled y ; 
{register halfword Result; halfword p  ; 
  p = getnode ( 7 ) ; 
  mem [ p ] .hhfield .b0 = 1 ; 
  mem [ p ] .hhfield .b1 = 1 ; 
  mem [ p + 1 ] .cint = x ; 
  mem [ p + 3 ] .cint = x ; 
  mem [ p + 5 ] .cint = x ; 
  mem [ p + 2 ] .cint = y ; 
  mem [ p + 4 ] .cint = y ; 
  mem [ p + 6 ] .cint = y ; 
  Result = p ; 
  return(Result) ; 
} 
halfword zmakepath ( penhead ) 
halfword penhead ; 
{register halfword Result; halfword p  ; 
  schar k  ; 
  halfword h  ; 
  integer m, n  ; 
  halfword w, ww  ; 
  p = memtop - 1 ; 
  {register integer for_end; k = 1 ; for_end = 8 ; if ( k <= for_end) do 
    {
      octant = octantcode [ k ] ; 
      h = penhead + octant ; 
      n = mem [ h ] .hhfield .lhfield ; 
      w = mem [ h ] .hhfield .v.RH ; 
      if ( ! odd ( k ) ) 
      w = mem [ w ] .hhfield .lhfield ; 
      {register integer for_end; m = 1 ; for_end = n + 1 ; if ( m <= for_end) 
      do 
	{
	  if ( odd ( k ) ) 
	  ww = mem [ w ] .hhfield .v.RH ; 
	  else ww = mem [ w ] .hhfield .lhfield ; 
	  if ( ( mem [ ww + 1 ] .cint != mem [ w + 1 ] .cint ) || ( mem [ ww + 
	  2 ] .cint != mem [ w + 2 ] .cint ) ) 
	  {
	    unskew ( mem [ ww + 1 ] .cint , mem [ ww + 2 ] .cint , octant ) ; 
	    mem [ p ] .hhfield .v.RH = trivialknot ( curx , cury ) ; 
	    p = mem [ p ] .hhfield .v.RH ; 
	  } 
	  w = ww ; 
	} 
      while ( m++ < for_end ) ; } 
    } 
  while ( k++ < for_end ) ; } 
  if ( p == memtop - 1 ) 
  {
    w = mem [ penhead + 1 ] .hhfield .v.RH ; 
    p = trivialknot ( mem [ w + 1 ] .cint + mem [ w + 2 ] .cint , mem [ w + 2 
    ] .cint ) ; 
    mem [ memtop - 1 ] .hhfield .v.RH = p ; 
  } 
  mem [ p ] .hhfield .v.RH = mem [ memtop - 1 ] .hhfield .v.RH ; 
  Result = mem [ memtop - 1 ] .hhfield .v.RH ; 
  return(Result) ; 
} 
void zfindoffset ( x , y , p ) 
scaled x ; 
scaled y ; 
halfword p ; 
{/* 30 10 */ schar octant  ; 
  schar s  ; 
  integer n  ; 
  halfword h, w, ww  ; 
  if ( x > 0 ) 
  octant = 1 ; 
  else if ( x == 0 ) 
  if ( y <= 0 ) 
  if ( y == 0 ) 
  {
    curx = 0 ; 
    cury = 0 ; 
    goto lab10 ; 
  } 
  else octant = 2 ; 
  else octant = 1 ; 
  else {
      
    x = - (integer) x ; 
    if ( y == 0 ) 
    octant = 4 ; 
    else octant = 2 ; 
  } 
  if ( y < 0 ) 
  {
    octant = octant + 2 ; 
    y = - (integer) y ; 
  } 
  if ( x >= y ) 
  x = x - y ; 
  else {
      
    octant = octant + 4 ; 
    x = y - x ; 
    y = y - x ; 
  } 
  if ( odd ( octantnumber [ octant ] ) ) 
  s = -1 ; 
  else s = 1 ; 
  h = p + octant ; 
  w = mem [ mem [ h ] .hhfield .v.RH ] .hhfield .v.RH ; 
  ww = mem [ w ] .hhfield .v.RH ; 
  n = mem [ h ] .hhfield .lhfield ; 
  while ( n > 1 ) {
      
    if ( abvscd ( x , mem [ ww + 2 ] .cint - mem [ w + 2 ] .cint , y , mem [ 
    ww + 1 ] .cint - mem [ w + 1 ] .cint ) != s ) 
    goto lab30 ; 
    w = ww ; 
    ww = mem [ w ] .hhfield .v.RH ; 
    decr ( n ) ; 
  } 
  lab30: unskew ( mem [ w + 1 ] .cint , mem [ w + 2 ] .cint , octant ) ; 
  lab10: ; 
} 
void zsplitforoffset ( p , t ) 
halfword p ; 
fraction t ; 
{halfword q  ; 
  halfword r  ; 
  q = mem [ p ] .hhfield .v.RH ; 
  splitcubic ( p , t , mem [ q + 1 ] .cint , mem [ q + 2 ] .cint ) ; 
  r = mem [ p ] .hhfield .v.RH ; 
  if ( mem [ r + 2 ] .cint < mem [ p + 2 ] .cint ) 
  mem [ r + 2 ] .cint = mem [ p + 2 ] .cint ; 
  else if ( mem [ r + 2 ] .cint > mem [ q + 2 ] .cint ) 
  mem [ r + 2 ] .cint = mem [ q + 2 ] .cint ; 
  if ( mem [ r + 1 ] .cint < mem [ p + 1 ] .cint ) 
  mem [ r + 1 ] .cint = mem [ p + 1 ] .cint ; 
  else if ( mem [ r + 1 ] .cint > mem [ q + 1 ] .cint ) 
  mem [ r + 1 ] .cint = mem [ q + 1 ] .cint ; 
} 
void zfinoffsetprep ( p , k , w , x0 , x1 , x2 , y0 , y1 , y2 , rising , n ) 
halfword p ; 
halfword k ; 
halfword w ; 
integer x0 ; 
integer x1 ; 
integer x2 ; 
integer y0 ; 
integer y1 ; 
integer y2 ; 
boolean rising ; 
integer n ; 
{/* 10 */ halfword ww  ; 
  scaled du, dv  ; 
  integer t0, t1, t2  ; 
  fraction t  ; 
  fraction s  ; 
  integer v  ; 
  while ( true ) {
      
    mem [ p ] .hhfield .b1 = k ; 
    if ( rising ) 
    if ( k == n ) 
    goto lab10 ; 
    else ww = mem [ w ] .hhfield .v.RH ; 
    else if ( k == 1 ) 
    goto lab10 ; 
    else ww = mem [ w ] .hhfield .lhfield ; 
    du = mem [ ww + 1 ] .cint - mem [ w + 1 ] .cint ; 
    dv = mem [ ww + 2 ] .cint - mem [ w + 2 ] .cint ; 
    if ( abs ( du ) >= abs ( dv ) ) 
    {
      s = makefraction ( dv , du ) ; 
      t0 = takefraction ( x0 , s ) - y0 ; 
      t1 = takefraction ( x1 , s ) - y1 ; 
      t2 = takefraction ( x2 , s ) - y2 ; 
    } 
    else {
	
      s = makefraction ( du , dv ) ; 
      t0 = x0 - takefraction ( y0 , s ) ; 
      t1 = x1 - takefraction ( y1 , s ) ; 
      t2 = x2 - takefraction ( y2 , s ) ; 
    } 
    t = crossingpoint ( t0 , t1 , t2 ) ; 
    if ( t >= 268435456L ) 
    goto lab10 ; 
    {
      splitforoffset ( p , t ) ; 
      mem [ p ] .hhfield .b1 = k ; 
      p = mem [ p ] .hhfield .v.RH ; 
      v = x0 - takefraction ( x0 - x1 , t ) ; 
      x1 = x1 - takefraction ( x1 - x2 , t ) ; 
      x0 = v - takefraction ( v - x1 , t ) ; 
      v = y0 - takefraction ( y0 - y1 , t ) ; 
      y1 = y1 - takefraction ( y1 - y2 , t ) ; 
      y0 = v - takefraction ( v - y1 , t ) ; 
      t1 = t1 - takefraction ( t1 - t2 , t ) ; 
      if ( t1 > 0 ) 
      t1 = 0 ; 
      t = crossingpoint ( 0 , - (integer) t1 , - (integer) t2 ) ; 
      if ( t < 268435456L ) 
      {
	splitforoffset ( p , t ) ; 
	mem [ mem [ p ] .hhfield .v.RH ] .hhfield .b1 = k ; 
	v = x1 - takefraction ( x1 - x2 , t ) ; 
	x1 = x0 - takefraction ( x0 - x1 , t ) ; 
	x2 = x1 - takefraction ( x1 - v , t ) ; 
	v = y1 - takefraction ( y1 - y2 , t ) ; 
	y1 = y0 - takefraction ( y0 - y1 , t ) ; 
	y2 = y1 - takefraction ( y1 - v , t ) ; 
      } 
    } 
    if ( rising ) 
    incr ( k ) ; 
    else decr ( k ) ; 
    w = ww ; 
  } 
  lab10: ; 
} 
void zoffsetprep ( c , h ) 
halfword c ; 
halfword h ; 
{/* 30 45 */ halfword n  ; 
  halfword p, q, r, lh, ww  ; 
  halfword k  ; 
  halfword w  ; 
  integer x0, x1, x2, y0, y1, y2  ; 
  integer t0, t1, t2  ; 
  integer du, dv, dx, dy  ; 
  integer lmaxcoef  ; 
  integer x0a, x1a, x2a, y0a, y1a, y2a  ; 
  fraction t  ; 
  fraction s  ; 
  p = c ; 
  n = mem [ h ] .hhfield .lhfield ; 
  lh = mem [ h ] .hhfield .v.RH ; 
  while ( mem [ p ] .hhfield .b1 != 0 ) {
      
    q = mem [ p ] .hhfield .v.RH ; 
    if ( n <= 1 ) 
    mem [ p ] .hhfield .b1 = 1 ; 
    else {
	
      x0 = mem [ p + 5 ] .cint - mem [ p + 1 ] .cint ; 
      x2 = mem [ q + 1 ] .cint - mem [ q + 3 ] .cint ; 
      x1 = mem [ q + 3 ] .cint - mem [ p + 5 ] .cint ; 
      y0 = mem [ p + 6 ] .cint - mem [ p + 2 ] .cint ; 
      y2 = mem [ q + 2 ] .cint - mem [ q + 4 ] .cint ; 
      y1 = mem [ q + 4 ] .cint - mem [ p + 6 ] .cint ; 
      lmaxcoef = abs ( x0 ) ; 
      if ( abs ( x1 ) > lmaxcoef ) 
      lmaxcoef = abs ( x1 ) ; 
      if ( abs ( x2 ) > lmaxcoef ) 
      lmaxcoef = abs ( x2 ) ; 
      if ( abs ( y0 ) > lmaxcoef ) 
      lmaxcoef = abs ( y0 ) ; 
      if ( abs ( y1 ) > lmaxcoef ) 
      lmaxcoef = abs ( y1 ) ; 
      if ( abs ( y2 ) > lmaxcoef ) 
      lmaxcoef = abs ( y2 ) ; 
      if ( lmaxcoef == 0 ) 
      goto lab45 ; 
      while ( lmaxcoef < 134217728L ) {
	  
	lmaxcoef = lmaxcoef + lmaxcoef ; 
	x0 = x0 + x0 ; 
	x1 = x1 + x1 ; 
	x2 = x2 + x2 ; 
	y0 = y0 + y0 ; 
	y1 = y1 + y1 ; 
	y2 = y2 + y2 ; 
      } 
      dx = x0 ; 
      dy = y0 ; 
      if ( dx == 0 ) 
      if ( dy == 0 ) 
      {
	dx = x1 ; 
	dy = y1 ; 
	if ( dx == 0 ) 
	if ( dy == 0 ) 
	{
	  dx = x2 ; 
	  dy = y2 ; 
	} 
      } 
      if ( dx == 0 ) 
      finoffsetprep ( p , n , mem [ mem [ lh ] .hhfield .lhfield ] .hhfield 
      .lhfield , - (integer) x0 , - (integer) x1 , - (integer) x2 , 
      - (integer) y0 , - (integer) y1 , - (integer) y2 , false , n ) ; 
      else {
	  
	k = 1 ; 
	w = mem [ lh ] .hhfield .v.RH ; 
	while ( true ) {
	    
	  if ( k == n ) 
	  goto lab30 ; 
	  ww = mem [ w ] .hhfield .v.RH ; 
	  if ( abvscd ( dy , abs ( mem [ ww + 1 ] .cint - mem [ w + 1 ] .cint 
	  ) , dx , abs ( mem [ ww + 2 ] .cint - mem [ w + 2 ] .cint ) ) >= 0 ) 
	  {
	    incr ( k ) ; 
	    w = ww ; 
	  } 
	  else goto lab30 ; 
	} 
	lab30: ; 
	if ( k == 1 ) 
	t = 268435457L ; 
	else {
	    
	  ww = mem [ w ] .hhfield .lhfield ; 
	  du = mem [ ww + 1 ] .cint - mem [ w + 1 ] .cint ; 
	  dv = mem [ ww + 2 ] .cint - mem [ w + 2 ] .cint ; 
	  if ( abs ( du ) >= abs ( dv ) ) 
	  {
	    s = makefraction ( dv , du ) ; 
	    t0 = takefraction ( x0 , s ) - y0 ; 
	    t1 = takefraction ( x1 , s ) - y1 ; 
	    t2 = takefraction ( x2 , s ) - y2 ; 
	  } 
	  else {
	      
	    s = makefraction ( du , dv ) ; 
	    t0 = x0 - takefraction ( y0 , s ) ; 
	    t1 = x1 - takefraction ( y1 , s ) ; 
	    t2 = x2 - takefraction ( y2 , s ) ; 
	  } 
	  t = crossingpoint ( - (integer) t0 , - (integer) t1 , - (integer) t2 
	  ) ; 
	} 
	if ( t >= 268435456L ) 
	finoffsetprep ( p , k , w , x0 , x1 , x2 , y0 , y1 , y2 , true , n ) ; 
	else {
	    
	  splitforoffset ( p , t ) ; 
	  r = mem [ p ] .hhfield .v.RH ; 
	  x1a = x0 - takefraction ( x0 - x1 , t ) ; 
	  x1 = x1 - takefraction ( x1 - x2 , t ) ; 
	  x2a = x1a - takefraction ( x1a - x1 , t ) ; 
	  y1a = y0 - takefraction ( y0 - y1 , t ) ; 
	  y1 = y1 - takefraction ( y1 - y2 , t ) ; 
	  y2a = y1a - takefraction ( y1a - y1 , t ) ; 
	  finoffsetprep ( p , k , w , x0 , x1a , x2a , y0 , y1a , y2a , true , 
	  n ) ; 
	  x0 = x2a ; 
	  y0 = y2a ; 
	  t1 = t1 - takefraction ( t1 - t2 , t ) ; 
	  if ( t1 < 0 ) 
	  t1 = 0 ; 
	  t = crossingpoint ( 0 , t1 , t2 ) ; 
	  if ( t < 268435456L ) 
	  {
	    splitforoffset ( r , t ) ; 
	    x1a = x1 - takefraction ( x1 - x2 , t ) ; 
	    x1 = x0 - takefraction ( x0 - x1 , t ) ; 
	    x0a = x1 - takefraction ( x1 - x1a , t ) ; 
	    y1a = y1 - takefraction ( y1 - y2 , t ) ; 
	    y1 = y0 - takefraction ( y0 - y1 , t ) ; 
	    y0a = y1 - takefraction ( y1 - y1a , t ) ; 
	    finoffsetprep ( mem [ r ] .hhfield .v.RH , k , w , x0a , x1a , x2 
	    , y0a , y1a , y2 , true , n ) ; 
	    x2 = x0a ; 
	    y2 = y0a ; 
	  } 
	  finoffsetprep ( r , k - 1 , ww , - (integer) x0 , - (integer) x1 , 
	  - (integer) x2 , - (integer) y0 , - (integer) y1 , - (integer) y2 , 
	  false , n ) ; 
	} 
      } 
      lab45: ; 
    } 
    do {
	r = mem [ p ] .hhfield .v.RH ; 
      if ( mem [ p + 1 ] .cint == mem [ p + 5 ] .cint ) 
      if ( mem [ p + 2 ] .cint == mem [ p + 6 ] .cint ) 
      if ( mem [ p + 1 ] .cint == mem [ r + 3 ] .cint ) 
      if ( mem [ p + 2 ] .cint == mem [ r + 4 ] .cint ) 
      if ( mem [ p + 1 ] .cint == mem [ r + 1 ] .cint ) 
      if ( mem [ p + 2 ] .cint == mem [ r + 2 ] .cint ) 
      {
	removecubic ( p ) ; 
	if ( r == q ) 
	q = p ; 
	r = p ; 
      } 
      p = r ; 
    } while ( ! ( p == q ) ) ; 
  } 
} 
void zskewlineedges ( p , w , ww ) 
halfword p ; 
halfword w ; 
halfword ww ; 
{scaled x0, y0, x1, y1  ; 
  if ( ( mem [ w + 1 ] .cint != mem [ ww + 1 ] .cint ) || ( mem [ w + 2 ] 
  .cint != mem [ ww + 2 ] .cint ) ) 
  {
    x0 = mem [ p + 1 ] .cint + mem [ w + 1 ] .cint ; 
    y0 = mem [ p + 2 ] .cint + mem [ w + 2 ] .cint ; 
    x1 = mem [ p + 1 ] .cint + mem [ ww + 1 ] .cint ; 
    y1 = mem [ p + 2 ] .cint + mem [ ww + 2 ] .cint ; 
    unskew ( x0 , y0 , octant ) ; 
    x0 = curx ; 
    y0 = cury ; 
    unskew ( x1 , y1 , octant ) ; 
	;
#ifdef STAT
    if ( internal [ 10 ] > 65536L ) 
    {
      printnl ( 583 ) ; 
      printtwo ( x0 , y0 ) ; 
      print ( 582 ) ; 
      printtwo ( curx , cury ) ; 
      printnl ( 283 ) ; 
    } 
#endif /* STAT */
    lineedges ( x0 , y0 , curx , cury ) ; 
  } 
} 
void zdualmoves ( h , p , q ) 
halfword h ; 
halfword p ; 
halfword q ; 
{/* 30 31 */ halfword r, s  ; 
  integer m, n  ; 
  integer mm0, mm1  ; 
  integer k  ; 
  halfword w, ww  ; 
  integer smoothbot, smoothtop  ; 
  scaled xx, yy, xp, yp, delx, dely, tx, ty  ; 
  k = mem [ h ] .hhfield .lhfield + 1 ; 
  ww = mem [ h ] .hhfield .v.RH ; 
  w = mem [ ww ] .hhfield .lhfield ; 
  mm0 = floorunscaled ( mem [ p + 1 ] .cint + mem [ w + 1 ] .cint - xycorr [ 
  octant ] ) ; 
  mm1 = floorunscaled ( mem [ q + 1 ] .cint + mem [ ww + 1 ] .cint - xycorr [ 
  octant ] ) ; 
  {register integer for_end; n = 1 ; for_end = n1 - n0 + 1 ; if ( n <= 
  for_end) do 
    envmove [ n ] = mm1 ; 
  while ( n++ < for_end ) ; } 
  envmove [ 0 ] = mm0 ; 
  moveptr = 0 ; 
  m = mm0 ; 
  r = p ; 
  while ( true ) {
      
    if ( r == q ) 
    smoothtop = moveptr ; 
    while ( mem [ r ] .hhfield .b1 != k ) {
	
      xx = mem [ r + 1 ] .cint + mem [ w + 1 ] .cint ; 
      yy = mem [ r + 2 ] .cint + mem [ w + 2 ] .cint + 32768L ; 
	;
#ifdef STAT
      if ( internal [ 10 ] > 65536L ) 
      {
	printnl ( 584 ) ; 
	printint ( k ) ; 
	print ( 585 ) ; 
	unskew ( xx , yy - 32768L , octant ) ; 
	printtwo ( curx , cury ) ; 
      } 
#endif /* STAT */
      if ( mem [ r ] .hhfield .b1 < k ) 
      {
	decr ( k ) ; 
	w = mem [ w ] .hhfield .lhfield ; 
	xp = mem [ r + 1 ] .cint + mem [ w + 1 ] .cint ; 
	yp = mem [ r + 2 ] .cint + mem [ w + 2 ] .cint + 32768L ; 
	if ( yp != yy ) 
	{
	  ty = floorscaled ( yy - ycorr [ octant ] ) ; 
	  dely = yp - yy ; 
	  yy = yy - ty ; 
	  ty = yp - ycorr [ octant ] - ty ; 
	  if ( ty >= 65536L ) 
	  {
	    delx = xp - xx ; 
	    yy = 65536L - yy ; 
	    while ( true ) {
		
	      if ( m < envmove [ moveptr ] ) 
	      envmove [ moveptr ] = m ; 
	      tx = takefraction ( delx , makefraction ( yy , dely ) ) ; 
	      if ( abvscd ( tx , dely , delx , yy ) + xycorr [ octant ] > 0 ) 
	      decr ( tx ) ; 
	      m = floorunscaled ( xx + tx ) ; 
	      ty = ty - 65536L ; 
	      incr ( moveptr ) ; 
	      if ( ty < 65536L ) 
	      goto lab31 ; 
	      yy = yy + 65536L ; 
	    } 
	    lab31: if ( m < envmove [ moveptr ] ) 
	    envmove [ moveptr ] = m ; 
	  } 
	} 
      } 
      else {
	  
	incr ( k ) ; 
	w = mem [ w ] .hhfield .v.RH ; 
	xp = mem [ r + 1 ] .cint + mem [ w + 1 ] .cint ; 
	yp = mem [ r + 2 ] .cint + mem [ w + 2 ] .cint + 32768L ; 
      } 
	;
#ifdef STAT
      if ( internal [ 10 ] > 65536L ) 
      {
	print ( 582 ) ; 
	unskew ( xp , yp - 32768L , octant ) ; 
	printtwo ( curx , cury ) ; 
	printnl ( 283 ) ; 
      } 
#endif /* STAT */
      m = floorunscaled ( xp - xycorr [ octant ] ) ; 
      moveptr = floorunscaled ( yp - ycorr [ octant ] ) - n0 ; 
      if ( m < envmove [ moveptr ] ) 
      envmove [ moveptr ] = m ; 
    } 
    if ( r == p ) 
    smoothbot = moveptr ; 
    if ( r == q ) 
    goto lab30 ; 
    move [ moveptr ] = 1 ; 
    n = moveptr ; 
    s = mem [ r ] .hhfield .v.RH ; 
    makemoves ( mem [ r + 1 ] .cint + mem [ w + 1 ] .cint , mem [ r + 5 ] 
    .cint + mem [ w + 1 ] .cint , mem [ s + 3 ] .cint + mem [ w + 1 ] .cint , 
    mem [ s + 1 ] .cint + mem [ w + 1 ] .cint , mem [ r + 2 ] .cint + mem [ w 
    + 2 ] .cint + 32768L , mem [ r + 6 ] .cint + mem [ w + 2 ] .cint + 32768L 
    , mem [ s + 4 ] .cint + mem [ w + 2 ] .cint + 32768L , mem [ s + 2 ] .cint 
    + mem [ w + 2 ] .cint + 32768L , xycorr [ octant ] , ycorr [ octant ] ) ; 
    do {
	if ( m < envmove [ n ] ) 
      envmove [ n ] = m ; 
      m = m + move [ n ] - 1 ; 
      incr ( n ) ; 
    } while ( ! ( n > moveptr ) ) ; 
    r = s ; 
  } 
  lab30: 
	;
#ifdef DEBUG
  if ( ( m != mm1 ) || ( moveptr != n1 - n0 ) ) 
  confusion ( 50 ) ; 
#endif /* DEBUG */
  move [ 0 ] = d0 + envmove [ 1 ] - mm0 ; 
  {register integer for_end; n = 1 ; for_end = moveptr ; if ( n <= for_end) 
  do 
    move [ n ] = envmove [ n + 1 ] - envmove [ n ] + 1 ; 
  while ( n++ < for_end ) ; } 
  move [ moveptr ] = move [ moveptr ] - d1 ; 
  if ( internal [ 35 ] > 0 ) 
  smoothmoves ( smoothbot , smoothtop ) ; 
  movetoedges ( m0 , n0 , m1 , n1 ) ; 
  if ( mem [ q + 6 ] .cint == 1 ) 
  {
    w = mem [ h ] .hhfield .v.RH ; 
    skewlineedges ( q , w , mem [ w ] .hhfield .lhfield ) ; 
  } 
} 
void zfillenvelope ( spechead ) 
halfword spechead ; 
{/* 30 31 */ halfword p, q, r, s  ; 
  halfword h  ; 
  halfword www  ; 
  integer m, n  ; 
  integer mm0, mm1  ; 
  integer k  ; 
  halfword w, ww  ; 
  integer smoothbot, smoothtop  ; 
  scaled xx, yy, xp, yp, delx, dely, tx, ty  ; 
  if ( internal [ 10 ] > 0 ) 
  beginedgetracing () ; 
  p = spechead ; 
  do {
      octant = mem [ p + 3 ] .cint ; 
    h = curpen + octant ; 
    q = p ; 
    while ( mem [ q ] .hhfield .b1 != 0 ) q = mem [ q ] .hhfield .v.RH ; 
    w = mem [ h ] .hhfield .v.RH ; 
    if ( mem [ p + 4 ] .cint == 1 ) 
    w = mem [ w ] .hhfield .lhfield ; 
	;
#ifdef STAT
    if ( internal [ 10 ] > 65536L ) 
    {
      printnl ( 579 ) ; 
      print ( octantdir [ octant ] ) ; 
      print ( 557 ) ; 
      printint ( mem [ h ] .hhfield .lhfield ) ; 
      print ( 580 ) ; 
      if ( mem [ h ] .hhfield .lhfield != 1 ) 
      printchar ( 115 ) ; 
      print ( 581 ) ; 
      unskew ( mem [ p + 1 ] .cint + mem [ w + 1 ] .cint , mem [ p + 2 ] .cint 
      + mem [ w + 2 ] .cint , octant ) ; 
      printtwo ( curx , cury ) ; 
      ww = mem [ h ] .hhfield .v.RH ; 
      if ( mem [ q + 6 ] .cint == 1 ) 
      ww = mem [ ww ] .hhfield .lhfield ; 
      print ( 582 ) ; 
      unskew ( mem [ q + 1 ] .cint + mem [ ww + 1 ] .cint , mem [ q + 2 ] 
      .cint + mem [ ww + 2 ] .cint , octant ) ; 
      printtwo ( curx , cury ) ; 
    } 
#endif /* STAT */
    ww = mem [ h ] .hhfield .v.RH ; 
    www = ww ; 
    if ( odd ( octantnumber [ octant ] ) ) 
    www = mem [ www ] .hhfield .lhfield ; 
    else ww = mem [ ww ] .hhfield .lhfield ; 
    if ( w != ww ) 
    skewlineedges ( p , w , ww ) ; 
    endround ( mem [ p + 1 ] .cint + mem [ ww + 1 ] .cint , mem [ p + 2 ] 
    .cint + mem [ ww + 2 ] .cint ) ; 
    m0 = m1 ; 
    n0 = n1 ; 
    d0 = d1 ; 
    endround ( mem [ q + 1 ] .cint + mem [ www + 1 ] .cint , mem [ q + 2 ] 
    .cint + mem [ www + 2 ] .cint ) ; 
    if ( n1 - n0 >= movesize ) 
    overflow ( 539 , movesize ) ; 
    offsetprep ( p , h ) ; 
    q = p ; 
    while ( mem [ q ] .hhfield .b1 != 0 ) q = mem [ q ] .hhfield .v.RH ; 
    if ( odd ( octantnumber [ octant ] ) ) 
    {
      k = 0 ; 
      w = mem [ h ] .hhfield .v.RH ; 
      ww = mem [ w ] .hhfield .lhfield ; 
      mm0 = floorunscaled ( mem [ p + 1 ] .cint + mem [ w + 1 ] .cint - xycorr 
      [ octant ] ) ; 
      mm1 = floorunscaled ( mem [ q + 1 ] .cint + mem [ ww + 1 ] .cint - 
      xycorr [ octant ] ) ; 
      {register integer for_end; n = 0 ; for_end = n1 - n0 ; if ( n <= 
      for_end) do 
	envmove [ n ] = mm0 ; 
      while ( n++ < for_end ) ; } 
      envmove [ n1 - n0 ] = mm1 ; 
      moveptr = 0 ; 
      m = mm0 ; 
      r = p ; 
      mem [ q ] .hhfield .b1 = mem [ h ] .hhfield .lhfield + 1 ; 
      while ( true ) {
	  
	if ( r == q ) 
	smoothtop = moveptr ; 
	while ( mem [ r ] .hhfield .b1 != k ) {
	    
	  xx = mem [ r + 1 ] .cint + mem [ w + 1 ] .cint ; 
	  yy = mem [ r + 2 ] .cint + mem [ w + 2 ] .cint + 32768L ; 
	;
#ifdef STAT
	  if ( internal [ 10 ] > 65536L ) 
	  {
	    printnl ( 584 ) ; 
	    printint ( k ) ; 
	    print ( 585 ) ; 
	    unskew ( xx , yy - 32768L , octant ) ; 
	    printtwo ( curx , cury ) ; 
	  } 
#endif /* STAT */
	  if ( mem [ r ] .hhfield .b1 > k ) 
	  {
	    incr ( k ) ; 
	    w = mem [ w ] .hhfield .v.RH ; 
	    xp = mem [ r + 1 ] .cint + mem [ w + 1 ] .cint ; 
	    yp = mem [ r + 2 ] .cint + mem [ w + 2 ] .cint + 32768L ; 
	    if ( yp != yy ) 
	    {
	      ty = floorscaled ( yy - ycorr [ octant ] ) ; 
	      dely = yp - yy ; 
	      yy = yy - ty ; 
	      ty = yp - ycorr [ octant ] - ty ; 
	      if ( ty >= 65536L ) 
	      {
		delx = xp - xx ; 
		yy = 65536L - yy ; 
		while ( true ) {
		    
		  tx = takefraction ( delx , makefraction ( yy , dely ) ) ; 
		  if ( abvscd ( tx , dely , delx , yy ) + xycorr [ octant ] > 
		  0 ) 
		  decr ( tx ) ; 
		  m = floorunscaled ( xx + tx ) ; 
		  if ( m > envmove [ moveptr ] ) 
		  envmove [ moveptr ] = m ; 
		  ty = ty - 65536L ; 
		  if ( ty < 65536L ) 
		  goto lab31 ; 
		  yy = yy + 65536L ; 
		  incr ( moveptr ) ; 
		} 
		lab31: ; 
	      } 
	    } 
	  } 
	  else {
	      
	    decr ( k ) ; 
	    w = mem [ w ] .hhfield .lhfield ; 
	    xp = mem [ r + 1 ] .cint + mem [ w + 1 ] .cint ; 
	    yp = mem [ r + 2 ] .cint + mem [ w + 2 ] .cint + 32768L ; 
	  } 
	;
#ifdef STAT
	  if ( internal [ 10 ] > 65536L ) 
	  {
	    print ( 582 ) ; 
	    unskew ( xp , yp - 32768L , octant ) ; 
	    printtwo ( curx , cury ) ; 
	    printnl ( 283 ) ; 
	  } 
#endif /* STAT */
	  m = floorunscaled ( xp - xycorr [ octant ] ) ; 
	  moveptr = floorunscaled ( yp - ycorr [ octant ] ) - n0 ; 
	  if ( m > envmove [ moveptr ] ) 
	  envmove [ moveptr ] = m ; 
	} 
	if ( r == p ) 
	smoothbot = moveptr ; 
	if ( r == q ) 
	goto lab30 ; 
	move [ moveptr ] = 1 ; 
	n = moveptr ; 
	s = mem [ r ] .hhfield .v.RH ; 
	makemoves ( mem [ r + 1 ] .cint + mem [ w + 1 ] .cint , mem [ r + 5 ] 
	.cint + mem [ w + 1 ] .cint , mem [ s + 3 ] .cint + mem [ w + 1 ] 
	.cint , mem [ s + 1 ] .cint + mem [ w + 1 ] .cint , mem [ r + 2 ] 
	.cint + mem [ w + 2 ] .cint + 32768L , mem [ r + 6 ] .cint + mem [ w + 
	2 ] .cint + 32768L , mem [ s + 4 ] .cint + mem [ w + 2 ] .cint + 
	32768L , mem [ s + 2 ] .cint + mem [ w + 2 ] .cint + 32768L , xycorr [ 
	octant ] , ycorr [ octant ] ) ; 
	do {
	    m = m + move [ n ] - 1 ; 
	  if ( m > envmove [ n ] ) 
	  envmove [ n ] = m ; 
	  incr ( n ) ; 
	} while ( ! ( n > moveptr ) ) ; 
	r = s ; 
      } 
      lab30: 
	;
#ifdef DEBUG
      if ( ( m != mm1 ) || ( moveptr != n1 - n0 ) ) 
      confusion ( 49 ) ; 
#endif /* DEBUG */
      move [ 0 ] = d0 + envmove [ 0 ] - mm0 ; 
      {register integer for_end; n = 1 ; for_end = moveptr ; if ( n <= 
      for_end) do 
	move [ n ] = envmove [ n ] - envmove [ n - 1 ] + 1 ; 
      while ( n++ < for_end ) ; } 
      move [ moveptr ] = move [ moveptr ] - d1 ; 
      if ( internal [ 35 ] > 0 ) 
      smoothmoves ( smoothbot , smoothtop ) ; 
      movetoedges ( m0 , n0 , m1 , n1 ) ; 
      if ( mem [ q + 6 ] .cint == 0 ) 
      {
	w = mem [ h ] .hhfield .v.RH ; 
	skewlineedges ( q , mem [ w ] .hhfield .lhfield , w ) ; 
      } 
    } 
    else dualmoves ( h , p , q ) ; 
    mem [ q ] .hhfield .b1 = 0 ; 
    p = mem [ q ] .hhfield .v.RH ; 
  } while ( ! ( p == spechead ) ) ; 
  if ( internal [ 10 ] > 0 ) 
  endedgetracing () ; 
  tossknotlist ( spechead ) ; 
} 
halfword zmakeellipse ( majoraxis , minoraxis , theta ) 
scaled majoraxis ; 
scaled minoraxis ; 
angle theta ; 
{/* 30 31 40 */ register halfword Result; halfword p, q, r, s  ; 
  halfword h  ; 
  integer alpha, beta, gamma, delta  ; 
  integer c, d  ; 
  integer u, v  ; 
  boolean symmetric  ; 
  if ( ( majoraxis == minoraxis ) || ( theta % 94371840L == 0 ) ) 
  {
    symmetric = true ; 
    alpha = 0 ; 
    if ( odd ( theta / 94371840L ) ) 
    {
      beta = majoraxis ; 
      gamma = minoraxis ; 
      nsin = 268435456L ; 
      ncos = 0 ; 
    } 
    else {
	
      beta = minoraxis ; 
      gamma = majoraxis ; 
    } 
  } 
  else {
      
    symmetric = false ; 
    nsincos ( theta ) ; 
    gamma = takefraction ( majoraxis , nsin ) ; 
    delta = takefraction ( minoraxis , ncos ) ; 
    beta = pythadd ( gamma , delta ) ; 
    alpha = makefraction ( gamma , beta ) ; 
    alpha = takefraction ( majoraxis , alpha ) ; 
    alpha = takefraction ( alpha , ncos ) ; 
    alpha = ( alpha + 32768L ) / 65536L ; 
    gamma = takefraction ( minoraxis , nsin ) ; 
    gamma = pythadd ( takefraction ( majoraxis , ncos ) , gamma ) ; 
  } 
  beta = ( beta + 32768L ) / 65536L ; 
  gamma = ( gamma + 32768L ) / 65536L ; 
  p = getnode ( 7 ) ; 
  q = getnode ( 7 ) ; 
  r = getnode ( 7 ) ; 
  if ( symmetric ) 
  s = 0 ; 
  else s = getnode ( 7 ) ; 
  h = p ; 
  mem [ p ] .hhfield .v.RH = q ; 
  mem [ q ] .hhfield .v.RH = r ; 
  mem [ r ] .hhfield .v.RH = s ; 
  if ( beta == 0 ) 
  beta = 1 ; 
  if ( gamma == 0 ) 
  gamma = 1 ; 
  if ( gamma <= abs ( alpha ) ) 
  if ( alpha > 0 ) 
  alpha = gamma - 1 ; 
  else alpha = 1 - gamma ; 
  mem [ p + 1 ] .cint = - (integer) alpha * 32768L ; 
  mem [ p + 2 ] .cint = - (integer) beta * 32768L ; 
  mem [ q + 1 ] .cint = gamma * 32768L ; 
  mem [ q + 2 ] .cint = mem [ p + 2 ] .cint ; 
  mem [ r + 1 ] .cint = mem [ q + 1 ] .cint ; 
  mem [ p + 5 ] .cint = 0 ; 
  mem [ q + 3 ] .cint = -32768L ; 
  mem [ q + 5 ] .cint = 32768L ; 
  mem [ r + 3 ] .cint = 0 ; 
  mem [ r + 5 ] .cint = 0 ; 
  mem [ p + 6 ] .cint = beta ; 
  mem [ q + 6 ] .cint = gamma ; 
  mem [ r + 6 ] .cint = beta ; 
  mem [ q + 4 ] .cint = gamma + alpha ; 
  if ( symmetric ) 
  {
    mem [ r + 2 ] .cint = 0 ; 
    mem [ r + 4 ] .cint = beta ; 
  } 
  else {
      
    mem [ r + 2 ] .cint = - (integer) mem [ p + 2 ] .cint ; 
    mem [ r + 4 ] .cint = beta + beta ; 
    mem [ s + 1 ] .cint = - (integer) mem [ p + 1 ] .cint ; 
    mem [ s + 2 ] .cint = mem [ r + 2 ] .cint ; 
    mem [ s + 3 ] .cint = 32768L ; 
    mem [ s + 4 ] .cint = gamma - alpha ; 
  } 
  while ( true ) {
      
    u = mem [ p + 5 ] .cint + mem [ q + 5 ] .cint ; 
    v = mem [ q + 3 ] .cint + mem [ r + 3 ] .cint ; 
    c = mem [ p + 6 ] .cint + mem [ q + 6 ] .cint ; 
    delta = pythadd ( u , v ) ; 
    if ( majoraxis == minoraxis ) 
    d = majoraxis ; 
    else {
	
      if ( theta == 0 ) 
      {
	alpha = u ; 
	beta = v ; 
      } 
      else {
	  
	alpha = takefraction ( u , ncos ) + takefraction ( v , nsin ) ; 
	beta = takefraction ( v , ncos ) - takefraction ( u , nsin ) ; 
      } 
      alpha = makefraction ( alpha , delta ) ; 
      beta = makefraction ( beta , delta ) ; 
      d = pythadd ( takefraction ( majoraxis , alpha ) , takefraction ( 
      minoraxis , beta ) ) ; 
    } 
    alpha = abs ( u ) ; 
    beta = abs ( v ) ; 
    if ( alpha < beta ) 
    {
      alpha = abs ( v ) ; 
      beta = abs ( u ) ; 
    } 
    if ( internal [ 38 ] != 0 ) 
    d = d - takefraction ( internal [ 38 ] , makefraction ( beta + beta , 
    delta ) ) ; 
    d = takefraction ( ( d + 4 ) / 8 , delta ) ; 
    alpha = alpha / 32768L ; 
    if ( d < alpha ) 
    d = alpha ; 
    delta = c - d ; 
    if ( delta > 0 ) 
    {
      if ( delta > mem [ r + 4 ] .cint ) 
      delta = mem [ r + 4 ] .cint ; 
      if ( delta >= mem [ q + 4 ] .cint ) 
      {
	delta = mem [ q + 4 ] .cint ; 
	mem [ p + 6 ] .cint = c - delta ; 
	mem [ p + 5 ] .cint = u ; 
	mem [ q + 3 ] .cint = v ; 
	mem [ q + 1 ] .cint = mem [ q + 1 ] .cint - delta * mem [ r + 3 ] 
	.cint ; 
	mem [ q + 2 ] .cint = mem [ q + 2 ] .cint + delta * mem [ q + 5 ] 
	.cint ; 
	mem [ r + 4 ] .cint = mem [ r + 4 ] .cint - delta ; 
      } 
      else {
	  
	s = getnode ( 7 ) ; 
	mem [ p ] .hhfield .v.RH = s ; 
	mem [ s ] .hhfield .v.RH = q ; 
	mem [ s + 1 ] .cint = mem [ q + 1 ] .cint + delta * mem [ q + 3 ] 
	.cint ; 
	mem [ s + 2 ] .cint = mem [ q + 2 ] .cint - delta * mem [ p + 5 ] 
	.cint ; 
	mem [ q + 1 ] .cint = mem [ q + 1 ] .cint - delta * mem [ r + 3 ] 
	.cint ; 
	mem [ q + 2 ] .cint = mem [ q + 2 ] .cint + delta * mem [ q + 5 ] 
	.cint ; 
	mem [ s + 3 ] .cint = mem [ q + 3 ] .cint ; 
	mem [ s + 5 ] .cint = u ; 
	mem [ q + 3 ] .cint = v ; 
	mem [ s + 6 ] .cint = c - delta ; 
	mem [ s + 4 ] .cint = mem [ q + 4 ] .cint - delta ; 
	mem [ q + 4 ] .cint = delta ; 
	mem [ r + 4 ] .cint = mem [ r + 4 ] .cint - delta ; 
      } 
    } 
    else p = q ; 
    while ( true ) {
	
      q = mem [ p ] .hhfield .v.RH ; 
      if ( q == 0 ) 
      goto lab30 ; 
      if ( mem [ q + 4 ] .cint == 0 ) 
      {
	mem [ p ] .hhfield .v.RH = mem [ q ] .hhfield .v.RH ; 
	mem [ p + 6 ] .cint = mem [ q + 6 ] .cint ; 
	mem [ p + 5 ] .cint = mem [ q + 5 ] .cint ; 
	freenode ( q , 7 ) ; 
      } 
      else {
	  
	r = mem [ q ] .hhfield .v.RH ; 
	if ( r == 0 ) 
	goto lab30 ; 
	if ( mem [ r + 4 ] .cint == 0 ) 
	{
	  mem [ p ] .hhfield .v.RH = r ; 
	  freenode ( q , 7 ) ; 
	  p = r ; 
	} 
	else goto lab40 ; 
      } 
    } 
    lab40: ; 
  } 
  lab30: ; 
  if ( symmetric ) 
  {
    s = 0 ; 
    q = h ; 
    while ( true ) {
	
      r = getnode ( 7 ) ; 
      mem [ r ] .hhfield .v.RH = s ; 
      s = r ; 
      mem [ s + 1 ] .cint = mem [ q + 1 ] .cint ; 
      mem [ s + 2 ] .cint = - (integer) mem [ q + 2 ] .cint ; 
      if ( q == p ) 
      goto lab31 ; 
      q = mem [ q ] .hhfield .v.RH ; 
      if ( mem [ q + 2 ] .cint == 0 ) 
      goto lab31 ; 
    } 
    lab31: mem [ p ] .hhfield .v.RH = s ; 
    beta = - (integer) mem [ h + 2 ] .cint ; 
    while ( mem [ p + 2 ] .cint != beta ) p = mem [ p ] .hhfield .v.RH ; 
    q = mem [ p ] .hhfield .v.RH ; 
  } 
  if ( q != 0 ) 
  {
    if ( mem [ h + 5 ] .cint == 0 ) 
    {
      p = h ; 
      h = mem [ h ] .hhfield .v.RH ; 
      freenode ( p , 7 ) ; 
      mem [ q + 1 ] .cint = - (integer) mem [ h + 1 ] .cint ; 
    } 
    p = q ; 
  } 
  else q = p ; 
  r = mem [ h ] .hhfield .v.RH ; 
  do {
      s = getnode ( 7 ) ; 
    mem [ p ] .hhfield .v.RH = s ; 
    p = s ; 
    mem [ p + 1 ] .cint = - (integer) mem [ r + 1 ] .cint ; 
    mem [ p + 2 ] .cint = - (integer) mem [ r + 2 ] .cint ; 
    r = mem [ r ] .hhfield .v.RH ; 
  } while ( ! ( r == q ) ) ; 
  mem [ p ] .hhfield .v.RH = h ; 
  Result = h ; 
  return(Result) ; 
} 
scaled zfinddirectiontime ( x , y , h ) 
scaled x ; 
scaled y ; 
halfword h ; 
{/* 10 40 45 30 */ register scaled Result; scaled max  ; 
  halfword p, q  ; 
  scaled n  ; 
  scaled tt  ; 
  scaled x1, x2, x3, y1, y2, y3  ; 
  angle theta, phi  ; 
  fraction t  ; 
  if ( abs ( x ) < abs ( y ) ) 
  {
    x = makefraction ( x , abs ( y ) ) ; 
    if ( y > 0 ) 
    y = 268435456L ; 
    else y = -268435456L ; 
  } 
  else if ( x == 0 ) 
  {
    Result = 0 ; 
    goto lab10 ; 
  } 
  else {
      
    y = makefraction ( y , abs ( x ) ) ; 
    if ( x > 0 ) 
    x = 268435456L ; 
    else x = -268435456L ; 
  } 
  n = 0 ; 
  p = h ; 
  while ( true ) {
      
    if ( mem [ p ] .hhfield .b1 == 0 ) 
    goto lab45 ; 
    q = mem [ p ] .hhfield .v.RH ; 
    tt = 0 ; 
    x1 = mem [ p + 5 ] .cint - mem [ p + 1 ] .cint ; 
    x2 = mem [ q + 3 ] .cint - mem [ p + 5 ] .cint ; 
    x3 = mem [ q + 1 ] .cint - mem [ q + 3 ] .cint ; 
    y1 = mem [ p + 6 ] .cint - mem [ p + 2 ] .cint ; 
    y2 = mem [ q + 4 ] .cint - mem [ p + 6 ] .cint ; 
    y3 = mem [ q + 2 ] .cint - mem [ q + 4 ] .cint ; 
    max = abs ( x1 ) ; 
    if ( abs ( x2 ) > max ) 
    max = abs ( x2 ) ; 
    if ( abs ( x3 ) > max ) 
    max = abs ( x3 ) ; 
    if ( abs ( y1 ) > max ) 
    max = abs ( y1 ) ; 
    if ( abs ( y2 ) > max ) 
    max = abs ( y2 ) ; 
    if ( abs ( y3 ) > max ) 
    max = abs ( y3 ) ; 
    if ( max == 0 ) 
    goto lab40 ; 
    while ( max < 134217728L ) {
	
      max = max + max ; 
      x1 = x1 + x1 ; 
      x2 = x2 + x2 ; 
      x3 = x3 + x3 ; 
      y1 = y1 + y1 ; 
      y2 = y2 + y2 ; 
      y3 = y3 + y3 ; 
    } 
    t = x1 ; 
    x1 = takefraction ( x1 , x ) + takefraction ( y1 , y ) ; 
    y1 = takefraction ( y1 , x ) - takefraction ( t , y ) ; 
    t = x2 ; 
    x2 = takefraction ( x2 , x ) + takefraction ( y2 , y ) ; 
    y2 = takefraction ( y2 , x ) - takefraction ( t , y ) ; 
    t = x3 ; 
    x3 = takefraction ( x3 , x ) + takefraction ( y3 , y ) ; 
    y3 = takefraction ( y3 , x ) - takefraction ( t , y ) ; 
    if ( y1 == 0 ) 
    if ( x1 >= 0 ) 
    goto lab40 ; 
    if ( n > 0 ) 
    {
      theta = narg ( x1 , y1 ) ; 
      if ( theta >= 0 ) 
      if ( phi <= 0 ) 
      if ( phi >= theta - 188743680L ) 
      goto lab40 ; 
      if ( theta <= 0 ) 
      if ( phi >= 0 ) 
      if ( phi <= theta + 188743680L ) 
      goto lab40 ; 
      if ( p == h ) 
      goto lab45 ; 
    } 
    if ( ( x3 != 0 ) || ( y3 != 0 ) ) 
    phi = narg ( x3 , y3 ) ; 
    if ( x1 < 0 ) 
    if ( x2 < 0 ) 
    if ( x3 < 0 ) 
    goto lab30 ; 
    if ( abvscd ( y1 , y3 , y2 , y2 ) == 0 ) 
    {
      if ( abvscd ( y1 , y2 , 0 , 0 ) < 0 ) 
      {
	t = makefraction ( y1 , y1 - y2 ) ; 
	x1 = x1 - takefraction ( x1 - x2 , t ) ; 
	x2 = x2 - takefraction ( x2 - x3 , t ) ; 
	if ( x1 - takefraction ( x1 - x2 , t ) >= 0 ) 
	{
	  tt = ( t + 2048 ) / 4096 ; 
	  goto lab40 ; 
	} 
      } 
      else if ( y3 == 0 ) 
      if ( y1 == 0 ) 
      {
	t = crossingpoint ( - (integer) x1 , - (integer) x2 , - (integer) x3 ) 
	; 
	if ( t <= 268435456L ) 
	{
	  tt = ( t + 2048 ) / 4096 ; 
	  goto lab40 ; 
	} 
	if ( abvscd ( x1 , x3 , x2 , x2 ) <= 0 ) 
	{
	  t = makefraction ( x1 , x1 - x2 ) ; 
	  {
	    tt = ( t + 2048 ) / 4096 ; 
	    goto lab40 ; 
	  } 
	} 
      } 
      else if ( x3 >= 0 ) 
      {
	tt = 65536L ; 
	goto lab40 ; 
      } 
      goto lab30 ; 
    } 
    if ( y1 <= 0 ) 
    if ( y1 < 0 ) 
    {
      y1 = - (integer) y1 ; 
      y2 = - (integer) y2 ; 
      y3 = - (integer) y3 ; 
    } 
    else if ( y2 > 0 ) 
    {
      y2 = - (integer) y2 ; 
      y3 = - (integer) y3 ; 
    } 
    t = crossingpoint ( y1 , y2 , y3 ) ; 
    if ( t > 268435456L ) 
    goto lab30 ; 
    y2 = y2 - takefraction ( y2 - y3 , t ) ; 
    x1 = x1 - takefraction ( x1 - x2 , t ) ; 
    x2 = x2 - takefraction ( x2 - x3 , t ) ; 
    x1 = x1 - takefraction ( x1 - x2 , t ) ; 
    if ( x1 >= 0 ) 
    {
      tt = ( t + 2048 ) / 4096 ; 
      goto lab40 ; 
    } 
    if ( y2 > 0 ) 
    y2 = 0 ; 
    tt = t ; 
    t = crossingpoint ( 0 , - (integer) y2 , - (integer) y3 ) ; 
    if ( t > 268435456L ) 
    goto lab30 ; 
    x1 = x1 - takefraction ( x1 - x2 , t ) ; 
    x2 = x2 - takefraction ( x2 - x3 , t ) ; 
    if ( x1 - takefraction ( x1 - x2 , t ) >= 0 ) 
    {
      t = tt - takefraction ( tt - 268435456L , t ) ; 
      {
	tt = ( t + 2048 ) / 4096 ; 
	goto lab40 ; 
      } 
    } 
    lab30: ; 
    p = q ; 
    n = n + 65536L ; 
  } 
  lab45: Result = -65536L ; 
  goto lab10 ; 
  lab40: Result = n + tt ; 
  lab10: ; 
  return(Result) ; 
} 
void zcubicintersection ( p , pp ) 
halfword p ; 
halfword pp ; 
{/* 22 45 10 */ halfword q, qq  ; 
  timetogo = 5000 ; 
  maxt = 2 ; 
  q = mem [ p ] .hhfield .v.RH ; 
  qq = mem [ pp ] .hhfield .v.RH ; 
  bisectptr = 20 ; 
  bisectstack [ bisectptr - 5 ] = mem [ p + 5 ] .cint - mem [ p + 1 ] .cint ; 
  bisectstack [ bisectptr - 4 ] = mem [ q + 3 ] .cint - mem [ p + 5 ] .cint ; 
  bisectstack [ bisectptr - 3 ] = mem [ q + 1 ] .cint - mem [ q + 3 ] .cint ; 
  if ( bisectstack [ bisectptr - 5 ] < 0 ) 
  if ( bisectstack [ bisectptr - 3 ] >= 0 ) 
  {
    if ( bisectstack [ bisectptr - 4 ] < 0 ) 
    bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] ; 
    else bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] ; 
    bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
    if ( bisectstack [ bisectptr - 1 ] < 0 ) 
    bisectstack [ bisectptr - 1 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
    if ( bisectstack [ bisectptr - 2 ] > bisectstack [ bisectptr - 5 ] ) 
    bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] ; 
    bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] ; 
    if ( bisectstack [ bisectptr - 1 ] < 0 ) 
    bisectstack [ bisectptr - 1 ] = 0 ; 
  } 
  else if ( bisectstack [ bisectptr - 3 ] <= 0 ) 
  {
    if ( bisectstack [ bisectptr - 4 ] > 0 ) 
    bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] ; 
    else bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] ; 
    bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
    if ( bisectstack [ bisectptr - 2 ] > 0 ) 
    bisectstack [ bisectptr - 2 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
    if ( bisectstack [ bisectptr - 1 ] < bisectstack [ bisectptr - 5 ] ) 
    bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] ; 
    bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
    bisectstack [ bisectptr - 4 ] ; 
    if ( bisectstack [ bisectptr - 2 ] > 0 ) 
    bisectstack [ bisectptr - 2 ] = 0 ; 
  } 
  bisectstack [ bisectptr - 10 ] = mem [ p + 6 ] .cint - mem [ p + 2 ] .cint ; 
  bisectstack [ bisectptr - 9 ] = mem [ q + 4 ] .cint - mem [ p + 6 ] .cint ; 
  bisectstack [ bisectptr - 8 ] = mem [ q + 2 ] .cint - mem [ q + 4 ] .cint ; 
  if ( bisectstack [ bisectptr - 10 ] < 0 ) 
  if ( bisectstack [ bisectptr - 8 ] >= 0 ) 
  {
    if ( bisectstack [ bisectptr - 9 ] < 0 ) 
    bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] ; 
    else bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] ; 
    bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
    if ( bisectstack [ bisectptr - 6 ] < 0 ) 
    bisectstack [ bisectptr - 6 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
    if ( bisectstack [ bisectptr - 7 ] > bisectstack [ bisectptr - 10 ] ) 
    bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] ; 
    bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] ; 
    if ( bisectstack [ bisectptr - 6 ] < 0 ) 
    bisectstack [ bisectptr - 6 ] = 0 ; 
  } 
  else if ( bisectstack [ bisectptr - 8 ] <= 0 ) 
  {
    if ( bisectstack [ bisectptr - 9 ] > 0 ) 
    bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] ; 
    else bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] ; 
    bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
    if ( bisectstack [ bisectptr - 7 ] > 0 ) 
    bisectstack [ bisectptr - 7 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
    if ( bisectstack [ bisectptr - 6 ] < bisectstack [ bisectptr - 10 ] ) 
    bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] ; 
    bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
    bisectstack [ bisectptr - 9 ] ; 
    if ( bisectstack [ bisectptr - 7 ] > 0 ) 
    bisectstack [ bisectptr - 7 ] = 0 ; 
  } 
  bisectstack [ bisectptr - 15 ] = mem [ pp + 5 ] .cint - mem [ pp + 1 ] .cint 
  ; 
  bisectstack [ bisectptr - 14 ] = mem [ qq + 3 ] .cint - mem [ pp + 5 ] .cint 
  ; 
  bisectstack [ bisectptr - 13 ] = mem [ qq + 1 ] .cint - mem [ qq + 3 ] .cint 
  ; 
  if ( bisectstack [ bisectptr - 15 ] < 0 ) 
  if ( bisectstack [ bisectptr - 13 ] >= 0 ) 
  {
    if ( bisectstack [ bisectptr - 14 ] < 0 ) 
    bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] ; 
    else bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] ; 
    bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
    if ( bisectstack [ bisectptr - 11 ] < 0 ) 
    bisectstack [ bisectptr - 11 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
    if ( bisectstack [ bisectptr - 12 ] > bisectstack [ bisectptr - 15 ] ) 
    bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] ; 
    bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] ; 
    if ( bisectstack [ bisectptr - 11 ] < 0 ) 
    bisectstack [ bisectptr - 11 ] = 0 ; 
  } 
  else if ( bisectstack [ bisectptr - 13 ] <= 0 ) 
  {
    if ( bisectstack [ bisectptr - 14 ] > 0 ) 
    bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] ; 
    else bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] ; 
    bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
    if ( bisectstack [ bisectptr - 12 ] > 0 ) 
    bisectstack [ bisectptr - 12 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
    if ( bisectstack [ bisectptr - 11 ] < bisectstack [ bisectptr - 15 ] ) 
    bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] ; 
    bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
    bisectstack [ bisectptr - 14 ] ; 
    if ( bisectstack [ bisectptr - 12 ] > 0 ) 
    bisectstack [ bisectptr - 12 ] = 0 ; 
  } 
  bisectstack [ bisectptr - 20 ] = mem [ pp + 6 ] .cint - mem [ pp + 2 ] .cint 
  ; 
  bisectstack [ bisectptr - 19 ] = mem [ qq + 4 ] .cint - mem [ pp + 6 ] .cint 
  ; 
  bisectstack [ bisectptr - 18 ] = mem [ qq + 2 ] .cint - mem [ qq + 4 ] .cint 
  ; 
  if ( bisectstack [ bisectptr - 20 ] < 0 ) 
  if ( bisectstack [ bisectptr - 18 ] >= 0 ) 
  {
    if ( bisectstack [ bisectptr - 19 ] < 0 ) 
    bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] ; 
    else bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] ; 
    bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
    if ( bisectstack [ bisectptr - 16 ] < 0 ) 
    bisectstack [ bisectptr - 16 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
    if ( bisectstack [ bisectptr - 17 ] > bisectstack [ bisectptr - 20 ] ) 
    bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] ; 
    bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] ; 
    if ( bisectstack [ bisectptr - 16 ] < 0 ) 
    bisectstack [ bisectptr - 16 ] = 0 ; 
  } 
  else if ( bisectstack [ bisectptr - 18 ] <= 0 ) 
  {
    if ( bisectstack [ bisectptr - 19 ] > 0 ) 
    bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] ; 
    else bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] ; 
    bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
    if ( bisectstack [ bisectptr - 17 ] > 0 ) 
    bisectstack [ bisectptr - 17 ] = 0 ; 
  } 
  else {
      
    bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
    if ( bisectstack [ bisectptr - 16 ] < bisectstack [ bisectptr - 20 ] ) 
    bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] ; 
    bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
    bisectstack [ bisectptr - 19 ] ; 
    if ( bisectstack [ bisectptr - 17 ] > 0 ) 
    bisectstack [ bisectptr - 17 ] = 0 ; 
  } 
  delx = mem [ p + 1 ] .cint - mem [ pp + 1 ] .cint ; 
  dely = mem [ p + 2 ] .cint - mem [ pp + 2 ] .cint ; 
  tol = 0 ; 
  uv = bisectptr ; 
  xy = bisectptr ; 
  threel = 0 ; 
  curt = 1 ; 
  curtt = 1 ; 
  while ( true ) {
      
    lab22: if ( delx - tol <= bisectstack [ xy - 11 ] - bisectstack [ uv - 2 ] 
    ) 
    if ( delx + tol >= bisectstack [ xy - 12 ] - bisectstack [ uv - 1 ] ) 
    if ( dely - tol <= bisectstack [ xy - 16 ] - bisectstack [ uv - 7 ] ) 
    if ( dely + tol >= bisectstack [ xy - 17 ] - bisectstack [ uv - 6 ] ) 
    {
      if ( curt >= maxt ) 
      {
	if ( maxt == 131072L ) 
	{
	  curt = ( curt + 1 ) / 2 ; 
	  curtt = ( curtt + 1 ) / 2 ; 
	  goto lab10 ; 
	} 
	maxt = maxt + maxt ; 
	apprt = curt ; 
	apprtt = curtt ; 
      } 
      bisectstack [ bisectptr ] = delx ; 
      bisectstack [ bisectptr + 1 ] = dely ; 
      bisectstack [ bisectptr + 2 ] = tol ; 
      bisectstack [ bisectptr + 3 ] = uv ; 
      bisectstack [ bisectptr + 4 ] = xy ; 
      bisectptr = bisectptr + 45 ; 
      curt = curt + curt ; 
      curtt = curtt + curtt ; 
      bisectstack [ bisectptr - 25 ] = bisectstack [ uv - 5 ] ; 
      bisectstack [ bisectptr - 3 ] = bisectstack [ uv - 3 ] ; 
      bisectstack [ bisectptr - 24 ] = ( bisectstack [ bisectptr - 25 ] + 
      bisectstack [ uv - 4 ] ) / 2 ; 
      bisectstack [ bisectptr - 4 ] = ( bisectstack [ bisectptr - 3 ] + 
      bisectstack [ uv - 4 ] ) / 2 ; 
      bisectstack [ bisectptr - 23 ] = ( bisectstack [ bisectptr - 24 ] + 
      bisectstack [ bisectptr - 4 ] ) / 2 ; 
      bisectstack [ bisectptr - 5 ] = bisectstack [ bisectptr - 23 ] ; 
      if ( bisectstack [ bisectptr - 25 ] < 0 ) 
      if ( bisectstack [ bisectptr - 23 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 24 ] < 0 ) 
	bisectstack [ bisectptr - 22 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] ; 
	else bisectstack [ bisectptr - 22 ] = bisectstack [ bisectptr - 25 ] ; 
	bisectstack [ bisectptr - 21 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] + bisectstack [ bisectptr - 23 ] ; 
	if ( bisectstack [ bisectptr - 21 ] < 0 ) 
	bisectstack [ bisectptr - 21 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 22 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] + bisectstack [ bisectptr - 23 ] ; 
	if ( bisectstack [ bisectptr - 22 ] > bisectstack [ bisectptr - 25 ] ) 
	bisectstack [ bisectptr - 22 ] = bisectstack [ bisectptr - 25 ] ; 
	bisectstack [ bisectptr - 21 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] ; 
	if ( bisectstack [ bisectptr - 21 ] < 0 ) 
	bisectstack [ bisectptr - 21 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 23 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 24 ] > 0 ) 
	bisectstack [ bisectptr - 21 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] ; 
	else bisectstack [ bisectptr - 21 ] = bisectstack [ bisectptr - 25 ] ; 
	bisectstack [ bisectptr - 22 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] + bisectstack [ bisectptr - 23 ] ; 
	if ( bisectstack [ bisectptr - 22 ] > 0 ) 
	bisectstack [ bisectptr - 22 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 21 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] + bisectstack [ bisectptr - 23 ] ; 
	if ( bisectstack [ bisectptr - 21 ] < bisectstack [ bisectptr - 25 ] ) 
	bisectstack [ bisectptr - 21 ] = bisectstack [ bisectptr - 25 ] ; 
	bisectstack [ bisectptr - 22 ] = bisectstack [ bisectptr - 25 ] + 
	bisectstack [ bisectptr - 24 ] ; 
	if ( bisectstack [ bisectptr - 22 ] > 0 ) 
	bisectstack [ bisectptr - 22 ] = 0 ; 
      } 
      if ( bisectstack [ bisectptr - 5 ] < 0 ) 
      if ( bisectstack [ bisectptr - 3 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 4 ] < 0 ) 
	bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] ; 
	else bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] ; 
	bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
	if ( bisectstack [ bisectptr - 1 ] < 0 ) 
	bisectstack [ bisectptr - 1 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
	if ( bisectstack [ bisectptr - 2 ] > bisectstack [ bisectptr - 5 ] ) 
	bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] ; 
	bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] ; 
	if ( bisectstack [ bisectptr - 1 ] < 0 ) 
	bisectstack [ bisectptr - 1 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 3 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 4 ] > 0 ) 
	bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] ; 
	else bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] ; 
	bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
	if ( bisectstack [ bisectptr - 2 ] > 0 ) 
	bisectstack [ bisectptr - 2 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] + bisectstack [ bisectptr - 3 ] ; 
	if ( bisectstack [ bisectptr - 1 ] < bisectstack [ bisectptr - 5 ] ) 
	bisectstack [ bisectptr - 1 ] = bisectstack [ bisectptr - 5 ] ; 
	bisectstack [ bisectptr - 2 ] = bisectstack [ bisectptr - 5 ] + 
	bisectstack [ bisectptr - 4 ] ; 
	if ( bisectstack [ bisectptr - 2 ] > 0 ) 
	bisectstack [ bisectptr - 2 ] = 0 ; 
      } 
      bisectstack [ bisectptr - 30 ] = bisectstack [ uv - 10 ] ; 
      bisectstack [ bisectptr - 8 ] = bisectstack [ uv - 8 ] ; 
      bisectstack [ bisectptr - 29 ] = ( bisectstack [ bisectptr - 30 ] + 
      bisectstack [ uv - 9 ] ) / 2 ; 
      bisectstack [ bisectptr - 9 ] = ( bisectstack [ bisectptr - 8 ] + 
      bisectstack [ uv - 9 ] ) / 2 ; 
      bisectstack [ bisectptr - 28 ] = ( bisectstack [ bisectptr - 29 ] + 
      bisectstack [ bisectptr - 9 ] ) / 2 ; 
      bisectstack [ bisectptr - 10 ] = bisectstack [ bisectptr - 28 ] ; 
      if ( bisectstack [ bisectptr - 30 ] < 0 ) 
      if ( bisectstack [ bisectptr - 28 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 29 ] < 0 ) 
	bisectstack [ bisectptr - 27 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] ; 
	else bisectstack [ bisectptr - 27 ] = bisectstack [ bisectptr - 30 ] ; 
	bisectstack [ bisectptr - 26 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] + bisectstack [ bisectptr - 28 ] ; 
	if ( bisectstack [ bisectptr - 26 ] < 0 ) 
	bisectstack [ bisectptr - 26 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 27 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] + bisectstack [ bisectptr - 28 ] ; 
	if ( bisectstack [ bisectptr - 27 ] > bisectstack [ bisectptr - 30 ] ) 
	bisectstack [ bisectptr - 27 ] = bisectstack [ bisectptr - 30 ] ; 
	bisectstack [ bisectptr - 26 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] ; 
	if ( bisectstack [ bisectptr - 26 ] < 0 ) 
	bisectstack [ bisectptr - 26 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 28 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 29 ] > 0 ) 
	bisectstack [ bisectptr - 26 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] ; 
	else bisectstack [ bisectptr - 26 ] = bisectstack [ bisectptr - 30 ] ; 
	bisectstack [ bisectptr - 27 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] + bisectstack [ bisectptr - 28 ] ; 
	if ( bisectstack [ bisectptr - 27 ] > 0 ) 
	bisectstack [ bisectptr - 27 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 26 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] + bisectstack [ bisectptr - 28 ] ; 
	if ( bisectstack [ bisectptr - 26 ] < bisectstack [ bisectptr - 30 ] ) 
	bisectstack [ bisectptr - 26 ] = bisectstack [ bisectptr - 30 ] ; 
	bisectstack [ bisectptr - 27 ] = bisectstack [ bisectptr - 30 ] + 
	bisectstack [ bisectptr - 29 ] ; 
	if ( bisectstack [ bisectptr - 27 ] > 0 ) 
	bisectstack [ bisectptr - 27 ] = 0 ; 
      } 
      if ( bisectstack [ bisectptr - 10 ] < 0 ) 
      if ( bisectstack [ bisectptr - 8 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 9 ] < 0 ) 
	bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] ; 
	else bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] ; 
	bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
	if ( bisectstack [ bisectptr - 6 ] < 0 ) 
	bisectstack [ bisectptr - 6 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
	if ( bisectstack [ bisectptr - 7 ] > bisectstack [ bisectptr - 10 ] ) 
	bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] ; 
	bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] ; 
	if ( bisectstack [ bisectptr - 6 ] < 0 ) 
	bisectstack [ bisectptr - 6 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 8 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 9 ] > 0 ) 
	bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] ; 
	else bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] ; 
	bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
	if ( bisectstack [ bisectptr - 7 ] > 0 ) 
	bisectstack [ bisectptr - 7 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] + bisectstack [ bisectptr - 8 ] ; 
	if ( bisectstack [ bisectptr - 6 ] < bisectstack [ bisectptr - 10 ] ) 
	bisectstack [ bisectptr - 6 ] = bisectstack [ bisectptr - 10 ] ; 
	bisectstack [ bisectptr - 7 ] = bisectstack [ bisectptr - 10 ] + 
	bisectstack [ bisectptr - 9 ] ; 
	if ( bisectstack [ bisectptr - 7 ] > 0 ) 
	bisectstack [ bisectptr - 7 ] = 0 ; 
      } 
      bisectstack [ bisectptr - 35 ] = bisectstack [ xy - 15 ] ; 
      bisectstack [ bisectptr - 13 ] = bisectstack [ xy - 13 ] ; 
      bisectstack [ bisectptr - 34 ] = ( bisectstack [ bisectptr - 35 ] + 
      bisectstack [ xy - 14 ] ) / 2 ; 
      bisectstack [ bisectptr - 14 ] = ( bisectstack [ bisectptr - 13 ] + 
      bisectstack [ xy - 14 ] ) / 2 ; 
      bisectstack [ bisectptr - 33 ] = ( bisectstack [ bisectptr - 34 ] + 
      bisectstack [ bisectptr - 14 ] ) / 2 ; 
      bisectstack [ bisectptr - 15 ] = bisectstack [ bisectptr - 33 ] ; 
      if ( bisectstack [ bisectptr - 35 ] < 0 ) 
      if ( bisectstack [ bisectptr - 33 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 34 ] < 0 ) 
	bisectstack [ bisectptr - 32 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] ; 
	else bisectstack [ bisectptr - 32 ] = bisectstack [ bisectptr - 35 ] ; 
	bisectstack [ bisectptr - 31 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] + bisectstack [ bisectptr - 33 ] ; 
	if ( bisectstack [ bisectptr - 31 ] < 0 ) 
	bisectstack [ bisectptr - 31 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 32 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] + bisectstack [ bisectptr - 33 ] ; 
	if ( bisectstack [ bisectptr - 32 ] > bisectstack [ bisectptr - 35 ] ) 
	bisectstack [ bisectptr - 32 ] = bisectstack [ bisectptr - 35 ] ; 
	bisectstack [ bisectptr - 31 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] ; 
	if ( bisectstack [ bisectptr - 31 ] < 0 ) 
	bisectstack [ bisectptr - 31 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 33 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 34 ] > 0 ) 
	bisectstack [ bisectptr - 31 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] ; 
	else bisectstack [ bisectptr - 31 ] = bisectstack [ bisectptr - 35 ] ; 
	bisectstack [ bisectptr - 32 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] + bisectstack [ bisectptr - 33 ] ; 
	if ( bisectstack [ bisectptr - 32 ] > 0 ) 
	bisectstack [ bisectptr - 32 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 31 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] + bisectstack [ bisectptr - 33 ] ; 
	if ( bisectstack [ bisectptr - 31 ] < bisectstack [ bisectptr - 35 ] ) 
	bisectstack [ bisectptr - 31 ] = bisectstack [ bisectptr - 35 ] ; 
	bisectstack [ bisectptr - 32 ] = bisectstack [ bisectptr - 35 ] + 
	bisectstack [ bisectptr - 34 ] ; 
	if ( bisectstack [ bisectptr - 32 ] > 0 ) 
	bisectstack [ bisectptr - 32 ] = 0 ; 
      } 
      if ( bisectstack [ bisectptr - 15 ] < 0 ) 
      if ( bisectstack [ bisectptr - 13 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 14 ] < 0 ) 
	bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] ; 
	else bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] ; 
	bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
	if ( bisectstack [ bisectptr - 11 ] < 0 ) 
	bisectstack [ bisectptr - 11 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
	if ( bisectstack [ bisectptr - 12 ] > bisectstack [ bisectptr - 15 ] ) 
	bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] ; 
	bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] ; 
	if ( bisectstack [ bisectptr - 11 ] < 0 ) 
	bisectstack [ bisectptr - 11 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 13 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 14 ] > 0 ) 
	bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] ; 
	else bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] ; 
	bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
	if ( bisectstack [ bisectptr - 12 ] > 0 ) 
	bisectstack [ bisectptr - 12 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] + bisectstack [ bisectptr - 13 ] ; 
	if ( bisectstack [ bisectptr - 11 ] < bisectstack [ bisectptr - 15 ] ) 
	bisectstack [ bisectptr - 11 ] = bisectstack [ bisectptr - 15 ] ; 
	bisectstack [ bisectptr - 12 ] = bisectstack [ bisectptr - 15 ] + 
	bisectstack [ bisectptr - 14 ] ; 
	if ( bisectstack [ bisectptr - 12 ] > 0 ) 
	bisectstack [ bisectptr - 12 ] = 0 ; 
      } 
      bisectstack [ bisectptr - 40 ] = bisectstack [ xy - 20 ] ; 
      bisectstack [ bisectptr - 18 ] = bisectstack [ xy - 18 ] ; 
      bisectstack [ bisectptr - 39 ] = ( bisectstack [ bisectptr - 40 ] + 
      bisectstack [ xy - 19 ] ) / 2 ; 
      bisectstack [ bisectptr - 19 ] = ( bisectstack [ bisectptr - 18 ] + 
      bisectstack [ xy - 19 ] ) / 2 ; 
      bisectstack [ bisectptr - 38 ] = ( bisectstack [ bisectptr - 39 ] + 
      bisectstack [ bisectptr - 19 ] ) / 2 ; 
      bisectstack [ bisectptr - 20 ] = bisectstack [ bisectptr - 38 ] ; 
      if ( bisectstack [ bisectptr - 40 ] < 0 ) 
      if ( bisectstack [ bisectptr - 38 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 39 ] < 0 ) 
	bisectstack [ bisectptr - 37 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] ; 
	else bisectstack [ bisectptr - 37 ] = bisectstack [ bisectptr - 40 ] ; 
	bisectstack [ bisectptr - 36 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] + bisectstack [ bisectptr - 38 ] ; 
	if ( bisectstack [ bisectptr - 36 ] < 0 ) 
	bisectstack [ bisectptr - 36 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 37 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] + bisectstack [ bisectptr - 38 ] ; 
	if ( bisectstack [ bisectptr - 37 ] > bisectstack [ bisectptr - 40 ] ) 
	bisectstack [ bisectptr - 37 ] = bisectstack [ bisectptr - 40 ] ; 
	bisectstack [ bisectptr - 36 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] ; 
	if ( bisectstack [ bisectptr - 36 ] < 0 ) 
	bisectstack [ bisectptr - 36 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 38 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 39 ] > 0 ) 
	bisectstack [ bisectptr - 36 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] ; 
	else bisectstack [ bisectptr - 36 ] = bisectstack [ bisectptr - 40 ] ; 
	bisectstack [ bisectptr - 37 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] + bisectstack [ bisectptr - 38 ] ; 
	if ( bisectstack [ bisectptr - 37 ] > 0 ) 
	bisectstack [ bisectptr - 37 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 36 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] + bisectstack [ bisectptr - 38 ] ; 
	if ( bisectstack [ bisectptr - 36 ] < bisectstack [ bisectptr - 40 ] ) 
	bisectstack [ bisectptr - 36 ] = bisectstack [ bisectptr - 40 ] ; 
	bisectstack [ bisectptr - 37 ] = bisectstack [ bisectptr - 40 ] + 
	bisectstack [ bisectptr - 39 ] ; 
	if ( bisectstack [ bisectptr - 37 ] > 0 ) 
	bisectstack [ bisectptr - 37 ] = 0 ; 
      } 
      if ( bisectstack [ bisectptr - 20 ] < 0 ) 
      if ( bisectstack [ bisectptr - 18 ] >= 0 ) 
      {
	if ( bisectstack [ bisectptr - 19 ] < 0 ) 
	bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] ; 
	else bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] ; 
	bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
	if ( bisectstack [ bisectptr - 16 ] < 0 ) 
	bisectstack [ bisectptr - 16 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
	if ( bisectstack [ bisectptr - 17 ] > bisectstack [ bisectptr - 20 ] ) 
	bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] ; 
	bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] ; 
	if ( bisectstack [ bisectptr - 16 ] < 0 ) 
	bisectstack [ bisectptr - 16 ] = 0 ; 
      } 
      else if ( bisectstack [ bisectptr - 18 ] <= 0 ) 
      {
	if ( bisectstack [ bisectptr - 19 ] > 0 ) 
	bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] ; 
	else bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] ; 
	bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
	if ( bisectstack [ bisectptr - 17 ] > 0 ) 
	bisectstack [ bisectptr - 17 ] = 0 ; 
      } 
      else {
	  
	bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] + bisectstack [ bisectptr - 18 ] ; 
	if ( bisectstack [ bisectptr - 16 ] < bisectstack [ bisectptr - 20 ] ) 
	bisectstack [ bisectptr - 16 ] = bisectstack [ bisectptr - 20 ] ; 
	bisectstack [ bisectptr - 17 ] = bisectstack [ bisectptr - 20 ] + 
	bisectstack [ bisectptr - 19 ] ; 
	if ( bisectstack [ bisectptr - 17 ] > 0 ) 
	bisectstack [ bisectptr - 17 ] = 0 ; 
      } 
      uv = bisectptr - 20 ; 
      xy = bisectptr - 20 ; 
      delx = delx + delx ; 
      dely = dely + dely ; 
      tol = tol - threel + tolstep ; 
      tol = tol + tol ; 
      threel = threel + tolstep ; 
      goto lab22 ; 
    } 
    if ( timetogo > 0 ) 
    decr ( timetogo ) ; 
    else {
	
      while ( apprt < 65536L ) {
	  
	apprt = apprt + apprt ; 
	apprtt = apprtt + apprtt ; 
      } 
      curt = apprt ; 
      curtt = apprtt ; 
      goto lab10 ; 
    } 
    lab45: if ( odd ( curtt ) ) 
    if ( odd ( curt ) ) 
    {
      curt = ( curt ) / 2 ; 
      curtt = ( curtt ) / 2 ; 
      if ( curt == 0 ) 
      goto lab10 ; 
      bisectptr = bisectptr - 45 ; 
      threel = threel - tolstep ; 
      delx = bisectstack [ bisectptr ] ; 
      dely = bisectstack [ bisectptr + 1 ] ; 
      tol = bisectstack [ bisectptr + 2 ] ; 
      uv = bisectstack [ bisectptr + 3 ] ; 
      xy = bisectstack [ bisectptr + 4 ] ; 
      goto lab45 ; 
    } 
    else {
	
      incr ( curt ) ; 
      delx = delx + bisectstack [ uv - 5 ] + bisectstack [ uv - 4 ] + 
      bisectstack [ uv - 3 ] ; 
      dely = dely + bisectstack [ uv - 10 ] + bisectstack [ uv - 9 ] + 
      bisectstack [ uv - 8 ] ; 
      uv = uv + 20 ; 
      decr ( curtt ) ; 
      xy = xy - 20 ; 
      delx = delx + bisectstack [ xy - 15 ] + bisectstack [ xy - 14 ] + 
      bisectstack [ xy - 13 ] ; 
      dely = dely + bisectstack [ xy - 20 ] + bisectstack [ xy - 19 ] + 
      bisectstack [ xy - 18 ] ; 
    } 
    else {
	
      incr ( curtt ) ; 
      tol = tol + threel ; 
      delx = delx - bisectstack [ xy - 15 ] - bisectstack [ xy - 14 ] - 
      bisectstack [ xy - 13 ] ; 
      dely = dely - bisectstack [ xy - 20 ] - bisectstack [ xy - 19 ] - 
      bisectstack [ xy - 18 ] ; 
      xy = xy + 20 ; 
    } 
  } 
  lab10: ; 
} 
