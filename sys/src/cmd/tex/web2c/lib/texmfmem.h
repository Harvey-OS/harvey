/* texmfmem.h: the memory_word type, which is too hard to translate
   automatically from Pascal.  We have to make sure the byte-swapping
   that the (un)dumping routines do suffices to put things in the right
   place in memory.

   A memory_word can be broken up into a `twohalves' or a
   `fourquarters', and a `twohalves' can be further broken up.  Here is
   a picture.  ..._M = most significant byte, ..._L = least significant
   byte.
   
   If BigEndian:
   twohalves.v:  RH_M  RH_L  LH_M  LH_L
   twohalves.u:  JNK1  JNK2    B0    B1
   fourquarters:   B0    B1    B2    B3
   
   If LittleEndian:
   twohalves.v:  LH_L  LH_M  RH_L  RH_M
   twohalves.u:    B1    B0  JNK1  JNK2
   fourquarters:   B3    B2    B1    B0
   
   The halfword fields are four bytes if we are building a TeX or MF;
   this leads to further complications:
   
   BigEndian:
   twohalves.v:  RH_MM RH_ML RH_LM RH_LL LH_MM LH_ML LH_LM LH_LL
   twohalves.u:  ---------JUNK----------  B0    B1
   fourquarters:   B0    B1    B2    B3

   LittleEndian:
   twohalves.v:  LH_LL LH_LM LH_ML LH_MM RH_LL RH_LM RH_ML RH_MM
   twohalves.u:  junkx junky  B1    B0
   fourquarters: ---------JUNK----------  B3    B2    B1    B0

   I guess TeX and Metafont never refer to the B1 and B0 in the
   fourquarters structure as the B1 and B0 in the twohalves.u structure.
   
   This file can't be part of texmf.h, because texmf.h gets included by
   {tex,mf}d.h before the `halfword' etc. types are defined.  So we
   include it from the change file instead.
*/

typedef union
{
  struct
  {
#ifdef WORDS_BIGENDIAN
    halfword RH, LH;
#else
    halfword LH, RH;
#endif
  } v;

  struct
  { /* Make B0,B1 overlap the most significant bytes of LH.  */
#ifdef WORDS_BIGENDIAN
    halfword junk;
    quarterword B0, B1;
#else /* not WORDS_BIGENDIAN */
  /* If 32-bit TeX/MF, have to have an extra two bytes of junk.  
     I would like to break this line, but I'm afraid that some
     preprocessors don't properly handle backslash-newline in # commands.  */
#if (defined (TeX) && !defined (SMALLTeX)) || !defined (TeX) && !defined (SMALLMF)
    quarterword junkx, junky;
#endif /* big TeX or big MF */
    quarterword B1, B0;
#endif /* not WORDS_BIGENDIAN */
  } u;
} twohalves;

typedef struct
{
  struct
  {
#ifdef WORDS_BIGENDIAN
    quarterword B0, B1, B2, B3;
#else
    quarterword B3, B2, B1, B0;
#endif
  } u;
} fourquarters;


typedef union
{
#ifdef TeX
  glueratio gr;
  twohalves hh;
#else
  twohalves hhfield;
#endif
#ifdef WORDS_BIGENDIAN
  integer cint;
  fourquarters qqqq;
#else /* not WORDS_BIGENDIAN */
  struct
  {
#if defined (TeX) && !defined (SMALLTeX) || !defined (TeX) && !defined (SMALLMF)
    halfword junk;
#endif /* big TeX or big MF */
    integer CINT;
  } u;

  struct
  {
#if defined (TeX) && !defined (SMALLTeX) || !defined (TeX) && !defined (SMALLMF)
    halfword junk;
#endif /* big TeX or big MF */
    fourquarters QQQQ;
  } v;
#endif /* not WORDS_BIGENDIAN */
} memoryword;


/* To keep the original structure accesses working, we must go through
   the extra names C forced us to introduce.  */
#define	b0 u.B0
#define	b1 u.B1
#define	b2 u.B2
#define	b3 u.B3

#define rh v.RH
#define lhfield	v.LH

#ifndef WORDS_BIGENDIAN
#define cint u.CINT
#define qqqq v.QQQQ
#endif
