#include "config.h"
#define linelength 79 
#define maxrow 79 
#define maxcol 79 
typedef schar ASCIIcode  ; 
typedef file_ptr /* of  char */ textfile  ; 
typedef unsigned char eightbits  ; 
typedef file_ptr /* of  eightbits */ bytefile  ; 
typedef char UNIXfilename [PATHMAX + 1] ; 
typedef schar pixel  ; 
ASCIIcode xord[256]  ; 
char xchr[256]  ; 
bytefile gffile  ; 
integer curloc  ; 
boolean wantsmnemonics  ; 
boolean wantspixels  ; 
integer m, n  ; 
pixel paintswitch  ; 
pixel imagearray[maxcol + 1][maxrow + 1]  ; 
integer maxsubrow, maxsubcol  ; 
integer minmstated, maxmstated, minnstated, maxnstated  ; 
integer maxmobserved, maxnobserved  ; 
integer minmoverall, maxmoverall, minnoverall, maxnoverall  ; 
integer totalchars  ; 
integer charptr[256]  ; 
integer gfprevptr  ; 
integer charactercode  ; 
boolean badchar  ; 
integer designsize, checksum  ; 
integer hppp, vppp  ; 
integer postloc  ; 
real pixratio  ; 
integer a  ; 
integer b, c, l, o, p, q, r  ; 

#include "gftype.h"
void initialize ( ) 
{integer i  ; 
  setpaths ( GFFILEPATHBIT ) ; 
  (void) fprintf( stdout , "%s\n",  "This is GFtype, C Version 3.1" ) ; 
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
  wantsmnemonics = false ; 
  wantspixels = false ; 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    charptr [ i ] = -1 ; 
  while ( i++ < for_end ) ; } 
  totalchars = 0 ; 
  minmoverall = 536870911L ; 
  maxmoverall = -536870911L ; 
  minnoverall = 536870911L ; 
  maxnoverall = -536870911L ; 
} 
void opengffile ( ) 
{integer j, k  ; 
  UNIXfilename gfname  ; 
  k = 1 ; 
  if ( ( argc < 2 ) || ( argc > 4 ) ) 
  {
    (void) fprintf( stdout , "%s\n",  "Usage: gftype [-m] [-i] <gf file>." ) ; 
    uexit ( 1 ) ; 
  } 
  argv ( 1 , gfname ) ; 
  while ( gfname [ 1 ] == xchr [ 45 ] ) {
      
    k = k + 1 ; 
    wantsmnemonics = wantsmnemonics || ( gfname [ 2 ] == xchr [ 109 ] ) || ( 
    gfname [ 3 ] == xchr [ 109 ] ) ; 
    wantspixels = wantspixels || ( gfname [ 2 ] == xchr [ 105 ] ) || ( gfname 
    [ 3 ] == xchr [ 105 ] ) ; 
    if ( wantspixels || wantsmnemonics ) 
    argv ( k , gfname ) ; 
    else {
	
      (void) fprintf( stdout , "%s\n",  "Usage: gftype [-m] [-i] <gf file>." ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  if ( testreadaccess ( gfname , GFFILEPATH ) ) 
  {
    reset ( gffile , gfname ) ; 
    curloc = 0 ; 
  } 
  else {
      
    printpascalstring ( gfname ) ; 
    {
      (void) fprintf( stdout , "%s\n",  ": GF file not found" ) ; 
      uexit ( 1 ) ; 
    } 
  } 
  (void) Fputs( stdout ,  "Options selected: Mnemonic output = " ) ; 
  if ( wantsmnemonics ) 
  (void) Fputs( stdout ,  "true" ) ; 
  else
  (void) Fputs( stdout ,  "false" ) ; 
  (void) Fputs( stdout ,  "; pixel output = " ) ; 
  if ( wantspixels ) 
  (void) Fputs( stdout ,  "true" ) ; 
  else
  (void) Fputs( stdout ,  "false" ) ; 
  (void) fprintf( stdout , "%c\n",  '.' ) ; 
} 
integer getbyte ( ) 
{register integer Result; eightbits b  ; 
  if ( eof ( gffile ) ) 
  Result = 0 ; 
  else {
      
    read ( gffile , b ) ; 
    curloc = curloc + 1 ; 
    Result = b ; 
  } 
  return(Result) ; 
} 
integer gettwobytes ( ) 
{register integer Result; eightbits a, b  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  curloc = curloc + 2 ; 
  Result = a * 256 + b ; 
  return(Result) ; 
} 
integer getthreebytes ( ) 
{register integer Result; eightbits a, b, c  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  read ( gffile , c ) ; 
  curloc = curloc + 3 ; 
  Result = ( a * 256 + b ) * 256 + c ; 
  return(Result) ; 
} 
integer signedquad ( ) 
{register integer Result; eightbits a, b, c, d  ; 
  read ( gffile , a ) ; 
  read ( gffile , b ) ; 
  read ( gffile , c ) ; 
  read ( gffile , d ) ; 
  curloc = curloc + 4 ; 
  if ( a < 128 ) 
  Result = ( ( a * 256 + b ) * 256 + c ) * 256 + d ; 
  else Result = ( ( ( a - 256 ) * 256 + b ) * 256 + c ) * 256 + d ; 
  return(Result) ; 
} 
void zprintscaled ( s ) 
integer s ; 
{integer delta  ; 
  if ( s < 0 ) 
  {
    (void) putc( '-' ,  stdout );
    s = - (integer) s ; 
  } 
  (void) fprintf( stdout , "%ld",  (long)s / 65536L ) ; 
  s = 10 * ( s % 65536L ) + 5 ; 
  if ( s != 5 ) 
  {
    delta = 10 ; 
    (void) putc( '.' ,  stdout );
    do {
	if ( delta > 65536L ) 
      s = s + 32768L - ( delta / 2 ) ; 
      (void) putc( xchr [ ord ( '0' ) + ( s / 65536L ) ] ,  stdout );
      s = 10 * ( s % 65536L ) ; 
      delta = delta * 10 ; 
    } while ( ! ( s <= delta ) ) ; 
  } 
} 
integer zfirstpar ( o ) 
eightbits o ; 
{register integer Result; switch ( o ) 
  {case 0 : 
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
    Result = o - 0 ; 
    break ; 
  case 64 : 
  case 71 : 
  case 245 : 
  case 246 : 
  case 239 : 
    Result = getbyte () ; 
    break ; 
  case 65 : 
  case 72 : 
  case 240 : 
    Result = gettwobytes () ; 
    break ; 
  case 66 : 
  case 73 : 
  case 241 : 
    Result = getthreebytes () ; 
    break ; 
  case 242 : 
  case 243 : 
    Result = signedquad () ; 
    break ; 
  case 67 : 
  case 68 : 
  case 69 : 
  case 70 : 
  case 244 : 
  case 247 : 
  case 248 : 
  case 249 : 
  case 250 : 
  case 251 : 
  case 252 : 
  case 253 : 
  case 254 : 
  case 255 : 
    Result = 0 ; 
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
    Result = o - 74 ; 
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
    Result = o - 74 ; 
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
    Result = o - 74 ; 
    break ; 
  } 
  return(Result) ; 
} 
boolean dochar ( ) 
{/* 9998 9999 */ register boolean Result; eightbits o  ; 
  integer p, q  ; 
  boolean aok  ; 
  aok = true ; 
  while ( true ) {
      
    a = curloc ; 
    o = getbyte () ; 
    p = firstpar ( o ) ; 
    if ( eof ( gffile ) ) 
    {
      (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "the file ended prematurely" , '!'       ) ; 
      uexit ( 1 ) ; 
    } 
    if ( o <= 67 ) 
    {
      if ( wantsmnemonics ) 
      (void) Fputs( stdout ,  " paint " ) ; 
      do {
	  if ( wantsmnemonics ) 
	if ( paintswitch == 0 ) 
	(void) fprintf( stdout , "%c%ld%c",  '(' , (long)p , ')' ) ; 
	else
	(void) fprintf( stdout , "%ld",  (long)p ) ; 
	m = m + p ; 
	if ( m > maxmobserved ) 
	maxmobserved = m - 1 ; 
	if ( wantspixels ) 
	if ( paintswitch == 1 ) 
	if ( n <= maxsubrow ) 
	{
	  l = m - p ; 
	  r = m - 1 ; 
	  if ( r > maxsubcol ) 
	  r = maxsubcol ; 
	  m = l ; 
	  while ( m <= r ) {
	      
	    imagearray [ m ][ n ] = 1 ; 
	    m = m + 1 ; 
	  } 
	  m = l + p ; 
	} 
	paintswitch = 1 - paintswitch ; 
	a = curloc ; 
	o = getbyte () ; 
	p = firstpar ( o ) ; 
	if ( eof ( gffile ) ) 
	{
	  (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "the file ended prematurely" ,           '!' ) ; 
	  uexit ( 1 ) ; 
	} 
      } while ( ! ( o > 67 ) ) ; 
    } 
    switch ( o ) 
    {case 70 : 
    case 71 : 
    case 72 : 
    case 73 : 
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%ld%c%ld",  (long)a , ": " , "skip" , (long)( o - 70 ) % 4 , ' ' , (long)p ) ; 
	} 
	n = n + p + 1 ; 
	m = 0 ; 
	paintswitch = 0 ; 
	if ( wantsmnemonics ) 
	(void) fprintf( stdout , "%s%ld%c",  " (n=" , (long)maxnstated - n , ')' ) ; 
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
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%ld",  (long)a , ": " , "newrow " , (long)p ) ; 
	} 
	n = n + 1 ; 
	m = p ; 
	paintswitch = 1 ; 
	if ( wantsmnemonics ) 
	(void) fprintf( stdout , "%s%ld%c",  " (n=" , (long)maxnstated - n , ')' ) ; 
      } 
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
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%ld",  (long)a , ": " , "newrow " , (long)p ) ; 
	} 
	n = n + 1 ; 
	m = p ; 
	paintswitch = 1 ; 
	if ( wantsmnemonics ) 
	(void) fprintf( stdout , "%s%ld%c",  " (n=" , (long)maxnstated - n , ')' ) ; 
      } 
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
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%ld",  (long)a , ": " , "newrow " , (long)p ) ; 
	} 
	n = n + 1 ; 
	m = p ; 
	paintswitch = 1 ; 
	if ( wantsmnemonics ) 
	(void) fprintf( stdout , "%s%ld%c",  " (n=" , (long)maxnstated - n , ')' ) ; 
      } 
      break ; 
    case 244 : 
      if ( wantsmnemonics ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%ld%s%s",  (long)a , ": " , "no op" ) ; 
      } 
      break ; 
    case 247 : 
      {
	{
	  (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " ,           "preamble command within a character!" ) ; 
	  (void) putc('\n',  stdout );
	} 
	goto lab9998 ; 
      } 
      break ; 
    case 248 : 
    case 249 : 
      {
	{
	  (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " ,           "postamble command within a character!" ) ; 
	  (void) putc('\n',  stdout );
	} 
	goto lab9998 ; 
      } 
      break ; 
    case 67 : 
    case 68 : 
      {
	{
	  (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " , "boc occurred before eoc!" ) ; 
	  (void) putc('\n',  stdout );
	} 
	goto lab9998 ; 
      } 
      break ; 
    case 69 : 
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s",  (long)a , ": " , "eoc" ) ; 
	} 
	(void) putc('\n',  stdout );
	goto lab9999 ; 
      } 
      break ; 
    case 239 : 
    case 240 : 
    case 241 : 
    case 242 : 
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s",  (long)a , ": " , "xxx '" ) ; 
	} 
	badchar = false ; 
	b = 16 ; 
	if ( p < 0 ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " , "string of negative length!" ) ; 
	  (void) putc('\n',  stdout );
	} 
	while ( p > 0 ) {
	    
	  q = getbyte () ; 
	  if ( ( q < 32 ) || ( q > 126 ) ) 
	  badchar = true ; 
	  if ( wantsmnemonics ) 
	  {
	    (void) putc( xchr [ q ] ,  stdout );
	    if ( b < linelength ) 
	    b = b + 1 ; 
	    else {
		
	      (void) putc('\n',  stdout );
	      b = 2 ; 
	    } 
	  } 
	  p = p - 1 ; 
	} 
	if ( wantsmnemonics ) 
	(void) putc( '\'' ,  stdout );
	if ( badchar ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " ,           "non-ASCII character in xxx command!" ) ; 
	  (void) putc('\n',  stdout );
	} 
      } 
      break ; 
    case 243 : 
      {
	if ( wantsmnemonics ) 
	{
	  (void) putc('\n',  stdout );
	  (void) fprintf( stdout , "%ld%s%s%ld%s",  (long)a , ": " , "yyy " , (long)p , " (" ) ; 
	} 
	if ( wantsmnemonics ) 
	{
	  printscaled ( p ) ; 
	  (void) putc( ')' ,  stdout );
	} 
      } 
      break ; 
      default: 
      {
	(void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "undefined command " , (long)o , '!' ) ; 
	(void) putc('\n',  stdout );
      } 
      break ; 
    } 
  } 
  lab9998: (void) fprintf( stdout , "%c\n",  '!' ) ; 
  aok = false ; 
  lab9999: Result = aok ; 
  return(Result) ; 
} 
void readpostamble ( ) 
{integer k  ; 
  integer p, q, m, u, v, w, c  ; 
  postloc = curloc - 1 ; 
  (void) fprintf( stdout , "%s%ld",  "Postamble starts at byte " , (long)postloc ) ; 
  if ( postloc == gfprevptr ) 
  (void) fprintf( stdout , "%c\n",  '.' ) ; 
  else
  (void) fprintf( stdout , "%s%ld%c\n",  ", after special info at byte " , (long)gfprevptr , '.' ) 
  ; 
  p = signedquad () ; 
  if ( p != gfprevptr ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%s%ld%s%ld%c",  (long)a , ": " , "! " , "backpointer in byte " , (long)curloc - 4 ,     " should be " , (long)gfprevptr , " not " , (long)p , '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  designsize = signedquad () ; 
  checksum = signedquad () ; 
  (void) fprintf( stdout , "%s%ld%s",  "design size = " , (long)designsize , " (" ) ; 
  printscaled ( designsize / 16 ) ; 
  (void) fprintf( stdout , "%s\n",  "pt)" ) ; 
  (void) fprintf( stdout , "%s%ld\n",  "check sum = " , (long)checksum ) ; 
  hppp = signedquad () ; 
  vppp = signedquad () ; 
  (void) fprintf( stdout , "%s%ld%s",  "hppp = " , (long)hppp , " (" ) ; 
  printscaled ( hppp ) ; 
  (void) fprintf( stdout , "%c\n",  ')' ) ; 
  (void) fprintf( stdout , "%s%ld%s",  "vppp = " , (long)vppp , " (" ) ; 
  printscaled ( vppp ) ; 
  (void) fprintf( stdout , "%c\n",  ')' ) ; 
  pixratio = ( designsize / ((double) 1048576L ) ) * ( hppp / ((double) 
  1048576L ) ) ; 
  minmstated = signedquad () ; 
  maxmstated = signedquad () ; 
  minnstated = signedquad () ; 
  maxnstated = signedquad () ; 
  (void) fprintf( stdout , "%s%ld%s%ld\n",  "min m = " , (long)minmstated , ", max m = " , (long)maxmstated ) ; 
  if ( minmstated > minmoverall ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "min m should be <=" , (long)minmoverall ,     '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  if ( maxmstated < maxmoverall ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "max m should be >=" , (long)maxmoverall ,     '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  (void) fprintf( stdout , "%s%ld%s%ld\n",  "min n = " , (long)minnstated , ", max n = " , (long)maxnstated ) ; 
  if ( minnstated > minnoverall ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "min n should be <=" , (long)minnoverall ,     '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  if ( maxnstated < maxnoverall ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "max n should be >=" , (long)maxnoverall ,     '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  do {
      a = curloc ; 
    k = getbyte () ; 
    if ( ( k == 245 ) || ( k == 246 ) ) 
    {
      c = firstpar ( k ) ; 
      if ( k == 245 ) 
      {
	u = signedquad () ; 
	v = signedquad () ; 
      } 
      else {
	  
	u = getbyte () * 65536L ; 
	v = 0 ; 
      } 
      w = signedquad () ; 
      p = signedquad () ; 
      (void) fprintf( stdout , "%s%ld%s%ld%s",  "Character " , (long)c , ": dx " , (long)u , " (" ) ; 
      printscaled ( u ) ; 
      if ( v != 0 ) 
      {
	(void) fprintf( stdout , "%s%ld%s",  "), dy " , (long)v , " (" ) ; 
	printscaled ( v ) ; 
      } 
      (void) fprintf( stdout , "%s%ld%s",  "), width " , (long)w , " (" ) ; 
      w = round ( w * pixratio ) ; 
      printscaled ( w ) ; 
      (void) fprintf( stdout , "%s%ld\n",  "), loc " , (long)p ) ; 
      if ( charptr [ c ] == 0 ) 
      {
	(void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " ,         "duplicate locator for this character!" ) ; 
	(void) putc('\n',  stdout );
      } 
      else if ( p != charptr [ c ] ) 
      {
	(void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "character location should be " ,         (long)charptr [ c ] , '!' ) ; 
	(void) putc('\n',  stdout );
      } 
      charptr [ c ] = 0 ; 
      k = 244 ; 
    } 
  } while ( ! ( k != 244 ) ) ; 
  if ( k != 249 ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " , "should be postpost!" ) ; 
    (void) putc('\n',  stdout );
  } 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    if ( charptr [ k ] > 0 ) 
    {
      (void) fprintf( stdout , "%ld%s%s%s%ld%c",  (long)a , ": " , "! " , "missing locator for character " , (long)k       , '!' ) ; 
      (void) putc('\n',  stdout );
    } 
  while ( k++ < for_end ) ; } 
  q = signedquad () ; 
  if ( q != postloc ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%s%ld%c",  (long)a , ": " , "! " , "postamble pointer should be " ,     (long)postloc , " not " , (long)q , '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  m = getbyte () ; 
  if ( m != 131 ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s%ld%s%ld%c",  (long)a , ": " , "! " , "identification byte should be " , (long)131     , ", not " , (long)m , '!' ) ; 
    (void) putc('\n',  stdout );
  } 
  k = curloc ; 
  m = 223 ; 
  while ( ( m == 223 ) && ! eof ( gffile ) ) m = getbyte () ; 
  if ( ! eof ( gffile ) ) 
  {
    (void) fprintf( stdout , "%s%s%ld%s%c\n",  "Bad GF file: " , "signature in byte " , (long)curloc - 1 ,     " should be 223" , '!' ) ; 
    uexit ( 1 ) ; 
  } 
  else if ( curloc < k + 4 ) 
  {
    (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " ,     "not enough signature bytes at end of file!" ) ; 
    (void) putc('\n',  stdout );
  } 
} 
void main_body() {
    
  initialize () ; 
  opengffile () ; 
  o = getbyte () ; 
  if ( o != 247 ) 
  {
    (void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "First byte isn't start of preamble!"     , '!' ) ; 
    uexit ( 1 ) ; 
  } 
  o = getbyte () ; 
  if ( o != 131 ) 
  {
    (void) fprintf( stdout , "%s%s%ld%s%ld%c\n",  "Bad GF file: " , "identification byte should be " ,     (long)131 , " not " , (long)o , '!' ) ; 
    uexit ( 1 ) ; 
  } 
  o = getbyte () ; 
  (void) putc( '\'' ,  stdout );
  while ( o > 0 ) {
      
    o = o - 1 ; 
    (void) putc( xchr [ getbyte () ] ,  stdout );
  } 
  (void) fprintf( stdout , "%c\n",  '\'' ) ; 
  do {
      gfprevptr = curloc ; 
    do {
	a = curloc ; 
      o = getbyte () ; 
      p = firstpar ( o ) ; 
      if ( eof ( gffile ) ) 
      {
	(void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "the file ended prematurely" ,         '!' ) ; 
	uexit ( 1 ) ; 
      } 
      if ( o == 243 ) 
      {
	{
	  if ( wantsmnemonics ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%ld%s%s%ld%s",  (long)a , ": " , "yyy " , (long)p , " (" ) ; 
	  } 
	  if ( wantsmnemonics ) 
	  {
	    printscaled ( p ) ; 
	    (void) putc( ')' ,  stdout );
	  } 
	} 
	o = 244 ; 
      } 
      else if ( ( o >= 239 ) && ( o <= 242 ) ) 
      {
	{
	  if ( wantsmnemonics ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%ld%s%s",  (long)a , ": " , "xxx '" ) ; 
	  } 
	  badchar = false ; 
	  b = 16 ; 
	  if ( p < 0 ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " , "string of negative length!" ) 
	    ; 
	    (void) putc('\n',  stdout );
	  } 
	  while ( p > 0 ) {
	      
	    q = getbyte () ; 
	    if ( ( q < 32 ) || ( q > 126 ) ) 
	    badchar = true ; 
	    if ( wantsmnemonics ) 
	    {
	      (void) putc( xchr [ q ] ,  stdout );
	      if ( b < linelength ) 
	      b = b + 1 ; 
	      else {
		  
		(void) putc('\n',  stdout );
		b = 2 ; 
	      } 
	    } 
	    p = p - 1 ; 
	  } 
	  if ( wantsmnemonics ) 
	  (void) putc( '\'' ,  stdout );
	  if ( badchar ) 
	  {
	    (void) putc('\n',  stdout );
	    (void) fprintf( stdout , "%ld%s%s%s",  (long)a , ": " , "! " ,             "non-ASCII character in xxx command!" ) ; 
	    (void) putc('\n',  stdout );
	  } 
	} 
	o = 244 ; 
      } 
      else if ( o == 244 ) 
      if ( wantsmnemonics ) 
      {
	(void) putc('\n',  stdout );
	(void) fprintf( stdout , "%ld%s%s",  (long)a , ": " , "no op" ) ; 
      } 
    } while ( ! ( o != 244 ) ) ; 
    if ( o != 248 ) 
    {
      if ( o != 67 ) 
      if ( o != 68 ) 
      {
	(void) fprintf( stdout , "%s%s%ld%s%ld%c%c\n",  "Bad GF file: " , "byte " , (long)curloc - 1 ,         " is not boc (" , (long)o , ')' , '!' ) ; 
	uexit ( 1 ) ; 
      } 
      (void) putc('\n',  stdout );
      (void) fprintf( stdout , "%ld%s",  (long)curloc - 1 , ": beginning of char " ) ; 
      a = curloc - 1 ; 
      totalchars = totalchars + 1 ; 
      if ( o == 67 ) 
      {
	charactercode = signedquad () ; 
	p = signedquad () ; 
	c = charactercode % 256 ; 
	if ( c < 0 ) 
	c = c + 256 ; 
	minmstated = signedquad () ; 
	maxmstated = signedquad () ; 
	minnstated = signedquad () ; 
	maxnstated = signedquad () ; 
      } 
      else {
	  
	charactercode = getbyte () ; 
	p = -1 ; 
	c = charactercode ; 
	q = getbyte () ; 
	maxmstated = getbyte () ; 
	minmstated = maxmstated - q ; 
	q = getbyte () ; 
	maxnstated = getbyte () ; 
	minnstated = maxnstated - q ; 
      } 
      (void) fprintf( stdout , "%ld",  (long)c ) ; 
      if ( charactercode != c ) 
      (void) fprintf( stdout , "%s%ld",  " with extension " , (long)( charactercode - c ) / 256 ) ; 
      if ( wantsmnemonics ) 
      (void) fprintf( stdout , "%s%ld%s%ld%c%ld%s%ld\n",  ": " , (long)minmstated , "<=m<=" , (long)maxmstated , ' ' ,       (long)minnstated , "<=n<=" , (long)maxnstated ) ; 
      maxmobserved = -1 ; 
      if ( charptr [ c ] != p ) 
      {
	(void) fprintf( stdout , "%ld%s%s%s%ld%s%ld%c",  (long)a , ": " , "! " ,         "previous character pointer should be " , (long)charptr [ c ] , ", not " , (long)p         , '!' ) ; 
	(void) putc('\n',  stdout );
      } 
      else if ( p > 0 ) 
      if ( wantsmnemonics ) 
      (void) fprintf( stdout , "%s%ld%c\n",        "(previous character with the same code started at byte " , (long)p , ')' ) ; 
      charptr [ c ] = gfprevptr ; 
      if ( wantsmnemonics ) 
      (void) fprintf( stdout , "%s%ld%c",  "(initially n=" , (long)maxnstated , ')' ) ; 
      if ( wantspixels ) 
      {
	maxsubcol = maxmstated - minmstated - 1 ; 
	if ( maxsubcol > maxcol ) 
	maxsubcol = maxcol ; 
	maxsubrow = maxnstated - minnstated ; 
	if ( maxsubrow > maxrow ) 
	maxsubrow = maxrow ; 
	n = 0 ; 
	while ( n <= maxsubrow ) {
	    
	  m = 0 ; 
	  while ( m <= maxsubcol ) {
	      
	    imagearray [ m ][ n ] = 0 ; 
	    m = m + 1 ; 
	  } 
	  n = n + 1 ; 
	} 
      } 
      m = 0 ; 
      n = 0 ; 
      paintswitch = 0 ; 
      if ( ! dochar () ) 
      {
	(void) fprintf( stdout , "%s%s%c\n",  "Bad GF file: " , "char ended unexpectedly" , '!' ) 
	; 
	uexit ( 1 ) ; 
      } 
      maxnobserved = n ; 
      if ( wantspixels ) 
      {
	if ( ( maxmobserved > maxcol ) || ( maxnobserved > maxrow ) ) 
	(void) fprintf( stdout , "%s\n",          "(The character is too large to be displayed in full.)" ) ; 
	if ( maxsubcol > maxmobserved ) 
	maxsubcol = maxmobserved ; 
	if ( maxsubrow > maxnobserved ) 
	maxsubrow = maxnobserved ; 
	if ( maxsubcol >= 0 ) 
	{
	  (void) fprintf( stdout , "%s%ld%c%ld%s\n",  ".<--This pixel's lower left corner is at (" ,           (long)minmstated , ',' , (long)maxnstated + 1 , ") in METAFONT coordinates" ) ; 
	  n = 0 ; 
	  while ( n <= maxsubrow ) {
	      
	    m = 0 ; 
	    b = 0 ; 
	    while ( m <= maxsubcol ) {
		
	      if ( imagearray [ m ][ n ] == 0 ) 
	      b = b + 1 ; 
	      else {
		  
		while ( b > 0 ) {
		    
		  (void) putc( ' ' ,  stdout );
		  b = b - 1 ; 
		} 
		(void) putc( '*' ,  stdout );
	      } 
	      m = m + 1 ; 
	    } 
	    (void) putc('\n',  stdout );
	    n = n + 1 ; 
	  } 
	  (void) fprintf( stdout , "%s%ld%c%ld%s\n",  ".<--This pixel's upper left corner is at (" ,           (long)minmstated , ',' , (long)maxnstated - maxsubrow ,           ") in METAFONT coordinates" ) ; 
	} 
	else
	(void) fprintf( stdout , "%s\n",  "(The character is entirely blank.)" ) ; 
      } 
      maxmobserved = minmstated + maxmobserved + 1 ; 
      n = maxnstated - maxnobserved ; 
      if ( minmstated < minmoverall ) 
      minmoverall = minmstated ; 
      if ( maxmobserved > maxmoverall ) 
      maxmoverall = maxmobserved ; 
      if ( n < minnoverall ) 
      minnoverall = n ; 
      if ( maxnstated > maxnoverall ) 
      maxnoverall = maxnstated ; 
      if ( maxmobserved > maxmstated ) 
      (void) fprintf( stdout , "%s%ld%c\n",  "The previous character should have had max m >= " ,       (long)maxmobserved , '!' ) ; 
      if ( n < minnstated ) 
      (void) fprintf( stdout , "%s%ld%c\n",  "The previous character should have had min n <= " ,       (long)n , '!' ) ; 
    } 
  } while ( ! ( o == 248 ) ) ; 
  (void) putc('\n',  stdout );
  readpostamble () ; 
  (void) fprintf( stdout , "%s%ld%s",  "The file had " , (long)totalchars , " character" ) ; 
  if ( totalchars != 1 ) 
  (void) putc( 's' ,  stdout );
  (void) fprintf( stdout , "%s\n",  " altogether." ) ; 
} 
