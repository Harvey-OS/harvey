/* Parameters derived from machine and compiler architecture */

	/* ---------------- Scalar alignments ---------------- */

#define arch_align_short_mod 2
#define arch_align_int_mod 4
#define arch_align_long_mod 4
#define arch_align_ptr_mod 4
#define arch_align_float_mod 4
#define arch_align_double_mod 4
#define arch_align_ref_mod 4

	/* ---------------- Scalar sizes ---------------- */

#define arch_log2_sizeof_short 1
#define arch_log2_sizeof_int 2
#define arch_log2_sizeof_long 2
#define arch_sizeof_ds_ptr 4
#define arch_sizeof_ptr 4
#define arch_sizeof_float 4
#define arch_sizeof_double 8
#define arch_sizeof_ref 8

	/* ---------------- Unsigned max values ---------------- */

#define arch_max_uchar ((unsigned char)0xff + (unsigned char)0)
#define arch_max_ushort ((unsigned short)0xffff + (unsigned short)0)
#define arch_max_uint ((unsigned int)0xffffffff + (unsigned int)0)
#define arch_max_ulong ((unsigned long)0xffffffffL + (unsigned long)0)

	/* ---------------- Miscellaneous ---------------- */

#define arch_is_big_endian 0
#define arch_ptrs_are_signed 0
#define arch_floats_are_IEEE 1
#define arch_arith_rshift 2
#define arch_can_shift_full_long 0
