#include "config.h"
typedef schar ASCIIcode  ; 
typedef file_ptr /* of  char */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
ASCIIcode xord[256]  ; 
char xchr[256]  ; 
bytefile pkfile  ; 
char pkname[PATHMAX + 1], typname[PATHMAX + 1]  ; 
integer curloc  ; 
integer termpos  ; 
integer magnification  ; 
integer designsize  ; 
integer checksum  ; 
integer hppp, vppp  ; 
integer i, j  ; 
integer flagbyte  ; 
integer endofpacket  ; 
integer width, height  ; 
integer xoff, yoff  ; 
integer tfmwidth  ; 
integer tfms[256]  ; 
integer dx, dy  ; 
integer dxs[256], dys[256]  ; 
boolean status[256]  ; 
integer dynf  ; 
integer car  ; 
integer packetlength  ; 
eightbits inputbyte  ; 
eightbits bitweight  ; 
eightbits nybble  ; 
integer repeatcount  ; 
integer rowsleft  ; 
boolean turnon  ; 
integer hbit  ; 
integer count  ; 

#include "pktype.h"
void initialize ( ) 
{integer i  ; 
  (void) fprintf( stdout , "%s\n",  "This is PKtype, C Version 2.3" ) ; 
  setpaths ( PKFILEPATHBIT ) ; 
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
} 
void openpkfile ( ) 
{integer j, k  ; 
  k = 1 ; 
  if ( argc != 2 ) 
  {
    (void) fprintf( stdout , "%s\n",  "Usage: pktype <pk file>." ) ; 
    uexit ( 1 ) ; 
  } 
  argv ( 1 , pkname ) ; 
  if ( testreadaccess ( pkname , PKFILEPATH ) ) 
  {
    reset ( pkfile , pkname ) ; 
    curloc = 0 ; 
  } 
  else {
      
    printpascalstring ( pkname ) ; 
    {
      (void) fprintf( stdout , "%s\n",  ": PK file not found." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
} 
integer getbyte ( ) 
{register integer Result; eightbits b  ; 
  if ( eof ( pkfile ) ) 
  Result = 0 ; 
  else {
      
    read ( pkfile , b ) ; 
    curloc = curloc + 1 ; 
    Result = b ; 
  } 
  return(Result) ; 
} 
integer gettwobytes ( ) 
{register integer Result; eightbits a, b  ; 
  read ( pkfile , a ) ; 
  read ( pkfile , b ) ; 
  curloc = curloc + 2 ; 
  Result = a * 256 + b ; 
  return(Result) ; 
} 
integer signedquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  read ( pkfile , a ) ; 
  read ( pkfile , b ) ; 
  read ( pkfile , c ) ; 
  read ( pkfile , d ) ; 
  curloc = curloc + 4 ; 
  if ( a < 128 ) 
  Result = ( ( a * 256 + b ) * 256 + c ) * 256 + d ; 
  else Result = ( ( ( a - 256 ) * 256 + b ) * 256 + c ) * 256 + d ; 
  return(Result) ; 
} 
integer getnyb ( ) 
{register integer Result; eightbits temp  ; 
  if ( bitweight == 0 ) 
  {
    inputbyte = getbyte () ; 
    bitweight = 16 ; 
  } 
  temp = inputbyte / bitweight ; 
  inputbyte = inputbyte - temp * bitweight ; 
  bitweight = bitweight / 16 ; 
  Result = temp ; 
  return(Result) ; 
} 
boolean getbit ( ) 
{register boolean Result; boolean temp  ; 
  bitweight = bitweight / 2 ; 
  if ( bitweight == 0 ) 
  {
    inputbyte = getbyte () ; 
    bitweight = 128 ; 
  } 
  temp = inputbyte >= bitweight ; 
  if ( temp ) 
  inputbyte = inputbyte - bitweight ; 
  Result = temp ; 
  return(Result) ; 
} 
void zsendout ( repeatcount , value ) 
boolean repeatcount ; 
integer value ; 
{integer i, len  ; 
  i = 10 ; 
  len = 1 ; 
  while ( value >= i ) {
      
    len = len + 1 ; 
    i = i * 10 ; 
  } 
  if ( repeatcount || ! turnon ) 
  len = len + 2 ; 
  if ( termpos + len > 78 ) 
  {
    termpos = len + 2 ; 
    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
    (void) Fputs( stdout ,  "  " ) ; 
  } 
  else termpos = termpos + len ; 
  if ( repeatcount ) 
  (void) fprintf( stdout , "%c%ld%c",  '[' , (long)value , ']' ) ; 
  else if ( turnon ) 
  (void) fprintf( stdout , "%ld",  (long)value ) ; 
  else
  (void) fprintf( stdout , "%c%ld%c",  '(' , (long)value , ')' ) ; 
} 
integer pkpackednum ( ) 
{register integer Result; integer i, j  ; 
  i = getnyb () ; 
  if ( i == 0 ) 
  {
    do {
	j = getnyb () ; 
      i = i + 1 ; 
    } while ( ! ( j != 0 ) ) ; 
    while ( i > 0 ) {
	
      j = j * 16 + getnyb () ; 
      i = i - 1 ; 
    } 
    Result = j - 15 + ( 13 - dynf ) * 16 + dynf ; 
  } 
  else if ( i <= dynf ) 
  Result = i ; 
  else if ( i < 14 ) 
  Result = ( i - dynf - 1 ) * 16 + getnyb () + dynf + 1 ; 
  else {
      
    if ( repeatcount != 0 ) 
    {
      (void) fprintf( stdout , "%s\n",  "Second repeat count for this row!" ) ; 
      uexit ( 1 ) ; 
    } 
    repeatcount = 1 ; 
    if ( i == 14 ) 
    repeatcount = pkpackednum () ; 
    sendout ( true , repeatcount ) ; 
    Result = pkpackednum () ; 
  } 
  return(Result) ; 
} 
void skipspecials ( ) 
{integer i, j  ; 
  do {
      flagbyte = getbyte () ; 
    if ( flagbyte >= 240 ) 
    switch ( flagbyte ) 
    {case 240 : 
    case 241 : 
    case 242 : 
    case 243 : 
      {
	(void) fprintf( stdout , "%ld%s",  (long)( curloc - 1 ) , ":  Special: '" ) ; 
	i = 0 ; 
	{register integer for_end; j = 240 ; for_end = flagbyte ; if ( j <= 
	for_end) do 
	  i = 256 * i + getbyte () ; 
	while ( j++ < for_end ) ; } 
	{register integer for_end; j = 1 ; for_end = i ; if ( j <= for_end) 
	do 
	  (void) putc( xchr [ getbyte () ] ,  stdout );
	while ( j++ < for_end ) ; } 
	(void) fprintf( stdout , "%c\n",  '\'' ) ; 
      } 
      break ; 
    case 244 : 
      (void) fprintf( stdout , "%ld%s%ld\n",  (long)( curloc - 1 ) , ":  Num special: " , (long)signedquad () ) 
      ; 
      break ; 
    case 245 : 
      (void) fprintf( stdout , "%ld%s\n",  (long)( curloc - 1 ) , ":  Postamble" ) ; 
      break ; 
    case 246 : 
      (void) fprintf( stdout , "%ld%s\n",  (long)( curloc - 1 ) , ":  No op" ) ; 
      break ; 
    case 247 : 
    case 248 : 
    case 249 : 
    case 250 : 
    case 251 : 
    case 252 : 
    case 253 : 
    case 254 : 
    case 255 : 
      {
	(void) fprintf( stdout , "%s%ld%c\n",  "Unexpected " , (long)flagbyte , '!' ) ; 
	uexit ( 1 ) ; 
      } 
      break ; 
    } 
  } while ( ! ( ( flagbyte < 240 ) || ( flagbyte == 245 ) ) ) ; 
} 
void main_body() {
    
  initialize () ; 
  openpkfile () ; 
  if ( getbyte () != 247 ) 
  {
    (void) fprintf( stdout , "%s\n",  "Bad PK file:  pre command missing!" ) ; 
    uexit ( 1 ) ; 
  } 
  if ( getbyte () != 89 ) 
  {
    (void) fprintf( stdout , "%s\n",  "Wrong version of PK file!" ) ; 
    uexit ( 1 ) ; 
  } 
  j = getbyte () ; 
  (void) putc( '\'' ,  stdout );
  {register integer for_end; i = 1 ; for_end = j ; if ( i <= for_end) do 
    (void) putc( xchr [ getbyte () ] ,  stdout );
  while ( i++ < for_end ) ; } 
  (void) fprintf( stdout , "%c\n",  '\'' ) ; 
  designsize = signedquad () ; 
  (void) fprintf( stdout , "%s%ld\n",  "Design size = " , (long)designsize ) ; 
  checksum = signedquad () ; 
  (void) fprintf( stdout , "%s%ld\n",  "Checksum = " , (long)checksum ) ; 
  hppp = signedquad () ; 
  vppp = signedquad () ; 
  (void) fprintf( stdout , "%s%ld%s%ld",  "Resolution: horizontal = " , (long)hppp , "  vertical = " , (long)vppp   ) ; 
  magnification = round ( hppp * 72.27 / ((double) 65536L ) ) ; 
  (void) fprintf( stdout , "%s%ld%s\n",  "  (" , (long)magnification , " dpi)" ) ; 
  if ( hppp != vppp ) 
  (void) fprintf( stdout , "%s\n",  "Warning:  aspect ratio not 1:1!" ) ; 
  skipspecials () ; 
  while ( flagbyte != 245 ) {
      
    (void) fprintf( stdout , "%ld%s%ld",  (long)( curloc - 1 ) , ":  Flag byte = " , (long)flagbyte ) ; 
    dynf = flagbyte / 16 ; 
    flagbyte = flagbyte % 16 ; 
    turnon = flagbyte >= 8 ; 
    if ( turnon ) 
    flagbyte = flagbyte - 8 ; 
    if ( flagbyte == 7 ) 
    {
      packetlength = signedquad () ; 
      car = signedquad () ; 
      endofpacket = packetlength + curloc ; 
      packetlength = packetlength + 9 ; 
      tfmwidth = signedquad () ; 
      dx = signedquad () ; 
      dy = signedquad () ; 
      width = signedquad () ; 
      height = signedquad () ; 
      xoff = signedquad () ; 
      yoff = signedquad () ; 
    } 
    else if ( flagbyte > 3 ) 
    {
      packetlength = ( flagbyte - 4 ) * 65536L + gettwobytes () ; 
      car = getbyte () ; 
      endofpacket = packetlength + curloc ; 
      packetlength = packetlength + 4 ; 
      i = getbyte () ; 
      tfmwidth = i * 65536L + gettwobytes () ; 
      dx = gettwobytes () * 65536L ; 
      dy = 0 ; 
      width = gettwobytes () ; 
      height = gettwobytes () ; 
      xoff = gettwobytes () ; 
      yoff = gettwobytes () ; 
      if ( xoff > 32767 ) 
      xoff = xoff - 65536L ; 
      if ( yoff > 32767 ) 
      yoff = yoff - 65536L ; 
    } 
    else {
	
      packetlength = flagbyte * 256 + getbyte () ; 
      car = getbyte () ; 
      endofpacket = packetlength + curloc ; 
      packetlength = packetlength + 3 ; 
      i = getbyte () ; 
      tfmwidth = i * 65536L + gettwobytes () ; 
      dx = getbyte () * 65536L ; 
      dy = 0 ; 
      width = getbyte () ; 
      height = getbyte () ; 
      xoff = getbyte () ; 
      yoff = getbyte () ; 
      if ( xoff > 127 ) 
      xoff = xoff - 256 ; 
      if ( yoff > 127 ) 
      yoff = yoff - 256 ; 
    } 
    (void) fprintf( stdout , "%s%ld%s%ld\n",  "  Character = " , (long)car , "  Packet length = " ,     (long)packetlength ) ; 
    (void) fprintf( stdout , "%s%ld\n",  "  Dynamic packing variable = " , (long)dynf ) ; 
    (void) fprintf( stdout , "%s%ld%s%ld",  "  TFM width = " , (long)tfmwidth , "  dx = " , (long)dx ) ; 
    if ( dy != 0 ) 
    (void) fprintf( stdout , "%s%ld\n",  "  dy = " , (long)dy ) ; 
    else
    (void) fprintf( stdout , "%c\n",  ' ' ) ; 
    (void) fprintf( stdout , "%s%ld%s%ld%s%ld%s%ld\n",  "  Height = " , (long)height , "  Width = " , (long)width ,     "  X-offset = " , (long)xoff , "  Y-offset = " , (long)yoff ) ; 
    bitweight = 0 ; 
    if ( dynf == 14 ) 
    {
      {register integer for_end; i = 1 ; for_end = height ; if ( i <= 
      for_end) do 
	{
	  (void) Fputs( stdout ,  "  " ) ; 
	  {register integer for_end; j = 1 ; for_end = width ; if ( j <= 
	  for_end) do 
	    if ( getbit () ) 
	    (void) putc( '*' ,  stdout );
	    else
	    (void) putc( '.' ,  stdout );
	  while ( j++ < for_end ) ; } 
	  (void) fprintf( stdout , "%c\n",  ' ' ) ; 
	} 
      while ( i++ < for_end ) ; } 
    } 
    else {
	
      termpos = 2 ; 
      (void) Fputs( stdout ,  "  " ) ; 
      rowsleft = height ; 
      hbit = width ; 
      repeatcount = 0 ; 
      while ( rowsleft > 0 ) {
	  
	count = pkpackednum () ; 
	sendout ( false , count ) ; 
	if ( count >= hbit ) 
	{
	  rowsleft = rowsleft - repeatcount - 1 ; 
	  repeatcount = 0 ; 
	  count = count - hbit ; 
	  hbit = width ; 
	  rowsleft = rowsleft - count / width ; 
	  count = count % width ; 
	} 
	hbit = hbit - count ; 
	turnon = ! turnon ; 
      } 
      (void) fprintf( stdout , "%c\n",  ' ' ) ; 
      if ( ( rowsleft != 0 ) || ( hbit != width ) ) 
      {
	(void) fprintf( stdout , "%s\n",  "Bad PK file: More bits than required!" ) ; 
	uexit ( 1 ) ; 
      } 
    } 
    if ( endofpacket != curloc ) 
    {
      (void) fprintf( stdout , "%s\n",  "Bad PK file: Bad packet length!" ) ; 
      uexit ( 1 ) ; 
    } 
    skipspecials () ; 
  } 
  j = 0 ; 
  while ( ! eof ( pkfile ) ) {
      
    i = getbyte () ; 
    if ( i != 246 ) 
    {
      (void) fprintf( stdout , "%s%ld\n",  "Bad byte at end of file: " , (long)i ) ; 
      uexit ( 1 ) ; 
    } 
    (void) fprintf( stdout , "%ld%s\n",  (long)( curloc - 1 ) , ":  No op" ) ; 
    j = j + 1 ; 
  } 
  (void) fprintf( stdout , "%ld%s\n",  (long)curloc , " bytes read from packed file." ) ; 
} 
