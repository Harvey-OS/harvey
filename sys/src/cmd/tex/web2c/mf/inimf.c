#define EXTERN extern
#include "mfd.h"

#ifdef INIMF
boolean getstringsstarted ( ) 
{/* 30 10 */ register boolean Result; unsigned char k, l  ; 
  ASCIIcode m, n  ; 
  strnumber g  ; 
  integer a  ; 
  boolean c  ; 
  poolptr = 0 ; 
  strptr = 0 ; 
  maxpoolptr = 0 ; 
  maxstrptr = 0 ; 
  strstart [ 0 ] = 0 ; 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    {
      if ( ( ( k < 32 ) || ( k > 126 ) ) ) 
      {
	{
	  strpool [ poolptr ] = 94 ; 
	  incr ( poolptr ) ; 
	} 
	{
	  strpool [ poolptr ] = 94 ; 
	  incr ( poolptr ) ; 
	} 
	if ( k < 64 ) 
	{
	  strpool [ poolptr ] = k + 64 ; 
	  incr ( poolptr ) ; 
	} 
	else if ( k < 128 ) 
	{
	  strpool [ poolptr ] = k - 64 ; 
	  incr ( poolptr ) ; 
	} 
	else {
	    
	  l = k / 16 ; 
	  if ( l < 10 ) 
	  {
	    strpool [ poolptr ] = l + 48 ; 
	    incr ( poolptr ) ; 
	  } 
	  else {
	      
	    strpool [ poolptr ] = l + 87 ; 
	    incr ( poolptr ) ; 
	  } 
	  l = k % 16 ; 
	  if ( l < 10 ) 
	  {
	    strpool [ poolptr ] = l + 48 ; 
	    incr ( poolptr ) ; 
	  } 
	  else {
	      
	    strpool [ poolptr ] = l + 87 ; 
	    incr ( poolptr ) ; 
	  } 
	} 
      } 
      else {
	  
	strpool [ poolptr ] = k ; 
	incr ( poolptr ) ; 
      } 
      g = makestring () ; 
      strref [ g ] = 127 ; 
    } 
  while ( k++ < for_end ) ; } 
  vstrcpy ( nameoffile + 1 , poolname ) ; 
  nameoffile [ 0 ] = ' ' ; 
  nameoffile [ strlen ( poolname ) + 1 ] = ' ' ; 
  namelength = strlen ( poolname ) ; 
  if ( aopenin ( poolfile , MFPOOLPATH ) ) 
  {
    c = false ; 
    do {
	{ 
	if ( eof ( poolfile ) ) 
	{
	  ; 
	  (void) fprintf( stdout , "%s\n",  "! mf.pool has no check sum." ) ; 
	  aclose ( poolfile ) ; 
	  Result = false ; 
	  goto lab10 ; 
	} 
	read ( poolfile , m ) ; 
	read ( poolfile , n ) ; 
	if ( m == '*' ) 
	{
	  a = 0 ; 
	  k = 1 ; 
	  while ( true ) {
	      
	    if ( ( xord [ n ] < 48 ) || ( xord [ n ] > 57 ) ) 
	    {
	      ; 
	      (void) fprintf( stdout , "%s\n",                "! mf.pool check sum doesn't have nine digits." ) ; 
	      aclose ( poolfile ) ; 
	      Result = false ; 
	      goto lab10 ; 
	    } 
	    a = 10 * a + xord [ n ] - 48 ; 
	    if ( k == 9 ) 
	    goto lab30 ; 
	    incr ( k ) ; 
	    read ( poolfile , n ) ; 
	  } 
	  lab30: if ( a != 426472440L ) 
	  {
	    ; 
	    (void) fprintf( stdout , "%s\n",  "! mf.pool doesn't match; tangle me again." ) ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    goto lab10 ; 
	  } 
	  c = true ; 
	} 
	else {
	    
	  if ( ( xord [ m ] < 48 ) || ( xord [ m ] > 57 ) || ( xord [ n ] < 48 
	  ) || ( xord [ n ] > 57 ) ) 
	  {
	    ; 
	    (void) fprintf( stdout , "%s\n",  "! mf.pool line doesn't begin with two digits."             ) ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    goto lab10 ; 
	  } 
	  l = xord [ m ] * 10 + xord [ n ] - 48 * 11 ; 
	  if ( poolptr + l + stringvacancies > poolsize ) 
	  {
	    ; 
	    (void) fprintf( stdout , "%s\n",  "! You have to increase POOLSIZE." ) ; 
	    aclose ( poolfile ) ; 
	    Result = false ; 
	    goto lab10 ; 
	  } 
	  {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) 
	  do 
	    {
	      if ( eoln ( poolfile ) ) 
	      m = ' ' ; 
	      else read ( poolfile , m ) ; 
	      {
		strpool [ poolptr ] = xord [ m ] ; 
		incr ( poolptr ) ; 
	      } 
	    } 
	  while ( k++ < for_end ) ; } 
	  readln ( poolfile ) ; 
	  g = makestring () ; 
	  strref [ g ] = 127 ; 
	} 
      } 
    } while ( ! ( c ) ) ; 
    aclose ( poolfile ) ; 
    Result = true ; 
  } 
  else {
      
    ; 
    (void) fprintf( stdout , "%s\n",  "! I can't read mf.pool." ) ; 
    aclose ( poolfile ) ; 
    Result = false ; 
    goto lab10 ; 
  } 
  lab10: ; 
  return(Result) ; 
} 
#endif /* INIMF */
#ifdef INIMF
void sortavail ( ) 
{halfword p, q, r  ; 
  halfword oldrover  ; 
  p = getnode ( 1073741824L ) ; 
  p = mem [ rover + 1 ] .hhfield .v.RH ; 
  mem [ rover + 1 ] .hhfield .v.RH = 262143L ; 
  oldrover = rover ; 
  while ( p != oldrover ) if ( p < rover ) 
  {
    q = p ; 
    p = mem [ q + 1 ] .hhfield .v.RH ; 
    mem [ q + 1 ] .hhfield .v.RH = rover ; 
    rover = q ; 
  } 
  else {
      
    q = rover ; 
    while ( mem [ q + 1 ] .hhfield .v.RH < p ) q = mem [ q + 1 ] .hhfield 
    .v.RH ; 
    r = mem [ p + 1 ] .hhfield .v.RH ; 
    mem [ p + 1 ] .hhfield .v.RH = mem [ q + 1 ] .hhfield .v.RH ; 
    mem [ q + 1 ] .hhfield .v.RH = p ; 
    p = r ; 
  } 
  p = rover ; 
  while ( mem [ p + 1 ] .hhfield .v.RH != 262143L ) {
      
    mem [ mem [ p + 1 ] .hhfield .v.RH + 1 ] .hhfield .lhfield = p ; 
    p = mem [ p + 1 ] .hhfield .v.RH ; 
  } 
  mem [ p + 1 ] .hhfield .v.RH = rover ; 
  mem [ rover + 1 ] .hhfield .lhfield = p ; 
} 
#endif /* INIMF */
#ifdef INIMF
void zprimitive ( s , c , o ) 
strnumber s ; 
halfword c ; 
halfword o ; 
{poolpointer k  ; 
  smallnumber j  ; 
  smallnumber l  ; 
  k = strstart [ s ] ; 
  l = strstart [ s + 1 ] - k ; 
  {register integer for_end; j = 0 ; for_end = l - 1 ; if ( j <= for_end) do 
    buffer [ j ] = strpool [ k + j ] ; 
  while ( j++ < for_end ) ; } 
  cursym = idlookup ( 0 , l ) ; 
  if ( s >= 256 ) 
  {
    flushstring ( strptr - 1 ) ; 
    hash [ cursym ] .v.RH = s ; 
  } 
  eqtb [ cursym ] .lhfield = c ; 
  eqtb [ cursym ] .v.RH = o ; 
} 
#endif /* INIMF */
#ifdef INIMF
void storebasefile ( ) 
{integer k  ; 
  halfword p, q  ; 
  integer x  ; 
  fourquarters w  ; 
  selector = 5 ; 
  print ( 1068 ) ; 
  print ( jobname ) ; 
  printchar ( 32 ) ; 
  printint ( roundunscaled ( internal [ 14 ] ) % 100 ) ; 
  printchar ( 46 ) ; 
  printint ( roundunscaled ( internal [ 15 ] ) ) ; 
  printchar ( 46 ) ; 
  printint ( roundunscaled ( internal [ 16 ] ) ) ; 
  printchar ( 41 ) ; 
  if ( interaction == 0 ) 
  selector = 2 ; 
  else selector = 3 ; 
  {
    if ( poolptr + 1 > maxpoolptr ) 
    {
      if ( poolptr + 1 > poolsize ) 
      overflow ( 257 , poolsize - initpoolptr ) ; 
      maxpoolptr = poolptr + 1 ; 
    } 
  } 
  baseident = makestring () ; 
  strref [ baseident ] = 127 ; 
  packjobname ( 739 ) ; 
  while ( ! wopenout ( basefile ) ) promptfilename ( 1069 , 739 ) ; 
  printnl ( 1070 ) ; 
  slowprint ( wmakenamestring ( basefile ) ) ; 
  flushstring ( strptr - 1 ) ; 
  printnl ( 283 ) ; 
  slowprint ( baseident ) ; 
  dumpint ( 426472440L ) ; 
  dumpint ( 0 ) ; 
  dumpint ( memtop ) ; 
  dumpint ( 9500 ) ; 
  dumpint ( 7919 ) ; 
  dumpint ( 15 ) ; 
  dumpint ( poolptr ) ; 
  dumpint ( strptr ) ; 
  {register integer for_end; k = 0 ; for_end = strptr ; if ( k <= for_end) do 
    dumpint ( strstart [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  k = 0 ; 
  while ( k + 4 < poolptr ) {
      
    w .b0 = strpool [ k ] ; 
    w .b1 = strpool [ k + 1 ] ; 
    w .b2 = strpool [ k + 2 ] ; 
    w .b3 = strpool [ k + 3 ] ; 
    dumpqqqq ( w ) ; 
    k = k + 4 ; 
  } 
  k = poolptr - 4 ; 
  w .b0 = strpool [ k ] ; 
  w .b1 = strpool [ k + 1 ] ; 
  w .b2 = strpool [ k + 2 ] ; 
  w .b3 = strpool [ k + 3 ] ; 
  dumpqqqq ( w ) ; 
  println () ; 
  printint ( strptr ) ; 
  print ( 1065 ) ; 
  printint ( poolptr ) ; 
  sortavail () ; 
  varused = 0 ; 
  dumpint ( lomemmax ) ; 
  dumpint ( rover ) ; 
  p = 0 ; 
  q = rover ; 
  x = 0 ; 
  do {
      { register integer for_end; k = p ; for_end = q + 1 ; if ( k <= 
    for_end) do 
      dumpwd ( mem [ k ] ) ; 
    while ( k++ < for_end ) ; } 
    x = x + q + 2 - p ; 
    varused = varused + q - p ; 
    p = q + mem [ q ] .hhfield .lhfield ; 
    q = mem [ q + 1 ] .hhfield .v.RH ; 
  } while ( ! ( q == rover ) ) ; 
  varused = varused + lomemmax - p ; 
  dynused = memend + 1 - himemmin ; 
  {register integer for_end; k = p ; for_end = lomemmax ; if ( k <= for_end) 
  do 
    dumpwd ( mem [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  x = x + lomemmax + 1 - p ; 
  dumpint ( himemmin ) ; 
  dumpint ( avail ) ; 
  {register integer for_end; k = himemmin ; for_end = memend ; if ( k <= 
  for_end) do 
    dumpwd ( mem [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  x = x + memend + 1 - himemmin ; 
  p = avail ; 
  while ( p != 0 ) {
      
    decr ( dynused ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  dumpint ( varused ) ; 
  dumpint ( dynused ) ; 
  println () ; 
  printint ( x ) ; 
  print ( 1066 ) ; 
  printint ( varused ) ; 
  printchar ( 38 ) ; 
  printint ( dynused ) ; 
  dumpint ( hashused ) ; 
  stcount = 9756 - hashused ; 
  {register integer for_end; p = 1 ; for_end = hashused ; if ( p <= for_end) 
  do 
    if ( hash [ p ] .v.RH != 0 ) 
    {
      dumpint ( p ) ; 
      dumphh ( hash [ p ] ) ; 
      dumphh ( eqtb [ p ] ) ; 
      incr ( stcount ) ; 
    } 
  while ( p++ < for_end ) ; } 
  {register integer for_end; p = hashused + 1 ; for_end = 9769 ; if ( p <= 
  for_end) do 
    {
      dumphh ( hash [ p ] ) ; 
      dumphh ( eqtb [ p ] ) ; 
    } 
  while ( p++ < for_end ) ; } 
  dumpint ( stcount ) ; 
  println () ; 
  printint ( stcount ) ; 
  print ( 1067 ) ; 
  dumpint ( intptr ) ; 
  {register integer for_end; k = 1 ; for_end = intptr ; if ( k <= for_end) do 
    {
      dumpint ( internal [ k ] ) ; 
      dumpint ( intname [ k ] ) ; 
    } 
  while ( k++ < for_end ) ; } 
  dumpint ( startsym ) ; 
  dumpint ( interaction ) ; 
  dumpint ( baseident ) ; 
  dumpint ( bgloc ) ; 
  dumpint ( egloc ) ; 
  dumpint ( serialno ) ; 
  dumpint ( 69069L ) ; 
  internal [ 12 ] = 0 ; 
  wclose ( basefile ) ; 
} 
#endif /* INIMF */
void finalcleanup ( ) 
{/* 10 */ smallnumber c  ; 
  c = curmod ; 
  if ( jobname == 0 ) 
  openlogfile () ; 
  while ( openparens > 0 ) {
      
    print ( 1072 ) ; 
    decr ( openparens ) ; 
  } 
  while ( condptr != 0 ) {
      
    printnl ( 1073 ) ; 
    printcmdmod ( 2 , curif ) ; 
    if ( ifline != 0 ) 
    {
      print ( 1074 ) ; 
      printint ( ifline ) ; 
    } 
    print ( 1075 ) ; 
    ifline = mem [ condptr + 1 ] .cint ; 
    curif = mem [ condptr ] .hhfield .b1 ; 
    condptr = mem [ condptr ] .hhfield .v.RH ; 
  } 
  if ( history != 0 ) 
  if ( ( ( history == 1 ) || ( interaction < 3 ) ) ) 
  if ( selector == 3 ) 
  {
    selector = 1 ; 
    printnl ( 1076 ) ; 
    selector = 3 ; 
  } 
  if ( c == 1 ) 
  {
	;
#ifdef INIMF
    storebasefile () ; 
    goto lab10 ; 
#endif /* INIMF */
    printnl ( 1077 ) ; 
    goto lab10 ; 
  } 
  lab10: ; 
} 
#ifdef INIMF
void initprim ( ) 
{primitive ( 408 , 40 , 1 ) ; 
  primitive ( 409 , 40 , 2 ) ; 
  primitive ( 410 , 40 , 3 ) ; 
  primitive ( 411 , 40 , 4 ) ; 
  primitive ( 412 , 40 , 5 ) ; 
  primitive ( 413 , 40 , 6 ) ; 
  primitive ( 414 , 40 , 7 ) ; 
  primitive ( 415 , 40 , 8 ) ; 
  primitive ( 416 , 40 , 9 ) ; 
  primitive ( 417 , 40 , 10 ) ; 
  primitive ( 418 , 40 , 11 ) ; 
  primitive ( 419 , 40 , 12 ) ; 
  primitive ( 420 , 40 , 13 ) ; 
  primitive ( 421 , 40 , 14 ) ; 
  primitive ( 422 , 40 , 15 ) ; 
  primitive ( 423 , 40 , 16 ) ; 
  primitive ( 424 , 40 , 17 ) ; 
  primitive ( 425 , 40 , 18 ) ; 
  primitive ( 426 , 40 , 19 ) ; 
  primitive ( 427 , 40 , 20 ) ; 
  primitive ( 428 , 40 , 21 ) ; 
  primitive ( 429 , 40 , 22 ) ; 
  primitive ( 430 , 40 , 23 ) ; 
  primitive ( 431 , 40 , 24 ) ; 
  primitive ( 432 , 40 , 25 ) ; 
  primitive ( 433 , 40 , 26 ) ; 
  primitive ( 434 , 40 , 27 ) ; 
  primitive ( 435 , 40 , 28 ) ; 
  primitive ( 436 , 40 , 29 ) ; 
  primitive ( 437 , 40 , 30 ) ; 
  primitive ( 438 , 40 , 31 ) ; 
  primitive ( 439 , 40 , 32 ) ; 
  primitive ( 440 , 40 , 33 ) ; 
  primitive ( 441 , 40 , 34 ) ; 
  primitive ( 442 , 40 , 35 ) ; 
  primitive ( 443 , 40 , 36 ) ; 
  primitive ( 444 , 40 , 37 ) ; 
  primitive ( 445 , 40 , 38 ) ; 
  primitive ( 446 , 40 , 39 ) ; 
  primitive ( 447 , 40 , 40 ) ; 
  primitive ( 448 , 40 , 41 ) ; 
  primitive ( 407 , 47 , 0 ) ; 
  primitive ( 91 , 63 , 0 ) ; 
  eqtb [ 9760 ] = eqtb [ cursym ] ; 
  primitive ( 93 , 64 , 0 ) ; 
  primitive ( 125 , 65 , 0 ) ; 
  primitive ( 123 , 46 , 0 ) ; 
  primitive ( 58 , 81 , 0 ) ; 
  eqtb [ 9762 ] = eqtb [ cursym ] ; 
  primitive ( 458 , 80 , 0 ) ; 
  primitive ( 459 , 79 , 0 ) ; 
  primitive ( 460 , 77 , 0 ) ; 
  primitive ( 44 , 82 , 0 ) ; 
  primitive ( 59 , 83 , 0 ) ; 
  eqtb [ 9763 ] = eqtb [ cursym ] ; 
  primitive ( 92 , 7 , 0 ) ; 
  primitive ( 461 , 18 , 0 ) ; 
  primitive ( 462 , 72 , 0 ) ; 
  primitive ( 463 , 59 , 0 ) ; 
  primitive ( 464 , 32 , 0 ) ; 
  bgloc = cursym ; 
  primitive ( 465 , 57 , 0 ) ; 
  primitive ( 466 , 19 , 0 ) ; 
  primitive ( 467 , 60 , 0 ) ; 
  primitive ( 468 , 27 , 0 ) ; 
  primitive ( 469 , 11 , 0 ) ; 
  primitive ( 452 , 84 , 0 ) ; 
  eqtb [ 9767 ] = eqtb [ cursym ] ; 
  egloc = cursym ; 
  primitive ( 470 , 26 , 0 ) ; 
  primitive ( 471 , 6 , 0 ) ; 
  primitive ( 472 , 9 , 0 ) ; 
  primitive ( 473 , 70 , 0 ) ; 
  primitive ( 474 , 73 , 0 ) ; 
  primitive ( 475 , 13 , 0 ) ; 
  primitive ( 476 , 14 , 0 ) ; 
  primitive ( 477 , 15 , 0 ) ; 
  primitive ( 478 , 69 , 0 ) ; 
  primitive ( 479 , 28 , 0 ) ; 
  primitive ( 480 , 24 , 0 ) ; 
  primitive ( 481 , 12 , 0 ) ; 
  primitive ( 482 , 8 , 0 ) ; 
  primitive ( 483 , 17 , 0 ) ; 
  primitive ( 484 , 78 , 0 ) ; 
  primitive ( 485 , 74 , 0 ) ; 
  primitive ( 486 , 35 , 0 ) ; 
  primitive ( 487 , 58 , 0 ) ; 
  primitive ( 488 , 71 , 0 ) ; 
  primitive ( 489 , 75 , 0 ) ; 
  primitive ( 652 , 16 , 1 ) ; 
  primitive ( 653 , 16 , 2 ) ; 
  primitive ( 654 , 16 , 53 ) ; 
  primitive ( 655 , 16 , 44 ) ; 
  primitive ( 656 , 16 , 49 ) ; 
  primitive ( 453 , 16 , 0 ) ; 
  eqtb [ 9765 ] = eqtb [ cursym ] ; 
  primitive ( 657 , 4 , 9770 ) ; 
  primitive ( 658 , 4 , 9920 ) ; 
  primitive ( 659 , 4 , 1 ) ; 
  primitive ( 454 , 4 , 0 ) ; 
  eqtb [ 9764 ] = eqtb [ cursym ] ; 
  primitive ( 660 , 61 , 0 ) ; 
  primitive ( 661 , 61 , 1 ) ; 
  primitive ( 64 , 61 , 2 ) ; 
  primitive ( 662 , 61 , 3 ) ; 
  primitive ( 673 , 56 , 9770 ) ; 
  primitive ( 674 , 56 , 9920 ) ; 
  primitive ( 675 , 56 , 10070 ) ; 
  primitive ( 676 , 56 , 1 ) ; 
  primitive ( 677 , 56 , 2 ) ; 
  primitive ( 678 , 56 , 3 ) ; 
  primitive ( 688 , 3 , 0 ) ; 
  primitive ( 614 , 3 , 1 ) ; 
  primitive ( 715 , 1 , 1 ) ; 
  primitive ( 451 , 2 , 2 ) ; 
  eqtb [ 9766 ] = eqtb [ cursym ] ; 
  primitive ( 716 , 2 , 3 ) ; 
  primitive ( 717 , 2 , 4 ) ; 
  primitive ( 346 , 33 , 30 ) ; 
  primitive ( 347 , 33 , 31 ) ; 
  primitive ( 348 , 33 , 32 ) ; 
  primitive ( 349 , 33 , 33 ) ; 
  primitive ( 350 , 33 , 34 ) ; 
  primitive ( 351 , 33 , 35 ) ; 
  primitive ( 352 , 33 , 36 ) ; 
  primitive ( 353 , 33 , 37 ) ; 
  primitive ( 354 , 34 , 38 ) ; 
  primitive ( 355 , 34 , 39 ) ; 
  primitive ( 356 , 34 , 40 ) ; 
  primitive ( 357 , 34 , 41 ) ; 
  primitive ( 358 , 34 , 42 ) ; 
  primitive ( 359 , 34 , 43 ) ; 
  primitive ( 360 , 34 , 44 ) ; 
  primitive ( 361 , 34 , 45 ) ; 
  primitive ( 362 , 34 , 46 ) ; 
  primitive ( 363 , 34 , 47 ) ; 
  primitive ( 364 , 34 , 48 ) ; 
  primitive ( 365 , 34 , 49 ) ; 
  primitive ( 366 , 34 , 50 ) ; 
  primitive ( 367 , 34 , 51 ) ; 
  primitive ( 368 , 34 , 52 ) ; 
  primitive ( 369 , 34 , 53 ) ; 
  primitive ( 370 , 34 , 54 ) ; 
  primitive ( 371 , 34 , 55 ) ; 
  primitive ( 372 , 34 , 56 ) ; 
  primitive ( 373 , 34 , 57 ) ; 
  primitive ( 374 , 34 , 58 ) ; 
  primitive ( 375 , 34 , 59 ) ; 
  primitive ( 376 , 34 , 60 ) ; 
  primitive ( 377 , 34 , 61 ) ; 
  primitive ( 378 , 34 , 62 ) ; 
  primitive ( 379 , 34 , 63 ) ; 
  primitive ( 380 , 34 , 64 ) ; 
  primitive ( 381 , 34 , 65 ) ; 
  primitive ( 382 , 34 , 66 ) ; 
  primitive ( 383 , 34 , 67 ) ; 
  primitive ( 384 , 36 , 68 ) ; 
  primitive ( 43 , 43 , 69 ) ; 
  primitive ( 45 , 43 , 70 ) ; 
  primitive ( 42 , 55 , 71 ) ; 
  primitive ( 47 , 54 , 72 ) ; 
  eqtb [ 9761 ] = eqtb [ cursym ] ; 
  primitive ( 385 , 45 , 73 ) ; 
  primitive ( 309 , 45 , 74 ) ; 
  primitive ( 387 , 52 , 76 ) ; 
  primitive ( 386 , 45 , 75 ) ; 
  primitive ( 60 , 50 , 77 ) ; 
  primitive ( 388 , 50 , 78 ) ; 
  primitive ( 62 , 50 , 79 ) ; 
  primitive ( 389 , 50 , 80 ) ; 
  primitive ( 61 , 51 , 81 ) ; 
  primitive ( 390 , 50 , 82 ) ; 
  primitive ( 400 , 37 , 94 ) ; 
  primitive ( 401 , 37 , 95 ) ; 
  primitive ( 402 , 37 , 96 ) ; 
  primitive ( 403 , 37 , 97 ) ; 
  primitive ( 404 , 37 , 98 ) ; 
  primitive ( 405 , 37 , 99 ) ; 
  primitive ( 406 , 37 , 100 ) ; 
  primitive ( 38 , 48 , 83 ) ; 
  primitive ( 391 , 55 , 84 ) ; 
  primitive ( 392 , 55 , 85 ) ; 
  primitive ( 393 , 55 , 86 ) ; 
  primitive ( 394 , 55 , 87 ) ; 
  primitive ( 395 , 55 , 88 ) ; 
  primitive ( 396 , 55 , 89 ) ; 
  primitive ( 397 , 55 , 90 ) ; 
  primitive ( 398 , 55 , 91 ) ; 
  primitive ( 399 , 45 , 92 ) ; 
  primitive ( 339 , 30 , 15 ) ; 
  primitive ( 325 , 30 , 4 ) ; 
  primitive ( 323 , 30 , 2 ) ; 
  primitive ( 330 , 30 , 9 ) ; 
  primitive ( 327 , 30 , 6 ) ; 
  primitive ( 332 , 30 , 11 ) ; 
  primitive ( 334 , 30 , 13 ) ; 
  primitive ( 335 , 30 , 14 ) ; 
  primitive ( 909 , 85 , 0 ) ; 
  primitive ( 910 , 85 , 1 ) ; 
  primitive ( 271 , 23 , 0 ) ; 
  primitive ( 272 , 23 , 1 ) ; 
  primitive ( 273 , 23 , 2 ) ; 
  primitive ( 916 , 23 , 3 ) ; 
  primitive ( 917 , 21 , 0 ) ; 
  primitive ( 918 , 21 , 1 ) ; 
  primitive ( 932 , 22 , 0 ) ; 
  primitive ( 933 , 22 , 1 ) ; 
  primitive ( 934 , 22 , 2 ) ; 
  primitive ( 935 , 22 , 3 ) ; 
  primitive ( 936 , 22 , 4 ) ; 
  primitive ( 953 , 68 , 1 ) ; 
  primitive ( 954 , 68 , 0 ) ; 
  primitive ( 955 , 68 , 2 ) ; 
  primitive ( 956 , 66 , 6 ) ; 
  primitive ( 957 , 66 , 16 ) ; 
  primitive ( 958 , 67 , 0 ) ; 
  primitive ( 959 , 67 , 1 ) ; 
  primitive ( 989 , 25 , 0 ) ; 
  primitive ( 990 , 25 , 1 ) ; 
  primitive ( 991 , 25 , 2 ) ; 
  primitive ( 1001 , 20 , 0 ) ; 
  primitive ( 1002 , 20 , 1 ) ; 
  primitive ( 1003 , 20 , 2 ) ; 
  primitive ( 1004 , 20 , 3 ) ; 
  primitive ( 1005 , 20 , 4 ) ; 
  primitive ( 1023 , 76 , 0 ) ; 
  primitive ( 1024 , 76 , 1 ) ; 
  primitive ( 1025 , 76 , 5 ) ; 
  primitive ( 1026 , 76 , 2 ) ; 
  primitive ( 1027 , 76 , 6 ) ; 
  primitive ( 1028 , 76 , 3 ) ; 
  primitive ( 1029 , 76 , 7 ) ; 
  primitive ( 1030 , 76 , 11 ) ; 
  primitive ( 1031 , 76 , 128 ) ; 
  primitive ( 1055 , 29 , 4 ) ; 
  primitive ( 1056 , 29 , 16 ) ; 
} 
void inittab ( ) 
{integer k  ; 
  rover = 23 ; 
  mem [ rover ] .hhfield .v.RH = 262143L ; 
  mem [ rover ] .hhfield .lhfield = 1000 ; 
  mem [ rover + 1 ] .hhfield .lhfield = rover ; 
  mem [ rover + 1 ] .hhfield .v.RH = rover ; 
  lomemmax = rover + 1000 ; 
  mem [ lomemmax ] .hhfield .v.RH = 0 ; 
  mem [ lomemmax ] .hhfield .lhfield = 0 ; 
  {register integer for_end; k = memtop - 2 ; for_end = memtop ; if ( k <= 
  for_end) do 
    mem [ k ] = mem [ lomemmax ] ; 
  while ( k++ < for_end ) ; } 
  avail = 0 ; 
  memend = memtop ; 
  himemmin = memtop - 2 ; 
  varused = 23 ; 
  dynused = memtop + 1 - himemmin ; 
  intname [ 1 ] = 408 ; 
  intname [ 2 ] = 409 ; 
  intname [ 3 ] = 410 ; 
  intname [ 4 ] = 411 ; 
  intname [ 5 ] = 412 ; 
  intname [ 6 ] = 413 ; 
  intname [ 7 ] = 414 ; 
  intname [ 8 ] = 415 ; 
  intname [ 9 ] = 416 ; 
  intname [ 10 ] = 417 ; 
  intname [ 11 ] = 418 ; 
  intname [ 12 ] = 419 ; 
  intname [ 13 ] = 420 ; 
  intname [ 14 ] = 421 ; 
  intname [ 15 ] = 422 ; 
  intname [ 16 ] = 423 ; 
  intname [ 17 ] = 424 ; 
  intname [ 18 ] = 425 ; 
  intname [ 19 ] = 426 ; 
  intname [ 20 ] = 427 ; 
  intname [ 21 ] = 428 ; 
  intname [ 22 ] = 429 ; 
  intname [ 23 ] = 430 ; 
  intname [ 24 ] = 431 ; 
  intname [ 25 ] = 432 ; 
  intname [ 26 ] = 433 ; 
  intname [ 27 ] = 434 ; 
  intname [ 28 ] = 435 ; 
  intname [ 29 ] = 436 ; 
  intname [ 30 ] = 437 ; 
  intname [ 31 ] = 438 ; 
  intname [ 32 ] = 439 ; 
  intname [ 33 ] = 440 ; 
  intname [ 34 ] = 441 ; 
  intname [ 35 ] = 442 ; 
  intname [ 36 ] = 443 ; 
  intname [ 37 ] = 444 ; 
  intname [ 38 ] = 445 ; 
  intname [ 39 ] = 446 ; 
  intname [ 40 ] = 447 ; 
  intname [ 41 ] = 448 ; 
  hashused = 9757 ; 
  stcount = 0 ; 
  hash [ 9768 ] .v.RH = 450 ; 
  hash [ 9766 ] .v.RH = 451 ; 
  hash [ 9767 ] .v.RH = 452 ; 
  hash [ 9765 ] .v.RH = 453 ; 
  hash [ 9764 ] .v.RH = 454 ; 
  hash [ 9763 ] .v.RH = 59 ; 
  hash [ 9762 ] .v.RH = 58 ; 
  hash [ 9761 ] .v.RH = 47 ; 
  hash [ 9760 ] .v.RH = 91 ; 
  hash [ 9759 ] .v.RH = 41 ; 
  hash [ 9757 ] .v.RH = 455 ; 
  eqtb [ 9759 ] .lhfield = 62 ; 
  mem [ 19 ] .hhfield .lhfield = 9770 ; 
  mem [ 19 ] .hhfield .v.RH = 0 ; 
  mem [ memtop ] .hhfield .lhfield = 262143L ; 
  mem [ 3 ] .hhfield .lhfield = 0 ; 
  mem [ 3 ] .hhfield .v.RH = 0 ; 
  mem [ 4 ] .hhfield .lhfield = 1 ; 
  mem [ 4 ] .hhfield .v.RH = 0 ; 
  {register integer for_end; k = 5 ; for_end = 11 ; if ( k <= for_end) do 
    mem [ k ] = mem [ 4 ] ; 
  while ( k++ < for_end ) ; } 
  mem [ 12 ] .cint = 0 ; 
  mem [ 0 ] .hhfield .v.RH = 0 ; 
  mem [ 0 ] .hhfield .lhfield = 0 ; 
  mem [ 1 ] .cint = 0 ; 
  mem [ 2 ] .cint = 0 ; 
  serialno = 0 ; 
  mem [ 13 ] .hhfield .v.RH = 13 ; 
  mem [ 14 ] .hhfield .lhfield = 13 ; 
  mem [ 13 ] .hhfield .lhfield = 0 ; 
  mem [ 14 ] .hhfield .v.RH = 0 ; 
  mem [ 21 ] .hhfield .b1 = 0 ; 
  mem [ 21 ] .hhfield .v.RH = 9768 ; 
  eqtb [ 9768 ] .v.RH = 21 ; 
  eqtb [ 9768 ] .lhfield = 41 ; 
  eqtb [ 9758 ] .lhfield = 91 ; 
  hash [ 9758 ] .v.RH = 732 ; 
  mem [ 17 ] .hhfield .b1 = 11 ; 
  mem [ 20 ] .cint = 1073741824L ; 
  mem [ 16 ] .cint = 0 ; 
  mem [ 15 ] .hhfield .lhfield = 0 ; 
  baseident = 1064 ; 
} 
#endif /* INIMF */
void main_body() {
    
  history = 3 ; 
  setpaths ( MFBASEPATHBIT + MFINPUTPATHBIT + MFPOOLPATHBIT ) ; 
  if ( readyalready == 314159L ) 
  goto lab1 ; 
  bad = 0 ; 
  if ( ( halferrorline < 30 ) || ( halferrorline > errorline - 15 ) ) 
  bad = 1 ; 
  if ( maxprintline < 60 ) 
  bad = 2 ; 
  if ( gfbufsize % 8 != 0 ) 
  bad = 3 ; 
  if ( 1100 > memtop ) 
  bad = 4 ; 
  if ( 7919 > 9500 ) 
  bad = 5 ; 
  if ( headersize % 4 != 0 ) 
  bad = 6 ; 
  if ( ( ligtablesize < 255 ) || ( ligtablesize > 32510 ) ) 
  bad = 7 ; 
#ifdef INIMF
  if ( memmax != memtop ) 
  bad = 10 ; 
#endif /* INIMF */
  if ( memmax < memtop ) 
  bad = 10 ; 
  if ( ( 0 > 0 ) || ( 255 < 127 ) ) 
  bad = 11 ; 
  if ( ( 0 > 0 ) || ( 262143L < 32767 ) ) 
  bad = 12 ; 
  if ( ( 0 < 0 ) || ( 255 > 262143L ) ) 
  bad = 13 ; 
  if ( ( 0 < 0 ) || ( memmax >= 262143L ) ) 
  bad = 14 ; 
  if ( maxstrings > 262143L ) 
  bad = 15 ; 
  if ( bufsize > 262143L ) 
  bad = 16 ; 
  if ( ( 255 < 255 ) || ( 262143L < 65535L ) ) 
  bad = 17 ; 
  if ( 9769 + maxinternal > 262143L ) 
  bad = 21 ; 
  if ( 10220 > 262143L ) 
  bad = 22 ; 
  if ( 15 * 11 > bistacksize ) 
  bad = 31 ; 
  if ( 20 + 17 * 45 > bistacksize ) 
  bad = 32 ; 
  if ( basedefaultlength > PATHMAX ) 
  bad = 41 ; 
  if ( bad > 0 ) 
  {
    (void) fprintf( stdout , "%s%s%ld\n",  "Ouch---my internal constants have been clobbered!" ,     "---case " , (long)bad ) ; 
    goto lab9999 ; 
  } 
  initialize () ; 
#ifdef INIMF
  if ( ! getstringsstarted () ) 
  goto lab9999 ; 
  inittab () ; 
  initprim () ; 
  initstrptr = strptr ; 
  initpoolptr = poolptr ; 
  maxstrptr = strptr ; 
  maxpoolptr = poolptr ; 
  fixdateandtime () ; 
#endif /* INIMF */
  readyalready = 314159L ; 
  lab1: selector = 1 ; 
  tally = 0 ; 
  termoffset = 0 ; 
  fileoffset = 0 ; 
  (void) Fputs( stdout ,  "This is METAFONT, C Version 2.71" ) ; 
  if ( baseident > 0 ) 
  slowprint ( baseident ) ; 
  println () ; 
  flush ( stdout ) ; 
  jobname = 0 ; 
  logopened = false ; 
  outputfilename = 0 ; 
  {
    {
      inputptr = 0 ; 
      maxinstack = 0 ; 
      inopen = 0 ; 
      openparens = 0 ; 
      maxbufstack = 0 ; 
      paramptr = 0 ; 
      maxparamstack = 0 ; 
      first = 1 ; 
      curinput .startfield = 1 ; 
      curinput .indexfield = 0 ; 
      line = 0 ; 
      curinput .namefield = 0 ; 
      forceeof = false ; 
      if ( ! initterminal () ) 
      goto lab9999 ; 
      curinput .limitfield = last ; 
      first = last + 1 ; 
    } 
    scannerstatus = 0 ; 
    if ( ( baseident == 0 ) || ( buffer [ curinput .locfield ] == 38 ) ) 
    {
      if ( baseident != 0 ) 
      initialize () ; 
      if ( ! openbasefile () ) 
      goto lab9999 ; 
      if ( ! loadbasefile () ) 
      {
	wclose ( basefile ) ; 
	goto lab9999 ; 
      } 
      wclose ( basefile ) ; 
      while ( ( curinput .locfield < curinput .limitfield ) && ( buffer [ 
      curinput .locfield ] == 32 ) ) incr ( curinput .locfield ) ; 
    } 
    buffer [ curinput .limitfield ] = 37 ; 
    fixdateandtime () ; 
    initrandoms ( ( internal [ 17 ] / 65536L ) + internal [ 16 ] ) ; 
    if ( interaction == 0 ) 
    selector = 0 ; 
    else selector = 1 ; 
    if ( curinput .locfield < curinput .limitfield ) 
    if ( buffer [ curinput .locfield ] != 92 ) 
    startinput () ; 
  } 
  history = 0 ; 
  if ( startsym > 0 ) 
  {
    cursym = startsym ; 
    backinput () ; 
  } 
  maincontrol () ; 
  finalcleanup () ; 
  closefilesandterminate () ; 
  lab9999: {
      
    flush ( stdout ) ; 
    readyalready = 0 ; 
    if ( ( history != 0 ) && ( history != 1 ) ) 
    uexit ( 1 ) ; 
    else uexit ( 0 ) ; 
  } 
} 
