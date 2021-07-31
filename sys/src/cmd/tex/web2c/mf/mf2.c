#define EXTERN extern
#include "mfd.h"

void zprintpen ( p , s , nuline ) 
halfword p ; 
strnumber s ; 
boolean nuline ; 
{boolean nothingprinted  ; 
  schar k  ; 
  halfword h  ; 
  integer m, n  ; 
  halfword w, ww  ; 
  printdiagnostic ( 568 , s , nuline ) ; 
  nothingprinted = true ; 
  println () ; 
  {register integer for_end; k = 1 ; for_end = 8 ; if ( k <= for_end) do 
    {
      octant = octantcode [ k ] ; 
      h = p + octant ; 
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
	    if ( nothingprinted ) 
	    nothingprinted = false ; 
	    else printnl ( 570 ) ; 
	    unskew ( mem [ ww + 1 ] .cint , mem [ ww + 2 ] .cint , octant ) ; 
	    printtwo ( curx , cury ) ; 
	  } 
	  w = ww ; 
	} 
      while ( m++ < for_end ) ; } 
    } 
  while ( k++ < for_end ) ; } 
  if ( nothingprinted ) 
  {
    w = mem [ p + 1 ] .hhfield .v.RH ; 
    printtwo ( mem [ w + 1 ] .cint + mem [ w + 2 ] .cint , mem [ w + 2 ] .cint 
    ) ; 
  } 
  printnl ( 569 ) ; 
  enddiagnostic ( true ) ; 
} 
void zprintdependency ( p , t ) 
halfword p ; 
smallnumber t ; 
{/* 10 */ integer v  ; 
  halfword pp, q  ; 
  pp = p ; 
  while ( true ) {
      
    v = abs ( mem [ p + 1 ] .cint ) ; 
    q = mem [ p ] .hhfield .lhfield ; 
    if ( q == 0 ) 
    {
      if ( ( v != 0 ) || ( p == pp ) ) 
      {
	if ( mem [ p + 1 ] .cint > 0 ) 
	if ( p != pp ) 
	printchar ( 43 ) ; 
	printscaled ( mem [ p + 1 ] .cint ) ; 
      } 
      goto lab10 ; 
    } 
    if ( mem [ p + 1 ] .cint < 0 ) 
    printchar ( 45 ) ; 
    else if ( p != pp ) 
    printchar ( 43 ) ; 
    if ( t == 17 ) 
    v = roundfraction ( v ) ; 
    if ( v != 65536L ) 
    printscaled ( v ) ; 
    if ( mem [ q ] .hhfield .b0 != 19 ) 
    confusion ( 586 ) ; 
    printvariablename ( q ) ; 
    v = mem [ q + 1 ] .cint % 64 ; 
    while ( v > 0 ) {
	
      print ( 587 ) ; 
      v = v - 2 ; 
    } 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  lab10: ; 
} 
void zprintdp ( t , p , verbosity ) 
smallnumber t ; 
halfword p ; 
smallnumber verbosity ; 
{halfword q  ; 
  q = mem [ p ] .hhfield .v.RH ; 
  if ( ( mem [ q ] .hhfield .lhfield == 0 ) || ( verbosity > 0 ) ) 
  printdependency ( p , t ) ; 
  else print ( 761 ) ; 
} 
halfword stashcurexp ( ) 
{register halfword Result; halfword p  ; 
  switch ( curtype ) 
  {case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 13 : 
  case 14 : 
  case 17 : 
  case 18 : 
  case 19 : 
    p = curexp ; 
    break ; 
    default: 
    {
      p = getnode ( 2 ) ; 
      mem [ p ] .hhfield .b1 = 11 ; 
      mem [ p ] .hhfield .b0 = curtype ; 
      mem [ p + 1 ] .cint = curexp ; 
    } 
    break ; 
  } 
  curtype = 1 ; 
  mem [ p ] .hhfield .v.RH = 1 ; 
  Result = p ; 
  return(Result) ; 
} 
void zunstashcurexp ( p ) 
halfword p ; 
{curtype = mem [ p ] .hhfield .b0 ; 
  switch ( curtype ) 
  {case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 13 : 
  case 14 : 
  case 17 : 
  case 18 : 
  case 19 : 
    curexp = p ; 
    break ; 
    default: 
    {
      curexp = mem [ p + 1 ] .cint ; 
      freenode ( p , 2 ) ; 
    } 
    break ; 
  } 
} 
void zprintexp ( p , verbosity ) 
halfword p ; 
smallnumber verbosity ; 
{boolean restorecurexp  ; 
  smallnumber t  ; 
  integer v  ; 
  halfword q  ; 
  if ( p != 0 ) 
  restorecurexp = false ; 
  else {
      
    p = stashcurexp () ; 
    restorecurexp = true ; 
  } 
  t = mem [ p ] .hhfield .b0 ; 
  if ( t < 17 ) 
  v = mem [ p + 1 ] .cint ; 
  else if ( t < 19 ) 
  v = mem [ p + 1 ] .hhfield .v.RH ; 
  switch ( t ) 
  {case 1 : 
    print ( 322 ) ; 
    break ; 
  case 2 : 
    if ( v == 30 ) 
    print ( 346 ) ; 
    else print ( 347 ) ; 
    break ; 
  case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 15 : 
    {
      printtype ( t ) ; 
      if ( v != 0 ) 
      {
	printchar ( 32 ) ; 
	while ( ( mem [ v ] .hhfield .b1 == 11 ) && ( v != p ) ) v = mem [ v + 
	1 ] .cint ; 
	printvariablename ( v ) ; 
      } 
    } 
    break ; 
  case 4 : 
    {
      printchar ( 34 ) ; 
      slowprint ( v ) ; 
      printchar ( 34 ) ; 
    } 
    break ; 
  case 6 : 
  case 8 : 
  case 9 : 
  case 11 : 
    if ( verbosity <= 1 ) 
    printtype ( t ) ; 
    else {
	
      if ( selector == 3 ) 
      if ( internal [ 13 ] <= 0 ) 
      {
	selector = 1 ; 
	printtype ( t ) ; 
	print ( 759 ) ; 
	selector = 3 ; 
      } 
      switch ( t ) 
      {case 6 : 
	printpen ( v , 283 , false ) ; 
	break ; 
      case 8 : 
	printpath ( v , 760 , false ) ; 
	break ; 
      case 9 : 
	printpath ( v , 283 , false ) ; 
	break ; 
      case 11 : 
	{
	  curedges = v ; 
	  printedges ( 283 , false , 0 , 0 ) ; 
	} 
	break ; 
      } 
    } 
    break ; 
  case 13 : 
  case 14 : 
    if ( v == 0 ) 
    printtype ( t ) ; 
    else {
	
      printchar ( 40 ) ; 
      q = v + bignodesize [ t ] ; 
      do {
	  if ( mem [ v ] .hhfield .b0 == 16 ) 
	printscaled ( mem [ v + 1 ] .cint ) ; 
	else if ( mem [ v ] .hhfield .b0 == 19 ) 
	printvariablename ( v ) ; 
	else printdp ( mem [ v ] .hhfield .b0 , mem [ v + 1 ] .hhfield .v.RH , 
	verbosity ) ; 
	v = v + 2 ; 
	if ( v != q ) 
	printchar ( 44 ) ; 
      } while ( ! ( v == q ) ) ; 
      printchar ( 41 ) ; 
    } 
    break ; 
  case 16 : 
    printscaled ( v ) ; 
    break ; 
  case 17 : 
  case 18 : 
    printdp ( t , v , verbosity ) ; 
    break ; 
  case 19 : 
    printvariablename ( p ) ; 
    break ; 
    default: 
    confusion ( 758 ) ; 
    break ; 
  } 
  if ( restorecurexp ) 
  unstashcurexp ( p ) ; 
} 
void zdisperr ( p , s ) 
halfword p ; 
strnumber s ; 
{if ( interaction == 3 ) 
  ; 
  printnl ( 762 ) ; 
  printexp ( p , 1 ) ; 
  if ( s != 283 ) 
  {
    printnl ( 261 ) ; 
    print ( s ) ; 
  } 
} 
halfword zpplusfq ( p , f , q , t , tt ) 
halfword p ; 
integer f ; 
halfword q ; 
smallnumber t ; 
smallnumber tt ; 
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
      
    if ( tt == 17 ) 
    v = mem [ p + 1 ] .cint + takefraction ( f , mem [ q + 1 ] .cint ) ; 
    else v = mem [ p + 1 ] .cint + takescaled ( f , mem [ q + 1 ] .cint ) ; 
    mem [ p + 1 ] .cint = v ; 
    s = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
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
    pp = mem [ p ] .hhfield .lhfield ; 
    q = mem [ q ] .hhfield .v.RH ; 
    qq = mem [ q ] .hhfield .lhfield ; 
  } 
  else if ( mem [ pp + 1 ] .cint < mem [ qq + 1 ] .cint ) 
  {
    if ( tt == 17 ) 
    v = takefraction ( f , mem [ q + 1 ] .cint ) ; 
    else v = takescaled ( f , mem [ q + 1 ] .cint ) ; 
    if ( abs ( v ) > ( threshold ) / 2 ) 
    {
      s = getnode ( 2 ) ; 
      mem [ s ] .hhfield .lhfield = qq ; 
      mem [ s + 1 ] .cint = v ; 
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
  else {
      
    mem [ r ] .hhfield .v.RH = p ; 
    r = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
    pp = mem [ p ] .hhfield .lhfield ; 
  } 
  lab30: if ( t == 17 ) 
  mem [ p + 1 ] .cint = slowadd ( mem [ p + 1 ] .cint , takefraction ( mem [ q 
  + 1 ] .cint , f ) ) ; 
  else mem [ p + 1 ] .cint = slowadd ( mem [ p + 1 ] .cint , takescaled ( mem 
  [ q + 1 ] .cint , f ) ) ; 
  mem [ r ] .hhfield .v.RH = p ; 
  depfinal = p ; 
  Result = mem [ memtop - 1 ] .hhfield .v.RH ; 
  return(Result) ; 
} 
halfword zpoverv ( p , v , t0 , t1 ) 
halfword p ; 
scaled v ; 
smallnumber t0 ; 
smallnumber t1 ; 
{register halfword Result; halfword r, s  ; 
  integer w  ; 
  integer threshold  ; 
  boolean scalingdown  ; 
  if ( t0 != t1 ) 
  scalingdown = true ; 
  else scalingdown = false ; 
  if ( t1 == 17 ) 
  threshold = 1342 ; 
  else threshold = 4 ; 
  r = memtop - 1 ; 
  while ( mem [ p ] .hhfield .lhfield != 0 ) {
      
    if ( scalingdown ) 
    if ( abs ( v ) < 524288L ) 
    w = makescaled ( mem [ p + 1 ] .cint , v * 4096 ) ; 
    else w = makescaled ( roundfraction ( mem [ p + 1 ] .cint ) , v ) ; 
    else w = makescaled ( mem [ p + 1 ] .cint , v ) ; 
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
  mem [ p + 1 ] .cint = makescaled ( mem [ p + 1 ] .cint , v ) ; 
  Result = mem [ memtop - 1 ] .hhfield .v.RH ; 
  return(Result) ; 
} 
void zvaltoobig ( x ) 
scaled x ; 
{if ( internal [ 40 ] > 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 588 ) ; 
    } 
    printscaled ( x ) ; 
    printchar ( 41 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 589 ; 
      helpline [ 2 ] = 590 ; 
      helpline [ 1 ] = 591 ; 
      helpline [ 0 ] = 592 ; 
    } 
    error () ; 
  } 
} 
void zmakeknown ( p , q ) 
halfword p ; 
halfword q ; 
{schar t  ; 
  mem [ mem [ q ] .hhfield .v.RH + 1 ] .hhfield .lhfield = mem [ p + 1 ] 
  .hhfield .lhfield ; 
  mem [ mem [ p + 1 ] .hhfield .lhfield ] .hhfield .v.RH = mem [ q ] .hhfield 
  .v.RH ; 
  t = mem [ p ] .hhfield .b0 ; 
  mem [ p ] .hhfield .b0 = 16 ; 
  mem [ p + 1 ] .cint = mem [ q + 1 ] .cint ; 
  freenode ( q , 2 ) ; 
  if ( abs ( mem [ p + 1 ] .cint ) >= 268435456L ) 
  valtoobig ( mem [ p + 1 ] .cint ) ; 
  if ( internal [ 2 ] > 0 ) 
  if ( interesting ( p ) ) 
  {
    begindiagnostic () ; 
    printnl ( 593 ) ; 
    printvariablename ( p ) ; 
    printchar ( 61 ) ; 
    printscaled ( mem [ p + 1 ] .cint ) ; 
    enddiagnostic ( false ) ; 
  } 
  if ( curexp == p ) 
  if ( curtype == t ) 
  {
    curtype = 16 ; 
    curexp = mem [ p + 1 ] .cint ; 
    freenode ( p , 2 ) ; 
  } 
} 
void fixdependencies ( ) 
{/* 30 */ halfword p, q, r, s, t  ; 
  halfword x  ; 
  r = mem [ 13 ] .hhfield .v.RH ; 
  s = 0 ; 
  while ( r != 13 ) {
      
    t = r ; 
    r = t + 1 ; 
    while ( true ) {
	
      q = mem [ r ] .hhfield .v.RH ; 
      x = mem [ q ] .hhfield .lhfield ; 
      if ( x == 0 ) 
      goto lab30 ; 
      if ( mem [ x ] .hhfield .b0 <= 1 ) 
      {
	if ( mem [ x ] .hhfield .b0 < 1 ) 
	{
	  p = getavail () ; 
	  mem [ p ] .hhfield .v.RH = s ; 
	  s = p ; 
	  mem [ s ] .hhfield .lhfield = x ; 
	  mem [ x ] .hhfield .b0 = 1 ; 
	} 
	mem [ q + 1 ] .cint = mem [ q + 1 ] .cint / 4 ; 
	if ( mem [ q + 1 ] .cint == 0 ) 
	{
	  mem [ r ] .hhfield .v.RH = mem [ q ] .hhfield .v.RH ; 
	  freenode ( q , 2 ) ; 
	  q = r ; 
	} 
      } 
      r = q ; 
    } 
    lab30: ; 
    r = mem [ q ] .hhfield .v.RH ; 
    if ( q == mem [ t + 1 ] .hhfield .v.RH ) 
    makeknown ( t , q ) ; 
  } 
  while ( s != 0 ) {
      
    p = mem [ s ] .hhfield .v.RH ; 
    x = mem [ s ] .hhfield .lhfield ; 
    {
      mem [ s ] .hhfield .v.RH = avail ; 
      avail = s ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
    } 
    s = p ; 
    mem [ x ] .hhfield .b0 = 19 ; 
    mem [ x + 1 ] .cint = mem [ x + 1 ] .cint + 2 ; 
  } 
  fixneeded = false ; 
} 
void ztossknotlist ( p ) 
halfword p ; 
{halfword q  ; 
  halfword r  ; 
  q = p ; 
  do {
      r = mem [ q ] .hhfield .v.RH ; 
    freenode ( q , 7 ) ; 
    q = r ; 
  } while ( ! ( q == p ) ) ; 
} 
void ztossedges ( h ) 
halfword h ; 
{halfword p, q  ; 
  q = mem [ h ] .hhfield .v.RH ; 
  while ( q != h ) {
      
    flushlist ( mem [ q + 1 ] .hhfield .v.RH ) ; 
    if ( mem [ q + 1 ] .hhfield .lhfield > 1 ) 
    flushlist ( mem [ q + 1 ] .hhfield .lhfield ) ; 
    p = q ; 
    q = mem [ q ] .hhfield .v.RH ; 
    freenode ( p , 2 ) ; 
  } 
  freenode ( h , 6 ) ; 
} 
void ztosspen ( p ) 
halfword p ; 
{schar k  ; 
  halfword w, ww  ; 
  if ( p != 3 ) 
  {
    {register integer for_end; k = 1 ; for_end = 8 ; if ( k <= for_end) do 
      {
	w = mem [ p + k ] .hhfield .v.RH ; 
	do {
	    ww = mem [ w ] .hhfield .v.RH ; 
	  freenode ( w , 3 ) ; 
	  w = ww ; 
	} while ( ! ( w == mem [ p + k ] .hhfield .v.RH ) ) ; 
      } 
    while ( k++ < for_end ) ; } 
    freenode ( p , 10 ) ; 
  } 
} 
void zringdelete ( p ) 
halfword p ; 
{halfword q  ; 
  q = mem [ p + 1 ] .cint ; 
  if ( q != 0 ) 
  if ( q != p ) 
  {
    while ( mem [ q + 1 ] .cint != p ) q = mem [ q + 1 ] .cint ; 
    mem [ q + 1 ] .cint = mem [ p + 1 ] .cint ; 
  } 
} 
void zrecyclevalue ( p ) 
halfword p ; 
{/* 30 */ smallnumber t  ; 
  integer v  ; 
  integer vv  ; 
  halfword q, r, s, pp  ; 
  t = mem [ p ] .hhfield .b0 ; 
  if ( t < 17 ) 
  v = mem [ p + 1 ] .cint ; 
  switch ( t ) 
  {case 0 : 
  case 1 : 
  case 2 : 
  case 16 : 
  case 15 : 
    ; 
    break ; 
  case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
    ringdelete ( p ) ; 
    break ; 
  case 4 : 
    {
      if ( strref [ v ] < 127 ) 
      if ( strref [ v ] > 1 ) 
      decr ( strref [ v ] ) ; 
      else flushstring ( v ) ; 
    } 
    break ; 
  case 6 : 
    if ( mem [ v ] .hhfield .lhfield == 0 ) 
    tosspen ( v ) ; 
    else decr ( mem [ v ] .hhfield .lhfield ) ; 
    break ; 
  case 9 : 
  case 8 : 
    tossknotlist ( v ) ; 
    break ; 
  case 11 : 
    tossedges ( v ) ; 
    break ; 
  case 14 : 
  case 13 : 
    if ( v != 0 ) 
    {
      q = v + bignodesize [ t ] ; 
      do {
	  q = q - 2 ; 
	recyclevalue ( q ) ; 
      } while ( ! ( q == v ) ) ; 
      freenode ( v , bignodesize [ t ] ) ; 
    } 
    break ; 
  case 17 : 
  case 18 : 
    {
      q = mem [ p + 1 ] .hhfield .v.RH ; 
      while ( mem [ q ] .hhfield .lhfield != 0 ) q = mem [ q ] .hhfield .v.RH 
      ; 
      mem [ mem [ p + 1 ] .hhfield .lhfield ] .hhfield .v.RH = mem [ q ] 
      .hhfield .v.RH ; 
      mem [ mem [ q ] .hhfield .v.RH + 1 ] .hhfield .lhfield = mem [ p + 1 ] 
      .hhfield .lhfield ; 
      mem [ q ] .hhfield .v.RH = 0 ; 
      flushnodelist ( mem [ p + 1 ] .hhfield .v.RH ) ; 
    } 
    break ; 
  case 19 : 
    {
      maxc [ 17 ] = 0 ; 
      maxc [ 18 ] = 0 ; 
      maxlink [ 17 ] = 0 ; 
      maxlink [ 18 ] = 0 ; 
      q = mem [ 13 ] .hhfield .v.RH ; 
      while ( q != 13 ) {
	  
	s = q + 1 ; 
	while ( true ) {
	    
	  r = mem [ s ] .hhfield .v.RH ; 
	  if ( mem [ r ] .hhfield .lhfield == 0 ) 
	  goto lab30 ; 
	  if ( mem [ r ] .hhfield .lhfield != p ) 
	  s = r ; 
	  else {
	      
	    t = mem [ q ] .hhfield .b0 ; 
	    mem [ s ] .hhfield .v.RH = mem [ r ] .hhfield .v.RH ; 
	    mem [ r ] .hhfield .lhfield = q ; 
	    if ( abs ( mem [ r + 1 ] .cint ) > maxc [ t ] ) 
	    {
	      if ( maxc [ t ] > 0 ) 
	      {
		mem [ maxptr [ t ] ] .hhfield .v.RH = maxlink [ t ] ; 
		maxlink [ t ] = maxptr [ t ] ; 
	      } 
	      maxc [ t ] = abs ( mem [ r + 1 ] .cint ) ; 
	      maxptr [ t ] = r ; 
	    } 
	    else {
		
	      mem [ r ] .hhfield .v.RH = maxlink [ t ] ; 
	      maxlink [ t ] = r ; 
	    } 
	  } 
	} 
	lab30: q = mem [ r ] .hhfield .v.RH ; 
      } 
      if ( ( maxc [ 17 ] > 0 ) || ( maxc [ 18 ] > 0 ) ) 
      {
	if ( ( maxc [ 17 ] >= 268435456L ) || ( maxc [ 17 ] / 4096 >= maxc [ 
	18 ] ) ) 
	t = 17 ; 
	else t = 18 ; 
	s = maxptr [ t ] ; 
	pp = mem [ s ] .hhfield .lhfield ; 
	v = mem [ s + 1 ] .cint ; 
	if ( t == 17 ) 
	mem [ s + 1 ] .cint = -268435456L ; 
	else mem [ s + 1 ] .cint = -65536L ; 
	r = mem [ pp + 1 ] .hhfield .v.RH ; 
	mem [ s ] .hhfield .v.RH = r ; 
	while ( mem [ r ] .hhfield .lhfield != 0 ) r = mem [ r ] .hhfield 
	.v.RH ; 
	q = mem [ r ] .hhfield .v.RH ; 
	mem [ r ] .hhfield .v.RH = 0 ; 
	mem [ q + 1 ] .hhfield .lhfield = mem [ pp + 1 ] .hhfield .lhfield ; 
	mem [ mem [ pp + 1 ] .hhfield .lhfield ] .hhfield .v.RH = q ; 
	{
	  mem [ pp ] .hhfield .b0 = 19 ; 
	  serialno = serialno + 64 ; 
	  mem [ pp + 1 ] .cint = serialno ; 
	} 
	if ( curexp == pp ) 
	if ( curtype == t ) 
	curtype = 19 ; 
	if ( internal [ 2 ] > 0 ) 
	if ( interesting ( p ) ) 
	{
	  begindiagnostic () ; 
	  printnl ( 764 ) ; 
	  if ( v > 0 ) 
	  printchar ( 45 ) ; 
	  if ( t == 17 ) 
	  vv = roundfraction ( maxc [ 17 ] ) ; 
	  else vv = maxc [ 18 ] ; 
	  if ( vv != 65536L ) 
	  printscaled ( vv ) ; 
	  printvariablename ( p ) ; 
	  while ( mem [ p + 1 ] .cint % 64 > 0 ) {
	      
	    print ( 587 ) ; 
	    mem [ p + 1 ] .cint = mem [ p + 1 ] .cint - 2 ; 
	  } 
	  if ( t == 17 ) 
	  printchar ( 61 ) ; 
	  else print ( 765 ) ; 
	  printdependency ( s , t ) ; 
	  enddiagnostic ( false ) ; 
	} 
	t = 35 - t ; 
	if ( maxc [ t ] > 0 ) 
	{
	  mem [ maxptr [ t ] ] .hhfield .v.RH = maxlink [ t ] ; 
	  maxlink [ t ] = maxptr [ t ] ; 
	} 
	if ( t != 17 ) 
	{register integer for_end; t = 17 ; for_end = 18 ; if ( t <= for_end) 
	do 
	  {
	    r = maxlink [ t ] ; 
	    while ( r != 0 ) {
		
	      q = mem [ r ] .hhfield .lhfield ; 
	      mem [ q + 1 ] .hhfield .v.RH = pplusfq ( mem [ q + 1 ] .hhfield 
	      .v.RH , makefraction ( mem [ r + 1 ] .cint , - (integer) v ) , s 
	      , t , 17 ) ; 
	      if ( mem [ q + 1 ] .hhfield .v.RH == depfinal ) 
	      makeknown ( q , depfinal ) ; 
	      q = r ; 
	      r = mem [ r ] .hhfield .v.RH ; 
	      freenode ( q , 2 ) ; 
	    } 
	  } 
	while ( t++ < for_end ) ; } 
	else {
	    register integer for_end; t = 17 ; for_end = 18 ; if ( t <= 
	for_end) do 
	  {
	    r = maxlink [ t ] ; 
	    while ( r != 0 ) {
		
	      q = mem [ r ] .hhfield .lhfield ; 
	      if ( t == 17 ) 
	      {
		if ( curexp == q ) 
		if ( curtype == 17 ) 
		curtype = 18 ; 
		mem [ q + 1 ] .hhfield .v.RH = poverv ( mem [ q + 1 ] .hhfield 
		.v.RH , 65536L , 17 , 18 ) ; 
		mem [ q ] .hhfield .b0 = 18 ; 
		mem [ r + 1 ] .cint = roundfraction ( mem [ r + 1 ] .cint ) ; 
	      } 
	      mem [ q + 1 ] .hhfield .v.RH = pplusfq ( mem [ q + 1 ] .hhfield 
	      .v.RH , makescaled ( mem [ r + 1 ] .cint , - (integer) v ) , s , 
	      18 , 18 ) ; 
	      if ( mem [ q + 1 ] .hhfield .v.RH == depfinal ) 
	      makeknown ( q , depfinal ) ; 
	      q = r ; 
	      r = mem [ r ] .hhfield .v.RH ; 
	      freenode ( q , 2 ) ; 
	    } 
	  } 
	while ( t++ < for_end ) ; } 
	flushnodelist ( s ) ; 
	if ( fixneeded ) 
	fixdependencies () ; 
	{
	  if ( aritherror ) 
	  cleararith () ; 
	} 
      } 
    } 
    break ; 
  case 20 : 
  case 21 : 
    confusion ( 763 ) ; 
    break ; 
  case 22 : 
  case 23 : 
    deletemacref ( mem [ p + 1 ] .cint ) ; 
    break ; 
  } 
  mem [ p ] .hhfield .b0 = 0 ; 
} 
void zflushcurexp ( v ) 
scaled v ; 
{switch ( curtype ) 
  {case 3 : 
  case 5 : 
  case 7 : 
  case 12 : 
  case 10 : 
  case 13 : 
  case 14 : 
  case 17 : 
  case 18 : 
  case 19 : 
    {
      recyclevalue ( curexp ) ; 
      freenode ( curexp , 2 ) ; 
    } 
    break ; 
  case 6 : 
    if ( mem [ curexp ] .hhfield .lhfield == 0 ) 
    tosspen ( curexp ) ; 
    else decr ( mem [ curexp ] .hhfield .lhfield ) ; 
    break ; 
  case 4 : 
    {
      if ( strref [ curexp ] < 127 ) 
      if ( strref [ curexp ] > 1 ) 
      decr ( strref [ curexp ] ) ; 
      else flushstring ( curexp ) ; 
    } 
    break ; 
  case 8 : 
  case 9 : 
    tossknotlist ( curexp ) ; 
    break ; 
  case 11 : 
    tossedges ( curexp ) ; 
    break ; 
    default: 
    ; 
    break ; 
  } 
  curtype = 16 ; 
  curexp = v ; 
} 
void zflusherror ( v ) 
scaled v ; 
{error () ; 
  flushcurexp ( v ) ; 
} 
void putgeterror ( ) 
{backerror () ; 
  getxnext () ; 
} 
void zputgetflusherror ( v ) 
scaled v ; 
{putgeterror () ; 
  flushcurexp ( v ) ; 
} 
void zflushbelowvariable ( p ) 
halfword p ; 
{halfword q, r  ; 
  if ( mem [ p ] .hhfield .b0 != 21 ) 
  recyclevalue ( p ) ; 
  else {
      
    q = mem [ p + 1 ] .hhfield .v.RH ; 
    while ( mem [ q ] .hhfield .b1 == 3 ) {
	
      flushbelowvariable ( q ) ; 
      r = q ; 
      q = mem [ q ] .hhfield .v.RH ; 
      freenode ( r , 3 ) ; 
    } 
    r = mem [ p + 1 ] .hhfield .lhfield ; 
    q = mem [ r ] .hhfield .v.RH ; 
    recyclevalue ( r ) ; 
    if ( mem [ p ] .hhfield .b1 <= 1 ) 
    freenode ( r , 2 ) ; 
    else freenode ( r , 3 ) ; 
    do {
	flushbelowvariable ( q ) ; 
      r = q ; 
      q = mem [ q ] .hhfield .v.RH ; 
      freenode ( r , 3 ) ; 
    } while ( ! ( q == 17 ) ) ; 
    mem [ p ] .hhfield .b0 = 0 ; 
  } 
} 
void zflushvariable ( p , t , discardsuffixes ) 
halfword p ; 
halfword t ; 
boolean discardsuffixes ; 
{/* 10 */ halfword q, r  ; 
  halfword n  ; 
  while ( t != 0 ) {
      
    if ( mem [ p ] .hhfield .b0 != 21 ) 
    goto lab10 ; 
    n = mem [ t ] .hhfield .lhfield ; 
    t = mem [ t ] .hhfield .v.RH ; 
    if ( n == 0 ) 
    {
      r = p + 1 ; 
      q = mem [ r ] .hhfield .v.RH ; 
      while ( mem [ q ] .hhfield .b1 == 3 ) {
	  
	flushvariable ( q , t , discardsuffixes ) ; 
	if ( t == 0 ) 
	if ( mem [ q ] .hhfield .b0 == 21 ) 
	r = q ; 
	else {
	    
	  mem [ r ] .hhfield .v.RH = mem [ q ] .hhfield .v.RH ; 
	  freenode ( q , 3 ) ; 
	} 
	else r = q ; 
	q = mem [ r ] .hhfield .v.RH ; 
      } 
    } 
    p = mem [ p + 1 ] .hhfield .lhfield ; 
    do {
	r = p ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } while ( ! ( mem [ p + 2 ] .hhfield .lhfield >= n ) ) ; 
    if ( mem [ p + 2 ] .hhfield .lhfield != n ) 
    goto lab10 ; 
  } 
  if ( discardsuffixes ) 
  flushbelowvariable ( p ) ; 
  else {
      
    if ( mem [ p ] .hhfield .b0 == 21 ) 
    p = mem [ p + 1 ] .hhfield .lhfield ; 
    recyclevalue ( p ) ; 
  } 
  lab10: ; 
} 
smallnumber zundtype ( p ) 
halfword p ; 
{register smallnumber Result; switch ( mem [ p ] .hhfield .b0 ) 
  {case 0 : 
  case 1 : 
    Result = 0 ; 
    break ; 
  case 2 : 
  case 3 : 
    Result = 3 ; 
    break ; 
  case 4 : 
  case 5 : 
    Result = 5 ; 
    break ; 
  case 6 : 
  case 7 : 
  case 8 : 
    Result = 7 ; 
    break ; 
  case 9 : 
  case 10 : 
    Result = 10 ; 
    break ; 
  case 11 : 
  case 12 : 
    Result = 12 ; 
    break ; 
  case 13 : 
  case 14 : 
  case 15 : 
    Result = mem [ p ] .hhfield .b0 ; 
    break ; 
  case 16 : 
  case 17 : 
  case 18 : 
  case 19 : 
    Result = 15 ; 
    break ; 
  } 
  return(Result) ; 
} 
void zclearsymbol ( p , saving ) 
halfword p ; 
boolean saving ; 
{halfword q  ; 
  q = eqtb [ p ] .v.RH ; 
  switch ( eqtb [ p ] .lhfield % 86 ) 
  {case 10 : 
  case 53 : 
  case 44 : 
  case 49 : 
    if ( ! saving ) 
    deletemacref ( q ) ; 
    break ; 
  case 41 : 
    if ( q != 0 ) 
    if ( saving ) 
    mem [ q ] .hhfield .b1 = 1 ; 
    else {
	
      flushbelowvariable ( q ) ; 
      freenode ( q , 2 ) ; 
    } 
    break ; 
    default: 
    ; 
    break ; 
  } 
  eqtb [ p ] = eqtb [ 9769 ] ; 
} 
void zsavevariable ( q ) 
halfword q ; 
{halfword p  ; 
  if ( saveptr != 0 ) 
  {
    p = getnode ( 2 ) ; 
    mem [ p ] .hhfield .lhfield = q ; 
    mem [ p ] .hhfield .v.RH = saveptr ; 
    mem [ p + 1 ] .hhfield = eqtb [ q ] ; 
    saveptr = p ; 
  } 
  clearsymbol ( q , ( saveptr != 0 ) ) ; 
} 
void zsaveinternal ( q ) 
halfword q ; 
{halfword p  ; 
  if ( saveptr != 0 ) 
  {
    p = getnode ( 2 ) ; 
    mem [ p ] .hhfield .lhfield = 9769 + q ; 
    mem [ p ] .hhfield .v.RH = saveptr ; 
    mem [ p + 1 ] .cint = internal [ q ] ; 
    saveptr = p ; 
  } 
} 
void unsave ( ) 
{halfword q  ; 
  halfword p  ; 
  while ( mem [ saveptr ] .hhfield .lhfield != 0 ) {
      
    q = mem [ saveptr ] .hhfield .lhfield ; 
    if ( q > 9769 ) 
    {
      if ( internal [ 8 ] > 0 ) 
      {
	begindiagnostic () ; 
	printnl ( 515 ) ; 
	slowprint ( intname [ q - ( 9769 ) ] ) ; 
	printchar ( 61 ) ; 
	printscaled ( mem [ saveptr + 1 ] .cint ) ; 
	printchar ( 125 ) ; 
	enddiagnostic ( false ) ; 
      } 
      internal [ q - ( 9769 ) ] = mem [ saveptr + 1 ] .cint ; 
    } 
    else {
	
      if ( internal [ 8 ] > 0 ) 
      {
	begindiagnostic () ; 
	printnl ( 515 ) ; 
	slowprint ( hash [ q ] .v.RH ) ; 
	printchar ( 125 ) ; 
	enddiagnostic ( false ) ; 
      } 
      clearsymbol ( q , false ) ; 
      eqtb [ q ] = mem [ saveptr + 1 ] .hhfield ; 
      if ( eqtb [ q ] .lhfield % 86 == 41 ) 
      {
	p = eqtb [ q ] .v.RH ; 
	if ( p != 0 ) 
	mem [ p ] .hhfield .b1 = 0 ; 
      } 
    } 
    p = mem [ saveptr ] .hhfield .v.RH ; 
    freenode ( saveptr , 2 ) ; 
    saveptr = p ; 
  } 
  p = mem [ saveptr ] .hhfield .v.RH ; 
  {
    mem [ saveptr ] .hhfield .v.RH = avail ; 
    avail = saveptr ; 
	;
#ifdef STAT
    decr ( dynused ) ; 
#endif /* STAT */
  } 
  saveptr = p ; 
} 
halfword zcopyknot ( p ) 
halfword p ; 
{register halfword Result; halfword q  ; 
  schar k  ; 
  q = getnode ( 7 ) ; 
  {register integer for_end; k = 0 ; for_end = 6 ; if ( k <= for_end) do 
    mem [ q + k ] = mem [ p + k ] ; 
  while ( k++ < for_end ) ; } 
  Result = q ; 
  return(Result) ; 
} 
halfword zcopypath ( p ) 
halfword p ; 
{/* 10 */ register halfword Result; halfword q, pp, qq  ; 
  q = getnode ( 7 ) ; 
  qq = q ; 
  pp = p ; 
  while ( true ) {
      
    mem [ qq ] .hhfield .b0 = mem [ pp ] .hhfield .b0 ; 
    mem [ qq ] .hhfield .b1 = mem [ pp ] .hhfield .b1 ; 
    mem [ qq + 1 ] .cint = mem [ pp + 1 ] .cint ; 
    mem [ qq + 2 ] .cint = mem [ pp + 2 ] .cint ; 
    mem [ qq + 3 ] .cint = mem [ pp + 3 ] .cint ; 
    mem [ qq + 4 ] .cint = mem [ pp + 4 ] .cint ; 
    mem [ qq + 5 ] .cint = mem [ pp + 5 ] .cint ; 
    mem [ qq + 6 ] .cint = mem [ pp + 6 ] .cint ; 
    if ( mem [ pp ] .hhfield .v.RH == p ) 
    {
      mem [ qq ] .hhfield .v.RH = q ; 
      Result = q ; 
      goto lab10 ; 
    } 
    mem [ qq ] .hhfield .v.RH = getnode ( 7 ) ; 
    qq = mem [ qq ] .hhfield .v.RH ; 
    pp = mem [ pp ] .hhfield .v.RH ; 
  } 
  lab10: ; 
  return(Result) ; 
} 
halfword zhtapypoc ( p ) 
halfword p ; 
{/* 10 */ register halfword Result; halfword q, pp, qq, rr  ; 
  q = getnode ( 7 ) ; 
  qq = q ; 
  pp = p ; 
  while ( true ) {
      
    mem [ qq ] .hhfield .b1 = mem [ pp ] .hhfield .b0 ; 
    mem [ qq ] .hhfield .b0 = mem [ pp ] .hhfield .b1 ; 
    mem [ qq + 1 ] .cint = mem [ pp + 1 ] .cint ; 
    mem [ qq + 2 ] .cint = mem [ pp + 2 ] .cint ; 
    mem [ qq + 5 ] .cint = mem [ pp + 3 ] .cint ; 
    mem [ qq + 6 ] .cint = mem [ pp + 4 ] .cint ; 
    mem [ qq + 3 ] .cint = mem [ pp + 5 ] .cint ; 
    mem [ qq + 4 ] .cint = mem [ pp + 6 ] .cint ; 
    if ( mem [ pp ] .hhfield .v.RH == p ) 
    {
      mem [ q ] .hhfield .v.RH = qq ; 
      pathtail = pp ; 
      Result = q ; 
      goto lab10 ; 
    } 
    rr = getnode ( 7 ) ; 
    mem [ rr ] .hhfield .v.RH = qq ; 
    qq = rr ; 
    pp = mem [ pp ] .hhfield .v.RH ; 
  } 
  lab10: ; 
  return(Result) ; 
} 
fraction zcurlratio ( gamma , atension , btension ) 
scaled gamma ; 
scaled atension ; 
scaled btension ; 
{register fraction Result; fraction alpha, beta, num, denom, ff  ; 
  alpha = makefraction ( 65536L , atension ) ; 
  beta = makefraction ( 65536L , btension ) ; 
  if ( alpha <= beta ) 
  {
    ff = makefraction ( alpha , beta ) ; 
    ff = takefraction ( ff , ff ) ; 
    gamma = takefraction ( gamma , ff ) ; 
    beta = beta / 4096 ; 
    denom = takefraction ( gamma , alpha ) + 196608L - beta ; 
    num = takefraction ( gamma , 805306368L - alpha ) + beta ; 
  } 
  else {
      
    ff = makefraction ( beta , alpha ) ; 
    ff = takefraction ( ff , ff ) ; 
    beta = takefraction ( beta , ff ) / 4096 ; 
    denom = takefraction ( gamma , alpha ) + ( ff / 1365 ) - beta ; 
    num = takefraction ( gamma , 805306368L - alpha ) + beta ; 
  } 
  if ( num >= denom + denom + denom + denom ) 
  Result = 1073741824L ; 
  else Result = makefraction ( num , denom ) ; 
  return(Result) ; 
} 
void zsetcontrols ( p , q , k ) 
halfword p ; 
halfword q ; 
integer k ; 
{fraction rr, ss  ; 
  scaled lt, rt  ; 
  fraction sine  ; 
  lt = abs ( mem [ q + 4 ] .cint ) ; 
  rt = abs ( mem [ p + 6 ] .cint ) ; 
  rr = velocity ( st , ct , sf , cf , rt ) ; 
  ss = velocity ( sf , cf , st , ct , lt ) ; 
  if ( ( mem [ p + 6 ] .cint < 0 ) || ( mem [ q + 4 ] .cint < 0 ) ) 
  if ( ( ( st >= 0 ) && ( sf >= 0 ) ) || ( ( st <= 0 ) && ( sf <= 0 ) ) ) 
  {
    sine = takefraction ( abs ( st ) , cf ) + takefraction ( abs ( sf ) , ct ) 
    ; 
    if ( sine > 0 ) 
    {
      sine = takefraction ( sine , 268500992L ) ; 
      if ( mem [ p + 6 ] .cint < 0 ) 
      if ( abvscd ( abs ( sf ) , 268435456L , rr , sine ) < 0 ) 
      rr = makefraction ( abs ( sf ) , sine ) ; 
      if ( mem [ q + 4 ] .cint < 0 ) 
      if ( abvscd ( abs ( st ) , 268435456L , ss , sine ) < 0 ) 
      ss = makefraction ( abs ( st ) , sine ) ; 
    } 
  } 
  mem [ p + 5 ] .cint = mem [ p + 1 ] .cint + takefraction ( takefraction ( 
  deltax [ k ] , ct ) - takefraction ( deltay [ k ] , st ) , rr ) ; 
  mem [ p + 6 ] .cint = mem [ p + 2 ] .cint + takefraction ( takefraction ( 
  deltay [ k ] , ct ) + takefraction ( deltax [ k ] , st ) , rr ) ; 
  mem [ q + 3 ] .cint = mem [ q + 1 ] .cint - takefraction ( takefraction ( 
  deltax [ k ] , cf ) + takefraction ( deltay [ k ] , sf ) , ss ) ; 
  mem [ q + 4 ] .cint = mem [ q + 2 ] .cint - takefraction ( takefraction ( 
  deltay [ k ] , cf ) - takefraction ( deltax [ k ] , sf ) , ss ) ; 
  mem [ p ] .hhfield .b1 = 1 ; 
  mem [ q ] .hhfield .b0 = 1 ; 
} 
void zsolvechoices ( p , q , n ) 
halfword p ; 
halfword q ; 
halfword n ; 
{/* 40 10 */ integer k  ; 
  halfword r, s, t  ; 
  fraction aa, bb, cc, ff, acc  ; 
  scaled dd, ee  ; 
  scaled lt, rt  ; 
  k = 0 ; 
  s = p ; 
  while ( true ) {
      
    t = mem [ s ] .hhfield .v.RH ; 
    if ( k == 0 ) 
    switch ( mem [ s ] .hhfield .b1 ) 
    {case 2 : 
      if ( mem [ t ] .hhfield .b0 == 2 ) 
      {
	aa = narg ( deltax [ 0 ] , deltay [ 0 ] ) ; 
	nsincos ( mem [ p + 5 ] .cint - aa ) ; 
	ct = ncos ; 
	st = nsin ; 
	nsincos ( mem [ q + 3 ] .cint - aa ) ; 
	cf = ncos ; 
	sf = - (integer) nsin ; 
	setcontrols ( p , q , 0 ) ; 
	goto lab10 ; 
      } 
      else {
	  
	vv [ 0 ] = mem [ s + 5 ] .cint - narg ( deltax [ 0 ] , deltay [ 0 ] ) 
	; 
	if ( abs ( vv [ 0 ] ) > 188743680L ) 
	if ( vv [ 0 ] > 0 ) 
	vv [ 0 ] = vv [ 0 ] - 377487360L ; 
	else vv [ 0 ] = vv [ 0 ] + 377487360L ; 
	uu [ 0 ] = 0 ; 
	ww [ 0 ] = 0 ; 
      } 
      break ; 
    case 3 : 
      if ( mem [ t ] .hhfield .b0 == 3 ) 
      {
	mem [ p ] .hhfield .b1 = 1 ; 
	mem [ q ] .hhfield .b0 = 1 ; 
	lt = abs ( mem [ q + 4 ] .cint ) ; 
	rt = abs ( mem [ p + 6 ] .cint ) ; 
	if ( rt == 65536L ) 
	{
	  if ( deltax [ 0 ] >= 0 ) 
	  mem [ p + 5 ] .cint = mem [ p + 1 ] .cint + ( ( deltax [ 0 ] + 1 ) / 
	  3 ) ; 
	  else mem [ p + 5 ] .cint = mem [ p + 1 ] .cint + ( ( deltax [ 0 ] - 
	  1 ) / 3 ) ; 
	  if ( deltay [ 0 ] >= 0 ) 
	  mem [ p + 6 ] .cint = mem [ p + 2 ] .cint + ( ( deltay [ 0 ] + 1 ) / 
	  3 ) ; 
	  else mem [ p + 6 ] .cint = mem [ p + 2 ] .cint + ( ( deltay [ 0 ] - 
	  1 ) / 3 ) ; 
	} 
	else {
	    
	  ff = makefraction ( 65536L , 3 * rt ) ; 
	  mem [ p + 5 ] .cint = mem [ p + 1 ] .cint + takefraction ( deltax [ 
	  0 ] , ff ) ; 
	  mem [ p + 6 ] .cint = mem [ p + 2 ] .cint + takefraction ( deltay [ 
	  0 ] , ff ) ; 
	} 
	if ( lt == 65536L ) 
	{
	  if ( deltax [ 0 ] >= 0 ) 
	  mem [ q + 3 ] .cint = mem [ q + 1 ] .cint - ( ( deltax [ 0 ] + 1 ) / 
	  3 ) ; 
	  else mem [ q + 3 ] .cint = mem [ q + 1 ] .cint - ( ( deltax [ 0 ] - 
	  1 ) / 3 ) ; 
	  if ( deltay [ 0 ] >= 0 ) 
	  mem [ q + 4 ] .cint = mem [ q + 2 ] .cint - ( ( deltay [ 0 ] + 1 ) / 
	  3 ) ; 
	  else mem [ q + 4 ] .cint = mem [ q + 2 ] .cint - ( ( deltay [ 0 ] - 
	  1 ) / 3 ) ; 
	} 
	else {
	    
	  ff = makefraction ( 65536L , 3 * lt ) ; 
	  mem [ q + 3 ] .cint = mem [ q + 1 ] .cint - takefraction ( deltax [ 
	  0 ] , ff ) ; 
	  mem [ q + 4 ] .cint = mem [ q + 2 ] .cint - takefraction ( deltay [ 
	  0 ] , ff ) ; 
	} 
	goto lab10 ; 
      } 
      else {
	  
	cc = mem [ s + 5 ] .cint ; 
	lt = abs ( mem [ t + 4 ] .cint ) ; 
	rt = abs ( mem [ s + 6 ] .cint ) ; 
	if ( ( rt == 65536L ) && ( lt == 65536L ) ) 
	uu [ 0 ] = makefraction ( cc + cc + 65536L , cc + 131072L ) ; 
	else uu [ 0 ] = curlratio ( cc , rt , lt ) ; 
	vv [ 0 ] = - (integer) takefraction ( psi [ 1 ] , uu [ 0 ] ) ; 
	ww [ 0 ] = 0 ; 
      } 
      break ; 
    case 4 : 
      {
	uu [ 0 ] = 0 ; 
	vv [ 0 ] = 0 ; 
	ww [ 0 ] = 268435456L ; 
      } 
      break ; 
    } 
    else switch ( mem [ s ] .hhfield .b0 ) 
    {case 5 : 
    case 4 : 
      {
	if ( abs ( mem [ r + 6 ] .cint ) == 65536L ) 
	{
	  aa = 134217728L ; 
	  dd = 2 * delta [ k ] ; 
	} 
	else {
	    
	  aa = makefraction ( 65536L , 3 * abs ( mem [ r + 6 ] .cint ) - 
	  65536L ) ; 
	  dd = takefraction ( delta [ k ] , 805306368L - makefraction ( 65536L 
	  , abs ( mem [ r + 6 ] .cint ) ) ) ; 
	} 
	if ( abs ( mem [ t + 4 ] .cint ) == 65536L ) 
	{
	  bb = 134217728L ; 
	  ee = 2 * delta [ k - 1 ] ; 
	} 
	else {
	    
	  bb = makefraction ( 65536L , 3 * abs ( mem [ t + 4 ] .cint ) - 
	  65536L ) ; 
	  ee = takefraction ( delta [ k - 1 ] , 805306368L - makefraction ( 
	  65536L , abs ( mem [ t + 4 ] .cint ) ) ) ; 
	} 
	cc = 268435456L - takefraction ( uu [ k - 1 ] , aa ) ; 
	dd = takefraction ( dd , cc ) ; 
	lt = abs ( mem [ s + 4 ] .cint ) ; 
	rt = abs ( mem [ s + 6 ] .cint ) ; 
	if ( lt != rt ) 
	if ( lt < rt ) 
	{
	  ff = makefraction ( lt , rt ) ; 
	  ff = takefraction ( ff , ff ) ; 
	  dd = takefraction ( dd , ff ) ; 
	} 
	else {
	    
	  ff = makefraction ( rt , lt ) ; 
	  ff = takefraction ( ff , ff ) ; 
	  ee = takefraction ( ee , ff ) ; 
	} 
	ff = makefraction ( ee , ee + dd ) ; 
	uu [ k ] = takefraction ( ff , bb ) ; 
	acc = - (integer) takefraction ( psi [ k + 1 ] , uu [ k ] ) ; 
	if ( mem [ r ] .hhfield .b1 == 3 ) 
	{
	  ww [ k ] = 0 ; 
	  vv [ k ] = acc - takefraction ( psi [ 1 ] , 268435456L - ff ) ; 
	} 
	else {
	    
	  ff = makefraction ( 268435456L - ff , cc ) ; 
	  acc = acc - takefraction ( psi [ k ] , ff ) ; 
	  ff = takefraction ( ff , aa ) ; 
	  vv [ k ] = acc - takefraction ( vv [ k - 1 ] , ff ) ; 
	  if ( ww [ k - 1 ] == 0 ) 
	  ww [ k ] = 0 ; 
	  else ww [ k ] = - (integer) takefraction ( ww [ k - 1 ] , ff ) ; 
	} 
	if ( mem [ s ] .hhfield .b0 == 5 ) 
	{
	  aa = 0 ; 
	  bb = 268435456L ; 
	  do {
	      decr ( k ) ; 
	    if ( k == 0 ) 
	    k = n ; 
	    aa = vv [ k ] - takefraction ( aa , uu [ k ] ) ; 
	    bb = ww [ k ] - takefraction ( bb , uu [ k ] ) ; 
	  } while ( ! ( k == n ) ) ; 
	  aa = makefraction ( aa , 268435456L - bb ) ; 
	  theta [ n ] = aa ; 
	  vv [ 0 ] = aa ; 
	  {register integer for_end; k = 1 ; for_end = n - 1 ; if ( k <= 
	  for_end) do 
	    vv [ k ] = vv [ k ] + takefraction ( aa , ww [ k ] ) ; 
	  while ( k++ < for_end ) ; } 
	  goto lab40 ; 
	} 
      } 
      break ; 
    case 3 : 
      {
	cc = mem [ s + 3 ] .cint ; 
	lt = abs ( mem [ s + 4 ] .cint ) ; 
	rt = abs ( mem [ r + 6 ] .cint ) ; 
	if ( ( rt == 65536L ) && ( lt == 65536L ) ) 
	ff = makefraction ( cc + cc + 65536L , cc + 131072L ) ; 
	else ff = curlratio ( cc , lt , rt ) ; 
	theta [ n ] = - (integer) makefraction ( takefraction ( vv [ n - 1 ] , 
	ff ) , 268435456L - takefraction ( ff , uu [ n - 1 ] ) ) ; 
	goto lab40 ; 
      } 
      break ; 
    case 2 : 
      {
	theta [ n ] = mem [ s + 3 ] .cint - narg ( deltax [ n - 1 ] , deltay [ 
	n - 1 ] ) ; 
	if ( abs ( theta [ n ] ) > 188743680L ) 
	if ( theta [ n ] > 0 ) 
	theta [ n ] = theta [ n ] - 377487360L ; 
	else theta [ n ] = theta [ n ] + 377487360L ; 
	goto lab40 ; 
      } 
      break ; 
    } 
    r = s ; 
    s = t ; 
    incr ( k ) ; 
  } 
  lab40: {
      register integer for_end; k = n - 1 ; for_end = 0 ; if ( k >= 
  for_end) do 
    theta [ k ] = vv [ k ] - takefraction ( theta [ k + 1 ] , uu [ k ] ) ; 
  while ( k-- > for_end ) ; } 
  s = p ; 
  k = 0 ; 
  do {
      t = mem [ s ] .hhfield .v.RH ; 
    nsincos ( theta [ k ] ) ; 
    st = nsin ; 
    ct = ncos ; 
    nsincos ( - (integer) psi [ k + 1 ] - theta [ k + 1 ] ) ; 
    sf = nsin ; 
    cf = ncos ; 
    setcontrols ( s , t , k ) ; 
    incr ( k ) ; 
    s = t ; 
  } while ( ! ( k == n ) ) ; 
  lab10: ; 
} 
void zmakechoices ( knots ) 
halfword knots ; 
{/* 30 */ halfword h  ; 
  halfword p, q  ; 
  integer k, n  ; 
  halfword s, t  ; 
  scaled delx, dely  ; 
  fraction sine, cosine  ; 
  {
    if ( aritherror ) 
    cleararith () ; 
  } 
  if ( internal [ 4 ] > 0 ) 
  printpath ( knots , 525 , true ) ; 
  p = knots ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    if ( mem [ p + 1 ] .cint == mem [ q + 1 ] .cint ) 
    if ( mem [ p + 2 ] .cint == mem [ q + 2 ] .cint ) 
    if ( mem [ p ] .hhfield .b1 > 1 ) 
    {
      mem [ p ] .hhfield .b1 = 1 ; 
      if ( mem [ p ] .hhfield .b0 == 4 ) 
      {
	mem [ p ] .hhfield .b0 = 3 ; 
	mem [ p + 3 ] .cint = 65536L ; 
      } 
      mem [ q ] .hhfield .b0 = 1 ; 
      if ( mem [ q ] .hhfield .b1 == 4 ) 
      {
	mem [ q ] .hhfield .b1 = 3 ; 
	mem [ q + 5 ] .cint = 65536L ; 
      } 
      mem [ p + 5 ] .cint = mem [ p + 1 ] .cint ; 
      mem [ q + 3 ] .cint = mem [ p + 1 ] .cint ; 
      mem [ p + 6 ] .cint = mem [ p + 2 ] .cint ; 
      mem [ q + 4 ] .cint = mem [ p + 2 ] .cint ; 
    } 
    p = q ; 
  } while ( ! ( p == knots ) ) ; 
  h = knots ; 
  while ( true ) {
      
    if ( mem [ h ] .hhfield .b0 != 4 ) 
    goto lab30 ; 
    if ( mem [ h ] .hhfield .b1 != 4 ) 
    goto lab30 ; 
    h = mem [ h ] .hhfield .v.RH ; 
    if ( h == knots ) 
    {
      mem [ h ] .hhfield .b0 = 5 ; 
      goto lab30 ; 
    } 
  } 
  lab30: ; 
  p = h ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    if ( mem [ p ] .hhfield .b1 >= 2 ) 
    {
      while ( ( mem [ q ] .hhfield .b0 == 4 ) && ( mem [ q ] .hhfield .b1 == 4 
      ) ) q = mem [ q ] .hhfield .v.RH ; 
      k = 0 ; 
      s = p ; 
      n = pathsize ; 
      do {
	  t = mem [ s ] .hhfield .v.RH ; 
	deltax [ k ] = mem [ t + 1 ] .cint - mem [ s + 1 ] .cint ; 
	deltay [ k ] = mem [ t + 2 ] .cint - mem [ s + 2 ] .cint ; 
	delta [ k ] = pythadd ( deltax [ k ] , deltay [ k ] ) ; 
	if ( k > 0 ) 
	{
	  sine = makefraction ( deltay [ k - 1 ] , delta [ k - 1 ] ) ; 
	  cosine = makefraction ( deltax [ k - 1 ] , delta [ k - 1 ] ) ; 
	  psi [ k ] = narg ( takefraction ( deltax [ k ] , cosine ) + 
	  takefraction ( deltay [ k ] , sine ) , takefraction ( deltay [ k ] , 
	  cosine ) - takefraction ( deltax [ k ] , sine ) ) ; 
	} 
	incr ( k ) ; 
	s = t ; 
	if ( k == pathsize ) 
	overflow ( 530 , pathsize ) ; 
	if ( s == q ) 
	n = k ; 
      } while ( ! ( ( k >= n ) && ( mem [ s ] .hhfield .b0 != 5 ) ) ) ; 
      if ( k == n ) 
      psi [ n ] = 0 ; 
      else psi [ k ] = psi [ 1 ] ; 
      if ( mem [ q ] .hhfield .b0 == 4 ) 
      {
	delx = mem [ q + 5 ] .cint - mem [ q + 1 ] .cint ; 
	dely = mem [ q + 6 ] .cint - mem [ q + 2 ] .cint ; 
	if ( ( delx == 0 ) && ( dely == 0 ) ) 
	{
	  mem [ q ] .hhfield .b0 = 3 ; 
	  mem [ q + 3 ] .cint = 65536L ; 
	} 
	else {
	    
	  mem [ q ] .hhfield .b0 = 2 ; 
	  mem [ q + 3 ] .cint = narg ( delx , dely ) ; 
	} 
      } 
      if ( ( mem [ p ] .hhfield .b1 == 4 ) && ( mem [ p ] .hhfield .b0 == 1 ) 
      ) 
      {
	delx = mem [ p + 1 ] .cint - mem [ p + 3 ] .cint ; 
	dely = mem [ p + 2 ] .cint - mem [ p + 4 ] .cint ; 
	if ( ( delx == 0 ) && ( dely == 0 ) ) 
	{
	  mem [ p ] .hhfield .b1 = 3 ; 
	  mem [ p + 5 ] .cint = 65536L ; 
	} 
	else {
	    
	  mem [ p ] .hhfield .b1 = 2 ; 
	  mem [ p + 5 ] .cint = narg ( delx , dely ) ; 
	} 
      } 
      solvechoices ( p , q , n ) ; 
    } 
    p = q ; 
  } while ( ! ( p == h ) ) ; 
  if ( internal [ 4 ] > 0 ) 
  printpath ( knots , 526 , true ) ; 
  if ( aritherror ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 527 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 528 ; 
      helpline [ 0 ] = 529 ; 
    } 
    putgeterror () ; 
    aritherror = false ; 
  } 
} 
void zmakemoves ( xx0 , xx1 , xx2 , xx3 , yy0 , yy1 , yy2 , yy3 , xicorr , 
etacorr ) 
scaled xx0 ; 
scaled xx1 ; 
scaled xx2 ; 
scaled xx3 ; 
scaled yy0 ; 
scaled yy1 ; 
scaled yy2 ; 
scaled yy3 ; 
smallnumber xicorr ; 
smallnumber etacorr ; 
{/* 22 30 10 */ integer x1, x2, x3, m, r, y1, y2, y3, n, s, l  ; 
  integer q, t, u, x2a, x3a, y2a, y3a  ; 
  if ( ( xx3 < xx0 ) || ( yy3 < yy0 ) ) 
  confusion ( 109 ) ; 
  l = 16 ; 
  bisectptr = 0 ; 
  x1 = xx1 - xx0 ; 
  x2 = xx2 - xx1 ; 
  x3 = xx3 - xx2 ; 
  if ( xx0 >= xicorr ) 
  r = ( xx0 - xicorr ) % 65536L ; 
  else r = 65535L - ( ( - (integer) xx0 + xicorr - 1 ) % 65536L ) ; 
  m = ( xx3 - xx0 + r ) / 65536L ; 
  y1 = yy1 - yy0 ; 
  y2 = yy2 - yy1 ; 
  y3 = yy3 - yy2 ; 
  if ( yy0 >= etacorr ) 
  s = ( yy0 - etacorr ) % 65536L ; 
  else s = 65535L - ( ( - (integer) yy0 + etacorr - 1 ) % 65536L ) ; 
  n = ( yy3 - yy0 + s ) / 65536L ; 
  if ( ( xx3 - xx0 >= 268435456L ) || ( yy3 - yy0 >= 268435456L ) ) 
  {
    x1 = ( x1 + xicorr ) / 2 ; 
    x2 = ( x2 + xicorr ) / 2 ; 
    x3 = ( x3 + xicorr ) / 2 ; 
    r = ( r + xicorr ) / 2 ; 
    y1 = ( y1 + etacorr ) / 2 ; 
    y2 = ( y2 + etacorr ) / 2 ; 
    y3 = ( y3 + etacorr ) / 2 ; 
    s = ( s + etacorr ) / 2 ; 
    l = 15 ; 
  } 
  while ( true ) {
      
    lab22: if ( m == 0 ) 
    while ( n > 0 ) {
	
      incr ( moveptr ) ; 
      move [ moveptr ] = 1 ; 
      decr ( n ) ; 
    } 
    else if ( n == 0 ) 
    move [ moveptr ] = move [ moveptr ] + m ; 
    else if ( m + n == 2 ) 
    {
      r = twotothe [ l ] - r ; 
      s = twotothe [ l ] - s ; 
      while ( l < 30 ) {
	  
	x3a = x3 ; 
	x2a = ( x2 + x3 + xicorr ) / 2 ; 
	x2 = ( x1 + x2 + xicorr ) / 2 ; 
	x3 = ( x2 + x2a + xicorr ) / 2 ; 
	t = x1 + x2 + x3 ; 
	r = r + r - xicorr ; 
	y3a = y3 ; 
	y2a = ( y2 + y3 + etacorr ) / 2 ; 
	y2 = ( y1 + y2 + etacorr ) / 2 ; 
	y3 = ( y2 + y2a + etacorr ) / 2 ; 
	u = y1 + y2 + y3 ; 
	s = s + s - etacorr ; 
	if ( t < r ) 
	if ( u < s ) 
	{
	  x1 = x3 ; 
	  x2 = x2a ; 
	  x3 = x3a ; 
	  r = r - t ; 
	  y1 = y3 ; 
	  y2 = y2a ; 
	  y3 = y3a ; 
	  s = s - u ; 
	} 
	else {
	    
	  {
	    incr ( moveptr ) ; 
	    move [ moveptr ] = 2 ; 
	  } 
	  goto lab30 ; 
	} 
	else if ( u < s ) 
	{
	  {
	    incr ( move [ moveptr ] ) ; 
	    incr ( moveptr ) ; 
	    move [ moveptr ] = 1 ; 
	  } 
	  goto lab30 ; 
	} 
	incr ( l ) ; 
      } 
      r = r - xicorr ; 
      s = s - etacorr ; 
      if ( abvscd ( x1 + x2 + x3 , s , y1 + y2 + y3 , r ) - xicorr >= 0 ) 
      {
	incr ( move [ moveptr ] ) ; 
	incr ( moveptr ) ; 
	move [ moveptr ] = 1 ; 
      } 
      else {
	  
	incr ( moveptr ) ; 
	move [ moveptr ] = 2 ; 
      } 
      lab30: ; 
    } 
    else {
	
      incr ( l ) ; 
      bisectstack [ bisectptr + 10 ] = l ; 
      bisectstack [ bisectptr + 2 ] = x3 ; 
      bisectstack [ bisectptr + 1 ] = ( x2 + x3 + xicorr ) / 2 ; 
      x2 = ( x1 + x2 + xicorr ) / 2 ; 
      x3 = ( x2 + bisectstack [ bisectptr + 1 ] + xicorr ) / 2 ; 
      bisectstack [ bisectptr ] = x3 ; 
      r = r + r + xicorr ; 
      t = x1 + x2 + x3 + r ; 
      q = t / twotothe [ l ] ; 
      bisectstack [ bisectptr + 3 ] = t % twotothe [ l ] ; 
      bisectstack [ bisectptr + 4 ] = m - q ; 
      m = q ; 
      bisectstack [ bisectptr + 7 ] = y3 ; 
      bisectstack [ bisectptr + 6 ] = ( y2 + y3 + etacorr ) / 2 ; 
      y2 = ( y1 + y2 + etacorr ) / 2 ; 
      y3 = ( y2 + bisectstack [ bisectptr + 6 ] + etacorr ) / 2 ; 
      bisectstack [ bisectptr + 5 ] = y3 ; 
      s = s + s + etacorr ; 
      u = y1 + y2 + y3 + s ; 
      q = u / twotothe [ l ] ; 
      bisectstack [ bisectptr + 8 ] = u % twotothe [ l ] ; 
      bisectstack [ bisectptr + 9 ] = n - q ; 
      n = q ; 
      bisectptr = bisectptr + 11 ; 
      goto lab22 ; 
    } 
    if ( bisectptr == 0 ) 
    goto lab10 ; 
    bisectptr = bisectptr - 11 ; 
    x1 = bisectstack [ bisectptr ] ; 
    x2 = bisectstack [ bisectptr + 1 ] ; 
    x3 = bisectstack [ bisectptr + 2 ] ; 
    r = bisectstack [ bisectptr + 3 ] ; 
    m = bisectstack [ bisectptr + 4 ] ; 
    y1 = bisectstack [ bisectptr + 5 ] ; 
    y2 = bisectstack [ bisectptr + 6 ] ; 
    y3 = bisectstack [ bisectptr + 7 ] ; 
    s = bisectstack [ bisectptr + 8 ] ; 
    n = bisectstack [ bisectptr + 9 ] ; 
    l = bisectstack [ bisectptr + 10 ] ; 
  } 
  lab10: ; 
} 
void zsmoothmoves ( b , t ) 
integer b ; 
integer t ; 
{integer k  ; 
  integer a, aa, aaa  ; 
  if ( t - b >= 3 ) 
  {
    k = b + 2 ; 
    aa = move [ k - 1 ] ; 
    aaa = move [ k - 2 ] ; 
    do {
	a = move [ k ] ; 
      if ( abs ( a - aa ) > 1 ) 
      if ( a > aa ) 
      {
	if ( aaa >= aa ) 
	if ( a >= move [ k + 1 ] ) 
	{
	  incr ( move [ k - 1 ] ) ; 
	  move [ k ] = a - 1 ; 
	} 
      } 
      else {
	  
	if ( aaa <= aa ) 
	if ( a <= move [ k + 1 ] ) 
	{
	  decr ( move [ k - 1 ] ) ; 
	  move [ k ] = a + 1 ; 
	} 
      } 
      incr ( k ) ; 
      aaa = aa ; 
      aa = a ; 
    } while ( ! ( k == t ) ) ; 
  } 
} 
void zinitedges ( h ) 
halfword h ; 
{mem [ h ] .hhfield .lhfield = h ; 
  mem [ h ] .hhfield .v.RH = h ; 
  mem [ h + 1 ] .hhfield .lhfield = 8191 ; 
  mem [ h + 1 ] .hhfield .v.RH = 1 ; 
  mem [ h + 2 ] .hhfield .lhfield = 8191 ; 
  mem [ h + 2 ] .hhfield .v.RH = 1 ; 
  mem [ h + 3 ] .hhfield .lhfield = 4096 ; 
  mem [ h + 3 ] .hhfield .v.RH = 0 ; 
  mem [ h + 4 ] .cint = 0 ; 
  mem [ h + 5 ] .hhfield .v.RH = h ; 
  mem [ h + 5 ] .hhfield .lhfield = 0 ; 
} 
void fixoffset ( ) 
{halfword p, q  ; 
  integer delta  ; 
  delta = 8 * ( mem [ curedges + 3 ] .hhfield .lhfield - 4096 ) ; 
  mem [ curedges + 3 ] .hhfield .lhfield = 4096 ; 
  q = mem [ curedges ] .hhfield .v.RH ; 
  while ( q != curedges ) {
      
    p = mem [ q + 1 ] .hhfield .v.RH ; 
    while ( p != memtop ) {
	
      mem [ p ] .hhfield .lhfield = mem [ p ] .hhfield .lhfield - delta ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } 
    p = mem [ q + 1 ] .hhfield .lhfield ; 
    while ( p > 1 ) {
	
      mem [ p ] .hhfield .lhfield = mem [ p ] .hhfield .lhfield - delta ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } 
    q = mem [ q ] .hhfield .v.RH ; 
  } 
} 
void zedgeprep ( ml , mr , nl , nr ) 
integer ml ; 
integer mr ; 
integer nl ; 
integer nr ; 
{halfword delta  ; 
  integer temp  ; 
  halfword p, q  ; 
  ml = ml + 4096 ; 
  mr = mr + 4096 ; 
  nl = nl + 4096 ; 
  nr = nr + 4095 ; 
  if ( ml < mem [ curedges + 2 ] .hhfield .lhfield ) 
  mem [ curedges + 2 ] .hhfield .lhfield = ml ; 
  if ( mr > mem [ curedges + 2 ] .hhfield .v.RH ) 
  mem [ curedges + 2 ] .hhfield .v.RH = mr ; 
  temp = mem [ curedges + 3 ] .hhfield .lhfield - 4096 ; 
  if ( ! ( abs ( mem [ curedges + 2 ] .hhfield .lhfield + temp - 4096 ) < 4096 
  ) || ! ( abs ( mem [ curedges + 2 ] .hhfield .v.RH + temp - 4096 ) < 4096 ) 
  ) 
  fixoffset () ; 
  if ( mem [ curedges ] .hhfield .v.RH == curedges ) 
  {
    mem [ curedges + 1 ] .hhfield .lhfield = nr + 1 ; 
    mem [ curedges + 1 ] .hhfield .v.RH = nr ; 
  } 
  if ( nl < mem [ curedges + 1 ] .hhfield .lhfield ) 
  {
    delta = mem [ curedges + 1 ] .hhfield .lhfield - nl ; 
    mem [ curedges + 1 ] .hhfield .lhfield = nl ; 
    p = mem [ curedges ] .hhfield .v.RH ; 
    do {
	q = getnode ( 2 ) ; 
      mem [ q + 1 ] .hhfield .v.RH = memtop ; 
      mem [ q + 1 ] .hhfield .lhfield = 1 ; 
      mem [ p ] .hhfield .lhfield = q ; 
      mem [ q ] .hhfield .v.RH = p ; 
      p = q ; 
      decr ( delta ) ; 
    } while ( ! ( delta == 0 ) ) ; 
    mem [ p ] .hhfield .lhfield = curedges ; 
    mem [ curedges ] .hhfield .v.RH = p ; 
    if ( mem [ curedges + 5 ] .hhfield .v.RH == curedges ) 
    mem [ curedges + 5 ] .hhfield .lhfield = nl - 1 ; 
  } 
  if ( nr > mem [ curedges + 1 ] .hhfield .v.RH ) 
  {
    delta = nr - mem [ curedges + 1 ] .hhfield .v.RH ; 
    mem [ curedges + 1 ] .hhfield .v.RH = nr ; 
    p = mem [ curedges ] .hhfield .lhfield ; 
    do {
	q = getnode ( 2 ) ; 
      mem [ q + 1 ] .hhfield .v.RH = memtop ; 
      mem [ q + 1 ] .hhfield .lhfield = 1 ; 
      mem [ p ] .hhfield .v.RH = q ; 
      mem [ q ] .hhfield .lhfield = p ; 
      p = q ; 
      decr ( delta ) ; 
    } while ( ! ( delta == 0 ) ) ; 
    mem [ p ] .hhfield .v.RH = curedges ; 
    mem [ curedges ] .hhfield .lhfield = p ; 
    if ( mem [ curedges + 5 ] .hhfield .v.RH == curedges ) 
    mem [ curedges + 5 ] .hhfield .lhfield = nr + 1 ; 
  } 
} 
halfword zcopyedges ( h ) 
halfword h ; 
{register halfword Result; halfword p, r  ; 
  halfword hh, pp, qq, rr, ss  ; 
  hh = getnode ( 6 ) ; 
  mem [ hh + 1 ] = mem [ h + 1 ] ; 
  mem [ hh + 2 ] = mem [ h + 2 ] ; 
  mem [ hh + 3 ] = mem [ h + 3 ] ; 
  mem [ hh + 4 ] = mem [ h + 4 ] ; 
  mem [ hh + 5 ] .hhfield .lhfield = mem [ hh + 1 ] .hhfield .v.RH + 1 ; 
  mem [ hh + 5 ] .hhfield .v.RH = hh ; 
  p = mem [ h ] .hhfield .v.RH ; 
  qq = hh ; 
  while ( p != h ) {
      
    pp = getnode ( 2 ) ; 
    mem [ qq ] .hhfield .v.RH = pp ; 
    mem [ pp ] .hhfield .lhfield = qq ; 
    r = mem [ p + 1 ] .hhfield .v.RH ; 
    rr = pp + 1 ; 
    while ( r != memtop ) {
	
      ss = getavail () ; 
      mem [ rr ] .hhfield .v.RH = ss ; 
      rr = ss ; 
      mem [ rr ] .hhfield .lhfield = mem [ r ] .hhfield .lhfield ; 
      r = mem [ r ] .hhfield .v.RH ; 
    } 
    mem [ rr ] .hhfield .v.RH = memtop ; 
    r = mem [ p + 1 ] .hhfield .lhfield ; 
    rr = memtop - 1 ; 
    while ( r > 1 ) {
	
      ss = getavail () ; 
      mem [ rr ] .hhfield .v.RH = ss ; 
      rr = ss ; 
      mem [ rr ] .hhfield .lhfield = mem [ r ] .hhfield .lhfield ; 
      r = mem [ r ] .hhfield .v.RH ; 
    } 
    mem [ rr ] .hhfield .v.RH = r ; 
    mem [ pp + 1 ] .hhfield .lhfield = mem [ memtop - 1 ] .hhfield .v.RH ; 
    p = mem [ p ] .hhfield .v.RH ; 
    qq = pp ; 
  } 
  mem [ qq ] .hhfield .v.RH = hh ; 
  mem [ hh ] .hhfield .lhfield = qq ; 
  Result = hh ; 
  return(Result) ; 
} 
void yreflectedges ( ) 
{halfword p, q, r  ; 
  p = mem [ curedges + 1 ] .hhfield .lhfield ; 
  mem [ curedges + 1 ] .hhfield .lhfield = 8191 - mem [ curedges + 1 ] 
  .hhfield .v.RH ; 
  mem [ curedges + 1 ] .hhfield .v.RH = 8191 - p ; 
  mem [ curedges + 5 ] .hhfield .lhfield = 8191 - mem [ curedges + 5 ] 
  .hhfield .lhfield ; 
  p = mem [ curedges ] .hhfield .v.RH ; 
  q = curedges ; 
  do {
      r = mem [ p ] .hhfield .v.RH ; 
    mem [ p ] .hhfield .v.RH = q ; 
    mem [ q ] .hhfield .lhfield = p ; 
    q = p ; 
    p = r ; 
  } while ( ! ( q == curedges ) ) ; 
  mem [ curedges + 4 ] .cint = 0 ; 
} 
