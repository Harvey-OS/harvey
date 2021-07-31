#define EXTERN extern
#include "mfd.h"

void znsincos ( z ) 
angle z ; 
{smallnumber k  ; 
  schar q  ; 
  fraction r  ; 
  integer x, y, t  ; 
  while ( z < 0 ) z = z + 377487360L ; 
  z = z % 377487360L ; 
  q = z / 47185920L ; 
  z = z % 47185920L ; 
  x = 268435456L ; 
  y = x ; 
  if ( ! odd ( q ) ) 
  z = 47185920L - z ; 
  k = 1 ; 
  while ( z > 0 ) {
      
    if ( z >= specatan [ k ] ) 
    {
      z = z - specatan [ k ] ; 
      t = x ; 
      x = t + y / twotothe [ k ] ; 
      y = y - t / twotothe [ k ] ; 
    } 
    incr ( k ) ; 
  } 
  if ( y < 0 ) 
  y = 0 ; 
  switch ( q ) 
  {case 0 : 
    ; 
    break ; 
  case 1 : 
    {
      t = x ; 
      x = y ; 
      y = t ; 
    } 
    break ; 
  case 2 : 
    {
      t = x ; 
      x = - (integer) y ; 
      y = t ; 
    } 
    break ; 
  case 3 : 
    x = - (integer) x ; 
    break ; 
  case 4 : 
    {
      x = - (integer) x ; 
      y = - (integer) y ; 
    } 
    break ; 
  case 5 : 
    {
      t = x ; 
      x = - (integer) y ; 
      y = - (integer) t ; 
    } 
    break ; 
  case 6 : 
    {
      t = x ; 
      x = y ; 
      y = - (integer) t ; 
    } 
    break ; 
  case 7 : 
    y = - (integer) y ; 
    break ; 
  } 
  r = pythadd ( x , y ) ; 
  ncos = makefraction ( x , r ) ; 
  nsin = makefraction ( y , r ) ; 
} 
void newrandoms ( ) 
{schar k  ; 
  fraction x  ; 
  {register integer for_end; k = 0 ; for_end = 23 ; if ( k <= for_end) do 
    {
      x = randoms [ k ] - randoms [ k + 31 ] ; 
      if ( x < 0 ) 
      x = x + 268435456L ; 
      randoms [ k ] = x ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = 24 ; for_end = 54 ; if ( k <= for_end) do 
    {
      x = randoms [ k ] - randoms [ k - 24 ] ; 
      if ( x < 0 ) 
      x = x + 268435456L ; 
      randoms [ k ] = x ; 
    } 
  while ( k++ < for_end ) ; } 
  jrandom = 54 ; 
} 
void zinitrandoms ( seed ) 
scaled seed ; 
{fraction j, jj, k  ; 
  schar i  ; 
  j = abs ( seed ) ; 
  while ( j >= 268435456L ) j = ( j ) / 2 ; 
  k = 1 ; 
  {register integer for_end; i = 0 ; for_end = 54 ; if ( i <= for_end) do 
    {
      jj = k ; 
      k = j - k ; 
      j = jj ; 
      if ( k < 0 ) 
      k = k + 268435456L ; 
      randoms [ ( i * 21 ) % 55 ] = j ; 
    } 
  while ( i++ < for_end ) ; } 
  newrandoms () ; 
  newrandoms () ; 
  newrandoms () ; 
} 
scaled zunifrand ( x ) 
scaled x ; 
{register scaled Result; scaled y  ; 
  if ( jrandom == 0 ) 
  newrandoms () ; 
  else decr ( jrandom ) ; 
  y = takefraction ( abs ( x ) , randoms [ jrandom ] ) ; 
  if ( y == abs ( x ) ) 
  Result = 0 ; 
  else if ( x > 0 ) 
  Result = y ; 
  else Result = - (integer) y ; 
  return(Result) ; 
} 
scaled normrand ( ) 
{register scaled Result; integer x, u, l  ; 
  do {
      do { if ( jrandom == 0 ) 
      newrandoms () ; 
      else decr ( jrandom ) ; 
      x = takefraction ( 112429L , randoms [ jrandom ] - 134217728L ) ; 
      if ( jrandom == 0 ) 
      newrandoms () ; 
      else decr ( jrandom ) ; 
      u = randoms [ jrandom ] ; 
    } while ( ! ( abs ( x ) < u ) ) ; 
    x = makefraction ( x , u ) ; 
    l = 139548960L - mlog ( u ) ; 
  } while ( ! ( abvscd ( 1024 , l , x , x ) >= 0 ) ) ; 
  Result = x ; 
  return(Result) ; 
} 
#ifdef DEBUG
void zprintword ( w ) 
memoryword w ; 
{printint ( w .cint ) ; 
  printchar ( 32 ) ; 
  printscaled ( w .cint ) ; 
  printchar ( 32 ) ; 
  printscaled ( w .cint / 4096 ) ; 
  println () ; 
  printint ( w .hhfield .lhfield ) ; 
  printchar ( 61 ) ; 
  printint ( w .hhfield .b0 ) ; 
  printchar ( 58 ) ; 
  printint ( w .hhfield .b1 ) ; 
  printchar ( 59 ) ; 
  printint ( w .hhfield .v.RH ) ; 
  printchar ( 32 ) ; 
  printint ( w .qqqq .b0 ) ; 
  printchar ( 58 ) ; 
  printint ( w .qqqq .b1 ) ; 
  printchar ( 58 ) ; 
  printint ( w .qqqq .b2 ) ; 
  printchar ( 58 ) ; 
  printint ( w .qqqq .b3 ) ; 
} 
#endif /* DEBUG */
void zshowtokenlist ( p , q , l , nulltally ) 
integer p ; 
integer q ; 
integer l ; 
integer nulltally ; 
{/* 10 */ smallnumber class, c  ; 
  integer r, v  ; 
  class = 3 ; 
  tally = nulltally ; 
  while ( ( p != 0 ) && ( tally < l ) ) {
      
    if ( p == q ) 
    {
      firstcount = tally ; 
      trickcount = tally + 1 + errorline - halferrorline ; 
      if ( trickcount < errorline ) 
      trickcount = errorline ; 
    } 
    c = 9 ; 
    if ( ( p < 0 ) || ( p > memend ) ) 
    {
      print ( 492 ) ; 
      goto lab10 ; 
    } 
    if ( p < himemmin ) 
    if ( mem [ p ] .hhfield .b1 == 12 ) 
    if ( mem [ p ] .hhfield .b0 == 16 ) 
    {
      if ( class == 0 ) 
      printchar ( 32 ) ; 
      v = mem [ p + 1 ] .cint ; 
      if ( v < 0 ) 
      {
	if ( class == 17 ) 
	printchar ( 32 ) ; 
	printchar ( 91 ) ; 
	printscaled ( v ) ; 
	printchar ( 93 ) ; 
	c = 18 ; 
      } 
      else {
	  
	printscaled ( v ) ; 
	c = 0 ; 
      } 
    } 
    else if ( mem [ p ] .hhfield .b0 != 4 ) 
    print ( 495 ) ; 
    else {
	
      printchar ( 34 ) ; 
      slowprint ( mem [ p + 1 ] .cint ) ; 
      printchar ( 34 ) ; 
      c = 4 ; 
    } 
    else if ( ( mem [ p ] .hhfield .b1 != 11 ) || ( mem [ p ] .hhfield .b0 < 1 
    ) || ( mem [ p ] .hhfield .b0 > 19 ) ) 
    print ( 495 ) ; 
    else {
	
      gpointer = p ; 
      printcapsule () ; 
      c = 8 ; 
    } 
    else {
	
      r = mem [ p ] .hhfield .lhfield ; 
      if ( r >= 9770 ) 
      {
	if ( r < 9920 ) 
	{
	  print ( 497 ) ; 
	  r = r - ( 9770 ) ; 
	} 
	else if ( r < 10070 ) 
	{
	  print ( 498 ) ; 
	  r = r - ( 9920 ) ; 
	} 
	else {
	    
	  print ( 499 ) ; 
	  r = r - ( 10070 ) ; 
	} 
	printint ( r ) ; 
	printchar ( 41 ) ; 
	c = 8 ; 
      } 
      else if ( r < 1 ) 
      if ( r == 0 ) 
      {
	if ( class == 17 ) 
	printchar ( 32 ) ; 
	print ( 496 ) ; 
	c = 18 ; 
      } 
      else print ( 493 ) ; 
      else {
	  
	r = hash [ r ] .v.RH ; 
	if ( ( r < 0 ) || ( r >= strptr ) ) 
	print ( 494 ) ; 
	else {
	    
	  c = charclass [ strpool [ strstart [ r ] ] ] ; 
	  if ( c == class ) 
	  switch ( c ) 
	  {case 9 : 
	    printchar ( 46 ) ; 
	    break ; 
	  case 5 : 
	  case 6 : 
	  case 7 : 
	  case 8 : 
	    ; 
	    break ; 
	    default: 
	    printchar ( 32 ) ; 
	    break ; 
	  } 
	  slowprint ( r ) ; 
	} 
      } 
    } 
    class = c ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  if ( p != 0 ) 
  print ( 491 ) ; 
  lab10: ; 
} 
void runaway ( ) 
{if ( scannerstatus > 2 ) 
  {
    printnl ( 635 ) ; 
    switch ( scannerstatus ) 
    {case 3 : 
      print ( 636 ) ; 
      break ; 
    case 4 : 
    case 5 : 
      print ( 637 ) ; 
      break ; 
    case 6 : 
      print ( 638 ) ; 
      break ; 
    } 
    println () ; 
    showtokenlist ( mem [ memtop - 2 ] .hhfield .v.RH , 0 , errorline - 10 , 0 
    ) ; 
  } 
} 
halfword getavail ( ) 
{register halfword Result; halfword p  ; 
  p = avail ; 
  if ( p != 0 ) 
  avail = mem [ avail ] .hhfield .v.RH ; 
  else if ( memend < memmax ) 
  {
    incr ( memend ) ; 
    p = memend ; 
  } 
  else {
      
    decr ( himemmin ) ; 
    p = himemmin ; 
    if ( himemmin <= lomemmax ) 
    {
      runaway () ; 
      overflow ( 314 , memmax + 1 ) ; 
    } 
  } 
  mem [ p ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
  incr ( dynused ) ; 
#endif /* STAT */
  Result = p ; 
  return(Result) ; 
} 
halfword zgetnode ( s ) 
integer s ; 
{/* 40 10 20 */ register halfword Result; halfword p  ; 
  halfword q  ; 
  integer r  ; 
  integer t, tt  ; 
  lab20: p = rover ; 
  do {
      q = p + mem [ p ] .hhfield .lhfield ; 
    while ( ( mem [ q ] .hhfield .v.RH == 262143L ) ) {
	
      t = mem [ q + 1 ] .hhfield .v.RH ; 
      tt = mem [ q + 1 ] .hhfield .lhfield ; 
      if ( q == rover ) 
      rover = t ; 
      mem [ t + 1 ] .hhfield .lhfield = tt ; 
      mem [ tt + 1 ] .hhfield .v.RH = t ; 
      q = q + mem [ q ] .hhfield .lhfield ; 
    } 
    r = q - s ; 
    if ( r > toint ( p + 1 ) ) 
    {
      mem [ p ] .hhfield .lhfield = r - p ; 
      rover = p ; 
      goto lab40 ; 
    } 
    if ( r == p ) 
    if ( mem [ p + 1 ] .hhfield .v.RH != p ) 
    {
      rover = mem [ p + 1 ] .hhfield .v.RH ; 
      t = mem [ p + 1 ] .hhfield .lhfield ; 
      mem [ rover + 1 ] .hhfield .lhfield = t ; 
      mem [ t + 1 ] .hhfield .v.RH = rover ; 
      goto lab40 ; 
    } 
    mem [ p ] .hhfield .lhfield = q - p ; 
    p = mem [ p + 1 ] .hhfield .v.RH ; 
  } while ( ! ( p == rover ) ) ; 
  if ( s == 1073741824L ) 
  {
    Result = 262143L ; 
    goto lab10 ; 
  } 
  if ( lomemmax + 2 < himemmin ) 
  if ( lomemmax + 2 <= 262143L ) 
  {
    if ( himemmin - lomemmax >= 1998 ) 
    t = lomemmax + 1000 ; 
    else t = lomemmax + 1 + ( himemmin - lomemmax ) / 2 ; 
    if ( t > 262143L ) 
    t = 262143L ; 
    p = mem [ rover + 1 ] .hhfield .lhfield ; 
    q = lomemmax ; 
    mem [ p + 1 ] .hhfield .v.RH = q ; 
    mem [ rover + 1 ] .hhfield .lhfield = q ; 
    mem [ q + 1 ] .hhfield .v.RH = rover ; 
    mem [ q + 1 ] .hhfield .lhfield = p ; 
    mem [ q ] .hhfield .v.RH = 262143L ; 
    mem [ q ] .hhfield .lhfield = t - lomemmax ; 
    lomemmax = t ; 
    mem [ lomemmax ] .hhfield .v.RH = 0 ; 
    mem [ lomemmax ] .hhfield .lhfield = 0 ; 
    rover = q ; 
    goto lab20 ; 
  } 
  overflow ( 314 , memmax + 1 ) ; 
  lab40: mem [ r ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
  varused = varused + s ; 
#endif /* STAT */
  Result = r ; 
  lab10: ; 
  return(Result) ; 
} 
void zfreenode ( p , s ) 
halfword p ; 
halfword s ; 
{halfword q  ; 
  mem [ p ] .hhfield .lhfield = s ; 
  mem [ p ] .hhfield .v.RH = 262143L ; 
  q = mem [ rover + 1 ] .hhfield .lhfield ; 
  mem [ p + 1 ] .hhfield .lhfield = q ; 
  mem [ p + 1 ] .hhfield .v.RH = rover ; 
  mem [ rover + 1 ] .hhfield .lhfield = p ; 
  mem [ q + 1 ] .hhfield .v.RH = p ; 
	;
#ifdef STAT
  varused = varused - s ; 
#endif /* STAT */
} 
void zflushlist ( p ) 
halfword p ; 
{/* 30 */ halfword q, r  ; 
  if ( p >= himemmin ) 
  if ( p != memtop ) 
  {
    r = p ; 
    do {
	q = r ; 
      r = mem [ r ] .hhfield .v.RH ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
      if ( r < himemmin ) 
      goto lab30 ; 
    } while ( ! ( r == memtop ) ) ; 
    lab30: mem [ q ] .hhfield .v.RH = avail ; 
    avail = p ; 
  } 
} 
void zflushnodelist ( p ) 
halfword p ; 
{halfword q  ; 
  while ( p != 0 ) {
      
    q = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
    if ( q < himemmin ) 
    freenode ( q , 2 ) ; 
    else {
	
      mem [ q ] .hhfield .v.RH = avail ; 
      avail = q ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
    } 
  } 
} 
#ifdef DEBUG
void zcheckmem ( printlocs ) 
boolean printlocs ; 
{/* 31 32 */ halfword p, q, r  ; 
  boolean clobbered  ; 
  {register integer for_end; p = 0 ; for_end = lomemmax ; if ( p <= for_end) 
  do 
    freearr [ p ] = false ; 
  while ( p++ < for_end ) ; } 
  {register integer for_end; p = himemmin ; for_end = memend ; if ( p <= 
  for_end) do 
    freearr [ p ] = false ; 
  while ( p++ < for_end ) ; } 
  p = avail ; 
  q = 0 ; 
  clobbered = false ; 
  while ( p != 0 ) {
      
    if ( ( p > memend ) || ( p < himemmin ) ) 
    clobbered = true ; 
    else if ( freearr [ p ] ) 
    clobbered = true ; 
    if ( clobbered ) 
    {
      printnl ( 315 ) ; 
      printint ( q ) ; 
      goto lab31 ; 
    } 
    freearr [ p ] = true ; 
    q = p ; 
    p = mem [ q ] .hhfield .v.RH ; 
  } 
  lab31: ; 
  p = rover ; 
  q = 0 ; 
  clobbered = false ; 
  do {
      if ( ( p >= lomemmax ) ) 
    clobbered = true ; 
    else if ( ( mem [ p + 1 ] .hhfield .v.RH >= lomemmax ) ) 
    clobbered = true ; 
    else if ( ! ( ( mem [ p ] .hhfield .v.RH == 262143L ) ) || ( mem [ p ] 
    .hhfield .lhfield < 2 ) || ( p + mem [ p ] .hhfield .lhfield > lomemmax ) 
    || ( mem [ mem [ p + 1 ] .hhfield .v.RH + 1 ] .hhfield .lhfield != p ) ) 
    clobbered = true ; 
    if ( clobbered ) 
    {
      printnl ( 316 ) ; 
      printint ( q ) ; 
      goto lab32 ; 
    } 
    {register integer for_end; q = p ; for_end = p + mem [ p ] .hhfield 
    .lhfield - 1 ; if ( q <= for_end) do 
      {
	if ( freearr [ q ] ) 
	{
	  printnl ( 317 ) ; 
	  printint ( q ) ; 
	  goto lab32 ; 
	} 
	freearr [ q ] = true ; 
      } 
    while ( q++ < for_end ) ; } 
    q = p ; 
    p = mem [ p + 1 ] .hhfield .v.RH ; 
  } while ( ! ( p == rover ) ) ; 
  lab32: ; 
  p = 0 ; 
  while ( p <= lomemmax ) {
      
    if ( ( mem [ p ] .hhfield .v.RH == 262143L ) ) 
    {
      printnl ( 318 ) ; 
      printint ( p ) ; 
    } 
    while ( ( p <= lomemmax ) && ! freearr [ p ] ) incr ( p ) ; 
    while ( ( p <= lomemmax ) && freearr [ p ] ) incr ( p ) ; 
  } 
  q = 13 ; 
  p = mem [ q ] .hhfield .v.RH ; 
  while ( p != 13 ) {
      
    if ( mem [ p + 1 ] .hhfield .lhfield != q ) 
    {
      printnl ( 595 ) ; 
      printint ( p ) ; 
    } 
    p = mem [ p + 1 ] .hhfield .v.RH ; 
    r = 19 ; 
    do {
	if ( mem [ mem [ p ] .hhfield .lhfield + 1 ] .cint >= mem [ r + 1 ] 
      .cint ) 
      {
	printnl ( 596 ) ; 
	printint ( p ) ; 
      } 
      r = mem [ p ] .hhfield .lhfield ; 
      q = p ; 
      p = mem [ q ] .hhfield .v.RH ; 
    } while ( ! ( r == 0 ) ) ; 
  } 
  if ( printlocs ) 
  {
    printnl ( 319 ) ; 
    {register integer for_end; p = 0 ; for_end = lomemmax ; if ( p <= 
    for_end) do 
      if ( ! freearr [ p ] && ( ( p > waslomax ) || wasfree [ p ] ) ) 
      {
	printchar ( 32 ) ; 
	printint ( p ) ; 
      } 
    while ( p++ < for_end ) ; } 
    {register integer for_end; p = himemmin ; for_end = memend ; if ( p <= 
    for_end) do 
      if ( ! freearr [ p ] && ( ( p < washimin ) || ( p > wasmemend ) || 
      wasfree [ p ] ) ) 
      {
	printchar ( 32 ) ; 
	printint ( p ) ; 
      } 
    while ( p++ < for_end ) ; } 
  } 
  {register integer for_end; p = 0 ; for_end = lomemmax ; if ( p <= for_end) 
  do 
    wasfree [ p ] = freearr [ p ] ; 
  while ( p++ < for_end ) ; } 
  {register integer for_end; p = himemmin ; for_end = memend ; if ( p <= 
  for_end) do 
    wasfree [ p ] = freearr [ p ] ; 
  while ( p++ < for_end ) ; } 
  wasmemend = memend ; 
  waslomax = lomemmax ; 
  washimin = himemmin ; 
} 
#endif /* DEBUG */
#ifdef DEBUG
void zsearchmem ( p ) 
halfword p ; 
{integer q  ; 
  {register integer for_end; q = 0 ; for_end = lomemmax ; if ( q <= for_end) 
  do 
    {
      if ( mem [ q ] .hhfield .v.RH == p ) 
      {
	printnl ( 320 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
      if ( mem [ q ] .hhfield .lhfield == p ) 
      {
	printnl ( 321 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
  {register integer for_end; q = himemmin ; for_end = memend ; if ( q <= 
  for_end) do 
    {
      if ( mem [ q ] .hhfield .v.RH == p ) 
      {
	printnl ( 320 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
      if ( mem [ q ] .hhfield .lhfield == p ) 
      {
	printnl ( 321 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
  {register integer for_end; q = 1 ; for_end = 9769 ; if ( q <= for_end) do 
    {
      if ( eqtb [ q ] .v.RH == p ) 
      {
	printnl ( 457 ) ; 
	printint ( q ) ; 
	printchar ( 41 ) ; 
      } 
    } 
  while ( q++ < for_end ) ; } 
} 
#endif /* DEBUG */
void zprintop ( c ) 
quarterword c ; 
{if ( c <= 15 ) 
  printtype ( c ) ; 
  else switch ( c ) 
  {case 30 : 
    print ( 346 ) ; 
    break ; 
  case 31 : 
    print ( 347 ) ; 
    break ; 
  case 32 : 
    print ( 348 ) ; 
    break ; 
  case 33 : 
    print ( 349 ) ; 
    break ; 
  case 34 : 
    print ( 350 ) ; 
    break ; 
  case 35 : 
    print ( 351 ) ; 
    break ; 
  case 36 : 
    print ( 352 ) ; 
    break ; 
  case 37 : 
    print ( 353 ) ; 
    break ; 
  case 38 : 
    print ( 354 ) ; 
    break ; 
  case 39 : 
    print ( 355 ) ; 
    break ; 
  case 40 : 
    print ( 356 ) ; 
    break ; 
  case 41 : 
    print ( 357 ) ; 
    break ; 
  case 42 : 
    print ( 358 ) ; 
    break ; 
  case 43 : 
    print ( 359 ) ; 
    break ; 
  case 44 : 
    print ( 360 ) ; 
    break ; 
  case 45 : 
    print ( 361 ) ; 
    break ; 
  case 46 : 
    print ( 362 ) ; 
    break ; 
  case 47 : 
    print ( 363 ) ; 
    break ; 
  case 48 : 
    print ( 364 ) ; 
    break ; 
  case 49 : 
    print ( 365 ) ; 
    break ; 
  case 50 : 
    print ( 366 ) ; 
    break ; 
  case 51 : 
    print ( 367 ) ; 
    break ; 
  case 52 : 
    print ( 368 ) ; 
    break ; 
  case 53 : 
    print ( 369 ) ; 
    break ; 
  case 54 : 
    print ( 370 ) ; 
    break ; 
  case 55 : 
    print ( 371 ) ; 
    break ; 
  case 56 : 
    print ( 372 ) ; 
    break ; 
  case 57 : 
    print ( 373 ) ; 
    break ; 
  case 58 : 
    print ( 374 ) ; 
    break ; 
  case 59 : 
    print ( 375 ) ; 
    break ; 
  case 60 : 
    print ( 376 ) ; 
    break ; 
  case 61 : 
    print ( 377 ) ; 
    break ; 
  case 62 : 
    print ( 378 ) ; 
    break ; 
  case 63 : 
    print ( 379 ) ; 
    break ; 
  case 64 : 
    print ( 380 ) ; 
    break ; 
  case 65 : 
    print ( 381 ) ; 
    break ; 
  case 66 : 
    print ( 382 ) ; 
    break ; 
  case 67 : 
    print ( 383 ) ; 
    break ; 
  case 68 : 
    print ( 384 ) ; 
    break ; 
  case 69 : 
    printchar ( 43 ) ; 
    break ; 
  case 70 : 
    printchar ( 45 ) ; 
    break ; 
  case 71 : 
    printchar ( 42 ) ; 
    break ; 
  case 72 : 
    printchar ( 47 ) ; 
    break ; 
  case 73 : 
    print ( 385 ) ; 
    break ; 
  case 74 : 
    print ( 309 ) ; 
    break ; 
  case 75 : 
    print ( 386 ) ; 
    break ; 
  case 76 : 
    print ( 387 ) ; 
    break ; 
  case 77 : 
    printchar ( 60 ) ; 
    break ; 
  case 78 : 
    print ( 388 ) ; 
    break ; 
  case 79 : 
    printchar ( 62 ) ; 
    break ; 
  case 80 : 
    print ( 389 ) ; 
    break ; 
  case 81 : 
    printchar ( 61 ) ; 
    break ; 
  case 82 : 
    print ( 390 ) ; 
    break ; 
  case 83 : 
    print ( 38 ) ; 
    break ; 
  case 84 : 
    print ( 391 ) ; 
    break ; 
  case 85 : 
    print ( 392 ) ; 
    break ; 
  case 86 : 
    print ( 393 ) ; 
    break ; 
  case 87 : 
    print ( 394 ) ; 
    break ; 
  case 88 : 
    print ( 395 ) ; 
    break ; 
  case 89 : 
    print ( 396 ) ; 
    break ; 
  case 90 : 
    print ( 397 ) ; 
    break ; 
  case 91 : 
    print ( 398 ) ; 
    break ; 
  case 92 : 
    print ( 399 ) ; 
    break ; 
  case 94 : 
    print ( 400 ) ; 
    break ; 
  case 95 : 
    print ( 401 ) ; 
    break ; 
  case 96 : 
    print ( 402 ) ; 
    break ; 
  case 97 : 
    print ( 403 ) ; 
    break ; 
  case 98 : 
    print ( 404 ) ; 
    break ; 
  case 99 : 
    print ( 405 ) ; 
    break ; 
  case 100 : 
    print ( 406 ) ; 
    break ; 
    default: 
    print ( 407 ) ; 
    break ; 
  } 
} 
void fixdateandtime ( ) 
{dateandtime ( internal [ 17 ] , internal [ 16 ] , internal [ 15 ] , internal 
  [ 14 ] ) ; 
  internal [ 17 ] = internal [ 17 ] * 65536L ; 
  internal [ 16 ] = internal [ 16 ] * 65536L ; 
  internal [ 15 ] = internal [ 15 ] * 65536L ; 
  internal [ 14 ] = internal [ 14 ] * 65536L ; 
} 
halfword zidlookup ( j , l ) 
integer j ; 
integer l ; 
{/* 40 */ register halfword Result; integer h  ; 
  halfword p  ; 
  halfword k  ; 
  if ( l == 1 ) 
  {
    p = buffer [ j ] + 1 ; 
    hash [ p ] .v.RH = p - 1 ; 
    goto lab40 ; 
  } 
  h = buffer [ j ] ; 
  {register integer for_end; k = j + 1 ; for_end = j + l - 1 ; if ( k <= 
  for_end) do 
    {
      h = h + h + buffer [ k ] ; 
      while ( h >= 7919 ) h = h - 7919 ; 
    } 
  while ( k++ < for_end ) ; } 
  p = h + 257 ; 
  while ( true ) {
      
    if ( hash [ p ] .v.RH > 0 ) 
    if ( ( strstart [ hash [ p ] .v.RH + 1 ] - strstart [ hash [ p ] .v.RH ] ) 
    == l ) 
    if ( streqbuf ( hash [ p ] .v.RH , j ) ) 
    goto lab40 ; 
    if ( hash [ p ] .lhfield == 0 ) 
    {
      if ( hash [ p ] .v.RH > 0 ) 
      {
	do {
	    if ( ( hashused == 257 ) ) 
	  overflow ( 456 , 9500 ) ; 
	  decr ( hashused ) ; 
	} while ( ! ( hash [ hashused ] .v.RH == 0 ) ) ; 
	hash [ p ] .lhfield = hashused ; 
	p = hashused ; 
      } 
      {
	if ( poolptr + l > maxpoolptr ) 
	{
	  if ( poolptr + l > poolsize ) 
	  overflow ( 257 , poolsize - initpoolptr ) ; 
	  maxpoolptr = poolptr + l ; 
	} 
      } 
      {register integer for_end; k = j ; for_end = j + l - 1 ; if ( k <= 
      for_end) do 
	{
	  strpool [ poolptr ] = buffer [ k ] ; 
	  incr ( poolptr ) ; 
	} 
      while ( k++ < for_end ) ; } 
      hash [ p ] .v.RH = makestring () ; 
      strref [ hash [ p ] .v.RH ] = 127 ; 
	;
#ifdef STAT
      incr ( stcount ) ; 
#endif /* STAT */
      goto lab40 ; 
    } 
    p = hash [ p ] .lhfield ; 
  } 
  lab40: Result = p ; 
  return(Result) ; 
} 
halfword znewnumtok ( v ) 
scaled v ; 
{register halfword Result; halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p + 1 ] .cint = v ; 
  mem [ p ] .hhfield .b0 = 16 ; 
  mem [ p ] .hhfield .b1 = 12 ; 
  Result = p ; 
  return(Result) ; 
} 
void zflushtokenlist ( p ) 
halfword p ; 
{halfword q  ; 
  while ( p != 0 ) {
      
    q = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
    if ( q >= himemmin ) 
    {
      mem [ q ] .hhfield .v.RH = avail ; 
      avail = q ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
    } 
    else {
	
      switch ( mem [ q ] .hhfield .b0 ) 
      {case 1 : 
      case 2 : 
      case 16 : 
	; 
	break ; 
      case 4 : 
	{
	  if ( strref [ mem [ q + 1 ] .cint ] < 127 ) 
	  if ( strref [ mem [ q + 1 ] .cint ] > 1 ) 
	  decr ( strref [ mem [ q + 1 ] .cint ] ) ; 
	  else flushstring ( mem [ q + 1 ] .cint ) ; 
	} 
	break ; 
      case 3 : 
      case 5 : 
      case 7 : 
      case 12 : 
      case 10 : 
      case 6 : 
      case 9 : 
      case 8 : 
      case 11 : 
      case 14 : 
      case 13 : 
      case 17 : 
      case 18 : 
      case 19 : 
	{
	  gpointer = q ; 
	  tokenrecycle () ; 
	} 
	break ; 
	default: 
	confusion ( 490 ) ; 
	break ; 
      } 
      freenode ( q , 2 ) ; 
    } 
  } 
} 
void zdeletemacref ( p ) 
halfword p ; 
{if ( mem [ p ] .hhfield .lhfield == 0 ) 
  flushtokenlist ( p ) ; 
  else decr ( mem [ p ] .hhfield .lhfield ) ; 
} 
void zprintcmdmod ( c , m ) 
integer c ; 
integer m ; 
{switch ( c ) 
  {case 18 : 
    print ( 461 ) ; 
    break ; 
  case 77 : 
    print ( 460 ) ; 
    break ; 
  case 59 : 
    print ( 463 ) ; 
    break ; 
  case 72 : 
    print ( 462 ) ; 
    break ; 
  case 79 : 
    print ( 459 ) ; 
    break ; 
  case 32 : 
    print ( 464 ) ; 
    break ; 
  case 81 : 
    print ( 58 ) ; 
    break ; 
  case 82 : 
    print ( 44 ) ; 
    break ; 
  case 57 : 
    print ( 465 ) ; 
    break ; 
  case 19 : 
    print ( 466 ) ; 
    break ; 
  case 60 : 
    print ( 467 ) ; 
    break ; 
  case 27 : 
    print ( 468 ) ; 
    break ; 
  case 11 : 
    print ( 469 ) ; 
    break ; 
  case 80 : 
    print ( 458 ) ; 
    break ; 
  case 84 : 
    print ( 452 ) ; 
    break ; 
  case 26 : 
    print ( 470 ) ; 
    break ; 
  case 6 : 
    print ( 471 ) ; 
    break ; 
  case 9 : 
    print ( 472 ) ; 
    break ; 
  case 70 : 
    print ( 473 ) ; 
    break ; 
  case 73 : 
    print ( 474 ) ; 
    break ; 
  case 13 : 
    print ( 475 ) ; 
    break ; 
  case 46 : 
    print ( 123 ) ; 
    break ; 
  case 63 : 
    print ( 91 ) ; 
    break ; 
  case 14 : 
    print ( 476 ) ; 
    break ; 
  case 15 : 
    print ( 477 ) ; 
    break ; 
  case 69 : 
    print ( 478 ) ; 
    break ; 
  case 28 : 
    print ( 479 ) ; 
    break ; 
  case 47 : 
    print ( 407 ) ; 
    break ; 
  case 24 : 
    print ( 480 ) ; 
    break ; 
  case 7 : 
    printchar ( 92 ) ; 
    break ; 
  case 65 : 
    print ( 125 ) ; 
    break ; 
  case 64 : 
    print ( 93 ) ; 
    break ; 
  case 12 : 
    print ( 481 ) ; 
    break ; 
  case 8 : 
    print ( 482 ) ; 
    break ; 
  case 83 : 
    print ( 59 ) ; 
    break ; 
  case 17 : 
    print ( 483 ) ; 
    break ; 
  case 78 : 
    print ( 484 ) ; 
    break ; 
  case 74 : 
    print ( 485 ) ; 
    break ; 
  case 35 : 
    print ( 486 ) ; 
    break ; 
  case 58 : 
    print ( 487 ) ; 
    break ; 
  case 71 : 
    print ( 488 ) ; 
    break ; 
  case 75 : 
    print ( 489 ) ; 
    break ; 
  case 16 : 
    if ( m <= 2 ) 
    if ( m == 1 ) 
    print ( 652 ) ; 
    else if ( m < 1 ) 
    print ( 453 ) ; 
    else print ( 653 ) ; 
    else if ( m == 53 ) 
    print ( 654 ) ; 
    else if ( m == 44 ) 
    print ( 655 ) ; 
    else print ( 656 ) ; 
    break ; 
  case 4 : 
    if ( m <= 1 ) 
    if ( m == 1 ) 
    print ( 659 ) ; 
    else print ( 454 ) ; 
    else if ( m == 9770 ) 
    print ( 657 ) ; 
    else print ( 658 ) ; 
    break ; 
  case 61 : 
    switch ( m ) 
    {case 1 : 
      print ( 661 ) ; 
      break ; 
    case 2 : 
      printchar ( 64 ) ; 
      break ; 
    case 3 : 
      print ( 662 ) ; 
      break ; 
      default: 
      print ( 660 ) ; 
      break ; 
    } 
    break ; 
  case 56 : 
    if ( m >= 9770 ) 
    if ( m == 9770 ) 
    print ( 673 ) ; 
    else if ( m == 9920 ) 
    print ( 674 ) ; 
    else print ( 675 ) ; 
    else if ( m < 2 ) 
    print ( 676 ) ; 
    else if ( m == 2 ) 
    print ( 677 ) ; 
    else print ( 678 ) ; 
    break ; 
  case 3 : 
    if ( m == 0 ) 
    print ( 688 ) ; 
    else print ( 614 ) ; 
    break ; 
  case 1 : 
  case 2 : 
    switch ( m ) 
    {case 1 : 
      print ( 715 ) ; 
      break ; 
    case 2 : 
      print ( 451 ) ; 
      break ; 
    case 3 : 
      print ( 716 ) ; 
      break ; 
      default: 
      print ( 717 ) ; 
      break ; 
    } 
    break ; 
  case 33 : 
  case 34 : 
  case 37 : 
  case 55 : 
  case 45 : 
  case 50 : 
  case 36 : 
  case 43 : 
  case 54 : 
  case 48 : 
  case 51 : 
  case 52 : 
    printop ( m ) ; 
    break ; 
  case 30 : 
    printtype ( m ) ; 
    break ; 
  case 85 : 
    if ( m == 0 ) 
    print ( 909 ) ; 
    else print ( 910 ) ; 
    break ; 
  case 23 : 
    switch ( m ) 
    {case 0 : 
      print ( 271 ) ; 
      break ; 
    case 1 : 
      print ( 272 ) ; 
      break ; 
    case 2 : 
      print ( 273 ) ; 
      break ; 
      default: 
      print ( 916 ) ; 
      break ; 
    } 
    break ; 
  case 21 : 
    if ( m == 0 ) 
    print ( 917 ) ; 
    else print ( 918 ) ; 
    break ; 
  case 22 : 
    switch ( m ) 
    {case 0 : 
      print ( 932 ) ; 
      break ; 
    case 1 : 
      print ( 933 ) ; 
      break ; 
    case 2 : 
      print ( 934 ) ; 
      break ; 
    case 3 : 
      print ( 935 ) ; 
      break ; 
      default: 
      print ( 936 ) ; 
      break ; 
    } 
    break ; 
  case 31 : 
  case 62 : 
    {
      if ( c == 31 ) 
      print ( 939 ) ; 
      else print ( 940 ) ; 
      print ( 941 ) ; 
      slowprint ( hash [ m ] .v.RH ) ; 
    } 
    break ; 
  case 41 : 
    if ( m == 0 ) 
    print ( 942 ) ; 
    else print ( 943 ) ; 
    break ; 
  case 10 : 
    print ( 944 ) ; 
    break ; 
  case 53 : 
  case 44 : 
  case 49 : 
    {
      printcmdmod ( 16 , c ) ; 
      print ( 945 ) ; 
      println () ; 
      showtokenlist ( mem [ mem [ m ] .hhfield .v.RH ] .hhfield .v.RH , 0 , 
      1000 , 0 ) ; 
    } 
    break ; 
  case 5 : 
    print ( 946 ) ; 
    break ; 
  case 40 : 
    slowprint ( intname [ m ] ) ; 
    break ; 
  case 68 : 
    if ( m == 1 ) 
    print ( 953 ) ; 
    else if ( m == 0 ) 
    print ( 954 ) ; 
    else print ( 955 ) ; 
    break ; 
  case 66 : 
    if ( m == 6 ) 
    print ( 956 ) ; 
    else print ( 957 ) ; 
    break ; 
  case 67 : 
    if ( m == 0 ) 
    print ( 958 ) ; 
    else print ( 959 ) ; 
    break ; 
  case 25 : 
    if ( m < 1 ) 
    print ( 989 ) ; 
    else if ( m == 1 ) 
    print ( 990 ) ; 
    else print ( 991 ) ; 
    break ; 
  case 20 : 
    switch ( m ) 
    {case 0 : 
      print ( 1001 ) ; 
      break ; 
    case 1 : 
      print ( 1002 ) ; 
      break ; 
    case 2 : 
      print ( 1003 ) ; 
      break ; 
    case 3 : 
      print ( 1004 ) ; 
      break ; 
      default: 
      print ( 1005 ) ; 
      break ; 
    } 
    break ; 
  case 76 : 
    switch ( m ) 
    {case 0 : 
      print ( 1023 ) ; 
      break ; 
    case 1 : 
      print ( 1024 ) ; 
      break ; 
    case 2 : 
      print ( 1026 ) ; 
      break ; 
    case 3 : 
      print ( 1028 ) ; 
      break ; 
    case 5 : 
      print ( 1025 ) ; 
      break ; 
    case 6 : 
      print ( 1027 ) ; 
      break ; 
    case 7 : 
      print ( 1029 ) ; 
      break ; 
    case 11 : 
      print ( 1030 ) ; 
      break ; 
      default: 
      print ( 1031 ) ; 
      break ; 
    } 
    break ; 
  case 29 : 
    if ( m == 16 ) 
    print ( 1056 ) ; 
    else print ( 1055 ) ; 
    break ; 
    default: 
    print ( 600 ) ; 
    break ; 
  } 
} 
void zshowmacro ( p , q , l ) 
halfword p ; 
integer q ; 
integer l ; 
{/* 10 */ halfword r  ; 
  p = mem [ p ] .hhfield .v.RH ; 
  while ( mem [ p ] .hhfield .lhfield > 7 ) {
      
    r = mem [ p ] .hhfield .v.RH ; 
    mem [ p ] .hhfield .v.RH = 0 ; 
    showtokenlist ( p , 0 , l , 0 ) ; 
    mem [ p ] .hhfield .v.RH = r ; 
    p = r ; 
    if ( l > 0 ) 
    l = l - tally ; 
    else goto lab10 ; 
  } 
  tally = 0 ; 
  switch ( mem [ p ] .hhfield .lhfield ) 
  {case 0 : 
    print ( 500 ) ; 
    break ; 
  case 1 : 
  case 2 : 
  case 3 : 
    {
      printchar ( 60 ) ; 
      printcmdmod ( 56 , mem [ p ] .hhfield .lhfield ) ; 
      print ( 501 ) ; 
    } 
    break ; 
  case 4 : 
    print ( 502 ) ; 
    break ; 
  case 5 : 
    print ( 503 ) ; 
    break ; 
  case 6 : 
    print ( 504 ) ; 
    break ; 
  case 7 : 
    print ( 505 ) ; 
    break ; 
  } 
  showtokenlist ( mem [ p ] .hhfield .v.RH , q , l - tally , 0 ) ; 
  lab10: ; 
} 
void zinitbignode ( p ) 
halfword p ; 
{halfword q  ; 
  smallnumber s  ; 
  s = bignodesize [ mem [ p ] .hhfield .b0 ] ; 
  q = getnode ( s ) ; 
  do {
      s = s - 2 ; 
    {
      mem [ q + s ] .hhfield .b0 = 19 ; 
      serialno = serialno + 64 ; 
      mem [ q + s + 1 ] .cint = serialno ; 
    } 
    mem [ q + s ] .hhfield .b1 = ( s ) / 2 + 5 ; 
    mem [ q + s ] .hhfield .v.RH = 0 ; 
  } while ( ! ( s == 0 ) ) ; 
  mem [ q ] .hhfield .v.RH = p ; 
  mem [ p + 1 ] .cint = q ; 
} 
halfword idtransform ( ) 
{register halfword Result; halfword p, q, r  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hhfield .b0 = 13 ; 
  mem [ p ] .hhfield .b1 = 11 ; 
  mem [ p + 1 ] .cint = 0 ; 
  initbignode ( p ) ; 
  q = mem [ p + 1 ] .cint ; 
  r = q + 12 ; 
  do {
      r = r - 2 ; 
    mem [ r ] .hhfield .b0 = 16 ; 
    mem [ r + 1 ] .cint = 0 ; 
  } while ( ! ( r == q ) ) ; 
  mem [ q + 5 ] .cint = 65536L ; 
  mem [ q + 11 ] .cint = 65536L ; 
  Result = p ; 
  return(Result) ; 
} 
void znewroot ( x ) 
halfword x ; 
{halfword p  ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hhfield .b0 = 0 ; 
  mem [ p ] .hhfield .b1 = 0 ; 
  mem [ p ] .hhfield .v.RH = x ; 
  eqtb [ x ] .v.RH = p ; 
} 
void zprintvariablename ( p ) 
halfword p ; 
{/* 40 10 */ halfword q  ; 
  halfword r  ; 
  while ( mem [ p ] .hhfield .b1 >= 5 ) {
      
    switch ( mem [ p ] .hhfield .b1 ) 
    {case 5 : 
      printchar ( 120 ) ; 
      break ; 
    case 6 : 
      printchar ( 121 ) ; 
      break ; 
    case 7 : 
      print ( 508 ) ; 
      break ; 
    case 8 : 
      print ( 509 ) ; 
      break ; 
    case 9 : 
      print ( 510 ) ; 
      break ; 
    case 10 : 
      print ( 511 ) ; 
      break ; 
    case 11 : 
      {
	print ( 512 ) ; 
	printint ( p - 0 ) ; 
	goto lab10 ; 
      } 
      break ; 
    } 
    print ( 513 ) ; 
    p = mem [ p - 2 * ( mem [ p ] .hhfield .b1 - 5 ) ] .hhfield .v.RH ; 
  } 
  q = 0 ; 
  while ( mem [ p ] .hhfield .b1 > 1 ) {
      
    if ( mem [ p ] .hhfield .b1 == 3 ) 
    {
      r = newnumtok ( mem [ p + 2 ] .cint ) ; 
      do {
	  p = mem [ p ] .hhfield .v.RH ; 
      } while ( ! ( mem [ p ] .hhfield .b1 == 4 ) ) ; 
    } 
    else if ( mem [ p ] .hhfield .b1 == 2 ) 
    {
      p = mem [ p ] .hhfield .v.RH ; 
      goto lab40 ; 
    } 
    else {
	
      if ( mem [ p ] .hhfield .b1 != 4 ) 
      confusion ( 507 ) ; 
      r = getavail () ; 
      mem [ r ] .hhfield .lhfield = mem [ p + 2 ] .hhfield .lhfield ; 
    } 
    mem [ r ] .hhfield .v.RH = q ; 
    q = r ; 
    lab40: p = mem [ p + 2 ] .hhfield .v.RH ; 
  } 
  r = getavail () ; 
  mem [ r ] .hhfield .lhfield = mem [ p ] .hhfield .v.RH ; 
  mem [ r ] .hhfield .v.RH = q ; 
  if ( mem [ p ] .hhfield .b1 == 1 ) 
  print ( 506 ) ; 
  showtokenlist ( r , 0 , 2147483647L , tally ) ; 
  flushtokenlist ( r ) ; 
  lab10: ; 
} 
boolean zinteresting ( p ) 
halfword p ; 
{register boolean Result; smallnumber t  ; 
  if ( internal [ 3 ] > 0 ) 
  Result = true ; 
  else {
      
    t = mem [ p ] .hhfield .b1 ; 
    if ( t >= 5 ) 
    if ( t != 11 ) 
    t = mem [ mem [ p - 2 * ( t - 5 ) ] .hhfield .v.RH ] .hhfield .b1 ; 
    Result = ( t != 11 ) ; 
  } 
  return(Result) ; 
} 
halfword znewstructure ( p ) 
halfword p ; 
{register halfword Result; halfword q, r  ; 
  switch ( mem [ p ] .hhfield .b1 ) 
  {case 0 : 
    {
      q = mem [ p ] .hhfield .v.RH ; 
      r = getnode ( 2 ) ; 
      eqtb [ q ] .v.RH = r ; 
    } 
    break ; 
  case 3 : 
    {
      q = p ; 
      do {
	  q = mem [ q ] .hhfield .v.RH ; 
      } while ( ! ( mem [ q ] .hhfield .b1 == 4 ) ) ; 
      q = mem [ q + 2 ] .hhfield .v.RH ; 
      r = q + 1 ; 
      do {
	  q = r ; 
	r = mem [ r ] .hhfield .v.RH ; 
      } while ( ! ( r == p ) ) ; 
      r = getnode ( 3 ) ; 
      mem [ q ] .hhfield .v.RH = r ; 
      mem [ r + 2 ] .cint = mem [ p + 2 ] .cint ; 
    } 
    break ; 
  case 4 : 
    {
      q = mem [ p + 2 ] .hhfield .v.RH ; 
      r = mem [ q + 1 ] .hhfield .lhfield ; 
      do {
	  q = r ; 
	r = mem [ r ] .hhfield .v.RH ; 
      } while ( ! ( r == p ) ) ; 
      r = getnode ( 3 ) ; 
      mem [ q ] .hhfield .v.RH = r ; 
      mem [ r + 2 ] = mem [ p + 2 ] ; 
      if ( mem [ p + 2 ] .hhfield .lhfield == 0 ) 
      {
	q = mem [ p + 2 ] .hhfield .v.RH + 1 ; 
	while ( mem [ q ] .hhfield .v.RH != p ) q = mem [ q ] .hhfield .v.RH ; 
	mem [ q ] .hhfield .v.RH = r ; 
      } 
    } 
    break ; 
    default: 
    confusion ( 514 ) ; 
    break ; 
  } 
  mem [ r ] .hhfield .v.RH = mem [ p ] .hhfield .v.RH ; 
  mem [ r ] .hhfield .b0 = 21 ; 
  mem [ r ] .hhfield .b1 = mem [ p ] .hhfield .b1 ; 
  mem [ r + 1 ] .hhfield .lhfield = p ; 
  mem [ p ] .hhfield .b1 = 2 ; 
  q = getnode ( 3 ) ; 
  mem [ p ] .hhfield .v.RH = q ; 
  mem [ r + 1 ] .hhfield .v.RH = q ; 
  mem [ q + 2 ] .hhfield .v.RH = r ; 
  mem [ q ] .hhfield .b0 = 0 ; 
  mem [ q ] .hhfield .b1 = 4 ; 
  mem [ q ] .hhfield .v.RH = 17 ; 
  mem [ q + 2 ] .hhfield .lhfield = 0 ; 
  Result = r ; 
  return(Result) ; 
} 
halfword zfindvariable ( t ) 
halfword t ; 
{/* 10 */ register halfword Result; halfword p, q, r, s  ; 
  halfword pp, qq, rr, ss  ; 
  integer n  ; 
  memoryword saveword  ; 
  p = mem [ t ] .hhfield .lhfield ; 
  t = mem [ t ] .hhfield .v.RH ; 
  if ( eqtb [ p ] .lhfield % 86 != 41 ) 
  {
    Result = 0 ; 
    goto lab10 ; 
  } 
  if ( eqtb [ p ] .v.RH == 0 ) 
  newroot ( p ) ; 
  p = eqtb [ p ] .v.RH ; 
  pp = p ; 
  while ( t != 0 ) {
      
    if ( mem [ pp ] .hhfield .b0 != 21 ) 
    {
      if ( mem [ pp ] .hhfield .b0 > 21 ) 
      {
	Result = 0 ; 
	goto lab10 ; 
      } 
      ss = newstructure ( pp ) ; 
      if ( p == pp ) 
      p = ss ; 
      pp = ss ; 
    } 
    if ( mem [ p ] .hhfield .b0 != 21 ) 
    p = newstructure ( p ) ; 
    if ( t < himemmin ) 
    {
      n = mem [ t + 1 ] .cint ; 
      pp = mem [ mem [ pp + 1 ] .hhfield .lhfield ] .hhfield .v.RH ; 
      q = mem [ mem [ p + 1 ] .hhfield .lhfield ] .hhfield .v.RH ; 
      saveword = mem [ q + 2 ] ; 
      mem [ q + 2 ] .cint = 2147483647L ; 
      s = p + 1 ; 
      do {
	  r = s ; 
	s = mem [ s ] .hhfield .v.RH ; 
      } while ( ! ( n <= mem [ s + 2 ] .cint ) ) ; 
      if ( n == mem [ s + 2 ] .cint ) 
      p = s ; 
      else {
	  
	p = getnode ( 3 ) ; 
	mem [ r ] .hhfield .v.RH = p ; 
	mem [ p ] .hhfield .v.RH = s ; 
	mem [ p + 2 ] .cint = n ; 
	mem [ p ] .hhfield .b1 = 3 ; 
	mem [ p ] .hhfield .b0 = 0 ; 
      } 
      mem [ q + 2 ] = saveword ; 
    } 
    else {
	
      n = mem [ t ] .hhfield .lhfield ; 
      ss = mem [ pp + 1 ] .hhfield .lhfield ; 
      do {
	  rr = ss ; 
	ss = mem [ ss ] .hhfield .v.RH ; 
      } while ( ! ( n <= mem [ ss + 2 ] .hhfield .lhfield ) ) ; 
      if ( n < mem [ ss + 2 ] .hhfield .lhfield ) 
      {
	qq = getnode ( 3 ) ; 
	mem [ rr ] .hhfield .v.RH = qq ; 
	mem [ qq ] .hhfield .v.RH = ss ; 
	mem [ qq + 2 ] .hhfield .lhfield = n ; 
	mem [ qq ] .hhfield .b1 = 4 ; 
	mem [ qq ] .hhfield .b0 = 0 ; 
	mem [ qq + 2 ] .hhfield .v.RH = pp ; 
	ss = qq ; 
      } 
      if ( p == pp ) 
      {
	p = ss ; 
	pp = ss ; 
      } 
      else {
	  
	pp = ss ; 
	s = mem [ p + 1 ] .hhfield .lhfield ; 
	do {
	    r = s ; 
	  s = mem [ s ] .hhfield .v.RH ; 
	} while ( ! ( n <= mem [ s + 2 ] .hhfield .lhfield ) ) ; 
	if ( n == mem [ s + 2 ] .hhfield .lhfield ) 
	p = s ; 
	else {
	    
	  q = getnode ( 3 ) ; 
	  mem [ r ] .hhfield .v.RH = q ; 
	  mem [ q ] .hhfield .v.RH = s ; 
	  mem [ q + 2 ] .hhfield .lhfield = n ; 
	  mem [ q ] .hhfield .b1 = 4 ; 
	  mem [ q ] .hhfield .b0 = 0 ; 
	  mem [ q + 2 ] .hhfield .v.RH = p ; 
	  p = q ; 
	} 
      } 
    } 
    t = mem [ t ] .hhfield .v.RH ; 
  } 
  if ( mem [ pp ] .hhfield .b0 >= 21 ) 
  if ( mem [ pp ] .hhfield .b0 == 21 ) 
  pp = mem [ pp + 1 ] .hhfield .lhfield ; 
  else {
      
    Result = 0 ; 
    goto lab10 ; 
  } 
  if ( mem [ p ] .hhfield .b0 == 21 ) 
  p = mem [ p + 1 ] .hhfield .lhfield ; 
  if ( mem [ p ] .hhfield .b0 == 0 ) 
  {
    if ( mem [ pp ] .hhfield .b0 == 0 ) 
    {
      mem [ pp ] .hhfield .b0 = 15 ; 
      mem [ pp + 1 ] .cint = 0 ; 
    } 
    mem [ p ] .hhfield .b0 = mem [ pp ] .hhfield .b0 ; 
    mem [ p + 1 ] .cint = 0 ; 
  } 
  Result = p ; 
  lab10: ; 
  return(Result) ; 
} 
void zprintpath ( h , s , nuline ) 
halfword h ; 
strnumber s ; 
boolean nuline ; 
{/* 30 31 */ halfword p, q  ; 
  printdiagnostic ( 516 , s , nuline ) ; 
  println () ; 
  p = h ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    if ( ( p == 0 ) || ( q == 0 ) ) 
    {
      printnl ( 259 ) ; 
      goto lab30 ; 
    } 
    printtwo ( mem [ p + 1 ] .cint , mem [ p + 2 ] .cint ) ; 
    switch ( mem [ p ] .hhfield .b1 ) 
    {case 0 : 
      {
	if ( mem [ p ] .hhfield .b0 == 4 ) 
	print ( 517 ) ; 
	if ( ( mem [ q ] .hhfield .b0 != 0 ) || ( q != h ) ) 
	q = 0 ; 
	goto lab31 ; 
      } 
      break ; 
    case 1 : 
      {
	print ( 523 ) ; 
	printtwo ( mem [ p + 5 ] .cint , mem [ p + 6 ] .cint ) ; 
	print ( 522 ) ; 
	if ( mem [ q ] .hhfield .b0 != 1 ) 
	print ( 524 ) ; 
	else printtwo ( mem [ q + 3 ] .cint , mem [ q + 4 ] .cint ) ; 
	goto lab31 ; 
      } 
      break ; 
    case 4 : 
      if ( ( mem [ p ] .hhfield .b0 != 1 ) && ( mem [ p ] .hhfield .b0 != 4 ) 
      ) 
      print ( 517 ) ; 
      break ; 
    case 3 : 
    case 2 : 
      {
	if ( mem [ p ] .hhfield .b0 == 4 ) 
	print ( 524 ) ; 
	if ( mem [ p ] .hhfield .b1 == 3 ) 
	{
	  print ( 520 ) ; 
	  printscaled ( mem [ p + 5 ] .cint ) ; 
	} 
	else {
	    
	  nsincos ( mem [ p + 5 ] .cint ) ; 
	  printchar ( 123 ) ; 
	  printscaled ( ncos ) ; 
	  printchar ( 44 ) ; 
	  printscaled ( nsin ) ; 
	} 
	printchar ( 125 ) ; 
      } 
      break ; 
      default: 
      print ( 259 ) ; 
      break ; 
    } 
    if ( mem [ q ] .hhfield .b0 <= 1 ) 
    print ( 518 ) ; 
    else if ( ( mem [ p + 6 ] .cint != 65536L ) || ( mem [ q + 4 ] .cint != 
    65536L ) ) 
    {
      print ( 521 ) ; 
      if ( mem [ p + 6 ] .cint < 0 ) 
      print ( 463 ) ; 
      printscaled ( abs ( mem [ p + 6 ] .cint ) ) ; 
      if ( mem [ p + 6 ] .cint != mem [ q + 4 ] .cint ) 
      {
	print ( 522 ) ; 
	if ( mem [ q + 4 ] .cint < 0 ) 
	print ( 463 ) ; 
	printscaled ( abs ( mem [ q + 4 ] .cint ) ) ; 
      } 
    } 
    lab31: ; 
    p = q ; 
    if ( ( p != h ) || ( mem [ h ] .hhfield .b0 != 0 ) ) 
    {
      printnl ( 519 ) ; 
      if ( mem [ p ] .hhfield .b0 == 2 ) 
      {
	nsincos ( mem [ p + 3 ] .cint ) ; 
	printchar ( 123 ) ; 
	printscaled ( ncos ) ; 
	printchar ( 44 ) ; 
	printscaled ( nsin ) ; 
	printchar ( 125 ) ; 
      } 
      else if ( mem [ p ] .hhfield .b0 == 3 ) 
      {
	print ( 520 ) ; 
	printscaled ( mem [ p + 3 ] .cint ) ; 
	printchar ( 125 ) ; 
      } 
    } 
  } while ( ! ( p == h ) ) ; 
  if ( mem [ h ] .hhfield .b0 != 0 ) 
  print ( 384 ) ; 
  lab30: enddiagnostic ( true ) ; 
} 
void zprintweight ( q , xoff ) 
halfword q ; 
integer xoff ; 
{integer w, m  ; 
  integer d  ; 
  d = mem [ q ] .hhfield .lhfield ; 
  w = d % 8 ; 
  m = ( d / 8 ) - mem [ curedges + 3 ] .hhfield .lhfield ; 
  if ( fileoffset > maxprintline - 9 ) 
  printnl ( 32 ) ; 
  else printchar ( 32 ) ; 
  printint ( m + xoff ) ; 
  while ( w > 4 ) {
      
    printchar ( 43 ) ; 
    decr ( w ) ; 
  } 
  while ( w < 4 ) {
      
    printchar ( 45 ) ; 
    incr ( w ) ; 
  } 
} 
void zprintedges ( s , nuline , xoff , yoff ) 
strnumber s ; 
boolean nuline ; 
integer xoff ; 
integer yoff ; 
{halfword p, q, r  ; 
  integer n  ; 
  printdiagnostic ( 531 , s , nuline ) ; 
  p = mem [ curedges ] .hhfield .lhfield ; 
  n = mem [ curedges + 1 ] .hhfield .v.RH - 4096 ; 
  while ( p != curedges ) {
      
    q = mem [ p + 1 ] .hhfield .lhfield ; 
    r = mem [ p + 1 ] .hhfield .v.RH ; 
    if ( ( q > 1 ) || ( r != memtop ) ) 
    {
      printnl ( 532 ) ; 
      printint ( n + yoff ) ; 
      printchar ( 58 ) ; 
      while ( q > 1 ) {
	  
	printweight ( q , xoff ) ; 
	q = mem [ q ] .hhfield .v.RH ; 
      } 
      print ( 533 ) ; 
      while ( r != memtop ) {
	  
	printweight ( r , xoff ) ; 
	r = mem [ r ] .hhfield .v.RH ; 
      } 
    } 
    p = mem [ p ] .hhfield .lhfield ; 
    decr ( n ) ; 
  } 
  enddiagnostic ( true ) ; 
} 
void zunskew ( x , y , octant ) 
scaled x ; 
scaled y ; 
smallnumber octant ; 
{switch ( octant ) 
  {case 1 : 
    {
      curx = x + y ; 
      cury = y ; 
    } 
    break ; 
  case 5 : 
    {
      curx = y ; 
      cury = x + y ; 
    } 
    break ; 
  case 6 : 
    {
      curx = - (integer) y ; 
      cury = x + y ; 
    } 
    break ; 
  case 2 : 
    {
      curx = - (integer) x - y ; 
      cury = y ; 
    } 
    break ; 
  case 4 : 
    {
      curx = - (integer) x - y ; 
      cury = - (integer) y ; 
    } 
    break ; 
  case 8 : 
    {
      curx = - (integer) y ; 
      cury = - (integer) x - y ; 
    } 
    break ; 
  case 7 : 
    {
      curx = y ; 
      cury = - (integer) x - y ; 
    } 
    break ; 
  case 3 : 
    {
      curx = x + y ; 
      cury = - (integer) y ; 
    } 
    break ; 
  } 
} 
