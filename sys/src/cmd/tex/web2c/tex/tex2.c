#define EXTERN extern
#include "texd.h"

void preparemag ( ) 
{preparemag_regmem 
  if ( ( magset > 0 ) && ( eqtb [ 12680 ] .cint != magset ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 543 ) ; 
    } 
    printint ( eqtb [ 12680 ] .cint ) ; 
    print ( 544 ) ; 
    printnl ( 545 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 546 ; 
      helpline [ 0 ] = 547 ; 
    } 
    interror ( magset ) ; 
    geqworddefine ( 12680 , magset ) ; 
  } 
  if ( ( eqtb [ 12680 ] .cint <= 0 ) || ( eqtb [ 12680 ] .cint > 32768L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 548 ) ; 
    } 
    {
      helpptr = 1 ; 
      helpline [ 0 ] = 549 ; 
    } 
    interror ( eqtb [ 12680 ] .cint ) ; 
    geqworddefine ( 12680 , 1000 ) ; 
  } 
  magset = eqtb [ 12680 ] .cint ; 
} 
void ztokenshow ( p ) 
halfword p ; 
{tokenshow_regmem 
  if ( p != 0 ) 
  showtokenlist ( mem [ p ] .hh .v.RH , 0 , 10000000L ) ; 
} 
void printmeaning ( ) 
{printmeaning_regmem 
  printcmdchr ( curcmd , curchr ) ; 
  if ( curcmd >= 111 ) 
  {
    printchar ( 58 ) ; 
    println () ; 
    tokenshow ( curchr ) ; 
  } 
  else if ( curcmd == 110 ) 
  {
    printchar ( 58 ) ; 
    println () ; 
    tokenshow ( curmark [ curchr ] ) ; 
  } 
} 
void showcurcmdchr ( ) 
{showcurcmdchr_regmem 
  begindiagnostic () ; 
  printnl ( 123 ) ; 
  if ( curlist .modefield != shownmode ) 
  {
    printmode ( curlist .modefield ) ; 
    print ( 564 ) ; 
    shownmode = curlist .modefield ; 
  } 
  printcmdchr ( curcmd , curchr ) ; 
  printchar ( 125 ) ; 
  enddiagnostic ( false ) ; 
} 
void showcontext ( ) 
{/* 30 */ showcontext_regmem 
  schar oldsetting  ; 
  integer nn  ; 
  boolean bottomline  ; 
  integer i  ; 
  integer j  ; 
  integer l  ; 
  integer m  ; 
  integer n  ; 
  integer p  ; 
  integer q  ; 
  baseptr = inputptr ; 
  inputstack [ baseptr ] = curinput ; 
  nn = -1 ; 
  bottomline = false ; 
  while ( true ) {
      
    curinput = inputstack [ baseptr ] ; 
    if ( ( curinput .statefield != 0 ) ) 
    if ( ( curinput .namefield > 17 ) || ( baseptr == 0 ) ) 
    bottomline = true ; 
    if ( ( baseptr == inputptr ) || bottomline || ( nn < eqtb [ 12717 ] .cint 
    ) ) 
    {
      if ( ( baseptr == inputptr ) || ( curinput .statefield != 0 ) || ( 
      curinput .indexfield != 3 ) || ( curinput .locfield != 0 ) ) 
      {
	tally = 0 ; 
	oldsetting = selector ; 
	if ( curinput .statefield != 0 ) 
	{
	  if ( curinput .namefield <= 17 ) 
	  if ( ( curinput .namefield == 0 ) ) 
	  if ( baseptr == 0 ) 
	  printnl ( 570 ) ; 
	  else printnl ( 571 ) ; 
	  else {
	      
	    printnl ( 572 ) ; 
	    if ( curinput .namefield == 17 ) 
	    printchar ( 42 ) ; 
	    else printint ( curinput .namefield - 1 ) ; 
	    printchar ( 62 ) ; 
	  } 
	  else {
	      
	    printnl ( 573 ) ; 
	    printint ( line ) ; 
	  } 
	  printchar ( 32 ) ; 
	  {
	    l = tally ; 
	    tally = 0 ; 
	    selector = 20 ; 
	    trickcount = 1000000L ; 
	  } 
	  if ( buffer [ curinput .limitfield ] == eqtb [ 12711 ] .cint ) 
	  j = curinput .limitfield ; 
	  else j = curinput .limitfield + 1 ; 
	  if ( j > 0 ) 
	  {register integer for_end; i = curinput .startfield ; for_end = j - 
	  1 ; if ( i <= for_end) do 
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
	  {case 0 : 
	    printnl ( 574 ) ; 
	    break ; 
	  case 1 : 
	  case 2 : 
	    printnl ( 575 ) ; 
	    break ; 
	  case 3 : 
	    if ( curinput .locfield == 0 ) 
	    printnl ( 576 ) ; 
	    else printnl ( 577 ) ; 
	    break ; 
	  case 4 : 
	    printnl ( 578 ) ; 
	    break ; 
	  case 5 : 
	    {
	      println () ; 
	      printcs ( curinput .namefield ) ; 
	    } 
	    break ; 
	  case 6 : 
	    printnl ( 579 ) ; 
	    break ; 
	  case 7 : 
	    printnl ( 580 ) ; 
	    break ; 
	  case 8 : 
	    printnl ( 581 ) ; 
	    break ; 
	  case 9 : 
	    printnl ( 582 ) ; 
	    break ; 
	  case 10 : 
	    printnl ( 583 ) ; 
	    break ; 
	  case 11 : 
	    printnl ( 584 ) ; 
	    break ; 
	  case 12 : 
	    printnl ( 585 ) ; 
	    break ; 
	  case 13 : 
	    printnl ( 586 ) ; 
	    break ; 
	  case 14 : 
	    printnl ( 587 ) ; 
	    break ; 
	  case 15 : 
	    printnl ( 588 ) ; 
	    break ; 
	    default: 
	    printnl ( 63 ) ; 
	    break ; 
	  } 
	  {
	    l = tally ; 
	    tally = 0 ; 
	    selector = 20 ; 
	    trickcount = 1000000L ; 
	  } 
	  if ( curinput .indexfield < 5 ) 
	  showtokenlist ( curinput .startfield , curinput .locfield , 100000L 
	  ) ; 
	  else showtokenlist ( mem [ curinput .startfield ] .hh .v.RH , 
	  curinput .locfield , 100000L ) ; 
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
	    
	  print ( 275 ) ; 
	  p = l + firstcount - halferrorline + 3 ; 
	  n = halferrorline ; 
	} 
	{register integer for_end; q = p ; for_end = firstcount - 1 ; if ( q 
	<= for_end) do 
	  printchar ( trickbuf [ q % errorline ] ) ; 
	while ( q++ < for_end ) ; } 
	println () ; 
	{register integer for_end; q = 1 ; for_end = n ; if ( q <= for_end) 
	do 
	  printchar ( 32 ) ; 
	while ( q++ < for_end ) ; } 
	if ( m + n <= errorline ) 
	p = firstcount + m ; 
	else p = firstcount + ( errorline - n - 3 ) ; 
	{register integer for_end; q = firstcount ; for_end = p - 1 ; if ( q 
	<= for_end) do 
	  printchar ( trickbuf [ q % errorline ] ) ; 
	while ( q++ < for_end ) ; } 
	if ( m + n > errorline ) 
	print ( 275 ) ; 
	incr ( nn ) ; 
      } 
    } 
    else if ( nn == eqtb [ 12717 ] .cint ) 
    {
      printnl ( 275 ) ; 
      incr ( nn ) ; 
    } 
    if ( bottomline ) 
    goto lab30 ; 
    decr ( baseptr ) ; 
  } 
  lab30: curinput = inputstack [ inputptr ] ; 
} 
void zbegintokenlist ( p , t ) 
halfword p ; 
quarterword t ; 
{begintokenlist_regmem 
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ; 
      if ( inputptr == stacksize ) 
      overflow ( 589 , stacksize ) ; 
    } 
    inputstack [ inputptr ] = curinput ; 
    incr ( inputptr ) ; 
  } 
  curinput .statefield = 0 ; 
  curinput .startfield = p ; 
  curinput .indexfield = t ; 
  if ( t >= 5 ) 
  {
    incr ( mem [ p ] .hh .v.LH ) ; 
    if ( t == 5 ) 
    curinput .limitfield = paramptr ; 
    else {
	
      curinput .locfield = mem [ p ] .hh .v.RH ; 
      if ( eqtb [ 12693 ] .cint > 1 ) 
      {
	begindiagnostic () ; 
	printnl ( 335 ) ; 
	switch ( t ) 
	{case 14 : 
	  printesc ( 348 ) ; 
	  break ; 
	case 15 : 
	  printesc ( 590 ) ; 
	  break ; 
	  default: 
	  printcmdchr ( 72 , t + 10807 ) ; 
	  break ; 
	} 
	print ( 552 ) ; 
	tokenshow ( p ) ; 
	enddiagnostic ( false ) ; 
      } 
    } 
  } 
  else curinput .locfield = p ; 
} 
void endtokenlist ( ) 
{endtokenlist_regmem 
  if ( curinput .indexfield >= 3 ) 
  {
    if ( curinput .indexfield <= 4 ) 
    flushlist ( curinput .startfield ) ; 
    else {
	
      deletetokenref ( curinput .startfield ) ; 
      if ( curinput .indexfield == 5 ) 
      while ( paramptr > curinput .limitfield ) {
	  
	decr ( paramptr ) ; 
	flushlist ( paramstack [ paramptr ] ) ; 
      } 
    } 
  } 
  else if ( curinput .indexfield == 1 ) 
  if ( alignstate > 500000L ) 
  alignstate = 0 ; 
  else fatalerror ( 591 ) ; 
  {
    decr ( inputptr ) ; 
    curinput = inputstack [ inputptr ] ; 
  } 
  {
    if ( interrupt != 0 ) 
    pauseforinstructions () ; 
  } 
} 
void backinput ( ) 
{backinput_regmem 
  halfword p  ; 
  while ( ( curinput .statefield == 0 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ; 
  p = getavail () ; 
  mem [ p ] .hh .v.LH = curtok ; 
  if ( curtok < 768 ) 
  if ( curtok < 512 ) 
  decr ( alignstate ) ; 
  else incr ( alignstate ) ; 
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ; 
      if ( inputptr == stacksize ) 
      overflow ( 589 , stacksize ) ; 
    } 
    inputstack [ inputptr ] = curinput ; 
    incr ( inputptr ) ; 
  } 
  curinput .statefield = 0 ; 
  curinput .startfield = p ; 
  curinput .indexfield = 3 ; 
  curinput .locfield = p ; 
} 
void backerror ( ) 
{backerror_regmem 
  OKtointerrupt = false ; 
  backinput () ; 
  OKtointerrupt = true ; 
  error () ; 
} 
void inserror ( ) 
{inserror_regmem 
  OKtointerrupt = false ; 
  backinput () ; 
  curinput .indexfield = 4 ; 
  OKtointerrupt = true ; 
  error () ; 
} 
void beginfilereading ( ) 
{beginfilereading_regmem 
  if ( inopen == maxinopen ) 
  overflow ( 592 , maxinopen ) ; 
  if ( first == bufsize ) 
  overflow ( 256 , bufsize ) ; 
  incr ( inopen ) ; 
  {
    if ( inputptr > maxinstack ) 
    {
      maxinstack = inputptr ; 
      if ( inputptr == stacksize ) 
      overflow ( 589 , stacksize ) ; 
    } 
    inputstack [ inputptr ] = curinput ; 
    incr ( inputptr ) ; 
  } 
  curinput .indexfield = inopen ; 
  linestack [ curinput .indexfield ] = line ; 
  curinput .startfield = first ; 
  curinput .statefield = 1 ; 
  curinput .namefield = 0 ; 
} 
void endfilereading ( ) 
{endfilereading_regmem 
  first = curinput .startfield ; 
  line = linestack [ curinput .indexfield ] ; 
  if ( curinput .namefield > 17 ) 
  aclose ( inputfile [ curinput .indexfield ] ) ; 
  {
    decr ( inputptr ) ; 
    curinput = inputstack [ inputptr ] ; 
  } 
  decr ( inopen ) ; 
} 
void clearforerrorprompt ( ) 
{clearforerrorprompt_regmem 
  while ( ( curinput .statefield != 0 ) && ( curinput .namefield == 0 ) && ( 
  inputptr > 0 ) && ( curinput .locfield > curinput .limitfield ) ) 
  endfilereading () ; 
  println () ; 
} 
void checkoutervalidity ( ) 
{checkoutervalidity_regmem 
  halfword p  ; 
  halfword q  ; 
  if ( scannerstatus != 0 ) 
  {
    deletionsallowed = false ; 
    if ( curcs != 0 ) 
    {
      if ( ( curinput .statefield == 0 ) || ( curinput .namefield < 1 ) || ( 
      curinput .namefield > 17 ) ) 
      {
	p = getavail () ; 
	mem [ p ] .hh .v.LH = 4095 + curcs ; 
	begintokenlist ( p , 3 ) ; 
      } 
      curcmd = 10 ; 
      curchr = 32 ; 
    } 
    if ( scannerstatus > 1 ) 
    {
      runaway () ; 
      if ( curcs == 0 ) 
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 600 ) ; 
      } 
      else {
	  
	curcs = 0 ; 
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 601 ) ; 
	} 
      } 
      print ( 602 ) ; 
      p = getavail () ; 
      switch ( scannerstatus ) 
      {case 2 : 
	{
	  print ( 566 ) ; 
	  mem [ p ] .hh .v.LH = 637 ; 
	} 
	break ; 
      case 3 : 
	{
	  print ( 608 ) ; 
	  mem [ p ] .hh .v.LH = partoken ; 
	  longstate = 113 ; 
	} 
	break ; 
      case 4 : 
	{
	  print ( 568 ) ; 
	  mem [ p ] .hh .v.LH = 637 ; 
	  q = p ; 
	  p = getavail () ; 
	  mem [ p ] .hh .v.RH = q ; 
	  mem [ p ] .hh .v.LH = 14110 ; 
	  alignstate = -1000000L ; 
	} 
	break ; 
      case 5 : 
	{
	  print ( 569 ) ; 
	  mem [ p ] .hh .v.LH = 637 ; 
	} 
	break ; 
      } 
      begintokenlist ( p , 4 ) ; 
      print ( 603 ) ; 
      sprintcs ( warningindex ) ; 
      {
	helpptr = 4 ; 
	helpline [ 3 ] = 604 ; 
	helpline [ 2 ] = 605 ; 
	helpline [ 1 ] = 606 ; 
	helpline [ 0 ] = 607 ; 
      } 
      error () ; 
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 594 ) ; 
      } 
      printcmdchr ( 105 , curif ) ; 
      print ( 595 ) ; 
      printint ( skipline ) ; 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 596 ; 
	helpline [ 1 ] = 597 ; 
	helpline [ 0 ] = 598 ; 
      } 
      if ( curcs != 0 ) 
      curcs = 0 ; 
      else helpline [ 2 ] = 599 ; 
      curtok = 14113 ; 
      inserror () ; 
    } 
    deletionsallowed = true ; 
  } 
} 
void getnext ( ) 
{/* 20 25 21 26 40 10 */ getnext_regmem 
  integer k  ; 
  halfword t  ; 
  schar cat  ; 
  ASCIIcode c, cc  ; 
  schar d  ; 
  lab20: curcs = 0 ; 
  if ( curinput .statefield != 0 ) 
  {
    lab25: if ( curinput .locfield <= curinput .limitfield ) 
    {
      curchr = buffer [ curinput .locfield ] ; 
      incr ( curinput .locfield ) ; 
      lab21: curcmd = eqtb [ 11383 + curchr ] .hh .v.RH ; 
      switch ( curinput .statefield + curcmd ) 
      {case 10 : 
      case 26 : 
      case 42 : 
      case 27 : 
      case 43 : 
	goto lab25 ; 
	break ; 
      case 1 : 
      case 17 : 
      case 33 : 
	{
	  if ( curinput .locfield > curinput .limitfield ) 
	  curcs = 513 ; 
	  else {
	      
	    lab26: k = curinput .locfield ; 
	    curchr = buffer [ k ] ; 
	    cat = eqtb [ 11383 + curchr ] .hh .v.RH ; 
	    incr ( k ) ; 
	    if ( cat == 11 ) 
	    curinput .statefield = 17 ; 
	    else if ( cat == 10 ) 
	    curinput .statefield = 17 ; 
	    else curinput .statefield = 1 ; 
	    if ( ( cat == 11 ) && ( k <= curinput .limitfield ) ) 
	    {
	      do {
		  curchr = buffer [ k ] ; 
		cat = eqtb [ 11383 + curchr ] .hh .v.RH ; 
		incr ( k ) ; 
	      } while ( ! ( ( cat != 11 ) || ( k > curinput .limitfield ) ) ) 
	      ; 
	      {
		if ( buffer [ k ] == curchr ) 
		if ( cat == 7 ) 
		if ( k < curinput .limitfield ) 
		{
		  c = buffer [ k + 1 ] ; 
		  if ( c < 128 ) 
		  {
		    d = 2 ; 
		    if ( ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 97 ) && 
		    ( c <= 102 ) ) ) ) 
		    if ( k + 2 <= curinput .limitfield ) 
		    {
		      cc = buffer [ k + 2 ] ; 
		      if ( ( ( ( cc >= 48 ) && ( cc <= 57 ) ) || ( ( cc >= 97 
		      ) && ( cc <= 102 ) ) ) ) 
		      incr ( d ) ; 
		    } 
		    if ( d > 2 ) 
		    {
		      if ( c <= 57 ) 
		      curchr = c - 48 ; 
		      else curchr = c - 87 ; 
		      if ( cc <= 57 ) 
		      curchr = 16 * curchr + cc - 48 ; 
		      else curchr = 16 * curchr + cc - 87 ; 
		      buffer [ k - 1 ] = curchr ; 
		    } 
		    else if ( c < 64 ) 
		    buffer [ k - 1 ] = c + 64 ; 
		    else buffer [ k - 1 ] = c - 64 ; 
		    curinput .limitfield = curinput .limitfield - d ; 
		    first = first - d ; 
		    while ( k <= curinput .limitfield ) {
			
		      buffer [ k ] = buffer [ k + d ] ; 
		      incr ( k ) ; 
		    } 
		    goto lab26 ; 
		  } 
		} 
	      } 
	      if ( cat != 11 ) 
	      decr ( k ) ; 
	      if ( k > curinput .locfield + 1 ) 
	      {
		curcs = idlookup ( curinput .locfield , k - curinput .locfield 
		) ; 
		curinput .locfield = k ; 
		goto lab40 ; 
	      } 
	    } 
	    else {
		
	      if ( buffer [ k ] == curchr ) 
	      if ( cat == 7 ) 
	      if ( k < curinput .limitfield ) 
	      {
		c = buffer [ k + 1 ] ; 
		if ( c < 128 ) 
		{
		  d = 2 ; 
		  if ( ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 97 ) && ( 
		  c <= 102 ) ) ) ) 
		  if ( k + 2 <= curinput .limitfield ) 
		  {
		    cc = buffer [ k + 2 ] ; 
		    if ( ( ( ( cc >= 48 ) && ( cc <= 57 ) ) || ( ( cc >= 97 ) 
		    && ( cc <= 102 ) ) ) ) 
		    incr ( d ) ; 
		  } 
		  if ( d > 2 ) 
		  {
		    if ( c <= 57 ) 
		    curchr = c - 48 ; 
		    else curchr = c - 87 ; 
		    if ( cc <= 57 ) 
		    curchr = 16 * curchr + cc - 48 ; 
		    else curchr = 16 * curchr + cc - 87 ; 
		    buffer [ k - 1 ] = curchr ; 
		  } 
		  else if ( c < 64 ) 
		  buffer [ k - 1 ] = c + 64 ; 
		  else buffer [ k - 1 ] = c - 64 ; 
		  curinput .limitfield = curinput .limitfield - d ; 
		  first = first - d ; 
		  while ( k <= curinput .limitfield ) {
		      
		    buffer [ k ] = buffer [ k + d ] ; 
		    incr ( k ) ; 
		  } 
		  goto lab26 ; 
		} 
	      } 
	    } 
	    curcs = 257 + buffer [ curinput .locfield ] ; 
	    incr ( curinput .locfield ) ; 
	  } 
	  lab40: curcmd = eqtb [ curcs ] .hh.b0 ; 
	  curchr = eqtb [ curcs ] .hh .v.RH ; 
	  if ( curcmd >= 113 ) 
	  checkoutervalidity () ; 
	} 
	break ; 
      case 14 : 
      case 30 : 
      case 46 : 
	{
	  curcs = curchr + 1 ; 
	  curcmd = eqtb [ curcs ] .hh.b0 ; 
	  curchr = eqtb [ curcs ] .hh .v.RH ; 
	  curinput .statefield = 1 ; 
	  if ( curcmd >= 113 ) 
	  checkoutervalidity () ; 
	} 
	break ; 
      case 8 : 
      case 24 : 
      case 40 : 
	{
	  if ( curchr == buffer [ curinput .locfield ] ) 
	  if ( curinput .locfield < curinput .limitfield ) 
	  {
	    c = buffer [ curinput .locfield + 1 ] ; 
	    if ( c < 128 ) 
	    {
	      curinput .locfield = curinput .locfield + 2 ; 
	      if ( ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 97 ) && ( c <= 
	      102 ) ) ) ) 
	      if ( curinput .locfield <= curinput .limitfield ) 
	      {
		cc = buffer [ curinput .locfield ] ; 
		if ( ( ( ( cc >= 48 ) && ( cc <= 57 ) ) || ( ( cc >= 97 ) && ( 
		cc <= 102 ) ) ) ) 
		{
		  incr ( curinput .locfield ) ; 
		  if ( c <= 57 ) 
		  curchr = c - 48 ; 
		  else curchr = c - 87 ; 
		  if ( cc <= 57 ) 
		  curchr = 16 * curchr + cc - 48 ; 
		  else curchr = 16 * curchr + cc - 87 ; 
		  goto lab21 ; 
		} 
	      } 
	      if ( c < 64 ) 
	      curchr = c + 64 ; 
	      else curchr = c - 64 ; 
	      goto lab21 ; 
	    } 
	  } 
	  curinput .statefield = 1 ; 
	} 
	break ; 
      case 16 : 
      case 32 : 
      case 48 : 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 609 ) ; 
	  } 
	  {
	    helpptr = 2 ; 
	    helpline [ 1 ] = 610 ; 
	    helpline [ 0 ] = 611 ; 
	  } 
	  deletionsallowed = false ; 
	  error () ; 
	  deletionsallowed = true ; 
	  goto lab20 ; 
	} 
	break ; 
      case 11 : 
	{
	  curinput .statefield = 17 ; 
	  curchr = 32 ; 
	} 
	break ; 
      case 6 : 
	{
	  curinput .locfield = curinput .limitfield + 1 ; 
	  curcmd = 10 ; 
	  curchr = 32 ; 
	} 
	break ; 
      case 22 : 
      case 15 : 
      case 31 : 
      case 47 : 
	{
	  curinput .locfield = curinput .limitfield + 1 ; 
	  goto lab25 ; 
	} 
	break ; 
      case 38 : 
	{
	  curinput .locfield = curinput .limitfield + 1 ; 
	  curcs = parloc ; 
	  curcmd = eqtb [ curcs ] .hh.b0 ; 
	  curchr = eqtb [ curcs ] .hh .v.RH ; 
	  if ( curcmd >= 113 ) 
	  checkoutervalidity () ; 
	} 
	break ; 
      case 2 : 
	incr ( alignstate ) ; 
	break ; 
      case 18 : 
      case 34 : 
	{
	  curinput .statefield = 1 ; 
	  incr ( alignstate ) ; 
	} 
	break ; 
      case 3 : 
	decr ( alignstate ) ; 
	break ; 
      case 19 : 
      case 35 : 
	{
	  curinput .statefield = 1 ; 
	  decr ( alignstate ) ; 
	} 
	break ; 
      case 20 : 
      case 21 : 
      case 23 : 
      case 25 : 
      case 28 : 
      case 29 : 
      case 36 : 
      case 37 : 
      case 39 : 
      case 41 : 
      case 44 : 
      case 45 : 
	curinput .statefield = 1 ; 
	break ; 
	default: 
	; 
	break ; 
      } 
    } 
    else {
	
      curinput .statefield = 33 ; 
      if ( curinput .namefield > 17 ) 
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
	  checkoutervalidity () ; 
	  goto lab20 ; 
	} 
	if ( ( eqtb [ 12711 ] .cint < 0 ) || ( eqtb [ 12711 ] .cint > 255 ) ) 
	decr ( curinput .limitfield ) ; 
	else buffer [ curinput .limitfield ] = eqtb [ 12711 ] .cint ; 
	first = curinput .limitfield + 1 ; 
	curinput .locfield = curinput .startfield ; 
      } 
      else {
	  
	if ( ! ( curinput .namefield == 0 ) ) 
	{
	  curcmd = 0 ; 
	  curchr = 0 ; 
	  return ; 
	} 
	if ( inputptr > 0 ) 
	{
	  endfilereading () ; 
	  goto lab20 ; 
	} 
	if ( selector < 18 ) 
	openlogfile () ; 
	if ( interaction > 1 ) 
	{
	  if ( ( eqtb [ 12711 ] .cint < 0 ) || ( eqtb [ 12711 ] .cint > 255 ) 
	  ) 
	  incr ( curinput .limitfield ) ; 
	  if ( curinput .limitfield == curinput .startfield ) 
	  printnl ( 612 ) ; 
	  println () ; 
	  first = curinput .startfield ; 
	  {
	    ; 
	    print ( 42 ) ; 
	    terminput () ; 
	  } 
	  curinput .limitfield = last ; 
	  if ( ( eqtb [ 12711 ] .cint < 0 ) || ( eqtb [ 12711 ] .cint > 255 ) 
	  ) 
	  decr ( curinput .limitfield ) ; 
	  else buffer [ curinput .limitfield ] = eqtb [ 12711 ] .cint ; 
	  first = curinput .limitfield + 1 ; 
	  curinput .locfield = curinput .startfield ; 
	} 
	else fatalerror ( 613 ) ; 
      } 
      {
	if ( interrupt != 0 ) 
	pauseforinstructions () ; 
      } 
      goto lab25 ; 
    } 
  } 
  else if ( curinput .locfield != 0 ) 
  {
    t = mem [ curinput .locfield ] .hh .v.LH ; 
    curinput .locfield = mem [ curinput .locfield ] .hh .v.RH ; 
    if ( t >= 4095 ) 
    {
      curcs = t - 4095 ; 
      curcmd = eqtb [ curcs ] .hh.b0 ; 
      curchr = eqtb [ curcs ] .hh .v.RH ; 
      if ( curcmd >= 113 ) 
      if ( curcmd == 116 ) 
      {
	curcs = mem [ curinput .locfield ] .hh .v.LH - 4095 ; 
	curinput .locfield = 0 ; 
	curcmd = eqtb [ curcs ] .hh.b0 ; 
	curchr = eqtb [ curcs ] .hh .v.RH ; 
	if ( curcmd > 100 ) 
	{
	  curcmd = 0 ; 
	  curchr = 257 ; 
	} 
      } 
      else checkoutervalidity () ; 
    } 
    else {
	
      curcmd = t / 256 ; 
      curchr = t % 256 ; 
      switch ( curcmd ) 
      {case 1 : 
	incr ( alignstate ) ; 
	break ; 
      case 2 : 
	decr ( alignstate ) ; 
	break ; 
      case 5 : 
	{
	  begintokenlist ( paramstack [ curinput .limitfield + curchr - 1 ] , 
	  0 ) ; 
	  goto lab20 ; 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
    } 
  } 
  else {
      
    endtokenlist () ; 
    goto lab20 ; 
  } 
  if ( curcmd <= 5 ) 
  if ( curcmd >= 4 ) 
  if ( alignstate == 0 ) 
  {
    if ( scannerstatus == 4 ) 
    fatalerror ( 591 ) ; 
    curcmd = mem [ curalign + 5 ] .hh .v.LH ; 
    mem [ curalign + 5 ] .hh .v.LH = curchr ; 
    if ( curcmd == 63 ) 
    begintokenlist ( memtop - 10 , 2 ) ; 
    else begintokenlist ( mem [ curalign + 2 ] .cint , 2 ) ; 
    alignstate = 1000000L ; 
    goto lab20 ; 
  } 
} 
void firmuptheline ( ) 
{firmuptheline_regmem 
  integer k  ; 
  curinput .limitfield = last ; 
  if ( eqtb [ 12691 ] .cint > 0 ) 
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
      print ( 614 ) ; 
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
void gettoken ( ) 
{gettoken_regmem 
  nonewcontrolsequence = false ; 
  getnext () ; 
  nonewcontrolsequence = true ; 
  if ( curcs == 0 ) 
  curtok = ( curcmd * 256 ) + curchr ; 
  else curtok = 4095 + curcs ; 
} 
void macrocall ( ) 
{/* 10 22 30 31 40 */ macrocall_regmem 
  halfword r  ; 
  halfword p  ; 
  halfword q  ; 
  halfword s  ; 
  halfword t  ; 
  halfword u, v  ; 
  halfword rbraceptr  ; 
  smallnumber n  ; 
  halfword unbalance  ; 
  halfword m  ; 
  halfword refcount  ; 
  smallnumber savescannerstatus  ; 
  halfword savewarningindex  ; 
  ASCIIcode matchchr  ; 
  savescannerstatus = scannerstatus ; 
  savewarningindex = warningindex ; 
  warningindex = curcs ; 
  refcount = curchr ; 
  r = mem [ refcount ] .hh .v.RH ; 
  n = 0 ; 
  if ( eqtb [ 12693 ] .cint > 0 ) 
  {
    begindiagnostic () ; 
    println () ; 
    printcs ( warningindex ) ; 
    tokenshow ( refcount ) ; 
    enddiagnostic ( false ) ; 
  } 
  if ( mem [ r ] .hh .v.LH != 3584 ) 
  {
    scannerstatus = 3 ; 
    unbalance = 0 ; 
    longstate = eqtb [ curcs ] .hh.b0 ; 
    if ( longstate >= 113 ) 
    longstate = longstate - 2 ; 
    do {
	mem [ memtop - 3 ] .hh .v.RH = 0 ; 
      if ( ( mem [ r ] .hh .v.LH > 3583 ) || ( mem [ r ] .hh .v.LH < 3328 ) ) 
      s = 0 ; 
      else {
	  
	matchchr = mem [ r ] .hh .v.LH - 3328 ; 
	s = mem [ r ] .hh .v.RH ; 
	r = s ; 
	p = memtop - 3 ; 
	m = 0 ; 
      } 
      lab22: gettoken () ; 
      if ( curtok == mem [ r ] .hh .v.LH ) 
      {
	r = mem [ r ] .hh .v.RH ; 
	if ( ( mem [ r ] .hh .v.LH >= 3328 ) && ( mem [ r ] .hh .v.LH <= 3584 
	) ) 
	{
	  if ( curtok < 512 ) 
	  decr ( alignstate ) ; 
	  goto lab40 ; 
	} 
	else goto lab22 ; 
      } 
      if ( s != r ) 
      if ( s == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 646 ) ; 
	} 
	sprintcs ( warningindex ) ; 
	print ( 647 ) ; 
	{
	  helpptr = 4 ; 
	  helpline [ 3 ] = 648 ; 
	  helpline [ 2 ] = 649 ; 
	  helpline [ 1 ] = 650 ; 
	  helpline [ 0 ] = 651 ; 
	} 
	error () ; 
	goto lab10 ; 
      } 
      else {
	  
	t = s ; 
	do {
	    { 
	    q = getavail () ; 
	    mem [ p ] .hh .v.RH = q ; 
	    mem [ q ] .hh .v.LH = mem [ t ] .hh .v.LH ; 
	    p = q ; 
	  } 
	  incr ( m ) ; 
	  u = mem [ t ] .hh .v.RH ; 
	  v = s ; 
	  while ( true ) {
	      
	    if ( u == r ) 
	    if ( curtok != mem [ v ] .hh .v.LH ) 
	    goto lab30 ; 
	    else {
		
	      r = mem [ v ] .hh .v.RH ; 
	      goto lab22 ; 
	    } 
	    if ( mem [ u ] .hh .v.LH != mem [ v ] .hh .v.LH ) 
	    goto lab30 ; 
	    u = mem [ u ] .hh .v.RH ; 
	    v = mem [ v ] .hh .v.RH ; 
	  } 
	  lab30: t = mem [ t ] .hh .v.RH ; 
	} while ( ! ( t == r ) ) ; 
	r = s ; 
      } 
      if ( curtok == partoken ) 
      if ( longstate != 112 ) 
      {
	if ( longstate == 111 ) 
	{
	  runaway () ; 
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 641 ) ; 
	  } 
	  sprintcs ( warningindex ) ; 
	  print ( 642 ) ; 
	  {
	    helpptr = 3 ; 
	    helpline [ 2 ] = 643 ; 
	    helpline [ 1 ] = 644 ; 
	    helpline [ 0 ] = 645 ; 
	  } 
	  backerror () ; 
	} 
	pstack [ n ] = mem [ memtop - 3 ] .hh .v.RH ; 
	alignstate = alignstate - unbalance ; 
	{register integer for_end; m = 0 ; for_end = n ; if ( m <= for_end) 
	do 
	  flushlist ( pstack [ m ] ) ; 
	while ( m++ < for_end ) ; } 
	goto lab10 ; 
      } 
      if ( curtok < 768 ) 
      if ( curtok < 512 ) 
      {
	unbalance = 1 ; 
	while ( true ) {
	    
	  {
	    {
	      q = avail ; 
	      if ( q == 0 ) 
	      q = getavail () ; 
	      else {
		  
		avail = mem [ q ] .hh .v.RH ; 
		mem [ q ] .hh .v.RH = 0 ; 
	;
#ifdef STAT
		incr ( dynused ) ; 
#endif /* STAT */
	      } 
	    } 
	    mem [ p ] .hh .v.RH = q ; 
	    mem [ q ] .hh .v.LH = curtok ; 
	    p = q ; 
	  } 
	  gettoken () ; 
	  if ( curtok == partoken ) 
	  if ( longstate != 112 ) 
	  {
	    if ( longstate == 111 ) 
	    {
	      runaway () ; 
	      {
		if ( interaction == 3 ) 
		; 
		printnl ( 262 ) ; 
		print ( 641 ) ; 
	      } 
	      sprintcs ( warningindex ) ; 
	      print ( 642 ) ; 
	      {
		helpptr = 3 ; 
		helpline [ 2 ] = 643 ; 
		helpline [ 1 ] = 644 ; 
		helpline [ 0 ] = 645 ; 
	      } 
	      backerror () ; 
	    } 
	    pstack [ n ] = mem [ memtop - 3 ] .hh .v.RH ; 
	    alignstate = alignstate - unbalance ; 
	    {register integer for_end; m = 0 ; for_end = n ; if ( m <= 
	    for_end) do 
	      flushlist ( pstack [ m ] ) ; 
	    while ( m++ < for_end ) ; } 
	    goto lab10 ; 
	  } 
	  if ( curtok < 768 ) 
	  if ( curtok < 512 ) 
	  incr ( unbalance ) ; 
	  else {
	      
	    decr ( unbalance ) ; 
	    if ( unbalance == 0 ) 
	    goto lab31 ; 
	  } 
	} 
	lab31: rbraceptr = p ; 
	{
	  q = getavail () ; 
	  mem [ p ] .hh .v.RH = q ; 
	  mem [ q ] .hh .v.LH = curtok ; 
	  p = q ; 
	} 
      } 
      else {
	  
	backinput () ; 
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 633 ) ; 
	} 
	sprintcs ( warningindex ) ; 
	print ( 634 ) ; 
	{
	  helpptr = 6 ; 
	  helpline [ 5 ] = 635 ; 
	  helpline [ 4 ] = 636 ; 
	  helpline [ 3 ] = 637 ; 
	  helpline [ 2 ] = 638 ; 
	  helpline [ 1 ] = 639 ; 
	  helpline [ 0 ] = 640 ; 
	} 
	incr ( alignstate ) ; 
	longstate = 111 ; 
	curtok = partoken ; 
	inserror () ; 
      } 
      else {
	  
	if ( curtok == 2592 ) 
	if ( mem [ r ] .hh .v.LH <= 3584 ) 
	if ( mem [ r ] .hh .v.LH >= 3328 ) 
	goto lab22 ; 
	{
	  q = getavail () ; 
	  mem [ p ] .hh .v.RH = q ; 
	  mem [ q ] .hh .v.LH = curtok ; 
	  p = q ; 
	} 
      } 
      incr ( m ) ; 
      if ( mem [ r ] .hh .v.LH > 3584 ) 
      goto lab22 ; 
      if ( mem [ r ] .hh .v.LH < 3328 ) 
      goto lab22 ; 
      lab40: if ( s != 0 ) 
      {
	if ( ( m == 1 ) && ( mem [ p ] .hh .v.LH < 768 ) && ( p != memtop - 3 
	) ) 
	{
	  mem [ rbraceptr ] .hh .v.RH = 0 ; 
	  {
	    mem [ p ] .hh .v.RH = avail ; 
	    avail = p ; 
	;
#ifdef STAT
	    decr ( dynused ) ; 
#endif /* STAT */
	  } 
	  p = mem [ memtop - 3 ] .hh .v.RH ; 
	  pstack [ n ] = mem [ p ] .hh .v.RH ; 
	  {
	    mem [ p ] .hh .v.RH = avail ; 
	    avail = p ; 
	;
#ifdef STAT
	    decr ( dynused ) ; 
#endif /* STAT */
	  } 
	} 
	else pstack [ n ] = mem [ memtop - 3 ] .hh .v.RH ; 
	incr ( n ) ; 
	if ( eqtb [ 12693 ] .cint > 0 ) 
	{
	  begindiagnostic () ; 
	  printnl ( matchchr ) ; 
	  printint ( n ) ; 
	  print ( 652 ) ; 
	  showtokenlist ( pstack [ n - 1 ] , 0 , 1000 ) ; 
	  enddiagnostic ( false ) ; 
	} 
      } 
    } while ( ! ( mem [ r ] .hh .v.LH == 3584 ) ) ; 
  } 
  while ( ( curinput .statefield == 0 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ; 
  begintokenlist ( refcount , 5 ) ; 
  curinput .namefield = warningindex ; 
  curinput .locfield = mem [ r ] .hh .v.RH ; 
  if ( n > 0 ) 
  {
    if ( paramptr + n > maxparamstack ) 
    {
      maxparamstack = paramptr + n ; 
      if ( maxparamstack > paramsize ) 
      overflow ( 632 , paramsize ) ; 
    } 
    {register integer for_end; m = 0 ; for_end = n - 1 ; if ( m <= for_end) 
    do 
      paramstack [ paramptr + m ] = pstack [ m ] ; 
    while ( m++ < for_end ) ; } 
    paramptr = paramptr + n ; 
  } 
  lab10: scannerstatus = savescannerstatus ; 
  warningindex = savewarningindex ; 
} 
void insertrelax ( ) 
{insertrelax_regmem 
  curtok = 4095 + curcs ; 
  backinput () ; 
  curtok = 14116 ; 
  backinput () ; 
  curinput .indexfield = 4 ; 
} 
void expand ( ) 
{expand_regmem 
  halfword t  ; 
  halfword p, q, r  ; 
  integer j  ; 
  integer cvbackup  ; 
  smallnumber cvlbackup, radixbackup, cobackup  ; 
  halfword backupbackup  ; 
  smallnumber savescannerstatus  ; 
  cvbackup = curval ; 
  cvlbackup = curvallevel ; 
  radixbackup = radix ; 
  cobackup = curorder ; 
  backupbackup = mem [ memtop - 13 ] .hh .v.RH ; 
  if ( curcmd < 111 ) 
  {
    if ( eqtb [ 12699 ] .cint > 1 ) 
    showcurcmdchr () ; 
    switch ( curcmd ) 
    {case 110 : 
      {
	if ( curmark [ curchr ] != 0 ) 
	begintokenlist ( curmark [ curchr ] , 14 ) ; 
      } 
      break ; 
    case 102 : 
      {
	gettoken () ; 
	t = curtok ; 
	gettoken () ; 
	if ( curcmd > 100 ) 
	expand () ; 
	else backinput () ; 
	curtok = t ; 
	backinput () ; 
      } 
      break ; 
    case 103 : 
      {
	savescannerstatus = scannerstatus ; 
	scannerstatus = 0 ; 
	gettoken () ; 
	scannerstatus = savescannerstatus ; 
	t = curtok ; 
	backinput () ; 
	if ( t >= 4095 ) 
	{
	  p = getavail () ; 
	  mem [ p ] .hh .v.LH = 14118 ; 
	  mem [ p ] .hh .v.RH = curinput .locfield ; 
	  curinput .startfield = p ; 
	  curinput .locfield = p ; 
	} 
      } 
      break ; 
    case 107 : 
      {
	r = getavail () ; 
	p = r ; 
	do {
	    getxtoken () ; 
	  if ( curcs == 0 ) 
	  {
	    q = getavail () ; 
	    mem [ p ] .hh .v.RH = q ; 
	    mem [ q ] .hh .v.LH = curtok ; 
	    p = q ; 
	  } 
	} while ( ! ( curcs != 0 ) ) ; 
	if ( curcmd != 67 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 621 ) ; 
	  } 
	  printesc ( 501 ) ; 
	  print ( 622 ) ; 
	  {
	    helpptr = 2 ; 
	    helpline [ 1 ] = 623 ; 
	    helpline [ 0 ] = 624 ; 
	  } 
	  backerror () ; 
	} 
	j = first ; 
	p = mem [ r ] .hh .v.RH ; 
	while ( p != 0 ) {
	    
	  if ( j >= maxbufstack ) 
	  {
	    maxbufstack = j + 1 ; 
	    if ( maxbufstack == bufsize ) 
	    overflow ( 256 , bufsize ) ; 
	  } 
	  buffer [ j ] = mem [ p ] .hh .v.LH % 256 ; 
	  incr ( j ) ; 
	  p = mem [ p ] .hh .v.RH ; 
	} 
	if ( j > first + 1 ) 
	{
	  nonewcontrolsequence = false ; 
	  curcs = idlookup ( first , j - first ) ; 
	  nonewcontrolsequence = true ; 
	} 
	else if ( j == first ) 
	curcs = 513 ; 
	else curcs = 257 + buffer [ first ] ; 
	flushlist ( r ) ; 
	if ( eqtb [ curcs ] .hh.b0 == 101 ) 
	{
	  eqdefine ( curcs , 0 , 256 ) ; 
	} 
	curtok = curcs + 4095 ; 
	backinput () ; 
      } 
      break ; 
    case 108 : 
      convtoks () ; 
      break ; 
    case 109 : 
      insthetoks () ; 
      break ; 
    case 105 : 
      conditional () ; 
      break ; 
    case 106 : 
      if ( curchr > iflimit ) 
      if ( iflimit == 1 ) 
      insertrelax () ; 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 772 ) ; 
	} 
	printcmdchr ( 106 , curchr ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 773 ; 
	} 
	error () ; 
      } 
      else {
	  
	while ( curchr != 2 ) passtext () ; 
	{
	  p = condptr ; 
	  ifline = mem [ p + 1 ] .cint ; 
	  curif = mem [ p ] .hh.b1 ; 
	  iflimit = mem [ p ] .hh.b0 ; 
	  condptr = mem [ p ] .hh .v.RH ; 
	  freenode ( p , 2 ) ; 
	} 
      } 
      break ; 
    case 104 : 
      if ( curchr > 0 ) 
      forceeof = true ; 
      else if ( nameinprogress ) 
      insertrelax () ; 
      else startinput () ; 
      break ; 
      default: 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 615 ) ; 
	} 
	{
	  helpptr = 5 ; 
	  helpline [ 4 ] = 616 ; 
	  helpline [ 3 ] = 617 ; 
	  helpline [ 2 ] = 618 ; 
	  helpline [ 1 ] = 619 ; 
	  helpline [ 0 ] = 620 ; 
	} 
	error () ; 
      } 
      break ; 
    } 
  } 
  else if ( curcmd < 115 ) 
  macrocall () ; 
  else {
      
    curtok = 14115 ; 
    backinput () ; 
  } 
  curval = cvbackup ; 
  curvallevel = cvlbackup ; 
  radix = radixbackup ; 
  curorder = cobackup ; 
  mem [ memtop - 13 ] .hh .v.RH = backupbackup ; 
} 
void getxtoken ( ) 
{/* 20 30 */ getxtoken_regmem 
  lab20: getnext () ; 
  if ( curcmd <= 100 ) 
  goto lab30 ; 
  if ( curcmd >= 111 ) 
  if ( curcmd < 115 ) 
  macrocall () ; 
  else {
      
    curcs = 10020 ; 
    curcmd = 9 ; 
    goto lab30 ; 
  } 
  else expand () ; 
  goto lab20 ; 
  lab30: if ( curcs == 0 ) 
  curtok = ( curcmd * 256 ) + curchr ; 
  else curtok = 4095 + curcs ; 
} 
void xtoken ( ) 
{xtoken_regmem 
  while ( curcmd > 100 ) {
      
    expand () ; 
    getnext () ; 
  } 
  if ( curcs == 0 ) 
  curtok = ( curcmd * 256 ) + curchr ; 
  else curtok = 4095 + curcs ; 
} 
void scanleftbrace ( ) 
{scanleftbrace_regmem 
  do {
      getxtoken () ; 
  } while ( ! ( ( curcmd != 10 ) && ( curcmd != 0 ) ) ) ; 
  if ( curcmd != 1 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 653 ) ; 
    } 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 654 ; 
      helpline [ 2 ] = 655 ; 
      helpline [ 1 ] = 656 ; 
      helpline [ 0 ] = 657 ; 
    } 
    backerror () ; 
    curtok = 379 ; 
    curcmd = 1 ; 
    curchr = 123 ; 
    incr ( alignstate ) ; 
  } 
} 
void scanoptionalequals ( ) 
{scanoptionalequals_regmem 
  do {
      getxtoken () ; 
  } while ( ! ( curcmd != 10 ) ) ; 
  if ( curtok != 3133 ) 
  backinput () ; 
} 
boolean zscankeyword ( s ) 
strnumber s ; 
{/* 10 */ register boolean Result; scankeyword_regmem 
  halfword p  ; 
  halfword q  ; 
  poolpointer k  ; 
  p = memtop - 13 ; 
  mem [ p ] .hh .v.RH = 0 ; 
  k = strstart [ s ] ; 
  while ( k < strstart [ s + 1 ] ) {
      
    getxtoken () ; 
    if ( ( curcs == 0 ) && ( ( curchr == strpool [ k ] ) || ( curchr == 
    strpool [ k ] - 32 ) ) ) 
    {
      {
	q = getavail () ; 
	mem [ p ] .hh .v.RH = q ; 
	mem [ q ] .hh .v.LH = curtok ; 
	p = q ; 
      } 
      incr ( k ) ; 
    } 
    else if ( ( curcmd != 10 ) || ( p != memtop - 13 ) ) 
    {
      backinput () ; 
      if ( p != memtop - 13 ) 
      begintokenlist ( mem [ memtop - 13 ] .hh .v.RH , 3 ) ; 
      Result = false ; 
      return(Result) ; 
    } 
  } 
  flushlist ( mem [ memtop - 13 ] .hh .v.RH ) ; 
  Result = true ; 
  return(Result) ; 
} 
void muerror ( ) 
{muerror_regmem 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 658 ) ; 
  } 
  {
    helpptr = 1 ; 
    helpline [ 0 ] = 659 ; 
  } 
  error () ; 
} 
void scaneightbitint ( ) 
{scaneightbitint_regmem 
  scanint () ; 
  if ( ( curval < 0 ) || ( curval > 255 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 683 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 684 ; 
      helpline [ 0 ] = 685 ; 
    } 
    interror ( curval ) ; 
    curval = 0 ; 
  } 
} 
void scancharnum ( ) 
{scancharnum_regmem 
  scanint () ; 
  if ( ( curval < 0 ) || ( curval > 255 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 686 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 687 ; 
      helpline [ 0 ] = 685 ; 
    } 
    interror ( curval ) ; 
    curval = 0 ; 
  } 
} 
void scanfourbitint ( ) 
{scanfourbitint_regmem 
  scanint () ; 
  if ( ( curval < 0 ) || ( curval > 15 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 688 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 689 ; 
      helpline [ 0 ] = 685 ; 
    } 
    interror ( curval ) ; 
    curval = 0 ; 
  } 
} 
void scanfifteenbitint ( ) 
{scanfifteenbitint_regmem 
  scanint () ; 
  if ( ( curval < 0 ) || ( curval > 32767 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 690 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 691 ; 
      helpline [ 0 ] = 685 ; 
    } 
    interror ( curval ) ; 
    curval = 0 ; 
  } 
} 
void scantwentysevenbitint ( ) 
{scantwentysevenbitint_regmem 
  scanint () ; 
  if ( ( curval < 0 ) || ( curval > 134217727L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 692 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 693 ; 
      helpline [ 0 ] = 685 ; 
    } 
    interror ( curval ) ; 
    curval = 0 ; 
  } 
} 
void scanfontident ( ) 
{scanfontident_regmem 
  internalfontnumber f  ; 
  halfword m  ; 
  do {
      getxtoken () ; 
  } while ( ! ( curcmd != 10 ) ) ; 
  if ( curcmd == 88 ) 
  f = eqtb [ 11334 ] .hh .v.RH ; 
  else if ( curcmd == 87 ) 
  f = curchr ; 
  else if ( curcmd == 86 ) 
  {
    m = curchr ; 
    scanfourbitint () ; 
    f = eqtb [ m + curval ] .hh .v.RH ; 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 810 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 811 ; 
      helpline [ 0 ] = 812 ; 
    } 
    backerror () ; 
    f = 0 ; 
  } 
  curval = f ; 
} 
void zfindfontdimen ( writing ) 
boolean writing ; 
{findfontdimen_regmem 
  internalfontnumber f  ; 
  integer n  ; 
  scanint () ; 
  n = curval ; 
  scanfontident () ; 
  f = curval ; 
  if ( n <= 0 ) 
  curval = fmemptr ; 
  else {
      
    if ( writing && ( n <= 4 ) && ( n >= 2 ) && ( fontglue [ f ] != 0 ) ) 
    {
      deleteglueref ( fontglue [ f ] ) ; 
      fontglue [ f ] = 0 ; 
    } 
    if ( n > fontparams [ f ] ) 
    if ( f < fontptr ) 
    curval = fmemptr ; 
    else {
	
      do {
	  if ( fmemptr == fontmemsize ) 
	overflow ( 817 , fontmemsize ) ; 
	fontinfo [ fmemptr ] .cint = 0 ; 
	incr ( fmemptr ) ; 
	incr ( fontparams [ f ] ) ; 
      } while ( ! ( n == fontparams [ f ] ) ) ; 
      curval = fmemptr - 1 ; 
    } 
    else curval = n + parambase [ f ] ; 
  } 
  if ( curval == fmemptr ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 795 ) ; 
    } 
    printesc ( hash [ 10024 + f ] .v.RH ) ; 
    print ( 813 ) ; 
    printint ( fontparams [ f ] ) ; 
    print ( 814 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 815 ; 
      helpline [ 0 ] = 816 ; 
    } 
    error () ; 
  } 
} 
void zscansomethinginternal ( level , negative ) 
smallnumber level ; 
boolean negative ; 
{scansomethinginternal_regmem 
  halfword m  ; 
  integer p  ; 
  m = curchr ; 
  switch ( curcmd ) 
  {case 85 : 
    {
      scancharnum () ; 
      if ( m == 12407 ) 
      {
	curval = eqtb [ 12407 + curval ] .hh .v.RH ; 
	curvallevel = 0 ; 
      } 
      else if ( m < 12407 ) 
      {
	curval = eqtb [ m + curval ] .hh .v.RH ; 
	curvallevel = 0 ; 
      } 
      else {
	  
	curval = eqtb [ m + curval ] .cint ; 
	curvallevel = 0 ; 
      } 
    } 
    break ; 
  case 71 : 
  case 72 : 
  case 86 : 
  case 87 : 
  case 88 : 
    if ( level != 5 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 660 ) ; 
      } 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 661 ; 
	helpline [ 1 ] = 662 ; 
	helpline [ 0 ] = 663 ; 
      } 
      backerror () ; 
      {
	curval = 0 ; 
	curvallevel = 1 ; 
      } 
    } 
    else if ( curcmd <= 72 ) 
    {
      if ( curcmd < 72 ) 
      {
	scaneightbitint () ; 
	m = 10822 + curval ; 
      } 
      {
	curval = eqtb [ m ] .hh .v.RH ; 
	curvallevel = 5 ; 
      } 
    } 
    else {
	
      backinput () ; 
      scanfontident () ; 
      {
	curval = 10024 + curval ; 
	curvallevel = 4 ; 
      } 
    } 
    break ; 
  case 73 : 
    {
      curval = eqtb [ m ] .cint ; 
      curvallevel = 0 ; 
    } 
    break ; 
  case 74 : 
    {
      curval = eqtb [ m ] .cint ; 
      curvallevel = 1 ; 
    } 
    break ; 
  case 75 : 
    {
      curval = eqtb [ m ] .hh .v.RH ; 
      curvallevel = 2 ; 
    } 
    break ; 
  case 76 : 
    {
      curval = eqtb [ m ] .hh .v.RH ; 
      curvallevel = 3 ; 
    } 
    break ; 
  case 79 : 
    if ( abs ( curlist .modefield ) != m ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 676 ) ; 
      } 
      printcmdchr ( 79 , m ) ; 
      {
	helpptr = 4 ; 
	helpline [ 3 ] = 677 ; 
	helpline [ 2 ] = 678 ; 
	helpline [ 1 ] = 679 ; 
	helpline [ 0 ] = 680 ; 
      } 
      error () ; 
      if ( level != 5 ) 
      {
	curval = 0 ; 
	curvallevel = 1 ; 
      } 
      else {
	  
	curval = 0 ; 
	curvallevel = 0 ; 
      } 
    } 
    else if ( m == 1 ) 
    {
      curval = curlist .auxfield .cint ; 
      curvallevel = 1 ; 
    } 
    else {
	
      curval = curlist .auxfield .hh .v.LH ; 
      curvallevel = 0 ; 
    } 
    break ; 
  case 80 : 
    if ( curlist .modefield == 0 ) 
    {
      curval = 0 ; 
      curvallevel = 0 ; 
    } 
    else {
	
      nest [ nestptr ] = curlist ; 
      p = nestptr ; 
      while ( abs ( nest [ p ] .modefield ) != 1 ) decr ( p ) ; 
      {
	curval = nest [ p ] .pgfield ; 
	curvallevel = 0 ; 
      } 
    } 
    break ; 
  case 82 : 
    {
      if ( m == 0 ) 
      curval = deadcycles ; 
      else curval = insertpenalties ; 
      curvallevel = 0 ; 
    } 
    break ; 
  case 81 : 
    {
      if ( ( pagecontents == 0 ) && ( ! outputactive ) ) 
      if ( m == 0 ) 
      curval = 1073741823L ; 
      else curval = 0 ; 
      else curval = pagesofar [ m ] ; 
      curvallevel = 1 ; 
    } 
    break ; 
  case 84 : 
    {
      if ( eqtb [ 10812 ] .hh .v.RH == 0 ) 
      curval = 0 ; 
      else curval = mem [ eqtb [ 10812 ] .hh .v.RH ] .hh .v.LH ; 
      curvallevel = 0 ; 
    } 
    break ; 
  case 83 : 
    {
      scaneightbitint () ; 
      if ( eqtb [ 11078 + curval ] .hh .v.RH == 0 ) 
      curval = 0 ; 
      else curval = mem [ eqtb [ 11078 + curval ] .hh .v.RH + m ] .cint ; 
      curvallevel = 1 ; 
    } 
    break ; 
  case 68 : 
  case 69 : 
    {
      curval = curchr ; 
      curvallevel = 0 ; 
    } 
    break ; 
  case 77 : 
    {
      findfontdimen ( false ) ; 
      fontinfo [ fmemptr ] .cint = 0 ; 
      {
	curval = fontinfo [ curval ] .cint ; 
	curvallevel = 1 ; 
      } 
    } 
    break ; 
  case 78 : 
    {
      scanfontident () ; 
      if ( m == 0 ) 
      {
	curval = hyphenchar [ curval ] ; 
	curvallevel = 0 ; 
      } 
      else {
	  
	curval = skewchar [ curval ] ; 
	curvallevel = 0 ; 
      } 
    } 
    break ; 
  case 89 : 
    {
      scaneightbitint () ; 
      switch ( m ) 
      {case 0 : 
	curval = eqtb [ 12718 + curval ] .cint ; 
	break ; 
      case 1 : 
	curval = eqtb [ 13251 + curval ] .cint ; 
	break ; 
      case 2 : 
	curval = eqtb [ 10300 + curval ] .hh .v.RH ; 
	break ; 
      case 3 : 
	curval = eqtb [ 10556 + curval ] .hh .v.RH ; 
	break ; 
      } 
      curvallevel = m ; 
    } 
    break ; 
  case 70 : 
    if ( curchr > 2 ) 
    {
      if ( curchr == 3 ) 
      curval = line ; 
      else curval = lastbadness ; 
      curvallevel = 0 ; 
    } 
    else {
	
      if ( curchr == 2 ) 
      curval = 0 ; 
      else curval = 0 ; 
      curvallevel = curchr ; 
      if ( ! ( curlist .tailfield >= himemmin ) && ( curlist .modefield != 0 ) 
      ) 
      switch ( curchr ) 
      {case 0 : 
	if ( mem [ curlist .tailfield ] .hh.b0 == 12 ) 
	curval = mem [ curlist .tailfield + 1 ] .cint ; 
	break ; 
      case 1 : 
	if ( mem [ curlist .tailfield ] .hh.b0 == 11 ) 
	curval = mem [ curlist .tailfield + 1 ] .cint ; 
	break ; 
      case 2 : 
	if ( mem [ curlist .tailfield ] .hh.b0 == 10 ) 
	{
	  curval = mem [ curlist .tailfield + 1 ] .hh .v.LH ; 
	  if ( mem [ curlist .tailfield ] .hh.b1 == 99 ) 
	  curvallevel = 3 ; 
	} 
	break ; 
      } 
      else if ( ( curlist .modefield == 1 ) && ( curlist .tailfield == curlist 
      .headfield ) ) 
      switch ( curchr ) 
      {case 0 : 
	curval = lastpenalty ; 
	break ; 
      case 1 : 
	curval = lastkern ; 
	break ; 
      case 2 : 
	if ( lastglue != 262143L ) 
	curval = lastglue ; 
	break ; 
      } 
    } 
    break ; 
    default: 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 681 ) ; 
      } 
      printcmdchr ( curcmd , curchr ) ; 
      print ( 682 ) ; 
      printesc ( 533 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 680 ; 
      } 
      error () ; 
      if ( level != 5 ) 
      {
	curval = 0 ; 
	curvallevel = 1 ; 
      } 
      else {
	  
	curval = 0 ; 
	curvallevel = 0 ; 
      } 
    } 
    break ; 
  } 
  while ( curvallevel > level ) {
      
    if ( curvallevel == 2 ) 
    curval = mem [ curval + 1 ] .cint ; 
    else if ( curvallevel == 3 ) 
    muerror () ; 
    decr ( curvallevel ) ; 
  } 
  if ( negative ) 
  if ( curvallevel >= 2 ) 
  {
    curval = newspec ( curval ) ; 
    {
      mem [ curval + 1 ] .cint = - (integer) mem [ curval + 1 ] .cint ; 
      mem [ curval + 2 ] .cint = - (integer) mem [ curval + 2 ] .cint ; 
      mem [ curval + 3 ] .cint = - (integer) mem [ curval + 3 ] .cint ; 
    } 
  } 
  else curval = - (integer) curval ; 
  else if ( ( curvallevel >= 2 ) && ( curvallevel <= 3 ) ) 
  incr ( mem [ curval ] .hh .v.RH ) ; 
} 
