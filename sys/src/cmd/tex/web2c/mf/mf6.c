#define EXTERN extern
#include "mfd.h"

void zmacrocall ( defref , arglist , macroname ) 
halfword defref ; 
halfword arglist ; 
halfword macroname ; 
{/* 40 */ halfword r  ; 
  halfword p, q  ; 
  integer n  ; 
  halfword ldelim, rdelim  ; 
  halfword tail  ; 
  r = mem [ defref ] .hhfield .v.RH ; 
  incr ( mem [ defref ] .hhfield .lhfield ) ; 
  if ( arglist == 0 ) 
  n = 0 ; 
  else {
      
    n = 1 ; 
    tail = arglist ; 
    while ( mem [ tail ] .hhfield .v.RH != 0 ) {
	
      incr ( n ) ; 
      tail = mem [ tail ] .hhfield .v.RH ; 
    } 
  } 
  if ( internal [ 9 ] > 0 ) 
  {
    begindiagnostic () ; 
    println () ; 
    printmacroname ( arglist , macroname ) ; 
    if ( n == 3 ) 
    print ( 662 ) ; 
    showmacro ( defref , 0 , 100000L ) ; 
    if ( arglist != 0 ) 
    {
      n = 0 ; 
      p = arglist ; 
      do {
	  q = mem [ p ] .hhfield .lhfield ; 
	printarg ( q , n , 0 ) ; 
	incr ( n ) ; 
	p = mem [ p ] .hhfield .v.RH ; 
      } while ( ! ( p == 0 ) ) ; 
    } 
    enddiagnostic ( false ) ; 
  } 
  curcmd = 83 ; 
  while ( mem [ r ] .hhfield .lhfield >= 9770 ) {
      
    if ( curcmd != 82 ) 
    {
      getxnext () ; 
      if ( curcmd != 31 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 706 ) ; 
	} 
	printmacroname ( arglist , macroname ) ; 
	{
	  helpptr = 3 ; 
	  helpline [ 2 ] = 707 ; 
	  helpline [ 1 ] = 708 ; 
	  helpline [ 0 ] = 709 ; 
	} 
	if ( mem [ r ] .hhfield .lhfield >= 9920 ) 
	{
	  curexp = 0 ; 
	  curtype = 20 ; 
	} 
	else {
	    
	  curexp = 0 ; 
	  curtype = 16 ; 
	} 
	backerror () ; 
	curcmd = 62 ; 
	goto lab40 ; 
      } 
      ldelim = cursym ; 
      rdelim = curmod ; 
    } 
    if ( mem [ r ] .hhfield .lhfield >= 10070 ) 
    scantextarg ( ldelim , rdelim ) ; 
    else {
	
      getxnext () ; 
      if ( mem [ r ] .hhfield .lhfield >= 9920 ) 
      scansuffix () ; 
      else scanexpression () ; 
    } 
    if ( curcmd != 82 ) 
    if ( ( curcmd != 62 ) || ( curmod != ldelim ) ) 
    if ( mem [ mem [ r ] .hhfield .v.RH ] .hhfield .lhfield >= 9770 ) 
    {
      missingerr ( 44 ) ; 
      {
	helpptr = 3 ; 
	helpline [ 2 ] = 710 ; 
	helpline [ 1 ] = 711 ; 
	helpline [ 0 ] = 705 ; 
      } 
      backerror () ; 
      curcmd = 82 ; 
    } 
    else {
	
      missingerr ( hash [ rdelim ] .v.RH ) ; 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 712 ; 
	helpline [ 0 ] = 705 ; 
      } 
      backerror () ; 
    } 
    lab40: {
	
      p = getavail () ; 
      if ( curtype == 20 ) 
      mem [ p ] .hhfield .lhfield = curexp ; 
      else mem [ p ] .hhfield .lhfield = stashcurexp () ; 
      if ( internal [ 9 ] > 0 ) 
      {
	begindiagnostic () ; 
	printarg ( mem [ p ] .hhfield .lhfield , n , mem [ r ] .hhfield 
	.lhfield ) ; 
	enddiagnostic ( false ) ; 
      } 
      if ( arglist == 0 ) 
      arglist = p ; 
      else mem [ tail ] .hhfield .v.RH = p ; 
      tail = p ; 
      incr ( n ) ; 
    } 
    r = mem [ r ] .hhfield .v.RH ; 
  } 
  if ( curcmd == 82 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 701 ) ; 
    } 
    printmacroname ( arglist , macroname ) ; 
    printchar ( 59 ) ; 
    printnl ( 702 ) ; 
    slowprint ( hash [ rdelim ] .v.RH ) ; 
    print ( 298 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 703 ; 
      helpline [ 1 ] = 704 ; 
      helpline [ 0 ] = 705 ; 
    } 
    error () ; 
  } 
  if ( mem [ r ] .hhfield .lhfield != 0 ) 
  {
    if ( mem [ r ] .hhfield .lhfield < 7 ) 
    {
      getxnext () ; 
      if ( mem [ r ] .hhfield .lhfield != 6 ) 
      if ( ( curcmd == 51 ) || ( curcmd == 77 ) ) 
      getxnext () ; 
    } 
    switch ( mem [ r ] .hhfield .lhfield ) 
    {case 1 : 
      scanprimary () ; 
      break ; 
    case 2 : 
      scansecondary () ; 
      break ; 
    case 3 : 
      scantertiary () ; 
      break ; 
    case 4 : 
      scanexpression () ; 
      break ; 
    case 5 : 
      {
	scanexpression () ; 
	p = getavail () ; 
	mem [ p ] .hhfield .lhfield = stashcurexp () ; 
	if ( internal [ 9 ] > 0 ) 
	{
	  begindiagnostic () ; 
	  printarg ( mem [ p ] .hhfield .lhfield , n , 0 ) ; 
	  enddiagnostic ( false ) ; 
	} 
	if ( arglist == 0 ) 
	arglist = p ; 
	else mem [ tail ] .hhfield .v.RH = p ; 
	tail = p ; 
	incr ( n ) ; 
	if ( curcmd != 69 ) 
	{
	  missingerr ( 478 ) ; 
	  print ( 713 ) ; 
	  printmacroname ( arglist , macroname ) ; 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 714 ; 
	  } 
	  backerror () ; 
	} 
	getxnext () ; 
	scanprimary () ; 
      } 
      break ; 
    case 6 : 
      {
	if ( curcmd != 31 ) 
	ldelim = 0 ; 
	else {
	    
	  ldelim = cursym ; 
	  rdelim = curmod ; 
	  getxnext () ; 
	} 
	scansuffix () ; 
	if ( ldelim != 0 ) 
	{
	  if ( ( curcmd != 62 ) || ( curmod != ldelim ) ) 
	  {
	    missingerr ( hash [ rdelim ] .v.RH ) ; 
	    {
	      helpptr = 2 ; 
	      helpline [ 1 ] = 712 ; 
	      helpline [ 0 ] = 705 ; 
	    } 
	    backerror () ; 
	  } 
	  getxnext () ; 
	} 
      } 
      break ; 
    case 7 : 
      scantextarg ( 0 , 0 ) ; 
      break ; 
    } 
    backinput () ; 
    {
      p = getavail () ; 
      if ( curtype == 20 ) 
      mem [ p ] .hhfield .lhfield = curexp ; 
      else mem [ p ] .hhfield .lhfield = stashcurexp () ; 
      if ( internal [ 9 ] > 0 ) 
      {
	begindiagnostic () ; 
	printarg ( mem [ p ] .hhfield .lhfield , n , mem [ r ] .hhfield 
	.lhfield ) ; 
	enddiagnostic ( false ) ; 
      } 
      if ( arglist == 0 ) 
      arglist = p ; 
      else mem [ tail ] .hhfield .v.RH = p ; 
      tail = p ; 
      incr ( n ) ; 
    } 
  } 
  r = mem [ r ] .hhfield .v.RH ; 
  while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
  endtokenlist () ; 
  if ( paramptr + n > maxparamstack ) 
  {
    maxparamstack = paramptr + n ; 
    if ( maxparamstack > 150 ) 
    overflow ( 684 , 150 ) ; 
  } 
  begintokenlist ( defref , 21 ) ; 
  curinput .namefield = macroname ; 
  curinput .locfield = r ; 
  if ( n > 0 ) 
  {
    p = arglist ; 
    do {
	paramstack [ paramptr ] = mem [ p ] .hhfield .lhfield ; 
      incr ( paramptr ) ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } while ( ! ( p == 0 ) ) ; 
    flushlist ( arglist ) ; 
  } 
} 
void expand ( ) 
{halfword p  ; 
  integer k  ; 
  poolpointer j  ; 
  if ( internal [ 7 ] > 65536L ) 
  if ( curcmd != 10 ) 
  showcmdmod ( curcmd , curmod ) ; 
  switch ( curcmd ) 
  {case 1 : 
    conditional () ; 
    break ; 
  case 2 : 
    if ( curmod > iflimit ) 
    if ( iflimit == 1 ) 
    {
      missingerr ( 58 ) ; 
      backinput () ; 
      cursym = 9762 ; 
      inserror () ; 
    } 
    else {
	
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 721 ) ; 
      } 
      printcmdmod ( 2 , curmod ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 722 ; 
      } 
      error () ; 
    } 
    else {
	
      while ( curmod != 2 ) passtext () ; 
      {
	p = condptr ; 
	ifline = mem [ p + 1 ] .cint ; 
	curif = mem [ p ] .hhfield .b1 ; 
	iflimit = mem [ p ] .hhfield .b0 ; 
	condptr = mem [ p ] .hhfield .v.RH ; 
	freenode ( p , 2 ) ; 
      } 
    } 
    break ; 
  case 3 : 
    if ( curmod > 0 ) 
    forceeof = true ; 
    else startinput () ; 
    break ; 
  case 4 : 
    if ( curmod == 0 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 685 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 686 ; 
	helpline [ 0 ] = 687 ; 
      } 
      error () ; 
    } 
    else beginiteration () ; 
    break ; 
  case 5 : 
    {
      while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 ) ) 
      endtokenlist () ; 
      if ( loopptr == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 689 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 690 ; 
	  helpline [ 0 ] = 691 ; 
	} 
	error () ; 
      } 
      else resumeiteration () ; 
    } 
    break ; 
  case 6 : 
    {
      getboolean () ; 
      if ( internal [ 7 ] > 65536L ) 
      showcmdmod ( 33 , curexp ) ; 
      if ( curexp == 30 ) 
      if ( loopptr == 0 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 261 ) ; 
	  print ( 692 ) ; 
	} 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 693 ; 
	} 
	if ( curcmd == 83 ) 
	error () ; 
	else backerror () ; 
      } 
      else {
	  
	p = 0 ; 
	do {
	    if ( ( curinput .indexfield <= 15 ) ) 
	  endfilereading () ; 
	  else {
	      
	    if ( curinput .indexfield <= 17 ) 
	    p = curinput .startfield ; 
	    endtokenlist () ; 
	  } 
	} while ( ! ( p != 0 ) ) ; 
	if ( p != mem [ loopptr ] .hhfield .lhfield ) 
	fatalerror ( 696 ) ; 
	stopiteration () ; 
      } 
      else if ( curcmd != 83 ) 
      {
	missingerr ( 59 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 694 ; 
	  helpline [ 0 ] = 695 ; 
	} 
	backerror () ; 
      } 
    } 
    break ; 
  case 7 : 
    ; 
    break ; 
  case 9 : 
    {
      getnext () ; 
      p = curtok () ; 
      getnext () ; 
      if ( curcmd < 11 ) 
      expand () ; 
      else backinput () ; 
      begintokenlist ( p , 19 ) ; 
    } 
    break ; 
  case 8 : 
    {
      getxnext () ; 
      scanprimary () ; 
      if ( curtype != 4 ) 
      {
	disperr ( 0 , 697 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 698 ; 
	  helpline [ 0 ] = 699 ; 
	} 
	putgetflusherror ( 0 ) ; 
      } 
      else {
	  
	backinput () ; 
	if ( ( strstart [ curexp + 1 ] - strstart [ curexp ] ) > 0 ) 
	{
	  beginfilereading () ; 
	  curinput .namefield = 2 ; 
	  k = first + ( strstart [ curexp + 1 ] - strstart [ curexp ] ) ; 
	  if ( k >= maxbufstack ) 
	  {
	    if ( k >= bufsize ) 
	    {
	      maxbufstack = bufsize ; 
	      overflow ( 256 , bufsize ) ; 
	    } 
	    maxbufstack = k + 1 ; 
	  } 
	  j = strstart [ curexp ] ; 
	  curinput .limitfield = k ; 
	  while ( first < curinput .limitfield ) {
	      
	    buffer [ first ] = strpool [ j ] ; 
	    incr ( j ) ; 
	    incr ( first ) ; 
	  } 
	  buffer [ curinput .limitfield ] = 37 ; 
	  first = curinput .limitfield + 1 ; 
	  curinput .locfield = curinput .startfield ; 
	  flushcurexp ( 0 ) ; 
	} 
      } 
    } 
    break ; 
  case 10 : 
    macrocall ( curmod , 0 , cursym ) ; 
    break ; 
  } 
} 
void getxnext ( ) 
{halfword saveexp  ; 
  getnext () ; 
  if ( curcmd < 11 ) 
  {
    saveexp = stashcurexp () ; 
    do {
	if ( curcmd == 10 ) 
      macrocall ( curmod , 0 , cursym ) ; 
      else expand () ; 
      getnext () ; 
    } while ( ! ( curcmd >= 11 ) ) ; 
    unstashcurexp ( saveexp ) ; 
  } 
} 
void zstackargument ( p ) 
halfword p ; 
{if ( paramptr == maxparamstack ) 
  {
    incr ( maxparamstack ) ; 
    if ( maxparamstack > 150 ) 
    overflow ( 684 , 150 ) ; 
  } 
  paramstack [ paramptr ] = p ; 
  incr ( paramptr ) ; 
} 
void passtext ( ) 
{/* 30 */ integer l  ; 
  scannerstatus = 1 ; 
  l = 0 ; 
  warninginfo = line ; 
  while ( true ) {
      
    getnext () ; 
    if ( curcmd <= 2 ) 
    if ( curcmd < 2 ) 
    incr ( l ) ; 
    else {
	
      if ( l == 0 ) 
      goto lab30 ; 
      if ( curmod == 2 ) 
      decr ( l ) ; 
    } 
    else if ( curcmd == 39 ) 
    {
      if ( strref [ curmod ] < 127 ) 
      if ( strref [ curmod ] > 1 ) 
      decr ( strref [ curmod ] ) ; 
      else flushstring ( curmod ) ; 
    } 
  } 
  lab30: scannerstatus = 0 ; 
} 
void zchangeiflimit ( l , p ) 
smallnumber l ; 
halfword p ; 
{/* 10 */ halfword q  ; 
  if ( p == condptr ) 
  iflimit = l ; 
  else {
      
    q = condptr ; 
    while ( true ) {
	
      if ( q == 0 ) 
      confusion ( 715 ) ; 
      if ( mem [ q ] .hhfield .v.RH == p ) 
      {
	mem [ q ] .hhfield .b0 = l ; 
	goto lab10 ; 
      } 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
  } 
  lab10: ; 
} 
void checkcolon ( ) 
{if ( curcmd != 81 ) 
  {
    missingerr ( 58 ) ; 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 718 ; 
      helpline [ 0 ] = 695 ; 
    } 
    backerror () ; 
  } 
} 
void conditional ( ) 
{/* 10 30 21 40 */ halfword savecondptr  ; 
  schar newiflimit  ; 
  halfword p  ; 
  {
    p = getnode ( 2 ) ; 
    mem [ p ] .hhfield .v.RH = condptr ; 
    mem [ p ] .hhfield .b0 = iflimit ; 
    mem [ p ] .hhfield .b1 = curif ; 
    mem [ p + 1 ] .cint = ifline ; 
    condptr = p ; 
    iflimit = 1 ; 
    ifline = line ; 
    curif = 1 ; 
  } 
  savecondptr = condptr ; 
  lab21: getboolean () ; 
  newiflimit = 4 ; 
  if ( internal [ 7 ] > 65536L ) 
  {
    begindiagnostic () ; 
    if ( curexp == 30 ) 
    print ( 719 ) ; 
    else print ( 720 ) ; 
    enddiagnostic ( false ) ; 
  } 
  lab40: checkcolon () ; 
  if ( curexp == 30 ) 
  {
    changeiflimit ( newiflimit , savecondptr ) ; 
    goto lab10 ; 
  } 
  while ( true ) {
      
    passtext () ; 
    if ( condptr == savecondptr ) 
    goto lab30 ; 
    else if ( curmod == 2 ) 
    {
      p = condptr ; 
      ifline = mem [ p + 1 ] .cint ; 
      curif = mem [ p ] .hhfield .b1 ; 
      iflimit = mem [ p ] .hhfield .b0 ; 
      condptr = mem [ p ] .hhfield .v.RH ; 
      freenode ( p , 2 ) ; 
    } 
  } 
  lab30: curif = curmod ; 
  ifline = line ; 
  if ( curmod == 2 ) 
  {
    p = condptr ; 
    ifline = mem [ p + 1 ] .cint ; 
    curif = mem [ p ] .hhfield .b1 ; 
    iflimit = mem [ p ] .hhfield .b0 ; 
    condptr = mem [ p ] .hhfield .v.RH ; 
    freenode ( p , 2 ) ; 
  } 
  else if ( curmod == 4 ) 
  goto lab21 ; 
  else {
      
    curexp = 30 ; 
    newiflimit = 2 ; 
    getxnext () ; 
    goto lab40 ; 
  } 
  lab10: ; 
} 
void zbadfor ( s ) 
strnumber s ; 
{disperr ( 0 , 723 ) ; 
  print ( s ) ; 
  print ( 305 ) ; 
  {
    helpptr = 4 ; 
    helpline [ 3 ] = 724 ; 
    helpline [ 2 ] = 725 ; 
    helpline [ 1 ] = 726 ; 
    helpline [ 0 ] = 307 ; 
  } 
  putgetflusherror ( 0 ) ; 
} 
void beginiteration ( ) 
{/* 22 30 40 */ halfword m  ; 
  halfword n  ; 
  halfword p, q, s, pp  ; 
  m = curmod ; 
  n = cursym ; 
  s = getnode ( 2 ) ; 
  if ( m == 1 ) 
  {
    mem [ s + 1 ] .hhfield .lhfield = 1 ; 
    p = 0 ; 
    getxnext () ; 
    goto lab40 ; 
  } 
  getsymbol () ; 
  p = getnode ( 2 ) ; 
  mem [ p ] .hhfield .lhfield = cursym ; 
  mem [ p + 1 ] .cint = m ; 
  getxnext () ; 
  if ( ( curcmd != 51 ) && ( curcmd != 77 ) ) 
  {
    missingerr ( 61 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 727 ; 
      helpline [ 1 ] = 670 ; 
      helpline [ 0 ] = 728 ; 
    } 
    backerror () ; 
  } 
  mem [ s + 1 ] .hhfield .lhfield = 0 ; 
  q = s + 1 ; 
  mem [ q ] .hhfield .v.RH = 0 ; 
  do {
      getxnext () ; 
    if ( m != 9770 ) 
    scansuffix () ; 
    else {
	
      if ( curcmd >= 81 ) 
      if ( curcmd <= 82 ) 
      goto lab22 ; 
      scanexpression () ; 
      if ( curcmd == 74 ) 
      if ( q == s + 1 ) 
      {
	if ( curtype != 16 ) 
	badfor ( 734 ) ; 
	pp = getnode ( 4 ) ; 
	mem [ pp + 1 ] .cint = curexp ; 
	getxnext () ; 
	scanexpression () ; 
	if ( curtype != 16 ) 
	badfor ( 735 ) ; 
	mem [ pp + 2 ] .cint = curexp ; 
	if ( curcmd != 75 ) 
	{
	  missingerr ( 489 ) ; 
	  {
	    helpptr = 2 ; 
	    helpline [ 1 ] = 736 ; 
	    helpline [ 0 ] = 737 ; 
	  } 
	  backerror () ; 
	} 
	getxnext () ; 
	scanexpression () ; 
	if ( curtype != 16 ) 
	badfor ( 738 ) ; 
	mem [ pp + 3 ] .cint = curexp ; 
	mem [ s + 1 ] .hhfield .lhfield = pp ; 
	goto lab30 ; 
      } 
      curexp = stashcurexp () ; 
    } 
    mem [ q ] .hhfield .v.RH = getavail () ; 
    q = mem [ q ] .hhfield .v.RH ; 
    mem [ q ] .hhfield .lhfield = curexp ; 
    curtype = 1 ; 
    lab22: ; 
  } while ( ! ( curcmd != 82 ) ) ; 
  lab30: ; 
  lab40: if ( curcmd != 81 ) 
  {
    missingerr ( 58 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 729 ; 
      helpline [ 1 ] = 730 ; 
      helpline [ 0 ] = 731 ; 
    } 
    backerror () ; 
  } 
  q = getavail () ; 
  mem [ q ] .hhfield .lhfield = 9758 ; 
  scannerstatus = 6 ; 
  warninginfo = n ; 
  mem [ s ] .hhfield .lhfield = scantoks ( 4 , p , q , 0 ) ; 
  scannerstatus = 0 ; 
  mem [ s ] .hhfield .v.RH = loopptr ; 
  loopptr = s ; 
  resumeiteration () ; 
} 
void resumeiteration ( ) 
{/* 45 10 */ halfword p, q  ; 
  p = mem [ loopptr + 1 ] .hhfield .lhfield ; 
  if ( p > 1 ) 
  {
    curexp = mem [ p + 1 ] .cint ; 
    if ( ( ( mem [ p + 2 ] .cint > 0 ) && ( curexp > mem [ p + 3 ] .cint ) ) 
    || ( ( mem [ p + 2 ] .cint < 0 ) && ( curexp < mem [ p + 3 ] .cint ) ) ) 
    goto lab45 ; 
    curtype = 16 ; 
    q = stashcurexp () ; 
    mem [ p + 1 ] .cint = curexp + mem [ p + 2 ] .cint ; 
  } 
  else if ( p < 1 ) 
  {
    p = mem [ loopptr + 1 ] .hhfield .v.RH ; 
    if ( p == 0 ) 
    goto lab45 ; 
    mem [ loopptr + 1 ] .hhfield .v.RH = mem [ p ] .hhfield .v.RH ; 
    q = mem [ p ] .hhfield .lhfield ; 
    {
      mem [ p ] .hhfield .v.RH = avail ; 
      avail = p ; 
	;
#ifdef STAT
      decr ( dynused ) ; 
#endif /* STAT */
    } 
  } 
  else {
      
    begintokenlist ( mem [ loopptr ] .hhfield .lhfield , 16 ) ; 
    goto lab10 ; 
  } 
  begintokenlist ( mem [ loopptr ] .hhfield .lhfield , 17 ) ; 
  stackargument ( q ) ; 
  if ( internal [ 7 ] > 65536L ) 
  {
    begindiagnostic () ; 
    printnl ( 733 ) ; 
    if ( ( q != 0 ) && ( mem [ q ] .hhfield .v.RH == 1 ) ) 
    printexp ( q , 1 ) ; 
    else showtokenlist ( q , 0 , 50 , 0 ) ; 
    printchar ( 125 ) ; 
    enddiagnostic ( false ) ; 
  } 
  goto lab10 ; 
  lab45: stopiteration () ; 
  lab10: ; 
} 
void stopiteration ( ) 
{halfword p, q  ; 
  p = mem [ loopptr + 1 ] .hhfield .lhfield ; 
  if ( p > 1 ) 
  freenode ( p , 4 ) ; 
  else if ( p < 1 ) 
  {
    q = mem [ loopptr + 1 ] .hhfield .v.RH ; 
    while ( q != 0 ) {
	
      p = mem [ q ] .hhfield .lhfield ; 
      if ( p != 0 ) 
      if ( mem [ p ] .hhfield .v.RH == 1 ) 
      {
	recyclevalue ( p ) ; 
	freenode ( p , 2 ) ; 
      } 
      else flushtokenlist ( p ) ; 
      p = q ; 
      q = mem [ q ] .hhfield .v.RH ; 
      {
	mem [ p ] .hhfield .v.RH = avail ; 
	avail = p ; 
	;
#ifdef STAT
	decr ( dynused ) ; 
#endif /* STAT */
      } 
    } 
  } 
  p = loopptr ; 
  loopptr = mem [ p ] .hhfield .v.RH ; 
  flushtokenlist ( mem [ p ] .hhfield .lhfield ) ; 
  freenode ( p , 2 ) ; 
} 
void beginname ( ) 
{areadelimiter = 0 ; 
  extdelimiter = 0 ; 
} 
boolean zmorename ( c ) 
ASCIIcode c ; 
{register boolean Result; if ( ( c == 32 ) || ( c == 9 ) ) 
  Result = false ; 
  else {
      
    if ( ( c == 47 ) ) 
    {
      areadelimiter = poolptr ; 
      extdelimiter = 0 ; 
    } 
    else if ( ( c == 46 ) && ( extdelimiter == 0 ) ) 
    extdelimiter = poolptr ; 
    {
      if ( poolptr + 1 > maxpoolptr ) 
      {
	if ( poolptr + 1 > poolsize ) 
	overflow ( 257 , poolsize - initpoolptr ) ; 
	maxpoolptr = poolptr + 1 ; 
      } 
    } 
    {
      strpool [ poolptr ] = c ; 
      incr ( poolptr ) ; 
    } 
    Result = true ; 
  } 
  return(Result) ; 
} 
void endname ( ) 
{if ( strptr + 3 > maxstrptr ) 
  {
    if ( strptr + 3 > maxstrings ) 
    overflow ( 258 , maxstrings - initstrptr ) ; 
    maxstrptr = strptr + 3 ; 
  } 
  if ( areadelimiter == 0 ) 
  curarea = 283 ; 
  else {
      
    curarea = strptr ; 
    incr ( strptr ) ; 
    strstart [ strptr ] = areadelimiter + 1 ; 
  } 
  if ( extdelimiter == 0 ) 
  {
    curext = 283 ; 
    curname = makestring () ; 
  } 
  else {
      
    curname = strptr ; 
    incr ( strptr ) ; 
    strstart [ strptr ] = extdelimiter ; 
    curext = makestring () ; 
  } 
} 
void zpackfilename ( n , a , e ) 
strnumber n ; 
strnumber a ; 
strnumber e ; 
{integer k  ; 
  ASCIIcode c  ; 
  poolpointer j  ; 
  k = 0 ; 
  {register integer for_end; j = strstart [ a ] ; for_end = strstart [ a + 1 
  ] - 1 ; if ( j <= for_end) do 
    {
      c = strpool [ j ] ; 
      incr ( k ) ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = strstart [ n ] ; for_end = strstart [ n + 1 
  ] - 1 ; if ( j <= for_end) do 
    {
      c = strpool [ j ] ; 
      incr ( k ) ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = strstart [ e ] ; for_end = strstart [ e + 1 
  ] - 1 ; if ( j <= for_end) do 
    {
      c = strpool [ j ] ; 
      incr ( k ) ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  if ( k < PATHMAX ) 
  namelength = k ; 
  else namelength = PATHMAX - 1 ; 
  {register integer for_end; k = namelength + 1 ; for_end = PATHMAX ; if ( k 
  <= for_end) do 
    nameoffile [ k ] = ' ' ; 
  while ( k++ < for_end ) ; } 
} 
void zpackbufferedname ( n , a , b ) 
smallnumber n ; 
integer a ; 
integer b ; 
{integer k  ; 
  ASCIIcode c  ; 
  integer j  ; 
  if ( n + b - a + 6 > PATHMAX ) 
  b = a + PATHMAX - n - 6 ; 
  k = 0 ; 
  {register integer for_end; j = 1 ; for_end = n ; if ( j <= for_end) do 
    {
      c = xord [ MFbasedefault [ j ] ] ; 
      incr ( k ) ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = a ; for_end = b ; if ( j <= for_end) do 
    {
      c = buffer [ j ] ; 
      incr ( k ) ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  {register integer for_end; j = basedefaultlength - 4 ; for_end = 
  basedefaultlength ; if ( j <= for_end) do 
    {
      c = xord [ MFbasedefault [ j ] ] ; 
      incr ( k ) ; 
      if ( k <= PATHMAX ) 
      nameoffile [ k ] = xchr [ c ] ; 
    } 
  while ( j++ < for_end ) ; } 
  if ( k < PATHMAX ) 
  namelength = k ; 
  else namelength = PATHMAX - 1 ; 
  {register integer for_end; k = namelength + 1 ; for_end = PATHMAX ; if ( k 
  <= for_end) do 
    nameoffile [ k ] = ' ' ; 
  while ( k++ < for_end ) ; } 
} 
strnumber makenamestring ( ) 
{register strnumber Result; integer k  ; 
  if ( ( poolptr + namelength > poolsize ) || ( strptr == maxstrings ) ) 
  Result = 63 ; 
  else {
      
    {register integer for_end; k = 1 ; for_end = namelength ; if ( k <= 
    for_end) do 
      {
	strpool [ poolptr ] = xord [ nameoffile [ k ] ] ; 
	incr ( poolptr ) ; 
      } 
    while ( k++ < for_end ) ; } 
    Result = makestring () ; 
  } 
  return(Result) ; 
} 
strnumber zamakenamestring ( f ) 
alphafile * f ; 
{register strnumber Result; Result = makenamestring () ; 
  return(Result) ; 
} 
strnumber zbmakenamestring ( f ) 
bytefile * f ; 
{register strnumber Result; Result = makenamestring () ; 
  return(Result) ; 
} 
strnumber zwmakenamestring ( f ) 
wordfile * f ; 
{register strnumber Result; Result = makenamestring () ; 
  return(Result) ; 
} 
void scanfilename ( ) 
{/* 30 */ beginname () ; 
  while ( ( buffer [ curinput .locfield ] == 32 ) || ( buffer [ curinput 
  .locfield ] == 9 ) ) incr ( curinput .locfield ) ; 
  while ( true ) {
      
    if ( ( buffer [ curinput .locfield ] == 59 ) || ( buffer [ curinput 
    .locfield ] == 37 ) ) 
    goto lab30 ; 
    if ( ! morename ( buffer [ curinput .locfield ] ) ) 
    goto lab30 ; 
    incr ( curinput .locfield ) ; 
  } 
  lab30: endname () ; 
} 
void zpackjobname ( s ) 
strnumber s ; 
{curarea = 283 ; 
  curext = s ; 
  curname = jobname ; 
  packfilename ( curname , curarea , curext ) ; 
} 
void zpromptfilename ( s , e ) 
strnumber s ; 
strnumber e ; 
{/* 30 */ integer k  ; 
  if ( interaction == 2 ) 
  ; 
  if ( s == 740 ) 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 741 ) ; 
  } 
  else {
      
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 742 ) ; 
  } 
  printfilename ( curname , curarea , curext ) ; 
  print ( 743 ) ; 
  if ( e == 744 ) 
  showcontext () ; 
  printnl ( 745 ) ; 
  print ( s ) ; 
  if ( interaction < 2 ) 
  fatalerror ( 746 ) ; 
  {
    ; 
    print ( 747 ) ; 
    terminput () ; 
  } 
  {
    beginname () ; 
    k = first ; 
    while ( ( ( buffer [ k ] == 32 ) || ( buffer [ k ] == 9 ) ) && ( k < last 
    ) ) incr ( k ) ; 
    while ( true ) {
	
      if ( k == last ) 
      goto lab30 ; 
      if ( ! morename ( buffer [ k ] ) ) 
      goto lab30 ; 
      incr ( k ) ; 
    } 
    lab30: endname () ; 
  } 
  if ( curext == 283 ) 
  curext = e ; 
  packfilename ( curname , curarea , curext ) ; 
} 
void openlogfile ( ) 
{schar oldsetting  ; 
  integer k  ; 
  integer l  ; 
  integer m  ; 
  ccharpointer months  ; 
  oldsetting = selector ; 
  if ( jobname == 0 ) 
  jobname = 748 ; 
  packjobname ( 749 ) ; 
  while ( ! aopenout ( logfile ) ) {
      
    selector = 1 ; 
    promptfilename ( 751 , 749 ) ; 
  } 
  texmflogname = amakenamestring ( logfile ) ; 
  selector = 2 ; 
  logopened = true ; 
  {
    (void) Fputs( logfile ,  "This is METAFONT, C Version 2.71" ) ; 
    slowprint ( baseident ) ; 
    print ( 752 ) ; 
    printint ( roundunscaled ( internal [ 16 ] ) ) ; 
    printchar ( 32 ) ; 
    months = " JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC" ; 
    m = roundunscaled ( internal [ 15 ] ) ; 
    {register integer for_end; k = 3 * m - 2 ; for_end = 3 * m ; if ( k <= 
    for_end) do 
      (void) putc( months [ k ] ,  logfile );
    while ( k++ < for_end ) ; } 
    printchar ( 32 ) ; 
    printint ( roundunscaled ( internal [ 14 ] ) ) ; 
    printchar ( 32 ) ; 
    m = roundunscaled ( internal [ 17 ] ) ; 
    printdd ( m / 60 ) ; 
    printchar ( 58 ) ; 
    printdd ( m % 60 ) ; 
  } 
  inputstack [ inputptr ] = curinput ; 
  printnl ( 750 ) ; 
  l = inputstack [ 0 ] .limitfield - 1 ; 
  {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) do 
    print ( buffer [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  println () ; 
  selector = oldsetting + 2 ; 
} 
void startinput ( ) 
{/* 30 */ while ( ( curinput .indexfield > 15 ) && ( curinput .locfield == 0 
  ) ) endtokenlist () ; 
  if ( ( curinput .indexfield > 15 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 754 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 755 ; 
      helpline [ 1 ] = 756 ; 
      helpline [ 0 ] = 757 ; 
    } 
    error () ; 
  } 
  if ( ( curinput .indexfield <= 15 ) ) 
  scanfilename () ; 
  else {
      
    curname = 283 ; 
    curext = 283 ; 
    curarea = 283 ; 
  } 
  packfilename ( curname , curarea , curext ) ; 
  while ( true ) {
      
    beginfilereading () ; 
    if ( ( curext != 744 ) && ( namelength + 4 < PATHMAX ) && ( ! 
    extensionirrelevantp ( nameoffile , "mf" ) ) ) 
    {
      nameoffile [ namelength + 1 ] = 46 ; 
      nameoffile [ namelength + 2 ] = 109 ; 
      nameoffile [ namelength + 3 ] = 102 ; 
      namelength = namelength + 3 ; 
    } 
    if ( aopenin ( inputfile [ curinput .indexfield ] , MFINPUTPATH ) ) 
    goto lab30 ; 
    if ( curext == 744 ) 
    curext = 283 ; 
    packfilename ( curname , curarea , curext ) ; 
    if ( aopenin ( inputfile [ curinput .indexfield ] , MFINPUTPATH ) ) 
    goto lab30 ; 
    endfilereading () ; 
    promptfilename ( 740 , 744 ) ; 
  } 
  lab30: curinput .namefield = amakenamestring ( inputfile [ curinput 
  .indexfield ] ) ; 
  strref [ curname ] = 127 ; 
  if ( jobname == 0 ) 
  {
    jobname = curname ; 
    openlogfile () ; 
  } 
  if ( termoffset + ( strstart [ curinput .namefield + 1 ] - strstart [ 
  curinput .namefield ] ) > maxprintline - 2 ) 
  println () ; 
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ; 
  printchar ( 40 ) ; 
  incr ( openparens ) ; 
  slowprint ( curinput .namefield ) ; 
  flush ( stdout ) ; 
  {
    line = 1 ; 
    if ( inputln ( inputfile [ curinput .indexfield ] , false ) ) 
    ; 
    firmuptheline () ; 
    buffer [ curinput .limitfield ] = 37 ; 
    first = curinput .limitfield + 1 ; 
    curinput .locfield = curinput .startfield ; 
  } 
} 
void zbadexp ( s ) 
strnumber s ; 
{schar saveflag  ; 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( s ) ; 
  } 
  print ( 767 ) ; 
  printcmdmod ( curcmd , curmod ) ; 
  printchar ( 39 ) ; 
  {
    helpptr = 4 ; 
    helpline [ 3 ] = 768 ; 
    helpline [ 2 ] = 769 ; 
    helpline [ 1 ] = 770 ; 
    helpline [ 0 ] = 771 ; 
  } 
  backinput () ; 
  cursym = 0 ; 
  curcmd = 42 ; 
  curmod = 0 ; 
  inserror () ; 
  saveflag = varflag ; 
  varflag = 0 ; 
  getxnext () ; 
  varflag = saveflag ; 
} 
void zstashin ( p ) 
halfword p ; 
{halfword q  ; 
  mem [ p ] .hhfield .b0 = curtype ; 
  if ( curtype == 16 ) 
  mem [ p + 1 ] .cint = curexp ; 
  else {
      
    if ( curtype == 19 ) 
    {
      q = singledependency ( curexp ) ; 
      if ( q == depfinal ) 
      {
	mem [ p ] .hhfield .b0 = 16 ; 
	mem [ p + 1 ] .cint = 0 ; 
	freenode ( q , 2 ) ; 
      } 
      else {
	  
	mem [ p ] .hhfield .b0 = 17 ; 
	newdep ( p , q ) ; 
      } 
      recyclevalue ( curexp ) ; 
    } 
    else {
	
      mem [ p + 1 ] = mem [ curexp + 1 ] ; 
      mem [ mem [ p + 1 ] .hhfield .lhfield ] .hhfield .v.RH = p ; 
    } 
    freenode ( curexp , 2 ) ; 
  } 
  curtype = 1 ; 
} 
void backexpr ( ) 
{halfword p  ; 
  p = stashcurexp () ; 
  mem [ p ] .hhfield .v.RH = 0 ; 
  begintokenlist ( p , 19 ) ; 
} 
void badsubscript ( ) 
{disperr ( 0 , 783 ) ; 
  {
    helpptr = 3 ; 
    helpline [ 2 ] = 784 ; 
    helpline [ 1 ] = 785 ; 
    helpline [ 0 ] = 786 ; 
  } 
  flusherror ( 0 ) ; 
} 
void zobliterated ( q ) 
halfword q ; 
{{
    
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( 787 ) ; 
  } 
  showtokenlist ( q , 0 , 1000 , 0 ) ; 
  print ( 788 ) ; 
  {
    helpptr = 5 ; 
    helpline [ 4 ] = 789 ; 
    helpline [ 3 ] = 790 ; 
    helpline [ 2 ] = 791 ; 
    helpline [ 1 ] = 792 ; 
    helpline [ 0 ] = 793 ; 
  } 
} 
void zbinarymac ( p , c , n ) 
halfword p ; 
halfword c ; 
halfword n ; 
{halfword q, r  ; 
  q = getavail () ; 
  r = getavail () ; 
  mem [ q ] .hhfield .v.RH = r ; 
  mem [ q ] .hhfield .lhfield = p ; 
  mem [ r ] .hhfield .lhfield = stashcurexp () ; 
  macrocall ( c , q , n ) ; 
} 
void materializepen ( ) 
{/* 50 */ scaled aminusb, aplusb, majoraxis, minoraxis  ; 
  angle theta  ; 
  halfword p  ; 
  halfword q  ; 
  q = curexp ; 
  if ( mem [ q ] .hhfield .b0 == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 803 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 804 ; 
      helpline [ 0 ] = 574 ; 
    } 
    putgeterror () ; 
    curexp = 3 ; 
    goto lab50 ; 
  } 
  else if ( mem [ q ] .hhfield .b0 == 4 ) 
  {
    tx = mem [ q + 1 ] .cint ; 
    ty = mem [ q + 2 ] .cint ; 
    txx = mem [ q + 3 ] .cint - tx ; 
    tyx = mem [ q + 4 ] .cint - ty ; 
    txy = mem [ q + 5 ] .cint - tx ; 
    tyy = mem [ q + 6 ] .cint - ty ; 
    aminusb = pythadd ( txx - tyy , tyx + txy ) ; 
    aplusb = pythadd ( txx + tyy , tyx - txy ) ; 
    majoraxis = ( aminusb + aplusb ) / 2 ; 
    minoraxis = ( abs ( aplusb - aminusb ) ) / 2 ; 
    if ( majoraxis == minoraxis ) 
    theta = 0 ; 
    else theta = ( narg ( txx - tyy , tyx + txy ) + narg ( txx + tyy , tyx - 
    txy ) ) / 2 ; 
    freenode ( q , 7 ) ; 
    q = makeellipse ( majoraxis , minoraxis , theta ) ; 
    if ( ( tx != 0 ) || ( ty != 0 ) ) 
    {
      p = q ; 
      do {
	  mem [ p + 1 ] .cint = mem [ p + 1 ] .cint + tx ; 
	mem [ p + 2 ] .cint = mem [ p + 2 ] .cint + ty ; 
	p = mem [ p ] .hhfield .v.RH ; 
      } while ( ! ( p == q ) ) ; 
    } 
  } 
  curexp = makepen ( q ) ; 
  lab50: tossknotlist ( q ) ; 
  curtype = 6 ; 
} 
void knownpair ( ) 
{halfword p  ; 
  if ( curtype != 14 ) 
  {
    disperr ( 0 , 806 ) ; 
    {
      helpptr = 5 ; 
      helpline [ 4 ] = 807 ; 
      helpline [ 3 ] = 808 ; 
      helpline [ 2 ] = 809 ; 
      helpline [ 1 ] = 810 ; 
      helpline [ 0 ] = 811 ; 
    } 
    putgetflusherror ( 0 ) ; 
    curx = 0 ; 
    cury = 0 ; 
  } 
  else {
      
    p = mem [ curexp + 1 ] .cint ; 
    if ( mem [ p ] .hhfield .b0 == 16 ) 
    curx = mem [ p + 1 ] .cint ; 
    else {
	
      disperr ( p , 812 ) ; 
      {
	helpptr = 5 ; 
	helpline [ 4 ] = 813 ; 
	helpline [ 3 ] = 808 ; 
	helpline [ 2 ] = 809 ; 
	helpline [ 1 ] = 810 ; 
	helpline [ 0 ] = 811 ; 
      } 
      putgeterror () ; 
      recyclevalue ( p ) ; 
      curx = 0 ; 
    } 
    if ( mem [ p + 2 ] .hhfield .b0 == 16 ) 
    cury = mem [ p + 3 ] .cint ; 
    else {
	
      disperr ( p + 2 , 814 ) ; 
      {
	helpptr = 5 ; 
	helpline [ 4 ] = 815 ; 
	helpline [ 3 ] = 808 ; 
	helpline [ 2 ] = 809 ; 
	helpline [ 1 ] = 810 ; 
	helpline [ 0 ] = 811 ; 
      } 
      putgeterror () ; 
      recyclevalue ( p + 2 ) ; 
      cury = 0 ; 
    } 
    flushcurexp ( 0 ) ; 
  } 
} 
halfword newknot ( ) 
{register halfword Result; halfword q  ; 
  q = getnode ( 7 ) ; 
  mem [ q ] .hhfield .b0 = 0 ; 
  mem [ q ] .hhfield .b1 = 0 ; 
  mem [ q ] .hhfield .v.RH = q ; 
  knownpair () ; 
  mem [ q + 1 ] .cint = curx ; 
  mem [ q + 2 ] .cint = cury ; 
  Result = q ; 
  return(Result) ; 
} 
smallnumber scandirection ( ) 
{register smallnumber Result; schar t  ; 
  scaled x  ; 
  getxnext () ; 
  if ( curcmd == 60 ) 
  {
    getxnext () ; 
    scanexpression () ; 
    if ( ( curtype != 16 ) || ( curexp < 0 ) ) 
    {
      disperr ( 0 , 818 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 819 ; 
      } 
      putgetflusherror ( 65536L ) ; 
    } 
    t = 3 ; 
  } 
  else {
      
    scanexpression () ; 
    if ( curtype > 14 ) 
    {
      if ( curtype != 16 ) 
      {
	disperr ( 0 , 812 ) ; 
	{
	  helpptr = 5 ; 
	  helpline [ 4 ] = 813 ; 
	  helpline [ 3 ] = 808 ; 
	  helpline [ 2 ] = 809 ; 
	  helpline [ 1 ] = 810 ; 
	  helpline [ 0 ] = 811 ; 
	} 
	putgetflusherror ( 0 ) ; 
      } 
      x = curexp ; 
      if ( curcmd != 82 ) 
      {
	missingerr ( 44 ) ; 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 820 ; 
	  helpline [ 0 ] = 821 ; 
	} 
	backerror () ; 
      } 
      getxnext () ; 
      scanexpression () ; 
      if ( curtype != 16 ) 
      {
	disperr ( 0 , 814 ) ; 
	{
	  helpptr = 5 ; 
	  helpline [ 4 ] = 815 ; 
	  helpline [ 3 ] = 808 ; 
	  helpline [ 2 ] = 809 ; 
	  helpline [ 1 ] = 810 ; 
	  helpline [ 0 ] = 811 ; 
	} 
	putgetflusherror ( 0 ) ; 
      } 
      cury = curexp ; 
      curx = x ; 
    } 
    else knownpair () ; 
    if ( ( curx == 0 ) && ( cury == 0 ) ) 
    t = 4 ; 
    else {
	
      t = 2 ; 
      curexp = narg ( curx , cury ) ; 
    } 
  } 
  if ( curcmd != 65 ) 
  {
    missingerr ( 125 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 816 ; 
      helpline [ 1 ] = 817 ; 
      helpline [ 0 ] = 695 ; 
    } 
    backerror () ; 
  } 
  getxnext () ; 
  Result = t ; 
  return(Result) ; 
} 
void zdonullary ( c ) 
quarterword c ; 
{integer k  ; 
  {
    if ( aritherror ) 
    cleararith () ; 
  } 
  if ( internal [ 7 ] > 131072L ) 
  showcmdmod ( 33 , c ) ; 
  switch ( c ) 
  {case 30 : 
  case 31 : 
    {
      curtype = 2 ; 
      curexp = c ; 
    } 
    break ; 
  case 32 : 
    {
      curtype = 11 ; 
      curexp = getnode ( 6 ) ; 
      initedges ( curexp ) ; 
    } 
    break ; 
  case 33 : 
    {
      curtype = 6 ; 
      curexp = 3 ; 
    } 
    break ; 
  case 37 : 
    {
      curtype = 16 ; 
      curexp = normrand () ; 
    } 
    break ; 
  case 36 : 
    {
      curtype = 8 ; 
      curexp = getnode ( 7 ) ; 
      mem [ curexp ] .hhfield .b0 = 4 ; 
      mem [ curexp ] .hhfield .b1 = 4 ; 
      mem [ curexp ] .hhfield .v.RH = curexp ; 
      mem [ curexp + 1 ] .cint = 0 ; 
      mem [ curexp + 2 ] .cint = 0 ; 
      mem [ curexp + 3 ] .cint = 65536L ; 
      mem [ curexp + 4 ] .cint = 0 ; 
      mem [ curexp + 5 ] .cint = 0 ; 
      mem [ curexp + 6 ] .cint = 65536L ; 
    } 
    break ; 
  case 34 : 
    {
      if ( jobname == 0 ) 
      openlogfile () ; 
      curtype = 4 ; 
      curexp = jobname ; 
    } 
    break ; 
  case 35 : 
    {
      if ( interaction <= 1 ) 
      fatalerror ( 832 ) ; 
      beginfilereading () ; 
      curinput .namefield = 1 ; 
      {
	; 
	print ( 283 ) ; 
	terminput () ; 
      } 
      {
	if ( poolptr + last - curinput .startfield > maxpoolptr ) 
	{
	  if ( poolptr + last - curinput .startfield > poolsize ) 
	  overflow ( 257 , poolsize - initpoolptr ) ; 
	  maxpoolptr = poolptr + last - curinput .startfield ; 
	} 
      } 
      {register integer for_end; k = curinput .startfield ; for_end = last - 
      1 ; if ( k <= for_end) do 
	{
	  strpool [ poolptr ] = buffer [ k ] ; 
	  incr ( poolptr ) ; 
	} 
      while ( k++ < for_end ) ; } 
      endfilereading () ; 
      curtype = 4 ; 
      curexp = makestring () ; 
    } 
    break ; 
  } 
  {
    if ( aritherror ) 
    cleararith () ; 
  } 
} 
boolean znicepair ( p , t ) 
integer p ; 
quarterword t ; 
{/* 10 */ register boolean Result; if ( t == 14 ) 
  {
    p = mem [ p + 1 ] .cint ; 
    if ( mem [ p ] .hhfield .b0 == 16 ) 
    if ( mem [ p + 2 ] .hhfield .b0 == 16 ) 
    {
      Result = true ; 
      goto lab10 ; 
    } 
  } 
  Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
void zprintknownorunknowntype ( t , v ) 
smallnumber t ; 
integer v ; 
{printchar ( 40 ) ; 
  if ( t < 17 ) 
  if ( t != 14 ) 
  printtype ( t ) ; 
  else if ( nicepair ( v , 14 ) ) 
  print ( 335 ) ; 
  else print ( 833 ) ; 
  else print ( 834 ) ; 
  printchar ( 41 ) ; 
} 
void zbadunary ( c ) 
quarterword c ; 
{disperr ( 0 , 835 ) ; 
  printop ( c ) ; 
  printknownorunknowntype ( curtype , curexp ) ; 
  {
    helpptr = 3 ; 
    helpline [ 2 ] = 836 ; 
    helpline [ 1 ] = 837 ; 
    helpline [ 0 ] = 838 ; 
  } 
  putgeterror () ; 
} 
void znegatedeplist ( p ) 
halfword p ; 
{/* 10 */ while ( true ) {
    
    mem [ p + 1 ] .cint = - (integer) mem [ p + 1 ] .cint ; 
    if ( mem [ p ] .hhfield .lhfield == 0 ) 
    goto lab10 ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  lab10: ; 
} 
void pairtopath ( ) 
{curexp = newknot () ; 
  curtype = 9 ; 
} 
void ztakepart ( c ) 
quarterword c ; 
{halfword p  ; 
  p = mem [ curexp + 1 ] .cint ; 
  mem [ 18 ] .cint = p ; 
  mem [ 17 ] .hhfield .b0 = curtype ; 
  mem [ p ] .hhfield .v.RH = 17 ; 
  freenode ( curexp , 2 ) ; 
  makeexpcopy ( p + 2 * ( c - 53 ) ) ; 
  recyclevalue ( 17 ) ; 
} 
void zstrtonum ( c ) 
quarterword c ; 
{integer n  ; 
  ASCIIcode m  ; 
  poolpointer k  ; 
  schar b  ; 
  boolean badchar  ; 
  if ( c == 49 ) 
  if ( ( strstart [ curexp + 1 ] - strstart [ curexp ] ) == 0 ) 
  n = -1 ; 
  else n = strpool [ strstart [ curexp ] ] ; 
  else {
      
    if ( c == 47 ) 
    b = 8 ; 
    else b = 16 ; 
    n = 0 ; 
    badchar = false ; 
    {register integer for_end; k = strstart [ curexp ] ; for_end = strstart [ 
    curexp + 1 ] - 1 ; if ( k <= for_end) do 
      {
	m = strpool [ k ] ; 
	if ( ( m >= 48 ) && ( m <= 57 ) ) 
	m = m - 48 ; 
	else if ( ( m >= 65 ) && ( m <= 70 ) ) 
	m = m - 55 ; 
	else if ( ( m >= 97 ) && ( m <= 102 ) ) 
	m = m - 87 ; 
	else {
	    
	  badchar = true ; 
	  m = 0 ; 
	} 
	if ( m >= b ) 
	{
	  badchar = true ; 
	  m = 0 ; 
	} 
	if ( n < 32768L / b ) 
	n = n * b + m ; 
	else n = 32767 ; 
      } 
    while ( k++ < for_end ) ; } 
    if ( badchar ) 
    {
      disperr ( 0 , 840 ) ; 
      if ( c == 47 ) 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 841 ; 
      } 
      else {
	  
	helpptr = 1 ; 
	helpline [ 0 ] = 842 ; 
      } 
      putgeterror () ; 
    } 
    if ( n > 4095 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 261 ) ; 
	print ( 843 ) ; 
      } 
      printint ( n ) ; 
      printchar ( 41 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 844 ; 
      } 
      putgeterror () ; 
    } 
  } 
  flushcurexp ( n * 65536L ) ; 
} 
scaled pathlength ( ) 
{register scaled Result; scaled n  ; 
  halfword p  ; 
  p = curexp ; 
  if ( mem [ p ] .hhfield .b0 == 0 ) 
  n = -65536L ; 
  else n = 0 ; 
  do {
      p = mem [ p ] .hhfield .v.RH ; 
    n = n + 65536L ; 
  } while ( ! ( p == curexp ) ) ; 
  Result = n ; 
  return(Result) ; 
} 
void ztestknown ( c ) 
quarterword c ; 
{/* 30 */ schar b  ; 
  halfword p, q  ; 
  b = 31 ; 
  switch ( curtype ) 
  {case 1 : 
  case 2 : 
  case 4 : 
  case 6 : 
  case 8 : 
  case 9 : 
  case 11 : 
  case 16 : 
    b = 30 ; 
    break ; 
  case 13 : 
  case 14 : 
    {
      p = mem [ curexp + 1 ] .cint ; 
      q = p + bignodesize [ curtype ] ; 
      do {
	  q = q - 2 ; 
	if ( mem [ q ] .hhfield .b0 != 16 ) 
	goto lab30 ; 
      } while ( ! ( q == p ) ) ; 
      b = 30 ; 
      lab30: ; 
    } 
    break ; 
    default: 
    ; 
    break ; 
  } 
  if ( c == 39 ) 
  flushcurexp ( b ) ; 
  else flushcurexp ( 61 - b ) ; 
  curtype = 2 ; 
} 
void zdounary ( c ) 
quarterword c ; 
{halfword p, q  ; 
  integer x  ; 
  {
    if ( aritherror ) 
    cleararith () ; 
  } 
  if ( internal [ 7 ] > 131072L ) 
  {
    begindiagnostic () ; 
    printnl ( 123 ) ; 
    printop ( c ) ; 
    printchar ( 40 ) ; 
    printexp ( 0 , 0 ) ; 
    print ( 839 ) ; 
    enddiagnostic ( false ) ; 
  } 
  switch ( c ) 
  {case 69 : 
    if ( curtype < 14 ) 
    if ( curtype != 11 ) 
    badunary ( 69 ) ; 
    break ; 
  case 70 : 
    switch ( curtype ) 
    {case 14 : 
    case 19 : 
      {
	q = curexp ; 
	makeexpcopy ( q ) ; 
	if ( curtype == 17 ) 
	negatedeplist ( mem [ curexp + 1 ] .hhfield .v.RH ) ; 
	else if ( curtype == 14 ) 
	{
	  p = mem [ curexp + 1 ] .cint ; 
	  if ( mem [ p ] .hhfield .b0 == 16 ) 
	  mem [ p + 1 ] .cint = - (integer) mem [ p + 1 ] .cint ; 
	  else negatedeplist ( mem [ p + 1 ] .hhfield .v.RH ) ; 
	  if ( mem [ p + 2 ] .hhfield .b0 == 16 ) 
	  mem [ p + 3 ] .cint = - (integer) mem [ p + 3 ] .cint ; 
	  else negatedeplist ( mem [ p + 3 ] .hhfield .v.RH ) ; 
	} 
	recyclevalue ( q ) ; 
	freenode ( q , 2 ) ; 
      } 
      break ; 
    case 17 : 
    case 18 : 
      negatedeplist ( mem [ curexp + 1 ] .hhfield .v.RH ) ; 
      break ; 
    case 16 : 
      curexp = - (integer) curexp ; 
      break ; 
    case 11 : 
      negateedges ( curexp ) ; 
      break ; 
      default: 
      badunary ( 70 ) ; 
      break ; 
    } 
    break ; 
  case 41 : 
    if ( curtype != 2 ) 
    badunary ( 41 ) ; 
    else curexp = 61 - curexp ; 
    break ; 
  case 59 : 
  case 60 : 
  case 61 : 
  case 62 : 
  case 63 : 
  case 64 : 
  case 65 : 
  case 38 : 
  case 66 : 
    if ( curtype != 16 ) 
    badunary ( c ) ; 
    else switch ( c ) 
    {case 59 : 
      curexp = squarert ( curexp ) ; 
      break ; 
    case 60 : 
      curexp = mexp ( curexp ) ; 
      break ; 
    case 61 : 
      curexp = mlog ( curexp ) ; 
      break ; 
    case 62 : 
    case 63 : 
      {
	nsincos ( ( curexp % 23592960L ) * 16 ) ; 
	if ( c == 62 ) 
	curexp = roundfraction ( nsin ) ; 
	else curexp = roundfraction ( ncos ) ; 
      } 
      break ; 
    case 64 : 
      curexp = floorscaled ( curexp ) ; 
      break ; 
    case 65 : 
      curexp = unifrand ( curexp ) ; 
      break ; 
    case 38 : 
      {
	if ( odd ( roundunscaled ( curexp ) ) ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	curtype = 2 ; 
      } 
      break ; 
    case 66 : 
      {
	curexp = roundunscaled ( curexp ) % 256 ; 
	if ( curexp < 0 ) 
	curexp = curexp + 256 ; 
	if ( charexists [ curexp ] ) 
	curexp = 30 ; 
	else curexp = 31 ; 
	curtype = 2 ; 
      } 
      break ; 
    } 
    break ; 
  case 67 : 
    if ( nicepair ( curexp , curtype ) ) 
    {
      p = mem [ curexp + 1 ] .cint ; 
      x = narg ( mem [ p + 1 ] .cint , mem [ p + 3 ] .cint ) ; 
      if ( x >= 0 ) 
      flushcurexp ( ( x + 8 ) / 16 ) ; 
      else flushcurexp ( - (integer) ( ( - (integer) x + 8 ) / 16 ) ) ; 
    } 
    else badunary ( 67 ) ; 
    break ; 
  case 53 : 
  case 54 : 
    if ( ( curtype <= 14 ) && ( curtype >= 13 ) ) 
    takepart ( c ) ; 
    else badunary ( c ) ; 
    break ; 
  case 55 : 
  case 56 : 
  case 57 : 
  case 58 : 
    if ( curtype == 13 ) 
    takepart ( c ) ; 
    else badunary ( c ) ; 
    break ; 
  case 50 : 
    if ( curtype != 16 ) 
    badunary ( 50 ) ; 
    else {
	
      curexp = roundunscaled ( curexp ) % 256 ; 
      curtype = 4 ; 
      if ( curexp < 0 ) 
      curexp = curexp + 256 ; 
      if ( ( strstart [ curexp + 1 ] - strstart [ curexp ] ) != 1 ) 
      {
	{
	  if ( poolptr + 1 > maxpoolptr ) 
	  {
	    if ( poolptr + 1 > poolsize ) 
	    overflow ( 257 , poolsize - initpoolptr ) ; 
	    maxpoolptr = poolptr + 1 ; 
	  } 
	} 
	{
	  strpool [ poolptr ] = curexp ; 
	  incr ( poolptr ) ; 
	} 
	curexp = makestring () ; 
      } 
    } 
    break ; 
  case 42 : 
    if ( curtype != 16 ) 
    badunary ( 42 ) ; 
    else {
	
      oldsetting = selector ; 
      selector = 5 ; 
      printscaled ( curexp ) ; 
      curexp = makestring () ; 
      selector = oldsetting ; 
      curtype = 4 ; 
    } 
    break ; 
  case 47 : 
  case 48 : 
  case 49 : 
    if ( curtype != 4 ) 
    badunary ( c ) ; 
    else strtonum ( c ) ; 
    break ; 
  case 51 : 
    if ( curtype == 4 ) 
    flushcurexp ( ( strstart [ curexp + 1 ] - strstart [ curexp ] ) * 65536L ) 
    ; 
    else if ( curtype == 9 ) 
    flushcurexp ( pathlength () ) ; 
    else if ( curtype == 16 ) 
    curexp = abs ( curexp ) ; 
    else if ( nicepair ( curexp , curtype ) ) 
    flushcurexp ( pythadd ( mem [ mem [ curexp + 1 ] .cint + 1 ] .cint , mem [ 
    mem [ curexp + 1 ] .cint + 3 ] .cint ) ) ; 
    else badunary ( c ) ; 
    break ; 
  case 52 : 
    if ( curtype == 14 ) 
    flushcurexp ( 0 ) ; 
    else if ( curtype != 9 ) 
    badunary ( 52 ) ; 
    else if ( mem [ curexp ] .hhfield .b0 == 0 ) 
    flushcurexp ( 0 ) ; 
    else {
	
      curpen = 3 ; 
      curpathtype = 1 ; 
      curexp = makespec ( curexp , -1879080960L , 0 ) ; 
      flushcurexp ( turningnumber * 65536L ) ; 
    } 
    break ; 
  case 2 : 
    {
      if ( ( curtype >= 2 ) && ( curtype <= 3 ) ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 4 : 
    {
      if ( ( curtype >= 4 ) && ( curtype <= 5 ) ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 6 : 
    {
      if ( ( curtype >= 6 ) && ( curtype <= 8 ) ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 9 : 
    {
      if ( ( curtype >= 9 ) && ( curtype <= 10 ) ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 11 : 
    {
      if ( ( curtype >= 11 ) && ( curtype <= 12 ) ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 13 : 
  case 14 : 
    {
      if ( curtype == c ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 15 : 
    {
      if ( ( curtype >= 16 ) && ( curtype <= 19 ) ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 39 : 
  case 40 : 
    testknown ( c ) ; 
    break ; 
  case 68 : 
    {
      if ( curtype != 9 ) 
      flushcurexp ( 31 ) ; 
      else if ( mem [ curexp ] .hhfield .b0 != 0 ) 
      flushcurexp ( 30 ) ; 
      else flushcurexp ( 31 ) ; 
      curtype = 2 ; 
    } 
    break ; 
  case 45 : 
    {
      if ( curtype == 14 ) 
      pairtopath () ; 
      if ( curtype == 9 ) 
      curtype = 8 ; 
      else badunary ( 45 ) ; 
    } 
    break ; 
  case 44 : 
    {
      if ( curtype == 8 ) 
      materializepen () ; 
      if ( curtype != 6 ) 
      badunary ( 44 ) ; 
      else {
	  
	flushcurexp ( makepath ( curexp ) ) ; 
	curtype = 9 ; 
      } 
    } 
    break ; 
  case 46 : 
    if ( curtype != 11 ) 
    badunary ( 46 ) ; 
    else flushcurexp ( totalweight ( curexp ) ) ; 
    break ; 
  case 43 : 
    if ( curtype == 9 ) 
    {
      p = htapypoc ( curexp ) ; 
      if ( mem [ p ] .hhfield .b1 == 0 ) 
      p = mem [ p ] .hhfield .v.RH ; 
      tossknotlist ( curexp ) ; 
      curexp = p ; 
    } 
    else if ( curtype == 14 ) 
    pairtopath () ; 
    else badunary ( 43 ) ; 
    break ; 
  } 
  {
    if ( aritherror ) 
    cleararith () ; 
  } 
} 
