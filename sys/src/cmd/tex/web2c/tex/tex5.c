#define EXTERN extern
#include "texd.h"

halfword zmathglue ( g , m ) 
halfword g ; 
scaled m ; 
{register halfword Result; mathglue_regmem 
  halfword p  ; 
  integer n  ; 
  scaled f  ; 
  n = xovern ( m , 65536L ) ; 
  f = texremainder ; 
  if ( f < 0 ) 
  {
    decr ( n ) ; 
    f = f + 65536L ; 
  } 
  p = getnode ( 4 ) ; 
  mem [ p + 1 ] .cint = multandadd ( n , mem [ g + 1 ] .cint , xnoverd ( mem [ 
  g + 1 ] .cint , f , 65536L ) , 1073741823L ) ; 
  mem [ p ] .hh.b0 = mem [ g ] .hh.b0 ; 
  if ( mem [ p ] .hh.b0 == 0 ) 
  mem [ p + 2 ] .cint = multandadd ( n , mem [ g + 2 ] .cint , xnoverd ( mem [ 
  g + 2 ] .cint , f , 65536L ) , 1073741823L ) ; 
  else mem [ p + 2 ] .cint = mem [ g + 2 ] .cint ; 
  mem [ p ] .hh.b1 = mem [ g ] .hh.b1 ; 
  if ( mem [ p ] .hh.b1 == 0 ) 
  mem [ p + 3 ] .cint = multandadd ( n , mem [ g + 3 ] .cint , xnoverd ( mem [ 
  g + 3 ] .cint , f , 65536L ) , 1073741823L ) ; 
  else mem [ p + 3 ] .cint = mem [ g + 3 ] .cint ; 
  Result = p ; 
  return(Result) ; 
} 
void zmathkern ( p , m ) 
halfword p ; 
scaled m ; 
{mathkern_regmem 
  integer n  ; 
  scaled f  ; 
  if ( mem [ p ] .hh.b1 == 99 ) 
  {
    n = xovern ( m , 65536L ) ; 
    f = texremainder ; 
    if ( f < 0 ) 
    {
      decr ( n ) ; 
      f = f + 65536L ; 
    } 
    mem [ p + 1 ] .cint = multandadd ( n , mem [ p + 1 ] .cint , xnoverd ( mem 
    [ p + 1 ] .cint , f , 65536L ) , 1073741823L ) ; 
    mem [ p ] .hh.b1 = 0 ; 
  } 
} 
void flushmath ( ) 
{flushmath_regmem 
  flushnodelist ( mem [ curlist .headfield ] .hh .v.RH ) ; 
  flushnodelist ( curlist .auxfield .cint ) ; 
  mem [ curlist .headfield ] .hh .v.RH = 0 ; 
  curlist .tailfield = curlist .headfield ; 
  curlist .auxfield .cint = 0 ; 
} 
halfword zcleanbox ( p , s ) 
halfword p ; 
smallnumber s ; 
{/* 40 */ register halfword Result; cleanbox_regmem 
  halfword q  ; 
  smallnumber savestyle  ; 
  halfword x  ; 
  halfword r  ; 
  switch ( mem [ p ] .hh .v.RH ) 
  {case 1 : 
    {
      curmlist = newnoad () ; 
      mem [ curmlist + 1 ] = mem [ p ] ; 
    } 
    break ; 
  case 2 : 
    {
      q = mem [ p ] .hh .v.LH ; 
      goto lab40 ; 
    } 
    break ; 
  case 3 : 
    curmlist = mem [ p ] .hh .v.LH ; 
    break ; 
    default: 
    {
      q = newnullbox () ; 
      goto lab40 ; 
    } 
    break ; 
  } 
  savestyle = curstyle ; 
  curstyle = s ; 
  mlistpenalties = false ; 
  mlisttohlist () ; 
  q = mem [ memtop - 3 ] .hh .v.RH ; 
  curstyle = savestyle ; 
  {
    if ( curstyle < 4 ) 
    cursize = 0 ; 
    else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ; 
    curmu = xovern ( fontinfo [ 6 + parambase [ eqtb [ 11337 + cursize ] .hh 
    .v.RH ] ] .cint , 18 ) ; 
  } 
  lab40: if ( ( q >= himemmin ) || ( q == 0 ) ) 
  x = hpack ( q , 0 , 1 ) ; 
  else if ( ( mem [ q ] .hh .v.RH == 0 ) && ( mem [ q ] .hh.b0 <= 1 ) && ( mem 
  [ q + 4 ] .cint == 0 ) ) 
  x = q ; 
  else x = hpack ( q , 0 , 1 ) ; 
  q = mem [ x + 5 ] .hh .v.RH ; 
  if ( ( q >= himemmin ) ) 
  {
    r = mem [ q ] .hh .v.RH ; 
    if ( r != 0 ) 
    if ( mem [ r ] .hh .v.RH == 0 ) 
    if ( ! ( r >= himemmin ) ) 
    if ( mem [ r ] .hh.b0 == 11 ) 
    {
      freenode ( r , 2 ) ; 
      mem [ q ] .hh .v.RH = 0 ; 
    } 
  } 
  Result = x ; 
  return(Result) ; 
} 
void zfetch ( a ) 
halfword a ; 
{fetch_regmem 
  curc = mem [ a ] .hh.b1 ; 
  curf = eqtb [ 11335 + mem [ a ] .hh.b0 + cursize ] .hh .v.RH ; 
  if ( curf == 0 ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 335 ) ; 
    } 
    printsize ( cursize ) ; 
    printchar ( 32 ) ; 
    printint ( mem [ a ] .hh.b0 ) ; 
    print ( 877 ) ; 
    print ( curc ) ; 
    printchar ( 41 ) ; 
    {
      helpptr = 4 ; 
      helpline [ 3 ] = 878 ; 
      helpline [ 2 ] = 879 ; 
      helpline [ 1 ] = 880 ; 
      helpline [ 0 ] = 881 ; 
    } 
    error () ; 
    curi = nullcharacter ; 
    mem [ a ] .hh .v.RH = 0 ; 
  } 
  else {
      
    if ( ( curc >= fontbc [ curf ] ) && ( curc <= fontec [ curf ] ) ) 
    curi = fontinfo [ charbase [ curf ] + curc ] .qqqq ; 
    else curi = nullcharacter ; 
    if ( ! ( ( curi .b0 > 0 ) ) ) 
    {
      charwarning ( curf , curc ) ; 
      mem [ a ] .hh .v.RH = 0 ; 
    } 
  } 
} 
void zmakeover ( q ) 
halfword q ; 
{makeover_regmem 
  mem [ q + 1 ] .hh .v.LH = overbar ( cleanbox ( q + 1 , 2 * ( curstyle / 2 ) 
  + 1 ) , 3 * fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH ] 
  ] .cint , fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH ] ] 
  .cint ) ; 
  mem [ q + 1 ] .hh .v.RH = 2 ; 
} 
void zmakeunder ( q ) 
halfword q ; 
{makeunder_regmem 
  halfword p, x, y  ; 
  scaled delta  ; 
  x = cleanbox ( q + 1 , curstyle ) ; 
  p = newkern ( 3 * fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh 
  .v.RH ] ] .cint ) ; 
  mem [ x ] .hh .v.RH = p ; 
  mem [ p ] .hh .v.RH = fractionrule ( fontinfo [ 8 + parambase [ eqtb [ 11338 
  + cursize ] .hh .v.RH ] ] .cint ) ; 
  y = vpackage ( x , 0 , 1 , 1073741823L ) ; 
  delta = mem [ y + 3 ] .cint + mem [ y + 2 ] .cint + fontinfo [ 8 + parambase 
  [ eqtb [ 11338 + cursize ] .hh .v.RH ] ] .cint ; 
  mem [ y + 3 ] .cint = mem [ x + 3 ] .cint ; 
  mem [ y + 2 ] .cint = delta - mem [ y + 3 ] .cint ; 
  mem [ q + 1 ] .hh .v.LH = y ; 
  mem [ q + 1 ] .hh .v.RH = 2 ; 
} 
void zmakevcenter ( q ) 
halfword q ; 
{makevcenter_regmem 
  halfword v  ; 
  scaled delta  ; 
  v = mem [ q + 1 ] .hh .v.LH ; 
  if ( mem [ v ] .hh.b0 != 1 ) 
  confusion ( 535 ) ; 
  delta = mem [ v + 3 ] .cint + mem [ v + 2 ] .cint ; 
  mem [ v + 3 ] .cint = fontinfo [ 22 + parambase [ eqtb [ 11337 + cursize ] 
  .hh .v.RH ] ] .cint + half ( delta ) ; 
  mem [ v + 2 ] .cint = delta - mem [ v + 3 ] .cint ; 
} 
void zmakeradical ( q ) 
halfword q ; 
{makeradical_regmem 
  halfword x, y  ; 
  scaled delta, clr  ; 
  x = cleanbox ( q + 1 , 2 * ( curstyle / 2 ) + 1 ) ; 
  if ( curstyle < 2 ) 
  clr = fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH ] ] 
  .cint + ( abs ( fontinfo [ 5 + parambase [ eqtb [ 11337 + cursize ] .hh 
  .v.RH ] ] .cint ) / 4 ) ; 
  else {
      
    clr = fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH ] ] 
    .cint ; 
    clr = clr + ( abs ( clr ) / 4 ) ; 
  } 
  y = vardelimiter ( q + 4 , cursize , mem [ x + 3 ] .cint + mem [ x + 2 ] 
  .cint + clr + fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH 
  ] ] .cint ) ; 
  delta = mem [ y + 2 ] .cint - ( mem [ x + 3 ] .cint + mem [ x + 2 ] .cint + 
  clr ) ; 
  if ( delta > 0 ) 
  clr = clr + half ( delta ) ; 
  mem [ y + 4 ] .cint = - (integer) ( mem [ x + 3 ] .cint + clr ) ; 
  mem [ y ] .hh .v.RH = overbar ( x , clr , mem [ y + 3 ] .cint ) ; 
  mem [ q + 1 ] .hh .v.LH = hpack ( y , 0 , 1 ) ; 
  mem [ q + 1 ] .hh .v.RH = 2 ; 
} 
void zmakemathaccent ( q ) 
halfword q ; 
{/* 30 31 */ makemathaccent_regmem 
  halfword p, x, y  ; 
  integer a  ; 
  quarterword c  ; 
  internalfontnumber f  ; 
  fourquarters i  ; 
  scaled s  ; 
  scaled h  ; 
  scaled delta  ; 
  scaled w  ; 
  fetch ( q + 4 ) ; 
  if ( ( curi .b0 > 0 ) ) 
  {
    i = curi ; 
    c = curc ; 
    f = curf ; 
    s = 0 ; 
    if ( mem [ q + 1 ] .hh .v.RH == 1 ) 
    {
      fetch ( q + 1 ) ; 
      if ( ( ( curi .b2 ) % 4 ) == 1 ) 
      {
	a = ligkernbase [ curf ] + curi .b3 ; 
	curi = fontinfo [ a ] .qqqq ; 
	if ( curi .b0 > 128 ) 
	{
	  a = ligkernbase [ curf ] + 256 * curi .b2 + curi .b3 + 32768L - 256 
	  * ( 128 ) ; 
	  curi = fontinfo [ a ] .qqqq ; 
	} 
	while ( true ) {
	    
	  if ( curi .b1 == skewchar [ curf ] ) 
	  {
	    if ( curi .b2 >= 128 ) 
	    if ( curi .b0 <= 128 ) 
	    s = fontinfo [ kernbase [ curf ] + 256 * curi .b2 + curi .b3 ] 
	    .cint ; 
	    goto lab31 ; 
	  } 
	  if ( curi .b0 >= 128 ) 
	  goto lab31 ; 
	  a = a + curi .b0 + 1 ; 
	  curi = fontinfo [ a ] .qqqq ; 
	} 
      } 
    } 
    lab31: ; 
    x = cleanbox ( q + 1 , 2 * ( curstyle / 2 ) + 1 ) ; 
    w = mem [ x + 1 ] .cint ; 
    h = mem [ x + 3 ] .cint ; 
    while ( true ) {
	
      if ( ( ( i .b2 ) % 4 ) != 2 ) 
      goto lab30 ; 
      y = i .b3 ; 
      i = fontinfo [ charbase [ f ] + y ] .qqqq ; 
      if ( ! ( i .b0 > 0 ) ) 
      goto lab30 ; 
      if ( fontinfo [ widthbase [ f ] + i .b0 ] .cint > w ) 
      goto lab30 ; 
      c = y ; 
    } 
    lab30: ; 
    if ( h < fontinfo [ 5 + parambase [ f ] ] .cint ) 
    delta = h ; 
    else delta = fontinfo [ 5 + parambase [ f ] ] .cint ; 
    if ( ( mem [ q + 2 ] .hh .v.RH != 0 ) || ( mem [ q + 3 ] .hh .v.RH != 0 ) 
    ) 
    if ( mem [ q + 1 ] .hh .v.RH == 1 ) 
    {
      flushnodelist ( x ) ; 
      x = newnoad () ; 
      mem [ x + 1 ] = mem [ q + 1 ] ; 
      mem [ x + 2 ] = mem [ q + 2 ] ; 
      mem [ x + 3 ] = mem [ q + 3 ] ; 
      mem [ q + 2 ] .hh = emptyfield ; 
      mem [ q + 3 ] .hh = emptyfield ; 
      mem [ q + 1 ] .hh .v.RH = 3 ; 
      mem [ q + 1 ] .hh .v.LH = x ; 
      x = cleanbox ( q + 1 , curstyle ) ; 
      delta = delta + mem [ x + 3 ] .cint - h ; 
      h = mem [ x + 3 ] .cint ; 
    } 
    y = charbox ( f , c ) ; 
    mem [ y + 4 ] .cint = s + half ( w - mem [ y + 1 ] .cint ) ; 
    mem [ y + 1 ] .cint = 0 ; 
    p = newkern ( - (integer) delta ) ; 
    mem [ p ] .hh .v.RH = x ; 
    mem [ y ] .hh .v.RH = p ; 
    y = vpackage ( y , 0 , 1 , 1073741823L ) ; 
    mem [ y + 1 ] .cint = mem [ x + 1 ] .cint ; 
    if ( mem [ y + 3 ] .cint < h ) 
    {
      p = newkern ( h - mem [ y + 3 ] .cint ) ; 
      mem [ p ] .hh .v.RH = mem [ y + 5 ] .hh .v.RH ; 
      mem [ y + 5 ] .hh .v.RH = p ; 
      mem [ y + 3 ] .cint = h ; 
    } 
    mem [ q + 1 ] .hh .v.LH = y ; 
    mem [ q + 1 ] .hh .v.RH = 2 ; 
  } 
} 
void zmakefraction ( q ) 
halfword q ; 
{makefraction_regmem 
  halfword p, v, x, y, z  ; 
  scaled delta, delta1, delta2, shiftup, shiftdown, clr  ; 
  if ( mem [ q + 1 ] .cint == 1073741824L ) 
  mem [ q + 1 ] .cint = fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] 
  .hh .v.RH ] ] .cint ; 
  x = cleanbox ( q + 2 , curstyle + 2 - 2 * ( curstyle / 6 ) ) ; 
  z = cleanbox ( q + 3 , 2 * ( curstyle / 2 ) + 3 - 2 * ( curstyle / 6 ) ) ; 
  if ( mem [ x + 1 ] .cint < mem [ z + 1 ] .cint ) 
  x = rebox ( x , mem [ z + 1 ] .cint ) ; 
  else z = rebox ( z , mem [ x + 1 ] .cint ) ; 
  if ( curstyle < 2 ) 
  {
    shiftup = fontinfo [ 8 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] 
    ] .cint ; 
    shiftdown = fontinfo [ 11 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH 
    ] ] .cint ; 
  } 
  else {
      
    shiftdown = fontinfo [ 12 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH 
    ] ] .cint ; 
    if ( mem [ q + 1 ] .cint != 0 ) 
    shiftup = fontinfo [ 9 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] 
    ] .cint ; 
    else shiftup = fontinfo [ 10 + parambase [ eqtb [ 11337 + cursize ] .hh 
    .v.RH ] ] .cint ; 
  } 
  if ( mem [ q + 1 ] .cint == 0 ) 
  {
    if ( curstyle < 2 ) 
    clr = 7 * fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH ] 
    ] .cint ; 
    else clr = 3 * fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh 
    .v.RH ] ] .cint ; 
    delta = half ( clr - ( ( shiftup - mem [ x + 2 ] .cint ) - ( mem [ z + 3 ] 
    .cint - shiftdown ) ) ) ; 
    if ( delta > 0 ) 
    {
      shiftup = shiftup + delta ; 
      shiftdown = shiftdown + delta ; 
    } 
  } 
  else {
      
    if ( curstyle < 2 ) 
    clr = 3 * mem [ q + 1 ] .cint ; 
    else clr = mem [ q + 1 ] .cint ; 
    delta = half ( mem [ q + 1 ] .cint ) ; 
    delta1 = clr - ( ( shiftup - mem [ x + 2 ] .cint ) - ( fontinfo [ 22 + 
    parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] ] .cint + delta ) ) ; 
    delta2 = clr - ( ( fontinfo [ 22 + parambase [ eqtb [ 11337 + cursize ] 
    .hh .v.RH ] ] .cint - delta ) - ( mem [ z + 3 ] .cint - shiftdown ) ) ; 
    if ( delta1 > 0 ) 
    shiftup = shiftup + delta1 ; 
    if ( delta2 > 0 ) 
    shiftdown = shiftdown + delta2 ; 
  } 
  v = newnullbox () ; 
  mem [ v ] .hh.b0 = 1 ; 
  mem [ v + 3 ] .cint = shiftup + mem [ x + 3 ] .cint ; 
  mem [ v + 2 ] .cint = mem [ z + 2 ] .cint + shiftdown ; 
  mem [ v + 1 ] .cint = mem [ x + 1 ] .cint ; 
  if ( mem [ q + 1 ] .cint == 0 ) 
  {
    p = newkern ( ( shiftup - mem [ x + 2 ] .cint ) - ( mem [ z + 3 ] .cint - 
    shiftdown ) ) ; 
    mem [ p ] .hh .v.RH = z ; 
  } 
  else {
      
    y = fractionrule ( mem [ q + 1 ] .cint ) ; 
    p = newkern ( ( fontinfo [ 22 + parambase [ eqtb [ 11337 + cursize ] .hh 
    .v.RH ] ] .cint - delta ) - ( mem [ z + 3 ] .cint - shiftdown ) ) ; 
    mem [ y ] .hh .v.RH = p ; 
    mem [ p ] .hh .v.RH = z ; 
    p = newkern ( ( shiftup - mem [ x + 2 ] .cint ) - ( fontinfo [ 22 + 
    parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] ] .cint + delta ) ) ; 
    mem [ p ] .hh .v.RH = y ; 
  } 
  mem [ x ] .hh .v.RH = p ; 
  mem [ v + 5 ] .hh .v.RH = x ; 
  if ( curstyle < 2 ) 
  delta = fontinfo [ 20 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] ] 
  .cint ; 
  else delta = fontinfo [ 21 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH 
  ] ] .cint ; 
  x = vardelimiter ( q + 4 , cursize , delta ) ; 
  mem [ x ] .hh .v.RH = v ; 
  z = vardelimiter ( q + 5 , cursize , delta ) ; 
  mem [ v ] .hh .v.RH = z ; 
  mem [ q + 1 ] .cint = hpack ( x , 0 , 1 ) ; 
} 
scaled zmakeop ( q ) 
halfword q ; 
{register scaled Result; makeop_regmem 
  scaled delta  ; 
  halfword p, v, x, y, z  ; 
  quarterword c  ; 
  fourquarters i  ; 
  scaled shiftup, shiftdown  ; 
  if ( ( mem [ q ] .hh.b1 == 0 ) && ( curstyle < 2 ) ) 
  mem [ q ] .hh.b1 = 1 ; 
  if ( mem [ q + 1 ] .hh .v.RH == 1 ) 
  {
    fetch ( q + 1 ) ; 
    if ( ( curstyle < 2 ) && ( ( ( curi .b2 ) % 4 ) == 2 ) ) 
    {
      c = curi .b3 ; 
      i = fontinfo [ charbase [ curf ] + c ] .qqqq ; 
      if ( ( i .b0 > 0 ) ) 
      {
	curc = c ; 
	curi = i ; 
	mem [ q + 1 ] .hh.b1 = c ; 
      } 
    } 
    delta = fontinfo [ italicbase [ curf ] + ( curi .b2 ) / 4 ] .cint ; 
    x = cleanbox ( q + 1 , curstyle ) ; 
    if ( ( mem [ q + 3 ] .hh .v.RH != 0 ) && ( mem [ q ] .hh.b1 != 1 ) ) 
    mem [ x + 1 ] .cint = mem [ x + 1 ] .cint - delta ; 
    mem [ x + 4 ] .cint = half ( mem [ x + 3 ] .cint - mem [ x + 2 ] .cint ) - 
    fontinfo [ 22 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] ] .cint ; 
    mem [ q + 1 ] .hh .v.RH = 2 ; 
    mem [ q + 1 ] .hh .v.LH = x ; 
  } 
  else delta = 0 ; 
  if ( mem [ q ] .hh.b1 == 1 ) 
  {
    x = cleanbox ( q + 2 , 2 * ( curstyle / 4 ) + 4 + ( curstyle % 2 ) ) ; 
    y = cleanbox ( q + 1 , curstyle ) ; 
    z = cleanbox ( q + 3 , 2 * ( curstyle / 4 ) + 5 ) ; 
    v = newnullbox () ; 
    mem [ v ] .hh.b0 = 1 ; 
    mem [ v + 1 ] .cint = mem [ y + 1 ] .cint ; 
    if ( mem [ x + 1 ] .cint > mem [ v + 1 ] .cint ) 
    mem [ v + 1 ] .cint = mem [ x + 1 ] .cint ; 
    if ( mem [ z + 1 ] .cint > mem [ v + 1 ] .cint ) 
    mem [ v + 1 ] .cint = mem [ z + 1 ] .cint ; 
    x = rebox ( x , mem [ v + 1 ] .cint ) ; 
    y = rebox ( y , mem [ v + 1 ] .cint ) ; 
    z = rebox ( z , mem [ v + 1 ] .cint ) ; 
    mem [ x + 4 ] .cint = half ( delta ) ; 
    mem [ z + 4 ] .cint = - (integer) mem [ x + 4 ] .cint ; 
    mem [ v + 3 ] .cint = mem [ y + 3 ] .cint ; 
    mem [ v + 2 ] .cint = mem [ y + 2 ] .cint ; 
    if ( mem [ q + 2 ] .hh .v.RH == 0 ) 
    {
      freenode ( x , 7 ) ; 
      mem [ v + 5 ] .hh .v.RH = y ; 
    } 
    else {
	
      shiftup = fontinfo [ 11 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH 
      ] ] .cint - mem [ x + 2 ] .cint ; 
      if ( shiftup < fontinfo [ 9 + parambase [ eqtb [ 11338 + cursize ] .hh 
      .v.RH ] ] .cint ) 
      shiftup = fontinfo [ 9 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH 
      ] ] .cint ; 
      p = newkern ( shiftup ) ; 
      mem [ p ] .hh .v.RH = y ; 
      mem [ x ] .hh .v.RH = p ; 
      p = newkern ( fontinfo [ 13 + parambase [ eqtb [ 11338 + cursize ] .hh 
      .v.RH ] ] .cint ) ; 
      mem [ p ] .hh .v.RH = x ; 
      mem [ v + 5 ] .hh .v.RH = p ; 
      mem [ v + 3 ] .cint = mem [ v + 3 ] .cint + fontinfo [ 13 + parambase [ 
      eqtb [ 11338 + cursize ] .hh .v.RH ] ] .cint + mem [ x + 3 ] .cint + mem 
      [ x + 2 ] .cint + shiftup ; 
    } 
    if ( mem [ q + 3 ] .hh .v.RH == 0 ) 
    freenode ( z , 7 ) ; 
    else {
	
      shiftdown = fontinfo [ 12 + parambase [ eqtb [ 11338 + cursize ] .hh 
      .v.RH ] ] .cint - mem [ z + 3 ] .cint ; 
      if ( shiftdown < fontinfo [ 10 + parambase [ eqtb [ 11338 + cursize ] 
      .hh .v.RH ] ] .cint ) 
      shiftdown = fontinfo [ 10 + parambase [ eqtb [ 11338 + cursize ] .hh 
      .v.RH ] ] .cint ; 
      p = newkern ( shiftdown ) ; 
      mem [ y ] .hh .v.RH = p ; 
      mem [ p ] .hh .v.RH = z ; 
      p = newkern ( fontinfo [ 13 + parambase [ eqtb [ 11338 + cursize ] .hh 
      .v.RH ] ] .cint ) ; 
      mem [ z ] .hh .v.RH = p ; 
      mem [ v + 2 ] .cint = mem [ v + 2 ] .cint + fontinfo [ 13 + parambase [ 
      eqtb [ 11338 + cursize ] .hh .v.RH ] ] .cint + mem [ z + 3 ] .cint + mem 
      [ z + 2 ] .cint + shiftdown ; 
    } 
    mem [ q + 1 ] .cint = v ; 
  } 
  Result = delta ; 
  return(Result) ; 
} 
void zmakeord ( q ) 
halfword q ; 
{/* 20 10 */ makeord_regmem 
  integer a  ; 
  halfword p, r  ; 
  lab20: if ( mem [ q + 3 ] .hh .v.RH == 0 ) 
  if ( mem [ q + 2 ] .hh .v.RH == 0 ) 
  if ( mem [ q + 1 ] .hh .v.RH == 1 ) 
  {
    p = mem [ q ] .hh .v.RH ; 
    if ( p != 0 ) 
    if ( ( mem [ p ] .hh.b0 >= 16 ) && ( mem [ p ] .hh.b0 <= 22 ) ) 
    if ( mem [ p + 1 ] .hh .v.RH == 1 ) 
    if ( mem [ p + 1 ] .hh.b0 == mem [ q + 1 ] .hh.b0 ) 
    {
      mem [ q + 1 ] .hh .v.RH = 4 ; 
      fetch ( q + 1 ) ; 
      if ( ( ( curi .b2 ) % 4 ) == 1 ) 
      {
	a = ligkernbase [ curf ] + curi .b3 ; 
	curc = mem [ p + 1 ] .hh.b1 ; 
	curi = fontinfo [ a ] .qqqq ; 
	if ( curi .b0 > 128 ) 
	{
	  a = ligkernbase [ curf ] + 256 * curi .b2 + curi .b3 + 32768L - 256 
	  * ( 128 ) ; 
	  curi = fontinfo [ a ] .qqqq ; 
	} 
	while ( true ) {
	    
	  if ( curi .b1 == curc ) 
	  if ( curi .b0 <= 128 ) 
	  if ( curi .b2 >= 128 ) 
	  {
	    p = newkern ( fontinfo [ kernbase [ curf ] + 256 * curi .b2 + curi 
	    .b3 ] .cint ) ; 
	    mem [ p ] .hh .v.RH = mem [ q ] .hh .v.RH ; 
	    mem [ q ] .hh .v.RH = p ; 
	    return ; 
	  } 
	  else {
	      
	    {
	      if ( interrupt != 0 ) 
	      pauseforinstructions () ; 
	    } 
	    switch ( curi .b2 ) 
	    {case 1 : 
	    case 5 : 
	      mem [ q + 1 ] .hh.b1 = curi .b3 ; 
	      break ; 
	    case 2 : 
	    case 6 : 
	      mem [ p + 1 ] .hh.b1 = curi .b3 ; 
	      break ; 
	    case 3 : 
	    case 7 : 
	    case 11 : 
	      {
		r = newnoad () ; 
		mem [ r + 1 ] .hh.b1 = curi .b3 ; 
		mem [ r + 1 ] .hh.b0 = mem [ q + 1 ] .hh.b0 ; 
		mem [ q ] .hh .v.RH = r ; 
		mem [ r ] .hh .v.RH = p ; 
		if ( curi .b2 < 11 ) 
		mem [ r + 1 ] .hh .v.RH = 1 ; 
		else mem [ r + 1 ] .hh .v.RH = 4 ; 
	      } 
	      break ; 
	      default: 
	      {
		mem [ q ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
		mem [ q + 1 ] .hh.b1 = curi .b3 ; 
		mem [ q + 3 ] = mem [ p + 3 ] ; 
		mem [ q + 2 ] = mem [ p + 2 ] ; 
		freenode ( p , 4 ) ; 
	      } 
	      break ; 
	    } 
	    if ( curi .b2 > 3 ) 
	    return ; 
	    mem [ q + 1 ] .hh .v.RH = 1 ; 
	    goto lab20 ; 
	  } 
	  if ( curi .b0 >= 128 ) 
	  return ; 
	  a = a + curi .b0 + 1 ; 
	  curi = fontinfo [ a ] .qqqq ; 
	} 
      } 
    } 
  } 
} 
void zmakescripts ( q , delta ) 
halfword q ; 
scaled delta ; 
{makescripts_regmem 
  halfword p, x, y, z  ; 
  scaled shiftup, shiftdown, clr  ; 
  smallnumber t  ; 
  p = mem [ q + 1 ] .cint ; 
  if ( ( p >= himemmin ) ) 
  {
    shiftup = 0 ; 
    shiftdown = 0 ; 
  } 
  else {
      
    z = hpack ( p , 0 , 1 ) ; 
    if ( curstyle < 4 ) 
    t = 16 ; 
    else t = 32 ; 
    shiftup = mem [ z + 3 ] .cint - fontinfo [ 18 + parambase [ eqtb [ 11337 + 
    t ] .hh .v.RH ] ] .cint ; 
    shiftdown = mem [ z + 2 ] .cint + fontinfo [ 19 + parambase [ eqtb [ 11337 
    + t ] .hh .v.RH ] ] .cint ; 
    freenode ( z , 7 ) ; 
  } 
  if ( mem [ q + 2 ] .hh .v.RH == 0 ) 
  {
    x = cleanbox ( q + 3 , 2 * ( curstyle / 4 ) + 5 ) ; 
    mem [ x + 1 ] .cint = mem [ x + 1 ] .cint + eqtb [ 13242 ] .cint ; 
    if ( shiftdown < fontinfo [ 16 + parambase [ eqtb [ 11337 + cursize ] .hh 
    .v.RH ] ] .cint ) 
    shiftdown = fontinfo [ 16 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH 
    ] ] .cint ; 
    clr = mem [ x + 3 ] .cint - ( abs ( fontinfo [ 5 + parambase [ eqtb [ 
    11337 + cursize ] .hh .v.RH ] ] .cint * 4 ) / 5 ) ; 
    if ( shiftdown < clr ) 
    shiftdown = clr ; 
    mem [ x + 4 ] .cint = shiftdown ; 
  } 
  else {
      
    {
      x = cleanbox ( q + 2 , 2 * ( curstyle / 4 ) + 4 + ( curstyle % 2 ) ) ; 
      mem [ x + 1 ] .cint = mem [ x + 1 ] .cint + eqtb [ 13242 ] .cint ; 
      if ( odd ( curstyle ) ) 
      clr = fontinfo [ 15 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] ] 
      .cint ; 
      else if ( curstyle < 2 ) 
      clr = fontinfo [ 13 + parambase [ eqtb [ 11337 + cursize ] .hh .v.RH ] ] 
      .cint ; 
      else clr = fontinfo [ 14 + parambase [ eqtb [ 11337 + cursize ] .hh 
      .v.RH ] ] .cint ; 
      if ( shiftup < clr ) 
      shiftup = clr ; 
      clr = mem [ x + 2 ] .cint + ( abs ( fontinfo [ 5 + parambase [ eqtb [ 
      11337 + cursize ] .hh .v.RH ] ] .cint ) / 4 ) ; 
      if ( shiftup < clr ) 
      shiftup = clr ; 
    } 
    if ( mem [ q + 3 ] .hh .v.RH == 0 ) 
    mem [ x + 4 ] .cint = - (integer) shiftup ; 
    else {
	
      y = cleanbox ( q + 3 , 2 * ( curstyle / 4 ) + 5 ) ; 
      mem [ y + 1 ] .cint = mem [ y + 1 ] .cint + eqtb [ 13242 ] .cint ; 
      if ( shiftdown < fontinfo [ 17 + parambase [ eqtb [ 11337 + cursize ] 
      .hh .v.RH ] ] .cint ) 
      shiftdown = fontinfo [ 17 + parambase [ eqtb [ 11337 + cursize ] .hh 
      .v.RH ] ] .cint ; 
      clr = 4 * fontinfo [ 8 + parambase [ eqtb [ 11338 + cursize ] .hh .v.RH 
      ] ] .cint - ( ( shiftup - mem [ x + 2 ] .cint ) - ( mem [ y + 3 ] .cint 
      - shiftdown ) ) ; 
      if ( clr > 0 ) 
      {
	shiftdown = shiftdown + clr ; 
	clr = ( abs ( fontinfo [ 5 + parambase [ eqtb [ 11337 + cursize ] .hh 
	.v.RH ] ] .cint * 4 ) / 5 ) - ( shiftup - mem [ x + 2 ] .cint ) ; 
	if ( clr > 0 ) 
	{
	  shiftup = shiftup + clr ; 
	  shiftdown = shiftdown - clr ; 
	} 
      } 
      mem [ x + 4 ] .cint = delta ; 
      p = newkern ( ( shiftup - mem [ x + 2 ] .cint ) - ( mem [ y + 3 ] .cint 
      - shiftdown ) ) ; 
      mem [ x ] .hh .v.RH = p ; 
      mem [ p ] .hh .v.RH = y ; 
      x = vpackage ( x , 0 , 1 , 1073741823L ) ; 
      mem [ x + 4 ] .cint = shiftdown ; 
    } 
  } 
  if ( mem [ q + 1 ] .cint == 0 ) 
  mem [ q + 1 ] .cint = x ; 
  else {
      
    p = mem [ q + 1 ] .cint ; 
    while ( mem [ p ] .hh .v.RH != 0 ) p = mem [ p ] .hh .v.RH ; 
    mem [ p ] .hh .v.RH = x ; 
  } 
} 
smallnumber zmakeleftright ( q , style , maxd , maxh ) 
halfword q ; 
smallnumber style ; 
scaled maxd ; 
scaled maxh ; 
{register smallnumber Result; makeleftright_regmem 
  scaled delta, delta1, delta2  ; 
  if ( style < 4 ) 
  cursize = 0 ; 
  else cursize = 16 * ( ( style - 2 ) / 2 ) ; 
  delta2 = maxd + fontinfo [ 22 + parambase [ eqtb [ 11337 + cursize ] .hh 
  .v.RH ] ] .cint ; 
  delta1 = maxh + maxd - delta2 ; 
  if ( delta2 > delta1 ) 
  delta1 = delta2 ; 
  delta = ( delta1 / 500 ) * eqtb [ 12681 ] .cint ; 
  delta2 = delta1 + delta1 - eqtb [ 13240 ] .cint ; 
  if ( delta < delta2 ) 
  delta = delta2 ; 
  mem [ q + 1 ] .cint = vardelimiter ( q + 1 , cursize , delta ) ; 
  Result = mem [ q ] .hh.b0 - ( 10 ) ; 
  return(Result) ; 
} 
void mlisttohlist ( ) 
{/* 21 82 80 81 83 30 */ mlisttohlist_regmem 
  halfword mlist  ; 
  boolean penalties  ; 
  smallnumber style  ; 
  smallnumber savestyle  ; 
  halfword q  ; 
  halfword r  ; 
  smallnumber rtype  ; 
  smallnumber t  ; 
  halfword p, x, y, z  ; 
  integer pen  ; 
  smallnumber s  ; 
  scaled maxh, maxd  ; 
  scaled delta  ; 
  mlist = curmlist ; 
  penalties = mlistpenalties ; 
  style = curstyle ; 
  q = mlist ; 
  r = 0 ; 
  rtype = 17 ; 
  maxh = 0 ; 
  maxd = 0 ; 
  {
    if ( curstyle < 4 ) 
    cursize = 0 ; 
    else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ; 
    curmu = xovern ( fontinfo [ 6 + parambase [ eqtb [ 11337 + cursize ] .hh 
    .v.RH ] ] .cint , 18 ) ; 
  } 
  while ( q != 0 ) {
      
    lab21: delta = 0 ; 
    switch ( mem [ q ] .hh.b0 ) 
    {case 18 : 
      switch ( rtype ) 
      {case 18 : 
      case 17 : 
      case 19 : 
      case 20 : 
      case 22 : 
      case 30 : 
	{
	  mem [ q ] .hh.b0 = 16 ; 
	  goto lab21 ; 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
      break ; 
    case 19 : 
    case 21 : 
    case 22 : 
    case 31 : 
      {
	if ( rtype == 18 ) 
	mem [ r ] .hh.b0 = 16 ; 
	if ( mem [ q ] .hh.b0 == 31 ) 
	goto lab80 ; 
      } 
      break ; 
    case 30 : 
      goto lab80 ; 
      break ; 
    case 25 : 
      {
	makefraction ( q ) ; 
	goto lab82 ; 
      } 
      break ; 
    case 17 : 
      {
	delta = makeop ( q ) ; 
	if ( mem [ q ] .hh.b1 == 1 ) 
	goto lab82 ; 
      } 
      break ; 
    case 16 : 
      makeord ( q ) ; 
      break ; 
    case 20 : 
    case 23 : 
      ; 
      break ; 
    case 24 : 
      makeradical ( q ) ; 
      break ; 
    case 27 : 
      makeover ( q ) ; 
      break ; 
    case 26 : 
      makeunder ( q ) ; 
      break ; 
    case 28 : 
      makemathaccent ( q ) ; 
      break ; 
    case 29 : 
      makevcenter ( q ) ; 
      break ; 
    case 14 : 
      {
	curstyle = mem [ q ] .hh.b1 ; 
	{
	  if ( curstyle < 4 ) 
	  cursize = 0 ; 
	  else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ; 
	  curmu = xovern ( fontinfo [ 6 + parambase [ eqtb [ 11337 + cursize ] 
	  .hh .v.RH ] ] .cint , 18 ) ; 
	} 
	goto lab81 ; 
      } 
      break ; 
    case 15 : 
      {
	switch ( curstyle / 2 ) 
	{case 0 : 
	  {
	    p = mem [ q + 1 ] .hh .v.LH ; 
	    mem [ q + 1 ] .hh .v.LH = 0 ; 
	  } 
	  break ; 
	case 1 : 
	  {
	    p = mem [ q + 1 ] .hh .v.RH ; 
	    mem [ q + 1 ] .hh .v.RH = 0 ; 
	  } 
	  break ; 
	case 2 : 
	  {
	    p = mem [ q + 2 ] .hh .v.LH ; 
	    mem [ q + 2 ] .hh .v.LH = 0 ; 
	  } 
	  break ; 
	case 3 : 
	  {
	    p = mem [ q + 2 ] .hh .v.RH ; 
	    mem [ q + 2 ] .hh .v.RH = 0 ; 
	  } 
	  break ; 
	} 
	flushnodelist ( mem [ q + 1 ] .hh .v.LH ) ; 
	flushnodelist ( mem [ q + 1 ] .hh .v.RH ) ; 
	flushnodelist ( mem [ q + 2 ] .hh .v.LH ) ; 
	flushnodelist ( mem [ q + 2 ] .hh .v.RH ) ; 
	mem [ q ] .hh.b0 = 14 ; 
	mem [ q ] .hh.b1 = curstyle ; 
	mem [ q + 1 ] .cint = 0 ; 
	mem [ q + 2 ] .cint = 0 ; 
	if ( p != 0 ) 
	{
	  z = mem [ q ] .hh .v.RH ; 
	  mem [ q ] .hh .v.RH = p ; 
	  while ( mem [ p ] .hh .v.RH != 0 ) p = mem [ p ] .hh .v.RH ; 
	  mem [ p ] .hh .v.RH = z ; 
	} 
	goto lab81 ; 
      } 
      break ; 
    case 3 : 
    case 4 : 
    case 5 : 
    case 8 : 
    case 12 : 
    case 7 : 
      goto lab81 ; 
      break ; 
    case 2 : 
      {
	if ( mem [ q + 3 ] .cint > maxh ) 
	maxh = mem [ q + 3 ] .cint ; 
	if ( mem [ q + 2 ] .cint > maxd ) 
	maxd = mem [ q + 2 ] .cint ; 
	goto lab81 ; 
      } 
      break ; 
    case 10 : 
      {
	if ( mem [ q ] .hh.b1 == 99 ) 
	{
	  x = mem [ q + 1 ] .hh .v.LH ; 
	  y = mathglue ( x , curmu ) ; 
	  deleteglueref ( x ) ; 
	  mem [ q + 1 ] .hh .v.LH = y ; 
	  mem [ q ] .hh.b1 = 0 ; 
	} 
	else if ( ( cursize != 0 ) && ( mem [ q ] .hh.b1 == 98 ) ) 
	{
	  p = mem [ q ] .hh .v.RH ; 
	  if ( p != 0 ) 
	  if ( ( mem [ p ] .hh.b0 == 10 ) || ( mem [ p ] .hh.b0 == 11 ) ) 
	  {
	    mem [ q ] .hh .v.RH = mem [ p ] .hh .v.RH ; 
	    mem [ p ] .hh .v.RH = 0 ; 
	    flushnodelist ( p ) ; 
	  } 
	} 
	goto lab81 ; 
      } 
      break ; 
    case 11 : 
      {
	mathkern ( q , curmu ) ; 
	goto lab81 ; 
      } 
      break ; 
      default: 
      confusion ( 882 ) ; 
      break ; 
    } 
    switch ( mem [ q + 1 ] .hh .v.RH ) 
    {case 1 : 
    case 4 : 
      {
	fetch ( q + 1 ) ; 
	if ( ( curi .b0 > 0 ) ) 
	{
	  delta = fontinfo [ italicbase [ curf ] + ( curi .b2 ) / 4 ] .cint ; 
	  p = newcharacter ( curf , curc ) ; 
	  if ( ( mem [ q + 1 ] .hh .v.RH == 4 ) && ( fontinfo [ 2 + parambase 
	  [ curf ] ] .cint != 0 ) ) 
	  delta = 0 ; 
	  if ( ( mem [ q + 3 ] .hh .v.RH == 0 ) && ( delta != 0 ) ) 
	  {
	    mem [ p ] .hh .v.RH = newkern ( delta ) ; 
	    delta = 0 ; 
	  } 
	} 
	else p = 0 ; 
      } 
      break ; 
    case 0 : 
      p = 0 ; 
      break ; 
    case 2 : 
      p = mem [ q + 1 ] .hh .v.LH ; 
      break ; 
    case 3 : 
      {
	curmlist = mem [ q + 1 ] .hh .v.LH ; 
	savestyle = curstyle ; 
	mlistpenalties = false ; 
	mlisttohlist () ; 
	curstyle = savestyle ; 
	{
	  if ( curstyle < 4 ) 
	  cursize = 0 ; 
	  else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ; 
	  curmu = xovern ( fontinfo [ 6 + parambase [ eqtb [ 11337 + cursize ] 
	  .hh .v.RH ] ] .cint , 18 ) ; 
	} 
	p = hpack ( mem [ memtop - 3 ] .hh .v.RH , 0 , 1 ) ; 
      } 
      break ; 
      default: 
      confusion ( 883 ) ; 
      break ; 
    } 
    mem [ q + 1 ] .cint = p ; 
    if ( ( mem [ q + 3 ] .hh .v.RH == 0 ) && ( mem [ q + 2 ] .hh .v.RH == 0 ) 
    ) 
    goto lab82 ; 
    makescripts ( q , delta ) ; 
    lab82: z = hpack ( mem [ q + 1 ] .cint , 0 , 1 ) ; 
    if ( mem [ z + 3 ] .cint > maxh ) 
    maxh = mem [ z + 3 ] .cint ; 
    if ( mem [ z + 2 ] .cint > maxd ) 
    maxd = mem [ z + 2 ] .cint ; 
    freenode ( z , 7 ) ; 
    lab80: r = q ; 
    rtype = mem [ r ] .hh.b0 ; 
    lab81: q = mem [ q ] .hh .v.RH ; 
  } 
  if ( rtype == 18 ) 
  mem [ r ] .hh.b0 = 16 ; 
  p = memtop - 3 ; 
  mem [ p ] .hh .v.RH = 0 ; 
  q = mlist ; 
  rtype = 0 ; 
  curstyle = style ; 
  {
    if ( curstyle < 4 ) 
    cursize = 0 ; 
    else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ; 
    curmu = xovern ( fontinfo [ 6 + parambase [ eqtb [ 11337 + cursize ] .hh 
    .v.RH ] ] .cint , 18 ) ; 
  } 
  while ( q != 0 ) {
      
    t = 16 ; 
    s = 4 ; 
    pen = 10000 ; 
    switch ( mem [ q ] .hh.b0 ) 
    {case 17 : 
    case 20 : 
    case 21 : 
    case 22 : 
    case 23 : 
      t = mem [ q ] .hh.b0 ; 
      break ; 
    case 18 : 
      {
	t = 18 ; 
	pen = eqtb [ 12672 ] .cint ; 
      } 
      break ; 
    case 19 : 
      {
	t = 19 ; 
	pen = eqtb [ 12673 ] .cint ; 
      } 
      break ; 
    case 16 : 
    case 29 : 
    case 27 : 
    case 26 : 
      ; 
      break ; 
    case 24 : 
      s = 5 ; 
      break ; 
    case 28 : 
      s = 5 ; 
      break ; 
    case 25 : 
      {
	t = 23 ; 
	s = 6 ; 
      } 
      break ; 
    case 30 : 
    case 31 : 
      t = makeleftright ( q , style , maxd , maxh ) ; 
      break ; 
    case 14 : 
      {
	curstyle = mem [ q ] .hh.b1 ; 
	s = 3 ; 
	{
	  if ( curstyle < 4 ) 
	  cursize = 0 ; 
	  else cursize = 16 * ( ( curstyle - 2 ) / 2 ) ; 
	  curmu = xovern ( fontinfo [ 6 + parambase [ eqtb [ 11337 + cursize ] 
	  .hh .v.RH ] ] .cint , 18 ) ; 
	} 
	goto lab83 ; 
      } 
      break ; 
    case 8 : 
    case 12 : 
    case 2 : 
    case 7 : 
    case 5 : 
    case 3 : 
    case 4 : 
    case 10 : 
    case 11 : 
      {
	mem [ p ] .hh .v.RH = q ; 
	p = q ; 
	q = mem [ q ] .hh .v.RH ; 
	mem [ p ] .hh .v.RH = 0 ; 
	goto lab30 ; 
      } 
      break ; 
      default: 
      confusion ( 884 ) ; 
      break ; 
    } 
    if ( rtype > 0 ) 
    {
      switch ( strpool [ rtype * 8 + t + magicoffset ] ) 
      {case 48 : 
	x = 0 ; 
	break ; 
      case 49 : 
	if ( curstyle < 4 ) 
	x = 15 ; 
	else x = 0 ; 
	break ; 
      case 50 : 
	x = 15 ; 
	break ; 
      case 51 : 
	if ( curstyle < 4 ) 
	x = 16 ; 
	else x = 0 ; 
	break ; 
      case 52 : 
	if ( curstyle < 4 ) 
	x = 17 ; 
	else x = 0 ; 
	break ; 
	default: 
	confusion ( 886 ) ; 
	break ; 
      } 
      if ( x != 0 ) 
      {
	y = mathglue ( eqtb [ 10282 + x ] .hh .v.RH , curmu ) ; 
	z = newglue ( y ) ; 
	mem [ y ] .hh .v.RH = 0 ; 
	mem [ p ] .hh .v.RH = z ; 
	p = z ; 
	mem [ z ] .hh.b1 = x + 1 ; 
      } 
    } 
    if ( mem [ q + 1 ] .cint != 0 ) 
    {
      mem [ p ] .hh .v.RH = mem [ q + 1 ] .cint ; 
      do {
	  p = mem [ p ] .hh .v.RH ; 
      } while ( ! ( mem [ p ] .hh .v.RH == 0 ) ) ; 
    } 
    if ( penalties ) 
    if ( mem [ q ] .hh .v.RH != 0 ) 
    if ( pen < 10000 ) 
    {
      rtype = mem [ mem [ q ] .hh .v.RH ] .hh.b0 ; 
      if ( rtype != 12 ) 
      if ( rtype != 19 ) 
      {
	z = newpenalty ( pen ) ; 
	mem [ p ] .hh .v.RH = z ; 
	p = z ; 
      } 
    } 
    rtype = t ; 
    lab83: r = q ; 
    q = mem [ q ] .hh .v.RH ; 
    freenode ( r , s ) ; 
    lab30: ; 
  } 
} 
void pushalignment ( ) 
{pushalignment_regmem 
  halfword p  ; 
  p = getnode ( 5 ) ; 
  mem [ p ] .hh .v.RH = alignptr ; 
  mem [ p ] .hh .v.LH = curalign ; 
  mem [ p + 1 ] .hh .v.LH = mem [ memtop - 8 ] .hh .v.RH ; 
  mem [ p + 1 ] .hh .v.RH = curspan ; 
  mem [ p + 2 ] .cint = curloop ; 
  mem [ p + 3 ] .cint = alignstate ; 
  mem [ p + 4 ] .hh .v.LH = curhead ; 
  mem [ p + 4 ] .hh .v.RH = curtail ; 
  alignptr = p ; 
  curhead = getavail () ; 
} 
void popalignment ( ) 
{popalignment_regmem 
  halfword p  ; 
  {
    mem [ curhead ] .hh .v.RH = avail ; 
    avail = curhead ; 
	;
#ifdef STAT
    decr ( dynused ) ; 
#endif /* STAT */
  } 
  p = alignptr ; 
  curtail = mem [ p + 4 ] .hh .v.RH ; 
  curhead = mem [ p + 4 ] .hh .v.LH ; 
  alignstate = mem [ p + 3 ] .cint ; 
  curloop = mem [ p + 2 ] .cint ; 
  curspan = mem [ p + 1 ] .hh .v.RH ; 
  mem [ memtop - 8 ] .hh .v.RH = mem [ p + 1 ] .hh .v.LH ; 
  curalign = mem [ p ] .hh .v.LH ; 
  alignptr = mem [ p ] .hh .v.RH ; 
  freenode ( p , 5 ) ; 
} 
void getpreambletoken ( ) 
{/* 20 */ getpreambletoken_regmem 
  lab20: gettoken () ; 
  while ( ( curchr == 256 ) && ( curcmd == 4 ) ) {
      
    gettoken () ; 
    if ( curcmd > 100 ) 
    {
      expand () ; 
      gettoken () ; 
    } 
  } 
  if ( curcmd == 9 ) 
  fatalerror ( 591 ) ; 
  if ( ( curcmd == 75 ) && ( curchr == 10293 ) ) 
  {
    scanoptionalequals () ; 
    scanglue ( 2 ) ; 
    if ( eqtb [ 12706 ] .cint > 0 ) 
    geqdefine ( 10293 , 117 , curval ) ; 
    else eqdefine ( 10293 , 117 , curval ) ; 
    goto lab20 ; 
  } 
} 
void initalign ( ) 
{/* 30 31 32 22 */ initalign_regmem 
  halfword savecsptr  ; 
  halfword p  ; 
  savecsptr = curcs ; 
  pushalignment () ; 
  alignstate = -1000000L ; 
  if ( ( curlist .modefield == 203 ) && ( ( curlist .tailfield != curlist 
  .headfield ) || ( curlist .auxfield .cint != 0 ) ) ) 
  {
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 676 ) ; 
    } 
    printesc ( 516 ) ; 
    print ( 887 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 888 ; 
      helpline [ 1 ] = 889 ; 
      helpline [ 0 ] = 890 ; 
    } 
    error () ; 
    flushmath () ; 
  } 
  pushnest () ; 
  if ( curlist .modefield == 203 ) 
  {
    curlist .modefield = -1 ; 
    curlist .auxfield .cint = nest [ nestptr - 2 ] .auxfield .cint ; 
  } 
  else if ( curlist .modefield > 0 ) 
  curlist .modefield = - (integer) curlist .modefield ; 
  scanspec ( 6 , false ) ; 
  mem [ memtop - 8 ] .hh .v.RH = 0 ; 
  curalign = memtop - 8 ; 
  curloop = 0 ; 
  scannerstatus = 4 ; 
  warningindex = savecsptr ; 
  alignstate = -1000000L ; 
  while ( true ) {
      
    mem [ curalign ] .hh .v.RH = newparamglue ( 11 ) ; 
    curalign = mem [ curalign ] .hh .v.RH ; 
    if ( curcmd == 5 ) 
    goto lab30 ; 
    p = memtop - 4 ; 
    mem [ p ] .hh .v.RH = 0 ; 
    while ( true ) {
	
      getpreambletoken () ; 
      if ( curcmd == 6 ) 
      goto lab31 ; 
      if ( ( curcmd <= 5 ) && ( curcmd >= 4 ) && ( alignstate == -1000000L ) ) 
      if ( ( p == memtop - 4 ) && ( curloop == 0 ) && ( curcmd == 4 ) ) 
      curloop = curalign ; 
      else {
	  
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 896 ) ; 
	} 
	{
	  helpptr = 3 ; 
	  helpline [ 2 ] = 897 ; 
	  helpline [ 1 ] = 898 ; 
	  helpline [ 0 ] = 899 ; 
	} 
	backerror () ; 
	goto lab31 ; 
      } 
      else if ( ( curcmd != 10 ) || ( p != memtop - 4 ) ) 
      {
	mem [ p ] .hh .v.RH = getavail () ; 
	p = mem [ p ] .hh .v.RH ; 
	mem [ p ] .hh .v.LH = curtok ; 
      } 
    } 
    lab31: ; 
    mem [ curalign ] .hh .v.RH = newnullbox () ; 
    curalign = mem [ curalign ] .hh .v.RH ; 
    mem [ curalign ] .hh .v.LH = memtop - 9 ; 
    mem [ curalign + 1 ] .cint = -1073741824L ; 
    mem [ curalign + 3 ] .cint = mem [ memtop - 4 ] .hh .v.RH ; 
    p = memtop - 4 ; 
    mem [ p ] .hh .v.RH = 0 ; 
    while ( true ) {
	
      lab22: getpreambletoken () ; 
      if ( ( curcmd <= 5 ) && ( curcmd >= 4 ) && ( alignstate == -1000000L ) ) 
      goto lab32 ; 
      if ( curcmd == 6 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 900 ) ; 
	} 
	{
	  helpptr = 3 ; 
	  helpline [ 2 ] = 897 ; 
	  helpline [ 1 ] = 898 ; 
	  helpline [ 0 ] = 901 ; 
	} 
	error () ; 
	goto lab22 ; 
      } 
      mem [ p ] .hh .v.RH = getavail () ; 
      p = mem [ p ] .hh .v.RH ; 
      mem [ p ] .hh .v.LH = curtok ; 
    } 
    lab32: mem [ p ] .hh .v.RH = getavail () ; 
    p = mem [ p ] .hh .v.RH ; 
    mem [ p ] .hh .v.LH = 14114 ; 
    mem [ curalign + 2 ] .cint = mem [ memtop - 4 ] .hh .v.RH ; 
  } 
  lab30: scannerstatus = 0 ; 
  newsavelevel ( 6 ) ; 
  if ( eqtb [ 10820 ] .hh .v.RH != 0 ) 
  begintokenlist ( eqtb [ 10820 ] .hh .v.RH , 13 ) ; 
  alignpeek () ; 
} 
void zinitspan ( p ) 
halfword p ; 
{initspan_regmem 
  pushnest () ; 
  if ( curlist .modefield == -102 ) 
  curlist .auxfield .hh .v.LH = 1000 ; 
  else {
      
    curlist .auxfield .cint = -65536000L ; 
    normalparagraph () ; 
  } 
  curspan = p ; 
} 
void initrow ( ) 
{initrow_regmem 
  pushnest () ; 
  curlist .modefield = ( -103 ) - curlist .modefield ; 
  if ( curlist .modefield == -102 ) 
  curlist .auxfield .hh .v.LH = 0 ; 
  else curlist .auxfield .cint = 0 ; 
  {
    mem [ curlist .tailfield ] .hh .v.RH = newglue ( mem [ mem [ memtop - 8 ] 
    .hh .v.RH + 1 ] .hh .v.LH ) ; 
    curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
  } 
  mem [ curlist .tailfield ] .hh.b1 = 12 ; 
  curalign = mem [ mem [ memtop - 8 ] .hh .v.RH ] .hh .v.RH ; 
  curtail = curhead ; 
  initspan ( curalign ) ; 
} 
void initcol ( ) 
{initcol_regmem 
  mem [ curalign + 5 ] .hh .v.LH = curcmd ; 
  if ( curcmd == 63 ) 
  alignstate = 0 ; 
  else {
      
    backinput () ; 
    begintokenlist ( mem [ curalign + 3 ] .cint , 1 ) ; 
  } 
} 
boolean fincol ( ) 
{/* 10 */ register boolean Result; fincol_regmem 
  halfword p  ; 
  halfword q, r  ; 
  halfword s  ; 
  halfword u  ; 
  scaled w  ; 
  glueord o  ; 
  halfword n  ; 
  if ( curalign == 0 ) 
  confusion ( 902 ) ; 
  q = mem [ curalign ] .hh .v.RH ; 
  if ( q == 0 ) 
  confusion ( 902 ) ; 
  if ( alignstate < 500000L ) 
  fatalerror ( 591 ) ; 
  p = mem [ q ] .hh .v.RH ; 
  if ( ( p == 0 ) && ( mem [ curalign + 5 ] .hh .v.LH < 257 ) ) 
  if ( curloop != 0 ) 
  {
    mem [ q ] .hh .v.RH = newnullbox () ; 
    p = mem [ q ] .hh .v.RH ; 
    mem [ p ] .hh .v.LH = memtop - 9 ; 
    mem [ p + 1 ] .cint = -1073741824L ; 
    curloop = mem [ curloop ] .hh .v.RH ; 
    q = memtop - 4 ; 
    r = mem [ curloop + 3 ] .cint ; 
    while ( r != 0 ) {
	
      mem [ q ] .hh .v.RH = getavail () ; 
      q = mem [ q ] .hh .v.RH ; 
      mem [ q ] .hh .v.LH = mem [ r ] .hh .v.LH ; 
      r = mem [ r ] .hh .v.RH ; 
    } 
    mem [ q ] .hh .v.RH = 0 ; 
    mem [ p + 3 ] .cint = mem [ memtop - 4 ] .hh .v.RH ; 
    q = memtop - 4 ; 
    r = mem [ curloop + 2 ] .cint ; 
    while ( r != 0 ) {
	
      mem [ q ] .hh .v.RH = getavail () ; 
      q = mem [ q ] .hh .v.RH ; 
      mem [ q ] .hh .v.LH = mem [ r ] .hh .v.LH ; 
      r = mem [ r ] .hh .v.RH ; 
    } 
    mem [ q ] .hh .v.RH = 0 ; 
    mem [ p + 2 ] .cint = mem [ memtop - 4 ] .hh .v.RH ; 
    curloop = mem [ curloop ] .hh .v.RH ; 
    mem [ p ] .hh .v.RH = newglue ( mem [ curloop + 1 ] .hh .v.LH ) ; 
  } 
  else {
      
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 903 ) ; 
    } 
    printesc ( 892 ) ; 
    {
      helpptr = 3 ; 
      helpline [ 2 ] = 904 ; 
      helpline [ 1 ] = 905 ; 
      helpline [ 0 ] = 906 ; 
    } 
    mem [ curalign + 5 ] .hh .v.LH = 257 ; 
    error () ; 
  } 
  if ( mem [ curalign + 5 ] .hh .v.LH != 256 ) 
  {
    unsave () ; 
    newsavelevel ( 6 ) ; 
    {
      if ( curlist .modefield == -102 ) 
      {
	adjusttail = curtail ; 
	u = hpack ( mem [ curlist .headfield ] .hh .v.RH , 0 , 1 ) ; 
	w = mem [ u + 1 ] .cint ; 
	curtail = adjusttail ; 
	adjusttail = 0 ; 
      } 
      else {
	  
	u = vpackage ( mem [ curlist .headfield ] .hh .v.RH , 0 , 1 , 0 ) ; 
	w = mem [ u + 3 ] .cint ; 
      } 
      n = 0 ; 
      if ( curspan != curalign ) 
      {
	q = curspan ; 
	do {
	    incr ( n ) ; 
	  q = mem [ mem [ q ] .hh .v.RH ] .hh .v.RH ; 
	} while ( ! ( q == curalign ) ) ; 
	if ( n > 255 ) 
	confusion ( 907 ) ; 
	q = curspan ; 
	while ( mem [ mem [ q ] .hh .v.LH ] .hh .v.RH < n ) q = mem [ q ] .hh 
	.v.LH ; 
	if ( mem [ mem [ q ] .hh .v.LH ] .hh .v.RH > n ) 
	{
	  s = getnode ( 2 ) ; 
	  mem [ s ] .hh .v.LH = mem [ q ] .hh .v.LH ; 
	  mem [ s ] .hh .v.RH = n ; 
	  mem [ q ] .hh .v.LH = s ; 
	  mem [ s + 1 ] .cint = w ; 
	} 
	else if ( mem [ mem [ q ] .hh .v.LH + 1 ] .cint < w ) 
	mem [ mem [ q ] .hh .v.LH + 1 ] .cint = w ; 
      } 
      else if ( w > mem [ curalign + 1 ] .cint ) 
      mem [ curalign + 1 ] .cint = w ; 
      mem [ u ] .hh.b0 = 13 ; 
      mem [ u ] .hh.b1 = n ; 
      if ( totalstretch [ 3 ] != 0 ) 
      o = 3 ; 
      else if ( totalstretch [ 2 ] != 0 ) 
      o = 2 ; 
      else if ( totalstretch [ 1 ] != 0 ) 
      o = 1 ; 
      else o = 0 ; 
      mem [ u + 5 ] .hh.b1 = o ; 
      mem [ u + 6 ] .cint = totalstretch [ o ] ; 
      if ( totalshrink [ 3 ] != 0 ) 
      o = 3 ; 
      else if ( totalshrink [ 2 ] != 0 ) 
      o = 2 ; 
      else if ( totalshrink [ 1 ] != 0 ) 
      o = 1 ; 
      else o = 0 ; 
      mem [ u + 5 ] .hh.b0 = o ; 
      mem [ u + 4 ] .cint = totalshrink [ o ] ; 
      popnest () ; 
      mem [ curlist .tailfield ] .hh .v.RH = u ; 
      curlist .tailfield = u ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newglue ( mem [ mem [ curalign ] 
      .hh .v.RH + 1 ] .hh .v.LH ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    mem [ curlist .tailfield ] .hh.b1 = 12 ; 
    if ( mem [ curalign + 5 ] .hh .v.LH >= 257 ) 
    {
      Result = true ; 
      return(Result) ; 
    } 
    initspan ( p ) ; 
  } 
  alignstate = 1000000L ; 
  do {
      getxtoken () ; 
  } while ( ! ( curcmd != 10 ) ) ; 
  curalign = p ; 
  initcol () ; 
  Result = false ; 
  return(Result) ; 
} 
void finrow ( ) 
{finrow_regmem 
  halfword p  ; 
  if ( curlist .modefield == -102 ) 
  {
    p = hpack ( mem [ curlist .headfield ] .hh .v.RH , 0 , 1 ) ; 
    popnest () ; 
    appendtovlist ( p ) ; 
    if ( curhead != curtail ) 
    {
      mem [ curlist .tailfield ] .hh .v.RH = mem [ curhead ] .hh .v.RH ; 
      curlist .tailfield = curtail ; 
    } 
  } 
  else {
      
    p = vpackage ( mem [ curlist .headfield ] .hh .v.RH , 0 , 1 , 1073741823L 
    ) ; 
    popnest () ; 
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    curlist .tailfield = p ; 
    curlist .auxfield .hh .v.LH = 1000 ; 
  } 
  mem [ p ] .hh.b0 = 13 ; 
  mem [ p + 6 ] .cint = 0 ; 
  if ( eqtb [ 10820 ] .hh .v.RH != 0 ) 
  begintokenlist ( eqtb [ 10820 ] .hh .v.RH , 13 ) ; 
  alignpeek () ; 
} 
void finalign ( ) 
{finalign_regmem 
  halfword p, q, r, s, u, v  ; 
  scaled t, w  ; 
  scaled o  ; 
  halfword n  ; 
  scaled rulesave  ; 
  memoryword auxsave  ; 
  if ( curgroup != 6 ) 
  confusion ( 908 ) ; 
  unsave () ; 
  if ( curgroup != 6 ) 
  confusion ( 909 ) ; 
  unsave () ; 
  if ( nest [ nestptr - 1 ] .modefield == 203 ) 
  o = eqtb [ 13245 ] .cint ; 
  else o = 0 ; 
  q = mem [ mem [ memtop - 8 ] .hh .v.RH ] .hh .v.RH ; 
  do {
      flushlist ( mem [ q + 3 ] .cint ) ; 
    flushlist ( mem [ q + 2 ] .cint ) ; 
    p = mem [ mem [ q ] .hh .v.RH ] .hh .v.RH ; 
    if ( mem [ q + 1 ] .cint == -1073741824L ) 
    {
      mem [ q + 1 ] .cint = 0 ; 
      r = mem [ q ] .hh .v.RH ; 
      s = mem [ r + 1 ] .hh .v.LH ; 
      if ( s != 0 ) 
      {
	incr ( mem [ 0 ] .hh .v.RH ) ; 
	deleteglueref ( s ) ; 
	mem [ r + 1 ] .hh .v.LH = 0 ; 
      } 
    } 
    if ( mem [ q ] .hh .v.LH != memtop - 9 ) 
    {
      t = mem [ q + 1 ] .cint + mem [ mem [ mem [ q ] .hh .v.RH + 1 ] .hh 
      .v.LH + 1 ] .cint ; 
      r = mem [ q ] .hh .v.LH ; 
      s = memtop - 9 ; 
      mem [ s ] .hh .v.LH = p ; 
      n = 1 ; 
      do {
	  mem [ r + 1 ] .cint = mem [ r + 1 ] .cint - t ; 
	u = mem [ r ] .hh .v.LH ; 
	while ( mem [ r ] .hh .v.RH > n ) {
	    
	  s = mem [ s ] .hh .v.LH ; 
	  n = mem [ mem [ s ] .hh .v.LH ] .hh .v.RH + 1 ; 
	} 
	if ( mem [ r ] .hh .v.RH < n ) 
	{
	  mem [ r ] .hh .v.LH = mem [ s ] .hh .v.LH ; 
	  mem [ s ] .hh .v.LH = r ; 
	  decr ( mem [ r ] .hh .v.RH ) ; 
	  s = r ; 
	} 
	else {
	    
	  if ( mem [ r + 1 ] .cint > mem [ mem [ s ] .hh .v.LH + 1 ] .cint ) 
	  mem [ mem [ s ] .hh .v.LH + 1 ] .cint = mem [ r + 1 ] .cint ; 
	  freenode ( r , 2 ) ; 
	} 
	r = u ; 
      } while ( ! ( r == memtop - 9 ) ) ; 
    } 
    mem [ q ] .hh.b0 = 13 ; 
    mem [ q ] .hh.b1 = 0 ; 
    mem [ q + 3 ] .cint = 0 ; 
    mem [ q + 2 ] .cint = 0 ; 
    mem [ q + 5 ] .hh.b1 = 0 ; 
    mem [ q + 5 ] .hh.b0 = 0 ; 
    mem [ q + 6 ] .cint = 0 ; 
    mem [ q + 4 ] .cint = 0 ; 
    q = p ; 
  } while ( ! ( q == 0 ) ) ; 
  saveptr = saveptr - 2 ; 
  packbeginline = - (integer) curlist .mlfield ; 
  if ( curlist .modefield == -1 ) 
  {
    rulesave = eqtb [ 13246 ] .cint ; 
    eqtb [ 13246 ] .cint = 0 ; 
    p = hpack ( mem [ memtop - 8 ] .hh .v.RH , savestack [ saveptr + 1 ] .cint 
    , savestack [ saveptr + 0 ] .cint ) ; 
    eqtb [ 13246 ] .cint = rulesave ; 
  } 
  else {
      
    q = mem [ mem [ memtop - 8 ] .hh .v.RH ] .hh .v.RH ; 
    do {
	mem [ q + 3 ] .cint = mem [ q + 1 ] .cint ; 
      mem [ q + 1 ] .cint = 0 ; 
      q = mem [ mem [ q ] .hh .v.RH ] .hh .v.RH ; 
    } while ( ! ( q == 0 ) ) ; 
    p = vpackage ( mem [ memtop - 8 ] .hh .v.RH , savestack [ saveptr + 1 ] 
    .cint , savestack [ saveptr + 0 ] .cint , 1073741823L ) ; 
    q = mem [ mem [ memtop - 8 ] .hh .v.RH ] .hh .v.RH ; 
    do {
	mem [ q + 1 ] .cint = mem [ q + 3 ] .cint ; 
      mem [ q + 3 ] .cint = 0 ; 
      q = mem [ mem [ q ] .hh .v.RH ] .hh .v.RH ; 
    } while ( ! ( q == 0 ) ) ; 
  } 
  packbeginline = 0 ; 
  q = mem [ curlist .headfield ] .hh .v.RH ; 
  s = curlist .headfield ; 
  while ( q != 0 ) {
      
    if ( ! ( q >= himemmin ) ) 
    if ( mem [ q ] .hh.b0 == 13 ) 
    {
      if ( curlist .modefield == -1 ) 
      {
	mem [ q ] .hh.b0 = 0 ; 
	mem [ q + 1 ] .cint = mem [ p + 1 ] .cint ; 
      } 
      else {
	  
	mem [ q ] .hh.b0 = 1 ; 
	mem [ q + 3 ] .cint = mem [ p + 3 ] .cint ; 
      } 
      mem [ q + 5 ] .hh.b1 = mem [ p + 5 ] .hh.b1 ; 
      mem [ q + 5 ] .hh.b0 = mem [ p + 5 ] .hh.b0 ; 
      mem [ q + 6 ] .gr = mem [ p + 6 ] .gr ; 
      mem [ q + 4 ] .cint = o ; 
      r = mem [ mem [ q + 5 ] .hh .v.RH ] .hh .v.RH ; 
      s = mem [ mem [ p + 5 ] .hh .v.RH ] .hh .v.RH ; 
      do {
	  n = mem [ r ] .hh.b1 ; 
	t = mem [ s + 1 ] .cint ; 
	w = t ; 
	u = memtop - 4 ; 
	while ( n > 0 ) {
	    
	  decr ( n ) ; 
	  s = mem [ s ] .hh .v.RH ; 
	  v = mem [ s + 1 ] .hh .v.LH ; 
	  mem [ u ] .hh .v.RH = newglue ( v ) ; 
	  u = mem [ u ] .hh .v.RH ; 
	  mem [ u ] .hh.b1 = 12 ; 
	  t = t + mem [ v + 1 ] .cint ; 
	  if ( mem [ p + 5 ] .hh.b0 == 1 ) 
	  {
	    if ( mem [ v ] .hh.b0 == mem [ p + 5 ] .hh.b1 ) 
	    t = t + round ( mem [ p + 6 ] .gr * mem [ v + 2 ] .cint ) ; 
	  } 
	  else if ( mem [ p + 5 ] .hh.b0 == 2 ) 
	  {
	    if ( mem [ v ] .hh.b1 == mem [ p + 5 ] .hh.b1 ) 
	    t = t - round ( mem [ p + 6 ] .gr * mem [ v + 3 ] .cint ) ; 
	  } 
	  s = mem [ s ] .hh .v.RH ; 
	  mem [ u ] .hh .v.RH = newnullbox () ; 
	  u = mem [ u ] .hh .v.RH ; 
	  t = t + mem [ s + 1 ] .cint ; 
	  if ( curlist .modefield == -1 ) 
	  mem [ u + 1 ] .cint = mem [ s + 1 ] .cint ; 
	  else {
	      
	    mem [ u ] .hh.b0 = 1 ; 
	    mem [ u + 3 ] .cint = mem [ s + 1 ] .cint ; 
	  } 
	} 
	if ( curlist .modefield == -1 ) 
	{
	  mem [ r + 3 ] .cint = mem [ q + 3 ] .cint ; 
	  mem [ r + 2 ] .cint = mem [ q + 2 ] .cint ; 
	  if ( t == mem [ r + 1 ] .cint ) 
	  {
	    mem [ r + 5 ] .hh.b0 = 0 ; 
	    mem [ r + 5 ] .hh.b1 = 0 ; 
	    mem [ r + 6 ] .gr = 0.0 ; 
	  } 
	  else if ( t > mem [ r + 1 ] .cint ) 
	  {
	    mem [ r + 5 ] .hh.b0 = 1 ; 
	    if ( mem [ r + 6 ] .cint == 0 ) 
	    mem [ r + 6 ] .gr = 0.0 ; 
	    else mem [ r + 6 ] .gr = ( t - mem [ r + 1 ] .cint ) / ((double) 
	    mem [ r + 6 ] .cint ) ; 
	  } 
	  else {
	      
	    mem [ r + 5 ] .hh.b1 = mem [ r + 5 ] .hh.b0 ; 
	    mem [ r + 5 ] .hh.b0 = 2 ; 
	    if ( mem [ r + 4 ] .cint == 0 ) 
	    mem [ r + 6 ] .gr = 0.0 ; 
	    else if ( ( mem [ r + 5 ] .hh.b1 == 0 ) && ( mem [ r + 1 ] .cint - 
	    t > mem [ r + 4 ] .cint ) ) 
	    mem [ r + 6 ] .gr = 1.0 ; 
	    else mem [ r + 6 ] .gr = ( mem [ r + 1 ] .cint - t ) / ((double) 
	    mem [ r + 4 ] .cint ) ; 
	  } 
	  mem [ r + 1 ] .cint = w ; 
	  mem [ r ] .hh.b0 = 0 ; 
	} 
	else {
	    
	  mem [ r + 1 ] .cint = mem [ q + 1 ] .cint ; 
	  if ( t == mem [ r + 3 ] .cint ) 
	  {
	    mem [ r + 5 ] .hh.b0 = 0 ; 
	    mem [ r + 5 ] .hh.b1 = 0 ; 
	    mem [ r + 6 ] .gr = 0.0 ; 
	  } 
	  else if ( t > mem [ r + 3 ] .cint ) 
	  {
	    mem [ r + 5 ] .hh.b0 = 1 ; 
	    if ( mem [ r + 6 ] .cint == 0 ) 
	    mem [ r + 6 ] .gr = 0.0 ; 
	    else mem [ r + 6 ] .gr = ( t - mem [ r + 3 ] .cint ) / ((double) 
	    mem [ r + 6 ] .cint ) ; 
	  } 
	  else {
	      
	    mem [ r + 5 ] .hh.b1 = mem [ r + 5 ] .hh.b0 ; 
	    mem [ r + 5 ] .hh.b0 = 2 ; 
	    if ( mem [ r + 4 ] .cint == 0 ) 
	    mem [ r + 6 ] .gr = 0.0 ; 
	    else if ( ( mem [ r + 5 ] .hh.b1 == 0 ) && ( mem [ r + 3 ] .cint - 
	    t > mem [ r + 4 ] .cint ) ) 
	    mem [ r + 6 ] .gr = 1.0 ; 
	    else mem [ r + 6 ] .gr = ( mem [ r + 3 ] .cint - t ) / ((double) 
	    mem [ r + 4 ] .cint ) ; 
	  } 
	  mem [ r + 3 ] .cint = w ; 
	  mem [ r ] .hh.b0 = 1 ; 
	} 
	mem [ r + 4 ] .cint = 0 ; 
	if ( u != memtop - 4 ) 
	{
	  mem [ u ] .hh .v.RH = mem [ r ] .hh .v.RH ; 
	  mem [ r ] .hh .v.RH = mem [ memtop - 4 ] .hh .v.RH ; 
	  r = u ; 
	} 
	r = mem [ mem [ r ] .hh .v.RH ] .hh .v.RH ; 
	s = mem [ mem [ s ] .hh .v.RH ] .hh .v.RH ; 
      } while ( ! ( r == 0 ) ) ; 
    } 
    else if ( mem [ q ] .hh.b0 == 2 ) 
    {
      if ( ( mem [ q + 1 ] .cint == -1073741824L ) ) 
      mem [ q + 1 ] .cint = mem [ p + 1 ] .cint ; 
      if ( ( mem [ q + 3 ] .cint == -1073741824L ) ) 
      mem [ q + 3 ] .cint = mem [ p + 3 ] .cint ; 
      if ( ( mem [ q + 2 ] .cint == -1073741824L ) ) 
      mem [ q + 2 ] .cint = mem [ p + 2 ] .cint ; 
      if ( o != 0 ) 
      {
	r = mem [ q ] .hh .v.RH ; 
	mem [ q ] .hh .v.RH = 0 ; 
	q = hpack ( q , 0 , 1 ) ; 
	mem [ q + 4 ] .cint = o ; 
	mem [ q ] .hh .v.RH = r ; 
	mem [ s ] .hh .v.RH = q ; 
      } 
    } 
    s = q ; 
    q = mem [ q ] .hh .v.RH ; 
  } 
  flushnodelist ( p ) ; 
  popalignment () ; 
  auxsave = curlist .auxfield ; 
  p = mem [ curlist .headfield ] .hh .v.RH ; 
  q = curlist .tailfield ; 
  popnest () ; 
  if ( curlist .modefield == 203 ) 
  {
    doassignments () ; 
    if ( curcmd != 3 ) 
    {
      {
	if ( interaction == 3 ) 
	; 
	printnl ( 262 ) ; 
	print ( 1163 ) ; 
      } 
      {
	helpptr = 2 ; 
	helpline [ 1 ] = 888 ; 
	helpline [ 0 ] = 889 ; 
      } 
      backerror () ; 
    } 
    else {
	
      getxtoken () ; 
      if ( curcmd != 3 ) 
      {
	{
	  if ( interaction == 3 ) 
	  ; 
	  printnl ( 262 ) ; 
	  print ( 1159 ) ; 
	} 
	{
	  helpptr = 2 ; 
	  helpline [ 1 ] = 1160 ; 
	  helpline [ 0 ] = 1161 ; 
	} 
	backerror () ; 
      } 
    } 
    popnest () ; 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( eqtb [ 12674 ] .cint 
      ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newparamglue ( 3 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    if ( p != 0 ) 
    curlist .tailfield = q ; 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newpenalty ( eqtb [ 12675 ] .cint 
      ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    {
      mem [ curlist .tailfield ] .hh .v.RH = newparamglue ( 4 ) ; 
      curlist .tailfield = mem [ curlist .tailfield ] .hh .v.RH ; 
    } 
    curlist .auxfield .cint = auxsave .cint ; 
    resumeafterdisplay () ; 
  } 
  else {
      
    curlist .auxfield = auxsave ; 
    mem [ curlist .tailfield ] .hh .v.RH = p ; 
    if ( p != 0 ) 
    curlist .tailfield = q ; 
    if ( curlist .modefield == 1 ) 
    buildpage () ; 
  } 
} 
void alignpeek ( ) 
{/* 20 */ alignpeek_regmem 
  lab20: alignstate = 1000000L ; 
  do {
      getxtoken () ; 
  } while ( ! ( curcmd != 10 ) ) ; 
  if ( curcmd == 34 ) 
  {
    scanleftbrace () ; 
    newsavelevel ( 7 ) ; 
    if ( curlist .modefield == -1 ) 
    normalparagraph () ; 
  } 
  else if ( curcmd == 2 ) 
  finalign () ; 
  else if ( ( curcmd == 5 ) && ( curchr == 258 ) ) 
  goto lab20 ; 
  else {
      
    initrow () ; 
    initcol () ; 
  } 
} 
halfword zfiniteshrink ( p ) 
halfword p ; 
{register halfword Result; finiteshrink_regmem 
  halfword q  ; 
  if ( noshrinkerroryet ) 
  {
    noshrinkerroryet = false ; 
    {
      if ( interaction == 3 ) 
      ; 
      printnl ( 262 ) ; 
      print ( 910 ) ; 
    } 
    {
      helpptr = 5 ; 
      helpline [ 4 ] = 911 ; 
      helpline [ 3 ] = 912 ; 
      helpline [ 2 ] = 913 ; 
      helpline [ 1 ] = 914 ; 
      helpline [ 0 ] = 915 ; 
    } 
    error () ; 
  } 
  q = newspec ( p ) ; 
  mem [ q ] .hh.b1 = 0 ; 
  deleteglueref ( p ) ; 
  Result = q ; 
  return(Result) ; 
} 
void ztrybreak ( pi , breaktype ) 
integer pi ; 
smallnumber breaktype ; 
{/* 10 30 31 22 60 */ trybreak_regmem 
  halfword r  ; 
  halfword prevr  ; 
  halfword oldl  ; 
  boolean nobreakyet  ; 
  halfword prevprevr  ; 
  halfword s  ; 
  halfword q  ; 
  halfword v  ; 
  integer t  ; 
  internalfontnumber f  ; 
  halfword l  ; 
  boolean noderstaysactive  ; 
  scaled linewidth  ; 
  schar fitclass  ; 
  halfword b  ; 
  integer d  ; 
  boolean artificialdemerits  ; 
  halfword savelink  ; 
  scaled shortfall  ; 
  if ( abs ( pi ) >= 10000 ) 
  if ( pi > 0 ) 
  goto lab10 ; 
  else pi = -10000 ; 
  nobreakyet = true ; 
  prevr = memtop - 7 ; 
  oldl = 0 ; 
  curactivewidth [ 1 ] = activewidth [ 1 ] ; 
  curactivewidth [ 2 ] = activewidth [ 2 ] ; 
  curactivewidth [ 3 ] = activewidth [ 3 ] ; 
  curactivewidth [ 4 ] = activewidth [ 4 ] ; 
  curactivewidth [ 5 ] = activewidth [ 5 ] ; 
  curactivewidth [ 6 ] = activewidth [ 6 ] ; 
  while ( true ) {
      
    lab22: r = mem [ prevr ] .hh .v.RH ; 
    if ( mem [ r ] .hh.b0 == 2 ) 
    {
      curactivewidth [ 1 ] = curactivewidth [ 1 ] + mem [ r + 1 ] .cint ; 
      curactivewidth [ 2 ] = curactivewidth [ 2 ] + mem [ r + 2 ] .cint ; 
      curactivewidth [ 3 ] = curactivewidth [ 3 ] + mem [ r + 3 ] .cint ; 
      curactivewidth [ 4 ] = curactivewidth [ 4 ] + mem [ r + 4 ] .cint ; 
      curactivewidth [ 5 ] = curactivewidth [ 5 ] + mem [ r + 5 ] .cint ; 
      curactivewidth [ 6 ] = curactivewidth [ 6 ] + mem [ r + 6 ] .cint ; 
      prevprevr = prevr ; 
      prevr = r ; 
      goto lab22 ; 
    } 
    {
      l = mem [ r + 1 ] .hh .v.LH ; 
      if ( l > oldl ) 
      {
	if ( ( minimumdemerits < 1073741823L ) && ( ( oldl != easyline ) || ( 
	r == memtop - 7 ) ) ) 
	{
	  if ( nobreakyet ) 
	  {
	    nobreakyet = false ; 
	    breakwidth [ 1 ] = background [ 1 ] ; 
	    breakwidth [ 2 ] = background [ 2 ] ; 
	    breakwidth [ 3 ] = background [ 3 ] ; 
	    breakwidth [ 4 ] = background [ 4 ] ; 
	    breakwidth [ 5 ] = background [ 5 ] ; 
	    breakwidth [ 6 ] = background [ 6 ] ; 
	    s = curp ; 
	    if ( breaktype > 0 ) 
	    if ( curp != 0 ) 
	    {
	      t = mem [ curp ] .hh.b1 ; 
	      v = curp ; 
	      s = mem [ curp + 1 ] .hh .v.RH ; 
	      while ( t > 0 ) {
		  
		decr ( t ) ; 
		v = mem [ v ] .hh .v.RH ; 
		if ( ( v >= himemmin ) ) 
		{
		  f = mem [ v ] .hh.b0 ; 
		  breakwidth [ 1 ] = breakwidth [ 1 ] - fontinfo [ widthbase [ 
		  f ] + fontinfo [ charbase [ f ] + mem [ v ] .hh.b1 ] .qqqq 
		  .b0 ] .cint ; 
		} 
		else switch ( mem [ v ] .hh.b0 ) 
		{case 6 : 
		  {
		    f = mem [ v + 1 ] .hh.b0 ; 
		    breakwidth [ 1 ] = breakwidth [ 1 ] - fontinfo [ widthbase 
		    [ f ] + fontinfo [ charbase [ f ] + mem [ v + 1 ] .hh.b1 ] 
		    .qqqq .b0 ] .cint ; 
		  } 
		  break ; 
		case 0 : 
		case 1 : 
		case 2 : 
		case 11 : 
		  breakwidth [ 1 ] = breakwidth [ 1 ] - mem [ v + 1 ] .cint ; 
		  break ; 
		  default: 
		  confusion ( 916 ) ; 
		  break ; 
		} 
	      } 
	      while ( s != 0 ) {
		  
		if ( ( s >= himemmin ) ) 
		{
		  f = mem [ s ] .hh.b0 ; 
		  breakwidth [ 1 ] = breakwidth [ 1 ] + fontinfo [ widthbase [ 
		  f ] + fontinfo [ charbase [ f ] + mem [ s ] .hh.b1 ] .qqqq 
		  .b0 ] .cint ; 
		} 
		else switch ( mem [ s ] .hh.b0 ) 
		{case 6 : 
		  {
		    f = mem [ s + 1 ] .hh.b0 ; 
		    breakwidth [ 1 ] = breakwidth [ 1 ] + fontinfo [ widthbase 
		    [ f ] + fontinfo [ charbase [ f ] + mem [ s + 1 ] .hh.b1 ] 
		    .qqqq .b0 ] .cint ; 
		  } 
		  break ; 
		case 0 : 
		case 1 : 
		case 2 : 
		  breakwidth [ 1 ] = breakwidth [ 1 ] + mem [ s + 1 ] .cint ; 
		  break ; 
		case 11 : 
		  if ( ( t == 0 ) && ( mem [ s ] .hh.b1 != 2 ) ) 
		  t = -1 ; 
		  else breakwidth [ 1 ] = breakwidth [ 1 ] + mem [ s + 1 ] 
		  .cint ; 
		  break ; 
		  default: 
		  confusion ( 917 ) ; 
		  break ; 
		} 
		incr ( t ) ; 
		s = mem [ s ] .hh .v.RH ; 
	      } 
	      breakwidth [ 1 ] = breakwidth [ 1 ] + discwidth ; 
	      if ( t == 0 ) 
	      s = mem [ v ] .hh .v.RH ; 
	    } 
	    while ( s != 0 ) {
		
	      if ( ( s >= himemmin ) ) 
	      goto lab30 ; 
	      switch ( mem [ s ] .hh.b0 ) 
	      {case 10 : 
		{
		  v = mem [ s + 1 ] .hh .v.LH ; 
		  breakwidth [ 1 ] = breakwidth [ 1 ] - mem [ v + 1 ] .cint ; 
		  breakwidth [ 2 + mem [ v ] .hh.b0 ] = breakwidth [ 2 + mem [ 
		  v ] .hh.b0 ] - mem [ v + 2 ] .cint ; 
		  breakwidth [ 6 ] = breakwidth [ 6 ] - mem [ v + 3 ] .cint ; 
		} 
		break ; 
	      case 12 : 
		; 
		break ; 
	      case 9 : 
	      case 11 : 
		if ( mem [ s ] .hh.b1 == 2 ) 
		goto lab30 ; 
		else breakwidth [ 1 ] = breakwidth [ 1 ] - mem [ s + 1 ] .cint 
		; 
		break ; 
		default: 
		goto lab30 ; 
		break ; 
	      } 
	      s = mem [ s ] .hh .v.RH ; 
	    } 
	    lab30: ; 
	  } 
	  if ( mem [ prevr ] .hh.b0 == 2 ) 
	  {
	    mem [ prevr + 1 ] .cint = mem [ prevr + 1 ] .cint - curactivewidth 
	    [ 1 ] + breakwidth [ 1 ] ; 
	    mem [ prevr + 2 ] .cint = mem [ prevr + 2 ] .cint - curactivewidth 
	    [ 2 ] + breakwidth [ 2 ] ; 
	    mem [ prevr + 3 ] .cint = mem [ prevr + 3 ] .cint - curactivewidth 
	    [ 3 ] + breakwidth [ 3 ] ; 
	    mem [ prevr + 4 ] .cint = mem [ prevr + 4 ] .cint - curactivewidth 
	    [ 4 ] + breakwidth [ 4 ] ; 
	    mem [ prevr + 5 ] .cint = mem [ prevr + 5 ] .cint - curactivewidth 
	    [ 5 ] + breakwidth [ 5 ] ; 
	    mem [ prevr + 6 ] .cint = mem [ prevr + 6 ] .cint - curactivewidth 
	    [ 6 ] + breakwidth [ 6 ] ; 
	  } 
	  else if ( prevr == memtop - 7 ) 
	  {
	    activewidth [ 1 ] = breakwidth [ 1 ] ; 
	    activewidth [ 2 ] = breakwidth [ 2 ] ; 
	    activewidth [ 3 ] = breakwidth [ 3 ] ; 
	    activewidth [ 4 ] = breakwidth [ 4 ] ; 
	    activewidth [ 5 ] = breakwidth [ 5 ] ; 
	    activewidth [ 6 ] = breakwidth [ 6 ] ; 
	  } 
	  else {
	      
	    q = getnode ( 7 ) ; 
	    mem [ q ] .hh .v.RH = r ; 
	    mem [ q ] .hh.b0 = 2 ; 
	    mem [ q ] .hh.b1 = 0 ; 
	    mem [ q + 1 ] .cint = breakwidth [ 1 ] - curactivewidth [ 1 ] ; 
	    mem [ q + 2 ] .cint = breakwidth [ 2 ] - curactivewidth [ 2 ] ; 
	    mem [ q + 3 ] .cint = breakwidth [ 3 ] - curactivewidth [ 3 ] ; 
	    mem [ q + 4 ] .cint = breakwidth [ 4 ] - curactivewidth [ 4 ] ; 
	    mem [ q + 5 ] .cint = breakwidth [ 5 ] - curactivewidth [ 5 ] ; 
	    mem [ q + 6 ] .cint = breakwidth [ 6 ] - curactivewidth [ 6 ] ; 
	    mem [ prevr ] .hh .v.RH = q ; 
	    prevprevr = prevr ; 
	    prevr = q ; 
	  } 
	  if ( abs ( eqtb [ 12679 ] .cint ) >= 1073741823L - minimumdemerits ) 
	  minimumdemerits = 1073741822L ; 
	  else minimumdemerits = minimumdemerits + abs ( eqtb [ 12679 ] .cint 
	  ) ; 
	  {register integer for_end; fitclass = 0 ; for_end = 3 ; if ( 
	  fitclass <= for_end) do 
	    {
	      if ( minimaldemerits [ fitclass ] <= minimumdemerits ) 
	      {
		q = getnode ( 2 ) ; 
		mem [ q ] .hh .v.RH = passive ; 
		passive = q ; 
		mem [ q + 1 ] .hh .v.RH = curp ; 
	;
#ifdef STAT
		incr ( passnumber ) ; 
		mem [ q ] .hh .v.LH = passnumber ; 
#endif /* STAT */
		mem [ q + 1 ] .hh .v.LH = bestplace [ fitclass ] ; 
		q = getnode ( 3 ) ; 
		mem [ q + 1 ] .hh .v.RH = passive ; 
		mem [ q + 1 ] .hh .v.LH = bestplline [ fitclass ] + 1 ; 
		mem [ q ] .hh.b1 = fitclass ; 
		mem [ q ] .hh.b0 = breaktype ; 
		mem [ q + 2 ] .cint = minimaldemerits [ fitclass ] ; 
		mem [ q ] .hh .v.RH = r ; 
		mem [ prevr ] .hh .v.RH = q ; 
		prevr = q ; 
	;
#ifdef STAT
		if ( eqtb [ 12695 ] .cint > 0 ) 
		{
		  printnl ( 918 ) ; 
		  printint ( mem [ passive ] .hh .v.LH ) ; 
		  print ( 919 ) ; 
		  printint ( mem [ q + 1 ] .hh .v.LH - 1 ) ; 
		  printchar ( 46 ) ; 
		  printint ( fitclass ) ; 
		  if ( breaktype == 1 ) 
		  printchar ( 45 ) ; 
		  print ( 920 ) ; 
		  printint ( mem [ q + 2 ] .cint ) ; 
		  print ( 921 ) ; 
		  if ( mem [ passive + 1 ] .hh .v.LH == 0 ) 
		  printchar ( 48 ) ; 
		  else printint ( mem [ mem [ passive + 1 ] .hh .v.LH ] .hh 
		  .v.LH ) ; 
		} 
#endif /* STAT */
	      } 
	      minimaldemerits [ fitclass ] = 1073741823L ; 
	    } 
	  while ( fitclass++ < for_end ) ; } 
	  minimumdemerits = 1073741823L ; 
	  if ( r != memtop - 7 ) 
	  {
	    q = getnode ( 7 ) ; 
	    mem [ q ] .hh .v.RH = r ; 
	    mem [ q ] .hh.b0 = 2 ; 
	    mem [ q ] .hh.b1 = 0 ; 
	    mem [ q + 1 ] .cint = curactivewidth [ 1 ] - breakwidth [ 1 ] ; 
	    mem [ q + 2 ] .cint = curactivewidth [ 2 ] - breakwidth [ 2 ] ; 
	    mem [ q + 3 ] .cint = curactivewidth [ 3 ] - breakwidth [ 3 ] ; 
	    mem [ q + 4 ] .cint = curactivewidth [ 4 ] - breakwidth [ 4 ] ; 
	    mem [ q + 5 ] .cint = curactivewidth [ 5 ] - breakwidth [ 5 ] ; 
	    mem [ q + 6 ] .cint = curactivewidth [ 6 ] - breakwidth [ 6 ] ; 
	    mem [ prevr ] .hh .v.RH = q ; 
	    prevprevr = prevr ; 
	    prevr = q ; 
	  } 
	} 
	if ( r == memtop - 7 ) 
	goto lab10 ; 
	if ( l > easyline ) 
	{
	  linewidth = secondwidth ; 
	  oldl = 262142L ; 
	} 
	else {
	    
	  oldl = l ; 
	  if ( l > lastspecialline ) 
	  linewidth = secondwidth ; 
	  else if ( eqtb [ 10812 ] .hh .v.RH == 0 ) 
	  linewidth = firstwidth ; 
	  else linewidth = mem [ eqtb [ 10812 ] .hh .v.RH + 2 * l ] .cint ; 
	} 
      } 
    } 
    {
      artificialdemerits = false ; 
      shortfall = linewidth - curactivewidth [ 1 ] ; 
      if ( shortfall > 0 ) 
      if ( ( curactivewidth [ 3 ] != 0 ) || ( curactivewidth [ 4 ] != 0 ) || ( 
      curactivewidth [ 5 ] != 0 ) ) 
      {
	b = 0 ; 
	fitclass = 2 ; 
      } 
      else {
	  
	if ( shortfall > 7230584L ) 
	if ( curactivewidth [ 2 ] < 1663497L ) 
	{
	  b = 10000 ; 
	  fitclass = 0 ; 
	  goto lab31 ; 
	} 
	b = badness ( shortfall , curactivewidth [ 2 ] ) ; 
	if ( b > 12 ) 
	if ( b > 99 ) 
	fitclass = 0 ; 
	else fitclass = 1 ; 
	else fitclass = 2 ; 
	lab31: ; 
      } 
      else {
	  
	if ( - (integer) shortfall > curactivewidth [ 6 ] ) 
	b = 10001 ; 
	else b = badness ( - (integer) shortfall , curactivewidth [ 6 ] ) ; 
	if ( b > 12 ) 
	fitclass = 3 ; 
	else fitclass = 2 ; 
      } 
      if ( ( b > 10000 ) || ( pi == -10000 ) ) 
      {
	if ( finalpass && ( minimumdemerits == 1073741823L ) && ( mem [ r ] 
	.hh .v.RH == memtop - 7 ) && ( prevr == memtop - 7 ) ) 
	artificialdemerits = true ; 
	else if ( b > threshold ) 
	goto lab60 ; 
	noderstaysactive = false ; 
      } 
      else {
	  
	prevr = r ; 
	if ( b > threshold ) 
	goto lab22 ; 
	noderstaysactive = true ; 
      } 
      if ( artificialdemerits ) 
      d = 0 ; 
      else {
	  
	d = eqtb [ 12665 ] .cint + b ; 
	if ( abs ( d ) >= 10000 ) 
	d = 100000000L ; 
	else d = d * d ; 
	if ( pi != 0 ) 
	if ( pi > 0 ) 
	d = d + pi * pi ; 
	else if ( pi > -10000 ) 
	d = d - pi * pi ; 
	if ( ( breaktype == 1 ) && ( mem [ r ] .hh.b0 == 1 ) ) 
	if ( curp != 0 ) 
	d = d + eqtb [ 12677 ] .cint ; 
	else d = d + eqtb [ 12678 ] .cint ; 
	if ( abs ( toint ( fitclass ) - toint ( mem [ r ] .hh.b1 ) ) > 1 ) 
	d = d + eqtb [ 12679 ] .cint ; 
      } 
	;
#ifdef STAT
      if ( eqtb [ 12695 ] .cint > 0 ) 
      {
	if ( printednode != curp ) 
	{
	  printnl ( 335 ) ; 
	  if ( curp == 0 ) 
	  shortdisplay ( mem [ printednode ] .hh .v.RH ) ; 
	  else {
	      
	    savelink = mem [ curp ] .hh .v.RH ; 
	    mem [ curp ] .hh .v.RH = 0 ; 
	    printnl ( 335 ) ; 
	    shortdisplay ( mem [ printednode ] .hh .v.RH ) ; 
	    mem [ curp ] .hh .v.RH = savelink ; 
	  } 
	  printednode = curp ; 
	} 
	printnl ( 64 ) ; 
	if ( curp == 0 ) 
	printesc ( 593 ) ; 
	else if ( mem [ curp ] .hh.b0 != 10 ) 
	{
	  if ( mem [ curp ] .hh.b0 == 12 ) 
	  printesc ( 527 ) ; 
	  else if ( mem [ curp ] .hh.b0 == 7 ) 
	  printesc ( 346 ) ; 
	  else if ( mem [ curp ] .hh.b0 == 11 ) 
	  printesc ( 337 ) ; 
	  else printesc ( 340 ) ; 
	} 
	print ( 922 ) ; 
	if ( mem [ r + 1 ] .hh .v.RH == 0 ) 
	printchar ( 48 ) ; 
	else printint ( mem [ mem [ r + 1 ] .hh .v.RH ] .hh .v.LH ) ; 
	print ( 923 ) ; 
	if ( b > 10000 ) 
	printchar ( 42 ) ; 
	else printint ( b ) ; 
	print ( 924 ) ; 
	printint ( pi ) ; 
	print ( 925 ) ; 
	if ( artificialdemerits ) 
	printchar ( 42 ) ; 
	else printint ( d ) ; 
      } 
#endif /* STAT */
      d = d + mem [ r + 2 ] .cint ; 
      if ( d <= minimaldemerits [ fitclass ] ) 
      {
	minimaldemerits [ fitclass ] = d ; 
	bestplace [ fitclass ] = mem [ r + 1 ] .hh .v.RH ; 
	bestplline [ fitclass ] = l ; 
	if ( d < minimumdemerits ) 
	minimumdemerits = d ; 
      } 
      if ( noderstaysactive ) 
      goto lab22 ; 
      lab60: mem [ prevr ] .hh .v.RH = mem [ r ] .hh .v.RH ; 
      freenode ( r , 3 ) ; 
      if ( prevr == memtop - 7 ) 
      {
	r = mem [ memtop - 7 ] .hh .v.RH ; 
	if ( mem [ r ] .hh.b0 == 2 ) 
	{
	  activewidth [ 1 ] = activewidth [ 1 ] + mem [ r + 1 ] .cint ; 
	  activewidth [ 2 ] = activewidth [ 2 ] + mem [ r + 2 ] .cint ; 
	  activewidth [ 3 ] = activewidth [ 3 ] + mem [ r + 3 ] .cint ; 
	  activewidth [ 4 ] = activewidth [ 4 ] + mem [ r + 4 ] .cint ; 
	  activewidth [ 5 ] = activewidth [ 5 ] + mem [ r + 5 ] .cint ; 
	  activewidth [ 6 ] = activewidth [ 6 ] + mem [ r + 6 ] .cint ; 
	  curactivewidth [ 1 ] = activewidth [ 1 ] ; 
	  curactivewidth [ 2 ] = activewidth [ 2 ] ; 
	  curactivewidth [ 3 ] = activewidth [ 3 ] ; 
	  curactivewidth [ 4 ] = activewidth [ 4 ] ; 
	  curactivewidth [ 5 ] = activewidth [ 5 ] ; 
	  curactivewidth [ 6 ] = activewidth [ 6 ] ; 
	  mem [ memtop - 7 ] .hh .v.RH = mem [ r ] .hh .v.RH ; 
	  freenode ( r , 7 ) ; 
	} 
      } 
      else if ( mem [ prevr ] .hh.b0 == 2 ) 
      {
	r = mem [ prevr ] .hh .v.RH ; 
	if ( r == memtop - 7 ) 
	{
	  curactivewidth [ 1 ] = curactivewidth [ 1 ] - mem [ prevr + 1 ] 
	  .cint ; 
	  curactivewidth [ 2 ] = curactivewidth [ 2 ] - mem [ prevr + 2 ] 
	  .cint ; 
	  curactivewidth [ 3 ] = curactivewidth [ 3 ] - mem [ prevr + 3 ] 
	  .cint ; 
	  curactivewidth [ 4 ] = curactivewidth [ 4 ] - mem [ prevr + 4 ] 
	  .cint ; 
	  curactivewidth [ 5 ] = curactivewidth [ 5 ] - mem [ prevr + 5 ] 
	  .cint ; 
	  curactivewidth [ 6 ] = curactivewidth [ 6 ] - mem [ prevr + 6 ] 
	  .cint ; 
	  mem [ prevprevr ] .hh .v.RH = memtop - 7 ; 
	  freenode ( prevr , 7 ) ; 
	  prevr = prevprevr ; 
	} 
	else if ( mem [ r ] .hh.b0 == 2 ) 
	{
	  curactivewidth [ 1 ] = curactivewidth [ 1 ] + mem [ r + 1 ] .cint ; 
	  curactivewidth [ 2 ] = curactivewidth [ 2 ] + mem [ r + 2 ] .cint ; 
	  curactivewidth [ 3 ] = curactivewidth [ 3 ] + mem [ r + 3 ] .cint ; 
	  curactivewidth [ 4 ] = curactivewidth [ 4 ] + mem [ r + 4 ] .cint ; 
	  curactivewidth [ 5 ] = curactivewidth [ 5 ] + mem [ r + 5 ] .cint ; 
	  curactivewidth [ 6 ] = curactivewidth [ 6 ] + mem [ r + 6 ] .cint ; 
	  mem [ prevr + 1 ] .cint = mem [ prevr + 1 ] .cint + mem [ r + 1 ] 
	  .cint ; 
	  mem [ prevr + 2 ] .cint = mem [ prevr + 2 ] .cint + mem [ r + 2 ] 
	  .cint ; 
	  mem [ prevr + 3 ] .cint = mem [ prevr + 3 ] .cint + mem [ r + 3 ] 
	  .cint ; 
	  mem [ prevr + 4 ] .cint = mem [ prevr + 4 ] .cint + mem [ r + 4 ] 
	  .cint ; 
	  mem [ prevr + 5 ] .cint = mem [ prevr + 5 ] .cint + mem [ r + 5 ] 
	  .cint ; 
	  mem [ prevr + 6 ] .cint = mem [ prevr + 6 ] .cint + mem [ r + 6 ] 
	  .cint ; 
	  mem [ prevr ] .hh .v.RH = mem [ r ] .hh .v.RH ; 
	  freenode ( r , 7 ) ; 
	} 
      } 
    } 
  } 
  lab10: 
	;
#ifdef STAT
  if ( curp == printednode ) 
  if ( curp != 0 ) 
  if ( mem [ curp ] .hh.b0 == 7 ) 
  {
    t = mem [ curp ] .hh.b1 ; 
    while ( t > 0 ) {
	
      decr ( t ) ; 
      printednode = mem [ printednode ] .hh .v.RH ; 
    } 
  } 
#endif /* STAT */
} 
