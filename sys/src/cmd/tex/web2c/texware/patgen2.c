#include "config.h"
/* 9999 */ 
#define triesize 55000L 
#define triecsize 26000 
#define maxops 4080 
#define maxval 10 
#define maxdot 15 
#define maxlen 50 
#define maxbuflen 80 
typedef unsigned char ASCIIcode  ; 
typedef ASCIIcode textchar  ; 
typedef text textfile  ; 
typedef unsigned char packedASCIIcode  ; 
typedef ASCIIcode internalcode  ; 
typedef packedASCIIcode packedinternalcode  ; 
typedef schar classtype  ; 
typedef schar digit  ; 
typedef schar hyftype  ; 
typedef unsigned char qindex  ; 
typedef integer valtype  ; 
typedef integer dottype  ; 
typedef integer optype  ; 
typedef integer wordindex  ; 
typedef integer triepointer  ; 
typedef integer triecpointer  ; 
typedef struct {
    dottype dot ; 
  valtype val ; 
  optype op ; 
} opword  ; 
dottype patstart, patfinish  ; 
valtype hyphstart, hyphfinish  ; 
integer goodwt, badwt, thresh  ; 
ASCIIcode xord[256]  ; 
textchar xchr[256]  ; 
classtype xclass[256]  ; 
internalcode xint[256]  ; 
textchar xdig[10]  ; 
textchar xext[256]  ; 
textchar xhyf[4]  ; 
internalcode cmax  ; 
packedinternalcode triec[triesize + 1]  ; 
triepointer triel[triesize + 1], trier[triesize + 1]  ; 
boolean trietaken[triesize + 1]  ; 
packedinternalcode triecc[triecsize + 1]  ; 
triecpointer triecl[triecsize + 1], triecr[triecsize + 1]  ; 
boolean triectaken[triecsize + 1]  ; 
opword ops[maxops + 1]  ; 
internalcode trieqc[256]  ; 
triepointer trieql[256], trieqr[256]  ; 
qindex qmax  ; 
qindex qmaxthresh  ; 
triepointer triemax  ; 
triepointer triebmax  ; 
triepointer triecount  ; 
optype opcount  ; 
internalcode pat[maxdot + 1]  ; 
dottype patlen  ; 
triecpointer triecmax, triecbmax, trieccount  ; 
triecpointer trieckmax  ; 
integer patcount  ; 
textfile dictionary, patterns, translate, patout, pattmp  ; 
char fname[PATHMAX + 1]  ; 
textchar buf[maxbuflen + 1]  ; 
integer bufptr  ; 
internalcode imax  ; 
dottype lefthyphenmin, righthyphenmin  ; 
integer goodpatcount, badpatcount  ; 
integer goodcount, badcount, misscount  ; 
integer levelpatterncount  ; 
boolean moretocome  ; 
internalcode word[maxlen + 1]  ; 
hyftype dots[maxlen + 1]  ; 
digit dotw[maxlen + 1]  ; 
valtype hval[maxlen + 1]  ; 
boolean nomore[maxlen + 1]  ; 
wordindex wlen  ; 
digit wordwt  ; 
boolean wtchg  ; 
wordindex hyfmin, hyfmax, hyflen  ; 
hyftype gooddot, baddot  ; 
wordindex dotmin, dotmax, dotlen  ; 
boolean procesp, hyphp  ; 
dottype patdot  ; 
valtype hyphlevel  ; 
char filnam[9]  ; 
valtype maxpat  ; 
integer n1, n2  ; 
valtype i  ; 
dottype j  ; 
dottype k  ; 
dottype dot1  ; 
boolean morethislevel[maxdot + 1]  ; 

#include "patgen2.h"
void initialize ( ) 
{integer bad  ; 
  textchar i  ; 
  ASCIIcode j  ; 
  if ( argc < 4 ) 
  {
    {
      (void) fprintf( stdout , "%s%s\n",        "Usage: patgen <dictionary file> <pattern file> <output file>" ,       " <translate file>" ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  (void) fprintf( stdout , "%s\n",  "This is PATGEN, C Version 2.0" ) ; 
  bad = 0 ; 
  if ( 255 < 127 ) 
  bad = 1 ; 
  if ( ( 0 != 0 ) || ( 0 != 0 ) ) 
  bad = 2 ; 
  if ( ( triecsize < 4096 ) || ( triesize < triecsize ) ) 
  bad = 3 ; 
  if ( maxops > triesize ) 
  bad = 4 ; 
  if ( maxval > 10 ) 
  bad = 5 ; 
  if ( maxbuflen < maxlen ) 
  bad = 6 ; 
  if ( bad > 0 ) 
  {
    (void) fprintf( stdout , "%s%ld\n",  "Bad constants---case " , (long)bad ) ; 
    uexit ( 1 ) ; 
  } 
  {register integer for_end; j = 0 ; for_end = 255 ; if ( j <= for_end) do 
    xchr [ j ] = ' ' ; 
  while ( j++ < for_end ) ; } 
  xchr [ 46 ] = '.' ; 
  xchr [ 48 ] = '0' ; 
  xchr [ 49 ] = '1' ; 
  xchr [ 50 ] = '2' ; 
  xchr [ 51 ] = '3' ; 
  xchr [ 52 ] = '4' ; 
  xchr [ 53 ] = '5' ; 
  xchr [ 54 ] = '6' ; 
  xchr [ 55 ] = '7' ; 
  xchr [ 56 ] = '8' ; 
  xchr [ 57 ] = '9' ; 
  xchr [ 65 ] = 'A' ; 
  xchr [ 66 ] = 'B' ; 
  xchr [ 67 ] = 'C' ; 
  xchr [ 68 ] = 'D' ; 
  xchr [ 69 ] = 'E' ; 
  xchr [ 70 ] = 'F' ; 
  xchr [ 71 ] = 'G' ; 
  xchr [ 72 ] = 'H' ; 
  xchr [ 73 ] = 'I' ; 
  xchr [ 74 ] = 'J' ; 
  xchr [ 75 ] = 'K' ; 
  xchr [ 76 ] = 'L' ; 
  xchr [ 77 ] = 'M' ; 
  xchr [ 78 ] = 'N' ; 
  xchr [ 79 ] = 'O' ; 
  xchr [ 80 ] = 'P' ; 
  xchr [ 81 ] = 'Q' ; 
  xchr [ 82 ] = 'R' ; 
  xchr [ 83 ] = 'S' ; 
  xchr [ 84 ] = 'T' ; 
  xchr [ 85 ] = 'U' ; 
  xchr [ 86 ] = 'V' ; 
  xchr [ 87 ] = 'W' ; 
  xchr [ 88 ] = 'X' ; 
  xchr [ 89 ] = 'Y' ; 
  xchr [ 90 ] = 'Z' ; 
  xchr [ 97 ] = 'a' ; 
  xchr [ 98 ] = 'b' ; 
  xchr [ 99 ] = 'c' ; 
  xchr [ 100 ] = 'd' ; 
  xchr [ 101 ] = 'e' ; 
  xchr [ 102 ] = 'f' ; 
  xchr [ 103 ] = 'g' ; 
  xchr [ 104 ] = 'h' ; 
  xchr [ 105 ] = 'i' ; 
  xchr [ 106 ] = 'j' ; 
  xchr [ 107 ] = 'k' ; 
  xchr [ 108 ] = 'l' ; 
  xchr [ 109 ] = 'm' ; 
  xchr [ 110 ] = 'n' ; 
  xchr [ 111 ] = 'o' ; 
  xchr [ 112 ] = 'p' ; 
  xchr [ 113 ] = 'q' ; 
  xchr [ 114 ] = 'r' ; 
  xchr [ 115 ] = 's' ; 
  xchr [ 116 ] = 't' ; 
  xchr [ 117 ] = 'u' ; 
  xchr [ 118 ] = 'v' ; 
  xchr [ 119 ] = 'w' ; 
  xchr [ 120 ] = 'x' ; 
  xchr [ 121 ] = 'y' ; 
  xchr [ 122 ] = 'z' ; 
  {register integer for_end; i = chr ( 0 ) ; for_end = chr ( 255 ) ; if ( i 
  <= for_end) do 
    xord [ i ] = 0 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; j = 0 ; for_end = 255 ; if ( j <= for_end) do 
    xord [ xchr [ j ] ] = j ; 
  while ( j++ < for_end ) ; } 
  xord [ ' ' ] = 32 ; 
  xord [ chr ( 9 ) ] = 32 ; 
  {register integer for_end; i = chr ( 0 ) ; for_end = chr ( 255 ) ; if ( i 
  <= for_end) do 
    {
      xclass [ i ] = 5 ; 
      xint [ i ] = 0 ; 
    } 
  while ( i++ < for_end ) ; } 
  xclass [ ' ' ] = 0 ; 
  {register integer for_end; j = 0 ; for_end = 255 ; if ( j <= for_end) do 
    xext [ j ] = ' ' ; 
  while ( j++ < for_end ) ; } 
  xext [ 1 ] = '.' ; 
  {register integer for_end; j = 0 ; for_end = 9 ; if ( j <= for_end) do 
    {
      xdig [ j ] = xchr [ j + 48 ] ; 
      xclass [ xdig [ j ] ] = 1 ; 
      xint [ xdig [ j ] ] = j ; 
    } 
  while ( j++ < for_end ) ; } 
  xhyf [ 1 ] = '.' ; 
  xhyf [ 2 ] = '-' ; 
  xhyf [ 3 ] = '*' ; 
} 
ASCIIcode zgetASCII ( c ) 
textchar c ; 
{/* 40 */ register ASCIIcode Result; ASCIIcode i  ; 
  i = xord [ c ] ; 
  if ( i == 0 ) 
  {
    while ( i < 255 ) {
	
      i = i + 1 ; 
      if ( ( xchr [ i ] == ' ' ) && ( i != 32 ) ) 
      goto lab40 ; 
    } 
    {
      (void) fprintf( stdout , "%s%ld%s%s\n",  "PATGEN capacity exceeded, sorry [" , (long)256 ,       " characters" , "]." ) ; 
      uexit ( 1 ) ; 
    } 
    lab40: xord [ c ] = i ; 
    xchr [ i ] = c ; 
  } 
  Result = i ; 
  return(Result) ; 
} 
void initpatterntrie ( ) 
{internalcode c  ; 
  optype h  ; 
  {register integer for_end; c = 0 ; for_end = 255 ; if ( c <= for_end) do 
    {
      triec [ 1 + c ] = c ; 
      triel [ 1 + c ] = 0 ; 
      trier [ 1 + c ] = 0 ; 
      trietaken [ 1 + c ] = false ; 
    } 
  while ( c++ < for_end ) ; } 
  trietaken [ 1 ] = true ; 
  triebmax = 1 ; 
  triemax = 256 ; 
  triecount = 256 ; 
  qmaxthresh = 5 ; 
  triel [ 0 ] = triemax + 1 ; 
  trier [ triemax + 1 ] = 0 ; 
  {register integer for_end; h = 1 ; for_end = maxops ; if ( h <= for_end) do 
    ops [ h ] .val = 0 ; 
  while ( h++ < for_end ) ; } 
  opcount = 0 ; 
} 
triepointer firstfit ( ) 
{/* 40 41 */ register triepointer Result; triepointer s, t  ; 
  qindex q  ; 
  if ( qmax > qmaxthresh ) 
  t = trier [ triemax + 1 ] ; 
  else t = 0 ; 
  while ( true ) {
      
    t = triel [ t ] ; 
    s = t - trieqc [ 1 ] ; 
    if ( s > triesize - 256 ) 
    {
      (void) fprintf( stdout , "%s%ld%s%s\n",  "PATGEN capacity exceeded, sorry [" , (long)triesize ,       " pattern trie nodes" , "]." ) ; 
      uexit ( 1 ) ; 
    } 
    while ( triebmax < s ) {
	
      triebmax = triebmax + 1 ; 
      trietaken [ triebmax ] = false ; 
      triec [ triebmax + 255 ] = 0 ; 
      triel [ triebmax + 255 ] = triebmax + 256 ; 
      trier [ triebmax + 256 ] = triebmax + 255 ; 
    } 
    if ( trietaken [ s ] ) 
    goto lab41 ; 
    {register integer for_end; q = qmax ; for_end = 2 ; if ( q >= for_end) do 
      if ( triec [ s + trieqc [ q ] ] != 0 ) 
      goto lab41 ; 
    while ( q-- > for_end ) ; } 
    goto lab40 ; 
    lab41: ; 
  } 
  lab40: ; 
  {register integer for_end; q = 1 ; for_end = qmax ; if ( q <= for_end) do 
    {
      t = s + trieqc [ q ] ; 
      triel [ trier [ t ] ] = triel [ t ] ; 
      trier [ triel [ t ] ] = trier [ t ] ; 
      triec [ t ] = trieqc [ q ] ; 
      triel [ t ] = trieql [ q ] ; 
      trier [ t ] = trieqr [ q ] ; 
      if ( t > triemax ) 
      triemax = t ; 
    } 
  while ( q++ < for_end ) ; } 
  trietaken [ s ] = true ; 
  Result = s ; 
  return(Result) ; 
} 
void zunpack ( s ) 
triepointer s ; 
{internalcode c  ; 
  triepointer t  ; 
  qmax = 1 ; 
  {register integer for_end; c = 1 ; for_end = cmax ; if ( c <= for_end) do 
    {
      t = s + c ; 
      if ( triec [ t ] == c ) 
      {
	trieqc [ qmax ] = c ; 
	trieql [ qmax ] = triel [ t ] ; 
	trieqr [ qmax ] = trier [ t ] ; 
	qmax = qmax + 1 ; 
	trier [ triel [ 0 ] ] = t ; 
	triel [ t ] = triel [ 0 ] ; 
	triel [ 0 ] = t ; 
	trier [ t ] = 0 ; 
	triec [ t ] = 0 ; 
      } 
    } 
  while ( c++ < for_end ) ; } 
  trietaken [ s ] = false ; 
} 
optype znewtrieop ( v , d , n ) 
valtype v ; 
dottype d ; 
optype n ; 
{/* 10 */ register optype Result; optype h  ; 
  h = ( ( n + 313 * d + 361 * v ) % maxops ) + 1 ; 
  while ( true ) {
      
    if ( ops [ h ] .val == 0 ) 
    {
      opcount = opcount + 1 ; 
      if ( opcount == maxops ) 
      {
	(void) fprintf( stdout , "%s%ld%s%s\n",  "PATGEN capacity exceeded, sorry [" , (long)maxops ,         " outputs" , "]." ) ; 
	uexit ( 1 ) ; 
      } 
      ops [ h ] .val = v ; 
      ops [ h ] .dot = d ; 
      ops [ h ] .op = n ; 
      Result = h ; 
      goto lab10 ; 
    } 
    if ( ( ops [ h ] .val == v ) && ( ops [ h ] .dot == d ) && ( ops [ h ] .op 
    == n ) ) 
    {
      Result = h ; 
      goto lab10 ; 
    } 
    if ( h > 1 ) 
    h = h - 1 ; 
    else h = maxops ; 
  } 
  lab10: ; 
  return(Result) ; 
} 
void zinsertpattern ( val , dot ) 
valtype val ; 
dottype dot ; 
{dottype i  ; 
  triepointer s, t  ; 
  i = 1 ; 
  s = 1 + pat [ i ] ; 
  t = triel [ s ] ; 
  while ( ( t > 0 ) && ( i < patlen ) ) {
      
    i = i + 1 ; 
    t = t + pat [ i ] ; 
    if ( triec [ t ] != pat [ i ] ) 
    {
      if ( triec [ t ] == 0 ) 
      {
	triel [ trier [ t ] ] = triel [ t ] ; 
	trier [ triel [ t ] ] = trier [ t ] ; 
	triec [ t ] = pat [ i ] ; 
	triel [ t ] = 0 ; 
	trier [ t ] = 0 ; 
	if ( t > triemax ) 
	triemax = t ; 
      } 
      else {
	  
	unpack ( t - pat [ i ] ) ; 
	trieqc [ qmax ] = pat [ i ] ; 
	trieql [ qmax ] = 0 ; 
	trieqr [ qmax ] = 0 ; 
	t = firstfit () ; 
	triel [ s ] = t ; 
	t = t + pat [ i ] ; 
      } 
      triecount = triecount + 1 ; 
    } 
    s = t ; 
    t = triel [ s ] ; 
  } 
  trieql [ 1 ] = 0 ; 
  trieqr [ 1 ] = 0 ; 
  qmax = 1 ; 
  while ( i < patlen ) {
      
    i = i + 1 ; 
    trieqc [ 1 ] = pat [ i ] ; 
    t = firstfit () ; 
    triel [ s ] = t ; 
    s = t + pat [ i ] ; 
    triecount = triecount + 1 ; 
  } 
  trier [ s ] = newtrieop ( val , dot , trier [ s ] ) ; 
} 
void initcounttrie ( ) 
{internalcode c  ; 
  {register integer for_end; c = 0 ; for_end = 255 ; if ( c <= for_end) do 
    {
      triecc [ 1 + c ] = c ; 
      triecl [ 1 + c ] = 0 ; 
      triecr [ 1 + c ] = 0 ; 
      triectaken [ 1 + c ] = false ; 
    } 
  while ( c++ < for_end ) ; } 
  triectaken [ 1 ] = true ; 
  triecbmax = 1 ; 
  triecmax = 256 ; 
  trieccount = 256 ; 
  trieckmax = 4096 ; 
  triecl [ 0 ] = triecmax + 1 ; 
  triecr [ triecmax + 1 ] = 0 ; 
  patcount = 0 ; 
} 
triecpointer firstcfit ( ) 
{/* 40 41 */ register triecpointer Result; triecpointer a, b  ; 
  qindex q  ; 
  if ( qmax > 3 ) 
  a = triecr [ triecmax + 1 ] ; 
  else a = 0 ; 
  while ( true ) {
      
    a = triecl [ a ] ; 
    b = a - trieqc [ 1 ] ; 
    if ( b > trieckmax - 256 ) 
    {
      if ( trieckmax == triecsize ) 
      {
	(void) fprintf( stdout , "%s%ld%s%s\n",  "PATGEN capacity exceeded, sorry [" , (long)triecsize ,         " count trie nodes" , "]." ) ; 
	uexit ( 1 ) ; 
      } 
      (void) fprintf( stdout , "%ld%s",  (long)trieckmax / 1024 , "K " ) ; 
      if ( trieckmax > triecsize - 4096 ) 
      trieckmax = triecsize ; 
      else trieckmax = trieckmax + 4096 ; 
    } 
    while ( triecbmax < b ) {
	
      triecbmax = triecbmax + 1 ; 
      triectaken [ triecbmax ] = false ; 
      triecc [ triecbmax + 255 ] = 0 ; 
      triecl [ triecbmax + 255 ] = triecbmax + 256 ; 
      triecr [ triecbmax + 256 ] = triecbmax + 255 ; 
    } 
    if ( triectaken [ b ] ) 
    goto lab41 ; 
    {register integer for_end; q = qmax ; for_end = 2 ; if ( q >= for_end) do 
      if ( triecc [ b + trieqc [ q ] ] != 0 ) 
      goto lab41 ; 
    while ( q-- > for_end ) ; } 
    goto lab40 ; 
    lab41: ; 
  } 
  lab40: ; 
  {register integer for_end; q = 1 ; for_end = qmax ; if ( q <= for_end) do 
    {
      a = b + trieqc [ q ] ; 
      triecl [ triecr [ a ] ] = triecl [ a ] ; 
      triecr [ triecl [ a ] ] = triecr [ a ] ; 
      triecc [ a ] = trieqc [ q ] ; 
      triecl [ a ] = trieql [ q ] ; 
      triecr [ a ] = trieqr [ q ] ; 
      if ( a > triecmax ) 
      triecmax = a ; 
    } 
  while ( q++ < for_end ) ; } 
  triectaken [ b ] = true ; 
  Result = b ; 
  return(Result) ; 
} 
void zunpackc ( b ) 
triecpointer b ; 
{internalcode c  ; 
  triecpointer a  ; 
  qmax = 1 ; 
  {register integer for_end; c = 1 ; for_end = cmax ; if ( c <= for_end) do 
    {
      a = b + c ; 
      if ( triecc [ a ] == c ) 
      {
	trieqc [ qmax ] = c ; 
	trieql [ qmax ] = triecl [ a ] ; 
	trieqr [ qmax ] = triecr [ a ] ; 
	qmax = qmax + 1 ; 
	triecr [ triecl [ 0 ] ] = a ; 
	triecl [ a ] = triecl [ 0 ] ; 
	triecl [ 0 ] = a ; 
	triecr [ a ] = 0 ; 
	triecc [ a ] = 0 ; 
      } 
    } 
  while ( c++ < for_end ) ; } 
  triectaken [ b ] = false ; 
} 
triecpointer zinsertcpat ( fpos ) 
wordindex fpos ; 
{register triecpointer Result; wordindex spos  ; 
  triecpointer a, b  ; 
  spos = fpos - patlen ; 
  spos = spos + 1 ; 
  b = 1 + word [ spos ] ; 
  a = triecl [ b ] ; 
  while ( ( a > 0 ) && ( spos < fpos ) ) {
      
    spos = spos + 1 ; 
    a = a + word [ spos ] ; 
    if ( triecc [ a ] != word [ spos ] ) 
    {
      if ( triecc [ a ] == 0 ) 
      {
	triecl [ triecr [ a ] ] = triecl [ a ] ; 
	triecr [ triecl [ a ] ] = triecr [ a ] ; 
	triecc [ a ] = word [ spos ] ; 
	triecl [ a ] = 0 ; 
	triecr [ a ] = 0 ; 
	if ( a > triecmax ) 
	triecmax = a ; 
      } 
      else {
	  
	unpackc ( a - word [ spos ] ) ; 
	trieqc [ qmax ] = word [ spos ] ; 
	trieql [ qmax ] = 0 ; 
	trieqr [ qmax ] = 0 ; 
	a = firstcfit () ; 
	triecl [ b ] = a ; 
	a = a + word [ spos ] ; 
      } 
      trieccount = trieccount + 1 ; 
    } 
    b = a ; 
    a = triecl [ a ] ; 
  } 
  trieql [ 1 ] = 0 ; 
  trieqr [ 1 ] = 0 ; 
  qmax = 1 ; 
  while ( spos < fpos ) {
      
    spos = spos + 1 ; 
    trieqc [ 1 ] = word [ spos ] ; 
    a = firstcfit () ; 
    triecl [ b ] = a ; 
    b = a + word [ spos ] ; 
    trieccount = trieccount + 1 ; 
  } 
  Result = b ; 
  patcount = patcount + 1 ; 
  return(Result) ; 
} 
void readtranslate ( ) 
{/* 30 */ textchar c  ; 
  integer n  ; 
  ASCIIcode j  ; 
  boolean bad  ; 
  boolean lower  ; 
  dottype i  ; 
  triepointer s, t  ; 
  imax = 1 ; 
  argv ( 4 , fname ) ; 
  reset ( translate , fname ) ; 
  if ( eof ( translate ) ) 
  {
    lefthyphenmin = 2 ; 
    righthyphenmin = 3 ; 
    {register integer for_end; j = 65 ; for_end = 90 ; if ( j <= for_end) do 
      {
	imax = imax + 1 ; 
	c = xchr [ j + 32 ] ; 
	xclass [ c ] = 3 ; 
	xint [ c ] = imax ; 
	xext [ imax ] = c ; 
	c = xchr [ j ] ; 
	xclass [ c ] = 3 ; 
	xint [ c ] = imax ; 
      } 
    while ( j++ < for_end ) ; } 
  } 
  else {
      
    {
      bufptr = 0 ; 
      while ( ! eoln ( translate ) ) {
	  
	if ( ( bufptr >= maxbuflen ) ) 
	{
	  {
	    bufptr = 0 ; 
	    do {
		bufptr = bufptr + 1 ; 
	      (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	    } while ( ! ( bufptr == maxbuflen ) ) ; 
	    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	  } 
	  {
	    (void) fprintf( stdout , "%s\n",  "Line too long" ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
	bufptr = bufptr + 1 ; 
	read ( translate , buf [ bufptr ] ) ; 
      } 
      readln ( translate ) ; 
      while ( bufptr < maxbuflen ) {
	  
	bufptr = bufptr + 1 ; 
	buf [ bufptr ] = ' ' ; 
      } 
    } 
    bad = false ; 
    if ( buf [ 1 ] == ' ' ) 
    n = 0 ; 
    else if ( xclass [ buf [ 1 ] ] == 1 ) 
    n = xint [ buf [ 1 ] ] ; 
    else bad = true ; 
    if ( xclass [ buf [ 2 ] ] == 1 ) 
    n = 10 * n + xint [ buf [ 2 ] ] ; 
    else bad = true ; 
    if ( ( n > 0 ) && ( n < maxdot ) ) 
    lefthyphenmin = n ; 
    else bad = true ; 
    if ( buf [ 3 ] == ' ' ) 
    n = 0 ; 
    else if ( xclass [ buf [ 3 ] ] == 1 ) 
    n = xint [ buf [ 3 ] ] ; 
    else bad = true ; 
    if ( xclass [ buf [ 4 ] ] == 1 ) 
    n = 10 * n + xint [ buf [ 4 ] ] ; 
    else bad = true ; 
    if ( ( n > 0 ) && ( n < maxdot ) ) 
    righthyphenmin = n ; 
    else bad = true ; 
    {register integer for_end; j = 1 ; for_end = 3 ; if ( j <= for_end) do 
      {
	if ( buf [ j + 4 ] != ' ' ) 
	xhyf [ j ] = buf [ j + 4 ] ; 
	if ( xclass [ xhyf [ j ] ] == 5 ) 
	xclass [ xhyf [ j ] ] = 2 ; 
	else bad = true ; 
      } 
    while ( j++ < for_end ) ; } 
    xclass [ '.' ] = 2 ; 
    if ( bad ) 
    {
      {
	bufptr = 0 ; 
	do {
	    bufptr = bufptr + 1 ; 
	  (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	} while ( ! ( bufptr == maxbuflen ) ) ; 
	(void) fprintf( stdout , "%c\n",  ' ' ) ; 
      } 
      {
	(void) fprintf( stdout , "%s\n",  "Bad hyphenation data" ) ; 
	uexit ( 1 ) ; 
      } 
    } 
    cmax = 254 ; 
    while ( ! eof ( translate ) ) {
	
      {
	bufptr = 0 ; 
	while ( ! eoln ( translate ) ) {
	    
	  if ( ( bufptr >= maxbuflen ) ) 
	  {
	    {
	      bufptr = 0 ; 
	      do {
		  bufptr = bufptr + 1 ; 
		(void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	      } while ( ! ( bufptr == maxbuflen ) ) ; 
	      (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	    } 
	    {
	      (void) fprintf( stdout , "%s\n",  "Line too long" ) ; 
	      uexit ( 1 ) ; 
	    } 
	  } 
	  bufptr = bufptr + 1 ; 
	  read ( translate , buf [ bufptr ] ) ; 
	} 
	readln ( translate ) ; 
	while ( bufptr < maxbuflen ) {
	    
	  bufptr = bufptr + 1 ; 
	  buf [ bufptr ] = ' ' ; 
	} 
      } 
      bufptr = 1 ; 
      lower = true ; 
      while ( ! bad ) {
	  
	patlen = 0 ; 
	do {
	    if ( bufptr < maxbuflen ) 
	  bufptr = bufptr + 1 ; 
	  else bad = true ; 
	  if ( buf [ bufptr ] == buf [ 1 ] ) 
	  if ( patlen == 0 ) 
	  goto lab30 ; 
	  else {
	      
	    if ( lower ) 
	    {
	      if ( imax == 255 ) 
	      {
		{
		  bufptr = 0 ; 
		  do {
		      bufptr = bufptr + 1 ; 
		    (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
		  } while ( ! ( bufptr == maxbuflen ) ) ; 
		  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
		} 
		{
		  (void) fprintf( stdout , "%s%ld%s%s\n",  "PATGEN capacity exceeded, sorry [" , (long)256                   , " letters" , "]." ) ; 
		  uexit ( 1 ) ; 
		} 
	      } 
	      imax = imax + 1 ; 
	      xext [ imax ] = xchr [ pat [ patlen ] ] ; 
	    } 
	    c = xchr [ pat [ 1 ] ] ; 
	    if ( patlen == 1 ) 
	    {
	      if ( xclass [ c ] != 5 ) 
	      bad = true ; 
	      xclass [ c ] = 3 ; 
	      xint [ c ] = imax ; 
	    } 
	    else {
		
	      if ( xclass [ c ] == 5 ) 
	      xclass [ c ] = 4 ; 
	      if ( xclass [ c ] != 4 ) 
	      bad = true ; 
	      i = 0 ; 
	      s = 1 ; 
	      t = triel [ s ] ; 
	      while ( ( t > 1 ) && ( i < patlen ) ) {
		  
		i = i + 1 ; 
		t = t + pat [ i ] ; 
		if ( triec [ t ] != pat [ i ] ) 
		{
		  if ( triec [ t ] == 0 ) 
		  {
		    triel [ trier [ t ] ] = triel [ t ] ; 
		    trier [ triel [ t ] ] = trier [ t ] ; 
		    triec [ t ] = pat [ i ] ; 
		    triel [ t ] = 0 ; 
		    trier [ t ] = 0 ; 
		    if ( t > triemax ) 
		    triemax = t ; 
		  } 
		  else {
		      
		    unpack ( t - pat [ i ] ) ; 
		    trieqc [ qmax ] = pat [ i ] ; 
		    trieql [ qmax ] = 0 ; 
		    trieqr [ qmax ] = 0 ; 
		    t = firstfit () ; 
		    triel [ s ] = t ; 
		    t = t + pat [ i ] ; 
		  } 
		  triecount = triecount + 1 ; 
		} 
		else if ( trier [ t ] > 0 ) 
		bad = true ; 
		s = t ; 
		t = triel [ s ] ; 
	      } 
	      if ( t > 1 ) 
	      bad = true ; 
	      trieql [ 1 ] = 0 ; 
	      trieqr [ 1 ] = 0 ; 
	      qmax = 1 ; 
	      while ( i < patlen ) {
		  
		i = i + 1 ; 
		trieqc [ 1 ] = pat [ i ] ; 
		t = firstfit () ; 
		triel [ s ] = t ; 
		s = t + pat [ i ] ; 
		triecount = triecount + 1 ; 
	      } 
	      trier [ s ] = imax ; 
	      if ( ! lower ) 
	      triel [ s ] = 1 ; 
	    } 
	  } 
	  else if ( patlen == maxdot ) 
	  bad = true ; 
	  else {
	      
	    patlen = patlen + 1 ; 
	    pat [ patlen ] = getASCII ( buf [ bufptr ] ) ; 
	  } 
	} while ( ! ( ( buf [ bufptr ] == buf [ 1 ] ) || bad ) ) ; 
	lower = false ; 
      } 
      lab30: if ( bad ) 
      {
	{
	  bufptr = 0 ; 
	  do {
	      bufptr = bufptr + 1 ; 
	    (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	  } while ( ! ( bufptr == maxbuflen ) ) ; 
	  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	} 
	{
	  (void) fprintf( stdout , "%s\n",  "Bad representation" ) ; 
	  uexit ( 1 ) ; 
	} 
      } 
    } 
  } 
  (void) fprintf( stdout , "%s%ld%s%ld%s%ld%s\n",  "left_hyphen_min = " , (long)lefthyphenmin ,   ", right_hyphen_min = " , (long)righthyphenmin , ", " , (long)imax - 1 , " letters" ) ; 
  cmax = imax ; 
} 
void zfindletters ( b , i ) 
triepointer b ; 
dottype i ; 
{ASCIIcode c  ; 
  triepointer a  ; 
  dottype j  ; 
  triecpointer l  ; 
  if ( i == 1 ) 
  initcounttrie () ; 
  {register integer for_end; c = 1 ; for_end = 255 ; if ( c <= for_end) do 
    {
      a = b + c ; 
      if ( triec [ a ] == c ) 
      {
	pat [ i ] = c ; 
	if ( trier [ a ] == 0 ) 
	findletters ( triel [ a ] , i + 1 ) ; 
	else if ( triel [ a ] == 0 ) 
	{
	  l = 1 + trier [ a ] ; 
	  {register integer for_end; j = 1 ; for_end = i - 1 ; if ( j <= 
	  for_end) do 
	    {
	      if ( triecmax == triecsize ) 
	      {
		(void) fprintf( stdout , "%s%ld%s%s\n",  "PATGEN capacity exceeded, sorry [" ,                 (long)triecsize , " count trie nodes" , "]." ) ; 
		uexit ( 1 ) ; 
	      } 
	      triecmax = triecmax + 1 ; 
	      triecl [ l ] = triecmax ; 
	      l = triecmax ; 
	      triecc [ l ] = pat [ j ] ; 
	    } 
	  while ( j++ < for_end ) ; } 
	  triecl [ l ] = 0 ; 
	} 
      } 
    } 
  while ( c++ < for_end ) ; } 
} 
void ztraversecounttrie ( b , i ) 
triecpointer b ; 
dottype i ; 
{internalcode c  ; 
  triecpointer a  ; 
  {register integer for_end; c = 1 ; for_end = cmax ; if ( c <= for_end) do 
    {
      a = b + c ; 
      if ( triecc [ a ] == c ) 
      {
	pat [ i ] = c ; 
	if ( i < patlen ) 
	traversecounttrie ( triecl [ a ] , i + 1 ) ; 
	else if ( goodwt * triecl [ a ] < thresh ) 
	{
	  insertpattern ( maxval , patdot ) ; 
	  badpatcount = badpatcount + 1 ; 
	} 
	else if ( goodwt * triecl [ a ] - badwt * triecr [ a ] >= thresh ) 
	{
	  insertpattern ( hyphlevel , patdot ) ; 
	  goodpatcount = goodpatcount + 1 ; 
	  goodcount = goodcount + triecl [ a ] ; 
	  badcount = badcount + triecr [ a ] ; 
	} 
	else moretocome = true ; 
      } 
    } 
  while ( c++ < for_end ) ; } 
} 
void collectcounttrie ( ) 
{goodpatcount = 0 ; 
  badpatcount = 0 ; 
  goodcount = 0 ; 
  badcount = 0 ; 
  moretocome = false ; 
  traversecounttrie ( 1 , 1 ) ; 
  (void) fprintf( stdout , "%ld%s%ld%s",  (long)goodpatcount , " good and " , (long)badpatcount ,   " bad patterns added" ) ; 
  levelpatterncount = levelpatterncount + goodpatcount ; 
  if ( moretocome ) 
  (void) fprintf( stdout , "%s\n",  " (more to come)" ) ; 
  else
  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
  (void) fprintf( stdout , "%s%ld%s%ld%s",  "finding " , (long)goodcount , " good and " , (long)badcount ,   " bad hyphens" ) ; 
  if ( goodpatcount > 0 ) 
  {
    (void) Fputs( stdout ,  ", efficiency = " ) ; 
    printreal ( goodcount / ((double) ( goodpatcount + badcount / ((double) ( 
    thresh / ((double) goodwt ) ) ) ) ) , 1 , 2 ) ; 
    (void) putc('\n',  stdout );
  } 
  else
  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
  (void) fprintf( stdout , "%s%ld%s%s%ld%s%ld%s\n",  "pattern trie has " , (long)triecount , " nodes, " ,   "trie_max = " , (long)triemax , ", " , (long)opcount , " outputs" ) ; 
} 
triepointer zdeletepatterns ( s ) 
triepointer s ; 
{register triepointer Result; internalcode c  ; 
  triepointer t  ; 
  boolean allfreed  ; 
  optype h, n  ; 
  allfreed = true ; 
  {register integer for_end; c = 1 ; for_end = cmax ; if ( c <= for_end) do 
    {
      t = s + c ; 
      if ( triec [ t ] == c ) 
      {
	{
	  h = 0 ; 
	  ops [ 0 ] .op = trier [ t ] ; 
	  n = ops [ 0 ] .op ; 
	  while ( n > 0 ) {
	      
	    if ( ops [ n ] .val == maxval ) 
	    ops [ h ] .op = ops [ n ] .op ; 
	    else h = n ; 
	    n = ops [ h ] .op ; 
	  } 
	  trier [ t ] = ops [ 0 ] .op ; 
	} 
	if ( triel [ t ] > 0 ) 
	triel [ t ] = deletepatterns ( triel [ t ] ) ; 
	if ( ( triel [ t ] > 0 ) || ( trier [ t ] > 0 ) || ( s == 1 ) ) 
	allfreed = false ; 
	else {
	    
	  triel [ trier [ triemax + 1 ] ] = t ; 
	  trier [ t ] = trier [ triemax + 1 ] ; 
	  triel [ t ] = triemax + 1 ; 
	  trier [ triemax + 1 ] = t ; 
	  triec [ t ] = 0 ; 
	  triecount = triecount - 1 ; 
	} 
      } 
    } 
  while ( c++ < for_end ) ; } 
  if ( allfreed ) 
  {
    trietaken [ s ] = false ; 
    s = 0 ; 
  } 
  Result = s ; 
  return(Result) ; 
} 
void deletebadpatterns ( ) 
{optype oldopcount  ; 
  triepointer oldtriecount  ; 
  triepointer t  ; 
  optype h  ; 
  oldopcount = opcount ; 
  oldtriecount = triecount ; 
  t = deletepatterns ( 1 ) ; 
  {register integer for_end; h = 1 ; for_end = maxops ; if ( h <= for_end) do 
    if ( ops [ h ] .val == maxval ) 
    {
      ops [ h ] .val = 0 ; 
      opcount = opcount - 1 ; 
    } 
  while ( h++ < for_end ) ; } 
  (void) fprintf( stdout , "%ld%s%ld%s\n",  (long)oldtriecount - triecount , " nodes and " , (long)oldopcount -   opcount , " outputs deleted" ) ; 
  qmaxthresh = 7 ; 
} 
void zoutputpatterns ( s , patlen ) 
triepointer s ; 
dottype patlen ; 
{internalcode c  ; 
  triepointer t  ; 
  optype h  ; 
  dottype d  ; 
  triecpointer l  ; 
  {register integer for_end; c = 1 ; for_end = cmax ; if ( c <= for_end) do 
    {
      t = s + c ; 
      if ( triec [ t ] == c ) 
      {
	pat [ patlen ] = c ; 
	h = trier [ t ] ; 
	if ( h > 0 ) 
	{
	  {register integer for_end; d = 0 ; for_end = patlen ; if ( d <= 
	  for_end) do 
	    hval [ d ] = 0 ; 
	  while ( d++ < for_end ) ; } 
	  do {
	      d = ops [ h ] .dot ; 
	    if ( hval [ d ] < ops [ h ] .val ) 
	    hval [ d ] = ops [ h ] .val ; 
	    h = ops [ h ] .op ; 
	  } while ( ! ( h == 0 ) ) ; 
	  if ( hval [ 0 ] > 0 ) 
	  (void) fprintf( patout , "%ld",  (long)xdig [ hval [ 0 ] ] ) ; 
	  {register integer for_end; d = 1 ; for_end = patlen ; if ( d <= 
	  for_end) do 
	    {
	      l = triecl [ 1 + pat [ d ] ] ; 
	      while ( l > 0 ) {
		  
		(void) putc( xchr [ triecc [ l ] ] ,  patout );
		l = triecl [ l ] ; 
	      } 
	      (void) fprintf( patout , "%ld",  (long)xext [ pat [ d ] ] ) ; 
	      if ( hval [ d ] > 0 ) 
	      (void) fprintf( patout , "%ld",  (long)xdig [ hval [ d ] ] ) ; 
	    } 
	  while ( d++ < for_end ) ; } 
	  (void) putc('\n',  patout );
	} 
	if ( triel [ t ] > 0 ) 
	outputpatterns ( triel [ t ] , patlen + 1 ) ; 
      } 
    } 
  while ( c++ < for_end ) ; } 
} 
void readword ( ) 
{/* 30 40 */ textchar c  ; 
  triepointer t  ; 
  {
    bufptr = 0 ; 
    while ( ! eoln ( dictionary ) ) {
	
      if ( ( bufptr >= maxbuflen ) ) 
      {
	{
	  bufptr = 0 ; 
	  do {
	      bufptr = bufptr + 1 ; 
	    (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	  } while ( ! ( bufptr == maxbuflen ) ) ; 
	  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	} 
	{
	  (void) fprintf( stdout , "%s\n",  "Line too long" ) ; 
	  uexit ( 1 ) ; 
	} 
      } 
      bufptr = bufptr + 1 ; 
      read ( dictionary , buf [ bufptr ] ) ; 
    } 
    readln ( dictionary ) ; 
    while ( bufptr < maxbuflen ) {
	
      bufptr = bufptr + 1 ; 
      buf [ bufptr ] = ' ' ; 
    } 
  } 
  word [ 1 ] = 1 ; 
  wlen = 1 ; 
  bufptr = 0 ; 
  do {
      bufptr = bufptr + 1 ; 
    c = buf [ bufptr ] ; 
    switch ( xclass [ c ] ) 
    {case 0 : 
      goto lab40 ; 
      break ; 
    case 1 : 
      if ( wlen == 1 ) 
      {
	if ( xint [ c ] != wordwt ) 
	wtchg = true ; 
	wordwt = xint [ c ] ; 
      } 
      else dotw [ wlen ] = xint [ c ] ; 
      break ; 
    case 2 : 
      dots [ wlen ] = xint [ c ] ; 
      break ; 
    case 3 : 
      {
	wlen = wlen + 1 ; 
	if ( wlen == maxlen ) 
	{
	  {
	    bufptr = 0 ; 
	    do {
		bufptr = bufptr + 1 ; 
	      (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	    } while ( ! ( bufptr == maxbuflen ) ) ; 
	    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	  } 
	  {
	    (void) fprintf( stdout , "%s%s%ld%s\n",  "PATGEN capacity exceeded, sorry [" ,             "word length=" , (long)maxlen , "]." ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
	word [ wlen ] = xint [ c ] ; 
	dots [ wlen ] = 0 ; 
	dotw [ wlen ] = wordwt ; 
      } 
      break ; 
    case 4 : 
      {
	wlen = wlen + 1 ; 
	if ( wlen == maxlen ) 
	{
	  {
	    bufptr = 0 ; 
	    do {
		bufptr = bufptr + 1 ; 
	      (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	    } while ( ! ( bufptr == maxbuflen ) ) ; 
	    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	  } 
	  {
	    (void) fprintf( stdout , "%s%s%ld%s\n",  "PATGEN capacity exceeded, sorry [" ,             "word length=" , (long)maxlen , "]." ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
	{
	  t = 1 ; 
	  while ( true ) {
	      
	    t = triel [ t ] + xord [ c ] ; 
	    if ( triec [ t ] != xord [ c ] ) 
	    {
	      {
		bufptr = 0 ; 
		do {
		    bufptr = bufptr + 1 ; 
		  (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
		} while ( ! ( bufptr == maxbuflen ) ) ; 
		(void) fprintf( stdout , "%c\n",  ' ' ) ; 
	      } 
	      {
		(void) fprintf( stdout , "%s\n",  "Bad representation" ) ; 
		uexit ( 1 ) ; 
	      } 
	    } 
	    if ( trier [ t ] != 0 ) 
	    {
	      word [ wlen ] = trier [ t ] ; 
	      goto lab30 ; 
	    } 
	    if ( bufptr == maxbuflen ) 
	    c = ' ' ; 
	    else {
		
	      bufptr = bufptr + 1 ; 
	      c = buf [ bufptr ] ; 
	    } 
	  } 
	  lab30: ; 
	} 
	dots [ wlen ] = 0 ; 
	dotw [ wlen ] = wordwt ; 
      } 
      break ; 
    case 5 : 
      {
	{
	  bufptr = 0 ; 
	  do {
	      bufptr = bufptr + 1 ; 
	    (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	  } while ( ! ( bufptr == maxbuflen ) ) ; 
	  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	} 
	{
	  (void) fprintf( stdout , "%s\n",  "Bad character" ) ; 
	  uexit ( 1 ) ; 
	} 
      } 
      break ; 
    } 
  } while ( ! ( bufptr == maxbuflen ) ) ; 
  lab40: wlen = wlen + 1 ; 
  word [ wlen ] = 1 ; 
} 
void hyphenate ( ) 
{/* 30 */ wordindex spos, dpos, fpos  ; 
  triepointer t  ; 
  optype h  ; 
  valtype v  ; 
  {register integer for_end; spos = wlen - hyfmax ; for_end = 0 ; if ( spos 
  >= for_end) do 
    {
      nomore [ spos ] = false ; 
      hval [ spos ] = 0 ; 
      fpos = spos + 1 ; 
      t = 1 + word [ fpos ] ; 
      do {
	  h = trier [ t ] ; 
	while ( h > 0 ) {
	    
	  dpos = spos + ops [ h ] .dot ; 
	  v = ops [ h ] .val ; 
	  if ( ( v < maxval ) && ( hval [ dpos ] < v ) ) 
	  hval [ dpos ] = v ; 
	  if ( ( v >= hyphlevel ) ) 
	  if ( ( ( fpos - patlen ) <= ( dpos - patdot ) ) && ( ( dpos - patdot 
	  ) <= spos ) ) 
	  nomore [ dpos ] = true ; 
	  h = ops [ h ] .op ; 
	} 
	t = triel [ t ] ; 
	if ( t == 0 ) 
	goto lab30 ; 
	fpos = fpos + 1 ; 
	t = t + word [ fpos ] ; 
      } while ( ! ( triec [ t ] != word [ fpos ] ) ) ; 
      lab30: ; 
    } 
  while ( spos-- > for_end ) ; } 
} 
void changedots ( ) 
{wordindex dpos  ; 
  {register integer for_end; dpos = wlen - hyfmax ; for_end = hyfmin ; if ( 
  dpos >= for_end) do 
    {
      if ( odd ( hval [ dpos ] ) ) 
      dots [ dpos ] = dots [ dpos ] + 1 ; 
      if ( dots [ dpos ] == 3 ) 
      goodcount = goodcount + dotw [ dpos ] ; 
      else if ( dots [ dpos ] == 1 ) 
      badcount = badcount + dotw [ dpos ] ; 
      else if ( dots [ dpos ] == 2 ) 
      misscount = misscount + dotw [ dpos ] ; 
    } 
  while ( dpos-- > for_end ) ; } 
} 
void outputhyphenatedword ( ) 
{wordindex dpos  ; 
  triecpointer l  ; 
  if ( wtchg ) 
  {
    (void) fprintf( pattmp , "%ld",  (long)xdig [ wordwt ] ) ; 
    wtchg = false ; 
  } 
  {register integer for_end; dpos = 2 ; for_end = wlen - 2 ; if ( dpos <= 
  for_end) do 
    {
      l = triecl [ 1 + word [ dpos ] ] ; 
      while ( l > 0 ) {
	  
	(void) putc( xchr [ triecc [ l ] ] ,  pattmp );
	l = triecl [ l ] ; 
      } 
      (void) fprintf( pattmp , "%ld",  (long)xext [ word [ dpos ] ] ) ; 
      if ( dots [ dpos ] != 0 ) 
      (void) fprintf( pattmp , "%ld",  (long)xhyf [ dots [ dpos ] ] ) ; 
      if ( dotw [ dpos ] != wordwt ) 
      (void) fprintf( pattmp , "%ld",  (long)xdig [ dotw [ dpos ] ] ) ; 
    } 
  while ( dpos++ < for_end ) ; } 
  l = triecl [ 1 + word [ wlen - 1 ] ] ; 
  while ( l > 0 ) {
      
    (void) putc( xchr [ triecc [ l ] ] ,  pattmp );
    l = triecl [ l ] ; 
  } 
  (void) fprintf( pattmp , "%ld\n",  (long)xext [ word [ wlen - 1 ] ] ) ; 
} 
void doword ( ) 
{/* 22 30 */ wordindex spos, dpos, fpos  ; 
  triecpointer a  ; 
  boolean goodp  ; 
  {register integer for_end; dpos = wlen - dotmax ; for_end = dotmin ; if ( 
  dpos >= for_end) do 
    {
      spos = dpos - patdot ; 
      fpos = spos + patlen ; 
      if ( nomore [ dpos ] ) 
      goto lab22 ; 
      if ( dots [ dpos ] == gooddot ) 
      goodp = true ; 
      else if ( dots [ dpos ] == baddot ) 
      goodp = false ; 
      else goto lab22 ; 
      spos = spos + 1 ; 
      a = 1 + word [ spos ] ; 
      while ( spos < fpos ) {
	  
	spos = spos + 1 ; 
	a = triecl [ a ] + word [ spos ] ; 
	if ( triecc [ a ] != word [ spos ] ) 
	{
	  a = insertcpat ( fpos ) ; 
	  goto lab30 ; 
	} 
      } 
      lab30: if ( goodp ) 
      triecl [ a ] = triecl [ a ] + dotw [ dpos ] ; 
      else triecr [ a ] = triecr [ a ] + dotw [ dpos ] ; 
      lab22: ; 
    } 
  while ( dpos-- > for_end ) ; } 
} 
void dodictionary ( ) 
{goodcount = 0 ; 
  badcount = 0 ; 
  misscount = 0 ; 
  wordwt = 1 ; 
  wtchg = false ; 
  argv ( 1 , fname ) ; 
  reset ( dictionary , fname ) ; 
  xclass [ '.' ] = 5 ; 
  xclass [ xhyf [ 1 ] ] = 2 ; 
  xint [ xhyf [ 1 ] ] = 0 ; 
  xclass [ xhyf [ 2 ] ] = 2 ; 
  xint [ xhyf [ 2 ] ] = 2 ; 
  xclass [ xhyf [ 3 ] ] = 2 ; 
  xint [ xhyf [ 3 ] ] = 2 ; 
  hyfmin = lefthyphenmin + 1 ; 
  hyfmax = righthyphenmin + 1 ; 
  hyflen = hyfmin + hyfmax ; 
  if ( procesp ) 
  {
    dotmin = patdot ; 
    dotmax = patlen - patdot ; 
    if ( dotmin < hyfmin ) 
    dotmin = hyfmin ; 
    if ( dotmax < hyfmax ) 
    dotmax = hyfmax ; 
    dotlen = dotmin + dotmax ; 
    if ( odd ( hyphlevel ) ) 
    {
      gooddot = 2 ; 
      baddot = 0 ; 
    } 
    else {
	
      gooddot = 1 ; 
      baddot = 3 ; 
    } 
  } 
  if ( procesp ) 
  {
    initcounttrie () ; 
    (void) fprintf( stdout , "%s%ld%s%ld\n",  "processing dictionary with pat_len = " , (long)patlen ,     ", pat_dot = " , (long)patdot ) ; 
  } 
  if ( hyphp ) 
  {
    filnam [ 1 ] = 'p' ; 
    filnam [ 2 ] = 'a' ; 
    filnam [ 3 ] = 't' ; 
    filnam [ 4 ] = 't' ; 
    filnam [ 5 ] = 'm' ; 
    filnam [ 6 ] = 'p' ; 
    filnam [ 7 ] = '.' ; 
    filnam [ 8 ] = xdig [ hyphlevel ] ; 
    rewrite ( pattmp , filnam ) ; 
    (void) fprintf( stdout , "%s%ld\n",  "writing pattmp." , (long)xdig [ hyphlevel ] ) ; 
  } 
  while ( ! eof ( dictionary ) ) {
      
    readword () ; 
    if ( wlen >= hyflen ) 
    {
      hyphenate () ; 
      changedots () ; 
    } 
    if ( hyphp ) 
    if ( wlen > 2 ) 
    outputhyphenatedword () ; 
    if ( procesp ) 
    if ( wlen >= dotlen ) 
    doword () ; 
  } 
  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
  (void) fprintf( stdout , "%ld%s%ld%s%ld%s\n",  (long)goodcount , " good, " , (long)badcount , " bad, " , (long)misscount ,   " missed" ) ; 
  if ( ( goodcount + misscount ) > 0 ) 
  {
    printreal ( ( 100 * goodcount / ((double) ( goodcount + misscount ) ) ) , 
    1 , 2 ) ; 
    (void) Fputs( stdout ,  " %, " ) ; 
    printreal ( ( 100 * badcount / ((double) ( goodcount + misscount ) ) ) , 1 
    , 2 ) ; 
    (void) Fputs( stdout ,  " %, " ) ; 
    printreal ( ( 100 * misscount / ((double) ( goodcount + misscount ) ) ) , 
    1 , 2 ) ; 
    (void) fprintf( stdout , "%s\n",  " %" ) ; 
  } 
  if ( procesp ) 
  (void) fprintf( stdout , "%ld%s%ld%s%s%ld\n",  (long)patcount , " patterns, " , (long)trieccount ,   " nodes in count trie, " , "triec_max = " , (long)triecmax ) ; 
  if ( hyphp ) 
  ; 
} 
void readpatterns ( ) 
{/* 30 40 */ textchar c  ; 
  digit d  ; 
  dottype i  ; 
  triepointer t  ; 
  xclass [ '.' ] = 3 ; 
  xint [ '.' ] = 1 ; 
  levelpatterncount = 0 ; 
  maxpat = 0 ; 
  argv ( 2 , fname ) ; 
  reset ( patterns , fname ) ; 
  while ( ! eof ( patterns ) ) {
      
    {
      bufptr = 0 ; 
      while ( ! eoln ( patterns ) ) {
	  
	if ( ( bufptr >= maxbuflen ) ) 
	{
	  {
	    bufptr = 0 ; 
	    do {
		bufptr = bufptr + 1 ; 
	      (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	    } while ( ! ( bufptr == maxbuflen ) ) ; 
	    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	  } 
	  {
	    (void) fprintf( stdout , "%s\n",  "Line too long" ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
	bufptr = bufptr + 1 ; 
	read ( patterns , buf [ bufptr ] ) ; 
      } 
      readln ( patterns ) ; 
      while ( bufptr < maxbuflen ) {
	  
	bufptr = bufptr + 1 ; 
	buf [ bufptr ] = ' ' ; 
      } 
    } 
    levelpatterncount = levelpatterncount + 1 ; 
    patlen = 0 ; 
    bufptr = 0 ; 
    hval [ 0 ] = 0 ; 
    do {
	bufptr = bufptr + 1 ; 
      c = buf [ bufptr ] ; 
      switch ( xclass [ c ] ) 
      {case 0 : 
	goto lab40 ; 
	break ; 
      case 1 : 
	{
	  d = xint [ c ] ; 
	  if ( d >= maxval ) 
	  {
	    {
	      bufptr = 0 ; 
	      do {
		  bufptr = bufptr + 1 ; 
		(void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	      } while ( ! ( bufptr == maxbuflen ) ) ; 
	      (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	    } 
	    {
	      (void) fprintf( stdout , "%s\n",  "Bad hyphenation value" ) ; 
	      uexit ( 1 ) ; 
	    } 
	  } 
	  if ( d > maxpat ) 
	  maxpat = d ; 
	  hval [ patlen ] = d ; 
	} 
	break ; 
      case 3 : 
	{
	  patlen = patlen + 1 ; 
	  hval [ patlen ] = 0 ; 
	  pat [ patlen ] = xint [ c ] ; 
	} 
	break ; 
      case 4 : 
	{
	  patlen = patlen + 1 ; 
	  hval [ patlen ] = 0 ; 
	  {
	    t = 1 ; 
	    while ( true ) {
		
	      t = triel [ t ] + xord [ c ] ; 
	      if ( triec [ t ] != xord [ c ] ) 
	      {
		{
		  bufptr = 0 ; 
		  do {
		      bufptr = bufptr + 1 ; 
		    (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
		  } while ( ! ( bufptr == maxbuflen ) ) ; 
		  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
		} 
		{
		  (void) fprintf( stdout , "%s\n",  "Bad representation" ) ; 
		  uexit ( 1 ) ; 
		} 
	      } 
	      if ( trier [ t ] != 0 ) 
	      {
		pat [ patlen ] = trier [ t ] ; 
		goto lab30 ; 
	      } 
	      if ( bufptr == maxbuflen ) 
	      c = ' ' ; 
	      else {
		  
		bufptr = bufptr + 1 ; 
		c = buf [ bufptr ] ; 
	      } 
	    } 
	    lab30: ; 
	  } 
	} 
	break ; 
      case 2 : 
      case 5 : 
	{
	  {
	    bufptr = 0 ; 
	    do {
		bufptr = bufptr + 1 ; 
	      (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	    } while ( ! ( bufptr == maxbuflen ) ) ; 
	    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	  } 
	  {
	    (void) fprintf( stdout , "%s\n",  "Bad character" ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
	break ; 
      } 
    } while ( ! ( bufptr == maxbuflen ) ) ; 
    lab40: if ( patlen > 0 ) 
    {register integer for_end; i = 0 ; for_end = patlen ; if ( i <= for_end) 
    do 
      {
	if ( hval [ i ] != 0 ) 
	insertpattern ( hval [ i ] , i ) ; 
	if ( i > 1 ) 
	if ( i < patlen ) 
	if ( pat [ i ] == 1 ) 
	{
	  {
	    bufptr = 0 ; 
	    do {
		bufptr = bufptr + 1 ; 
	      (void) fprintf( stdout , "%ld",  (long)buf [ bufptr ] ) ; 
	    } while ( ! ( bufptr == maxbuflen ) ) ; 
	    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	  } 
	  {
	    (void) fprintf( stdout , "%s\n",  "Bad edge_of_word" ) ; 
	    uexit ( 1 ) ; 
	  } 
	} 
      } 
    while ( i++ < for_end ) ; } 
  } 
  (void) fprintf( stdout , "%ld%s\n",  (long)levelpatterncount , " patterns read in" ) ; 
  (void) fprintf( stdout , "%s%ld%s%s%ld%s%ld%s\n",  "pattern trie has " , (long)triecount , " nodes, " ,   "trie_max = " , (long)triemax , ", " , (long)opcount , " outputs" ) ; 
} 
void main_body() {
    
  initialize () ; 
  initpatterntrie () ; 
  readtranslate () ; 
  readpatterns () ; 
  procesp = true ; 
  hyphp = false ; 
  do {
      (void) Fputs( stdout ,  "hyph_start, hyph_finish: " ) ; 
    input2ints ( n1 , n2 ) ; 
    if ( ( n1 >= 1 ) && ( n1 < maxval ) && ( n2 >= 1 ) && ( n2 < maxval ) ) 
    {
      hyphstart = n1 ; 
      hyphfinish = n2 ; 
    } 
    else {
	
      n1 = 0 ; 
      (void) fprintf( stdout , "%s%ld%s\n",  "Specify 1<=hyph_start,hyph_finish<=" , (long)maxval - 1 ,       " !" ) ; 
    } 
  } while ( ! ( n1 > 0 ) ) ; 
  hyphlevel = maxpat ; 
  {register integer for_end; i = hyphstart ; for_end = hyphfinish ; if ( i <= 
  for_end) do 
    {
      hyphlevel = i ; 
      levelpatterncount = 0 ; 
      if ( hyphlevel > hyphstart ) 
      (void) fprintf( stdout , "%c\n",  ' ' ) ; 
      else if ( hyphstart <= maxpat ) 
      (void) fprintf( stdout , "%s%ld%s\n",  "Largest hyphenation value " , (long)maxpat ,       " in patterns should be less than hyph_start" ) ; 
      do {
	  (void) Fputs( stdout ,  "pat_start, pat_finish: " ) ; 
	input2ints ( n1 , n2 ) ; 
	if ( ( n1 >= 1 ) && ( n1 <= n2 ) && ( n2 <= maxdot ) ) 
	{
	  patstart = n1 ; 
	  patfinish = n2 ; 
	} 
	else {
	    
	  n1 = 0 ; 
	  (void) fprintf( stdout , "%s%ld%s\n",  "Specify 1<=pat_start<=pat_finish<=" , (long)maxdot ,           " !" ) ; 
	} 
      } while ( ! ( n1 > 0 ) ) ; 
      (void) Fputs( stdout ,  "good weight, bad weight, threshold: " ) ; 
      input3ints ( goodwt , badwt , thresh ) ; 
      {register integer for_end; k = 0 ; for_end = maxdot ; if ( k <= 
      for_end) do 
	morethislevel [ k ] = true ; 
      while ( k++ < for_end ) ; } 
      {register integer for_end; j = patstart ; for_end = patfinish ; if ( j 
      <= for_end) do 
	{
	  patlen = j ; 
	  patdot = patlen / 2 ; 
	  dot1 = patdot * 2 ; 
	  do {
	      patdot = dot1 - patdot ; 
	    dot1 = patlen * 2 - dot1 - 1 ; 
	    if ( morethislevel [ patdot ] ) 
	    {
	      dodictionary () ; 
	      collectcounttrie () ; 
	      morethislevel [ patdot ] = moretocome ; 
	    } 
	  } while ( ! ( patdot == patlen ) ) ; 
	  {register integer for_end; k = maxdot ; for_end = 1 ; if ( k >= 
	  for_end) do 
	    if ( ! morethislevel [ k - 1 ] ) 
	    morethislevel [ k ] = false ; 
	  while ( k-- > for_end ) ; } 
	} 
      while ( j++ < for_end ) ; } 
      deletebadpatterns () ; 
      (void) fprintf( stdout , "%s%ld%s%ld\n",  "total of " , (long)levelpatterncount ,       " patterns at hyph_level " , (long)hyphlevel ) ; 
    } 
  while ( i++ < for_end ) ; } 
  findletters ( triel [ 1 ] , 1 ) ; 
  argv ( 3 , fname ) ; 
  rewrite ( patout , fname ) ; 
  outputpatterns ( 1 , 1 ) ; 
  procesp = false ; 
  hyphp = true ; 
  (void) Fputs( stdout ,  "hyphenate word list? " ) ; 
  {
    buf [ 1 ] = getc ( stdin ) ; 
    readln ( stdin ) ; 
  } 
  if ( ( buf [ 1 ] == 'Y' ) || ( buf [ 1 ] == 'y' ) ) 
  dodictionary () ; 
  lab9999: ; 
} 
