#define EXTERN extern
#include "texd.h"

void showbox ( halfword p ) 
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
void deletetokenref ( halfword p ) 
{deletetokenref_regmem 
  if ( mem [ p ] .hh .v.LH == 0 ) 
  flushlist ( p ) ; 
  else decr ( mem [ p ] .hh .v.LH ) ; 
} 
void deleteglueref ( halfword p ) 
{deleteglueref_regmem 
  if ( mem [ p ] .hh .v.RH == 0 ) 
  freenode ( p , 4 ) ; 
  else decr ( mem [ p ] .hh .v.RH ) ; 
} 
void flushnodelist ( halfword p ) 
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
	    confusion ( 1285 ) ; 
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
halfword copynodelist ( halfword p ) 
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
	confusion ( 1284 ) ; 
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
void printmode ( integer m ) 
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
void printparam ( integer n ) 
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
  