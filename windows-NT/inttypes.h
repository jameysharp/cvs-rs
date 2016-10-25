/* Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Derek Price & Paul Eggert.
   This file is part of gnulib.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef INTTYPES_H
#define INTTYPES_H

/*
 * A wrapper for the ISO C 99 <inttypes.h>.
 * <http://www.opengroup.org/susv3xbd/inttypes.h.html>
 *
 * Currently, if the system <inttypes.h> is missing or not C99 compliant, then
 * this header may only provide the required <stdint.h> (which may be the
 * *almost* C99 compliant one from GNULIB) and prototypes for the strtoimax and
 * strtoumax functions.
 */

#include <stdint.h>
/* Get CHAR_BIT.  */
#include <limits.h>

#if !(INT_MIN == INT32_MIN && INT_MAX == INT32_MAX)
# error "This file assumes that 'int' has exactly 32 bits. Please report your platform and compiler to <bug-cvs@nongnu.org>."
#endif

/* 7.8.1 Macros for format specifiers */

#if ! defined __cplusplus || defined __STDC_FORMAT_MACROS

# if !defined PRId8
#  undef PRId8
#  ifdef INT8_MAX
#   define PRId8 "d"
#  endif
# endif
# if !defined PRIi8
#  undef PRIi8
#  ifdef INT8_MAX
#   define PRIi8 "i"
#  endif
# endif
# if !defined PRIo8
#  undef PRIo8
#  ifdef UINT8_MAX
#   define PRIo8 "o"
#  endif
# endif
# if !defined PRIu8
#  undef PRIu8
#  ifdef UINT8_MAX
#   define PRIu8 "u"
#  endif
# endif
# if !defined PRIx8
#  undef PRIx8
#  ifdef UINT8_MAX
#   define PRIx8 "x"
#  endif
# endif
# if !defined PRIX8
#  undef PRIX8
#  ifdef UINT8_MAX
#   define PRIX8 "X"
#  endif
# endif
# if !defined PRId16
#  undef PRId16
#  ifdef INT16_MAX
#   define PRId16 "d"
#  endif
# endif
# if !defined PRIi16
#  undef PRIi16
#  ifdef INT16_MAX
#   define PRIi16 "i"
#  endif
# endif
# if !defined PRIo16
#  undef PRIo16
#  ifdef UINT16_MAX
#   define PRIo16 "o"
#  endif
# endif
# if !defined PRIu16
#  undef PRIu16
#  ifdef UINT16_MAX
#   define PRIu16 "u"
#  endif
# endif
# if !defined PRIx16
#  undef PRIx16
#  ifdef UINT16_MAX
#   define PRIx16 "x"
#  endif
# endif
# if !defined PRIX16
#  undef PRIX16
#  ifdef UINT16_MAX
#   define PRIX16 "X"
#  endif
# endif
# if !defined PRId32
#  undef PRId32
#  ifdef INT32_MAX
#   define PRId32 "d"
#  endif
# endif
# if !defined PRIi32
#  undef PRIi32
#  ifdef INT32_MAX
#   define PRIi32 "i"
#  endif
# endif
# if !defined PRIo32
#  undef PRIo32
#  ifdef UINT32_MAX
#   define PRIo32 "o"
#  endif
# endif
# if !defined PRIu32
#  undef PRIu32
#  ifdef UINT32_MAX
#   define PRIu32 "u"
#  endif
# endif
# if !defined PRIx32
#  undef PRIx32
#  ifdef UINT32_MAX
#   define PRIx32 "x"
#  endif
# endif
# if !defined PRIX32
#  undef PRIX32
#  ifdef UINT32_MAX
#   define PRIX32 "X"
#  endif
# endif
# ifdef INT64_MAX
#  if INT64_MAX == LONG_MAX
#   define _PRI64_PREFIX "l"
#  elif defined _MSC_VER || defined __MINGW32__
#   define _PRI64_PREFIX "I64"
#  elif 0 && LONG_MAX >> 30 == 1
#   define _PRI64_PREFIX "ll"
#  endif
#  if !defined PRId64
#   undef PRId64
#   define PRId64 _PRI64_PREFIX "d"
#  endif
#  if !defined PRIi64
#   undef PRIi64
#   define PRIi64 _PRI64_PREFIX "i"
#  endif
# endif
# ifdef UINT64_MAX
#  if INT64_MAX == LONG_MAX
#   define _PRIu64_PREFIX "l"
#  elif defined _MSC_VER || defined __MINGW32__
#   define _PRIu64_PREFIX "I64"
#  elif 0 && LONG_MAX >> 30 == 1
#   define _PRIu64_PREFIX "ll"
#  endif
#  if !defined PRIo64
#   undef PRIo64
#   define PRIo64 _PRIu64_PREFIX "o"
#  endif
#  if !defined PRIu64
#   undef PRIu64
#   define PRIu64 _PRIu64_PREFIX "u"
#  endif
#  if !defined PRIx64
#   undef PRIx64
#   define PRIx64 _PRIu64_PREFIX "x"
#  endif
#  if !defined PRIX64
#   undef PRIX64
#   define PRIX64 _PRIu64_PREFIX "X"
#  endif
# endif

# if !defined PRIdLEAST8
#  undef PRIdLEAST8
#  define PRIdLEAST8 "d"
# endif
# if !defined PRIiLEAST8
#  undef PRIiLEAST8
#  define PRIiLEAST8 "i"
# endif
# if !defined PRIoLEAST8
#  undef PRIoLEAST8
#  define PRIoLEAST8 "o"
# endif
# if !defined PRIuLEAST8
#  undef PRIuLEAST8
#  define PRIuLEAST8 "u"
# endif
# if !defined PRIxLEAST8
#  undef PRIxLEAST8
#  define PRIxLEAST8 "x"
# endif
# if !defined PRIXLEAST8
#  undef PRIXLEAST8
#  define PRIXLEAST8 "X"
# endif
# if !defined PRIdLEAST16
#  undef PRIdLEAST16
#  define PRIdLEAST16 "d"
# endif
# if !defined PRIiLEAST16
#  undef PRIiLEAST16
#  define PRIiLEAST16 "i"
# endif
# if !defined PRIoLEAST16
#  undef PRIoLEAST16
#  define PRIoLEAST16 "o"
# endif
# if !defined PRIuLEAST16
#  undef PRIuLEAST16
#  define PRIuLEAST16 "u"
# endif
# if !defined PRIxLEAST16
#  undef PRIxLEAST16
#  define PRIxLEAST16 "x"
# endif
# if !defined PRIXLEAST16
#  undef PRIXLEAST16
#  define PRIXLEAST16 "X"
# endif
# if !defined PRIdLEAST32
#  undef PRIdLEAST32
#  define PRIdLEAST32 "d"
# endif
# if !defined PRIiLEAST32
#  undef PRIiLEAST32
#  define PRIiLEAST32 "i"
# endif
# if !defined PRIoLEAST32
#  undef PRIoLEAST32
#  define PRIoLEAST32 "o"
# endif
# if !defined PRIuLEAST32
#  undef PRIuLEAST32
#  define PRIuLEAST32 "u"
# endif
# if !defined PRIxLEAST32
#  undef PRIxLEAST32
#  define PRIxLEAST32 "x"
# endif
# if !defined PRIXLEAST32
#  undef PRIXLEAST32
#  define PRIXLEAST32 "X"
# endif
# ifdef INT64_MAX
#  if !defined PRIdLEAST64
#   undef PRIdLEAST64
#   define PRIdLEAST64 PRId64
#  endif
#  if !defined PRIiLEAST64
#   undef PRIiLEAST64
#   define PRIiLEAST64 PRIi64
#  endif
# endif
# ifdef UINT64_MAX
#  if !defined PRIoLEAST64
#   undef PRIoLEAST64
#   define PRIoLEAST64 PRIo64
#  endif
#  if !defined PRIuLEAST64
#   undef PRIuLEAST64
#   define PRIuLEAST64 PRIu64
#  endif
#  if !defined PRIxLEAST64
#   undef PRIxLEAST64
#   define PRIxLEAST64 PRIx64
#  endif
#  if !defined PRIXLEAST64
#   undef PRIXLEAST64
#   define PRIXLEAST64 PRIX64
#  endif
# endif

# if !defined PRIdFAST8
#  undef PRIdFAST8
#  if INT_FAST8_MAX > INT32_MAX
#   define PRIdFAST8 PRId64
#  else
#   define PRIdFAST8 "d"
#  endif
# endif
# if !defined PRIiFAST8
#  undef PRIiFAST8
#  if INT_FAST8_MAX > INT32_MAX
#   define PRIiFAST8 PRIi64
#  else
#   define PRIiFAST8 "i"
#  endif
# endif
# if !defined PRIoFAST8
#  undef PRIoFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define PRIoFAST8 PRIo64
#  else
#   define PRIoFAST8 "o"
#  endif
# endif
# if !defined PRIuFAST8
#  undef PRIuFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define PRIuFAST8 PRIu64
#  else
#   define PRIuFAST8 "u"
#  endif
# endif
# if !defined PRIxFAST8
#  undef PRIxFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define PRIxFAST8 PRIx64
#  else
#   define PRIxFAST8 "x"
#  endif
# endif
# if !defined PRIXFAST8
#  undef PRIXFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define PRIXFAST8 PRIX64
#  else
#   define PRIXFAST8 "X"
#  endif
# endif
# if !defined PRIdFAST16
#  undef PRIdFAST16
#  if INT_FAST16_MAX > INT32_MAX
#   define PRIdFAST16 PRId64
#  else
#   define PRIdFAST16 "d"
#  endif
# endif
# if !defined PRIiFAST16
#  undef PRIiFAST16
#  if INT_FAST16_MAX > INT32_MAX
#   define PRIiFAST16 PRIi64
#  else
#   define PRIiFAST16 "i"
#  endif
# endif
# if !defined PRIoFAST16
#  undef PRIoFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define PRIoFAST16 PRIo64
#  else
#   define PRIoFAST16 "o"
#  endif
# endif
# if !defined PRIuFAST16
#  undef PRIuFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define PRIuFAST16 PRIu64
#  else
#   define PRIuFAST16 "u"
#  endif
# endif
# if !defined PRIxFAST16
#  undef PRIxFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define PRIxFAST16 PRIx64
#  else
#   define PRIxFAST16 "x"
#  endif
# endif
# if !defined PRIXFAST16
#  undef PRIXFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define PRIXFAST16 PRIX64
#  else
#   define PRIXFAST16 "X"
#  endif
# endif
# if !defined PRIdFAST32
#  undef PRIdFAST32
#  if INT_FAST32_MAX > INT32_MAX
#   define PRIdFAST32 PRId64
#  else
#   define PRIdFAST32 "d"
#  endif
# endif
# if !defined PRIiFAST32
#  undef PRIiFAST32
#  if INT_FAST32_MAX > INT32_MAX
#   define PRIiFAST32 PRIi64
#  else
#   define PRIiFAST32 "i"
#  endif
# endif
# if !defined PRIoFAST32
#  undef PRIoFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define PRIoFAST32 PRIo64
#  else
#   define PRIoFAST32 "o"
#  endif
# endif
# if !defined PRIuFAST32
#  undef PRIuFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define PRIuFAST32 PRIu64
#  else
#   define PRIuFAST32 "u"
#  endif
# endif
# if !defined PRIxFAST32
#  undef PRIxFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define PRIxFAST32 PRIx64
#  else
#   define PRIxFAST32 "x"
#  endif
# endif
# if !defined PRIXFAST32
#  undef PRIXFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define PRIXFAST32 PRIX64
#  else
#   define PRIXFAST32 "X"
#  endif
# endif
# ifdef INT64_MAX
#  if !defined PRIdFAST64
#   undef PRIdFAST64
#   define PRIdFAST64 PRId64
#  endif
#  if !defined PRIiFAST64
#   undef PRIiFAST64
#   define PRIiFAST64 PRIi64
#  endif
# endif
# ifdef UINT64_MAX
#  if !defined PRIoFAST64
#   undef PRIoFAST64
#   define PRIoFAST64 PRIo64
#  endif
#  if !defined PRIuFAST64
#   undef PRIuFAST64
#   define PRIuFAST64 PRIu64
#  endif
#  if !defined PRIxFAST64
#   undef PRIxFAST64
#   define PRIxFAST64 PRIx64
#  endif
#  if !defined PRIXFAST64
#   undef PRIXFAST64
#   define PRIXFAST64 PRIX64
#  endif
# endif

# if !defined PRIdMAX
#  undef PRIdMAX
#  if INTMAX_MAX > INT32_MAX
#   define PRIdMAX PRId64
#  else
#   define PRIdMAX "ld"
#  endif
# endif
# if !defined PRIiMAX
#  undef PRIiMAX
#  if INTMAX_MAX > INT32_MAX
#   define PRIiMAX PRIi64
#  else
#   define PRIiMAX "li"
#  endif
# endif
# if !defined PRIoMAX
#  undef PRIoMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define PRIoMAX PRIo64
#  else
#   define PRIoMAX "lo"
#  endif
# endif
# if !defined PRIuMAX
#  undef PRIuMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define PRIuMAX PRIu64
#  else
#   define PRIuMAX "lu"
#  endif
# endif
# if !defined PRIxMAX
#  undef PRIxMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define PRIxMAX PRIx64
#  else
#   define PRIxMAX "lx"
#  endif
# endif
# if !defined PRIXMAX
#  undef PRIXMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define PRIXMAX PRIX64
#  else
#   define PRIXMAX "lX"
#  endif
# endif

# if !defined PRIdPTR
#  undef PRIdPTR
#  ifdef INTPTR_MAX
#   define PRIdPTR "l" "d"
#  endif
# endif
# if !defined PRIiPTR
#  undef PRIiPTR
#  ifdef INTPTR_MAX
#   define PRIiPTR "l" "i"
#  endif
# endif
# if !defined PRIoPTR
#  undef PRIoPTR
#  ifdef UINTPTR_MAX
#   define PRIoPTR "l" "o"
#  endif
# endif
# if !defined PRIuPTR
#  undef PRIuPTR
#  ifdef UINTPTR_MAX
#   define PRIuPTR "l" "u"
#  endif
# endif
# if !defined PRIxPTR
#  undef PRIxPTR
#  ifdef UINTPTR_MAX
#   define PRIxPTR "l" "x"
#  endif
# endif
# if !defined PRIXPTR
#  undef PRIXPTR
#  ifdef UINTPTR_MAX
#   define PRIXPTR "l" "X"
#  endif
# endif

# if !defined SCNd8
#  undef SCNd8
#  ifdef INT8_MAX
#   define SCNd8 "hhd"
#  endif
# endif
# if !defined SCNi8
#  undef SCNi8
#  ifdef INT8_MAX
#   define SCNi8 "hhi"
#  endif
# endif
# if !defined SCNo8
#  undef SCNo8
#  ifdef UINT8_MAX
#   define SCNo8 "hho"
#  endif
# endif
# if !defined SCNu8
#  undef SCNu8
#  ifdef UINT8_MAX
#   define SCNu8 "hhu"
#  endif
# endif
# if !defined SCNx8
#  undef SCNx8
#  ifdef UINT8_MAX
#   define SCNx8 "hhx"
#  endif
# endif
# if !defined SCNd16
#  undef SCNd16
#  ifdef INT16_MAX
#   define SCNd16 "hd"
#  endif
# endif
# if !defined SCNi16
#  undef SCNi16
#  ifdef INT16_MAX
#   define SCNi16 "hi"
#  endif
# endif
# if !defined SCNo16
#  undef SCNo16
#  ifdef UINT16_MAX
#   define SCNo16 "ho"
#  endif
# endif
# if !defined SCNu16
#  undef SCNu16
#  ifdef UINT16_MAX
#   define SCNu16 "hu"
#  endif
# endif
# if !defined SCNx16
#  undef SCNx16
#  ifdef UINT16_MAX
#   define SCNx16 "hx"
#  endif
# endif
# if !defined SCNd32
#  undef SCNd32
#  ifdef INT32_MAX
#   define SCNd32 "d"
#  endif
# endif
# if !defined SCNi32
#  undef SCNi32
#  ifdef INT32_MAX
#   define SCNi32 "i"
#  endif
# endif
# if !defined SCNo32
#  undef SCNo32
#  ifdef UINT32_MAX
#   define SCNo32 "o"
#  endif
# endif
# if !defined SCNu32
#  undef SCNu32
#  ifdef UINT32_MAX
#   define SCNu32 "u"
#  endif
# endif
# if !defined SCNx32
#  undef SCNx32
#  ifdef UINT32_MAX
#   define SCNx32 "x"
#  endif
# endif
# ifdef INT64_MAX
#  if INT64_MAX == LONG_MAX
#   define _SCN64_PREFIX "l"
#  elif defined _MSC_VER || defined __MINGW32__
#   define _SCN64_PREFIX "I64"
#  elif 0 && LONG_MAX >> 30 == 1
#   define _SCN64_PREFIX "ll"
#  endif
#  if !defined SCNd64
#   undef SCNd64
#   define SCNd64 _SCN64_PREFIX "d"
#  endif
#  if !defined SCNi64
#   undef SCNi64
#   define SCNi64 _SCN64_PREFIX "i"
#  endif
# endif
# ifdef UINT64_MAX
#  if INT64_MAX == LONG_MAX
#   define _SCNu64_PREFIX "l"
#  elif defined _MSC_VER || defined __MINGW32__
#   define _SCNu64_PREFIX "I64"
#  elif 0 && LONG_MAX >> 30 == 1
#   define _SCNu64_PREFIX "ll"
#  endif
#  if !defined SCNo64
#   undef SCNo64
#   define SCNo64 _SCNu64_PREFIX "o"
#  endif
#  if !defined SCNu64
#   undef SCNu64
#   define SCNu64 _SCNu64_PREFIX "u"
#  endif
#  if !defined SCNx64
#   undef SCNx64
#   define SCNx64 _SCNu64_PREFIX "x"
#  endif
# endif

# if !defined SCNdLEAST8
#  undef SCNdLEAST8
#  define SCNdLEAST8 "hhd"
# endif
# if !defined SCNiLEAST8
#  undef SCNiLEAST8
#  define SCNiLEAST8 "hhi"
# endif
# if !defined SCNoLEAST8
#  undef SCNoLEAST8
#  define SCNoLEAST8 "hho"
# endif
# if !defined SCNuLEAST8
#  undef SCNuLEAST8
#  define SCNuLEAST8 "hhu"
# endif
# if !defined SCNxLEAST8
#  undef SCNxLEAST8
#  define SCNxLEAST8 "hhx"
# endif
# if !defined SCNdLEAST16
#  undef SCNdLEAST16
#  define SCNdLEAST16 "hd"
# endif
# if !defined SCNiLEAST16
#  undef SCNiLEAST16
#  define SCNiLEAST16 "hi"
# endif
# if !defined SCNoLEAST16
#  undef SCNoLEAST16
#  define SCNoLEAST16 "ho"
# endif
# if !defined SCNuLEAST16
#  undef SCNuLEAST16
#  define SCNuLEAST16 "hu"
# endif
# if !defined SCNxLEAST16
#  undef SCNxLEAST16
#  define SCNxLEAST16 "hx"
# endif
# if !defined SCNdLEAST32
#  undef SCNdLEAST32
#  define SCNdLEAST32 "d"
# endif
# if !defined SCNiLEAST32
#  undef SCNiLEAST32
#  define SCNiLEAST32 "i"
# endif
# if !defined SCNoLEAST32
#  undef SCNoLEAST32
#  define SCNoLEAST32 "o"
# endif
# if !defined SCNuLEAST32
#  undef SCNuLEAST32
#  define SCNuLEAST32 "u"
# endif
# if !defined SCNxLEAST32
#  undef SCNxLEAST32
#  define SCNxLEAST32 "x"
# endif
# ifdef INT64_MAX
#  if !defined SCNdLEAST64
#   undef SCNdLEAST64
#   define SCNdLEAST64 SCNd64
#  endif
#  if !defined SCNiLEAST64
#   undef SCNiLEAST64
#   define SCNiLEAST64 SCNi64
#  endif
# endif
# ifdef UINT64_MAX
#  if !defined SCNoLEAST64
#   undef SCNoLEAST64
#   define SCNoLEAST64 SCNo64
#  endif
#  if !defined SCNuLEAST64
#   undef SCNuLEAST64
#   define SCNuLEAST64 SCNu64
#  endif
#  if !defined SCNxLEAST64
#   undef SCNxLEAST64
#   define SCNxLEAST64 SCNx64
#  endif
# endif

# if !defined SCNdFAST8
#  undef SCNdFAST8
#  if INT_FAST8_MAX > INT32_MAX
#   define SCNdFAST8 SCNd64
#  elif INT_FAST8_MAX == 0x7fff
#   define SCNdFAST8 "hd"
#  elif INT_FAST8_MAX == 0x7f
#   define SCNdFAST8 "hhd"
#  else
#   define SCNdFAST8 "d"
#  endif
# endif
# if !defined SCNiFAST8
#  undef SCNiFAST8
#  if INT_FAST8_MAX > INT32_MAX
#   define SCNiFAST8 SCNi64
#  elif INT_FAST8_MAX == 0x7fff
#   define SCNiFAST8 "hi"
#  elif INT_FAST8_MAX == 0x7f
#   define SCNiFAST8 "hhi"
#  else
#   define SCNiFAST8 "i"
#  endif
# endif
# if !defined SCNoFAST8
#  undef SCNoFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define SCNoFAST8 SCNo64
#  elif UINT_FAST8_MAX == 0xffff
#   define SCNoFAST8 "ho"
#  elif UINT_FAST8_MAX == 0xff
#   define SCNoFAST8 "hho"
#  else
#   define SCNoFAST8 "o"
#  endif
# endif
# if !defined SCNuFAST8
#  undef SCNuFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define SCNuFAST8 SCNu64
#  elif UINT_FAST8_MAX == 0xffff
#   define SCNuFAST8 "hu"
#  elif UINT_FAST8_MAX == 0xff
#   define SCNuFAST8 "hhu"
#  else
#   define SCNuFAST8 "u"
#  endif
# endif
# if !defined SCNxFAST8
#  undef SCNxFAST8
#  if UINT_FAST8_MAX > UINT32_MAX
#   define SCNxFAST8 SCNx64
#  elif UINT_FAST8_MAX == 0xffff
#   define SCNxFAST8 "hx"
#  elif UINT_FAST8_MAX == 0xff
#   define SCNxFAST8 "hhx"
#  else
#   define SCNxFAST8 "x"
#  endif
# endif
# if !defined SCNdFAST16
#  undef SCNdFAST16
#  if INT_FAST16_MAX > INT32_MAX
#   define SCNdFAST16 SCNd64
#  elif INT_FAST16_MAX == 0x7fff
#   define SCNdFAST16 "hd"
#  else
#   define SCNdFAST16 "d"
#  endif
# endif
# if !defined SCNiFAST16
#  undef SCNiFAST16
#  if INT_FAST16_MAX > INT32_MAX
#   define SCNiFAST16 SCNi64
#  elif INT_FAST16_MAX == 0x7fff
#   define SCNiFAST16 "hi"
#  else
#   define SCNiFAST16 "i"
#  endif
# endif
# if !defined SCNoFAST16
#  undef SCNoFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define SCNoFAST16 SCNo64
#  elif UINT_FAST16_MAX == 0xffff
#   define SCNoFAST16 "ho"
#  else
#   define SCNoFAST16 "o"
#  endif
# endif
# if !defined SCNuFAST16
#  undef SCNuFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define SCNuFAST16 SCNu64
#  elif UINT_FAST16_MAX == 0xffff
#   define SCNuFAST16 "hu"
#  else
#   define SCNuFAST16 "u"
#  endif
# endif
# if !defined SCNxFAST16
#  undef SCNxFAST16
#  if UINT_FAST16_MAX > UINT32_MAX
#   define SCNxFAST16 SCNx64
#  elif UINT_FAST16_MAX == 0xffff
#   define SCNxFAST16 "hx"
#  else
#   define SCNxFAST16 "x"
#  endif
# endif
# if !defined SCNdFAST32
#  undef SCNdFAST32
#  if INT_FAST32_MAX > INT32_MAX
#   define SCNdFAST32 SCNd64
#  else
#   define SCNdFAST32 "d"
#  endif
# endif
# if !defined SCNiFAST32
#  undef SCNiFAST32
#  if INT_FAST32_MAX > INT32_MAX
#   define SCNiFAST32 SCNi64
#  else
#   define SCNiFAST32 "i"
#  endif
# endif
# if !defined SCNoFAST32
#  undef SCNoFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define SCNoFAST32 SCNo64
#  else
#   define SCNoFAST32 "o"
#  endif
# endif
# if !defined SCNuFAST32
#  undef SCNuFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define SCNuFAST32 SCNu64
#  else
#   define SCNuFAST32 "u"
#  endif
# endif
# if !defined SCNxFAST32
#  undef SCNxFAST32
#  if UINT_FAST32_MAX > UINT32_MAX
#   define SCNxFAST32 SCNx64
#  else
#   define SCNxFAST32 "x"
#  endif
# endif
# ifdef INT64_MAX
#  if !defined SCNdFAST64
#   undef SCNdFAST64
#   define SCNdFAST64 SCNd64
#  endif
#  if !defined SCNiFAST64
#   undef SCNiFAST64
#   define SCNiFAST64 SCNi64
#  endif
# endif
# ifdef UINT64_MAX
#  if !defined SCNoFAST64
#   undef SCNoFAST64
#   define SCNoFAST64 SCNo64
#  endif
#  if !defined SCNuFAST64
#   undef SCNuFAST64
#   define SCNuFAST64 SCNu64
#  endif
#  if !defined SCNxFAST64
#   undef SCNxFAST64
#   define SCNxFAST64 SCNx64
#  endif
# endif

# if !defined SCNdMAX
#  undef SCNdMAX
#  if INTMAX_MAX > INT32_MAX
#   define SCNdMAX SCNd64
#  else
#   define SCNdMAX "ld"
#  endif
# endif
# if !defined SCNiMAX
#  undef SCNiMAX
#  if INTMAX_MAX > INT32_MAX
#   define SCNiMAX SCNi64
#  else
#   define SCNiMAX "li"
#  endif
# endif
# if !defined SCNoMAX
#  undef SCNoMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define SCNoMAX SCNo64
#  else
#   define SCNoMAX "lo"
#  endif
# endif
# if !defined SCNuMAX
#  undef SCNuMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define SCNuMAX SCNu64
#  else
#   define SCNuMAX "lu"
#  endif
# endif
# if !defined SCNxMAX
#  undef SCNxMAX
#  if UINTMAX_MAX > UINT32_MAX
#   define SCNxMAX SCNx64
#  else
#   define SCNxMAX "lx"
#  endif
# endif

# if !defined SCNdPTR
#  undef SCNdPTR
#  ifdef INTPTR_MAX
#   define SCNdPTR "l" "d"
#  endif
# endif
# if !defined SCNiPTR
#  undef SCNiPTR
#  ifdef INTPTR_MAX
#   define SCNiPTR "l" "i"
#  endif
# endif
# if !defined SCNoPTR
#  undef SCNoPTR
#  ifdef UINTPTR_MAX
#   define SCNoPTR "l" "o"
#  endif
# endif
# if !defined SCNuPTR
#  undef SCNuPTR
#  ifdef UINTPTR_MAX
#   define SCNuPTR "l" "u"
#  endif
# endif
# if !defined SCNxPTR
#  undef SCNxPTR
#  ifdef UINTPTR_MAX
#   define SCNxPTR "l" "x"
#  endif
# endif

#endif

/* 7.8.2 Functions for greatest-width integer types */

#ifdef __cplusplus
extern "C" {
#endif

extern intmax_t imaxabs (intmax_t);

typedef struct { intmax_t quot; intmax_t rem; } imaxdiv_t;
extern imaxdiv_t imaxdiv (intmax_t, intmax_t);

intmax_t strtoimax (const char *, char **, int);
uintmax_t strtoumax (const char *, char **, int);


/* Don't bother defining or declaring wcstoimax and wcstoumax, since
   wide-character functions like this are hardly ever useful.  */

#ifdef __cplusplus
}
#endif

#endif /* INTTYPES_H */
