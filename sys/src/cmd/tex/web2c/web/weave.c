#include "config.h"
#define maxbytes 45000L 
#define maxnames 5000 
#define maxmodules 2000 
#define hashsize 353 
#define bufsize 100 
#define longestname 400 
#define longbufsize 500 
#define linelength 80 
#define maxrefs 30000 
#define maxtoks 30000 
#define maxtexts 2000 
#define maxscraps 1000 
#define stacksize 400 
typedef unsigned char ASCIIcode  ; 
typedef file_ptr /* of  ASCIIcode */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef unsigned short sixteenbits  ; 
typedef integer namepointer  ; 
typedef integer xrefnumber  ; 
typedef integer textpointer  ; 
typedef schar mode  ; 
typedef struct {
    sixteenbits endfield ; 
  sixteenbits tokfield ; 
  mode modefield ; 
} outputstate  ; 
schar history  ; 
ASCIIcode xord[256]  ; 
ASCIIcode xchr[256]  ; 
textfile webfile  ; 
textfile changefile  ; 
textfile texfile  ; 
ASCIIcode buffer[longbufsize + 1]  ; 
boolean phaseone  ; 
boolean phasethree  ; 
ASCIIcode bytemem[2][maxbytes + 1]  ; 
sixteenbits bytestart[maxnames + 1]  ; 
sixteenbits link[maxnames + 1]  ; 
sixteenbits ilk[maxnames + 1]  ; 
sixteenbits xref[maxnames + 1]  ; 
namepointer nameptr  ; 
integer byteptr[2]  ; 
integer modulecount  ; 
boolean changedmodule[maxmodules + 1]  ; 
boolean changeexists  ; 
struct {
    sixteenbits numfield ; 
  sixteenbits xlinkfield ; 
} xmem[maxrefs + 1]  ; 
xrefnumber xrefptr  ; 
short xrefswitch, modxrefswitch  ; 
sixteenbits tokmem[maxtoks + 1]  ; 
sixteenbits tokstart[maxtexts + 1]  ; 
textpointer textptr  ; 
integer tokptr  ; 
integer idfirst  ; 
integer idloc  ; 
sixteenbits hash[hashsize + 1]  ; 
namepointer curname  ; 
ASCIIcode modtext[longestname + 1]  ; 
integer ii  ; 
integer line  ; 
integer otherline  ; 
integer templine  ; 
integer limit  ; 
integer loc  ; 
boolean inputhasended  ; 
boolean changing  ; 
boolean changepending  ; 
ASCIIcode changebuffer[bufsize + 1]  ; 
integer changelimit  ; 
namepointer curmodule  ; 
boolean scanninghex  ; 
eightbits nextcontrol  ; 
namepointer lhs, rhs  ; 
xrefnumber curxref  ; 
ASCIIcode outbuf[linelength + 1]  ; 
integer outptr  ; 
integer outline  ; 
schar dig[5]  ; 
eightbits cat[maxscraps + 1]  ; 
short trans[maxscraps + 1]  ; 
integer pp  ; 
integer scrapbase  ; 
integer scrapptr  ; 
integer loptr  ; 
integer hiptr  ; 
outputstate curstate  ; 
outputstate stack[stacksize + 1]  ; 
integer stackptr  ; 
integer saveline  ; 
sixteenbits saveplace  ; 
namepointer thismodule  ; 
xrefnumber nextxref, thisxref, firstxref, midxref  ; 
integer kmodule  ; 
namepointer bucket[256]  ; 
namepointer nextname  ; 
ASCIIcode c  ; 
integer h  ; 
sixteenbits blink[maxnames + 1]  ; 
eightbits curdepth  ; 
integer curbyte  ; 
schar curbank  ; 
sixteenbits curval  ; 
ASCIIcode collate[230]  ; 
char webfilename[101], changefilename[101], texfilename[101]  ; 
boolean noxref  ; 

#include "weave.h"
void error ( ) 
{integer k, l  ; 
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
    if ( buffer [ limit ] == 124 ) 
    (void) putc( xchr [ 124 ] ,  stdout );
    (void) putc( ' ' ,  stdout );
  } 
  flush ( stdout ) ; 
  history = 2 ; 
} 
void jumpout ( ) 
{switch ( history ) 
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
void scanargs ( ) 
{integer slashpos, dotpos, i, a  ; 
  char fname[96]  ; 
  boolean foundweb, foundchange  ; 
  foundweb = false ; 
  foundchange = false ; 
  noxref = false ; 
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
	  while ( ( fname [ i ] != ' ' ) && ( i <= 95 ) ) {
	      
	    webfilename [ i ] = fname [ i ] ; 
	    if ( fname [ i ] == '.' ) 
	    dotpos = i ; 
	    if ( fname [ i ] == '/' ) 
	    slashpos = i ; 
	    i = i + 1 ; 
	  } 
	  webfilename [ i ] = ' ' ; 
	  if ( ( dotpos == -1 ) || ( dotpos < slashpos ) ) 
	  {
	    dotpos = i ; 
	    webfilename [ dotpos ] = '.' ; 
	    webfilename [ dotpos + 1 ] = 'w' ; 
	    webfilename [ dotpos + 2 ] = 'e' ; 
	    webfilename [ dotpos + 3 ] = 'b' ; 
	    webfilename [ dotpos + 4 ] = ' ' ; 
	  } 
	  {register integer for_end; i = 1 ; for_end = dotpos ; if ( i <= 
	  for_end) do 
	    texfilename [ i ] = webfilename [ i ] ; 
	  while ( i++ < for_end ) ; } 
	  texfilename [ dotpos + 1 ] = 't' ; 
	  texfilename [ dotpos + 2 ] = 'e' ; 
	  texfilename [ dotpos + 3 ] = 'x' ; 
	  texfilename [ dotpos + 4 ] = ' ' ; 
	  foundweb = true ; 
	} 
	else if ( ! foundchange ) 
	{
	  dotpos = -1 ; 
	  slashpos = -1 ; 
	  i = 1 ; 
	  while ( ( fname [ i ] != ' ' ) && ( i <= 95 ) ) {
	      
	    changefilename [ i ] = fname [ i ] ; 
	    if ( fname [ i ] == '.' ) 
	    dotpos = i ; 
	    if ( fname [ i ] == '/' ) 
	    slashpos = i ; 
	    i = i + 1 ; 
	  } 
	  changefilename [ i ] = ' ' ; 
	  if ( ( dotpos == -1 ) || ( dotpos < slashpos ) ) 
	  {
	    dotpos = i ; 
	    changefilename [ dotpos ] = '.' ; 
	    changefilename [ dotpos + 1 ] = 'c' ; 
	    changefilename [ dotpos + 2 ] = 'h' ; 
	    changefilename [ dotpos + 3 ] = ' ' ; 
	  } 
	  foundchange = true ; 
	} 
	else {
	    
	  (void) fprintf( stdout , "%s\n",            "Usage: weave webfile[.web] [changefile[.ch]] [-x]." ) ; 
	  uexit ( 1 ) ; 
	} 
      } 
      else {
	  
	i = 2 ; 
	while ( ( fname [ i ] != ' ' ) && ( i <= 95 ) ) {
	    
	  if ( fname [ i ] == 'x' ) 
	  noxref = true ; 
	  i = i + 1 ; 
	} 
      } 
    } 
  while ( a++ < for_end ) ; } 
  if ( ! foundweb ) 
  {
    (void) fprintf( stdout , "%s\n",  "Usage: weave webfile[.web] [changefile[.ch]] [-x]." ) 
    ; 
    uexit ( 1 ) ; 
  } 
  if ( ! foundchange ) 
  {
    changefilename [ 1 ] = '/' ; 
    changefilename [ 2 ] = 'd' ; 
    changefilename [ 3 ] = 'e' ; 
    changefilename [ 4 ] = 'v' ; 
    changefilename [ 5 ] = '/' ; 
    changefilename [ 6 ] = 'n' ; 
    changefilename [ 7 ] = 'u' ; 
    changefilename [ 8 ] = 'l' ; 
    changefilename [ 9 ] = 'l' ; 
    changefilename [ 10 ] = ' ' ; 
  } 
} 
void initialize ( ) 
{unsigned char i  ; 
  schar wi  ; 
  integer h  ; 
  ASCIIcode c  ; 
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
  rewrite ( texfile , texfilename ) ; 
  {register integer for_end; wi = 0 ; for_end = 1 ; if ( wi <= for_end) do 
    {
      bytestart [ wi ] = 0 ; 
      byteptr [ wi ] = 0 ; 
    } 
  while ( wi++ < for_end ) ; } 
  bytestart [ 2 ] = 0 ; 
  nameptr = 1 ; 
  ilk [ 0 ] = 0 ; 
  xrefptr = 0 ; 
  xrefswitch = 0 ; 
  modxrefswitch = 0 ; 
  xmem [ 0 ] .numfield = 0 ; 
  xref [ 0 ] = 0 ; 
  tokptr = 1 ; 
  textptr = 1 ; 
  tokstart [ 0 ] = 1 ; 
  tokstart [ 1 ] = 1 ; 
  {register integer for_end; h = 0 ; for_end = hashsize - 1 ; if ( h <= 
  for_end) do 
    hash [ h ] = 0 ; 
  while ( h++ < for_end ) ; } 
  scanninghex = false ; 
  modtext [ 0 ] = 32 ; 
  outptr = 1 ; 
  outline = 1 ; 
  outbuf [ 1 ] = 99 ; 
  (void) Fputs( texfile ,  "\\input webma" ) ; 
  outbuf [ 0 ] = 92 ; 
  scrapbase = 1 ; 
  scrapptr = 0 ; 
  collate [ 0 ] = 0 ; 
  collate [ 1 ] = 32 ; 
  {register integer for_end; c = 1 ; for_end = 31 ; if ( c <= for_end) do 
    collate [ c + 1 ] = c ; 
  while ( c++ < for_end ) ; } 
  {register integer for_end; c = 33 ; for_end = 47 ; if ( c <= for_end) do 
    collate [ c ] = c ; 
  while ( c++ < for_end ) ; } 
  {register integer for_end; c = 58 ; for_end = 64 ; if ( c <= for_end) do 
    collate [ c - 10 ] = c ; 
  while ( c++ < for_end ) ; } 
  {register integer for_end; c = 91 ; for_end = 94 ; if ( c <= for_end) do 
    collate [ c - 36 ] = c ; 
  while ( c++ < for_end ) ; } 
  collate [ 59 ] = 96 ; 
  {register integer for_end; c = 123 ; for_end = 255 ; if ( c <= for_end) do 
    collate [ c - 63 ] = c ; 
  while ( c++ < for_end ) ; } 
  collate [ 193 ] = 95 ; 
  {register integer for_end; c = 97 ; for_end = 122 ; if ( c <= for_end) do 
    collate [ c + 97 ] = c ; 
  while ( c++ < for_end ) ; } 
  {register integer for_end; c = 48 ; for_end = 57 ; if ( c <= for_end) do 
    collate [ c + 172 ] = c ; 
  while ( c++ < for_end ) ; } 
} 
void openinput ( ) 
{reset ( webfile , webfilename ) ; 
  reset ( changefile , changefilename ) ; 
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
      
    w = p % 2 ; 
    {register integer for_end; k = bytestart [ p ] ; for_end = bytestart [ p 
    + 2 ] - 1 ; if ( k <= for_end) do 
      (void) putc( xchr [ bytemem [ w ][ k ] ] ,  stdout );
    while ( k++ < for_end ) ; } 
  } 
} 
void znewxref ( p ) 
namepointer p ; 
{/* 10 */ xrefnumber q  ; 
  sixteenbits m, n  ; 
  if ( noxref ) 
  goto lab10 ; 
  if ( ( ( ilk [ p ] > 3 ) || ( bytestart [ p ] + 1 == bytestart [ p + 2 ] ) ) 
  && ( xrefswitch == 0 ) ) 
  goto lab10 ; 
  m = modulecount + xrefswitch ; 
  xrefswitch = 0 ; 
  q = xref [ p ] ; 
  if ( q > 0 ) 
  {
    n = xmem [ q ] .numfield ; 
    if ( ( n == m ) || ( n == m + 10240 ) ) 
    goto lab10 ; 
    else if ( m == n + 10240 ) 
    {
      xmem [ q ] .numfield = m ; 
      goto lab10 ; 
    } 
  } 
  if ( xrefptr == maxrefs ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "cross reference" , " capacity exceeded" ) 
    ; 
    error () ; 
    history = 3 ; 
    jumpout () ; 
  } 
  else {
      
    xrefptr = xrefptr + 1 ; 
    xmem [ xrefptr ] .numfield = m ; 
  } 
  xmem [ xrefptr ] .xlinkfield = q ; 
  xref [ p ] = xrefptr ; 
  lab10: ; 
} 
void znewmodxref ( p ) 
namepointer p ; 
{xrefnumber q, r  ; 
  q = xref [ p ] ; 
  r = 0 ; 
  if ( q > 0 ) 
  {
    if ( modxrefswitch == 0 ) 
    while ( xmem [ q ] .numfield >= 10240 ) {
	
      r = q ; 
      q = xmem [ q ] .xlinkfield ; 
    } 
    else if ( xmem [ q ] .numfield >= 10240 ) 
    {
      r = q ; 
      q = xmem [ q ] .xlinkfield ; 
    } 
  } 
  if ( xrefptr == maxrefs ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "cross reference" , " capacity exceeded" ) 
    ; 
    error () ; 
    history = 3 ; 
    jumpout () ; 
  } 
  else {
      
    xrefptr = xrefptr + 1 ; 
    xmem [ xrefptr ] .numfield = modulecount + modxrefswitch ; 
  } 
  xmem [ xrefptr ] .xlinkfield = q ; 
  modxrefswitch = 0 ; 
  if ( r == 0 ) 
  xref [ p ] = xrefptr ; 
  else xmem [ r ] .xlinkfield = xrefptr ; 
} 
namepointer zidlookup ( t ) 
eightbits t ; 
{/* 31 */ register namepointer Result; integer i  ; 
  integer h  ; 
  integer k  ; 
  schar w  ; 
  integer l  ; 
  namepointer p  ; 
  l = idloc - idfirst ; 
  h = buffer [ idfirst ] ; 
  i = idfirst + 1 ; 
  while ( i < idloc ) {
      
    h = ( h + h + buffer [ i ] ) % hashsize ; 
    i = i + 1 ; 
  } 
  p = hash [ h ] ; 
  while ( p != 0 ) {
      
    if ( ( bytestart [ p + 2 ] - bytestart [ p ] == l ) && ( ( ilk [ p ] == t 
    ) || ( ( t == 0 ) && ( ilk [ p ] > 3 ) ) ) ) 
    {
      i = idfirst ; 
      k = bytestart [ p ] ; 
      w = p % 2 ; 
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
  if ( p == nameptr ) 
  {
    w = nameptr % 2 ; 
    if ( byteptr [ w ] + l > maxbytes ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "byte memory" , " capacity exceeded" ) ; 
      error () ; 
      history = 3 ; 
      jumpout () ; 
    } 
    if ( nameptr + 2 > maxnames ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "name" , " capacity exceeded" ) ; 
      error () ; 
      history = 3 ; 
      jumpout () ; 
    } 
    i = idfirst ; 
    k = byteptr [ w ] ; 
    while ( i < idloc ) {
	
      bytemem [ w ][ k ] = buffer [ i ] ; 
      k = k + 1 ; 
      i = i + 1 ; 
    } 
    byteptr [ w ] = k ; 
    bytestart [ nameptr + 2 ] = k ; 
    nameptr = nameptr + 1 ; 
    ilk [ p ] = t ; 
    xref [ p ] = 0 ; 
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
      w = p % 2 ; 
      c = 1 ; 
      j = 1 ; 
      while ( ( k < bytestart [ p + 2 ] ) && ( j <= l ) && ( modtext [ j ] == 
      bytemem [ w ][ k ] ) ) {
	  
	k = k + 1 ; 
	j = j + 1 ; 
      } 
      if ( k == bytestart [ p + 2 ] ) 
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
  w = nameptr % 2 ; 
  k = byteptr [ w ] ; 
  if ( k + l > maxbytes ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "byte memory" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    jumpout () ; 
  } 
  if ( nameptr > maxnames - 2 ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "name" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    jumpout () ; 
  } 
  p = nameptr ; 
  if ( c == 0 ) 
  link [ q ] = p ; 
  else ilk [ q ] = p ; 
  link [ p ] = 0 ; 
  ilk [ p ] = 0 ; 
  xref [ p ] = 0 ; 
  c = 1 ; 
  {register integer for_end; j = 1 ; for_end = l ; if ( j <= for_end) do 
    bytemem [ w ][ k + j - 1 ] = modtext [ j ] ; 
  while ( j++ < for_end ) ; } 
  byteptr [ w ] = k + l ; 
  bytestart [ nameptr + 2 ] = k + l ; 
  nameptr = nameptr + 1 ; 
  lab31: if ( c != 1 ) 
  {
    {
      if ( ! phaseone ) 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Incompatible section names" ) ; 
	error () ; 
      } 
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
      w = p % 2 ; 
      c = 1 ; 
      j = 1 ; 
      while ( ( k < bytestart [ p + 2 ] ) && ( j <= l ) && ( modtext [ j ] == 
      bytemem [ w ][ k ] ) ) {
	  
	k = k + 1 ; 
	j = j + 1 ; 
      } 
      if ( k == bytestart [ p + 2 ] ) 
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
    if ( ! phaseone ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Name does not match" ) ; 
      error () ; 
    } 
  } 
  else {
      
    if ( ! phaseone ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Ambiguous prefix" ) ; 
      error () ; 
    } 
  } 
  Result = r ; 
  return(Result) ; 
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
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Where is the matching @x?" ) ; 
	  error () ; 
	} 
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
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Change file ended after @x" ) ; 
	  error () ; 
	} 
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
  changepending = false ; 
  if ( ! changedmodule [ modulecount ] ) 
  {
    loc = 0 ; 
    buffer [ limit ] = 33 ; 
    while ( ( buffer [ loc ] == 32 ) || ( buffer [ loc ] == 9 ) ) loc = loc + 
    1 ; 
    buffer [ limit ] = 32 ; 
    if ( buffer [ loc ] == 64 ) 
    if ( ( buffer [ loc + 1 ] == 42 ) || ( buffer [ loc + 1 ] == 32 ) || ( 
    buffer [ loc + 1 ] == 9 ) ) 
    changepending = true ; 
    if ( ! changepending ) 
    changedmodule [ modulecount ] = true ; 
  } 
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
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Change file ended before @y" ) ; 
	  error () ; 
	} 
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
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Where is the matching @y?" ) ; 
	    error () ; 
	  } 
	} 
      } 
      else if ( buffer [ 1 ] == 121 ) 
      {
	if ( n > 0 ) 
	{
	  loc = 2 ; 
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%ld%s",  "! Hmm... " , (long)n ,               " of the preceding lines failed to match" ) ; 
	      error () ; 
	    } 
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
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! WEB file ended during a change" ) ; 
	  error () ; 
	} 
      } 
      inputhasended = true ; 
      goto lab10 ; 
    } 
    if ( linesdontmatch () ) 
    n = n + 1 ; 
  } 
  lab10: ; 
} 
void resetinput ( ) 
{openinput () ; 
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
} 
void getline ( ) 
{/* 20 */ lab20: if ( changing ) 
  {
    line = line + 1 ; 
    if ( ! inputln ( changefile ) ) 
    {
      {
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Change file ended without @z" ) ; 
	  error () ; 
	} 
      } 
      buffer [ 0 ] = 64 ; 
      buffer [ 1 ] = 122 ; 
      limit = 2 ; 
    } 
    if ( limit > 0 ) 
    {
      if ( changepending ) 
      {
	loc = 0 ; 
	buffer [ limit ] = 33 ; 
	while ( ( buffer [ loc ] == 32 ) || ( buffer [ loc ] == 9 ) ) loc = 
	loc + 1 ; 
	buffer [ limit ] = 32 ; 
	if ( buffer [ loc ] == 64 ) 
	if ( ( buffer [ loc + 1 ] == 42 ) || ( buffer [ loc + 1 ] == 32 ) || ( 
	buffer [ loc + 1 ] == 9 ) ) 
	changepending = false ; 
	if ( changepending ) 
	{
	  changedmodule [ modulecount ] = true ; 
	  changepending = false ; 
	} 
      } 
      buffer [ limit ] = 32 ; 
      if ( buffer [ 0 ] == 64 ) 
      {
	if ( ( buffer [ 1 ] >= 88 ) && ( buffer [ 1 ] <= 90 ) ) 
	buffer [ 1 ] = buffer [ 1 ] + 32 ; 
	if ( ( buffer [ 1 ] == 120 ) || ( buffer [ 1 ] == 121 ) ) 
	{
	  loc = 2 ; 
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! Where is the matching @z?" ) ; 
	      error () ; 
	    } 
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
    Result = 135 ; 
    break ; 
  case 32 : 
  case 9 : 
  case 42 : 
    Result = 147 ; 
    break ; 
  case 61 : 
    Result = 2 ; 
    break ; 
  case 92 : 
    Result = 3 ; 
    break ; 
  case 68 : 
  case 100 : 
    Result = 144 ; 
    break ; 
  case 70 : 
  case 102 : 
    Result = 143 ; 
    break ; 
  case 123 : 
    Result = 9 ; 
    break ; 
  case 125 : 
    Result = 10 ; 
    break ; 
  case 80 : 
  case 112 : 
    Result = 145 ; 
    break ; 
  case 38 : 
    Result = 136 ; 
    break ; 
  case 60 : 
    Result = 146 ; 
    break ; 
  case 62 : 
    {
      {
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Extra @>" ) ; 
	  error () ; 
	} 
      } 
      Result = 0 ; 
    } 
    break ; 
  case 84 : 
  case 116 : 
    Result = 134 ; 
    break ; 
  case 33 : 
    Result = 126 ; 
    break ; 
  case 63 : 
    Result = 125 ; 
    break ; 
  case 94 : 
    Result = 131 ; 
    break ; 
  case 58 : 
    Result = 132 ; 
    break ; 
  case 46 : 
    Result = 133 ; 
    break ; 
  case 44 : 
    Result = 137 ; 
    break ; 
  case 124 : 
    Result = 138 ; 
    break ; 
  case 47 : 
    Result = 139 ; 
    break ; 
  case 35 : 
    Result = 140 ; 
    break ; 
  case 43 : 
    Result = 141 ; 
    break ; 
  case 59 : 
    Result = 142 ; 
    break ; 
    default: 
    {
      {
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Unknown control code" ) ; 
	  error () ; 
	} 
      } 
      Result = 0 ; 
    } 
    break ; 
  } 
  return(Result) ; 
} 
void skiplimbo ( ) 
{/* 10 */ ASCIIcode c  ; 
  while ( true ) if ( loc > limit ) 
  {
    getline () ; 
    if ( inputhasended ) 
    goto lab10 ; 
  } 
  else {
      
    buffer [ limit + 1 ] = 64 ; 
    while ( buffer [ loc ] != 64 ) loc = loc + 1 ; 
    if ( loc <= limit ) 
    {
      loc = loc + 2 ; 
      c = buffer [ loc - 1 ] ; 
      if ( ( c == 32 ) || ( c == 9 ) || ( c == 42 ) ) 
      goto lab10 ; 
    } 
  } 
  lab10: ; 
} 
eightbits skipTeX ( ) 
{/* 30 */ register eightbits Result; eightbits c  ; 
  while ( true ) {
      
    if ( loc > limit ) 
    {
      getline () ; 
      if ( inputhasended ) 
      {
	c = 147 ; 
	goto lab30 ; 
      } 
    } 
    buffer [ limit + 1 ] = 64 ; 
    do {
	c = buffer [ loc ] ; 
      loc = loc + 1 ; 
      if ( c == 124 ) 
      goto lab30 ; 
    } while ( ! ( c == 64 ) ) ; 
    if ( loc <= limit ) 
    {
      c = controlcode ( buffer [ loc ] ) ; 
      loc = loc + 1 ; 
      goto lab30 ; 
    } 
  } 
  lab30: Result = c ; 
  return(Result) ; 
} 
eightbits zskipcomment ( bal ) 
eightbits bal ; 
{/* 30 */ register eightbits Result; ASCIIcode c  ; 
  while ( true ) {
      
    if ( loc > limit ) 
    {
      getline () ; 
      if ( inputhasended ) 
      {
	bal = 0 ; 
	goto lab30 ; 
      } 
    } 
    c = buffer [ loc ] ; 
    loc = loc + 1 ; 
    if ( c == 124 ) 
    goto lab30 ; 
    if ( c == 64 ) 
    {
      c = buffer [ loc ] ; 
      if ( ( c != 32 ) && ( c != 9 ) && ( c != 42 ) ) 
      loc = loc + 1 ; 
      else {
	  
	loc = loc - 1 ; 
	bal = 0 ; 
	goto lab30 ; 
      } 
    } 
    else if ( ( c == 92 ) && ( buffer [ loc ] != 64 ) ) 
    loc = loc + 1 ; 
    else if ( c == 123 ) 
    bal = bal + 1 ; 
    else if ( c == 125 ) 
    {
      bal = bal - 1 ; 
      if ( bal == 0 ) 
      goto lab30 ; 
    } 
  } 
  lab30: Result = bal ; 
  return(Result) ; 
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
      c = 147 ; 
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
      if ( ( ( c == 69 ) || ( c == 101 ) ) && ( loc > 1 ) ) 
      if ( ( buffer [ loc - 2 ] <= 57 ) && ( buffer [ loc - 2 ] >= 48 ) ) 
      c = 128 ; 
      if ( c != 128 ) 
      {
	loc = loc - 1 ; 
	idfirst = loc ; 
	do {
	    loc = loc + 1 ; 
	  d = buffer [ loc ] ; 
	} while ( ! ( ( ( d < 48 ) || ( ( d > 57 ) && ( d < 65 ) ) || ( ( d > 
	90 ) && ( d < 97 ) ) || ( d > 122 ) ) && ( d != 95 ) ) ) ; 
	c = 130 ; 
	idloc = loc ; 
      } 
    } 
    break ; 
  case 39 : 
  case 34 : 
    {
      idfirst = loc - 1 ; 
      do {
	  d = buffer [ loc ] ; 
	loc = loc + 1 ; 
	if ( loc > limit ) 
	{
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! String constant didn't end" ) ; 
	      error () ; 
	    } 
	  } 
	  loc = limit ; 
	  d = c ; 
	} 
      } while ( ! ( d == c ) ) ; 
      idloc = loc ; 
      c = 129 ; 
    } 
    break ; 
  case 64 : 
    {
      c = controlcode ( buffer [ loc ] ) ; 
      loc = loc + 1 ; 
      if ( c == 126 ) 
      {
	xrefswitch = 10240 ; 
	goto lab20 ; 
      } 
      else if ( c == 125 ) 
      {
	xrefswitch = 0 ; 
	goto lab20 ; 
      } 
      else if ( ( c <= 134 ) && ( c >= 131 ) ) 
      {
	idfirst = loc ; 
	buffer [ limit + 1 ] = 64 ; 
	while ( buffer [ loc ] != 64 ) loc = loc + 1 ; 
	idloc = loc ; 
	if ( loc > limit ) 
	{
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! Control text didn't end" ) ; 
	      error () ; 
	    } 
	  } 
	  loc = limit ; 
	} 
	else {
	    
	  loc = loc + 2 ; 
	  if ( buffer [ loc - 1 ] != 62 ) 
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! Control codes are forbidden in control text"               ) ; 
	      error () ; 
	    } 
	  } 
	} 
      } 
      else if ( c == 13 ) 
      scanninghex = true ; 
      else if ( c == 146 ) 
      {
	k = 0 ; 
	while ( true ) {
	    
	  if ( loc > limit ) 
	  {
	    getline () ; 
	    if ( inputhasended ) 
	    {
	      {
		if ( ! phaseone ) 
		{
		  (void) putc('\n',  stdout );
		  (void) Fputs( stdout ,  "! Input ended in section name" ) ; 
		  error () ; 
		} 
	      } 
	      loc = 1 ; 
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
		if ( ! phaseone ) 
		{
		  (void) putc('\n',  stdout );
		  (void) Fputs( stdout ,  "! Section name didn't end" ) ; 
		  error () ; 
		} 
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
	xrefswitch = 0 ; 
      } 
      else if ( c == 2 ) 
      {
	idfirst = loc ; 
	loc = loc + 1 ; 
	buffer [ limit + 1 ] = 64 ; 
	buffer [ limit + 2 ] = 62 ; 
	while ( ( buffer [ loc ] != 64 ) || ( buffer [ loc + 1 ] != 62 ) ) loc 
	= loc + 1 ; 
	if ( loc >= limit ) 
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Verbatim string didn't end" ) ; 
	    error () ; 
	  } 
	} 
	idloc = loc ; 
	loc = loc + 2 ; 
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
  case 125 : 
    {
      {
	if ( ! phaseone ) 
	{
	  (void) putc('\n',  stdout );
	  (void) Fputs( stdout ,  "! Extra }" ) ; 
	  error () ; 
	} 
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
void Pascalxref ( ) 
{/* 10 */ namepointer p  ; 
  while ( nextcontrol < 143 ) {
      
    if ( ( nextcontrol >= 130 ) && ( nextcontrol <= 133 ) ) 
    {
      p = idlookup ( nextcontrol - 130 ) ; 
      newxref ( p ) ; 
      if ( ( ilk [ p ] == 17 ) || ( ilk [ p ] == 22 ) ) 
      xrefswitch = 10240 ; 
    } 
    nextcontrol = getnext () ; 
    if ( ( nextcontrol == 124 ) || ( nextcontrol == 123 ) ) 
    goto lab10 ; 
  } 
  lab10: ; 
} 
void outerxref ( ) 
{eightbits bal  ; 
  while ( nextcontrol < 143 ) if ( nextcontrol != 123 ) 
  Pascalxref () ; 
  else {
      
    bal = skipcomment ( 1 ) ; 
    nextcontrol = 124 ; 
    while ( bal > 0 ) {
	
      Pascalxref () ; 
      if ( nextcontrol == 124 ) 
      bal = skipcomment ( bal ) ; 
      else bal = 0 ; 
    } 
  } 
} 
void zmodcheck ( p ) 
namepointer p ; 
{if ( p > 0 ) 
  {
    modcheck ( link [ p ] ) ; 
    curxref = xref [ p ] ; 
    if ( xmem [ curxref ] .numfield < 10240 ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Never defined: <" ) ; 
      } 
      printid ( p ) ; 
      (void) putc( '>' ,  stdout );
      if ( history == 0 ) 
      history = 1 ; 
    } 
    while ( xmem [ curxref ] .numfield >= 10240 ) curxref = xmem [ curxref ] 
    .xlinkfield ; 
    if ( curxref == 0 ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Never used: <" ) ; 
      } 
      printid ( p ) ; 
      (void) putc( '>' ,  stdout );
      if ( history == 0 ) 
      history = 1 ; 
    } 
    modcheck ( ilk [ p ] ) ; 
  } 
} 
void zflushbuffer ( b , percent , carryover ) 
eightbits b ; 
boolean percent ; 
boolean carryover ; 
{/* 30 31 */ integer j, k  ; 
  j = b ; 
  if ( ! percent ) 
  while ( true ) {
      
    if ( j == 0 ) 
    goto lab30 ; 
    if ( outbuf [ j ] != 32 ) 
    goto lab30 ; 
    j = j - 1 ; 
  } 
  lab30: {
      register integer for_end; k = 1 ; for_end = j ; if ( k <= for_end) 
  do 
    (void) putc( xchr [ outbuf [ k ] ] ,  texfile );
  while ( k++ < for_end ) ; } 
  if ( percent ) 
  (void) putc( xchr [ 37 ] ,  texfile );
  (void) putc('\n',  texfile );
  outline = outline + 1 ; 
  if ( carryover ) 
  {register integer for_end; k = 1 ; for_end = j ; if ( k <= for_end) do 
    if ( outbuf [ k ] == 37 ) 
    if ( ( k == 1 ) || ( outbuf [ k - 1 ] != 92 ) ) 
    {
      outbuf [ b ] = 37 ; 
      b = b - 1 ; 
      goto lab31 ; 
    } 
  while ( k++ < for_end ) ; } 
  lab31: if ( ( b < outptr ) ) 
  {register integer for_end; k = b + 1 ; for_end = outptr ; if ( k <= 
  for_end) do 
    outbuf [ k - b ] = outbuf [ k ] ; 
  while ( k++ < for_end ) ; } 
  outptr = outptr - b ; 
} 
void finishline ( ) 
{/* 10 */ integer k  ; 
  if ( outptr > 0 ) 
  flushbuffer ( outptr , false , false ) ; 
  else {
      
    {register integer for_end; k = 0 ; for_end = limit ; if ( k <= for_end) 
    do 
      if ( ( buffer [ k ] != 32 ) && ( buffer [ k ] != 9 ) ) 
      goto lab10 ; 
    while ( k++ < for_end ) ; } 
    flushbuffer ( 0 , false , false ) ; 
  } 
  lab10: ; 
} 
void breakout ( ) 
{/* 10 */ integer k  ; 
  ASCIIcode d  ; 
  k = outptr ; 
  while ( true ) {
      
    if ( k == 0 ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%ld",  "! Line had to be broken (output l." , (long)outline ) ; 
      } 
      (void) fprintf( stdout , "%s\n",  "):" ) ; 
      {register integer for_end; k = 1 ; for_end = outptr - 1 ; if ( k <= 
      for_end) do 
	(void) putc( xchr [ outbuf [ k ] ] ,  stdout );
      while ( k++ < for_end ) ; } 
      (void) putc('\n',  stdout );
      if ( history == 0 ) 
      history = 1 ; 
      flushbuffer ( outptr - 1 , true , true ) ; 
      goto lab10 ; 
    } 
    d = outbuf [ k ] ; 
    if ( d == 32 ) 
    {
      flushbuffer ( k , false , true ) ; 
      goto lab10 ; 
    } 
    if ( ( d == 92 ) && ( outbuf [ k - 1 ] != 92 ) ) 
    {
      flushbuffer ( k - 1 , true , true ) ; 
      goto lab10 ; 
    } 
    k = k - 1 ; 
  } 
  lab10: ; 
} 
void zoutmod ( m ) 
integer m ; 
{schar k  ; 
  integer a  ; 
  k = 0 ; 
  a = m ; 
  do {
      dig [ k ] = a % 10 ; 
    a = a / 10 ; 
    k = k + 1 ; 
  } while ( ! ( a == 0 ) ) ; 
  do {
      k = k - 1 ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = dig [ k ] + 48 ; 
    } 
  } while ( ! ( k == 0 ) ) ; 
  if ( changedmodule [ m ] ) 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 92 ; 
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 42 ; 
  } 
} 
void zoutname ( p ) 
namepointer p ; 
{integer k  ; 
  schar w  ; 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 123 ; 
  } 
  w = p % 2 ; 
  {register integer for_end; k = bytestart [ p ] ; for_end = bytestart [ p + 
  2 ] - 1 ; if ( k <= for_end) do 
    {
      if ( bytemem [ w ][ k ] == 95 ) 
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 92 ; 
      } 
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = bytemem [ w ][ k ] ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 125 ; 
  } 
} 
void copylimbo ( ) 
{/* 10 */ ASCIIcode c  ; 
  while ( true ) if ( loc > limit ) 
  {
    finishline () ; 
    getline () ; 
    if ( inputhasended ) 
    goto lab10 ; 
  } 
  else {
      
    buffer [ limit + 1 ] = 64 ; 
    while ( buffer [ loc ] != 64 ) {
	
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = buffer [ loc ] ; 
      } 
      loc = loc + 1 ; 
    } 
    if ( loc <= limit ) 
    {
      loc = loc + 2 ; 
      c = buffer [ loc - 1 ] ; 
      if ( ( c == 32 ) || ( c == 9 ) || ( c == 42 ) ) 
      goto lab10 ; 
      if ( ( c != 122 ) && ( c != 90 ) ) 
      {
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 64 ; 
	} 
	if ( c != 64 ) 
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Double @ required outside of sections" ) ; 
	    error () ; 
	  } 
	} 
      } 
    } 
  } 
  lab10: ; 
} 
eightbits copyTeX ( ) 
{/* 30 */ register eightbits Result; eightbits c  ; 
  while ( true ) {
      
    if ( loc > limit ) 
    {
      finishline () ; 
      getline () ; 
      if ( inputhasended ) 
      {
	c = 147 ; 
	goto lab30 ; 
      } 
    } 
    buffer [ limit + 1 ] = 64 ; 
    do {
	c = buffer [ loc ] ; 
      loc = loc + 1 ; 
      if ( c == 124 ) 
      goto lab30 ; 
      if ( c != 64 ) 
      {
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = c ; 
	} 
	if ( ( outptr == 1 ) && ( ( c == 32 ) || ( c == 9 ) ) ) 
	outptr = outptr - 1 ; 
      } 
    } while ( ! ( c == 64 ) ) ; 
    if ( loc <= limit ) 
    {
      c = controlcode ( buffer [ loc ] ) ; 
      loc = loc + 1 ; 
      goto lab30 ; 
    } 
  } 
  lab30: Result = c ; 
  return(Result) ; 
} 
eightbits zcopycomment ( bal ) 
eightbits bal ; 
{/* 30 */ register eightbits Result; ASCIIcode c  ; 
  while ( true ) {
      
    if ( loc > limit ) 
    {
      getline () ; 
      if ( inputhasended ) 
      {
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Input ended in mid-comment" ) ; 
	    error () ; 
	  } 
	} 
	loc = 1 ; 
	{
	  if ( tokptr + 2 > maxtoks ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	    error () ; 
	    history = 3 ; 
	    jumpout () ; 
	  } 
	  tokmem [ tokptr ] = 32 ; 
	  tokptr = tokptr + 1 ; 
	} 
	do {
	    { 
	    if ( tokptr + 2 > maxtoks ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) 
	      ; 
	      error () ; 
	      history = 3 ; 
	      jumpout () ; 
	    } 
	    tokmem [ tokptr ] = 125 ; 
	    tokptr = tokptr + 1 ; 
	  } 
	  bal = bal - 1 ; 
	} while ( ! ( bal == 0 ) ) ; 
	goto lab30 ; 
      } 
    } 
    c = buffer [ loc ] ; 
    loc = loc + 1 ; 
    if ( c == 124 ) 
    goto lab30 ; 
    {
      if ( tokptr + 2 > maxtoks ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
      tokmem [ tokptr ] = c ; 
      tokptr = tokptr + 1 ; 
    } 
    if ( c == 64 ) 
    {
      loc = loc + 1 ; 
      if ( buffer [ loc - 1 ] != 64 ) 
      {
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Illegal use of @ in comment" ) ; 
	    error () ; 
	  } 
	} 
	loc = loc - 2 ; 
	tokptr = tokptr - 1 ; 
	{
	  if ( tokptr + 2 > maxtoks ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	    error () ; 
	    history = 3 ; 
	    jumpout () ; 
	  } 
	  tokmem [ tokptr ] = 32 ; 
	  tokptr = tokptr + 1 ; 
	} 
	do {
	    { 
	    if ( tokptr + 2 > maxtoks ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) 
	      ; 
	      error () ; 
	      history = 3 ; 
	      jumpout () ; 
	    } 
	    tokmem [ tokptr ] = 125 ; 
	    tokptr = tokptr + 1 ; 
	  } 
	  bal = bal - 1 ; 
	} while ( ! ( bal == 0 ) ) ; 
	goto lab30 ; 
      } 
    } 
    else if ( ( c == 92 ) && ( buffer [ loc ] != 64 ) ) 
    {
      {
	if ( tokptr + 2 > maxtoks ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	  error () ; 
	  history = 3 ; 
	  jumpout () ; 
	} 
	tokmem [ tokptr ] = buffer [ loc ] ; 
	tokptr = tokptr + 1 ; 
      } 
      loc = loc + 1 ; 
    } 
    else if ( c == 123 ) 
    bal = bal + 1 ; 
    else if ( c == 125 ) 
    {
      bal = bal - 1 ; 
      if ( bal == 0 ) 
      goto lab30 ; 
    } 
  } 
  lab30: Result = bal ; 
  return(Result) ; 
} 
void zred ( j , k , c , d ) 
sixteenbits j ; 
eightbits k ; 
eightbits c ; 
integer d ; 
{integer i  ; 
  cat [ j ] = c ; 
  trans [ j ] = textptr ; 
  textptr = textptr + 1 ; 
  tokstart [ textptr ] = tokptr ; 
  if ( k > 1 ) 
  {
    {register integer for_end; i = j + k ; for_end = loptr ; if ( i <= 
    for_end) do 
      {
	cat [ i - k + 1 ] = cat [ i ] ; 
	trans [ i - k + 1 ] = trans [ i ] ; 
      } 
    while ( i++ < for_end ) ; } 
    loptr = loptr - k + 1 ; 
  } 
  if ( pp + d >= scrapbase ) 
  pp = pp + d ; 
  else pp = scrapbase ; 
} 
void zsq ( j , k , c , d ) 
sixteenbits j ; 
eightbits k ; 
eightbits c ; 
integer d ; 
{integer i  ; 
  if ( k == 1 ) 
  {
    cat [ j ] = c ; 
    if ( pp + d >= scrapbase ) 
    pp = pp + d ; 
    else pp = scrapbase ; 
  } 
  else {
      
    {register integer for_end; i = j ; for_end = j + k - 1 ; if ( i <= 
    for_end) do 
      {
	tokmem [ tokptr ] = 40960L + trans [ i ] ; 
	tokptr = tokptr + 1 ; 
      } 
    while ( i++ < for_end ) ; } 
    red ( j , k , c , d ) ; 
  } 
} 
void fivecases ( ) 
{/* 31 */ switch ( cat [ pp ] ) 
  {case 5 : 
    if ( cat [ pp + 1 ] == 6 ) 
    {
      if ( ( cat [ pp + 2 ] == 10 ) || ( cat [ pp + 2 ] == 11 ) ) 
      {
	sq ( pp , 3 , 11 , -2 ) ; 
	goto lab31 ; 
      } 
    } 
    else if ( cat [ pp + 1 ] == 11 ) 
    {
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 140 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 5 , -1 ) ; 
      goto lab31 ; 
    } 
    break ; 
  case 3 : 
    if ( cat [ pp + 1 ] == 11 ) 
    {
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 32 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 138 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 55 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 135 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 11 , -2 ) ; 
      goto lab31 ; 
    } 
    break ; 
  case 2 : 
    if ( cat [ pp + 1 ] == 6 ) 
    {
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 1 , 11 , -2 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 14 ) 
    {
      tokmem [ tokptr ] = 141 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 139 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 3 , -3 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 2 ) 
    {
      sq ( pp , 2 , 2 , -1 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 1 ) 
    {
      sq ( pp , 2 , 2 , -1 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 11 ) 
    {
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 136 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 140 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 135 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 137 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 141 ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 11 , -2 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 10 ) 
    {
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 11 , -2 ) ; 
      goto lab31 ; 
    } 
    break ; 
  case 4 : 
    if ( ( cat [ pp + 1 ] == 17 ) && ( cat [ pp + 2 ] == 6 ) ) 
    {
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 135 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 135 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 137 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 3 , 2 , -1 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 6 ) 
    {
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 44 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 2 , -1 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 2 ) 
    {
      if ( ( cat [ pp + 2 ] == 17 ) && ( cat [ pp + 3 ] == 6 ) ) 
      {
	tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 36 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 135 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 135 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 137 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 36 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 40960L + trans [ pp + 3 ] ; 
	tokptr = tokptr + 1 ; 
	red ( pp , 4 , 2 , -1 ) ; 
	goto lab31 ; 
      } 
      else if ( cat [ pp + 2 ] == 6 ) 
      {
	sq ( pp , 3 , 2 , -1 ) ; 
	goto lab31 ; 
      } 
      else if ( cat [ pp + 2 ] == 14 ) 
      {
	sq ( pp + 1 , 2 , 2 , 0 ) ; 
	goto lab31 ; 
      } 
      else if ( cat [ pp + 2 ] == 16 ) 
      {
	if ( cat [ pp + 3 ] == 3 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 133 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 125 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp + 1 , 3 , 2 , 0 ) ; 
	  goto lab31 ; 
	} 
      } 
      else if ( cat [ pp + 2 ] == 9 ) 
      {
	tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 92 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 44 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 138 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 53 ; 
	tokptr = tokptr + 1 ; 
	red ( pp + 1 , 2 , 2 , 0 ) ; 
	goto lab31 ; 
      } 
      else if ( cat [ pp + 2 ] == 19 ) 
      {
	if ( cat [ pp + 3 ] == 3 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 133 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 125 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp + 1 , 3 , 2 , 0 ) ; 
	  goto lab31 ; 
	} 
      } 
    } 
    else if ( cat [ pp + 1 ] == 16 ) 
    {
      if ( cat [ pp + 2 ] == 3 ) 
      {
	tokmem [ tokptr ] = 133 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 135 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 125 ; 
	tokptr = tokptr + 1 ; 
	red ( pp + 1 , 2 , 2 , 0 ) ; 
	goto lab31 ; 
      } 
    } 
    else if ( cat [ pp + 1 ] == 1 ) 
    {
      sq ( pp + 1 , 1 , 2 , 0 ) ; 
      goto lab31 ; 
    } 
    else if ( ( cat [ pp + 1 ] == 11 ) && ( cat [ pp + 2 ] == 6 ) ) 
    {
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 135 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 135 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 3 , 2 , -1 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 19 ) 
    {
      if ( cat [ pp + 2 ] == 3 ) 
      {
	tokmem [ tokptr ] = 133 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 135 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 125 ; 
	tokptr = tokptr + 1 ; 
	red ( pp + 1 , 2 , 2 , 0 ) ; 
	goto lab31 ; 
      } 
    } 
    break ; 
  case 1 : 
    if ( cat [ pp + 1 ] == 6 ) 
    {
      sq ( pp , 1 , 11 , -2 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 14 ) 
    {
      tokmem [ tokptr ] = 141 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 139 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 2 , 3 , -3 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 2 ) 
    {
      sq ( pp , 2 , 2 , -1 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 22 ) 
    {
      sq ( pp , 2 , 22 , 0 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 1 ) 
    {
      sq ( pp , 2 , 1 , -2 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 1 ] == 10 ) 
    {
      sq ( pp , 2 , 11 , -2 ) ; 
      goto lab31 ; 
    } 
    break ; 
    default: 
    ; 
    break ; 
  } 
  pp = pp + 1 ; 
  lab31: ; 
} 
void alphacases ( ) 
{/* 31 */ if ( cat [ pp + 1 ] == 2 ) 
  {
    if ( cat [ pp + 2 ] == 14 ) 
    {
      sq ( pp + 1 , 2 , 2 , 0 ) ; 
      goto lab31 ; 
    } 
    else if ( cat [ pp + 2 ] == 8 ) 
    {
      tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 32 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 36 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 32 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 136 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
      tokptr = tokptr + 1 ; 
      red ( pp , 3 , 13 , -2 ) ; 
      goto lab31 ; 
    } 
  } 
  else if ( cat [ pp + 1 ] == 8 ) 
  {
    tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
    tokptr = tokptr + 1 ; 
    tokmem [ tokptr ] = 32 ; 
    tokptr = tokptr + 1 ; 
    tokmem [ tokptr ] = 136 ; 
    tokptr = tokptr + 1 ; 
    tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
    tokptr = tokptr + 1 ; 
    red ( pp , 2 , 13 , -2 ) ; 
    goto lab31 ; 
  } 
  else if ( cat [ pp + 1 ] == 1 ) 
  {
    sq ( pp + 1 , 1 , 2 , 0 ) ; 
    goto lab31 ; 
  } 
  pp = pp + 1 ; 
  lab31: ; 
} 
textpointer translate ( ) 
{/* 30 31 */ register textpointer Result; integer i  ; 
  integer j  ; 
  integer k  ; 
  pp = scrapbase ; 
  loptr = pp - 1 ; 
  hiptr = pp ; 
  while ( true ) {
      
    if ( loptr < pp + 3 ) 
    {
      do {
	  if ( hiptr <= scrapptr ) 
	{
	  loptr = loptr + 1 ; 
	  cat [ loptr ] = cat [ hiptr ] ; 
	  trans [ loptr ] = trans [ hiptr ] ; 
	  hiptr = hiptr + 1 ; 
	} 
      } while ( ! ( ( hiptr > scrapptr ) || ( loptr == pp + 3 ) ) ) ; 
      {register integer for_end; i = loptr + 1 ; for_end = pp + 3 ; if ( i <= 
      for_end) do 
	cat [ i ] = 0 ; 
      while ( i++ < for_end ) ; } 
    } 
    if ( ( tokptr + 8 > maxtoks ) || ( textptr + 4 > maxtexts ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token/text" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
    } 
    if ( pp > loptr ) 
    goto lab30 ; 
    if ( cat [ pp ] <= 7 ) 
    if ( cat [ pp ] < 7 ) 
    fivecases () ; 
    else alphacases () ; 
    else {
	
      switch ( cat [ pp ] ) 
      {case 17 : 
	if ( cat [ pp + 1 ] == 21 ) 
	{
	  if ( cat [ pp + 2 ] == 13 ) 
	  {
	    tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 137 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	    tokptr = tokptr + 1 ; 
	    red ( pp , 3 , 17 , 0 ) ; 
	    goto lab31 ; 
	  } 
	} 
	else if ( cat [ pp + 1 ] == 6 ) 
	{
	  if ( cat [ pp + 2 ] == 10 ) 
	  {
	    tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 135 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 137 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	    tokptr = tokptr + 1 ; 
	    red ( pp , 3 , 11 , -2 ) ; 
	    goto lab31 ; 
	  } 
	} 
	else if ( cat [ pp + 1 ] == 11 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 141 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 17 , 0 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 21 : 
	if ( cat [ pp + 1 ] == 13 ) 
	{
	  sq ( pp , 2 , 17 , 0 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 13 : 
	if ( cat [ pp + 1 ] == 11 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 140 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 137 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 141 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 11 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 12 : 
	if ( ( cat [ pp + 1 ] == 13 ) && ( cat [ pp + 2 ] == 11 ) ) 
	if ( cat [ pp + 3 ] == 20 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 140 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 3 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 32 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 4 , 13 , -2 ) ; 
	  goto lab31 ; 
	} 
	else {
	    
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 140 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 137 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 141 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 3 , 11 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 20 : 
	{
	  sq ( pp , 1 , 3 , -3 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 15 : 
	if ( cat [ pp + 1 ] == 2 ) 
	{
	  if ( cat [ pp + 2 ] == 1 ) 
	  if ( cat [ pp + 3 ] != 1 ) 
	  {
	    tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 125 ; 
	    tokptr = tokptr + 1 ; 
	    red ( pp , 3 , 2 , -1 ) ; 
	    goto lab31 ; 
	  } 
	} 
	else if ( cat [ pp + 1 ] == 1 ) 
	if ( cat [ pp + 2 ] != 1 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 125 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 2 , -1 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 22 : 
	if ( ( cat [ pp + 1 ] == 10 ) || ( cat [ pp + 1 ] == 9 ) ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 141 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 11 , -2 ) ; 
	  goto lab31 ; 
	} 
	else {
	    
	  sq ( pp , 1 , 1 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 16 : 
	if ( cat [ pp + 1 ] == 5 ) 
	{
	  if ( ( cat [ pp + 2 ] == 6 ) && ( cat [ pp + 3 ] == 10 ) ) 
	  {
	    tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 135 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 137 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 3 ] ; 
	    tokptr = tokptr + 1 ; 
	    red ( pp , 4 , 11 , -2 ) ; 
	    goto lab31 ; 
	  } 
	} 
	else if ( cat [ pp + 1 ] == 11 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 140 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 16 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 18 : 
	if ( ( cat [ pp + 1 ] == 3 ) && ( cat [ pp + 2 ] == 21 ) ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 32 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 3 , 21 , -2 ) ; 
	  goto lab31 ; 
	} 
	else {
	    
	  tokmem [ tokptr ] = 136 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 135 ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 1 , 17 , 0 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 9 : 
	{
	  sq ( pp , 1 , 10 , -3 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 11 : 
	if ( cat [ pp + 1 ] == 11 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 140 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 11 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 10 : 
	{
	  sq ( pp , 1 , 11 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
      case 19 : 
	if ( cat [ pp + 1 ] == 5 ) 
	{
	  sq ( pp , 1 , 11 , -2 ) ; 
	  goto lab31 ; 
	} 
	else if ( cat [ pp + 1 ] == 2 ) 
	{
	  if ( cat [ pp + 2 ] == 14 ) 
	  {
	    tokmem [ tokptr ] = 36 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 36 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 40960L + trans [ pp + 2 ] ; 
	    tokptr = tokptr + 1 ; 
	    red ( pp + 1 , 2 , 3 , 1 ) ; 
	    goto lab31 ; 
	  } 
	} 
	else if ( cat [ pp + 1 ] == 1 ) 
	{
	  if ( cat [ pp + 2 ] == 14 ) 
	  {
	    sq ( pp + 1 , 2 , 3 , 1 ) ; 
	    goto lab31 ; 
	  } 
	} 
	else if ( cat [ pp + 1 ] == 11 ) 
	{
	  tokmem [ tokptr ] = 40960L + trans [ pp ] ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 140 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 40960L + trans [ pp + 1 ] ; 
	  tokptr = tokptr + 1 ; 
	  red ( pp , 2 , 19 , -2 ) ; 
	  goto lab31 ; 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
      pp = pp + 1 ; 
      lab31: ; 
    } 
  } 
  lab30: ; 
  if ( ( loptr == scrapbase ) && ( cat [ loptr ] != 2 ) ) 
  Result = trans [ loptr ] ; 
  else {
      
    ; 
    {register integer for_end; j = scrapbase ; for_end = loptr ; if ( j <= 
    for_end) do 
      {
	if ( j != scrapbase ) 
	{
	  tokmem [ tokptr ] = 32 ; 
	  tokptr = tokptr + 1 ; 
	} 
	if ( cat [ j ] == 2 ) 
	{
	  tokmem [ tokptr ] = 36 ; 
	  tokptr = tokptr + 1 ; 
	} 
	tokmem [ tokptr ] = 40960L + trans [ j ] ; 
	tokptr = tokptr + 1 ; 
	if ( cat [ j ] == 2 ) 
	{
	  tokmem [ tokptr ] = 36 ; 
	  tokptr = tokptr + 1 ; 
	} 
	if ( tokptr + 6 > maxtoks ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	  error () ; 
	  history = 3 ; 
	  jumpout () ; 
	} 
      } 
    while ( j++ < for_end ) ; } 
    textptr = textptr + 1 ; 
    tokstart [ textptr ] = tokptr ; 
    Result = textptr - 1 ; 
  } 
  return(Result) ; 
} 
void appcomment ( ) 
{textptr = textptr + 1 ; 
  tokstart [ textptr ] = tokptr ; 
  if ( ( scrapptr < scrapbase ) || ( cat [ scrapptr ] < 8 ) || ( cat [ 
  scrapptr ] > 10 ) ) 
  {
    scrapptr = scrapptr + 1 ; 
    cat [ scrapptr ] = 10 ; 
    trans [ scrapptr ] = 0 ; 
  } 
  else {
      
    tokmem [ tokptr ] = 40960L + trans [ scrapptr ] ; 
    tokptr = tokptr + 1 ; 
  } 
  tokmem [ tokptr ] = textptr + 40959L ; 
  tokptr = tokptr + 1 ; 
  trans [ scrapptr ] = textptr ; 
  textptr = textptr + 1 ; 
  tokstart [ textptr ] = tokptr ; 
} 
void appoctal ( ) 
{tokmem [ tokptr ] = 92 ; 
  tokptr = tokptr + 1 ; 
  tokmem [ tokptr ] = 79 ; 
  tokptr = tokptr + 1 ; 
  tokmem [ tokptr ] = 123 ; 
  tokptr = tokptr + 1 ; 
  while ( ( buffer [ loc ] >= 48 ) && ( buffer [ loc ] <= 55 ) ) {
      
    {
      if ( tokptr + 2 > maxtoks ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
      tokmem [ tokptr ] = buffer [ loc ] ; 
      tokptr = tokptr + 1 ; 
    } 
    loc = loc + 1 ; 
  } 
  {
    tokmem [ tokptr ] = 125 ; 
    tokptr = tokptr + 1 ; 
    scrapptr = scrapptr + 1 ; 
    cat [ scrapptr ] = 1 ; 
    trans [ scrapptr ] = textptr ; 
    textptr = textptr + 1 ; 
    tokstart [ textptr ] = tokptr ; 
  } 
} 
void apphex ( ) 
{tokmem [ tokptr ] = 92 ; 
  tokptr = tokptr + 1 ; 
  tokmem [ tokptr ] = 72 ; 
  tokptr = tokptr + 1 ; 
  tokmem [ tokptr ] = 123 ; 
  tokptr = tokptr + 1 ; 
  while ( ( ( buffer [ loc ] >= 48 ) && ( buffer [ loc ] <= 57 ) ) || ( ( 
  buffer [ loc ] >= 65 ) && ( buffer [ loc ] <= 70 ) ) ) {
      
    {
      if ( tokptr + 2 > maxtoks ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
      tokmem [ tokptr ] = buffer [ loc ] ; 
      tokptr = tokptr + 1 ; 
    } 
    loc = loc + 1 ; 
  } 
  {
    tokmem [ tokptr ] = 125 ; 
    tokptr = tokptr + 1 ; 
    scrapptr = scrapptr + 1 ; 
    cat [ scrapptr ] = 1 ; 
    trans [ scrapptr ] = textptr ; 
    textptr = textptr + 1 ; 
    tokstart [ textptr ] = tokptr ; 
  } 
} 
void easycases ( ) 
{switch ( nextcontrol ) 
  {case 6 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 105 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 110 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 32 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 116 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 111 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 35 : 
  case 36 : 
  case 37 : 
  case 94 : 
  case 95 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = nextcontrol ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 0 : 
  case 124 : 
  case 131 : 
  case 132 : 
  case 133 : 
    ; 
    break ; 
  case 40 : 
  case 91 : 
    {
      tokmem [ tokptr ] = nextcontrol ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 4 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 41 : 
  case 93 : 
    {
      tokmem [ tokptr ] = nextcontrol ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 6 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 42 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 97 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 115 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 116 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 44 : 
    {
      tokmem [ tokptr ] = 44 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 138 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 57 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 46 : 
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
      tokmem [ tokptr ] = nextcontrol ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 1 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 59 : 
    {
      tokmem [ tokptr ] = 59 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 9 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 58 : 
    {
      tokmem [ tokptr ] = 58 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 14 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 26 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 73 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 28 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 76 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 29 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 71 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 30 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 83 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 4 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 87 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 31 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 86 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 5 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 82 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 24 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 75 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 128 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 69 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 123 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 15 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 9 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 66 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 10 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 84 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 12 : 
    appoctal () ; 
    break ; 
  case 13 : 
    apphex () ; 
    break ; 
  case 135 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 41 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 1 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 3 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 93 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 1 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 137 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 44 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 138 : 
    {
      tokmem [ tokptr ] = 138 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 48 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 1 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 139 : 
    {
      tokmem [ tokptr ] = 141 ; 
      tokptr = tokptr + 1 ; 
      appcomment () ; 
    } 
    break ; 
  case 140 : 
    {
      tokmem [ tokptr ] = 142 ; 
      tokptr = tokptr + 1 ; 
      appcomment () ; 
    } 
    break ; 
  case 141 : 
    {
      tokmem [ tokptr ] = 134 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 32 ; 
      tokptr = tokptr + 1 ; 
      {
	tokmem [ tokptr ] = 134 ; 
	tokptr = tokptr + 1 ; 
	appcomment () ; 
      } 
    } 
    break ; 
  case 142 : 
    {
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 9 ; 
      trans [ scrapptr ] = 0 ; 
    } 
    break ; 
  case 136 : 
    {
      tokmem [ tokptr ] = 92 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 74 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
    default: 
    {
      tokmem [ tokptr ] = nextcontrol ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  } 
} 
void zsubcases ( p ) 
namepointer p ; 
{switch ( ilk [ p ] ) 
  {case 0 : 
    {
      tokmem [ tokptr ] = 10240 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 1 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 4 : 
    {
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 7 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 7 : 
    {
      tokmem [ tokptr ] = 141 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 139 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 3 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 8 : 
    {
      tokmem [ tokptr ] = 131 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 125 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 9 : 
    {
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 8 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 12 : 
    {
      tokmem [ tokptr ] = 141 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 7 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 13 : 
    {
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 3 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 16 : 
    {
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 1 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  case 20 : 
    {
      tokmem [ tokptr ] = 132 ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 20480 + p ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 125 ; 
      tokptr = tokptr + 1 ; 
      scrapptr = scrapptr + 1 ; 
      cat [ scrapptr ] = 2 ; 
      trans [ scrapptr ] = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
    } 
    break ; 
  } 
} 
void Pascalparse ( ) 
{/* 21 10 */ integer j  ; 
  namepointer p  ; 
  while ( nextcontrol < 143 ) {
      
    if ( ( scrapptr + 4 > maxscraps ) || ( tokptr + 6 > maxtoks ) || ( textptr 
    + 4 > maxtexts ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "scrap/token/text" ,         " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
    } 
    lab21: switch ( nextcontrol ) 
    {case 129 : 
    case 2 : 
      {
	tokmem [ tokptr ] = 92 ; 
	tokptr = tokptr + 1 ; 
	if ( nextcontrol == 2 ) 
	{
	  tokmem [ tokptr ] = 61 ; 
	  tokptr = tokptr + 1 ; 
	} 
	else {
	    
	  tokmem [ tokptr ] = 46 ; 
	  tokptr = tokptr + 1 ; 
	} 
	tokmem [ tokptr ] = 123 ; 
	tokptr = tokptr + 1 ; 
	j = idfirst ; 
	while ( j < idloc ) {
	    
	  switch ( buffer [ j ] ) 
	  {case 32 : 
	  case 92 : 
	  case 35 : 
	  case 37 : 
	  case 36 : 
	  case 94 : 
	  case 39 : 
	  case 96 : 
	  case 123 : 
	  case 125 : 
	  case 126 : 
	  case 38 : 
	  case 95 : 
	    {
	      tokmem [ tokptr ] = 92 ; 
	      tokptr = tokptr + 1 ; 
	    } 
	    break ; 
	  case 64 : 
	    if ( buffer [ j + 1 ] == 64 ) 
	    j = j + 1 ; 
	    else {
		
	      if ( ! phaseone ) 
	      {
		(void) putc('\n',  stdout );
		(void) Fputs( stdout ,  "! Double @ should be used in strings" ) ; 
		error () ; 
	      } 
	    } 
	    break ; 
	    default: 
	    ; 
	    break ; 
	  } 
	  {
	    if ( tokptr + 2 > maxtoks ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) 
	      ; 
	      error () ; 
	      history = 3 ; 
	      jumpout () ; 
	    } 
	    tokmem [ tokptr ] = buffer [ j ] ; 
	    tokptr = tokptr + 1 ; 
	  } 
	  j = j + 1 ; 
	} 
	{
	  tokmem [ tokptr ] = 125 ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 1 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
      } 
      break ; 
    case 130 : 
      {
	p = idlookup ( 0 ) ; 
	switch ( ilk [ p ] ) 
	{case 0 : 
	case 4 : 
	case 7 : 
	case 8 : 
	case 9 : 
	case 12 : 
	case 13 : 
	case 16 : 
	case 20 : 
	  subcases ( p ) ; 
	  break ; 
	case 5 : 
	  {
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 135 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 5 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 3 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	  } 
	  break ; 
	case 6 : 
	  {
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 21 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 7 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	  } 
	  break ; 
	case 10 : 
	  {
	    if ( ( scrapptr < scrapbase ) || ( ( cat [ scrapptr ] != 10 ) && ( 
	    cat [ scrapptr ] != 9 ) ) ) 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 10 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 139 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 20 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	  } 
	  break ; 
	case 11 : 
	  {
	    if ( ( scrapptr < scrapbase ) || ( ( cat [ scrapptr ] != 10 ) && ( 
	    cat [ scrapptr ] != 9 ) ) ) 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 10 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 6 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	  } 
	  break ; 
	case 14 : 
	  {
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 12 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 7 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	  } 
	  break ; 
	case 23 : 
	  {
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 92 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 126 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 7 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 8 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	  } 
	  break ; 
	case 17 : 
	  {
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 139 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 135 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 16 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      tokmem [ tokptr ] = 136 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 92 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 32 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 3 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	  } 
	  break ; 
	case 18 : 
	  {
	    {
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 18 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 3 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	  } 
	  break ; 
	case 19 : 
	  {
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 136 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 135 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 5 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 3 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	  } 
	  break ; 
	case 21 : 
	  {
	    if ( ( scrapptr < scrapbase ) || ( ( cat [ scrapptr ] != 10 ) && ( 
	    cat [ scrapptr ] != 9 ) ) ) 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 10 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 139 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 6 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 13 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	  } 
	  break ; 
	case 22 : 
	  {
	    {
	      tokmem [ tokptr ] = 141 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 139 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 20480 + p ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 135 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 19 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    {
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 3 ; 
	      trans [ scrapptr ] = 0 ; 
	    } 
	  } 
	  break ; 
	  default: 
	  {
	    nextcontrol = ilk [ p ] - 24 ; 
	    goto lab21 ; 
	  } 
	  break ; 
	} 
      } 
      break ; 
    case 134 : 
      {
	tokmem [ tokptr ] = 92 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 104 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 98 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 111 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 120 ; 
	tokptr = tokptr + 1 ; 
	tokmem [ tokptr ] = 123 ; 
	tokptr = tokptr + 1 ; 
	{register integer for_end; j = idfirst ; for_end = idloc - 1 ; if ( j 
	<= for_end) do 
	  {
	    if ( tokptr + 2 > maxtoks ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) 
	      ; 
	      error () ; 
	      history = 3 ; 
	      jumpout () ; 
	    } 
	    tokmem [ tokptr ] = buffer [ j ] ; 
	    tokptr = tokptr + 1 ; 
	  } 
	while ( j++ < for_end ) ; } 
	{
	  tokmem [ tokptr ] = 125 ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 1 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
      } 
      break ; 
      default: 
      easycases () ; 
      break ; 
    } 
    nextcontrol = getnext () ; 
    if ( ( nextcontrol == 124 ) || ( nextcontrol == 123 ) ) 
    goto lab10 ; 
  } 
  lab10: ; 
} 
textpointer Pascaltranslate ( ) 
{register textpointer Result; textpointer p  ; 
  integer savebase  ; 
  savebase = scrapbase ; 
  scrapbase = scrapptr + 1 ; 
  Pascalparse () ; 
  if ( nextcontrol != 124 ) 
  {
    if ( ! phaseone ) 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "! Missing \"|\" after Pascal text" ) ; 
      error () ; 
    } 
  } 
  {
    if ( tokptr + 2 > maxtoks ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
      error () ; 
      history = 3 ; 
      jumpout () ; 
    } 
    tokmem [ tokptr ] = 135 ; 
    tokptr = tokptr + 1 ; 
  } 
  appcomment () ; 
  p = translate () ; 
  scrapptr = scrapbase - 1 ; 
  scrapbase = savebase ; 
  Result = p ; 
  return(Result) ; 
} 
void outerparse ( ) 
{eightbits bal  ; 
  textpointer p, q  ; 
  while ( nextcontrol < 143 ) if ( nextcontrol != 123 ) 
  Pascalparse () ; 
  else {
      
    if ( ( tokptr + 7 > maxtoks ) || ( textptr + 3 > maxtexts ) || ( scrapptr 
    >= maxscraps ) ) 
    {
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token/text/scrap" ,         " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
    } 
    tokmem [ tokptr ] = 92 ; 
    tokptr = tokptr + 1 ; 
    tokmem [ tokptr ] = 67 ; 
    tokptr = tokptr + 1 ; 
    tokmem [ tokptr ] = 123 ; 
    tokptr = tokptr + 1 ; 
    bal = copycomment ( 1 ) ; 
    nextcontrol = 124 ; 
    while ( bal > 0 ) {
	
      p = textptr ; 
      textptr = textptr + 1 ; 
      tokstart [ textptr ] = tokptr ; 
      q = Pascaltranslate () ; 
      tokmem [ tokptr ] = 40960L + p ; 
      tokptr = tokptr + 1 ; 
      tokmem [ tokptr ] = 51200L + q ; 
      tokptr = tokptr + 1 ; 
      if ( nextcontrol == 124 ) 
      bal = copycomment ( bal ) ; 
      else bal = 0 ; 
    } 
    tokmem [ tokptr ] = 141 ; 
    tokptr = tokptr + 1 ; 
    appcomment () ; 
  } 
} 
void zpushlevel ( p ) 
textpointer p ; 
{if ( stackptr == stacksize ) 
  {
    (void) putc('\n',  stdout );
    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "stack" , " capacity exceeded" ) ; 
    error () ; 
    history = 3 ; 
    jumpout () ; 
  } 
  else {
      
    if ( stackptr > 0 ) 
    stack [ stackptr ] = curstate ; 
    stackptr = stackptr + 1 ; 
    curstate .tokfield = tokstart [ p ] ; 
    curstate .endfield = tokstart [ p + 1 ] ; 
  } 
} 
eightbits getoutput ( ) 
{/* 20 */ register eightbits Result; sixteenbits a  ; 
  lab20: while ( curstate .tokfield == curstate .endfield ) {
      
    stackptr = stackptr - 1 ; 
    curstate = stack [ stackptr ] ; 
  } 
  a = tokmem [ curstate .tokfield ] ; 
  curstate .tokfield = curstate .tokfield + 1 ; 
  if ( a >= 256 ) 
  {
    curname = a % 10240 ; 
    switch ( a / 10240 ) 
    {case 2 : 
      a = 129 ; 
      break ; 
    case 3 : 
      a = 128 ; 
      break ; 
    case 4 : 
      {
	pushlevel ( curname ) ; 
	goto lab20 ; 
      } 
      break ; 
    case 5 : 
      {
	pushlevel ( curname ) ; 
	curstate .modefield = 0 ; 
	goto lab20 ; 
      } 
      break ; 
      default: 
      a = 130 ; 
      break ; 
    } 
  } 
  Result = a ; 
  return(Result) ; 
} 
void outputPascal ( ) 
{sixteenbits savetokptr, savetextptr, savenextcontrol  ; 
  textpointer p  ; 
  savetokptr = tokptr ; 
  savetextptr = textptr ; 
  savenextcontrol = nextcontrol ; 
  nextcontrol = 124 ; 
  p = Pascaltranslate () ; 
  tokmem [ tokptr ] = p + 51200L ; 
  tokptr = tokptr + 1 ; 
  makeoutput () ; 
  textptr = savetextptr ; 
  tokptr = savetokptr ; 
  nextcontrol = savenextcontrol ; 
} 
void makeoutput ( ) 
{/* 21 10 31 */ eightbits a  ; 
  eightbits b  ; 
  integer k, klimit  ; 
  schar w  ; 
  integer j  ; 
  ASCIIcode stringdelimiter  ; 
  integer saveloc, savelimit  ; 
  namepointer curmodname  ; 
  mode savemode  ; 
  tokmem [ tokptr ] = 143 ; 
  tokptr = tokptr + 1 ; 
  textptr = textptr + 1 ; 
  tokstart [ textptr ] = tokptr ; 
  pushlevel ( textptr - 1 ) ; 
  while ( true ) {
      
    a = getoutput () ; 
    lab21: switch ( a ) 
    {case 143 : 
      goto lab10 ; 
      break ; 
    case 130 : 
    case 129 : 
      {
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 92 ; 
	} 
	if ( a == 130 ) 
	if ( bytestart [ curname + 2 ] - bytestart [ curname ] == 1 ) 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 124 ; 
	} 
	else {
	    
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 92 ; 
	} 
	else {
	    
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 38 ; 
	} 
	if ( bytestart [ curname + 2 ] - bytestart [ curname ] == 1 ) 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = bytemem [ curname % 2 ][ bytestart [ curname ] ] 
	  ; 
	} 
	else outname ( curname ) ; 
      } 
      break ; 
    case 128 : 
      {
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 92 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 88 ; 
	} 
	curxref = xref [ curname ] ; 
	if ( xmem [ curxref ] .numfield >= 10240 ) 
	{
	  outmod ( xmem [ curxref ] .numfield - 10240 ) ; 
	  if ( phasethree ) 
	  {
	    curxref = xmem [ curxref ] .xlinkfield ; 
	    while ( xmem [ curxref ] .numfield >= 10240 ) {
		
	      {
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = 44 ; 
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = 32 ; 
	      } 
	      outmod ( xmem [ curxref ] .numfield - 10240 ) ; 
	      curxref = xmem [ curxref ] .xlinkfield ; 
	    } 
	  } 
	} 
	else {
	    
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 48 ; 
	} 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 58 ; 
	} 
	k = bytestart [ curname ] ; 
	w = curname % 2 ; 
	klimit = bytestart [ curname + 2 ] ; 
	curmodname = curname ; 
	while ( k < klimit ) {
	    
	  b = bytemem [ w ][ k ] ; 
	  k = k + 1 ; 
	  if ( b == 64 ) 
	  {
	    if ( bytemem [ w ][ k ] != 64 ) 
	    {
	      {
		(void) putc('\n',  stdout );
		(void) Fputs( stdout ,  "! Illegal control code in section name:" ) ; 
	      } 
	      {
		(void) putc('\n',  stdout );
		(void) putc( '<' ,  stdout );
	      } 
	      printid ( curmodname ) ; 
	      (void) Fputs( stdout ,  "> " ) ; 
	      history = 2 ; 
	    } 
	    k = k + 1 ; 
	  } 
	  if ( b != 124 ) 
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = b ; 
	  } 
	  else {
	      
	    j = limit + 1 ; 
	    buffer [ j ] = 124 ; 
	    stringdelimiter = 0 ; 
	    while ( true ) {
		
	      if ( k >= klimit ) 
	      {
		{
		  (void) putc('\n',  stdout );
		  (void) Fputs( stdout ,  "! Pascal text in section name didn't end:"                   ) ; 
		} 
		{
		  (void) putc('\n',  stdout );
		  (void) putc( '<' ,  stdout );
		} 
		printid ( curmodname ) ; 
		(void) Fputs( stdout ,  "> " ) ; 
		history = 2 ; 
		goto lab31 ; 
	      } 
	      b = bytemem [ w ][ k ] ; 
	      k = k + 1 ; 
	      if ( b == 64 ) 
	      {
		if ( j > longbufsize - 4 ) 
		{
		  (void) putc('\n',  stdout );
		  (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "buffer" ,                   " capacity exceeded" ) ; 
		  error () ; 
		  history = 3 ; 
		  jumpout () ; 
		} 
		buffer [ j + 1 ] = 64 ; 
		buffer [ j + 2 ] = bytemem [ w ][ k ] ; 
		j = j + 2 ; 
		k = k + 1 ; 
	      } 
	      else {
		  
		if ( ( b == 34 ) || ( b == 39 ) ) 
		if ( stringdelimiter == 0 ) 
		stringdelimiter = b ; 
		else if ( stringdelimiter == b ) 
		stringdelimiter = 0 ; 
		if ( ( b != 124 ) || ( stringdelimiter != 0 ) ) 
		{
		  if ( j > longbufsize - 3 ) 
		  {
		    (void) putc('\n',  stdout );
		    (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "buffer" ,                     " capacity exceeded" ) ; 
		    error () ; 
		    history = 3 ; 
		    jumpout () ; 
		  } 
		  j = j + 1 ; 
		  buffer [ j ] = b ; 
		} 
		else goto lab31 ; 
	      } 
	    } 
	    lab31: ; 
	    saveloc = loc ; 
	    savelimit = limit ; 
	    loc = limit + 2 ; 
	    limit = j + 1 ; 
	    buffer [ limit ] = 124 ; 
	    outputPascal () ; 
	    loc = saveloc ; 
	    limit = savelimit ; 
	  } 
	} 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 92 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 88 ; 
	} 
      } 
      break ; 
    case 131 : 
    case 133 : 
    case 132 : 
      {
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 92 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 109 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 97 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 116 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 104 ; 
	} 
	if ( a == 131 ) 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 98 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 105 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 110 ; 
	} 
	else if ( a == 132 ) 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 114 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 101 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 108 ; 
	} 
	else {
	    
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 111 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 112 ; 
	} 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 123 ; 
	} 
      } 
      break ; 
    case 135 : 
      {
	do {
	    a = getoutput () ; 
	} while ( ! ( ( a < 139 ) || ( a > 142 ) ) ) ; 
	goto lab21 ; 
      } 
      break ; 
    case 134 : 
      {
	do {
	    a = getoutput () ; 
	} while ( ! ( ( ( a < 139 ) && ( a != 32 ) ) || ( a > 142 ) ) ) ; 
	goto lab21 ; 
      } 
      break ; 
    case 136 : 
    case 137 : 
    case 138 : 
    case 139 : 
    case 140 : 
    case 141 : 
    case 142 : 
      if ( a < 140 ) 
      {
	if ( curstate .modefield == 1 ) 
	{
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 92 ; 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = a - 87 ; 
	  } 
	  if ( a == 138 ) 
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = getoutput () ; 
	  } 
	} 
	else if ( a == 138 ) 
	b = getoutput () ; 
      } 
      else {
	  
	b = a ; 
	savemode = curstate .modefield ; 
	while ( true ) {
	    
	  a = getoutput () ; 
	  if ( ( a == 135 ) || ( a == 134 ) ) 
	  goto lab21 ; 
	  if ( ( ( a != 32 ) && ( a < 140 ) ) || ( a > 142 ) ) 
	  {
	    if ( savemode == 1 ) 
	    {
	      if ( outptr > 3 ) 
	      if ( ( outbuf [ outptr ] == 80 ) && ( outbuf [ outptr - 1 ] == 
	      92 ) && ( outbuf [ outptr - 2 ] == 89 ) && ( outbuf [ outptr - 3 
	      ] == 92 ) ) 
	      goto lab21 ; 
	      {
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = 92 ; 
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = b - 87 ; 
	      } 
	      if ( a != 143 ) 
	      finishline () ; 
	    } 
	    else if ( ( a != 143 ) && ( curstate .modefield == 0 ) ) 
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 32 ; 
	    } 
	    goto lab21 ; 
	  } 
	  if ( a > b ) 
	  b = a ; 
	} 
      } 
      break ; 
      default: 
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = a ; 
      } 
      break ; 
    } 
  } 
  lab10: ; 
} 
void finishPascal ( ) 
{textpointer p  ; 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 92 ; 
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 80 ; 
  } 
  {
    if ( tokptr + 2 > maxtoks ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "token" , " capacity exceeded" ) ; 
      error () ; 
      history = 3 ; 
      jumpout () ; 
    } 
    tokmem [ tokptr ] = 141 ; 
    tokptr = tokptr + 1 ; 
  } 
  appcomment () ; 
  p = translate () ; 
  tokmem [ tokptr ] = p + 40960L ; 
  tokptr = tokptr + 1 ; 
  makeoutput () ; 
  if ( outptr > 1 ) 
  if ( outbuf [ outptr - 1 ] == 92 ) 
  if ( outbuf [ outptr ] == 54 ) 
  outptr = outptr - 2 ; 
  else if ( outbuf [ outptr ] == 55 ) 
  outbuf [ outptr ] = 89 ; 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 92 ; 
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 112 ; 
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 97 ; 
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 114 ; 
  } 
  finishline () ; 
  tokptr = 1 ; 
  textptr = 1 ; 
  scrapptr = 0 ; 
} 
void zfootnote ( flag ) 
sixteenbits flag ; 
{/* 30 10 */ xrefnumber q  ; 
  if ( xmem [ curxref ] .numfield <= flag ) 
  goto lab10 ; 
  finishline () ; 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 92 ; 
  } 
  if ( flag == 0 ) 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 85 ; 
  } 
  else {
      
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 65 ; 
  } 
  q = curxref ; 
  if ( xmem [ xmem [ q ] .xlinkfield ] .numfield > flag ) 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 115 ; 
  } 
  while ( true ) {
      
    outmod ( xmem [ curxref ] .numfield - flag ) ; 
    curxref = xmem [ curxref ] .xlinkfield ; 
    if ( xmem [ curxref ] .numfield <= flag ) 
    goto lab30 ; 
    if ( xmem [ xmem [ curxref ] .xlinkfield ] .numfield > flag ) 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 44 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 32 ; 
    } 
    else {
	
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 92 ; 
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 69 ; 
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 84 ; 
      } 
      if ( curxref != xmem [ q ] .xlinkfield ) 
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 115 ; 
      } 
    } 
  } 
  lab30: ; 
  {
    if ( outptr == linelength ) 
    breakout () ; 
    outptr = outptr + 1 ; 
    outbuf [ outptr ] = 46 ; 
  } 
  lab10: ; 
} 
void zunbucket ( d ) 
eightbits d ; 
{ASCIIcode c  ; 
  {register integer for_end; c = 229 ; for_end = 0 ; if ( c >= for_end) do 
    if ( bucket [ collate [ c ] ] > 0 ) 
    {
      if ( scrapptr > maxscraps ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "sorting" , " capacity exceeded" ) ; 
	error () ; 
	history = 3 ; 
	jumpout () ; 
      } 
      scrapptr = scrapptr + 1 ; 
      if ( c == 0 ) 
      cat [ scrapptr ] = 255 ; 
      else cat [ scrapptr ] = d ; 
      trans [ scrapptr ] = bucket [ collate [ c ] ] ; 
      bucket [ collate [ c ] ] = 0 ; 
    } 
  while ( c-- > for_end ) ; } 
} 
void zmodprint ( p ) 
namepointer p ; 
{if ( p > 0 ) 
  {
    modprint ( link [ p ] ) ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 58 ; 
    } 
    tokptr = 1 ; 
    textptr = 1 ; 
    scrapptr = 0 ; 
    stackptr = 0 ; 
    curstate .modefield = 1 ; 
    tokmem [ tokptr ] = p + 30720 ; 
    tokptr = tokptr + 1 ; 
    makeoutput () ; 
    footnote ( 0 ) ; 
    finishline () ; 
    modprint ( ilk [ p ] ) ; 
  } 
} 
void PhaseI ( ) 
{phaseone = true ; 
  phasethree = false ; 
  resetinput () ; 
  modulecount = 0 ; 
  skiplimbo () ; 
  changeexists = false ; 
  while ( ! inputhasended ) {
      
    modulecount = modulecount + 1 ; 
    if ( modulecount == maxmodules ) 
    {
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%s%s%s",  "! Sorry, " , "section number" , " capacity exceeded" ) 
      ; 
      error () ; 
      history = 3 ; 
      jumpout () ; 
    } 
    changedmodule [ modulecount ] = changing ; 
    if ( buffer [ loc - 1 ] == 42 ) 
    {
      (void) fprintf( stdout , "%c%ld",  '*' , (long)modulecount ) ; 
      flush ( stdout ) ; 
    } 
    do {
	nextcontrol = skipTeX () ; 
      switch ( nextcontrol ) 
      {case 126 : 
	xrefswitch = 10240 ; 
	break ; 
      case 125 : 
	xrefswitch = 0 ; 
	break ; 
      case 124 : 
	Pascalxref () ; 
	break ; 
      case 131 : 
      case 132 : 
      case 133 : 
      case 146 : 
	{
	  loc = loc - 2 ; 
	  nextcontrol = getnext () ; 
	  if ( nextcontrol != 146 ) 
	  newxref ( idlookup ( nextcontrol - 130 ) ) ; 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
    } while ( ! ( nextcontrol >= 143 ) ) ; 
    while ( nextcontrol <= 144 ) {
	
      xrefswitch = 10240 ; 
      if ( nextcontrol == 144 ) 
      nextcontrol = getnext () ; 
      else {
	  
	nextcontrol = getnext () ; 
	if ( nextcontrol == 130 ) 
	{
	  lhs = idlookup ( 0 ) ; 
	  ilk [ lhs ] = 0 ; 
	  newxref ( lhs ) ; 
	  nextcontrol = getnext () ; 
	  if ( nextcontrol == 30 ) 
	  {
	    nextcontrol = getnext () ; 
	    if ( nextcontrol == 130 ) 
	    {
	      rhs = idlookup ( 0 ) ; 
	      ilk [ lhs ] = ilk [ rhs ] ; 
	      ilk [ rhs ] = 0 ; 
	      newxref ( rhs ) ; 
	      ilk [ rhs ] = ilk [ lhs ] ; 
	      nextcontrol = getnext () ; 
	    } 
	  } 
	} 
      } 
      outerxref () ; 
    } 
    if ( nextcontrol <= 146 ) 
    {
      if ( nextcontrol == 145 ) 
      modxrefswitch = 0 ; 
      else modxrefswitch = 10240 ; 
      do {
	  if ( nextcontrol == 146 ) 
	newmodxref ( curmodule ) ; 
	nextcontrol = getnext () ; 
	outerxref () ; 
      } while ( ! ( nextcontrol > 146 ) ) ; 
    } 
    if ( changedmodule [ modulecount ] ) 
    changeexists = true ; 
  } 
  changedmodule [ modulecount ] = changeexists ; 
  phaseone = false ; 
  modcheck ( ilk [ 0 ] ) ; 
} 
void PhaseII ( ) 
{resetinput () ; 
  {
    (void) putc('\n',  stdout );
    (void) Fputs( stdout ,  "Writing the output file..." ) ; 
  } 
  modulecount = 0 ; 
  copylimbo () ; 
  finishline () ; 
  flushbuffer ( 0 , false , false ) ; 
  while ( ! inputhasended ) {
      
    modulecount = modulecount + 1 ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
    } 
    if ( buffer [ loc - 1 ] != 42 ) 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 77 ; 
    } 
    else {
	
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 78 ; 
      } 
      (void) fprintf( stdout , "%c%ld",  '*' , (long)modulecount ) ; 
      flush ( stdout ) ; 
    } 
    outmod ( modulecount ) ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 46 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 32 ; 
    } 
    saveline = outline ; 
    saveplace = outptr ; 
    do {
	nextcontrol = copyTeX () ; 
      switch ( nextcontrol ) 
      {case 124 : 
	{
	  stackptr = 0 ; 
	  curstate .modefield = 1 ; 
	  outputPascal () ; 
	} 
	break ; 
      case 64 : 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 64 ; 
	} 
	break ; 
      case 12 : 
	{
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 92 ; 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 79 ; 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 123 ; 
	  } 
	  while ( ( buffer [ loc ] >= 48 ) && ( buffer [ loc ] <= 55 ) ) {
	      
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = buffer [ loc ] ; 
	    } 
	    loc = loc + 1 ; 
	  } 
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 125 ; 
	  } 
	} 
	break ; 
      case 13 : 
	{
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 92 ; 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 72 ; 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 123 ; 
	  } 
	  while ( ( ( buffer [ loc ] >= 48 ) && ( buffer [ loc ] <= 57 ) ) || 
	  ( ( buffer [ loc ] >= 65 ) && ( buffer [ loc ] <= 70 ) ) ) {
	      
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = buffer [ loc ] ; 
	    } 
	    loc = loc + 1 ; 
	  } 
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 125 ; 
	  } 
	} 
	break ; 
      case 134 : 
      case 131 : 
      case 132 : 
      case 133 : 
      case 146 : 
	{
	  loc = loc - 2 ; 
	  nextcontrol = getnext () ; 
	  if ( nextcontrol == 134 ) 
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! TeX string should be in Pascal text only" ) 
	      ; 
	      error () ; 
	    } 
	  } 
	} 
	break ; 
      case 9 : 
      case 10 : 
      case 135 : 
      case 137 : 
      case 138 : 
      case 139 : 
      case 140 : 
      case 141 : 
      case 136 : 
      case 142 : 
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! You can't do that in TeX text" ) ; 
	    error () ; 
	  } 
	} 
	break ; 
	default: 
	; 
	break ; 
      } 
    } while ( ! ( nextcontrol >= 143 ) ) ; 
    if ( nextcontrol <= 144 ) 
    {
      if ( ( saveline != outline ) || ( saveplace != outptr ) ) 
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 92 ; 
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 89 ; 
      } 
      saveline = outline ; 
      saveplace = outptr ; 
    } 
    while ( nextcontrol <= 144 ) {
	
      stackptr = 0 ; 
      curstate .modefield = 1 ; 
      if ( nextcontrol == 144 ) 
      {
	{
	  tokmem [ tokptr ] = 92 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 68 ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 3 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
	nextcontrol = getnext () ; 
	if ( nextcontrol != 130 ) 
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Improper macro definition" ) ; 
	    error () ; 
	  } 
	} 
	else {
	    
	  tokmem [ tokptr ] = 10240 + idlookup ( 0 ) ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 2 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
	nextcontrol = getnext () ; 
      } 
      else {
	  
	{
	  tokmem [ tokptr ] = 92 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 70 ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 3 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
	nextcontrol = getnext () ; 
	if ( nextcontrol == 130 ) 
	{
	  {
	    tokmem [ tokptr ] = 10240 + idlookup ( 0 ) ; 
	    tokptr = tokptr + 1 ; 
	    scrapptr = scrapptr + 1 ; 
	    cat [ scrapptr ] = 2 ; 
	    trans [ scrapptr ] = textptr ; 
	    textptr = textptr + 1 ; 
	    tokstart [ textptr ] = tokptr ; 
	  } 
	  nextcontrol = getnext () ; 
	  if ( nextcontrol == 30 ) 
	  {
	    {
	      tokmem [ tokptr ] = 92 ; 
	      tokptr = tokptr + 1 ; 
	      tokmem [ tokptr ] = 83 ; 
	      tokptr = tokptr + 1 ; 
	      scrapptr = scrapptr + 1 ; 
	      cat [ scrapptr ] = 2 ; 
	      trans [ scrapptr ] = textptr ; 
	      textptr = textptr + 1 ; 
	      tokstart [ textptr ] = tokptr ; 
	    } 
	    nextcontrol = getnext () ; 
	    if ( nextcontrol == 130 ) 
	    {
	      {
		tokmem [ tokptr ] = 10240 + idlookup ( 0 ) ; 
		tokptr = tokptr + 1 ; 
		scrapptr = scrapptr + 1 ; 
		cat [ scrapptr ] = 2 ; 
		trans [ scrapptr ] = textptr ; 
		textptr = textptr + 1 ; 
		tokstart [ textptr ] = tokptr ; 
	      } 
	      {
		scrapptr = scrapptr + 1 ; 
		cat [ scrapptr ] = 9 ; 
		trans [ scrapptr ] = 0 ; 
	      } 
	      nextcontrol = getnext () ; 
	    } 
	  } 
	} 
	if ( scrapptr != 5 ) 
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! Improper format definition" ) ; 
	    error () ; 
	  } 
	} 
      } 
      outerparse () ; 
      finishPascal () ; 
    } 
    thismodule = 0 ; 
    if ( nextcontrol <= 146 ) 
    {
      if ( ( saveline != outline ) || ( saveplace != outptr ) ) 
      {
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 92 ; 
	if ( outptr == linelength ) 
	breakout () ; 
	outptr = outptr + 1 ; 
	outbuf [ outptr ] = 89 ; 
      } 
      stackptr = 0 ; 
      curstate .modefield = 1 ; 
      if ( nextcontrol == 145 ) 
      nextcontrol = getnext () ; 
      else {
	  
	thismodule = curmodule ; 
	do {
	    nextcontrol = getnext () ; 
	} while ( ! ( nextcontrol != 43 ) ) ; 
	if ( ( nextcontrol != 61 ) && ( nextcontrol != 30 ) ) 
	{
	  if ( ! phaseone ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) Fputs( stdout ,  "! You need an = sign after the section name" ) ; 
	    error () ; 
	  } 
	} 
	else nextcontrol = getnext () ; 
	if ( outptr > 1 ) 
	if ( ( outbuf [ outptr ] == 89 ) && ( outbuf [ outptr - 1 ] == 92 ) ) 
	{
	  tokmem [ tokptr ] = 139 ; 
	  tokptr = tokptr + 1 ; 
	} 
	{
	  tokmem [ tokptr ] = 30720 + thismodule ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 22 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
	curxref = xref [ thismodule ] ; 
	if ( xmem [ curxref ] .numfield != modulecount + 10240 ) 
	{
	  {
	    tokmem [ tokptr ] = 132 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 43 ; 
	    tokptr = tokptr + 1 ; 
	    tokmem [ tokptr ] = 125 ; 
	    tokptr = tokptr + 1 ; 
	    scrapptr = scrapptr + 1 ; 
	    cat [ scrapptr ] = 2 ; 
	    trans [ scrapptr ] = textptr ; 
	    textptr = textptr + 1 ; 
	    tokstart [ textptr ] = tokptr ; 
	  } 
	  thismodule = 0 ; 
	} 
	{
	  tokmem [ tokptr ] = 92 ; 
	  tokptr = tokptr + 1 ; 
	  tokmem [ tokptr ] = 83 ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 2 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
	{
	  tokmem [ tokptr ] = 141 ; 
	  tokptr = tokptr + 1 ; 
	  scrapptr = scrapptr + 1 ; 
	  cat [ scrapptr ] = 9 ; 
	  trans [ scrapptr ] = textptr ; 
	  textptr = textptr + 1 ; 
	  tokstart [ textptr ] = tokptr ; 
	} 
      } 
      while ( nextcontrol <= 146 ) {
	  
	outerparse () ; 
	if ( nextcontrol < 146 ) 
	{
	  {
	    if ( ! phaseone ) 
	    {
	      (void) putc('\n',  stdout );
	      (void) Fputs( stdout ,  "! You can't do that in Pascal text" ) ; 
	      error () ; 
	    } 
	  } 
	  nextcontrol = getnext () ; 
	} 
	else if ( nextcontrol == 146 ) 
	{
	  {
	    tokmem [ tokptr ] = 30720 + curmodule ; 
	    tokptr = tokptr + 1 ; 
	    scrapptr = scrapptr + 1 ; 
	    cat [ scrapptr ] = 22 ; 
	    trans [ scrapptr ] = textptr ; 
	    textptr = textptr + 1 ; 
	    tokstart [ textptr ] = tokptr ; 
	  } 
	  nextcontrol = getnext () ; 
	} 
      } 
      finishPascal () ; 
    } 
    if ( thismodule > 0 ) 
    {
      firstxref = xref [ thismodule ] ; 
      thisxref = xmem [ firstxref ] .xlinkfield ; 
      if ( xmem [ thisxref ] .numfield > 10240 ) 
      {
	midxref = thisxref ; 
	curxref = 0 ; 
	do {
	    nextxref = xmem [ thisxref ] .xlinkfield ; 
	  xmem [ thisxref ] .xlinkfield = curxref ; 
	  curxref = thisxref ; 
	  thisxref = nextxref ; 
	} while ( ! ( xmem [ thisxref ] .numfield <= 10240 ) ) ; 
	xmem [ firstxref ] .xlinkfield = curxref ; 
      } 
      else midxref = 0 ; 
      curxref = 0 ; 
      while ( thisxref != 0 ) {
	  
	nextxref = xmem [ thisxref ] .xlinkfield ; 
	xmem [ thisxref ] .xlinkfield = curxref ; 
	curxref = thisxref ; 
	thisxref = nextxref ; 
      } 
      if ( midxref > 0 ) 
      xmem [ midxref ] .xlinkfield = curxref ; 
      else xmem [ firstxref ] .xlinkfield = curxref ; 
      curxref = xmem [ firstxref ] .xlinkfield ; 
      footnote ( 10240 ) ; 
      footnote ( 0 ) ; 
    } 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 102 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 105 ; 
    } 
    finishline () ; 
    flushbuffer ( 0 , false , false ) ; 
  } 
} 
void main_body() {
    
  initialize () ; 
  (void) fprintf( stdout , "%s\n",  "This is WEAVE, C Version 4.4" ) ; 
  idloc = 10 ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 97 ; 
  buffer [ 8 ] = 110 ; 
  buffer [ 9 ] = 100 ; 
  curname = idlookup ( 28 ) ; 
  idfirst = 5 ; 
  buffer [ 5 ] = 97 ; 
  buffer [ 6 ] = 114 ; 
  buffer [ 7 ] = 114 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 9 ] = 121 ; 
  curname = idlookup ( 4 ) ; 
  idfirst = 5 ; 
  buffer [ 5 ] = 98 ; 
  buffer [ 6 ] = 101 ; 
  buffer [ 7 ] = 103 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 9 ] = 110 ; 
  curname = idlookup ( 5 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 99 ; 
  buffer [ 7 ] = 97 ; 
  buffer [ 8 ] = 115 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 6 ) ; 
  idfirst = 5 ; 
  buffer [ 5 ] = 99 ; 
  buffer [ 6 ] = 111 ; 
  buffer [ 7 ] = 110 ; 
  buffer [ 8 ] = 115 ; 
  buffer [ 9 ] = 116 ; 
  curname = idlookup ( 7 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 100 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 9 ] = 118 ; 
  curname = idlookup ( 8 ) ; 
  idfirst = 8 ; 
  buffer [ 8 ] = 100 ; 
  buffer [ 9 ] = 111 ; 
  curname = idlookup ( 9 ) ; 
  idfirst = 4 ; 
  buffer [ 4 ] = 100 ; 
  buffer [ 5 ] = 111 ; 
  buffer [ 6 ] = 119 ; 
  buffer [ 7 ] = 110 ; 
  buffer [ 8 ] = 116 ; 
  buffer [ 9 ] = 111 ; 
  curname = idlookup ( 20 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 101 ; 
  buffer [ 7 ] = 108 ; 
  buffer [ 8 ] = 115 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 10 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 101 ; 
  buffer [ 8 ] = 110 ; 
  buffer [ 9 ] = 100 ; 
  curname = idlookup ( 11 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 102 ; 
  buffer [ 7 ] = 105 ; 
  buffer [ 8 ] = 108 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 4 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 102 ; 
  buffer [ 8 ] = 111 ; 
  buffer [ 9 ] = 114 ; 
  curname = idlookup ( 12 ) ; 
  idfirst = 2 ; 
  buffer [ 2 ] = 102 ; 
  buffer [ 3 ] = 117 ; 
  buffer [ 4 ] = 110 ; 
  buffer [ 5 ] = 99 ; 
  buffer [ 6 ] = 116 ; 
  buffer [ 7 ] = 105 ; 
  buffer [ 8 ] = 111 ; 
  buffer [ 9 ] = 110 ; 
  curname = idlookup ( 17 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 103 ; 
  buffer [ 7 ] = 111 ; 
  buffer [ 8 ] = 116 ; 
  buffer [ 9 ] = 111 ; 
  curname = idlookup ( 13 ) ; 
  idfirst = 8 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 9 ] = 102 ; 
  curname = idlookup ( 14 ) ; 
  idfirst = 8 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 9 ] = 110 ; 
  curname = idlookup ( 30 ) ; 
  idfirst = 5 ; 
  buffer [ 5 ] = 108 ; 
  buffer [ 6 ] = 97 ; 
  buffer [ 7 ] = 98 ; 
  buffer [ 8 ] = 101 ; 
  buffer [ 9 ] = 108 ; 
  curname = idlookup ( 7 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 109 ; 
  buffer [ 8 ] = 111 ; 
  buffer [ 9 ] = 100 ; 
  curname = idlookup ( 8 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 110 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 9 ] = 108 ; 
  curname = idlookup ( 16 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 110 ; 
  buffer [ 8 ] = 111 ; 
  buffer [ 9 ] = 116 ; 
  curname = idlookup ( 29 ) ; 
  idfirst = 8 ; 
  buffer [ 8 ] = 111 ; 
  buffer [ 9 ] = 102 ; 
  curname = idlookup ( 9 ) ; 
  idfirst = 8 ; 
  buffer [ 8 ] = 111 ; 
  buffer [ 9 ] = 114 ; 
  curname = idlookup ( 55 ) ; 
  idfirst = 4 ; 
  buffer [ 4 ] = 112 ; 
  buffer [ 5 ] = 97 ; 
  buffer [ 6 ] = 99 ; 
  buffer [ 7 ] = 107 ; 
  buffer [ 8 ] = 101 ; 
  buffer [ 9 ] = 100 ; 
  curname = idlookup ( 13 ) ; 
  idfirst = 1 ; 
  buffer [ 1 ] = 112 ; 
  buffer [ 2 ] = 114 ; 
  buffer [ 3 ] = 111 ; 
  buffer [ 4 ] = 99 ; 
  buffer [ 5 ] = 101 ; 
  buffer [ 6 ] = 100 ; 
  buffer [ 7 ] = 117 ; 
  buffer [ 8 ] = 114 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 17 ) ; 
  idfirst = 3 ; 
  buffer [ 3 ] = 112 ; 
  buffer [ 4 ] = 114 ; 
  buffer [ 5 ] = 111 ; 
  buffer [ 6 ] = 103 ; 
  buffer [ 7 ] = 114 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 9 ] = 109 ; 
  curname = idlookup ( 17 ) ; 
  idfirst = 4 ; 
  buffer [ 4 ] = 114 ; 
  buffer [ 5 ] = 101 ; 
  buffer [ 6 ] = 99 ; 
  buffer [ 7 ] = 111 ; 
  buffer [ 8 ] = 114 ; 
  buffer [ 9 ] = 100 ; 
  curname = idlookup ( 18 ) ; 
  idfirst = 4 ; 
  buffer [ 4 ] = 114 ; 
  buffer [ 5 ] = 101 ; 
  buffer [ 6 ] = 112 ; 
  buffer [ 7 ] = 101 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 9 ] = 116 ; 
  curname = idlookup ( 19 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 115 ; 
  buffer [ 8 ] = 101 ; 
  buffer [ 9 ] = 116 ; 
  curname = idlookup ( 4 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 116 ; 
  buffer [ 7 ] = 104 ; 
  buffer [ 8 ] = 101 ; 
  buffer [ 9 ] = 110 ; 
  curname = idlookup ( 9 ) ; 
  idfirst = 8 ; 
  buffer [ 8 ] = 116 ; 
  buffer [ 9 ] = 111 ; 
  curname = idlookup ( 20 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 116 ; 
  buffer [ 7 ] = 121 ; 
  buffer [ 8 ] = 112 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 7 ) ; 
  idfirst = 5 ; 
  buffer [ 5 ] = 117 ; 
  buffer [ 6 ] = 110 ; 
  buffer [ 7 ] = 116 ; 
  buffer [ 8 ] = 105 ; 
  buffer [ 9 ] = 108 ; 
  curname = idlookup ( 21 ) ; 
  idfirst = 7 ; 
  buffer [ 7 ] = 118 ; 
  buffer [ 8 ] = 97 ; 
  buffer [ 9 ] = 114 ; 
  curname = idlookup ( 22 ) ; 
  idfirst = 5 ; 
  buffer [ 5 ] = 119 ; 
  buffer [ 6 ] = 104 ; 
  buffer [ 7 ] = 105 ; 
  buffer [ 8 ] = 108 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 12 ) ; 
  idfirst = 6 ; 
  buffer [ 6 ] = 119 ; 
  buffer [ 7 ] = 105 ; 
  buffer [ 8 ] = 116 ; 
  buffer [ 9 ] = 104 ; 
  curname = idlookup ( 12 ) ; 
  idfirst = 3 ; 
  buffer [ 3 ] = 120 ; 
  buffer [ 4 ] = 99 ; 
  buffer [ 5 ] = 108 ; 
  buffer [ 6 ] = 97 ; 
  buffer [ 7 ] = 117 ; 
  buffer [ 8 ] = 115 ; 
  buffer [ 9 ] = 101 ; 
  curname = idlookup ( 23 ) ; 
  PhaseI () ; 
  PhaseII () ; 
  if ( noxref ) 
  {
    finishline () ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
    } 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 118 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 102 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 105 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 108 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 108 ; 
    } 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 101 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 110 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 100 ; 
    } 
    finishline () ; 
  } 
  else {
      
    phasethree = true ; 
    {
      (void) putc('\n',  stdout );
      (void) Fputs( stdout ,  "Writing the index..." ) ; 
    } 
    if ( changeexists ) 
    {
      finishline () ; 
      {
	kmodule = 1 ; 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 92 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 99 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 104 ; 
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 32 ; 
	} 
	while ( kmodule < modulecount ) {
	    
	  if ( changedmodule [ kmodule ] ) 
	  {
	    outmod ( kmodule ) ; 
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 44 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 32 ; 
	    } 
	  } 
	  kmodule = kmodule + 1 ; 
	} 
	outmod ( kmodule ) ; 
	{
	  if ( outptr == linelength ) 
	  breakout () ; 
	  outptr = outptr + 1 ; 
	  outbuf [ outptr ] = 46 ; 
	} 
      } 
    } 
    finishline () ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 105 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 110 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 120 ; 
    } 
    finishline () ; 
    {register integer for_end; c = 0 ; for_end = 255 ; if ( c <= for_end) do 
      bucket [ c ] = 0 ; 
    while ( c++ < for_end ) ; } 
    {register integer for_end; h = 0 ; for_end = hashsize - 1 ; if ( h <= 
    for_end) do 
      {
	nextname = hash [ h ] ; 
	while ( nextname != 0 ) {
	    
	  curname = nextname ; 
	  nextname = link [ curname ] ; 
	  if ( xref [ curname ] != 0 ) 
	  {
	    c = bytemem [ curname % 2 ][ bytestart [ curname ] ] ; 
	    if ( ( c <= 90 ) && ( c >= 65 ) ) 
	    c = c + 32 ; 
	    blink [ curname ] = bucket [ c ] ; 
	    bucket [ c ] = curname ; 
	  } 
	} 
      } 
    while ( h++ < for_end ) ; } 
    scrapptr = 0 ; 
    unbucket ( 1 ) ; 
    while ( scrapptr > 0 ) {
	
      curdepth = cat [ scrapptr ] ; 
      if ( ( blink [ trans [ scrapptr ] ] == 0 ) || ( curdepth == 255 ) ) 
      {
	curname = trans [ scrapptr ] ; 
	do {
	    { 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 92 ; 
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 58 ; 
	  } 
	  switch ( ilk [ curname ] ) 
	  {case 0 : 
	    if ( bytestart [ curname + 2 ] - bytestart [ curname ] == 1 ) 
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 92 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 124 ; 
	    } 
	    else {
		
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 92 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 92 ; 
	    } 
	    break ; 
	  case 1 : 
	    ; 
	    break ; 
	  case 2 : 
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 92 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 57 ; 
	    } 
	    break ; 
	  case 3 : 
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 92 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 46 ; 
	    } 
	    break ; 
	    default: 
	    {
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 92 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 38 ; 
	    } 
	    break ; 
	  } 
	  outname ( curname ) ; 
	  thisxref = xref [ curname ] ; 
	  curxref = 0 ; 
	  do {
	      nextxref = xmem [ thisxref ] .xlinkfield ; 
	    xmem [ thisxref ] .xlinkfield = curxref ; 
	    curxref = thisxref ; 
	    thisxref = nextxref ; 
	  } while ( ! ( thisxref == 0 ) ) ; 
	  do {
	      { 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 44 ; 
	      if ( outptr == linelength ) 
	      breakout () ; 
	      outptr = outptr + 1 ; 
	      outbuf [ outptr ] = 32 ; 
	    } 
	    curval = xmem [ curxref ] .numfield ; 
	    if ( curval < 10240 ) 
	    outmod ( curval ) ; 
	    else {
		
	      {
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = 92 ; 
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = 91 ; 
	      } 
	      outmod ( curval - 10240 ) ; 
	      {
		if ( outptr == linelength ) 
		breakout () ; 
		outptr = outptr + 1 ; 
		outbuf [ outptr ] = 93 ; 
	      } 
	    } 
	    curxref = xmem [ curxref ] .xlinkfield ; 
	  } while ( ! ( curxref == 0 ) ) ; 
	  {
	    if ( outptr == linelength ) 
	    breakout () ; 
	    outptr = outptr + 1 ; 
	    outbuf [ outptr ] = 46 ; 
	  } 
	  finishline () ; 
	  curname = blink [ curname ] ; 
	} while ( ! ( curname == 0 ) ) ; 
	scrapptr = scrapptr - 1 ; 
      } 
      else {
	  
	nextname = trans [ scrapptr ] ; 
	do {
	    curname = nextname ; 
	  nextname = blink [ curname ] ; 
	  curbyte = bytestart [ curname ] + curdepth ; 
	  curbank = curname % 2 ; 
	  if ( curbyte == bytestart [ curname + 2 ] ) 
	  c = 0 ; 
	  else {
	      
	    c = bytemem [ curbank ][ curbyte ] ; 
	    if ( ( c <= 90 ) && ( c >= 65 ) ) 
	    c = c + 32 ; 
	  } 
	  blink [ curname ] = bucket [ c ] ; 
	  bucket [ c ] = curname ; 
	} while ( ! ( nextname == 0 ) ) ; 
	scrapptr = scrapptr - 1 ; 
	unbucket ( curdepth + 1 ) ; 
      } 
    } 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 102 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 105 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 110 ; 
    } 
    finishline () ; 
    modprint ( ilk [ 0 ] ) ; 
    {
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 92 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 99 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 111 ; 
      if ( outptr == linelength ) 
      breakout () ; 
      outptr = outptr + 1 ; 
      outbuf [ outptr ] = 110 ; 
    } 
    finishline () ; 
  } 
  (void) Fputs( stdout ,  "Done." ) ; 
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
      if ( ! phaseone ) 
      {
	(void) putc('\n',  stdout );
	(void) Fputs( stdout ,  "! Change file entry did not match" ) ; 
	error () ; 
      } 
    } 
  } 
  jumpout () ; 
} 
