#define EXTERN extern
#include "mfd.h"

void xreflectedges ( ) 
{halfword p, q, r, s  ; 
  integer m  ; 
  p = mem [ curedges + 2 ] .hhfield .lhfield ; 
  mem [ curedges + 2 ] .hhfield .lhfield = 8192 - mem [ curedges + 2 ] 
  .hhfield .v.RH ; 
  mem [ curedges + 2 ] .hhfield .v.RH = 8192 - p ; 
  m = ( 4096 + mem [ curedges + 3 ] .hhfield .lhfield ) * 8 + 8 ; 
  mem [ curedges + 3 ] .hhfield .lhfield = 4096 ; 
  p = mem [ curedges ] .hhfield .v.RH ; 
  do {
      q = mem [ p + 1 ] .hhfield .v.RH ; 
    r = memtop ; 
    while ( q != memtop ) {
	
      s = mem [ q ] .hhfield .v.RH ; 
      mem [ q ] .hhfield .v.RH = r ; 
      r = q ; 
      mem [ r ] .hhfield .lhfield = m - mem [ q ] .hhfield .lhfield ; 
      q = s ; 
    } 
    mem [ p + 1 ] .hhfield .v.RH = r ; 
    q = mem [ p + 1 ] .hhfield .lhfield ; 
    while ( q > 1 ) {
	
      mem [ q ] .hhfield .lhfield = m - mem [ q ] .hhfield .lhfield ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    p = mem [ p ] .hhfield .v.RH ; 
  } while ( ! ( p == curedges ) ) ; 
  mem [ curedges + 4 ] .cint = 0 ; 
} 
void zyscaleedges ( s ) 
integer s ; 
{halfword p, q, pp, r, rr, ss  ; 
  integer t  ; 
  if ( ( s * ( mem [ curedges + 1 ] .hhfield .v.RH - 4095 ) >= 4096 ) || ( s * 
  ( mem [ curedges + 1 ] .hhfield .lhfield - 4096 ) <= -4096 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 534 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 535 ; 
      helpline [ 1 ] = 536 ; 
      helpline [ 0 ] = 537 ; 
    } 
    putgeterror () ; 
  } 
  else {
      
    mem [ curedges + 1 ] .hhfield .v.RH = s * ( mem [ curedges + 1 ] .hhfield 
    .v.RH - 4095 ) + 4095 ; 
    mem [ curedges + 1 ] .hhfield .lhfield = s * ( mem [ curedges + 1 ] 
    .hhfield .lhfield - 4096 ) + 4096 ; 
    p = curedges ; 
    do {
	q = p ; 
      p = mem [ p ] .hhfield .v.RH ; 
      {register integer for_end; t = 2 ; for_end = s ; if ( t <= for_end) do 
	{
	  pp = getnode ( 2 ) ; 
	  mem [ q ] .hhfield .v.RH = pp ; 
	  mem [ p ] .hhfield .lhfield = pp ; 
	  mem [ pp ] .hhfield .v.RH = p ; 
	  mem [ pp ] .hhfield .lhfield = q ; 
	  q = pp ; 
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
	  mem [ pp + 1 ] .hhfield .lhfield = mem [ memtop - 1 ] .hhfield .v.RH 
	  ; 
	} 
      while ( t++ < for_end ) ; } 
    } while ( ! ( mem [ p ] .hhfield .v.RH == curedges ) ) ; 
    mem [ curedges + 4 ] .cint = 0 ; 
  } 
} 
void zxscaleedges ( s ) 
integer s ; 
{halfword p, q  ; 
  unsigned short t  ; 
  schar w  ; 
  integer delta  ; 
  if ( ( s * ( mem [ curedges + 2 ] .hhfield .v.RH - 4096 ) >= 4096 ) || ( s * 
  ( mem [ curedges + 2 ] .hhfield .lhfield - 4096 ) <= -4096 ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 261 ) ; 
      print ( 534 ) ; 
    } 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 538 ; 
      helpline [ 1 ] = 536 ; 
      helpline [ 0 ] = 537 ; 
    } 
    putgeterror () ; 
  } 
  else if ( ( mem [ curedges + 2 ] .hhfield .v.RH != 4096 ) || ( mem [ 
  curedges + 2 ] .hhfield .lhfield != 4096 ) ) 
  {
    mem [ curedges + 2 ] .hhfield .v.RH = s * ( mem [ curedges + 2 ] .hhfield 
    .v.RH - 4096 ) + 4096 ; 
    mem [ curedges + 2 ] .hhfield .lhfield = s * ( mem [ curedges + 2 ] 
    .hhfield .lhfield - 4096 ) + 4096 ; 
    delta = 8 * ( 4096 - s * mem [ curedges + 3 ] .hhfield .lhfield ) + 0 ; 
    mem [ curedges + 3 ] .hhfield .lhfield = 4096 ; 
    q = mem [ curedges ] .hhfield .v.RH ; 
    do {
	p = mem [ q + 1 ] .hhfield .v.RH ; 
      while ( p != memtop ) {
	  
	t = mem [ p ] .hhfield .lhfield ; 
	w = t % 8 ; 
	mem [ p ] .hhfield .lhfield = ( t - w ) * s + w + delta ; 
	p = mem [ p ] .hhfield .v.RH ; 
      } 
      p = mem [ q + 1 ] .hhfield .lhfield ; 
      while ( p > 1 ) {
	  
	t = mem [ p ] .hhfield .lhfield ; 
	w = t % 8 ; 
	mem [ p ] .hhfield .lhfield = ( t - w ) * s + w + delta ; 
	p = mem [ p ] .hhfield .v.RH ; 
      } 
      q = mem [ q ] .hhfield .v.RH ; 
    } while ( ! ( q == curedges ) ) ; 
    mem [ curedges + 4 ] .cint = 0 ; 
  } 
} 
void znegateedges ( h ) 
halfword h ; 
{/* 30 */ halfword p, q, r, s, t, u  ; 
  p = mem [ h ] .hhfield .v.RH ; 
  while ( p != h ) {
      
    q = mem [ p + 1 ] .hhfield .lhfield ; 
    while ( q > 1 ) {
	
      mem [ q ] .hhfield .lhfield = 8 - 2 * ( ( mem [ q ] .hhfield .lhfield ) 
      % 8 ) + mem [ q ] .hhfield .lhfield ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    q = mem [ p + 1 ] .hhfield .v.RH ; 
    if ( q != memtop ) 
    {
      do {
	  mem [ q ] .hhfield .lhfield = 8 - 2 * ( ( mem [ q ] .hhfield 
	.lhfield ) % 8 ) + mem [ q ] .hhfield .lhfield ; 
	q = mem [ q ] .hhfield .v.RH ; 
      } while ( ! ( q == memtop ) ) ; 
      u = p + 1 ; 
      q = mem [ u ] .hhfield .v.RH ; 
      r = q ; 
      s = mem [ r ] .hhfield .v.RH ; 
      while ( true ) if ( mem [ s ] .hhfield .lhfield > mem [ r ] .hhfield 
      .lhfield ) 
      {
	mem [ u ] .hhfield .v.RH = q ; 
	if ( s == memtop ) 
	goto lab30 ; 
	u = r ; 
	q = s ; 
	r = q ; 
	s = mem [ r ] .hhfield .v.RH ; 
      } 
      else {
	  
	t = s ; 
	s = mem [ t ] .hhfield .v.RH ; 
	mem [ t ] .hhfield .v.RH = q ; 
	q = t ; 
      } 
      lab30: mem [ r ] .hhfield .v.RH = memtop ; 
    } 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  mem [ h + 4 ] .cint = 0 ; 
} 
void zsortedges ( h ) 
halfword h ; 
{/* 30 */ halfword k  ; 
  halfword p, q, r, s  ; 
  r = mem [ h + 1 ] .hhfield .lhfield ; 
  mem [ h + 1 ] .hhfield .lhfield = 0 ; 
  p = mem [ r ] .hhfield .v.RH ; 
  mem [ r ] .hhfield .v.RH = memtop ; 
  mem [ memtop - 1 ] .hhfield .v.RH = r ; 
  while ( p > 1 ) {
      
    k = mem [ p ] .hhfield .lhfield ; 
    q = memtop - 1 ; 
    do {
	r = q ; 
      q = mem [ r ] .hhfield .v.RH ; 
    } while ( ! ( k <= mem [ q ] .hhfield .lhfield ) ) ; 
    mem [ r ] .hhfield .v.RH = p ; 
    r = mem [ p ] .hhfield .v.RH ; 
    mem [ p ] .hhfield .v.RH = q ; 
    p = r ; 
  } 
  {
    r = h + 1 ; 
    q = mem [ r ] .hhfield .v.RH ; 
    p = mem [ memtop - 1 ] .hhfield .v.RH ; 
    while ( true ) {
	
      k = mem [ p ] .hhfield .lhfield ; 
      while ( k > mem [ q ] .hhfield .lhfield ) {
	  
	r = q ; 
	q = mem [ r ] .hhfield .v.RH ; 
      } 
      mem [ r ] .hhfield .v.RH = p ; 
      s = mem [ p ] .hhfield .v.RH ; 
      mem [ p ] .hhfield .v.RH = q ; 
      if ( s == memtop ) 
      goto lab30 ; 
      r = p ; 
      p = s ; 
    } 
    lab30: ; 
  } 
} 
void zculledges ( wlo , whi , wout , win ) 
integer wlo ; 
integer whi ; 
integer wout ; 
integer win ; 
{/* 30 */ halfword p, q, r, s  ; 
  integer w  ; 
  integer d  ; 
  integer m  ; 
  integer mm  ; 
  integer ww  ; 
  integer prevw  ; 
  halfword n, minn, maxn  ; 
  halfword mind, maxd  ; 
  mind = 262143L ; 
  maxd = 0 ; 
  minn = 262143L ; 
  maxn = 0 ; 
  p = mem [ curedges ] .hhfield .v.RH ; 
  n = mem [ curedges + 1 ] .hhfield .lhfield ; 
  while ( p != curedges ) {
      
    if ( mem [ p + 1 ] .hhfield .lhfield > 1 ) 
    sortedges ( p ) ; 
    if ( mem [ p + 1 ] .hhfield .v.RH != memtop ) 
    {
      r = memtop - 1 ; 
      q = mem [ p + 1 ] .hhfield .v.RH ; 
      ww = 0 ; 
      m = 1000000L ; 
      prevw = 0 ; 
      while ( true ) {
	  
	if ( q == memtop ) 
	mm = 1000000L ; 
	else {
	    
	  d = mem [ q ] .hhfield .lhfield ; 
	  mm = d / 8 ; 
	  ww = ww + ( d % 8 ) - 4 ; 
	} 
	if ( mm > m ) 
	{
	  if ( w != prevw ) 
	  {
	    s = getavail () ; 
	    mem [ r ] .hhfield .v.RH = s ; 
	    mem [ s ] .hhfield .lhfield = 8 * m + 4 + w - prevw ; 
	    r = s ; 
	    prevw = w ; 
	  } 
	  if ( q == memtop ) 
	  goto lab30 ; 
	} 
	m = mm ; 
	if ( ww >= wlo ) 
	if ( ww <= whi ) 
	w = win ; 
	else w = wout ; 
	else w = wout ; 
	s = mem [ q ] .hhfield .v.RH ; 
	{
	  mem [ q ] .hhfield .v.RH = avail ; 
	  avail = q ; 
	;
#ifdef STAT
	  decr ( dynused ) ; 
#endif /* STAT */
	} 
	q = s ; 
      } 
      lab30: mem [ r ] .hhfield .v.RH = memtop ; 
      mem [ p + 1 ] .hhfield .v.RH = mem [ memtop - 1 ] .hhfield .v.RH ; 
      if ( r != memtop - 1 ) 
      {
	if ( minn == 262143L ) 
	minn = n ; 
	maxn = n ; 
	if ( mind > mem [ mem [ memtop - 1 ] .hhfield .v.RH ] .hhfield 
	.lhfield ) 
	mind = mem [ mem [ memtop - 1 ] .hhfield .v.RH ] .hhfield .lhfield ; 
	if ( maxd < mem [ r ] .hhfield .lhfield ) 
	maxd = mem [ r ] .hhfield .lhfield ; 
      } 
    } 
    p = mem [ p ] .hhfield .v.RH ; 
    incr ( n ) ; 
  } 
  if ( minn > maxn ) 
  {
    p = mem [ curedges ] .hhfield .v.RH ; 
    while ( p != curedges ) {
	
      q = mem [ p ] .hhfield .v.RH ; 
      freenode ( p , 2 ) ; 
      p = q ; 
    } 
    initedges ( curedges ) ; 
  } 
  else {
      
    n = mem [ curedges + 1 ] .hhfield .lhfield ; 
    mem [ curedges + 1 ] .hhfield .lhfield = minn ; 
    while ( minn > n ) {
	
      p = mem [ curedges ] .hhfield .v.RH ; 
      mem [ curedges ] .hhfield .v.RH = mem [ p ] .hhfield .v.RH ; 
      mem [ mem [ p ] .hhfield .v.RH ] .hhfield .lhfield = curedges ; 
      freenode ( p , 2 ) ; 
      incr ( n ) ; 
    } 
    n = mem [ curedges + 1 ] .hhfield .v.RH ; 
    mem [ curedges + 1 ] .hhfield .v.RH = maxn ; 
    mem [ curedges + 5 ] .hhfield .lhfield = maxn + 1 ; 
    mem [ curedges + 5 ] .hhfield .v.RH = curedges ; 
    while ( maxn < n ) {
	
      p = mem [ curedges ] .hhfield .lhfield ; 
      mem [ curedges ] .hhfield .lhfield = mem [ p ] .hhfield .lhfield ; 
      mem [ mem [ p ] .hhfield .lhfield ] .hhfield .v.RH = curedges ; 
      freenode ( p , 2 ) ; 
      decr ( n ) ; 
    } 
    mem [ curedges + 2 ] .hhfield .lhfield = ( ( mind ) / 8 ) - mem [ curedges 
    + 3 ] .hhfield .lhfield + 4096 ; 
    mem [ curedges + 2 ] .hhfield .v.RH = ( ( maxd ) / 8 ) - mem [ curedges + 
    3 ] .hhfield .lhfield + 4096 ; 
  } 
  mem [ curedges + 4 ] .cint = 0 ; 
} 
void xyswapedges ( ) 
{/* 30 */ integer mmagic, nmagic  ; 
  halfword p, q, r, s  ; 
  integer mspread  ; 
  integer j, jj  ; 
  integer m, mm  ; 
  integer pd, rd  ; 
  integer pm, rm  ; 
  integer w  ; 
  integer ww  ; 
  integer dw  ; 
  integer extras  ; 
  schar xw  ; 
  integer k  ; 
  mspread = mem [ curedges + 2 ] .hhfield .v.RH - mem [ curedges + 2 ] 
  .hhfield .lhfield ; 
  if ( mspread > movesize ) 
  overflow ( 539 , movesize ) ; 
  {register integer for_end; j = 0 ; for_end = mspread ; if ( j <= for_end) 
  do 
    move [ j ] = memtop ; 
  while ( j++ < for_end ) ; } 
  p = getnode ( 2 ) ; 
  mem [ p + 1 ] .hhfield .v.RH = memtop ; 
  mem [ p + 1 ] .hhfield .lhfield = 0 ; 
  mem [ p ] .hhfield .lhfield = curedges ; 
  mem [ mem [ curedges ] .hhfield .v.RH ] .hhfield .lhfield = p ; 
  p = getnode ( 2 ) ; 
  mem [ p + 1 ] .hhfield .v.RH = memtop ; 
  mem [ p ] .hhfield .lhfield = mem [ curedges ] .hhfield .lhfield ; 
  mmagic = mem [ curedges + 2 ] .hhfield .lhfield + mem [ curedges + 3 ] 
  .hhfield .lhfield - 4096 ; 
  nmagic = 8 * mem [ curedges + 1 ] .hhfield .v.RH + 12 ; 
  do {
      q = mem [ p ] .hhfield .lhfield ; 
    if ( mem [ q + 1 ] .hhfield .lhfield > 1 ) 
    sortedges ( q ) ; 
    r = mem [ p + 1 ] .hhfield .v.RH ; 
    freenode ( p , 2 ) ; 
    p = r ; 
    pd = mem [ p ] .hhfield .lhfield ; 
    pm = pd / 8 ; 
    r = mem [ q + 1 ] .hhfield .v.RH ; 
    rd = mem [ r ] .hhfield .lhfield ; 
    rm = rd / 8 ; 
    w = 0 ; 
    while ( true ) {
	
      if ( pm < rm ) 
      mm = pm ; 
      else mm = rm ; 
      if ( w != 0 ) 
      if ( m != mm ) 
      {
	if ( mm - mmagic >= movesize ) 
	confusion ( 509 ) ; 
	extras = ( abs ( w ) - 1 ) / 3 ; 
	if ( extras > 0 ) 
	{
	  if ( w > 0 ) 
	  xw = 3 ; 
	  else xw = -3 ; 
	  ww = w - extras * xw ; 
	} 
	else ww = w ; 
	do {
	    j = m - mmagic ; 
	  {register integer for_end; k = 1 ; for_end = extras ; if ( k <= 
	  for_end) do 
	    {
	      s = getavail () ; 
	      mem [ s ] .hhfield .lhfield = nmagic + xw ; 
	      mem [ s ] .hhfield .v.RH = move [ j ] ; 
	      move [ j ] = s ; 
	    } 
	  while ( k++ < for_end ) ; } 
	  s = getavail () ; 
	  mem [ s ] .hhfield .lhfield = nmagic + ww ; 
	  mem [ s ] .hhfield .v.RH = move [ j ] ; 
	  move [ j ] = s ; 
	  incr ( m ) ; 
	} while ( ! ( m == mm ) ) ; 
      } 
      if ( pd < rd ) 
      {
	dw = ( pd % 8 ) - 4 ; 
	s = mem [ p ] .hhfield .v.RH ; 
	{
	  mem [ p ] .hhfield .v.RH = avail ; 
	  avail = p ; 
	;
#ifdef STAT
	  decr ( dynused ) ; 
#endif /* STAT */
	} 
	p = s ; 
	pd = mem [ p ] .hhfield .lhfield ; 
	pm = pd / 8 ; 
      } 
      else {
	  
	if ( r == memtop ) 
	goto lab30 ; 
	dw = - (integer) ( ( rd % 8 ) - 4 ) ; 
	r = mem [ r ] .hhfield .v.RH ; 
	rd = mem [ r ] .hhfield .lhfield ; 
	rm = rd / 8 ; 
      } 
      m = mm ; 
      w = w + dw ; 
    } 
    lab30: ; 
    p = q ; 
    nmagic = nmagic - 8 ; 
  } while ( ! ( mem [ p ] .hhfield .lhfield == curedges ) ) ; 
  freenode ( p , 2 ) ; 
  move [ mspread ] = 0 ; 
  j = 0 ; 
  while ( move [ j ] == memtop ) incr ( j ) ; 
  if ( j == mspread ) 
  initedges ( curedges ) ; 
  else {
      
    mm = mem [ curedges + 2 ] .hhfield .lhfield ; 
    mem [ curedges + 2 ] .hhfield .lhfield = mem [ curedges + 1 ] .hhfield 
    .lhfield ; 
    mem [ curedges + 2 ] .hhfield .v.RH = mem [ curedges + 1 ] .hhfield .v.RH 
    + 1 ; 
    mem [ curedges + 3 ] .hhfield .lhfield = 4096 ; 
    jj = mspread - 1 ; 
    while ( move [ jj ] == memtop ) decr ( jj ) ; 
    mem [ curedges + 1 ] .hhfield .lhfield = j + mm ; 
    mem [ curedges + 1 ] .hhfield .v.RH = jj + mm ; 
    q = curedges ; 
    do {
	p = getnode ( 2 ) ; 
      mem [ q ] .hhfield .v.RH = p ; 
      mem [ p ] .hhfield .lhfield = q ; 
      mem [ p + 1 ] .hhfield .v.RH = move [ j ] ; 
      mem [ p + 1 ] .hhfield .lhfield = 0 ; 
      incr ( j ) ; 
      q = p ; 
    } while ( ! ( j > jj ) ) ; 
    mem [ q ] .hhfield .v.RH = curedges ; 
    mem [ curedges ] .hhfield .lhfield = q ; 
    mem [ curedges + 5 ] .hhfield .lhfield = mem [ curedges + 1 ] .hhfield 
    .v.RH + 1 ; 
    mem [ curedges + 5 ] .hhfield .v.RH = curedges ; 
    mem [ curedges + 4 ] .cint = 0 ; 
  } 
} 
void zmergeedges ( h ) 
halfword h ; 
{/* 30 */ halfword p, q, r, pp, qq, rr  ; 
  integer n  ; 
  halfword k  ; 
  integer delta  ; 
  if ( mem [ h ] .hhfield .v.RH != h ) 
  {
    if ( ( mem [ h + 2 ] .hhfield .lhfield < mem [ curedges + 2 ] .hhfield 
    .lhfield ) || ( mem [ h + 2 ] .hhfield .v.RH > mem [ curedges + 2 ] 
    .hhfield .v.RH ) || ( mem [ h + 1 ] .hhfield .lhfield < mem [ curedges + 1 
    ] .hhfield .lhfield ) || ( mem [ h + 1 ] .hhfield .v.RH > mem [ curedges + 
    1 ] .hhfield .v.RH ) ) 
    edgeprep ( mem [ h + 2 ] .hhfield .lhfield - 4096 , mem [ h + 2 ] .hhfield 
    .v.RH - 4096 , mem [ h + 1 ] .hhfield .lhfield - 4096 , mem [ h + 1 ] 
    .hhfield .v.RH - 4095 ) ; 
    if ( mem [ h + 3 ] .hhfield .lhfield != mem [ curedges + 3 ] .hhfield 
    .lhfield ) 
    {
      pp = mem [ h ] .hhfield .v.RH ; 
      delta = 8 * ( mem [ curedges + 3 ] .hhfield .lhfield - mem [ h + 3 ] 
      .hhfield .lhfield ) ; 
      do {
	  qq = mem [ pp + 1 ] .hhfield .v.RH ; 
	while ( qq != memtop ) {
	    
	  mem [ qq ] .hhfield .lhfield = mem [ qq ] .hhfield .lhfield + delta 
	  ; 
	  qq = mem [ qq ] .hhfield .v.RH ; 
	} 
	qq = mem [ pp + 1 ] .hhfield .lhfield ; 
	while ( qq > 1 ) {
	    
	  mem [ qq ] .hhfield .lhfield = mem [ qq ] .hhfield .lhfield + delta 
	  ; 
	  qq = mem [ qq ] .hhfield .v.RH ; 
	} 
	pp = mem [ pp ] .hhfield .v.RH ; 
      } while ( ! ( pp == h ) ) ; 
    } 
    n = mem [ curedges + 1 ] .hhfield .lhfield ; 
    p = mem [ curedges ] .hhfield .v.RH ; 
    pp = mem [ h ] .hhfield .v.RH ; 
    while ( n < mem [ h + 1 ] .hhfield .lhfield ) {
	
      incr ( n ) ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } 
    do {
	qq = mem [ pp + 1 ] .hhfield .lhfield ; 
      if ( qq > 1 ) 
      if ( mem [ p + 1 ] .hhfield .lhfield <= 1 ) 
      mem [ p + 1 ] .hhfield .lhfield = qq ; 
      else {
	  
	while ( mem [ qq ] .hhfield .v.RH > 1 ) qq = mem [ qq ] .hhfield .v.RH 
	; 
	mem [ qq ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
	mem [ p + 1 ] .hhfield .lhfield = mem [ pp + 1 ] .hhfield .lhfield ; 
      } 
      mem [ pp + 1 ] .hhfield .lhfield = 0 ; 
      qq = mem [ pp + 1 ] .hhfield .v.RH ; 
      if ( qq != memtop ) 
      {
	if ( mem [ p + 1 ] .hhfield .lhfield == 1 ) 
	mem [ p + 1 ] .hhfield .lhfield = 0 ; 
	mem [ pp + 1 ] .hhfield .v.RH = memtop ; 
	r = p + 1 ; 
	q = mem [ r ] .hhfield .v.RH ; 
	if ( q == memtop ) 
	mem [ p + 1 ] .hhfield .v.RH = qq ; 
	else while ( true ) {
	    
	  k = mem [ qq ] .hhfield .lhfield ; 
	  while ( k > mem [ q ] .hhfield .lhfield ) {
	      
	    r = q ; 
	    q = mem [ r ] .hhfield .v.RH ; 
	  } 
	  mem [ r ] .hhfield .v.RH = qq ; 
	  rr = mem [ qq ] .hhfield .v.RH ; 
	  mem [ qq ] .hhfield .v.RH = q ; 
	  if ( rr == memtop ) 
	  goto lab30 ; 
	  r = qq ; 
	  qq = rr ; 
	} 
      } 
      lab30: ; 
      pp = mem [ pp ] .hhfield .v.RH ; 
      p = mem [ p ] .hhfield .v.RH ; 
    } while ( ! ( pp == h ) ) ; 
  } 
} 
integer ztotalweight ( h ) 
halfword h ; 
{register integer Result; halfword p, q  ; 
  integer n  ; 
  unsigned short m  ; 
  n = 0 ; 
  p = mem [ h ] .hhfield .v.RH ; 
  while ( p != h ) {
      
    q = mem [ p + 1 ] .hhfield .v.RH ; 
    while ( q != memtop ) {
	
      m = mem [ q ] .hhfield .lhfield ; 
      n = n - ( ( m % 8 ) - 4 ) * ( m / 8 ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    q = mem [ p + 1 ] .hhfield .lhfield ; 
    while ( q > 1 ) {
	
      m = mem [ q ] .hhfield .lhfield ; 
      n = n - ( ( m % 8 ) - 4 ) * ( m / 8 ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    p = mem [ p ] .hhfield .v.RH ; 
  } 
  Result = n ; 
  return(Result) ; 
} 
void beginedgetracing ( ) 
{printdiagnostic ( 540 , 283 , true ) ; 
  print ( 541 ) ; 
  printint ( curwt ) ; 
  printchar ( 41 ) ; 
  tracex = -4096 ; 
} 
void traceacorner ( ) 
{if ( fileoffset > maxprintline - 13 ) 
  printnl ( 283 ) ; 
  printchar ( 40 ) ; 
  printint ( tracex ) ; 
  printchar ( 44 ) ; 
  printint ( traceyy ) ; 
  printchar ( 41 ) ; 
  tracey = traceyy ; 
} 
void endedgetracing ( ) 
{if ( tracex == -4096 ) 
  printnl ( 542 ) ; 
  else {
      
    traceacorner () ; 
    printchar ( 46 ) ; 
  } 
  enddiagnostic ( true ) ; 
} 
void ztracenewedge ( r , n ) 
halfword r ; 
integer n ; 
{integer d  ; 
  schar w  ; 
  integer m, n0, n1  ; 
  d = mem [ r ] .hhfield .lhfield ; 
  w = ( d % 8 ) - 4 ; 
  m = ( d / 8 ) - mem [ curedges + 3 ] .hhfield .lhfield ; 
  if ( w == curwt ) 
  {
    n0 = n + 1 ; 
    n1 = n ; 
  } 
  else {
      
    n0 = n ; 
    n1 = n + 1 ; 
  } 
  if ( m != tracex ) 
  {
    if ( tracex == -4096 ) 
    {
      printnl ( 283 ) ; 
      traceyy = n0 ; 
    } 
    else if ( traceyy != n0 ) 
    printchar ( 63 ) ; 
    else traceacorner () ; 
    tracex = m ; 
    traceacorner () ; 
  } 
  else {
      
    if ( n0 != traceyy ) 
    printchar ( 33 ) ; 
    if ( ( ( n0 < n1 ) && ( tracey > traceyy ) ) || ( ( n0 > n1 ) && ( tracey 
    < traceyy ) ) ) 
    traceacorner () ; 
  } 
  traceyy = n1 ; 
} 
void zlineedges ( x0 , y0 , x1 , y1 ) 
scaled x0 ; 
scaled y0 ; 
scaled x1 ; 
scaled y1 ; 
{/* 30 31 */ integer m0, n0, m1, n1  ; 
  scaled delx, dely  ; 
  scaled yt  ; 
  scaled tx  ; 
  halfword p, r  ; 
  integer base  ; 
  integer n  ; 
  n0 = roundunscaled ( y0 ) ; 
  n1 = roundunscaled ( y1 ) ; 
  if ( n0 != n1 ) 
  {
    m0 = roundunscaled ( x0 ) ; 
    m1 = roundunscaled ( x1 ) ; 
    delx = x1 - x0 ; 
    dely = y1 - y0 ; 
    yt = n0 * 65536L - 32768L ; 
    y0 = y0 - yt ; 
    y1 = y1 - yt ; 
    if ( n0 < n1 ) 
    {
      base = 8 * mem [ curedges + 3 ] .hhfield .lhfield + 4 - curwt ; 
      if ( m0 <= m1 ) 
      edgeprep ( m0 , m1 , n0 , n1 ) ; 
      else edgeprep ( m1 , m0 , n0 , n1 ) ; 
      n = mem [ curedges + 5 ] .hhfield .lhfield - 4096 ; 
      p = mem [ curedges + 5 ] .hhfield .v.RH ; 
      if ( n != n0 ) 
      if ( n < n0 ) 
      do {
	  incr ( n ) ; 
	p = mem [ p ] .hhfield .v.RH ; 
      } while ( ! ( n == n0 ) ) ; 
      else do {
	  decr ( n ) ; 
	p = mem [ p ] .hhfield .lhfield ; 
      } while ( ! ( n == n0 ) ) ; 
      y0 = 65536L - y0 ; 
      while ( true ) {
	  
	r = getavail () ; 
	mem [ r ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
	mem [ p + 1 ] .hhfield .lhfield = r ; 
	tx = takefraction ( delx , makefraction ( y0 , dely ) ) ; 
	if ( abvscd ( delx , y0 , dely , tx ) < 0 ) 
	decr ( tx ) ; 
	mem [ r ] .hhfield .lhfield = 8 * roundunscaled ( x0 + tx ) + base ; 
	y1 = y1 - 65536L ; 
	if ( internal [ 10 ] > 0 ) 
	tracenewedge ( r , n ) ; 
	if ( y1 < 65536L ) 
	goto lab30 ; 
	p = mem [ p ] .hhfield .v.RH ; 
	y0 = y0 + 65536L ; 
	incr ( n ) ; 
      } 
      lab30: ; 
    } 
    else {
	
      base = 8 * mem [ curedges + 3 ] .hhfield .lhfield + 4 + curwt ; 
      if ( m0 <= m1 ) 
      edgeprep ( m0 , m1 , n1 , n0 ) ; 
      else edgeprep ( m1 , m0 , n1 , n0 ) ; 
      decr ( n0 ) ; 
      n = mem [ curedges + 5 ] .hhfield .lhfield - 4096 ; 
      p = mem [ curedges + 5 ] .hhfield .v.RH ; 
      if ( n != n0 ) 
      if ( n < n0 ) 
      do {
	  incr ( n ) ; 
	p = mem [ p ] .hhfield .v.RH ; 
      } while ( ! ( n == n0 ) ) ; 
      else do {
	  decr ( n ) ; 
	p = mem [ p ] .hhfield .lhfield ; 
      } while ( ! ( n == n0 ) ) ; 
      while ( true ) {
	  
	r = getavail () ; 
	mem [ r ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
	mem [ p + 1 ] .hhfield .lhfield = r ; 
	tx = takefraction ( delx , makefraction ( y0 , dely ) ) ; 
	if ( abvscd ( delx , y0 , dely , tx ) < 0 ) 
	incr ( tx ) ; 
	mem [ r ] .hhfield .lhfield = 8 * roundunscaled ( x0 - tx ) + base ; 
	y1 = y1 + 65536L ; 
	if ( internal [ 10 ] > 0 ) 
	tracenewedge ( r , n ) ; 
	if ( y1 >= 0 ) 
	goto lab31 ; 
	p = mem [ p ] .hhfield .lhfield ; 
	y0 = y0 + 65536L ; 
	decr ( n ) ; 
      } 
      lab31: ; 
    } 
    mem [ curedges + 5 ] .hhfield .v.RH = p ; 
    mem [ curedges + 5 ] .hhfield .lhfield = n + 4096 ; 
  } 
} 
void zmovetoedges ( m0 , n0 , m1 , n1 ) 
integer m0 ; 
integer n0 ; 
integer m1 ; 
integer n1 ; 
{/* 60 61 62 63 30 */ integer delta  ; 
  integer k  ; 
  halfword p, r  ; 
  integer dx  ; 
  integer edgeandweight  ; 
  integer j  ; 
  integer n  ; 
#ifdef DEBUG
  integer sum  ; 
#endif /* DEBUG */
  delta = n1 - n0 ; 
	;
#ifdef DEBUG
  sum = move [ 0 ] ; 
  {register integer for_end; k = 1 ; for_end = delta ; if ( k <= for_end) do 
    sum = sum + abs ( move [ k ] ) ; 
  while ( k++ < for_end ) ; } 
  if ( sum != m1 - m0 ) 
  confusion ( 48 ) ; 
#endif /* DEBUG */
  switch ( octant ) 
  {case 1 : 
    {
      dx = 8 ; 
      edgeprep ( m0 , m1 , n0 , n1 ) ; 
      goto lab60 ; 
    } 
    break ; 
  case 5 : 
    {
      dx = 8 ; 
      edgeprep ( n0 , n1 , m0 , m1 ) ; 
      goto lab62 ; 
    } 
    break ; 
  case 6 : 
    {
      dx = -8 ; 
      edgeprep ( - (integer) n1 , - (integer) n0 , m0 , m1 ) ; 
      n0 = - (integer) n0 ; 
      goto lab62 ; 
    } 
    break ; 
  case 2 : 
    {
      dx = -8 ; 
      edgeprep ( - (integer) m1 , - (integer) m0 , n0 , n1 ) ; 
      m0 = - (integer) m0 ; 
      goto lab60 ; 
    } 
    break ; 
  case 4 : 
    {
      dx = -8 ; 
      edgeprep ( - (integer) m1 , - (integer) m0 , - (integer) n1 , 
      - (integer) n0 ) ; 
      m0 = - (integer) m0 ; 
      goto lab61 ; 
    } 
    break ; 
  case 8 : 
    {
      dx = -8 ; 
      edgeprep ( - (integer) n1 , - (integer) n0 , - (integer) m1 , 
      - (integer) m0 ) ; 
      n0 = - (integer) n0 ; 
      goto lab63 ; 
    } 
    break ; 
  case 7 : 
    {
      dx = 8 ; 
      edgeprep ( n0 , n1 , - (integer) m1 , - (integer) m0 ) ; 
      goto lab63 ; 
    } 
    break ; 
  case 3 : 
    {
      dx = 8 ; 
      edgeprep ( m0 , m1 , - (integer) n1 , - (integer) n0 ) ; 
      goto lab61 ; 
    } 
    break ; 
  } 
  lab60: n = mem [ curedges + 5 ] .hhfield .lhfield - 4096 ; 
  p = mem [ curedges + 5 ] .hhfield .v.RH ; 
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } while ( ! ( n == n0 ) ) ; 
  else do {
      decr ( n ) ; 
    p = mem [ p ] .hhfield .lhfield ; 
  } while ( ! ( n == n0 ) ) ; 
  if ( delta > 0 ) 
  {
    k = 0 ; 
    edgeandweight = 8 * ( m0 + mem [ curedges + 3 ] .hhfield .lhfield ) + 4 - 
    curwt ; 
    do {
	edgeandweight = edgeandweight + dx * move [ k ] ; 
      {
	r = avail ; 
	if ( r == 0 ) 
	r = getavail () ; 
	else {
	    
	  avail = mem [ r ] .hhfield .v.RH ; 
	  mem [ r ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
	  incr ( dynused ) ; 
#endif /* STAT */
	} 
      } 
      mem [ r ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
      mem [ r ] .hhfield .lhfield = edgeandweight ; 
      if ( internal [ 10 ] > 0 ) 
      tracenewedge ( r , n ) ; 
      mem [ p + 1 ] .hhfield .lhfield = r ; 
      p = mem [ p ] .hhfield .v.RH ; 
      incr ( k ) ; 
      incr ( n ) ; 
    } while ( ! ( k == delta ) ) ; 
  } 
  goto lab30 ; 
  lab61: n0 = - (integer) n0 - 1 ; 
  n = mem [ curedges + 5 ] .hhfield .lhfield - 4096 ; 
  p = mem [ curedges + 5 ] .hhfield .v.RH ; 
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } while ( ! ( n == n0 ) ) ; 
  else do {
      decr ( n ) ; 
    p = mem [ p ] .hhfield .lhfield ; 
  } while ( ! ( n == n0 ) ) ; 
  if ( delta > 0 ) 
  {
    k = 0 ; 
    edgeandweight = 8 * ( m0 + mem [ curedges + 3 ] .hhfield .lhfield ) + 4 + 
    curwt ; 
    do {
	edgeandweight = edgeandweight + dx * move [ k ] ; 
      {
	r = avail ; 
	if ( r == 0 ) 
	r = getavail () ; 
	else {
	    
	  avail = mem [ r ] .hhfield .v.RH ; 
	  mem [ r ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
	  incr ( dynused ) ; 
#endif /* STAT */
	} 
      } 
      mem [ r ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
      mem [ r ] .hhfield .lhfield = edgeandweight ; 
      if ( internal [ 10 ] > 0 ) 
      tracenewedge ( r , n ) ; 
      mem [ p + 1 ] .hhfield .lhfield = r ; 
      p = mem [ p ] .hhfield .lhfield ; 
      incr ( k ) ; 
      decr ( n ) ; 
    } while ( ! ( k == delta ) ) ; 
  } 
  goto lab30 ; 
  lab62: edgeandweight = 8 * ( n0 + mem [ curedges + 3 ] .hhfield .lhfield ) + 
  4 - curwt ; 
  n0 = m0 ; 
  k = 0 ; 
  n = mem [ curedges + 5 ] .hhfield .lhfield - 4096 ; 
  p = mem [ curedges + 5 ] .hhfield .v.RH ; 
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } while ( ! ( n == n0 ) ) ; 
  else do {
      decr ( n ) ; 
    p = mem [ p ] .hhfield .lhfield ; 
  } while ( ! ( n == n0 ) ) ; 
  do {
      j = move [ k ] ; 
    while ( j > 0 ) {
	
      {
	r = avail ; 
	if ( r == 0 ) 
	r = getavail () ; 
	else {
	    
	  avail = mem [ r ] .hhfield .v.RH ; 
	  mem [ r ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
	  incr ( dynused ) ; 
#endif /* STAT */
	} 
      } 
      mem [ r ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
      mem [ r ] .hhfield .lhfield = edgeandweight ; 
      if ( internal [ 10 ] > 0 ) 
      tracenewedge ( r , n ) ; 
      mem [ p + 1 ] .hhfield .lhfield = r ; 
      p = mem [ p ] .hhfield .v.RH ; 
      decr ( j ) ; 
      incr ( n ) ; 
    } 
    edgeandweight = edgeandweight + dx ; 
    incr ( k ) ; 
  } while ( ! ( k > delta ) ) ; 
  goto lab30 ; 
  lab63: edgeandweight = 8 * ( n0 + mem [ curedges + 3 ] .hhfield .lhfield ) + 
  4 + curwt ; 
  n0 = - (integer) m0 - 1 ; 
  k = 0 ; 
  n = mem [ curedges + 5 ] .hhfield .lhfield - 4096 ; 
  p = mem [ curedges + 5 ] .hhfield .v.RH ; 
  if ( n != n0 ) 
  if ( n < n0 ) 
  do {
      incr ( n ) ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } while ( ! ( n == n0 ) ) ; 
  else do {
      decr ( n ) ; 
    p = mem [ p ] .hhfield .lhfield ; 
  } while ( ! ( n == n0 ) ) ; 
  do {
      j = move [ k ] ; 
    while ( j > 0 ) {
	
      {
	r = avail ; 
	if ( r == 0 ) 
	r = getavail () ; 
	else {
	    
	  avail = mem [ r ] .hhfield .v.RH ; 
	  mem [ r ] .hhfield .v.RH = 0 ; 
	;
#ifdef STAT
	  incr ( dynused ) ; 
#endif /* STAT */
	} 
      } 
      mem [ r ] .hhfield .v.RH = mem [ p + 1 ] .hhfield .lhfield ; 
      mem [ r ] .hhfield .lhfield = edgeandweight ; 
      if ( internal [ 10 ] > 0 ) 
      tracenewedge ( r , n ) ; 
      mem [ p + 1 ] .hhfield .lhfield = r ; 
      p = mem [ p ] .hhfield .lhfield ; 
      decr ( j ) ; 
      decr ( n ) ; 
    } 
    edgeandweight = edgeandweight + dx ; 
    incr ( k ) ; 
  } while ( ! ( k > delta ) ) ; 
  goto lab30 ; 
  lab30: mem [ curedges + 5 ] .hhfield .lhfield = n + 4096 ; 
  mem [ curedges + 5 ] .hhfield .v.RH = p ; 
} 
void zskew ( x , y , octant ) 
scaled x ; 
scaled y ; 
smallnumber octant ; 
{switch ( octant ) 
  {case 1 : 
    {
      curx = x - y ; 
      cury = y ; 
    } 
    break ; 
  case 5 : 
    {
      curx = y - x ; 
      cury = x ; 
    } 
    break ; 
  case 6 : 
    {
      curx = y + x ; 
      cury = - (integer) x ; 
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
      curx = - (integer) x + y ; 
      cury = - (integer) y ; 
    } 
    break ; 
  case 8 : 
    {
      curx = - (integer) y + x ; 
      cury = - (integer) x ; 
    } 
    break ; 
  case 7 : 
    {
      curx = - (integer) y - x ; 
      cury = x ; 
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
void zabnegate ( x , y , octantbefore , octantafter ) 
scaled x ; 
scaled y ; 
smallnumber octantbefore ; 
smallnumber octantafter ; 
{if ( odd ( octantbefore ) == odd ( octantafter ) ) 
  curx = x ; 
  else curx = - (integer) x ; 
  if ( ( octantbefore > 2 ) == ( octantafter > 2 ) ) 
  cury = y ; 
  else cury = - (integer) y ; 
} 
fraction zcrossingpoint ( a , b , c ) 
integer a ; 
integer b ; 
integer c ; 
{/* 10 */ register fraction Result; integer d  ; 
  integer x, xx, x0, x1, x2  ; 
  if ( a < 0 ) 
  {
    Result = 0 ; 
    goto lab10 ; 
  } 
  if ( c >= 0 ) 
  {
    if ( b >= 0 ) 
    if ( c > 0 ) 
    {
      Result = 268435457L ; 
      goto lab10 ; 
    } 
    else if ( ( a == 0 ) && ( b == 0 ) ) 
    {
      Result = 268435457L ; 
      goto lab10 ; 
    } 
    else {
	
      Result = 268435456L ; 
      goto lab10 ; 
    } 
    if ( a == 0 ) 
    {
      Result = 0 ; 
      goto lab10 ; 
    } 
  } 
  else if ( a == 0 ) 
  if ( b <= 0 ) 
  {
    Result = 0 ; 
    goto lab10 ; 
  } 
  d = 1 ; 
  x0 = a ; 
  x1 = a - b ; 
  x2 = b - c ; 
  do {
      x = ( x1 + x2 ) / 2 ; 
    if ( x1 - x0 > x0 ) 
    {
      x2 = x ; 
      x0 = x0 + x0 ; 
      d = d + d ; 
    } 
    else {
	
      xx = x1 + x - x0 ; 
      if ( xx > x0 ) 
      {
	x2 = x ; 
	x0 = x0 + x0 ; 
	d = d + d ; 
      } 
      else {
	  
	x0 = x0 - xx ; 
	if ( x <= x0 ) 
	if ( x + x2 <= x0 ) 
	{
	  Result = 268435457L ; 
	  goto lab10 ; 
	} 
	x1 = x ; 
	d = d + d + 1 ; 
      } 
    } 
  } while ( ! ( d >= 268435456L ) ) ; 
  Result = d - 268435456L ; 
  lab10: ; 
  return(Result) ; 
} 
void zprintspec ( s ) 
strnumber s ; 
{/* 45 30 */ halfword p, q  ; 
  smallnumber octant  ; 
  printdiagnostic ( 543 , s , true ) ; 
  p = curspec ; 
  octant = mem [ p + 3 ] .cint ; 
  println () ; 
  unskew ( mem [ curspec + 1 ] .cint , mem [ curspec + 2 ] .cint , octant ) ; 
  printtwo ( curx , cury ) ; 
  print ( 544 ) ; 
  while ( true ) {
      
    print ( octantdir [ octant ] ) ; 
    printchar ( 39 ) ; 
    while ( true ) {
	
      q = mem [ p ] .hhfield .v.RH ; 
      if ( mem [ p ] .hhfield .b1 == 0 ) 
      goto lab45 ; 
      {
	printnl ( 555 ) ; 
	unskew ( mem [ p + 5 ] .cint , mem [ p + 6 ] .cint , octant ) ; 
	printtwo ( curx , cury ) ; 
	print ( 522 ) ; 
	unskew ( mem [ q + 3 ] .cint , mem [ q + 4 ] .cint , octant ) ; 
	printtwo ( curx , cury ) ; 
	printnl ( 519 ) ; 
	unskew ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint , octant ) ; 
	printtwo ( curx , cury ) ; 
	print ( 556 ) ; 
	printint ( mem [ q ] .hhfield .b0 - 1 ) ; 
      } 
      p = q ; 
    } 
    lab45: if ( q == curspec ) 
    goto lab30 ; 
    p = q ; 
    octant = mem [ p + 3 ] .cint ; 
    printnl ( 545 ) ; 
  } 
  lab30: printnl ( 546 ) ; 
  enddiagnostic ( true ) ; 
} 
void zprintstrange ( s ) 
strnumber s ; 
{halfword p  ; 
  halfword f  ; 
  halfword q  ; 
  integer t  ; 
  if ( interaction == 3 ) 
  ; 
  printnl ( 62 ) ; 
  p = curspec ; 
  t = 256 ; 
  do {
      p = mem [ p ] .hhfield .v.RH ; 
    if ( mem [ p ] .hhfield .b0 != 0 ) 
    {
      if ( mem [ p ] .hhfield .b0 < t ) 
      f = p ; 
      t = mem [ p ] .hhfield .b0 ; 
    } 
  } while ( ! ( p == curspec ) ) ; 
  p = curspec ; 
  q = p ; 
  do {
      p = mem [ p ] .hhfield .v.RH ; 
    if ( mem [ p ] .hhfield .b0 == 0 ) 
    q = p ; 
  } while ( ! ( p == f ) ) ; 
  t = 0 ; 
  do {
      if ( mem [ p ] .hhfield .b0 != 0 ) 
    {
      if ( mem [ p ] .hhfield .b0 != t ) 
      {
	t = mem [ p ] .hhfield .b0 ; 
	printchar ( 32 ) ; 
	printint ( t - 1 ) ; 
      } 
      if ( q != 0 ) 
      {
	if ( mem [ mem [ q ] .hhfield .v.RH ] .hhfield .b0 == 0 ) 
	{
	  print ( 557 ) ; 
	  print ( octantdir [ mem [ q + 3 ] .cint ] ) ; 
	  q = mem [ q ] .hhfield .v.RH ; 
	  while ( mem [ mem [ q ] .hhfield .v.RH ] .hhfield .b0 == 0 ) {
	      
	    printchar ( 32 ) ; 
	    print ( octantdir [ mem [ q + 3 ] .cint ] ) ; 
	    q = mem [ q ] .hhfield .v.RH ; 
	  } 
	  printchar ( 41 ) ; 
	} 
	printchar ( 32 ) ; 
	print ( octantdir [ mem [ q + 3 ] .cint ] ) ; 
	q = 0 ; 
      } 
    } 
    else if ( q == 0 ) 
    q = p ; 
    p = mem [ p ] .hhfield .v.RH ; 
  } while ( ! ( p == f ) ) ; 
  printchar ( 32 ) ; 
  printint ( mem [ p ] .hhfield .b0 - 1 ) ; 
  if ( q != 0 ) 
  if ( mem [ mem [ q ] .hhfield .v.RH ] .hhfield .b0 == 0 ) 
  {
    print ( 557 ) ; 
    print ( octantdir [ mem [ q + 3 ] .cint ] ) ; 
    q = mem [ q ] .hhfield .v.RH ; 
    while ( mem [ mem [ q ] .hhfield .v.RH ] .hhfield .b0 == 0 ) {
	
      printchar ( 32 ) ; 
      print ( octantdir [ mem [ q + 3 ] .cint ] ) ; 
      q = mem [ q ] .hhfield .v.RH ; 
    } 
    printchar ( 41 ) ; 
  } 
  {
    if ( interaction == 3 ) 
    ; 
    printnl ( 261 ) ; 
    print ( s ) ; 
  } 
} 
void zremovecubic ( p ) 
halfword p ; 
{halfword q  ; 
  q = mem [ p ] .hhfield .v.RH ; 
  mem [ p ] .hhfield .b1 = mem [ q ] .hhfield .b1 ; 
  mem [ p ] .hhfield .v.RH = mem [ q ] .hhfield .v.RH ; 
  mem [ p + 1 ] .cint = mem [ q + 1 ] .cint ; 
  mem [ p + 2 ] .cint = mem [ q + 2 ] .cint ; 
  mem [ p + 5 ] .cint = mem [ q + 5 ] .cint ; 
  mem [ p + 6 ] .cint = mem [ q + 6 ] .cint ; 
  freenode ( q , 7 ) ; 
} 
void zsplitcubic ( p , t , xq , yq ) 
halfword p ; 
fraction t ; 
scaled xq ; 
scaled yq ; 
{scaled v  ; 
  halfword q, r  ; 
  q = mem [ p ] .hhfield .v.RH ; 
  r = getnode ( 7 ) ; 
  mem [ p ] .hhfield .v.RH = r ; 
  mem [ r ] .hhfield .v.RH = q ; 
  mem [ r ] .hhfield .b0 = mem [ q ] .hhfield .b0 ; 
  mem [ r ] .hhfield .b1 = mem [ p ] .hhfield .b1 ; 
  v = mem [ p + 5 ] .cint - takefraction ( mem [ p + 5 ] .cint - mem [ q + 3 ] 
  .cint , t ) ; 
  mem [ p + 5 ] .cint = mem [ p + 1 ] .cint - takefraction ( mem [ p + 1 ] 
  .cint - mem [ p + 5 ] .cint , t ) ; 
  mem [ q + 3 ] .cint = mem [ q + 3 ] .cint - takefraction ( mem [ q + 3 ] 
  .cint - xq , t ) ; 
  mem [ r + 3 ] .cint = mem [ p + 5 ] .cint - takefraction ( mem [ p + 5 ] 
  .cint - v , t ) ; 
  mem [ r + 5 ] .cint = v - takefraction ( v - mem [ q + 3 ] .cint , t ) ; 
  mem [ r + 1 ] .cint = mem [ r + 3 ] .cint - takefraction ( mem [ r + 3 ] 
  .cint - mem [ r + 5 ] .cint , t ) ; 
  v = mem [ p + 6 ] .cint - takefraction ( mem [ p + 6 ] .cint - mem [ q + 4 ] 
  .cint , t ) ; 
  mem [ p + 6 ] .cint = mem [ p + 2 ] .cint - takefraction ( mem [ p + 2 ] 
  .cint - mem [ p + 6 ] .cint , t ) ; 
  mem [ q + 4 ] .cint = mem [ q + 4 ] .cint - takefraction ( mem [ q + 4 ] 
  .cint - yq , t ) ; 
  mem [ r + 4 ] .cint = mem [ p + 6 ] .cint - takefraction ( mem [ p + 6 ] 
  .cint - v , t ) ; 
  mem [ r + 6 ] .cint = v - takefraction ( v - mem [ q + 4 ] .cint , t ) ; 
  mem [ r + 2 ] .cint = mem [ r + 4 ] .cint - takefraction ( mem [ r + 4 ] 
  .cint - mem [ r + 6 ] .cint , t ) ; 
} 
void quadrantsubdivide ( ) 
{/* 22 10 */ halfword p, q, r, s, pp, qq  ; 
  scaled firstx, firsty  ; 
  scaled del1, del2, del3, del, dmax  ; 
  fraction t  ; 
  scaled destx, desty  ; 
  boolean constantx  ; 
  p = curspec ; 
  firstx = mem [ curspec + 1 ] .cint ; 
  firsty = mem [ curspec + 2 ] .cint ; 
  do {
      lab22: q = mem [ p ] .hhfield .v.RH ; 
    if ( q == curspec ) 
    {
      destx = firstx ; 
      desty = firsty ; 
    } 
    else {
	
      destx = mem [ q + 1 ] .cint ; 
      desty = mem [ q + 2 ] .cint ; 
    } 
    del1 = mem [ p + 5 ] .cint - mem [ p + 1 ] .cint ; 
    del2 = mem [ q + 3 ] .cint - mem [ p + 5 ] .cint ; 
    del3 = destx - mem [ q + 3 ] .cint ; 
    if ( del1 != 0 ) 
    del = del1 ; 
    else if ( del2 != 0 ) 
    del = del2 ; 
    else del = del3 ; 
    if ( del != 0 ) 
    {
      dmax = abs ( del1 ) ; 
      if ( abs ( del2 ) > dmax ) 
      dmax = abs ( del2 ) ; 
      if ( abs ( del3 ) > dmax ) 
      dmax = abs ( del3 ) ; 
      while ( dmax < 134217728L ) {
	  
	dmax = dmax + dmax ; 
	del1 = del1 + del1 ; 
	del2 = del2 + del2 ; 
	del3 = del3 + del3 ; 
      } 
    } 
    if ( del == 0 ) 
    constantx = true ; 
    else {
	
      constantx = false ; 
      if ( del < 0 ) 
      {
	mem [ p + 1 ] .cint = - (integer) mem [ p + 1 ] .cint ; 
	mem [ p + 5 ] .cint = - (integer) mem [ p + 5 ] .cint ; 
	mem [ q + 3 ] .cint = - (integer) mem [ q + 3 ] .cint ; 
	del1 = - (integer) del1 ; 
	del2 = - (integer) del2 ; 
	del3 = - (integer) del3 ; 
	destx = - (integer) destx ; 
	mem [ p ] .hhfield .b1 = 2 ; 
      } 
      t = crossingpoint ( del1 , del2 , del3 ) ; 
      if ( t < 268435456L ) 
      {
	splitcubic ( p , t , destx , desty ) ; 
	r = mem [ p ] .hhfield .v.RH ; 
	if ( mem [ r ] .hhfield .b1 > 1 ) 
	mem [ r ] .hhfield .b1 = 1 ; 
	else mem [ r ] .hhfield .b1 = 2 ; 
	if ( mem [ r + 1 ] .cint < mem [ p + 1 ] .cint ) 
	mem [ r + 1 ] .cint = mem [ p + 1 ] .cint ; 
	mem [ r + 3 ] .cint = mem [ r + 1 ] .cint ; 
	if ( mem [ p + 5 ] .cint > mem [ r + 1 ] .cint ) 
	mem [ p + 5 ] .cint = mem [ r + 1 ] .cint ; 
	mem [ r + 1 ] .cint = - (integer) mem [ r + 1 ] .cint ; 
	mem [ r + 5 ] .cint = mem [ r + 1 ] .cint ; 
	mem [ q + 3 ] .cint = - (integer) mem [ q + 3 ] .cint ; 
	destx = - (integer) destx ; 
	del2 = del2 - takefraction ( del2 - del3 , t ) ; 
	if ( del2 > 0 ) 
	del2 = 0 ; 
	t = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ; 
	if ( t < 268435456L ) 
	{
	  splitcubic ( r , t , destx , desty ) ; 
	  s = mem [ r ] .hhfield .v.RH ; 
	  if ( mem [ s + 1 ] .cint < destx ) 
	  mem [ s + 1 ] .cint = destx ; 
	  if ( mem [ s + 1 ] .cint < mem [ r + 1 ] .cint ) 
	  mem [ s + 1 ] .cint = mem [ r + 1 ] .cint ; 
	  mem [ s ] .hhfield .b1 = mem [ p ] .hhfield .b1 ; 
	  mem [ s + 3 ] .cint = mem [ s + 1 ] .cint ; 
	  if ( mem [ q + 3 ] .cint < destx ) 
	  mem [ q + 3 ] .cint = - (integer) destx ; 
	  else if ( mem [ q + 3 ] .cint > mem [ s + 1 ] .cint ) 
	  mem [ q + 3 ] .cint = - (integer) mem [ s + 1 ] .cint ; 
	  else mem [ q + 3 ] .cint = - (integer) mem [ q + 3 ] .cint ; 
	  mem [ s + 1 ] .cint = - (integer) mem [ s + 1 ] .cint ; 
	  mem [ s + 5 ] .cint = mem [ s + 1 ] .cint ; 
	} 
	else {
	    
	  if ( mem [ r + 1 ] .cint > destx ) 
	  {
	    mem [ r + 1 ] .cint = destx ; 
	    mem [ r + 3 ] .cint = - (integer) mem [ r + 1 ] .cint ; 
	    mem [ r + 5 ] .cint = mem [ r + 1 ] .cint ; 
	  } 
	  if ( mem [ q + 3 ] .cint > destx ) 
	  mem [ q + 3 ] .cint = destx ; 
	  else if ( mem [ q + 3 ] .cint < mem [ r + 1 ] .cint ) 
	  mem [ q + 3 ] .cint = mem [ r + 1 ] .cint ; 
	} 
      } 
    } 
    pp = p ; 
    do {
	qq = mem [ pp ] .hhfield .v.RH ; 
      abnegate ( mem [ qq + 1 ] .cint , mem [ qq + 2 ] .cint , mem [ qq ] 
      .hhfield .b1 , mem [ pp ] .hhfield .b1 ) ; 
      destx = curx ; 
      desty = cury ; 
      del1 = mem [ pp + 6 ] .cint - mem [ pp + 2 ] .cint ; 
      del2 = mem [ qq + 4 ] .cint - mem [ pp + 6 ] .cint ; 
      del3 = desty - mem [ qq + 4 ] .cint ; 
      if ( del1 != 0 ) 
      del = del1 ; 
      else if ( del2 != 0 ) 
      del = del2 ; 
      else del = del3 ; 
      if ( del != 0 ) 
      {
	dmax = abs ( del1 ) ; 
	if ( abs ( del2 ) > dmax ) 
	dmax = abs ( del2 ) ; 
	if ( abs ( del3 ) > dmax ) 
	dmax = abs ( del3 ) ; 
	while ( dmax < 134217728L ) {
	    
	  dmax = dmax + dmax ; 
	  del1 = del1 + del1 ; 
	  del2 = del2 + del2 ; 
	  del3 = del3 + del3 ; 
	} 
      } 
      if ( del != 0 ) 
      {
	if ( del < 0 ) 
	{
	  mem [ pp + 2 ] .cint = - (integer) mem [ pp + 2 ] .cint ; 
	  mem [ pp + 6 ] .cint = - (integer) mem [ pp + 6 ] .cint ; 
	  mem [ qq + 4 ] .cint = - (integer) mem [ qq + 4 ] .cint ; 
	  del1 = - (integer) del1 ; 
	  del2 = - (integer) del2 ; 
	  del3 = - (integer) del3 ; 
	  desty = - (integer) desty ; 
	  mem [ pp ] .hhfield .b1 = mem [ pp ] .hhfield .b1 + 2 ; 
	} 
	t = crossingpoint ( del1 , del2 , del3 ) ; 
	if ( t < 268435456L ) 
	{
	  splitcubic ( pp , t , destx , desty ) ; 
	  r = mem [ pp ] .hhfield .v.RH ; 
	  if ( mem [ r ] .hhfield .b1 > 2 ) 
	  mem [ r ] .hhfield .b1 = mem [ r ] .hhfield .b1 - 2 ; 
	  else mem [ r ] .hhfield .b1 = mem [ r ] .hhfield .b1 + 2 ; 
	  if ( mem [ r + 2 ] .cint < mem [ pp + 2 ] .cint ) 
	  mem [ r + 2 ] .cint = mem [ pp + 2 ] .cint ; 
	  mem [ r + 4 ] .cint = mem [ r + 2 ] .cint ; 
	  if ( mem [ pp + 6 ] .cint > mem [ r + 2 ] .cint ) 
	  mem [ pp + 6 ] .cint = mem [ r + 2 ] .cint ; 
	  mem [ r + 2 ] .cint = - (integer) mem [ r + 2 ] .cint ; 
	  mem [ r + 6 ] .cint = mem [ r + 2 ] .cint ; 
	  mem [ qq + 4 ] .cint = - (integer) mem [ qq + 4 ] .cint ; 
	  desty = - (integer) desty ; 
	  if ( mem [ r + 1 ] .cint < mem [ pp + 1 ] .cint ) 
	  mem [ r + 1 ] .cint = mem [ pp + 1 ] .cint ; 
	  else if ( mem [ r + 1 ] .cint > destx ) 
	  mem [ r + 1 ] .cint = destx ; 
	  if ( mem [ r + 3 ] .cint > mem [ r + 1 ] .cint ) 
	  {
	    mem [ r + 3 ] .cint = mem [ r + 1 ] .cint ; 
	    if ( mem [ pp + 5 ] .cint > mem [ r + 1 ] .cint ) 
	    mem [ pp + 5 ] .cint = mem [ r + 1 ] .cint ; 
	  } 
	  if ( mem [ r + 5 ] .cint < mem [ r + 1 ] .cint ) 
	  {
	    mem [ r + 5 ] .cint = mem [ r + 1 ] .cint ; 
	    if ( mem [ qq + 3 ] .cint < mem [ r + 1 ] .cint ) 
	    mem [ qq + 3 ] .cint = mem [ r + 1 ] .cint ; 
	  } 
	  del2 = del2 - takefraction ( del2 - del3 , t ) ; 
	  if ( del2 > 0 ) 
	  del2 = 0 ; 
	  t = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ; 
	  if ( t < 268435456L ) 
	  {
	    splitcubic ( r , t , destx , desty ) ; 
	    s = mem [ r ] .hhfield .v.RH ; 
	    if ( mem [ s + 2 ] .cint < desty ) 
	    mem [ s + 2 ] .cint = desty ; 
	    if ( mem [ s + 2 ] .cint < mem [ r + 2 ] .cint ) 
	    mem [ s + 2 ] .cint = mem [ r + 2 ] .cint ; 
	    mem [ s ] .hhfield .b1 = mem [ pp ] .hhfield .b1 ; 
	    mem [ s + 4 ] .cint = mem [ s + 2 ] .cint ; 
	    if ( mem [ qq + 4 ] .cint < desty ) 
	    mem [ qq + 4 ] .cint = - (integer) desty ; 
	    else if ( mem [ qq + 4 ] .cint > mem [ s + 2 ] .cint ) 
	    mem [ qq + 4 ] .cint = - (integer) mem [ s + 2 ] .cint ; 
	    else mem [ qq + 4 ] .cint = - (integer) mem [ qq + 4 ] .cint ; 
	    mem [ s + 2 ] .cint = - (integer) mem [ s + 2 ] .cint ; 
	    mem [ s + 6 ] .cint = mem [ s + 2 ] .cint ; 
	    if ( mem [ s + 1 ] .cint < mem [ r + 1 ] .cint ) 
	    mem [ s + 1 ] .cint = mem [ r + 1 ] .cint ; 
	    else if ( mem [ s + 1 ] .cint > destx ) 
	    mem [ s + 1 ] .cint = destx ; 
	    if ( mem [ s + 3 ] .cint > mem [ s + 1 ] .cint ) 
	    {
	      mem [ s + 3 ] .cint = mem [ s + 1 ] .cint ; 
	      if ( mem [ r + 5 ] .cint > mem [ s + 1 ] .cint ) 
	      mem [ r + 5 ] .cint = mem [ s + 1 ] .cint ; 
	    } 
	    if ( mem [ s + 5 ] .cint < mem [ s + 1 ] .cint ) 
	    {
	      mem [ s + 5 ] .cint = mem [ s + 1 ] .cint ; 
	      if ( mem [ qq + 3 ] .cint < mem [ s + 1 ] .cint ) 
	      mem [ qq + 3 ] .cint = mem [ s + 1 ] .cint ; 
	    } 
	  } 
	  else {
	      
	    if ( mem [ r + 2 ] .cint > desty ) 
	    {
	      mem [ r + 2 ] .cint = desty ; 
	      mem [ r + 4 ] .cint = - (integer) mem [ r + 2 ] .cint ; 
	      mem [ r + 6 ] .cint = mem [ r + 2 ] .cint ; 
	    } 
	    if ( mem [ qq + 4 ] .cint > desty ) 
	    mem [ qq + 4 ] .cint = desty ; 
	    else if ( mem [ qq + 4 ] .cint < mem [ r + 2 ] .cint ) 
	    mem [ qq + 4 ] .cint = mem [ r + 2 ] .cint ; 
	  } 
	} 
      } 
      else if ( constantx ) 
      {
	if ( q != p ) 
	{
	  removecubic ( p ) ; 
	  if ( curspec != q ) 
	  goto lab22 ; 
	  else {
	      
	    curspec = p ; 
	    goto lab10 ; 
	  } 
	} 
      } 
      else if ( ! odd ( mem [ pp ] .hhfield .b1 ) ) 
      {
	mem [ pp + 2 ] .cint = - (integer) mem [ pp + 2 ] .cint ; 
	mem [ pp + 6 ] .cint = - (integer) mem [ pp + 6 ] .cint ; 
	mem [ qq + 4 ] .cint = - (integer) mem [ qq + 4 ] .cint ; 
	del1 = - (integer) del1 ; 
	del2 = - (integer) del2 ; 
	del3 = - (integer) del3 ; 
	desty = - (integer) desty ; 
	mem [ pp ] .hhfield .b1 = mem [ pp ] .hhfield .b1 + 2 ; 
      } 
      pp = qq ; 
    } while ( ! ( pp == q ) ) ; 
    if ( constantx ) 
    {
      pp = p ; 
      do {
	  qq = mem [ pp ] .hhfield .v.RH ; 
	if ( mem [ pp ] .hhfield .b1 > 2 ) 
	{
	  mem [ pp ] .hhfield .b1 = mem [ pp ] .hhfield .b1 + 1 ; 
	  mem [ pp + 1 ] .cint = - (integer) mem [ pp + 1 ] .cint ; 
	  mem [ pp + 5 ] .cint = - (integer) mem [ pp + 5 ] .cint ; 
	  mem [ qq + 3 ] .cint = - (integer) mem [ qq + 3 ] .cint ; 
	} 
	pp = qq ; 
      } while ( ! ( pp == q ) ) ; 
    } 
    p = q ; 
  } while ( ! ( p == curspec ) ) ; 
  lab10: ; 
} 
void octantsubdivide ( ) 
{halfword p, q, r, s  ; 
  scaled del1, del2, del3, del, dmax  ; 
  fraction t  ; 
  scaled destx, desty  ; 
  p = curspec ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    mem [ p + 1 ] .cint = mem [ p + 1 ] .cint - mem [ p + 2 ] .cint ; 
    mem [ p + 5 ] .cint = mem [ p + 5 ] .cint - mem [ p + 6 ] .cint ; 
    mem [ q + 3 ] .cint = mem [ q + 3 ] .cint - mem [ q + 4 ] .cint ; 
    if ( q == curspec ) 
    {
      unskew ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint , mem [ q ] .hhfield 
      .b1 ) ; 
      skew ( curx , cury , mem [ p ] .hhfield .b1 ) ; 
      destx = curx ; 
      desty = cury ; 
    } 
    else {
	
      abnegate ( mem [ q + 1 ] .cint , mem [ q + 2 ] .cint , mem [ q ] 
      .hhfield .b1 , mem [ p ] .hhfield .b1 ) ; 
      destx = curx - cury ; 
      desty = cury ; 
    } 
    del1 = mem [ p + 5 ] .cint - mem [ p + 1 ] .cint ; 
    del2 = mem [ q + 3 ] .cint - mem [ p + 5 ] .cint ; 
    del3 = destx - mem [ q + 3 ] .cint ; 
    if ( del1 != 0 ) 
    del = del1 ; 
    else if ( del2 != 0 ) 
    del = del2 ; 
    else del = del3 ; 
    if ( del != 0 ) 
    {
      dmax = abs ( del1 ) ; 
      if ( abs ( del2 ) > dmax ) 
      dmax = abs ( del2 ) ; 
      if ( abs ( del3 ) > dmax ) 
      dmax = abs ( del3 ) ; 
      while ( dmax < 134217728L ) {
	  
	dmax = dmax + dmax ; 
	del1 = del1 + del1 ; 
	del2 = del2 + del2 ; 
	del3 = del3 + del3 ; 
      } 
    } 
    if ( del != 0 ) 
    {
      if ( del < 0 ) 
      {
	mem [ p + 2 ] .cint = mem [ p + 1 ] .cint + mem [ p + 2 ] .cint ; 
	mem [ p + 1 ] .cint = - (integer) mem [ p + 1 ] .cint ; 
	mem [ p + 6 ] .cint = mem [ p + 5 ] .cint + mem [ p + 6 ] .cint ; 
	mem [ p + 5 ] .cint = - (integer) mem [ p + 5 ] .cint ; 
	mem [ q + 4 ] .cint = mem [ q + 3 ] .cint + mem [ q + 4 ] .cint ; 
	mem [ q + 3 ] .cint = - (integer) mem [ q + 3 ] .cint ; 
	del1 = - (integer) del1 ; 
	del2 = - (integer) del2 ; 
	del3 = - (integer) del3 ; 
	desty = destx + desty ; 
	destx = - (integer) destx ; 
	mem [ p ] .hhfield .b1 = mem [ p ] .hhfield .b1 + 4 ; 
      } 
      t = crossingpoint ( del1 , del2 , del3 ) ; 
      if ( t < 268435456L ) 
      {
	splitcubic ( p , t , destx , desty ) ; 
	r = mem [ p ] .hhfield .v.RH ; 
	if ( mem [ r ] .hhfield .b1 > 4 ) 
	mem [ r ] .hhfield .b1 = mem [ r ] .hhfield .b1 - 4 ; 
	else mem [ r ] .hhfield .b1 = mem [ r ] .hhfield .b1 + 4 ; 
	if ( mem [ r + 2 ] .cint < mem [ p + 2 ] .cint ) 
	mem [ r + 2 ] .cint = mem [ p + 2 ] .cint ; 
	else if ( mem [ r + 2 ] .cint > desty ) 
	mem [ r + 2 ] .cint = desty ; 
	if ( mem [ p + 1 ] .cint + mem [ r + 2 ] .cint > destx + desty ) 
	mem [ r + 2 ] .cint = destx + desty - mem [ p + 1 ] .cint ; 
	if ( mem [ r + 4 ] .cint > mem [ r + 2 ] .cint ) 
	{
	  mem [ r + 4 ] .cint = mem [ r + 2 ] .cint ; 
	  if ( mem [ p + 6 ] .cint > mem [ r + 2 ] .cint ) 
	  mem [ p + 6 ] .cint = mem [ r + 2 ] .cint ; 
	} 
	if ( mem [ r + 6 ] .cint < mem [ r + 2 ] .cint ) 
	{
	  mem [ r + 6 ] .cint = mem [ r + 2 ] .cint ; 
	  if ( mem [ q + 4 ] .cint < mem [ r + 2 ] .cint ) 
	  mem [ q + 4 ] .cint = mem [ r + 2 ] .cint ; 
	} 
	if ( mem [ r + 1 ] .cint < mem [ p + 1 ] .cint ) 
	mem [ r + 1 ] .cint = mem [ p + 1 ] .cint ; 
	else if ( mem [ r + 1 ] .cint + mem [ r + 2 ] .cint > destx + desty ) 
	mem [ r + 1 ] .cint = destx + desty - mem [ r + 2 ] .cint ; 
	mem [ r + 3 ] .cint = mem [ r + 1 ] .cint ; 
	if ( mem [ p + 5 ] .cint > mem [ r + 1 ] .cint ) 
	mem [ p + 5 ] .cint = mem [ r + 1 ] .cint ; 
	mem [ r + 2 ] .cint = mem [ r + 2 ] .cint + mem [ r + 1 ] .cint ; 
	mem [ r + 6 ] .cint = mem [ r + 6 ] .cint + mem [ r + 1 ] .cint ; 
	mem [ r + 1 ] .cint = - (integer) mem [ r + 1 ] .cint ; 
	mem [ r + 5 ] .cint = mem [ r + 1 ] .cint ; 
	mem [ q + 4 ] .cint = mem [ q + 4 ] .cint + mem [ q + 3 ] .cint ; 
	mem [ q + 3 ] .cint = - (integer) mem [ q + 3 ] .cint ; 
	desty = desty + destx ; 
	destx = - (integer) destx ; 
	if ( mem [ r + 6 ] .cint < mem [ r + 2 ] .cint ) 
	{
	  mem [ r + 6 ] .cint = mem [ r + 2 ] .cint ; 
	  if ( mem [ q + 4 ] .cint < mem [ r + 2 ] .cint ) 
	  mem [ q + 4 ] .cint = mem [ r + 2 ] .cint ; 
	} 
	del2 = del2 - takefraction ( del2 - del3 , t ) ; 
	if ( del2 > 0 ) 
	del2 = 0 ; 
	t = crossingpoint ( 0 , - (integer) del2 , - (integer) del3 ) ; 
	if ( t < 268435456L ) 
	{
	  splitcubic ( r , t , destx , desty ) ; 
	  s = mem [ r ] .hhfield .v.RH ; 
	  if ( mem [ s + 2 ] .cint < mem [ r + 2 ] .cint ) 
	  mem [ s + 2 ] .cint = mem [ r + 2 ] .cint ; 
	  else if ( mem [ s + 2 ] .cint > desty ) 
	  mem [ s + 2 ] .cint = desty ; 
	  if ( mem [ r + 1 ] .cint + mem [ s + 2 ] .cint > destx + desty ) 
	  mem [ s + 2 ] .cint = destx + desty - mem [ r + 1 ] .cint ; 
	  if ( mem [ s + 4 ] .cint > mem [ s + 2 ] .cint ) 
	  {
	    mem [ s + 4 ] .cint = mem [ s + 2 ] .cint ; 
	    if ( mem [ r + 6 ] .cint > mem [ s + 2 ] .cint ) 
	    mem [ r + 6 ] .cint = mem [ s + 2 ] .cint ; 
	  } 
	  if ( mem [ s + 6 ] .cint < mem [ s + 2 ] .cint ) 
	  {
	    mem [ s + 6 ] .cint = mem [ s + 2 ] .cint ; 
	    if ( mem [ q + 4 ] .cint < mem [ s + 2 ] .cint ) 
	    mem [ q + 4 ] .cint = mem [ s + 2 ] .cint ; 
	  } 
	  if ( mem [ s + 1 ] .cint + mem [ s + 2 ] .cint > destx + desty ) 
	  mem [ s + 1 ] .cint = destx + desty - mem [ s + 2 ] .cint ; 
	  else {
	      
	    if ( mem [ s + 1 ] .cint < destx ) 
	    mem [ s + 1 ] .cint = destx ; 
	    if ( mem [ s + 1 ] .cint < mem [ r + 1 ] .cint ) 
	    mem [ s + 1 ] .cint = mem [ r + 1 ] .cint ; 
	  } 
	  mem [ s ] .hhfield .b1 = mem [ p ] .hhfield .b1 ; 
	  mem [ s + 3 ] .cint = mem [ s + 1 ] .cint ; 
	  if ( mem [ q + 3 ] .cint < destx ) 
	  {
	    mem [ q + 4 ] .cint = mem [ q + 4 ] .cint + destx ; 
	    mem [ q + 3 ] .cint = - (integer) destx ; 
	  } 
	  else if ( mem [ q + 3 ] .cint > mem [ s + 1 ] .cint ) 
	  {
	    mem [ q + 4 ] .cint = mem [ q + 4 ] .cint + mem [ s + 1 ] .cint ; 
	    mem [ q + 3 ] .cint = - (integer) mem [ s + 1 ] .cint ; 
	  } 
	  else {
	      
	    mem [ q + 4 ] .cint = mem [ q + 4 ] .cint + mem [ q + 3 ] .cint ; 
	    mem [ q + 3 ] .cint = - (integer) mem [ q + 3 ] .cint ; 
	  } 
	  mem [ s + 2 ] .cint = mem [ s + 2 ] .cint + mem [ s + 1 ] .cint ; 
	  mem [ s + 6 ] .cint = mem [ s + 6 ] .cint + mem [ s + 1 ] .cint ; 
	  mem [ s + 1 ] .cint = - (integer) mem [ s + 1 ] .cint ; 
	  mem [ s + 5 ] .cint = mem [ s + 1 ] .cint ; 
	  if ( mem [ s + 6 ] .cint < mem [ s + 2 ] .cint ) 
	  {
	    mem [ s + 6 ] .cint = mem [ s + 2 ] .cint ; 
	    if ( mem [ q + 4 ] .cint < mem [ s + 2 ] .cint ) 
	    mem [ q + 4 ] .cint = mem [ s + 2 ] .cint ; 
	  } 
	} 
	else {
	    
	  if ( mem [ r + 1 ] .cint > destx ) 
	  {
	    mem [ r + 1 ] .cint = destx ; 
	    mem [ r + 3 ] .cint = - (integer) mem [ r + 1 ] .cint ; 
	    mem [ r + 5 ] .cint = mem [ r + 1 ] .cint ; 
	  } 
	  if ( mem [ q + 3 ] .cint > destx ) 
	  mem [ q + 3 ] .cint = destx ; 
	  else if ( mem [ q + 3 ] .cint < mem [ r + 1 ] .cint ) 
	  mem [ q + 3 ] .cint = mem [ r + 1 ] .cint ; 
	} 
      } 
    } 
    p = q ; 
  } while ( ! ( p == curspec ) ) ; 
} 
void makesafe ( ) 
{integer k  ; 
  boolean allsafe  ; 
  scaled nexta  ; 
  scaled deltaa, deltab  ; 
  before [ curroundingptr ] = before [ 0 ] ; 
  nodetoround [ curroundingptr ] = nodetoround [ 0 ] ; 
  do {
      after [ curroundingptr ] = after [ 0 ] ; 
    allsafe = true ; 
    nexta = after [ 0 ] ; 
    {register integer for_end; k = 0 ; for_end = curroundingptr - 1 ; if ( k 
    <= for_end) do 
      {
	deltab = before [ k + 1 ] - before [ k ] ; 
	if ( deltab >= 0 ) 
	deltaa = after [ k + 1 ] - nexta ; 
	else deltaa = nexta - after [ k + 1 ] ; 
	nexta = after [ k + 1 ] ; 
	if ( ( deltaa < 0 ) || ( deltaa > abs ( deltab + deltab ) ) ) 
	{
	  allsafe = false ; 
	  after [ k ] = before [ k ] ; 
	  if ( k == curroundingptr - 1 ) 
	  after [ 0 ] = before [ 0 ] ; 
	  else after [ k + 1 ] = before [ k + 1 ] ; 
	} 
      } 
    while ( k++ < for_end ) ; } 
  } while ( ! ( allsafe ) ) ; 
} 
void zbeforeandafter ( b , a , p ) 
scaled b ; 
scaled a ; 
halfword p ; 
{if ( curroundingptr == maxroundingptr ) 
  if ( maxroundingptr < maxwiggle ) 
  incr ( maxroundingptr ) ; 
  else overflow ( 567 , maxwiggle ) ; 
  after [ curroundingptr ] = a ; 
  before [ curroundingptr ] = b ; 
  nodetoround [ curroundingptr ] = p ; 
  incr ( curroundingptr ) ; 
} 
scaled zgoodval ( b , o ) 
scaled b ; 
scaled o ; 
{register scaled Result; scaled a  ; 
  a = b + o ; 
  if ( a >= 0 ) 
  a = a - ( a % curgran ) - o ; 
  else a = a + ( ( - (integer) ( a + 1 ) ) % curgran ) - curgran + 1 - o ; 
  if ( b - a < a + curgran - b ) 
  Result = a ; 
  else Result = a + curgran ; 
  return(Result) ; 
} 
scaled zcompromise ( u , v ) 
scaled u ; 
scaled v ; 
{register scaled Result; Result = ( goodval ( u + u , - (integer) u - v ) ) / 
  2 ; 
  return(Result) ; 
} 
void xyround ( ) 
{halfword p, q  ; 
  scaled b, a  ; 
  scaled penedge  ; 
  fraction alpha  ; 
  curgran = abs ( internal [ 37 ] ) ; 
  if ( curgran == 0 ) 
  curgran = 65536L ; 
  p = curspec ; 
  curroundingptr = 0 ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    if ( odd ( mem [ p ] .hhfield .b1 ) != odd ( mem [ q ] .hhfield .b1 ) ) 
    {
      if ( odd ( mem [ q ] .hhfield .b1 ) ) 
      b = mem [ q + 1 ] .cint ; 
      else b = - (integer) mem [ q + 1 ] .cint ; 
      if ( ( abs ( mem [ q + 1 ] .cint - mem [ q + 5 ] .cint ) < 655 ) || ( 
      abs ( mem [ q + 1 ] .cint + mem [ q + 3 ] .cint ) < 655 ) ) 
      {
	if ( curpen == 3 ) 
	penedge = 0 ; 
	else if ( curpathtype == 0 ) 
	penedge = compromise ( mem [ mem [ curpen + 5 ] .hhfield .v.RH + 2 ] 
	.cint , mem [ mem [ curpen + 7 ] .hhfield .v.RH + 2 ] .cint ) ; 
	else if ( odd ( mem [ q ] .hhfield .b1 ) ) 
	penedge = mem [ mem [ curpen + 7 ] .hhfield .v.RH + 2 ] .cint ; 
	else penedge = mem [ mem [ curpen + 5 ] .hhfield .v.RH + 2 ] .cint ; 
	a = goodval ( b , penedge ) ; 
      } 
      else a = b ; 
      if ( abs ( a ) > maxallowed ) 
      if ( a > 0 ) 
      a = maxallowed ; 
      else a = - (integer) maxallowed ; 
      beforeandafter ( b , a , q ) ; 
    } 
    p = q ; 
  } while ( ! ( p == curspec ) ) ; 
  if ( curroundingptr > 0 ) 
  {
    makesafe () ; 
    do {
	decr ( curroundingptr ) ; 
      if ( ( after [ curroundingptr ] != before [ curroundingptr ] ) || ( 
      after [ curroundingptr + 1 ] != before [ curroundingptr + 1 ] ) ) 
      {
	p = nodetoround [ curroundingptr ] ; 
	if ( odd ( mem [ p ] .hhfield .b1 ) ) 
	{
	  b = before [ curroundingptr ] ; 
	  a = after [ curroundingptr ] ; 
	} 
	else {
	    
	  b = - (integer) before [ curroundingptr ] ; 
	  a = - (integer) after [ curroundingptr ] ; 
	} 
	if ( before [ curroundingptr ] == before [ curroundingptr + 1 ] ) 
	alpha = 268435456L ; 
	else alpha = makefraction ( after [ curroundingptr + 1 ] - after [ 
	curroundingptr ] , before [ curroundingptr + 1 ] - before [ 
	curroundingptr ] ) ; 
	do {
	    mem [ p + 1 ] .cint = takefraction ( alpha , mem [ p + 1 ] .cint 
	  - b ) + a ; 
	  mem [ p + 5 ] .cint = takefraction ( alpha , mem [ p + 5 ] .cint - b 
	  ) + a ; 
	  p = mem [ p ] .hhfield .v.RH ; 
	  mem [ p + 3 ] .cint = takefraction ( alpha , mem [ p + 3 ] .cint - b 
	  ) + a ; 
	} while ( ! ( p == nodetoround [ curroundingptr + 1 ] ) ) ; 
      } 
    } while ( ! ( curroundingptr == 0 ) ) ; 
  } 
  p = curspec ; 
  curroundingptr = 0 ; 
  do {
      q = mem [ p ] .hhfield .v.RH ; 
    if ( ( mem [ p ] .hhfield .b1 > 2 ) != ( mem [ q ] .hhfield .b1 > 2 ) ) 
    {
      if ( mem [ q ] .hhfield .b1 <= 2 ) 
      b = mem [ q + 2 ] .cint ; 
      else b = - (integer) mem [ q + 2 ] .cint ; 
      if ( ( abs ( mem [ q + 2 ] .cint - mem [ q + 6 ] .cint ) < 655 ) || ( 
      abs ( mem [ q + 2 ] .cint + mem [ q + 4 ] .cint ) < 655 ) ) 
      {
	if ( curpen == 3 ) 
	penedge = 0 ; 
	else if ( curpathtype == 0 ) 
	penedge = compromise ( mem [ mem [ curpen + 2 ] .hhfield .v.RH + 2 ] 
	.cint , mem [ mem [ curpen + 1 ] .hhfield .v.RH + 2 ] .cint ) ; 
	else if ( mem [ q ] .hhfield .b1 <= 2 ) 
	penedge = mem [ mem [ curpen + 1 ] .hhfield .v.RH + 2 ] .cint ; 
	else penedge = mem [ mem [ curpen + 2 ] .hhfield .v.RH + 2 ] .cint ; 
	a = goodval ( b , penedge ) ; 
      } 
      else a = b ; 
      if ( abs ( a ) > maxallowed ) 
      if ( a > 0 ) 
      a = maxallowed ; 
      else a = - (integer) maxallowed ; 
      beforeandafter ( b , a , q ) ; 
    } 
    p = q ; 
  } while ( ! ( p == curspec ) ) ; 
  if ( curroundingptr > 0 ) 
  {
    makesafe () ; 
    do {
	decr ( curroundingptr ) ; 
      if ( ( after [ curroundingptr ] != before [ curroundingptr ] ) || ( 
      after [ curroundingptr + 1 ] != before [ curroundingptr + 1 ] ) ) 
      {
	p = nodetoround [ curroundingptr ] ; 
	if ( mem [ p ] .hhfield .b1 <= 2 ) 
	{
	  b = before [ curroundingptr ] ; 
	  a = after [ curroundingptr ] ; 
	} 
	else {
	    
	  b = - (integer) before [ curroundingptr ] ; 
	  a = - (integer) after [ curroundingptr ] ; 
	} 
	if ( before [ curroundingptr ] == before [ curroundingptr + 1 ] ) 
	alpha = 268435456L ; 
	else alpha = makefraction ( after [ curroundingptr + 1 ] - after [ 
	curroundingptr ] , before [ curroundingptr + 1 ] - before [ 
	curroundingptr ] ) ; 
	do {
	    mem [ p + 2 ] .cint = takefraction ( alpha , mem [ p + 2 ] .cint 
	  - b ) + a ; 
	  mem [ p + 6 ] .cint = takefraction ( alpha , mem [ p + 6 ] .cint - b 
	  ) + a ; 
	  p = mem [ p ] .hhfield .v.RH ; 
	  mem [ p + 4 ] .cint = takefraction ( alpha , mem [ p + 4 ] .cint - b 
	  ) + a ; 
	} while ( ! ( p == nodetoround [ curroundingptr + 1 ] ) ) ; 
      } 
    } while ( ! ( curroundingptr == 0 ) ) ; 
  } 
} 
