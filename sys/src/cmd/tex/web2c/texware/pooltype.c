#include "config.h"
typedef unsigned char ASCIIcode  ; 
ASCIIcode xord[256]  ; 
ASCIIcode xchr[256]  ; 
unsigned char k, l  ; 
ASCIIcode m, n  ; 
integer s  ; 
integer count  ; 
file_ptr /* of  ASCIIcode */ poolfile  ; 
char poolname[PATHMAX + 1]  ; 
boolean xsum  ; 

#include "pooltype.h"
void initialize ( ) 
{integer i  ; 
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
  {register integer for_end; i = 0 ; for_end = 31 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 127 ; for_end = 255 ; if ( i <= for_end) do 
    xchr [ i ] = chr ( i ) ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ chr ( i ) ] = 127 ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 128 ; for_end = 255 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  {register integer for_end; i = 0 ; for_end = 126 ; if ( i <= for_end) do 
    xord [ xchr [ i ] ] = i ; 
  while ( i++ < for_end ) ; } 
  count = 0 ; 
} 
void main_body() {
    
  initialize () ; 
  {register integer for_end; k = 0 ; for_end = 255 ; if ( k <= for_end) do 
    {
      (void) fprintf(stdout, "%ld%s", (long)k , ": \"" ) ; 
      l = k ; 
      if ( ( ( k < 32 ) || ( k > 126 ) ) ) 
      {
	(void) fprintf(stdout, "%c%c", xchr [ 94 ] , xchr [ 94 ] ) ; 
	if ( k < 64 ) 
	l = k + 64 ; 
	else if ( k < 128 ) 
	l = k - 64 ; 
	else {
	    
	  l = k / 16 ; 
	  if ( l < 10 ) 
	  l = l + 48 ; 
	  else l = l + 87 ; 
	  (void) putc(xchr [ l ] , stdout);
	  l = k % 16 ; 
	  if ( l < 10 ) 
	  l = l + 48 ; 
	  else l = l + 87 ; 
	  count = count + 1 ; 
	} 
	count = count + 2 ; 
      } 
      if ( l == 34 ) 
      (void) fprintf(stdout, "%c%c", xchr [ l ] , xchr [ l ] ) ; 
      else
      (void) putc(xchr [ l ] , stdout);
      count = count + 1 ; 
      (void) fprintf(stdout, "%c\n", '"' ) ; 
    } 
  while ( k++ < for_end ) ; } 
  s = 256 ; 
  if ( argc != 2 ) 
  {
    (void) fprintf(stdout, "%s\n", "Usage: pooltype <pool file>." ) ; 
    uexit ( 1 ) ; 
  } 
  argv ( 1 , poolname ) ; 
  reset ( poolfile , poolname ) ; 
  xsum = false ; 
  if ( eof ( poolfile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "! I can't read the POOL file." ) ; 
    uexit ( 1 ) ; 
  } 
  do {
      if ( eof ( poolfile ) ) 
    {
      (void) fprintf(stdout, "%s\n", "! POOL file contained no check sum" ) ; 
      uexit ( 1 ) ; 
    } 
    read ( poolfile , m ) ; 
    read ( poolfile , n ) ; 
    if ( m != '*' ) 
    {
      if ( ( xord [ m ] < 48 ) || ( xord [ m ] > 57 ) || ( xord [ n ] < 48 ) 
      || ( xord [ n ] > 57 ) ) 
      {
	(void) fprintf(stdout, "%s\n", "! POOL line doesn't begin with two digits" ) ; 
	uexit ( 1 ) ; 
      } 
      l = xord [ m ] * 10 + xord [ n ] - 48 * 11 ; 
      (void) fprintf(stdout, "%ld%s", (long)s , ": \"" ) ; 
      count = count + l ; 
      {register integer for_end; k = 1 ; for_end = l ; if ( k <= for_end) do 
	{
	  if ( eoln ( poolfile ) ) 
	  {
	    (void) fprintf(stdout, "%c\n", '"' ) ; 
	    {
	      (void) fprintf(stdout, "%s\n", "! That POOL line was too short" ) ; 
	      uexit ( 1 ) ; 
	    } 
	  } 
	  read ( poolfile , m ) ; 
	  (void) putc(xchr [ xord [ m ] ] , stdout);
	  if ( xord [ m ] == 34 ) 
	  (void) putc(xchr [ 34 ] , stdout);
	} 
      while ( k++ < for_end ) ; } 
      (void) fprintf(stdout, "%c\n", '"' ) ; 
      s = s + 1 ; 
    } 
    else xsum = true ; 
    readln ( poolfile ) ; 
  } while ( ! ( xsum ) ) ; 
  if ( ! eof ( poolfile ) ) 
  {
    (void) fprintf(stdout, "%s\n", "! There's junk after the check sum" ) ; 
    uexit ( 1 ) ; 
  } 
  (void) fprintf(stdout, "%c%ld%s\n", '(' , (long)count , " characters in all.)" ) ; 
  uexit ( 0 ) ; 
} 
