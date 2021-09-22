/* atob: version 4.0
 * stream filter to change printable ascii from "btoa" back into 8 bit bytes
 * if bad chars, or Csums do not match: exit(1) [and NO output]
 *
 *  Paul Rutter		Joe Orost
 *  philabs!per		petsd!joe
 */

#define _PLAN9_SOURCE
#define _BSD_EXTENSION
#define _POSIX_SOURCE

#include <u.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define streq(s0, s1)  strcmp(s0, s1) == 0

#define times85(x)	((((((x<<2)+x)<<2)+x)<<2)+x)

long int Ceor = 0;
long int Csum = 0;
long int Crot = 0;
long int word = 0;
long int bcount = 0;

void	byteout(int);

void
fatal(char *msg)
{
  fprintf(stderr, "atob: %s\n", msg);
  exit(1);
}

#define DE(c) ((c) - '!')

void
decode(int c) 
{
  if (c == 'z') {
    if (bcount != 0) {
      fatal("non-zero bcount");
    } else {
      byteout(0);
      byteout(0);
      byteout(0);
      byteout(0);
    }
  } else if ((c >= '!') && (c < ('!' + 85))) {
    if (bcount == 0) {
      word = DE(c);
      ++bcount;
    } else if (bcount < 4) {
      word = times85(word);
      word += DE(c);
      ++bcount;
    } else {
      word = times85(word) + DE(c);
      byteout((int)((word >> 24) & 255));
      byteout((int)((word >> 16) & 255));
      byteout((int)((word >> 8) & 255));
      byteout((int)(word & 255));
      word = 0;
      bcount = 0;
    }
  } else {
    fatal("char out of range perhaps?");
  }
}

FILE *tmp_file;

void
byteout(int c) 
{
  Ceor ^= c;
  Csum += c;
  Csum += 1;
  if ((Crot & 0x80000000)) {
    Crot <<= 1;
    Crot += 1;
  } else {
    Crot <<= 1;
  }
  Crot += c;
  putc(c, tmp_file);
}

main(argc, argv) 
  char **argv;
{
  int c;
  char tmp_name[100];
  char buf[100];
  long i, n1, n2, oeor, osum, orot;

  if (argc != 1) {
    fprintf(stderr,"bad args to %s\n", argv[0]);
    exit(1);
  }
  sprintf(tmp_name, "/tmp/atob.%x", getpid());
  tmp_file = fopen(tmp_name, "w+");
  if (tmp_file == NULL) {
    fatal("cannot make temporary");
  }
  unlink(tmp_name);	/* Make file disappear */
  /*search for header line*/
  for (;;) {
    if (fgets(buf, sizeof buf, stdin) == NULL) {
      fatal("premature eof");
    }
    if (streq(buf, "xbtoa Begin\n")) {
      break;
    }
  }

  while ((c = getchar()) != EOF) {
    if (c == '\n') {
      continue;
    } else if (c == 'x') {
      break;
    } else {
      decode(c);
    }
  }
  if(scanf("btoa End N %ld %lx E %lx S %lx R %lx\n",
         &n1, &n2, &oeor, &osum, &orot) != 5) {
    fatal("checksum line ill-formatted");
  }
  if ((n1 != n2) || (oeor != Ceor) || (osum != Csum) || (orot != Crot)) {
    fatal("checksums wrong");
  } else {
    /*copy OK tmp file to stdout*/;
    fseek(tmp_file, 0L, 0);
    for (i = n1; --i >= 0;) {
      putchar(getc(tmp_file));
    }
  }
  exit(0);
  return 0;
}
