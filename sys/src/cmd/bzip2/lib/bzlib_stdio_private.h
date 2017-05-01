/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * THIS FILE IS NOT IDENTICAL TO THE ORIGINAL
 * FROM THE BZIP2 DISTRIBUTION.
 *
 * It has been modified, mainly to break the library
 * into smaller pieces.
 *
 * Russ Cox
 * rsc@plan9.bell-labs.com
 * July 2000
 */

extern void BZ2_bz__AssertH__fail ( int errcode );

/* undo definitions in bzlib_private.h */
#undef AssertH
#undef AssertD
#undef VPrintf0
#undef VPrintf1
#undef VPrintf2
#undef VPrintf3
#undef VPrintf4
#undef VPrintf5

#define AssertH(cond,errcode) \
   { if (!(cond)) BZ2_bz__AssertH__fail ( errcode ); }
#if BZ_DEBUG
#define AssertD(cond,msg) \
   { if (!(cond)) {       \
      fprintf ( stderr,   \
        "\n\nlibbzip2(debug build): internal error\n\t%s\n", msg );\
      exit(1); \
   }}
#else
#define AssertD(cond,msg) /* */
#endif
#define VPrintf0(zf) \
   fprintf(stderr,zf)
#define VPrintf1(zf,za1) \
   fprintf(stderr,zf,za1)
#define VPrintf2(zf,za1,za2) \
   fprintf(stderr,zf,za1,za2)
#define VPrintf3(zf,za1,za2,za3) \
   fprintf(stderr,zf,za1,za2,za3)
#define VPrintf4(zf,za1,za2,za3,za4) \
   fprintf(stderr,zf,za1,za2,za3,za4)
#define VPrintf5(zf,za1,za2,za3,za4,za5) \
   fprintf(stderr,zf,za1,za2,za3,za4,za5)

#define BZ_SETERR(eee)                    \
{                                         \
   if (bzerror != NULL) *bzerror = eee;   \
   if (bzf != NULL) bzf->lastErr = eee;   \
}

typedef
   struct {
      FILE*     handle;
      Char      buf[BZ_MAX_UNUSED];
      Int32     bufN;
      Bool      writing;
      bz_stream strm;
      Int32     lastErr;
      Bool      initialisedOk;
   }
   bzFile;

extern Bool bz_feof( FILE* );
