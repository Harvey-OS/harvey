#define EXTERN extern
#include "mfd.h"

void zpathintersection ( h , hh ) 
halfword h ; 
halfword hh ; 
{/* 10 */ halfword p, pp  ; 
  integer n, nn  ; 
  if ( mem [ h ] .hhfield .b1 == 0 ) 
  {
    mem [ h + 5 ] .cint = mem [ h + 1 ] .cint ; 
    mem [ h + 3 ] .cint = mem [ h + 1 ] .cint ; 
    mem [ h + 6 ] .cint = mem [ h + 2 ] .cint ; 
    mem [ h + 4 ] .cint = mem [ h + 2 ] .cint ; 
    mem [ h ] .hhfield .b1 = 1 ; 
  } 
  if ( mem [ hh ] .hhfield .b1 == 0 ) 
  {
    mem [ hh + 5 ] .cint = mem [ hh + 1 ] .cint ; 
    mem [ hh + 3 ] .cint = mem [ hh + 1 ] .cint ; 
    mem [ hh + 6 ] .cint = mem [ hh + 2 ] .cint ; 
    mem [ hh + 4 ] .cint = mem [ hh + 2 ] .cint ; 
    mem [ hh ] .hhfield .b1 = 1 ; 
  } 
  tolstep = 0 ; 
  do {
      n = -65536L ; 
    p = h ; 
    do {
	if ( mem [ p ] .hhfield .b1 != 0 ) 
      {
	nn = -65536L ; 
	pp = hh ; 
	do {
	    if ( mem [ pp ] .hhfield .b1 != 0 ) 
	  {
	    cubicintersection ( p , pp ) ; 
	    if ( curt > 0 ) 
	    {
	      curt = curt + n ; 
	      curtt = curtt + nn ; 
	      goto lab10 ; 
	    } 
	  } 
	  nn = nn + 65536L ; 
	  pp = mem [ pp ] .hhfield .v.RH ; 
	} while ( ! ( pp == hh ) ) ; 
      } 
      n = n + 65536L ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } while ( ! ( p == h ) ) ; 
    tolstep = tolstep + 3 ; 
  } while ( ! ( tolstep > 3 ) ) ; 
  curt = -65536L ; 
  curtt = -65536L ; 
  lab10: ; 
} 
void zopenawindow ( k , r0 , c0 , r1 , c1 , x , y ) 
windownumber k ; 
scaled r0 ; 
scaled c0 ; 
scaled r1 ; 
scaled c1 ; 
scaled x ; 
scaled y ; 
{integer m, n  ; 
  if ( r0 < 0 ) 
  r0 = 0 ; 
  else r0 = roundunscaled ( r0 ) ; 
  r1 = roundunscaled ( r1 ) ; 
  if ( r1 > screendepth ) 
  r1 = screendepth ; 
  if ( r1 < r0 ) 
  if ( r0 > screendepth ) 
  r0 = r1 ; 
  else r1 = r0 ; 
  if ( c0 < 0 ) 
  c0 = 0 ; 
  else c0 = roundunscaled ( c0 ) ; 
  c1 = roundunscaled ( c1 ) ; 
  if ( c1 > screenwidth ) 
  c1 = screenwidth ; 
  if ( c1 < c0 ) 
  if ( c0 > screenwidth ) 
  c0 = c1 ; 
  else c1 = c0 ; 
  windowopen [ k ] = true ; 
  incr ( windowtime [ k ] ) ; 
  leftcol [ k ] = c0 ; 
  rightcol [ k ] = c1 ; 
  toprow [ k ] = r0 ; 
  botrow [ k ] = r1 ; 
  m = roundunscaled ( x ) ; 
  n = roundunscaled ( y ) - 1 ; 
  mwindow [ k ] = c0 - m ; 
  nwindow [ k ] = r0 + n ; 
  {
    if ( ! screenstarted ) 
    {
      screenOK = initscreen () ; 
      screenstarted = true ; 
    } 
  } 
  if ( screenOK ) 
  {
    blankrectangle ( c0 , c1 , r0 , r1 ) ; 
    updatescreen () ; 
  } 
} 
void zdispedges ( k ) 
windownumber k ; 
{/* 30 40 */ halfword p, q  ; 
  boolean alreadythere  ; 
  integer r  ; 
  screencol n  ; 
  integer w, ww  ; 
  pixelcolor b  ; 
  integer m, mm  ; 
  integer d  ; 
  integer madjustment  ; 
  integer rightedge  ; 
  screencol mincol  ; 
  if ( screenOK ) 
  if ( leftcol [ k ] < rightcol [ k ] ) 
  if ( toprow [ k ] < botrow [ k ] ) 
  {
    alreadythere = false ; 
    if ( mem [ curedges + 3 ] .hhfield .v.RH == k ) 
    if ( mem [ curedges + 4 ] .cint == windowtime [ k ] ) 
    alreadythere = true ; 
    if ( ! alreadythere ) 
    blankrectangle ( leftcol [ k ] , rightcol [ k ] , toprow [ k ] , botrow [ 
    k ] ) ; 
    madjustment = mwindow [ k ] - mem [ curedges + 3 ] .hhfield .lhfield ; 
    rightedge = 8 * ( rightcol [ k ] - madjustment ) ; 
    mincol = leftcol [ k ] ; 
    p = mem [ curedges ] .hhfield .v.RH ; 
    r = nwindow [ k ] - ( mem [ curedges + 1 ] .hhfield .lhfield - 4096 ) ; 
    while ( ( p != curedges ) && ( r >= toprow [ k ] ) ) {
	
      if ( r < botrow [ k ] ) 
      {
	if ( mem [ p + 1 ] .hhfield .lhfield > 1 ) 
	sortedges ( p ) ; 
	else if ( mem [ p + 1 ] .hhfield .lhfield == 1 ) 
	if ( alreadythere ) 
	goto lab30 ; 
	mem [ p + 1 ] .hhfield .lhfield = 1 ; 
	n = 0 ; 
	ww = 0 ; 
	m = -1 ; 
	w = 0 ; 
	q = mem [ p + 1 ] .hhfield .v.RH ; 
	rowtransition [ 0 ] = mincol ; 
	while ( true ) {
	    
	  if ( q == memtop ) 
	  d = rightedge ; 
	  else d = mem [ q ] .hhfield .lhfield ; 
	  mm = ( d / 8 ) + madjustment ; 
	  if ( mm != m ) 
	  {
	    if ( w <= 0 ) 
	    {
	      if ( ww > 0 ) 
	      if ( m > mincol ) 
	      {
		if ( n == 0 ) 
		if ( alreadythere ) 
		{
		  b = 0 ; 
		  incr ( n ) ; 
		} 
		else b = 1 ; 
		else incr ( n ) ; 
		rowtransition [ n ] = m ; 
	      } 
	    } 
	    else if ( ww <= 0 ) 
	    if ( m > mincol ) 
	    {
	      if ( n == 0 ) 
	      b = 1 ; 
	      incr ( n ) ; 
	      rowtransition [ n ] = m ; 
	    } 
	    m = mm ; 
	    w = ww ; 
	  } 
	  if ( d >= rightedge ) 
	  goto lab40 ; 
	  ww = ww + ( d % 8 ) - 4 ; 
	  q = mem [ q ] .hhfield .v.RH ; 
	} 
	lab40: if ( alreadythere || ( ww > 0 ) ) 
	{
	  if ( n == 0 ) 
	  if ( ww > 0 ) 
	  b = 1 ; 
	  else b = 0 ; 
	  incr ( n ) ; 
	  rowtransition [ n ] = rightcol [ k ] ; 
	} 
	else if ( n == 0 ) 
	goto lab30 ; 
	paintrow ( r , b , rowtransition , n ) ; 
	lab30: ; 
      } 
      p = mem [ p ] .hhfield .v.RH ; 
      decr ( r ) ; 
    } 
    updatescreen () ; 
    incr ( windowtime [ k ] ) ; 
    mem [ curedges + 3 ] .hhfield .v.RH = k ; 
    mem [ curedges + 4 ] .cint = windowtime [ k ] ; 
  } 
} 
fraction zmaxcoef ( p ) 
halfword p ; 
{register fraction Result; fraction x  ; 
  x = 0 ; 
  while ( mem [ p ] .hhfield .lhfield != 0 ) {
      
    if ( abs ( mem [ p + 1 ] .cint ) > x ) 
    x = abs ( mem [ p + 1 ] .cint ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  Result = x ; 
  return(Result) ; 
} 
halfword zpplusq ( p , q , t ) 
halfword p ; 
halfword q ; 
smallnumber t ; 
{/* 30 */ register halfword Result; halfword pp, qq  ; 
  halfword r, s  ; 
  integer threshold  ; 
  integer v  ; 
  if ( t == 17 ) 
  threshold = 2685 ; 
  else threshold = 8 ; 
  r = memtop - 1 ; 
  pp = mem [ p ] .hhfield .lhfield ; 
  qq = mem [ q ] .hhfield .lhfield ; 
  while ( true ) if ( pp == qq ) 
  if ( pp == 0 ) 
  goto lab30 ; 
  else {
      
    v = mem [ p + 1 ] .cint + mem [ q + 1 ] .cint ; 
    mem [ p + 1 ] .cint = v ; 
    s = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
    pp = mem [ p ] .hhfield .lhfield ; 
    if ( abs ( v ) < threshold ) 
    freenode ( s , 2 ) ; 
    else {
	
      if ( abs ( v ) >= 626349397L ) 
      if ( watchcoefs ) 
      {
	mem [ qq ] .hhfield .b0 = 0 ; 
	fixneeded = true ; 
      } 
      mem [ r ] .hhfield .v.RH = s ; 
      r = s ; 
    } 
    q = mem [ q ] .hhfield .v.RH ; 
    qq = mem [ q ] .hhfield .lhfield ; 
  } 
  else if ( mem [ pp + 1 ] .cint < mem [ qq + 1 ] .cint ) 
  {
    s = getnode ( 2 ) ; 
    mem [ s ] .hhfield .lhfield = qq ; 
    mem [ s + 1 ] .cint = mem [ q + 1 ] .cint ; 
    q = mem [ q ] .hhfield .v.RH ; 
    qq = mem [ q ] .hhfield .lhfield ; 
    mem [ r ] .hhfield .v.RH = s ; 
    r = s ; 
  } 
  else {
      
    mem [ r ] .hhfield .v.RH = p ; 
    r = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
    pp = mem [ p ] .hhfield .lhfield ; 
  } 
  lab30: mem [ p + 1 ] .cint = slowadd ( mem [ p + 1 ] .cint , mem [ q + 1 ] 
  .cint ) ; 
  mem [ r ] .hhfield .v.RH = p ; 
  depfinal = p ; 
  Result = mem [ memtop - 1 ] .hhfield .v.RH ; 
  return(Result) ; 
} 
halfword zptimesv ( p , v , t0 , t1 , visscaled ) 
halfword p ; 
integer v ; 
smallnumber t0 ; 
smallnumber t1 ; 
boolean visscaled ; 
{register halfword Result; halfword r, s  ; 
  integer w  ; 
  integer threshold  ; 
  boolean scalingdown  ; 
  if ( t0 != t1 ) 
  scalingdown = true ; 
  else scalingdown = ! visscaled ; 
  if ( t1 == 17 ) 
  threshold = 1342 ; 
  else threshold = 4 ; 
  r = memtop - 1 ; 
  while ( mem [ p ] .hhfield .lhfield != 0 ) {
      
    if ( scalingdown ) 
    w = takefraction ( v , mem [ p + 1 ] .cint ) ; 
    else w = takescaled ( v , mem [ p + 1 ] .cint ) ; 
    if ( abs ( w ) <= threshold ) 
    {
      s = mem [ p ] .hhfield .v.RH ; 
      freenode ( p , 2 ) ; 
      p = s ; 
    } 
    else {
	
      if ( abs ( w ) >= 626349397L ) 
      {
	fixneeded = true ; 
	mem [ mem [ p ] .hhfield .lhfield ] .hhfield .b0 = 0 ; 
      } 
      mem [ r ] .hhfield .v.RH = p ; 
      r = p ; 
      mem [ p + 1 ] .cint = w ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } 
  } 
  mem [ r ] .hhfield .v.RH = p ; 
  if ( visscaled ) 
  mem [ p + 1 ] .cint = takescaled ( mem [ p + 1 ] .cint , v ) ; 
  else mem [ p + 1 ] .cint = takefraction ( mem [ p + 1 ] .cint , v ) ; 
  Result = mem [ memtop - 1 ] .hhfield .v.RH ; 
  return(Result) ; 
} 
halfword zpwithxbecomingq ( p , x , q , t ) 
halfword p ; 
halfword x ; 
halfword q ; 
smallnumber t ; 
{register halfword Result; halfword r, s  ; 
  integer v  ; 
  integer sx  ; 
  s = p ; 
  r = memtop - 1 ; 
  sx = mem [ x + 1 ] .cint ; 
  while ( mem [ mem [ s ] .hhfield .lhfield + 1 ] .cint > sx ) {
      
    r = s ; 
    s = mem [ s ] .hhfield .v.RH ; 
  } 
  if ( mem [ s ] .hhfield .lhfield != x ) 
  Result = p ; 
  else {
      
    mem [ memtop - 1 ] .hhfield .v.RH = p ; 
    mem [ r ] .hhfield .v.RH = mem [ s ] .hhfield .v.RH ; 
    v = mem [ s + 1 ] .cint ; 
    freenode ( s , 2 ) ; 
    Result = pplusfq ( mem [ memtop - 1 ] .hhfield .v.RH , v , q , t , 17 ) ; 
  } 
  return(Result) ; 
} 
void znewdep ( q , p ) 
halfword q ; 
halfword p ; 
{halfword r  ; 
  mem [ q + 1 ] .hhfield .v.RH = p ; 
  mem [ q + 1 ] .hhfield .lhfield = 13 ; 
  r = mem [ 13 ] .hhfield .v.RH ; 
  mem [ depfinal ] .hhfield .v.RH = r ; 
  mem [ r + 1 ] .hhfield .lhfield = depfinal ; 
  mem [ 13 ] .hhfield .v.RH = q ; 
} 
halfword zconstdependency ( v ) 
scaled v ; 
{register halfword Result; depfinal = getnode ( 2 ) ; 
  mem [ depfinal + 1 ] .cint = v ; 
  mem [ depfinal ] .hhfield .lhfield = 0 ; 
  Result = depfinal ; 
  return(Result) ; 
} 
halfword zsingledependency ( p ) 
halfword p ; 
{register halfword Result; halfword q  ; 
  integer m  ; 
  m = mem [ p + 1 ] .cint % 64 ; 
  if ( m > 28 ) 
  Result = constdependency ( 0 ) ; 
  else {
      
    q = getnode ( 2 ) ; 
    mem [ q + 1 ] .cint = twotothe [ 28 - m ] ; 
    mem [ q ] .hhfield .lhfield = p ; 
    mem [ q ] .hhfield .v.RH = constdependency ( 0 ) ; 
    Result = q ; 
  } 
  return(Result) ; 
} 
halfword zcopydeplist ( p ) 
halfword p ; 
{/* 30 */ register halfword Result; halfword q  ; 
  q = getnode ( 2 ) ; 
  depfinal = q ; 
  while ( true ) {
      
    mem [ depfinal ] .hhfield .lhfield = mem [ p ] .hhfield .lhfield ; 
    mem [ depfinal + 1 ] .cint = mem [ p + 1 ] .cint ; 
    if ( mem [ depfinal ] .hhfield .lhfield == 0 ) 
    goto lab30 ; 
    mem [ depfinal ] .hhfield .v.RH = getnode ( 2 ) ; 
    depfinal = mem [ depfinal ] .hhfield .v.RH ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  lab30: Result = q ; 
  return(Result) ; 
} 
void zlineareq ( p , t ) 
halfword p ; 
smallnumber t ; 
{halfword q, r, s  ; 
  halfword x  ; 
  integer n  ; 
  integer v  ; 
  halfword prevr  ; 
  halfword finalnode  ; 
  integer w  ; 
  q = p ; 
  r = mem [ p ] .hhfield .v.RH ; 
  v = mem [ q + 1 ] .cint ; 
  while ( mem [ r ] .hhfield .lhfield != 0 ) {
      
    if ( abs ( mem [ r + 1 ] .cint ) > abs ( v ) ) 
    {
      q = r ; 
      v = mem [ r + 1 ] .cint ; 
    } 
    r = mem [ r ] .hhfield .v.RH ; 
  } 
  x = mem [ q ] .hhfield .lhfield ; 
  n = mem [ x + 1 ] .cint % 64 ; 
  s = memtop - 1 ; 
  mem [ s ] .hhfield .v.RH = p ; 
  r = p ; 
  do {
      if ( r == q ) 
    {
      mem [ s ] .hhfield .v.RH = mem [ r ] .hhfield .v.RH ; 
      freenode ( r , 2 ) ; 
    } 
    else {
	
      w = makefraction ( mem [ r + 1 ] .cint , v ) ; 
      if ( abs ( w ) <= 1342 ) 
      {
	mem [ s ] .hhfield .v.RH = mem [ r ] .hhfield .v.RH ; 
	freenode ( r , 2 ) ; 
      } 
      else {
	  
	mem [ r + 1 ] .cint = - (integer) w ; 
	s = r ; 
      } 
    } 
    r = mem [ s ] .hhfield .v.RH ; 
  } while ( ! ( mem [ r ] .hhfield .lhfield == 0 ) ) ; 
  if ( t == 18 ) 
  mem [ r + 1 ] .cint = - (integer) makescaled ( mem [ r + 1 ] .cint , v ) ; 
  else if ( v != -268435456L ) 
  mem [ r + 1 ] .cint = - (integer) makefraction ( mem [ r + 1 ] .cint , v ) ; 
  finalnode = r ; 
  p = mem [ memtop - 1 ] .hhfield .v.RH ; 
  if ( internal [ 2 ] > 0 ) 
  if ( interesting ( x ) ) 
  {
    begindiagnostic () ; 
    printnl ( 594 ) ; 
    printvariablename ( x ) ; 
    w = n ; 
    while ( w > 0 ) {
	
      print ( 587 ) ; 
      w = w - 2 ; 
    } 
    printchar ( 61 ) ; 
    printdependency ( p , 17 ) ; 
    enddiagnostic ( false ) ; 
  } 
  prevr = 13 ; 
  r = mem [ 13 ] .hhfield .v.RH ; 
  while ( r != 13 ) {
      
    s = mem [ r + 1 ] .hhfield .v.RH ; 
    q = pwithxbecomingq ( s , x , p , mem [ r ] .hhfield .b0 ) ; 
    if ( mem [ q ] .hhfield .lhfield == 0 ) 
    makeknown ( r , q ) ; 
    else {
	
      mem [ r + 1 ] .hhfield .v.RH = q ; 
      do {
	  q = mem [ q ] .hhfield .v.RH ; 
      } while ( ! ( mem [ q ] .hhfield .lhfield == 0 ) ) ; 
      prevr = q ; 
    } 
    r = mem [ prevr ] .hhfield .v.RH ; 
  } 
  if ( n > 0 ) 
  {
    s = memtop - 1 ; 
    mem [ memtop - 1 ] .hhfield .v.RH = p ; 
    r = p ; 
    do {
	if ( n > 30 ) 
      w = 0 ; 
      else w = mem [ r + 1 ] .cint / twotothe [ n ] ; 
      if ( ( abs ( w ) <= 1342 ) && ( mem [ r ] .hhfield .lhfield != 0 ) ) 
      {
	mem [ s ] .hhfield .v.RH = mem [ r ] .hhfield .v.RH ; 
	freenode ( r , 2 ) ; 
      } 
      else {
	  
	mem [ r + 1 ] .cint = w ; 
	s = r ; 
      } 
      r = mem [ s ] .hhfield .v.RH ; 
    } while ( ! ( mem [ s ] .hhfield .lhfield == 0 ) ) ; 
    p = mem [ memtop - 1 ] .hhfield .v.RH ; 
  } 
  if ( mem [ p ] .hhfield .lhfield == 0 ) 
  {
    mem [ x ] .hhfield .b0 = 16 ; 
    mem [ x + 1 ] .cint = mem [ p + 1 ] .cint ; 
    if ( abs ( mem [ x + 1 ] .cint ) >= 268435456L ) 
    valtoobig ( mem [ x + 1 ] .cint ) ; 
    freenode ( p , 2 ) ; 
    if ( curexp == x ) 
    if ( curtype == 19 ) 
    {
      curexp = mem [ x + 1 ] .cint ; 
      curtype = 16 ; 
      freenode ( x , 2 ) ; 
    } 
  } 
  else {
      
    mem [ x ] .hhfield .b0 = 17 ; 
    depfinal = finalnode ; 
    newdep ( x , p ) ; 
    if ( curexp == x ) 
    if ( curtype == 19 ) 
    curtype = 17 ; 
  } 
  if ( fixneeded ) 
  fixdependencies () ; 
} 
halfword znewringentry ( p ) 
halfword p ; 
{register halfword Result; halfword q  ; 
  q = getnode ( 2 ) ; 
  mem [ q ] .hhfield .b1 = 11 ; 
  mem [ q ] .hhfield .b0 = mem [ p ] .hhfield .b0 ; 
  if ( mem [ p + 1 ] .cint == 0 ) 
  mem [ q + 1 ] .cint = p ; 
  else mem [ q + 1 ] .cint = mem [ p + 1 ] .cint ; 
  mem [ p + 1 ] .cint = q ; 
  Result = q ; 
  return(Result) ; 
} 
void znonlineareq ( v , p , flushp ) 
integer v ; 
halfword p ; 
boolean flushp ; 
{smallnumber t  ; 
  halfword q, r  ; 
  t = mem [ p ] .hhfield .b0 - 1 ; 
  q = mem [ p + 1 ] .cint ; 
  if ( flushp ) 
  mem [ p ] .hhfield .b0 = 1 ; 
  else p = q ; 
  do {
      r = mem [ q + 1 ] .cint ; 
    mem [ q ] .hhfield .b0 = t ; 
    switch ( t ) 
    {case 2 : 
      mem [ q + 1 ] .cint = v ; 
      break ; 
    case 4 : 
      {
	mem [ q + 1 ] .cint = v ; 
	{
	  if ( strref [ v ] < 127 ) 
	  incr ( strref [ v ] ) ; 
	} 
      } 
      break ; 
    case 6 : 
      {
	mem [ q + 1 ] .cint = v ; 
	incr ( mem [ v ] .hhfield .lhfield ) ; 
      } 
      break ; 
    case 9 : 
      mem [ q + 1 ] .cint = copypath ( v ) ; 
      break ; 
    case 11 : 
      mem [ q + 1 ] .cint = copyedges ( v ) ; 
      break ; 
    } 
    q = r ; 
  } while ( ! ( q == p ) ) ; 
} 
void zringmerge ( p , q ) 
halfword p ; 
halfword q ; 
{/* 10 */ halfword r  ; 
  r = mem [ p + 1 ] .cint ; 
  while ( r != p ) {
      
    if ( r == q ) 
    {
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 597 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 598 ; 
	  helpline [ 0 ] = 599 ; 
	} 
	putgeterror () ; 
      } 
      goto lab10 ; 
    } 
    r = mem [ r + 1 ] .cint ; 
  } 
  r = mem [ p + 1 ] .cint ; 
  mem [ p + 1 ] .cint = mem [ q + 1 ] .cint ; 
  mem [ q + 1 ] .cint = r ; 
  lab10: ; 
} 
void zshowcmdmod ( c , m ) 
integer c ; 
integer m ; 
{begindiagnostic () ; 
  printnl ( 123 ) ; 
  printcmdmod ( c , m ) ; 
  printchar ( 125 ) ; 
  enddiagnostic ( false ) ; 
} 
void showcontext ( ) 
{/* 30 */ schar oldsetting  ; 
  integer i  ; 
  integer l  ; 
  integer m  ; 
  integer n  ; 
  integer p  ; 
  integer q  ; 
  fileptr = inputptr ; 
  inputstack [ fileptr ] = curinput ; 
  while ( true ) {
      
    curinput = inputstack [ fileptr ] ; 
    if ( ( fileptr == inputptr ) || ( curinput .indexfield <= 15 ) || ( 
    curinput .indexfield != 19 ) || ( curinput .locfield != 0 ) ) 
    {
      tally = 0 ; 
      oldsetting = selector ; 
      if ( ( curinput .indexfield <= 15 ) ) 
      {
	if ( curinput .namefield <= 1 ) 
	if ( ( curinput .namefield == 0 ) && ( fileptr == 0 ) ) 
	printnl ( 601 ) ; 
	else printnl ( 602 ) ; 
	else if ( curinput .namefield == 2 ) 
	printnl ( 603 ) ; 
	else {
	    
	  printnl ( 604 ) ; 
	  printint ( line ) ; 
	} 
	printchar ( 32 ) ; 
	{
	  l = tally ; 
	  tally = 0 ; 
	  selector = 4 ; 
	  trickcount = 1000000L ; 
	} 
	if ( curinput .limitfield > 0 ) 
	{register integer for_end; i = curinput .startfield ; for_end = 
	curinput .limitfield - 1 ; if ( i <= for_end) do 
	  {
	    if ( i == curinput .locfield ) 
	    {
	      firstcount = tally ; 
	      trickcount = tally + 1 + errorline - halferrorline ; 
	      if ( trickcount < errorline ) 
	      trickcount = errorline ; 
	    } 
	    print ( buffer [ i ] ) ; 
	  } 
	while ( i++ < for_end ) ; } 
      } 
      else {
	  
	switch ( curinput .indexfield ) 
	{case 16 : 
	  printnl ( 605 ) ; 
	  break ; 
	case 17 : 
	  {
	    printnl ( 610 ) ; 
	    p = paramstack [ curinput .limitfield ] ; 
	    if ( p != 0 ) 
	    if ( mem [ p ] .hhfield .v.RH == 1 ) 
	    printexp ( p , 0 ) ; 
	    else showtokenlist ( p , 0 , 20 , tally ) ; 
	    print ( 611 ) ; 
	  } 
	  break ; 
	case 18 : 
	  printnl ( 606 ) ; 
	  break ; 
	case 19 : 
	  if ( curinput .locfield == 0 ) 
	  printnl ( 607 ) ; 
	  else printnl ( 608 ) ; 
	  break ; 
	case 20 : 
	  printnl ( 609 ) ; 
	  break ; 
	case 21 : 
	  {
	    println () ; 
	    if ( curinput .namefield != 0 ) 
	    slowprint ( hash [ curinput .namefield ] .v.RH ) ; 
	    else {
		
	      p = paramstack [ curinput .limitfield ] ; 
	      if ( p == 0 ) 
	      showtokenlist ( paramstack [ curinput .limitfield + 1 ] , 0 , 20 
	      , tally ) ; 
	      else {
		  
		q = p ; 
		while ( mem [ q ] .hhfield .v.RH != 0 ) q = mem [ q ] .hhfield 
		.v.RH ; 
		mem [ q ] .hhfield .v.RH = paramstack [ curinput .limitfield + 
		1 ] ; 
		showtokenlist ( p , 0 , 20 , tally ) ; 
		mem [ q ] .hhfield .v.RH = 0 ; 
	      } 
	    } 
	    print ( 500 ) ; 
	  } 
	  break ; 
	  default: 
	  printnl ( 63 ) ; 
	  break ; 
	} 
	{
	  l = tally ; 
	  tally = 0 ; 
	  selector = 4 ; 
	  trickcount = 1000000L ; 
	} 
	if ( curinput .indexfield != 21 ) 
	showtokenlist ( curinput .startfield , curinput .locfield , 100000L , 
	0 ) ; 
	else showmacro ( curinput .startfield , curinput .locfield , 100000L ) 
	; 
      } 
      selector = oldsetting ; 
      if ( trickcount == 1000000L ) 
      {
	firstcount = tally ; 
	trickcount = tally + 1 + errorline - halferrorline ; 
	if ( trickcount < errorline ) 
	trickcount = errorline ; 
      } 
      if ( tally < trickcount ) 
      m = tally - firstcount ; 
      else m = trickcount - firstcount ; 
      if ( l + firstcount <= halferrorline ) 
      {
	p = 0 ; 
	n = l + firstcount ; 
      } 
      else {
	  
	print ( 274 ) ; 
	p = l + firstcount - halferrorline + 3 ; 
	n = halferrorline ; 
      } 
      {register integer for_end; q = p ; for_end = firstcount - 1 ; if ( q <= 
      for_end) do 
	printchar ( trickbuf [ q % errorline ] ) ; 
      while ( q++ < for_end ) ; } 
      println () ; 
      {register integer for_end; q = 1 ; for_end = n ; if ( q <= for_end) do 
	printchar ( 32 ) ; 
      while ( q++ < for_end ) ; } 
      if ( m + n <= errorline ) 
      p = firstcount + m ; 
      else p = firstcount + ( errorline - n - 3 ) ; 
      {register integer for_end; q = firstcount ; for_end = p - 1 ; if ( q <= 
      for_end) do 
	printchar ( trickbuf [ q % errorline ] ) ; 
      while ( q++ < for_end ) ; } 
      if ( m + n > errorline ) 
      print ( 274 ) ; 
    } 
    if ( ( curinput .indexfield <= 15 ) ) 
    if ( ( curinput .namefield > 2 ) || ( fileptr == 0 ) ) 
    goto lab30 ; 
    decr ( fileptr ) ; 
  } 
  lab30: curinput = inputstack [ inputptr ] ; 
} 
void zbegintokenlist ( p , t ) 
halfword p ; 
quarterword t ; 
{{
    
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ; 
      if ( inputptr == stacksize ) 
      overflow ( 612 , stacksize ) ; 
    } 
    inputstack [ inputptr ] = curinput ; 
    incr ( inputptr ) ; 
  } 
  curinput .startfield = p ; 
  curinput .indexfield = t ; 
  curinput .limitfield = paramptr ; 
  curinput .locfield = p ; 
} 
void endtokenlist ( ) 
{/* 30 */ halfword p  ; 
  if ( curinput .indexfield >= 19 ) 
  if ( curinput .indexfield <= 20 ) 
  {
    flushtokenlist ( curinput .startfield ) ; 
    goto lab30 ; 
  } 
  else deletemacref ( curinput .startfield ) ; 
  while ( paramptr > curinput .limitfield ) {
      
    decr ( paramptr ) ; 
    p = paramstack [ paramptr ] ; 
    if ( p != 0 ) 
    if ( mem [ p ] .hhfield .v.RH == 1 ) 
    {
      recyclevalue ( p ) ; 
      freenode ( p , 2 ) ; 
    } 
    else flushtokenlist ( p ) ; 
  } 
  lab30: {
      
    decr ( inputptr ) ; 
    curinput = inputstack [ inputptr ] ; 
  } 
  {
    if ( interrupt != 0 ) 
    pauseforinstructions () ; 
  } 
} 
void zencapsulate ( p ) 
halfword p ; 
{curexp = getnode ( 2 ) ; 
  mem [ curexp ] .hhfield .b0 = curtype ; 
  mem [ curexp ] .hhfield .b1 = 11 ; 
  newdep ( curexp , p ) ; 
} 
void zinstall ( r , q ) 
halfword r ; 
halfword q ; 
{halfword p  ; 
  if ( mem [ q ] .hhfield .b0 == 16 ) 
  {
    mem [ r + 1 ] .cint = mem [ q + 1 ] .cint ; 
    mem [ r ] .hhfield .b0 = 16 ; 
  } 
  else if ( mem [ q ] .hhfield .b0 == 19 ) 
  {
    p = singledependency ( q ) ; 
    if ( p == depfinal ) 
    {
      mem [ r ] .hhfield .b0 = 16 ; 
      mem [ r + 1 ] .cint = 0 ; 
      freenode ( p , 2 ) ; 
    } 
    else {
	
      mem [ r ] .hhfield .b0 = 17 ; 
      newdep ( r , p ) ; 
    } 
  } 
  else {
      
    mem [ r ] .hhfield .b0 = mem [ q ] .hhfield .b0 ; 
    newdep ( r , copydeplist ( mem [ q + 1 ] .hhfield .v.RH ) ) ; 
  } 
} 
void zmakeexpcopy ( p ) 
halfword p ; 
{/* 20 */ halfword q, r, t  ; 
  lab20: curtype = mem [ p ] .hhfield .b0 ; 
  switch ( curtype ) 
  {case 1 : 
  case 2 : 
  case 16 : 
    curexp = mem [ p + 1 ] .cint ; 
    break ; 
  case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
    curexp = newringentry ( p ) ; 
    break ; 
  case 4 : 
    {
      curexp = mem [ p + 1 ] .cint ; 
      {
	if ( strref [ curexp ] < 127 ) 
	incr ( strref [ curexp ] ) ; 
      } 
    } 
    break ; 
  case 6 : 
    {
      curexp = mem [ p + 1 ] .cint ; 
      incr ( mem [ curexp ] .hhfield .lhfield ) ; 
    } 
    break ; 
  case 11 : 
    curexp = copyedges ( mem [ p + 1 ] .cint ) ; 
    break ; 
  case 9 : 
  case 8 : 
    curexp = copypath ( mem [ p + 1 ] .cint ) ; 
    break ; 
  case 13 : 
  case 14 : 
    {
      if ( mem [ p + 1 ] .cint == 0 ) 
      initbignode ( p ) ; 
      t = getnode ( 2 ) ; 
      mem [ t ] .hhfield .b1 = 11 ; 
      mem [ t ] .hhfield .b0 = curtype ; 
      initbignode ( t ) ; 
      q = mem [ p + 1 ] .cint + bignodesize [ curtype ] ; 
      r = mem [ t + 1 ] .cint + bignodesize [ curtype ] ; 
      do {
	  q = q - 2 ; 
	r = r - 2 ; 
	install ( r , q ) ; 
      } while ( ! ( q == mem [ p + 1 ] .cint ) ) ; 
      curexp = t ; 
    } 
    break ; 
  case 17 : 
  case 18 : 
    encapsulate ( copydeplist ( mem [ p + 1 ] .hhfield .v.RH ) ) ; 
    break ; 
  case 15 : 
    {
      {
	mem [ p ] .hhfield .b0 = 19 ; 
	serialno = serialno + 64 ; 
	mem [ p + 1 ] .cint = serialno ; 
      } 
      goto lab20 ; 
    } 
    break ; 
  case 19 : 
    {
      q = singledependency ( p ) ; 
      if ( q == depfinal ) 
      {
	curtype = 16 ; 
	curexp = 0 ; 
	freenode ( q , 2 ) ; 
      } 
      else {
	  
	curtype = 17 ; 
	encapsulate ( q ) ; 
      } 
    } 
    break ; 
    default: 
    confusion ( 797 ) ; 
    break ; 
  } 
} 
halfword curtok ( ) 
{register halfword Result; halfword p  ; 
  smallnumber savetype  ; 
  integer saveexp  ; 
  if ( cursym == 0 ) 
  if ( curcmd == 38 ) 
  {
    savetype = curtype ; 
    saveexp = curexp ; 
    makeexpcopy ( curmod ) ; 
    p = stashcurexp () ; 
    mem [ p ] .hhfield .v.RH = 0 ; 
    curtype = savetype ; 
    curexp = saveexp ; 
  } 
  else {
      
    p = getnode ( 2 ) ; 
    mem [ p + 1 ] .cint = curmod ; 
    mem [ p ] .hhfield .b1 = 12 ; 
    if ( curcmd == 42 ) 
    mem [ p ] .hhfield .b0 = 16 ; 
    else mem [ p ] .hhfield .b0 = 4 ; 
  } 
  else {
      
    {
      p = avail ; 
      if ( p == 0 ) 
      p = getavail () ; 
      else {
	  
	avail = mem [ p ] .hhfield .v.RH ; 
	mem [ p ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
	incr ( dynused ) ; 
#endif /* STAT */
      } 
    } 
    mem [ p ] .hhfield .lhfield = cursym ; 
  } 
  Result = p ; 
  return(Result) ; 
} 
void backinput ( ) 
{halfword p  ; 
  p = curtok () ; 
  while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ; 
  begintokenlist ( p , 19 ) ; 
} 
void backerror ( ) 
{OKtointerrupt = false ; 
  backinput () ; 
  OKtointerrupt = true ; 
  error () ; 
} 
void inserror ( ) 
{OKtointerrupt = false ; 
  backinput () ; 
  curinput .indexfield = 20 ; 
  OKtointerrupt = true ; 
  error () ; 
} 
void beginfilereading ( ) 
{if ( inopen == 15 ) 
  overflow ( 613 , 15 ) ; 
  if ( first == bufsize ) 
  overflow ( 256 , bufsize ) ; 
  incr ( inopen ) ; 
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ; 
      if ( inputptr == stacksize ) 
      overflow ( 612 , stacksize ) ; 
    } 
    inputstack [ inputptr ] = curinput ; 
    incr ( inputptr ) ; 
  } 
  curinput .indexfield = inopen ; 
  linestack [ curinput .indexfield ] = line ; 
  curinput .startfield = first ; 
  curinput .namefield = 0 ; 
} 
void endfilereading ( ) 
{first = curinput .startfield ; 
  line = linestack [ curinput .indexfield ] ; 
  if ( curinput .indexfield != inopen ) 
  confusion ( 614 ) ; 
  if ( curinput .namefield > 2 ) 
  aclose ( inputfile [ curinput .indexfield ] ) ; 
  {
    decr ( inputptr ) ; 
    curinput = inputstack [ inputptr ] ; 
  } 
  decr ( inopen ) ; 
} 
void clearforerrorprompt ( ) 
{while ( ( curinput .indexfield <= 15 ) && ( curinput .namefield == 0 ) && ( 
  inputptr > 0 ) && ( curinput .locfield == curinput .limitfield ) ) 
  endfilereading () ; 
  println () ; 
} 
boolean checkoutervalidity ( ) 
{register boolean Result; halfword p  ; 
  if ( scannerstatus == 0 ) 
  Result = true ; 
  else {
      
    deletionsallowed = false ; 
    if ( cursym != 0 ) 
    {
      p = getavail () ; 
      mem [ p ] .hhfield .lhfield = cursym ; 
      begintokenlist ( p , 19 ) ; 
    } 
    if ( scannerstatus > 1 ) 
    {
      runaway () ; 
      if ( cursym == 0 ) 
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 620 ) ; 
      } 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 621 ) ; 
	} 
      } 
      print ( 622 ) ; 
      {
	helpptr = 4 ; 
	helpline [ 3 ] = 623 ; 
	helpline [ 2 ] = 624 ; 
	helpline [ 1 ] = 625 ; 
	helpline [ 0 ] = 626 ; 
      } 
      switch ( scannerstatus ) 
      {case 2 : 
	{
	  print ( 627 ) ; 
	  helpline [ 3 ] = 628 ; 
	  cursym = 9763 ; 
	} 
	break ; 
      case 3 : 
	{
	  print ( 629 ) ; 
	  helpline [ 3 ] = 630 ; 
	  if ( warninginfo == 0 ) 
	  cursym = 9767 ; 
	  else {
	      
	    cursym = 9759 ; 
	    eqtb [ 9759 ] .v.RH = warninginfo ; 
	  } 
	} 
	break ; 
      case 4 : 
      case 5 : 
	{
	  print ( 631 ) ; 
	  if ( scannerstatus == 5 ) 
	  slowprint ( hash [ warninginfo ] .v.RH ) ; 
	  else printvariablename ( warninginfo ) ; 
	  cursym = 9765 ; 
	} 
	break ; 
      case 6 : 
	{
	  print ( 632 ) ; 
	  slowprint ( hash [ warninginfo ] .v.RH ) ; 
	  print ( 633 ) ; 
	  helpline [ 3 ] = 634 ; 
	  cursym = 9764 ; 
	} 
	break ; 
      } 
      inserror () ; 
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 615 ) ; 
      } 
      printint ( warninginfo ) ; 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 616 ; 
	helpline [ 1 ] = 617 ; 
	helpline [ 0 ] = 618 ; 
      } 
      if ( cursym == 0 ) 
      helpline [ 2 ] = 619 ; 
      cursym = 9766 ; 
      inserror () ; 
    } 
    deletionsallowed = true ; 
    Result = false ; 
  } 
  return(Result) ; 
} 
void getnext ( ) 
{/* 20 10 40 25 85 86 87 30 */ integer k  ; 
  ASCIIcode c  ; 
  ASCIIcode class  ; 
  integer n, f  ; 
  lab20: cursym = 0 ; 
  if ( ( curinput .indexfield <= 15 ) ) 
  {
    lab25: c = buffer [ curinput .locfield ] ; 
    incr ( curinput .locfield ) ; 
    class = charclass [ c ] ; 
    switch ( class ) 
    {case 0 : 
      goto lab85 ; 
      break ; 
    case 1 : 
      {
	class = charclass [ buffer [ curinput .locfield ] ] ; 
	if ( class > 1 ) 
	goto lab25 ; 
	else if ( class < 1 ) 
	{
	  n = 0 ; 
	  goto lab86 ; 
	} 
      } 
      break ; 
    case 2 : 
      goto lab25 ; 
      break ; 
    case 3 : 
      {
	if ( curinput .namefield > 2 ) 
	{
	  incr ( line ) ; 
	  first = curinput .startfield ; 
	  if ( ! forceeof ) 
	  {
	    if ( inputln ( inputfile [ curinput .indexfield ] , true ) ) 
	    firmuptheline () ; 
	    else forceeof = true ; 
	  } 
	  if ( forceeof ) 
	  {
	    printchar ( 41 ) ; 
	    decr ( openparens ) ; 
	    flush ( stdout ) ; 
	    forceeof = false ; 
	    endfilereading () ; 
	    if ( checkoutervalidity () ) 
	    goto lab20 ; 
	    else goto lab20 ; 
	  } 
	  buffer [ curinput .limitfield ] = 37 ; 
	  first = curinput .limitfield + 1 ; 
	  curinput .locfield = curinput .startfield ; 
	} 
	else {
	    
	  if ( inputptr > 0 ) 
	  {
	    endfilereading () ; 
	    goto lab20 ; 
	  } 
	  if ( selector < 2 ) 
	  openlogfile () ; 
	  if ( interaction > 1 ) 
	  {
	    if ( curinput .limitfield == curinput .startfield ) 
	    printnl ( 649 ) ; 
	    println () ; 
	    first = curinput .startfield ; 
	    {
	      ; 
	      print ( 42 ) ; 
	      terminput () ; 
	    } 
	    curinput .limitfield = last ; 
	    buffer [ curinput .limitfield ] = 37 ; 
	    first = curinput .limitfield + 1 ; 
	    curinput .locfield = curinput .startfield ; 
	  } 
	  else fatalerror ( 650 ) ; 
	} 
	{
	  if ( interrupt != 0 ) 
	  pauseforinstructions () ; 
	} 
	goto lab25 ; 
      } 
      break ; 
    case 4 : 
      {
	if ( buffer [ curinput .locfield ] == 34 ) 
	curmod = 283 ; 
	else {
	    
	  k = curinput .locfield ; 
	  buffer [ curinput .limitfield + 1 ] = 34 ; 
	  do {
	      incr ( curinput .locfield ) ; 
	  } while ( ! ( buffer [ curinput .locfield ] == 34 ) ) ; 
	  if ( curinput .locfield > curinput .limitfield ) 
	  {
	    curinput .locfield = curinput .limitfield ; 
	    {
	      if ( interaction == 3 ) 
	      ; 
	      printnl ( 261 ) ; 
	      print ( 642 ) ; 
	    } 
	    {
	      helpptr = 3 ; 
	      helpline [ 2 ] = 643 ; 
	      helpline [ 1 ] = 644 ; 
	      helpline [ 0 ] = 645 ; 
	    } 
	    deletionsallowed = false ; 
	    error () ; 
	    deletionsallowed = true ; 
	    goto lab20 ; 
	  } 
	  if ( curinput .locfield == k + 1 ) 
	  curmod = buffer [ k ] ; 
	  else {
	      
	    {
	      if ( poolptr + curinput .locfield - k > maxpoolptr ) 
	      {
		if ( poolptr + curinput .locfield - k > poolsize ) 
		overflow ( 257 , poolsize - initpoolptr ) ; 
		maxpoolptr = poolptr + curinput .locfield - k ; 
	      } 
	    } 
	    do {
		{ 
		strpool [ poolptr ] = buffer [ k ] ; 
		incr ( poolptr ) ; 
	      } 
	      incr ( k ) ; 
	    } while ( ! ( k == curinput .locfield ) ) ; 
	    curmod = makestring () ; 
	  } 
	} 
	incr ( curinput .locfield ) ; 
	curcmd = 39 ; 
	goto lab10 ; 
      } 
      break ; 
    case 5 : 
    case 6 : 
    case 7 : 
    case 8 : 
      {
	k = curinput .locfield - 1 ; 
	goto lab40 ; 
      } 
      break ; 
    case 20 : 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 639 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 640 ; 
	  helpline [ 0 ] = 641 ; 
	} 
	deletionsallowed = false ; 
	error () ; 
	deletionsallowed = true ; 
	goto lab20 ; 
      } 
      break ; 
      default: 
      ; 
      break ; 
    } 
    k = curinput .locfield - 1 ; 
    while ( charclass [ buffer [ curinput .locfield ] ] == class ) incr ( 
    curinput .locfield ) ; 
    goto lab40 ; 
    lab85: n = c - 48 ; 
    while ( charclass [ buffer [ curinput .locfield ] ] == 0 ) {
	
      if ( n < 4096 ) 
      n = 10 * n + buffer [ curinput .locfield ] - 48 ; 
      incr ( curinput .locfield ) ; 
    } 
    if ( buffer [ curinput .locfield ] == 46 ) 
    if ( charclass [ buffer [ curinput .locfield + 1 ] ] == 0 ) 
    goto lab30 ; 
    f = 0 ; 
    goto lab87 ; 
    lab30: incr ( curinput .locfield ) ; 
    lab86: k = 0 ; 
    do {
	if ( k < 17 ) 
      {
	dig [ k ] = buffer [ curinput .locfield ] - 48 ; 
	incr ( k ) ; 
      } 
      incr ( curinput .locfield ) ; 
    } while ( ! ( charclass [ buffer [ curinput .locfield ] ] != 0 ) ) ; 
    f = rounddecimals ( k ) ; 
    if ( f == 65536L ) 
    {
      incr ( n ) ; 
      f = 0 ; 
    } 
    lab87: if ( n < 4096 ) 
    curmod = n * 65536L + f ; 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 646 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 647 ; 
	helpline [ 0 ] = 648 ; 
      } 
      deletionsallowed = false ; 
      error () ; 
      deletionsallowed = true ; 
      curmod = 268435455L ; 
    } 
    curcmd = 42 ; 
    goto lab10 ; 
    lab40: cursym = idlookup ( k , curinput .locfield - k ) ; 
  } 
  else if ( curinput .locfield >= himemmin ) 
  {
    cursym = mem [ curinput .locfield ] .hhfield .lhfield ; 
    curinput .locfield = mem [ curinput .locfield ] .hhfield .v.RH ; 
    if ( cursym >= 9770 ) 
    if ( cursym >= 9920 ) 
    {
      if ( cursym >= 10070 ) 
      cursym = cursym - 150 ; 
      begintokenlist ( paramstack [ curinput .limitfield + cursym - ( 9920 ) ] 
      , 18 ) ; 
      goto lab20 ; 
    } 
    else {
	
      curcmd = 38 ; 
      curmod = paramstack [ curinput .limitfield + cursym - ( 9770 ) ] ; 
      cursym = 0 ; 
      goto lab10 ; 
    } 
  } 
  else if ( curinput .locfield > 0 ) 
  {
    if ( mem [ curinput .locfield ] .hhfield .b1 == 12 ) 
    {
      curmod = mem [ curinput .locfield + 1 ] .cint ; 
      if ( mem [ curinput .locfield ] .hhfield .b0 == 16 ) 
      curcmd = 42 ; 
      else {
	  
	curcmd = 39 ; 
	{
	  if ( strref [ curmod ] < 127 ) 
	  incr ( strref [ curmod ] ) ; 
	} 
      } 
    } 
    else {
	
      curmod = curinput .locfield ; 
      curcmd = 38 ; 
    } 
    curinput .locfield = mem [ curinput .locfield ] .hhfield .v.RH ; 
    goto lab10 ; 
  } 
  else {
      
    endtokenlist () ; 
    goto lab20 ; 
  } 
  curcmd = eqtb [ cursym ] .lhfield ; 
  curmod = eqtb [ cursym ] .v.RH ; 
  if ( curcmd >= 86 ) 
  if ( checkoutervalidity () ) 
  curcmd = curcmd - 86 ; 
  else goto lab20 ; 
  lab10: ; 
} 
void firmuptheline ( ) 
{integer k  ; 
  curinput .limitfield = last ; 
  if ( internal [ 31 ] > 0 ) 
  if ( interaction > 1 ) 
  {
    ; 
    println () ; 
    if ( curinput .startfield < curinput .limitfield ) 
    {register integer for_end; k = curinput .startfield ; for_end = curinput 
    .limitfield - 1 ; if ( k <= for_end) do 
      print ( buffer [ k ] ) ; 
    while ( k++ < for_end ) ; } 
    first = curinput .limitfield ; 
    {
      ; 
      print ( 651 ) ; 
      terminput () ; 
    } 
    if ( last > first ) 
    {
      {register integer for_end; k = first ; for_end = last - 1 ; if ( k <= 
      for_end) do 
	buffer [ k + curinput .startfield - first ] = buffer [ k ] ; 
      while ( k++ < for_end ) ; } 
      curinput .limitfield = curinput .startfield + last - first ; 
    } 
  } 
} 
halfword zscantoks ( terminator , substlist , tailend , suffixcount ) 
commandcode terminator ; 
halfword substlist ; 
halfword tailend ; 
smallnumber suffixcount ; 
{/* 30 40 */ register halfword Result; halfword p  ; 
  halfword q  ; 
  integer balance  ; 
  p = memtop - 2 ; 
  balance = 1 ; 
  mem [ memtop - 2 ] .hhfield .v.RH = 0 ; 
  while ( true ) {
      
    getnext () ; 
    if ( cursym > 0 ) 
    {
      {
	q = substlist ; 
	while ( q != 0 ) {
	    
	  if ( mem [ q ] .hhfield .lhfield == cursym ) 
	  {
	    cursym = mem [ q + 1 ] .cint ; 
	    curcmd = 7 ; 
	    goto lab40 ; 
	  } 
	  q = mem [ q ] .hhfield .v.RH ; 
	} 
	lab40: ; 
      } 
      if ( curcmd == terminator ) 
      if ( curmod > 0 ) 
      incr ( balance ) ; 
      else {
	  
	decr ( balance ) ; 
	if ( balance == 0 ) 
	goto lab30 ; 
      } 
      else if ( curcmd == 61 ) 
      {
	if ( curmod == 0 ) 
	getnext () ; 
	else if ( curmod <= suffixcount ) 
	cursym = 9919 + curmod ; 
      } 
    } 
    mem [ p ] .hhfield .v.RH = curtok () ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  lab30: mem [ p ] .hhfield .v.RH = tailend ; 
  flushnodelist ( substlist ) ; 
  Result = mem [ memtop - 2 ] .hhfield .v.RH ; 
  return(Result) ; 
} 
void getsymbol ( ) 
{/* 20 */ lab20: getnext () ; 
  if ( ( cursym == 0 ) || ( cursym > 9757 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 663 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 664 ; 
      helpline [ 1 ] = 665 ; 
      helpline [ 0 ] = 666 ; 
    } 
    if ( cursym > 0 ) 
    helpline [ 2 ] = 667 ; 
    else if ( curcmd == 39 ) 
    {
      if ( strref [ curmod ] < 127 ) 
      if ( strref [ curmod ] > 1 ) 
      decr ( strref [ curmod ] ) ; 
      else flushstring ( curmod ) ; 
    } 
    cursym = 9757 ; 
    inserror () ; 
    goto lab20 ; 
  } 
} 
void getclearsymbol ( ) 
{getsymbol () ; 
  clearsymbol ( cursym , false ) ; 
} 
void checkequals ( ) 
{if ( curcmd != 51 ) 
  if ( curcmd != 77 ) 
  {
    missingerr ( 61 ) ; 
    {
      helpptr = 5 ; 
      helpline [ 4 ] = 668 ; 
      helpline [ 3 ] = 669 ; 
      helpline [ 2 ] = 670 ; 
      helpline [ 1 ] = 671 ; 
      helpline [ 0 ] = 672 ; 
    } 
    backerror () ; 
  } 
} 
void makeopdef ( ) 
{commandcode m  ; 
  halfword p, q, r  ; 
  m = curmod ; 
  getsymbol () ; 
  q = getnode ( 2 ) ; 
  mem [ q ] .hhfield .lhfield = cursym ; 
  mem [ q + 1 ] .cint = 9770 ; 
  getclearsymbol () ; 
  warninginfo = cursym ; 
  getsymbol () ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hhfield .lhfield = cursym ; 
  mem [ p + 1 ] .cint = 9771 ; 
  mem [ p ] .hhfield .v.RH = q ; 
  getnext () ; 
  checkequals () ; 
  scannerstatus = 5 ; 
  q = getavail () ; 
  mem [ q ] .hhfield .lhfield = 0 ; 
  r = getavail () ; 
  mem [ q ] .hhfield .v.RH = r ; 
  mem [ r ] .hhfield .lhfield = 0 ; 
  mem [ r ] .hhfield .v.RH = scantoks ( 16 , p , 0 , 0 ) ; 
  scannerstatus = 0 ; 
  eqtb [ warninginfo ] .lhfield = m ; 
  eqtb [ warninginfo ] .v.RH = q ; 
  getxnext () ; 
} 
void zcheckdelimiter ( ldelim , rdelim ) 
halfword ldelim ; 
halfword rdelim ; 
{/* 10 */ if ( curcmd == 62 ) 
  if ( curmod == ldelim ) 
  goto lab10 ; 
  if ( cursym != rdelim ) 
  {
    missingerr ( hash [ rdelim ] .v.RH ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 919 ; 
      helpline [ 0 ] = 920 ; 
    } 
    backerror () ; 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 921 ) ; 
    } 
    slowprint ( hash [ rdelim ] .v.RH ) ; 
    print ( 922 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 923 ; 
      helpline [ 1 ] = 924 ; 
      helpline [ 0 ] = 925 ; 
    } 
    error () ; 
  } 
  lab10: ; 
} 
halfword scandeclaredvariable ( ) 
{/* 30 */ register halfword Result; halfword x  ; 
  halfword h, t  ; 
  halfword l  ; 
  getsymbol () ; 
  x = cursym ; 
  if ( curcmd != 41 ) 
  clearsymbol ( x , false ) ; 
  h = getavail () ; 
  mem [ h ] .hhfield .lhfield = x ; 
  t = h ; 
  while ( true ) {
      
    getxnext () ; 
    if ( cursym == 0 ) 
    goto lab30 ; 
    if ( curcmd != 41 ) 
    if ( curcmd != 40 ) 
    if ( curcmd == 63 ) 
    {
      l = cursym ; 
      getxnext () ; 
      if ( curcmd != 64 ) 
      {
	backinput () ; 
	cursym = l ; 
	curcmd = 63 ; 
	goto lab30 ; 
      } 
      else cursym = 0 ; 
    } 
    else goto lab30 ; 
    mem [ t ] .hhfield .v.RH = getavail () ; 
    t = mem [ t ] .hhfield .v.RH ; 
    mem [ t ] .hhfield .lhfield = cursym ; 
  } 
  lab30: if ( eqtb [ x ] .lhfield != 41 ) 
  clearsymbol ( x , false ) ; 
  if ( eqtb [ x ] .v.RH == 0 ) 
  newroot ( x ) ; 
  Result = h ; 
  return(Result) ; 
} 
void scandef ( ) 
{schar m  ; 
  schar n  ; 
  unsigned char k  ; 
  schar c  ; 
  halfword r  ; 
  halfword q  ; 
  halfword p  ; 
  halfword base  ; 
  halfword ldelim, rdelim  ; 
  m = curmod ; 
  c = 0 ; 
  mem [ memtop - 2 ] .hhfield .v.RH = 0 ; 
  q = getavail () ; 
  mem [ q ] .hhfield .lhfield = 0 ; 
  r = 0 ; 
  if ( m == 1 ) 
  {
    getclearsymbol () ; 
    warninginfo = cursym ; 
    getnext () ; 
    scannerstatus = 5 ; 
    n = 0 ; 
    eqtb [ warninginfo ] .lhfield = 10 ; 
    eqtb [ warninginfo ] .v.RH = q ; 
  } 
  else {
      
    p = scandeclaredvariable () ; 
    flushvariable ( eqtb [ mem [ p ] .hhfield .lhfield ] .v.RH , mem [ p ] 
    .hhfield .v.RH , true ) ; 
    warninginfo = findvariable ( p ) ; 
    flushlist ( p ) ; 
    if ( warninginfo == 0 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 679 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 680 ; 
	helpline [ 0 ] = 681 ; 
      } 
      error () ; 
      warninginfo = 21 ; 
    } 
    scannerstatus = 4 ; 
    n = 2 ; 
    if ( curcmd == 61 ) 
    if ( curmod == 3 ) 
    {
      n = 3 ; 
      getnext () ; 
    } 
    mem [ warninginfo ] .hhfield .b0 = 20 + n ; 
    mem [ warninginfo + 1 ] .cint = q ; 
  } 
  k = n ; 
  if ( curcmd == 31 ) 
  do {
      ldelim = cursym ; 
    rdelim = curmod ; 
    getnext () ; 
    if ( ( curcmd == 56 ) && ( curmod >= 9770 ) ) 
    base = curmod ; 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 682 ) ; 
      } 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 683 ; 
      } 
      backerror () ; 
      base = 9770 ; 
    } 
    do {
	mem [ q ] .hhfield .v.RH = getavail () ; 
      q = mem [ q ] .hhfield .v.RH ; 
      mem [ q ] .hhfield .lhfield = base + k ; 
      getsymbol () ; 
      p = getnode ( 2 ) ; 
      mem [ p + 1 ] .cint = base + k ; 
      mem [ p ] .hhfield .lhfield = cursym ; 
      if ( k == 150 ) 
      overflow ( 684 , 150 ) ; 
      incr ( k ) ; 
      mem [ p ] .hhfield .v.RH = r ; 
      r = p ; 
      getnext () ; 
    } while ( ! ( curcmd != 82 ) ) ; 
    checkdelimiter ( ldelim , rdelim ) ; 
    getnext () ; 
  } while ( ! ( curcmd != 31 ) ) ; 
  if ( curcmd == 56 ) 
  {
    p = getnode ( 2 ) ; 
    if ( curmod < 9770 ) 
    {
      c = curmod ; 
      mem [ p + 1 ] .cint = 9770 + k ; 
    } 
    else {
	
      mem [ p + 1 ] .cint = curmod + k ; 
      if ( curmod == 9770 ) 
      c = 4 ; 
      else if ( curmod == 9920 ) 
      c = 6 ; 
      else c = 7 ; 
    } 
    if ( k == 150 ) 
    overflow ( 684 , 150 ) ; 
    incr ( k ) ; 
    getsymbol () ; 
    mem [ p ] .hhfield .lhfield = cursym ; 
    mem [ p ] .hhfield .v.RH = r ; 
    r = p ; 
    getnext () ; 
    if ( c == 4 ) 
    if ( curcmd == 69 ) 
    {
      c = 5 ; 
      p = getnode ( 2 ) ; 
      if ( k == 150 ) 
      overflow ( 684 , 150 ) ; 
      mem [ p + 1 ] .cint = 9770 + k ; 
      getsymbol () ; 
      mem [ p ] .hhfield .lhfield = cursym ; 
      mem [ p ] .hhfield .v.RH = r ; 
      r = p ; 
      getnext () ; 
    } 
  } 
  checkequals () ; 
  p = getavail () ; 
  mem [ p ] .hhfield .lhfield = c ; 
  mem [ q ] .hhfield .v.RH = p ; 
  if ( m == 1 ) 
  mem [ p ] .hhfield .v.RH = scantoks ( 16 , r , 0 , n ) ; 
  else {
      
    q = getavail () ; 
    mem [ q ] .hhfield .lhfield = bgloc ; 
    mem [ p ] .hhfield .v.RH = q ; 
    p = getavail () ; 
    mem [ p ] .hhfield .lhfield = egloc ; 
    mem [ q ] .hhfield .v.RH = scantoks ( 16 , r , p , n ) ; 
  } 
  if ( warninginfo == 21 ) 
  flushtokenlist ( mem [ 22 ] .cint ) ; 
  scannerstatus = 0 ; 
  getxnext () ; 
} 
void zprintmacroname ( a , n ) 
halfword a ; 
halfword n ; 
{halfword p, q  ; 
  if ( n != 0 ) 
  slowprint ( hash [ n ] .v.RH ) ; 
  else {
      
    p = mem [ a ] .hhfield .lhfield ; 
    if ( p == 0 ) 
    slowprint ( hash [ mem [ mem [ mem [ a ] .hhfield .v.RH ] .hhfield 
    .lhfield ] .hhfield .lhfield ] .v.RH ) ; 
    else {
	
      q = p ; 
      while ( mem [ q ] .hhfield .v.RH != 0 ) q = mem [ q ] .hhfield .v.RH ; 
      mem [ q ] .hhfield .v.RH = mem [ mem [ a ] .hhfield .v.RH ] .hhfield 
      .lhfield ; 
      showtokenlist ( p , 0 , 1000 , 0 ) ; 
      mem [ q ] .hhfield .v.RH = 0 ; 
    } 
  } 
} 
void zprintarg ( q , n , b ) 
halfword q ; 
integer n ; 
halfword b ; 
{if ( mem [ q ] .hhfield .v.RH == 1 ) 
  printnl ( 497 ) ; 
  else if ( ( b < 10070 ) && ( b != 7 ) ) 
  printnl ( 498 ) ; 
  else printnl ( 499 ) ; 
  printint ( n ) ; 
  print ( 700 ) ; 
  if ( mem [ q ] .hhfield .v.RH == 1 ) 
  printexp ( q , 1 ) ; 
  else showtokenlist ( q , 0 , 1000 , 0 ) ; 
} 
void zscantextarg ( ldelim , rdelim ) 
halfword ldelim ; 
halfword rdelim ; 
{/* 30 */ integer balance  ; 
  halfword p  ; 
  warninginfo = ldelim ; 
  scannerstatus = 3 ; 
  p = memtop - 2 ; 
  balance = 1 ; 
  mem [ memtop - 2 ] .hhfield .v.RH = 0 ; 
  while ( true ) {
      
    getnext () ; 
    if ( ldelim == 0 ) 
    {
      if ( curcmd > 82 ) 
      {
	if ( balance == 1 ) 
	goto lab30 ; 
	else if ( curcmd == 84 ) 
	decr ( balance ) ; 
      } 
      else if ( curcmd == 32 ) 
      incr ( balance ) ; 
    } 
    else {
	
      if ( curcmd == 62 ) 
      {
	if ( curmod == ldelim ) 
	{
	  decr ( balance ) ; 
	  if ( balance == 0 ) 
	  goto lab30 ; 
	} 
      } 
      else if ( curcmd == 31 ) 
      if ( curmod == rdelim ) 
      incr ( balance ) ; 
    } 
    mem [ p ] .hhfield .v.RH = curtok () ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  lab30: curexp = mem [ memtop - 2 ] .hhfield .v.RH ; 
  curtype = 20 ; 
  scannerstatus = 0 ; 
} 
