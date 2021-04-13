/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef RISCV_STDINT_H
#define RISCV_STDINT_H

/* Exact integral types */
typedef unsigned char u8;
typedef signed char i8;

typedef unsigned short u16;
typedef signed short i16;

typedef unsigned int u32;
typedef signed int i32;

typedef unsigned long long u64;
typedef signed long long i64;

/* Small types */
typedef unsigned char uint_least8_t;
typedef signed char int_least8_t;

typedef unsigned short uint_least16_t;
typedef signed short int_least16_t;

typedef unsigned int uint_least32_t;
typedef signed int int_least32_t;

typedef unsigned long long uint_least64_t;
typedef signed long long int_least64_t;

/* Fast Types */
typedef unsigned char uint_fast8_t;
typedef signed char int_fast8_t;

typedef unsigned int uint_fast16_t;
typedef signed int int_fast16_t;

typedef unsigned int uint_fast32_t;
typedef signed int int_fast32_t;

typedef unsigned long long uint_fast64_t;
typedef signed long long int_fast64_t;

typedef long long int intmax_t;
typedef unsigned long long uintmax_t;

typedef u8 u8;
typedef u16 u16;
typedef u32 u32;
typedef u64 u64;
typedef i8 s8;
typedef i16 s16;
typedef i32 s32;
typedef i64 s64;

typedef u8 bool;
#define true 1
#define false 0

/* Types for `void *' pointers.  */
typedef s64 isize;
typedef u64 usize;

#endif /* RISCV_STDINT_H */
