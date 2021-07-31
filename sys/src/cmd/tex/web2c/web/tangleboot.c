#include "config.h"
/* 9999 */ 
#define bufsize 100 
#define maxbytes 45000L 
#define maxtoks 50000L 
#define maxnames 4000 
#define maxtexts 2000 
#define hashsize 353 
#define longestname 400 
#define linelength 72 
#define outbufsize 144 
#define stacksize 100 
#define maxidlength 50 
#define unambiglength 20 
typedef unsigned char ASCIIcode  ; 
typedef file_ptr /* of  ASCIIcode */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef unsigned short sixteenbits  ; 
typedef integer namepointer  ; 
typedef integer textpointer  ; 
typedef struct {
    sixteenbits endfield ; 
  sixteenbits bytefield ; 
  namepointer namefield ; 
  textpointer replfield ; 
  short modfield ; 
} outputstate  ; 
schar history  ; 
ASCIIcode xord[256]  ; 
ASCIIcode xchr[256]  ; 
textfile webfile  ; 
textfile changefile  ; 
textfile Pascalfile  ; 
textfile pool  ; 
ASCIIcode buffer[bufsize + 1]  ; 
boolean phaseone  ; 
ASCIIcode bytemem[3][maxbytes + 1]  ; 
eightbits tokmem[4][maxtoks + 1]  ; 
sixteenbits bytestart[maxnames + 1]  ; 
sixteenbits tokstart[maxtexts + 1]  ; 
sixteenbits link[maxnames + 1]  ; 
sixteenbits ilk[maxnames + 1]  ; 
sixteenbits equiv[maxnames + 1]  ; 
sixteenbits textlink[maxtexts + 1]  ; 
namepointer nameptr  ; 
namepointer stringptr  ; 
integer byteptr[3]  ; 
integer poolchecksum  ; 
textpointer textptr  ; 
integer tokptr[4]  ; 
schar z  ; 
integer idfirst  ; 
integer idloc  ; 
integer doublechars  ; 
sixteenbits hash[hashsize + 1], chophash[hashsize + 1]  ; 
ASCIIcode choppedid[unambiglength + 1]  ; 
ASCIIcode modtext[longestname + 1]  ; 
textpointer lastunnamed  ; 
outputstate curstate  ; 
outputstate stack[stacksize + 1]  ; 
integer stackptr  ; 
schar zo  ; 
eightbits bracelevel  ; 
integer curval  ; 
ASCIIcode outbuf[outbufsize + 1]  ; 
integer outptr  ; 
integer breakptr  ; 
integer semiptr  ; 
eightbits outstate  ; 
integer outval, outapp  ; 
ASCIIcode outsign  ; 
schar lastsign  ; 
ASCIIcode outcontrib[linelength + 1]  ; 
integer ii  ; 
integer line  ; 
integer otherline  ; 
integer templine  ; 
integer limit  ; 
integer loc  ; 
boolean inputhasended  ; 
boolean changing  ; 
ASCIIcode changebuffer[bufsize + 1]  ; 
integer changelimit  ; 
namepointer curmodule  ; 
boolean scanninghex  ; 
eightbits nextcontrol  ; 
textpointer currepltext  ; 
short modulecount  ; 
char webname[PATHMAX + 1], chgname[PATHMAX + 1], pascalfilename[PATHMAX + 1], 
poolfilename[PATHMAX + 1]  ; 

#include "tangleboot.h"
void error ( ) 
{integer j  ; 
  integer k, l  ; 
  if ( phaseone ) 
  {
    if ( changing ) 
    (void) Fputs( stdout ,  ". (change file " ) ; 
    else
    (void) Fputs( stdout ,  ". (" ) ; 
    (void) fprintf( stdout , "%s%ld%c\n",  "l." , (long)line , ')' ) ; 
    if ( loc >= limit ) 
    l = limit ; 
    else l = loc ; 
    {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) do 
      if ( buffer [ k - 1 ] == 9 ) 
      (void) putc( ' ' ,  stdout );
      else
      (void) putc( xchr [ buffer [ k - 1 ] ] ,  stdout );
    while ( k++ < for_end ) ; } 
    (void) putc('\n',  stdout );
    {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) do 
      (void) putc( ' ' ,  stdout );
    while ( k++ < for_end ) ; } 
    {register integer for_end; k = l + 1 ; for_end = limit ; if ( k <= 
    for_end) do 
      (void) putc( xchr [ buffer [ k - 1 ] ] ,  stdout );
    while ( k++ < for_end ) ; } 
    (void) putc( ' ' ,  stdout );
  } 
  else {
      
    (void) fprintf( stdout , "%s%ld%c\n",  ". (l." , (long)line , ')' ) ; 
    {register integer for_end; j = 1 ; for_end = outptr ; if ( j <= for_end) 
    do 
      (void) putc( xchr [ outbuf [ j - 1 ] ] ,  stdout );
    while ( j++ < for_end ) ; } 
    (void) Fputs( stdout ,  "... " ) ; 
  } 
  flush ( stdout ) ; 
  history = 2 ; 
} 
void scanargs ( ) 
{integer dotpos, slashpos, i, a  ; 
  char c  ; 
  char fname[PATHMAX + 1]  ; 
  boolean foundweb, foundchange  ; 
  foundweb = false ; 
  foundchange = false ; 
  {register integer for_end; a = 1 ; for_end = argc - 1 ; if ( a <= for_end) 
  do 
    {
      argv ( a , fname ) ; 
      if ( fname [ 1 ] != '-' ) 
      {
	if ( ! foundweb ) 
	{
	  dotpos = -1 ; 
	  slashpos = -1 ; 
	  i = 1 ; 
	  while ( ( fname [ i ] != ' ' ) && ( i <= PATHMAX - 5 ) ) {
	      
	    webname [ i ] = fname [ i ] ; 
	    if ( fname [ i ] == '.' ) 
	    dotpos = i ; 
	    if ( fname [ i ] == '/' ) 
	    slashpos = i ; 
	    i = i + 1 ; 
	  } 
	  webname [ i ] = ' ' ; 
	  if ( ( dotpos == -1 ) || ( dotpos < slashpos ) ) 
	  {
	    dotpos = i ; 
	    webname [ dotpos ] = '.' ; 
	    webname [ dotpos + 1 ] = 'w' ; 
	    webname [ dotpos + 2 ] = 'e' ; 
	    webname [ dotpos + 3 ] = 'b' ; 
	    webname [ dotpos + 4 ] = ' ' ; 
	  } 
	  {register integer for_end; i = 1 ; for_end = dotpos ; if ( i <= 
	  for_end) do 
	    {
	      c = webname [ i ] ; 
	      pascalfilename [ i ] = c ; 
	      poolfilename [ i ] = c ; 
	    } 
	  while ( i++ < for_end ) ; } 
	  pascalfilename [ dotpos + 1 ] = 'p' ; 
	  pascalfilename [ dotpos + 2 ] = ' ' ; 
	  poolfilename [ dotpos + 1 ] = 'p' ; 
	  poolfilename [ dotpos + 2 ] = 'o' ; 
	  poolfilename [ dotpos + 3 ] = 'o' ; 
	  poolfilename [ dotpos + 4 ] = 'l' ; 
	  poolfilename [ dotpos + 5 ] = ' ' ; 
	  foundweb = true ; 
	} 
	else if ( ! foundchange ) 
	{
	  dotpos = -1 ; 
	  slashpos = -1 ; 
	  i = 1 ; 
	  while ( ( fname [ i ] != ' ' ) && ( i <= PATHMAX - 5 ) ) {
	      
	    chgname [ i ] = fname [ i ] ; 
	    if ( fname [ i ] == '.' ) 
	    dotpos = i ; 
	    if ( fname [ i ] == '/' ) 
	    slashpos = i ; 
	    i = i + 1 ; 
	  } 
	  chgname [ i ] = ' ' ; 
	  if ( ( dotpos == -1 ) || ( dotpos < slashpos ) ) 
	  {
	    dotpos = i ; 
	    chgname [ dotpos ] = '.' ; 
	    chgname [ dotpos + 1 ] = 'c' ; 
	    chgname [ dotpos + 2 ] = 'h' ; 
	    chgname [ dotpos + 3 ] = ' ' ; 
	  } 
	  foundchange = true ; 
	} 
	else {
	    
	  (void) fprintf( stdout , "%s\n",  "Usage: tangle webfile[.web] [changefile[.ch]]."           ) ; 
	  uexit ( 1 ) ; 
	} 
      } 
      else {
	  
	{
	  (void) fprintf( stdout , "%s\n",  "Usage: tangle webfile[.web] [changefile[.ch]]."           ) ; 
	  uexit ( 1 ) ; 
	} 
      } 
    } 
  while ( a++ < for_end ) ; } 
  if ( ! foundweb ) 
  {
    (void) fprintf( stdout , "%s\n",  "Usage: tangle webfile[.web] [changefile[.ch]]." ) ; 
    uexit ( 1 ) ; 
  } 
  if ( ! foundchange ) 
  {
    chgname [ 1 ] = '/' ; 
    chgname [ 2 ] = 'd' ; 
    chgname [ 3 ] = 'e' ; 
    chgname [ 4 ] = 'v' ; 
    chgname [ 5 ] = '/' ; 
    chgname [ 6 ] = 'n' ; 
    chgname [ 7 ] = 'u' ; 
    chgname [ 8 ] = 'l' ; 
    chgname [ 9 ] = 'l' ; 
    chgname [ 10 ] = ' ' ; 
  } 
} 
void initialize ( ) 
{unsigned char i  ; 
  schar wi  ; 
  schar zi  ; 
  integer h  ; 
  history = 0 ; 
  xchr [ 32 ] = ' ' ; 
  xchr [ 33 ] = '!' ; 
  xchr [ 34 ] = '"' ; 
  xchr [ 35 ] = '#' ; 
  xchr [ 36 ] = '$' ; 
  xchr [ 37 ] = '%' ; 
  xchr [ 38 ] = '&' ; 
  xchr [ 39 ] = '\'' ; 
  xchr [ 40 ] = '(' ; 
  xchr [ 41 ] = ')' ; 
  xchr [ 42 ] = '*' ; 
  xchr [ 43 ] = '+' ; 
  xchr [ 44 ] = ',' ; 
  xchr [ 45 ] = '-' ; 
  xchr [ 46 ] = '.' ; 
  xchr [ 47 ] = '/' ; 
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
  xchr [ 58 ] = ':' ; 
  xchr [ 59 ] = ';' ; 
  xchr [ 60 ] = '<' ; 
  xchr [ 61 ] = '=' ; 
  xchr [ 62 ] = '>' ; 
  xchr [ 63 ] = '?' ; 
  xchr [ 64 ] = '@' ; 
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
  xchr [ 91 ] = '[' ; 
  xchr [ 92 ] = '\\' ; 
  xchr [ 93 ] = ']' ; 
  xchr [ 94 ] = '^' ; 
  xchr [ 95 ] = '_' ; 
  xchr [ 96 ] = '`' ; 
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
  xchr [ 123 ] = '{' ; 
  xchr [ 124 ] = '|' ; 
  xchr [ 125 ] = '}' ; 
  xchr [ 126 ] = '~' ; 
  xchr [ 0 ] = ' ' ; 
  xchr [ 127 ] = ' ' ; 
  {register integer for_end; i = 1 ; for_end = 31 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 128 ; for_end = 255 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ chr ( i ) ] = 32 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 1 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  xord [ ' ' ] = 32 ; 
  scanargs () ; 
  rewrite ( Pascalfile , pascalfilename ) ; 
  {register integer for_end; wi = 0 ; for_end = 2 ; if ( wi <= for_end) do 
    {
      bytestart [ wi ] = 0 ; 
      byteptr [ wi ] = 0 ; 
    } 
  while ( wi++ < for_end ) ; } 
  bytestart [ 3 ] = 0 ; 
  nameptr = 1 ; 
  stringptr = 256 ; 
  poolchecksum = 271828L ; 
  {register integer for_end; zi = 0 ; for_end = 3 ; if ( zi <= for_end) do 
    {
      tokstart [ zi ] = 0 ; 
      tokptr [ zi ] = 0 ; 
    } 
  while ( zi++ < for_end ) ; } 
  tokstart [ 4 ] = 0 ; 
  textptr = 1 ; 
  z = 1 % 4 ; 
  ilk [ 0 ] = 0 ; 
  equiv [ 0 ] = 0 ; 
  {register integer for_end; h = 0 ; for_end = hashsize - 1 ; if ( h <= 
  for_end) do 
    {
      hash [ h ] = 0 ; 
      chophash [ h ] = 0 ; 
    } 
  while ( h++ < for_end ) ; } 
  lastunnamed = 0 ; 
  textlink [ 0 ] = 0 ; 
  scanninghex = false ; 
  modtext [ 0 ] = 32 ; 
} 
void openinput ( ) 
{reset ( webfile , webname ) ; 
  reset ( changefile , chgname ) ; 
} 
boolean zinputln ( f ) 
textfile f ; 
{register boolean Result; integer finallimit  ; 
  limit = 0 ; 
  finallimit = 0 ; 
  if ( eof ( f ) ) 
  Result = false ; 
  else {
      
    while ( ! eoln ( f ) ) {
	
      buffer [ limit ] = xord [ getc ( f ) ] ; 
      limit = limit + 1 ; 
      if ( buffer [ limit - 1 ] != 32 ) 
      finallimit = limit ; 
      if ( limit == bufsize ) 
      {
	while ( ! eoln ( f ) ) vgetc ( f ) ; 
	limit = limit - 1 ; 
	if ( finallimit > limit ) 
	finallimit = limit ; 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Input line too long" ) ; 
	} 
	loc = 0 ; 
	error () ; 
      } 
    } 
    readln ( f ) ; 
    limit = finallimit ; 
    Result = true ; 
  } 
  return(Result) ; 
} 
void zprintid ( p ) 
namepointer p ; 
{integer k  ; 
  schar w  ; 
  if ( p >= nameptr ) 
  (void) Fputs( stdout ,  "IMPOSSIBLE" ) ; 
  else {
      
    w = p % 3 ; 
    {register integer for_end; k = bytestart [ p ] ; for_end = bytestart [ p 
    + 3 ] - 1 ; if ( k <= for_end) do 
      (void) putc( xchr [ bytemem [ w ][ k ] ] ,  stdout );
    while ( k++ < for_end ) ; } 
  } 
} 
namepointer zidlookup ( t ) 
eightbits t ; 
{/* 31 32 */ register namepointer Result; eightbits c  ; 
  integer i  ; 
  integer h  ; 
  integer k  ; 
  schar w  ; 
  integer l  ; 
  namepointer p, q  ; 
  integer s  ; 
  l = idloc - idfirst ; 
  h = buffer [ idfirst ] ; 
  i = idfirst + 1 ; 
  while ( i < idloc ) {
      
    h = ( h + h + buffer [ i ] ) % hashsize ; 
    i = i + 1 ; 
  } 
  p = hash [ h ] ; 
  while ( p != 0 ) {
      
    if ( bytestart [ p + 3 ] - bytestart [ p ] == l ) 
    {
      i = idfirst ; 
      k = bytestart [ p ] ; 
      w = p % 3 ; 
      while ( ( i < idloc ) && ( buffer [ i ] == bytemem [ w ][ k ] ) ) {
	  
	i = i + 1 ; 
	k = k + 1 ; 
      } 
      if ( i == idloc ) 
      goto lab31 ; 
    } 
    p = link [ p ] ; 
  } 
  p = nameptr ; 
  link [ p ] = hash [ h ] ; 
  hash [ h ] = p ; 
  lab31: ; 
  if ( ( p == nameptr ) || ( t != 0 ) ) 
  {
    if ( ( ( p != nameptr ) && ( t != 0 ) && ( ilk [ p ] == 0 ) ) || ( ( p == 
    nameptr ) && ( t == 0 ) && ( buffer [ idfirst ] != 34 ) ) ) 
    {
      i = idfirst ; 
      s = 0 ; 
      h = 0 ; 
      while ( ( i < idloc ) && ( s < unambiglength ) ) {
	  
	if ( buffer [ i ] != 95 ) 
	{
	  if ( buffer [ i ] >= 97 ) 
	  choppedid [ s ] = buffer [ i ] - 32 ; 
	  else choppedid [ s ] = buffer [ i ] ; 
	  h = ( h + h + choppedid [ s ] ) % hashsize ; 
	  s = s + 1 ; 
	} 
	i = i + 1 ; 
      } 
      choppedid [ s ] = 0 ; 
    } 
    if ( p != nameptr ) 
    {
      if ( ilk [ p ] == 0 ) 
      {
	if ( t == 1 ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! This identifier has already appeared" ) ; 
	  error () ; 
	} 
	q = chophash [ h ] ; 
	if ( q == p ) 
	chophash [ h ] = equiv [ p ] ; 
	else {
	    
	  while ( equiv [ q ] != p ) q = equiv [ q ] ; 
	  equiv [ q ] = equiv [ p ] ; 
	} 
      } 
      else {
	  
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! This identifier was defined before" ) ; 
	error () ; 
      } 
      ilk [ p ] = t ; 
    } 
    else {
	
      if ( ( t == 0 ) && ( buffer [ idfirst ] != 34 ) ) 
      {
	q = chophash [ h ] ; 
	while ( q != 0 ) {
	    
	  {
	    k = bytestart [ q ] ; 
	    s = 0 ; 
	    w = q % 3 ; 
	    while ( ( k < bytestart [ q + 3 ] ) && ( s < unambiglength ) ) {
		
	      c = bytemem [ w ][ k ] ; 
	      if ( c != 95 ) 
	      {
		if ( choppedid [ s ] != c ) 
		goto lab32 ; 
		s = s + 1 ; 
	      } 
	      k = k + 1 ; 
	    } 
	    if ( ( k == bytestart [ q + 3 ] ) && ( choppedid [ s ] != 0 ) ) 
	    goto lab32 ; 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! Identifier conflict with " ) ; 
	    } 
	    {register integer for_end; k = bytestart [ q ] ; for_end = 
	    bytestart [ q + 3 ] - 1 ; if ( k <= for_end) do 
	      (void) putc( xchr [ bytemem [ w ][ k ] ] ,  stdout );
	    while ( k++ < for_end ) ; } 
	    error () ; 
	    q = 0 ; 
	    lab32: ; 
	  } 
	  q = equiv [ q ] ; 
	} 
	equiv [ p ] = chophash [ h ] ; 
	chophash [ h ] = p ; 
      } 
      w = nameptr % 3 ; 
      k = byteptr [ w ] ; 
      if ( k + l > maxbytes ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "byte memory" , " capacity exceeded" ) 
	; 
	error () ; 
	history = 3 ; 
	uexit ( 1 ) ; 
      } 
      if ( nameptr > maxnames - 3 ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "name" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	uexit ( 1 ) ; 
      } 
      i = idfirst ; 
      while ( i < idloc ) {
	  
	bytemem [ w ][ k ] = buffer [ i ] ; 
	k = k + 1 ; 
	i = i + 1 ; 
      } 
      byteptr [ w ] = k ; 
      bytestart [ nameptr + 3 ] = k ; 
      nameptr = nameptr + 1 ; 
      if ( buffer [ idfirst ] != 34 ) 
      ilk [ p ] = t ; 
      else {
	  
	ilk [ p ] = 1 ; 
	if ( l - doublechars == 2 ) 
	equiv [ p ] = buffer [ idfirst + 1 ] + 32768L ; 
	else {
	    
	  if ( stringptr == 256 ) 
	  rewrite ( pool , poolfilename ) ; 
	  equiv [ p ] = stringptr + 32768L ; 
	  l = l - doublechars - 1 ; 
	  if ( l > 99 ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Preprocessed string is too long" ) ; 
	    error () ; 
	  } 
	  stringptr = stringptr + 1 ; 
	  (void) fprintf( pool , "%c%c",  xchr [ 48 + l / 10 ] , xchr [ 48 + l % 10 ] ) ; 
	  poolchecksum = poolchecksum + poolchecksum + l ; 
	  while ( poolchecksum > 536870839L ) poolchecksum = poolchecksum - 
	  536870839L ; 
	  i = idfirst + 1 ; 
	  while ( i < idloc ) {
	      
	    (void) putc( xchr [ buffer [ i ] ] ,  pool );
	    poolchecksum = poolchecksum + poolchecksum + buffer [ i ] ; 
	    while ( poolchecksum > 536870839L ) poolchecksum = poolchecksum - 
	    536870839L ; 
	    if ( ( buffer [ i ] == 34 ) || ( buffer [ i ] == 64 ) ) 
	    i = i + 2 ; 
	    else i = i + 1 ; 
	  } 
	  (void) putc('\n',  pool );
	} 
      } 
    } 
  } 
  Result = p ; 
  return(Result) ; 
} 
namepointer zmodlookup ( l ) 
sixteenbits l ; 
{/* 31 */ register namepointer Result; schar c  ; 
  integer j  ; 
  integer k  ; 
  schar w  ; 
  namepointer p  ; 
  namepointer q  ; 
  c = 2 ; 
  q = 0 ; 
  p = ilk [ 0 ] ; 
  while ( p != 0 ) {
      
    {
      k = bytestart [ p ] ; 
      w = p % 3 ; 
      c = 1 ; 
      j = 1 ; 
      while ( ( k < bytestart [ p + 3 ] ) && ( j <= l ) && ( modtext [ j ] == 
      bytemem [ w ][ k ] ) ) {
	  
	k = k + 1 ; 
	j = j + 1 ; 
      } 
      if ( k == bytestart [ p + 3 ] ) 
      if ( j > l ) 
      c = 1 ; 
      else c = 4 ; 
      else if ( j > l ) 
      c = 3 ; 
      else if ( modtext [ j ] < bytemem [ w ][ k ] ) 
      c = 0 ; 
      else c = 2 ; 
    } 
    q = p ; 
    if ( c == 0 ) 
    p = link [ q ] ; 
    else if ( c == 2 ) 
    p = ilk [ q ] ; 
    else goto lab31 ; 
  } 
  w = nameptr % 3 ; 
  k = byteptr [ w ] ; 
  if ( k + l > maxbytes ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "byte memory" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    uexit ( 1 ) ; 
  } 
  if ( nameptr > maxnames - 3 ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "name" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    uexit ( 1 ) ; 
  } 
  p = nameptr ; 
  if ( c == 0 ) 
  link [ q ] = p ; 
  else ilk [ q ] = p ; 
  link [ p ] = 0 ; 
  ilk [ p ] = 0 ; 
  c = 1 ; 
  equiv [ p ] = 0 ; 
  {register integer for_end; j = 1 ; for_end = l ; if ( j <= for_end) do 
    bytemem [ w ][ k + j - 1 ] = modtext [ j ] ; 
  while ( j++ < for_end ) ; } 
  byteptr [ w ] = k + l ; 
  bytestart [ nameptr + 3 ] = k + l ; 
  nameptr = nameptr + 1 ; 
  lab31: if ( c != 1 ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Incompatible section names" ) ; 
      error () ; 
    } 
    p = 0 ; 
  } 
  Result = p ; 
  return(Result) ; 
} 
namepointer zprefixlookup ( l ) 
sixteenbits l ; 
{register namepointer Result; schar c  ; 
  integer count  ; 
  integer j  ; 
  integer k  ; 
  schar w  ; 
  namepointer p  ; 
  namepointer q  ; 
  namepointer r  ; 
  q = 0 ; 
  p = ilk [ 0 ] ; 
  count = 0 ; 
  r = 0 ; 
  while ( p != 0 ) {
      
    {
      k = bytestart [ p ] ; 
      w = p % 3 ; 
      c = 1 ; 
      j = 1 ; 
      while ( ( k < bytestart [ p + 3 ] ) && ( j <= l ) && ( modtext [ j ] == 
      bytemem [ w ][ k ] ) ) {
	  
	k = k + 1 ; 
	j = j + 1 ; 
      } 
      if ( k == bytestart [ p + 3 ] ) 
      if ( j > l ) 
      c = 1 ; 
      else c = 4 ; 
      else if ( j > l ) 
      c = 3 ; 
      else if ( modtext [ j ] < bytemem [ w ][ k ] ) 
      c = 0 ; 
      else c = 2 ; 
    } 
    if ( c == 0 ) 
    p = link [ p ] ; 
    else if ( c == 2 ) 
    p = ilk [ p ] ; 
    else {
	
      r = p ; 
      count = count + 1 ; 
      q = ilk [ p ] ; 
      p = link [ p ] ; 
    } 
    if ( p == 0 ) 
    {
      p = q ; 
      q = 0 ; 
    } 
  } 
  if ( count != 1 ) 
  if ( count == 0 ) 
  {
    (void) putc('\n',  stdout );
    (void) Fputs( stdout ,  "! Name does not match" ) ; 
    error () ; 
  } 
  else {
      
    (void) putc('\n',  stdout );
    (void) Fputs( stdout ,  "! Ambiguous prefix" ) ; 
    error () ; 
  } 
  Result = r ; 
  return(Result) ; 
} 
void zstoretwobytes ( x ) 
sixteenbits x ; 
{if ( tokptr [ z ] + 2 > maxtoks ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    uexit ( 1 ) ; 
  } 
  tokmem [ z ][ tokptr [ z ] ] = x / 256 ; 
  tokmem [ z ][ tokptr [ z ] + 1 ] = x % 256 ; 
  tokptr [ z ] = tokptr [ z ] + 2 ; 
} 
void zpushlevel ( p ) 
namepointer p ; 
{if ( stackptr == stacksize ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "stack" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    uexit ( 1 ) ; 
  } 
  else {
      
    stack [ stackptr ] = curstate ; 
    stackptr = stackptr + 1 ; 
    curstate .namefield = p ; 
    curstate .replfield = equiv [ p ] ; 
    zo = curstate .replfield % 4 ; 
    curstate .bytefield = tokstart [ curstate .replfield ] ; 
    curstate .endfield = tokstart [ curstate .replfield + 4 ] ; 
    curstate .modfield = 0 ; 
  } 
} 
void poplevel ( ) 
{/* 10 */ if ( textlink [ curstate .replfield ] == 0 ) 
  {
    if ( ilk [ curstate .namefield ] == 3 ) 
    {
      nameptr = nameptr - 1 ; 
      textptr = textptr - 1 ; 
      z = textptr % 4 ; 
      tokptr [ z ] = tokstart [ textptr ] ; 
    } 
  } 
  else if ( textlink [ curstate .replfield ] < maxtexts ) 
  {
    curstate .replfield = textlink [ curstate .replfield ] ; 
    zo = curstate .replfield % 4 ; 
    curstate .bytefield = tokstart [ curstate .replfield ] ; 
    curstate .endfield = tokstart [ curstate .replfield + 4 ] ; 
    goto lab10 ; 
  } 
  stackptr = stackptr - 1 ; 
  if ( stackptr > 0 ) 
  {
    curstate = stack [ stackptr ] ; 
    zo = curstate .replfield % 4 ; 
  } 
  lab10: ; 
} 
sixteenbits getoutput ( ) 
{/* 20 30 31 */ register sixteenbits Result; sixteenbits a  ; 
  eightbits b  ; 
  sixteenbits bal  ; 
  integer k  ; 
  schar w  ; 
  lab20: if ( stackptr == 0 ) 
  {
    a = 0 ; 
    goto lab31 ; 
  } 
  if ( curstate .bytefield == curstate .endfield ) 
  {
    curval = - (integer) curstate .modfield ; 
    poplevel () ; 
    if ( curval == 0 ) 
    goto lab20 ; 
    a = 129 ; 
    goto lab31 ; 
  } 
  a = tokmem [ zo ][ curstate .bytefield ] ; 
  curstate .bytefield = curstate .bytefield + 1 ; 
  if ( a < 128 ) 
  if ( a == 0 ) 
  {
    pushlevel ( nameptr - 1 ) ; 
    goto lab20 ; 
  } 
  else goto lab31 ; 
  a = ( a - 128 ) * 256 + tokmem [ zo ][ curstate .bytefield ] ; 
  curstate .bytefield = curstate .bytefield + 1 ; 
  if ( a < 10240 ) 
  {
    switch ( ilk [ a ] ) 
    {case 0 : 
      {
	curval = a ; 
	a = 130 ; 
      } 
      break ; 
    case 1 : 
      {
	curval = equiv [ a ] - 32768L ; 
	a = 128 ; 
      } 
      break ; 
    case 2 : 
      {
	pushlevel ( a ) ; 
	goto lab20 ; 
      } 
      break ; 
    case 3 : 
      {
	while ( ( curstate .bytefield == curstate .endfield ) && ( stackptr > 
	0 ) ) poplevel () ; 
	if ( ( stackptr == 0 ) || ( tokmem [ zo ][ curstate .bytefield ] != 40 
	) ) 
	{
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! No parameter given for " ) ; 
	  } 
	  printid ( a ) ; 
	  error () ; 
	  goto lab20 ; 
	} 
	bal = 1 ; 
	curstate .bytefield = curstate .bytefield + 1 ; 
	while ( true ) {
	    
	  b = tokmem [ zo ][ curstate .bytefield ] ; 
	  curstate .bytefield = curstate .bytefield + 1 ; 
	  if ( b == 0 ) 
	  storetwobytes ( nameptr + 32767 ) ; 
	  else {
	      
	    if ( b >= 128 ) 
	    {
	      {
		if ( tokptr [ z ] == maxtoks ) 
		{
		  (void) putc('\n',  stdout );
		  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" ,                   " capacity exceeded" ) ; 
		  error () ; 
		  history = 3 ; 
		  uexit ( 1 ) ; 
		} 
		tokmem [ z ][ tokptr [ z ] ] = b ; 
		tokptr [ z ] = tokptr [ z ] + 1 ; 
	      } 
	      b = tokmem [ zo ][ curstate .bytefield ] ; 
	      curstate .bytefield = curstate .bytefield + 1 ; 
	    } 
	    else switch ( b ) 
	    {case 40 : 
	      bal = bal + 1 ; 
	      break ; 
	    case 41 : 
	      {
		bal = bal - 1 ; 
		if ( bal == 0 ) 
		goto lab30 ; 
	      } 
	      break ; 
	    case 39 : 
	      do {
		  { 
		  if ( tokptr [ z ] == maxtoks ) 
		  {
		    (void) putc('\n',  stdout );
		    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" ,                     " capacity exceeded" ) ; 
		    error () ; 
		    history = 3 ; 
		    uexit ( 1 ) ; 
		  } 
		  tokmem [ z ][ tokptr [ z ] ] = b ; 
		  tokptr [ z ] = tokptr [ z ] + 1 ; 
		} 
		b = tokmem [ zo ][ curstate .bytefield ] ; 
		curstate .bytefield = curstate .bytefield + 1 ; 
	      } while ( ! ( b == 39 ) ) ; 
	      break ; 
	      default: 
	      ; 
	      break ; 
	    } 
	    {
	      if ( tokptr [ z ] == maxtoks ) 
	      {
		(void) putc('\n',  stdout );
		(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded"                 ) ; 
		error () ; 
		history = 3 ; 
		uexit ( 1 ) ; 
	      } 
	      tokmem [ z ][ tokptr [ z ] ] = b ; 
	      tokptr [ z ] = tokptr [ z ] + 1 ; 
	    } 
	  } 
	} 
	lab30: ; 
	equiv [ nameptr ] = textptr ; 
	ilk [ nameptr ] = 2 ; 
	w = nameptr % 3 ; 
	k = byteptr [ w ] ; 
	if ( nameptr > maxnames - 3 ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "name" , " capacity exceeded" ) ; 
	  error () ; 
	  history = 3 ; 
	  uexit ( 1 ) ; 
	} 
	bytestart [ nameptr + 3 ] = k ; 
	nameptr = nameptr + 1 ; 
	if ( textptr > maxtexts - 4 ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "text" , " capacity exceeded" ) ; 
	  error () ; 
	  history = 3 ; 
	  uexit ( 1 ) ; 
	} 
	textlink [ textptr ] = 0 ; 
	tokstart [ textptr + 4 ] = tokptr [ z ] ; 
	textptr = textptr + 1 ; 
	z = textptr % 4 ; 
	pushlevel ( a ) ; 
	goto lab20 ; 
      } 
      break ; 
      default: 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%c",  "! This can't happen (" , "output" , ')' ) ; 
	error () ; 
	history = 3 ; 
	uexit ( 1 ) ; 
      } 
      break ; 
    } 
    goto lab31 ; 
  } 
  if ( a < 20480 ) 
  {
    a = a - 10240 ; 
    if ( equiv [ a ] != 0 ) 
    pushlevel ( a ) ; 
    else if ( a != 0 ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Not present: <" ) ; 
      } 
      printid ( a ) ; 
      (void) putc( '>' ,  stdout );
      error () ; 
    } 
    goto lab20 ; 
  } 
  curval = a - 20480 ; 
  a = 129 ; 
  curstate .modfield = curval ; 
  lab31: Result = a ; 
  return(Result) ; 
} 
void flushbuffer ( ) 
{integer k  ; 
  integer b  ; 
  b = breakptr ; 
  if ( ( semiptr != 0 ) && ( outptr - semiptr <= linelength ) ) 
  breakptr = semiptr ; 
  {register integer for_end; k = 1 ; for_end = breakptr ; if ( k <= for_end) 
  do 
    (void) putc( xchr [ outbuf [ k - 1 ] ] ,  Pascalfile );
  while ( k++ < for_end ) ; } 
  (void) putc('\n',  Pascalfile );
  line = line + 1 ; 
  if ( line % 100 == 0 ) 
  {
    (void) putc( '.' ,  stdout );
    if ( line % 500 == 0 ) 
    (void) fprintf( stdout , "%ld",  (long)line ) ; 
    flush ( stdout ) ; 
  } 
  if ( breakptr < outptr ) 
  {
    if ( outbuf [ breakptr ] == 32 ) 
    {
      breakptr = breakptr + 1 ; 
      if ( breakptr > b ) 
      b = breakptr ; 
    } 
    {register integer for_end; k = breakptr ; for_end = outptr - 1 ; if ( k 
    <= for_end) do 
      outbuf [ k - breakptr ] = outbuf [ k ] ; 
    while ( k++ < for_end ) ; } 
  } 
  outptr = outptr - breakptr ; 
  breakptr = b - breakptr ; 
  semiptr = 0 ; 
  if ( outptr > linelength ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Long line must be truncated" ) ; 
      error () ; 
    } 
    outptr = linelength ; 
  } 
} 
void zappval ( v ) 
integer v ; 
{integer k  ; 
  k = outbufsize ; 
  do {
      outbuf [ k ] = v % 10 ; 
    v = v / 10 ; 
    k = k - 1 ; 
  } while ( ! ( v == 0 ) ) ; 
  do {
      k = k + 1 ; 
    {
      outbuf [ outptr ] = outbuf [ k ] + 48 ; 
      outptr = outptr + 1 ; 
    } 
  } while ( ! ( k == outbufsize ) ) ; 
} 
void zsendout ( t , v ) 
eightbits t ; 
sixteenbits v ; 
{/* 20 */ integer k  ; 
  lab20: switch ( outstate ) 
  {case 1 : 
    if ( t != 3 ) 
    {
      breakptr = outptr ; 
      if ( t == 2 ) 
      {
	outbuf [ outptr ] = 32 ; 
	outptr = outptr + 1 ; 
      } 
    } 
    break ; 
  case 2 : 
    {
      {
	outbuf [ outptr ] = 44 - outapp ; 
	outptr = outptr + 1 ; 
      } 
      if ( outptr > linelength ) 
      flushbuffer () ; 
      breakptr = outptr ; 
    } 
    break ; 
  case 3 : 
  case 4 : 
    {
      if ( ( outval < 0 ) || ( ( outval == 0 ) && ( lastsign < 0 ) ) ) 
      {
	outbuf [ outptr ] = 45 ; 
	outptr = outptr + 1 ; 
      } 
      else if ( outsign > 0 ) 
      {
	outbuf [ outptr ] = outsign ; 
	outptr = outptr + 1 ; 
      } 
      appval ( abs ( outval ) ) ; 
      if ( outptr > linelength ) 
      flushbuffer () ; 
      outstate = outstate - 2 ; 
      goto lab20 ; 
    } 
    break ; 
  case 5 : 
    {
      if ( ( t == 3 ) || ( ( ( t == 2 ) && ( v == 3 ) && ( ( ( outcontrib [ 1 
      ] == 68 ) && ( outcontrib [ 2 ] == 73 ) && ( outcontrib [ 3 ] == 86 ) ) 
      || ( ( outcontrib [ 1 ] == 100 ) && ( outcontrib [ 2 ] == 105 ) && ( 
      outcontrib [ 3 ] == 118 ) ) || ( ( outcontrib [ 1 ] == 77 ) && ( 
      outcontrib [ 2 ] == 79 ) && ( outcontrib [ 3 ] == 68 ) ) || ( ( 
      outcontrib [ 1 ] == 109 ) && ( outcontrib [ 2 ] == 111 ) && ( outcontrib 
      [ 3 ] == 100 ) ) ) ) || ( ( t == 0 ) && ( ( v == 42 ) || ( v == 47 ) ) ) 
      ) ) 
      {
	if ( ( outval < 0 ) || ( ( outval == 0 ) && ( lastsign < 0 ) ) ) 
	{
	  outbuf [ outptr ] = 45 ; 
	  outptr = outptr + 1 ; 
	} 
	else if ( outsign > 0 ) 
	{
	  outbuf [ outptr ] = outsign ; 
	  outptr = outptr + 1 ; 
	} 
	appval ( abs ( outval ) ) ; 
	if ( outptr > linelength ) 
	flushbuffer () ; 
	outsign = 43 ; 
	outval = outapp ; 
      } 
      else outval = outval + outapp ; 
      outstate = 3 ; 
      goto lab20 ; 
    } 
    break ; 
  case 0 : 
    if ( t != 3 ) 
    breakptr = outptr ; 
    break ; 
    default: 
    ; 
    break ; 
  } 
  if ( t != 0 ) 
  {register integer for_end; k = 1 ; for_end = v ; if ( k <= for_end) do 
    {
      outbuf [ outptr ] = outcontrib [ k ] ; 
      outptr = outptr + 1 ; 
    } 
  while ( k++ < for_end ) ; } 
  else {
      
    outbuf [ outptr ] = v ; 
    outptr = outptr + 1 ; 
  } 
  if ( outptr > linelength ) 
  flushbuffer () ; 
  if ( ( t == 0 ) && ( ( v == 59 ) || ( v == 125 ) ) ) 
  {
    semiptr = outptr ; 
    breakptr = outptr ; 
  } 
  if ( t >= 2 ) 
  outstate = 1 ; 
  else outstate = 0 ; 
} 
void zsendsign ( v ) 
integer v ; 
{switch ( outstate ) 
  {case 2 : 
  case 4 : 
    outapp = outapp * v ; 
    break ; 
  case 3 : 
    {
      outapp = v ; 
      outstate = 4 ; 
    } 
    break ; 
  case 5 : 
    {
      outval = outval + outapp ; 
      outapp = v ; 
      outstate = 4 ; 
    } 
    break ; 
    default: 
    {
      breakptr = outptr ; 
      outapp = v ; 
      outstate = 2 ; 
    } 
    break ; 
  } 
  lastsign = outapp ; 
} 
void zsendval ( v ) 
integer v ; 
{/* 666 10 */ switch ( outstate ) 
  {case 1 : 
    {
      if ( ( outptr == breakptr + 3 ) || ( ( outptr == breakptr + 4 ) && ( 
      outbuf [ breakptr ] == 32 ) ) ) 
      if ( ( ( outbuf [ outptr - 3 ] == 68 ) && ( outbuf [ outptr - 2 ] == 73 
      ) && ( outbuf [ outptr - 1 ] == 86 ) ) || ( ( outbuf [ outptr - 3 ] == 
      100 ) && ( outbuf [ outptr - 2 ] == 105 ) && ( outbuf [ outptr - 1 ] == 
      118 ) ) || ( ( outbuf [ outptr - 3 ] == 77 ) && ( outbuf [ outptr - 2 ] 
      == 79 ) && ( outbuf [ outptr - 1 ] == 68 ) ) || ( ( outbuf [ outptr - 3 
      ] == 109 ) && ( outbuf [ outptr - 2 ] == 111 ) && ( outbuf [ outptr - 1 
      ] == 100 ) ) ) 
      goto lab666 ; 
      outsign = 32 ; 
      outstate = 3 ; 
      outval = v ; 
      breakptr = outptr ; 
      lastsign = 1 ; 
    } 
    break ; 
  case 0 : 
    {
      if ( ( outptr == breakptr + 1 ) && ( ( outbuf [ breakptr ] == 42 ) || ( 
      outbuf [ breakptr ] == 47 ) ) ) 
      goto lab666 ; 
      outsign = 0 ; 
      outstate = 3 ; 
      outval = v ; 
      breakptr = outptr ; 
      lastsign = 1 ; 
    } 
    break ; 
  case 2 : 
    {
      outsign = 43 ; 
      outstate = 3 ; 
      outval = outapp * v ; 
    } 
    break ; 
  case 3 : 
    {
      outstate = 5 ; 
      outapp = v ; 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Two numbers occurred without a sign between them"         ) ; 
	error () ; 
      } 
    } 
    break ; 
  case 4 : 
    {
      outstate = 5 ; 
      outapp = outapp * v ; 
    } 
    break ; 
  case 5 : 
    {
      outval = outval + outapp ; 
      outapp = v ; 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Two numbers occurred without a sign between them"         ) ; 
	error () ; 
      } 
    } 
    break ; 
    default: 
    goto lab666 ; 
    break ; 
  } 
  goto lab10 ; 
  lab666: if ( v >= 0 ) 
  {
    if ( outstate == 1 ) 
    {
      breakptr = outptr ; 
      {
	outbuf [ outptr ] = 32 ; 
	outptr = outptr + 1 ; 
      } 
    } 
    appval ( v ) ; 
    if ( outptr > linelength ) 
    flushbuffer () ; 
    outstate = 1 ; 
  } 
  else {
      
    {
      outbuf [ outptr ] = 40 ; 
      outptr = outptr + 1 ; 
    } 
    {
      outbuf [ outptr ] = 45 ; 
      outptr = outptr + 1 ; 
    } 
    appval ( - (integer) v ) ; 
    {
      outbuf [ outptr ] = 41 ; 
      outptr = outptr + 1 ; 
    } 
    if ( outptr > linelength ) 
    flushbuffer () ; 
    outstate = 0 ; 
  } 
  lab10: ; 
} 
void sendtheoutput ( ) 
{/* 2 21 22 */ eightbits curchar  ; 
  integer k  ; 
  integer j  ; 
  schar w  ; 
  integer n  ; 
  while ( stackptr > 0 ) {
      
    curchar = getoutput () ; 
    lab21: switch ( curchar ) 
    {case 0 : 
      ; 
      break ; 
    case 65 : 
    case 66 : 
    case 67 : 
    case 68 : 
    case 69 : 
    case 70 : 
    case 71 : 
    case 72 : 
    case 73 : 
    case 74 : 
    case 75 : 
    case 76 : 
    case 77 : 
    case 78 : 
    case 79 : 
    case 80 : 
    case 81 : 
    case 82 : 
    case 83 : 
    case 84 : 
    case 85 : 
    case 86 : 
    case 87 : 
    case 88 : 
    case 89 : 
    case 90 : 
    case 97 : 
    case 98 : 
    case 99 : 
    case 100 : 
    case 101 : 
    case 102 : 
    case 103 : 
    case 104 : 
    case 105 : 
    case 106 : 
    case 107 : 
    case 108 : 
    case 109 : 
    case 110 : 
    case 111 : 
    case 112 : 
    case 113 : 
    case 114 : 
    case 115 : 
    case 116 : 
    case 117 : 
    case 118 : 
    case 119 : 
    case 120 : 
    case 121 : 
    case 122 : 
      {
	outcontrib [ 1 ] = curchar ; 
	sendout ( 2 , 1 ) ; 
      } 
      break ; 
    case 130 : 
      {
	k = 0 ; 
	j = bytestart [ curval ] ; 
	w = curval % 3 ; 
	while ( ( k < maxidlength ) && ( j < bytestart [ curval + 3 ] ) ) {
	    
	  k = k + 1 ; 
	  outcontrib [ k ] = bytemem [ w ][ j ] ; 
	  j = j + 1 ; 
	  if ( outcontrib [ k ] == 95 ) 
	  k = k - 1 ; 
	} 
	sendout ( 2 , k ) ; 
      } 
      break ; 
    case 48 : 
    case 49 : 
    case 50 : 
    case 51 : 
    case 52 : 
    case 53 : 
    case 54 : 
    case 55 : 
    case 56 : 
    case 57 : 
      {
	n = 0 ; 
	do {
	    curchar = curchar - 48 ; 
	  if ( n >= 214748364L ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Constant too big" ) ; 
	    error () ; 
	  } 
	  else n = 10 * n + curchar ; 
	  curchar = getoutput () ; 
	} while ( ! ( ( curchar > 57 ) || ( curchar < 48 ) ) ) ; 
	sendval ( n ) ; 
	k = 0 ; 
	if ( curchar == 101 ) 
	curchar = 69 ; 
	if ( curchar == 69 ) 
	goto lab2 ; 
	else goto lab21 ; 
      } 
      break ; 
    case 125 : 
      sendval ( poolchecksum ) ; 
      break ; 
    case 12 : 
      {
	n = 0 ; 
	curchar = 48 ; 
	do {
	    curchar = curchar - 48 ; 
	  if ( n >= 268435456L ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Constant too big" ) ; 
	    error () ; 
	  } 
	  else n = 8 * n + curchar ; 
	  curchar = getoutput () ; 
	} while ( ! ( ( curchar > 55 ) || ( curchar < 48 ) ) ) ; 
	sendval ( n ) ; 
	goto lab21 ; 
      } 
      break ; 
    case 13 : 
      {
	n = 0 ; 
	curchar = 48 ; 
	do {
	    if ( curchar >= 65 ) 
	  curchar = curchar - 55 ; 
	  else curchar = curchar - 48 ; 
	  if ( n >= 134217728L ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Constant too big" ) ; 
	    error () ; 
	  } 
	  else n = 16 * n + curchar ; 
	  curchar = getoutput () ; 
	} while ( ! ( ( curchar > 70 ) || ( curchar < 48 ) || ( ( curchar > 57 
	) && ( curchar < 65 ) ) ) ) ; 
	sendval ( n ) ; 
	goto lab21 ; 
      } 
      break ; 
    case 128 : 
      sendval ( curval ) ; 
      break ; 
    case 46 : 
      {
	k = 1 ; 
	outcontrib [ 1 ] = 46 ; 
	curchar = getoutput () ; 
	if ( curchar == 46 ) 
	{
	  outcontrib [ 2 ] = 46 ; 
	  sendout ( 1 , 2 ) ; 
	} 
	else if ( ( curchar >= 48 ) && ( curchar <= 57 ) ) 
	goto lab2 ; 
	else {
	    
	  sendout ( 0 , 46 ) ; 
	  goto lab21 ; 
	} 
      } 
      break ; 
    case 43 : 
    case 45 : 
      sendsign ( 44 - curchar ) ; 
      break ; 
    case 4 : 
      {
	outcontrib [ 1 ] = 97 ; 
	outcontrib [ 2 ] = 110 ; 
	outcontrib [ 3 ] = 100 ; 
	sendout ( 2 , 3 ) ; 
      } 
      break ; 
    case 5 : 
      {
	outcontrib [ 1 ] = 110 ; 
	outcontrib [ 2 ] = 111 ; 
	outcontrib [ 3 ] = 116 ; 
	sendout ( 2 , 3 ) ; 
      } 
      break ; 
    case 6 : 
      {
	outcontrib [ 1 ] = 105 ; 
	outcontrib [ 2 ] = 110 ; 
	sendout ( 2 , 2 ) ; 
      } 
      break ; 
    case 31 : 
      {
	outcontrib [ 1 ] = 111 ; 
	outcontrib [ 2 ] = 114 ; 
	sendout ( 2 , 2 ) ; 
      } 
      break ; 
    case 24 : 
      {
	outcontrib [ 1 ] = 58 ; 
	outcontrib [ 2 ] = 61 ; 
	sendout ( 1 , 2 ) ; 
      } 
      break ; 
    case 26 : 
      {
	outcontrib [ 1 ] = 60 ; 
	outcontrib [ 2 ] = 62 ; 
	sendout ( 1 , 2 ) ; 
      } 
      break ; 
    case 28 : 
      {
	outcontrib [ 1 ] = 60 ; 
	outcontrib [ 2 ] = 61 ; 
	sendout ( 1 , 2 ) ; 
      } 
      break ; 
    case 29 : 
      {
	outcontrib [ 1 ] = 62 ; 
	outcontrib [ 2 ] = 61 ; 
	sendout ( 1 , 2 ) ; 
      } 
      break ; 
    case 30 : 
      {
	outcontrib [ 1 ] = 61 ; 
	outcontrib [ 2 ] = 61 ; 
	sendout ( 1 , 2 ) ; 
      } 
      break ; 
    case 32 : 
      {
	outcontrib [ 1 ] = 46 ; 
	outcontrib [ 2 ] = 46 ; 
	sendout ( 1 , 2 ) ; 
      } 
      break ; 
    case 39 : 
      {
	k = 1 ; 
	outcontrib [ 1 ] = 39 ; 
	do {
	    if ( k < linelength ) 
	  k = k + 1 ; 
	  outcontrib [ k ] = getoutput () ; 
	} while ( ! ( ( outcontrib [ k ] == 39 ) || ( stackptr == 0 ) ) ) ; 
	if ( k == linelength ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! String too long" ) ; 
	  error () ; 
	} 
	sendout ( 1 , k ) ; 
	curchar = getoutput () ; 
	if ( curchar == 39 ) 
	outstate = 6 ; 
	goto lab21 ; 
      } 
      break ; 
    case 33 : 
    case 34 : 
    case 35 : 
    case 36 : 
    case 37 : 
    case 38 : 
    case 40 : 
    case 41 : 
    case 42 : 
    case 44 : 
    case 47 : 
    case 58 : 
    case 59 : 
    case 60 : 
    case 61 : 
    case 62 : 
    case 63 : 
    case 64 : 
    case 91 : 
    case 92 : 
    case 93 : 
    case 94 : 
    case 95 : 
    case 96 : 
    case 123 : 
    case 124 : 
      sendout ( 0 , curchar ) ; 
      break ; 
    case 9 : 
      {
	if ( bracelevel == 0 ) 
	sendout ( 0 , 123 ) ; 
	else sendout ( 0 , 91 ) ; 
	bracelevel = bracelevel + 1 ; 
      } 
      break ; 
    case 10 : 
      if ( bracelevel > 0 ) 
      {
	bracelevel = bracelevel - 1 ; 
	if ( bracelevel == 0 ) 
	sendout ( 0 , 125 ) ; 
	else sendout ( 0 , 93 ) ; 
      } 
      else {
	  
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Extra @}" ) ; 
	error () ; 
      } 
      break ; 
    case 129 : 
      {
	if ( bracelevel == 0 ) 
	sendout ( 0 , 123 ) ; 
	else sendout ( 0 , 91 ) ; 
	if ( curval < 0 ) 
	{
	  sendout ( 0 , 58 ) ; 
	  sendval ( - (integer) curval ) ; 
	} 
	else {
	    
	  sendval ( curval ) ; 
	  sendout ( 0 , 58 ) ; 
	} 
	if ( bracelevel == 0 ) 
	sendout ( 0 , 125 ) ; 
	else sendout ( 0 , 93 ) ; 
      } 
      break ; 
    case 127 : 
      {
	sendout ( 3 , 0 ) ; 
	outstate = 6 ; 
      } 
      break ; 
    case 2 : 
      {
	k = 0 ; 
	do {
	    if ( k < linelength ) 
	  k = k + 1 ; 
	  outcontrib [ k ] = getoutput () ; 
	} while ( ! ( ( outcontrib [ k ] == 2 ) || ( stackptr == 0 ) ) ) ; 
	if ( k == linelength ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Verbatim string too long" ) ; 
	  error () ; 
	} 
	sendout ( 1 , k - 1 ) ; 
      } 
      break ; 
    case 3 : 
      {
	sendout ( 1 , 0 ) ; 
	while ( outptr > 0 ) {
	    
	  if ( outptr <= linelength ) 
	  breakptr = outptr ; 
	  flushbuffer () ; 
	} 
	outstate = 0 ; 
      } 
      break ; 
      default: 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%ld",  "! Can't output ASCII code " , (long)curchar ) ; 
	error () ; 
      } 
      break ; 
    } 
    goto lab22 ; 
    lab2: do {
	if ( k < linelength ) 
      k = k + 1 ; 
      outcontrib [ k ] = curchar ; 
      curchar = getoutput () ; 
      if ( ( outcontrib [ k ] == 69 ) && ( ( curchar == 43 ) || ( curchar == 
      45 ) ) ) 
      {
	if ( k < linelength ) 
	k = k + 1 ; 
	outcontrib [ k ] = curchar ; 
	curchar = getoutput () ; 
      } 
      else if ( curchar == 101 ) 
      curchar = 69 ; 
    } while ( ! ( ( curchar != 69 ) && ( ( curchar < 48 ) || ( curchar > 57 ) 
    ) ) ) ; 
    if ( k == linelength ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Fraction too long" ) ; 
      error () ; 
    } 
    sendout ( 3 , k ) ; 
    goto lab21 ; 
    lab22: ; 
  } 
} 
boolean linesdontmatch ( ) 
{/* 10 */ register boolean Result; integer k  ; 
  Result = true ; 
  if ( changelimit != limit ) 
  goto lab10 ; 
  if ( limit > 0 ) 
  {register integer for_end; k = 0 ; for_end = limit - 1 ; if ( k <= for_end) 
  do 
    if ( changebuffer [ k ] != buffer [ k ] ) 
    goto lab10 ; 
  while ( k++ < for_end ) ; } 
  Result = false ; 
  lab10: ; 
  return(Result) ; 
} 
void primethechangebuffer ( ) 
{/* 22 30 10 */ integer k  ; 
  changelimit = 0 ; 
  while ( true ) {
      
    line = line + 1 ; 
    if ( ! inputln ( changefile ) ) 
    goto lab10 ; 
    if ( limit < 2 ) 
    goto lab22 ; 
    if ( buffer [ 0 ] != 64 ) 
    goto lab22 ; 
    if ( ( buffer [ 1 ] >= 88 ) && ( buffer [ 1 ] <= 90 ) ) 
    buffer [ 1 ] = buffer [ 1 ] + 32 ; 
    if ( buffer [ 1 ] == 120 ) 
    goto lab30 ; 
    if ( ( buffer [ 1 ] == 121 ) || ( buffer [ 1 ] == 122 ) ) 
    {
      loc = 2 ; 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Where is the matching @x?" ) ; 
	error () ; 
      } 
    } 
    lab22: ; 
  } 
  lab30: ; 
  do {
      line = line + 1 ; 
    if ( ! inputln ( changefile ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Change file ended after @x" ) ; 
	error () ; 
      } 
      goto lab10 ; 
    } 
  } while ( ! ( limit > 0 ) ) ; 
  {
    changelimit = limit ; 
    if ( limit > 0 ) 
    {register integer for_end; k = 0 ; for_end = limit - 1 ; if ( k <= 
    for_end) do 
      changebuffer [ k ] = buffer [ k ] ; 
    while ( k++ < for_end ) ; } 
  } 
  lab10: ; 
} 
void checkchange ( ) 
{/* 10 */ integer n  ; 
  integer k  ; 
  if ( linesdontmatch () ) 
  goto lab10 ; 
  n = 0 ; 
  while ( true ) {
      
    changing = ! changing ; 
    templine = otherline ; 
    otherline = line ; 
    line = templine ; 
    line = line + 1 ; 
    if ( ! inputln ( changefile ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Change file ended before @y" ) ; 
	error () ; 
      } 
      changelimit = 0 ; 
      changing = ! changing ; 
      templine = otherline ; 
      otherline = line ; 
      line = templine ; 
      goto lab10 ; 
    } 
    if ( limit > 1 ) 
    if ( buffer [ 0 ] == 64 ) 
    {
      if ( ( buffer [ 1 ] >= 88 ) && ( buffer [ 1 ] <= 90 ) ) 
      buffer [ 1 ] = buffer [ 1 ] + 32 ; 
      if ( ( buffer [ 1 ] == 120 ) || ( buffer [ 1 ] == 122 ) ) 
      {
	loc = 2 ; 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Where is the matching @y?" ) ; 
	  error () ; 
	} 
      } 
      else if ( buffer [ 1 ] == 121 ) 
      {
	if ( n > 0 ) 
	{
	  loc = 2 ; 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%s%ld%s",  "! Hmm... " , (long)n ,             " of the preceding lines failed to match" ) ; 
	    error () ; 
	  } 
	} 
	goto lab10 ; 
      } 
    } 
    {
      changelimit = limit ; 
      if ( limit > 0 ) 
      {register integer for_end; k = 0 ; for_end = limit - 1 ; if ( k <= 
      for_end) do 
	changebuffer [ k ] = buffer [ k ] ; 
      while ( k++ < for_end ) ; } 
    } 
    changing = ! changing ; 
    templine = otherline ; 
    otherline = line ; 
    line = templine ; 
    line = line + 1 ; 
    if ( ! inputln ( webfile ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! WEB file ended during a change" ) ; 
	error () ; 
      } 
      inputhasended = true ; 
      goto lab10 ; 
    } 
    if ( linesdontmatch () ) 
    n = n + 1 ; 
  } 
  lab10: ; 
} 
void getline ( ) 
{/* 20 */ lab20: if ( changing ) 
  {
    line = line + 1 ; 
    if ( ! inputln ( changefile ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Change file ended without @z" ) ; 
	error () ; 
      } 
      buffer [ 0 ] = 64 ; 
      buffer [ 1 ] = 122 ; 
      limit = 2 ; 
    } 
    if ( limit > 1 ) 
    if ( buffer [ 0 ] == 64 ) 
    {
      if ( ( buffer [ 1 ] >= 88 ) && ( buffer [ 1 ] <= 90 ) ) 
      buffer [ 1 ] = buffer [ 1 ] + 32 ; 
      if ( ( buffer [ 1 ] == 120 ) || ( buffer [ 1 ] == 121 ) ) 
      {
	loc = 2 ; 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Where is the matching @z?" ) ; 
	  error () ; 
	} 
      } 
      else if ( buffer [ 1 ] == 122 ) 
      {
	primethechangebuffer () ; 
	changing = ! changing ; 
	templine = otherline ; 
	otherline = line ; 
	line = templine ; 
      } 
    } 
  } 
  if ( ! changing ) 
  {
    {
      line = line + 1 ; 
      if ( ! inputln ( webfile ) ) 
      inputhasended = true ; 
      else if ( limit == changelimit ) 
      if ( buffer [ 0 ] == changebuffer [ 0 ] ) 
      if ( changelimit > 0 ) 
      checkchange () ; 
    } 
    if ( changing ) 
    goto lab20 ; 
  } 
  loc = 0 ; 
  buffer [ limit ] = 32 ; 
} 
eightbits zcontrolcode ( c ) 
ASCIIcode c ; 
{register eightbits Result; switch ( c ) 
  {case 64 : 
    Result = 64 ; 
    break ; 
  case 39 : 
    Result = 12 ; 
    break ; 
  case 34 : 
    Result = 13 ; 
    break ; 
  case 36 : 
    Result = 125 ; 
    break ; 
  case 32 : 
  case 9 : 
    Result = 136 ; 
    break ; 
  case 42 : 
    {
      (void) fprintf( stdout , "%c%ld",  '*' , (long)modulecount + 1 ) ; 
      flush ( stdout ) ; 
      Result = 136 ; 
    } 
    break ; 
  case 68 : 
  case 100 : 
    Result = 133 ; 
    break ; 
  case 70 : 
  case 102 : 
    Result = 132 ; 
    break ; 
  case 123 : 
    Result = 9 ; 
    break ; 
  case 125 : 
    Result = 10 ; 
    break ; 
  case 80 : 
  case 112 : 
    Result = 134 ; 
    break ; 
  case 84 : 
  case 116 : 
  case 94 : 
  case 46 : 
  case 58 : 
    Result = 131 ; 
    break ; 
  case 38 : 
    Result = 127 ; 
    break ; 
  case 60 : 
    Result = 135 ; 
    break ; 
  case 61 : 
    Result = 2 ; 
    break ; 
  case 92 : 
    Result = 3 ; 
    break ; 
    default: 
    Result = 0 ; 
    break ; 
  } 
  return(Result) ; 
} 
eightbits skipahead ( ) 
{/* 30 */ register eightbits Result; eightbits c  ; 
  while ( true ) {
      
    if ( loc > limit ) 
    {
      getline () ; 
      if ( inputhasended ) 
      {
	c = 136 ; 
	goto lab30 ; 
      } 
    } 
    buffer [ limit + 1 ] = 64 ; 
    while ( buffer [ loc ] != 64 ) loc = loc + 1 ; 
    if ( loc <= limit ) 
    {
      loc = loc + 2 ; 
      c = controlcode ( buffer [ loc - 1 ] ) ; 
      if ( ( c != 0 ) || ( buffer [ loc - 1 ] == 62 ) ) 
      goto lab30 ; 
    } 
  } 
  lab30: Result = c ; 
  return(Result) ; 
} 
void skipcomment ( ) 
{/* 10 */ eightbits bal  ; 
  ASCIIcode c  ; 
  bal = 0 ; 
  while ( true ) {
      
    if ( loc > limit ) 
    {
      getline () ; 
      if ( inputhasended ) 
      {
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Input ended in mid-comment" ) ; 
	  error () ; 
	} 
	goto lab10 ; 
      } 
    } 
    c = buffer [ loc ] ; 
    loc = loc + 1 ; 
    if ( c == 64 ) 
    {
      c = buffer [ loc ] ; 
      if ( ( c != 32 ) && ( c != 9 ) && ( c != 42 ) && ( c != 122 ) && ( c != 
      90 ) ) 
      loc = loc + 1 ; 
      else {
	  
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Section ended in mid-comment" ) ; 
	  error () ; 
	} 
	loc = loc - 1 ; 
	goto lab10 ; 
      } 
    } 
    else if ( ( c == 92 ) && ( buffer [ loc ] != 64 ) ) 
    loc = loc + 1 ; 
    else if ( c == 123 ) 
    bal = bal + 1 ; 
    else if ( c == 125 ) 
    {
      if ( bal == 0 ) 
      goto lab10 ; 
      bal = bal - 1 ; 
    } 
  } 
  lab10: ; 
} 
eightbits getnext ( ) 
{/* 20 30 31 */ register eightbits Result; eightbits c  ; 
  eightbits d  ; 
  integer j, k  ; 
  lab20: if ( loc > limit ) 
  {
    getline () ; 
    if ( inputhasended ) 
    {
      c = 136 ; 
      goto lab31 ; 
    } 
  } 
  c = buffer [ loc ] ; 
  loc = loc + 1 ; 
  if ( scanninghex ) 
  if ( ( ( c >= 48 ) && ( c <= 57 ) ) || ( ( c >= 65 ) && ( c <= 70 ) ) ) 
  goto lab31 ; 
  else scanninghex = false ; 
  switch ( c ) 
  {case 65 : 
  case 66 : 
  case 67 : 
  case 68 : 
  case 69 : 
  case 70 : 
  case 71 : 
  case 72 : 
  case 73 : 
  case 74 : 
  case 75 : 
  case 76 : 
  case 77 : 
  case 78 : 
  case 79 : 
  case 80 : 
  case 81 : 
  case 82 : 
  case 83 : 
  case 84 : 
  case 85 : 
  case 86 : 
  case 87 : 
  case 88 : 
  case 89 : 
  case 90 : 
  case 97 : 
  case 98 : 
  case 99 : 
  case 100 : 
  case 101 : 
  case 102 : 
  case 103 : 
  case 104 : 
  case 105 : 
  case 106 : 
  case 107 : 
  case 108 : 
  case 109 : 
  case 110 : 
  case 111 : 
  case 112 : 
  case 113 : 
  case 114 : 
  case 115 : 
  case 116 : 
  case 117 : 
  case 118 : 
  case 119 : 
  case 120 : 
  case 121 : 
  case 122 : 
    {
      if ( ( ( c == 101 ) || ( c == 69 ) ) && ( loc > 1 ) ) 
      if ( ( buffer [ loc - 2 ] <= 57 ) && ( buffer [ loc - 2 ] >= 48 ) ) 
      c = 0 ; 
      if ( c != 0 ) 
      {
	loc = loc - 1 ; 
	idfirst = loc ; 
	do {
	    loc = loc + 1 ; 
	  d = buffer [ loc ] ; 
	} while ( ! ( ( ( d < 48 ) || ( ( d > 57 ) && ( d < 65 ) ) || ( ( d > 
	90 ) && ( d < 97 ) ) || ( d > 122 ) ) && ( d != 95 ) ) ) ; 
	if ( loc > idfirst + 1 ) 
	{
	  c = 130 ; 
	  idloc = loc ; 
	} 
      } 
      else c = 69 ; 
    } 
    break ; 
  case 34 : 
    {
      doublechars = 0 ; 
      idfirst = loc - 1 ; 
      do {
	  d = buffer [ loc ] ; 
	loc = loc + 1 ; 
	if ( ( d == 34 ) || ( d == 64 ) ) 
	if ( buffer [ loc ] == d ) 
	{
	  loc = loc + 1 ; 
	  d = 0 ; 
	  doublechars = doublechars + 1 ; 
	} 
	else {
	    
	  if ( d == 64 ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Double @ sign missing" ) ; 
	    error () ; 
	  } 
	} 
	else if ( loc > limit ) 
	{
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! String constant didn't end" ) ; 
	    error () ; 
	  } 
	  d = 34 ; 
	} 
      } while ( ! ( d == 34 ) ) ; 
      idloc = loc - 1 ; 
      c = 130 ; 
    } 
    break ; 
  case 64 : 
    {
      c = controlcode ( buffer [ loc ] ) ; 
      loc = loc + 1 ; 
      if ( c == 0 ) 
      goto lab20 ; 
      else if ( c == 13 ) 
      scanninghex = true ; 
      else if ( c == 135 ) 
      {
	k = 0 ; 
	while ( true ) {
	    
	  if ( loc > limit ) 
	  {
	    getline () ; 
	    if ( inputhasended ) 
	    {
	      {
		(void) putc('\n',  stdout );
		(void) Fputs( stdout ,  "! Input ended in section name" ) ; 
		error () ; 
	      } 
	      goto lab30 ; 
	    } 
	  } 
	  d = buffer [ loc ] ; 
	  if ( d == 64 ) 
	  {
	    d = buffer [ loc + 1 ] ; 
	    if ( d == 62 ) 
	    {
	      loc = loc + 2 ; 
	      goto lab30 ; 
	    } 
	    if ( ( d == 32 ) || ( d == 9 ) || ( d == 42 ) ) 
	    {
	      {
		(void) putc('\n',  stdout );
		(void) Fputs( stdout ,  "! Section name didn't end" ) ; 
		error () ; 
	      } 
	      goto lab30 ; 
	    } 
	    k = k + 1 ; 
	    modtext [ k ] = 64 ; 
	    loc = loc + 1 ; 
	  } 
	  loc = loc + 1 ; 
	  if ( k < longestname - 1 ) 
	  k = k + 1 ; 
	  if ( ( d == 32 ) || ( d == 9 ) ) 
	  {
	    d = 32 ; 
	    if ( modtext [ k - 1 ] == 32 ) 
	    k = k - 1 ; 
	  } 
	  modtext [ k ] = d ; 
	} 
	lab30: if ( k >= longestname - 2 ) 
	{
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Section name too long: " ) ; 
	  } 
	  {register integer for_end; j = 1 ; for_end = 25 ; if ( j <= 
	  for_end) do 
	    (void) putc( xchr [ modtext [ j ] ] ,  stdout );
	  while ( j++ < for_end ) ; } 
	  (void) Fputs( stdout ,  "..." ) ; 
	  if ( history == 0 ) 
	  history = 1 ; 
	} 
	if ( ( modtext [ k ] == 32 ) && ( k > 0 ) ) 
	k = k - 1 ; 
	if ( k > 3 ) 
	{
	  if ( ( modtext [ k ] == 46 ) && ( modtext [ k - 1 ] == 46 ) && ( 
	  modtext [ k - 2 ] == 46 ) ) 
	  curmodule = prefixlookup ( k - 3 ) ; 
	  else curmodule = modlookup ( k ) ; 
	} 
	else curmodule = modlookup ( k ) ; 
      } 
      else if ( c == 131 ) 
      {
	do {
	    c = skipahead () ; 
	} while ( ! ( c != 64 ) ) ; 
	if ( buffer [ loc - 1 ] != 62 ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Improper @ within control text" ) ; 
	  error () ; 
	} 
	goto lab20 ; 
      } 
    } 
    break ; 
  case 46 : 
    if ( buffer [ loc ] == 46 ) 
    {
      if ( loc <= limit ) 
      {
	c = 32 ; 
	loc = loc + 1 ; 
      } 
    } 
    else if ( buffer [ loc ] == 41 ) 
    {
      if ( loc <= limit ) 
      {
	c = 93 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 58 : 
    if ( buffer [ loc ] == 61 ) 
    {
      if ( loc <= limit ) 
      {
	c = 24 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 61 : 
    if ( buffer [ loc ] == 61 ) 
    {
      if ( loc <= limit ) 
      {
	c = 30 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 62 : 
    if ( buffer [ loc ] == 61 ) 
    {
      if ( loc <= limit ) 
      {
	c = 29 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 60 : 
    if ( buffer [ loc ] == 61 ) 
    {
      if ( loc <= limit ) 
      {
	c = 28 ; 
	loc = loc + 1 ; 
      } 
    } 
    else if ( buffer [ loc ] == 62 ) 
    {
      if ( loc <= limit ) 
      {
	c = 26 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 40 : 
    if ( buffer [ loc ] == 42 ) 
    {
      if ( loc <= limit ) 
      {
	c = 9 ; 
	loc = loc + 1 ; 
      } 
    } 
    else if ( buffer [ loc ] == 46 ) 
    {
      if ( loc <= limit ) 
      {
	c = 91 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 42 : 
    if ( buffer [ loc ] == 41 ) 
    {
      if ( loc <= limit ) 
      {
	c = 10 ; 
	loc = loc + 1 ; 
      } 
    } 
    break ; 
  case 32 : 
  case 9 : 
    goto lab20 ; 
    break ; 
  case 123 : 
    {
      skipcomment () ; 
      goto lab20 ; 
    } 
    break ; 
  case 125 : 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Extra }" ) ; 
	error () ; 
      } 
      goto lab20 ; 
    } 
    break ; 
    default: 
    if ( c >= 128 ) 
    goto lab20 ; 
    else ; 
    break ; 
  } 
  lab31: Result = c ; 
  return(Result) ; 
} 
void zscannumeric ( p ) 
namepointer p ; 
{/* 21 30 */ integer accumulator  ; 
  schar nextsign  ; 
  namepointer q  ; 
  integer val  ; 
  accumulator = 0 ; 
  nextsign = 1 ; 
  while ( true ) {
      
    nextcontrol = getnext () ; 
    lab21: switch ( nextcontrol ) 
    {case 48 : 
    case 49 : 
    case 50 : 
    case 51 : 
    case 52 : 
    case 53 : 
    case 54 : 
    case 55 : 
    case 56 : 
    case 57 : 
      {
	val = 0 ; 
	do {
	    val = 10 * val + nextcontrol - 48 ; 
	  nextcontrol = getnext () ; 
	} while ( ! ( ( nextcontrol > 57 ) || ( nextcontrol < 48 ) ) ) ; 
	{
	  accumulator = accumulator + nextsign * toint ( val ) ; 
	  nextsign = 1 ; 
	} 
	goto lab21 ; 
      } 
      break ; 
    case 12 : 
      {
	val = 0 ; 
	nextcontrol = 48 ; 
	do {
	    val = 8 * val + nextcontrol - 48 ; 
	  nextcontrol = getnext () ; 
	} while ( ! ( ( nextcontrol > 55 ) || ( nextcontrol < 48 ) ) ) ; 
	{
	  accumulator = accumulator + nextsign * toint ( val ) ; 
	  nextsign = 1 ; 
	} 
	goto lab21 ; 
      } 
      break ; 
    case 13 : 
      {
	val = 0 ; 
	nextcontrol = 48 ; 
	do {
	    if ( nextcontrol >= 65 ) 
	  nextcontrol = nextcontrol - 7 ; 
	  val = 16 * val + nextcontrol - 48 ; 
	  nextcontrol = getnext () ; 
	} while ( ! ( ( nextcontrol > 70 ) || ( nextcontrol < 48 ) || ( ( 
	nextcontrol > 57 ) && ( nextcontrol < 65 ) ) ) ) ; 
	{
	  accumulator = accumulator + nextsign * toint ( val ) ; 
	  nextsign = 1 ; 
	} 
	goto lab21 ; 
      } 
      break ; 
    case 130 : 
      {
	q = idlookup ( 0 ) ; 
	if ( ilk [ q ] != 1 ) 
	{
	  nextcontrol = 42 ; 
	  goto lab21 ; 
	} 
	{
	  accumulator = accumulator + nextsign * toint ( equiv [ q ] - 32768L 
	  ) ; 
	  nextsign = 1 ; 
	} 
      } 
      break ; 
    case 43 : 
      ; 
      break ; 
    case 45 : 
      nextsign = - (integer) nextsign ; 
      break ; 
    case 132 : 
    case 133 : 
    case 135 : 
    case 134 : 
    case 136 : 
      goto lab30 ; 
      break ; 
    case 59 : 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Omit semicolon in numeric definition" ) ; 
	error () ; 
      } 
      break ; 
      default: 
      {
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Improper numeric definition will be flushed" ) ; 
	  error () ; 
	} 
	do {
	    nextcontrol = skipahead () ; 
	} while ( ! ( ( nextcontrol >= 132 ) ) ) ; 
	if ( nextcontrol == 135 ) 
	{
	  loc = loc - 2 ; 
	  nextcontrol = getnext () ; 
	} 
	accumulator = 0 ; 
	goto lab30 ; 
      } 
      break ; 
    } 
  } 
  lab30: ; 
  if ( abs ( accumulator ) >= 32768L ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%ld",  "! Value too big: " , (long)accumulator ) ; 
      error () ; 
    } 
    accumulator = 0 ; 
  } 
  equiv [ p ] = accumulator + 32768L ; 
} 
void zscanrepl ( t ) 
eightbits t ; 
{/* 22 30 31 21 */ sixteenbits a  ; 
  ASCIIcode b  ; 
  eightbits bal  ; 
  bal = 0 ; 
  while ( true ) {
      
    lab22: a = getnext () ; 
    switch ( a ) 
    {case 40 : 
      bal = bal + 1 ; 
      break ; 
    case 41 : 
      if ( bal == 0 ) 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Extra )" ) ; 
	error () ; 
      } 
      else bal = bal - 1 ; 
      break ; 
    case 39 : 
      {
	b = 39 ; 
	while ( true ) {
	    
	  {
	    if ( tokptr [ z ] == maxtoks ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) 
	      ; 
	      error () ; 
	      history = 3 ; 
	      uexit ( 1 ) ; 
	    } 
	    tokmem [ z ][ tokptr [ z ] ] = b ; 
	    tokptr [ z ] = tokptr [ z ] + 1 ; 
	  } 
	  if ( b == 64 ) 
	  if ( buffer [ loc ] == 64 ) 
	  loc = loc + 1 ; 
	  else {
	      
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! You should double @ signs in strings" ) ; 
	    error () ; 
	  } 
	  if ( loc == limit ) 
	  {
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! String didn't end" ) ; 
	      error () ; 
	    } 
	    buffer [ loc ] = 39 ; 
	    buffer [ loc + 1 ] = 0 ; 
	  } 
	  b = buffer [ loc ] ; 
	  loc = loc + 1 ; 
	  if ( b == 39 ) 
	  {
	    if ( buffer [ loc ] != 39 ) 
	    goto lab31 ; 
	    else {
		
	      loc = loc + 1 ; 
	      {
		if ( tokptr [ z ] == maxtoks ) 
		{
		  (void) putc('\n',  stdout );
		  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" ,                   " capacity exceeded" ) ; 
		  error () ; 
		  history = 3 ; 
		  uexit ( 1 ) ; 
		} 
		tokmem [ z ][ tokptr [ z ] ] = 39 ; 
		tokptr [ z ] = tokptr [ z ] + 1 ; 
	      } 
	    } 
	  } 
	} 
	lab31: ; 
      } 
      break ; 
    case 35 : 
      if ( t == 3 ) 
      a = 0 ; 
      break ; 
    case 130 : 
      {
	a = idlookup ( 0 ) ; 
	{
	  if ( tokptr [ z ] == maxtoks ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	    error () ; 
	    history = 3 ; 
	    uexit ( 1 ) ; 
	  } 
	  tokmem [ z ][ tokptr [ z ] ] = ( a / 256 ) + 128 ; 
	  tokptr [ z ] = tokptr [ z ] + 1 ; 
	} 
	a = a % 256 ; 
      } 
      break ; 
    case 135 : 
      if ( t != 135 ) 
      goto lab30 ; 
      else {
	  
	{
	  if ( tokptr [ z ] == maxtoks ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	    error () ; 
	    history = 3 ; 
	    uexit ( 1 ) ; 
	  } 
	  tokmem [ z ][ tokptr [ z ] ] = ( curmodule / 256 ) + 168 ; 
	  tokptr [ z ] = tokptr [ z ] + 1 ; 
	} 
	a = curmodule % 256 ; 
      } 
      break ; 
    case 2 : 
      {
	{
	  if ( tokptr [ z ] == maxtoks ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	    error () ; 
	    history = 3 ; 
	    uexit ( 1 ) ; 
	  } 
	  tokmem [ z ][ tokptr [ z ] ] = 2 ; 
	  tokptr [ z ] = tokptr [ z ] + 1 ; 
	} 
	buffer [ limit + 1 ] = 64 ; 
	lab21: if ( buffer [ loc ] == 64 ) 
	{
	  if ( loc < limit ) 
	  if ( buffer [ loc + 1 ] == 64 ) 
	  {
	    {
	      if ( tokptr [ z ] == maxtoks ) 
	      {
		(void) putc('\n',  stdout );
		(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded"                 ) ; 
		error () ; 
		history = 3 ; 
		uexit ( 1 ) ; 
	      } 
	      tokmem [ z ][ tokptr [ z ] ] = 64 ; 
	      tokptr [ z ] = tokptr [ z ] + 1 ; 
	    } 
	    loc = loc + 2 ; 
	    goto lab21 ; 
	  } 
	} 
	else {
	    
	  {
	    if ( tokptr [ z ] == maxtoks ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) 
	      ; 
	      error () ; 
	      history = 3 ; 
	      uexit ( 1 ) ; 
	    } 
	    tokmem [ z ][ tokptr [ z ] ] = buffer [ loc ] ; 
	    tokptr [ z ] = tokptr [ z ] + 1 ; 
	  } 
	  loc = loc + 1 ; 
	  goto lab21 ; 
	} 
	if ( loc >= limit ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Verbatim string didn't end" ) ; 
	  error () ; 
	} 
	else if ( buffer [ loc + 1 ] != 62 ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! You should double @ signs in verbatim strings" ) 
	  ; 
	  error () ; 
	} 
	loc = loc + 2 ; 
      } 
      break ; 
    case 133 : 
    case 132 : 
    case 134 : 
      if ( t != 135 ) 
      goto lab30 ; 
      else {
	  
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%c%s",  "! @" , xchr [ buffer [ loc - 1 ] ] ,           " is ignored in Pascal text" ) ; 
	  error () ; 
	} 
	goto lab22 ; 
      } 
      break ; 
    case 136 : 
      goto lab30 ; 
      break ; 
      default: 
      ; 
      break ; 
    } 
    {
      if ( tokptr [ z ] == maxtoks ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	uexit ( 1 ) ; 
      } 
      tokmem [ z ][ tokptr [ z ] ] = a ; 
      tokptr [ z ] = tokptr [ z ] + 1 ; 
    } 
  } 
  lab30: nextcontrol = a ; 
  if ( bal > 0 ) 
  {
    if ( bal == 1 ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Missing )" ) ; 
      error () ; 
    } 
    else {
	
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%ld%s",  "! Missing " , (long)bal , " )'s" ) ; 
      error () ; 
    } 
    while ( bal > 0 ) {
	
      {
	if ( tokptr [ z ] == maxtoks ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	  error () ; 
	  history = 3 ; 
	  uexit ( 1 ) ; 
	} 
	tokmem [ z ][ tokptr [ z ] ] = 41 ; 
	tokptr [ z ] = tokptr [ z ] + 1 ; 
      } 
      bal = bal - 1 ; 
    } 
  } 
  if ( textptr > maxtexts - 4 ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "text" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    uexit ( 1 ) ; 
  } 
  currepltext = textptr ; 
  tokstart [ textptr + 4 ] = tokptr [ z ] ; 
  textptr = textptr + 1 ; 
  if ( z == 3 ) 
  z = 0 ; 
  else z = z + 1 ; 
} 
void zdefinemacro ( t ) 
eightbits t ; 
{namepointer p  ; 
  p = idlookup ( t ) ; 
  scanrepl ( t ) ; 
  equiv [ p ] = currepltext ; 
  textlink [ currepltext ] = 0 ; 
} 
void scanmodule ( ) 
{/* 22 30 10 */ namepointer p  ; 
  modulecount = modulecount + 1 ; 
  nextcontrol = 0 ; 
  while ( true ) {
      
    lab22: while ( nextcontrol <= 132 ) {
	
      nextcontrol = skipahead () ; 
      if ( nextcontrol == 135 ) 
      {
	loc = loc - 2 ; 
	nextcontrol = getnext () ; 
      } 
    } 
    if ( nextcontrol != 133 ) 
    goto lab30 ; 
    nextcontrol = getnext () ; 
    if ( nextcontrol != 130 ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s",  "! Definition flushed, must start with " ,         "identifier of length > 1" ) ; 
	error () ; 
      } 
      goto lab22 ; 
    } 
    nextcontrol = getnext () ; 
    if ( nextcontrol == 61 ) 
    {
      scannumeric ( idlookup ( 1 ) ) ; 
      goto lab22 ; 
    } 
    else if ( nextcontrol == 30 ) 
    {
      definemacro ( 2 ) ; 
      goto lab22 ; 
    } 
    else if ( nextcontrol == 40 ) 
    {
      nextcontrol = getnext () ; 
      if ( nextcontrol == 35 ) 
      {
	nextcontrol = getnext () ; 
	if ( nextcontrol == 41 ) 
	{
	  nextcontrol = getnext () ; 
	  if ( nextcontrol == 61 ) 
	  {
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! Use == for macros" ) ; 
	      error () ; 
	    } 
	    nextcontrol = 30 ; 
	  } 
	  if ( nextcontrol == 30 ) 
	  {
	    definemacro ( 3 ) ; 
	    goto lab22 ; 
	  } 
	} 
      } 
    } 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Definition flushed since it starts badly" ) ; 
      error () ; 
    } 
  } 
  lab30: ; 
  switch ( nextcontrol ) 
  {case 134 : 
    p = 0 ; 
    break ; 
  case 135 : 
    {
      p = curmodule ; 
      do {
	  nextcontrol = getnext () ; 
      } while ( ! ( nextcontrol != 43 ) ) ; 
      if ( ( nextcontrol != 61 ) && ( nextcontrol != 30 ) ) 
      {
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Pascal text flushed, = sign is missing" ) ; 
	  error () ; 
	} 
	do {
	    nextcontrol = skipahead () ; 
	} while ( ! ( nextcontrol == 136 ) ) ; 
	goto lab10 ; 
      } 
    } 
    break ; 
    default: 
    goto lab10 ; 
    break ; 
  } 
  storetwobytes ( 53248L + modulecount ) ; 
  scanrepl ( 135 ) ; 
  if ( p == 0 ) 
  {
    textlink [ lastunnamed ] = currepltext ; 
    lastunnamed = currepltext ; 
  } 
  else if ( equiv [ p ] == 0 ) 
  equiv [ p ] = currepltext ; 
  else {
      
    p = equiv [ p ] ; 
    while ( textlink [ p ] < maxtexts ) p = textlink [ p ] ; 
    textlink [ p ] = currepltext ; 
  } 
  textlink [ currepltext ] = maxtexts ; 
  lab10: ; 
} 
void main_body() {
    
  initialize () ; 
  openinput () ; 
  line = 0 ; 
  otherline = 0 ; 
  changing = true ; 
  primethechangebuffer () ; 
  changing = ! changing ; 
  templine = otherline ; 
  otherline = line ; 
  line = templine ; 
  limit = 0 ; 
  loc = 1 ; 
  buffer [ 0 ] = 32 ; 
  inputhasended = false ; 
  (void) fprintf( stdout , "%s\n",  "This is TANGLE, C Version 4.3" ) ; 
  phaseone = true ; 
  modulecount = 0 ; 
  do {
      nextcontrol = skipahead () ; 
  } while ( ! ( nextcontrol == 136 ) ) ; 
  while ( ! inputhasended ) scanmodule () ; 
  if ( changelimit != 0 ) 
  {
    {register integer for_end; ii = 0 ; for_end = changelimit ; if ( ii <= 
    for_end) do 
      buffer [ ii ] = changebuffer [ ii ] ; 
    while ( ii++ < for_end ) ; } 
    limit = changelimit ; 
    changing = true ; 
    line = otherline ; 
    loc = changelimit ; 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Change file entry did not match" ) ; 
      error () ; 
    } 
  } 
  phaseone = false ; 
  if ( textlink [ 0 ] == 0 ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! No output was specified." ) ; 
    } 
    if ( history == 0 ) 
    history = 1 ; 
  } 
  else {
      
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "Writing the output file" ) ; 
    } 
    flush ( stdout ) ; 
    stackptr = 1 ; 
    bracelevel = 0 ; 
    curstate .namefield = 0 ; 
    curstate .replfield = textlink [ 0 ] ; 
    zo = curstate .replfield % 4 ; 
    curstate .bytefield = tokstart [ curstate .replfield ] ; 
    curstate .endfield = tokstart [ curstate .replfield + 4 ] ; 
    curstate .modfield = 0 ; 
    outstate = 0 ; 
    outptr = 0 ; 
    breakptr = 0 ; 
    semiptr = 0 ; 
    outbuf [ 0 ] = 0 ; 
    line = 1 ; 
    sendtheoutput () ; 
    breakptr = outptr ; 
    semiptr = 0 ; 
    flushbuffer () ; 
    if ( bracelevel != 0 ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%ld",  "! Program ended at brace level " , (long)bracelevel ) ; 
      error () ; 
    } 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "Done." ) ; 
    } 
  } 
  lab9999: if ( stringptr > 256 ) 
  {
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%ld%s",  (long)stringptr - 256 ,       " strings written to string pool file." ) ; 
    } 
    (void) putc( '*' ,  pool );
    {register integer for_end; ii = 1 ; for_end = 9 ; if ( ii <= for_end) do 
      {
	outbuf [ ii ] = poolchecksum % 10 ; 
	poolchecksum = poolchecksum / 10 ; 
      } 
    while ( ii++ < for_end ) ; } 
    {register integer for_end; ii = 9 ; for_end = 1 ; if ( ii >= for_end) do 
      (void) putc( xchr [ 48 + outbuf [ ii ] ] ,  pool );
    while ( ii-- > for_end ) ; } 
    (void) putc('\n',  pool );
  } 
  switch ( history ) 
  {case 0 : 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "(No errors were found.)" ) ; 
    } 
    break ; 
  case 1 : 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "(Did you see the warning message above?)" ) ; 
    } 
    break ; 
  case 2 : 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "(Pardon me, but I think I spotted something wrong.)" ) 
      ; 
    } 
    break ; 
  case 3 : 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "(That was a fatal error, my friend.)" ) ; 
    } 
    break ; 
  } 
  (void) putc('\n',  stdout );
  if ( ( history != 0 ) && ( history != 1 ) ) 
  uexit ( 1 ) ; 
  else uexit ( 0 ) ; 
} 
