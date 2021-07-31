#include "config.h"
#define linelength 79 
#define maxrow 16000 
typedef schar ASCIIcode  ; 
typedef file_ptr /* of  char */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
typedef char UNIXfilename [PATHMAX + 1] ; 
ASCIIcode xord[256]  ; 
char xchr[256]  ; 
bytefile gffile  ; 
bytefile pkfile  ; 
boolean verbose  ; 
integer pkarg  ; 
UNIXfilename gfname  ; 
integer pkloc  ; 
integer gfloc  ; 
boolean pkopen  ; 
integer bitweight  ; 
integer outputbyte  ; 
integer gflen  ; 
integer tfmwidth[256]  ; 
integer dx[256], dy[256]  ; 
schar status[256]  ; 
integer row[maxrow + 1]  ; 
integer gfch  ; 
integer gfchmod256  ; 
integer predpkloc  ; 
integer maxn, minn  ; 
integer maxm, minm  ; 
integer rowptr  ; 
integer power[9]  ; 
char comment[1]  ; 
integer checksum  ; 
integer designsize  ; 
integer hmag  ; 
integer i  ; 

#include "gftopk.h"
void initialize ( ) 
{integer i  ; 
  setpaths ( GFFILEPATHBIT ) ; 
  {register integer for_end; i = 0 ; for_end = 31 ; if ( i <= for_end) do 
    xchr [ i ] = '?' ; 
  while ( i++ < for_end ) ; } 
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
  {register integer for_end; i = 127 ; for_end = 255 ; if ( i <= for_end) do 
    xchr [ i ] = '?' ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 127 ; if ( i <= for_end) do 
    xord [ chr ( i ) ] = 32 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 32 ; for_end = 126 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  pkopen = false ; 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    status [ i ] = 0 ; 
  while ( i++ < for_end ) ; } 
  power [ 0 ] = 1 ; 
  {register integer for_end; i = 1 ; for_end = 8 ; if ( i <= for_end) do 
    power [ i ] = power [ i - 1 ] + power [ i - 1 ] ; 
  while ( i++ < for_end ) ; } 
} 
void opengffile ( ) 
{integer j  ; 
  verbose = false ; 
  pkarg = 3 ; 
  if ( ( argc < 2 ) || ( argc > 4 ) ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s\n",  "Usage: gftopk [-v] <gf file> [pk file]." ) ; 
    uexit ( 1 ) ; 
  } 
  argv ( 1 , gfname ) ; 
  if ( gfname [ 1 ] == xchr [ 45 ] ) 
  {
    if ( gfname [ 2 ] == xchr [ 118 ] ) 
    {
      verbose = true ; 
      argv ( 2 , gfname ) ; 
      pkarg = pkarg + 1 ; 
    } 
    else {
	
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s\n",  "Usage: gftopk [-v] <gf file> [pk file]." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  if ( verbose ) 
  (void) fprintf( stdout , "%s\n",  "This is GFtoPK, C Version 2.3" ) ; 
  if ( testreadaccess ( gfname , GFFILEPATH ) ) 
  {
    reset ( gffile , gfname ) ; 
  } 
  else {
      
    printpascalstring ( gfname ) ; 
    {
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s\n",  ": GF file not found." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  gfloc = 0 ; 
} 
void openpkfile ( ) 
{integer dotpos, slashpos, last, gfindex, pkindex  ; 
  UNIXfilename pkname  ; 
  if ( argc == pkarg ) 
  argv ( argc - 1 , pkname ) ; 
  else {
      
    dotpos = -1 ; 
    slashpos = -1 ; 
    last = 1 ; 
    while ( ( gfname [ last ] != ' ' ) && ( last <= PATHMAX - 5 ) ) {
	
      if ( gfname [ last ] == '.' ) 
      dotpos = last ; 
      if ( gfname [ last ] == '/' ) 
      slashpos = last ; 
      last = last + 1 ; 
    } 
    if ( slashpos == -1 ) 
    slashpos = 0 ; 
    if ( dotpos < slashpos ) 
    dotpos = last - 1 ; 
    pkindex = 1 ; 
    {register integer for_end; gfindex = slashpos + 1 ; for_end = dotpos 
    ; if ( gfindex <= for_end) do 
      {
	pkname [ pkindex ] = gfname [ gfindex ] ; 
	pkindex = pkindex + 1 ; 
      } 
    while ( gfindex++ < for_end ) ; } 
    gfindex = dotpos + 1 ; 
    while ( ( gfindex < last ) && ( gfname [ gfindex ] != 'g' ) ) {
	
      pkname [ pkindex ] = gfname [ gfindex ] ; 
      gfindex = gfindex + 1 ; 
      pkindex = pkindex + 1 ; 
    } 
    pkname [ pkindex ] = 'p' ; 
    pkname [ pkindex + 1 ] = 'k' ; 
    pkname [ pkindex + 2 ] = ' ' ; 
  } 
  if ( verbose ) 
  (void) putc( xchr [ xord [ '[' ] ] ,  stdout );
  gfindex = 1 ; 
  while ( gfname [ gfindex ] != ' ' ) {
      
    if ( verbose ) 
    (void) putc( xchr [ xord [ gfname [ gfindex ] ] ] ,  stdout );
    gfindex = gfindex + 1 ; 
  } 
  if ( verbose ) 
  (void) putc( xchr [ xord [ '-' ] ] ,  stdout );
  if ( verbose ) 
  (void) putc( xchr [ xord [ '>' ] ] ,  stdout );
  pkindex = 1 ; 
  while ( pkname [ pkindex ] != ' ' ) {
      
    if ( verbose ) 
    (void) putc( xchr [ xord [ pkname [ pkindex ] ] ] ,  stdout );
    pkindex = pkindex + 1 ; 
  } 
  if ( verbose ) 
  (void) putc( xchr [ xord [ ']' ] ] ,  stdout );
  if ( verbose ) 
  (void) fprintf( stdout , "%c\n",  xchr [ xord [ ' ' ] ] ) ; 
  rewrite ( pkfile , pkname ) ; 
  pkloc = 0 ; 
  pkopen = true ; 
} 
integer gfbyte ( ) 
{register integer Result; eightbits b  ; 
  if ( eof ( gffile ) ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "Unexpected end of file!" , '!' ) ; 
    uexit ( 1 ) ; 
  } 
  else {
      
    read ( gffile , b ) ; 
    Result = b ; 
  } 
  gfloc = gfloc + 1 ; 
  return(Result) ; 
} 
integer gfsignedquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  read ( gffile , c ) ; 
  read ( gffile , d ) ; 
  if ( a < 128 ) 
  Result = ( ( a * 256 + b ) * 256 + c ) * 256 + d ; 
  else Result = ( ( ( a - 256 ) * 256 + b ) * 256 + c ) * 256 + d ; 
  gfloc = gfloc + 4 ; 
  return(Result) ; 
} 
void zpkhalfword ( a ) 
integer a ; 
{if ( a < 0 ) 
  a = a + 65536L ; 
  putbyte ( a / 256 , pkfile ) ; 
  putbyte ( a % 256 , pkfile ) ; 
  pkloc = pkloc + 2 ; 
} 
void zpkthreebytes ( a ) 
integer a ; 
{putbyte ( a / 65536L % 256 , pkfile ) ; 
  putbyte ( a / 256 % 256 , pkfile ) ; 
  putbyte ( a % 256 , pkfile ) ; 
  pkloc = pkloc + 3 ; 
} 
void zpkword ( a ) 
integer a ; 
{integer b  ; 
  if ( a < 0 ) 
  {
    a = a + 1073741824L ; 
    a = a + 1073741824L ; 
    b = 128 + a / 16777216L ; 
  } 
  else b = a / 16777216L ; 
  putbyte ( b , pkfile ) ; 
  putbyte ( a / 65536L % 256 , pkfile ) ; 
  putbyte ( a / 256 % 256 , pkfile ) ; 
  putbyte ( a % 256 , pkfile ) ; 
  pkloc = pkloc + 4 ; 
} 
void zpknyb ( a ) 
integer a ; 
{if ( bitweight == 16 ) 
  {
    outputbyte = a * 16 ; 
    bitweight = 1 ; 
  } 
  else {
      
    {
      putbyte ( outputbyte + a , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    bitweight = 16 ; 
  } 
} 
integer gflength ( ) 
{register integer Result; checkedfseek ( gffile , 0 , 2 ) ; 
  Result = ftell ( gffile ) ; 
  return(Result) ; 
} 
void zmovetobyte ( n ) 
integer n ; 
{checkedfseek ( gffile , n , 0 ) ; 
} 
void packandsendcharacter ( ) 
{integer i, j, k  ; 
  integer extra  ; 
  integer putptr  ; 
  integer repeatflag  ; 
  integer hbit  ; 
  integer buff  ; 
  integer dynf  ; 
  integer height, width  ; 
  integer xoffset, yoffset  ; 
  integer deriv[14]  ; 
  integer bcompsize  ; 
  boolean firston  ; 
  integer flagbyte  ; 
  boolean state  ; 
  boolean on  ; 
  integer compsize  ; 
  integer count  ; 
  integer pbit  ; 
  boolean ron, son  ; 
  integer rcount, scount  ; 
  integer ri, si  ; 
  integer max2  ; 
  i = 2 ; 
  rowptr = rowptr - 1 ; 
  while ( row [ i ] == ( -99999L ) ) i = i + 1 ; 
  if ( row [ i ] != ( -99998L ) ) 
  {
    maxn = maxn - i + 2 ; 
    while ( row [ rowptr - 2 ] == ( -99999L ) ) {
	
      rowptr = rowptr - 1 ; 
      row [ rowptr ] = ( -99998L ) ; 
    } 
    minn = maxn + 1 ; 
    extra = maxm - minm + 1 ; 
    maxm = 0 ; 
    j = i ; 
    while ( row [ j ] != ( -99998L ) ) {
	
      minn = minn - 1 ; 
      if ( row [ j ] != ( -99999L ) ) 
      {
	k = row [ j ] ; 
	if ( k < extra ) 
	extra = k ; 
	j = j + 1 ; 
	while ( row [ j ] != ( -99999L ) ) {
	    
	  k = k + row [ j ] ; 
	  j = j + 1 ; 
	} 
	if ( maxm < k ) 
	maxm = k ; 
      } 
      j = j + 1 ; 
    } 
    minm = minm + extra ; 
    maxm = minm + maxm - 1 - extra ; 
    height = maxn - minn + 1 ; 
    width = maxm - minm + 1 ; 
    xoffset = - (integer) minm ; 
    yoffset = maxn ; 
  } 
  else {
      
    height = 0 ; 
    width = 0 ; 
    xoffset = 0 ; 
    yoffset = 0 ; 
  } 
  putptr = 0 ; 
  rowptr = 2 ; 
  repeatflag = 0 ; 
  state = true ; 
  buff = 0 ; 
  while ( row [ rowptr ] == ( -99999L ) ) rowptr = rowptr + 1 ; 
  while ( row [ rowptr ] != ( -99998L ) ) {
      
    i = rowptr ; 
    if ( ( row [ i ] != ( -99999L ) ) && ( ( row [ i ] != extra ) || ( row [ i 
    + 1 ] != width ) ) ) 
    {
      j = i + 1 ; 
      while ( row [ j - 1 ] != ( -99999L ) ) j = j + 1 ; 
      while ( row [ i ] == row [ j ] ) {
	  
	if ( row [ i ] == ( -99999L ) ) 
	{
	  repeatflag = repeatflag + 1 ; 
	  rowptr = i + 1 ; 
	} 
	i = i + 1 ; 
	j = j + 1 ; 
      } 
    } 
    if ( row [ rowptr ] != ( -99999L ) ) 
    row [ rowptr ] = row [ rowptr ] - extra ; 
    hbit = 0 ; 
    while ( row [ rowptr ] != ( -99999L ) ) {
	
      hbit = hbit + row [ rowptr ] ; 
      if ( state ) 
      {
	buff = buff + row [ rowptr ] ; 
	state = false ; 
      } 
      else if ( row [ rowptr ] > 0 ) 
      {
	{
	  row [ putptr ] = buff ; 
	  putptr = putptr + 1 ; 
	  if ( repeatflag > 0 ) 
	  {
	    row [ putptr ] = - (integer) repeatflag ; 
	    repeatflag = 0 ; 
	    putptr = putptr + 1 ; 
	  } 
	} 
	buff = row [ rowptr ] ; 
      } 
      else state = true ; 
      rowptr = rowptr + 1 ; 
    } 
    if ( hbit < width ) 
    if ( state ) 
    buff = buff + width - hbit ; 
    else {
	
      {
	row [ putptr ] = buff ; 
	putptr = putptr + 1 ; 
	if ( repeatflag > 0 ) 
	{
	  row [ putptr ] = - (integer) repeatflag ; 
	  repeatflag = 0 ; 
	  putptr = putptr + 1 ; 
	} 
      } 
      buff = width - hbit ; 
      state = true ; 
    } 
    else state = false ; 
    rowptr = rowptr + 1 ; 
  } 
  if ( buff > 0 ) 
  {
    row [ putptr ] = buff ; 
    putptr = putptr + 1 ; 
    if ( repeatflag > 0 ) 
    {
      row [ putptr ] = - (integer) repeatflag ; 
      repeatflag = 0 ; 
      putptr = putptr + 1 ; 
    } 
  } 
  {
    row [ putptr ] = ( -99998L ) ; 
    putptr = putptr + 1 ; 
    if ( repeatflag > 0 ) 
    {
      row [ putptr ] = - (integer) repeatflag ; 
      repeatflag = 0 ; 
      putptr = putptr + 1 ; 
    } 
  } 
  {register integer for_end; i = 1 ; for_end = 13 ; if ( i <= for_end) do 
    deriv [ i ] = 0 ; 
  while ( i++ < for_end ) ; } 
  i = 0 ; 
  firston = row [ i ] == 0 ; 
  if ( firston ) 
  i = i + 1 ; 
  compsize = 0 ; 
  while ( row [ i ] != ( -99998L ) ) {
      
    j = row [ i ] ; 
    if ( j == -1 ) 
    compsize = compsize + 1 ; 
    else {
	
      if ( j < 0 ) 
      {
	compsize = compsize + 1 ; 
	j = - (integer) j ; 
      } 
      if ( j < 209 ) 
      compsize = compsize + 2 ; 
      else {
	  
	k = j - 193 ; 
	while ( k >= 16 ) {
	    
	  k = k / 16 ; 
	  compsize = compsize + 2 ; 
	} 
	compsize = compsize + 1 ; 
      } 
      if ( j < 14 ) 
      deriv [ j ] = deriv [ j ] - 1 ; 
      else if ( j < 209 ) 
      deriv [ ( 223 - j ) / 15 ] = deriv [ ( 223 - j ) / 15 ] + 1 ; 
      else {
	  
	k = 16 ; 
	while ( ( k * 16 < j + 3 ) ) k = k * 16 ; 
	if ( j - k <= 192 ) 
	deriv [ ( 207 - j + k ) / 15 ] = deriv [ ( 207 - j + k ) / 15 ] + 2 ; 
      } 
    } 
    i = i + 1 ; 
  } 
  bcompsize = compsize ; 
  dynf = 0 ; 
  {register integer for_end; i = 1 ; for_end = 13 ; if ( i <= for_end) do 
    {
      compsize = compsize + deriv [ i ] ; 
      if ( compsize <= bcompsize ) 
      {
	bcompsize = compsize ; 
	dynf = i ; 
      } 
    } 
  while ( i++ < for_end ) ; } 
  compsize = ( bcompsize + 1 ) / 2 ; 
  if ( ( compsize > ( height * width + 7 ) / 8 ) || ( height * width == 0 ) ) 
  {
    compsize = ( height * width + 7 ) / 8 ; 
    dynf = 14 ; 
  } 
  flagbyte = dynf * 16 ; 
  if ( firston ) 
  flagbyte = flagbyte + 8 ; 
  if ( ( gfch != gfchmod256 ) || ( tfmwidth [ gfchmod256 ] > 16777215L ) || ( 
  tfmwidth [ gfchmod256 ] < 0 ) || ( dy [ gfchmod256 ] != 0 ) || ( dx [ 
  gfchmod256 ] < 0 ) || ( dx [ gfchmod256 ] % 65536L != 0 ) || ( compsize > 
  196594L ) || ( width > 65535L ) || ( height > 65535L ) || ( xoffset > 32767 
  ) || ( yoffset > 32767 ) || ( xoffset < -32768L ) || ( yoffset < -32768L ) ) 
  {
    flagbyte = flagbyte + 7 ; 
    {
      putbyte ( flagbyte , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    compsize = compsize + 28 ; 
    pkword ( compsize ) ; 
    pkword ( gfch ) ; 
    predpkloc = pkloc + compsize ; 
    pkword ( tfmwidth [ gfchmod256 ] ) ; 
    pkword ( dx [ gfchmod256 ] ) ; 
    pkword ( dy [ gfchmod256 ] ) ; 
    pkword ( width ) ; 
    pkword ( height ) ; 
    pkword ( xoffset ) ; 
    pkword ( yoffset ) ; 
  } 
  else if ( ( dx [ gfch ] > 16777215L ) || ( width > 255 ) || ( height > 255 ) 
  || ( xoffset > 127 ) || ( yoffset > 127 ) || ( xoffset < -128 ) || ( yoffset 
  < -128 ) || ( compsize > 1015 ) ) 
  {
    compsize = compsize + 13 ; 
    flagbyte = flagbyte + compsize / 65536L + 4 ; 
    {
      putbyte ( flagbyte , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    pkhalfword ( compsize % 65536L ) ; 
    {
      putbyte ( gfch , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    predpkloc = pkloc + compsize ; 
    pkthreebytes ( tfmwidth [ gfchmod256 ] ) ; 
    pkhalfword ( dx [ gfchmod256 ] / 65536L ) ; 
    pkhalfword ( width ) ; 
    pkhalfword ( height ) ; 
    pkhalfword ( xoffset ) ; 
    pkhalfword ( yoffset ) ; 
  } 
  else {
      
    compsize = compsize + 8 ; 
    flagbyte = flagbyte + compsize / 256 ; 
    {
      putbyte ( flagbyte , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    {
      putbyte ( compsize % 256 , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    {
      putbyte ( gfch , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    predpkloc = pkloc + compsize ; 
    pkthreebytes ( tfmwidth [ gfchmod256 ] ) ; 
    {
      putbyte ( dx [ gfchmod256 ] / 65536L , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    {
      putbyte ( width , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    {
      putbyte ( height , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    {
      putbyte ( xoffset , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
    {
      putbyte ( yoffset , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
  } 
  if ( dynf != 14 ) 
  {
    bitweight = 16 ; 
    max2 = 208 - 15 * dynf ; 
    i = 0 ; 
    if ( row [ i ] == 0 ) 
    i = i + 1 ; 
    while ( row [ i ] != ( -99998L ) ) {
	
      j = row [ i ] ; 
      if ( j == -1 ) 
      pknyb ( 15 ) ; 
      else {
	  
	if ( j < 0 ) 
	{
	  pknyb ( 14 ) ; 
	  j = - (integer) j ; 
	} 
	if ( j <= dynf ) 
	pknyb ( j ) ; 
	else if ( j <= max2 ) 
	{
	  j = j - dynf - 1 ; 
	  pknyb ( j / 16 + dynf + 1 ) ; 
	  pknyb ( j % 16 ) ; 
	} 
	else {
	    
	  j = j - max2 + 15 ; 
	  k = 16 ; 
	  while ( k <= j ) {
	      
	    k = k * 16 ; 
	    pknyb ( 0 ) ; 
	  } 
	  while ( k > 1 ) {
	      
	    k = k / 16 ; 
	    pknyb ( j / k ) ; 
	    j = j % k ; 
	  } 
	} 
      } 
      i = i + 1 ; 
    } 
    if ( bitweight != 16 ) 
    {
      putbyte ( outputbyte , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
  } 
  else if ( height > 0 ) 
  {
    buff = 0 ; 
    pbit = 8 ; 
    i = 1 ; 
    hbit = width ; 
    on = false ; 
    state = false ; 
    count = row [ 0 ] ; 
    repeatflag = 0 ; 
    while ( ( row [ i ] != ( -99998L ) ) || state || ( count > 0 ) ) {
	
      if ( state ) 
      {
	count = rcount ; 
	i = ri ; 
	on = ron ; 
	repeatflag = repeatflag - 1 ; 
      } 
      else {
	  
	rcount = count ; 
	ri = i ; 
	ron = on ; 
      } 
      do {
	  if ( count == 0 ) 
	{
	  if ( row [ i ] < 0 ) 
	  {
	    if ( ! state ) 
	    repeatflag = - (integer) row [ i ] ; 
	    i = i + 1 ; 
	  } 
	  count = row [ i ] ; 
	  i = i + 1 ; 
	  on = ! on ; 
	} 
	if ( ( count >= pbit ) && ( pbit < hbit ) ) 
	{
	  if ( on ) 
	  buff = buff + power [ pbit ] - 1 ; 
	  {
	    putbyte ( buff , pkfile ) ; 
	    pkloc = pkloc + 1 ; 
	  } 
	  buff = 0 ; 
	  hbit = hbit - pbit ; 
	  count = count - pbit ; 
	  pbit = 8 ; 
	} 
	else if ( ( count < pbit ) && ( count < hbit ) ) 
	{
	  if ( on ) 
	  buff = buff + power [ pbit ] - power [ pbit - count ] ; 
	  pbit = pbit - count ; 
	  hbit = hbit - count ; 
	  count = 0 ; 
	} 
	else {
	    
	  if ( on ) 
	  buff = buff + power [ pbit ] - power [ pbit - hbit ] ; 
	  count = count - hbit ; 
	  pbit = pbit - hbit ; 
	  hbit = width ; 
	  if ( pbit == 0 ) 
	  {
	    {
	      putbyte ( buff , pkfile ) ; 
	      pkloc = pkloc + 1 ; 
	    } 
	    buff = 0 ; 
	    pbit = 8 ; 
	  } 
	} 
      } while ( ! ( hbit == width ) ) ; 
      if ( state && ( repeatflag == 0 ) ) 
      {
	count = scount ; 
	i = si ; 
	on = son ; 
	state = false ; 
      } 
      else if ( ! state && ( repeatflag > 0 ) ) 
      {
	scount = count ; 
	si = i ; 
	son = on ; 
	state = true ; 
      } 
    } 
    if ( pbit != 8 ) 
    {
      putbyte ( buff , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
  } 
} 
void convertgffile ( ) 
{integer i, j, k  ; 
  integer gfcom  ; 
  boolean dotherows  ; 
  boolean on  ; 
  boolean state  ; 
  integer extra  ; 
  boolean bad  ; 
  integer hppp, vppp  ; 
  integer q  ; 
  integer postloc  ; 
  opengffile () ; 
  if ( gfbyte () != 247 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "First byte is not preamble" , '!' ) 
    ; 
    uexit ( 1 ) ; 
  } 
  if ( gfbyte () != 131 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "Identification byte is incorrect" ,     '!' ) ; 
    uexit ( 1 ) ; 
  } 
  gflen = gflength () ; 
  postloc = gflen - 4 ; 
  do {
      if ( postloc == 0 ) 
    {
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "all 223's" , '!' ) ; 
      uexit ( 1 ) ; 
    } 
    movetobyte ( postloc ) ; 
    k = gfbyte () ; 
    postloc = postloc - 1 ; 
  } while ( ! ( k != 223 ) ) ; 
  if ( k != 131 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%s%ld%c\n",  "Bad GF file: " , "ID byte is " , (long)k , '!' ) ; 
    uexit ( 1 ) ; 
  } 
  movetobyte ( postloc - 3 ) ; 
  q = gfsignedquad () ; 
  if ( ( q < 0 ) || ( q > postloc - 3 ) ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%s%ld%c\n",  "Bad GF file: " , "post pointer is " , (long)q , '!' ) ; 
    uexit ( 1 ) ; 
  } 
  movetobyte ( q ) ; 
  k = gfbyte () ; 
  if ( k != 248 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%s%ld%s%c\n",  "Bad GF file: " , "byte at " , (long)q , " is not post" , '!'     ) ; 
    uexit ( 1 ) ; 
  } 
  i = gfsignedquad () ; 
  designsize = gfsignedquad () ; 
  checksum = gfsignedquad () ; 
  hppp = gfsignedquad () ; 
  hmag = round ( hppp * 72.27 / ((double) 65536L ) ) ; 
  vppp = gfsignedquad () ; 
  if ( hppp != vppp ) 
  if ( verbose ) 
  (void) fprintf( stdout , "%s\n",  "Odd aspect ratio!" ) ; 
  i = gfsignedquad () ; 
  i = gfsignedquad () ; 
  i = gfsignedquad () ; 
  i = gfsignedquad () ; 
  do {
      gfcom = gfbyte () ; 
    switch ( gfcom ) 
    {case 245 : 
    case 246 : 
      {
	gfch = gfbyte () ; 
	if ( status [ gfch ] != 0 ) 
	{
	  verbose = true ; 
	  if ( verbose ) 
	  (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " ,           "Locator for this character already found." , '!' ) ; 
	  uexit ( 1 ) ; 
	} 
	if ( gfcom == 245 ) 
	{
	  dx [ gfch ] = gfsignedquad () ; 
	  dy [ gfch ] = gfsignedquad () ; 
	} 
	else {
	    
	  dx [ gfch ] = gfbyte () * 65536L ; 
	  dy [ gfch ] = 0 ; 
	} 
	tfmwidth [ gfch ] = gfsignedquad () ; 
	i = gfsignedquad () ; 
	status [ gfch ] = 1 ; 
      } 
      break ; 
    case 239 : 
    case 240 : 
    case 241 : 
    case 242 : 
      {
	{
	  putbyte ( gfcom + 1 , pkfile ) ; 
	  pkloc = pkloc + 1 ; 
	} 
	i = 0 ; 
	{register integer for_end; j = 0 ; for_end = gfcom - 239 ; if ( j <= 
	for_end) do 
	  {
	    k = gfbyte () ; 
	    {
	      putbyte ( k , pkfile ) ; 
	      pkloc = pkloc + 1 ; 
	    } 
	    i = i * 256 + k ; 
	  } 
	while ( j++ < for_end ) ; } 
	{register integer for_end; j = 1 ; for_end = i ; if ( j <= for_end) 
	do 
	  {
	    putbyte ( gfbyte () , pkfile ) ; 
	    pkloc = pkloc + 1 ; 
	  } 
	while ( j++ < for_end ) ; } 
      } 
      break ; 
    case 243 : 
      {
	{
	  putbyte ( 244 , pkfile ) ; 
	  pkloc = pkloc + 1 ; 
	} 
	pkword ( gfsignedquad () ) ; 
      } 
      break ; 
    case 244 : 
      ; 
      break ; 
    case 249 : 
      ; 
      break ; 
      default: 
      {
	verbose = true ; 
	if ( verbose ) 
	(void) fprintf( stdout , "%s%s%ld%s%c\n",  "Bad GF file: " , "Unexpected " , (long)gfcom ,         " in postamble" , '!' ) ; 
	uexit ( 1 ) ; 
      } 
      break ; 
    } 
  } while ( ! ( gfcom == 249 ) ) ; 
  movetobyte ( 2 ) ; 
  openpkfile () ; 
  {
    putbyte ( 247 , pkfile ) ; 
    pkloc = pkloc + 1 ; 
  } 
  {
    putbyte ( 89 , pkfile ) ; 
    pkloc = pkloc + 1 ; 
  } 
  i = gfbyte () ; 
  do {
      if ( i == 0 ) 
    j = 46 ; 
    else j = gfbyte () ; 
    i = i - 1 ; 
  } while ( ! ( j != 32 ) ) ; 
  i = i + 1 ; 
  if ( i == 0 ) 
  k = -0 ; 
  else k = i + 0 ; 
  if ( k > 255 ) 
  {
    putbyte ( 255 , pkfile ) ; 
    pkloc = pkloc + 1 ; 
  } 
  else {
      
    putbyte ( k , pkfile ) ; 
    pkloc = pkloc + 1 ; 
  } 
  {register integer for_end; k = 1 ; for_end = 0 ; if ( k <= for_end) do 
    if ( ( i > 0 ) || ( k <= -0 ) ) 
    {
      putbyte ( xord [ comment [ k ] ] , pkfile ) ; 
      pkloc = pkloc + 1 ; 
    } 
  while ( k++ < for_end ) ; } 
  if ( verbose ) 
  (void) putc( '\'' ,  stdout );
  {register integer for_end; k = 1 ; for_end = i ; if ( k <= for_end) do 
    {
      if ( k > 1 ) 
      j = gfbyte () ; 
      if ( verbose ) 
      (void) putc( xchr [ j ] ,  stdout );
      if ( k < 256 ) 
      {
	putbyte ( j , pkfile ) ; 
	pkloc = pkloc + 1 ; 
      } 
    } 
  while ( k++ < for_end ) ; } 
  if ( verbose ) 
  (void) fprintf( stdout , "%c\n",  '\'' ) ; 
  pkword ( designsize ) ; 
  pkword ( checksum ) ; 
  pkword ( hppp ) ; 
  pkword ( vppp ) ; 
  do {
      gfcom = gfbyte () ; 
    dotherows = false ; 
    switch ( gfcom ) 
    {case 67 : 
    case 68 : 
      {
	if ( gfcom == 67 ) 
	{
	  gfch = gfsignedquad () ; 
	  i = gfsignedquad () ; 
	  minm = gfsignedquad () ; 
	  maxm = gfsignedquad () ; 
	  minn = gfsignedquad () ; 
	  maxn = gfsignedquad () ; 
	} 
	else {
	    
	  gfch = gfbyte () ; 
	  i = gfbyte () ; 
	  maxm = gfbyte () ; 
	  minm = maxm - i ; 
	  i = gfbyte () ; 
	  maxn = gfbyte () ; 
	  minn = maxn - i ; 
	} 
	if ( gfch >= 0 ) 
	gfchmod256 = gfch % 256 ; 
	else gfchmod256 = 255 - ( ( - (integer) ( 1 + gfch ) ) % 256 ) ; 
	if ( status [ gfchmod256 ] == 0 ) 
	{
	  verbose = true ; 
	  if ( verbose ) 
	  (void) fprintf( stdout , "%s%s%ld%c\n",  "Bad GF file: " ,           "no character locator for character " , (long)gfch , '!' ) ; 
	  uexit ( 1 ) ; 
	} 
	{
	  bad = false ; 
	  rowptr = 2 ; 
	  on = false ; 
	  extra = 0 ; 
	  state = true ; 
	  do {
	      gfcom = gfbyte () ; 
	    switch ( gfcom ) 
	    {case 0 : 
	      {
		state = ! state ; 
		on = ! on ; 
	      } 
	      break ; 
	    case 1 : 
	    case 2 : 
	    case 3 : 
	    case 4 : 
	    case 5 : 
	    case 6 : 
	    case 7 : 
	    case 8 : 
	    case 9 : 
	    case 10 : 
	    case 11 : 
	    case 12 : 
	    case 13 : 
	    case 14 : 
	    case 15 : 
	    case 16 : 
	    case 17 : 
	    case 18 : 
	    case 19 : 
	    case 20 : 
	    case 21 : 
	    case 22 : 
	    case 23 : 
	    case 24 : 
	    case 25 : 
	    case 26 : 
	    case 27 : 
	    case 28 : 
	    case 29 : 
	    case 30 : 
	    case 31 : 
	    case 32 : 
	    case 33 : 
	    case 34 : 
	    case 35 : 
	    case 36 : 
	    case 37 : 
	    case 38 : 
	    case 39 : 
	    case 40 : 
	    case 41 : 
	    case 42 : 
	    case 43 : 
	    case 44 : 
	    case 45 : 
	    case 46 : 
	    case 47 : 
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
	    case 58 : 
	    case 59 : 
	    case 60 : 
	    case 61 : 
	    case 62 : 
	    case 63 : 
	    case 64 : 
	    case 65 : 
	    case 66 : 
	      {
		if ( gfcom < 64 ) 
		i = gfcom - 0 ; 
		else {
		    
		  i = 0 ; 
		  {register integer for_end; j = 0 ; for_end = gfcom - 64 
		  ; if ( j <= for_end) do 
		    i = i * 256 + gfbyte () ; 
		  while ( j++ < for_end ) ; } 
		} 
		if ( state ) 
		{
		  extra = extra + i ; 
		  state = false ; 
		} 
		else {
		    
		  {
		    if ( rowptr > maxrow ) 
		    bad = true ; 
		    else {
			
		      row [ rowptr ] = extra ; 
		      rowptr = rowptr + 1 ; 
		    } 
		  } 
		  extra = i ; 
		} 
		on = ! on ; 
	      } 
	      break ; 
	    case 70 : 
	    case 71 : 
	    case 72 : 
	    case 73 : 
	      {
		i = 0 ; 
		{register integer for_end; j = 1 ; for_end = gfcom - 70 
		; if ( j <= for_end) do 
		  i = i * 256 + gfbyte () ; 
		while ( j++ < for_end ) ; } 
		if ( on == state ) 
		{
		  if ( rowptr > maxrow ) 
		  bad = true ; 
		  else {
		      
		    row [ rowptr ] = extra ; 
		    rowptr = rowptr + 1 ; 
		  } 
		} 
		{register integer for_end; j = 0 ; for_end = i ; if ( j <= 
		for_end) do 
		  {
		    if ( rowptr > maxrow ) 
		    bad = true ; 
		    else {
			
		      row [ rowptr ] = ( -99999L ) ; 
		      rowptr = rowptr + 1 ; 
		    } 
		  } 
		while ( j++ < for_end ) ; } 
		on = false ; 
		extra = 0 ; 
		state = true ; 
	      } 
	      break ; 
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
	    case 91 : 
	    case 92 : 
	    case 93 : 
	    case 94 : 
	    case 95 : 
	    case 96 : 
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
	    case 123 : 
	    case 124 : 
	    case 125 : 
	    case 126 : 
	    case 127 : 
	    case 128 : 
	    case 129 : 
	    case 130 : 
	    case 131 : 
	    case 132 : 
	    case 133 : 
	    case 134 : 
	    case 135 : 
	    case 136 : 
	    case 137 : 
	      dotherows = true ; 
	      break ; 
	    case 138 : 
	    case 139 : 
	    case 140 : 
	    case 141 : 
	    case 142 : 
	    case 143 : 
	    case 144 : 
	    case 145 : 
	    case 146 : 
	    case 147 : 
	    case 148 : 
	    case 149 : 
	    case 150 : 
	    case 151 : 
	    case 152 : 
	    case 153 : 
	    case 154 : 
	    case 155 : 
	    case 156 : 
	    case 157 : 
	    case 158 : 
	    case 159 : 
	    case 160 : 
	    case 161 : 
	    case 162 : 
	    case 163 : 
	    case 164 : 
	    case 165 : 
	    case 166 : 
	    case 167 : 
	    case 168 : 
	    case 169 : 
	    case 170 : 
	    case 171 : 
	    case 172 : 
	    case 173 : 
	    case 174 : 
	    case 175 : 
	    case 176 : 
	    case 177 : 
	    case 178 : 
	    case 179 : 
	    case 180 : 
	    case 181 : 
	    case 182 : 
	    case 183 : 
	    case 184 : 
	    case 185 : 
	    case 186 : 
	    case 187 : 
	    case 188 : 
	    case 189 : 
	    case 190 : 
	    case 191 : 
	    case 192 : 
	    case 193 : 
	    case 194 : 
	    case 195 : 
	    case 196 : 
	    case 197 : 
	    case 198 : 
	    case 199 : 
	    case 200 : 
	    case 201 : 
	      dotherows = true ; 
	      break ; 
	    case 202 : 
	    case 203 : 
	    case 204 : 
	    case 205 : 
	    case 206 : 
	    case 207 : 
	    case 208 : 
	    case 209 : 
	    case 210 : 
	    case 211 : 
	    case 212 : 
	    case 213 : 
	    case 214 : 
	    case 215 : 
	    case 216 : 
	    case 217 : 
	    case 218 : 
	    case 219 : 
	    case 220 : 
	    case 221 : 
	    case 222 : 
	    case 223 : 
	    case 224 : 
	    case 225 : 
	    case 226 : 
	    case 227 : 
	    case 228 : 
	    case 229 : 
	    case 230 : 
	    case 231 : 
	    case 232 : 
	    case 233 : 
	    case 234 : 
	    case 235 : 
	    case 236 : 
	    case 237 : 
	    case 238 : 
	      dotherows = true ; 
	      break ; 
	    case 239 : 
	    case 240 : 
	    case 241 : 
	    case 242 : 
	      {
		{
		  putbyte ( gfcom + 1 , pkfile ) ; 
		  pkloc = pkloc + 1 ; 
		} 
		i = 0 ; 
		{register integer for_end; j = 0 ; for_end = gfcom - 239 
		; if ( j <= for_end) do 
		  {
		    k = gfbyte () ; 
		    {
		      putbyte ( k , pkfile ) ; 
		      pkloc = pkloc + 1 ; 
		    } 
		    i = i * 256 + k ; 
		  } 
		while ( j++ < for_end ) ; } 
		{register integer for_end; j = 1 ; for_end = i ; if ( j <= 
		for_end) do 
		  {
		    putbyte ( gfbyte () , pkfile ) ; 
		    pkloc = pkloc + 1 ; 
		  } 
		while ( j++ < for_end ) ; } 
	      } 
	      break ; 
	    case 243 : 
	      {
		{
		  putbyte ( 244 , pkfile ) ; 
		  pkloc = pkloc + 1 ; 
		} 
		pkword ( gfsignedquad () ) ; 
	      } 
	      break ; 
	    case 244 : 
	      ; 
	      break ; 
	    case 69 : 
	      {
		if ( on == state ) 
		{
		  if ( rowptr > maxrow ) 
		  bad = true ; 
		  else {
		      
		    row [ rowptr ] = extra ; 
		    rowptr = rowptr + 1 ; 
		  } 
		} 
		if ( ( rowptr > 2 ) && ( row [ rowptr - 1 ] != ( -99999L ) ) ) 
		{
		  if ( rowptr > maxrow ) 
		  bad = true ; 
		  else {
		      
		    row [ rowptr ] = ( -99999L ) ; 
		    rowptr = rowptr + 1 ; 
		  } 
		} 
		{
		  if ( rowptr > maxrow ) 
		  bad = true ; 
		  else {
		      
		    row [ rowptr ] = ( -99998L ) ; 
		    rowptr = rowptr + 1 ; 
		  } 
		} 
		if ( bad ) 
		{
		  verbose = true ; 
		  if ( verbose ) 
		  (void) fprintf( stdout , "%s\n",                    "Ran out of internal memory for row counts!" ) ; 
		  uexit ( 1 ) ; 
		} 
		packandsendcharacter () ; 
		status [ gfchmod256 ] = 2 ; 
		if ( pkloc != predpkloc ) 
		{
		  verbose = true ; 
		  if ( verbose ) 
		  (void) fprintf( stdout , "%s\n",  "Internal error while writing character!"                   ) ; 
		  uexit ( 1 ) ; 
		} 
	      } 
	      break ; 
	      default: 
	      {
		verbose = true ; 
		if ( verbose ) 
		(void) fprintf( stdout , "%s%s%ld%s%c\n",  "Bad GF file: " , "Unexpected " , (long)gfcom ,                 " character in character definition" , '!' ) ; 
		uexit ( 1 ) ; 
	      } 
	      break ; 
	    } 
	    if ( dotherows ) 
	    {
	      dotherows = false ; 
	      if ( on == state ) 
	      {
		if ( rowptr > maxrow ) 
		bad = true ; 
		else {
		    
		  row [ rowptr ] = extra ; 
		  rowptr = rowptr + 1 ; 
		} 
	      } 
	      {
		if ( rowptr > maxrow ) 
		bad = true ; 
		else {
		    
		  row [ rowptr ] = ( -99999L ) ; 
		  rowptr = rowptr + 1 ; 
		} 
	      } 
	      on = true ; 
	      extra = gfcom - 74 ; 
	      state = false ; 
	    } 
	  } while ( ! ( gfcom == 69 ) ) ; 
	} 
      } 
      break ; 
    case 239 : 
    case 240 : 
    case 241 : 
    case 242 : 
      {
	{
	  putbyte ( gfcom + 1 , pkfile ) ; 
	  pkloc = pkloc + 1 ; 
	} 
	i = 0 ; 
	{register integer for_end; j = 0 ; for_end = gfcom - 239 ; if ( j <= 
	for_end) do 
	  {
	    k = gfbyte () ; 
	    {
	      putbyte ( k , pkfile ) ; 
	      pkloc = pkloc + 1 ; 
	    } 
	    i = i * 256 + k ; 
	  } 
	while ( j++ < for_end ) ; } 
	{register integer for_end; j = 1 ; for_end = i ; if ( j <= for_end) 
	do 
	  {
	    putbyte ( gfbyte () , pkfile ) ; 
	    pkloc = pkloc + 1 ; 
	  } 
	while ( j++ < for_end ) ; } 
      } 
      break ; 
    case 243 : 
      {
	{
	  putbyte ( 244 , pkfile ) ; 
	  pkloc = pkloc + 1 ; 
	} 
	pkword ( gfsignedquad () ) ; 
      } 
      break ; 
    case 244 : 
      ; 
      break ; 
    case 248 : 
      ; 
      break ; 
      default: 
      {
	verbose = true ; 
	if ( verbose ) 
	(void) fprintf( stdout , "%s%s%ld%s%c\n",  "Bad GF file: " , "Unexpected " , (long)gfcom ,         " command between characters" , '!' ) ; 
	uexit ( 1 ) ; 
      } 
      break ; 
    } 
  } while ( ! ( gfcom == 248 ) ) ; 
  {
    putbyte ( 245 , pkfile ) ; 
    pkloc = pkloc + 1 ; 
  } 
  while ( ( pkloc % 4 != 0 ) ) {
      
    putbyte ( 246 , pkfile ) ; 
    pkloc = pkloc + 1 ; 
  } 
} 
void main_body() {
    
  initialize () ; 
  convertgffile () ; 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    if ( status [ i ] == 1 ) 
    if ( verbose ) 
    (void) fprintf( stdout , "%s%ld%s\n",  "Character " , (long)i , " missing raster information!" ) ; 
  while ( i++ < for_end ) ; } 
  if ( verbose ) 
  (void) fprintf( stdout , "%ld%s%ld%s\n",  (long)gflen , " bytes packed to " , (long)pkloc , " bytes." ) ; 
} 
