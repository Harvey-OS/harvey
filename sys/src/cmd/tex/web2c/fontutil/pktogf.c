#include "config.h"
#define maxcounts 400 
typedef schar ASCIIcode  ; 
typedef file_ptr /* of  char */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
ASCIIcode xord[256]  ; 
char xchr[256]  ; 
bytefile gffile, pkfile  ; 
char gfname[PATHMAX + 1], pkname[PATHMAX + 1]  ; 
integer gfloc, curloc  ; 
integer gfarg  ; 
boolean verbose  ; 
integer i, j  ; 
integer endofpacket  ; 
integer dynf  ; 
integer car  ; 
integer tfmwidth  ; 
integer xoff, yoff  ; 
char comment[18]  ; 
integer magnification  ; 
integer designsize  ; 
integer checksum  ; 
integer hppp, vppp  ; 
integer cheight, cwidth  ; 
integer wordwidth  ; 
integer horesc, veresc  ; 
integer packetlength  ; 
integer lasteoc  ; 
integer minm, maxm, minn, maxn  ; 
integer mminm, mmaxm, mminn, mmaxn  ; 
integer charpointer[256], stfmwidth[256]  ; 
integer shoresc[256], sveresc[256]  ; 
integer thischarptr  ; 
eightbits inputbyte  ; 
eightbits bitweight  ; 
integer rowcounts[maxcounts + 1]  ; 
integer rcp  ; 
integer countdown  ; 
boolean done  ; 
integer max  ; 
integer repeatcount  ; 
integer xtogo, ytogo  ; 
boolean turnon, firston  ; 
integer count  ; 
integer curn  ; 
integer flagbyte  ; 

#include "pktogf.h"
void initialize ( ) 
{integer i  ; 
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
  mminm = 999999L ; 
  mminn = 999999L ; 
  mmaxm = -999999L ; 
  mmaxn = -999999L ; 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    charpointer [ i ] = -1 ; 
  while ( i++ < for_end ) ; } 
} 
void openpkfile ( ) 
{integer j  ; 
  verbose = false ; 
  gfarg = 3 ; 
  if ( argc < 2 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s\n",  "Usage: pktogf [-v] <pk file> [gf file]." ) ; 
    uexit ( 1 ) ; 
  } 
  argv ( 1 , pkname ) ; 
  if ( pkname [ 1 ] == xchr [ 45 ] ) 
  {
    if ( argc > 4 ) 
    {
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s\n",  "Usage: pktogf [-v] <pk file> [gf file]." ) ; 
      uexit ( 1 ) ; 
    } 
    if ( pkname [ 2 ] == xchr [ 118 ] ) 
    {
      verbose = true ; 
      argv ( 2 , pkname ) ; 
      gfarg = gfarg + 1 ; 
    } 
    else {
	
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s\n",  "Usage: pktogf [-v] <pk file> [gf file]." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  if ( verbose ) 
  (void) fprintf( stdout , "%s\n",  "This is PKtoGF, C Version 1.1 " ) ; 
  if ( testreadaccess ( pkname , PKFILEPATH ) ) 
  {
    reset ( pkfile , pkname ) ; 
  } 
  else {
      
    printpascalstring ( pkname ) ; 
    {
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s\n",  ": PK file not found." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  curloc = 0 ; 
} 
void opengffile ( ) 
{integer dotpos, slashpos, last, gfindex, pkindex  ; 
  if ( argc == gfarg ) 
  argv ( argc - 1 , gfname ) ; 
  else {
      
    dotpos = -1 ; 
    slashpos = -1 ; 
    last = 1 ; 
    while ( ( pkname [ last ] != ' ' ) && ( last <= PATHMAX - 5 ) ) {
	
      if ( pkname [ last ] == '.' ) 
      dotpos = last ; 
      if ( pkname [ last ] == '/' ) 
      slashpos = last ; 
      last = last + 1 ; 
    } 
    if ( slashpos == -1 ) 
    slashpos = 0 ; 
    if ( dotpos < slashpos ) 
    dotpos = last - 1 ; 
    gfindex = 1 ; 
    {register integer for_end; pkindex = slashpos + 1 ; for_end = dotpos 
    ; if ( pkindex <= for_end) do 
      {
	gfname [ gfindex ] = pkname [ pkindex ] ; 
	gfindex = gfindex + 1 ; 
      } 
    while ( pkindex++ < for_end ) ; } 
    pkindex = dotpos + 1 ; 
    while ( ( pkindex < last ) && ( pkname [ pkindex ] != 'p' ) ) {
	
      gfname [ gfindex ] = pkname [ pkindex ] ; 
      pkindex = pkindex + 1 ; 
      gfindex = gfindex + 1 ; 
    } 
    gfname [ gfindex ] = 'g' ; 
    gfname [ gfindex + 1 ] = 'f' ; 
    gfname [ gfindex + 2 ] = ' ' ; 
  } 
  if ( verbose ) 
  (void) putc( xchr [ xord [ '[' ] ] ,  stdout );
  pkindex = 1 ; 
  while ( pkname [ pkindex ] != ' ' ) {
      
    if ( verbose ) 
    (void) putc( xchr [ xord [ pkname [ pkindex ] ] ] ,  stdout );
    pkindex = pkindex + 1 ; 
  } 
  if ( verbose ) 
  (void) putc( xchr [ xord [ '-' ] ] ,  stdout );
  if ( verbose ) 
  (void) putc( xchr [ xord [ '>' ] ] ,  stdout );
  gfindex = 1 ; 
  while ( gfname [ gfindex ] != ' ' ) {
      
    if ( verbose ) 
    (void) putc( xchr [ xord [ gfname [ gfindex ] ] ] ,  stdout );
    gfindex = gfindex + 1 ; 
  } 
  if ( verbose ) 
  (void) putc( xchr [ xord [ ']' ] ] ,  stdout );
  if ( verbose ) 
  (void) fprintf( stdout , "%c\n",  xchr [ xord [ ' ' ] ] ) ; 
  rewrite ( gffile , gfname ) ; 
  gfloc = 0 ; 
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
integer signedbyte ( ) 
{register integer Result; eightbits b  ; 
  read ( pkfile , b ) ; 
  curloc = curloc + 1 ; 
  if ( b < 128 ) 
  Result = b ; 
  else Result = b - 256 ; 
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
integer signedpair ( ) 
{register integer Result; eightbits a, b  ; 
  read ( pkfile , a ) ; 
  read ( pkfile , b ) ; 
  curloc = curloc + 2 ; 
  if ( a < 128 ) 
  Result = a * 256 + b ; 
  else Result = ( a - 256 ) * 256 + b ; 
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
void zgf16 ( i ) 
integer i ; 
{{
    
    putbyte ( i / 256 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  {
    putbyte ( i % 256 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
} 
void zgf24 ( i ) 
integer i ; 
{{
    
    putbyte ( i / 65536L , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  gf16 ( i % 65536L ) ; 
} 
void zgfquad ( i ) 
integer i ; 
{if ( i >= 0 ) 
  {
    {
      putbyte ( i / 16777216L , gffile ) ; 
      gfloc = gfloc + 1 ; 
    } 
  } 
  else {
      
    i = ( i + 1073741824L ) + 1073741824L ; 
    {
      putbyte ( 128 + ( i / 16777216L ) , gffile ) ; 
      gfloc = gfloc + 1 ; 
    } 
  } 
  gf24 ( i % 16777216L ) ; 
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
      
    if ( i == 14 ) 
    repeatcount = pkpackednum () ; 
    else repeatcount = 1 ; 
    Result = pkpackednum () ; 
  } 
  return(Result) ; 
} 
void skipspecials ( ) 
{integer i, j, k  ; 
  thischarptr = gfloc ; 
  do {
      flagbyte = getbyte () ; 
    if ( flagbyte >= 240 ) 
    switch ( flagbyte ) 
    {case 240 : 
    case 241 : 
    case 242 : 
    case 243 : 
      {
	i = 0 ; 
	{
	  putbyte ( flagbyte - 1 , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{register integer for_end; j = 240 ; for_end = flagbyte ; if ( j <= 
	for_end) do 
	  {
	    k = getbyte () ; 
	    {
	      putbyte ( k , gffile ) ; 
	      gfloc = gfloc + 1 ; 
	    } 
	    i = 256 * i + k ; 
	  } 
	while ( j++ < for_end ) ; } 
	{register integer for_end; j = 1 ; for_end = i ; if ( j <= for_end) 
	do 
	  {
	    putbyte ( getbyte () , gffile ) ; 
	    gfloc = gfloc + 1 ; 
	  } 
	while ( j++ < for_end ) ; } 
      } 
      break ; 
    case 244 : 
      {
	{
	  putbyte ( 243 , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	gfquad ( signedquad () ) ; 
      } 
      break ; 
    case 245 : 
      {
	; 
      } 
      break ; 
    case 246 : 
      {
	; 
      } 
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
	verbose = true ; 
	if ( verbose ) 
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
  opengffile () ; 
  if ( getbyte () != 247 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s\n",  "Bad pk file!  pre command missing." ) ; 
    uexit ( 1 ) ; 
  } 
  {
    putbyte ( 247 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  if ( getbyte () != 89 ) 
  {
    verbose = true ; 
    if ( verbose ) 
    (void) fprintf( stdout , "%s\n",  "Wrong version of packed file!." ) ; 
    uexit ( 1 ) ; 
  } 
  {
    putbyte ( 131 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  j = getbyte () ; 
  {
    putbyte ( j , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  if ( verbose ) 
  (void) putc( '{' ,  stdout );
  {register integer for_end; i = 1 ; for_end = j ; if ( i <= for_end) do 
    {
      hppp = getbyte () ; 
      {
	putbyte ( hppp , gffile ) ; 
	gfloc = gfloc + 1 ; 
      } 
      if ( verbose ) 
      (void) putc( xchr [ xord [ hppp ] ] ,  stdout );
    } 
  while ( i++ < for_end ) ; } 
  if ( verbose ) 
  (void) fprintf( stdout , "%c\n",  '}' ) ; 
  designsize = signedquad () ; 
  checksum = signedquad () ; 
  hppp = signedquad () ; 
  vppp = signedquad () ; 
  if ( hppp != vppp ) 
  if ( verbose ) 
  (void) fprintf( stdout , "%s\n",  "Warning:  aspect ratio not 1:1!" ) ; 
  magnification = round ( hppp * 72.27 * 5 / ((double) 65536L ) ) ; 
  lasteoc = gfloc ; 
  skipspecials () ; 
  while ( flagbyte != 245 ) {
      
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
      tfmwidth = signedquad () ; 
      horesc = signedquad () ; 
      veresc = signedquad () ; 
      cwidth = signedquad () ; 
      cheight = signedquad () ; 
      wordwidth = ( cwidth + 31 ) / 32 ; 
      xoff = signedquad () ; 
      yoff = signedquad () ; 
    } 
    else if ( flagbyte > 3 ) 
    {
      packetlength = ( flagbyte - 4 ) * 65536L + gettwobytes () ; 
      car = getbyte () ; 
      endofpacket = packetlength + curloc ; 
      i = getbyte () ; 
      tfmwidth = i * 65536L + gettwobytes () ; 
      horesc = gettwobytes () * 65536L ; 
      veresc = 0 ; 
      cwidth = gettwobytes () ; 
      cheight = gettwobytes () ; 
      wordwidth = ( cwidth + 31 ) / 32 ; 
      xoff = signedpair () ; 
      yoff = signedpair () ; 
    } 
    else {
	
      packetlength = flagbyte * 256 + getbyte () ; 
      car = getbyte () ; 
      endofpacket = packetlength + curloc ; 
      i = getbyte () ; 
      tfmwidth = i * 65536L + gettwobytes () ; 
      horesc = getbyte () * 65536L ; 
      veresc = 0 ; 
      cwidth = getbyte () ; 
      cheight = getbyte () ; 
      wordwidth = ( cwidth + 31 ) / 32 ; 
      xoff = signedbyte () ; 
      yoff = signedbyte () ; 
    } 
    if ( ( cheight == 0 ) || ( cwidth == 0 ) ) 
    {
      cheight = 0 ; 
      cwidth = 0 ; 
      xoff = 0 ; 
      yoff = 0 ; 
    } 
    minm = - (integer) xoff ; 
    if ( minm < mminm ) 
    mminm = minm ; 
    maxm = cwidth + minm ; 
    if ( maxm > mmaxm ) 
    mmaxm = maxm ; 
    minn = yoff - cheight + 1 ; 
    maxn = yoff ; 
    if ( minn > maxn ) 
    minn = maxn ; 
    if ( minn < mminn ) 
    mminn = minn ; 
    if ( maxn > mmaxn ) 
    mmaxn = maxn ; 
    {
      i = car % 256 ; 
      if ( ( charpointer [ i ] == -1 ) ) 
      {
	sveresc [ i ] = veresc ; 
	shoresc [ i ] = horesc ; 
	stfmwidth [ i ] = tfmwidth ; 
      } 
      else {
	  
	if ( ( sveresc [ i ] != veresc ) || ( shoresc [ i ] != horesc ) || ( 
	stfmwidth [ i ] != tfmwidth ) ) 
	if ( verbose ) 
	(void) fprintf( stdout , "%s%ld%s\n",  "Two characters mod " , (long)i ,         " have mismatched parameters" ) ; 
      } 
    } 
    {
      if ( ( charpointer [ car % 256 ] == -1 ) && ( car >= 0 ) && ( car < 256 
      ) && ( maxm >= 0 ) && ( maxm < 256 ) && ( maxn >= 0 ) && ( maxn < 256 ) 
      && ( maxm >= minm ) && ( maxn >= minn ) && ( maxm < minm + 256 ) && ( 
      maxn < minn + 256 ) ) 
      {
	charpointer [ car % 256 ] = thischarptr ; 
	{
	  putbyte ( 68 , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( car , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( maxm - minm , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( maxm , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( maxn - minn , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( maxn , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
      } 
      else {
	  
	{
	  putbyte ( 67 , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	gfquad ( car ) ; 
	gfquad ( charpointer [ car % 256 ] ) ; 
	charpointer [ car % 256 ] = thischarptr ; 
	gfquad ( minm ) ; 
	gfquad ( maxm ) ; 
	gfquad ( minn ) ; 
	gfquad ( maxn ) ; 
      } 
    } 
    if ( ( cwidth > 0 ) && ( cheight > 0 ) ) 
    {
      bitweight = 0 ; 
      countdown = cheight * cwidth - 1 ; 
      if ( dynf == 14 ) 
      turnon = getbit () ; 
      repeatcount = 0 ; 
      xtogo = cwidth ; 
      ytogo = cheight ; 
      curn = cheight ; 
      count = 0 ; 
      firston = turnon ; 
      turnon = ! turnon ; 
      rcp = 0 ; 
      while ( ytogo > 0 ) {
	  
	if ( count == 0 ) 
	{
	  turnon = ! turnon ; 
	  if ( dynf == 14 ) 
	  {
	    count = 1 ; 
	    done = false ; 
	    while ( ! done ) {
		
	      if ( countdown <= 0 ) 
	      done = true ; 
	      else if ( ( turnon == getbit () ) ) 
	      count = count + 1 ; 
	      else done = true ; 
	      countdown = countdown - 1 ; 
	    } 
	  } 
	  else count = pkpackednum () ; 
	} 
	if ( rcp == 0 ) 
	firston = turnon ; 
	while ( count >= xtogo ) {
	    
	  rowcounts [ rcp ] = xtogo ; 
	  count = count - xtogo ; 
	  {register integer for_end; i = 0 ; for_end = repeatcount ; if ( i 
	  <= for_end) do 
	    {
	      if ( ( rcp > 0 ) || firston ) 
	      {
		j = 0 ; 
		max = rcp ; 
		if ( ! turnon ) 
		max = max - 1 ; 
		if ( curn - ytogo == 1 ) 
		{
		  if ( firston ) 
		  {
		    putbyte ( 74 , gffile ) ; 
		    gfloc = gfloc + 1 ; 
		  } 
		  else if ( rowcounts [ 0 ] < 165 ) 
		  {
		    {
		      putbyte ( 74 + rowcounts [ 0 ] , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		    j = j + 1 ; 
		  } 
		  else {
		      
		    putbyte ( 70 , gffile ) ; 
		    gfloc = gfloc + 1 ; 
		  } 
		} 
		else if ( curn > ytogo ) 
		{
		  if ( curn - ytogo < 257 ) 
		  {
		    {
		      putbyte ( 71 , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		    {
		      putbyte ( curn - ytogo - 1 , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		  } 
		  else {
		      
		    {
		      putbyte ( 72 , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		    gf16 ( curn - ytogo - 1 ) ; 
		  } 
		  if ( firston ) 
		  {
		    putbyte ( 0 , gffile ) ; 
		    gfloc = gfloc + 1 ; 
		  } 
		} 
		else if ( firston ) 
		{
		  putbyte ( 0 , gffile ) ; 
		  gfloc = gfloc + 1 ; 
		} 
		curn = ytogo ; 
		while ( j <= max ) {
		    
		  if ( rowcounts [ j ] < 64 ) 
		  {
		    putbyte ( 0 + rowcounts [ j ] , gffile ) ; 
		    gfloc = gfloc + 1 ; 
		  } 
		  else if ( rowcounts [ j ] < 256 ) 
		  {
		    {
		      putbyte ( 64 , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		    {
		      putbyte ( rowcounts [ j ] , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		  } 
		  else {
		      
		    {
		      putbyte ( 65 , gffile ) ; 
		      gfloc = gfloc + 1 ; 
		    } 
		    gf16 ( rowcounts [ j ] ) ; 
		  } 
		  j = j + 1 ; 
		} 
	      } 
	      ytogo = ytogo - 1 ; 
	    } 
	  while ( i++ < for_end ) ; } 
	  repeatcount = 0 ; 
	  xtogo = cwidth ; 
	  rcp = 0 ; 
	  if ( ( count > 0 ) ) 
	  firston = turnon ; 
	} 
	if ( count > 0 ) 
	{
	  rowcounts [ rcp ] = count ; 
	  if ( rcp == 0 ) 
	  firston = turnon ; 
	  rcp = rcp + 1 ; 
	  if ( rcp > maxcounts ) 
	  {
	    verbose = true ; 
	    if ( verbose ) 
	    (void) fprintf( stdout , "%s\n",  "A character had too many run counts" ) ; 
	    uexit ( 1 ) ; 
	  } 
	  xtogo = xtogo - count ; 
	  count = 0 ; 
	} 
      } 
    } 
    {
      putbyte ( 69 , gffile ) ; 
      gfloc = gfloc + 1 ; 
    } 
    lasteoc = gfloc ; 
    if ( endofpacket != curloc ) 
    {
      verbose = true ; 
      if ( verbose ) 
      (void) fprintf( stdout , "%s\n",  "Bad pk file!  Bad packet length." ) ; 
      uexit ( 1 ) ; 
    } 
    skipspecials () ; 
  } 
  while ( ! eof ( pkfile ) ) i = getbyte () ; 
  j = gfloc ; 
  {
    putbyte ( 248 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  gfquad ( lasteoc ) ; 
  gfquad ( designsize ) ; 
  gfquad ( checksum ) ; 
  gfquad ( hppp ) ; 
  gfquad ( vppp ) ; 
  gfquad ( mminm ) ; 
  gfquad ( mmaxm ) ; 
  gfquad ( mminn ) ; 
  gfquad ( mmaxn ) ; 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    if ( charpointer [ i ] != -1 ) 
    {
      if ( ( sveresc [ i ] == 0 ) && ( shoresc [ i ] >= 0 ) && ( shoresc [ i ] 
      < 16777216L ) && ( shoresc [ i ] % 65536L == 0 ) ) 
      {
	{
	  putbyte ( 246 , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( i , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( shoresc [ i ] / 65536L , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
      } 
      else {
	  
	{
	  putbyte ( 245 , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	{
	  putbyte ( i , gffile ) ; 
	  gfloc = gfloc + 1 ; 
	} 
	gfquad ( shoresc [ i ] ) ; 
	gfquad ( sveresc [ i ] ) ; 
      } 
      gfquad ( stfmwidth [ i ] ) ; 
      gfquad ( charpointer [ i ] ) ; 
    } 
  while ( i++ < for_end ) ; } 
  {
    putbyte ( 249 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  gfquad ( j ) ; 
  {
    putbyte ( 131 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  {register integer for_end; i = 0 ; for_end = 3 ; if ( i <= for_end) do 
    {
      putbyte ( 223 , gffile ) ; 
      gfloc = gfloc + 1 ; 
    } 
  while ( i++ < for_end ) ; } 
  while ( gfloc % 4 != 0 ) {
      
    putbyte ( 223 , gffile ) ; 
    gfloc = gfloc + 1 ; 
  } 
  if ( verbose ) 
  (void) fprintf( stdout , "%ld%s%ld%s\n",  (long)curloc , " bytes unpacked to " , (long)gfloc , " bytes." ) ; 
} 
