#define EXTERN extern
#include "texd.h"

void scanint ( ) 
{/* 30 */ scanint_regmem 
  boolean negative  ; 
  integer m  ; 
  smallnumber d  ; 
  boolean vacuous  ; 
  boolean OKsofar  ; 
  radix = 0 ; 
  OKsofar = true ; 
  negative = false ; 
  do {
      do { getxtoken () ; 
    } while ( ! ( curcmd != 10 ) ) ; 
    if ( curtok == 3117 ) 
    {
      negative = ! negative ; 
      curtok = 3115 ; 
    } 
  } while ( ! ( curtok != 3115 ) ) ; 
  if ( curtok == 3168 ) 
  {
    gettoken () ; 
    if ( curtok < 4095 ) 
    {
      curval = curchr ; 
      if ( curcmd <= 2 ) 
      if ( curcmd == 2 ) 
      incr ( alignstate ) ; 
      else decr ( alignstate ) ; 
    } 
    else if ( curtok < 4352 ) 
    curval = curtok - 4096 ; 
    else curval = curtok - 4352 ; 
    if ( curval > 255 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 694 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 695 ; 
	helpline [ 0 ] = 696 ; 
      } 
      curval = 48 ; 
      backerror () ; 
    } 
    else {
	
      getxtoken () ; 
      if ( curcmd != 10 ) 
      backinput () ; 
    } 
  } 
  else if ( ( curcmd >= 68 ) && ( curcmd <= 89 ) ) 
  scansomethinginternal ( 0 , false ) ; 
  else {
      
    radix = 10 ; 
    m = 214748364L ; 
    if ( curtok == 3111 ) 
    {
      radix = 8 ; 
      m = 268435456L ; 
      getxtoken () ; 
    } 
    else if ( curtok == 3106 ) 
    {
      radix = 16 ; 
      m = 134217728L ; 
      getxtoken () ; 
    } 
    vacuous = true ; 
    curval = 0 ; 
    while ( true ) {
	
      if ( ( curtok < 3120 + radix ) && ( curtok >= 3120 ) && ( curtok <= 3129 
      ) ) 
      d = curtok - 3120 ; 
      else if ( radix == 16 ) 
      if ( ( curtok <= 2886 ) && ( curtok >= 2881 ) ) 
      d = curtok - 2871 ; 
      else if ( ( curtok <= 3142 ) && ( curtok >= 3137 ) ) 
      d = curtok - 3127 ; 
      else goto lab30 ; 
      else goto lab30 ; 
      vacuous = false ; 
      if ( ( curval >= m ) && ( ( curval > m ) || ( d > 7 ) || ( radix != 10 ) 
      ) ) 
      {
	if ( OKsofar ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 697 ) ; 
	  } 
	  {
	    helpptr = 2 ; 
	    helpline [ 1 ] = 698 ; 
	    helpline [ 0 ] = 699 ; 
	  } 
	  error () ; 
	  curval = 2147483647L ; 
	  OKsofar = false ; 
	} 
      } 
      else curval = curval * radix + d ; 
      getxtoken () ; 
    } 
    lab30: ; 
    if ( vacuous ) 
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
    } 
    else if ( curcmd != 10 ) 
    backinput () ; 
  } 
  if ( negative ) 
  curval = - (integer) curval ; 
} 
void zscandimen ( mu , inf , shortcut ) 
boolean mu ; 
boolean inf ; 
boolean shortcut ; 
{/* 30 31 32 40 45 88 89 */ scandimen_regmem 
  boolean negative  ; 
  integer f  ; 
  integer num, denom  ; 
  smallnumber k, kk  ; 
  halfword p, q  ; 
  scaled v  ; 
  integer savecurval  ; 
  f = 0 ; 
  aritherror = false ; 
  curorder = 0 ; 
  negative = false ; 
  if ( ! shortcut ) 
  {
    negative = false ; 
    do {
	do { getxtoken () ; 
      } while ( ! ( curcmd != 10 ) ) ; 
      if ( curtok == 3117 ) 
      {
	negative = ! negative ; 
	curtok = 3115 ; 
      } 
    } while ( ! ( curtok != 3115 ) ) ; 
    if ( ( curcmd >= 68 ) && ( curcmd <= 89 ) ) 
    if ( mu ) 
    {
      scansomethinginternal ( 3 , false ) ; 
      if ( curvallevel >= 2 ) 
      {
	v = mem [ curval + 1 ] .cint ; 
	deleteglueref ( curval ) ; 
	curval = v ; 
      } 
      if ( curvallevel == 3 ) 
      goto lab89 ; 
      if ( curvallevel != 0 ) 
      muerror () ; 
    } 
    else {
	
      scansomethinginternal ( 1 , false ) ; 
      if ( curvallevel == 1 ) 
      goto lab89 ; 
    } 
    else {
	
      backinput () ; 
      if ( curtok == 3116 ) 
      curtok = 3118 ; 
      if ( curtok != 3118 ) 
      scanint () ; 
      else {
	  
	radix = 10 ; 
	curval = 0 ; 
      } 
      if ( curtok == 3116 ) 
      curtok = 3118 ; 
      if ( ( radix == 10 ) && ( curtok == 3118 ) ) 
      {
	k = 0 ; 
	p = 0 ; 
	gettoken () ; 
	while ( true ) {
	    
	  getxtoken () ; 
	  if ( ( curtok > 3129 ) || ( curtok < 3120 ) ) 
	  goto lab31 ; 
	  if ( k < 17 ) 
	  {
	    q = getavail () ; 
	    mem [ q ] .hh .v.RH = p ; 
	    mem [ q ] .hh .v.LH = curtok - 3120 ; 
	    p = q ; 
	    incr ( k ) ; 
	  } 
	} 
	lab31: {
	    register integer for_end; kk = k ; for_end = 1 ; if ( kk >= 
	for_end) do 
	  {
	    dig [ kk - 1 ] = mem [ p ] .hh .v.LH ; 
	    q = p ; 
	    p = mem [ p ] .hh .v.RH ; 
	    {
	      mem [ q ] .hh .v.RH = avail ; 
	      avail = q ; 
	;
#ifdef STAT
	      decr ( dynused ) ; 
#endif /* STAT */
	    } 
	  } 
	while ( kk-- > for_end ) ; } 
	f = rounddecimals ( k ) ; 
	if ( curcmd != 10 ) 
	backinput () ; 
      } 
    } 
  } 
  if ( curval < 0 ) 
  {
    negative = ! negative ; 
    curval = - (integer) curval ; 
  } 
  if ( inf ) 
  if ( scankeyword ( 309 ) ) 
  {
    curorder = 1 ; 
    while ( scankeyword ( 108 ) ) {
	
      if ( curorder == 3 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 701 ) ; 
	} 
	print ( 702 ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 703 ; 
	} 
	error () ; 
      } 
      else incr ( curorder ) ; 
    } 
    goto lab88 ; 
  } 
  savecurval = curval ; 
  do {
      getxtoken () ; 
  } while ( ! ( curcmd != 10 ) ) ; 
  if ( ( curcmd < 68 ) || ( curcmd > 89 ) ) 
  backinput () ; 
  else {
      
    if ( mu ) 
    {
      scansomethinginternal ( 3 , false ) ; 
      if ( curvallevel >= 2 ) 
      {
	v = mem [ curval + 1 ] .cint ; 
	deleteglueref ( curval ) ; 
	curval = v ; 
      } 
      if ( curvallevel != 3 ) 
      muerror () ; 
    } 
    else scansomethinginternal ( 1 , false ) ; 
    v = curval ; 
    goto lab40 ; 
  } 
  if ( mu ) 
  goto lab45 ; 
  if ( scankeyword ( 704 ) ) 
  v = ( fontinfo [ 6 + parambase [ eqtb [ 11334 ] .hh .v.RH ] ] .cint ) ; 
  else if ( scankeyword ( 705 ) ) 
  v = ( fontinfo [ 5 + parambase [ eqtb [ 11334 ] .hh .v.RH ] ] .cint ) ; 
  else goto lab45 ; 
  {
    getxtoken () ; 
    if ( curcmd != 10 ) 
    backinput () ; 
  } 
  lab40: curval = multandadd ( savecurval , v , xnoverd ( v , f , 65536L ) , 
  1073741823L ) ; 
  goto lab89 ; 
  lab45: ; 
  if ( mu ) 
  if ( scankeyword ( 334 ) ) 
  goto lab88 ; 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 701 ) ; 
    } 
    print ( 706 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 707 ; 
      helpline [ 2 ] = 708 ; 
      helpline [ 1 ] = 709 ; 
      helpline [ 0 ] = 710 ; 
    } 
    error () ; 
    goto lab88 ; 
  } 
  if ( scankeyword ( 700 ) ) 
  {
    preparemag () ; 
    if ( eqtb [ 12680 ] .cint != 1000 ) 
    {
      curval = xnoverd ( curval , 1000 , eqtb [ 12680 ] .cint ) ; 
      f = ( 1000 * f + 65536L * texremainder ) / eqtb [ 12680 ] .cint ; 
      curval = curval + ( f / 65536L ) ; 
      f = f % 65536L ; 
    } 
  } 
  if ( scankeyword ( 393 ) ) 
  goto lab88 ; 
  if ( scankeyword ( 711 ) ) 
  {
    num = 7227 ; 
    denom = 100 ; 
  } 
  else if ( scankeyword ( 712 ) ) 
  {
    num = 12 ; 
    denom = 1 ; 
  } 
  else if ( scankeyword ( 713 ) ) 
  {
    num = 7227 ; 
    denom = 254 ; 
  } 
  else if ( scankeyword ( 714 ) ) 
  {
    num = 7227 ; 
    denom = 2540 ; 
  } 
  else if ( scankeyword ( 715 ) ) 
  {
    num = 7227 ; 
    denom = 7200 ; 
  } 
  else if ( scankeyword ( 716 ) ) 
  {
    num = 1238 ; 
    denom = 1157 ; 
  } 
  else if ( scankeyword ( 717 ) ) 
  {
    num = 14856 ; 
    denom = 1157 ; 
  } 
  else if ( scankeyword ( 718 ) ) 
  goto lab30 ; 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 701 ) ; 
    } 
    print ( 719 ) ; 
    {
      helpptr = 6 ; 
      helpline [ 5 ] = 720 ; 
      helpline [ 4 ] = 721 ; 
      helpline [ 3 ] = 722 ; 
      helpline [ 2 ] = 708 ; 
      helpline [ 1 ] = 709 ; 
      helpline [ 0 ] = 710 ; 
    } 
    error () ; 
    goto lab32 ; 
  } 
  curval = xnoverd ( curval , num , denom ) ; 
  f = ( num * f + 65536L * texremainder ) / denom ; 
  curval = curval + ( f / 65536L ) ; 
  f = f % 65536L ; 
  lab32: ; 
  lab88: if ( curval >= 16384 ) 
  aritherror = true ; 
  else curval = curval * 65536L + f ; 
  lab30: ; 
  {
    getxtoken () ; 
    if ( curcmd != 10 ) 
    backinput () ; 
  } 
  lab89: if ( aritherror || ( abs ( curval ) >= 1073741824L ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 723 ) ; 
    } 
    {
      helpptr = 2 ; 
      helpline [ 1 ] = 724 ; 
      helpline [ 0 ] = 725 ; 
    } 
    error () ; 
    curval = 1073741823L ; 
    aritherror = false ; 
  } 
  if ( negative ) 
  curval = - (integer) curval ; 
} 
void zscanglue ( level ) 
smallnumber level ; 
{/* 10 */ scanglue_regmem 
  boolean negative  ; 
  halfword q  ; 
  boolean mu  ; 
  mu = ( level == 3 ) ; 
  negative = false ; 
  do {
      do { getxtoken () ; 
    } while ( ! ( curcmd != 10 ) ) ; 
    if ( curtok == 3117 ) 
    {
      negative = ! negative ; 
      curtok = 3115 ; 
    } 
  } while ( ! ( curtok != 3115 ) ) ; 
  if ( ( curcmd >= 68 ) && ( curcmd <= 89 ) ) 
  {
    scansomethinginternal ( level , negative ) ; 
    if ( curvallevel >= 2 ) 
    {
      if ( curvallevel != level ) 
      muerror () ; 
      return ; 
    } 
    if ( curvallevel == 0 ) 
    scandimen ( mu , false , true ) ; 
    else if ( level == 3 ) 
    muerror () ; 
  } 
  else {
      
    backinput () ; 
    scandimen ( mu , false , false ) ; 
    if ( negative ) 
    curval = - (integer) curval ; 
  } 
  q = newspec ( 0 ) ; 
  mem [ q + 1 ] .cint = curval ; 
  if ( scankeyword ( 726 ) ) 
  {
    scandimen ( mu , true , false ) ; 
    mem [ q + 2 ] .cint = curval ; 
    mem [ q ] .hh.b0 = curorder ; 
  } 
  if ( scankeyword ( 727 ) ) 
  {
    scandimen ( mu , true , false ) ; 
    mem [ q + 3 ] .cint = curval ; 
    mem [ q ] .hh.b1 = curorder ; 
  } 
  curval = q ; 
} 
halfword scanrulespec ( ) 
{/* 21 */ register halfword Result; scanrulespec_regmem 
  halfword q  ; 
  q = newrule () ; 
  if ( curcmd == 35 ) 
  mem [ q + 1 ] .cint = 26214 ; 
  else {
      
    mem [ q + 3 ] .cint = 26214 ; 
    mem [ q + 2 ] .cint = 0 ; 
  } 
  lab21: if ( scankeyword ( 728 ) ) 
  {
    scandimen ( false , false , false ) ; 
    mem [ q + 1 ] .cint = curval ; 
    goto lab21 ; 
  } 
  if ( scankeyword ( 729 ) ) 
  {
    scandimen ( false , false , false ) ; 
    mem [ q + 3 ] .cint = curval ; 
    goto lab21 ; 
  } 
  if ( scankeyword ( 730 ) ) 
  {
    scandimen ( false , false , false ) ; 
    mem [ q + 2 ] .cint = curval ; 
    goto lab21 ; 
  } 
  Result = q ; 
  return(Result) ; 
} 
halfword zstrtoks ( b ) 
poolpointer b ; 
{register halfword Result; strtoks_regmem 
  halfword p  ; 
  halfword q  ; 
  halfword t  ; 
  poolpointer k  ; 
  {
    if ( poolptr + 1 > poolsize ) 
    overflow ( 257 , poolsize - initpoolptr ) ; 
  } 
  p = memtop - 3 ; 
  mem [ p ] .hh .v.RH = 0 ; 
  k = b ; 
  while ( k < poolptr ) {
      
    t = strpool [ k ] ; 
    if ( t == 32 ) 
    t = 2592 ; 
    else t = 3072 + t ; 
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
      mem [ q ] .hh .v.LH = t ; 
      p = q ; 
    } 
    incr ( k ) ; 
  } 
  poolptr = b ; 
  Result = p ; 
  return(Result) ; 
} 
halfword thetoks ( ) 
{register halfword Result; thetoks_regmem 
  schar oldsetting  ; 
  halfword p, q, r  ; 
  poolpointer b  ; 
  getxtoken () ; 
  scansomethinginternal ( 5 , false ) ; 
  if ( curvallevel >= 4 ) 
  {
    p = memtop - 3 ; 
    mem [ p ] .hh .v.RH = 0 ; 
    if ( curvallevel == 4 ) 
    {
      q = getavail () ; 
      mem [ p ] .hh .v.RH = q ; 
      mem [ q ] .hh .v.LH = 4095 + curval ; 
      p = q ; 
    } 
    else if ( curval != 0 ) 
    {
      r = mem [ curval ] .hh .v.RH ; 
      while ( r != 0 ) {
	  
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
	  mem [ q ] .hh .v.LH = mem [ r ] .hh .v.LH ; 
	  p = q ; 
	} 
	r = mem [ r ] .hh .v.RH ; 
      } 
    } 
    Result = p ; 
  } 
  else {
      
    oldsetting = selector ; 
    selector = 21 ; 
    b = poolptr ; 
    switch ( curvallevel ) 
    {case 0 : 
      printint ( curval ) ; 
      break ; 
    case 1 : 
      {
	printscaled ( curval ) ; 
	print ( 393 ) ; 
      } 
      break ; 
    case 2 : 
      {
	printspec ( curval , 393 ) ; 
	deleteglueref ( curval ) ; 
      } 
      break ; 
    case 3 : 
      {
	printspec ( curval , 334 ) ; 
	deleteglueref ( curval ) ; 
      } 
      break ; 
    } 
    selector = oldsetting ; 
    Result = strtoks ( b ) ; 
  } 
  return(Result) ; 
} 
void insthetoks ( ) 
{insthetoks_regmem 
  mem [ memtop - 12 ] .hh .v.RH = thetoks () ; 
  begintokenlist ( mem [ memtop - 3 ] .hh .v.RH , 4 ) ; 
} 
void convtoks ( ) 
{convtoks_regmem 
  schar oldsetting  ; 
  schar c  ; 
  smallnumber savescannerstatus  ; 
  poolpointer b  ; 
  c = curchr ; 
  switch ( c ) 
  {case 0 : 
  case 1 : 
    scanint () ; 
    break ; 
  case 2 : 
  case 3 : 
    {
      savescannerstatus = scannerstatus ; 
      scannerstatus = 0 ; 
      gettoken () ; 
      scannerstatus = savescannerstatus ; 
    } 
    break ; 
  case 4 : 
    scanfontident () ; 
    break ; 
  case 5 : 
    if ( jobname == 0 ) 
    openlogfile () ; 
    break ; 
  } 
  oldsetting = selector ; 
  selector = 21 ; 
  b = poolptr ; 
  switch ( c ) 
  {case 0 : 
    printint ( curval ) ; 
    break ; 
  case 1 : 
    printromanint ( curval ) ; 
    break ; 
  case 2 : 
    if ( curcs != 0 ) 
    sprintcs ( curcs ) ; 
    else printchar ( curchr ) ; 
    break ; 
  case 3 : 
    printmeaning () ; 
    break ; 
  case 4 : 
    {
      print ( fontname [ curval ] ) ; 
      if ( fontsize [ curval ] != fontdsize [ curval ] ) 
      {
	print ( 737 ) ; 
	printscaled ( fontsize [ curval ] ) ; 
	print ( 393 ) ; 
      } 
    } 
    break ; 
  case 5 : 
    print ( jobname ) ; 
    break ; 
  } 
  selector = oldsetting ; 
  mem [ memtop - 12 ] .hh .v.RH = strtoks ( b ) ; 
  begintokenlist ( mem [ memtop - 3 ] .hh .v.RH , 4 ) ; 
} 
halfword zscantoks ( macrodef , xpand ) 
boolean macrodef ; 
boolean xpand ; 
{/* 40 30 31 32 */ register halfword Result; scantoks_regmem 
  halfword t  ; 
  halfword s  ; 
  halfword p  ; 
  halfword q  ; 
  halfword unbalance  ; 
  halfword hashbrace  ; 
  if ( macrodef ) 
  scannerstatus = 2 ; 
  else scannerstatus = 5 ; 
  warningindex = curcs ; 
  defref = getavail () ; 
  mem [ defref ] .hh .v.LH = 0 ; 
  p = defref ; 
  hashbrace = 0 ; 
  t = 3120 ; 
  if ( macrodef ) 
  {
    while ( true ) {
	
      gettoken () ; 
      if ( curtok < 768 ) 
      goto lab31 ; 
      if ( curcmd == 6 ) 
      {
	s = 3328 + curchr ; 
	gettoken () ; 
	if ( curcmd == 1 ) 
	{
	  hashbrace = curtok ; 
	  {
	    q = getavail () ; 
	    mem [ p ] .hh .v.RH = q ; 
	    mem [ q ] .hh .v.LH = curtok ; 
	    p = q ; 
	  } 
	  {
	    q = getavail () ; 
	    mem [ p ] .hh .v.RH = q ; 
	    mem [ q ] .hh .v.LH = 3584 ; 
	    p = q ; 
	  } 
	  goto lab30 ; 
	} 
	if ( t == 3129 ) 
	{
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 740 ) ; 
	  } 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 741 ; 
	  } 
	  error () ; 
	} 
	else {
	    
	  incr ( t ) ; 
	  if ( curtok != t ) 
	  {
	    {
	      if ( interaction == 3 ) 
	      ; 
	      printnl ( 262 ) ; 
	      print ( 742 ) ; 
	    } 
	    {
	      helpptr = 2 ; 
	      helpline [ 1 ] = 743 ; 
	      helpline [ 0 ] = 744 ; 
	    } 
	    backerror () ; 
	  } 
	  curtok = s ; 
	} 
      } 
      {
	q = getavail () ; 
	mem [ p ] .hh .v.RH = q ; 
	mem [ q ] .hh .v.LH = curtok ; 
	p = q ; 
      } 
    } 
    lab31: {
	
      q = getavail () ; 
      mem [ p ] .hh .v.RH = q ; 
      mem [ q ] .hh .v.LH = 3584 ; 
      p = q ; 
    } 
    if ( curcmd == 2 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 653 ) ; 
      } 
      incr ( alignstate ) ; 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 738 ; 
	helpline [ 0 ] = 739 ; 
      } 
      error () ; 
      goto lab40 ; 
    } 
    lab30: ; 
  } 
  else scanleftbrace () ; 
  unbalance = 1 ; 
  while ( true ) {
      
    if ( xpand ) 
    {
      while ( true ) {
	  
	getnext () ; 
	if ( curcmd <= 100 ) 
	goto lab32 ; 
	if ( curcmd != 109 ) 
	expand () ; 
	else {
	    
	  q = thetoks () ; 
	  if ( mem [ memtop - 3 ] .hh .v.RH != 0 ) 
	  {
	    mem [ p ] .hh .v.RH = mem [ memtop - 3 ] .hh .v.RH ; 
	    p = q ; 
	  } 
	} 
      } 
      lab32: xtoken () ; 
    } 
    else gettoken () ; 
    if ( curtok < 768 ) 
    if ( curcmd < 2 ) 
    incr ( unbalance ) ; 
    else {
	
      decr ( unbalance ) ; 
      if ( unbalance == 0 ) 
      goto lab40 ; 
    } 
    else if ( curcmd == 6 ) 
    if ( macrodef ) 
    {
      s = curtok ; 
      if ( xpand ) 
      getxtoken () ; 
      else gettoken () ; 
      if ( curcmd != 6 ) 
      if ( ( curtok <= 3120 ) || ( curtok > t ) ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 745 ) ; 
	} 
	sprintcs ( warningindex ) ; 
	{
	  helpptr = 3 ; 
	  helpline [ 2 ] = 746 ; 
	  helpline [ 1 ] = 747 ; 
	  helpline [ 0 ] = 748 ; 
	} 
	backerror () ; 
	curtok = s ; 
      } 
      else curtok = 1232 + curchr ; 
    } 
    {
      q = getavail () ; 
      mem [ p ] .hh .v.RH = q ; 
      mem [ q ] .hh .v.LH = curtok ; 
      p = q ; 
    } 
  } 
  lab40: scannerstatus = 0 ; 
  if ( hashbrace != 0 ) 
  {
    q = getavail () ; 
    mem [ p ] .hh .v.RH = q ; 
    mem [ q ] .hh .v.LH = hashbrace ; 
    p = q ; 
  } 
  Result = p ; 
  return(Result) ; 
} 
void zreadtoks ( n , r ) 
integer n ; 
halfword r ; 
{/* 30 */ readtoks_regmem 
  halfword p  ; 
  halfword q  ; 
  integer s  ; 
  smallnumber m  ; 
  scannerstatus = 2 ; 
  warningindex = r ; 
  defref = getavail () ; 
  mem [ defref ] .hh .v.LH = 0 ; 
  p = defref ; 
  {
    q = getavail () ; 
    mem [ p ] .hh .v.RH = q ; 
    mem [ q ] .hh .v.LH = 3584 ; 
    p = q ; 
  } 
  if ( ( n < 0 ) || ( n > 15 ) ) 
  m = 16 ; 
  else m = n ; 
  s = alignstate ; 
  alignstate = 1000000L ; 
  do {
      beginfilereading () ; 
    curinput .namefield = m + 1 ; 
    if ( readopen [ m ] == 2 ) 
    if ( interaction > 1 ) 
    if ( n < 0 ) 
    {
      ; 
      print ( 335 ) ; 
      terminput () ; 
    } 
    else {
	
      ; 
      println () ; 
      sprintcs ( r ) ; 
      {
	; 
	print ( 61 ) ; 
	terminput () ; 
      } 
      n = -1 ; 
    } 
    else fatalerror ( 749 ) ; 
    else if ( readopen [ m ] == 1 ) 
    if ( inputln ( readfile [ m ] , false ) ) 
    readopen [ m ] = 0 ; 
    else {
	
      aclose ( readfile [ m ] ) ; 
      readopen [ m ] = 2 ; 
    } 
    else {
	
      if ( ! inputln ( readfile [ m ] , true ) ) 
      {
	aclose ( readfile [ m ] ) ; 
	readopen [ m ] = 2 ; 
	if ( alignstate != 1000000L ) 
	{
	  runaway () ; 
	  {
	    if ( interaction == 3 ) 
	    ; 
	    printnl ( 262 ) ; 
	    print ( 750 ) ; 
	  } 
	  printesc ( 530 ) ; 
	  {
	    helpptr = 1 ; 
	    helpline [ 0 ] = 751 ; 
	  } 
	  alignstate = 1000000L ; 
	  error () ; 
	} 
      } 
    } 
    curinput .limitfield = last ; 
    if ( ( eqtb [ 12711 ] .cint < 0 ) || ( eqtb [ 12711 ] .cint > 255 ) ) 
    decr ( curinput .limitfield ) ; 
    else buffer [ curinput .limitfield ] = eqtb [ 12711 ] .cint ; 
    first = curinput .limitfield + 1 ; 
    curinput .locfield = curinput .startfield ; 
    curinput .statefield = 33 ; 
    while ( true ) {
	
      gettoken () ; 
      if ( curtok == 0 ) 
      goto lab30 ; 
      if ( alignstate < 1000000L ) 
      {
	do {
	    gettoken () ; 
	} while ( ! ( curtok == 0 ) ) ; 
	alignstate = 1000000L ; 
	goto lab30 ; 
      } 
      {
	q = getavail () ; 
	mem [ p ] .hh .v.RH = q ; 
	mem [ q ] .hh .v.LH = curtok ; 
	p = q ; 
      } 
    } 
    lab30: endfilereading () ; 
  } while ( ! ( alignstate == 1000000L ) ) ; 
  curval = defref ; 
  scannerstatus = 0 ; 
  alignstate = s ; 
} 
void passtext ( ) 
{/* 30 */ passtext_regmem 
  integer l  ; 
  smallnumber savescannerstatus  ; 
  savescannerstatus = scannerstatus ; 
  scannerstatus = 1 ; 
  l = 0 ; 
  skipline = line ; 
  while ( true ) {
      
    getnext () ; 
    if ( curcmd == 106 ) 
    {
      if ( l == 0 ) 
      goto lab30 ; 
      if ( curchr == 2 ) 
      decr ( l ) ; 
    } 
    else if ( curcmd == 105 ) 
    incr ( l ) ; 
  } 
  lab30: scannerstatus = savescannerstatus ; 
} 
void zchangeiflimit ( l , p ) 
smallnumber l ; 
halfword p ; 
{/* 10 */ changeiflimit_regmem 
  halfword q  ; 
  if ( p == condptr ) 
  iflimit = l ; 
  else {
      
    q = condptr ; 
    while ( true ) {
	
      if ( q == 0 ) 
      confusion ( 752 ) ; 
      if ( mem [ q ] .hh .v.RH == p ) 
      {
	mem [ q ] .hh.b0 = l ; 
	return ; 
      } 
      q = mem [ q ] .hh .v.RH ; 
    } 
  } 
} 
void conditional ( ) 
{/* 10 50 */ conditional_regmem 
  boolean b  ; 
  schar r  ; 
  integer m, n  ; 
  halfword p, q  ; 
  smallnumber savescannerstatus  ; 
  halfword savecondptr  ; 
  smallnumber thisif  ; 
  {
    p = getnode ( 2 ) ; 
    mem [ p ] .hh .v.RH = condptr ; 
    mem [ p ] .hh.b0 = iflimit ; 
    mem [ p ] .hh.b1 = curif ; 
    mem [ p + 1 ] .cint = ifline ; 
    condptr = p ; 
    curif = curchr ; 
    iflimit = 1 ; 
    ifline = line ; 
  } 
  savecondptr = condptr ; 
  thisif = curchr ; 
  switch ( thisif ) 
  {case 0 : 
  case 1 : 
    {
      {
	getxtoken () ; 
	if ( curcmd == 0 ) 
	if ( curchr == 257 ) 
	{
	  curcmd = 13 ; 
	  curchr = curtok - 4096 ; 
	} 
      } 
      if ( ( curcmd > 13 ) || ( curchr > 255 ) ) 
      {
	m = 0 ; 
	n = 256 ; 
      } 
      else {
	  
	m = curcmd ; 
	n = curchr ; 
      } 
      {
	getxtoken () ; 
	if ( curcmd == 0 ) 
	if ( curchr == 257 ) 
	{
	  curcmd = 13 ; 
	  curchr = curtok - 4096 ; 
	} 
      } 
      if ( ( curcmd > 13 ) || ( curchr > 255 ) ) 
      {
	curcmd = 0 ; 
	curchr = 256 ; 
      } 
      if ( thisif == 0 ) 
      b = ( n == curchr ) ; 
      else b = ( m == curcmd ) ; 
    } 
    break ; 
  case 2 : 
  case 3 : 
    {
      if ( thisif == 2 ) 
      scanint () ; 
      else scandimen ( false , false , false ) ; 
      n = curval ; 
      do {
	  getxtoken () ; 
      } while ( ! ( curcmd != 10 ) ) ; 
      if ( ( curtok >= 3132 ) && ( curtok <= 3134 ) ) 
      r = curtok - 3072 ; 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 776 ) ; 
	} 
	printcmdchr ( 105 , thisif ) ; 
	{
	  helpptr = 1 ; 
	  helpline [ 0 ] = 777 ; 
	} 
	backerror () ; 
	r = 61 ; 
      } 
      if ( thisif == 2 ) 
      scanint () ; 
      else scandimen ( false , false , false ) ; 
      switch ( r ) 
      {case 60 : 
	b = ( n < curval ) ; 
	break ; 
      case 61 : 
	b = ( n == curval ) ; 
	break ; 
      case 62 : 
	b = ( n > curval ) ; 
	break ; 
      } 
    } 
    break ; 
  case 4 : 
    {
      scanint () ; 
      b = odd ( curval ) ; 
    } 
    break ; 
  case 5 : 
    b = ( abs ( curlist .modefield ) == 1 ) ; 
    break ; 
  case 6 : 
    b = ( abs ( curlist .modefield ) == 102 ) ; 
    break ; 
  case 7 : 
    b = ( abs ( curlist .modefield ) == 203 ) ; 
    break ; 
  case 8 : 
    b = ( curlist .modefield < 0 ) ; 
    break ; 
  case 9 : 
  case 10 : 
  case 11 : 
    {
      scaneightbitint () ; 
      p = eqtb [ 11078 + curval ] .hh .v.RH ; 
      if ( thisif == 9 ) 
      b = ( p == 0 ) ; 
      else if ( p == 0 ) 
      b = false ; 
      else if ( thisif == 10 ) 
      b = ( mem [ p ] .hh.b0 == 0 ) ; 
      else b = ( mem [ p ] .hh.b0 == 1 ) ; 
    } 
    break ; 
  case 12 : 
    {
      savescannerstatus = scannerstatus ; 
      scannerstatus = 0 ; 
      getnext () ; 
      n = curcs ; 
      p = curcmd ; 
      q = curchr ; 
      getnext () ; 
      if ( curcmd != p ) 
      b = false ; 
      else if ( curcmd < 111 ) 
      b = ( curchr == q ) ; 
      else {
	  
	p = mem [ curchr ] .hh .v.RH ; 
	q = mem [ eqtb [ n ] .hh .v.RH ] .hh .v.RH ; 
	if ( p == q ) 
	b = true ; 
	else {
	    
	  while ( ( p != 0 ) && ( q != 0 ) ) if ( mem [ p ] .hh .v.LH != mem [ 
	  q ] .hh .v.LH ) 
	  p = 0 ; 
	  else {
	      
	    p = mem [ p ] .hh .v.RH ; 
	    q = mem [ q ] .hh .v.RH ; 
	  } 
	  b = ( ( p == 0 ) && ( q == 0 ) ) ; 
	} 
      } 
      scannerstatus = savescannerstatus ; 
    } 
    break ; 
  case 13 : 
    {
      scanfourbitint () ; 
      b = ( readopen [ curval ] == 2 ) ; 
    } 
    break ; 
  case 14 : 
    b = true ; 
    break ; 
  case 15 : 
    b = false ; 
    break ; 
  case 16 : 
    {
      scanint () ; 
      n = curval ; 
      if ( eqtb [ 12699 ] .cint > 1 ) 
      {
	begindiagnostic () ; 
	print ( 778 ) ; 
	printint ( n ) ; 
	printchar ( 125 ) ; 
	enddiagnostic ( false ) ; 
      } 
      while ( n != 0 ) {
	  
	passtext () ; 
	if ( condptr == savecondptr ) 
	if ( curchr == 4 ) 
	decr ( n ) ; 
	else goto lab50 ; 
	else if ( curchr == 2 ) 
	{
	  p = condptr ; 
	  ifline = mem [ p + 1 ] .cint ; 
	  curif = mem [ p ] .hh.b1 ; 
	  iflimit = mem [ p ] .hh.b0 ; 
	  condptr = mem [ p ] .hh .v.RH ; 
	  freenode ( p , 2 ) ; 
	} 
      } 
      changeiflimit ( 4 , savecondptr ) ; 
      return ; 
    } 
    break ; 
  } 
  if ( eqtb [ 12699 ] .cint > 1 ) 
  {
    begindiagnostic () ; 
    if ( b ) 
    print ( 774 ) ; 
    else print ( 775 ) ; 
    enddiagnostic ( false ) ; 
  } 
  if ( b ) 
  {
    changeiflimit ( 3 , savecondptr ) ; 
    return ; 
  } 
  while ( true ) {
      
    passtext () ; 
    if ( condptr == savecondptr ) 
    {
      if ( curchr != 4 ) 
      goto lab50 ; 
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 772 ) ; 
      } 
      printesc ( 770 ) ; 
      {
	helpptr = 1 ; 
	helpline [ 0 ] = 773 ; 
      } 
      error () ; 
    } 
    else if ( curchr == 2 ) 
    {
      p = condptr ; 
      ifline = mem [ p + 1 ] .cint ; 
      curif = mem [ p ] .hh.b1 ; 
      iflimit = mem [ p ] .hh.b0 ; 
      condptr = mem [ p ] .hh .v.RH ; 
      freenode ( p , 2 ) ; 
    } 
  } 
  lab50: if ( curchr == 2 ) 
  {
    p = condptr ; 
    ifline = mem [ p + 1 ] .cint ; 
    curif = mem [ p ] .hh.b1 ; 
    iflimit = mem [ p ] .hh.b0 ; 
    condptr = mem [ p ] .hh .v.RH ; 
    freenode ( p , 2 ) ; 
  } 
  else iflimit = 2 ; 
} 
void beginname ( ) 
{beginname_regmem 
  areadelimiter = 0 ; 
  extdelimiter = 0 ; 
} 
boolean zmorename ( c ) 
ASCIIcode c ; 
{register boolean Result; morename_regmem 
  if ( c == 32 ) 
  Result = false ; 
  else {
      
    {
      if ( poolptr + 1 > poolsize ) 
      overflow ( 257 , poolsize - initpoolptr ) ; 
    } 
    {
      strpool [ poolptr ] = c ; 
      incr ( poolptr ) ; 
    } 
    if ( ( c == 47 ) ) 
    {
      areadelimiter = ( poolptr - strstart [ strptr ] ) ; 
      extdelimiter = 0 ; 
    } 
    else if ( c == 46 ) 
    extdelimiter = ( poolptr - strstart [ strptr ] ) ; 
    Result = true ; 
  } 
  return(Result) ; 
} 
void endname ( ) 
{endname_regmem 
  if ( strptr + 3 > maxstrings ) 
  overflow ( 258 , maxstrings - initstrptr ) ; 
  if ( areadelimiter == 0 ) 
  curarea = 335 ; 
  else {
      
    curarea = strptr ; 
    strstart [ strptr + 1 ] = strstart [ strptr ] + areadelimiter ; 
    incr ( strptr ) ; 
  } 
  if ( extdelimiter == 0 ) 
  {
    curext = 335 ; 
    curname = makestring () ; 
  } 
  else {
      
    curname = strptr ; 
    strstart [ strptr + 1 ] = strstart [ strptr ] + extdelimiter - 
    areadelimiter - 1 ; 
    incr ( strptr ) ; 
    curext = makestring () ; 
  } 
} 
void zpackfilename ( n , a , e ) 
strnumber n ; 
strnumber a ; 
strnumber e ; 
{packfilename_regmem 
  integer k  ; 
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
{packbufferedname_regmem 
  integer k  ; 
  ASCIIcode c  ; 
  integer j  ; 
  if ( n + b - a + 5 > PATHMAX ) 
  b = a + PATHMAX - n - 5 ; 
  k = 0 ; 
  {register integer for_end; j = 1 ; for_end = n ; if ( j <= for_end) do 
    {
      c = xord [ TEXformatdefault [ j ] ] ; 
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
  {register integer for_end; j = formatdefaultlength - 3 ; for_end = 
  formatdefaultlength ; if ( j <= for_end) do 
    {
      c = xord [ TEXformatdefault [ j ] ] ; 
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
{register strnumber Result; makenamestring_regmem 
  integer k  ; 
  if ( ( poolptr + namelength > poolsize ) || ( strptr == maxstrings ) || ( ( 
  poolptr - strstart [ strptr ] ) > 0 ) ) 
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
{register strnumber Result; amakenamestring_regmem 
  Result = makenamestring () ; 
  return(Result) ; 
} 
strnumber zbmakenamestring ( f ) 
bytefile * f ; 
{register strnumber Result; bmakenamestring_regmem 
  Result = makenamestring () ; 
  return(Result) ; 
} 
strnumber zwmakenamestring ( f ) 
wordfile * f ; 
{register strnumber Result; wmakenamestring_regmem 
  Result = makenamestring () ; 
  return(Result) ; 
} 
void scanfilename ( ) 
{/* 30 */ scanfilename_regmem 
  nameinprogress = true ; 
  beginname () ; 
  do {
      getxtoken () ; 
  } while ( ! ( curcmd != 10 ) ) ; 
  while ( true ) {
      
    if ( ( curcmd > 12 ) || ( curchr > 255 ) ) 
    {
      backinput () ; 
      goto lab30 ; 
    } 
    if ( ! morename ( curchr ) ) 
    goto lab30 ; 
    getxtoken () ; 
  } 
  lab30: endname () ; 
  nameinprogress = false ; 
} 
void zpackjobname ( s ) 
strnumber s ; 
{packjobname_regmem 
  curarea = 335 ; 
  curext = s ; 
  curname = jobname ; 
  packfilename ( curname , curarea , curext ) ; 
} 
void zpromptfilename ( s , e ) 
strnumber s ; 
strnumber e ; 
{/* 30 */ promptfilename_regmem 
  integer k  ; 
  if ( interaction == 2 ) 
  ; 
  if ( s == 780 ) 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 781 ) ; 
  } 
  else {
      
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 782 ) ; 
  } 
  printfilename ( curname , curarea , curext ) ; 
  print ( 783 ) ; 
  if ( e == 784 ) 
  showcontext () ; 
  printnl ( 785 ) ; 
  print ( s ) ; 
  if ( interaction < 2 ) 
  fatalerror ( 786 ) ; 
  {
    ; 
    print ( 564 ) ; 
    terminput () ; 
  } 
  {
    beginname () ; 
    k = first ; 
    while ( ( buffer [ k ] == 32 ) && ( k < last ) ) incr ( k ) ; 
    while ( true ) {
	
      if ( k == last ) 
      goto lab30 ; 
      if ( ! morename ( buffer [ k ] ) ) 
      goto lab30 ; 
      incr ( k ) ; 
    } 
    lab30: endname () ; 
  } 
  if ( curext == 335 ) 
  curext = e ; 
  packfilename ( curname , curarea , curext ) ; 
} 
void openlogfile ( ) 
{openlogfile_regmem 
  schar oldsetting  ; 
  integer k  ; 
  integer l  ; 
  ccharpointer months  ; 
  oldsetting = selector ; 
  if ( jobname == 0 ) 
  jobname = 789 ; 
  packjobname ( 790 ) ; 
  while ( ! aopenout ( logfile ) ) {
      
    selector = 17 ; 
    promptfilename ( 792 , 790 ) ; 
  } 
  texmflogname = amakenamestring ( logfile ) ; 
  selector = 18 ; 
  logopened = true ; 
  {
    (void) Fputs( logfile ,  "This is TeX, C Version 3.141" ) ; 
    slowprint ( formatident ) ; 
    print ( 793 ) ; 
    printint ( eqtb [ 12684 ] .cint ) ; 
    printchar ( 32 ) ; 
    months = " JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC" ; 
    {register integer for_end; k = 3 * eqtb [ 12685 ] .cint - 2 ; for_end = 3 
    * eqtb [ 12685 ] .cint ; if ( k <= for_end) do 
      (void) putc( months [ k ] ,  logfile );
    while ( k++ < for_end ) ; } 
    printchar ( 32 ) ; 
    printint ( eqtb [ 12686 ] .cint ) ; 
    printchar ( 32 ) ; 
    printtwo ( eqtb [ 12683 ] .cint / 60 ) ; 
    printchar ( 58 ) ; 
    printtwo ( eqtb [ 12683 ] .cint % 60 ) ; 
  } 
  inputstack [ inputptr ] = curinput ; 
  printnl ( 791 ) ; 
  l = inputstack [ 0 ] .limitfield ; 
  if ( buffer [ l ] == eqtb [ 12711 ] .cint ) 
  decr ( l ) ; 
  {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) do 
    print ( buffer [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  println () ; 
  selector = oldsetting + 2 ; 
} 
void startinput ( ) 
{/* 30 */ startinput_regmem 
  scanfilename () ; 
  packfilename ( curname , curarea , curext ) ; 
  while ( true ) {
      
    beginfilereading () ; 
    if ( ( curext != 784 ) && ( namelength + 5 < PATHMAX ) && ( ! 
    extensionirrelevantp ( nameoffile , "tex" ) ) ) 
    {
      nameoffile [ namelength + 1 ] = 46 ; 
      nameoffile [ namelength + 2 ] = 116 ; 
      nameoffile [ namelength + 3 ] = 101 ; 
      nameoffile [ namelength + 4 ] = 120 ; 
      namelength = namelength + 4 ; 
    } 
    if ( aopenin ( inputfile [ curinput .indexfield ] , TEXINPUTPATH ) ) 
    goto lab30 ; 
    if ( curext == 784 ) 
    curext = 335 ; 
    packfilename ( curname , curarea , curext ) ; 
    if ( aopenin ( inputfile [ curinput .indexfield ] , TEXINPUTPATH ) ) 
    goto lab30 ; 
    endfilereading () ; 
    promptfilename ( 780 , 784 ) ; 
  } 
  lab30: curinput .namefield = amakenamestring ( inputfile [ curinput 
  .indexfield ] ) ; 
  if ( jobname == 0 ) 
  {
    jobname = curname ; 
    openlogfile () ; 
  } 
  if ( termoffset + ( strstart [ curinput .namefield + 1 ] - strstart [ 
  curinput .namefield ] ) > maxprintline - 3 ) 
  println () ; 
  else if ( ( termoffset > 0 ) || ( fileoffset > 0 ) ) 
  printchar ( 32 ) ; 
  printchar ( 40 ) ; 
  incr ( openparens ) ; 
  slowprint ( curinput .namefield ) ; 
  flush ( stdout ) ; 
  curinput .statefield = 33 ; 
  {
    line = 1 ; 
    if ( inputln ( inputfile [ curinput .indexfield ] , false ) ) 
    ; 
    firmuptheline () ; 
    if ( ( eqtb [ 12711 ] .cint < 0 ) || ( eqtb [ 12711 ] .cint > 255 ) ) 
    decr ( curinput .limitfield ) ; 
    else buffer [ curinput .limitfield ] = eqtb [ 12711 ] .cint ; 
    first = curinput .limitfield + 1 ; 
    curinput .locfield = curinput .startfield ; 
  } 
} 
internalfontnumber zreadfontinfo ( u , nom , aire , s ) 
halfword u ; 
strnumber nom ; 
strnumber aire ; 
scaled s ; 
{/* 30 11 45 */ register internalfontnumber Result; readfontinfo_regmem 
  fontindex k  ; 
  boolean fileopened  ; 
  halfword lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np  ; 
  internalfontnumber f  ; 
  internalfontnumber g  ; 
  eightbits a, b, c, d  ; 
  fourquarters qw  ; 
  scaled sw  ; 
  integer bchlabel  ; 
  short bchar  ; 
  scaled z  ; 
  integer alpha  ; 
  schar beta  ; 
  g = 0 ; 
  fileopened = false ; 
  packfilename ( nom , aire , 804 ) ; 
  if ( ! bopenin ( tfmfile ) ) 
  goto lab11 ; 
  fileopened = true ; 
  {
    {
      lf = tfmtemp ; 
      if ( lf > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      lf = lf * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      lh = tfmtemp ; 
      if ( lh > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      lh = lh * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      bc = tfmtemp ; 
      if ( bc > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      bc = bc * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      ec = tfmtemp ; 
      if ( ec > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      ec = ec * 256 + tfmtemp ; 
    } 
    if ( ( bc > ec + 1 ) || ( ec > 255 ) ) 
    goto lab11 ; 
    if ( bc > 255 ) 
    {
      bc = 1 ; 
      ec = 0 ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      nw = tfmtemp ; 
      if ( nw > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      nw = nw * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      nh = tfmtemp ; 
      if ( nh > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      nh = nh * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      nd = tfmtemp ; 
      if ( nd > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      nd = nd * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      ni = tfmtemp ; 
      if ( ni > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      ni = ni * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      nl = tfmtemp ; 
      if ( nl > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      nl = nl * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      nk = tfmtemp ; 
      if ( nk > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      nk = nk * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      ne = tfmtemp ; 
      if ( ne > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      ne = ne * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      np = tfmtemp ; 
      if ( np > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      np = np * 256 + tfmtemp ; 
    } 
    if ( lf != 6 + lh + ( ec - bc + 1 ) + nw + nh + nd + ni + nl + nk + ne + 
    np ) 
    goto lab11 ; 
  } 
  lf = lf - 6 - lh ; 
  if ( np < 7 ) 
  lf = lf + 7 - np ; 
  if ( ( fontptr == fontmax ) || ( fmemptr + lf > fontmemsize ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 795 ) ; 
    } 
    sprintcs ( u ) ; 
    printchar ( 61 ) ; 
    printfilename ( nom , aire , 335 ) ; 
    if ( s >= 0 ) 
    {
      print ( 737 ) ; 
      printscaled ( s ) ; 
      print ( 393 ) ; 
    } 
    else if ( s != -1000 ) 
    {
      print ( 796 ) ; 
      printint ( - (integer) s ) ; 
    } 
    print ( 805 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 806 ; 
      helpline [ 2 ] = 807 ; 
      helpline [ 1 ] = 808 ; 
      helpline [ 0 ] = 809 ; 
    } 
    error () ; 
    goto lab30 ; 
  } 
  f = fontptr + 1 ; 
  charbase [ f ] = fmemptr - bc ; 
  widthbase [ f ] = charbase [ f ] + ec + 1 ; 
  heightbase [ f ] = widthbase [ f ] + nw ; 
  depthbase [ f ] = heightbase [ f ] + nh ; 
  italicbase [ f ] = depthbase [ f ] + nd ; 
  ligkernbase [ f ] = italicbase [ f ] + ni ; 
  kernbase [ f ] = ligkernbase [ f ] + nl - 256 * ( 128 ) ; 
  extenbase [ f ] = kernbase [ f ] + 256 * ( 128 ) + nk ; 
  parambase [ f ] = extenbase [ f ] + ne ; 
  {
    if ( lh < 2 ) 
    goto lab11 ; 
    {
      tfmtemp = getc ( tfmfile ) ; 
      a = tfmtemp ; 
      qw .b0 = a ; 
      tfmtemp = getc ( tfmfile ) ; 
      b = tfmtemp ; 
      qw .b1 = b ; 
      tfmtemp = getc ( tfmfile ) ; 
      c = tfmtemp ; 
      qw .b2 = c ; 
      tfmtemp = getc ( tfmfile ) ; 
      d = tfmtemp ; 
      qw .b3 = d ; 
      fontcheck [ f ] = qw ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    {
      z = tfmtemp ; 
      if ( z > 127 ) 
      goto lab11 ; 
      tfmtemp = getc ( tfmfile ) ; 
      z = z * 256 + tfmtemp ; 
    } 
    tfmtemp = getc ( tfmfile ) ; 
    z = z * 256 + tfmtemp ; 
    tfmtemp = getc ( tfmfile ) ; 
    z = ( z * 16 ) + ( tfmtemp / 16 ) ; 
    if ( z < 65536L ) 
    goto lab11 ; 
    while ( lh > 2 ) {
	
      tfmtemp = getc ( tfmfile ) ; 
      tfmtemp = getc ( tfmfile ) ; 
      tfmtemp = getc ( tfmfile ) ; 
      tfmtemp = getc ( tfmfile ) ; 
      decr ( lh ) ; 
    } 
    fontdsize [ f ] = z ; 
    if ( s != -1000 ) 
    if ( s >= 0 ) 
    z = s ; 
    else z = xnoverd ( z , - (integer) s , 1000 ) ; 
    fontsize [ f ] = z ; 
  } 
  {register integer for_end; k = fmemptr ; for_end = widthbase [ f ] - 1 
  ; if ( k <= for_end) do 
    {
      {
	tfmtemp = getc ( tfmfile ) ; 
	a = tfmtemp ; 
	qw .b0 = a ; 
	tfmtemp = getc ( tfmfile ) ; 
	b = tfmtemp ; 
	qw .b1 = b ; 
	tfmtemp = getc ( tfmfile ) ; 
	c = tfmtemp ; 
	qw .b2 = c ; 
	tfmtemp = getc ( tfmfile ) ; 
	d = tfmtemp ; 
	qw .b3 = d ; 
	fontinfo [ k ] .qqqq = qw ; 
      } 
      if ( ( a >= nw ) || ( b / 16 >= nh ) || ( b % 16 >= nd ) || ( c / 4 >= 
      ni ) ) 
      goto lab11 ; 
      switch ( c % 4 ) 
      {case 1 : 
	if ( d >= nl ) 
	goto lab11 ; 
	break ; 
      case 3 : 
	if ( d >= ne ) 
	goto lab11 ; 
	break ; 
      case 2 : 
	{
	  {
	    if ( ( d < bc ) || ( d > ec ) ) 
	    goto lab11 ; 
	  } 
	  while ( d < k + bc - fmemptr ) {
	      
	    qw = fontinfo [ charbase [ f ] + d ] .qqqq ; 
	    if ( ( ( qw .b2 ) % 4 ) != 2 ) 
	    goto lab45 ; 
	    d = qw .b3 ; 
	  } 
	  if ( d == k + bc - fmemptr ) 
	  goto lab11 ; 
	  lab45: ; 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  {
    {
      alpha = 16 ; 
      while ( z >= 8388608L ) {
	  
	z = z / 2 ; 
	alpha = alpha + alpha ; 
      } 
      beta = 256 / alpha ; 
      alpha = alpha * z ; 
    } 
    {register integer for_end; k = widthbase [ f ] ; for_end = ligkernbase [ 
    f ] - 1 ; if ( k <= for_end) do 
      {
	tfmtemp = getc ( tfmfile ) ; 
	a = tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	b = tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	c = tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	d = tfmtemp ; 
	sw = ( ( ( ( ( d * z ) / 256 ) + ( c * z ) ) / 256 ) + ( b * z ) ) / 
	beta ; 
	if ( a == 0 ) 
	fontinfo [ k ] .cint = sw ; 
	else if ( a == 255 ) 
	fontinfo [ k ] .cint = sw - alpha ; 
	else goto lab11 ; 
      } 
    while ( k++ < for_end ) ; } 
    if ( fontinfo [ widthbase [ f ] ] .cint != 0 ) 
    goto lab11 ; 
    if ( fontinfo [ heightbase [ f ] ] .cint != 0 ) 
    goto lab11 ; 
    if ( fontinfo [ depthbase [ f ] ] .cint != 0 ) 
    goto lab11 ; 
    if ( fontinfo [ italicbase [ f ] ] .cint != 0 ) 
    goto lab11 ; 
  } 
  bchlabel = 32767 ; 
  bchar = 256 ; 
  if ( nl > 0 ) 
  {
    {register integer for_end; k = ligkernbase [ f ] ; for_end = kernbase [ f 
    ] + 256 * ( 128 ) - 1 ; if ( k <= for_end) do 
      {
	{
	  tfmtemp = getc ( tfmfile ) ; 
	  a = tfmtemp ; 
	  qw .b0 = a ; 
	  tfmtemp = getc ( tfmfile ) ; 
	  b = tfmtemp ; 
	  qw .b1 = b ; 
	  tfmtemp = getc ( tfmfile ) ; 
	  c = tfmtemp ; 
	  qw .b2 = c ; 
	  tfmtemp = getc ( tfmfile ) ; 
	  d = tfmtemp ; 
	  qw .b3 = d ; 
	  fontinfo [ k ] .qqqq = qw ; 
	} 
	if ( a > 128 ) 
	{
	  if ( 256 * c + d >= nl ) 
	  goto lab11 ; 
	  if ( a == 255 ) 
	  if ( k == ligkernbase [ f ] ) 
	  bchar = b ; 
	} 
	else {
	    
	  if ( b != bchar ) 
	  {
	    {
	      if ( ( b < bc ) || ( b > ec ) ) 
	      goto lab11 ; 
	    } 
	    qw = fontinfo [ charbase [ f ] + b ] .qqqq ; 
	    if ( ! ( qw .b0 > 0 ) ) 
	    goto lab11 ; 
	  } 
	  if ( c < 128 ) 
	  {
	    {
	      if ( ( d < bc ) || ( d > ec ) ) 
	      goto lab11 ; 
	    } 
	    qw = fontinfo [ charbase [ f ] + d ] .qqqq ; 
	    if ( ! ( qw .b0 > 0 ) ) 
	    goto lab11 ; 
	  } 
	  else if ( 256 * ( c - 128 ) + d >= nk ) 
	  goto lab11 ; 
	  if ( a < 128 ) 
	  if ( k - ligkernbase [ f ] + a + 1 >= nl ) 
	  goto lab11 ; 
	} 
      } 
    while ( k++ < for_end ) ; } 
    if ( a == 255 ) 
    bchlabel = 256 * c + d ; 
  } 
  {register integer for_end; k = kernbase [ f ] + 256 * ( 128 ) ; for_end = 
  extenbase [ f ] - 1 ; if ( k <= for_end) do 
    {
      tfmtemp = getc ( tfmfile ) ; 
      a = tfmtemp ; 
      tfmtemp = getc ( tfmfile ) ; 
      b = tfmtemp ; 
      tfmtemp = getc ( tfmfile ) ; 
      c = tfmtemp ; 
      tfmtemp = getc ( tfmfile ) ; 
      d = tfmtemp ; 
      sw = ( ( ( ( ( d * z ) / 256 ) + ( c * z ) ) / 256 ) + ( b * z ) ) / 
      beta ; 
      if ( a == 0 ) 
      fontinfo [ k ] .cint = sw ; 
      else if ( a == 255 ) 
      fontinfo [ k ] .cint = sw - alpha ; 
      else goto lab11 ; 
    } 
  while ( k++ < for_end ) ; } 
  {register integer for_end; k = extenbase [ f ] ; for_end = parambase [ f ] 
  - 1 ; if ( k <= for_end) do 
    {
      {
	tfmtemp = getc ( tfmfile ) ; 
	a = tfmtemp ; 
	qw .b0 = a ; 
	tfmtemp = getc ( tfmfile ) ; 
	b = tfmtemp ; 
	qw .b1 = b ; 
	tfmtemp = getc ( tfmfile ) ; 
	c = tfmtemp ; 
	qw .b2 = c ; 
	tfmtemp = getc ( tfmfile ) ; 
	d = tfmtemp ; 
	qw .b3 = d ; 
	fontinfo [ k ] .qqqq = qw ; 
      } 
      if ( a != 0 ) 
      {
	{
	  if ( ( a < bc ) || ( a > ec ) ) 
	  goto lab11 ; 
	} 
	qw = fontinfo [ charbase [ f ] + a ] .qqqq ; 
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ; 
      } 
      if ( b != 0 ) 
      {
	{
	  if ( ( b < bc ) || ( b > ec ) ) 
	  goto lab11 ; 
	} 
	qw = fontinfo [ charbase [ f ] + b ] .qqqq ; 
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ; 
      } 
      if ( c != 0 ) 
      {
	{
	  if ( ( c < bc ) || ( c > ec ) ) 
	  goto lab11 ; 
	} 
	qw = fontinfo [ charbase [ f ] + c ] .qqqq ; 
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ; 
      } 
      {
	{
	  if ( ( d < bc ) || ( d > ec ) ) 
	  goto lab11 ; 
	} 
	qw = fontinfo [ charbase [ f ] + d ] .qqqq ; 
	if ( ! ( qw .b0 > 0 ) ) 
	goto lab11 ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  {
    {register integer for_end; k = 1 ; for_end = np ; if ( k <= for_end) do 
      if ( k == 1 ) 
      {
	tfmtemp = getc ( tfmfile ) ; 
	sw = tfmtemp ; 
	if ( sw > 127 ) 
	sw = sw - 256 ; 
	tfmtemp = getc ( tfmfile ) ; 
	sw = sw * 256 + tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	sw = sw * 256 + tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	fontinfo [ parambase [ f ] ] .cint = ( sw * 16 ) + ( tfmtemp / 16 ) ; 
      } 
      else {
	  
	tfmtemp = getc ( tfmfile ) ; 
	a = tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	b = tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	c = tfmtemp ; 
	tfmtemp = getc ( tfmfile ) ; 
	d = tfmtemp ; 
	sw = ( ( ( ( ( d * z ) / 256 ) + ( c * z ) ) / 256 ) + ( b * z ) ) / 
	beta ; 
	if ( a == 0 ) 
	fontinfo [ parambase [ f ] + k - 1 ] .cint = sw ; 
	else if ( a == 255 ) 
	fontinfo [ parambase [ f ] + k - 1 ] .cint = sw - alpha ; 
	else goto lab11 ; 
      } 
    while ( k++ < for_end ) ; } 
    if ( feof ( tfmfile ) ) 
    goto lab11 ; 
    {register integer for_end; k = np + 1 ; for_end = 7 ; if ( k <= for_end) 
    do 
      fontinfo [ parambase [ f ] + k - 1 ] .cint = 0 ; 
    while ( k++ < for_end ) ; } 
  } 
  if ( np >= 7 ) 
  fontparams [ f ] = np ; 
  else fontparams [ f ] = 7 ; 
  hyphenchar [ f ] = eqtb [ 12709 ] .cint ; 
  skewchar [ f ] = eqtb [ 12710 ] .cint ; 
  if ( bchlabel < nl ) 
  bcharlabel [ f ] = bchlabel + ligkernbase [ f ] ; 
  else bcharlabel [ f ] = fontmemsize ; 
  fontbchar [ f ] = bchar ; 
  fontfalsebchar [ f ] = bchar ; 
  if ( bchar <= ec ) 
  if ( bchar >= bc ) 
  {
    qw = fontinfo [ charbase [ f ] + bchar ] .qqqq ; 
    if ( ( qw .b0 > 0 ) ) 
    fontfalsebchar [ f ] = 256 ; 
  } 
  fontname [ f ] = nom ; 
  fontarea [ f ] = aire ; 
  fontbc [ f ] = bc ; 
  fontec [ f ] = ec ; 
  fontglue [ f ] = 0 ; 
  charbase [ f ] = charbase [ f ] ; 
  widthbase [ f ] = widthbase [ f ] ; 
  ligkernbase [ f ] = ligkernbase [ f ] ; 
  kernbase [ f ] = kernbase [ f ] ; 
  extenbase [ f ] = extenbase [ f ] ; 
  decr ( parambase [ f ] ) ; 
  fmemptr = fmemptr + lf ; 
  fontptr = f ; 
  g = f ; 
  goto lab30 ; 
  lab11: {
      
    if ( interaction == 3 ) 
    ; 
    printnl ( 262 ) ; 
    print ( 795 ) ; 
  } 
  sprintcs ( u ) ; 
  printchar ( 61 ) ; 
  printfilename ( nom , aire , 335 ) ; 
  if ( s >= 0 ) 
  {
    print ( 737 ) ; 
    printscaled ( s ) ; 
    print ( 393 ) ; 
  } 
  else if ( s != -1000 ) 
  {
    print ( 796 ) ; 
    printint ( - (integer) s ) ; 
  } 
  if ( fileopened ) 
  print ( 797 ) ; 
  else print ( 798 ) ; 
  {
    helpptr = 5 ; 
    helpline [ 4 ] = 799 ; 
    helpline [ 3 ] = 800 ; 
    helpline [ 2 ] = 801 ; 
    helpline [ 1 ] = 802 ; 
    helpline [ 0 ] = 803 ; 
  } 
  error () ; 
  lab30: if ( fileopened ) 
  bclose ( tfmfile ) ; 
  Result = g ; 
  return(Result) ; 
} 
