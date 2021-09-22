#ifdef little_endian   /* Eg: Intel */
   #include <dos.h>
   #include <graphics.h>
   #include <io.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef little_endian   /* Eg: Intel */
   #include <alloc.h>
#endif

#include <ctype.h>

#ifdef little_endian   /* Eg: Intel */
   #include <dir.h>
   #include <bios.h>
#endif

#ifdef big_endian
   #include <Types.h>
#endif

#include "Blowfish.h"

#define N               16
#define noErr            0
#define DATAERROR         -1
#define KEYBYTES         8
#define subkeyfilename   "Blowfish.dat"

unsigned long P[N + 2];
unsigned long S[4][256];
FILE*         SubkeyFile;

short opensubkeyfile(void) /* read only */
{
   short error;

   error = noErr;

   if((SubkeyFile = fopen(subkeyfilename,"rb")) == NULL) {
      error = DATAERROR;
   }
 
   return error;
}

unsigned long F(unsigned long x)
{
   unsigned short a;
   unsigned short b;
   unsigned short c;
   unsigned short d;
   unsigned long  y;

   d = x & 0x00FF;
   x >>= 8;
   c = x & 0x00FF;
   x >>= 8;
   b = x & 0x00FF;
   x >>= 8;
   a = x & 0x00FF;
   //y = ((S[0][a] + S[1][b]) ^ S[2][c]) + S[3][d];
   y = S[0][a] + S[1][b];
   y = y ^ S[2][c];
   y = y + S[3][d];

   return y;
}

void Blowfish_encipher(unsigned long *xl, unsigned long *xr)
{
   unsigned long  Xl;
   unsigned long  Xr;
   unsigned long  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = 0; i < N; ++i) {
      Xl = Xl ^ P[i];
      Xr = F(Xl) ^ Xr;

      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ P[N];
   Xl = Xl ^ P[N + 1];
  
   *xl = Xl;
   *xr = Xr;
}

void Blowfish_decipher(unsigned long *xl, unsigned long *xr)
{
   unsigned long  Xl;
   unsigned long  Xr;
   unsigned long  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = N + 1; i > 1; --i) {
      Xl = Xl ^ P[i];
      Xr = F(Xl) ^ Xr;

      /* Exchange Xl and Xr */
      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   /* Exchange Xl and Xr */
   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ P[1];
   Xl = Xl ^ P[0];

   *xl = Xl;
   *xr = Xr;
}

short InitializeBlowfish(char key[], short keybytes)
{
   short          i;
   short          j;
   short          k;
   short          error;
   short          numread;
   unsigned long  data;
   unsigned long  datal;
   unsigned long  datar;

   /* First, open the file containing the array initialization data */
   error = opensubkeyfile();
   if (error == noErr) {
      for (i = 0; i < N + 2; ++i) {
	 numread = fread(&data, 4, 1, SubkeyFile);
   #ifdef little_endian      /* Eg: Intel   We want to process things in byte   */
			   /*   order, not as rearranged in a longword          */
	 data = ((data & 0xFF000000) >> 24) |
		((data & 0x00FF0000) >>  8) |
		((data & 0x0000FF00) <<  8) |
		((data & 0x000000FF) << 24);
   #endif

	 if (numread != 1) {
	    return DATAERROR;
	 } else {
	    P[i] = data;
	 }
      }

      for (i = 0; i < 4; ++i) {
	 for (j = 0; j < 256; ++j) {
	     numread = fread(&data, 4, 1, SubkeyFile);

   #ifdef little_endian      /* Eg: Intel   We want to process things in byte   */
			   /*   order, not as rearranged in a longword          */
	    data = ((data & 0xFF000000) >> 24) |
		   ((data & 0x00FF0000) >>  8) |
		   ((data & 0x0000FF00) <<  8) |
		   ((data & 0x000000FF) << 24);
   #endif

	     if (numread != 1) {
	       return DATAERROR;
	    } else {
	       S[i][j] = data;
	    }
	 }
      }

      fclose(SubkeyFile);

      j = 0;
      for (i = 0; i < N + 2; ++i) {
	 data = 0x00000000;
	 for (k = 0; k < 4; ++k) {
	    data = (data << 8) | key[j];
	    j = j + 1;
	    if (j >= keybytes) {
	       j = 0;
	    }
	 }
	 P[i] = P[i] ^ data;
      }

	datal = 0x00000000;
      datar = 0x00000000;

      for (i = 0; i < N + 2; i += 2) {
	 Blowfish_encipher(&datal, &datar);

	 P[i] = datal;
	 P[i + 1] = datar;
      }

      for (i = 0; i < 4; ++i) {
	 for (j = 0; j < 256; j += 2) {

	    Blowfish_encipher(&datal, &datar);
   
	    S[i][j] = datal;
	    S[i][j + 1] = datar;
	 }
      }
   } else {
      printf("Unable to open subkey initialization file : %d\n", error);
   }

   return error;
}

