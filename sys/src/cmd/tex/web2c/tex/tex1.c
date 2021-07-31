#define EXTERN extern
#include "texd.h"

void zshowbox ( p ) 
halfword p ; 
{showbox_regmem 
  depththreshold = eqtb [ 12688 ] .cint ; 
  breadthmax = eqtb [ 12687 ] .cint ; 
  if ( breadthmax <= 0 ) 
  breadthmax = 5 ; 
  if ( poolptr + depththreshold >= poolsize ) 
  depththreshold = poolsize - poolptr - 1 ; 
  shownodelist ( p ) ; 
  println () ; 
} 
void zdeletetokenref ( p ) 
halfword p ; 
{deletetokenref_regmem 
  if ( mem [ p ] .hh .v.LH == 0 ) 
  flushlist ( p ) ; 
  else decr ( mem [ p ] .hh .v.LH ) ; 
} 
void zdeleteglueref ( p ) 
halfword p ; 
{deleteglueref_regmem 
  if ( mem [ p ] .hh .v.RH == 0 ) 
  freenode ( p , 4 ) ; 
  else decr ( mem [ p ] .hh .v.RH ) ; 
} 
void zflushnodelist ( p ) 
halfword p ; 
{/* 30 */ flushnodelist_regmem 
  halfword q  ; 
  while ( p != 0 ) {
      
    q = mem [ p ] .hh .v.RH ; 
    if ( ( p >= himemmin ) ) 
    {
      mem [ p ] .hh .v.RH = avail ; 
      avail = p ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
    } 
    else {
	
      switch ( mem [ p ] .hh.b0 ) 
      {case 0 : 
      case 1 : 
      case 13 : 
	{
	  flushnodelist ( mem [ p + 5 ] .hh .v.RH ) ; 
	  freenode ( p , 7 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 2 : 
	{
	  freenode ( p , 4 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 3 : 
	{
	  flushnodelist ( mem [ p + 4 ] .hh .v.LH ) ; 
	  deleteglueref ( mem [ p + 4 ] .hh .v.RH ) ; 
	  freenode ( p , 5 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 8 : 
	{
	  switch ( mem [ p ] .hh.b1 ) 
	  {case 0 : 
	    freenode ( p , 3 ) ; 
	    break ; 
	  case 1 : 
	  case 3 : 
	    {
	      deletetokenref ( mem [ p + 1 ] .hh .v.RH ) ; 
	      freenode ( p , 2 ) ; 
	      goto lab30 ; 
	    } 
	    break ; 
	  case 2 : 
	  case 4 : 
	    freenode ( p , 2 ) ; 
	    break ; 
	    default: 
	    confusion ( 1287 ) ; 
	    break ; 
	  } 
	  goto lab30 ; 
	} 
	break ; 
      case 10 : 
	{
	  {
	    if ( mem [ mem [ p + 1 ] .hh .v.LH ] .hh .v.RH == 0 ) 
	    freenode ( mem [ p + 1 ] .hh .v.LH , 4 ) ; 
	    else decr ( mem [ mem [ p + 1 ] .hh .v.LH ] .hh .v.RH ) ; 
	  } 
	  if ( mem [ p + 1 ] .hh .v.RH != 0 ) 
	  flushnodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	} 
	break ; 
      case 11 : 
      case 9 : 
      case 12 : 
	; 
	break ; 
      case 6 : 
	flushnodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	break ; 
      case 4 : 
	deletetokenref ( mem [ p + 1 ] .cint ) ; 
	break ; 
      case 7 : 
	{
	  flushnodelist ( mem [ p + 1 ] .hh .v.LH ) ; 
	  flushnodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	} 
	break ; 
      case 5 : 
	flushnodelist ( mem [ p + 1 ] .cint ) ; 
	break ; 
      case 14 : 
	{
	  freenode ( p , 3 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 15 : 
	{
	  flushnodelist ( mem [ p + 1 ] .hh .v.LH ) ; 
	  flushnodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
	  flushnodelist ( mem [ p + 2 ] .hh .v.LH ) ; 
	  flushnodelist ( mem [ p + 2 ] .hh .v.RH ) ; 
	  freenode ( p , 3 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 16 : 
      case 17 : 
      case 18 : 
      case 19 : 
      case 20 : 
      case 21 : 
      case 22 : 
      case 23 : 
      case 24 : 
      case 27 : 
      case 26 : 
      case 29 : 
      case 28 : 
	{
	  if ( mem [ p + 1 ] .hh .v.RH >= 2 ) 
	  flushnodelist ( mem [ p + 1 ] .hh .v.LH ) ; 
	  if ( mem [ p + 2 ] .hh .v.RH >= 2 ) 
	  flushnodelist ( mem [ p + 2 ] .hh .v.LH ) ; 
	  if ( mem [ p + 3 ] .hh .v.RH >= 2 ) 
	  flushnodelist ( mem [ p + 3 ] .hh .v.LH ) ; 
	  if ( mem [ p ] .hh.b0 == 24 ) 
	  freenode ( p , 5 ) ; 
	  else if ( mem [ p ] .hh.b0 == 28 ) 
	  freenode ( p , 5 ) ; 
	  else freenode ( p , 4 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 30 : 
      case 31 : 
	{
	  freenode ( p , 4 ) ; 
	  goto lab30 ; 
	} 
	break ; 
      case 25 : 
	{
	  flushnodelist ( mem [ p + 2 ] .hh .v.LH ) ; 
	  flushnodelist ( mem [ p + 3 ] .hh .v.LH ) ; 
	  freenode ( p , 6 ) ; 
	  goto lab30 ; 
	} 
	break ; 
	default: 
	confusion ( 350 ) ; 
	break ; 
      } 
      freenode ( p , 2 ) ; 
      lab30: ; 
    } 
    p = q ; 
  } 
} 
halfword zcopynodelist ( p ) 
halfword p ; 
{register halfword Result; copynodelist_regmem 
  halfword h  ; 
  halfword q  ; 
  halfword r  ; 
  schar words  ; 
  h = getavail () ; 
  q = h ; 
  while ( p != 0 ) {
      
    words = 1 ; 
    if ( ( p >= himemmin ) ) 
    r = getavail () ; 
    else switch ( mem [ p ] .hh.b0 ) 
    {case 0 : 
    case 1 : 
    case 13 : 
      {
	r = getnode ( 7 ) ; 
	mem [ r + 6 ] = mem [ p + 6 ] ; 
	mem [ r + 5 ] = mem [ p + 5 ] ; 
	mem [ r + 5 ] .hh .v.RH = copynodelist ( mem [ p + 5 ] .hh .v.RH ) ; 
	words = 5 ; 
      } 
      break ; 
    case 2 : 
      {
	r = getnode ( 4 ) ; 
	words = 4 ; 
      } 
      break ; 
    case 3 : 
      {
	r = getnode ( 5 ) ; 
	mem [ r + 4 ] = mem [ p + 4 ] ; 
	incr ( mem [ mem [ p + 4 ] .hh .v.RH ] .hh .v.RH ) ; 
	mem [ r + 4 ] .hh .v.LH = copynodelist ( mem [ p + 4 ] .hh .v.LH ) ; 
	words = 4 ; 
      } 
      break ; 
    case 8 : 
      switch ( mem [ p ] .hh.b1 ) 
      {case 0 : 
	{
	  r = getnode ( 3 ) ; 
	  words = 3 ; 
	} 
	break ; 
      case 1 : 
      case 3 : 
	{
	  r = getnode ( 2 ) ; 
	  incr ( mem [ mem [ p + 1 ] .hh .v.RH ] .hh .v.LH ) ; 
	  words = 2 ; 
	} 
	break ; 
      case 2 : 
      case 4 : 
	{
	  r = getnode ( 2 ) ; 
	  words = 2 ; 
	} 
	break ; 
	default: 
	confusion ( 1286 ) ; 
	break ; 
      } 
      break ; 
    case 10 : 
      {
	r = getnode ( 2 ) ; 
	incr ( mem [ mem [ p + 1 ] .hh .v.LH ] .hh .v.RH ) ; 
	mem [ r + 1 ] .hh .v.LH = mem [ p + 1 ] .hh .v.LH ; 
	mem [ r + 1 ] .hh .v.RH = copynodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
      } 
      break ; 
    case 11 : 
    case 9 : 
    case 12 : 
      {
	r = getnode ( 2 ) ; 
	words = 2 ; 
      } 
      break ; 
    case 6 : 
      {
	r = getnode ( 2 ) ; 
	mem [ r + 1 ] = mem [ p + 1 ] ; 
	mem [ r + 1 ] .hh .v.RH = copynodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
      } 
      break ; 
    case 7 : 
      {
	r = getnode ( 2 ) ; 
	mem [ r + 1 ] .hh .v.LH = copynodelist ( mem [ p + 1 ] .hh .v.LH ) ; 
	mem [ r + 1 ] .hh .v.RH = copynodelist ( mem [ p + 1 ] .hh .v.RH ) ; 
      } 
      break ; 
    case 4 : 
      {
	r = getnode ( 2 ) ; 
	incr ( mem [ mem [ p + 1 ] .cint ] .hh .v.LH ) ; 
	words = 2 ; 
      } 
      break ; 
    case 5 : 
      {
	r = getnode ( 2 ) ; 
	mem [ r + 1 ] .cint = copynodelist ( mem [ p + 1 ] .cint ) ; 
      } 
      break ; 
      default: 
      confusion ( 351 ) ; 
      break ; 
    } 
    while ( words > 0 ) {
	
      decr ( words ) ; 
      mem [ r + words ] = mem [ p + words ] ; 
    } 
    mem [ q ] .hh .v.RH = r ; 
    q = r ; 
    p = mem [ p ] .hh .v.RH ; 
  } 
  mem [ q ] .hh .v.RH = 0 ; 
  q = mem [ h ] .hh .v.RH ; 
  {
    mem [ h ] .hh .v.RH = avail ; 
    avail = h ; 
	;
#ifdef STAT
    decr ( dynused ) ; 
#endif /* STAT */
  } 
  Result = q ; 
  return(Result) ; 
} 
void zprintmode ( m ) 
integer m ; 
{printmode_regmem 
  if ( m > 0 ) 
  switch ( m / ( 101 ) ) 
  {case 0 : 
    print ( 352 ) ; 
    break ; 
  case 1 : 
    print ( 353 ) ; 
    break ; 
  case 2 : 
    print ( 354 ) ; 
    break ; 
  } 
  else if ( m == 0 ) 
  print ( 355 ) ; 
  else switch ( ( - (integer) m ) / ( 101 ) ) 
  {case 0 : 
    print ( 356 ) ; 
    break ; 
  case 1 : 
    print ( 357 ) ; 
    break ; 
  case 2 : 
    print ( 340 ) ; 
    break ; 
  } 
  print ( 358 ) ; 
} 
void pushnest ( ) 
{pushnest_regmem 
  if ( nestptr > maxneststack ) 
  {
    maxneststack = nestptr ; 
    if ( nestptr == nestsize ) 
    overflow ( 359 , nestsize ) ; 
  } 
  nest [ nestptr ] = curlist ; 
  incr ( nestptr ) ; 
  curlist .headfield = getavail () ; 
  curlist .tailfield = curlist .headfield ; 
  curlist .pgfield = 0 ; 
  curlist .mlfield = line ; 
} 
void popnest ( ) 
{popnest_regmem 
  {
    mem [ curlist .headfield ] .hh .v.RH = avail ; 
    avail = curlist .headfield ; 
	;
#ifdef STAT
    decr ( dynused ) ; 
#endif /* STAT */
  } 
  decr ( nestptr ) ; 
  curlist = nest [ nestptr ] ; 
} 
void showactivities ( ) 
{showactivities_regmem 
  integer p  ; 
  short m  ; 
  memoryword a  ; 
  halfword q, r  ; 
  integer t  ; 
  nest [ nestptr ] = curlist ; 
  printnl ( 335 ) ; 
  println () ; 
  {register integer for_end; p = nestptr ; for_end = 0 ; if ( p >= for_end) 
  do 
    {
      m = nest [ p ] .modefield ; 
      a = nest [ p ] .auxfield ; 
      printnl ( 360 ) ; 
      printmode ( m ) ; 
      print ( 361 ) ; 
      printint ( abs ( nest [ p ] .mlfield ) ) ; 
      if ( m == 102 ) 
      if ( ( nest [ p ] .lhmfield != 2 ) || ( nest [ p ] .rhmfield != 3 ) ) 
      {
	print ( 362 ) ; 
	printint ( nest [ p ] .lhmfield ) ; 
	printchar ( 44 ) ; 
	printint ( nest [ p ] .rhmfield ) ; 
	printchar ( 41 ) ; 
      } 
      if ( nest [ p ] .mlfield < 0 ) 
      print ( 363 ) ; 
      if ( p == 0 ) 
      {
	if ( memtop - 2 != pagetail ) 
	{
	  printnl ( 973 ) ; 
	  if ( outputactive ) 
	  print ( 974 ) ; 
	  showbox ( mem [ memtop - 2 ] .hh .v.RH ) ; 
	  if ( pagecontents > 0 ) 
	  {
	    printnl ( 975 ) ; 
	    printtotals () ; 
	    printnl ( 976 ) ; 
	    printscaled ( pagesofar [ 0 ] ) ; 
	    r = mem [ memtop ] .hh .v.RH ; 
	    while ( r != memtop ) {
		
	      println () ; 
	      printesc ( 327 ) ; 
	      t = mem [ r ] .hh.b1 ; 
	      printint ( t ) ; 
	      print ( 977 ) ; 
	      t = xovern ( mem [ r + 3 ] .cint , 1000 ) * eqtb [ 12718 + t ] 
	      .cint ; 
	      printscaled ( t ) ; 
	      if ( mem [ r ] .hh.b0 == 1 ) 
	      {
		q = memtop - 2 ; 
		t = 0 ; 
		do {
		    q = mem [ q ] .hh .v.RH ; 
		  if ( ( mem [ q ] .hh.b0 == 3 ) && ( mem [ q ] .hh.b1 == mem 
		  [ r ] .hh.b1 ) ) 
		  incr ( t ) ; 
		} while ( ! ( q == mem [ r + 1 ] .hh .v.LH ) ) ; 
		print ( 978 ) ; 
		printint ( t ) ; 
		print ( 979 ) ; 
	      } 
	      r = mem [ r ] .hh .v.RH ; 
	    } 
	  } 
	} 
	if ( mem [ memtop - 1 ] .hh .v.RH != 0 ) 
	printnl ( 364 ) ; 
      } 
      showbox ( mem [ nest [ p ] .headfield ] .hh .v.RH ) ; 
      switch ( abs ( m ) / ( 101 ) ) 
      {case 0 : 
	{
	  printnl ( 365 ) ; 
	  if ( a .cint <= -65536000L ) 
	  print ( 366 ) ; 
	  else printscaled ( a .cint ) ; 
	  if ( nest [ p ] .pgfield != 0 ) 
	  {
	    print ( 367 ) ; 
	    printint ( nest [ p ] .pgfield ) ; 
	    print ( 368 ) ; 
	    if ( nest [ p ] .pgfield != 1 ) 
	    printchar ( 115 ) ; 
	  } 
	} 
	break ; 
      case 1 : 
	{
	  printnl ( 369 ) ; 
	  printint ( a .hh .v.LH ) ; 
	  if ( m > 0 ) 
	  if ( a .hh .v.RH > 0 ) 
	  {
	    print ( 370 ) ; 
	    printint ( a .hh .v.RH ) ; 
	  } 
	} 
	break ; 
      case 2 : 
	if ( a .cint != 0 ) 
	{
	  print ( 371 ) ; 
	  showbox ( a .cint ) ; 
	} 
	break ; 
      } 
    } 
  while ( p-- > for_end ) ; } 
} 
void zprintparam ( n ) 
integer n ; 
{printparam_regmem 
  switch ( n ) 
  {case 0 : 
    printesc ( 416 ) ; 
    break ; 
  case 1 : 
    printesc ( 417 ) ; 
    break ; 
  case 2 : 
    printesc ( 418 ) ; 
    break ; 
  case 3 : 
    printesc ( 419 ) ; 
    break ; 
  case 4 : 
    printesc ( 420 ) ; 
    break ; 
  case 5 : 
    printesc ( 421 ) ; 
    break ; 
  case 6 : 
    printesc ( 422 ) ; 
    break ; 
  case 7 : 
    printesc ( 423 ) ; 
    break ; 
  case 8 : 
    printesc ( 424 ) ; 
    break ; 
  case 9 : 
    printesc ( 425 ) ; 
    break ; 
  case 10 : 
    printesc ( 426 ) ; 
    break ; 
  case 11 : 
    printesc ( 427 ) ; 
    break ; 
  case 12 : 
    printesc ( 428 ) ; 
    break ; 
  case 13 : 
    printesc ( 429 ) ; 
    break ; 
  case 14 : 
    printesc ( 430 ) ; 
    break ; 
  case 15 : 
    printesc ( 431 ) ; 
    break ; 
  case 16 : 
    printesc ( 432 ) ; 
    break ; 
  case 17 : 
    printesc ( 433 ) ; 
    break ; 
  case 18 : 
    printesc ( 434 ) ; 
    break ; 
  case 19 : 
    printesc ( 435 ) ; 
    break ; 
  case 20 : 
    printesc ( 436 ) ; 
    break ; 
  case 21 : 
    printesc ( 437 ) ; 
    break ; 
  case 22 : 
    printesc ( 438 ) ; 
    break ; 
  case 23 : 
    printesc ( 439 ) ; 
    break ; 
  case 24 : 
    printesc ( 440 ) ; 
    break ; 
  case 25 : 
    printesc ( 441 ) ; 
    break ; 
  case 26 : 
    printesc ( 442 ) ; 
    break ; 
  case 27 : 
    printesc ( 443 ) ; 
    break ; 
  case 28 : 
    printesc ( 444 ) ; 
    break ; 
  case 29 : 
    printesc ( 445 ) ; 
    break ; 
  case 30 : 
    printesc ( 446 ) ; 
    break ; 
  case 31 : 
    printesc ( 447 ) ; 
    break ; 
  case 32 : 
    printesc ( 448 ) ; 
    break ; 
  case 33 : 
    printesc ( 449 ) ; 
    break ; 
  case 34 : 
    printesc ( 450 ) ; 
    break ; 
  case 35 : 
    printesc ( 451 ) ; 
    break ; 
  case 36 : 
    printesc ( 452 ) ; 
    break ; 
  case 37 : 
    printesc ( 453 ) ; 
    break ; 
  case 38 : 
    printesc ( 454 ) ; 
    break ; 
  case 39 : 
    printesc ( 455 ) ; 
    break ; 
  case 40 : 
    printesc ( 456 ) ; 
    break ; 
  case 41 : 
    printesc ( 457 ) ; 
    break ; 
  case 42 : 
    printesc ( 458 ) ; 
    break ; 
  case 43 : 
    printesc ( 459 ) ; 
    break ; 
  case 44 : 
    printesc ( 460 ) ; 
    break ; 
  case 45 : 
    printesc ( 461 ) ; 
    break ; 
  case 46 : 
    printesc ( 462 ) ; 
    break ; 
  case 47 : 
    printesc ( 463 ) ; 
    break ; 
  case 48 : 
    printesc ( 464 ) ; 
    break ; 
  case 49 : 
    printesc ( 465 ) ; 
    break ; 
  case 50 : 
    printesc ( 466 ) ; 
    break ; 
  case 51 : 
    printesc ( 467 ) ; 
    break ; 
  case 52 : 
    printesc ( 468 ) ; 
    break ; 
  case 53 : 
    printesc ( 469 ) ; 
    break ; 
  case 54 : 
    printesc ( 470 ) ; 
    break ; 
    default: 
    print ( 471 ) ; 
    break ; 
  } 
} 
void begindiagnostic ( ) 
{begindiagnostic_regmem 
  oldsetting = selector ; 
  if ( ( eqtb [ 12692 ] .cint <= 0 ) && ( selector == 19 ) ) 
  {
    decr ( selector ) ; 
    if ( history == 0 ) 
    history = 1 ; 
  } 
} 
void zenddiagnostic ( blankline ) 
boolean blankline ; 
{enddiagnostic_regmem 
  printnl ( 335 ) ; 
  if ( blankline ) 
  println () ; 
  selector = oldsetting ; 
} 
void zprintlengthparam ( n ) 
integer n ; 
{printlengthparam_regmem 
  switch ( n ) 
  {case 0 : 
    printesc ( 474 ) ; 
    break ; 
  case 1 : 
    printesc ( 475 ) ; 
    break ; 
  case 2 : 
    printesc ( 476 ) ; 
    break ; 
  case 3 : 
    printesc ( 477 ) ; 
    break ; 
  case 4 : 
    printesc ( 478 ) ; 
    break ; 
  case 5 : 
    printesc ( 479 ) ; 
    break ; 
  case 6 : 
    printesc ( 480 ) ; 
    break ; 
  case 7 : 
    printesc ( 481 ) ; 
    break ; 
  case 8 : 
    printesc ( 482 ) ; 
    break ; 
  case 9 : 
    printesc ( 483 ) ; 
    break ; 
  case 10 : 
    printesc ( 484 ) ; 
    break ; 
  case 11 : 
    printesc ( 485 ) ; 
    break ; 
  case 12 : 
    printesc ( 486 ) ; 
    break ; 
  case 13 : 
    printesc ( 487 ) ; 
    break ; 
  case 14 : 
    printesc ( 488 ) ; 
    break ; 
  case 15 : 
    printesc ( 489 ) ; 
    break ; 
  case 16 : 
    printesc ( 490 ) ; 
    break ; 
  case 17 : 
    printesc ( 491 ) ; 
    break ; 
  case 18 : 
    printesc ( 492 ) ; 
    break ; 
  case 19 : 
    printesc ( 493 ) ; 
    break ; 
  case 20 : 
    printesc ( 494 ) ; 
    break ; 
    default: 
    print ( 495 ) ; 
    break ; 
  } 
} 
void zprintcmdchr ( cmd , chrcode ) 
quarterword cmd ; 
halfword chrcode ; 
{printcmdchr_regmem 
  switch ( cmd ) 
  {case 1 : 
    {
      print ( 553 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 2 : 
    {
      print ( 554 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 3 : 
    {
      print ( 555 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 6 : 
    {
      print ( 556 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 7 : 
    {
      print ( 557 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 8 : 
    {
      print ( 558 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 9 : 
    print ( 559 ) ; 
    break ; 
  case 10 : 
    {
      print ( 560 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 11 : 
    {
      print ( 561 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 12 : 
    {
      print ( 562 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 75 : 
  case 76 : 
    if ( chrcode < 10300 ) 
    printskipparam ( chrcode - 10282 ) ; 
    else if ( chrcode < 10556 ) 
    {
      printesc ( 391 ) ; 
      printint ( chrcode - 10300 ) ; 
    } 
    else {
	
      printesc ( 392 ) ; 
      printint ( chrcode - 10556 ) ; 
    } 
    break ; 
  case 72 : 
    if ( chrcode >= 10822 ) 
    {
      printesc ( 403 ) ; 
      printint ( chrcode - 10822 ) ; 
    } 
    else switch ( chrcode ) 
    {case 10813 : 
      printesc ( 394 ) ; 
      break ; 
    case 10814 : 
      printesc ( 395 ) ; 
      break ; 
    case 10815 : 
      printesc ( 396 ) ; 
      break ; 
    case 10816 : 
      printesc ( 397 ) ; 
      break ; 
    case 10817 : 
      printesc ( 398 ) ; 
      break ; 
    case 10818 : 
      printesc ( 399 ) ; 
      break ; 
    case 10819 : 
      printesc ( 400 ) ; 
      break ; 
    case 10820 : 
      printesc ( 401 ) ; 
      break ; 
      default: 
      printesc ( 402 ) ; 
      break ; 
    } 
    break ; 
  case 73 : 
    if ( chrcode < 12718 ) 
    printparam ( chrcode - 12663 ) ; 
    else {
	
      printesc ( 472 ) ; 
      printint ( chrcode - 12718 ) ; 
    } 
    break ; 
  case 74 : 
    if ( chrcode < 13251 ) 
    printlengthparam ( chrcode - 13230 ) ; 
    else {
	
      printesc ( 496 ) ; 
      printint ( chrcode - 13251 ) ; 
    } 
    break ; 
  case 45 : 
    printesc ( 504 ) ; 
    break ; 
  case 90 : 
    printesc ( 505 ) ; 
    break ; 
  case 40 : 
    printesc ( 506 ) ; 
    break ; 
  case 41 : 
    printesc ( 507 ) ; 
    break ; 
  case 77 : 
    printesc ( 515 ) ; 
    break ; 
  case 61 : 
    printesc ( 508 ) ; 
    break ; 
  case 42 : 
    printesc ( 527 ) ; 
    break ; 
  case 16 : 
    printesc ( 509 ) ; 
    break ; 
  case 107 : 
    printesc ( 500 ) ; 
    break ; 
  case 88 : 
    printesc ( 514 ) ; 
    break ; 
  case 15 : 
    printesc ( 510 ) ; 
    break ; 
  case 92 : 
    printesc ( 511 ) ; 
    break ; 
  case 67 : 
    printesc ( 501 ) ; 
    break ; 
  case 62 : 
    printesc ( 512 ) ; 
    break ; 
  case 64 : 
    printesc ( 32 ) ; 
    break ; 
  case 102 : 
    printesc ( 513 ) ; 
    break ; 
  case 32 : 
    printesc ( 516 ) ; 
    break ; 
  case 36 : 
    printesc ( 517 ) ; 
    break ; 
  case 39 : 
    printesc ( 518 ) ; 
    break ; 
  case 37 : 
    printesc ( 327 ) ; 
    break ; 
  case 44 : 
    printesc ( 47 ) ; 
    break ; 
  case 18 : 
    printesc ( 348 ) ; 
    break ; 
  case 46 : 
    printesc ( 519 ) ; 
    break ; 
  case 17 : 
    printesc ( 520 ) ; 
    break ; 
  case 54 : 
    printesc ( 521 ) ; 
    break ; 
  case 91 : 
    printesc ( 522 ) ; 
    break ; 
  case 34 : 
    printesc ( 523 ) ; 
    break ; 
  case 65 : 
    printesc ( 524 ) ; 
    break ; 
  case 103 : 
    printesc ( 525 ) ; 
    break ; 
  case 55 : 
    printesc ( 332 ) ; 
    break ; 
  case 63 : 
    printesc ( 526 ) ; 
    break ; 
  case 66 : 
    printesc ( 529 ) ; 
    break ; 
  case 96 : 
    printesc ( 530 ) ; 
    break ; 
  case 0 : 
    printesc ( 531 ) ; 
    break ; 
  case 98 : 
    printesc ( 532 ) ; 
    break ; 
  case 80 : 
    printesc ( 528 ) ; 
    break ; 
  case 84 : 
    printesc ( 404 ) ; 
    break ; 
  case 109 : 
    printesc ( 533 ) ; 
    break ; 
  case 71 : 
    printesc ( 403 ) ; 
    break ; 
  case 38 : 
    printesc ( 349 ) ; 
    break ; 
  case 33 : 
    printesc ( 534 ) ; 
    break ; 
  case 56 : 
    printesc ( 535 ) ; 
    break ; 
  case 35 : 
    printesc ( 536 ) ; 
    break ; 
  case 13 : 
    printesc ( 593 ) ; 
    break ; 
  case 104 : 
    if ( chrcode == 0 ) 
    printesc ( 625 ) ; 
    else printesc ( 626 ) ; 
    break ; 
  case 110 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 628 ) ; 
      break ; 
    case 2 : 
      printesc ( 629 ) ; 
      break ; 
    case 3 : 
      printesc ( 630 ) ; 
      break ; 
    case 4 : 
      printesc ( 631 ) ; 
      break ; 
      default: 
      printesc ( 627 ) ; 
      break ; 
    } 
    break ; 
  case 89 : 
    if ( chrcode == 0 ) 
    printesc ( 472 ) ; 
    else if ( chrcode == 1 ) 
    printesc ( 496 ) ; 
    else if ( chrcode == 2 ) 
    printesc ( 391 ) ; 
    else printesc ( 392 ) ; 
    break ; 
  case 79 : 
    if ( chrcode == 1 ) 
    printesc ( 665 ) ; 
    else printesc ( 664 ) ; 
    break ; 
  case 82 : 
    if ( chrcode == 0 ) 
    printesc ( 666 ) ; 
    else printesc ( 667 ) ; 
    break ; 
  case 83 : 
    if ( chrcode == 1 ) 
    printesc ( 668 ) ; 
    else if ( chrcode == 3 ) 
    printesc ( 669 ) ; 
    else printesc ( 670 ) ; 
    break ; 
  case 70 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 671 ) ; 
      break ; 
    case 1 : 
      printesc ( 672 ) ; 
      break ; 
    case 2 : 
      printesc ( 673 ) ; 
      break ; 
    case 3 : 
      printesc ( 674 ) ; 
      break ; 
      default: 
      printesc ( 675 ) ; 
      break ; 
    } 
    break ; 
  case 108 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 731 ) ; 
      break ; 
    case 1 : 
      printesc ( 732 ) ; 
      break ; 
    case 2 : 
      printesc ( 733 ) ; 
      break ; 
    case 3 : 
      printesc ( 734 ) ; 
      break ; 
    case 4 : 
      printesc ( 735 ) ; 
      break ; 
      default: 
      printesc ( 736 ) ; 
      break ; 
    } 
    break ; 
  case 105 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 753 ) ; 
      break ; 
    case 2 : 
      printesc ( 754 ) ; 
      break ; 
    case 3 : 
      printesc ( 755 ) ; 
      break ; 
    case 4 : 
      printesc ( 756 ) ; 
      break ; 
    case 5 : 
      printesc ( 757 ) ; 
      break ; 
    case 6 : 
      printesc ( 758 ) ; 
      break ; 
    case 7 : 
      printesc ( 759 ) ; 
      break ; 
    case 8 : 
      printesc ( 760 ) ; 
      break ; 
    case 9 : 
      printesc ( 761 ) ; 
      break ; 
    case 10 : 
      printesc ( 762 ) ; 
      break ; 
    case 11 : 
      printesc ( 763 ) ; 
      break ; 
    case 12 : 
      printesc ( 764 ) ; 
      break ; 
    case 13 : 
      printesc ( 765 ) ; 
      break ; 
    case 14 : 
      printesc ( 766 ) ; 
      break ; 
    case 15 : 
      printesc ( 767 ) ; 
      break ; 
    case 16 : 
      printesc ( 768 ) ; 
      break ; 
      default: 
      printesc ( 752 ) ; 
      break ; 
    } 
    break ; 
  case 106 : 
    if ( chrcode == 2 ) 
    printesc ( 769 ) ; 
    else if ( chrcode == 4 ) 
    printesc ( 770 ) ; 
    else printesc ( 771 ) ; 
    break ; 
  case 4 : 
    if ( chrcode == 256 ) 
    printesc ( 891 ) ; 
    else {
	
      print ( 895 ) ; 
      print ( chrcode ) ; 
    } 
    break ; 
  case 5 : 
    if ( chrcode == 257 ) 
    printesc ( 892 ) ; 
    else printesc ( 893 ) ; 
    break ; 
  case 81 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 963 ) ; 
      break ; 
    case 1 : 
      printesc ( 964 ) ; 
      break ; 
    case 2 : 
      printesc ( 965 ) ; 
      break ; 
    case 3 : 
      printesc ( 966 ) ; 
      break ; 
    case 4 : 
      printesc ( 967 ) ; 
      break ; 
    case 5 : 
      printesc ( 968 ) ; 
      break ; 
    case 6 : 
      printesc ( 969 ) ; 
      break ; 
      default: 
      printesc ( 970 ) ; 
      break ; 
    } 
    break ; 
  case 14 : 
    if ( chrcode == 1 ) 
    printesc ( 1019 ) ; 
    else printesc ( 1018 ) ; 
    break ; 
  case 26 : 
    switch ( chrcode ) 
    {case 4 : 
      printesc ( 1020 ) ; 
      break ; 
    case 0 : 
      printesc ( 1021 ) ; 
      break ; 
    case 1 : 
      printesc ( 1022 ) ; 
      break ; 
    case 2 : 
      printesc ( 1023 ) ; 
      break ; 
      default: 
      printesc ( 1024 ) ; 
      break ; 
    } 
    break ; 
  case 27 : 
    switch ( chrcode ) 
    {case 4 : 
      printesc ( 1025 ) ; 
      break ; 
    case 0 : 
      printesc ( 1026 ) ; 
      break ; 
    case 1 : 
      printesc ( 1027 ) ; 
      break ; 
    case 2 : 
      printesc ( 1028 ) ; 
      break ; 
      default: 
      printesc ( 1029 ) ; 
      break ; 
    } 
    break ; 
  case 28 : 
    printesc ( 333 ) ; 
    break ; 
  case 29 : 
    printesc ( 337 ) ; 
    break ; 
  case 30 : 
    printesc ( 339 ) ; 
    break ; 
  case 21 : 
    if ( chrcode == 1 ) 
    printesc ( 1047 ) ; 
    else printesc ( 1048 ) ; 
    break ; 
  case 22 : 
    if ( chrcode == 1 ) 
    printesc ( 1049 ) ; 
    else printesc ( 1050 ) ; 
    break ; 
  case 20 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 405 ) ; 
      break ; 
    case 1 : 
      printesc ( 1051 ) ; 
      break ; 
    case 2 : 
      printesc ( 1052 ) ; 
      break ; 
    case 3 : 
      printesc ( 958 ) ; 
      break ; 
    case 4 : 
      printesc ( 1053 ) ; 
      break ; 
    case 5 : 
      printesc ( 960 ) ; 
      break ; 
      default: 
      printesc ( 1054 ) ; 
      break ; 
    } 
    break ; 
  case 31 : 
    if ( chrcode == 100 ) 
    printesc ( 1056 ) ; 
    else if ( chrcode == 101 ) 
    printesc ( 1057 ) ; 
    else if ( chrcode == 102 ) 
    printesc ( 1058 ) ; 
    else printesc ( 1055 ) ; 
    break ; 
  case 43 : 
    if ( chrcode == 0 ) 
    printesc ( 1074 ) ; 
    else printesc ( 1073 ) ; 
    break ; 
  case 25 : 
    if ( chrcode == 10 ) 
    printesc ( 1085 ) ; 
    else if ( chrcode == 11 ) 
    printesc ( 1084 ) ; 
    else printesc ( 1083 ) ; 
    break ; 
  case 23 : 
    if ( chrcode == 1 ) 
    printesc ( 1087 ) ; 
    else printesc ( 1086 ) ; 
    break ; 
  case 24 : 
    if ( chrcode == 1 ) 
    printesc ( 1089 ) ; 
    else printesc ( 1088 ) ; 
    break ; 
  case 47 : 
    if ( chrcode == 1 ) 
    printesc ( 45 ) ; 
    else printesc ( 346 ) ; 
    break ; 
  case 48 : 
    if ( chrcode == 1 ) 
    printesc ( 1121 ) ; 
    else printesc ( 1120 ) ; 
    break ; 
  case 50 : 
    switch ( chrcode ) 
    {case 16 : 
      printesc ( 859 ) ; 
      break ; 
    case 17 : 
      printesc ( 860 ) ; 
      break ; 
    case 18 : 
      printesc ( 861 ) ; 
      break ; 
    case 19 : 
      printesc ( 862 ) ; 
      break ; 
    case 20 : 
      printesc ( 863 ) ; 
      break ; 
    case 21 : 
      printesc ( 864 ) ; 
      break ; 
    case 22 : 
      printesc ( 865 ) ; 
      break ; 
    case 23 : 
      printesc ( 866 ) ; 
      break ; 
    case 26 : 
      printesc ( 868 ) ; 
      break ; 
      default: 
      printesc ( 867 ) ; 
      break ; 
    } 
    break ; 
  case 51 : 
    if ( chrcode == 1 ) 
    printesc ( 871 ) ; 
    else if ( chrcode == 2 ) 
    printesc ( 872 ) ; 
    else printesc ( 1122 ) ; 
    break ; 
  case 53 : 
    printstyle ( chrcode ) ; 
    break ; 
  case 52 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 1141 ) ; 
      break ; 
    case 2 : 
      printesc ( 1142 ) ; 
      break ; 
    case 3 : 
      printesc ( 1143 ) ; 
      break ; 
    case 4 : 
      printesc ( 1144 ) ; 
      break ; 
    case 5 : 
      printesc ( 1145 ) ; 
      break ; 
      default: 
      printesc ( 1140 ) ; 
      break ; 
    } 
    break ; 
  case 49 : 
    if ( chrcode == 30 ) 
    printesc ( 869 ) ; 
    else printesc ( 870 ) ; 
    break ; 
  case 93 : 
    if ( chrcode == 1 ) 
    printesc ( 1164 ) ; 
    else if ( chrcode == 2 ) 
    printesc ( 1165 ) ; 
    else printesc ( 1166 ) ; 
    break ; 
  case 97 : 
    if ( chrcode == 0 ) 
    printesc ( 1167 ) ; 
    else if ( chrcode == 1 ) 
    printesc ( 1168 ) ; 
    else if ( chrcode == 2 ) 
    printesc ( 1169 ) ; 
    else printesc ( 1170 ) ; 
    break ; 
  case 94 : 
    if ( chrcode != 0 ) 
    printesc ( 1185 ) ; 
    else printesc ( 1184 ) ; 
    break ; 
  case 95 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 1186 ) ; 
      break ; 
    case 1 : 
      printesc ( 1187 ) ; 
      break ; 
    case 2 : 
      printesc ( 1188 ) ; 
      break ; 
    case 3 : 
      printesc ( 1189 ) ; 
      break ; 
    case 4 : 
      printesc ( 1190 ) ; 
      break ; 
    case 5 : 
      printesc ( 1191 ) ; 
      break ; 
      default: 
      printesc ( 1192 ) ; 
      break ; 
    } 
    break ; 
  case 68 : 
    {
      printesc ( 509 ) ; 
      printhex ( chrcode ) ; 
    } 
    break ; 
  case 69 : 
    {
      printesc ( 520 ) ; 
      printhex ( chrcode ) ; 
    } 
    break ; 
  case 85 : 
    if ( chrcode == 11383 ) 
    printesc ( 411 ) ; 
    else if ( chrcode == 12407 ) 
    printesc ( 415 ) ; 
    else if ( chrcode == 11639 ) 
    printesc ( 412 ) ; 
    else if ( chrcode == 11895 ) 
    printesc ( 413 ) ; 
    else if ( chrcode == 12151 ) 
    printesc ( 414 ) ; 
    else printesc ( 473 ) ; 
    break ; 
  case 86 : 
    printsize ( chrcode - 11335 ) ; 
    break ; 
  case 99 : 
    if ( chrcode == 1 ) 
    printesc ( 946 ) ; 
    else printesc ( 934 ) ; 
    break ; 
  case 78 : 
    if ( chrcode == 0 ) 
    printesc ( 1210 ) ; 
    else printesc ( 1211 ) ; 
    break ; 
  case 87 : 
    {
      print ( 1219 ) ; 
      slowprint ( fontname [ chrcode ] ) ; 
      if ( fontsize [ chrcode ] != fontdsize [ chrcode ] ) 
      {
	print ( 737 ) ; 
	printscaled ( fontsize [ chrcode ] ) ; 
	print ( 393 ) ; 
      } 
    } 
    break ; 
  case 100 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 272 ) ; 
      break ; 
    case 1 : 
      printesc ( 273 ) ; 
      break ; 
    case 2 : 
      printesc ( 274 ) ; 
      break ; 
      default: 
      printesc ( 1220 ) ; 
      break ; 
    } 
    break ; 
  case 60 : 
    if ( chrcode == 0 ) 
    printesc ( 1222 ) ; 
    else printesc ( 1221 ) ; 
    break ; 
  case 58 : 
    if ( chrcode == 0 ) 
    printesc ( 1223 ) ; 
    else printesc ( 1224 ) ; 
    break ; 
  case 57 : 
    if ( chrcode == 11639 ) 
    printesc ( 1230 ) ; 
    else printesc ( 1231 ) ; 
    break ; 
  case 19 : 
    switch ( chrcode ) 
    {case 1 : 
      printesc ( 1233 ) ; 
      break ; 
    case 2 : 
      printesc ( 1234 ) ; 
      break ; 
    case 3 : 
      printesc ( 1235 ) ; 
      break ; 
      default: 
      printesc ( 1232 ) ; 
      break ; 
    } 
    break ; 
  case 101 : 
    print ( 1242 ) ; 
    break ; 
  case 111 : 
    print ( 1243 ) ; 
    break ; 
  case 112 : 
    printesc ( 1244 ) ; 
    break ; 
  case 113 : 
    printesc ( 1245 ) ; 
    break ; 
  case 114 : 
    {
      printesc ( 1164 ) ; 
      printesc ( 1245 ) ; 
    } 
    break ; 
  case 115 : 
    printesc ( 1246 ) ; 
    break ; 
  case 59 : 
    switch ( chrcode ) 
    {case 0 : 
      printesc ( 1278 ) ; 
      break ; 
    case 1 : 
      printesc ( 590 ) ; 
      break ; 
    case 2 : 
      printesc ( 1279 ) ; 
      break ; 
    case 3 : 
      printesc ( 1280 ) ; 
      break ; 
    case 4 : 
      printesc ( 1281 ) ; 
      break ; 
    case 5 : 
      printesc ( 1282 ) ; 
      break ; 
      default: 
      print ( 1283 ) ; 
      break ; 
    } 
    break ; 
    default: 
    print ( 563 ) ; 
    break ; 
  } 
} 
#ifdef STAT
void zshoweqtb ( n ) 
halfword n ; 
{showeqtb_regmem 
  if ( n < 1 ) 
  printchar ( 63 ) ; 
  else if ( n < 10282 ) 
  {
    sprintcs ( n ) ; 
    printchar ( 61 ) ; 
    printcmdchr ( eqtb [ n ] .hh.b0 , eqtb [ n ] .hh .v.RH ) ; 
    if ( eqtb [ n ] .hh.b0 >= 111 ) 
    {
      printchar ( 58 ) ; 
      showtokenlist ( mem [ eqtb [ n ] .hh .v.RH ] .hh .v.RH , 0 , 32 ) ; 
    } 
  } 
  else if ( n < 10812 ) 
  if ( n < 10300 ) 
  {
    printskipparam ( n - 10282 ) ; 
    printchar ( 61 ) ; 
    if ( n < 10297 ) 
    printspec ( eqtb [ n ] .hh .v.RH , 393 ) ; 
    else printspec ( eqtb [ n ] .hh .v.RH , 334 ) ; 
  } 
  else if ( n < 10556 ) 
  {
    printesc ( 391 ) ; 
    printint ( n - 10300 ) ; 
    printchar ( 61 ) ; 
    printspec ( eqtb [ n ] .hh .v.RH , 393 ) ; 
  } 
  else {
      
    printesc ( 392 ) ; 
    printint ( n - 10556 ) ; 
    printchar ( 61 ) ; 
    printspec ( eqtb [ n ] .hh .v.RH , 334 ) ; 
  } 
  else if ( n < 12663 ) 
  if ( n == 10812 ) 
  {
    printesc ( 404 ) ; 
    printchar ( 61 ) ; 
    if ( eqtb [ 10812 ] .hh .v.RH == 0 ) 
    printchar ( 48 ) ; 
    else printint ( mem [ eqtb [ 10812 ] .hh .v.RH ] .hh .v.LH ) ; 
  } 
  else if ( n < 10822 ) 
  {
    printcmdchr ( 72 , n ) ; 
    printchar ( 61 ) ; 
    if ( eqtb [ n ] .hh .v.RH != 0 ) 
    showtokenlist ( mem [ eqtb [ n ] .hh .v.RH ] .hh .v.RH , 0 , 32 ) ; 
  } 
  else if ( n < 11078 ) 
  {
    printesc ( 403 ) ; 
    printint ( n - 10822 ) ; 
    printchar ( 61 ) ; 
    if ( eqtb [ n ] .hh .v.RH != 0 ) 
    showtokenlist ( mem [ eqtb [ n ] .hh .v.RH ] .hh .v.RH , 0 , 32 ) ; 
  } 
  else if ( n < 11334 ) 
  {
    printesc ( 405 ) ; 
    printint ( n - 11078 ) ; 
    printchar ( 61 ) ; 
    if ( eqtb [ n ] .hh .v.RH == 0 ) 
    print ( 406 ) ; 
    else {
	
      depththreshold = 0 ; 
      breadthmax = 1 ; 
      shownodelist ( eqtb [ n ] .hh .v.RH ) ; 
    } 
  } 
  else if ( n < 11383 ) 
  {
    if ( n == 11334 ) 
    print ( 407 ) ; 
    else if ( n < 11351 ) 
    {
      printesc ( 408 ) ; 
      printint ( n - 11335 ) ; 
    } 
    else if ( n < 11367 ) 
    {
      printesc ( 409 ) ; 
      printint ( n - 11351 ) ; 
    } 
    else {
	
      printesc ( 410 ) ; 
      printint ( n - 11367 ) ; 
    } 
    printchar ( 61 ) ; 
    printesc ( hash [ 10024 + eqtb [ n ] .hh .v.RH ] .v.RH ) ; 
  } 
  else if ( n < 12407 ) 
  {
    if ( n < 11639 ) 
    {
      printesc ( 411 ) ; 
      printint ( n - 11383 ) ; 
    } 
    else if ( n < 11895 ) 
    {
      printesc ( 412 ) ; 
      printint ( n - 11639 ) ; 
    } 
    else if ( n < 12151 ) 
    {
      printesc ( 413 ) ; 
      printint ( n - 11895 ) ; 
    } 
    else {
	
      printesc ( 414 ) ; 
      printint ( n - 12151 ) ; 
    } 
    printchar ( 61 ) ; 
    printint ( eqtb [ n ] .hh .v.RH ) ; 
  } 
  else {
      
    printesc ( 415 ) ; 
    printint ( n - 12407 ) ; 
    printchar ( 61 ) ; 
    printint ( eqtb [ n ] .hh .v.RH ) ; 
  } 
  else if ( n < 13230 ) 
  {
    if ( n < 12718 ) 
    printparam ( n - 12663 ) ; 
    else if ( n < 12974 ) 
    {
      printesc ( 472 ) ; 
      printint ( n - 12718 ) ; 
    } 
    else {
	
      printesc ( 473 ) ; 
      printint ( n - 12974 ) ; 
    } 
    printchar ( 61 ) ; 
    printint ( eqtb [ n ] .cint ) ; 
  } 
  else if ( n <= 13506 ) 
  {
    if ( n < 13251 ) 
    printlengthparam ( n - 13230 ) ; 
    else {
	
      printesc ( 496 ) ; 
      printint ( n - 13251 ) ; 
    } 
    printchar ( 61 ) ; 
    printscaled ( eqtb [ n ] .cint ) ; 
    print ( 393 ) ; 
  } 
  else printchar ( 63 ) ; 
} 
#endif /* STAT */
halfword zidlookup ( j , l ) 
integer j ; 
integer l ; 
{/* 40 */ register halfword Result; idlookup_regmem 
  integer h  ; 
  integer d  ; 
  halfword p  ; 
  halfword k  ; 
  h = buffer [ j ] ; 
  {register integer for_end; k = j + 1 ; for_end = j + l - 1 ; if ( k <= 
  for_end) do 
    {
      h = h + h + buffer [ k ] ; 
      while ( h >= 7919 ) h = h - 7919 ; 
    } 
  while ( k++ < for_end ) ; } 
  p = h + 514 ; 
  while ( true ) {
      
    if ( hash [ p ] .v.RH > 0 ) 
    if ( ( strstart [ hash [ p ] .v.RH + 1 ] - strstart [ hash [ p ] .v.RH ] ) 
    == l ) 
    if ( streqbuf ( hash [ p ] .v.RH , j ) ) 
    goto lab40 ; 
    if ( hash [ p ] .v.LH == 0 ) 
    {
      if ( nonewcontrolsequence ) 
      p = 10281 ; 
      else {
	  
	if ( hash [ p ] .v.RH > 0 ) 
	{
	  do {
	      if ( ( hashused == 514 ) ) 
	    overflow ( 499 , 9500 ) ; 
	    decr ( hashused ) ; 
	  } while ( ! ( hash [ hashused ] .v.RH == 0 ) ) ; 
	  hash [ p ] .v.LH = hashused ; 
	  p = hashused ; 
	} 
	{
	  if ( poolptr + l > poolsize ) 
	  overflow ( 257 , poolsize - initpoolptr ) ; 
	} 
	d = ( poolptr - strstart [ strptr ] ) ; 
	while ( poolptr > strstart [ strptr ] ) {
	    
	  decr ( poolptr ) ; 
	  strpool [ poolptr + l ] = strpool [ poolptr ] ; 
	} 
	{register integer for_end; k = j ; for_end = j + l - 1 ; if ( k <= 
	for_end) do 
	  {
	    strpool [ poolptr ] = buffer [ k ] ; 
	    incr ( poolptr ) ; 
	  } 
	while ( k++ < for_end ) ; } 
	hash [ p ] .v.RH = makestring () ; 
	poolptr = poolptr + d ; 
	;
#ifdef STAT
	incr ( cscount ) ; 
#endif /* STAT */
      } 
      goto lab40 ; 
    } 
    p = hash [ p ] .v.LH ; 
  } 
  lab40: Result = p ; 
  return(Result) ; 
} 
void znewsavelevel ( c ) 
groupcode c ; 
{newsavelevel_regmem 
  if ( saveptr > maxsavestack ) 
  {
    maxsavestack = saveptr ; 
    if ( maxsavestack > savesize - 6 ) 
    overflow ( 537 , savesize ) ; 
  } 
  savestack [ saveptr ] .hh.b0 = 3 ; 
  savestack [ saveptr ] .hh.b1 = curgroup ; 
  savestack [ saveptr ] .hh .v.RH = curboundary ; 
  if ( curlevel == 255 ) 
  overflow ( 538 , 255 ) ; 
  curboundary = saveptr ; 
  incr ( curlevel ) ; 
  incr ( saveptr ) ; 
  curgroup = c ; 
} 
void zeqdestroy ( w ) 
memoryword w ; 
{eqdestroy_regmem 
  halfword q  ; 
  switch ( w .hh.b0 ) 
  {case 111 : 
  case 112 : 
  case 113 : 
  case 114 : 
    deletetokenref ( w .hh .v.RH ) ; 
    break ; 
  case 117 : 
    deleteglueref ( w .hh .v.RH ) ; 
    break ; 
  case 118 : 
    {
      q = w .hh .v.RH ; 
      if ( q != 0 ) 
      freenode ( q , mem [ q ] .hh .v.LH + mem [ q ] .hh .v.LH + 1 ) ; 
    } 
    break ; 
  case 119 : 
    flushnodelist ( w .hh .v.RH ) ; 
    break ; 
    default: 
    ; 
    break ; 
  } 
} 
void zeqsave ( p , l ) 
halfword p ; 
quarterword l ; 
{eqsave_regmem 
  if ( saveptr > maxsavestack ) 
  {
    maxsavestack = saveptr ; 
    if ( maxsavestack > savesize - 6 ) 
    overflow ( 537 , savesize ) ; 
  } 
  if ( l == 0 ) 
  savestack [ saveptr ] .hh.b0 = 1 ; 
  else {
      
    savestack [ saveptr ] = eqtb [ p ] ; 
    incr ( saveptr ) ; 
    savestack [ saveptr ] .hh.b0 = 0 ; 
  } 
  savestack [ saveptr ] .hh.b1 = l ; 
  savestack [ saveptr ] .hh .v.RH = p ; 
  incr ( saveptr ) ; 
} 
void zeqdefine ( p , t , e ) 
halfword p ; 
quarterword t ; 
halfword e ; 
{eqdefine_regmem 
  if ( eqtb [ p ] .hh.b1 == curlevel ) 
  eqdestroy ( eqtb [ p ] ) ; 
  else if ( curlevel > 1 ) 
  eqsave ( p , eqtb [ p ] .hh.b1 ) ; 
  eqtb [ p ] .hh.b1 = curlevel ; 
  eqtb [ p ] .hh.b0 = t ; 
  eqtb [ p ] .hh .v.RH = e ; 
} 
void zeqworddefine ( p , w ) 
halfword p ; 
integer w ; 
{eqworddefine_regmem 
  if ( xeqlevel [ p ] != curlevel ) 
  {
    eqsave ( p , xeqlevel [ p ] ) ; 
    xeqlevel [ p ] = curlevel ; 
  } 
  eqtb [ p ] .cint = w ; 
} 
void zgeqdefine ( p , t , e ) 
halfword p ; 
quarterword t ; 
halfword e ; 
{geqdefine_regmem 
  eqdestroy ( eqtb [ p ] ) ; 
  eqtb [ p ] .hh.b1 = 1 ; 
  eqtb [ p ] .hh.b0 = t ; 
  eqtb [ p ] .hh .v.RH = e ; 
} 
void zgeqworddefine ( p , w ) 
halfword p ; 
integer w ; 
{geqworddefine_regmem 
  eqtb [ p ] .cint = w ; 
  xeqlevel [ p ] = 1 ; 
} 
void zsaveforafter ( t ) 
halfword t ; 
{saveforafter_regmem 
  if ( curlevel > 1 ) 
  {
    if ( saveptr > maxsavestack ) 
    {
      maxsavestack = saveptr ; 
      if ( maxsavestack > savesize - 6 ) 
      overflow ( 537 , savesize ) ; 
    } 
    savestack [ saveptr ] .hh.b0 = 2 ; 
    savestack [ saveptr ] .hh.b1 = 0 ; 
    savestack [ saveptr ] .hh .v.RH = t ; 
    incr ( saveptr ) ; 
  } 
} 
#ifdef STAT
void zrestoretrace ( p , s ) 
halfword p ; 
strnumber s ; 
{restoretrace_regmem 
  begindiagnostic () ; 
  printchar ( 123 ) ; 
  print ( s ) ; 
  printchar ( 32 ) ; 
  showeqtb ( p ) ; 
  printchar ( 125 ) ; 
  enddiagnostic ( false ) ; 
} 
#endif /* STAT */
void unsave ( ) 
{/* 30 */ unsave_regmem 
  halfword p  ; 
  quarterword l  ; 
  halfword t  ; 
  if ( curlevel > 1 ) 
  {
    decr ( curlevel ) ; 
    while ( true ) {
	
      decr ( saveptr ) ; 
      if ( savestack [ saveptr ] .hh.b0 == 3 ) 
      goto lab30 ; 
      p = savestack [ saveptr ] .hh .v.RH ; 
      if ( savestack [ saveptr ] .hh.b0 == 2 ) 
      {
	t = curtok ; 
	curtok = p ; 
	backinput () ; 
	curtok = t ; 
      } 
      else {
	  
	if ( savestack [ saveptr ] .hh.b0 == 0 ) 
	{
	  l = savestack [ saveptr ] .hh.b1 ; 
	  decr ( saveptr ) ; 
	} 
	else savestack [ saveptr ] = eqtb [ 10281 ] ; 
	if ( p < 12663 ) 
	if ( eqtb [ p ] .hh.b1 == 1 ) 
	{
	  eqdestroy ( savestack [ saveptr ] ) ; 
	;
#ifdef STAT
	  if ( eqtb [ 12700 ] .cint > 0 ) 
	  restoretrace ( p , 540 ) ; 
#endif /* STAT */
	} 
	else {
	    
	  eqdestroy ( eqtb [ p ] ) ; 
	  eqtb [ p ] = savestack [ saveptr ] ; 
	;
#ifdef STAT
	  if ( eqtb [ 12700 ] .cint > 0 ) 
	  restoretrace ( p , 541 ) ; 
#endif /* STAT */
	} 
	else if ( xeqlevel [ p ] != 1 ) 
	{
	  eqtb [ p ] = savestack [ saveptr ] ; 
	  xeqlevel [ p ] = l ; 
	;
#ifdef STAT
	  if ( eqtb [ 12700 ] .cint > 0 ) 
	  restoretrace ( p , 541 ) ; 
#endif /* STAT */
	} 
	else {
	    
	;
#ifdef STAT
	  if ( eqtb [ 12700 ] .cint > 0 ) 
	  restoretrace ( p , 540 ) ; 
#endif /* STAT */
	} 
      } 
    } 
    lab30: curgroup = savestack [ saveptr ] .hh.b1 ; 
    curboundary = savestack [ saveptr ] .hh .v.RH ; 
  } 
  else confusion ( 539 ) ; 
} 
